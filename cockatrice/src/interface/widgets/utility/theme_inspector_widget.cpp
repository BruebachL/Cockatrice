#include "theme_inspector_widget.h"

#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMetaObject>
#include <QRegularExpression>
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

    // ---- Right: rules + editor ----
    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);

    ruleTree = new QTreeWidget;
    ruleTree->setHeaderLabels({"Selector", "Line"});
    ruleTree->setUniformRowHeights(true);
    rightLayout->addWidget(ruleTree);

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

    rebuildTree();
    setStylesheetPath(liveCssPath);
}

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

void ThemeInspectorWidget::rebuildTree()
{
    widgetTree->clear();

    for (QWidget *top : QApplication::topLevelWidgets()) {
        if (top == this)
            continue;
        auto *mw = qobject_cast<QMainWindow *>(top);
        if (!mw)
            continue;

        auto *root = new QTreeWidgetItem(
            widgetTree, {mw->objectName().isEmpty() ? mw->metaObject()->className() : mw->objectName()});
        root->setData(0, Qt::UserRole, QVariant::fromValue((QObject *)mw));
        addChildrenToItem(mw, root);
    }
    widgetTree->expandToDepth(1);
}

void ThemeInspectorWidget::addChildrenToItem(QWidget *w, QTreeWidgetItem *parent)
{
    for (QObject *c : w->children()) {
        if (auto *cw = qobject_cast<QWidget *>(c)) {
            auto *it = new QTreeWidgetItem(
                parent, {cw->objectName().isEmpty() ? cw->metaObject()->className() : cw->objectName()});
            it->setData(0, Qt::UserRole, QVariant::fromValue((QObject *)cw));
            addChildrenToItem(cw, it);
        }
    }
}

void ThemeInspectorWidget::updateForSelection()
{
    widgetInfo->clear();
    selectorList->clear();
    ruleTree->clear();
    ruleEditor->clear();

    auto items = widgetTree->selectedItems();
    if (items.isEmpty())
        return;

    auto *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QObject *>());
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

void ThemeInspectorWidget::showRuleBody()
{
    auto items = ruleTree->selectedItems();
    if (items.isEmpty())
        return;

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    ruleEditor->setPlainText(rules[idx].body);
}

void ThemeInspectorWidget::applyRuleEdit()
{
    auto items = ruleTree->selectedItems();
    if (items.isEmpty())
        return;

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    rules[idx].body = ruleEditor->toPlainText();

    // Rebuild stylesheet text
    QString out;
    for (const auto &r : rules)
        out += r.selector + " {\n" + r.body + "\n}\n\n";

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(out.toUtf8());
}

QString ThemeInspectorWidget::widgetSummary(QWidget *w) const
{
    return QString("Class: %1\n"
                   "objectName: %2\n"
                   "Visible: %3\n"
                   "Enabled: %4")
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

    qInfo() << "[possibleSelectors] Widget:" << w->metaObject()->className() << "objectName:" << w->objectName() << "->"
            << out;

    return out;
}

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
