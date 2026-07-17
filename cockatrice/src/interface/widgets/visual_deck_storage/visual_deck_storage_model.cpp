#include "visual_deck_storage_model.h"

#include "../../../client/settings/cache_settings.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <libcockatrice/card/database/card_database_manager.h>

VisualDeckStorageModel::VisualDeckStorageModel(QObject *parent) : QAbstractListModel(parent)
{
}

VisualDeckStorageModel::~VisualDeckStorageModel()
{
    for (auto &entry : entries) {
        delete entry.deckLoader;
    }
}

void VisualDeckStorageModel::setDeckPath(const QString &path)
{
    beginResetModel();

    for (auto &entry : entries) {
        delete entry.deckLoader;
    }
    entries.clear();

    deckPath = path;
    scanDirectory(path);

    endResetModel();
}

QString VisualDeckStorageModel::getDeckPath() const
{
    return deckPath;
}

void VisualDeckStorageModel::scanDirectory(const QString &path)
{
    QDirIterator it(path, DeckLoader::ACCEPTED_FILE_EXTENSIONS, QDir::Files,
                    QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

    while (it.hasNext()) {
        QString filePath = it.next();
        DeckEntry entry;
        entry.filePath = filePath;
        entry.deckLoader = new DeckLoader(nullptr);
        entry.lastModifiedTime = QFileInfo(filePath).lastModified();

        connectEntryLoader(entry, entries.size());
        entry.deckLoader->loadFromFileAsync(filePath, DeckFileFormat::getFormatFromName(filePath), false);

        entries.append(entry);
    }
}

void VisualDeckStorageModel::connectEntryLoader(DeckEntry &entry, int row)
{
    connect(entry.deckLoader, &DeckLoader::loadFinished, this, [this, row](bool success) {
        if (row >= 0 && row < entries.size()) {
            DeckEntry &entry = entries[row];
            if (success) {
                updateEntryCache(row);
            }
            entry.loaded = success;
            QModelIndex idx = index(row);
            emit dataChanged(idx, idx, {VDSModelRoles::LoadedRole});
            emit entryLoaded(row);
        }
    });
}

void VisualDeckStorageModel::updateEntryCache(int row)
{
    DeckEntry &entry = entries[row];
    const auto &deck = entry.deckLoader->getDeck();

    entry.tags = deck.deckList.getTags();

    QStringList cardList = deck.deckList.getCardList({DECK_ZONE_MAIN, DECK_ZONE_SIDE});
    QSet<QChar> colorSet;
    for (const QString &cardName : cardList) {
        CardInfoPtr currentCard = CardDatabaseManager::query()->getCardInfo(cardName);
        if (currentCard) {
            QString colors = currentCard->getColors();
            for (const QChar &color : colors) {
                colorSet.insert(color);
            }
        }
    }

    QString colorIdentity;
    const QString wubrgOrder = "WUBRG";
    for (const QChar &color : wubrgOrder) {
        if (colorSet.contains(color)) {
            colorIdentity.append(color);
        }
    }
    entry.colorIdentity = colorIdentity;
}

void VisualDeckStorageModel::refresh()
{
    setDeckPath(deckPath);
}

void VisualDeckStorageModel::reloadEntry(int row)
{
    if (row < 0 || row >= entries.size()) {
        return;
    }

    DeckEntry &entry = entries[row];
    QFileInfo fileInfo(entry.filePath);
    QDateTime newLastModifiedTime = fileInfo.lastModified();

    if (!newLastModifiedTime.isValid() || newLastModifiedTime <= entry.lastModifiedTime) {
        return;
    }

    bool success = entry.deckLoader->reload();
    if (success) {
        fileInfo.refresh();
        entry.lastModifiedTime = fileInfo.lastModified();
        updateEntryCache(row);
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    }
}

void VisualDeckStorageModel::renameDeck(int row, const QString &newName)
{
    if (row < 0 || row >= entries.size()) {
        return;
    }

    DeckEntry &entry = entries[row];
    entry.deckLoader->getDeck().deckList.setName(newName);
    DeckLoader::saveToFile(entry.deckLoader->getDeck());
    entry.lastModifiedTime = QFileInfo(entry.filePath).lastModified();
    updateEntryCache(row);

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {VDSModelRoles::DeckNameRole});
}

