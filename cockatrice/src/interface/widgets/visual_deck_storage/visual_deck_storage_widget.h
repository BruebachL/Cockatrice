/**
 * @file visual_deck_storage_widget.h
 * @ingroup VisualDeckStorageWidgets
 */
//! \todo Document this file.

#ifndef VISUAL_DECK_STORAGE_WIDGET_H
#define VISUAL_DECK_STORAGE_WIDGET_H

#include "../../deck_loader/deck_loader.h"
#include "../cards/card_size_widget.h"
#include "../quick_settings/settings_button_widget.h"
#include "deck_preview/deck_preview_color_identity_filter_widget.h"
#include "visual_deck_storage_folder_display_widget.h"
#include "visual_deck_storage_model.h"
#include "visual_deck_storage_proxy_model.h"
#include "visual_deck_storage_quick_settings_widget.h"
#include "visual_deck_storage_search_widget.h"
#include "visual_deck_storage_sort_widget.h"
#include "visual_deck_storage_tag_filter_widget.h"

#include <QTimer>

class VisualDeckStorageWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit VisualDeckStorageWidget(QWidget *parent);
    void refreshIfPossible();
    void retranslateUi();

    VisualDeckStorageTagFilterWidget *tagFilterWidget;
    VisualDeckStorageModel *deckStorageModel;
    VisualDeckStorageProxyModel *deckStorageProxyModel;

    [[nodiscard]] const VisualDeckStorageQuickSettingsWidget *settings() const;

public slots:
    void loadModel();
    void updateShowFolders(bool enabled);
    void updateTagFilter();
    void updateColorFilter();
    void updateSearchFilter();
    void updateTagsVisibility(bool visible);
    void updateSortOrder();
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

signals:
    void bannerCardsRefreshed();
    void deckLoadRequested(const QString &filePath);
    void openDeckEditor(const LoadedDeck &deck);

private:
    QVBoxLayout *layout;
    QWidget *searchAndSortContainer;
    QHBoxLayout *searchAndSortLayout;
    QLabel *databaseLoadIndicator;
    VisualDeckStorageSortWidget *sortWidget;
    VisualDeckStorageSearchWidget *searchWidget;
    DeckPreviewColorIdentityFilterWidget *deckPreviewColorIdentityFilterWidget;
    QToolButton *refreshButton;
    VisualDeckStorageQuickSettingsWidget *quickSettingsWidget;
    QScrollArea *scrollArea;
    VisualDeckStorageFolderDisplayWidget *folderWidget;
    QTimer *rebuildTimer;

    void rebuildFolderWidget();
};

#endif // VISUAL_DECK_STORAGE_WIDGET_H
