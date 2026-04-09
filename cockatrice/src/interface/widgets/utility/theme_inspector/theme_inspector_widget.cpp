#include "theme_inspector_widget.h"

#include "css_parser.h"
#include "selector_dialog.h"
#include "widget_inspector.h"

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
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
    setWindowTitle("Theme Inspector - Visual Stylesheet Editor");
    setMinimumSize(1400, 800);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(createToolbar());

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->setObjectName("mainSplitter");

    // ---------------- Widget Tree ----------------
    auto *treeGroup = new QGroupBox("Application Widget Tree");
    auto *treeLayout = new QVBoxLayout(treeGroup);
    treeLayout->setContentsMargins(4, 8, 4, 4);

    widgetTree = new QTreeWidget;
    widgetTree->setHeaderLabels({"Widget / Object Name", "Type"});
    widgetTree->setColumnWidth(0, 350);
    widgetTree->setUniformRowHeights(true);
    widgetTree->setSelectionMode(QAbstractItemView::SingleSelection);
    widgetTree->setAlternatingRowColors(true);
    treeLayout->addWidget(widgetTree);

    mainSplitter->addWidget(treeGroup);

    auto *bottom = new QSplitter(Qt::Horizontal);
    bottom->setObjectName("bottomSplitter");

    // ---------------- Left panel ----------------
    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(4, 4, 4, 4);

    auto *infoGroup = new QGroupBox("Widget Details");
    auto *infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->setContentsMargins(4, 8, 4, 4);

    widgetInfo = new QPlainTextEdit;
    widgetInfo->setReadOnly(true);
    widgetInfo->setMinimumHeight(120);
    widgetInfo->setMaximumHeight(180);
    widgetInfo->setPlaceholderText("Select a widget from the tree to see its properties...");
    infoLayout->addWidget(widgetInfo);
    leftLayout->addWidget(infoGroup);

    auto *suggestGroup = new QGroupBox("How to Target This Widget");
    auto *suggestLayout = new QVBoxLayout(suggestGroup);
    suggestLayout->setContentsMargins(4, 8, 4, 4);

    selectorSuggestionList = new QListWidget;
    selectorSuggestionList->setToolTip("Double-click any selector to create a new styling rule");
    suggestLayout->addWidget(selectorSuggestionList);
    leftLayout->addWidget(suggestGroup);

    bottom->addWidget(left);

    // ---------------- Right panel ----------------
    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(4, 4, 4, 4);

    auto *tabs = new QTabWidget;

    // Tab 1: Affecting Rules
    auto *rulesTab = new QWidget;
    auto *rulesTabLayout = new QVBoxLayout(rulesTab);
    rulesTabLayout->setContentsMargins(4, 4, 4, 4);

    ruleTree = new QTreeWidget;
    ruleTree->setHeaderLabels({"Selector", "Line"});
    ruleTree->setColumnWidth(0, 300);
    ruleTree->setSelectionMode(QAbstractItemView::SingleSelection);
    ruleTree->setToolTip("Rules affecting the selected widget (click to edit)");
    ruleTree->setAlternatingRowColors(true);
    rulesTabLayout->addWidget(ruleTree);

    tabs->addTab(rulesTab, "Affecting Rules");

    // Tab 2: All Rules
    auto *allRulesTab = new QWidget;
    auto *allRulesTabLayout = new QVBoxLayout(allRulesTab);
    allRulesTabLayout->setContentsMargins(4, 4, 4, 4);

    allRulesTree = new QTreeWidget;
    allRulesTree->setHeaderLabels({"Selector", "Line", "Widgets"});
    allRulesTree->setColumnWidth(0, 300);
    allRulesTree->setSortingEnabled(true);
    allRulesTree->setSelectionMode(QAbstractItemView::SingleSelection);
    allRulesTree->setToolTip("All CSS rules (click to edit)");
    allRulesTree->setAlternatingRowColors(true);
    allRulesTabLayout->addWidget(allRulesTree);

    tabs->addTab(allRulesTab, "All Rules");

    // Tab 3: Object Names Reference
    auto *objectNamesTab = new QWidget;
    auto *objectNamesLayout = new QVBoxLayout(objectNamesTab);
    objectNamesLayout->setContentsMargins(4, 4, 4, 4);

    objectNamesTree = new QTreeWidget;
    objectNamesTree->setHeaderLabels({"Object Name", "Widget Type", "Path"});
    objectNamesTree->setColumnWidth(0, 200);
    objectNamesTree->setColumnWidth(1, 150);
    objectNamesTree->setSelectionMode(QAbstractItemView::SingleSelection);
    objectNamesTree->setAlternatingRowColors(true);
    objectNamesTree->setSortingEnabled(true);
    objectNamesTree->setToolTip("Named widgets (use #objectName in CSS). Double-click to locate.");
    objectNamesLayout->addWidget(objectNamesTree);

    connect(objectNamesTree, &QTreeWidget::itemDoubleClicked, [this](QTreeWidgetItem *item) {
        if (!item)
            return;
        QWidget *w = qobject_cast<QWidget *>(item->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w) {
            QTreeWidgetItemIterator it(widgetTree);
            while (*it) {
                QWidget *tw = qobject_cast<QWidget *>((*it)->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
                if (tw == w) {
                    widgetTree->setCurrentItem(*it);
                    widgetTree->scrollToItem(*it);
                    updateForSelection();
                    return;
                }
                ++it;
            }
        }
    });

    tabs->addTab(objectNamesTab, "Object Names");

    // Tab 4: Icons Reference
    auto *iconsTab = new QWidget;
    auto *iconsLayout = new QVBoxLayout(iconsTab);
    iconsLayout->setContentsMargins(4, 4, 4, 4);

    iconsTree = new QTreeWidget;
    iconsTree->setHeaderLabels({"Icon Preview", "Widget", "Object Name"});
    iconsTree->setColumnWidth(0, 150);
    iconsTree->setColumnWidth(1, 150);
    iconsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    iconsTree->setAlternatingRowColors(true);
    iconsTree->setIconSize(QSize(24, 24));
    iconsTree->setToolTip("Icons in your app (use icon: url(...) in CSS)");
    iconsLayout->addWidget(iconsTree);

    tabs->addTab(iconsTab, "Icons");

    rightLayout->addWidget(tabs);

    // Rule editor
    auto *editorGroup = new QGroupBox("CSS Editor");
    auto *editorGroupLayout = new QVBoxLayout(editorGroup);
    editorGroupLayout->setContentsMargins(4, 8, 4, 4);

    ruleEditor = new QPlainTextEdit;
    ruleEditor->setPlaceholderText("Select a rule to edit its CSS properties...\n\nExample:\n    background-color: "
                                   "#2b2b2b;\n    color: white;\n    border: 1px solid #555;");
    ruleEditor->setMinimumHeight(150);
    editorGroupLayout->addWidget(ruleEditor);

    auto *propLabel = new QLabel("Quick Insert Properties:");
    propLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    editorGroupLayout->addWidget(propLabel);

    propertySuggestionList = new QListWidget;
    propertySuggestionList->setMaximumHeight(100);
    propertySuggestionList->setToolTip("Click to insert a CSS property template");
    propertySuggestionList->setSelectionMode(QAbstractItemView::SingleSelection);
    editorGroupLayout->addWidget(propertySuggestionList);

    connect(propertySuggestionList, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
        if (!item || ruleEditor->toPlainText().isEmpty())
            return;
        QString prop = item->text().split(" ").first();
        QTextCursor cursor = ruleEditor->textCursor();
        cursor.insertText(QString("    %1: /* value */;\n").arg(prop));
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 13);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 9);
        ruleEditor->setTextCursor(cursor);
        ruleEditor->setFocus();
    });

    auto *buttonLayout = new QHBoxLayout;

    auto *apply = new QPushButton("Save & Apply");
    apply->setToolTip("Save changes to stylesheet and reload the theme");
    connect(apply, &QPushButton::clicked, this, &ThemeInspectorWidget::applyRuleEdit);
    buttonLayout->addWidget(apply);

    auto *revert = new QPushButton("Revert");
    revert->setToolTip("Reload stylesheet from disk, discarding unsaved changes");
    connect(revert, &QPushButton::clicked, this, &ThemeInspectorWidget::reloadStylesheet);
    buttonLayout->addWidget(revert);

    buttonLayout->addStretch();
    editorGroupLayout->addLayout(buttonLayout);

    rightLayout->addWidget(editorGroup);

    bottom->addWidget(right);

    bottom->setStretchFactor(0, 1);
    bottom->setStretchFactor(1, 2);

    mainSplitter->addWidget(bottom);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 2);

    mainLayout->addWidget(mainSplitter);
    setLayout(mainLayout);

    // ---------------- Connections ----------------
    connect(widgetTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::updateForSelection);
    connect(ruleTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBody);
    connect(allRulesTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBodyFromAllRules);

    connect(selectorSuggestionList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        if (!item)
            return;
        auto items = widgetTree->selectedItems();
        if (items.isEmpty())
            return;
        QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (!w)
            return;
        QString selector = item->text().split("\n").first();
        addRuleForWidget(w, selector, "    /* Add your styles here */\n    background-color: ;\n    color: ;");
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
    bar->setStyleSheet("#toolbar { background-color: #f0f0f0; border-bottom: 2px solid #ccc; } "
                       "QToolButton { background-color: white; border: 1px solid #ccc; border-radius: 3px; padding: "
                       "6px 12px; color: black; } "
                       "QToolButton:hover { background-color: #e8e8e8; } "
                       "QToolButton:checked { background-color: #d0d0ff; border-color: #8888ff; } "
                       "QLabel { color: black; }");
    auto *l = new QHBoxLayout(bar);
    l->setContentsMargins(8, 8, 8, 8);
    l->setSpacing(12);

    hideInspectorCheckbox = new QToolButton;
    hideInspectorCheckbox->setText("Hide Inspector from Tree");
    hideInspectorCheckbox->setCheckable(true);
    hideInspectorCheckbox->setChecked(true);
    hideInspectorCheckbox->setToolTip("When checked, the Theme Inspector itself won't appear in the widget tree");
    connect(hideInspectorCheckbox, &QToolButton::toggled, this, &ThemeInspectorWidget::rebuildTree);
    l->addWidget(hideInspectorCheckbox);

    l->addWidget(createSeparator());

    auto *refresh = new QToolButton;
    refresh->setText("↻ Refresh");
    refresh->setToolTip("Refresh the widget tree to see any new widgets");
    connect(refresh, &QToolButton::clicked, this, &ThemeInspectorWidget::rebuildTree);
    l->addWidget(refresh);

    auto *expand = new QToolButton;
    expand->setText("Expand All");
    connect(expand, &QToolButton::clicked, widgetTree, &QTreeWidget::expandAll);
    l->addWidget(expand);

    auto *collapse = new QToolButton;
    collapse->setText("Collapse All");
    connect(collapse, &QToolButton::clicked, widgetTree, &QTreeWidget::collapseAll);
    l->addWidget(collapse);

    l->addWidget(createSeparator());

    auto *addWidgetRule = new QToolButton;
    addWidgetRule->setText("+ New Rule for Widget");
    addWidgetRule->setToolTip("Create a new CSS rule for the selected widget");
    connect(addWidgetRule, &QToolButton::clicked, [this]() {
        auto items = widgetTree->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::information(this, "No Widget Selected",
                                     "Please select a widget from the tree first.\n\n"
                                     "Tip: Click on any widget in the tree to select it.");
            return;
        }
        QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (!w) {
            QMessageBox::warning(this, "Invalid Selection", "The selected item is not a valid widget.");
            return;
        }
        QtSelectors::SelectorGroups g = QtSelectors::possibleSelectorsGrouped(w);
        SelectorDialog dlg(g.types, g.objects, g.pseudos, this);
        dlg.setWindowTitle("Choose Selector for Widget Rule");
        if (dlg.exec() == QDialog::Accepted) {
            const QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForWidget(w, sel, "    /* Widget-specific styles */\n    background-color: ;\n    color: ;");
        }
    });
    l->addWidget(addWidgetRule);

    auto *addGlobalRule = new QToolButton;
    addGlobalRule->setText("+ New Global Rule");
    addGlobalRule->setToolTip("Create a new CSS rule that applies globally");
    connect(addGlobalRule, &QToolButton::clicked, [this]() {
        updateGlobalTypes();
        SelectorDialog dlg(globalTypes, globalObjects, globalPseudos, this);
        dlg.setWindowTitle("Choose Selector for Global Rule");
        if (dlg.exec() == QDialog::Accepted) {
            const QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForAll(sel, "    /* Global styles */\n    background-color: ;\n    color: ;");
        }
    });
    l->addWidget(addGlobalRule);

    auto *delRule = new QToolButton;
    delRule->setText("Delete Rule");
    connect(delRule, &QToolButton::clicked, this, &ThemeInspectorWidget::deleteSelectedRule);
    l->addWidget(delRule);

    l->addWidget(createSeparator());

    auto *editSelector = new QToolButton;
    editSelector->setText("Edit Selector");
    connect(editSelector, &QToolButton::clicked, this, &ThemeInspectorWidget::editSelectedRuleSelector);
    l->addWidget(editSelector);

    auto *moveUp = new QToolButton;
    moveUp->setText("↑ Move Up");
    moveUp->setToolTip("Move rule earlier (lower priority in Qt CSS)");
    connect(moveUp, &QToolButton::clicked, [this]() { moveSelectedRule(-1); });
    l->addWidget(moveUp);

    auto *moveDown = new QToolButton;
    moveDown->setText("↓ Move Down");
    moveDown->setToolTip("Move rule later (HIGHER priority - will override earlier rules)");
    connect(moveDown, &QToolButton::clicked, [this]() { moveSelectedRule(1); });
    l->addWidget(moveDown);

    l->addStretch();

    statusLabel = new QLabel("Ready");
    statusLabel->setStyleSheet("color: #666; padding: 4px;");
    l->addWidget(statusLabel);

    return bar;
}

