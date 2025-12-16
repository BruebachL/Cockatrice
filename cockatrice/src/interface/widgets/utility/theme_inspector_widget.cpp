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
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSet>
#include <QTabWidget>
#include <QVBoxLayout>

// ===========================================================
// Constructor
// ===========================================================
ThemeInspectorWidget::ThemeInspectorWidget(const QString &liveCssPath, QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Theme Inspector");
    setMinimumSize(1200, 700);

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);

    // ---------------- Widget Tree ----------------
    widgetTree = new QTreeWidget;
    widgetTree->setHeaderLabels({"Widget"});
    widgetTree->setUniformRowHeights(true);
    mainSplitter->addWidget(widgetTree);

    auto *bottom = new QSplitter(Qt::Horizontal);

    // ---------------- Left panel ----------------
    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);

    widgetInfo = new QPlainTextEdit;
    widgetInfo->setReadOnly(true);
    widgetInfo->setMinimumHeight(140);
    leftLayout->addWidget(widgetInfo);

    auto *suggestGroup = new QGroupBox("Ways to target this widget");
    auto *suggestLayout = new QVBoxLayout(suggestGroup);

    selectorSuggestionList = new QListWidget;
    suggestLayout->addWidget(selectorSuggestionList);
    leftLayout->addWidget(suggestGroup);

    bottom->addWidget(left);

    // ---------------- Right panel ----------------
    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);

    auto *tabs = new QTabWidget;

    ruleTree = new QTreeWidget;
    ruleTree->setHeaderLabels({"Target", "Line"});
    tabs->addTab(ruleTree, "Styles affecting this widget");

    allRulesTree = new QTreeWidget;
    allRulesTree->setHeaderLabels({"Target", "Line", "Matches"});
    tabs->addTab(allRulesTree, "All theme rules");

    rightLayout->addWidget(tabs);

    ruleEditor = new QPlainTextEdit;
    ruleEditor->setPlaceholderText("Edit CSS here");
    rightLayout->addWidget(ruleEditor);

    // ---------------- Property suggestions ----------------
    propertySuggestionList = new QListWidget;
    propertySuggestionList->setMaximumHeight(120);
    propertySuggestionList->setToolTip("Click to insert a CSS property");
    rightLayout->addWidget(propertySuggestionList);

    connect(propertySuggestionList, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
        if (!item)
            return;

        QString prop = item->text().split(" ").first(); // take property name only
        ruleEditor->insertPlainText(prop + ": ;\n");
    });

    auto *apply = new QToolButton;
    apply->setText("Save & Apply");
    connect(apply, &QToolButton::clicked, this, &ThemeInspectorWidget::applyRuleEdit);
    rightLayout->addWidget(apply);

    bottom->addWidget(right);
    mainSplitter->addWidget(bottom);

    // ---------------- Main layout ----------------
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(createToolbar());
    layout->addWidget(mainSplitter);
    setLayout(layout);

    // ---------------- Connections ----------------
    connect(widgetTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::updateForSelection);
    connect(ruleTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBody);
    connect(allRulesTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBodyFromAllRules);

    rebuildTree();
    setStylesheetPath(liveCssPath);
}

// ===========================================================
// Toolbar
// ===========================================================
QWidget *ThemeInspectorWidget::createToolbar()
{
    auto *bar = new QWidget;
    auto *l = new QHBoxLayout(bar);
    l->setContentsMargins(4, 2, 4, 2);

    auto *refresh = new QToolButton;
    refresh->setText("↻");
    connect(refresh, &QToolButton::clicked, this, &ThemeInspectorWidget::rebuildTree);
    l->addWidget(refresh);

    auto *expand = new QToolButton;
    expand->setText("⯆");
    connect(expand, &QToolButton::clicked, this, &ThemeInspectorWidget::expandAllItems);
    l->addWidget(expand);

    auto *collapse = new QToolButton;
    collapse->setText("⯈");
    connect(collapse, &QToolButton::clicked, this, &ThemeInspectorWidget::collapseAllItems);
    l->addWidget(collapse);

    // Add rule for selected widget
    auto *addWidgetRule = new QToolButton;
    addWidgetRule->setText("+Widget");
    connect(addWidgetRule, &QToolButton::clicked, [this]() {
        auto items = widgetTree->selectedItems();
        if (items.isEmpty())
            return;

        QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (!w)
            return;

        QtSelectors::SelectorGroups g = QtSelectors::possibleSelectorsGrouped(w);
        SelectorDialog dlg(g.types, g.objects, g.pseudos, this);

        if (dlg.exec() == QDialog::Accepted) {
            const QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForWidget(w, sel, "");
        }
    });
    l->addWidget(addWidgetRule);

    // Add global rule
    auto *addGlobalRule = new QToolButton;
    addGlobalRule->setText("+Global");
    connect(addGlobalRule, &QToolButton::clicked, [this]() {
        updateGlobalTypes();
        SelectorDialog dlg(globalTypes, globalObjects, globalPseudos, this);
        if (dlg.exec() == QDialog::Accepted) {
            const QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForAll(sel, "");
        }
    });
    l->addWidget(addGlobalRule);

    auto *delRule = new QToolButton;
    delRule->setText("−");
    connect(delRule, &QToolButton::clicked, this, &ThemeInspectorWidget::deleteSelectedRule);
    l->addWidget(delRule);

    l->addStretch();
    return bar;
}

