#include "theme_inspector_widget.h"

#include "selector_dialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSet>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

// ===========================================================
// Constructor
// ===========================================================
ThemeInspectorWidget::ThemeInspectorWidget(const QString &liveCssPath, QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Theme Inspector");
    setMinimumSize(1200, 700);

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->setObjectName("mainSplitter");

    // ---------------- Widget Tree ----------------
    widgetTree = new QTreeWidget;
    widgetTree->setHeaderLabels({"Widget"});
    widgetTree->setUniformRowHeights(true);
    widgetTree->setSelectionMode(QAbstractItemView::SingleSelection);
    mainSplitter->addWidget(widgetTree);

    auto *bottom = new QSplitter(Qt::Horizontal);
    bottom->setObjectName("bottomSplitter");

    // ---------------- Left panel ----------------
    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(4, 4, 4, 4);

    widgetInfo = new QPlainTextEdit;
    widgetInfo->setReadOnly(true);
    widgetInfo->setMinimumHeight(120);
    widgetInfo->setMaximumHeight(180);
    widgetInfo->setPlaceholderText("Select a widget to see its details...");
    leftLayout->addWidget(widgetInfo);

    auto *suggestGroup = new QGroupBox("Ways to target this widget");
    auto *suggestLayout = new QVBoxLayout(suggestGroup);
    suggestLayout->setContentsMargins(4, 8, 4, 4);

    selectorSuggestionList = new QListWidget;
    selectorSuggestionList->setToolTip("Double-click to create a new rule with this selector");
    suggestLayout->addWidget(selectorSuggestionList);
    leftLayout->addWidget(suggestGroup);

    bottom->addWidget(left);

    // ---------------- Right panel ----------------
    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(4, 4, 4, 4);

    auto *tabs = new QTabWidget;

    ruleTree = new QTreeWidget;
    ruleTree->setHeaderLabels({"Selector", "Line"});
    ruleTree->setColumnWidth(0, 300);
    ruleTree->setSelectionMode(QAbstractItemView::SingleSelection);
    ruleTree->setToolTip("Rules that apply to the selected widget");
    tabs->addTab(ruleTree, "Affecting Rules");

    allRulesTree = new QTreeWidget;
    allRulesTree->setHeaderLabels({"Selector", "Line", "Matches"});
    allRulesTree->setColumnWidth(0, 300);
    allRulesTree->setSortingEnabled(true);
    allRulesTree->setSelectionMode(QAbstractItemView::SingleSelection);
    allRulesTree->setToolTip("All rules in the stylesheet");
    tabs->addTab(allRulesTree, "All Rules");

    rightLayout->addWidget(tabs);

    // Rule editor section
    auto *editorLabel = new QLabel("CSS Rule Editor:");
    editorLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    rightLayout->addWidget(editorLabel);

    ruleEditor = new QPlainTextEdit;
    ruleEditor->setPlaceholderText("Select a rule to edit its CSS properties...");
    ruleEditor->setMinimumHeight(150);
    rightLayout->addWidget(ruleEditor);

    // ---------------- Property suggestions ----------------
    auto *propLabel = new QLabel("Common CSS Properties (click to insert):");
    propLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    rightLayout->addWidget(propLabel);

    propertySuggestionList = new QListWidget;
    propertySuggestionList->setMaximumHeight(100);
    propertySuggestionList->setToolTip("Click to insert a CSS property into the editor");
    propertySuggestionList->setSelectionMode(QAbstractItemView::SingleSelection);
    rightLayout->addWidget(propertySuggestionList);

    connect(propertySuggestionList, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
        if (!item || ruleEditor->toPlainText().isEmpty())
            return;

        QString prop = item->text().split(" ").first();

        // Insert at cursor position
        QTextCursor cursor = ruleEditor->textCursor();

        // Add proper formatting
        QString insertion = QString("    %1: ;\n").arg(prop);
        cursor.insertText(insertion);

        // Move cursor back to after the colon
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
        ruleEditor->setTextCursor(cursor);
        ruleEditor->setFocus();
    });

    auto *buttonLayout = new QHBoxLayout;

    auto *apply = new QPushButton("Save & Apply");
    apply->setToolTip("Save changes to stylesheet and reload");
    connect(apply, &QPushButton::clicked, this, &ThemeInspectorWidget::applyRuleEdit);
    buttonLayout->addWidget(apply);

    auto *revert = new QPushButton("Revert");
    revert->setToolTip("Discard changes and reload from file");
    connect(revert, &QPushButton::clicked, this, &ThemeInspectorWidget::reloadStylesheet);
    buttonLayout->addWidget(revert);

    buttonLayout->addStretch();
    rightLayout->addLayout(buttonLayout);

    bottom->addWidget(right);

    // Set splitter ratios
    bottom->setStretchFactor(0, 1);
    bottom->setStretchFactor(1, 2);

    mainSplitter->addWidget(bottom);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 2);

    // ---------------- Main layout ----------------
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(createToolbar());
    layout->addWidget(mainSplitter);
    setLayout(layout);

    // ---------------- Connections ----------------
    connect(widgetTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::updateForSelection);
    connect(ruleTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBody);
    connect(allRulesTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBodyFromAllRules);

    // Double-click on selector suggestion creates a rule
    connect(selectorSuggestionList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        if (!item)
            return;

        auto items = widgetTree->selectedItems();
        if (items.isEmpty())
            return;

        QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (!w)
            return;

        QString fullText = item->text();
        QString selector = fullText.split("\n").first();

        addRuleForWidget(w, selector, "    /* Add your styles here */");
    });

    rebuildTree();
    setStylesheetPath(liveCssPath);
}

