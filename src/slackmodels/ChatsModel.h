#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QPointer>
#include <QAbstractListModel>
#include <QSize>
#include <QJsonArray>
#include <QDateTime>

#include "UsersModel.h"
#include "MessagesModel.h"

class ChatsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ChatFields {
        Id,
        Type,
        Name,
        IsOpen,
        LastReadId,
        UnreadCount,
        MembersModel,
        MessagesModel,
        UserObject,
        Presence
    };

    enum ChatType {
        Channel,
        Group,
        Conversation,
        MultiUserConversation
    };
    Q_ENUM(ChatType)

    struct Chat
    {
        Chat(const QJsonObject &data, const ChatType type);
        Chat() = default;

        QString id;
        QString presence;
        ChatType type;
        QString name;
        bool isOpen = false;
        QString lastReadId;
        int unreadCount = 0;
        QPointer<UsersModel> membersModel;
        QPointer<MessageListModel> messagesModel;
        QPointer<User> user;
    };

    ChatsModel(QObject *parent = nullptr, UsersModel *networkUsers = nullptr);

    int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const override { return m_chats.count(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addChat(const QJsonObject &data, const ChatsModel::ChatType type);
    void addChats(const QJsonArray &chats, const ChatType type);

    bool hasChannel(const QString &id);

    Q_INVOKABLE MessageListModel *messages(const QString &id);
    UsersModel *members(const QString &id);

private:
    QMap<QString, Chat> m_chats;
    QStringList m_chatIds;
    UsersModel *m_networkUsers;
};

