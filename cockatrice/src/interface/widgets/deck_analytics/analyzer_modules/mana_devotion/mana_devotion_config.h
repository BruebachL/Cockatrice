#ifndef COCKATRICE_MANA_DEVOTION_CONFIG_H
#define COCKATRICE_MANA_DEVOTION_CONFIG_H

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

struct ManaDevotionConfig
{
    QStringList filters; // colors
    bool showTotals = true;

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

    static ManaDevotionConfig fromJson(const QJsonObject &o)
    {
        ManaDevotionConfig c;
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

#endif // COCKATRICE_MANA_DEVOTION_CONFIG_H
