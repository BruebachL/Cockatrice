#include "deck_preview_widget.h"

#include "../../../../client/settings/cache_settings.h"
#include "../../cards/additional_info/color_identity_widget.h"
#include "../../cards/deck_preview_card_picture_widget.h"
#include "../visual_deck_storage_model.h"
#include "../visual_deck_storage_proxy_model.h"
#include "deck_preview_deck_tags_display_widget.h"

#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSet>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <libcockatrice/card/database/card_database_manager.h>

DeckPreviewWidget::DeckPreviewWidget(QWidget *_parent, VisualDeckStorageWidget *_visualDeckStorageWidget)
    : QWidget(_parent), visualDeckStorageWidget(_visualDeckStorageWidget)
{
    layout = new QVBoxLayout(this);
    setLayout(layout);

    bannerCardDisplayWidget = new DeckPreviewCardPictureWidget(
        this, false, SettingsCache::instance().getVisualDeckStorageSelectionAnimation());

    connect(bannerCardDisplayWidget, &DeckPreviewCardPictureWidget::imageClicked, this,
            &DeckPreviewWidget::imageClickedEvent);
    connect(bannerCardDisplayWidget, &DeckPreviewCardPictureWidget::imageDoubleClicked, this,
            &DeckPreviewWidget::imageDoubleClickedEvent);

    connect(&SettingsCache::instance(), &SettingsCache::visualDeckStorageShowColorIdentityChanged, this,
            &DeckPreviewWidget::updateColorIdentityVisibility);
    connect(&SettingsCache::instance(), &SettingsCache::visualDeckStorageShowTagsOnDeckPreviewsChanged, this,
            &DeckPreviewWidget::updateTagsVisibility);
    connect(&SettingsCache::instance(), &SettingsCache::visualDeckStorageShowBannerCardComboBoxChanged, this,
            &DeckPreviewWidget::updateBannerCardComboBoxVisibility);

    layout->addWidget(bannerCardDisplayWidget);
}

void DeckPreviewWidget::setModelIndex(const QModelIndex &proxyIndex)
{
    proxyModel = qobject_cast<QSortFilterProxyModel *>(const_cast<QAbstractItemModel *>(proxyIndex.model()));
    if (!proxyModel) {
        return;
    }
    sourceModelRow = proxyModel->mapToSource(proxyIndex).row();
    if (sourceModelRow < 0) {
        return;
    }

    auto *model = getSourceModel();
    if (!model) {
        qInfo() << "[VDS-DEBUG] setModelIndex: getSourceModel() returned nullptr!";
        return;
    }

    qInfo() << "[VDS-DEBUG] setModelIndex: sourceRow =" << sourceModelRow << "model rowCount =" << model->rowCount();
    if (sourceModelRow >= model->rowCount()) {
        return;
    }

    bool loaded = model->entryAt(sourceModelRow).loaded;
    qInfo() << "[VDS-DEBUG] setModelIndex: loaded =" << loaded;
    if (loaded) {
        initializeUi(true);
    } else {
        connect(model, &VisualDeckStorageModel::dataChanged, this,
                [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
                    Q_UNUSED(bottomRight);
                    if (topLeft.row() == sourceModelRow && roles.contains(VDSModelRoles::LoadedRole)) {
                        initializeUi(true);
                    }
                });
    }

    QString displayName = model->data(model->index(sourceModelRow, 0), VDSModelRoles::DeckNameRole).toString();
    bannerCardDisplayWidget->setFontSize(24);
    bannerCardDisplayWidget->setOverlayText(displayName);

    refreshBannerCardToolTip();
}

void DeckPreviewWidget::retranslateUi()
{
    if (bannerCardLabel) {
        bannerCardLabel->setText(tr("Banner Card"));
    }
}

void DeckPreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (bannerCardDisplayWidget == nullptr) {
        return;
    }
    const int maxWidth = bannerCardDisplayWidget->width();
    const QList<QWidget *> widgets = findChildren<QWidget *>();
    for (QWidget *widget : widgets) {
        if (widget == bannerCardDisplayWidget) {
            continue;
        }
        widget->setMaximumWidth(maxWidth);
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void DeckPreviewWidget::enterEvent(QEnterEvent *event)
#else
void DeckPreviewWidget::enterEvent(QEvent *event)
#endif
{
    QWidget::enterEvent(event);

    if (bannerCardComboBox != nullptr) {
        auto *model = getSourceModel();
        if (!model) {
            return;
        }
        model->reloadEntry(sourceRow());
        updateFromModel();
    }
}

int DeckPreviewWidget::sourceRow() const
{
    return sourceModelRow;
}

VisualDeckStorageModel *DeckPreviewWidget::getSourceModel() const
{
    if (!proxyModel) {
        return nullptr;
    }
    return qobject_cast<VisualDeckStorageModel *>(proxyModel->sourceModel());
}

void DeckPreviewWidget::initializeUi(const bool deckLoadSuccess)
{
    if (!deckLoadSuccess) {
        return;
    }

    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row < 0 || row >= model->rowCount()) {
        return;
    }

    if (colorIdentityWidget == nullptr) {
        colorIdentityWidget = new ColorIdentityWidget(this);
        layout->addWidget(colorIdentityWidget);
    }

    if (deckTagsDisplayWidget == nullptr) {
        deckTagsDisplayWidget = new DeckPreviewDeckTagsDisplayWidget(this);
        connect(deckTagsDisplayWidget, &DeckPreviewDeckTagsDisplayWidget::tagsChanged, this,
                &DeckPreviewWidget::setTags);
        layout->addWidget(deckTagsDisplayWidget);
    }

    if (bannerCardLabel == nullptr) {
        bannerCardLabel = new QLabel(this);
        bannerCardLabel->setObjectName("bannerCardLabel");
        layout->addWidget(bannerCardLabel);
    }

    if (bannerCardComboBox == nullptr) {
        bannerCardComboBox = new QComboBox(this);
        bannerCardComboBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        bannerCardComboBox->setObjectName("bannerCardComboBox");
        bannerCardComboBox->installEventFilter(new NoScrollFilter(bannerCardComboBox));
        connect(bannerCardComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                &DeckPreviewWidget::setBannerCard);
        layout->addWidget(bannerCardComboBox);
    }

    updateColorIdentityVisibility(SettingsCache::instance().getVisualDeckStorageShowColorIdentity());
    updateBannerCardComboBoxVisibility(SettingsCache::instance().getVisualDeckStorageShowBannerCardComboBox());
    updateTagsVisibility(SettingsCache::instance().getVisualDeckStorageShowTagsOnDeckPreviews());

    retranslateUi();
    resyncWidgets();
}

void DeckPreviewWidget::resyncWidgets()
{
    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row < 0 || row >= model->rowCount()) {
        return;
    }

    DeckLoader *loader = model->entryAt(row).deckLoader;
    auto bannerCardRef = loader->getDeck().deckList.getBannerCard();
    auto bannerCard = bannerCardRef.name.isEmpty() ? ExactCard() : CardDatabaseManager::query()->getCard(bannerCardRef);

    bannerCardDisplayWidget->setCard(bannerCard);
    refreshBannerCardText();
    updateBannerCardComboBox(bannerCardRef.name);
    colorIdentityWidget->setColorIdentity(model->entryAt(row).colorIdentity);
    deckTagsDisplayWidget->setTags(loader->getDeck().deckList.getTags());
}

void DeckPreviewWidget::updateFromModel()
{
    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row < 0 || row >= model->rowCount()) {
        return;
    }

    if (colorIdentityWidget) {
        colorIdentityWidget->setColorIdentity(model->entryAt(row).colorIdentity);
    }
    if (deckTagsDisplayWidget) {
        DeckLoader *loader = model->entryAt(row).deckLoader;
        deckTagsDisplayWidget->setTags(loader->getDeck().deckList.getTags());
    }
    refreshBannerCardText();
}

void DeckPreviewWidget::updateLastModifiedTime()
{
    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row >= 0 && row < model->rowCount()) {
        model->entryAt(row).lastModifiedTime = QFileInfo(model->entryAt(row).filePath).lastModified();
    }
}

void DeckPreviewWidget::writeDeckToFile()
{
    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row >= 0 && row < model->rowCount()) {
        DeckLoader::saveToFile(model->entryAt(row).deckLoader->getDeck());
        updateLastModifiedTime();
    }
}

void DeckPreviewWidget::refreshBannerCardText()
{
    bannerCardDisplayWidget->setOverlayText(getDisplayName());
    refreshBannerCardToolTip();
}

void DeckPreviewWidget::refreshBannerCardToolTip()
{
    auto tooltipType = SettingsCache::instance().getVisualDeckStorageTooltipType();
    switch (tooltipType) {
        case 0: // None
            bannerCardDisplayWidget->setToolTip("");
            break;
        case 1: // Filepath
            bannerCardDisplayWidget->setToolTip(getFilePath());
            break;
    }
}

QString DeckPreviewWidget::getDisplayName() const
{
    auto *model = getSourceModel();
    if (!model) {
        return {};
    }
    int row = sourceRow();
    if (row < 0 || row >= model->rowCount()) {
        return {};
    }
    return model->data(model->index(row, 0), VDSModelRoles::DeckNameRole).toString();
}