// ===========================================================
// Toolbar
// ===========================================================
QWidget *ThemeInspectorWidget::createToolbar()
{
    auto *bar = new QWidget;
    bar->setObjectName("toolbar");
    auto *l = new QHBoxLayout(bar);
    l->setContentsMargins(6, 4, 6, 4);
    l->setSpacing(4);

    auto *refresh = new QToolButton;
    refresh->setText("⟳ Refresh");
    refresh->setToolTip("Refresh widget tree");
    connect(refresh, &QToolButton::clicked, this, &ThemeInspectorWidget::rebuildTree);
    l->addWidget(refresh);

    auto *expand = new QToolButton;
    expand->setText("⯆ Expand All");
    expand->setToolTip("Expand all items in tree");
    connect(expand, &QToolButton::clicked, this, &ThemeInspectorWidget::expandAllItems);
    l->addWidget(expand);

    auto *collapse = new QToolButton;
    collapse->setText("⯈ Collapse All");
    collapse->setToolTip("Collapse all items in tree");
    connect(collapse, &QToolButton::clicked, this, &ThemeInspectorWidget::collapseAllItems);
    l->addWidget(collapse);

    l->addWidget(createSeparator());

    // Add rule for selected widget
    auto *addWidgetRule = new QToolButton;
    addWidgetRule->setText("+ Widget Rule");
    addWidgetRule->setToolTip("Add a new rule for the selected widget");
    connect(addWidgetRule, &QToolButton::clicked, [this]() {
        auto items = widgetTree->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::information(this, "No Selection", "Please select a widget first.");
            return;
        }

        QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (!w) {
            QMessageBox::warning(this, "Invalid Widget", "Selected item is not a valid widget.");
            return;
        }

        QtSelectors::SelectorGroups g = QtSelectors::possibleSelectorsGrouped(w);
        SelectorDialog dlg(g.types, g.objects, g.pseudos, this);

        if (dlg.exec() == QDialog::Accepted) {
            const QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForWidget(w, sel, "    /* Widget-specific styles */");
        }
    });
    l->addWidget(addWidgetRule);

    // Add global rule
    auto *addGlobalRule = new QToolButton;
    addGlobalRule->setText("+ Global Rule");
    addGlobalRule->setToolTip("Add a new global rule");
    connect(addGlobalRule, &QToolButton::clicked, [this]() {
        updateGlobalTypes();
        SelectorDialog dlg(globalTypes, globalObjects, globalPseudos, this);
        if (dlg.exec() == QDialog::Accepted) {
            const QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForAll(sel, "    /* Global styles */");
        }
    });
    l->addWidget(addGlobalRule);

    auto *delRule = new QToolButton;
    delRule->setText("− Delete Rule");
    delRule->setToolTip("Delete the selected rule");
    connect(delRule, &QToolButton::clicked, this, &ThemeInspectorWidget::deleteSelectedRule);
    l->addWidget(delRule);

    l->addStretch();

    // Status label
    statusLabel = new QLabel;
    statusLabel->setStyleSheet("color: gray;");
    l->addWidget(statusLabel);

    return bar;
}

