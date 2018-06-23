#ifndef STORAGE_H
#define STORAGE_H

//#include <QObject>
#include <QVariantMap>

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QPointer>
#include <QAbstractListModel>
#include <QSize>
#include <QJsonArray>
#include <QDateTime>


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

    int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const override { return m_users.count(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addUser(User *user);
    void addUsers(const QJsonArray &usersData);
    QPointer<User> user(const QString &id);
    Q_INVOKABLE int fooCount() { return m_users.count(); }

private:
    QMap<QString, QPointer<User>> m_users;
    QStringList m_userIds;
};

class Reaction : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList userIds MEMBER userIds NOTIFY usersChanged)
    Q_PROPERTY(QString emoji MEMBER emoji CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(int usersCount READ usersCount NOTIFY usersChanged)

public:
    Reaction(const QJsonObject &data, QObject *parent);
    int usersCount() { return userIds.count(); }

signals:
    void usersChanged();

public:
    QStringList userIds;
    QString name;
    QString emoji;
};

class Attachment {
    Q_GADGET
    Q_PROPERTY(QString titleLink MEMBER titleLink CONSTANT)
    Q_PROPERTY(QString title MEMBER title CONSTANT)
    Q_PROPERTY(QString pretext MEMBER pretext CONSTANT)
    Q_PROPERTY(QString text MEMBER text CONSTANT)
    Q_PROPERTY(QString fallback MEMBER fallback CONSTANT)
    Q_PROPERTY(QString color MEMBER color CONSTANT)

public:
    Attachment(const QJsonObject &data);

    QString titleLink;
    QString title;
    QString pretext;
    QString text;
    QString fallback;
    QString color;

    QUrl imageUrl;
    QSize imageSize;
};

struct Message {
    Message(const QJsonObject &data);
    ~Message();

    QString text;
    QDateTime time;
    QString type;

    QPointer<User> user;
    QList<Reaction*> reactions;


    QList<Attachment*> attachments;
};

class ChatsModel;

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum MessageFields {
        Text,
        User,
        Time,
        Attachments,
        Reactions,

        MessageFieldCount
    };

    MessageListModel(QObject *parent, UsersModel *usersModel);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addMessage(const Message &message);
    void addMessages(const QJsonArray &messages);

private:
    QList<Message> m_messages;
    QMap<QString, Reaction*> m_reactions;
    UsersModel *m_usersModel;
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
        UserObject
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

    ChatsModel(QObject *parent, UsersModel *networkUsers);

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
        Network() = default;
        Network(const QJsonObject &data);

        QString id;
        QString name;
        QUrl icon;
        ChatsModel *chats;
        UsersModel *users;

        bool isValid();
    };

    int rowCount(const QModelIndex &/*parent*/) const override { return m_networks.count(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString addNetwork(const QJsonObject &networkData);
    ChatsModel *chatsModel(const QString &id);
    UsersModel *usersModel(const QString &id);

private:
    QStringList m_networkIds;
    QMap<QString, Network> m_networks;
};


class Storage : public QObject
{
    Q_OBJECT

public:
    Storage();

    QVariantMap user(const QVariant& id);
    const QVariantList &users();
    void saveUser(const QVariantMap& user);
    void updateUsersList();

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
};

#endif // STORAGE_H
