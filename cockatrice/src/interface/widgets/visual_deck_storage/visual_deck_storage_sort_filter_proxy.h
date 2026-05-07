/**
 * @file visual_deck_storage_sort_filter_proxy.h
 * @ingroup VisualDeckStorageWidgets
 * @brief QSortFilterProxyModel that combines all VDS filter/sort axes:
 *   - search text (display name, file path)
 *   - colour identity (WUBRG subset)
 *   - tags
 *   - sort order (name / filename / last-modified / last-loaded)
 *
 * Folder items are never hidden by the proxy — they become hidden only
 * when all of their deck children are filtered out, which TiledTreeView
 * handles via the model's IsCollapsedRole.
 */

#ifndef VISUAL_DECK_STORAGE_SORT_FILTER_PROXY_H
#define VISUAL_DECK_STORAGE_SORT_FILTER_PROXY_H

#include <QSortFilterProxyModel>

class VisualDeckStorageSortFilterProxy : public QSortFilterProxyModel
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

    explicit VisualDeckStorageSortFilterProxy(QObject *parent = nullptr);

    void setSearchText(const QString &text);
    void setColorFilter(const QString &colors, bool exactMatch = false);
    void setTagFilter(const QStringList &required, const QStringList &excluded = {});
    void setSortOrder(SortOrder order);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString searchText_;
    QString colorFilter_;
    bool exactColorMatch_ = false;
    QStringList tagFilter_;
    QStringList excludedTagFilter_;
    SortOrder sortOrder_ = Alphabetical;
};

#endif // VISUAL_DECK_STORAGE_SORT_FILTER_PROXY_H