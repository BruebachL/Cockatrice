#include "chat_view.h"

#include "../../../../client/settings/cache_settings.h"
#include "../../client/sound_engine.h"
#include "../../interface/pixel_map_generator.h"
#include "../user/user_context_menu.h"
#include "../user/user_list_manager.h"
#include "../user/user_list_proxy.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QScrollBar>
#include <libcockatrice/network/server/remote/user_level.h>

// ── Constructor ───────────────────────────────────────────────────────────────

ChatView::ChatView(TabSupervisor *tabSupervisor, AbstractGame *game, bool showTimestamps, QWidget *parent)
    : QAbstractScrollArea(parent), tabSupervisor(tabSupervisor), game(game),
      userListProxy(tabSupervisor->getUserListManager()), showTimestamps(showTimestamps)
{
    m_ownUserName = userListProxy->getOwnUsername();
    m_mention = "@" + m_ownUserName;

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    viewport()->setMouseTracking(true);
    viewport()->setAutoFillBackground(false);

    userContextMenu = new UserContextMenu(tabSupervisor, this, game);
    connect(userContextMenu, &UserContextMenu::openMessageDialog, this, &ChatView::openMessageDialog);

    // When the avatar provider finishes loading any avatar, repaint so the
    // new image appears in place of the pawn fallback.
    connect(tabSupervisor->getUserListManager()->getAvatarProvider(), &UserAvatarProvider::avatarUpdated, viewport(),
            QOverload<>::of(&QWidget::update));
}

void ChatView::retranslateUi()
{
    userContextMenu->retranslateUi();
}

// ── Geometry ──────────────────────────────────────────────────────────────────

int ChatView::textLeft() const
{
    // All three message kinds share the same text left edge so the text column
    // is visually consistent. Compressed messages just omit the avatar/header.
    return leftPad + avatarSize + avatarGap;
}

int ChatView::availableTextWidth(int viewportWidth) const
{
    return viewportWidth - textLeft() - rightPad;
}

int ChatView::messageTop(int idx) const
{
    int y = 0;
    for (int i = 0; i < idx; ++i) {
        y += m_layouts.at(i).totalHeight;
    }
    return y;
}

// ── Layout ────────────────────────────────────────────────────────────────────

void ChatView::layoutMessage(int idx, int availableWidth)
{
    const ChatMessage &msg = m_messages.at(idx);
    MessageLayout &layout = m_layouts[idx];

    // Server messages don't have an avatar column — start text closer to the
    // left edge so the wide indent doesn't waste space on system lines.
    const int msgTextLeft =
        (msg.kind == ChatMessage::Server) ? leftPad + accentBarWidth + 8 : leftPad + avatarSize + avatarGap;
    layout.textLeft = msgTextLeft;

    const int tw = availableWidth - msgTextLeft - rightPad;

    // Base font — bold for bold server messages
    QFont layoutFont = font();
    if (msg.kind == ChatMessage::Server && msg.serverBold) {
        layoutFont.setBold(true);
    }

    // Build QTextLayout format ranges from spans
    QVector<QTextLayout::FormatRange> formats;
    formats.reserve(msg.spans.size());
    for (const ChatSpan &span : msg.spans) {
        const QTextCharFormat fmt = formatForSpan(span);
        if (fmt != QTextCharFormat{}) {
            QTextLayout::FormatRange fr;
            fr.start = span.start;
            fr.length = span.length;
            fr.format = fmt;
            formats << fr;
        }
    }

    layout.textLayout->setText(msg.text);
    layout.textLayout->setFont(layoutFont);
    layout.textLayout->setFormats(formats);

    // Perform line layout
    layout.textLayout->beginLayout();
    int lineY = 0;
    while (true) {
        QTextLine line = layout.textLayout->createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(qMax(tw, 1));
        line.setPosition(QPointF(0, lineY));
        lineY += qCeil(line.height());
    }
    layout.textLayout->endLayout();

    // Compute item height by kind
    const int textH = qMax(0, qCeil(layout.textLayout->boundingRect().height()));

    switch (msg.kind) {
        case ChatMessage::Full:
            layout.textTop = headerH + textTopPad;
            layout.totalHeight = qMax(layout.textTop + textH + textBottomPad, headerH + textBottomPad);
            break;
        case ChatMessage::Compressed:
            layout.textTop = compressedTopPad;
            layout.totalHeight = layout.textTop + textH + compressedBottomPad;
            break;
        case ChatMessage::Server:
            layout.textTop = serverHeaderH + serverTopPad; // header band + gap
            layout.totalHeight = layout.textTop + textH + serverBottomPad;
            break;
    }
    layout.totalHeight += cardSpacingV;
    // Note: layout.textLeft already set above — do NOT reassign textLeft() here
    layout.valid = true;

    // Precompute link hit areas (local to text origin)
    layout.linkAreas.clear();
    for (const ChatSpan &span : msg.spans) {
        if (span.type != ChatSpan::CardLink && span.type != ChatSpan::UrlLink && span.type != ChatSpan::OtherMention) {
            continue;
        }

        QRectF spanRect;
        const int spanEnd = span.start + span.length;
        for (int l = 0; l < layout.textLayout->lineCount(); ++l) {
            const QTextLine line = layout.textLayout->lineAt(l);
            const int lineEnd = line.textStart() + line.textLength();
            if (span.start >= lineEnd || spanEnd <= line.textStart()) {
                continue;
            }
            const int ovStart = qMax(span.start, line.textStart());
            const int ovEnd = qMin(spanEnd, lineEnd);
            qreal x1 = line.cursorToX(ovStart, QTextLine::Leading);
            qreal x2 = line.cursorToX(ovEnd, QTextLine::Trailing);
            if (x1 > x2) {
                qSwap(x1, x2);
            }
            const QRectF r(x1, line.y(), x2 - x1, line.height());
            spanRect = spanRect.isEmpty() ? r : spanRect.united(r);
        }
        if (!spanRect.isEmpty()) {
            layout.linkAreas.append({spanRect, span.href});
        }
    }
}

