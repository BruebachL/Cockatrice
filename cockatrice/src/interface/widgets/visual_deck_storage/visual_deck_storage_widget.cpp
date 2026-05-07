/**
 * @file visual_deck_storage_widget.cpp
 * @ingroup VisualDeckStorageWidgets
 */

#include "visual_deck_storage_widget.h"

#include "../../../client/settings/cache_settings.h"
#include "deck_preview/deck_preview_deck_tags_display_widget.h"
#include "deck_preview_delegate.h"
#include "deck_storage_model.h"
#include "tiled_tree_view.h"
#include "visual_deck_storage_sort_filter_proxy.h"

#include <QClipboard>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <libcockatrice/card/database/card_database_manager.h>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

VisualDeckStorageWidget::VisualDeckStorageWidget(QWidget *parent) : QWidget(parent)
{
    layout_ = new QVBoxLayout(this);
    layout_->setSpacing(0);
    layout_->setContentsMargins(9, 0, 9, 5);
    setLayout(layout_);

    // --- Search / sort bar ---
    searchAndSortContainer_ = new QWidget(this);
    searchAndSortLayout_ = new QHBoxLayout(searchAndSortContainer_);
    searchAndSortLayout_->setSpacing(3);
    searchAndSortLayout_->setContentsMargins(9, 0, 9, 0);
    searchAndSortContainer_->setLayout(searchAndSortLayout_);

    colorFilterWidget_ = new DeckPreviewColorIdentityFilterWidget(this);
    sortWidget_ = new VisualDeckStorageSortWidget(this);
    searchWidget_ = new VisualDeckStorageSearchWidget(this);

    refreshButton_ = new QToolButton(this);
    refreshButton_->setIcon(QPixmap("theme:icons/reload"));
    refreshButton_->setFixedSize(32, 32);
    connect(refreshButton_, &QToolButton::clicked, this, &VisualDeckStorageWidget::refreshIfPossible);

    quickSettingsWidget_ = new VisualDeckStorageQuickSettingsWidget(this);
    connect(quickSettingsWidget_, &VisualDeckStorageQuickSettingsWidget::showFoldersChanged, this,
            &VisualDeckStorageWidget::updateShowFolders);
    connect(quickSettingsWidget_, &VisualDeckStorageQuickSettingsWidget::showTagFilterChanged, this,
            &VisualDeckStorageWidget::updateTagsVisibility);
    connect(quickSettingsWidget_, &VisualDeckStorageQuickSettingsWidget::cardSizeChanged, this,
            &VisualDeckStorageWidget::onCardSizeChanged);

    searchAndSortLayout_->addWidget(colorFilterWidget_);
    searchAndSortLayout_->addWidget(sortWidget_);
    searchAndSortLayout_->addWidget(searchWidget_);
    searchAndSortLayout_->addWidget(refreshButton_);
    searchAndSortLayout_->addWidget(quickSettingsWidget_);

    // --- Tag filter ---
    tagFilterWidget = new VisualDeckStorageTagFilterWidget(this);
    updateTagsVisibility(SettingsCache::instance().getVisualDeckStorageShowTagFilter());

    // --- Model / proxy / view ---
    model_ = new DeckStorageModel(this);
    proxy_ = new VisualDeckStorageSortFilterProxy(this);
    proxy_->setSourceModel(model_);

    view_ = new TiledTreeView(this);
    view_->setModel(proxy_);
    view_->setItemDelegate(new DeckPreviewDelegate(quickSettingsWidget_, this));
    onCardSizeChanged(SettingsCache::instance().getVisualDeckStorageCardSize());

    connect(view_, &TiledTreeView::folderToggled, this, &VisualDeckStorageWidget::onFolderToggled);
    connect(view_, &TiledTreeView::deckActivated, this, &VisualDeckStorageWidget::onDeckActivated);
    connect(view_, &TiledTreeView::deckContextMenuRequested, this,
            &VisualDeckStorageWidget::onDeckContextMenuRequested);
    connect(view_, &TiledTreeView::deckHovered, this, &VisualDeckStorageWidget::onDeckHovered);

    // Filter / sort wiring
    connect(searchWidget_, &VisualDeckStorageSearchWidget::searchTextChanged, this,
            &VisualDeckStorageWidget::updateSearchFilter);
    connect(colorFilterWidget_, &DeckPreviewColorIdentityFilterWidget::colorFilterChanged, this,
            &VisualDeckStorageWidget::updateColorFilter);
    connect(tagFilterWidget, &VisualDeckStorageTagFilterWidget::tagFilterChanged, this,
            &VisualDeckStorageWidget::updateTagFilter);
    connect(sortWidget_, &VisualDeckStorageSortWidget::sortOrderChanged, this,
            &VisualDeckStorageWidget::updateSortOrder);

    // Tags discovered asynchronously
    connect(model_, &DeckStorageModel::tagsUpdated, tagFilterWidget,
            &VisualDeckStorageTagFilterWidget::setAvailableTags);

    // Settings that only affect painting (no rebuild needed, just repaint)
    connect(quickSettingsWidget_, &VisualDeckStorageQuickSettingsWidget::showColorIdentityChanged, view_->viewport(),
            qOverload<>(&QWidget::update));
    connect(quickSettingsWidget_, &VisualDeckStorageQuickSettingsWidget::showTagsOnDeckPreviewsChanged,
            view_->viewport(), qOverload<>(&QWidget::update));
    connect(quickSettingsWidget_, &VisualDeckStorageQuickSettingsWidget::showBannerCardComboBoxChanged,
            view_->viewport(), qOverload<>(&QWidget::update));
    connect(quickSettingsWidget_, &VisualDeckStorageQuickSettingsWidget::deckPreviewTooltipChanged, view_->viewport(),
            qOverload<>(&QWidget::update));

    // --- Loading indicator ---
    databaseLoadIndicator_ = new QLabel(tr("Loading database …"), this);
    databaseLoadIndicator_->setAlignment(Qt::AlignCenter);

    layout_->addWidget(searchAndSortContainer_);
    layout_->addWidget(tagFilterWidget);
    layout_->addWidget(databaseLoadIndicator_);
    layout_->addWidget(view_);

    showFolders_ = SettingsCache::instance().getVisualDeckStorageShowFolders();

    connect(CardDatabaseManager::getInstance(), &CardDatabase::cardDatabaseLoadingFinished, this,
            &VisualDeckStorageWidget::createRootModel);

    if (CardDatabaseManager::getInstance()->getLoadStatus() == LoadStatus::Ok) {
        createRootModel();
        databaseLoadIndicator_->setVisible(false);
    } else {
        view_->setVisible(false);
    }
}

