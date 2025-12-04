
#ifndef COCKATRICE_MANA_CURVE_CONFIG_H
#define COCKATRICE_MANA_CURVE_CONFIG_H

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

struct ManaCurveConfig
{
    QString groupBy = "type"; // "type", "color", "subtype", etc.
    QStringList filters;      // empty = all
    bool showMain = true;
    bool showCategoryRows = true;

    QJsonObject toJson() const
    {
        QJsonObject o;
        o["groupBy"] = groupBy;
        QJsonArray arr;
        for (auto &s : filters)
            arr.append(s);
        o["filters"] = arr;
        o["showMain"] = showMain;
        o["showCategoryRows"] = showCategoryRows;
        return o;
    }

    static ManaCurveConfig fromJson(const QJsonObject &o)
    {
        ManaCurveConfig c;
        if (o.contains("groupBy"))
            c.groupBy = o["groupBy"].toString();
        if (o.contains("filters")) {
            c.filters.clear();
            for (auto v : o["filters"].toArray())
                c.filters << v.toString();
        }
        if (o.contains("showMain"))
            c.showMain = o["showMain"].toBool(true);
        if (o.contains("showCategoryRows"))
            c.showCategoryRows = o["showCategoryRows"].toBool(true);
        return c;
    }
};

#endif // COCKATRICE_MANA_CURVE_CONFIG_H