// ===========================================================
// Global selector collection
// ===========================================================
void ThemeInspectorWidget::updateGlobalTypes()
{
    globalTypes.clear();
    globalObjects.clear();
    globalPseudos = {":hover", ":disabled", ":focus"};

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
    for (int i = 0; i < widgetTree->topLevelItemCount(); ++i)
        expandItemRecursive(widgetTree->topLevelItem(i));
}

void ThemeInspectorWidget::collapseAllItems()
{
    for (int i = 0; i < widgetTree->topLevelItemCount(); ++i)
        collapseItemRecursive(widgetTree->topLevelItem(i));
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

    for (QWidget *top : QApplication::topLevelWidgets()) {
        if (top == this)
            continue;
        if (!qobject_cast<QMainWindow *>(top))
            continue;

        auto *root = new QTreeWidgetItem(widgetTree, {top->metaObject()->className()});
        root->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(top)));
        addChildrenToItem(top, root);
    }

    widgetTree->expandToDepth(1);
    rebuildAllRulesTree();
}

void ThemeInspectorWidget::addChildrenToItem(QWidget *w, QTreeWidgetItem *parent)
{
    for (QObject *c : w->children()) {
        if (auto *cw = qobject_cast<QWidget *>(c)) {
            auto *it = new QTreeWidgetItem(
                parent, {cw->objectName().isEmpty() ? cw->metaObject()->className() : cw->objectName()});
            it->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(cw)));
            addChildrenToItem(cw, it);
        }
    }
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

    auto items = widgetTree->selectedItems();
    if (items.isEmpty())
        return;

    QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
    if (!w)
        return;

    widgetInfo->setPlainText(widgetSummary(w));

    auto suggestions = selectorSuggestions(w);
    for (const auto &s : suggestions)
        selectorSuggestionList->addItem(QString("%1\n→ %2").arg(s.selector, s.explanation));

    propertySuggestionList->clear();

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

    // Convert to sorted list and add to the list widget
    QStringList sortedProps = QStringList(suggestedProps.begin(), suggestedProps.end());
    std::sort(sortedProps.begin(), sortedProps.end());

    for (const QString &prop : sortedProps)
        propertySuggestionList->addItem(prop);

    updateRuleMatches(w);
}

QString ThemeInspectorWidget::widgetSummary(QWidget *w) const
{
    return QString("Widget: %1\nObject name: %2\nPath: %3\nVisible: %4\nEnabled: %5")
        .arg(w->metaObject()->className())
        .arg(w->objectName())
        .arg(widgetPath(w))
        .arg(w->isVisible() ? "yes" : "no")
        .arg(w->isEnabled() ? "yes" : "no");
}

QString ThemeInspectorWidget::widgetPath(QWidget *w) const
{
    QStringList parts;
    while (w) {
        parts.prepend(w->objectName().isEmpty() ? w->metaObject()->className() : w->objectName());
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

    if (!w->objectName().isEmpty())
        out.push_back({"#" + w->objectName(), "Only this exact widget", 100});

    out.push_back({type, QString("All widgets of type %1").arg(type), 60});

    if (!w->objectName().isEmpty())
        out.push_back({type + "#" + w->objectName(), QString("Widget %1 of type %2").arg(w->objectName(), type), 90});

    // --- Only include valid pseudos ---
    const auto validPseudos = QtSelectors::WIDGET_VALID_PSEUDOS.value(type);
    for (const auto &p : QtSelectors::PSEUDO_STATES) {
        if (validPseudos.contains(p.first))
            out.push_back({type + p.first, p.second, 50});
    }

    // --- Only include valid subcontrols ---
    const auto validSubs = QtSelectors::WIDGET_VALID_SUBCONTROLS.value(type);
    for (const auto &s : QtSelectors::SUBCONTROLS) {
        if (validSubs.contains(s.first))
            out.push_back({type + s.first, s.second, 30});
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
        connect(&watcher, &QFileSystemWatcher::fileChanged, this, &ThemeInspectorWidget::reloadStylesheet);
        reloadStylesheet();
    }
}

void ThemeInspectorWidget::reloadStylesheet()
{
    QFile f(stylesheetPath);
    if (!f.open(QIODevice::ReadOnly))
        return;

    stylesheetText = QString::fromUtf8(f.readAll());
    parseStylesheet();
    rebuildAllRulesTree();

    qApp->setStyleSheet({});
    qApp->setStyleSheet(stylesheetText);
}

void ThemeInspectorWidget::parseStylesheet()
{
    rules.clear();

    QString text = stylesheetText;
    text.remove(QRegularExpression(R"(/\*[\s\S]*?\*/)"));

    QRegularExpression re(R"(([^\{]+)\{([^\}]*)\})");
    int line = 1;

    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        for (QString sel : m.captured(1).split(',', Qt::SkipEmptyParts))
            rules.push_back({sel.trimmed(), m.captured(2).trimmed(), line});
        line += m.captured(0).count('\n');
    }
}

