#ifndef COCKATRICE_CHAT_MODEL_H
#define COCKATRICE_CHAT_MODEL_H

#include "chat_message.h"

#include <QAbstractListModel>

class ChatModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        MessageRole = Qt::UserRole + 1
    };

    explicit ChatModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void clear();

    void addMessage(const ChatMessage &message);

    void redactMessages(const QString &userName, int amount);

    const ChatMessage *messageAt(int row) const;

private:
    QVector<ChatMessage> messages;
};

#endif // COCKATRICE_CHAT_MODEL_H
