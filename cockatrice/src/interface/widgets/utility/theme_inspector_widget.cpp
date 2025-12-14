#include "theme_inspector_widget.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMainWindow>
#include <QMetaObject>
#include <QPointer>
#include <QRegularExpression>
#include <QTabWidget>
#include <QVBoxLayout>

ThemeInspectorWidget::ThemeInspectorWidget(const QString &liveCssPath, QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Theme Inspector");
    setMinimumSize(1200, 650);

    // ===== Main vertical splitter =====
    auto *mainSplitter = new QSplitter(Qt::Vertical, this);

    widgetTree = new QTreeWidget;
    widgetTree->setHeaderLabels({"Widget", "objectName"});
    widgetTree->setUniformRowHeights(true);
    mainSplitter->addWidget(widgetTree);

    // ===== Bottom splitter =====
    auto *bottomSplitter = new QSplitter(Qt::Horizontal);

    // ---- Left: widget info + selectors ----
    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);

    widgetInfo = new QPlainTextEdit;
    widgetInfo->setReadOnly(true);
    leftLayout->addWidget(widgetInfo);

    selectorList = new QListWidget;
    leftLayout->addWidget(selectorList);

    bottomSplitter->addWidget(left);

    // ---- Right: rules + editor + all rules ----
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

    // ===== Connections =====
    connect(widgetTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::updateForSelection);
    connect(ruleTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBody);
    connect(allRulesTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBodyFromAllRules);

    rebuildTree();
    setStylesheetPath(liveCssPath);
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

    // update allRulesTree "Has Widget?" after rebuilding the object tree
    rebuildAllRulesTree();
}

void ThemeInspectorWidget::addChildrenToItem(QWidget *w, QTreeWidgetItem *parent)
{
    for (QObject *c : w->children()) {
        auto *cw = qobject_cast<QWidget *>(c);
        if (!cw)
            continue; // skip non-widgets

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
    selectorList->clear();
    ruleTree->clear();
    ruleEditor->clear();

    auto items = widgetTree->selectedItems();
    if (items.isEmpty())
        return;

    auto *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
    if (!w)
        return;

    widgetInfo->setPlainText(widgetSummary(w));

    for (const QString &s : possibleSelectors(w))
        selectorList->addItem(s);

    updateRuleMatches(w);
}

void ThemeInspectorWidget::updateRuleMatches(QWidget *w)
{
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
// Selector matching
// ===========================================================

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

    // highlight matching widgets
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

    // Rebuild stylesheet text
    QString out;
    for (const auto &r : rules)
        out += r.selector + " {\n" + r.body + "\n}\n\n";

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(out.toUtf8());

    // refresh global rules tree
    rebuildAllRulesTree();
}

// ===========================================================
// All Rules / Widget Diff
// ===========================================================

void ThemeInspectorWidget::rebuildAllRulesTree()
{
    allRulesTree->clear();

    for (int i = 0; i < rules.size(); ++i) {
        const auto &r = rules[i];

        auto *it = new QTreeWidgetItem(allRulesTree);
        it->setText(0, r.selector);
        it->setText(1, QString::number(r.line));
        it->setData(0, Qt::UserRole, i);

        // check if any widget matches
        bool hasWidget = false;
        QString objName;
        for (QWidget *top : QApplication::topLevelWidgets()) {
            if (matchesAnyWidget(top, r.selector)) {
                hasWidget = true;
                objName = top->objectName();
                break;
            }
        }

        it->setText(2, hasWidget ? "✓" : "✗");
        it->setText(3, objName);
    }

    allRulesTree->expandToDepth(1);
}

bool ThemeInspectorWidget::matchesAnyWidget(QWidget *w, const QString &selector) const
{
    if (!w)
        return false;

    if (selectorMatches(w, selector))
        return true;

    for (QObject *c : w->children()) {
        if (!c->isWidgetType())
            continue;
        if (auto *cw = qobject_cast<QWidget *>(c)) {
            if (matchesAnyWidget(cw, selector))
                return true;
        }
    }
    return false;
}

void ThemeInspectorWidget::highlightMatchingWidget(const QString &sel)
{
    if (!widgetTree)
        return;

    widgetTree->blockSignals(true);
    for (int i = 0; i < widgetTree->topLevelItemCount(); ++i)
        highlightWidgetRecursive(widgetTree->topLevelItem(i), sel);
    widgetTree->blockSignals(false);
}

bool ThemeInspectorWidget::highlightWidgetRecursive(QTreeWidgetItem *item, const QString &sel)
{
    if (!item)
        return false;

    QVariant var = item->data(0, Qt::UserRole);
    QWidget *w = nullptr;

    if (var.isValid() && var.canConvert<QPointer<QObject>>()) {
        QPointer<QObject> ptr = var.value<QPointer<QObject>>();
        w = ptr ? qobject_cast<QWidget *>(ptr.data()) : nullptr;
    }

    bool matches = w ? selectorMatches(w, sel) : false;

    for (int i = 0; i < item->childCount(); ++i)
        matches |= highlightWidgetRecursive(item->child(i), sel);

    if (w)
        item->setSelected(matches);

    return matches;
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

QStringList ThemeInspectorWidget::possibleSelectors(QWidget *w) const
{
    QStringList out;
    const QMetaObject *mo = w->metaObject();
    while (mo) {
        out << mo->className();
        mo = mo->superClass();
    }

    if (!w->objectName().isEmpty())
        out << "#" + w->objectName();

    for (const QByteArray &p : w->dynamicPropertyNames()) {
        out << QString("[%1=\"%2\"]").arg(p).arg(w->property(p).toString());
    }

    out << ":hover" << ":focus" << ":disabled";

    return out;
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
    l->addStretch();
    return bar;
}
