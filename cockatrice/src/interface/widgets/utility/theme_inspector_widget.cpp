#include "theme_inspector_widget.h"

#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

ThemeInspectorWidget::ThemeInspectorWidget(const QString &liveCssPath, QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Theme Inspector");
    setMinimumSize(1200, 650);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    widgetTree = new QTreeWidget;
    widgetTree->setHeaderLabels({"Widget", "objectName"});
    widgetTree->setUniformRowHeights(true);

    widgetInfo = new QPlainTextEdit;
    widgetInfo->setReadOnly(true);
    widgetInfo->setMaximumHeight(160);

    ruleTree = new QTreeWidget;
    ruleTree->setHeaderLabels({"Selector", "Line"});
    ruleTree->setUniformRowHeights(true);

    ruleEditor = new QPlainTextEdit;
    ruleEditor->setPlaceholderText("Rule body (read-only preview)");

    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->addWidget(widgetTree);
    leftLayout->addWidget(widgetInfo);

    splitter->addWidget(left);
    splitter->addWidget(ruleTree);
    splitter->addWidget(ruleEditor);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 3);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(createToolbar());
    layout->addWidget(splitter);
    setLayout(layout);

    connect(widgetTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::updateForSelection);
    connect(ruleTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBody);

    rebuildTree();
    setStylesheetPath(liveCssPath);
}

void ThemeInspectorWidget::setStylesheetPath(const QString &path)
{
    if (path == stylesheetPath)
        return;

    watcher.removePaths(watcher.files());
    stylesheetPath = path;

    if (!stylesheetPath.isEmpty()) {
        watcher.addPath(stylesheetPath);
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

    // Remove all C-style comments: /* ... */
    text.remove(QRegularExpression(R"(/\*[\s\S]*?\*/)", QRegularExpression::MultilineOption));

    // Regex for selector { body }
    QRegularExpression re(R"(([^\{]+)\{([^\}]*)\})");
    int line = 1;

    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();

        const QStringList selectors = m.captured(1).split(',', Qt::SkipEmptyParts);
        for (QString s : selectors) {
            s = s.trimmed();
            // Ignore private selectors
            if (s.contains("::") || s.startsWith("qproperty-"))
                continue;

            rules.push_back({s, m.captured(2).trimmed(), line});

            qInfo() << "[parseStylesheet] Found rule:" << s << "line" << line;
        }

        line += m.captured(0).count('\n');
    }

    qInfo() << "[parseStylesheet] Total rules:" << rules.size();
}

void ThemeInspectorWidget::rebuildTree()
{
    widgetTree->clear();

    for (QWidget *top : QApplication::topLevelWidgets()) {
        if (top == this)
            continue; // skip the inspector itself
        QMainWindow *mw = qobject_cast<QMainWindow *>(top);
        if (!mw)
            continue; // skip non-main windows

        QString rootName = mw->objectName().isEmpty() ? mw->metaObject()->className() : mw->objectName();

        QTreeWidgetItem *rootItem = new QTreeWidgetItem(widgetTree, QStringList(rootName));
        rootItem->setData(0, Qt::UserRole, QVariant::fromValue((QObject *)mw));

        addChildrenToItem(mw, rootItem);
    }

    widgetTree->expandToDepth(1);
}

void ThemeInspectorWidget::addChildrenToItem(QWidget *w, QTreeWidgetItem *parent)
{
    if (!w || !parent)
        return;

    for (QObject *c : w->children()) {
        QWidget *cw = qobject_cast<QWidget *>(c);
        if (!cw)
            continue;

        QString name = cw->objectName().isEmpty() ? cw->metaObject()->className() : cw->objectName();
        QTreeWidgetItem *it = new QTreeWidgetItem(parent, QStringList{name});
        it->setData(0, Qt::UserRole, QVariant::fromValue((QObject *)cw)); // store QObject*

        addChildrenToItem(cw, it);
    }
}

void ThemeInspectorWidget::updateForSelection()
{
    widgetInfo->clear();
    ruleTree->clear();
    ruleEditor->clear();

    auto items = widgetTree->selectedItems();
    if (items.isEmpty())
        return;

    QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QObject *>()); // <- fixed

    if (!w)
        return;

    widgetInfo->appendPlainText(widgetSummary(w));
    widgetInfo->appendPlainText("\nPossible selectors:\n");

    for (const QString &s : possibleSelectors(w))
        widgetInfo->appendPlainText("  " + s);

    updateRuleMatches(w);
}

void ThemeInspectorWidget::updateRuleMatches(QWidget *w)
{
    qInfo() << "\n[updateRuleMatches] Checking widget:" << w->metaObject()->className()
            << "objectName:" << w->objectName();

    for (int i = 0; i < rules.size(); ++i) {
        bool match = selectorMatches(w, rules[i].selector);
        qInfo() << "  Checking selector:" << rules[i].selector << "->" << (match ? "MATCH" : "no match");

        if (match) {
            auto *item = new QTreeWidgetItem(ruleTree);
            item->setText(0, rules[i].selector);
            item->setText(1, QString::number(rules[i].line));
            item->setData(0, Qt::UserRole, i);
        }
    }
}

bool ThemeInspectorWidget::selectorMatches(QWidget *w, const QString &sel) const
{
    QString s = sel.trimmed();

    // Handle child selector: Parent > Child
    if (s.contains('>')) {
        QStringList parts = s.split('>', Qt::SkipEmptyParts);
        if (parts.size() != 2)
            return false;

        QString parentSel = parts[0].trimmed();
        QString childSel = parts[1].trimmed();

        QWidget *parent = w->parentWidget();
        return parent && selectorMatches(parent, parentSel) && selectorMatches(w, childSel);
    }

    // Handle #objectName
    if (s.startsWith('#'))
        return w->objectName() == s.mid(1);

    // Handle attribute selectors [prop="value"]
    if (s.startsWith('[') && s.endsWith(']')) {
        auto parts = s.mid(1, s.size() - 2).split('=');
        if (parts.size() != 2)
            return false;
        return w->property(parts[0].toUtf8()).toString() == parts[1].remove('"');
    }

    // Handle pseudo states
    if (s == ":disabled")
        return !w->isEnabled();
    if (s == ":focus")
        return w->hasFocus();
    if (s == ":hover")
        return w->underMouse();

    // Handle class names (walk hierarchy)
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
    if (idx < 0 || idx >= rules.size())
        return;

    ruleEditor->setPlainText(rules[idx].body);
}

QString ThemeInspectorWidget::widgetSummary(QWidget *w) const
{
    return QString("Class: %1\n"
                   "objectName: %2\n"
                   "Visible: %3\n"
                   "Enabled: %4\n")
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
    bar->setFixedHeight(28);

    auto *l = new QHBoxLayout(bar);
    l->setContentsMargins(4, 2, 4, 2);

    auto *refresh = new QToolButton;
    refresh->setText("↻");
    refresh->setToolTip("Rebuild widget tree");
    connect(refresh, &QToolButton::clicked, this, &ThemeInspectorWidget::rebuildTree);

    l->addWidget(refresh);
    l->addStretch();
    return bar;
}

void ThemeInspectorWidget::addWidgetRecursive(QTreeWidgetItem *parent, QWidget *w)
{
    auto *item = new QTreeWidgetItem;
    item->setText(0, w->metaObject()->className());
    item->setText(1, w->objectName());
    item->setData(0, Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(w)));

    parent ? parent->addChild(item) : widgetTree->addTopLevelItem(item);

    for (QObject *c : w->children()) {
        if (auto *cw = qobject_cast<QWidget *>(c))
            addWidgetRecursive(item, cw);
    }
}
