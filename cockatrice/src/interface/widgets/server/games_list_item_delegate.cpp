#include "games_list_item_delegate.h"

#include "../../pixel_map_generator.h"
#include "../general/layout_containers/tiling_list_view.h"
#include "game/server_game.h"
#include "games_model.h"

#include <QPainter>
#include <QPainterPath>
#include <QTimeZone>

// ── Constructor / sizeHint ────────────────────────────────────────────────────

GameListItemDelegate::GameListItemDelegate(const QMap<int, QString> &rooms,
                                           const QMap<int, GameTypeMap> &gameTypes,
                                           const QMap<QString, QPixmap> *avatarCache,
                                           QObject *parent)
    : QStyledItemDelegate(parent), rooms(rooms), gameTypes(gameTypes), m_avatarCache(avatarCache)
{
}

QSize GameListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
    if (const auto *v = qobject_cast<const TilingListView *>(option.widget)) {
        return QSize(v->cellWidth(), 48);
    }
    return QSize(500, 48);
}

// ── Static helpers ────────────────────────────────────────────────────────────

/*static*/ QColor GameListItemDelegate::accentForGame(const ServerInfo_Game &game)
{
    if (game.started()) {
        return QColor(239, 68, 68); // red    — in progress
    }
    if (game.player_count() >= game.max_players()) {
        return QColor(249, 115, 22); // orange — full
    }
    if (game.with_password()) {
        return QColor(59, 130, 246); // blue   — password
    }
    return QColor(34, 197, 94); // green  — open
}

