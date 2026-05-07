/**
 * @file deck_storage_model.cpp
 * @ingroup VisualDeckStorageWidgets
 */

#include "deck_storage_model.h"

#include "../../../client/settings/cache_settings.h"
#include "../../deck_loader/deck_loader.h"

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <libcockatrice/card/database/card_database_manager.h>
#include <libcockatrice/deck_list/tree/deck_list_card_node.h>

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

DeckStorageModel::DeckStorageModel(QObject *parent)
    : QAbstractItemModel(parent), root_(new DeckStorageItem(DeckStorageItem::Type::Root, {}))
{
}

DeckStorageModel::~DeckStorageModel()
{
    delete root_;
}

// ---------------------------------------------------------------------------
// populate
// ---------------------------------------------------------------------------

void DeckStorageModel::populate(const QString &rootPath, bool showFolders)
{
    beginResetModel();
    delete root_;
    root_ = new DeckStorageItem(DeckStorageItem::Type::Root, rootPath);
    allDiscoveredTags_.clear();
    buildTree(root_, rootPath, showFolders);
    endResetModel();

    scheduleLoadsUnder(root_);
}

// ---------------------------------------------------------------------------
// Tree building
// ---------------------------------------------------------------------------

static QStringList scanFiles(const QString &dirPath, bool recursive)
{
    QStringList result;
    auto flags =
        recursive ? QDirIterator::Subdirectories | QDirIterator::FollowSymlinks : QDirIterator::NoIteratorFlags;
    QDirIterator it(dirPath, DeckLoader::ACCEPTED_FILE_EXTENSIONS, QDir::Files, flags);
    while (it.hasNext())
        result << it.next();
    return result;
}

static QStringList scanSubFolders(const QString &dirPath)
{
    QStringList result;
    QDirIterator it(dirPath, QDir::Dirs | QDir::NoDotAndDotDot);
    while (it.hasNext())
        result << it.next();
    return result;
}

void DeckStorageModel::buildTree(DeckStorageItem *parentItem, const QString &path, bool showFolders)
{
    if (!showFolders) {
        // Flat mode: all files recursively under root_, no folder items
        for (const QString &file : scanFiles(path, true)) {
            auto *deck = new DeckStorageItem(DeckStorageItem::Type::Deck, file, parentItem);
            deck->lastModified = QFileInfo(file).lastModified();
            parentItem->children.append(deck);
        }
        return;
    }

    // Folder mode: direct files first (shown without a header in the view,
    // matching the original "root folder with hidden banner" behaviour),
    // then one Folder item per subdirectory.
    for (const QString &file : scanFiles(path, false)) {
        auto *deck = new DeckStorageItem(DeckStorageItem::Type::Deck, file, parentItem);
        deck->lastModified = QFileInfo(file).lastModified();
        parentItem->children.append(deck);
    }

    for (const QString &dir : scanSubFolders(path)) {
        auto *folder = new DeckStorageItem(DeckStorageItem::Type::Folder, dir, parentItem);
        // Recurse — supports arbitrary nesting
        buildTree(folder, dir, true);
        parentItem->children.append(folder);
    }
}

// ---------------------------------------------------------------------------
// Async loading
// ---------------------------------------------------------------------------

void DeckStorageModel::scheduleLoadsUnder(DeckStorageItem *item)
{
    for (DeckStorageItem *child : item->children) {
        if (child->type == DeckStorageItem::Type::Deck)
            scheduleLoad(child);
        else
            scheduleLoadsUnder(child);
    }
}

void DeckStorageModel::scheduleLoad(DeckStorageItem *item)
{
    auto *loader = new DeckLoader(this); // model owns the loader
    item->loader = loader;

    connect(loader, &DeckLoader::loadFinished, this, [this, item](bool ok) { onDeckLoaded(item, ok); });

    loader->loadFromFileAsync(item->filePath, DeckFileFormat::getFormatFromName(item->filePath), false);
}

void DeckStorageModel::onDeckLoaded(DeckStorageItem *item, bool ok)
{
    if (!ok || !item->loader)
        return;

    item->loadedDeck = item->loader->getDeck();
    item->colorIdentity = computeColorIdentity(item->loader);
    item->bannerCard = resolveBannerCard(item->loader);
    item->tags = item->loadedDeck.deckList.getTags();
    item->lastLoaded = QDateTime::fromString(item->loadedDeck.deckList.getLastLoadedTimestamp());
    item->loaded = true;

    // Display name: deck name, or filename fallback
    item->displayName = item->loadedDeck.deckList.getName();
    if (item->displayName.isEmpty())
        item->displayName = QFileInfo(item->filePath).fileName();

    // Accumulate discovered tags
    for (const QString &tag : item->tags) {
        if (!allDiscoveredTags_.contains(tag))
            allDiscoveredTags_.append(tag);
    }
    emit tagsUpdated(allDiscoveredTags_);

    // Connect card pixmap updates so the view repaints when an image arrives
    if (auto cardPtr = item->bannerCard.getCardPtr()) {
        connect(cardPtr.data(), &CardInfo::pixmapUpdated, this, [this, item] {
            QModelIndex idx = indexForItem(item);
            if (idx.isValid())
                emit dataChanged(idx, idx, {BannerCardRole});
        });
    }

    QModelIndex idx = indexForItem(item);
    if (idx.isValid())
        emit dataChanged(idx, idx);
}

