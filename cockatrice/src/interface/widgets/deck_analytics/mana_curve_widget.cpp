#include "mana_curve_widget.h"

#include "analyzer_modules/mana_curve/mana_curve_add_dialog.h"
#include "bar_chart_background_widget.h"
#include "deck_list_statistics_analyzer.h"
#include "segmented_bar_widget.h"

#include <QInputDialog>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QSettings>

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

ManaCurveWidget::ManaCurveWidget(QWidget *parent, DeckListStatisticsAnalyzer *analyzer, ManaCurveConfig cfg)
    : AnalyticsWidgetBase(parent, analyzer), config(std::move(cfg))
{
    bannerWidget->setText(widgetTitle());

    // bar container
    barContainer = new QWidget(this);
    barLayout = new QHBoxLayout(barContainer);
    barLayout->setSpacing(6);
    barLayout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(barContainer);

    // per-category
    byCriteriaContainer = new QWidget(this);
    byCriteriaLayout = new QVBoxLayout(byCriteriaContainer);
    byCriteriaLayout->setSpacing(6);
    byCriteriaLayout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(byCriteriaContainer);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    setMinimumHeight(220);
    connect(analyzer, &DeckListStatisticsAnalyzer::statsUpdated, this, &ManaCurveWidget::updateDisplay);
    updateDisplay();
}

QSize ManaCurveWidget::sizeHint() const
{
    // Estimate height:
    //  - main bar row: ~160px
    //  - each category row: ~60px
    //  - plus title + config button + margins

    int h = 60; // title + button baseline

    if (config.showMain)
        h += 160;

    if (config.showCategoryRows) {
        int rows = config.filters.isEmpty() ? analyzer->getManaCurveByType().size() : config.filters.size();
        h += rows * 60;
    }

    return QSize(800, h);
}

QDialog *ManaCurveWidget::createConfigDialog(QWidget *parent)
{
    // reuse existing "add" dialog, but pass current config
    ManaCurveAddDialog *dlg = new ManaCurveAddDialog(analyzer, parent);
    dlg->setFromConfig(config);
    return dlg;
}

QJsonObject ManaCurveWidget::extractConfigFromDialog(QDialog *dlg) const
{
    auto *mc = qobject_cast<ManaCurveAddDialog *>(dlg);
    if (!mc)
        return {};
    return mc->result().toJson();
}

static void clearLayoutRec(QLayout *l)
{
    if (!l)
        return;
    QLayoutItem *it;
    while ((it = l->takeAt(0)) != nullptr) {
        if (QWidget *w = it->widget())
            w->deleteLater();
        if (QLayout *sub = it->layout())
            clearLayoutRec(sub);
        delete it;
    }
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

void ManaCurveWidget::updateDisplay()
{
    clearLayoutRec(barLayout);
    clearLayoutRec(byCriteriaLayout);

    // Fetch maps according to config.groupBy
    QHash<QString, QHash<int, int>> qCategoryCounts;
    QHash<QString, QHash<int, QStringList>> qCategoryCards;

    if (config.groupBy == "color") {
        qCategoryCounts = analyzer->getManaCurveByColor();
        qCategoryCards = analyzer->getManaCurveCardsByColor();
    } else if (config.groupBy == "subtype") {
        qCategoryCounts = analyzer->getManaCurveBySubtype();
        qCategoryCards = analyzer->getManaCurveCardsBySubtype();
    } else if (config.groupBy == "power") {
        qCategoryCounts = analyzer->getManaCurveByPower();
        qCategoryCards = analyzer->getManaCurveCardsByPower();
    } else {
        qCategoryCounts = analyzer->getManaCurveByType();
        qCategoryCards = analyzer->getManaCurveCardsByType();
    }

    // Build maps
    std::map<int, std::map<QString, int>> cmcMap;
    std::map<QString, std::map<int, QStringList>> cardsMap;
    buildMapsByCategory(qCategoryCounts, qCategoryCards, cmcMap, cardsMap);

    int minCmc = 0, maxCmc = 0;
    findGlobalCmcRange(qCategoryCounts, minCmc, maxCmc);

    int highest = 1;
    for (int cmc = minCmc; cmc <= maxCmc; ++cmc) {
        int sum = 0;
        auto it = cmcMap.find(cmc);
        if (it != cmcMap.end())
            for (auto &kv : it->second)
                sum += kv.second;
        highest = std::max(highest, sum);
    }

    // ================================
    //   MAIN CHART WITH BACKGROUND
    // ================================
    if (config.showMain) {

        auto *bg = new BarChartBackgroundWidget(this);
        bg->highest = highest;
        bg->barCount = maxCmc - minCmc + 1;

        auto *inner = new QHBoxLayout(bg);
        inner->setContentsMargins(46, 6, 6, 26); // leaves space for axes
        inner->setSpacing(6);

        for (int cmc = minCmc; cmc <= maxCmc; ++cmc) {
            QVector<SegmentedBarWidget::Segment> segments;
            auto it = cmcMap.find(cmc);
            if (it != cmcMap.end()) {
                for (auto &kv : it->second) {
                    QString cat = kv.first;
                    int val = kv.second;

                    QStringList cards;
                    auto cit = cardsMap.find(cat);
                    if (cit != cardsMap.end()) {
                        auto it2 = cit->second.find(cmc);
                        if (it2 != cit->second.end())
                            cards = it2->second;
                    }

                    segments.push_back({cat, val, cards, colorHelper(cat)});
                }
            }

            std::sort(segments.begin(), segments.end(), [](auto &a, auto &b) { return a.category < b.category; });

            inner->addWidget(new SegmentedBarWidget(QString::number(cmc), segments, highest, this));
        }

        barLayout->addWidget(bg);
    }

    // ================================
    //   PER-CATEGORY ROWS (unchanged)
    // ================================
    if (config.showCategoryRows) {
        QStringList categories = qCategoryCounts.keys();
        std::sort(categories.begin(), categories.end());

        for (const QString &cat : categories) {
            QWidget *row = new QWidget(this);
            QHBoxLayout *h = new QHBoxLayout(row);
            h->setContentsMargins(0, 0, 0, 0);
            h->setSpacing(4);

            QLabel *lbl = new QLabel(cat, this);
            lbl->setFixedWidth(80);
            h->addWidget(lbl);

            const auto cmcCounts = qCategoryCounts.value(cat);
            const auto cmcCards = qCategoryCards.value(cat);

            int maxVal = 1;
            for (auto it = cmcCounts.begin(); it != cmcCounts.end(); ++it)
                maxVal = std::max(maxVal, it.value());

            for (int cmc = minCmc; cmc <= maxCmc; ++cmc) {
                int v = cmcCounts.value(cmc, 0);
                QStringList cards = cmcCards.value(cmc);

                QVector<SegmentedBarWidget::Segment> seg;
                if (v > 0)
                    seg.push_back({cat, v, cards, colorHelper(cat)});

                h->addWidget(new SegmentedBarWidget(QString::number(cmc), seg, maxVal, this));
            }

            byCriteriaLayout->addWidget(row);
        }
    }
}
