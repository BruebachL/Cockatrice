#ifndef COCKATRICE_CHAT_MESSAGE_DELEGATE_H
#define COCKATRICE_CHAT_MESSAGE_DELEGATE_H

#include "chat_hit_region.h"

#include <QHash>
#include <QPersistentModelIndex>
#include <QStyledItemDelegate>

class ChatMessage;
class UserAvatarProvider;

class ChatMessageDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ChatMessageDelegate(UserAvatarProvider *avatarProvider, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    ChatHitRegion hitTest(const QModelIndex &index, const QPoint &viewportPos) const;

private:
    QRect paintHeader(QPainter *painter,
                      const QRect &rect,
                      const ChatMessage &msg,
                      const QStyleOptionViewItem &option,
                      const QModelIndex &index) const;

    QRect paintTokens(QPainter *painter, const QRect &rect, const ChatMessage &msg, const QModelIndex &index) const;

    void paintChip(QPainter *painter,
                   QRect &cursorRect,
                   const QString &text,
                   const QColor &background,
                   const QColor &foreground,
                   ChatHitRegion::Type type,
                   const QString &payload,
                   const QModelIndex &index) const;

private:
    UserAvatarProvider *avatarProvider;

    mutable QHash<QPersistentModelIndex, QVector<ChatHitRegion>> hitRegions;
};

#endif // COCKATRICE_CHAT_MESSAGE_DELEGATE_H
