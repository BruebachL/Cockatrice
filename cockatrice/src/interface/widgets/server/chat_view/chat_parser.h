#ifndef COCKATRICE_CHAT_PARSER_H
#define COCKATRICE_CHAT_PARSER_H

#include "chat_message.h"
#include "user_level.h"

class UserListProxy;

class ChatParser
{
public:
    struct ParseResult
    {
        QList<ChatToken> tokens;

        bool containsMention = false;
        bool containsHighlight = false;
        bool containsGlobalMention = false;
    };

    static ParseResult parse(const QString &message,
                             const QString &ownUserName,
                             const QStringList &highlightWords,
                             const UserListProxy *userListProxy,
                             const ServerInfo_User &senderInfo);

private:
    static void parseTag(QString &message, QList<ChatToken> &tokens);

    static void parseMention(QString &message,
                             QList<ChatToken> &tokens,
                             const QString &ownUserName,
                             const UserListProxy *userListProxy,
                             const ServerInfo_User &senderInfo,
                             bool &containsMention,
                             bool &containsGlobalMention);

    static void
    parseWord(QString &message, QList<ChatToken> &tokens, const QStringList &highlightWords, bool &containsHighlight);

    static QString extractNextWord(QString &message, QString &rest);

    static bool isModeratorGlobalMention(UserLevelFlags level, const QString &mention);
};

#endif // COCKATRICE_CHAT_PARSER_H