QFrame *ThemeInspectorWidget::createSeparator()
{
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setStyleSheet("margin: 0px 4px;");
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
        WidgetInspector::collectAllWidgetsRecursive(w, widgets);

    QSet<QString> types, objects;
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
// Widget tree
// ===========================================================
void ThemeInspectorWidget::rebuildTree()
{
    widgetTree->clear();

    int totalWidgets = 0;
    QList<QWidget *> topLevelWindows;

    for (QWidget *top : QApplication::topLevelWidgets()) {
        if (hideInspectorCheckbox && hideInspectorCheckbox->isChecked())
            if (top == this || WidgetInspector::isChildOf(top, this))
                continue;
        if (qobject_cast<QMainWindow *>(top) || top->isWindow())
            topLevelWindows.append(top);
    }

    std::sort(topLevelWindows.begin(), topLevelWindows.end(), [](QWidget *a, QWidget *b) {
        return (qobject_cast<QMainWindow *>(a) != nullptr) > (qobject_cast<QMainWindow *>(b) != nullptr);
    });

    for (QWidget *top : topLevelWindows) {
        auto *root = new QTreeWidgetItem(widgetTree);
        root->setText(0, top->objectName().isEmpty() ? "(unnamed)" : top->objectName());
        root->setText(1, top->metaObject()->className());
        root->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(top)));

        if (qobject_cast<QMainWindow *>(top)) {
            QFont f = root->font(0);
            f.setBold(true);
            root->setFont(0, f);
            root->setFont(1, f);
            root->setForeground(0, QBrush(QColor(50, 100, 200)));
            root->setForeground(1, QBrush(QColor(50, 100, 200)));
        }

        totalWidgets += addChildrenToItem(top, root) + 1;
    }

    widgetTree->expandToDepth(1);
    rebuildAllRulesTree();
    rebuildObjectNamesTree();
    rebuildIconsTree();

    updateStatus(QString("Loaded %1 widgets from application").arg(totalWidgets));
}

