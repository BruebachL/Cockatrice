#include "games_list_item_delegate.h"

#include "../../pixel_map_generator.h"
#include "../general/layout_containers/tiling_list_view.h"
#include "game/server_game.h"
#include "games_model.h"

#include <QPainter>
#include <QTimeZone>

GameListItemDelegate::GameListItemDelegate(const QMap<int, QString> &rooms,
                                           const QMap<int, GameTypeMap> &gameTypes,
                                           QObject *parent)
    : QStyledItemDelegate(parent), rooms(rooms), gameTypes(gameTypes)
{
}

QSize GameListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
    if (const auto *v = qobject_cast<const TilingListView *>(option.widget)) {
        return QSize(v->cellWidth(), 90);
    }
    return QSize(320, 90);
}

// ----------------------------------------------------------------------------
// Static helpers
// ----------------------------------------------------------------------------

QColor GameListItemDelegate::accentForGame(const ServerInfo_Game &game)
{
    if (game.started()) {
        return QColor(239, 68, 68); // red    — in progress
    }
    if (game.player_count() >= game.max_players()) {
        return QColor(249, 115, 22); // orange — full
    }
    if (game.with_password()) {
        return QColor(59, 130, 246); // blue   — password protected
    }
    return QColor(34, 197, 94); // green  — open
}

/*static*/ int GameListItemDelegate::paintBadge(QPainter *p,
                                                const QPoint &topLeft,
                                                int h,
                                                const QString &text,
                                                const QColor &bg,
                                                const QColor &fg,
                                                const QFont &font)
{
    const int w = QFontMetrics(font).horizontalAdvance(text) + badgeHPad * 2;
    const QRect r(topLeft, QSize(w, h));
    p->setFont(font);
    p->setPen(Qt::NoPen);
    p->setBrush(bg);
    p->drawRoundedRect(r, badgeRadius, badgeRadius);
    p->setPen(fg);
    p->drawText(r, Qt::AlignCenter, text);
    return w;
}

// ----------------------------------------------------------------------------
// paintCardBackground
// ----------------------------------------------------------------------------

void GameListItemDelegate::paintCardBackground(QPainter *p,
                                               const QRectF &card,
                                               const QColor &accent,
                                               bool selected) const
{
    // Main card gradient
    QLinearGradient bg(card.topLeft(), card.bottomRight());
    bg.setColorAt(0, selected ? accent.darker(200) : QColor(22, 28, 38));
    bg.setColorAt(1, selected ? QColor(30, 38, 52) : QColor(14, 18, 26));
    p->setPen(Qt::NoPen);
    p->setBrush(bg);
    p->drawRoundedRect(card, cardRadius, cardRadius);

    // Left accent bar
    p->setBrush(accent);
    p->drawRoundedRect(QRectF(card.left(), card.top(), accentBarWidth, card.height()), accentBarRadius,
                       accentBarRadius);

    // Subtle border
    p->setPen(QPen(accent.darker(300), 0.5));
    p->setBrush(Qt::NoBrush);
    p->drawRoundedRect(card.adjusted(0.5, 0.5, -0.5, -0.5), cardRadius, cardRadius);
}

// ----------------------------------------------------------------------------
// paintRow1 — title + age badge
// ----------------------------------------------------------------------------

void GameListItemDelegate::paintRow1(QPainter *p,
                                     const QStyleOptionViewItem &option,
                                     const QRect &rect,
                                     const ServerInfo_Game &game,
                                     const QColor &accent,
                                     int lx,
                                     int rx,
                                     const QFont &titleFont,
                                     const QFont &badgeFont) const
{
    Q_UNUSED(option);
    const QColor titleColor = game.started() ? QColor(150, 160, 175) : QColor(220, 228, 240);
    const int rowTop = rect.top() + row1Y;

    // Age badge — drawn first so we know how wide it is before placing the title
    const qint64 ageSecs =
        QDateTime::fromSecsSinceEpoch(game.start_time(), QTimeZone::utc()).secsTo(QDateTime::currentDateTimeUtc());
    const QString ageStr = GamesModel::getGameCreatedString(static_cast<int>(ageSecs));
    const int ageBadgeW =
        paintBadge(p, QPoint(rx - QFontMetrics(badgeFont).horizontalAdvance(ageStr) - badgeHPad * 2, rowTop), row1H - 2,
                   ageStr, accent.darker(250), accent.lighter(150), badgeFont);

    // Lock icon (password-protected games)
    int titleX = lx;
    if (game.with_password()) {
        p->drawPixmap(titleX, rowTop + 1, LockPixmapGenerator::generatePixmap(lockSize));
        titleX += lockTextGap;
    }

    // Game title — elided to avoid overlapping the age badge
    const int titleW = rx - ageBadgeW - titleX - badgeGap;
    const QRect titleRect(titleX, rowTop, titleW, row1H);
    const QString elidedTitle =
        QFontMetrics(titleFont).elidedText(QString::fromStdString(game.description()), Qt::ElideRight, titleW);
    p->setFont(titleFont);
    p->setPen(titleColor);
    p->drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, elidedTitle);
}

// ----------------------------------------------------------------------------
// paintRow2 — creator pawn + name + game-type badges
// ----------------------------------------------------------------------------