// ---------------------------------------------------------------------------
// QAbstractItemModel interface
// ---------------------------------------------------------------------------

QModelIndex DeckStorageModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    DeckStorageItem *parentItem = parent.isValid() ? static_cast<DeckStorageItem *>(parent.internalPointer()) : root_;

    if (row < 0 || row >= parentItem->children.size())
        return {};

    return createIndex(row, column, parentItem->children[row]);
}

QModelIndex DeckStorageModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};

    auto *item = static_cast<DeckStorageItem *>(child.internalPointer());
    if (!item || !item->parent || item->parent == root_)
        return {};

    return createIndex(item->parent->row(), 0, item->parent);
}

int DeckStorageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    DeckStorageItem *parentItem = parent.isValid() ? static_cast<DeckStorageItem *>(parent.internalPointer()) : root_;

    return parentItem ? static_cast<int>(parentItem->children.size()) : 0;
}

int DeckStorageModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant DeckStorageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    auto *item = static_cast<DeckStorageItem *>(index.internalPointer());
    if (!item)
        return {};

    switch (role) {
        case Qt::DisplayRole: {
            if (item->type == DeckStorageItem::Type::Folder) {
                const QString deckPath = SettingsCache::instance().getDeckPath();
                QString relative = item->filePath;
                if (relative.startsWith(deckPath)) {
                    relative = relative.mid(deckPath.length());
                    if (relative.startsWith('/'))
                        relative.remove(0, 1);
                }
                return relative.isEmpty() ? tr("Deck Storage") : relative;
            }
            return item->displayName.isEmpty() ? QFileInfo(item->filePath).fileName() : item->displayName;
        }

        case DeckNameRole:
            return item->displayName;

        case FilePathRole:
            return item->filePath;

        case ColorIdentityRole:
            return item->colorIdentity;

        case TagsRole:
            return item->tags;

        case BannerCardRole:
            return QVariant::fromValue(item->bannerCard);

        case LastModifiedRole:
            return item->lastModified;

        case LastLoadedRole:
            return item->lastLoaded;

        case IsLoadedRole:
            return item->loaded;

        case IsFolderRole:
            return item->type == DeckStorageItem::Type::Folder;

        case IsCollapsedRole:
            return item->collapsed;

        default:
            return {};
    }
}

const LoadedDeck *DeckStorageModel::deckAt(const QModelIndex &index) const
{
    const auto *item = itemForIndex(index);
    if (!item || item->type != DeckStorageItem::Type::Deck)
        return nullptr;
    return &item->loadedDeck;
}

Qt::ItemFlags DeckStorageModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

DeckStorageItem *DeckStorageModel::itemForIndex(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<DeckStorageItem *>(index.internalPointer()) : root_;
}

QModelIndex DeckStorageModel::indexForItem(DeckStorageItem *item) const
{
    if (!item || item == root_ || !item->parent)
        return {};
    return createIndex(item->row(), 0, item);
}

// ---------------------------------------------------------------------------
// View-driven state
// ---------------------------------------------------------------------------

void DeckStorageModel::toggleCollapsed(const QModelIndex &index)
{
    auto *item = itemForIndex(index);
    if (!item || item->type != DeckStorageItem::Type::Folder)
        return;

    item->collapsed = !item->collapsed;
    emit dataChanged(index, index, {IsCollapsedRole});
}

// ---------------------------------------------------------------------------
// Mutation operations
// ---------------------------------------------------------------------------

void DeckStorageModel::writeDeckToFile(DeckStorageItem *item)
{
    DeckLoader::saveToFile(item->loadedDeck);
    item->lastModified = QFileInfo(item->filePath).lastModified();
}

void DeckStorageModel::renameDeck(const QModelIndex &index, const QString &newName)
{
    auto *item = itemForIndex(index);
    if (!item || item->type != DeckStorageItem::Type::Deck)
        return;

    item->loadedDeck.deckList.setName(newName);
    item->displayName = newName.isEmpty() ? QFileInfo(item->filePath).fileName() : newName;
    writeDeckToFile(item);

    emit dataChanged(index, index, {Qt::DisplayRole, DeckNameRole});
}

