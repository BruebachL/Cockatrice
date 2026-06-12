#ifndef COCKATRICE_GAMES_LIST_ITEM_DELEGATE_H
#define COCKATRICE_GAMES_LIST_ITEM_DELEGATE_H

#include "game_type_map.h"

#include <QMap>
#include <QPixmap>
#include <QStyledItemDelegate>

class ServerInfo_Game;

/**
 * @class GameListItemDelegate
 * @brief Paints each game in the TilingListView as a dark card with three rows.
 *
 * Layout:
 *   [● avatar] Row 1 — title (bold)  ·  age (dim, inline — no eye-bounce)
 *              Row 2 — creator name  ···  game-type badges (per-type colour, right)
 *              Row 3 — 👥 N/M  ·  👁 N (chat)  ·  restrictions
 *
 * Avatar (●): 36 px circle.  Real avatar drawn when the cache has an entry for
 * the creator username; otherwise a larger pawn icon on a neutral dark fill.
 * Accent ring encodes join state (green/blue/orange/red).
 *
 * Game-type badge colours derive from a stable djb2 hash of the type name so
 * the same type always maps to the same palette slot across sessions.
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

    // ── Spacing ───────────────────────────────────────────────────────────────
    static constexpr int leftPad = 14; ///< rect.left() → avatar left edge
    static constexpr int rightPad = 10;
    static constexpr int avatarSize = 36;         ///< Creator avatar circle diameter
    static constexpr int avatarFallbackPawn = 26; ///< Pawn size when no avatar image available
    static constexpr int avatarGap = 8;           ///< Gap from avatar right edge to text

    // ── Row geometry (y offset from rect.top()) ───────────────────────────────
    static constexpr int row1Y = 10;
    static constexpr int row1H = 18;
    static constexpr int row2Y = 33;
    static constexpr int row2H = 14;
    static constexpr int row3Y = 56;
    static constexpr int row3H = 14;

    // ── Icon sizes ────────────────────────────────────────────────────────────
    static constexpr int lockSize = 13;
    static constexpr int lockTextGap = 17;

    // ── Badge geometry ────────────────────────────────────────────────────────
    static constexpr int badgeHPad = 5;
    static constexpr int badgeGap = 4;
    static constexpr int badgeRadius = 3;
    static constexpr int dividerW = 8;
    static constexpr int dividerGap = 4;
    static constexpr int sectionGap = 8;

    // ── Font scales ───────────────────────────────────────────────────────────
    static constexpr double titleFontScale = 1.05;
    static constexpr double badgeFontScale = 0.68;

    const QMap<int, QString> rooms;
    const QMap<int, GameTypeMap> gameTypes;
    const QMap<QString, QPixmap> *m_avatarCache; ///< Non-owning; null means pawn fallback always

public:
    /**
     * @param avatarCache  Pointer to the live username→pixmap cache from
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
     * Maps a game-type display name to a stable bg/fg colour pair via djb2
     * hash.  Deterministic across sessions (does not use qHash's random seed).
     */
    static void typeColors(const QString &typeName, QColor &outBg, QColor &outFg);

    /** Draws the rounded card background gradient and left accent bar. */
    void paintCardBackground(QPainter *p, const QRectF &card, const QColor &accent, bool selected) const;

    /**
     * Draws the 36 px circular creator avatar.
     * Real avatar image used when m_avatarCache has an entry for the creator
     * username; otherwise a neutral dark fill with a larger pawn icon.
     * Accent-coloured ring drawn in both cases.
     */
    void
    paintCreatorAvatar(QPainter *p, const QRect &avatarRect, const ServerInfo_Game &game, const QColor &accent) const;

    /**
     * Draws row 1: optional lock icon, bold game title, and age inline
     * immediately after the title text ("Title  ·  5m").
     * The title elides before the age so the age is always visible.
     */
    void paintRow1(QPainter *p,
                   const QStyleOptionViewItem &option,
                   const QRect &rect,
                   const ServerInfo_Game &game,
                   int lx,
                   int rx,
                   const QFont &titleFont) const;

    /** Draws row 2: creator name (left, auto-elided) + coloured type badges (right). */
    void paintRow2(QPainter *p,
                   const QStyleOptionViewItem &option,
                   const QRect &rect,
                   const ServerInfo_Game &game,
                   int lx,
                   int rx,
                   const QFont &badgeFont) const;

    /** Draws row 3: player count · spectator info · restriction tags. */
    void paintRow3(QPainter *p,
                   const QStyleOptionViewItem &option,
                   const QRect &rect,
                   const ServerInfo_Game &game,
                   int lx,
                   int rx) const;

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