#include "chat_view.h"

#include "../../../../client/settings/cache_settings.h"
#include "../../client/sound_engine.h"
#include "../../interface/pixel_map_generator.h"
#include "../../interface/widgets/tabs/tab_account.h"
#include "../../interface/widgets/tabs/tab_supervisor.h"
#include "../user/user_avatar_provider.h"
#include "../user/user_context_menu.h"
#include "../user/user_list_manager.h"
#include "../user/user_list_proxy.h"
#include "chat_hit_region.h"
#include "chat_message_delegate.h"
#include "chat_model.h"
#include "chat_parser.h"

#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QMouseEvent>
#include <QScrollBar>
#include <libcockatrice/network/server/remote/user_level.h>

const QColor DEFAULT_MENTION_COLOR = QColor(194, 31, 47);

ChatView::ChatView(TabSupervisor *_tabSupervisor, AbstractGame *_game, bool, QWidget *parent)
    : QListView(parent), tabSupervisor(_tabSupervisor), game(_game), userListProxy(_tabSupervisor->getUserListManager())
{
    ownUserName = userListProxy->getOwnUsername();
    setMouseTracking(true);

    chatModel = new ChatModel(this);

    avatarProvider = new UserAvatarProvider(tabSupervisor->getClient(), this);

    chatDelegate = new ChatMessageDelegate(avatarProvider, this);

    setModel(chatModel);
    setItemDelegate(chatDelegate);

    setSelectionMode(NoSelection);

    setVerticalScrollMode(ScrollPerPixel);

    setUniformItemSizes(false);
}

void ChatView::appendMessage(QString message,
                             RoomMessageTypeFlags messageType,
                             const ServerInfo_User &userInfo,
                             bool playerBold)
{
    ChatMessage msg;

    msg.sender = QString::fromStdString(userInfo.name());

    msg.userInfo = userInfo;

    msg.timestamp = QDateTime::currentDateTime();

    msg.messageType = messageType;

    msg.playerBold = playerBold;

    ChatParser::ParseResult result = ChatParser::parse(
        message, ownUserName, SettingsCache::instance().getHighlightWords().split(' ', Qt::SkipEmptyParts),
        userListProxy, userInfo);

    msg.tokens = result.tokens;

    if (result.containsMention || result.containsHighlight || result.containsGlobalMention) {

        QApplication::alert(this);

        if (result.containsMention || result.containsGlobalMention) {

            soundEngine->playSound("chat_mention");

            if (SettingsCache::instance().getShowMentionPopup()) {
                emit showMentionPopup(msg.sender);
            }
        }
    }

    chatModel->addMessage(msg);

    if (!msg.sender.isEmpty()) {
        avatarProvider->requestAvatar(msg.sender);
    }

    scrollToBottom();
}

void ChatView::appendHtml(const QString &html)
{
    ChatMessage msg;

    ChatToken token;
    token.type = ChatToken::Text;
    token.text = html;

    msg.tokens.append(token);

    chatModel->addMessage(msg);

    scrollToBottom();
}

void ChatView::appendHtmlServerMessage(const QString &html, bool optionalIsBold, QString optionalFontColor)
{
    Q_UNUSED(optionalIsBold)
    Q_UNUSED(optionalFontColor)

    ChatMessage msg;

    msg.sender = "Servatrice";

    ChatToken token;
    token.type = ChatToken::Text;
    token.text = html;

    msg.tokens.append(token);

    chatModel->addMessage(msg);

    scrollToBottom();
}

void ChatView::clearChat()
{
    chatModel->clear();
}

void ChatView::redactMessages(const QString &userName, int amount)
{
    chatModel->redactMessages(userName, amount);
}

ChatModel *ChatView::getModel() const
{
    return chatModel;
}

void ChatView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex idx = indexAt(event->pos());

    if (!idx.isValid()) {
        QListView::mousePressEvent(event);
        return;
    }

    ChatHitRegion region = chatDelegate->hitTest(idx, event->pos());

    switch (region.type) {

        case ChatHitRegion::User:
            emit addMentionTag("@" + region.payload);
            break;

        case ChatHitRegion::Card:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            emit showCardInfoPopup(event->globalPosition().toPoint(), CardRef(region.payload));
#else
            emit showCardInfoPopup(event->globalPos(), CardRef(region.payload));
#endif
            break;

        case ChatHitRegion::Url:
            QDesktopServices::openUrl(QUrl(region.payload));
            break;

        default:
            break;
    }

    QListView::mousePressEvent(event);
}