void ChatView::relayoutAll(int availableWidth)
{
    for (int i = 0; i < m_messages.size(); ++i) {
        layoutMessage(i, availableWidth);
    }
    m_totalH = 0;
    for (const MessageLayout &l : m_layouts) {
        m_totalH += l.totalHeight;
    }
    m_lastWidth = availableWidth;
    updateScrollRange();
}

void ChatView::updateScrollRange()
{
    const int vh = viewport()->height();
    verticalScrollBar()->setRange(0, qMax(0, m_totalH - vh));
    verticalScrollBar()->setPageStep(vh);
}

void ChatView::scrollToBottom()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

// ── Adding messages ───────────────────────────────────────────────────────────

void ChatView::addMessage(ChatMessage &&msg)
{
    const bool atBottom = verticalScrollBar()->value() >= verticalScrollBar()->maximum() - 2;

    m_messages.push_back(std::move(msg));
    m_layouts.push_back(MessageLayout{});

    const int idx = m_messages.size() - 1;
    layoutMessage(idx, viewport()->width());
    m_totalH += m_layouts.at(idx).totalHeight;
    updateScrollRange();

    if (atBottom) {
        scrollToBottom();
    }
    viewport()->update();
}

void ChatView::appendMessage(QString raw,
                             RoomMessageTypeFlags messageType,
                             const ServerInfo_User &userInfo,
                             bool /*playerBold*/)
{
    m_highlightedWords = SettingsCache::instance().getHighlightWords().split(' ', Qt::SkipEmptyParts);
    const bool mentionEnabled = SettingsCache::instance().getChatMention();

    const QString userName = QString::fromStdString(userInfo.name());
    const bool isUserMessage = !(userName.toLower() == "servatrice" || userName.isEmpty());

    ChatMessage msg;
    msg.timestamp = QDateTime::currentDateTime();
    msg.userInfo = userInfo;

    if (isUserMessage) {
        // ── Normal user message ───────────────────────────────────────────────
        const bool sameSender =
            (userName == m_lastSender) && !m_messages.isEmpty() && m_messages.last().kind != ChatMessage::Server;
        msg.kind = sameSender ? ChatMessage::Compressed : ChatMessage::Full;
        msg.accentColor = sameSender ? m_messages.last().accentColor // inherit from parent
                                     : accentForUser(userInfo);
        m_lastSender = userName;
        parseMessageText(raw, msg, mentionEnabled);

    } else if (messageType == Event_RoomSay::ChatHistory) {
        // ── Chat history: "[d MMM yyyy HH:mm:ss] sender: body" ────────────────
        // processRoomSayEvent prepends the timestamp before calling us, so the
        // sender name is embedded in the text when userInfo is unavailable.
        static const QRegularExpression histRe(QStringLiteral("^\\[([^\\]]+)\\]\\s+(\\S+):\\s+(.*)$"));
        const QRegularExpressionMatch m = histRe.match(raw);

        if (m.hasMatch()) {
            const QString tsStr = m.captured(1);
            const QString sender = m.captured(2);
            const QString body = m.captured(3);

            ServerInfo_User synth;
            synth.set_name(sender.toStdString());
            msg.userInfo = synth;
            const bool sameHistorySender =
                sender == m_lastHistorySender && !m_messages.isEmpty() && m_messages.last().kind != ChatMessage::Server;
            m_lastHistorySender = sender;

            msg.kind = sameHistorySender ? ChatMessage::Compressed : ChatMessage::Full;
            msg.accentColor = accentForUser(synth);

            const QDateTime hist = QDateTime::fromString(tsStr, QStringLiteral("d MMM yyyy HH:mm:ss"));
            if (hist.isValid()) {
                msg.timestamp = hist;
            }

            // Fetch the user's full info and avatar from the server.
            // requestAvatar is a no-op if the entry is already cached or in flight.
            tabSupervisor->getUserListManager()->getAvatarProvider()->requestAvatar(sender);

            m_lastSender.clear();
            parseMessageText(body, msg, false);
        } else {
            // Unrecognised format — plain server line
            msg.kind = ChatMessage::Server;
            msg.accentColor = QColor(65, 80, 100);
            msg.serverColor = QStringLiteral("#888888");
            m_lastSender.clear();
            parseMessageText(raw, msg, false);
        }

    } else {
        // ── System / server message ───────────────────────────────────────────
        msg.kind = ChatMessage::Server;
        msg.accentColor = QColor(65, 80, 100);
        // Server messages break any live compression chain
        m_lastSender.clear();
        parseMessageText(raw, msg, mentionEnabled);
    }

    addMessage(std::move(msg));
}

