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
        return QSize(v->cellWidth(), 90);
    }
    return QSize(320, 90);
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
    // djb2-style hash — deterministic across sessions, independent of
    // Qt's per-process qHash seed randomisation.
    uint h = 5381u;
    for (const QChar c : name) {
        h = h * 33u ^ static_cast<uint>(c.unicode());
    }

    struct Slot
    {
        QColor bg;
        QColor fg;
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
            // Scale to fill the circle, crop centre — identical to UserListPainter
            p->drawPixmap(avatarRect,
                          it->scaled(avatarRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            drewRealAvatar = true;
        }
    }

    if (!drewRealAvatar) {
        // Neutral dark fill — close to the card background so it reads as
        // "no image" rather than a coloured accent circle.
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(28, 35, 46));
        p->drawEllipse(avatarRect);

        // Larger pawn (avatarFallbackPawn px) to fill the circle comfortably
        const QPixmap pawn = UserLevelPixmapGenerator::generatePixmap(
            avatarFallbackPawn, UserLevelFlags(creator.user_level()), creator.pawn_colors(), false,
            QString::fromStdString(creator.privlevel()));
        p->drawPixmap(avatarRect.center().x() - avatarFallbackPawn / 2,
                      avatarRect.center().y() - avatarFallbackPawn / 2, pawn);
    }

    p->restore();

    // Accent-coloured ring — visible in both real-avatar and fallback cases
    p->setPen(QPen(accent, 1.5));
    p->setBrush(Qt::NoBrush);
    p->drawEllipse(QRectF(avatarRect).adjusted(-0.75, -0.75, 0.75, 0.75));
}

// ── paintRow1 — title  ·  age (inline) ───────────────────────────────────────

void GameListItemDelegate::paintRow1(QPainter *p,
                                     const QStyleOptionViewItem &option,
                                     const QRect &rect,
                                     const ServerInfo_Game &game,
                                     int lx,
                                     int rx,
                                     const QFont &titleFont) const
{
    const int rowTop = rect.top() + row1Y;
    const QColor titleColor = game.started() ? QColor(150, 160, 175) : QColor(220, 228, 240);

    // Age string — computed first so we can reserve its width before placing
    // the title.  The dot-separator is baked in so the two draw calls are
    // independent and the title never overlaps the age.
    const qint64 ageSecs =
        QDateTime::fromSecsSinceEpoch(game.start_time(), QTimeZone::utc()).secsTo(QDateTime::currentDateTimeUtc());
    const QString ageStr = QStringLiteral("· ") + GamesModel::getGameCreatedString(static_cast<int>(ageSecs));
    const QFontMetrics ageFm(option.font);
    const int agePad = ageFm.horizontalAdvance(QStringLiteral("  ")); // gap between title and age
    const int ageW = ageFm.horizontalAdvance(ageStr);

    // Lock icon
    int titleX = lx;
    if (game.with_password()) {
        p->drawPixmap(titleX, rowTop + (row1H - lockSize) / 2, LockPixmapGenerator::generatePixmap(lockSize));
        titleX += lockTextGap;
    }

    // Title — elides before the age so the age string is always fully visible
    const int titleAvailW = rx - titleX - agePad - ageW;
    const QFontMetrics titleFm(titleFont);
    const QString elidedTitle =
        titleFm.elidedText(QString::fromStdString(game.description()), Qt::ElideRight, qMax(0, titleAvailW));
    const int titleRenderedW = titleFm.horizontalAdvance(elidedTitle);

    p->setFont(titleFont);
    p->setPen(QColor(0, 0, 0, 160)); // drop shadow
    p->drawText(QRect(titleX + 1, rowTop + 1, titleAvailW, row1H), Qt::AlignVCenter | Qt::AlignLeft, elidedTitle);
    p->setPen(titleColor);
    p->drawText(QRect(titleX, rowTop, titleAvailW, row1H), Qt::AlignVCenter | Qt::AlignLeft, elidedTitle);

    // Age — starts right after the rendered title text, never jumps to far right
    const int ageX = titleX + titleRenderedW + agePad;
    p->setFont(option.font);
    p->setPen(QColor(80, 95, 115));
    p->drawText(QRect(ageX, rowTop, rx - ageX, row1H), Qt::AlignVCenter | Qt::AlignLeft, ageStr);
}

// ── paintRow2 — creator name + type badges ────────────────────────────────────

