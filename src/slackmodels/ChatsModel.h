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


class Chat;

class ChatsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ChatFields {
        Id,
        Type,
        Name,
        IsOpen,
        IsGeneral,
        LastRead,
        LastReadTs,
        UnreadCount,
        UnreadCountDisplay,
        UnreadCountPersonal,
        MembersModel,
        MessagesModel,
        Presence,
        PresenceIcon,
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

    QString addChat(Chat *chat);
    void removeChat(const QString &channelId);
    QString doAddChat(Chat *chat);
    void addChats(const QList<Chat *> &chats, bool last);
    void addMembers(const QString &channelId, const QStringList &members);

    bool hasChannel(const QString &id);

    Q_INVOKABLE MessageListModel *messages(const QString &id);
    UsersModel *members(const QString &id);
    UsersModel *users() { return m_networkUsers; }
    Chat* chat(const QString &id);
    Chat* chat(int row);
    Chat* generalChat();
    void chatChanged(Chat* chat);
    //only for IM aka Conversations chats
    void setPresence(const QStringList &users, const QString& presence, const QDateTime& snoozeEnds = QDateTime(), bool force = false);
    void increaseUnreadsInNull(const QString &channelId, bool personal);
    int unreadsInNull(ChatsModel::ChatType type, bool personal);
    int unreadsInNullChannel(const QString &channelId, bool personal);

    QStringList getChatIds() const;

    void requestMissedMessages(const QString &lastChannel);
public slots:
    void onUsersDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
private:
    QString getSectionName(Chat *chat) const;

private:
    QMap<QString, Chat*> m_chats;
    QStringList m_chatIds;
    UsersModel *m_networkUsers;
    QString m_selfId; // its you
    QMap<QString, int> m_unreadNullChats; //keep unreads for still not created chats
    QMap<QString, int> m_unreadPersonalNullChats;
};

class Chat: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id MEMBER id CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(QString user MEMBER user CONSTANT)
    Q_PROPERTY(QString readableName MEMBER readableName CONSTANT)
    Q_PROPERTY(QString topic MEMBER topic)
    Q_PROPERTY(quint64 creationDate MEMBER creationDate CONSTANT)
    Q_PROPERTY(quint64 lastRead READ lastRead NOTIFY lastReadChanged)
    Q_PROPERTY(QString lastReadTs READ lastReadTs NOTIFY lastReadTsChanged)
    Q_PROPERTY(int unreadCount MEMBER unreadCount)
    Q_PROPERTY(int unreadCountDisplay MEMBER unreadCountDisplay)
    Q_PROPERTY(bool isOpen MEMBER isOpen)
    Q_PROPERTY(bool isPrivate MEMBER isPrivate CONSTANT)
    Q_PROPERTY(bool isGeneral MEMBER isGeneral CONSTANT)
    Q_PROPERTY(ChatsModel::ChatType type MEMBER type CONSTANT)

public:
    Chat(const QJsonObject &data, const ChatsModel::ChatType type = ChatsModel::Channel, QObject* parent = nullptr);
    Chat() = default;

    Q_INVOKABLE bool isChatEmpty() const;

    QString id;
    QString presence;
    ChatsModel::ChatType type;
    QString name;
    QString user;
    QString readableName;
    QString topic;
    QString purpose;
    quint64 creationDate;
    bool isOpen = false;
    bool isPrivate = false;
    bool isGeneral = false;

    int unreadCount = 0;
    int unreadCountDisplay = 0;
    int unreadCountPersonal = 0; //used for broadcast or personal messages on channel
    QPointer<UsersModel> membersModel;
    QPointer<MessageListModel> messagesModel;

    void setReadableName(const QString &selfId);
    void setData(const QJsonObject &data,  ChatsModel::ChatType type_ = ChatsModel::Channel);
    QString lastReadTs() const;
    quint64 lastRead() const;

private:
    void setLastReadTs(const QString &lastReadTs);
    void setLastRead(const quint64 &lastRead);

public slots:
    void setLastReadData(const QString &lastread);
signals:
    void lastReadTsChanged(QString lastReadTs);
    void lastReadChanged(quint64 lastRead);

private:
    QString m_lastReadTs;
    quint64 m_lastRead;
};