void ChatView::appendHtmlServerMessage(const QString &html, bool bold, QString color)
{
    ChatMessage msg;
    msg.kind = ChatMessage::Server;
    msg.timestamp = QDateTime::currentDateTime();
    msg.serverBold = bold;
    msg.serverColor = color;
    msg.text = stripHtml(html);
    if (!msg.text.isEmpty()) {
        msg.spans.append({ChatSpan::Plain, 0, static_cast<int>(msg.text.length()), {}});
    }
    addMessage(std::move(msg));
}

void ChatView::appendHtml(const QString &html)
{
    appendHtmlServerMessage(html, false, {});
}

// ── Span parsing ──────────────────────────────────────────────────────────────

void ChatView::appendSpan(ChatMessage &msg, ChatSpan::Type type, const QString &text, const QString &href)
{
    if (text.isEmpty()) {
        return;
    }
    ChatSpan span;
    span.type = type;
    span.start = msg.text.length();
    span.length = text.length();
    span.href = href;
    msg.text += text;
    msg.spans << span;
}

void ChatView::parseMessageText(QString raw, ChatMessage &msg, bool mentionEnabled)
{
    while (!raw.isEmpty()) {
        const QChar c = raw.at(0);
        switch (c.toLatin1()) {
            case '[':
                parseTag(raw, msg);
                break;
            case '@':
                if (mentionEnabled) {
                    parseMention(raw, msg);
                } else {
                    appendSpan(msg, ChatSpan::Plain, QString(c));
                    raw = raw.mid(1);
                }
                break;
            default:
                if (c.isLetterOrNumber()) {
                    parseWord(raw, msg);
                } else {
                    appendSpan(msg, ChatSpan::Plain, QString(c));
                    raw = raw.mid(1);
                }
                break;
        }
    }
}

void ChatView::parseTag(QString &raw, ChatMessage &msg)
{
    // [card]Name[/card]
    if (raw.startsWith("[card]")) {
        raw = raw.mid(6);
        const int closeIdx = raw.indexOf("[/card]");
        const QString name = (closeIdx == -1) ? raw : raw.left(closeIdx);
        raw = (closeIdx == -1) ? QString{} : raw.mid(closeIdx + 7);
        appendSpan(msg, ChatSpan::CardLink, name, "card://" + name);
        return;
    }

    // [[Name]]
    if (raw.startsWith("[[")) {
        raw = raw.mid(2);
        const int closeIdx = raw.indexOf("]]");
        const QString name = (closeIdx == -1) ? raw : raw.left(closeIdx);
        raw = (closeIdx == -1) ? QString{} : raw.mid(closeIdx + 2);
        appendSpan(msg, ChatSpan::CardLink, name, "card://" + name);
        return;
    }

    // [url]Link[/url]
    if (raw.startsWith("[url]")) {
        raw = raw.mid(5);
        const int closeIdx = raw.indexOf("[/url]");
        const QString url = (closeIdx == -1) ? raw : raw.left(closeIdx);
        raw = (closeIdx == -1) ? QString{} : raw.mid(closeIdx + 6);
        const QString href = url.startsWith("www.") ? "https://" + url : url;
        appendSpan(msg, ChatSpan::UrlLink, url, href);
        return;
    }

    // No valid tag — fall through to word parsing
    parseWord(raw, msg);
}

