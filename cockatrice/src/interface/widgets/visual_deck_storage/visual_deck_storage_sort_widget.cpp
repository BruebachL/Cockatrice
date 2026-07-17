#include "visual_deck_storage_sort_widget.h"

#include "../../../client/settings/cache_settings.h"
#include "visual_deck_storage_widget.h"

VisualDeckStorageSortWidget::VisualDeckStorageSortWidget(VisualDeckStorageWidget *parent)
    : parent(parent)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    sortComboBox = new QComboBox(this);
    layout->addWidget(sortComboBox);

    retranslateUi();

    sortComboBox->setCurrentIndex(SettingsCache::instance().getVisualDeckStorageSortingOrder());

    connect(sortComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &VisualDeckStorageSortWidget::sortOrderChanged);
    connect(this, &VisualDeckStorageSortWidget::sortOrderChanged, parent, &VisualDeckStorageWidget::updateSortOrder);
}

void VisualDeckStorageSortWidget::retranslateUi()
{
    sortComboBox->blockSignals(true);

    int oldIndex = sortComboBox->currentIndex();

    sortComboBox->clear();
    sortComboBox->addItem(tr("Sort Alphabetically (Deck Name)"), VisualDeckStorageProxyModel::ByName);
    sortComboBox->addItem(tr("Sort Alphabetically (Filename)"), VisualDeckStorageProxyModel::Alphabetical);
    sortComboBox->addItem(tr("Sort by Last Modified"), VisualDeckStorageProxyModel::ByLastModified);
    sortComboBox->addItem(tr("Sort by Last Loaded"), VisualDeckStorageProxyModel::ByLastLoaded);

    sortComboBox->setCurrentIndex(oldIndex);

    sortComboBox->blockSignals(false);
}

void VisualDeckStorageSortWidget::applySortOrder(VisualDeckStorageProxyModel *proxyModel)
{
    auto order = static_cast<VisualDeckStorageProxyModel::SortOrder>(sortComboBox->currentIndex());
    SettingsCache::instance().setVisualDeckStorageSortingOrder(sortComboBox->currentIndex());
    proxyModel->setSortOrder(order);
}
