/**
 * @file visual_deck_storage_widget.h
 * @ingroup VisualDeckStorageWidgets
 * @brief Main container widget for the Visual Deck Storage feature.
 *
 * Replaces the old FlowWidget+DeckPreviewWidget approach with a
 * TiledTreeView backed by DeckStorageModel, giving O(visible) render cost.
 */

#ifndef VISUAL_DECK_STORAGE_WIDGET_H
#define VISUAL_DECK_STORAGE_WIDGET_H

#include "../../deck_loader/loaded_deck.h"
#include "../cards/card_size_widget.h"
#include "../quick_settings/settings_button_widget.h"
#include "deck_preview/deck_preview_color_identity_filter_widget.h"
#include "visual_deck_storage_quick_settings_widget.h"
#include "visual_deck_storage_search_widget.h"
#include "visual_deck_storage_sort_widget.h"
#include "visual_deck_storage_tag_filter_widget.h"

#include <libcockatrice/models/deck_list/deck_list_model.h>

class QSortFilterProxyModel;
class DeckStorageModel;
class TiledTreeView;
class VisualDeckStorageSortFilterProxy;

class VisualDeckStorageWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit VisualDeckStorageWidget(QWidget *parent);
    void retranslateUi();

    // Kept for DeckPreviewDelegate / tag filter access
    VisualDeckStorageTagFilterWidget *tagFilterWidget;

    [[nodiscard]] const VisualDeckStorageQuickSettingsWidget *settings() const;

public slots:
    void refreshIfPossible();
    void updateShowFolders(bool enabled);
    void updateTagFilter();
    void updateColorFilter();
    void updateSearchFilter();
    void updateTagsVisibility(bool visible);
    void updateSortOrder();
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

signals:
    void deckLoadRequested(const QString &filePath);
    void openDeckEditor(const LoadedDeck &deck);

private slots:
    void onFolderToggled(const QModelIndex &proxyIndex);
    void onDeckActivated(const QModelIndex &proxyIndex);
    void onDeckContextMenuRequested(const QModelIndex &proxyIndex, const QPoint &globalPos);
    void onDeckHovered(const QModelIndex &proxyIndex);
    void onCardSizeChanged(int newSize);
    void createRootModel();

private:
    void buildContextMenu(const QModelIndex &sourceIndex, const QPoint &globalPos);

    QVBoxLayout *layout_;
    QWidget *searchAndSortContainer_;
    QHBoxLayout *searchAndSortLayout_;

    DeckPreviewColorIdentityFilterWidget *colorFilterWidget_;
    VisualDeckStorageSortWidget *sortWidget_;
    VisualDeckStorageSearchWidget *searchWidget_;
    QToolButton *refreshButton_;
    VisualDeckStorageQuickSettingsWidget *quickSettingsWidget_;

    QLabel *databaseLoadIndicator_;

    DeckStorageModel *model_;
    VisualDeckStorageSortFilterProxy *proxy_;
    TiledTreeView *view_;

    bool showFolders_ = true;
};

#endif // VISUAL_DECK_STORAGE_WIDGET_H