void ChatView::parseMention(QString &raw, ChatMessage &msg)
{
    static const QRegularExpression notAlphaNum(QStringLiteral("[^a-zA-Z0-9]"));

    const int firstSpace = raw.indexOf(' ');
    QString candidate = (firstSpace == -1) ? raw.mid(1) : raw.mid(1, firstSpace - 1);
    const QString original = candidate;

    while (!candidate.isEmpty()) {
        // Self-mention
        if (m_ownUserName.toLower() == candidate.toLower()) {
            soundEngine->playSound("chat_mention");
            appendSpan(msg, ChatSpan::SelfMention, m_mention);
            raw = raw.mid(m_mention.size());
            showSystemPopup(QString::fromStdString(msg.userInfo.name()));
            return;
        }

        // Global /all from moderator
        if (isModeratorSendingGlobal(UserLevelFlags(msg.userInfo.user_level()), candidate)) {
            soundEngine->playSound("all_mention");
            appendSpan(msg, ChatSpan::SelfMention, "@" + candidate);
            raw = raw.mid(candidate.size() + 1);
            showSystemPopup(QString::fromStdString(msg.userInfo.name()));
            return;
        }

        // Other online user
        const ServerInfo_User *onlineUser = userListProxy->getOnlineUser(candidate);
        if (onlineUser) {
            const QString correctName = QString::fromStdString(onlineUser->name());
            const QString href = QStringLiteral("user://%1_%2").arg(onlineUser->user_level()).arg(correctName);
            appendSpan(msg, ChatSpan::OtherMention, "@" + correctName, href);
            raw = raw.mid(correctName.size() + 1);
            return;
        }

        if (candidate.right(1).indexOf(notAlphaNum) == -1 || candidate.size() < 2) {
            appendSpan(msg, ChatSpan::Plain, "@" + original);
            raw = raw.mid(original.size() + 1);
            return;
        }
        candidate.chop(1);
    }

    parseWord(raw, msg);
}

void ChatView::parseWord(QString &raw, ChatMessage &msg)
{
    // Extract next word (up to first space)
    const int firstSpace = raw.indexOf(' ');
    QString word;
    if (firstSpace == -1) {
        word = raw;
        raw.clear();
    } else {
        word = raw.left(firstSpace);
        raw = raw.mid(firstSpace);
    }

    // Strip trailing punctuation into a separate suffix
    int lastAlnum = word.size() - 1;
    while (lastAlnum >= 0 && !word.at(lastAlnum).isLetterOrNumber()) {
        --lastAlnum;
    }
    const QString suffix = word.mid(lastAlnum + 1);
    word = word.left(lastAlnum + 1);

    // URL check
    if (word.startsWith("http://", Qt::CaseInsensitive) || word.startsWith("https://", Qt::CaseInsensitive) ||
        word.startsWith("www.", Qt::CaseInsensitive)) {
        const QUrl qurl(word);
        if (qurl.isValid()) {
            const QString href = word.startsWith("www.", Qt::CaseInsensitive) ? "https://" + word : word;
            appendSpan(msg, ChatSpan::UrlLink, word, href);
            if (!suffix.isEmpty()) {
                appendSpan(msg, ChatSpan::Plain, suffix);
            }
            return;
        }
    }

    // Custom highlighted word
    for (const QString &hw : m_highlightedWords) {
        if (word.compare(hw, Qt::CaseInsensitive) == 0) {
            appendSpan(msg, ChatSpan::Highlight, word + suffix);
            return;
        }
    }

    appendSpan(msg, ChatSpan::Plain, word + suffix);
}

// ── Painting ──────────────────────────────────────────────────────────────────

void ChatView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(viewport());
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    p.fillRect(viewport()->rect(), QColor(12, 15, 22));

    const int scrollY = verticalScrollBar()->value();
    const int viewH = viewport()->height();
    // const int vw      = viewport()->width();

    int y = 0;
    for (int i = 0; i < m_messages.size(); ++i) {
        const int itemH = m_layouts.at(i).totalHeight;
        const int screenY = y - scrollY;

        if (screenY + itemH < 0) {
            y += itemH;
            continue;
        }
        if (screenY >= viewH) {
            break;
        }

        paintMessage(p, i, screenY);
        y += itemH;
    }
}