/*static*/ void GameListItemDelegate::typeColors(const QString &name, QColor &outBg, QColor &outFg)
{
    uint h = 5381u;
    for (const QChar c : name) {
        h = h * 33u ^ static_cast<uint>(c.unicode());
    }

    struct Slot
    {
        QColor bg, fg;
    };
    static const Slot palette[] = {
        {QColor(20, 50, 80), QColor(96, 165, 220)},  // steel-blue
        {QColor(48, 25, 75), QColor(167, 139, 250)}, // indigo
        {QColor(15, 58, 32), QColor(74, 222, 128)},  // emerald
        {QColor(68, 32, 12), QColor(251, 146, 60)},  // amber
        {QColor(60, 18, 25), QColor(248, 113, 113)}, // rose
        {QColor(12, 55, 55), QColor(45, 212, 191)},  // teal
        {QColor(50, 48, 12), QColor(217, 210, 80)},  // yellow
        {QColor(55, 18, 50), QColor(244, 114, 182)}, // pink
    };
    constexpr int N = static_cast<int>(sizeof(palette) / sizeof(palette[0]));

    const int idx = static_cast<int>(h % static_cast<uint>(N));
    outBg = palette[idx].bg;
    outFg = palette[idx].fg;
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

// ── paintCardBackground ───────────────────────────────────────────────────────

void GameListItemDelegate::paintCardBackground(QPainter *p,
                                               const QRectF &card,
                                               const QColor &accent,
                                               bool selected) const
{
    QLinearGradient bg(card.topLeft(), card.bottomRight());
    bg.setColorAt(0, selected ? accent.darker(200) : QColor(22, 28, 38));
    bg.setColorAt(1, selected ? QColor(30, 38, 52) : QColor(14, 18, 26));
    p->setPen(Qt::NoPen);
    p->setBrush(bg);
    p->drawRoundedRect(card, cardRadius, cardRadius);

    p->setBrush(accent);
    p->drawRoundedRect(QRectF(card.left(), card.top(), accentBarWidth, card.height()), accentBarRadius,
                       accentBarRadius);

    p->setPen(QPen(accent.darker(300), 0.5));
    p->setBrush(Qt::NoBrush);
    p->drawRoundedRect(card.adjusted(0.5, 0.5, -0.5, -0.5), cardRadius, cardRadius);
}

// ── paintCreatorAvatar ────────────────────────────────────────────────────────

void GameListItemDelegate::paintCreatorAvatar(QPainter *p,
                                              const QRect &avatarRect,
                                              const ServerInfo_Game &game,
                                              const QColor &accent) const
{
    const auto &creator = game.creator_info();
    const QString creatorName = QString::fromStdString(creator.name());

    QPainterPath clip;
    clip.addEllipse(avatarRect);
    p->save();
    p->setClipPath(clip);

    bool drewRealAvatar = false;
    if (m_avatarCache) {
        const auto it = m_avatarCache->find(creatorName);
        if (it != m_avatarCache->end() && !it->isNull()) {
            p->drawPixmap(avatarRect,
                          it->scaled(avatarRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            drewRealAvatar = true;
        }
    }

    if (!drewRealAvatar) {
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(28, 35, 46));
        p->drawEllipse(avatarRect);

        const QPixmap pawn = UserLevelPixmapGenerator::generatePixmap(
            avatarFallbackPawn, UserLevelFlags(creator.user_level()), creator.pawn_colors(), false,
            QString::fromStdString(creator.privlevel()));
        p->drawPixmap(avatarRect.center().x() - avatarFallbackPawn / 2,
                      avatarRect.center().y() - avatarFallbackPawn / 2, pawn);
    }

    p->restore();

    p->setPen(QPen(accent, 1.5));
    p->setBrush(Qt::NoBrush);
    p->drawEllipse(QRectF(avatarRect).adjusted(-0.75, -0.75, 0.75, 0.75));
}

// ── Zone painters ─────────────────────────────────────────────────────────────

void GameListItemDelegate::paintAge(QPainter *p,
                                    const QStyleOptionViewItem &option,
                                    const QRect &rect,
                                    const ServerInfo_Game &game,
                                    int ageLeft) const
{
    const qint64 ageSecs =
        QDateTime::fromSecsSinceEpoch(game.start_time(), QTimeZone::utc()).secsTo(QDateTime::currentDateTimeUtc());
    const QString ageStr = GamesModel::getGameCreatedString(static_cast<int>(ageSecs));

    p->setFont(option.font);
    p->setPen(QColor(80, 95, 115));
    p->drawText(QRect(ageLeft, rect.top(), zoneAgeW, rect.height()), Qt::AlignVCenter | Qt::AlignLeft, ageStr);
}

void GameListItemDelegate::paintCreator(QPainter *p,
                                        const QStyleOptionViewItem &option,
                                        const QRect &rect,
                                        const ServerInfo_Game &game,
                                        int creatorLeft) const
{
    const QString name = QString::fromStdString(game.creator_info().name());
    const QFontMetrics cfm(option.font);
    p->setFont(option.font);
    p->setPen(QColor(110, 125, 145));
    p->drawText(QRect(creatorLeft, rect.top(), zoneCreatorW, rect.height()), Qt::AlignVCenter | Qt::AlignLeft,
                cfm.elidedText(name, Qt::ElideRight, zoneCreatorW));
}

void GameListItemDelegate::paintTitle(QPainter *p,
                                      const QStyleOptionViewItem &option,
                                      const QRect &rect,
                                      const ServerInfo_Game &game,
                                      int lx,
                                      int titleW,
                                      const QFont &titleFont) const
{
    Q_UNUSED(option);

    int x = lx;
    int w = titleW;

    if (game.with_password()) {
        const int iconY = rect.top() + (rect.height() - lockSize) / 2;
        p->drawPixmap(x, iconY, LockPixmapGenerator::generatePixmap(lockSize));
        x += lockTextGap;
        w -= lockTextGap;
    }

    if (w <= 0) {
        return;
    }

    const QColor color = game.started() ? QColor(150, 160, 175) : QColor(220, 228, 240);
    const QString elided =
        QFontMetrics(titleFont).elidedText(QString::fromStdString(game.description()), Qt::ElideRight, w);

    p->setFont(titleFont);
    p->setPen(color);
    p->drawText(QRect(x, rect.top(), w, rect.height()), Qt::AlignVCenter | Qt::AlignLeft, elided);
}

void GameListItemDelegate::paintTypeBadges(QPainter *p,
                                           const QRect &rect,
                                           const ServerInfo_Game &game,
                                           int typesLeft,
                                           const QFont &badgeFont) const
{
    const GameTypeMap &gtMap = gameTypes.value(game.room_id());
    const int badgeY = rect.top() + (rect.height() - badgeH) / 2;
    const int xMax = typesLeft + zoneTypesW;

    int x = typesLeft;
    for (int i = 0; i < game.game_types_size(); ++i) {
        const QString typeName = gtMap.value(game.game_types(i));
        if (typeName.isEmpty()) {
            continue;
        }
        const int tw = QFontMetrics(badgeFont).horizontalAdvance(typeName) + badgeHPad * 2;
        if (x + tw > xMax) {
            break;
        }
        QColor bg, fg;
        typeColors(typeName, bg, fg);
        paintBadge(p, QPoint(x, badgeY), badgeH, typeName, bg, fg, badgeFont);
        x += tw + badgeGap;
    }
}

void GameListItemDelegate::paintStatus(QPainter *p,
                                       const QRect &rect,
                                       const ServerInfo_Game &game,
                                       int statusLeft,
                                       const QColor &accent,
                                       const QFont &badgeFont) const
{
    Q_UNUSED(accent);
    // Build the ordered list of status pills.  Each pill is a short label with
    // explicit bg/fg colours derived from game state.
    struct Pill
    {
        QString text;
        QColor bg;
        QColor fg;
    };
    QVector<Pill> pills;

    // Players — always first, colour encodes availability
    const bool full = game.player_count() >= game.max_players();
    const bool started = game.started();
    const QString players = QStringLiteral("%1/%2").arg(game.player_count()).arg(game.max_players());
    const QColor playerBg = started ? QColor(90, 20, 20) : full ? QColor(90, 42, 10) : QColor(15, 50, 25);
    const QColor playerFg = started ? QColor(248, 113, 113) : full ? QColor(251, 146, 60) : QColor(74, 222, 128);
    pills.append({players, playerBg, playerFg});

    // Spectators — only when allowed; label encodes chat/omniscient flags
    if (game.spectators_allowed()) {
        QString specLabel = QStringLiteral("👁 %1").arg(game.spectators_count());
        if (game.spectators_can_chat() && game.spectators_omniscient()) {
            specLabel += QStringLiteral(" c+h");
        } else if (game.spectators_can_chat()) {
            specLabel += QStringLiteral(" chat");
        } else if (game.spectators_omniscient()) {
            specLabel += QStringLiteral(" hands");
        }
        pills.append({specLabel, QColor(15, 40, 65), QColor(96, 165, 220)});
    }

    // Restriction tags — one pill each
    if (game.only_buddies()) {
        pills.append({tr("buddies"), QColor(48, 25, 75), QColor(167, 139, 250)});
    }
    if (game.only_registered()) {
        pills.append({tr("reg."), QColor(50, 48, 12), QColor(217, 210, 80)});
    }
    if (game.share_decklists_on_load()) {
        pills.append({tr("open decks"), QColor(12, 55, 55), QColor(45, 212, 191)});
    }

    // Draw left-to-right within the zone, stopping when space is exhausted
    const int badgeY = rect.top() + (rect.height() - badgeH) / 2;
    const int xMax = statusLeft + zoneStatusW;
    int x = statusLeft;

    for (const Pill &pill : std::as_const(pills)) {
        const int tw = QFontMetrics(badgeFont).horizontalAdvance(pill.text) + badgeHPad * 2;
        if (x + tw > xMax) {
            break;
        }
        paintBadge(p, QPoint(x, badgeY), badgeH, pill.text, pill.bg, pill.fg, badgeFont);
        x += tw + badgeGap;
    }
}

// ── paint — zone layout ───────────────────────────────────────────────────────

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
    const QRectF card = QRectF(rect).adjusted(cardMarginH, cardMarginV, -cardMarginH, -cardMarginV);

    // ── Left-anchored zones (left-to-right) ───────────────────────────────────
    const int ageLeft = rect.left() + leftPad; // age butts up against accent bar
    const int avatarLeft = ageLeft + zoneAgeW + avatarGap;
    const int avatarY = rect.top() + (rect.height() - avatarSize) / 2;
    const QRect avatarRect(avatarLeft, avatarY, avatarSize, avatarSize);
    const int creatorLeft = avatarLeft + avatarSize + creatorGap;
    const int lx = creatorLeft + zoneCreatorW + interZoneGap; // title start

    // ── Right-anchored zones (right-to-left) ──────────────────────────────────
    const int rx = rect.right() - rightPad;
    const int statusLeft = rx - zoneStatusW;
    const int typesLeft = statusLeft - interZoneGap - zoneTypesW;
    const int titleW = typesLeft - interZoneGap - lx;

    QFont titleFont = option.font;
    titleFont.setBold(true);

    QFont badgeFont = option.font;
    badgeFont.setPointSizeF(badgeFont.pointSizeF() * badgeFontScale);
    badgeFont.setBold(true);

    paintCardBackground(painter, card, accent, selected);
    paintAge(painter, option, rect, game, ageLeft);
    paintCreatorAvatar(painter, avatarRect, game, accent);
    paintCreator(painter, option, rect, game, creatorLeft);
    paintTitle(painter, option, rect, game, lx, titleW, titleFont);
    paintTypeBadges(painter, rect, game, typesLeft, badgeFont);
    paintStatus(painter, rect, game, statusLeft, accent, badgeFont);

    painter->restore();
}