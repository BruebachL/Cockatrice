/**
 * @file mana_curve_widget.h
 * @ingroup DeckEditorAnalyticsWidgets
 * @brief TODO: Document this.
 */

#ifndef MANA_CURVE_WIDGET_H
#define MANA_CURVE_WIDGET_H

#include "analyzer_modules/mana_curve/mana_curve_config.h"
#include "deck_analytics_widget_base.h"

#include <QVBoxLayout>

class SegmentedBarWidget;
class DeckListStatisticsAnalyzer;

class ManaCurveWidget : public AnalyticsWidgetBase
{
    Q_OBJECT

public slots:
    QSize sizeHint() const override;
    void updateDisplay() override;
    QDialog *createConfigDialog(QWidget *parent) override;

public:
    ManaCurveWidget(QWidget *parent, DeckListStatisticsAnalyzer *analyzer, ManaCurveConfig cfg = {});
    QString widgetType() const override
    {
        return "manaCurve";
    }
    QString widgetTitle() const override
    {
        return tr("Mana Curve (%1)").arg(config.groupBy);
    }
    QJsonObject saveConfig() const override
    {
        return config.toJson();
    }
    void loadConfig(const QJsonObject &o) override
    {
        config = ManaCurveConfig::fromJson(o);
        updateDisplay();
    };

    QJsonObject extractConfigFromDialog(QDialog *dlg) const override;

private:
    ManaCurveConfig config;
    QWidget *barContainer = nullptr;
    QHBoxLayout *barLayout = nullptr;
    QWidget *byCriteriaContainer = nullptr;
    QVBoxLayout *byCriteriaLayout = nullptr;
};

#endif // MANA_CURVE_WIDGET_H
