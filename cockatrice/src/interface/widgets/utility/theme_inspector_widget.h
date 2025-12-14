#ifndef COCKATRICE_THEME_INSPECTOR_WIDGET_H
#define COCKATRICE_THEME_INSPECTOR_WIDGET_H

#include <QFileSystemWatcher>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QWidget>

class ThemeInspectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ThemeInspectorWidget(const QString &liveCssPath = {}, QWidget *parent = nullptr);
    void setStylesheetPath(const QString &path);

private slots:
    void rebuildTree();
    void addChildrenToItem(QWidget *w, QTreeWidgetItem *parent);
    void reloadStylesheet();
    void updateForSelection();
    void showRuleBody();

private:
    struct Rule
    {
        QString selector;
        QString body;
        int line;
    };

    // UI
    QTreeWidget *widgetTree{};
    QTreeWidget *ruleTree{};
    QPlainTextEdit *widgetInfo{};
    QPlainTextEdit *ruleEditor{};

    QWidget *createToolbar();

    // Widget tree
    void addWidgetRecursive(QTreeWidgetItem *parent, QWidget *w);

    // Stylesheet handling
    void parseStylesheet();
    void updateRuleMatches(QWidget *w);

    bool selectorMatches(QWidget *w, const QString &selector) const;
    QString widgetSummary(QWidget *w) const;
    QStringList possibleSelectors(QWidget *w) const;

    QString stylesheetPath;
    QString stylesheetText;
    QFileSystemWatcher watcher;
    QVector<Rule> rules;
};

#endif // COCKATRICE_THEME_INSPECTOR_WIDGET_H
