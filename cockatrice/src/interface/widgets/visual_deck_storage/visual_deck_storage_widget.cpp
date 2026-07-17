#include "visual_deck_storage_widget.h"

#include "../../../client/settings/cache_settings.h"
#include "../quick_settings/settings_button_widget.h"
#include "deck_preview/deck_preview_widget.h"
#include "visual_deck_storage_folder_display_widget.h"
#include "visual_deck_storage_search_widget.h"
#include "visual_deck_storage_sort_widget.h"
#include "visual_deck_storage_tag_filter_widget.h"

#include <QComboBox>
#include <QDirIterator>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <libcockatrice/card/database/card_database_manager.h>

VisualDeckStorageWidget::VisualDeckStorageWidget(QWidget *parent)
    : QWidget(parent), folderWidget(nullptr), rebuildTimer(new QTimer(this))
{
    deckStorageModel = new VisualDeckStorageModel(this);
    deckStorageProxyModel = new VisualDeckStorageProxyModel(this);
    deckStorageProxyModel->setSourceModel(deckStorageModel);

    rebuildTimer->setSingleShot(true);
    rebuildTimer->setInterval(50);

    layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(9, 0, 9, 5);
    setLayout(layout);

    // search bar row
    searchAndSortContainer = new QWidget(this);
    searchAndSortLayout = new QHBoxLayout(searchAndSortContainer);
    searchAndSortLayout->setSpacing(3);
    searchAndSortLayout->setContentsMargins(9, 0, 9, 0);
    searchAndSortContainer->setLayout(searchAndSortLayout);

    deckPreviewColorIdentityFilterWidget = new DeckPreviewColorIdentityFilterWidget(this);
    sortWidget = new VisualDeckStorageSortWidget(this);
    searchWidget = new VisualDeckStorageSearchWidget(this);

    refreshButton = new QToolButton(this);
    refreshButton->setIcon(QPixmap("theme:icons/reload"));
    refreshButton->setFixedSize(32, 32);
    connect(refreshButton, &QPushButton::clicked, this, &VisualDeckStorageWidget::refreshIfPossible);

    quickSettingsWidget = new VisualDeckStorageQuickSettingsWidget(this);
    connect(quickSettingsWidget, &VisualDeckStorageQuickSettingsWidget::showFoldersChanged, this,
            &VisualDeckStorageWidget::updateShowFolders);
    connect(quickSettingsWidget, &VisualDeckStorageQuickSettingsWidget::showTagFilterChanged, this,
            &VisualDeckStorageWidget::updateTagsVisibility);

    searchAndSortLayout->addWidget(deckPreviewColorIdentityFilterWidget);
    searchAndSortLayout->addWidget(sortWidget);
    searchAndSortLayout->addWidget(searchWidget);
    searchAndSortLayout->addWidget(refreshButton);
    searchAndSortLayout->addWidget(quickSettingsWidget);

    // tag filter box
    tagFilterWidget = new VisualDeckStorageTagFilterWidget(this);
    updateTagsVisibility(SettingsCache::instance().getVisualDeckStorageShowTagFilter());

    // deck area
    scrollArea = new QScrollArea(this);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);

    // putting everything together
    layout->addWidget(searchAndSortContainer);
    layout->addWidget(tagFilterWidget);
    layout->addWidget(scrollArea);

    connect(CardDatabaseManager::getInstance(), &CardDatabase::cardDatabaseLoadingFinished, this,
            &VisualDeckStorageWidget::loadModel);

    connect(deckStorageModel, &VisualDeckStorageModel::entryLoaded, this, [this](int row) {
        qInfo() << "[VDS-DEBUG] entryLoaded signal fired for row" << row << "folderWidget set?" << (folderWidget != nullptr);
        if (folderWidget) {
            rebuildTimer->start();
        }
    });

    databaseLoadIndicator = new QLabel(this);
    databaseLoadIndicator->setAlignment(Qt::AlignCenter);

    retranslateUi();

    if (CardDatabaseManager::getInstance()->getLoadStatus() == LoadStatus::Ok) {
        loadModel();
        databaseLoadIndicator->setVisible(false);
    } else {
        scrollArea->setWidget(databaseLoadIndicator);
    }
}

