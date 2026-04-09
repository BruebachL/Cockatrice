#ifndef COCKATRICE_WIDGET_INSPECTOR_H
#define COCKATRICE_WIDGET_INSPECTOR_H

#include "qt_selectors.h"

#include <QList>
#include <QString>
#include <QVector>
#include <QWidget>

// ---------------------------------------------------------------------------
// WidgetInspector
//
// Pure utility functions that interrogate a QWidget and return plain data.
// No UI state, no rule list — safe to call from anywhere.
// ---------------------------------------------------------------------------
class WidgetInspector
{
public:
    // Human-readable summary of a widget's properties (type, size, path, …).
    static QString widgetSummary(QWidget *w);

    // "Root → parent → objectName" breadcrumb path.
    static QString widgetPath(QWidget *w);

    // Ordered list of CSS selector candidates for `w`, most specific first.
    static QVector<QtSelectors::SelectorSuggestion> selectorSuggestions(QWidget *w);

    // Recursively collect `w` and all its QWidget descendants into `out`.
    static void collectAllWidgetsRecursive(QWidget *w, QList<QWidget *> &out);

    // True if `widget` is `potentialParent` or any descendant of it.
    static bool isChildOf(QWidget *widget, QWidget *potentialParent);
};

#endif // COCKATRICE_WIDGET_INSPECTOR_H