QFrame *ThemeInspectorWidget::createSeparator()
{
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    return sep;
}

// ===========================================================
// Global selector collection
// ===========================================================
void ThemeInspectorWidget::updateGlobalTypes()
{
    globalTypes.clear();
    globalObjects.clear();
    globalPseudos = {":hover", ":disabled", ":focus", ":enabled", ":checked", ":unchecked"};

    QList<QWidget *> widgets;
    for (QWidget *w : QApplication::topLevelWidgets())
        collectAllWidgetsRecursive(w, widgets);

    QSet<QString> types;
    QSet<QString> objects;

    for (QWidget *w : widgets) {
        types.insert(w->metaObject()->className());
        if (!w->objectName().isEmpty())
            objects.insert("#" + w->objectName());
    }

    globalTypes = QStringList(types.begin(), types.end());
    globalObjects = QStringList(objects.begin(), objects.end());

    std::sort(globalTypes.begin(), globalTypes.end());
    std::sort(globalObjects.begin(), globalObjects.end());
}

// ===========================================================
// Tree expand / collapse
// ===========================================================
void ThemeInspectorWidget::expandAllItems()
{
    widgetTree->expandAll();
}

void ThemeInspectorWidget::collapseAllItems()
{
    widgetTree->collapseAll();
}

void ThemeInspectorWidget::expandItemRecursive(QTreeWidgetItem *item)
{
    if (!item)
        return;
    widgetTree->expandItem(item);
    for (int i = 0; i < item->childCount(); ++i)
        expandItemRecursive(item->child(i));
}

void ThemeInspectorWidget::collapseItemRecursive(QTreeWidgetItem *item)
{
    if (!item)
        return;
    widgetTree->collapseItem(item);
    for (int i = 0; i < item->childCount(); ++i)
        collapseItemRecursive(item->child(i));
}

// ===========================================================
// Widget tree
// ===========================================================
void ThemeInspectorWidget::rebuildTree()
{
    widgetTree->clear();

    int totalWidgets = 0;
    for (QWidget *top : QApplication::topLevelWidgets()) {
        if (top == this)
            continue;
        if (!qobject_cast<QMainWindow *>(top) && !top->isWindow())
            continue;

        QString label = top->metaObject()->className();
        if (!top->objectName().isEmpty())
            label += QString(" (%1)").arg(top->objectName());

        auto *root = new QTreeWidgetItem(widgetTree, {label});
        root->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(top)));
        totalWidgets += addChildrenToItem(top, root) + 1;
    }

    widgetTree->expandToDepth(1);
    rebuildAllRulesTree();

    updateStatus(QString("Loaded %1 widgets").arg(totalWidgets));
}

int ThemeInspectorWidget::addChildrenToItem(QWidget *w, QTreeWidgetItem *parent)
{
    int count = 0;
    for (QObject *c : w->children()) {
        if (auto *cw = qobject_cast<QWidget *>(c)) {
            QString label = cw->metaObject()->className();
            if (!cw->objectName().isEmpty())
                label = cw->objectName() + QString(" [%1]").arg(cw->metaObject()->className());

            auto *it = new QTreeWidgetItem(parent, {label});
            it->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(cw)));
            count += addChildrenToItem(cw, it) + 1;
        }
    }
    return count;
}

