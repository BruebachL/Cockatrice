#ifndef COCKATRICE_CHAT_MESSAGE_H
#define COCKATRICE_CHAT_MESSAGE_H

#include "chat_token.h"

#include <QDateTime>
#include <QList>
#include <libcockatrice/network/server/remote/room_message_type.h>
#include <libcockatrice/protocol/pb/serverinfo_user.pb.h>

struct ChatMessage
{
    QString sender;

    ServerInfo_User userInfo;

    QList<ChatToken> tokens;

    RoomMessageTypeFlags messageType;

    QDateTime timestamp;

    bool ownMessage = false;
    bool highlighted = false;

    bool sameSenderAsPrevious = false;

    bool playerBold = false;
};

Q_DECLARE_METATYPE(const ChatMessage *)

#endif // COCKATRICE_CHAT_MESSAGE_H