int ThemeInspectorWidget::addChildrenToItem(QWidget *w, QTreeWidgetItem *parent)
{
    int count = 0;
    for (QObject *c : w->children()) {
        if (auto *cw = qobject_cast<QWidget *>(c)) {
            auto *it = new QTreeWidgetItem(parent);
            it->setText(0, cw->objectName().isEmpty() ? "(unnamed)" : cw->objectName());
            it->setText(1, cw->metaObject()->className());
            it->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(cw)));

            if (!cw->objectName().isEmpty()) {
                QFont f = it->font(0);
                f.setBold(true);
                it->setFont(0, f);
                it->setForeground(0, QBrush(QColor(100, 150, 255)));
            }

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

    // Delegate to WidgetInspector
    widgetInfo->setPlainText(WidgetInspector::widgetSummary(w));

    for (const auto &s : WidgetInspector::selectorSuggestions(w)) {
        auto *item = new QListWidgetItem(QString("%1\n  ➜ %2").arg(s.selector, s.explanation));
        item->setToolTip("Double-click to create a new CSS rule with this selector");
        selectorSuggestionList->addItem(item);
    }

    // Property suggestions based on widget type
    QSet<QString> suggestedProps;
    const QMetaObject *mo = w->metaObject();
    while (mo) {
        for (auto it = QtSelectors::qtPropertyMap.constBegin(); it != QtSelectors::qtPropertyMap.constEnd(); ++it)
            if (it.value().contains(mo->className()))
                suggestedProps.insert(it.key());
        mo = mo->superClass();
    }
    // Essentials always present
    static const QStringList essentials = {"color",   "background-color", "border",    "border-radius",
                                           "padding", "margin",           "font-size", "font-weight"};
    for (const QString &p : essentials) {
        suggestedProps.insert(p);
    }

    QStringList sorted = QStringList(suggestedProps.begin(), suggestedProps.end());
    std::sort(sorted.begin(), sorted.end());

    for (const QString &prop : sorted) {
        QString hint;
        for (const auto &p : QtSelectors::QT_CSS_PROPERTIES) {
            if (p.first == prop) {
                hint = " - " + p.second.split(":").last().trimmed();
                break;
            }
        }
        propertySuggestionList->addItem(prop + hint);
    }

    updateRuleMatches(w);
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
        updateStatus("Error: Could not open stylesheet file", true);
        return;
    }

    stylesheetText = QString::fromUtf8(f.readAll());

    // Delegate parsing to CssParser
    m_parser.parse(stylesheetText);

    rebuildAllRulesTree();

    qApp->setStyleSheet({});
    qApp->setStyleSheet(stylesheetText);

    updateStatus(QString("✓ Loaded %1 CSS rules from stylesheet").arg(m_parser.rules().size()));

    if (!watcher.files().contains(stylesheetPath))
        watcher.addPath(stylesheetPath);
}