// ===========================================================
// Selection & Summary
// ===========================================================
void ThemeInspectorWidget::updateForSelection()
{
    widgetInfo->clear();
    selectorSuggestionList->clear();
    ruleTree->clear();
    ruleEditor->clear();
    propertySuggestionList->clear();

    auto items = widgetTree->selectedItems();
    if (items.isEmpty())
        return;

    QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
    if (!w)
        return;

    widgetInfo->setPlainText(widgetSummary(w));

    auto suggestions = selectorSuggestions(w);
    for (const auto &s : suggestions) {
        QString displayText = QString("%1\n  → %2").arg(s.selector, s.explanation);
        auto *item = new QListWidgetItem(displayText);
        item->setToolTip("Double-click to create a new rule with this selector");
        selectorSuggestionList->addItem(item);
    }

    // Populate property suggestions based on widget type
    const QMetaObject *mo = w->metaObject();
    QSet<QString> suggestedProps;

    while (mo) {
        QString typeName = mo->className();
        for (auto it = QtSelectors::qtPropertyMap.constBegin(); it != QtSelectors::qtPropertyMap.constEnd(); ++it) {
            if (it.value().contains(typeName))
                suggestedProps.insert(it.key());
        }
        mo = mo->superClass();
    }

    // Add common properties for all widgets
    suggestedProps.insert("color");
    suggestedProps.insert("background-color");
    suggestedProps.insert("border");
    suggestedProps.insert("padding");
    suggestedProps.insert("margin");

    QStringList sortedProps = QStringList(suggestedProps.begin(), suggestedProps.end());
    std::sort(sortedProps.begin(), sortedProps.end());

    for (const QString &prop : sortedProps)
        propertySuggestionList->addItem(prop);

    updateRuleMatches(w);
}

QString ThemeInspectorWidget::widgetSummary(QWidget *w) const
{
    if (!w)
        return QString();

    QStringList info;
    info << QString("Class: %1").arg(w->metaObject()->className());
    info << QString("Object Name: %1").arg(w->objectName().isEmpty() ? "(none)" : w->objectName());
    info << QString("Path: %1").arg(widgetPath(w));
    info << QString("Visible: %1").arg(w->isVisible() ? "yes" : "no");
    info << QString("Enabled: %1").arg(w->isEnabled() ? "yes" : "no");
    info << QString("Geometry: %1x%2 at (%3, %4)").arg(w->width()).arg(w->height()).arg(w->x()).arg(w->y());

    return info.join("\n");
}

QString ThemeInspectorWidget::widgetPath(QWidget *w) const
{
    QStringList parts;
    while (w) {
        QString part = w->objectName().isEmpty() ? w->metaObject()->className() : w->objectName();
        parts.prepend(part);
        w = w->parentWidget();
    }
    return parts.join(" > ");
}

QVector<QtSelectors::SelectorSuggestion> ThemeInspectorWidget::selectorSuggestions(QWidget *w) const
{
    QVector<QtSelectors::SelectorSuggestion> out;
    if (!w)
        return out;

    QString type = w->metaObject()->className();

    // Object name selectors (highest specificity)
    if (!w->objectName().isEmpty()) {
        out.push_back({"#" + w->objectName(), "Targets only this widget by ID", 100});
        out.push_back(
            {type + "#" + w->objectName(), QString("Type-qualified ID selector for %1").arg(w->objectName()), 95});
    }

    // Type selector
    out.push_back({type, QString("All %1 widgets").arg(type), 80});

    // Only include valid pseudos
    const auto validPseudos = QtSelectors::WIDGET_VALID_PSEUDOS.value(type);
    if (!validPseudos.isEmpty()) {
        for (const auto &p : QtSelectors::PSEUDO_STATES) {
            if (validPseudos.contains(p.first))
                out.push_back({type + p.first, QString("%1: %2").arg(type, p.second), 60});
        }
    }

    // Only include valid subcontrols
    const auto validSubs = QtSelectors::WIDGET_VALID_SUBCONTROLS.value(type);
    if (!validSubs.isEmpty()) {
        for (const auto &s : QtSelectors::SUBCONTROLS) {
            if (validSubs.contains(s.first))
                out.push_back({type + s.first, QString("%1: %2").arg(type, s.second), 40});
        }
    }

    std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) { return a.confidence > b.confidence; });

    return out;
}

