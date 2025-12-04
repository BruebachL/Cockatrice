
#ifndef COCKATRICE_MANA_BASE_CONFIG_H
#define COCKATRICE_MANA_BASE_CONFIG_H

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

struct ManaBaseConfig
{
    QStringList filters;    // which colors to show, empty = all
    bool showTotals = true; // show totals or just individual bars

    QJsonObject toJson() const
    {
        QJsonObject o;
        QJsonArray arr;
        for (auto &s : filters)
            arr.append(s);
        o["filters"] = arr;
        o["showTotals"] = showTotals;
        return o;
    }

    static ManaBaseConfig fromJson(const QJsonObject &o)
    {
        ManaBaseConfig c;
        if (o.contains("filters")) {
            c.filters.clear();
            for (auto v : o["filters"].toArray())
                c.filters << v.toString();
        }
        if (o.contains("showTotals"))
            c.showTotals = o["showTotals"].toBool(true);
        return c;
    }
};

#endif // COCKATRICE_MANA_BASE_CONFIG_H
