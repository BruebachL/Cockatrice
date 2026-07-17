/**
 * @file visual_deck_storage_model.h
 * @ingroup VisualDeckStorageWidgets
 * @brief Model holding all deck data for the Visual Deck Storage.
 *
 * Centralizes deck loading and metadata caching independently of any widgets.
 * Each deck entry owns a DeckLoader that loads asynchronously. Computed data
 * (color identity, tags) is cached and exposed via Qt model roles.
 */

#ifndef VISUAL_DECK_STORAGE_MODEL_H
#define VISUAL_DECK_STORAGE_MODEL_H

#include "../../deck_loader/deck_loader.h"

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <libcockatrice/utility/card_ref.h>

namespace VDSModelRoles
{
enum
{
    FilePathRole = Qt::UserRole + 1,
    DeckNameRole,
    ColorIdentityRole,
    TagsRole,
    LastModifiedTimeRole,
    LastLoadedTimeRole,
    GameFormatRole,
    CommentsRole,
    RelativeFilePathRole,
    LoadedRole,
    DeckLoaderRole,
    BannerCardRole,
};
}

struct DeckEntry
{
    QString filePath;
    DeckLoader *deckLoader;
    QDateTime lastModifiedTime;
    QDateTime lastLoadedTime;
    QString colorIdentity;
    QStringList tags;
    bool loaded = false;
};

class VisualDeckStorageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit VisualDeckStorageModel(QObject *parent = nullptr);
    ~VisualDeckStorageModel() override;

    void setDeckPath(const QString &path);
    [[nodiscard]] QString getDeckPath() const;

    void refresh();
    void reloadEntry(int row);

    void renameDeck(int row, const QString &newName);
    void renameFile(int row, const QString &newFileName);
    bool deleteFile(int row);
    void setTags(int row, const QStringList &tags);
    void setBannerCard(int row, const CardRef &cardRef);

    [[nodiscard]] QStringList getAllTags() const;
    [[nodiscard]] DeckEntry &entryAt(int row);
    [[nodiscard]] const DeckEntry &entryAt(int row) const;

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QString getRelativeFilePath(const QString &filePath) const;

signals:
    void entryLoaded(int row);

private:
    void scanDirectory(const QString &path);
    void connectEntryLoader(DeckEntry &entry, int row);
    void updateEntryCache(int row);

    QList<DeckEntry> entries;
    QString deckPath;
};

#endif // VISUAL_DECK_STORAGE_MODEL_H
