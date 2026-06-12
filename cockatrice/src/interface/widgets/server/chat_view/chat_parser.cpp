#include "chat_parser.h"

#include "../user/user_list_proxy.h"

#include <QRegularExpression>
#include <QUrl>

ChatParser::ParseResult ChatParser::parse(const QString &inputMessage,
                                          const QString &ownUserName,
                                          const QStringList &highlightWords,
                                          const UserListProxy *userListProxy,
                                          const ServerInfo_User &senderInfo)
{
    ParseResult result;

    QString message = inputMessage;

    while (!message.isEmpty()) {
        const QChar c = message.at(0);

        switch (c.toLatin1()) {
            case '[':
                parseTag(message, result.tokens);
                break;

            case '@':
                parseMention(message, result.tokens, ownUserName, userListProxy, senderInfo, result.containsMention,
                             result.containsGlobalMention);
                break;

            default:
                parseWord(message, result.tokens, highlightWords, result.containsHighlight);
                break;
        }
    }

    return result;
}

void ChatParser::parseTag(QString &message, QList<ChatToken> &tokens)
{
    if (message.startsWith("[card]")) {
        message.remove(0, 6);

        const int closePos = message.indexOf("[/card]");

        if (closePos < 0) {
            return;
        }

        const QString cardName = message.left(closePos);

        tokens.append(ChatToken(ChatToken::Card, cardName, cardName));

        message.remove(0, closePos + 7);
        return;
    }

    if (message.startsWith("[[")) {
        message.remove(0, 2);

        const int closePos = message.indexOf("]]");

        if (closePos < 0) {
            return;
        }

        const QString cardName = message.left(closePos);

        tokens.append(ChatToken(ChatToken::Card, cardName, cardName));

        message.remove(0, closePos + 2);
        return;
    }

    if (message.startsWith("[url]")) {
        message.remove(0, 5);

        const int closePos = message.indexOf("[/url]");

        if (closePos < 0) {
            return;
        }

        const QString url = message.left(closePos);

        tokens.append(ChatToken(ChatToken::Url, url, url));

        message.remove(0, closePos + 6);
        return;
    }

    tokens.append(ChatToken(ChatToken::Text, "["));
    message.remove(0, 1);
}

void ChatParser::parseMention(QString &message,
                              QList<ChatToken> &tokens,
                              const QString &ownUserName,
                              const UserListProxy *userListProxy,
                              const ServerInfo_User &senderInfo,
                              bool &containsMention,
                              bool &containsGlobalMention)
{
    static const QRegularExpression notAlphaNum("[^a-zA-Z0-9]");

    const int firstSpace = message.indexOf(' ');

    QString mention = (firstSpace < 0) ? message.mid(1) : message.mid(1, firstSpace - 1);

    const QString originalMention = mention;

    while (!mention.isEmpty()) {

        if (const ServerInfo_User *onlineUser = userListProxy->getOnlineUser(mention)) {

            const QString userName = QString::fromStdString(onlineUser->name());

            tokens.append(ChatToken(ChatToken::Mention, "@" + userName, userName));

            if (userName.compare(ownUserName, Qt::CaseInsensitive) == 0) {
                containsMention = true;
            }

            message.remove(0, userName.length() + 1);
            return;
        }

        if (isModeratorGlobalMention(UserLevelFlags(senderInfo.user_level()), mention)) {

            tokens.append(ChatToken(ChatToken::Mention, "@" + mention, mention));

            containsGlobalMention = true;

            message.remove(0, mention.length() + 1);
            return;
        }

        if (mention.size() < 2 || mention.right(1).indexOf(notAlphaNum) == -1) {

            tokens.append(ChatToken(ChatToken::Text, "@" + originalMention));

            message.remove(0, originalMention.length() + 1);
            return;
        }

        mention.chop(1);
    }

    tokens.append(ChatToken(ChatToken::Text, "@"));
    message.remove(0, 1);
}

void ChatParser::parseWord(QString &message,
                           QList<ChatToken> &tokens,
                           const QStringList &highlightWords,
                           bool &containsHighlight)
{
    QString rest;

    const QString word = extractNextWord(message, rest);

    if (word.isEmpty()) {
        if (!rest.isEmpty()) {
            tokens.append(ChatToken(ChatToken::Text, rest));
        }
        return;
    }

    if (word.startsWith("http://", Qt::CaseInsensitive) || word.startsWith("https://", Qt::CaseInsensitive) ||
        word.startsWith("www.", Qt::CaseInsensitive)) {

        QUrl url(word);

        if (url.isValid()) {
            tokens.append(ChatToken(ChatToken::Url, word, word));

            if (!rest.isEmpty()) {
                tokens.append(ChatToken(ChatToken::Text, rest));
            }

            return;
        }
    }

    for (const QString &highlight : highlightWords) {
        if (word.compare(highlight, Qt::CaseInsensitive) == 0) {

            containsHighlight = true;

            tokens.append(ChatToken(ChatToken::Highlight, word));

            if (!rest.isEmpty()) {
                tokens.append(ChatToken(ChatToken::Text, rest));
            }

            return;
        }
    }

    tokens.append(ChatToken(ChatToken::Text, word + rest));
}

QString ChatParser::extractNextWord(QString &message, QString &rest)
{
    QString word;

    const int firstSpace = message.indexOf(' ');

    if (firstSpace < 0) {
        word = message;
        message.clear();
    } else {
        word = message.left(firstSpace);
        message.remove(0, firstSpace);
    }

    for (int i = word.size() - 1; i >= 0; --i) {
        if (word.at(i).isLetterOrNumber()) {
            rest = word.mid(i + 1);
            return word.left(i + 1);
        }
    }

    rest = word;
    return {};
}

bool ChatParser::isModeratorGlobalMention(UserLevelFlags level, const QString &mention)
{
    static const QStringList globals = {"/all"};

    return globals.contains(mention) &&
           (level.testFlag(ServerInfo_User::IsModerator) || level.testFlag(ServerInfo_User::IsAdmin));
}