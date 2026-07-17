#include "visual_deck_storage_folder_display_widget.h"

#include "../../../client/settings/cache_settings.h"
#include "deck_preview/deck_preview_widget.h"
#include "visual_deck_storage_model.h"
#include "visual_deck_storage_proxy_model.h"
#include "visual_deck_storage_widget.h"

#include <QDir>
#include <QDirIterator>
#include <QMouseEvent>

VisualDeckStorageFolderDisplayWidget::VisualDeckStorageFolderDisplayWidget(
    QWidget *parent,
    VisualDeckStorageWidget *_visualDeckStorageWidget,
    QString _filePath,
    bool canBeHidden,
    bool _showFolders)
    : QWidget(parent), showFolders(_showFolders), visualDeckStorageWidget(_visualDeckStorageWidget), filePath(_filePath)
{
    layout = new QVBoxLayout(this);
    setLayout(layout);

    header = new BannerWidget(this, "");
    header->setClickable(canBeHidden);
    header->setHidden(!showFolders);
    layout->addWidget(header);

    container = new QWidget(this);
    containerLayout = new QVBoxLayout(container);
    container->setLayout(containerLayout);

    header->setBuddy(container);

    layout->addWidget(container);

    flowWidget = new FlowWidget(this, Qt::Horizontal, Qt::ScrollBarAlwaysOff, Qt::ScrollBarAlwaysOff);
    containerLayout->addWidget(flowWidget);

    createWidgetsForFiles();
    createWidgetsForFolders();

    refreshUi();
}

void VisualDeckStorageFolderDisplayWidget::refreshUi()
{
    QString bannerText = tr("Deck Storage");
    QString deckPath = SettingsCache::instance().getDeckPath();
    if (filePath != deckPath) {
        QString relativePath = filePath;

        if (filePath.startsWith(deckPath)) {
            relativePath = filePath.mid(deckPath.length());
            if (relativePath.startsWith('/')) {
                relativePath.remove(0, 1);
            }
        }

        bannerText = relativePath;
    }
    header->setText(bannerText);
}

void VisualDeckStorageFolderDisplayWidget::createWidgetsForFiles()
{
    auto *proxyModel = visualDeckStorageWidget->deckStorageProxyModel;

    qInfo() << "[VDS-DEBUG] createWidgetsForFiles() showFolders =" << showFolders << "filePath =" << filePath
            << "proxy rowCount =" << proxyModel->rowCount();

    int created = 0;
    int skipped = 0;
    for (int i = 0; i < proxyModel->rowCount(); ++i) {
        QModelIndex proxyIndex = proxyModel->index(i, 0);
        QString entryFilePath = proxyModel->data(proxyIndex, VDSModelRoles::FilePathRole).toString();
        QFileInfo fileInfo(entryFilePath);
        QString entryParentDir = fileInfo.path();

        bool belongsHere = false;
        if (showFolders) {
            belongsHere = (QDir::cleanPath(entryParentDir) == QDir::cleanPath(filePath));
        } else {
            belongsHere = true;
        }

        if (!belongsHere) {
            ++skipped;
            continue;
        }

        auto *display = new DeckPreviewWidget(flowWidget, visualDeckStorageWidget);
        display->setModelIndex(proxyIndex);

        connect(display, &DeckPreviewWidget::deckLoadRequested, visualDeckStorageWidget,
                &VisualDeckStorageWidget::deckLoadRequested);
        connect(display, &DeckPreviewWidget::openDeckEditor, visualDeckStorageWidget,
                &VisualDeckStorageWidget::openDeckEditor);

        flowWidget->addWidget(display);
        ++created;
    }
    qInfo() << "[VDS-DEBUG] createWidgetsForFiles() created =" << created << "skipped =" << skipped
            << "flowWidget count =" << flowWidget->count();
}

void VisualDeckStorageFolderDisplayWidget::updateVisibility(bool recursive)
{
    bool atLeastOneWidgetVisible = checkVisibility();
    if (atLeastOneWidgetVisible) {
        setVisible(true);
        if (recursive) {
            for (auto *subFolder : findChildren<VisualDeckStorageFolderDisplayWidget *>()) {
                subFolder->updateVisibility(false);
            }
        }
    } else {
        setVisible(false);
    }
}

bool VisualDeckStorageFolderDisplayWidget::checkVisibility()
{
    bool atLeastOneWidgetVisible = false;
    if (flowWidget) {
        for (int i = 0; i < flowWidget->count(); ++i) {
            QLayoutItem *item = flowWidget->itemAt(i);
            if (item && item->widget() && item->widget()->isVisible()) {
                atLeastOneWidgetVisible = true;
                break;
            }
        }
    }
    if (!atLeastOneWidgetVisible) {
        for (VisualDeckStorageFolderDisplayWidget *subFolder : findChildren<VisualDeckStorageFolderDisplayWidget *>()) {
            if (subFolder->isVisible()) {
                atLeastOneWidgetVisible = true;
                break;
            }
        }
    }
    return atLeastOneWidgetVisible;
}

void VisualDeckStorageFolderDisplayWidget::createWidgetsForFolders()
{
    if (!showFolders) {
        return;
    }

    QString deckPath = SettingsCache::instance().getDeckPath();
    QDirIterator it(filePath, QDir::Dirs | QDir::NoDotAndDotDot);

    while (it.hasNext()) {
        QString dir = it.next();
        auto *display = new VisualDeckStorageFolderDisplayWidget(this, visualDeckStorageWidget, dir, true, showFolders);
        containerLayout->addWidget(display);
    }
}

void VisualDeckStorageFolderDisplayWidget::updateShowFolders(bool enabled)
{
    showFolders = enabled;

    if (!showFolders) {
        flattenFolderStructure();
    } else {
        createWidgetsForFiles();
        createWidgetsForFolders();
    }

    header->setHidden(!showFolders);
}

void VisualDeckStorageFolderDisplayWidget::flattenFolderStructure()
{
    for (auto *subFolder : findChildren<VisualDeckStorageFolderDisplayWidget *>()) {
        for (auto *deck : subFolder->getFlowWidget()->findChildren<DeckPreviewWidget *>()) {
            flowWidget->addWidget(deck);
        }
        subFolder->deleteLater();
    }
}

QStringList VisualDeckStorageFolderDisplayWidget::gatherAllTagsFromFlowWidget() const
{
    auto *model = visualDeckStorageWidget->deckStorageModel;
    return model->getAllTags();
}
