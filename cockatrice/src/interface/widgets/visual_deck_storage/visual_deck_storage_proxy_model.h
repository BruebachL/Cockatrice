/**
 * @file visual_deck_storage_proxy_model.h
 * @ingroup VisualDeckStorageWidgets
 * @brief Proxy model handling filtering and sorting for the Visual Deck Storage.
 *
 * Operates entirely on model data (no widget references). Supports:
 * - Search text filtering (deck name, filename, format, comments, card content)
 * - Tag filtering (selected AND, excluded NOT)
 * - Color identity filtering (exact match or includes mode)
 * - Sort by name, filename, last modified, last loaded
 */

#ifndef VISUAL_DECK_STORAGE_PROXY_MODEL_H
#define VISUAL_DECK_STORAGE_PROXY_MODEL_H

#include "../../../filters/deck_filter_string.h"

#include <QMap>
#include <QSortFilterProxyModel>
#include <QStringList>

class VisualDeckStorageProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum SortOrder
    {
        ByName,
        Alphabetical,
        ByLastModified,
        ByLastLoaded,
    };

    explicit VisualDeckStorageProxyModel(QObject *parent = nullptr);

    void setSearchText(const QString &text);
    void setTagFilter(const QStringList &selectedTags, const QStringList &excludedTags);
    void setColorFilter(const QMap<QChar, bool> &activeColors, bool exactMatch);
    void setSortOrder(SortOrder order);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    bool matchesSearch(int sourceRow, const QModelIndex &sourceParent) const;
    bool matchesTags(int sourceRow, const QModelIndex &sourceParent) const;
    bool matchesColor(int sourceRow, const QModelIndex &sourceParent) const;

    DeckFilterString filterString;
    bool hasSearchFilter = false;
    QStringList selectedTags;
    QStringList excludedTags;
    QMap<QChar, bool> activeColors;
    bool exactColorMatch = false;
    SortOrder currentSortOrder = Alphabetical;
};

#endif // VISUAL_DECK_STORAGE_PROXY_MODEL_H
