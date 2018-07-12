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


struct Chat;

class ChatsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ChatFields {
        Id,
        Type,
        Name,
        IsOpen,
        LastRead,
        UnreadCount,
        UnreadCountDisplay,
        MembersModel,
        MessagesModel,
        UserObject,
        Presence,
        Section
    };

    enum ChatType {
        Channel,
        Group,
        Conversation,
        MultiUserConversation
    };
    Q_ENUM(ChatType)


    ChatsModel(const QString& selfId, QObject *parent = nullptr, UsersModel *networkUsers = nullptr);

    int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const override { return m_chats.count(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addChat(const QJsonObject &data, const ChatsModel::ChatType type);
    void addChats(const QJsonArray &chats, const ChatType type);

    bool hasChannel(const QString &id);

    Q_INVOKABLE MessageListModel *messages(const QString &id);
    UsersModel *members(const QString &id);
    Chat& chat(const QString &id);
    void chatChanged(const Chat& chat);

private:
    QString getSectionName(const Chat& chat) const;

private:
    QMap<QString, Chat> m_chats;
    QStringList m_chatIds;
    UsersModel *m_networkUsers;
    QString m_selfId; // its you

};

struct Chat
{
    Chat(const QJsonObject &data, const ChatsModel::ChatType type);
    Chat() = default;

    QString id;
    QString presence;
    ChatsModel::ChatType type;
    QString name;
    QString readableName;
    bool isOpen = false;
    bool isPrivate = false;
    bool isMpim = false;

    QDateTime lastRead;
    int unreadCount = 0;
    int unreadCountDisplay = 0;
    QPointer<UsersModel> membersModel;
    QPointer<MessageListModel> messagesModel;
    QPointer<User> user;

    void setReadableName(const QString &selfId);
};
