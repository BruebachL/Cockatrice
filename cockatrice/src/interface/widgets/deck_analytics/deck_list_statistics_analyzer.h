#ifndef COCKATRICE_DECK_LIST_STATISTICS_ANALYZER_H
#define COCKATRICE_DECK_LIST_STATISTICS_ANALYZER_H

#include "deck_list_model.h"

#include <QHash>
#include <QObject>
#include <unordered_map>

class DeckListModel;

struct DeckListStatisticsAnalyzerConfig
{
    bool computeManaBase = true;
    bool computeManaCurve = true;
    bool computeDevotion = true;
    bool computeCategories = true;      // NEW
    bool computeCurveBreakdowns = true; // NEW
    bool computeProbabilities = true;   // NEW
};

class DeckListStatisticsAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit DeckListStatisticsAnalyzer(QObject *parent,
                                        DeckListModel *model,
                                        DeckListStatisticsAnalyzerConfig cfg = DeckListStatisticsAnalyzerConfig());

    void analyze();

    // ======= Existing API =======
    [[nodiscard]] const QHash<QString, int> &getManaBase() const
    {
        return manaBaseMap;
    }
    [[nodiscard]] const std::unordered_map<int, int> &getManaCurve() const
    {
        return manaCurveMap;
    }
    [[nodiscard]] const std::unordered_map<char, int> &getDevotion() const
    {
        return manaDevotionMap;
    }

    // ======= NEW PUBLIC GETTERS (optional; UI uses only what it needs) =======

    // --- Devotion ---
    const QHash<QString, int> &getDevotionPipCount() const
    {
        return devotionPipCount;
    }
    const QHash<QString, int> &getDevotionCardCount() const
    {
        return devotionCardCount;
    }

    // --- Mana Production ---
    const QHash<QString, int> &getProductionPipCount() const
    {
        return productionPipCount;
    }
    const QHash<QString, int> &getProductionCardCount() const
    {
        return productionCardCount;
    }

    // --- Category Quantities ---
    const QHash<QString, int> &getTypeCount() const
    {
        return typeCount;
    }
    const QHash<QString, int> &getSubtypeCount() const
    {
        return subtypeCount;
    }
    const QHash<QString, int> &getColorCount() const
    {
        return colorCount;
    }
    const QHash<QString, int> &getRarityCount() const
    {
        return rarityCount;
    }
    const QHash<int, int> &getManaValueCount() const
    {
        return manaValueCount;
    }

    // --- Mana Curve Breakdowns ---
    const QHash<QString, QHash<int, int>> &getManaCurveByType() const
    {
        return manaCurveByType;
    }
    const QHash<QString, QHash<int, int>> &getManaCurveBySubtype() const
    {
        return manaCurveBySubtype;
    }
    const QHash<QString, QHash<int, int>> &getManaCurveByColor() const
    {
        return manaCurveByColor;
    }
    const QHash<QString, QHash<int, int>> &getManaCurveByPower() const
    {
        return manaCurveByPower;
    }
    const QHash<QString, QHash<int, int>> &getManaCurveByToughness() const
    {
        return manaCurveByToughness;
    }

    const QHash<QString, QHash<int, QStringList>> &getManaCurveCardsByType() const
    {
        return manaCurveCardsByType;
    }

    const QHash<QString, QHash<int, QStringList>> &getManaCurveCardsBySubtype() const
    {
        return manaCurveCardsBySubtype;
    }

    const QHash<QString, QHash<int, QStringList>> &getManaCurveCardsByColor() const
    {
        return manaCurveCardsByColor;
    }

    const QHash<QString, QHash<int, QStringList>> &getManaCurveCardsByPower() const
    {
        return manaCurveCardsByPower;
    }

    const QHash<QString, QHash<int, QStringList>> &getManaCurveCardsByToughness() const
    {
        return manaCurveCardsByToughness;
    }

signals:
    void statsUpdated();

private:
    DeckListModel *model;
    DeckListStatisticsAnalyzerConfig config;

    // ======== Existing Containers =========
    QHash<QString, int> manaBaseMap;
    std::unordered_map<int, int> manaCurveMap;
    std::unordered_map<char, int> manaDevotionMap;

    // ======== NEW: Devotion / Production Detail =========
    QHash<QString, int> devotionPipCount;  // W/U/B/R/G total symbols
    QHash<QString, int> devotionCardCount; // how many cards provide devotion

    QHash<QString, int> productionPipCount;  // mana produced by cards
    QHash<QString, int> productionCardCount; // number of producers

    // ======== NEW: Category Quantities =========
    QHash<QString, int> typeCount;
    QHash<QString, int> subtypeCount;
    QHash<QString, int> colorCount;
    QHash<QString, int> rarityCount;
    QHash<int, int> manaValueCount;

    // ======== NEW: Mana Curve Breakdowns =========
    QHash<QString, QHash<int, int>> manaCurveByType;
    QHash<QString, QHash<int, int>> manaCurveBySubtype;
    QHash<QString, QHash<int, int>> manaCurveByColor;
    QHash<QString, QHash<int, int>> manaCurveByPower;
    QHash<QString, QHash<int, int>> manaCurveByToughness;

    QHash<QString, QHash<int, QStringList>> manaCurveCardsByType;
    QHash<QString, QHash<int, QStringList>> manaCurveCardsBySubtype;
    QHash<QString, QHash<int, QStringList>> manaCurveCardsByColor;
    QHash<QString, QHash<int, QStringList>> manaCurveCardsByPower;
    QHash<QString, QHash<int, QStringList>> manaCurveCardsByToughness;

    // ======== NEW: Probability (optional on-demand) =========
    // Not storing card info — only numeric results.
    QHash<QString, QHash<int, QHash<int, double>>> probabilityExact;
    QHash<QString, QHash<int, QHash<int, double>>> probabilityAtLeast;

    // ======== Internal Helpers =========
    QHash<QString, int> determineManaProduction(const QString &);
    std::unordered_map<char, int> countManaSymbols(const QString &);
    double hypergeometric(int N, int K, int n, int k);

    // Probability helper (no card duplication)
    double hypergeometric(int N, int K, int n, int k) const;
};

#endif // COCKATRICE_DECK_LIST_STATISTICS_ANALYZER_H
