/**
 * @file mana_devotion_widget.h
 * @ingroup DeckEditorAnalyticsWidgets
 * @brief TODO: Document this.
 */

#ifndef MANA_DEVOTION_WIDGET_H
#define MANA_DEVOTION_WIDGET_H
#include "../general/display/banner_widget.h"
#include "analyzer_modules/mana_devotion/mana_devotion_config.h"
#include "deck_analytics_widget_base.h"

#include <QHBoxLayout>

class ManaDevotionWidget : public AnalyticsWidgetBase
{
    Q_OBJECT

public slots:
    QSize sizeHint() const override;
    void updateDisplay() override;
    QDialog *createConfigDialog(QWidget *parent) override;

public:
    ManaDevotionWidget(QWidget *parent, DeckListStatisticsAnalyzer *analyzer, ManaDevotionConfig cfg = {});

    QString widgetType() const override
    {
        return "manaDevotion";
    }
    QString widgetTitle() const override
    {
        return tr("Mana Devotion");
    }

    QJsonObject saveConfig() const override
    {
        return config.toJson();
    }
    void loadConfig(const QJsonObject &o) override
    {
        config = ManaDevotionConfig::fromJson(o);
        updateDisplay();
    }

    QJsonObject extractConfigFromDialog(QDialog *dlg) const override;

private:
    ManaDevotionConfig config;
    QHBoxLayout *barLayout;
};

#endif // MANA_DEVOTION_WIDGET_H