// ===========================================================
// Stylesheet
// ===========================================================
void ThemeInspectorWidget::setStylesheetPath(const QString &path)
{
    watcher.removePaths(watcher.files());
    stylesheetPath = path;

    if (!path.isEmpty()) {
        watcher.addPath(path);
        connect(&watcher, &QFileSystemWatcher::fileChanged, this, &ThemeInspectorWidget::reloadStylesheet,
                Qt::UniqueConnection);
        reloadStylesheet();
    }
}

void ThemeInspectorWidget::reloadStylesheet()
{
    if (stylesheetPath.isEmpty())
        return;

    QFile f(stylesheetPath);
    if (!f.open(QIODevice::ReadOnly)) {
        updateStatus(QString("Error: Could not open %1").arg(stylesheetPath), true);
        return;
    }

    stylesheetText = QString::fromUtf8(f.readAll());
    parseStylesheet();
    rebuildAllRulesTree();

    qApp->setStyleSheet({});
    qApp->setStyleSheet(stylesheetText);

    updateStatus(QString("Loaded %1 rules from stylesheet").arg(rules.size()));

    // Re-add the file to watcher if it was removed
    if (!watcher.files().contains(stylesheetPath))
        watcher.addPath(stylesheetPath);
}

void ThemeInspectorWidget::parseStylesheet()
{
    rules.clear();

    QString text = stylesheetText;
    // Remove C-style comments
    text.remove(QRegularExpression(R"(/\*[\s\S]*?\*/)"));

    QRegularExpression re(R"(([^\{]+)\{([^\}]*)\})");
    int line = 1;

    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        QString body = m.captured(2).trimmed();

        // Split multiple selectors
        for (QString sel : m.captured(1).split(',', Qt::SkipEmptyParts)) {
            rules.push_back({sel.trimmed(), body, line});
        }

        line += m.captured(0).count('\n');
    }
}

// ===========================================================
// Rule logic
// ===========================================================
void ThemeInspectorWidget::updateRuleMatches(QWidget *w)
{
    ruleTree->clear();
    if (!w)
        return;

    for (int i = 0; i < rules.size(); ++i) {
        if (selectorMatches(w, rules[i].selector)) {
            auto *it = new QTreeWidgetItem(ruleTree);
            it->setText(0, rules[i].selector);
            it->setText(1, QString::number(rules[i].line));
            it->setData(0, Qt::UserRole, i);
            it->setToolTip(0, rules[i].body);
        }
    }

    ruleTree->resizeColumnToContents(0);
}

// ===========================================================
// Enhanced selector matching
// ===========================================================
bool ThemeInspectorWidget::widgetHasPseudo(QWidget *w, const QString &pseudo) const
{
    if (pseudo.isEmpty())
        return true;

    if (pseudo == "hover")
        return w->underMouse();
    if (pseudo == "disabled")
        return !w->isEnabled();
    if (pseudo == "enabled")
        return w->isEnabled();
    if (pseudo == "focus")
        return w->hasFocus();
    if (pseudo == "checked") {
        if (auto cb = qobject_cast<QCheckBox *>(w))
            return cb->isChecked();
        if (auto rb = qobject_cast<QRadioButton *>(w))
            return rb->isChecked();
        if (auto btn = qobject_cast<QPushButton *>(w))
            return btn->isCheckable() && btn->isChecked();
    }
    if (pseudo == "unchecked") {
        if (auto cb = qobject_cast<QCheckBox *>(w))
            return !cb->isChecked();
        if (auto rb = qobject_cast<QRadioButton *>(w))
            return !rb->isChecked();
        if (auto btn = qobject_cast<QPushButton *>(w))
            return btn->isCheckable() && !btn->isChecked();
    }
    if (pseudo == "visible")
        return w->isVisible();
    if (pseudo == "hidden")
        return !w->isVisible();

    // For other pseudos, assume they match (conservative approach)
    return true;
}

