#include "mana_base_widget.h"

#include "../general/display/bar_widget.h"
#include "analyzer_modules/mana_base/mana_base_add_dialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>

ManaBaseWidget::ManaBaseWidget(QWidget *parent, DeckListStatisticsAnalyzer *analyzer, ManaBaseConfig cfg)
    : AnalyticsWidgetBase(parent, analyzer), config(std::move(cfg))
{
    bannerWidget->setText(widgetTitle());

    barContainer = new QWidget(this);
    barLayout = new QHBoxLayout(barContainer);
    layout->addWidget(barContainer);

    updateDisplay();
}

void ManaBaseWidget::updateDisplay()
{
    while (QLayoutItem *item = barLayout->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    auto &pipCount = analyzer->getProductionPipCount();
    auto &cardCount = analyzer->getProductionCardCount();

    QHash<QString, int> manaMap;
    for (auto key : pipCount.keys())
        manaMap[key] = pipCount[key];

    if (!config.filters.isEmpty()) {
        QHash<QString, int> filtered;
        for (auto f : config.filters)
            if (manaMap.contains(f))
                filtered[f] = manaMap[f];
        manaMap = filtered;
    }

    int highest = 1;
    for (auto val : manaMap)
        highest = std::max(highest, val);

    QHash<QString, QColor> colors = {{"W", QColor(248, 231, 185)}, {"U", QColor(14, 104, 171)},
                                     {"B", QColor(21, 11, 0)},     {"R", QColor(211, 32, 42)},
                                     {"G", QColor(0, 115, 62)},    {"C", QColor(150, 150, 150)}};

    for (auto color : manaMap.keys()) {
        QString label = QString("%1 %2 (%3)").arg(color).arg(manaMap[color]).arg(cardCount.value(color));
        BarWidget *bar = new BarWidget(label, manaMap[color], highest, colors.value(color, Qt::gray), this);
        barLayout->addWidget(bar);
    }

    update();
}

QSize ManaBaseWidget::sizeHint() const
{
    return QSize(800, 150);
}

QDialog *ManaBaseWidget::createConfigDialog(QWidget *parent)
{
    // reuse existing "add" dialog, but pass current config
    ManaBaseConfigDialog *dlg = new ManaBaseConfigDialog(analyzer, config, parent);
    return dlg;
}

QJsonObject ManaBaseWidget::extractConfigFromDialog(QDialog *dlg) const
{
    auto *mc = qobject_cast<ManaBaseConfigDialog *>(dlg);
    if (!mc)
        return {};
    return mc->result().toJson();
}