/**
 * @file deck_storage_model.h
 * @ingroup VisualDeckStorageWidgets
 * @brief Tree model backing the Visual Deck Storage view.
 *
 * Top-level items are either:
 *  - DeckStorageItem::Type::Folder  (when showFolders=true)
 *  - DeckStorageItem::Type::Deck    (when showFolders=false, flat list)
 *
 * Folder items have Deck children. Deck items are always leaves.
 * Decks are loaded asynchronously; the model emits dataChanged once each deck
 * finishes loading so the view can repaint the cell.
 */

#ifndef DECK_STORAGE_MODEL_H
#define DECK_STORAGE_MODEL_H

#include "../../deck_loader/loaded_deck.h"

#include <QAbstractItemModel>
#include <QDateTime>
#include <QStringList>
#include <libcockatrice/card/database/card_database_manager.h>

class DeckLoader;

// ---------------------------------------------------------------------------
// DeckStorageItem — internal tree node
// ---------------------------------------------------------------------------

struct DeckStorageItem
{
    enum class Type
    {
        Root,
        Folder,
        Deck
    };

    Type type;
    QString filePath;

    // Folder-only state
    bool collapsed = false;

    // Deck-only — populated after async load
    LoadedDeck loadedDeck;
    ExactCard bannerCard;
    QString displayName;   // deck name, or filename if name is empty
    QString colorIdentity; // WUBRG subset string, e.g. "WUB"
    QStringList tags;
    QDateTime lastModified;
    QDateTime lastLoaded;
    bool loaded = false;
    DeckLoader *loader = nullptr; // owned by DeckStorageModel (as QObject parent)

    // Tree linkage
    DeckStorageItem *parent = nullptr;
    QVector<DeckStorageItem *> children;

    DeckStorageItem(Type t, const QString &path, DeckStorageItem *p = nullptr) : type(t), filePath(path), parent(p)
    {
    }

    ~DeckStorageItem()
    {
        qDeleteAll(children);
    }

    int row() const
    {
        if (parent)
            return static_cast<int>(parent->children.indexOf(const_cast<DeckStorageItem *>(this)));
        return 0;
    }
};

// ---------------------------------------------------------------------------
// DeckStorageModel
// ---------------------------------------------------------------------------

class DeckStorageModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles
    {
        FilePathRole = Qt::UserRole + 1,
        DeckNameRole = Qt::UserRole + 2,
        ColorIdentityRole = Qt::UserRole + 3,
        TagsRole = Qt::UserRole + 4,
        BannerCardRole = Qt::UserRole + 5, // ExactCard
        LastModifiedRole = Qt::UserRole + 6,
        LastLoadedRole = Qt::UserRole + 7,
        IsLoadedRole = Qt::UserRole + 8,
        IsFolderRole = Qt::UserRole + 9,
        IsCollapsedRole = Qt::UserRole + 10,
    };

    explicit DeckStorageModel(QObject *parent = nullptr);
    ~DeckStorageModel() override;

    /**
     * Scans @p rootPath and (re)populates the model.
     * When @p showFolders is true the tree mirrors the directory hierarchy.
     * When false all decks appear as flat top-level items.
     */
    void populate(const QString &rootPath, bool showFolders);

    // Direct typed accessor — avoids QVariant round-trip for LoadedDeck
    const LoadedDeck *deckAt(const QModelIndex &index) const;

    // --- QAbstractItemModel interface ---
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    DeckStorageItem *itemForIndex(const QModelIndex &index) const;
    QModelIndex indexForItem(DeckStorageItem *item) const;

    // --- View-driven state ---
    void toggleCollapsed(const QModelIndex &index);

    // --- Mutation operations (write-through to disk) ---
    void renameDeck(const QModelIndex &index, const QString &newName);
    bool renameFile(const QModelIndex &index, const QString &newBaseName);
    bool deleteFile(const QModelIndex &index);
    void setBannerCard(const QModelIndex &index, const CardRef &ref);
    void setTags(const QModelIndex &index, const QStringList &tags);

    /** Re-reads the file if it has been modified since last load. */
    void reloadIfModified(const QModelIndex &index);

    /** All unique tags seen across loaded decks. */
    QStringList allTags() const
    {
        return allDiscoveredTags_;
    }

signals:
    /** Emitted (from main thread) whenever the full tag set changes. */
    void tagsUpdated(const QStringList &allTags);

private:
    void buildTree(DeckStorageItem *parentItem, const QString &path, bool showFolders);
    void scheduleLoadsUnder(DeckStorageItem *item);
    void scheduleLoad(DeckStorageItem *item);
    void onDeckLoaded(DeckStorageItem *item, bool ok);
    void writeDeckToFile(DeckStorageItem *item);

    static QString computeColorIdentity(DeckLoader *loader);
    static ExactCard resolveBannerCard(DeckLoader *loader);

    DeckStorageItem *root_;
    QStringList allDiscoveredTags_;
};

#endif // DECK_STORAGE_MODEL_H