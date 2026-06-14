#ifndef COCKATRICE_GAMES_LIST_ITEM_DELEGATE_H
#define COCKATRICE_GAMES_LIST_ITEM_DELEGATE_H

#include "game_type_map.h"

#include <QMap>
#include <QPixmap>
#include <QStyledItemDelegate>

class ServerInfo_Game;

/**
 * @class GameListItemDelegate
 * @brief Paints each game in the TilingListView as a two-row card.
 *
 * Row 1 — identity:  age · avatar · creator name · [status pills, right-anchored]
 * Row 2 — content:   title (bold, flex, left-aligned with creator) · [type badges, right-anchored]
 *
 * Both rows share the same left indent (creator left edge) so content forms a
 * clean visual block under the avatar.  Right-anchored zones on each row are
 * built right-to-left from the card edge; remaining space is left empty (row 1)
 * or consumed by the title (row 2).
 *
 * Status pills (row 1): players, spectators, restrictions — rendered left-to-right
 * within the available right-side space, stopping when exhausted.
 *
 * Type badges (row 2): rendered left-to-right within zoneTypesW, right-anchored,
 * stopping when the zone is exhausted.
 *
 * Avatar: real image from m_avatarCache when available; neutral dark fill plus
 *         the creator's pawn otherwise.  Accent ring encodes join state:
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

    // ── Row geometry ──────────────────────────────────────────────────────────
    static constexpr int row1H = 48; ///< Avatar(32) + 8px top/bottom margin
    static constexpr int row2H = 32;

    // ── Left-anchored zones (shared x between row 1 age and row 2 players) ───
    static constexpr int leftPad = 16;     ///< Clears accent bar with breathing room
    static constexpr int zoneAgeW = 44;    ///< Row 1: age text; Row 2: player count — same width
    static constexpr int ageRightPad = 10; ///< Gap between age/players column and avatar

    // ── Avatar ────────────────────────────────────────────────────────────────
    static constexpr int avatarSize = 32; ///< 32px fits in row1H(48) with 8px margin each side
    static constexpr int avatarFallbackPawn = 22;
    static constexpr int creatorGap = 8; ///< Avatar right → creator left

    // ── Row 1 right side ──────────────────────────────────────────────────────
    static constexpr int zoneCreatorW = 100;
    static constexpr int zoneStatusW = 220; ///< Status pills, right-anchored

    // ── Row 2 right side ──────────────────────────────────────────────────────
    static constexpr int zoneTypesW = 160; ///< Type badges, right-anchored

    // ── Lock icon ─────────────────────────────────────────────────────────────
    static constexpr int lockSize = 13;
    static constexpr int lockTextGap = 17;

    // ── Shared ────────────────────────────────────────────────────────────────
    static constexpr int rightPad = 10;
    static constexpr int interZoneGap = 8;

    // ── Badge geometry ────────────────────────────────────────────────────────
    static constexpr int badgeH = 16;
    static constexpr int badgeHPad = 6;
    static constexpr int badgeGap = 4;
    static constexpr int badgeRadius = 3;

    static constexpr double badgeFontScale = 0.75;

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

    /** Row 1: game creation age, left-aligned in zoneAgeW. */
    void paintAge(QPainter *p,
                  const QStyleOptionViewItem &option,
                  const QRect &row1Rect,
                  const ServerInfo_Game &game,
                  int ageLeft) const;

    /** Row 1: creator name, elided to zoneCreatorW. */
    void paintCreator(QPainter *p,
                      const QStyleOptionViewItem &option,
                      const QRect &row1Rect,
                      const ServerInfo_Game &game,
                      int creatorLeft) const;

    /**
     * Row 1: status pills rendered left-to-right from statusLeft, stopping
     * when the available width (rx - statusLeft) is exhausted.
     * Pills: player count · spectators (if allowed) · restrictions.
     */
    void paintStatus(QPainter *p,
                     const QRect &row1Rect,
                     const ServerInfo_Game &game,
                     int statusLeft,
                     int rx,
                     const QFont &badgeFont) const;

    /** Row 2: player count as plain text (no emoji), left-aligned, coloured by state. */
    void paintPlayers(QPainter *p,
                      const QStyleOptionViewItem &option,
                      const QRect &row2Rect,
                      const ServerInfo_Game &game,
                      int playersLeft) const;

    /**
     * Row 2: bold game title, elided to titleW.
     * Optional lock icon prefixed for password-protected games.
     */
    void paintTitle(QPainter *p,
                    const QStyleOptionViewItem &option,
                    const QRect &row2Rect,
                    const ServerInfo_Game &game,
                    int lx,
                    int titleW,
                    const QFont &titleFont) const;

    /**
     * Row 2: game-type pill badges, right-anchored, rendered left-to-right
     * within zoneTypesW.  Badges that exceed the zone are silently omitted.
     */
    void paintTypeBadges(QPainter *p,
                         const QRect &row2Rect,
                         const ServerInfo_Game &game,
                         int typesLeft,
                         const QFont &badgeFont) const;

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