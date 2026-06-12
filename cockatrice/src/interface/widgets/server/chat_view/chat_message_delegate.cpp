#include "chat_message_delegate.h"

#include "../user/user_avatar_provider.h"
#include "chat_message.h"
#include "chat_model.h"

#include <QApplication>
#include <QPainter>

ChatMessageDelegate::ChatMessageDelegate(UserAvatarProvider *avatarProvider, QObject *parent)
    : QStyledItemDelegate(parent), avatarProvider(avatarProvider)
{
}

QSize ChatMessageDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    return {option.rect.width(), 72};
}

QRect ChatMessageDelegate::paintHeader(QPainter *painter,
                                       const QRect &rect,
                                       const ChatMessage &msg,
                                       const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    constexpr int avatarSize = 40;

    QRect avatarRect(rect.left() + 12, rect.top() + 8, avatarSize, avatarSize);

    if (avatarProvider) {
        const auto &cache = avatarProvider->cache();

        auto it = cache.find(msg.sender);

        if (it != cache.end() && !it.value().isNull()) {
            painter->drawPixmap(avatarRect, it.value());
        }
    }

    QRect textRect(avatarRect.right() + 12, rect.top() + 6, rect.width() - avatarRect.width() - 30, 20);

    QFont nameFont = painter->font();

    nameFont.setBold(true);

    painter->setFont(nameFont);

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, msg.sender);

    QRect timestampRect(textRect.right() - 80, textRect.top(), 80, textRect.height());

    painter->setPen(QApplication::palette().color(QPalette::Mid));

    painter->drawText(timestampRect, Qt::AlignRight | Qt::AlignVCenter, msg.timestamp.toString("hh:mm"));

    ChatHitRegion region;

    region.type = ChatHitRegion::User;
    region.rect = textRect;
    region.payload = msg.sender;

    hitRegions[index].push_back(region);

    return QRect(textRect.left(), textRect.bottom() + 6, rect.width() - textRect.left(), rect.height());
}

void ChatMessageDelegate::paintChip(QPainter *painter,
                                    QRect &cursorRect,
                                    const QString &text,
                                    const QColor &background,
                                    const QColor &foreground,
                                    ChatHitRegion::Type type,
                                    const QString &payload,
                                    const QModelIndex &index) const
{
    QFontMetrics fm(painter->font());

    QRect chipRect(cursorRect.left(), cursorRect.top(), fm.horizontalAdvance(text) + 16, fm.height() + 6);

    painter->setPen(Qt::NoPen);
    painter->setBrush(background);

    painter->drawRoundedRect(chipRect, 6, 6);

    painter->setPen(foreground);

    painter->drawText(chipRect.adjusted(8, 0, -8, 0), Qt::AlignCenter, text);

    ChatHitRegion region;

    region.type = type;
    region.payload = payload;
    region.rect = chipRect;

    hitRegions[index].push_back(region);

    cursorRect.moveLeft(chipRect.right() + 4);
}

QRect ChatMessageDelegate::paintTokens(QPainter *painter,
                                       const QRect &rect,
                                       const ChatMessage &msg,
                                       const QModelIndex &index) const
{
    QRect cursorRect(rect.left(), rect.top(), 0, 0);

    QFontMetrics fm(painter->font());

    for (const ChatToken &token : msg.tokens) {

        switch (token.type) {

            case ChatToken::Text: {
                QRect textRect(cursorRect.left(), cursorRect.top(), fm.horizontalAdvance(token.text), fm.height());

                painter->setPen(QApplication::palette().text().color());

                painter->drawText(textRect, Qt::AlignLeft, token.text);

                cursorRect.moveLeft(textRect.right());

                break;
            }

            case ChatToken::Mention: {
                paintChip(painter, cursorRect, token.text, QColor(194, 31, 47), Qt::white, ChatHitRegion::Mention,
                          token.payload, index);

                break;
            }

            case ChatToken::Card: {
                paintChip(painter, cursorRect, token.text, QColor(50, 90, 180), Qt::white, ChatHitRegion::Card,
                          token.payload, index);

                break;
            }

            case ChatToken::Url: {
                paintChip(painter, cursorRect, token.text, QColor(70, 70, 70), Qt::white, ChatHitRegion::Url,
                          token.payload, index);

                break;
            }

            case ChatToken::Highlight: {
                paintChip(painter, cursorRect, token.text, QColor(255, 180, 0), Qt::black, ChatHitRegion::Mention,
                          token.payload, index);

                break;
            }
        }
    }

    return cursorRect;
}

void ChatMessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const ChatMessage *msg = static_cast<const ChatMessage *>(index.data(ChatModel::MessageRole).value<void *>());

    if (!msg) {
        return;
    }

    hitRegions[index].clear();

    painter->save();

    QRect cardRect = option.rect.adjusted(4, 4, -4, -4);

    painter->setPen(Qt::NoPen);

    painter->setBrush(option.state & QStyle::State_Selected ? QApplication::palette().highlight()
                                                            : QApplication::palette().base());

    painter->drawRoundedRect(cardRect, 8, 8);

    QRect bodyRect = paintHeader(painter, cardRect, *msg, option, index);

    paintTokens(painter, bodyRect, *msg, index);

    painter->restore();
}

ChatHitRegion ChatMessageDelegate::hitTest(const QModelIndex &index, const QPoint &viewportPos) const
{
    auto it = hitRegions.find(index);

    if (it == hitRegions.end()) {
        return {};
    }

    for (const ChatHitRegion &region : it.value()) {
        if (region.rect.contains(viewportPos)) {
            return region;
        }
    }

    return {};
}