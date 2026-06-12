#include "chat_model.h"

ChatModel::ChatModel(QObject *parent) : QAbstractListModel(parent)
{
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return messages.size();
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.row() < 0 || index.row() >= messages.size()) {
        return {};
    }

    if (role == MessageRole) {
        return QVariant::fromValue(static_cast<const ChatMessage *>(&messages[index.row()]));
    }

    return {};
}

void ChatModel::addMessage(const ChatMessage &message)
{
    beginInsertRows(QModelIndex(), messages.size(), messages.size());

    messages.push_back(message);

    endInsertRows();
}

void ChatModel::clear()
{
    beginResetModel();
    messages.clear();
    endResetModel();
}

const ChatMessage *ChatModel::messageAt(int row) const
{
    if (row < 0 || row >= messages.size()) {
        return nullptr;
    }

    return &messages[row];
}

void ChatModel::redactMessages(const QString &userName, int amount)
{
    for (int row = messages.size() - 1; row >= 0 && amount > 0; --row) {
        if (messages[row].sender != userName) {
            continue;
        }

        messages[row].tokens.clear();

        ChatToken token;
        token.type = ChatToken::Text;
        token.text = "[message removed]";

        messages[row].tokens.append(token);

        emit dataChanged(index(row), index(row));

        --amount;
    }
}