// ===========================================================
// Rule matching
// ===========================================================
void ThemeInspectorWidget::updateRuleMatches(QWidget *w)
{
    ruleTree->clear();
    if (!w)
        return;

    const auto &rules = m_parser.rules();
    QVector<int> matchingIndices;
    for (int i = 0; i < rules.size(); ++i)
        if (m_parser.selectorMatches(w, rules[i].selector))
            matchingIndices.push_back(i);

    for (int idx : matchingIndices) {
        auto *it = new QTreeWidgetItem(ruleTree);
        it->setText(0, rules[idx].selector);
        it->setText(1, QString::number(rules[idx].line));
        it->setData(0, Qt::UserRole, idx);
        it->setToolTip(0, "Click to edit this rule's properties\n\n" + rules[idx].body);

        if (idx == matchingIndices.last()) {
            QFont font = it->font(0);
            font.setBold(true);
            it->setFont(0, font);
            it->setFont(1, font);
            it->setForeground(0, QBrush(QColor(50, 150, 50)));
            it->setToolTip(0, "✓ THIS RULE WINS (appears last)\n\nClick to edit:\n" + rules[idx].body);
        }
    }

    ruleTree->resizeColumnToContents(0);

    const int matchCount = matchingIndices.size();
    if (matchCount == 0) {
        auto *hint = new QTreeWidgetItem(ruleTree);
        hint->setText(0, "No rules currently affect this widget");
        hint->setForeground(0, QBrush(Qt::gray));
        hint->setFlags(hint->flags() & ~Qt::ItemIsSelectable);
    } else if (matchCount > 1) {
        auto *hint = new QTreeWidgetItem(ruleTree);
        hint->setText(0, QString("⚠ %1 rules match - LAST ONE WINS in Qt CSS!").arg(matchCount));
        hint->setForeground(0, QBrush(QColor(200, 100, 0)));
        hint->setFlags(hint->flags() & ~Qt::ItemIsSelectable);
        ruleTree->insertTopLevelItem(0, hint);
    }
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
    const auto &rules = m_parser.rules();
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
    const auto &rules = m_parser.rules();
    if (idx >= 0 && idx < rules.size()) {
        ruleEditor->setPlainText(rules[idx].body);
        highlightMatchingWidget(rules[idx].selector);
    }
}

