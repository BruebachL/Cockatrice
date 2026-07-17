#include "visual_deck_storage_proxy_model.h"

#include "visual_deck_storage_model.h"

#include <QFileInfo>
#include <algorithm>

VisualDeckStorageProxyModel::VisualDeckStorageProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setFilterRole(Qt::DisplayRole);
}

void VisualDeckStorageProxyModel::setSearchText(const QString &text)
{
    beginFilterChange();
    hasSearchFilter = !text.isEmpty();
    filterString = DeckFilterString(text);
    endFilterChange();
}

void VisualDeckStorageProxyModel::setTagFilter(const QStringList &selected, const QStringList &excluded)
{
    beginFilterChange();
    selectedTags = selected;
    excludedTags = excluded;
    endFilterChange();
}

void VisualDeckStorageProxyModel::setColorFilter(const QMap<QChar, bool> &colors, bool exactMatch)
{
    beginFilterChange();
    activeColors = colors;
    exactColorMatch = exactMatch;
    endFilterChange();
}

void VisualDeckStorageProxyModel::setSortOrder(SortOrder order)
{
    currentSortOrder = order;
    invalidate();
}

bool VisualDeckStorageProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return matchesSearch(sourceRow, sourceParent) && matchesTags(sourceRow, sourceParent) &&
           matchesColor(sourceRow, sourceParent);
}

bool VisualDeckStorageProxyModel::matchesSearch(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!hasSearchFilter) {
        return true;
    }

    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    DeckFilterData filterData;
    filterData.filePath = sourceModel()->data(idx, VDSModelRoles::FilePathRole).toString();
    filterData.deckLoader = sourceModel()->data(idx, VDSModelRoles::DeckLoaderRole).value<DeckLoader *>();

    ExtraDeckSearchInfo extraInfo;
    extraInfo.relativeFilePath = sourceModel()->data(idx, VDSModelRoles::RelativeFilePathRole).toString();

    return filterString.check(filterData, extraInfo);
}

bool VisualDeckStorageProxyModel::matchesTags(int sourceRow, const QModelIndex &sourceParent) const
{
    if (selectedTags.isEmpty() && excludedTags.isEmpty()) {
        return true;
    }

    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    QStringList deckTags = sourceModel()->data(idx, VDSModelRoles::TagsRole).toStringList();

    bool hasAllSelected =
        std::all_of(selectedTags.begin(), selectedTags.end(), [&deckTags](const QString &tag) {
            return deckTags.contains(tag);
        });

    bool hasAnyExcluded =
        std::any_of(excludedTags.begin(), excludedTags.end(), [&deckTags](const QString &tag) {
            return deckTags.contains(tag);
        });

    return hasAllSelected && !hasAnyExcluded;
}

bool VisualDeckStorageProxyModel::matchesColor(int sourceRow, const QModelIndex &sourceParent) const
{
    bool noColorsActive = true;
    for (auto it = activeColors.constBegin(); it != activeColors.constEnd(); ++it) {
        if (it.value()) {
            noColorsActive = false;
            break;
        }
    }

    if (noColorsActive) {
        return true;
    }

    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    QString colorIdentity = sourceModel()->data(idx, VDSModelRoles::ColorIdentityRole).toString();

    if (exactColorMatch) {
        QSet<QChar> activeColorSet;
        for (auto it = activeColors.constBegin(); it != activeColors.constEnd(); ++it) {
            if (it.value()) {
                activeColorSet.insert(it.key().toUpper());
            }
        }

        QSet<QChar> colorIdentitySet;
        for (const QChar &color : colorIdentity) {
            colorIdentitySet.insert(color.toUpper());
        }

        return activeColorSet == colorIdentitySet;
    } else {
        for (auto it = activeColors.constBegin(); it != activeColors.constEnd(); ++it) {
            if (it.value() && !colorIdentity.contains(it.key())) {
                return false;
            }
        }
        return true;
    }
}

bool VisualDeckStorageProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (currentSortOrder) {
        case ByName: {
            QString leftName = sourceModel()->data(left, VDSModelRoles::DeckNameRole).toString();
            QString rightName = sourceModel()->data(right, VDSModelRoles::DeckNameRole).toString();
            return leftName.toLower() < rightName.toLower();
        }
        case Alphabetical: {
            QString leftPath = sourceModel()->data(left, VDSModelRoles::FilePathRole).toString();
            QString rightPath = sourceModel()->data(right, VDSModelRoles::FilePathRole).toString();
            return QString::localeAwareCompare(QFileInfo(leftPath).fileName(), QFileInfo(rightPath).fileName()) < 0;
        }
        case ByLastModified: {
            QDateTime leftTime = sourceModel()->data(left, VDSModelRoles::LastModifiedTimeRole).toDateTime();
            QDateTime rightTime = sourceModel()->data(right, VDSModelRoles::LastModifiedTimeRole).toDateTime();
            return leftTime > rightTime;
        }
        case ByLastLoaded: {
            QDateTime leftTime = sourceModel()->data(left, VDSModelRoles::LastLoadedTimeRole).toDateTime();
            QDateTime rightTime = sourceModel()->data(right, VDSModelRoles::LastLoadedTimeRole).toDateTime();
            return leftTime > rightTime;
        }
    }
    return false;
}
