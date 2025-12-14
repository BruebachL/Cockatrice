#include "theme_inspector_widget.h"

#include "selector_dialog.h"

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMainWindow>
#include <QMetaObject>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

// ===========================================================
// Constructor
// ===========================================================
ThemeInspectorWidget::ThemeInspectorWidget(const QString &liveCssPath, QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Theme Inspector");
    setMinimumSize(1200, 650);

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);

    widgetTree = new QTreeWidget;
    widgetTree->setHeaderLabels({"Widget", "objectName"});
    widgetTree->setUniformRowHeights(true);
    mainSplitter->addWidget(widgetTree);

    auto *bottomSplitter = new QSplitter(Qt::Horizontal);

    // ===== Left panel: widget info + selectors =====
    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);

    widgetInfo = new QPlainTextEdit;
    widgetInfo->setReadOnly(true);
    leftLayout->addWidget(widgetInfo);

    auto *typesGroup = new QGroupBox("Types");
    typesList = new QListWidget;
    auto *typesLayout = new QVBoxLayout(typesGroup);
    typesLayout->addWidget(typesList);

    auto *objectsGroup = new QGroupBox("Object Selectors");
    objectsList = new QListWidget;
    auto *objLayout = new QVBoxLayout(objectsGroup);
    objLayout->addWidget(objectsList);

    auto *pseudosGroup = new QGroupBox("Pseudo-Selectors");
    pseudosList = new QListWidget;
    auto *pseudoLayout = new QVBoxLayout(pseudosGroup);
    pseudoLayout->addWidget(pseudosList);

    leftLayout->addWidget(typesGroup);
    leftLayout->addWidget(objectsGroup);
    leftLayout->addWidget(pseudosGroup);

    bottomSplitter->addWidget(left);

    // ===== Right panel: rules + editor + all rules =====
    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);

    auto *tabs = new QTabWidget;

    // --- Widget-specific rules ---
    ruleTree = new QTreeWidget;
    ruleTree->setHeaderLabels({"Selector", "Line"});
    ruleTree->setUniformRowHeights(true);

    QWidget *ruleTab = new QWidget;
    QVBoxLayout *ruleTabLayout = new QVBoxLayout(ruleTab);
    ruleTabLayout->addWidget(ruleTree);
    tabs->addTab(ruleTab, "Widget Rules");

    // --- All rules view ---
    allRulesTree = new QTreeWidget;
    allRulesTree->setHeaderLabels({"Selector", "Line", "Has Widget?", "ObjectName"});
    allRulesTree->setUniformRowHeights(true);

    QWidget *allRulesTab = new QWidget;
    QVBoxLayout *allRulesLayout = new QVBoxLayout(allRulesTab);
    allRulesLayout->addWidget(allRulesTree);
    tabs->addTab(allRulesTab, "All Rules");

    rightLayout->addWidget(tabs);

    // --- Rule editor ---
    ruleEditor = new QPlainTextEdit;
    ruleEditor->setPlaceholderText("Edit rule body here");
    rightLayout->addWidget(ruleEditor);

    auto *apply = new QToolButton;
    apply->setText("Apply");
    connect(apply, &QToolButton::clicked, this, &ThemeInspectorWidget::applyRuleEdit);
    rightLayout->addWidget(apply);

    bottomSplitter->addWidget(right);

    bottomSplitter->setStretchFactor(0, 2);
    bottomSplitter->setStretchFactor(1, 3);

    mainSplitter->addWidget(bottomSplitter);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(createToolbar());
    layout->addWidget(mainSplitter);
    setLayout(layout);

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

    // ---- Expand all ----
    auto *expandBtn = new QToolButton;
    expandBtn->setText("⯆");
    connect(expandBtn, &QToolButton::clicked, this, &ThemeInspectorWidget::expandAllItems);
    l->addWidget(expandBtn);

    // ---- Collapse all ----
    auto *collapseBtn = new QToolButton;
    collapseBtn->setText("⯈");
    connect(collapseBtn, &QToolButton::clicked, this, &ThemeInspectorWidget::collapseAllItems);
    l->addWidget(collapseBtn);

    // ---- Add Widget-specific Rule ----
    auto *addRuleForWidgetBtn = new QToolButton;
    addRuleForWidgetBtn->setText("+");
    connect(addRuleForWidgetBtn, &QToolButton::clicked, [this]() {
        auto items = widgetTree->selectedItems();
        if (items.isEmpty())
            return;

        QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (!w)
            return;

        SelectorGroups groups = possibleSelectorsGrouped(w);
        SelectorDialog dlg(groups.types, groups.objects, groups.pseudos, this);

        if (dlg.exec() == QDialog::Accepted) {
            QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForWidget(w, sel, "");
        }
    });
    l->addWidget(addRuleForWidgetBtn);

    // ---- Add Global Rule ----
    auto *addRuleForAllBtn = new QToolButton;
    addRuleForAllBtn->setText("+");
    connect(addRuleForAllBtn, &QToolButton::clicked, [this]() {
        updateGlobalTypes(); // refresh types
        QStringList globalTypes = globalTypesList;

        QStringList objList = collectObjectsWithNames();
        QSet<QString> objectsSet = QSet<QString>(objList.begin(), objList.end());
        QStringList globalObjects = QStringList(objectsSet.begin(), objectsSet.end());

        QStringList pseudos = {":hover", ":focus", ":disabled"};

        SelectorDialog dlg(globalTypes, globalObjects, pseudos, this);
        if (dlg.exec() == QDialog::Accepted) {
            QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForAll(sel, "");
        }
    });
    l->addWidget(addRuleForAllBtn);

    // ---- Delete rule ----
    auto *delRuleBtn = new QToolButton;
    delRuleBtn->setText("-");
    connect(delRuleBtn, &QToolButton::clicked, this, &ThemeInspectorWidget::deleteSelectedRule);
    l->addWidget(delRuleBtn);

    l->addStretch();
    return bar;
}

