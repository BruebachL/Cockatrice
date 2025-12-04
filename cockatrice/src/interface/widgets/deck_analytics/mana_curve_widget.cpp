// mana_curve_widget.cpp
#include "mana_curve_widget.h"

#include "../../../main.h"
#include "../../deck_loader/deck_loader.h"
#include "../general/display/banner_widget.h"
#include "../general/display/bar_widget.h"
#include "segmented_bar_widget.h"

#include <QPushButton>
#include <algorithm>
#include <libcockatrice/card/database/card_database.h>
#include <libcockatrice/card/database/card_database_manager.h>
#include <libcockatrice/deck_list/deck_list.h>
#include <unordered_map>

// small helper to clear a layout and all widgets/child layouts inside it
static void clearLayout(QLayout *layout)
{
    if (!layout)
        return;
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (auto w = item->widget()) {
            w->deleteLater();
        } else if (auto sub = item->layout()) {
            clearLayout(sub);
            delete sub;
        }
        delete item;
    }
}

ManaCurveWidget::ManaCurveWidget(QWidget *parent, DeckListStatisticsAnalyzer *_deckStatAnalyzer)
    : QWidget(parent), deckStatAnalyzer(_deckStatAnalyzer)
{
    layout = new QVBoxLayout(this);
    setLayout(layout);

    bannerWidget = new BannerWidget(this, tr("Mana Curve"), Qt::Vertical, 100);
    bannerWidget->setMaximumHeight(100);
    layout->addWidget(bannerWidget);

    barContainer = new QWidget(this);
    barLayout = new QHBoxLayout(barContainer);
    barLayout->setSpacing(6);
    barLayout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(barContainer);

    byCriteriaContainer = new QWidget(this);
    byCriteriaLayout = new QVBoxLayout(byCriteriaContainer);
    byCriteriaLayout->setSpacing(6);
    byCriteriaLayout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(byCriteriaContainer);

    auto toggleButton = new QPushButton(tr("Toggle Group (type/color)"), this);
    connect(toggleButton, &QPushButton::clicked, this, &ManaCurveWidget::updateDisplayType);
    layout->addWidget(toggleButton);

    if (groupBy.isEmpty())
        groupBy = "type";

    connect(deckStatAnalyzer, &DeckListStatisticsAnalyzer::statsUpdated, this, &ManaCurveWidget::updateDisplay);

    retranslateUi();
}

void ManaCurveWidget::retranslateUi()
{
    bannerWidget->setText(tr("Mana Curve"));
}

void ManaCurveWidget::updateDisplayType()
{
    if (groupBy == "type") {
        groupBy = "color";
    } else {
        groupBy = "type";
    }
    updateDisplay();
}

QColor colorHelper(const QString &name)
{
    static const QMap<QString, QColor> colorMap = {
        {"W", QColor(245, 245, 220)},
        {"U", QColor(80, 140, 255)},
        {"B", QColor(60, 60, 60)},
        {"R", QColor(220, 60, 50)},
        {"G", QColor(70, 160, 70)},
        {"Creature", QColor(70, 130, 180)},
        {"Instant", QColor(138, 43, 226)},
        {"Sorcery", QColor(199, 21, 133)},
        {"Enchantment", QColor(218, 165, 32)},
        {"Artifact", QColor(169, 169, 169)},
        {"Planeswalker", QColor(210, 105, 30)},
        {"Land", QColor(110, 80, 50)},
        {"Elf", QColor(34, 139, 34)},
        {"Zombie", QColor(72, 61, 139)},
        {"Human", QColor(205, 133, 63)},
        {"Goblin", QColor(178, 34, 34)},
        {"Angel", QColor(245, 245, 255)},
        {"Dragon", QColor(178, 34, 34)},
    };

    if (colorMap.contains(name))
        return colorMap[name];

    if (name.length() == 1 && colorMap.contains(name.toUpper()))
        return colorMap[name.toUpper()];

    uint h = qHash(name);
    int r = 100 + (h % 120);
    int g = 100 + ((h >> 8) % 120);
    int b = 100 + ((h >> 16) % 120);

    return QColor(r, g, b);
}

static void buildMapsByCategory(const QHash<QString, QHash<int, int>> &qCategoryCounts,
                                const QHash<QString, QHash<int, QStringList>> &qCategoryCards,
                                std::map<int, std::map<QString, int>> &outCmcMap,
                                std::map<QString, std::map<int, QStringList>> &outCardsMap)
{
    outCmcMap.clear();
    outCardsMap.clear();

    for (auto catIt = qCategoryCounts.constBegin(); catIt != qCategoryCounts.constEnd(); ++catIt) {
        const QString category = catIt.key();
        const QHash<int, int> inner = catIt.value();
        for (auto it = inner.constBegin(); it != inner.constEnd(); ++it) {
            outCmcMap[it.key()][category] = it.value();
        }
    }

    for (auto catIt = qCategoryCards.constBegin(); catIt != qCategoryCards.constEnd(); ++catIt) {
        const QString category = catIt.key();
        const QHash<int, QStringList> inner = catIt.value();
        for (auto it = inner.constBegin(); it != inner.constEnd(); ++it) {
            outCardsMap[category][it.key()] = it.value();
        }
    }
}