// ---------------------------------------------------------------------------
// Public slots
// ---------------------------------------------------------------------------

void VisualDeckStorageWidget::refreshIfPossible()
{
    if (!databaseLoadIndicator_->isVisible())
        createRootModel();
}

void VisualDeckStorageWidget::updateShowFolders(bool enabled)
{
    showFolders_ = enabled;
    createRootModel();
}

void VisualDeckStorageWidget::updateSortOrder()
{
    using SO = VisualDeckStorageSortFilterProxy::SortOrder;
    proxy_->setSortOrder(static_cast<SO>(sortWidget_->currentSortIndex()));
    proxy_->sort(0);
}

void VisualDeckStorageWidget::updateTagFilter()
{
    proxy_->setTagFilter(tagFilterWidget->selectedTags(), tagFilterWidget->excludedTags());
}

void VisualDeckStorageWidget::updateColorFilter()
{
    proxy_->setColorFilter(colorFilterWidget_->selectedColors(), colorFilterWidget_->isExactMatch());
}

void VisualDeckStorageWidget::updateSearchFilter()
{
    proxy_->setSearchText(searchWidget_->getSearchText());
}

void VisualDeckStorageWidget::updateTagsVisibility(bool visible)
{
    tagFilterWidget->setVisible(visible);
}

void VisualDeckStorageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void VisualDeckStorageWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