void ThemeInspectorWidget::applyRuleEdit()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty())
        items = ruleTree->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::information(this, "No Rule Selected",
                                 "Please select a CSS rule to edit.\n\n"
                                 "Tip: Click on any rule in the 'Active Rules' or 'All Rules' tab.");
        return;
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    auto &rules = m_parser.rules();
    if (idx < 0 || idx >= rules.size())
        return;

    rules[idx].body = ruleEditor->toPlainText();
    saveRulesToDisk();
    updateStatus("✓ Stylesheet saved and applied successfully");
}

// ===========================================================
// All rules tree
// ===========================================================
void ThemeInspectorWidget::rebuildAllRulesTree()
{
    allRulesTree->clear();

    QList<QWidget *> widgets;
    for (QWidget *w : QApplication::topLevelWidgets())
        WidgetInspector::collectAllWidgetsRecursive(w, widgets);

    const auto &rules = m_parser.rules();
    for (int i = 0; i < rules.size(); ++i) {
        int count = 0;
        for (QWidget *w : widgets)
            if (m_parser.selectorMatches(w, rules[i].selector))
                ++count;

        auto *it = new QTreeWidgetItem(allRulesTree);
        it->setText(0, rules[i].selector);
        it->setText(1, QString::number(rules[i].line));
        it->setText(2, QString::number(count));
        it->setData(0, Qt::UserRole, i);
        it->setToolTip(0, "Click to edit this rule\n\n" + rules[i].body);

        if (count == 0)
            it->setForeground(2, QBrush(Qt::gray));
        else if (count > 10)
            it->setForeground(2, QBrush(QColor(50, 150, 50)));
    }

    allRulesTree->resizeColumnToContents(0);
    allRulesTree->resizeColumnToContents(1);
}

