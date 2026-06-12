#ifndef COCKATRICE_GAMES_LIST_ITEM_DELEGATE_H
#define COCKATRICE_GAMES_LIST_ITEM_DELEGATE_H

#include "game_type_map.h"

#include <QMap>
#include <QPixmap>
#include <QStyledItemDelegate>

class ServerInfo_Game;

/**
 * @class GameListItemDelegate
 * @brief Paints each game in the TilingListView as a compact single-row card.
 *
 * Each card is 48 px tall.  All attribute zones are always present at fixed
 * pixel widths — there are no conditional zones, so the layout never shifts
 * and every column is always scannable regardless of card width.
 *
 * Zone layout (left → right, all always visible):
 *
 * @code
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │▌  (●)  │  Game Title (flex)  │  Creator  │  Types  │  👥 N/M  │·│  age  │
 * └──────────────────────────────────────────────────────────────────────────┘
 * @endcode
 *
 * Title is the only flexible zone; it receives all width not consumed by the
 * fixed zones.  minCardW in TilingListView is set high enough (500 px) that
 * every zone always has comfortable room.
 *
 * Avatar: real image from m_avatarCache when available; neutral dark fill
 *         plus the creator's pawn otherwise.  Accent ring encodes join state:
 *         green = open · blue = password · orange = full · red = in progress.
 *
 * Type badge colours derive from a stable djb2 hash of the type name so the
 * same type always maps to the same palette slot across sessions.
 */
class GameListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    // ── Card chrome ───────────────────────────────────────────────────────────
    static constexpr int cardMarginH = 4;
    static constexpr int cardMarginV = 3;
    static constexpr int cardRadius = 7;
    static constexpr int accentBarWidth = 4;
    static constexpr int accentBarRadius = 2;

    // ── Left-anchored zones (left-to-right, before title) ─────────────────────
    static constexpr int leftPad = 8; ///< rect.left() → age left (just clears accent bar)
    static constexpr int zoneAgeW = 44;
    static constexpr int avatarGap = 6; ///< age right → avatar left
    static constexpr int avatarSize = 36;
    static constexpr int avatarFallbackPawn = 26;
    static constexpr int creatorGap = 8; ///< avatar right → creator left
    static constexpr int zoneCreatorW = 100;
    static constexpr int lockSize = 13;
    static constexpr int lockTextGap = 17;

    // ── Right-anchored zones (right-to-left, after title) ─────────────────────
    static constexpr int rightPad = 8;
    static constexpr int zoneTypesW = 90;
    static constexpr int zoneStatusW = 110; ///< Players + spectator + restriction pills
    static constexpr int interZoneGap = 8;

    // ── Badge geometry ────────────────────────────────────────────────────────
    static constexpr int badgeH = 14;
    static constexpr int badgeHPad = 5;
    static constexpr int badgeGap = 4;
    static constexpr int badgeRadius = 3;

    static constexpr double badgeFontScale = 0.72;

    const QMap<int, QString> rooms;
    const QMap<int, GameTypeMap> gameTypes;
    const QMap<QString, QPixmap> *m_avatarCache; ///< Non-owning; null = pawn fallback always

public:
    /**
     * @param avatarCache  Pointer to the live username→pixmap cache kept by
     *                     UserListManager (or null for pawn-only fallback).
     *                     The map must outlive this delegate.
     */
    GameListItemDelegate(const QMap<int, QString> &rooms,
                         const QMap<int, GameTypeMap> &gameTypes,
                         const QMap<QString, QPixmap> *avatarCache,
                         QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    /** Returns the accent colour encoding the game's current join state. */
    static QColor accentForGame(const ServerInfo_Game &game);

    /**
     * Maps a game-type display name to a stable bg/fg colour pair via djb2 hash.
     * Deterministic across sessions — independent of Qt's per-process qHash seed.
     */
    static void typeColors(const QString &name, QColor &outBg, QColor &outFg);

    /** Draws the rounded card background gradient and left accent bar. */
    void paintCardBackground(QPainter *p, const QRectF &card, const QColor &accent, bool selected) const;

    /**
     * Draws the 36 px circular creator avatar.
     * Real image used when m_avatarCache has an entry for the creator username;
     * falls back to a neutral dark fill with the creator's pawn at
     * avatarFallbackPawn px.  Accent-coloured ring drawn in both cases.
     */
    void
    paintCreatorAvatar(QPainter *p, const QRect &avatarRect, const ServerInfo_Game &game, const QColor &accent) const;

    /**
     * Zone: bold game title occupying all flexible width left of the creator
     * zone.  Optional lock icon is prepended for password-protected games.
     * Title elides if it exceeds titleW.
     */
    void paintTitle(QPainter *p,
                    const QStyleOptionViewItem &option,
                    const QRect &rect,
                    const ServerInfo_Game &game,
                    int lx,
                    int titleW,
                    const QFont &titleFont) const;

    /** Zone: creator name (regular weight, dimmer colour), elided to zoneCreatorW. */
    void paintCreator(QPainter *p,
                      const QStyleOptionViewItem &option,
                      const QRect &rect,
                      const ServerInfo_Game &game,
                      int creatorLeft) const;

    /**
     * Zone: game-type pill badges drawn left-to-right within zoneTypesW.
     * Badges that exceed the zone width are silently omitted.
     */
    void paintTypeBadges(QPainter *p,
                         const QRect &rect,
                         const ServerInfo_Game &game,
                         int typesLeft,
                         const QFont &badgeFont) const;

    /**
     * Zone: player count, spectator mode, and restriction tags rendered as
     * pill badges left-to-right within zoneStatusW.  Badges that exceed the
     * zone are silently omitted.  Colour encodes join state: red = in progress,
     * orange = full, blue = password, grey = open.
     */
    void paintStatus(QPainter *p,
                     const QRect &rect,
                     const ServerInfo_Game &game,
                     int statusLeft,
                     const QColor &accent,
                     const QFont &badgeFont) const;

    /** Zone: creation age string right-aligned within zoneAgeW. */
    void paintAge(QPainter *p,
                  const QStyleOptionViewItem &option,
                  const QRect &rect,
                  const ServerInfo_Game &game,
                  int ageLeft) const;

    /**
     * Draws a pill-shaped badge and returns its pixel width.
     * @param p       Active painter.
     * @param topLeft Top-left corner of the badge rectangle.
     * @param h       Badge height in pixels.
     * @param text    Label text.
     * @param bg      Background colour.
     * @param fg      Foreground (text) colour.
     * @param font    Font used for measurement and rendering.
     * @return Width of the rendered badge including horizontal padding.
     */
    static int paintBadge(QPainter *p,
                          const QPoint &topLeft,
                          int h,
                          const QString &text,
                          const QColor &bg,
                          const QColor &fg,
                          const QFont &font);
};

#endif // COCKATRICE_GAMES_LIST_ITEM_DELEGATE_H