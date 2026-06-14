#ifndef COCKATRICE_CHAT_MESSAGE_H
#define COCKATRICE_CHAT_MESSAGE_H

#include <QColor>
#include <QDateTime>
#include <QString>
#include <QVector>
#include <libcockatrice/protocol/pb/serverinfo_user.pb.h>

/**
 * @brief A single formatted span within a ChatMessage's text.
 *
 * Spans are non-overlapping and cover contiguous character ranges within
 * ChatMessage::text.  They drive both QTextCharFormat ranges during layout
 * and LinkHitArea computation for interactive elements.
 */
struct ChatSpan
{
    enum Type
    {
        Plain,        ///< Ordinary text — uses painter base colour
        SelfMention,  ///< @ownUsername — bold, highlighted background
        OtherMention, ///< @otherUser  — bold, link colour, clickable
        CardLink,     ///< [card]Name[/card] — italic, link colour, hoverable
        UrlLink,      ///< http(s):// / www. — underline, link colour, clickable
        Highlight,    ///< Custom highlighted word — bold, highlighted background
    };

    Type type = Plain;
    int start = 0; ///< First character index in ChatMessage::text
    int length = 0;
    QString href; ///< OtherMention: "user://level_name"
                  ///< CardLink:     "card://Name"
                  ///< UrlLink:      full URL
};

/**
 * @brief One rendered message row.
 *
 * Full       — first message from a sender: accent card, avatar, username, timestamp, text.
 * Compressed — follow-up from the same sender: text only, indented to align with Full.
 * Server     — system / server message: tinted text card, no avatar.
 */
struct ChatMessage
{
    enum Kind
    {
        Full,
        Compressed,
        Server
    };

    Kind kind = Full;
    QString text; ///< Plain-text content (all span indices reference this)
    QVector<ChatSpan> spans;
    ServerInfo_User userInfo; ///< Valid for Full and Compressed kinds
    QDateTime timestamp;
    QColor accentColor;  ///< Precomputed accent; valid for Full and Compressed
    QString serverColor; ///< Explicit hex colour for Server kind (empty = default)
    bool serverBold = false;
    bool redacted = false; ///< Set by redactMessages(); text replaced with placeholder
};

#endif // COCKATRICE_CHAT_MESSAGE_H