void ChatView::paintMessage(QPainter &p, int idx, int screenY) const
{
    const ChatMessage &msg = m_messages.at(idx);
    const MessageLayout &layout = m_layouts.at(idx);
    const int vw = viewport()->width();

    // ── Compressed: text only, sits inside the parent Full card ──────────────
    if (msg.kind == ChatMessage::Compressed) {
        paintText(p, idx, screenY + layout.textTop);
        return;
    }

    // ── Full / Server: compute how far the card extends ───────────────────────
    //
    // For Full messages the card stretches downward to cover all immediately
    // following Compressed messages, making them visually part of the same card.
    // The final cardSpacingV (inter-card gap) comes from the last item in the group.

    int groupEndY = screenY + layout.totalHeight; // Y after this item's cardSpacingV

    if (msg.kind == ChatMessage::Full) {
        for (int j = idx + 1; j < m_messages.size(); ++j) {
            if (m_messages.at(j).kind != ChatMessage::Compressed) {
                break;
            }
            groupEndY += m_layouts.at(j).totalHeight;
        }
    }

    // Card bottom = groupEnd minus the trailing spacing and margin
    const int cardTop = screenY + cardMarginV;
    const int cardBottom = groupEndY - cardSpacingV - cardMarginV;

    const QRectF card(cardMarginH, cardTop, vw - 2 * cardMarginH, cardBottom - cardTop);

    paintCardBackground(p, card, msg.accentColor);

    if (msg.kind == ChatMessage::Full) {
        paintHeader(p, QRect(0, screenY, vw, headerH), msg, msg.accentColor);
    } else if (msg.kind == ChatMessage::Server && showTimestamps) {
        // Mini-header: just a right-aligned timestamp, no avatar or username
        QFont tsFont = font();
        tsFont.setPointSizeF(tsFont.pointSizeF() * 0.78);
        p.setFont(tsFont);
        p.setPen(QColor(70, 85, 105));
        const QString ts = msg.timestamp.toString(QStringLiteral("hh:mm"));
        const int tsW = QFontMetrics(tsFont).horizontalAdvance(ts) + 6;
        p.drawText(QRect(vw - rightPad - tsW, screenY + cardMarginV, tsW, serverHeaderH),
                   Qt::AlignVCenter | Qt::AlignRight, ts);
    }

    paintText(p, idx, screenY + layout.textTop);
}

void ChatView::paintCardBackground(QPainter &p, const QRectF &card, const QColor &accent) const
{
    QLinearGradient bg(card.topLeft(), card.bottomRight());
    bg.setColorAt(0, QColor(22, 28, 38));
    bg.setColorAt(1, QColor(14, 18, 26));
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(card, cardRadius, cardRadius);

    p.setBrush(accent);
    p.drawRoundedRect(QRectF(card.left(), card.top(), accentBarWidth, card.height()), accentBarRadius, accentBarRadius);

    p.setPen(QPen(accent.darker(300), 0.5));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(card.adjusted(0.5, 0.5, -0.5, -0.5), cardRadius, cardRadius);
}

void ChatView::paintHeader(QPainter &p, const QRect &headerRect, const ChatMessage &msg, const QColor &accent) const
{
    const QString userName = QString::fromStdString(msg.userInfo.name());
    const UserLevelFlags level(msg.userInfo.user_level());

    // Avatar
    const int avatarX = headerRect.left() + leftPad;
    const int avatarY = headerRect.top() + (headerH - avatarSize) / 2;
    const QRect avatarRect(avatarX, avatarY, avatarSize, avatarSize);
    paintAvatar(p, avatarRect, msg, accent);

    const int tx = avatarX + avatarSize + avatarGap;
    const int rx = headerRect.right() - rightPad;

    // Timestamp
    int tsReserve = 0;
    if (showTimestamps) {
        QFont tsFont = font();
        tsFont.setPointSizeF(tsFont.pointSizeF() * 0.78);
        p.setFont(tsFont);
        p.setPen(QColor(70, 85, 105));
        const QString ts = msg.timestamp.toString(QStringLiteral("hh:mm"));
        tsReserve = QFontMetrics(tsFont).horizontalAdvance(ts) + 6;
        p.drawText(QRect(rx - tsReserve, headerRect.top(), tsReserve, headerH), Qt::AlignVCenter | Qt::AlignRight, ts);
    }

    // Username — coloured by level
    QFont nameFont = font();
    nameFont.setBold(true);
    p.setFont(nameFont);

    const QColor nameColor = [&]() -> QColor {
        if (level.testFlag(ServerInfo_User::IsAdmin)) {
            return QColor(245, 158, 11);
        }
        if (level.testFlag(ServerInfo_User::IsModerator)) {
            return QColor(59, 130, 246);
        }
        if (level.testFlag(ServerInfo_User::IsJudge)) {
            return QColor(168, 85, 247);
        }
        if (userName == m_ownUserName) {
            return QColor(74, 222, 128);
        }
        return QColor(210, 220, 235);
    }();

    const int nameW = rx - tx - tsReserve - 4;
    const QString elidedName = QFontMetrics(nameFont).elidedText(userName, Qt::ElideRight, nameW);
    p.setPen(nameColor);
    p.drawText(QRect(tx, headerRect.top(), nameW, headerH), Qt::AlignVCenter | Qt::AlignLeft, elidedName);
}