// ===========================================================
// Object Names tree
// ===========================================================
void ThemeInspectorWidget::rebuildObjectNamesTree()
{
    objectNamesTree->clear();

    QList<QWidget *> widgets;
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (hideInspectorCheckbox && hideInspectorCheckbox->isChecked() &&
            (w == this || WidgetInspector::isChildOf(w, this)))
            continue;
        WidgetInspector::collectAllWidgetsRecursive(w, widgets);
    }

    QMap<QString, QList<QWidget *>> grouped;
    for (QWidget *w : widgets)
        if (!w->objectName().isEmpty())
            grouped[w->objectName()].append(w);

    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        const QString &name = it.key();
        const QList<QWidget *> &list = it.value();

        if (list.size() == 1) {
            QWidget *w = list.first();
            auto *item = new QTreeWidgetItem(objectNamesTree);
            item->setText(0, name);
            item->setText(1, w->metaObject()->className());
            item->setText(2, WidgetInspector::widgetPath(w));
            item->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(w)));
            item->setToolTip(1, QString("Use #%1 in CSS to target this widget").arg(name));
        } else {
            auto *parent = new QTreeWidgetItem(objectNamesTree);
            parent->setText(0, QString("%1 (%2 widgets)").arg(name).arg(list.size()));
            QFont f = parent->font(0);
            f.setBold(true);
            parent->setFont(0, f);
            parent->setForeground(0, QBrush(QColor(200, 100, 0)));
            parent->setToolTip(0, QString("⚠ Multiple widgets share #%1 - CSS will affect all of them!").arg(name));

            for (QWidget *w : list) {
                auto *child = new QTreeWidgetItem(parent);
                child->setText(1, w->metaObject()->className());
                child->setText(2, WidgetInspector::widgetPath(w));
                child->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(w)));
            }
        }
    }

    objectNamesTree->expandAll();
    objectNamesTree->resizeColumnToContents(0);
    objectNamesTree->resizeColumnToContents(1);
}

