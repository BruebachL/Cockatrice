#include "widget_inspector.h"

#include <QStringList>

// ---------------------------------------------------------------------------
// widgetSummary
// ---------------------------------------------------------------------------
QString WidgetInspector::widgetSummary(QWidget *w)
{
    if (!w)
        return {};

    QStringList info;
    info << QString("Type: %1").arg(w->metaObject()->className());
    info << QString("Object Name: %1").arg(w->objectName().isEmpty() ? "(none)" : w->objectName());

    if (!w->objectName().isEmpty()) {
        info << "";
        info << "⚠ QT CSS RULE ORDER MATTERS:";
        info << "Unlike web CSS, Qt stylesheets use LAST RULE WINS";
        info << "regardless of specificity! Use ↑↓ buttons to reorder.";
        info << "";
        info << "Best selector for this widget:";
        info << QString("  %1#%2").arg(w->metaObject()->className(), w->objectName());
    }

    info << "";
    info << QString("Path: %1").arg(widgetPath(w));
    info
        << QString("Visible: %1  |  Enabled: %2").arg(w->isVisible() ? "Yes" : "No").arg(w->isEnabled() ? "Yes" : "No");
    info << QString("Size: %1×%2px  |  Position: (%3, %4)").arg(w->width()).arg(w->height()).arg(w->x()).arg(w->y());

    return info.join("\n");
}

// ---------------------------------------------------------------------------
// widgetPath
// ---------------------------------------------------------------------------
QString WidgetInspector::widgetPath(QWidget *w)
{
    QStringList parts;
    while (w) {
        parts.prepend(w->objectName().isEmpty() ? w->metaObject()->className() : w->objectName());
        w = w->parentWidget();
    }
    return parts.join(" → ");
}

// ---------------------------------------------------------------------------
// selectorSuggestions
// ---------------------------------------------------------------------------
QVector<QtSelectors::SelectorSuggestion> WidgetInspector::selectorSuggestions(QWidget *w)
{
    QVector<QtSelectors::SelectorSuggestion> out;
    if (!w)
        return out;

    const QString type = w->metaObject()->className();

    // Object-name selectors — highest specificity
    if (!w->objectName().isEmpty()) {
        out.push_back(
            {type + "#" + w->objectName(), QString("Targets this specific %1 (highest specificity)").arg(type), 100});
        out.push_back({"#" + w->objectName(), "Targets by ID only (high specificity)", 95});
    }

    // Type selector
    out.push_back({type, QString("All %1 widgets (overrides parent classes)").arg(type), 80});

    // Valid pseudo-states for this type
    const auto validPseudos = QtSelectors::WIDGET_VALID_PSEUDOS.value(type);
    if (!validPseudos.isEmpty()) {
        for (const auto &p : QtSelectors::PSEUDO_STATES) {
            if (validPseudos.contains(p.first))
                out.push_back({type + p.first, QString("%1 when %2").arg(type, p.second.toLower()), 60});
        }
    }

    // Valid subcontrols for this type
    const auto validSubs = QtSelectors::WIDGET_VALID_SUBCONTROLS.value(type);
    if (!validSubs.isEmpty()) {
        for (const auto &s : QtSelectors::SUBCONTROLS) {
            if (validSubs.contains(s.first))
                out.push_back({type + s.first, QString("%1's %2").arg(type, s.second.toLower()), 40});
        }
    }

    std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) { return a.confidence > b.confidence; });

    return out;
}

// ---------------------------------------------------------------------------
// collectAllWidgetsRecursive
// ---------------------------------------------------------------------------
void WidgetInspector::collectAllWidgetsRecursive(QWidget *w, QList<QWidget *> &out)
{
    if (!w)
        return;
    out.append(w);
    for (QObject *c : w->children())
        if (auto *cw = qobject_cast<QWidget *>(c))
            collectAllWidgetsRecursive(cw, out);
}

// ---------------------------------------------------------------------------
// isChildOf
// ---------------------------------------------------------------------------
bool WidgetInspector::isChildOf(QWidget *widget, QWidget *potentialParent)
{
    QWidget *p = widget;
    while (p) {
        if (p == potentialParent)
            return true;
        p = p->parentWidget();
    }
    return false;
}