void VisualDeckStorageWidget::refreshIfPossible()
{
    if (scrollArea->widget() != databaseLoadIndicator) {
        loadModel();
    }
}

void VisualDeckStorageWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (scrollArea->widget() == folderWidget) {
        scrollArea->widget()->setMaximumWidth(scrollArea->viewport()->width());
    }
}

void VisualDeckStorageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (scrollArea->widget() == folderWidget) {
        scrollArea->widget()->setMaximumWidth(scrollArea->viewport()->width());
    }
}

void VisualDeckStorageWidget::retranslateUi()
{
    databaseLoadIndicator->setText(tr("Loading database ..."));

    refreshButton->setToolTip(tr("Refresh loaded files"));
    quickSettingsWidget->setToolTip(tr("Visual Deck Storage Settings"));
}

const VisualDeckStorageQuickSettingsWidget *VisualDeckStorageWidget::settings() const
{
    return quickSettingsWidget;
}

void VisualDeckStorageWidget::loadModel()
{
    qInfo() << "[VDS-DEBUG] loadModel() called, deckPath =" << SettingsCache::instance().getDeckPath();
    deckStorageModel->setDeckPath(SettingsCache::instance().getDeckPath());
    qInfo() << "[VDS-DEBUG] loadModel() source model rowCount =" << deckStorageModel->rowCount();
    rebuildFolderWidget();
}

void VisualDeckStorageWidget::rebuildFolderWidget()
{
    qInfo() << "[VDS-DEBUG] rebuildFolderWidget() before sort: source rowCount ="
             << deckStorageModel->rowCount() << "proxy rowCount =" << deckStorageProxyModel->rowCount();
    deckStorageProxyModel->sort(0);
    qInfo() << "[VDS-DEBUG] rebuildFolderWidget() after sort: proxy rowCount ="
             << deckStorageProxyModel->rowCount();

    folderWidget = new VisualDeckStorageFolderDisplayWidget(this, this, SettingsCache::instance().getDeckPath(), false,
                                                            quickSettingsWidget->getShowFolders());

    scrollArea->setWidget(folderWidget);
    scrollArea->widget()->setMaximumWidth(scrollArea->viewport()->width());

    QTimer::singleShot(0, this, [this]() {
        updateSortOrder();
        updateTagFilter();
        updateColorFilter();
        updateSearchFilter();
    });
}

void VisualDeckStorageWidget::updateShowFolders(bool enabled)
{
    if (folderWidget) {
        folderWidget->updateShowFolders(enabled);
        QTimer::singleShot(0, this, [this]() {
            updateSortOrder();
            updateTagFilter();
            updateColorFilter();
            updateSearchFilter();
        });
    }
}

void VisualDeckStorageWidget::updateSortOrder()
{
    if (sortWidget) {
        sortWidget->applySortOrder(deckStorageProxyModel);
    }
}

void VisualDeckStorageWidget::updateTagFilter()
{
    if (tagFilterWidget && folderWidget) {
        tagFilterWidget->updateFilterFromModel();
        folderWidget->updateVisibility();
    }
}

void VisualDeckStorageWidget::updateColorFilter()
{
    if (deckPreviewColorIdentityFilterWidget && folderWidget) {
        deckPreviewColorIdentityFilterWidget->applyColorFilter(deckStorageProxyModel);
        folderWidget->updateVisibility();
    }
}

void VisualDeckStorageWidget::updateSearchFilter()
{
    if (searchWidget && folderWidget) {
        searchWidget->applySearchFilter(deckStorageProxyModel);
        folderWidget->updateVisibility();
    }
}

void VisualDeckStorageWidget::updateTagsVisibility(const bool visible)
{
    tagFilterWidget->setVisible(visible);
}
