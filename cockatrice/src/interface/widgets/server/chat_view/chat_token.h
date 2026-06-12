#ifndef COCKATRICE_CHAT_TOKEN_H
#define COCKATRICE_CHAT_TOKEN_H

#include <QString>

struct ChatToken
{
    enum Type
    {
        Text,
        Mention,
        Url,
        Card,
        Highlight
    };

    Type type = Text;

    QString text;
    QString payload;

    ChatToken() = default;

    ChatToken(Type type, const QString &text, const QString &payload = {}) : type(type), text(text), payload(payload)
    {
    }
};

#endif // COCKATRICE_CHAT_TOKEN_H
