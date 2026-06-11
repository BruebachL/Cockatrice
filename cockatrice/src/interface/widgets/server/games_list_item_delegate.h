#ifndef COCKATRICE_GAMES_LIST_ITEM_DELEGATE_H
#define COCKATRICE_GAMES_LIST_ITEM_DELEGATE_H

#include "game_type_map.h"

#include <QStyledItemDelegate>
class ServerInfo_Game;

/**
 * @class GameListItemDelegate
 * @brief Paints each game in the TilingListView as a dark card with three rows.
 *
 * Row 1 — game title (bold, elided) + age badge top-right
 * Row 2 — creator pawn icon + creator name + game-type pill badges right-aligned
 * Row 3 — player count · spectator count · restriction tags
 *
 * A coloured accent bar on the left edge encodes game state:
 *   green  = open
 *   blue   = password protected
 *   orange = full
 *   red    = in progress
 *
 * Only used when SettingsCache::instance().getStyleGamesList() returns true.
 */
class GameListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    // --- Layout constants (pixels, relative to option.rect unless noted) ---
    static constexpr int cardMarginH = 4; ///< Horizontal inset from option.rect to card edge
    static constexpr int cardMarginV = 3; ///< Vertical inset from option.rect to card edge
    static constexpr int cardRadius = 7;
    static constexpr int accentBarWidth = 4;
    static constexpr int accentBarRadius = 2;
    static constexpr int leftPad = 16; ///< Left content margin (right of accent bar)
    static constexpr int rightPad = 8; ///< Right content margin
    static constexpr int row1Y = 10;   ///< Row 1 top, relative to rect.top()
    static constexpr int row1H = 16;
    static constexpr int row2Y = 33; ///< Row 2 top, relative to rect.top()
    static constexpr int row2H = 14;
    static constexpr int row3Y = 56; ///< Row 3 top, relative to rect.top()
    static constexpr int row3H = 14;
    static constexpr int pawnSize = 13;
    static constexpr int lockSize = 13;
    static constexpr int lockTextGap = 17; ///< X advance after lock icon
    static constexpr int pawnTextGap = 17; ///< X advance after pawn icon
    static constexpr int nameMaxW = 160;
    static constexpr int badgeHPad = 5; ///< Horizontal padding inside a pill badge (each side)
    static constexpr int badgeGap = 4;  ///< Gap between successive badges
    static constexpr int badgeRadius = 3;
    static constexpr int dividerW = 6;
    static constexpr int dividerGap = 4; ///< Total advance after a "·" divider
    static constexpr int sectionGap = 8; ///< Gap between adjacent row-3 sections

    // --- Font scale factors ---
    static constexpr double titleFontScale = 1.05;
    static constexpr double badgeFontScale = 0.68;

    const QMap<int, QString> rooms;
    const QMap<int, GameTypeMap> gameTypes;

public:
    GameListItemDelegate(const QMap<int, QString> &rooms,
                         const QMap<int, GameTypeMap> &gameTypes,
                         QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    /** Returns the accent colour encoding the game's current join state. */
    static QColor accentForGame(const ServerInfo_Game &game);

    /** Draws the rounded card background gradient and left accent bar. */
    void paintCardBackground(QPainter *p, const QRectF &card, const QColor &accent, bool selected) const;

    /** Draws row 1: lock icon (optional), elided game title, age badge. */
    void paintRow1(QPainter *p,
                   const QStyleOptionViewItem &option,
                   const QRect &rect,
                   const ServerInfo_Game &game,
                   const QColor &accent,
                   int lx,
                   int rx,
                   const QFont &titleFont,
                   const QFont &badgeFont) const;

    /** Draws row 2: creator pawn + name, game-type pill badges (right-aligned). */
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
     * @brief Draws a pill-shaped badge and returns its pixel width.
     * @param p        Active painter.
     * @param topLeft  Top-left corner of the badge.
     * @param h        Badge height in pixels.
     * @param text     Label text.
     * @param bg       Background colour.
     * @param fg       Foreground (text) colour.
     * @param font     Font used to measure and render the text.
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