bool DeckStorageModel::renameFile(const QModelIndex &index, const QString &newBaseName)
{
    auto *item = itemForIndex(index);
    if (!item || item->type != DeckStorageItem::Type::Deck)
        return false;

    const QFileInfo info(item->filePath);
    QString newFileName = newBaseName;
    if (!info.suffix().isEmpty())
        newFileName += "." + info.suffix();

    const QString newFilePath = QFileInfo(info.dir(), newFileName).filePath();
    if (!QFile::rename(info.filePath(), newFilePath))
        return false;

    item->filePath = newFilePath;
    item->loadedDeck.lastLoadInfo.fileName = newFilePath;
    item->lastModified = QFileInfo(newFilePath).lastModified();

    if (item->displayName.isEmpty())
        item->displayName = newFileName;

    emit dataChanged(index, index, {FilePathRole, Qt::DisplayRole});
    return true;
}

bool DeckStorageModel::deleteFile(const QModelIndex &index)
{
    auto *item = itemForIndex(index);
    if (!item || item->type != DeckStorageItem::Type::Deck)
        return false;

    if (!QFile::remove(item->filePath))
        return false;

    DeckStorageItem *parentItem = item->parent;
    QModelIndex parentIndex = indexForItem(parentItem);
    const int row = item->row();

    beginRemoveRows(parentIndex, row, row);
    parentItem->children.removeAt(row);
    delete item;
    endRemoveRows();

    return true;
}

void DeckStorageModel::setBannerCard(const QModelIndex &index, const CardRef &ref)
{
    auto *item = itemForIndex(index);
    if (!item || item->type != DeckStorageItem::Type::Deck || !item->loaded)
        return;

    item->loadedDeck.deckList.setBannerCard(ref);
    item->bannerCard = ref.name.isEmpty() ? ExactCard() : CardDatabaseManager::query()->getCard(ref);
    writeDeckToFile(item);

    emit dataChanged(index, index, {BannerCardRole});
}

void DeckStorageModel::setTags(const QModelIndex &index, const QStringList &tags)
{
    auto *item = itemForIndex(index);
    if (!item || item->type != DeckStorageItem::Type::Deck || !item->loaded)
        return;

    item->tags = tags;
    item->loadedDeck.deckList.setTags(tags);
    writeDeckToFile(item);

    for (const QString &tag : tags) {
        if (!allDiscoveredTags_.contains(tag))
            allDiscoveredTags_.append(tag);
    }
    emit tagsUpdated(allDiscoveredTags_);
    emit dataChanged(index, index, {TagsRole});
}

void DeckStorageModel::reloadIfModified(const QModelIndex &index)
{
    auto *item = itemForIndex(index);
    if (!item || item->type != DeckStorageItem::Type::Deck || !item->loader)
        return;

    const QFileInfo info(item->filePath);
    if (!info.exists())
        return;

    if (info.lastModified() <= item->lastModified)
        return;

    if (item->loader->reload()) {
        item->loadedDeck = item->loader->getDeck();
        item->lastModified = QFileInfo(item->filePath).lastModified();
        item->displayName = item->loadedDeck.deckList.getName();
        if (item->displayName.isEmpty())
            item->displayName = QFileInfo(item->filePath).fileName();
        item->colorIdentity = computeColorIdentity(item->loader);
        item->bannerCard = resolveBannerCard(item->loader);
        item->tags = item->loadedDeck.deckList.getTags();

        emit dataChanged(index, index);
    }
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

QString DeckStorageModel::computeColorIdentity(DeckLoader *loader)
{
    const QStringList cardList = loader->getDeck().deckList.getCardList({DECK_ZONE_MAIN, DECK_ZONE_SIDE});
    if (cardList.isEmpty())
        return {};

    QSet<QChar> colorSet;
    for (const QString &name : cardList) {
        if (CardInfoPtr info = CardDatabaseManager::query()->getCardInfo(name))
            for (const QChar &c : info->getColors())
                colorSet.insert(c);
    }

    QString result;
    for (const QChar &c : QStringLiteral("WUBRG"))
        if (colorSet.contains(c))
            result += c;
    return result;
}

ExactCard DeckStorageModel::resolveBannerCard(DeckLoader *loader)
{
    CardRef ref = loader->getDeck().deckList.getBannerCard();
    if (!ref.name.isEmpty())
        return CardDatabaseManager::query()->getCard(ref);

    // Fall back to the first main-deck card
    const auto nodes = loader->getDeck().deckList.getCardNodes({DECK_ZONE_MAIN});
    if (!nodes.isEmpty()) {
        const QString name = nodes.first()->getName();
        const QString id = nodes.first()->getCardProviderId();
        return CardDatabaseManager::query()->getCard({name, id});
    }
    return ExactCard();
}