void ChatView::paintAvatar(QPainter &p, const QRect &avatarRect, const ChatMessage &msg, const QColor &accent) const
{
    const QString userName = QString::fromStdString(msg.userInfo.name());
    const UserLevelFlags level(msg.userInfo.user_level());
    const QString privLevel = QString::fromStdString(msg.userInfo.privlevel());

    QPainterPath clip;
    clip.addEllipse(avatarRect);
    p.save();
    p.setClipPath(clip);

    bool drewAvatar = false;
    const auto *avatarCache = &tabSupervisor->getUserListManager()->getAvatarProvider()->cache();
    if (avatarCache) {
        const auto it = avatarCache->find(userName);
        if (it != avatarCache->end() && !it->isNull()) {
            p.drawPixmap(avatarRect,
                         it->scaled(avatarRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            drewAvatar = true;
        }
    }

    if (!drewAvatar) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(28, 35, 46));
        p.drawEllipse(avatarRect);

        const QPixmap pawn = UserLevelPixmapGenerator::generatePixmap(avatarFallbackPawn, level,
                                                                      msg.userInfo.pawn_colors(), false, privLevel);
        p.drawPixmap(avatarRect.center().x() - avatarFallbackPawn / 2, avatarRect.center().y() - avatarFallbackPawn / 2,
                     pawn);
    }

    p.restore();

    p.setPen(QPen(accent, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(avatarRect).adjusted(-0.75, -0.75, 0.75, 0.75));
}

void ChatView::paintText(QPainter &p, int idx, int textScreenY) const
{
    const MessageLayout &layout = m_layouts.at(idx);
    const ChatMessage &msg = m_messages.at(idx);

    // Selection ranges for this message
    QVector<QTextLayout::FormatRange> selections;
    SelPoint lo, hi;
    normalisedSelection(lo, hi);

    if (lo.msg >= 0 && idx >= lo.msg && idx <= hi.msg) {
        const int selStart = (idx == lo.msg) ? lo.pos : 0;
        const int selEnd = (idx == hi.msg) ? hi.pos : msg.text.length();
        const int selLen = selEnd - selStart;
        if (selLen > 0) {
            QTextLayout::FormatRange sel;
            sel.start = selStart;
            sel.length = selLen;
            sel.format.setBackground(QColor(59, 130, 246, 110));
            sel.format.setForeground(Qt::white);
            selections << sel;
        }
    }

    // Base text colour — QTextLayout uses the painter pen for spans without
    // an explicit foreground format (i.e. ChatSpan::Plain)
    const QColor baseColor = (msg.kind == ChatMessage::Server)
                                 ? (msg.serverColor.isEmpty() ? QColor(0xFF, 0x73, 0x83) : QColor(msg.serverColor))
                                 : QColor(195, 208, 225);

    p.save();
    p.setPen(baseColor);
    p.translate(layout.textLeft, textScreenY);
    layout.textLayout->draw(&p, QPointF(0, 0), selections);
    p.restore();
}

// ── Colours ───────────────────────────────────────────────────────────────────

/*static*/ QColor ChatView::accentForUser(const ServerInfo_User &user)
{
    const UserLevelFlags level(user.user_level());
    if (level.testFlag(ServerInfo_User::IsAdmin)) {
        return QColor(245, 158, 11);
    }
    if (level.testFlag(ServerInfo_User::IsModerator)) {
        return QColor(59, 130, 246);
    }
    if (level.testFlag(ServerInfo_User::IsJudge)) {
        return QColor(168, 85, 247);
    }
    return QColor(75, 90, 110);
}

QTextCharFormat ChatView::formatForSpan(const ChatSpan &span) const
{
    QTextCharFormat fmt;
    switch (span.type) {
        case ChatSpan::Plain:
            break; // base painter pen colour used
        case ChatSpan::SelfMention:
            fmt.setFontWeight(QFont::Bold);
            fmt.setBackground(QColor(194, 31, 47, 180));
            fmt.setForeground(Qt::white);
            break;
        case ChatSpan::OtherMention:
            fmt.setFontWeight(QFont::Bold);
            fmt.setForeground(m_linkColor);
            break;
        case ChatSpan::CardLink:
            fmt.setFontItalic(true);
            fmt.setForeground(m_linkColor);
            break;
        case ChatSpan::UrlLink:
            fmt.setFontUnderline(true);
            fmt.setForeground(m_linkColor);
            break;
        case ChatSpan::Highlight:
            fmt.setFontWeight(QFont::Bold);
            fmt.setBackground(QColor(194, 31, 47, 120));
            fmt.setForeground(Qt::white);
            break;
    }
    return fmt;
}

// ── Hit-testing ───────────────────────────────────────────────────────────────

QPair<int, int> ChatView::hitTest(const QPoint &viewportPos) const
{
    const int absY = viewportPos.y() + verticalScrollBar()->value();

    int y = 0;
    for (int i = 0; i < m_layouts.size(); ++i) {
        const MessageLayout &layout = m_layouts.at(i);
        const int itemH = layout.totalHeight;
        if (absY < y + itemH) {
            if (!layout.valid || m_messages.at(i).text.isEmpty()) {
                return {i, 0};
            }
            const int localX = viewportPos.x() - layout.textLeft;
            const int localY = absY - y - layout.textTop;

            int charPos = m_messages.at(i).text.length();
            for (int l = 0; l < layout.textLayout->lineCount(); ++l) {
                const QTextLine line = layout.textLayout->lineAt(l);
                if (localY >= line.y() && localY < line.y() + line.height()) {
                    charPos = line.xToCursor(localX, QTextLine::CursorBetweenCharacters);
                    break;
                }
                if (l == layout.textLayout->lineCount() - 1 && localY >= line.y() + line.height()) {
                    charPos = line.textStart() + line.textLength();
                }
            }
            return {i, charPos};
        }
        y += itemH;
    }
    return {-1, 0};
}

QString ChatView::hrefAt(const QPoint &viewportPos) const
{
    const int absY = viewportPos.y() + verticalScrollBar()->value();

    int y = 0;
    for (int i = 0; i < m_layouts.size(); ++i) {
        const MessageLayout &layout = m_layouts.at(i);
        if (absY < y + layout.totalHeight) {
            const qreal localX = viewportPos.x() - layout.textLeft;
            const qreal localY = absY - y - layout.textTop;
            for (const LinkHitArea &area : layout.linkAreas) {
                if (area.rect.contains(localX, localY)) {
                    return area.href;
                }
            }
            return {};
        }
        y += layout.totalHeight;
    }
    return {};
}

void ChatView::updateHover(const QPoint &viewportPos)
{
    const QString href = hrefAt(viewportPos);
    if (href == m_hoveredHref) {
        return;
    }
    m_hoveredHref = href;

    if (href.isEmpty()) {
        m_hoveredType = HoveredNothing;
        viewport()->setCursor(Qt::IBeamCursor);
        emit deleteCardInfoPopup(QStringLiteral("_"));
        return;
    }

    const int delim = href.indexOf("://");
    const QString scheme = href.left(delim);
    const QString content = href.mid(delim + 3);

    if (scheme == "card") {
        m_hoveredType = HoveredCard;
        viewport()->setCursor(Qt::PointingHandCursor);
        emit cardNameHovered(content);
    } else if (scheme == "user") {
        m_hoveredType = HoveredUser;
        viewport()->setCursor(Qt::PointingHandCursor);
    } else {
        m_hoveredType = HoveredUrl;
        viewport()->setCursor(Qt::PointingHandCursor);
    }
}

// ── Selection ─────────────────────────────────────────────────────────────────

void ChatView::normalisedSelection(SelPoint &lo, SelPoint &hi) const
{
    if (m_selAnchor.msg < 0 || m_selFocus.msg < 0) {
        lo = hi = {-1, 0};
        return;
    }
    const bool anchorFirst =
        m_selAnchor.msg < m_selFocus.msg || (m_selAnchor.msg == m_selFocus.msg && m_selAnchor.pos <= m_selFocus.pos);
    lo = anchorFirst ? m_selAnchor : m_selFocus;
    hi = anchorFirst ? m_selFocus : m_selAnchor;
}

QString ChatView::selectedText() const
{
    SelPoint lo, hi;
    normalisedSelection(lo, hi);
    if (lo.msg < 0) {
        return {};
    }

    QString result;
    for (int i = lo.msg; i <= hi.msg && i < m_messages.size(); ++i) {
        const QString &text = m_messages.at(i).text;
        const int start = (i == lo.msg) ? lo.pos : 0;
        const int end = (i == hi.msg) ? hi.pos : text.length();
        if (i > lo.msg) {
            result += '\n';
        }
        result += text.mid(start, end - start);
    }
    return result;
}

// ── Event handlers ────────────────────────────────────────────────────────────

/*void ChatView::scrollContentsBy(int dx#1#, int dy#1#)
{
    viewport()->update();
}*/

void ChatView::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);

    const bool atBottom = verticalScrollBar()->value() >= verticalScrollBar()->maximum() - 2;

    const int vw = viewport()->width();

    if (vw != m_lastWidth) {
        relayoutAll(vw);
    } else {
        updateScrollRange();
    }

    if (atBottom) {
        scrollToBottom();
    }
}
void ChatView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const auto [msgIdx, charPos] = hitTest(event->pos());
        m_selAnchor = {msgIdx, charPos};
        m_selFocus = {msgIdx, charPos};
        m_selecting = true;
        viewport()->update();
        return;
    }

    if (event->button() == Qt::RightButton) {
        const QString href = hrefAt(event->pos());
        if (!href.isEmpty()) {
            const int delim = href.indexOf("://");
            const QString scheme = href.left(delim);
            const QString content = href.mid(delim + 3);
            if (scheme == "user") {
                const int underscore = content.indexOf('_');
                UserLevelFlags level(content.left(underscore).toInt());
                const QString userName = content.mid(underscore + 1);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                userContextMenu->showContextMenu(event->globalPosition().toPoint(), userName, level, this);
#else
                userContextMenu->showContextMenu(event->globalPos(), userName, level, this);
#endif
            }
        }
    }
}