static void findGlobalCmcRange(const QHash<QString, QHash<int, int>> &qCategoryCounts, int &minCmc, int &maxCmc)
{
    minCmc = 0;
    maxCmc = 0;

    for (auto catIt = qCategoryCounts.constBegin(); catIt != qCategoryCounts.constEnd(); ++catIt) {
        for (auto it = catIt.value().constBegin(); it != catIt.value().constEnd(); ++it) {
            maxCmc = std::max(maxCmc, it.key());
        }
    }
}

void ManaCurveWidget::createMainCurve()
{
    QHash<QString, QHash<int, int>> qCategoryCounts =
        (groupBy == "color") ? deckStatAnalyzer->getManaCurveByColor() : deckStatAnalyzer->getManaCurveByType();

    QHash<QString, QHash<int, QStringList>> qCategoryCards = (groupBy == "color")
                                                                 ? deckStatAnalyzer->getManaCurveCardsByColor()
                                                                 : deckStatAnalyzer->getManaCurveCardsByType();

    std::map<int, std::map<QString, int>> cmcMap;
    std::map<QString, std::map<int, QStringList>> cardsMap;
    buildMapsByCategory(qCategoryCounts, qCategoryCards, cmcMap, cardsMap);

    int minCmc, maxCmc;
    findGlobalCmcRange(qCategoryCounts, minCmc, maxCmc);

    int highest = 0;
    for (int cmc = minCmc; cmc <= maxCmc; ++cmc) {
        int sum = 0;
        auto it = cmcMap.find(cmc);
        if (it != cmcMap.end()) {
            for (const auto &kv : it->second)
                sum += kv.second;
        }
        highest = std::max(highest, sum);
    }
    if (highest == 0)
        highest = 1;

    for (int cmc = minCmc; cmc <= maxCmc; ++cmc) {
        QVector<SegmentedBarWidget::Segment> segments;
        auto it = cmcMap.find(cmc);

        if (it != cmcMap.end()) {
            for (const auto &kv : it->second) {
                const QString &category = kv.first;
                int value = kv.second;

                QStringList cards;
                auto cit = cardsMap.find(category);
                if (cit != cardsMap.end()) {
                    auto it2 = cit->second.find(cmc);
                    if (it2 != cit->second.end())
                        cards = it2->second;
                }

                segments.push_back({category, value, cards, colorHelper(category)});
            }
        }

        std::sort(segments.begin(), segments.end(),
                  [](const auto &a, const auto &b) { return a.category < b.category; });

        auto *widget = new SegmentedBarWidget(QString::number(cmc), segments, highest, this);
        barLayout->addWidget(widget);
    }
}

void ManaCurveWidget::createCategoryCurves()
{
    QHash<QString, QHash<int, int>> qCategoryCounts =
        (groupBy == "color") ? deckStatAnalyzer->getManaCurveByColor() : deckStatAnalyzer->getManaCurveByType();

    QHash<QString, QHash<int, QStringList>> qCategoryCards = (groupBy == "color")
                                                                 ? deckStatAnalyzer->getManaCurveCardsByColor()
                                                                 : deckStatAnalyzer->getManaCurveCardsByType();

    QStringList categories = qCategoryCounts.keys();
    std::sort(categories.begin(), categories.end());

    int minCmc, maxCmc;
    findGlobalCmcRange(qCategoryCounts, minCmc, maxCmc);

    for (const QString &category : categories) {
        QWidget *rowWidget = new QWidget(this);
        QHBoxLayout *hbox = new QHBoxLayout(rowWidget);
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->setSpacing(4);

        QLabel *lbl = new QLabel(category, this);
        lbl->setFixedWidth(80);
        hbox->addWidget(lbl);

        const QHash<int, int> cmcCounts = qCategoryCounts.value(category);
        const QHash<int, QStringList> cmcCards = qCategoryCards.value(category);

        int maxValue = 1;
        for (auto it = cmcCounts.constBegin(); it != cmcCounts.constEnd(); ++it)
            maxValue = std::max(maxValue, it.value());
        if (maxValue == 0)
            maxValue = 1;

        for (int cmc = minCmc; cmc <= maxCmc; ++cmc) {
            int value = cmcCounts.value(cmc, 0);
            QStringList cards = cmcCards.value(cmc);

            QVector<SegmentedBarWidget::Segment> seg;
            seg.push_back({category, value, cards, colorHelper(category)});

            auto *w = new SegmentedBarWidget(QString::number(cmc), seg, maxValue, this);
            hbox->addWidget(w);
        }

        byCriteriaLayout->addWidget(rowWidget);
    }
}

void ManaCurveWidget::updateDisplay()
{
    clearLayout(barLayout);
    clearLayout(byCriteriaLayout);

    createMainCurve();
    createCategoryCurves();

    update();
}
