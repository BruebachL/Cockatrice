#ifndef COCKATRICE_THEME_INSPECTOR_WIDGET_H
#define COCKATRICE_THEME_INSPECTOR_WIDGET_H

#include <QFileSystemWatcher>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QToolButton>
#include <QTreeWidget>
#include <QWidget>

class ThemeInspectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ThemeInspectorWidget(const QString &liveCssPath, QWidget *parent = nullptr);

private slots:
    void setStylesheetPath(const QString &path);
    void reloadStylesheet();

    void rebuildTree();
    void updateForSelection();
    void showRuleBody();
    void applyRuleEdit();

private:
    struct Rule
    {
        QString selector;
        QString body;
        int line;
    };

    // UI
    QTreeWidget *widgetTree = nullptr;
    QPlainTextEdit *widgetInfo = nullptr;
    QListWidget *selectorList = nullptr;
    QTreeWidget *ruleTree = nullptr;
    QPlainTextEdit *ruleEditor = nullptr;

    QFileSystemWatcher watcher;
    QString stylesheetPath;
    QString stylesheetText;
    QVector<Rule> rules;

    // Helpers
    QWidget *createToolbar();
    void parseStylesheet();
    void updateRuleMatches(QWidget *w);
    void addChildrenToItem(QWidget *w, QTreeWidgetItem *parent);

    bool selectorMatches(QWidget *w, const QString &sel) const;

    QString widgetSummary(QWidget *w) const;
    QStringList possibleSelectors(QWidget *w) const;
};

#endif // COCKATRICE_THEME_INSPECTOR_WIDGET_H
