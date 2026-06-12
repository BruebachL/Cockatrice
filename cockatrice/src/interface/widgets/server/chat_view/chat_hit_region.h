#ifndef COCKATRICE_CHAT_HIT_REGION_H
#define COCKATRICE_CHAT_HIT_REGION_H

#include <QRect>
#include <QString>

struct ChatHitRegion
{
    enum Type
    {
        None,
        User,
        Mention,
        Card,
        Url
    };

    QRect rect;

    Type type = None;

    QString payload;
};

#endif // COCKATRICE_CHAT_HIT_REGION_H