void GameListItemDelegate::paintRow2(QPainter *p,
                                     const QStyleOptionViewItem &option,
                                     const QRect &rect,
                                     const ServerInfo_Game &game,
                                     int lx,
                                     int rx,
                                     const QFont &badgeFont) const
{
    const int rowTop = rect.top() + row2Y;

    // Creator pawn icon + name
    const auto &creator = game.creator_info();
    const QPixmap pawn =
        UserLevelPixmapGenerator::generatePixmap(pawnSize, UserLevelFlags(creator.user_level()), creator.pawn_colors(),
                                                 false, QString::fromStdString(creator.privlevel()));
    p->drawPixmap(lx, rowTop + 1, pawn);

    const QString creatorName = QString::fromStdString(creator.name());
    const QFontMetrics cfm(option.font);
    p->setFont(option.font);
    p->setPen(QColor(148, 163, 184));
    p->drawText(lx + pawnTextGap, rowTop, nameMaxW, row2H, Qt::AlignVCenter | Qt::AlignLeft,
                cfm.elidedText(creatorName, Qt::ElideRight, nameMaxW));

    // Game-type badge pills, laid out right-to-left
    const GameTypeMap &gtMap = gameTypes.value(game.room_id());
    QStringList typeNames;
    typeNames.reserve(game.game_types_size());
    for (int i = 0; i < game.game_types_size(); ++i) {
        typeNames << gtMap.value(game.game_types(i));
    }

    int tbx = rx;
    for (const QString &typeName : std::as_const(typeNames)) {
        const int tw = QFontMetrics(badgeFont).horizontalAdvance(typeName) + badgeHPad * 2;
        tbx -= tw;
        paintBadge(p, QPoint(tbx, rowTop), row2H - 1, typeName, QColor(30, 58, 80), QColor(96, 165, 200), badgeFont);
        tbx -= badgeGap;
    }
}

// ----------------------------------------------------------------------------
// paintRow3 — players · spectators · restrictions
// ----------------------------------------------------------------------------

void GameListItemDelegate::paintRow3(QPainter *p,
                                     const QStyleOptionViewItem &option,
                                     const QRect &rect,
                                     const ServerInfo_Game &game,
                                     int lx,
                                     int rx) const
{
    const int rowTop = rect.top() + row3Y;
    const QFontMetrics cfm(option.font);
    p->setFont(option.font);

    // Helper: draw a mid-dot divider and advance the x cursor
    auto drawDivider = [&](int &x) {
        p->setPen(QColor(50, 60, 75));
        p->drawText(x, rowTop, dividerW, row3H, Qt::AlignCenter, QStringLiteral("·"));
        x += dividerW + dividerGap;
    };

    // Players
    const bool full = game.player_count() >= game.max_players();
    const QString playerStr = QStringLiteral("👥 %1/%2").arg(game.player_count()).arg(game.max_players());
    p->setPen(full ? QColor(249, 115, 22) : QColor(148, 163, 184));
    const int pw = cfm.horizontalAdvance(playerStr);
    p->drawText(lx, rowTop, pw, row3H, Qt::AlignVCenter | Qt::AlignLeft, playerStr);

    int infoX = lx + pw + sectionGap;
    drawDivider(infoX);

    // Spectators (omitted entirely when not allowed)
    if (game.spectators_allowed()) {
        QString specStr = QStringLiteral("👁 %1").arg(game.spectators_count());
        if (game.spectators_can_chat() && game.spectators_omniscient()) {
            specStr += QStringLiteral(" (chat+hands)");
        } else if (game.spectators_can_chat()) {
            specStr += QStringLiteral(" (chat)");
        } else if (game.spectators_omniscient()) {
            specStr += QStringLiteral(" (hands)");
        }
        p->setPen(QColor(148, 163, 184));
        const int sw = cfm.horizontalAdvance(specStr);
        p->drawText(infoX, rowTop, sw, row3H, Qt::AlignVCenter | Qt::AlignLeft, specStr);
        infoX += sw + sectionGap;
        drawDivider(infoX);
    }

    // Restriction tags
    QStringList restr;
    if (game.only_buddies()) {
        restr << tr("buddies");
    }
    if (game.only_registered()) {
        restr << tr("reg. only");
    }
    if (game.share_decklists_on_load()) {
        restr << tr("open decks");
    }
    if (game.started()) {
        restr << tr("in progress");
    }

    if (!restr.isEmpty()) {
        p->setPen(QColor(100, 110, 130));
        p->drawText(infoX, rowTop, rx - infoX, row3H, Qt::AlignVCenter | Qt::AlignLeft,
                    cfm.elidedText(restr.join(QStringLiteral(" · ")), Qt::ElideRight, rx - infoX));
    }
}

// ----------------------------------------------------------------------------
// paint — orchestrates the three-row card
// ----------------------------------------------------------------------------

void GameListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QVariant var = index.data(GamesModel::GameDataRole);
    if (!var.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    const ServerInfo_Game game = var.value<ServerInfo_Game>();
    const QRect rect = option.rect;
    const bool selected = option.state & QStyle::State_Selected;
    const QColor accent = accentForGame(game);
    const int lx = rect.left() + leftPad;
    const int rx = rect.right() - rightPad;

    QFont titleFont = option.font;
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * titleFontScale);

    QFont badgeFont = option.font;
    badgeFont.setPointSizeF(badgeFont.pointSizeF() * badgeFontScale);
    badgeFont.setBold(true);

    const QRectF card = QRectF(rect).adjusted(cardMarginH, cardMarginV, -cardMarginH, -cardMarginV);

    paintCardBackground(painter, card, accent, selected);
    paintRow1(painter, option, rect, game, accent, lx, rx, titleFont, badgeFont);
    paintRow2(painter, option, rect, game, lx, rx, badgeFont);
    paintRow3(painter, option, rect, game, lx, rx);

    painter->restore();
}