#ifndef COCKATRICE_THEME_INSPECTOR_WIDGET_H
#define COCKATRICE_THEME_INSPECTOR_WIDGET_H

#include "css_parser.h"
#include "widget_inspector.h"

#include <QFileSystemWatcher>
#include <QFrame>
#include <QHash>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QWidget>

/**
 * @brief Interactive Qt stylesheet inspector and editor
 *
 * Features:
 * - Live widget tree visualization
 * - Rule matching and highlighting
 * - Interactive CSS editor with auto-save
 * - Selector suggestions
 * - Property suggestions
 */
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
    void rebuildObjectNamesTree();

    void highlightMatchingWidget(const QString &selector);
    void moveSelectedRule(int direction);
    void editSelectedRuleSelector();

    void updateStatus(const QString &message, bool isError = false);

private:
    // ---------------- UI Components ----------------
    QTreeWidget *widgetTree = nullptr;

    QPlainTextEdit *widgetInfo = nullptr;
    QListWidget *selectorSuggestionList = nullptr;

    QTreeWidget *ruleTree = nullptr;
    QTreeWidget *allRulesTree = nullptr;
    QPlainTextEdit *ruleEditor = nullptr;

    QListWidget *propertySuggestionList = nullptr;
    QLabel *statusLabel = nullptr;
    QToolButton *hideInspectorCheckbox = nullptr;

    QTreeWidget *objectNamesTree = nullptr;
    QTreeWidget *iconsTree = nullptr;

    QFileSystemWatcher watcher;

    // ---------------- Data ----------------
    CssParser m_parser; // owns the rule list (replaces QVector<Rule> rules)
    QString stylesheetPath;
    QString stylesheetText;

    QStringList globalTypes;
    QStringList globalObjects;
    QStringList globalPseudos;

    // ---------------- Helper Methods ----------------
    QWidget *createToolbar();
    QFrame *createSeparator();
    void updateGlobalTypes();

    void expandItemRecursive(QTreeWidgetItem *item);
    void collapseItemRecursive(QTreeWidgetItem *item);

    void updateRuleMatches(QWidget *w);

    int addChildrenToItem(QWidget *w, QTreeWidgetItem *parent);

    void rebuildIconsTree();

    // Shared write-and-reload helper (eliminates the copy-pasted file-write block)
    void saveRulesToDisk();
    // Rebuilds the all-rules tree and refreshes rule matches for the currently selected widget
    void refreshAfterRuleChange();
};

#endif // COCKATRICE_THEME_INSPECTOR_WIDGET_H