QString DeckPreviewWidget::getFilePath() const
{
    auto *model = getSourceModel();
    if (!model) {
        return {};
    }
    int row = sourceRow();
    if (row < 0 || row >= model->rowCount()) {
        return {};
    }
    return model->data(model->index(row, 0), VDSModelRoles::FilePathRole).toString();
}

DeckLoader *DeckPreviewWidget::getDeckLoader() const
{
    auto *model = getSourceModel();
    if (!model) {
        return nullptr;
    }
    int row = sourceRow();
    if (row >= 0 && row < model->rowCount()) {
        return model->entryAt(row).deckLoader;
    }
    return nullptr;
}

QStringList DeckPreviewWidget::getAllModelTags() const
{
    auto *model = getSourceModel();
    if (!model) {
        return {};
    }
    return model->getAllTags();
}

void DeckPreviewWidget::updateBannerCardComboBox(const QString &currentText)
{
    DeckLoader *loader = getDeckLoader();
    if (!loader) {
        return;
    }

    bool wasBlocked = bannerCardComboBox->blockSignals(true);
    bannerCardComboBox->setUpdatesEnabled(false);

    bannerCardComboBox->clear();

    QSet<QPair<QString, QString>> bannerCardSet;
    QList<const DecklistCardNode *> cardsInDeck = loader->getDeck().deckList.getCardNodes();

    for (auto currentCard : cardsInDeck) {
        for (int k = 0; k < currentCard->getNumber(); ++k) {
            bannerCardSet.insert(QPair<QString, QString>(currentCard->getName(), currentCard->getCardProviderId()));
        }
    }

    QList<QPair<QString, QString>> pairList = bannerCardSet.values();

    std::sort(pairList.begin(), pairList.end(), [](const QPair<QString, QString> &a, const QPair<QString, QString> &b) {
        return a.first.toLower() < b.first.toLower();
    });

    QStandardItemModel *model = new QStandardItemModel(pairList.size(), 1, bannerCardComboBox);

    int row = 0;
    for (const auto &pair : pairList) {
        QStandardItem *item = new QStandardItem(pair.first);
        item->setData(QVariant::fromValue(pair), Qt::UserRole);
        model->setItem(row++, 0, item);
    }

    bannerCardComboBox->setModel(model);

    int restoredIndex = bannerCardComboBox->findText(currentText);
    if (restoredIndex != -1) {
        bannerCardComboBox->setCurrentIndex(restoredIndex);
    } else {
        int bannerIndex = bannerCardComboBox->findText(loader->getDeck().deckList.getBannerCard().name);
        if (bannerIndex != -1) {
            bannerCardComboBox->setCurrentIndex(bannerIndex);
        } else {
            bannerCardComboBox->insertItem(0, "-");
            bannerCardComboBox->setCurrentIndex(0);
        }
    }

    bannerCardComboBox->blockSignals(wasBlocked);
    bannerCardComboBox->setUpdatesEnabled(true);
}

void DeckPreviewWidget::setBannerCard(int /* changedIndex */)
{
    auto [name, id] = bannerCardComboBox->currentData().value<QPair<QString, QString>>();
    CardRef cardRef = {name, id};

    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row >= 0 && row < model->rowCount()) {
        model->setBannerCard(row, cardRef);
        bannerCardDisplayWidget->setCard(CardDatabaseManager::query()->getCard(cardRef));
    }
}

void DeckPreviewWidget::imageClickedEvent(QMouseEvent *event, DeckPreviewCardPictureWidget *instance)
{
    Q_UNUSED(instance);

    if (event && event->button() == Qt::RightButton) {
        createRightClickMenu()->popup(QCursor::pos());
    }
}

void DeckPreviewWidget::imageDoubleClickedEvent(QMouseEvent *event, DeckPreviewCardPictureWidget *instance)
{
    Q_UNUSED(event);
    Q_UNUSED(instance);
    emit deckLoadRequested(getFilePath());
}

void DeckPreviewWidget::setTags(const QStringList &tags)
{
    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row >= 0 && row < model->rowCount()) {
        model->setTags(row, tags);
    }
}

