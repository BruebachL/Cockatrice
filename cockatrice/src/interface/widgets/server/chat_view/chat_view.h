#ifndef CHATVIEW_H
#define CHATVIEW_H

#include "../../interface/widgets/tabs/tab_supervisor.h"
#include "../user/user_list_widget.h"
#include "chat_message.h"

#include <QAbstractScrollArea>
#include <QColor>
#include <QTextLayout>
#include <QVector>
#include <libcockatrice/network/server/remote/room_message_type.h>
#include <libcockatrice/network/server/remote/user_level.h>

class AbstractGame;
class UserContextMenu;
class UserListProxy;
class QKeyEvent;
class QMouseEvent;

// ── Per-message precomputed link area ────────────────────────────────────────

/**
 * @brief A clickable/hoverable region within a message's text area.
 *
 * rect is local to the message's text origin (textLeft, textTop within the
 * item).  It must be translated by the message's screen-space Y position
 * and textLeft before use in viewport hit-testing.
 */
struct LinkHitArea
{
    QRectF rect;
    QString href;
};

// ── Per-message layout cache ─────────────────────────────────────────────────

/**
 * @brief Cached layout state for one ChatMessage.
 *
 * Rebuilt whenever the viewport width changes or the message content changes
 * (e.g. after redaction).  The QTextLayout holds the fully laid-out line
 * structure; linkAreas holds precomputed hit rects for interactive spans.
 */
struct MessageLayout
{
    QTextLayout *textLayout = nullptr;
    QVector<LinkHitArea> linkAreas;
    int totalHeight = 0;
    int textTop = 0;
    int textLeft = 0;
    bool valid = false;

    MessageLayout() : textLayout(new QTextLayout())
    {
    }
};

Q_DECLARE_TYPEINFO(MessageLayout, Q_MOVABLE_TYPE);

// ── ChatView ─────────────────────────────────────────────────────────────────

/**
 * @class ChatView
 * @brief Fully custom-painted chat widget replacing the QTextBrowser implementation.
 *
 * Each message is rendered as a dark card matching the game-list visual style.
 * Three row kinds are supported:
 *
 *   Full       — first message from a sender
 *                [accent bar][avatar][username · · · · · ][timestamp]
 *                [           ][message text, word-wrapped           ]
 *
 *   Compressed — follow-up from the same sender (no card, just indented text)
 *                [           ][message text, word-wrapped           ]
 *
 *   Server     — system / server message
 *                [tinted card][message text][timestamp right-anchored]
 *
 * Text selection is implemented via QTextLayout selection ranges tracked as
 * character positions across messages.  Interactive spans (card names, URLs,
 * @mentions) are hit-tested against precomputed LinkHitArea rects.
 *
 * Public API is source-compatible with the original QTextBrowser-based
 * ChatView; MessageLogWidget's appendHtmlServerMessage override compiles
 * unchanged.  HTML passed to appendHtml / appendHtmlServerMessage is stripped
 * to plain text.
 */
class ChatView : public QAbstractScrollArea
{
    Q_OBJECT

    // ── Layout constants ──────────────────────────────────────────────────────
    static constexpr int cardMarginH = 4;
    static constexpr int cardMarginV = 2;
    static constexpr int cardSpacingV = 4; ///< Vertical gap between cards
    static constexpr int cardRadius = 6;
    static constexpr int accentBarWidth = 4;
    static constexpr int accentBarRadius = 2;
    static constexpr int leftPad = 12; ///< rect.left() → avatar left
    static constexpr int avatarSize = 32;
    static constexpr int avatarFallbackPawn = 22;
    static constexpr int avatarGap = 8; ///< Avatar right → text left
    static constexpr int rightPad = 12;
    static constexpr int headerH = 48;   ///< Header row height (Full kind)
    static constexpr int textTopPad = 4; ///< Gap below header before text
    static constexpr int textBottomPad = 8;
    static constexpr int compressedTopPad = 2;
    static constexpr int compressedBottomPad = 4;
    static constexpr int serverHeaderH = 22; ///< Mini-header band: timestamp only, no avatar
    static constexpr int serverTopPad = 4;   ///< Gap between header band and text (was 6)
    static constexpr int serverBottomPad = 6;

public:
    explicit ChatView(TabSupervisor *tabSupervisor, AbstractGame *game, bool showTimestamps, QWidget *parent = nullptr);

    void retranslateUi();

    void appendHtml(const QString &html);
    virtual void
    appendHtmlServerMessage(const QString &html, bool optionalIsBold = false, QString optionalFontColor = {});
    void appendMessage(QString message,
                       RoomMessageTypeFlags messageType = {},
                       const ServerInfo_User &userInfo = {},
                       bool playerBold = false);