// ===========================================================
// Expand / Collapse helpers
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
// Collect Types / Object Names
// ===========================================================
void ThemeInspectorWidget::updateGlobalTypes()
{
    QSet<QString> types;
    for (QWidget *w : QApplication::topLevelWidgets())
        collectWidgetTypesRecursive(w, types);
    globalTypesList = QStringList(types.begin(), types.end());
    globalTypesList.sort();
}

void ThemeInspectorWidget::collectWidgetTypesRecursive(QWidget *w, QSet<QString> &types)
{
    if (!w)
        return;
    const QMetaObject *mo = w->metaObject();
    while (mo) {
        types.insert(mo->className());
        mo = mo->superClass();
    }

    for (QObject *c : w->children())
        if (auto *cw = qobject_cast<QWidget *>(c))
            collectWidgetTypesRecursive(cw, types);
}

QStringList ThemeInspectorWidget::collectObjectsWithNames() const
{
    QSet<QString> objectsSet;
    for (QWidget *w : QApplication::topLevelWidgets())
        collectObjectsWithNamesRecursive(w, objectsSet);

    QStringList out(objectsSet.begin(), objectsSet.end());
    out.sort();
    return out;
}

void ThemeInspectorWidget::collectObjectsWithNamesRecursive(QWidget *w, QSet<QString> &out) const
{
    if (!w)
        return;
    if (!w->objectName().isEmpty())
        out.insert("#" + w->objectName());

    for (QObject *c : w->children())
        if (auto *cw = qobject_cast<QWidget *>(c))
            collectObjectsWithNamesRecursive(cw, out);
}

// ===========================================================
// Stylesheet loading
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

        for (QString sel : m.captured(1).split(',', Qt::SkipEmptyParts)) {
            sel = sel.trimmed();
            if (sel.contains("::") || sel.startsWith("qproperty-"))
                continue;

            rules.push_back({sel, m.captured(2).trimmed(), line});
        }
        line += m.captured(0).count('\n');
    }
}

// ===========================================================
// Widget tree
// ===========================================================
void ThemeInspectorWidget::rebuildTree()
{
    if (!widgetTree)
        return;
    widgetTree->blockSignals(true);
    widgetTree->clear();

    for (QWidget *top : QApplication::topLevelWidgets()) {
        if (top == this)
            continue;
        auto *mw = qobject_cast<QMainWindow *>(top);
        if (!mw)
            continue;

        auto *root = new QTreeWidgetItem(
            widgetTree, {mw->objectName().isEmpty() ? mw->metaObject()->className() : mw->objectName()});
        root->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(mw)));
        addChildrenToItem(mw, root);
    }
    widgetTree->expandToDepth(1);

    widgetTree->blockSignals(false);

    rebuildAllRulesTree();
    updateGlobalTypes();
}