void ChatView::mouseMoveEvent(QMouseEvent *event)
{
    updateHover(event->pos());

    if (m_selecting && (event->buttons() & Qt::LeftButton)) {
        const auto [msgIdx, charPos] = hitTest(event->pos());
        if (msgIdx >= 0) {
            m_selFocus = {msgIdx, charPos};
            viewport()->update();
        }
    }
}

void ChatView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_selecting = false;

        // Zero-length selection = click — activate any link under the cursor
        if (m_selAnchor.msg == m_selFocus.msg && m_selAnchor.pos == m_selFocus.pos) {
            const QString href = hrefAt(event->pos());
            if (!href.isEmpty()) {
                const int delim = href.indexOf("://");
                const QString scheme = href.left(delim);
                const QString content = href.mid(delim + 3);

                if (scheme == "card") {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                    emit showCardInfoPopup(event->globalPosition().toPoint(), {content});
#else
                    emit showCardInfoPopup(event->globalPos(), {content});
#endif
                } else if (scheme == "user") {
                    const int underscore = content.indexOf('_');
                    const QString userName = content.mid(underscore + 1);
                    if (event->modifiers() == Qt::ControlModifier) {
                        emit openMessageDialog(userName, true);
                    } else {
                        emit addMentionTag("@" + userName);
                    }
                } else {
                    QDesktopServices::openUrl(QUrl(href));
                }
            }
        }
    }

    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        emit deleteCardInfoPopup(QStringLiteral("_"));
    }
}