// ===========================================================
// Rule logic
// ===========================================================
void ThemeInspectorWidget::updateRuleMatches(QWidget *w)
{
    ruleTree->clear();
    for (int i = 0; i < rules.size(); ++i) {
        if (selectorMatches(w, rules[i].selector)) {
            auto *it = new QTreeWidgetItem(ruleTree);
            it->setText(0, rules[i].selector);
            it->setText(1, QString::number(rules[i].line));
            it->setData(0, Qt::UserRole, i);
        }
    }
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

    QRegularExpression re(R"(^(?<type>\w+)?(#(?<obj>[\w-]+))?(?::(?<pseudo>[\w-]+))?(?:::(?<sub>[\w-]+))?$)");
    QRegularExpressionMatch m = re.match(s);
    if (!m.hasMatch())
        return false;

    QString selType = m.captured("type");
    QString selObj = m.captured("obj");
    QString selPseudo = m.captured("pseudo");
    QString selSub = m.captured("sub");

    // Match type
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
    if (!items.isEmpty())
        ruleEditor->setPlainText(rules[items.first()->data(0, Qt::UserRole).toInt()].body);
}

void ThemeInspectorWidget::showRuleBodyFromAllRules()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty())
        return;

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    ruleEditor->setPlainText(rules[idx].body);
    highlightMatchingWidget(rules[idx].selector);
}

void ThemeInspectorWidget::applyRuleEdit()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty())
        return;

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    rules[idx].body = ruleEditor->toPlainText();

    QString out;
    for (const auto &r : rules)
        out += r.selector + " {\n" + r.body + "\n}\n\n";

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(out.toUtf8());

    rebuildAllRulesTree();
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
        for (QWidget *w : widgets)
            if (selectorMatches(w, rules[i].selector))
                ++count;

        auto *it = new QTreeWidgetItem(allRulesTree);
        it->setText(0, rules[i].selector);
        it->setText(1, QString::number(rules[i].line));
        it->setText(2, QString::number(count));
        it->setData(0, Qt::UserRole, i);
    }
}

void ThemeInspectorWidget::collectAllWidgetsRecursive(QWidget *w, QList<QWidget *> &out) const
{
    out.append(w);
    for (QObject *c : w->children())
        if (auto *cw = qobject_cast<QWidget *>(c))
            collectAllWidgetsRecursive(cw, out);
}

// ===========================================================
// Highlight
// ===========================================================
void ThemeInspectorWidget::highlightMatchingWidget(const QString &selector)
{
    if (selector.trimmed() == "QWidget")
        return;

    widgetTree->clearSelection();

    for (int i = 0; i < widgetTree->topLevelItemCount(); ++i) {
        auto *item = widgetTree->topLevelItem(i);
        QWidget *w = qobject_cast<QWidget *>(item->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w && selectorMatches(w, selector)) {
            item->setSelected(true);
            widgetTree->scrollToItem(item);
            break;
        }
    }
}

// ===========================================================
// Rule creation / deletion
// ===========================================================
void ThemeInspectorWidget::addRuleForWidget(QWidget *, const QString &selector, const QString &body)
{
    rules.push_back({selector, body, rules.isEmpty() ? 1 : rules.back().line + 1});
    rebuildTree();
}

void ThemeInspectorWidget::addRuleForAll(const QString &selector, const QString &body)
{
    rules.push_back({selector, body, rules.isEmpty() ? 1 : rules.back().line + 1});
    rebuildAllRulesTree();
}

void ThemeInspectorWidget::deleteSelectedRule()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty())
        return;

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx < 0 || idx >= rules.size())
        return;

    rules.removeAt(idx);
    rebuildAllRulesTree();
    ruleEditor->clear();
}