QMenu *DeckPreviewWidget::createRightClickMenu()
{
    auto *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    connect(menu->addAction(tr("Open in deck editor")), &QAction::triggered, this, [this] {
        DeckLoader *loader = getDeckLoader();
        if (loader) {
            emit openDeckEditor(loader->getDeck());
        }
    });

    connect(menu->addAction(tr("Edit Tags")), &QAction::triggered, deckTagsDisplayWidget,
            &DeckPreviewDeckTagsDisplayWidget::openTagEditDlg);

    addSetBannerCardMenu(menu);

    menu->addSeparator();

    connect(menu->addAction(tr("Rename Deck")), &QAction::triggered, this, &DeckPreviewWidget::actRenameDeck);

    auto saveToClipboardMenu = menu->addMenu(tr("Save Deck to Clipboard"));

    connect(saveToClipboardMenu->addAction(tr("Annotated")), &QAction::triggered, this, [this] {
        DeckLoader *loader = getDeckLoader();
        if (loader) {
            DeckLoader::saveToClipboard(loader->getDeck().deckList, true, true);
        }
    });
    connect(saveToClipboardMenu->addAction(tr("Annotated (No set info)")), &QAction::triggered, this, [this] {
        DeckLoader *loader = getDeckLoader();
        if (loader) {
            DeckLoader::saveToClipboard(loader->getDeck().deckList, true, false);
        }
    });
    connect(saveToClipboardMenu->addAction(tr("Not Annotated")), &QAction::triggered, this, [this] {
        DeckLoader *loader = getDeckLoader();
        if (loader) {
            DeckLoader::saveToClipboard(loader->getDeck().deckList, false, true);
        }
    });
    connect(saveToClipboardMenu->addAction(tr("Not Annotated (No set info)")), &QAction::triggered, this, [this] {
        DeckLoader *loader = getDeckLoader();
        if (loader) {
            DeckLoader::saveToClipboard(loader->getDeck().deckList, false, false);
        }
    });

    menu->addSeparator();

    connect(menu->addAction(tr("Rename File")), &QAction::triggered, this, &DeckPreviewWidget::actRenameFile);

    connect(menu->addAction(tr("Delete File")), &QAction::triggered, this, &DeckPreviewWidget::actDeleteFile);

    return menu;
}

void DeckPreviewWidget::addSetBannerCardMenu(QMenu *menu)
{
    if (!bannerCardComboBox) {
        return;
    }

    auto bannerCardMenu = menu->addMenu(tr("Set Banner Card"));

    for (int i = 0; i < bannerCardComboBox->count(); ++i) {
        auto action = bannerCardMenu->addAction(bannerCardComboBox->itemText(i));
        connect(action, &QAction::triggered, this, [this, i] { bannerCardComboBox->setCurrentIndex(i); });

        action->setCheckable(true);
        action->setChecked(bannerCardComboBox->currentIndex() == i);
    }
}

void DeckPreviewWidget::actRenameDeck()
{
    DeckLoader *loader = getDeckLoader();
    if (!loader) {
        return;
    }

    const QString oldName = loader->getDeck().deckList.getName();

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename deck", tr("New name:"), QLineEdit::Normal, oldName, &ok);
    if (!ok || oldName == newName) {
        return;
    }

    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row >= 0 && row < model->rowCount()) {
        model->renameDeck(row, newName);
        refreshBannerCardText();
    }
}

void DeckPreviewWidget::actRenameFile()
{
    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row < 0 || row >= model->rowCount()) {
        return;
    }

    const QString filePath = model->data(model->index(row, 0), VDSModelRoles::FilePathRole).toString();
    const auto info = QFileInfo(filePath);
    const QString oldName = info.baseName();

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename file", tr("New name:"), QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.isEmpty() || oldName == newName) {
        return;
    }

    QString newFileName = newName;
    if (!info.suffix().isEmpty()) {
        newFileName += "." + info.suffix();
    }

    model->renameFile(row, newFileName);
    refreshBannerCardText();
}

void DeckPreviewWidget::actDeleteFile()
{
    auto res = QMessageBox::warning(this, tr("Delete file"), tr("Are you sure you want to delete the selected file?"),
                                    QMessageBox::Yes | QMessageBox::No);
    if (res != QMessageBox::Yes) {
        return;
    }

    auto *model = getSourceModel();
    if (!model) {
        return;
    }

    int row = sourceRow();
    if (row >= 0 && row < model->rowCount()) {
        model->deleteFile(row);
    }
}

void DeckPreviewWidget::updateColorIdentityVisibility(bool visible)
{
    if (colorIdentityWidget == nullptr) {
        return;
    }
    colorIdentityWidget->setVisible(visible);
}

void DeckPreviewWidget::updateBannerCardComboBoxVisibility(bool visible)
{
    if (bannerCardComboBox == nullptr) {
        return;
    }

    bannerCardComboBox->setVisible(visible);
    bannerCardLabel->setVisible(visible);
}

void DeckPreviewWidget::updateTagsVisibility(bool visible)
{
    if (deckTagsDisplayWidget == nullptr) {
        return;
    }
    deckTagsDisplayWidget->setVisible(visible);
}
