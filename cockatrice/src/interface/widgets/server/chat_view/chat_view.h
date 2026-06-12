/**
 * @file chat_view.h
 * @ingroup NetworkingWidgets
 * @ingroup Lobby
 */
//! \todo Document this file.

#ifndef CHATVIEW_H
#define CHATVIEW_H

#include "libcockatrice/utility/card_ref.h"
#include "room_message_type.h"

#include <QListView>
#include <libcockatrice/protocol/pb/serverinfo_user.pb.h>

class UserListProxy;
class ChatModel;
class ChatMessageDelegate;
class UserAvatarProvider;
class UserContextMenu;
class TabSupervisor;
class AbstractGame;

class ChatView : public QListView
{
    Q_OBJECT

public:
    ChatView(TabSupervisor *tabSupervisor, AbstractGame *game, bool showTimestamps, QWidget *parent = nullptr);

    void appendMessage(QString message,
                       RoomMessageTypeFlags messageType = {},
                       const ServerInfo_User &userInfo = {},
                       bool playerBold = false);

    void retranslateUi();

    virtual void appendHtml(const QString &html);

    virtual void
    appendHtmlServerMessage(const QString &html, bool optionalIsBold = false, QString optionalFontColor = QString());

    void clearChat();

    void redactMessages(const QString &userName, int amount);

    ChatModel *getModel() const;

    TabSupervisor *tabSupervisor;
    AbstractGame *game;

signals:
    void openMessageDialog(const QString &userName, bool focus);

    void cardNameHovered(QString cardName);

    void showCardInfoPopup(const QPoint &pos, const CardRef &cardRef);

    void deleteCardInfoPopup(QString cardName);

    void addMentionTag(QString mentionTag);

    void messageClickedSignal();

    void showMentionPopup(const QString &userName);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    const UserListProxy *userListProxy;

    ChatModel *chatModel;
    ChatMessageDelegate *chatDelegate;
    UserAvatarProvider *avatarProvider;
    UserContextMenu *userContextMenu;

    QString ownUserName;
};

#endif