// ===========================================================
// Icons tree
// ===========================================================
void ThemeInspectorWidget::rebuildIconsTree()
{
    iconsTree->clear();

    QList<QWidget *> widgets;
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (hideInspectorCheckbox && hideInspectorCheckbox->isChecked() &&
            (w == this || WidgetInspector::isChildOf(w, this)))
            continue;
        WidgetInspector::collectAllWidgetsRecursive(w, widgets);
    }

    QSet<QString> seen;
    for (QWidget *w : widgets) {
        QIcon icon;
        QPixmap pixmap;
        QString iconSource;

        if (auto *btn = qobject_cast<QPushButton *>(w)) {
            icon = btn->icon();
            iconSource = "QPushButton";
        } else if (auto *tb = qobject_cast<QToolButton *>(w)) {
            icon = tb->icon();
            iconSource = "QToolButton";
        } else if (auto *cb = qobject_cast<QCheckBox *>(w)) {
            icon = cb->icon();
            iconSource = "QCheckBox";
        } else if (auto *rb = qobject_cast<QRadioButton *>(w)) {
            icon = rb->icon();
            iconSource = "QRadioButton";
        } else if (auto *lbl = qobject_cast<QLabel *>(w)) {
            pixmap = lbl->pixmap(Qt::ReturnByValue);
            if (!pixmap.isNull())
                iconSource = "QLabel (pixmap)";
        }

        if (icon.isNull() && pixmap.isNull())
            continue;

        QString key = QString("%1_%2_%3").arg(iconSource, w->objectName(), QString::number((qintptr)w));
        if (seen.contains(key))
            continue;
        seen.insert(key);

        auto *it = new QTreeWidgetItem(iconsTree);
        if (!icon.isNull()) {
            it->setIcon(0, icon);
            it->setText(0, "(icon)");
        } else {
            it->setIcon(0, QIcon(pixmap));
            it->setText(0, QString("(pixmap %1×%2)").arg(pixmap.width()).arg(pixmap.height()));
        }
        it->setText(1, iconSource);
        it->setText(2, w->objectName().isEmpty() ? "(unnamed)" : w->objectName());
        it->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(w)));

        QString cssEx = w->objectName().isEmpty()
                            ? QString("%1 { icon: url(:/path/to/icon.png); }").arg(iconSource)
                            : QString("#%1 { icon: url(:/path/to/icon.png); }").arg(w->objectName());
        it->setToolTip(0, "CSS Example:\n" + cssEx);
    }

    iconsTree->resizeColumnToContents(0);
    iconsTree->resizeColumnToContents(1);
}

// ===========================================================
// Highlight
// ===========================================================
void ThemeInspectorWidget::highlightMatchingWidget(const QString &selector)
{
    if (selector.trimmed().isEmpty() || selector.trimmed() == "QWidget")
        return;

    widgetTree->clearSelection();

    QTreeWidgetItemIterator it(widgetTree);
    while (*it) {
        QWidget *w = qobject_cast<QWidget *>((*it)->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w && m_parser.selectorMatches(w, selector)) {
            (*it)->setSelected(true);
            widgetTree->scrollToItem(*it);
            updateForSelection();
            return;
        }
        ++it;
    }
}

// ===========================================================
// Rule CRUD helpers
// ===========================================================

// Shared helper: write current rule list to disk and reload.
void ThemeInspectorWidget::saveRulesToDisk()
{
    QString out;
    for (const auto &r : m_parser.rules())
        out += QString("%1 {\n%2\n}\n\n").arg(r.selector, r.body);

    QFile f(stylesheetPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(
            this, "Save Error",
            QString("Could not write to stylesheet:\n%1\n\nCheck file permissions.").arg(stylesheetPath));
        return;
    }
    f.write(out.toUtf8());
    f.close();
    // File watcher triggers reloadStylesheet() automatically.
}

void ThemeInspectorWidget::refreshAfterRuleChange()
{
    rebuildAllRulesTree();
    auto widgetItems = widgetTree->selectedItems();
    if (!widgetItems.isEmpty()) {
        QWidget *w =
            qobject_cast<QWidget *>(widgetItems.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w)
            updateRuleMatches(w);
    }
}