void ThemeInspectorWidget::addChildrenToItem(QWidget *w, QTreeWidgetItem *parent)
{
    for (QObject *c : w->children()) {
        auto *cw = qobject_cast<QWidget *>(c);
        if (!cw)
            continue;

        auto *it = new QTreeWidgetItem(parent,
                                       {cw->objectName().isEmpty() ? cw->metaObject()->className() : cw->objectName()});
        it->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(cw)));
        addChildrenToItem(cw, it);
    }
}

// ===========================================================
// Widget selection
// ===========================================================
void ThemeInspectorWidget::updateForSelection()
{
    widgetInfo->clear();
    typesList->clear();
    objectsList->clear();
    pseudosList->clear();
    ruleTree->clear();
    ruleEditor->clear();

    auto items = widgetTree->selectedItems();
    if (items.isEmpty())
        return;

    auto *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
    if (!w)
        return;

    widgetInfo->setPlainText(widgetSummary(w));

    SelectorGroups sg = possibleSelectorsGrouped(w);
    typesList->addItems(sg.types);
    objectsList->addItems(sg.objects);
    pseudosList->addItems(sg.pseudos);

    updateRuleMatches(w);
}

// ===========================================================
// Selector matching & rule updates
// ===========================================================
void ThemeInspectorWidget::updateRuleMatches(QWidget *w)
{
    ruleTree->clear();
    for (int i = 0; i < rules.size(); ++i) {
        if (selectorAppliesToWidgetIgnoringPseudo(w, rules[i].selector)) {
            auto *it = new QTreeWidgetItem(ruleTree);
            it->setText(0, rules[i].selector);
            it->setText(1, QString::number(rules[i].line));
            it->setData(0, Qt::UserRole, i);
        }
    }
}

bool ThemeInspectorWidget::selectorAppliesToWidgetIgnoringPseudo(QWidget *w, const QString &sel) const
{
    if (!w)
        return false;

    QString s = sel.trimmed();
    if (s.isEmpty())
        return false; // <--- prevent empty selector from recursing

    // Remove pseudo selectors
    s.remove(QRegularExpression(":(hover|focus|disabled)$"));

    if (s.contains('>')) {
        auto parts = s.split('>');
        if (parts.size() != 2)
            return false;

        if (!w->parentWidget())
            return false;

        return selectorAppliesToWidgetIgnoringPseudo(w->parentWidget(), parts[0].trimmed()) &&
               selectorAppliesToWidgetIgnoringPseudo(w, parts[1].trimmed());
    }

    if (s.startsWith('#'))
        return w->objectName() == s.mid(1);

    const QMetaObject *mo = w->metaObject();
    while (mo) {
        if (s == mo->className())
            return true;
        mo = mo->superClass();
    }
    return false;
}

bool ThemeInspectorWidget::selectorMatches(QWidget *w, const QString &sel) const
{
    QString s = sel.trimmed();

    if (s.contains('>')) {
        auto parts = s.split('>');
        if (parts.size() != 2)
            return false;
        return w->parentWidget() && selectorMatches(w->parentWidget(), parts[0].trimmed()) &&
               selectorMatches(w, parts[1].trimmed());
    }

    if (s.startsWith('#'))
        return w->objectName() == s.mid(1);

    if (s == ":disabled")
        return !w->isEnabled();
    if (s == ":focus")
        return w->hasFocus();
    if (s == ":hover")
        return w->underMouse();

    const QMetaObject *mo = w->metaObject();
    while (mo) {
        if (s == mo->className())
            return true;
        mo = mo->superClass();
    }
    return false;
}

// ===========================================================
// Rule editing
// ===========================================================
void ThemeInspectorWidget::showRuleBody()
{
    auto items = ruleTree->selectedItems();
    if (items.isEmpty())
        return;

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    ruleEditor->setPlainText(rules[idx].body);
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
    auto items = ruleTree->selectedItems();
    if (!items.isEmpty()) {
        int idx = items.first()->data(0, Qt::UserRole).toInt();
        rules[idx].body = ruleEditor->toPlainText();
    }

    items = allRulesTree->selectedItems();
    if (!items.isEmpty()) {
        int idx = items.first()->data(0, Qt::UserRole).toInt();
        rules[idx].body = ruleEditor->toPlainText();
    }

    QString out;
    for (const auto &r : rules)
        out += r.selector + " {\n" + r.body + "\n}\n\n";

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(out.toUtf8());

    rebuildAllRulesTree();
}