void GameListItemDelegate::paintRow2(QPainter *p,
                                     const QStyleOptionViewItem &option,
                                     const QRect &rect,
                                     const ServerInfo_Game &game,
                                     int lx,
                                     int rx,
                                     const QFont &badgeFont) const
{
    const int rowTop = rect.top() + row2Y;

    // Measure the full badge strip width first so the name can be elided to
    // exactly the remaining space without a second layout pass.
    const GameTypeMap &gtMap = gameTypes.value(game.room_id());
    QStringList typeNames;
    typeNames.reserve(game.game_types_size());
    for (int i = 0; i < game.game_types_size(); ++i) {
        typeNames << gtMap.value(game.game_types(i));
    }

    const QFontMetrics bfm(badgeFont);
    int totalBadgeW = 0;
    for (const QString &t : std::as_const(typeNames)) {
        totalBadgeW += bfm.horizontalAdvance(t) + badgeHPad * 2 + badgeGap;
    }
    if (!typeNames.isEmpty()) {
        totalBadgeW -= badgeGap; // no trailing gap after the rightmost badge
    }

    const int nameAvailW = qMax(0, rx - totalBadgeW - (totalBadgeW > 0 ? sectionGap : 0) - lx);
    const QFontMetrics cfm(option.font);
    p->setFont(option.font);
    p->setPen(QColor(148, 163, 184));
    p->drawText(QRect(lx, rowTop, nameAvailW, row2H), Qt::AlignVCenter | Qt::AlignLeft,
                cfm.elidedText(QString::fromStdString(game.creator_info().name()), Qt::ElideRight, nameAvailW));

    // Type badges — right-to-left, each with its own stable colour
    int tbx = rx;
    for (const QString &typeName : std::as_const(typeNames)) {
        const int tw = bfm.horizontalAdvance(typeName) + badgeHPad * 2;
        tbx -= tw;
        QColor bg, fg;
        typeColors(typeName, bg, fg);
        paintBadge(p, QPoint(tbx, rowTop), row2H - 1, typeName, bg, fg, badgeFont);
        tbx -= badgeGap;
    }
}

// ── paintRow3 — players · specs · restrictions ────────────────────────────────

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

    // Spectators (skipped entirely when not allowed)
    if (game.spectators_allowed()) {
        QString specStr = QStringLiteral("👁 %1").arg(game.spectators_count());
        if (game.spectators_can_chat() && game.spectators_omniscient()) {
            specStr += QStringLiteral(" (chat+hands)");
        } else if (game.spectators_can_chat()) {
            specStr += QStringLiteral(" (chat)");
        } else if (game.spectators_omniscient()) {
            specStr += QStringLiteral(" (hands)");
        }
        const int sw = cfm.horizontalAdvance(specStr);
        if (infoX + sw <= rx) {
            p->setPen(QColor(148, 163, 184));
            p->drawText(infoX, rowTop, sw, row3H, Qt::AlignVCenter | Qt::AlignLeft, specStr);
            infoX += sw + sectionGap;
            drawDivider(infoX);
        }
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

    if (!restr.isEmpty() && infoX < rx) {
        p->setPen(QColor(100, 110, 130));
        p->drawText(infoX, rowTop, rx - infoX, row3H, Qt::AlignVCenter | Qt::AlignLeft,
                    cfm.elidedText(restr.join(QStringLiteral(" · ")), Qt::ElideRight, rx - infoX));
    }
}

// ── paint — orchestrates the card ─────────────────────────────────────────────

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

    const int avatarX = rect.left() + leftPad;
    const int avatarY = rect.top() + (rect.height() - avatarSize) / 2;
    const QRect avatarRect(avatarX, avatarY, avatarSize, avatarSize);
    const int lx = avatarX + avatarSize + avatarGap;
    const int rx = rect.right() - rightPad;

    QFont titleFont = option.font;
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * titleFontScale);

    QFont badgeFont = option.font;
    badgeFont.setBold(true);
    badgeFont.setPointSizeF(badgeFont.pointSizeF() * badgeFontScale);

    paintCardBackground(painter, card, accent, selected);
    paintCreatorAvatar(painter, avatarRect, game, accent);
    paintRow1(painter, option, rect, game, lx, rx, titleFont);
    paintRow2(painter, option, rect, game, lx, rx, badgeFont);
    paintRow3(painter, option, rect, game, lx, rx);

    painter->restore();
}