const VisualDeckStorageQuickSettingsWidget *VisualDeckStorageWidget::settings() const
{
    return quickSettingsWidget_;
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void VisualDeckStorageWidget::createRootModel()
{
    databaseLoadIndicator_->setVisible(false);
    view_->setVisible(true);
    model_->populate(SettingsCache::instance().getDeckPath(), showFolders_);
    QTimer::singleShot(0, this, &VisualDeckStorageWidget::updateSortOrder);
}

void VisualDeckStorageWidget::onCardSizeChanged(int newSize)
{
    // newSize is the slider value (percentage); BASE_WIDTH in CardInfoPictureWidget is 200
    const int w = 200 * newSize / 100;
    view_->setCellWidth(w);
}

void VisualDeckStorageWidget::onFolderToggled(const QModelIndex &proxyIndex)
{
    const QModelIndex sourceIndex = proxy_->mapToSource(proxyIndex);
    model_->toggleCollapsed(sourceIndex);
    view_->rebuildLayout();
}

void VisualDeckStorageWidget::onDeckActivated(const QModelIndex &proxyIndex)
{
    const QModelIndex src = proxy_->mapToSource(proxyIndex);
    const QString path = model_->data(src, DeckStorageModel::FilePathRole).toString();
    emit deckLoadRequested(path);
}

void VisualDeckStorageWidget::onDeckHovered(const QModelIndex &proxyIndex)
{
    const QModelIndex src = proxy_->mapToSource(proxyIndex);
    model_->reloadIfModified(src);
}

void VisualDeckStorageWidget::onDeckContextMenuRequested(const QModelIndex &proxyIndex, const QPoint &globalPos)
{
    const QModelIndex src = proxy_->mapToSource(proxyIndex);
    buildContextMenu(src, globalPos);
}

// ---------------------------------------------------------------------------
// Context menu — mirrors DeckPreviewWidget::createRightClickMenu
// ---------------------------------------------------------------------------

void VisualDeckStorageWidget::buildContextMenu(const QModelIndex &src, const QPoint &globalPos)
{
    auto *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    const LoadedDeck *deckPtr = model_->deckAt(src);
    if (!deckPtr)
        return;
    const LoadedDeck deck = *deckPtr;

    // Open in deck editor
    connect(menu->addAction(tr("Open in deck editor")), &QAction::triggered, this,
            [this, deck] { emit openDeckEditor(deck); });

    // Edit tags
    connect(menu->addAction(tr("Edit Tags")), &QAction::triggered, this, [this, src, deck] {
        // Reuse the existing tag-edit dialog via a temporary helper widget
        auto *tagsDisplay = new DeckPreviewDeckTagsDisplayWidget(this);
        tagsDisplay->setTags(deck.deckList.getTags());
        connect(tagsDisplay, &DeckPreviewDeckTagsDisplayWidget::tagsChanged, this,
                [this, src](const QStringList &tags) { model_->setTags(src, tags); });
        tagsDisplay->openTagEditDlg();
        tagsDisplay->deleteLater();
    });

    // Set banner card submenu
    auto *bannerMenu = menu->addMenu(tr("Set Banner Card"));
    const auto cards = deck.deckList.getCardNodes();
    QSet<QString> seen;
    for (const auto *node : cards) {
        if (seen.contains(node->getName()))
            continue;
        seen.insert(node->getName());
        const QString name = node->getName();
        const QString id = node->getCardProviderId();
        QAction *act = bannerMenu->addAction(name);
        act->setCheckable(true);
        act->setChecked(deck.deckList.getBannerCard().name == name);
        connect(act, &QAction::triggered, this, [this, src, name, id] { model_->setBannerCard(src, {name, id}); });
    }

    menu->addSeparator();

    // Rename deck (name field)
    connect(menu->addAction(tr("Rename Deck")), &QAction::triggered, this, [this, src, deck] {
        bool ok;
        const QString oldName = deck.deckList.getName();
        QString newName =
            QInputDialog::getText(this, tr("Rename deck"), tr("New name:"), QLineEdit::Normal, oldName, &ok);
        if (ok && newName != oldName)
            model_->renameDeck(src, newName);
    });

    // Save to clipboard submenu
    auto *clipMenu = menu->addMenu(tr("Save Deck to Clipboard"));
    connect(clipMenu->addAction(tr("Annotated")), &QAction::triggered, this,
            [deck] { DeckLoader::saveToClipboard(deck.deckList, true, true); });
    connect(clipMenu->addAction(tr("Annotated (No set info)")), &QAction::triggered, this,
            [deck] { DeckLoader::saveToClipboard(deck.deckList, true, false); });
    connect(clipMenu->addAction(tr("Not Annotated")), &QAction::triggered, this,
            [deck] { DeckLoader::saveToClipboard(deck.deckList, false, true); });
    connect(clipMenu->addAction(tr("Not Annotated (No set info)")), &QAction::triggered, this,
            [deck] { DeckLoader::saveToClipboard(deck.deckList, false, false); });

    menu->addSeparator();

    // Rename file
    connect(menu->addAction(tr("Rename File")), &QAction::triggered, this, [this, src, deck] {
        const QFileInfo info(deck.lastLoadInfo.fileName);
        bool ok;
        QString newBase =
            QInputDialog::getText(this, tr("Rename file"), tr("New name:"), QLineEdit::Normal, info.baseName(), &ok);
        if (ok && !newBase.isEmpty() && newBase != info.baseName()) {
            if (!model_->renameFile(src, newBase))
                QMessageBox::critical(this, tr("Error"), tr("Rename failed"));
        }
    });

    // Delete file
    connect(menu->addAction(tr("Delete File")), &QAction::triggered, this, [this, src] {
        auto res =
            QMessageBox::warning(this, tr("Delete file"), tr("Are you sure you want to delete the selected file?"),
                                 QMessageBox::Yes | QMessageBox::No);
        if (res == QMessageBox::Yes) {
            if (!model_->deleteFile(src))
                QMessageBox::critical(this, tr("Error"), tr("Delete failed"));
        }
    });

    menu->popup(globalPos);
}

// ---------------------------------------------------------------------------
// retranslateUi
// ---------------------------------------------------------------------------

void VisualDeckStorageWidget::retranslateUi()
{
    databaseLoadIndicator_->setText(tr("Loading database …"));
    refreshButton_->setToolTip(tr("Refresh loaded files"));
    quickSettingsWidget_->setToolTip(tr("Visual Deck Storage Settings"));
}