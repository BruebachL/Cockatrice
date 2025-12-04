#ifndef COCKATRICE_DECK_ANALYTICS_WIDGET_FACTORY_H
#define COCKATRICE_DECK_ANALYTICS_WIDGET_FACTORY_H

#include <QMap>
#include <QWidget>
#include <functional>

class AnalyticsWidgetBase;
class DeckListStatisticsAnalyzer;

class AnalyticsWidgetFactory
{
public:
    using Creator = std::function<AnalyticsWidgetBase *(QWidget *, DeckListStatisticsAnalyzer *)>;
    static AnalyticsWidgetFactory &instance()
    {
        static AnalyticsWidgetFactory f;
        return f;
    }
    void registerType(const QString &name, Creator c)
    {
        creators[name] = c;
    }
    AnalyticsWidgetBase *create(const QString &name, QWidget *parent, DeckListStatisticsAnalyzer *an)
    {
        if (!creators.contains(name))
            return nullptr;
        return creators[name](parent, an);
    }
    QStringList available() const
    {
        return creators.keys();
    }

private:
    QMap<QString, Creator> creators;
};

#endif // COCKATRICE_DECK_ANALYTICS_WIDGET_FACTORY_H
