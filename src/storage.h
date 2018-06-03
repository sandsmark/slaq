#ifndef STORAGE_H
#define STORAGE_H

//#include <QObject>
#include <QVariantMap>

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QPointer>
#include <QAbstractListModel>
#include <QJsonArray>


class User : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString userId MEMBER m_userId CONSTANT)
    Q_PROPERTY(QString username MEMBER m_username CONSTANT)
    Q_PROPERTY(QString fullName MEMBER m_fullName CONSTANT)
    Q_PROPERTY(QUrl avatarUrl MEMBER m_avatarUrl CONSTANT)
    Q_PROPERTY(bool isBot MEMBER m_isBot CONSTANT)
    Q_PROPERTY(Presence presence READ presence NOTIFY presenceChanged)

public:
    enum Presence {
        Unknown,
        Active,
        Away
    };
    Q_ENUM(Presence)

    User(const QJsonObject &data, QObject *parent);

    void setPresence(const Presence presence);
    Presence presence();

    const QString &userId() { return m_userId; }

signals:
    void presenceChanged();

private:
    QString m_userId;
    QString m_username;
    QString m_fullName;
    QUrl m_avatarUrl;
    bool m_isBot = false;
    Presence m_presence = Unknown;
};

class UsersModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Fields {
        UserObject,

        FieldsCount
    };

    enum Presence {
        Unknown,
        Active,
        Away
    };
    Q_ENUM(Presence)

    UsersModel(QObject *parent);

    int rowCount(const QModelIndex &/*parent*/) const override { return m_users.count(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addUser(User *user);
    void addUsers(const QJsonArray &usersData);

private:
    QList<QPointer<User>> m_users;
};

struct Message {
    QString text;
    QPointer<User> user;
    QString time;
    QStringList attachments;
    QString type;
};

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum MessageFields {
        Text,
        User,
        Time,
        Attachments,

        MessageFieldCount
    };

    MessageListModel(QObject *parent);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addMessage(const Message &message);

private:
    QList<Message> m_messages;
};

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
    };

    enum ChatType {
        Channel,
        Group,
        Conversation,
    };
    Q_ENUM(ChatType)

    struct Chat
    {
        Chat(const QJsonObject &data, const ChatType type);

        QString id;
        QString presence;
        ChatType type;
        QString name;
        bool isOpen = false;
        QString lastReadId;
        int unreadCount = 0;
        QHash<QString, QPointer<User>> membersModel;
        MessageListModel *messagesModel;
    };

    ChatsModel(QObject *parent);

    int rowCount(const QModelIndex &/*parent*/) const override { return m_chats.count(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addChat(const Chat &chat);
    void addChats(const QJsonArray &chats, const ChatType type);

private:
    QList<Chat> m_chats;
};

class NetworksModel : public QAbstractListModel
{
    Q_OBJECT

public:
    NetworksModel(QObject *parent);

    enum NetworkFields {
        Id,
        Name,
        Chats,
        Users
    };

    struct Network {
        QString id;
        QString name;
        ChatsModel *chats;
        UsersModel *users;
    };

    int rowCount(const QModelIndex &/*parent*/) const override { return m_networks.count(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addNetwork(const QJsonObject &networkData);

private:
    QList<Network> m_networks;
};


class Storage : public QObject
{
    Q_OBJECT

public:
    Storage();

    QVariantMap user(const QVariant& id);
    const QVariantList &users();
    void saveUser(const QVariantMap& user);

    QVariantMap channel(const QVariant& id);
    QVariantList channels();
    void saveChannel(const QVariantMap& channel);

    QVariantList channelMessages(const QVariant& channelId);
    bool channelMessagesExist(const QVariant& channelId);
    void setChannelMessages(const QVariant& channelId, const QVariantList& messages);
    void appendChannelMessage(const QVariant& channelId, const QVariantMap& message);
    void clearChannelMessages();
    void clearChannels();

signals:

public slots:

private:
    QVariantMap userMap;
    QVariantMap channelMap;
    QVariantMap channelMessageMap;
    QVariantList userList;
    NetworksModel *m_networksModel;
};

#endif // STORAGE_H