bool ThemeInspectorWidget::widgetHasSubcontrol(QWidget *w, const QString &sub) const
{
    if (sub.isEmpty())
        return true;

    const QString type = w->metaObject()->className();
    if (!QtSelectors::WIDGET_VALID_SUBCONTROLS.contains(type))
        return false;
    return QtSelectors::WIDGET_VALID_SUBCONTROLS[type].contains("::" + sub);
}

bool ThemeInspectorWidget::selectorMatches(QWidget *w, const QString &s) const
{
    if (!w || s.isEmpty())
        return false;

    // Simple regex for basic selector matching
    QRegularExpression re(R"(^(?<type>[\w]+)?(#(?<obj>[\w-]+))?(?::(?<pseudo>[\w-]+))?(?:::(?<sub>[\w-]+))?$)");
    QRegularExpressionMatch m = re.match(s.trimmed());
    if (!m.hasMatch())
        return false;

    QString selType = m.captured("type");
    QString selObj = m.captured("obj");
    QString selPseudo = m.captured("pseudo");
    QString selSub = m.captured("sub");

    // Match type (check inheritance chain)
    if (!selType.isEmpty()) {
        const QMetaObject *mo = w->metaObject();
        bool found = false;
        while (mo) {
            if (selType == mo->className()) {
                found = true;
                break;
            }
            mo = mo->superClass();
        }
        if (!found)
            return false;
    }

    // Match object name
    if (!selObj.isEmpty() && w->objectName() != selObj)
        return false;

    // Match pseudo-state
    if (!widgetHasPseudo(w, selPseudo))
        return false;

    // Match subcontrol
    if (!widgetHasSubcontrol(w, selSub))
        return false;

    return true;
}

// ===========================================================
// Rule editing
// ===========================================================
void ThemeInspectorWidget::showRuleBody()
{
    auto items = ruleTree->selectedItems();
    if (items.isEmpty()) {
        ruleEditor->clear();
        return;
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && idx < rules.size())
        ruleEditor->setPlainText(rules[idx].body);
}

void ThemeInspectorWidget::showRuleBodyFromAllRules()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty()) {
        ruleEditor->clear();
        return;
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && idx < rules.size()) {
        ruleEditor->setPlainText(rules[idx].body);
        highlightMatchingWidget(rules[idx].selector);
    }
}

void ThemeInspectorWidget::applyRuleEdit()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty()) {
        // Try ruleTree instead
        items = ruleTree->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::information(this, "No Selection", "Please select a rule to edit.");
            return;
        }
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx < 0 || idx >= rules.size())
        return;

    rules[idx].body = ruleEditor->toPlainText();

    // Reconstruct stylesheet
    QString out;
    for (const auto &r : rules)
        out += QString("%1 {\n%2\n}\n\n").arg(r.selector, r.body);

    QFile f(stylesheetPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Save Error", QString("Could not write to %1").arg(stylesheetPath));
        return;
    }

    f.write(out.toUtf8());
    f.close();

    updateStatus("Stylesheet saved successfully");

    // Reload will happen automatically via file watcher
}