void ChatView::wheelEvent(QWheelEvent *event)
{
    if (!event->pixelDelta().isNull()) {
        // Smooth trackpad / high-resolution wheel: use pixel delta directly
        verticalScrollBar()->setValue(verticalScrollBar()->value() - event->pixelDelta().y());
    } else {
        // Standard notched mouse wheel: 120 units per notch → scroll 60px
        const int pixels = -(event->angleDelta().y() * 60) / 120;
        verticalScrollBar()->setValue(verticalScrollBar()->value() + pixels);
    }
    event->accept();
}

void ChatView::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy)) {
        const QString text = selectedText();
        if (!text.isEmpty()) {
            QGuiApplication::clipboard()->setText(text);
        }
        return;
    }
    QAbstractScrollArea::keyPressEvent(event);
}

void ChatView::leaveEvent(QEvent * /*event*/)
{
    m_hoveredHref.clear();
    m_hoveredType = HoveredNothing;
    viewport()->setCursor(Qt::IBeamCursor);
}

// ── Public API ────────────────────────────────────────────────────────────────

void ChatView::clearChat()
{
    m_messages.clear();
    m_layouts.clear();
    m_totalH = 0;
    m_lastSender.clear();
    m_selAnchor = {-1, 0};
    m_selFocus = {-1, 0};
    updateScrollRange();
    viewport()->update();
}

void ChatView::redactMessages(const QString &userName, int amount)
{
    const QString placeholder = tr("[message removed]");
    int count = 0;
    for (int i = m_messages.size() - 1; i >= 0 && count < amount; --i) {
        const QString msgUser = QString::fromStdString(m_messages.at(i).userInfo.name());
        if (msgUser == userName && !m_messages.at(i).redacted) {
            m_messages[i].text = placeholder;
            m_messages[i].spans = {{ChatSpan::Plain, 0, static_cast<int>(placeholder.length()), {}}};
            m_messages[i].redacted = true;
            layoutMessage(i, viewport()->width());
            ++count;
        }
    }
    m_totalH = 0;
    for (const MessageLayout &l : m_layouts) {
        m_totalH += l.totalHeight;
    }
    updateScrollRange();
    viewport()->update();
}

// ── Misc ──────────────────────────────────────────────────────────────────────

void ChatView::showSystemPopup(const QString &userName)
{
    QApplication::alert(this);
    if (SettingsCache::instance().getShowMentionPopup()) {
        emit showMentionPopup(userName);
    }
}

/*static*/ bool ChatView::isModeratorSendingGlobal(QFlags<ServerInfo_User::UserLevelFlag> level, const QString &word)
{
    static const QStringList globalCommands{QStringLiteral("/all")};
    return globalCommands.contains(word) && (level & ServerInfo_User::IsModerator || level & ServerInfo_User::IsAdmin);
}

/*static*/ QString ChatView::stripHtml(const QString &html)
{
    static const QRegularExpression tagRe(QStringLiteral("<[^>]*>"));
    QString result = html;
    result.remove(tagRe);
    result.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    result.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    result.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    result.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    return result.trimmed();
}