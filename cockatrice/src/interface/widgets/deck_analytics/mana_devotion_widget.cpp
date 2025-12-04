#include "mana_devotion_widget.h"

#include "../general/display/bar_widget.h"
#include "analyzer_modules/mana_devotion/mana_devotion_add_dialog.h"
#include "deck_list_statistics_analyzer.h"

#include <QDialogButtonBox>
#include <QHash>
#include <QInputDialog>
#include <QListWidget>
#include <QPushButton>

ManaDevotionWidget::ManaDevotionWidget(QWidget *parent, DeckListStatisticsAnalyzer *analyzer, ManaDevotionConfig cfg)
    : AnalyticsWidgetBase(parent, analyzer), config(std::move(cfg))
{
    bannerWidget->setText(widgetTitle());

    barLayout = new QHBoxLayout(this);
    layout->addLayout(barLayout);

    updateDisplay();
}

void ManaDevotionWidget::updateDisplay()
{
    while (QLayoutItem *item = barLayout->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    auto &pipCount = analyzer->getDevotionPipCount();
    auto &cardCount = analyzer->getDevotionCardCount();

    QHash<QChar, int> devoMap;
    for (auto key : pipCount.keys())
        devoMap[key[0]] = pipCount[key];

    if (!config.filters.isEmpty()) {
        QHash<QChar, int> filtered;
        for (auto f : config.filters)
            if (devoMap.contains(f[0]))
                filtered[f[0]] = devoMap[f[0]];
        devoMap = filtered;
    }

    int highest = 1;
    for (auto val : devoMap)
        highest = std::max(highest, val);

    QHash<QChar, QColor> colors = {{'W', QColor(248, 231, 185)}, {'U', QColor(14, 104, 171)},
                                   {'B', QColor(21, 11, 0)},     {'R', QColor(211, 32, 42)},
                                   {'G', QColor(0, 115, 62)},    {'C', QColor(150, 150, 150)}};

    for (auto c : devoMap.keys()) {
        QString label = QString("%1 %2 (%3)").arg(c).arg(devoMap[c]).arg(cardCount.value(QString(c)));
        BarWidget *bar = new BarWidget(label, devoMap[c], highest, colors.value(c, Qt::gray), this);
        barLayout->addWidget(bar);
    }

    update();
}

QDialog *ManaDevotionWidget::createConfigDialog(QWidget *parent)
{
    ManaDevotionConfigDialog *dlg = new ManaDevotionConfigDialog(analyzer, config, parent);
    return dlg;
}

QJsonObject ManaDevotionWidget::extractConfigFromDialog(QDialog *dlg) const
{
    auto *mc = qobject_cast<ManaDevotionConfigDialog *>(dlg);
    if (!mc)
        return {};
    return mc->result().toJson();
}

QSize ManaDevotionWidget::sizeHint() const
{
    return QSize(800, 150);
}