// ===========================================================
// All rules
// ===========================================================
void ThemeInspectorWidget::rebuildAllRulesTree()
{
    allRulesTree->clear();

    QList<QWidget *> widgets;
    for (QWidget *w : QApplication::topLevelWidgets())
        collectAllWidgetsRecursive(w, widgets);

    for (int i = 0; i < rules.size(); ++i) {
        int count = 0;
        for (QWidget *w : widgets) {
            if (selectorMatches(w, rules[i].selector))
                ++count;
        }

        auto *it = new QTreeWidgetItem(allRulesTree);
        it->setText(0, rules[i].selector);
        it->setText(1, QString::number(rules[i].line));
        it->setText(2, QString::number(count));
        it->setData(0, Qt::UserRole, i);
        it->setToolTip(0, rules[i].body);

        // Color-code by match count
        if (count == 0)
            it->setForeground(2, QBrush(Qt::gray));
        else if (count > 10)
            it->setForeground(2, QBrush(Qt::darkGreen));
    }

    allRulesTree->resizeColumnToContents(0);
    allRulesTree->resizeColumnToContents(1);
}

void ThemeInspectorWidget::collectAllWidgetsRecursive(QWidget *w, QList<QWidget *> &out) const
{
    if (!w)
        return;
    out.append(w);
    for (QObject *c : w->children()) {
        if (auto *cw = qobject_cast<QWidget *>(c))
            collectAllWidgetsRecursive(cw, out);
    }
}

// ===========================================================
// Highlight
// ===========================================================
void ThemeInspectorWidget::highlightMatchingWidget(const QString &selector)
{
    if (selector.trimmed().isEmpty() || selector.trimmed() == "QWidget")
        return;

    widgetTree->clearSelection();

    // Try to find and select a matching widget
    QTreeWidgetItemIterator it(widgetTree);
    while (*it) {
        QWidget *w = qobject_cast<QWidget *>((*it)->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w && selectorMatches(w, selector)) {
            (*it)->setSelected(true);
            widgetTree->scrollToItem(*it);
            updateForSelection();
            return;
        }
        ++it;
    }
}

// ===========================================================
// Rule creation / deletion
// ===========================================================
void ThemeInspectorWidget::addRuleForWidget(QWidget *, const QString &selector, const QString &body)
{
    if (selector.isEmpty())
        return;

    // Check if rule already exists
    for (const auto &r : rules) {
        if (r.selector == selector) {
            QMessageBox::information(this, "Rule Exists",
                                     QString("A rule for selector '%1' already exists.").arg(selector));
            return;
        }
    }

    rules.push_back({selector, body, rules.isEmpty() ? 1 : rules.back().line + 1});

    // Write to file immediately
    QString out;
    for (const auto &r : rules)
        out += QString("%1 {\n%2\n}\n\n").arg(r.selector, r.body);

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(out.toUtf8());
        f.close();
        updateStatus(QString("Added rule: %1").arg(selector));
    }

    rebuildAllRulesTree();
}

void ThemeInspectorWidget::addRuleForAll(const QString &selector, const QString &body)
{
    addRuleForWidget(nullptr, selector, body);
}

void ThemeInspectorWidget::deleteSelectedRule()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a rule from the 'All Rules' tab to delete.");
        return;
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx < 0 || idx >= rules.size())
        return;

    QString selector = rules[idx].selector;

    auto reply =
        QMessageBox::question(this, "Confirm Deletion", QString("Delete rule for selector '%1'?").arg(selector),
                              QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    rules.removeAt(idx);

    // Write to file
    QString out;
    for (const auto &r : rules)
        out += QString("%1 {\n%2\n}\n\n").arg(r.selector, r.body);

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(out.toUtf8());
        f.close();
        updateStatus(QString("Deleted rule: %1").arg(selector));
    }

    rebuildAllRulesTree();
    ruleEditor->clear();
}

// ===========================================================
// Status updates
// ===========================================================
void ThemeInspectorWidget::updateStatus(const QString &message, bool isError)
{
    if (statusLabel) {
        statusLabel->setText(message);
        statusLabel->setStyleSheet(isError ? "color: red;" : "color: green;");

        // Reset to gray after 3 seconds
        QTimer::singleShot(3000, this, [this]() {
            if (statusLabel)
                statusLabel->setStyleSheet("color: gray;");
        });
    }
}