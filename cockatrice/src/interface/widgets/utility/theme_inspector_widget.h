#ifndef COCKATRICE_THEME_INSPECTOR_WIDGET_H
#define COCKATRICE_THEME_INSPECTOR_WIDGET_H

#include <QFileSystemWatcher>
#include <QListWidget>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPointer>
#include <QSet>
#include <QSplitter>
#include <QToolButton>
#include <QTreeWidget>
#include <QWidget>

struct SelectorGroups
{
    QStringList types;
    QStringList objects;
    QStringList pseudos;
};

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
    void showRuleBodyFromAllRules();
    void applyRuleEdit();
    void addRuleForWidget(QWidget *w, const QString &selector, const QString &body);
    void addRuleForAll(const QString &selector, const QString &body);
    void deleteSelectedRule();
    void rebuildAllRulesTree();
    void collectAllWidgetsRecursive(QWidget *w, QList<QWidget *> &out) const;
    void highlightMatchingWidget(const QString &sel);
    void collectMatchingItems(QTreeWidgetItem *item,
                              const QString &sel,
                              bool simpleType,
                              QList<QTreeWidgetItem *> &out) const;

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
    QListWidget *typesList = nullptr;
    QListWidget *objectsList = nullptr;
    QListWidget *pseudosList = nullptr;
    QTreeWidget *ruleTree = nullptr;
    QTreeWidget *allRulesTree = nullptr;
    QPlainTextEdit *ruleEditor = nullptr;

    QFileSystemWatcher watcher;
    QString stylesheetPath;
    QString stylesheetText;
    QVector<Rule> rules;

    QStringList globalTypesList;

    // Cache of all widgets for fast lookup
    QList<QWidget *> cachedWidgets;
    QHash<QWidget *, QTreeWidgetItem *> widgetToItemMap;

    // Helpers
    QWidget *createToolbar();
    void expandAllItems();
    void collapseAllItems();
    void expandItemRecursive(QTreeWidgetItem *item);
    void collapseItemRecursive(QTreeWidgetItem *item);
    void updateGlobalTypes();
    void collectWidgetTypesRecursive(QWidget *w, QSet<QString> &types);
    void parseStylesheet();
    void updateRuleMatches(QWidget *w);
    void addChildrenToItem(QWidget *w, QTreeWidgetItem *parent);

    bool selectorMatches(QWidget *w, const QString &sel) const;
    bool selectorAppliesToWidgetIgnoringPseudo(QWidget *w, const QString &sel) const;

    QString widgetSummary(QWidget *w) const;
    SelectorGroups possibleSelectorsGrouped(QWidget *w) const;

    QStringList collectObjectsWithNames() const;
    void collectObjectsWithNamesRecursive(QWidget *w, QSet<QString> &out) const;
};

#endif // COCKATRICE_THEME_INSPECTOR_WIDGET_H