void ThemeInspectorWidget::moveSelectedRule(int direction)
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty())
        items = ruleTree->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::information(this, "No Rule Selected", "Please select a CSS rule to move.");
        return;
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    auto &rules = m_parser.rules();
    if (idx < 0 || idx >= rules.size())
        return;

    int newIdx = idx + direction;
    if (newIdx < 0 || newIdx >= rules.size()) {
        updateStatus("Cannot move rule further in that direction", true);
        return;
    }

    std::swap(rules[idx], rules[newIdx]);
    saveRulesToDisk();
    updateStatus(QString("✓ Moved rule: %1").arg(rules[newIdx].selector));
    refreshAfterRuleChange();

    // Reselect the moved rule
    for (int i = 0; i < allRulesTree->topLevelItemCount(); ++i) {
        auto *item = allRulesTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toInt() == newIdx) {
            allRulesTree->setCurrentItem(item);
            break;
        }
    }
}

void ThemeInspectorWidget::editSelectedRuleSelector()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty())
        items = ruleTree->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::information(this, "No Rule Selected", "Please select a CSS rule to edit its selector.");
        return;
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    auto &rules = m_parser.rules();
    if (idx < 0 || idx >= rules.size())
        return;

    const QString oldSel = rules[idx].selector;
    bool ok;
    QString newSel =
        QInputDialog::getText(this, "Edit Selector", QString("Enter new selector:\n\nCurrent: %1").arg(oldSel),
                              QLineEdit::Normal, oldSel, &ok);

    if (!ok || newSel.trimmed().isEmpty() || newSel == oldSel)
        return;
    newSel = newSel.trimmed();

    for (int i = 0; i < rules.size(); ++i) {
        if (i != idx && rules[i].selector == newSel) {
            QMessageBox::warning(this, "Selector Exists",
                                 QString("A rule with selector '%1' already exists.").arg(newSel));
            return;
        }
    }

    rules[idx].selector = newSel;
    saveRulesToDisk();
    updateStatus(QString("✓ Updated selector: %1 → %2").arg(oldSel, newSel));
    refreshAfterRuleChange();
}

void ThemeInspectorWidget::addRuleForWidget(QWidget *, const QString &selector, const QString &body)
{
    if (selector.isEmpty())
        return;

    auto &rules = m_parser.rules();
    for (const auto &r : rules) {
        if (r.selector == selector) {
            QMessageBox::information(this, "Rule Already Exists",
                                     QString("A CSS rule for '%1' already exists.\n\n"
                                             "You can edit it in the 'All Rules' tab.")
                                         .arg(selector));
            return;
        }
    }

    rules.push_back({selector, body, rules.isEmpty() ? 1 : rules.back().line + 1});
    saveRulesToDisk();
    updateStatus(QString("✓ Created new rule: %1").arg(selector));
    refreshAfterRuleChange();
}

void ThemeInspectorWidget::addRuleForAll(const QString &selector, const QString &body)
{
    addRuleForWidget(nullptr, selector, body);
}

void ThemeInspectorWidget::deleteSelectedRule()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty())
        items = ruleTree->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::information(this, "No Rule Selected", "Please select a CSS rule to delete.");
        return;
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    auto &rules = m_parser.rules();
    if (idx < 0 || idx >= rules.size())
        return;

    const QString selector = rules[idx].selector;
    auto reply =
        QMessageBox::question(this, "Confirm Deletion",
                              QString("Delete this CSS rule?\n\nSelector: %1\n\nThis cannot be undone.").arg(selector),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    rules.removeAt(idx);
    saveRulesToDisk();
    updateStatus(QString("✓ Deleted rule: %1").arg(selector));
    refreshAfterRuleChange();
    ruleEditor->clear();
}

// ===========================================================
// Status
// ===========================================================
void ThemeInspectorWidget::updateStatus(const QString &message, bool isError)
{
    if (!statusLabel)
        return;
    statusLabel->setText(message);
    statusLabel->setStyleSheet(isError ? "color: #ff4444; padding: 4px; font-weight: bold;"
                                       : "color: #44ff44; padding: 4px;");
    QTimer::singleShot(4000, this, [this]() {
        if (statusLabel) {
            statusLabel->setText("Ready");
            statusLabel->setStyleSheet("color: gray; padding: 4px;");
        }
    });
}