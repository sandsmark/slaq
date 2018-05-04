#ifndef STORAGE_H
#define STORAGE_H

//#include <QObject>
#include <QVariantMap>

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QPointer>
#include <QAbstractListModel>

class User : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString m_userId MEMBER m_userId CONSTANT)
    Q_PROPERTY(QString m_username MEMBER m_username CONSTANT)
    Q_PROPERTY(QString m_fullName MEMBER m_fullName CONSTANT)
    Q_PROPERTY(QUrl m_avatarUrl MEMBER m_avatarUrl CONSTANT)
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

class Chat : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString channelId MEMBER m_channelId CONSTANT)
    Q_PROPERTY(Type type MEMBER m_type CONSTANT)
    Q_PROPERTY(QList<User*> members READ members NOTIFY membersChanged)

public:
    enum Type {
        Channel,
        Group,
        Conversation,
    };
    Q_ENUM(Type)

    Chat(const QJsonObject &data, QObject *parent);
    void setMembers(const QList<QPointer<User>> &members);
    void addMember(QPointer<User> user);
    void removeMember(const QString &id);

    QList<User*> members(); // for QML

    MessageListModel *messageModel() { return m_messages; }

signals:
    void isOpenChanged();
    void lastReadIdChanged();
    void unreadCountChanged();
    void membersChanged();

private:
    QString m_channelId;
    QString m_presence;
    Type m_type;
    QString m_name;
    bool m_isOpen = false;
    QString m_lastReadId;
    int m_unreadCount = 0;
    QHash<QString, QPointer<User>> m_members;
    MessageListModel *m_messages;
};

class Storage
{
public:
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
    QHash<QString, User> m_users;
};

#endif // STORAGE_H
