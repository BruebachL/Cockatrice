/**
 * @file visual_deck_storage_sort_filter_proxy.cpp
 * @ingroup VisualDeckStorageWidgets
 */

#include "visual_deck_storage_sort_filter_proxy.h"

#include "deck_storage_model.h"

#include <QDateTime>
#include <QFileInfo>

VisualDeckStorageSortFilterProxy::VisualDeckStorageSortFilterProxy(QObject *parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setRecursiveFilteringEnabled(true); // Qt 5.10+: folder visible if any child passes
    setSortRole(DeckStorageModel::DeckNameRole);
}

void VisualDeckStorageSortFilterProxy::setSearchText(const QString &text)
{
    beginFilterChange();
    searchText_ = text;
    endFilterChange();
}

void VisualDeckStorageSortFilterProxy::setColorFilter(const QString &colors, bool exactMatch)
{
    beginFilterChange();
    colorFilter_ = colors;
    exactColorMatch_ = exactMatch;
    endFilterChange();
}

void VisualDeckStorageSortFilterProxy::setTagFilter(const QStringList &required, const QStringList &excluded)
{
    beginFilterChange();
    tagFilter_ = required;
    excludedTagFilter_ = excluded;
    endFilterChange();
}

void VisualDeckStorageSortFilterProxy::setSortOrder(SortOrder order)
{
    sortOrder_ = order;
    invalidate(); // re-sort
}

// ---------------------------------------------------------------------------
// filterAcceptsRow
// ---------------------------------------------------------------------------

bool VisualDeckStorageSortFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!idx.isValid())
        return false;

    // Folders are always accepted; child filtering is handled by
    // setRecursiveFilteringEnabled — the folder is hidden automatically when
    // all children are rejected.
    if (idx.data(DeckStorageModel::IsFolderRole).toBool())
        return true;

    // --- Search text ---
    if (!searchText_.isEmpty()) {
        const QString name = idx.data(DeckStorageModel::DeckNameRole).toString();
        const QString path = idx.data(DeckStorageModel::FilePathRole).toString();
        if (!name.contains(searchText_, Qt::CaseInsensitive) && !path.contains(searchText_, Qt::CaseInsensitive))
            return false;
    }

    // --- Colour filter ---
    if (!colorFilter_.isEmpty()) {
        const QString ci = idx.data(DeckStorageModel::ColorIdentityRole).toString();
        if (exactColorMatch_) {
            if (ci != colorFilter_)
                return false;
        } else {
            for (const QChar &c : colorFilter_)
                if (!ci.contains(c))
                    return false;
        }
    }

    // --- Tag filter ---
    if (!tagFilter_.isEmpty() || !excludedTagFilter_.isEmpty()) {
        const QStringList deckTags = idx.data(DeckStorageModel::TagsRole).toStringList();
        for (const QString &req : tagFilter_)
            if (!deckTags.contains(req))
                return false;
        for (const QString &ex : excludedTagFilter_)
            if (deckTags.contains(ex))
                return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// lessThan
// ---------------------------------------------------------------------------

bool VisualDeckStorageSortFilterProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // Folders always sort before decks
    const bool lf = left.data(DeckStorageModel::IsFolderRole).toBool();
    const bool rf = right.data(DeckStorageModel::IsFolderRole).toBool();
    if (lf != rf)
        return lf > rf;
    if (lf && rf) {
        // Sort folders alphabetically by display name
        return left.data(Qt::DisplayRole).toString() < right.data(Qt::DisplayRole).toString();
    }

    switch (sortOrder_) {
        case ByName:
            return left.data(DeckStorageModel::DeckNameRole).toString() <
                   right.data(DeckStorageModel::DeckNameRole).toString();

        case Alphabetical: {
            const QString la = QFileInfo(left.data(DeckStorageModel::FilePathRole).toString()).fileName();
            const QString ra = QFileInfo(right.data(DeckStorageModel::FilePathRole).toString()).fileName();
            return QString::localeAwareCompare(la, ra) < 0;
        }

        case ByLastModified:
            return left.data(DeckStorageModel::LastModifiedRole).toDateTime() >
                   right.data(DeckStorageModel::LastModifiedRole).toDateTime();

        case ByLastLoaded:
            return left.data(DeckStorageModel::LastLoadedRole).toDateTime() >
                   right.data(DeckStorageModel::LastLoadedRole).toDateTime();
    }
    return false;
}