void VisualDeckStorageModel::renameFile(int row, const QString &newFileName)
{
    if (row < 0 || row >= entries.size()) {
        return;
    }

    DeckEntry &entry = entries[row];
    QFileInfo info(entry.filePath);
    const QString newFilePath = QFileInfo(info.dir(), newFileName).filePath();

    if (!QFile::rename(info.filePath(), newFilePath)) {
        return;
    }

    entry.filePath = newFilePath;
    entry.deckLoader->getDeck().lastLoadInfo.fileName = newFilePath;
    entry.lastModifiedTime = QFileInfo(newFilePath).lastModified();

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {VDSModelRoles::FilePathRole, VDSModelRoles::RelativeFilePathRole});
}

bool VisualDeckStorageModel::deleteFile(int row)
{
    if (row < 0 || row >= entries.size()) {
        return false;
    }

    const DeckEntry &entry = entries[row];
    if (!QFile::remove(QFileInfo(entry.filePath).filePath())) {
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row);
    DeckEntry removed = entries.takeAt(row);
    delete removed.deckLoader;
    endRemoveRows();

    return true;
}

void VisualDeckStorageModel::setTags(int row, const QStringList &tags)
{
    if (row < 0 || row >= entries.size()) {
        return;
    }

    DeckEntry &entry = entries[row];
    entry.deckLoader->getDeck().deckList.setTags(tags);
    DeckLoader::saveToFile(entry.deckLoader->getDeck());
    entry.tags = tags;
    entry.lastModifiedTime = QFileInfo(entry.filePath).lastModified();

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {VDSModelRoles::TagsRole});
}

void VisualDeckStorageModel::setBannerCard(int row, const CardRef &cardRef)
{
    if (row < 0 || row >= entries.size()) {
        return;
    }

    DeckEntry &entry = entries[row];
    entry.deckLoader->getDeck().deckList.setBannerCard(cardRef);
    DeckLoader::saveToFile(entry.deckLoader->getDeck());
    entry.lastModifiedTime = QFileInfo(entry.filePath).lastModified();

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {VDSModelRoles::BannerCardRole});
}

QStringList VisualDeckStorageModel::getAllTags() const
{
    QSet<QString> allTags;
    for (const DeckEntry &entry : entries) {
        if (entry.loaded) {
            for (const QString &tag : entry.tags) {
                allTags.insert(tag);
            }
        }
    }
    return allTags.values();
}

DeckEntry &VisualDeckStorageModel::entryAt(int row)
{
    return entries[row];
}

const DeckEntry &VisualDeckStorageModel::entryAt(int row) const
{
    return entries[row];
}

int VisualDeckStorageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return entries.size();
}

QVariant VisualDeckStorageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= entries.size()) {
        return {};
    }

    const DeckEntry &entry = entries[index.row()];
    const auto &deck = entry.deckLoader->getDeck();

    switch (role) {
        case VDSModelRoles::FilePathRole:
            return entry.filePath;
        case VDSModelRoles::DeckNameRole: {
            QString name = deck.deckList.getName();
            return !name.isEmpty() ? name : QFileInfo(entry.filePath).fileName();
        }
        case VDSModelRoles::ColorIdentityRole:
            return entry.colorIdentity;
        case VDSModelRoles::TagsRole:
            return entry.tags;
        case VDSModelRoles::LastModifiedTimeRole:
            return entry.lastModifiedTime;
        case VDSModelRoles::LastLoadedTimeRole:
            return QDateTime::fromString(deck.deckList.getLastLoadedTimestamp());
        case VDSModelRoles::GameFormatRole:
            return deck.deckList.getGameFormat();
        case VDSModelRoles::CommentsRole:
            return deck.deckList.getComments();
        case VDSModelRoles::RelativeFilePathRole:
            return getRelativeFilePath(entry.filePath);
        case VDSModelRoles::LoadedRole:
            return entry.loaded;
        case VDSModelRoles::DeckLoaderRole:
            return QVariant::fromValue(entry.deckLoader);
        case VDSModelRoles::BannerCardRole:
            return QVariant::fromValue(deck.deckList.getBannerCard());
        default:
            return {};
    }
}

QString VisualDeckStorageModel::getRelativeFilePath(const QString &filePath) const
{
    if (filePath.startsWith(deckPath)) {
        QString relativePath = filePath.mid(deckPath.length());
        if (relativePath.startsWith('/')) {
            relativePath.remove(0, 1);
        }
        return relativePath;
    }
    return QFileInfo(filePath).fileName();
}