    void clearChat();
    void redactMessages(const QString &userName, int amount);

    AbstractGame *getGame()
    {
        return game;
    }

signals:
    void openMessageDialog(const QString &userName, bool focus);
    void cardNameHovered(QString cardName);
    void showCardInfoPopup(const QPoint &pos, const CardRef &cardRef);
    void deleteCardInfoPopup(QString cardName);
    void addMentionTag(QString mentionTag);
    void messageClickedSignal();
    void showMentionPopup(const QString &userName);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    // void scrollContentsBy(int dx, int dy) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    // ── Core references ───────────────────────────────────────────────────────
    TabSupervisor *const tabSupervisor;
    AbstractGame *const game;
    const UserListProxy *const userListProxy;
    UserContextMenu *userContextMenu;
    bool showTimestamps;

    // ── Message store ─────────────────────────────────────────────────────────
    QVector<ChatMessage> m_messages;
    QList<MessageLayout> m_layouts;
    int m_totalH = 0;
    int m_lastWidth = -1;
    QString m_lastSender;
    QString m_lastHistorySender;

    // ── Text selection ────────────────────────────────────────────────────────
    struct SelPoint
    {
        int msg = -1;
        int pos = 0;
    };
    SelPoint m_selAnchor;
    SelPoint m_selFocus;
    bool m_selecting = false;

    // ── Hover state ───────────────────────────────────────────────────────────
    enum HoveredType
    {
        HoveredNothing,
        HoveredCard,
        HoveredUser,
        HoveredUrl
    };
    HoveredType m_hoveredType = HoveredNothing;
    QString m_hoveredHref;

    // ── Parsing state ─────────────────────────────────────────────────────────
    QString m_ownUserName;
    QString m_mention; ///< "@" + m_ownUserName
    QColor m_linkColor{71, 158, 252};
    QStringList m_highlightedWords;

    // ── Geometry ──────────────────────────────────────────────────────────────
    [[nodiscard]] int textLeft() const;
    [[nodiscard]] int availableTextWidth(int viewportWidth) const;
    [[nodiscard]] int messageTop(int idx) const; ///< Scroll-space Y of message idx

    // ── Layout ────────────────────────────────────────────────────────────────
    void addMessage(ChatMessage &&msg);
    void layoutMessage(int idx, int availableWidth);
    void relayoutAll(int availableWidth);
    void updateScrollRange();
    void scrollToBottom();

    // ── Hit-testing ───────────────────────────────────────────────────────────
    /// Returns {messageIndex, charPos} for a viewport point, or {-1, 0}.
    [[nodiscard]] QPair<int, int> hitTest(const QPoint &viewportPos) const;
    [[nodiscard]] QString hrefAt(const QPoint &viewportPos) const;
    void updateHover(const QPoint &viewportPos);

    // ── Selection helpers ─────────────────────────────────────────────────────
    void normalisedSelection(SelPoint &lo, SelPoint &hi) const;
    [[nodiscard]] QString selectedText() const;

    // ── Painting ──────────────────────────────────────────────────────────────
    void paintMessage(QPainter &p, int idx, int screenY) const;
    void paintCardBackground(QPainter &p, const QRectF &card, const QColor &accent) const;
    void paintAvatar(QPainter &p, const QRect &avatarRect, const ChatMessage &msg, const QColor &accent) const;
    void paintHeader(QPainter &p, const QRect &headerRect, const ChatMessage &msg, const QColor &accent) const;
    void paintText(QPainter &p, int idx, int textScreenY) const;

    // ── Span parsing ──────────────────────────────────────────────────────────
    void parseMessageText(QString raw, ChatMessage &msg, bool mentionEnabled);
    void parseTag(QString &raw, ChatMessage &msg);
    void parseMention(QString &raw, ChatMessage &msg);
    void parseWord(QString &raw, ChatMessage &msg);
    void appendSpan(ChatMessage &msg, ChatSpan::Type type, const QString &text, const QString &href = {});

    // ── Colour helpers ────────────────────────────────────────────────────────
    [[nodiscard]] static QColor accentForUser(const ServerInfo_User &user);
    [[nodiscard]] QTextCharFormat formatForSpan(const ChatSpan &span) const;

    // ── Misc ──────────────────────────────────────────────────────────────────
    [[nodiscard]] static QString stripHtml(const QString &html);
    void showSystemPopup(const QString &userName);
    static bool isModeratorSendingGlobal(QFlags<ServerInfo_User::UserLevelFlag> level, const QString &word);
};

#endif // CHATVIEW_H