// ===========================================================
// Add / Delete Rules
// ===========================================================
void ThemeInspectorWidget::addRuleForWidget(QWidget *w, const QString &selector, const QString &body)
{
    if (!w)
        return;
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
    allRulesTree->clearSelection();
}

// ===========================================================
// All Rules / Widget Diff
// ===========================================================
void ThemeInspectorWidget::rebuildAllRulesTree()
{
    allRulesTree->clear();

    QList<QWidget *> allWidgets;
    for (QWidget *top : QApplication::topLevelWidgets())
        collectAllWidgetsRecursive(top, allWidgets);

    for (int i = 0; i < rules.size(); ++i) {
        const auto &r = rules[i];

        auto *it = new QTreeWidgetItem(allRulesTree);
        it->setText(0, r.selector);
        it->setText(1, QString::number(r.line));
        it->setData(0, Qt::UserRole, i);

        bool hasWidget = false;
        QString objName;

        for (QWidget *w : allWidgets) {
            if (selectorMatches(w, r.selector)) {
                hasWidget = true;
                objName = w->objectName();
                break;
            }
        }

        it->setText(2, hasWidget ? "✓" : "✗");
        it->setText(3, objName);
    }

    allRulesTree->expandToDepth(1);
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
// Highlight matching widgets (skip pure QWidget)
// ===========================================================
void ThemeInspectorWidget::highlightMatchingWidget(const QString &sel)
{
    if (sel.trimmed() == "QWidget")
        return;

    if (!widgetTree)
        return;

    widgetTree->blockSignals(true);
    widgetTree->clearSelection();

    bool simpleType = !sel.startsWith('#') && !sel.contains('>') && !sel.contains(':') && !sel.contains('[');
    QList<QTreeWidgetItem *> itemsToSelect;

    for (int i = 0; i < widgetTree->topLevelItemCount(); ++i)
        collectMatchingItems(widgetTree->topLevelItem(i), sel, simpleType, itemsToSelect);

    for (QTreeWidgetItem *it : itemsToSelect)
        it->setSelected(true);

    widgetTree->blockSignals(false);
}

void ThemeInspectorWidget::collectMatchingItems(QTreeWidgetItem *item,
                                                const QString &sel,
                                                bool simpleType,
                                                QList<QTreeWidgetItem *> &out) const
{
    if (!item)
        return;

    QVariant var = item->data(0, Qt::UserRole);
    QWidget *w = nullptr;

    if (var.isValid() && var.canConvert<QPointer<QObject>>()) {
        QPointer<QObject> ptr = var.value<QPointer<QObject>>();
        w = ptr ? qobject_cast<QWidget *>(ptr.data()) : nullptr;
    }

    if (!w)
        return;

    if (w->metaObject()->className() == QStringLiteral("QWidget") && sel.trimmed() == "QWidget")
        return;

    bool matches = false;

    if (simpleType) {
        const QMetaObject *mo = w->metaObject();
        while (mo) {
            if (sel == mo->className()) {
                matches = true;
                break;
            }
            mo = mo->superClass();
        }
    } else {
        matches = selectorMatches(w, sel);
    }

    if (matches)
        out.append(item);

    for (int i = 0; i < item->childCount(); ++i)
        collectMatchingItems(item->child(i), sel, simpleType, out);
}

// ===========================================================
// Widget info
// ===========================================================
QString ThemeInspectorWidget::widgetSummary(QWidget *w) const
{
    return QString("Class: %1\nobjectName: %2\nVisible: %3\nEnabled: %4")
        .arg(w->metaObject()->className())
        .arg(w->objectName())
        .arg(w->isVisible())
        .arg(w->isEnabled());
}

SelectorGroups ThemeInspectorWidget::possibleSelectorsGrouped(QWidget *w) const
{
    SelectorGroups out;
    QSet<QString> seenTypes;

    const QMetaObject *mo = w->metaObject();
    while (mo) {
        if (!seenTypes.contains(mo->className())) {
            out.types << mo->className();
            seenTypes.insert(mo->className());
        }
        mo = mo->superClass();
    }

    if (!w->objectName().isEmpty())
        out.objects << "#" + w->objectName();
    for (const QByteArray &p : w->dynamicPropertyNames())
        out.objects << QString("[%1=\"%2\"]").arg(p).arg(w->property(p).toString());

    out.pseudos << ":hover" << ":focus" << ":disabled";

    return out;
}