#include "storage.h"

#include <QDebug>
#include <QJsonObject>

void Storage::saveUser(const QVariantMap& user)
{
    userMap.insert(user.value("id").toString(), user);
    userList = userMap.values();
}

QVariantMap Storage::user(const QVariant& id)
{
    return userMap.value(id.toString()).toMap();
}

const QVariantList& Storage::users()
{
    return userList;
}

void Storage::saveChannel(const QVariantMap& channel)
{
    channelMap.insert(channel.value("id").toString(), channel);
}

QVariantMap Storage::channel(const QVariant& id)
{
    return channelMap.value(id.toString()).toMap();
}

QVariantList Storage::channels()
{
    return channelMap.values();
}

QVariantList Storage::channelMessages(const QVariant& channelId)
{
    return channelMessageMap.value(channelId.toString()).toList();
}

bool Storage::channelMessagesExist(const QVariant& channelId)
{
    return channelMessageMap.contains(channelId.toString());
}

void Storage::setChannelMessages(const QVariant& channelId, const QVariantList& messages)
{
    channelMessageMap.insert(channelId.toString(), messages);
}

void Storage::appendChannelMessage(const QVariant& channelId, const QVariantMap& message)
{
    QVariantList messages = channelMessages(channelId);
    messages.append(message);
    setChannelMessages(channelId, messages);
}

void Storage::clearChannelMessages()
{
    channelMessageMap.clear();
}

void Storage::clearChannels()
{
    channelMap.clear();
}

User::User(const QJsonObject &data, QObject *parent) : QObject(parent)
{
    m_userId = data.value(QStringLiteral("id")).toString();
    if (m_userId.isEmpty()) {
        qWarning() << "No user id";
        return;
    }

    m_fullName = data.value(QStringLiteral("name")).toString();
    if (m_fullName.isEmpty()) {
        qWarning() << "No full name set";
    }

    const QString presenceString = data.value(QStringLiteral("presence")).toString();
    if (presenceString == "active") {
        m_presence = Active;
    } else if (presenceString == "away") {
        m_presence = Away;
    } else {
        qWarning() << "Unknown presence type" << presenceString;
        m_presence = Unknown;
    }

    QJsonObject profile = data.value(QStringLiteral("profile")).toObject();
    m_avatarUrl = QUrl(profile[QStringLiteral("image_512")].toString());
    if (!m_avatarUrl.isValid()) {
        qWarning() << "No avatar URL";
    }
}

void User::setPresence(const User::Presence presence)
{
    if (presence == m_presence) {
        return;
    }

    m_presence = presence;
    emit presenceChanged();
}

User::Presence User::presence()
{
    return m_presence;
}


Chat::Chat(const QJsonObject &data, QObject *parent) : QObject(parent),
    m_messages(new MessageListModel(this))
{
    data.insert(QStringLiteral("type"), QVariant(QStringLiteral("im")));
    data.insert(QStringLiteral("category"), QVariant(QStringLiteral("chat")));
    m_channelId = data.value(QStringLiteral("id")).toVariant();
//    data.insert(QStringLiteral("userId"), userId);
    m_name = data.value(QStringLiteral("name"));
    m_presence = data.value(QStringLiteral("presence"));
    m_isOpen = data.value(QStringLiteral("is_open")).toBool();
    m_lastReadId = data.value(QStringLiteral("last_read")).toString();
    m_unreadCount = data.value(QStringLiteral("unread_count_display")).toInt();
}

void Chat::setMembers(const QList<QPointer<User> > &members)
{
    m_members.clear();
    for (QPointer<User> user : members) {
        if (!user) {
            qWarning() << "Handled a null user";
            continue;
        }
        const QString userId = user->userId();
        if (userId.isEmpty()) {
            qWarning() << "No user id, can't add user";
            continue;
        }
        m_members[userId] = user;
    }

    emit membersChanged();
}

void Chat::addMember(QPointer<User> user)
{
    if (!user) {
        qWarning() << "Can't add null user";
        return;
    }

    const QString userId = user->userId();
    if (userId.isEmpty()) {
        qWarning() << "No user id, can't add user";
        return;
    }

    if (m_members.contains(userId)) {
        qWarning() << "we already have user" << userId;
    }

    m_members[userId] = user;
    emit membersChanged();
}

void Chat::removeMember(const QString &id)
{
    if (!m_members.contains(id)) {
        qDebug() << "We don't have user" << id;
        return;
    }

    m_members.remove(id);
    emit membersChanged();
}

QList<User *> Chat::members()
{
    QList<User*> ret;

    const QStringList userIds = m_members.uniqueKeys();
    for (const QString &id : userIds) {

        if (!m_members[id]) {
            qDebug() << "User" << id << "has gone away";
            m_members.remove(id);
            continue;
        }
        ret.append(m_members[id]);
    }

    return ret;
}

MessageListModel::MessageListModel(QObject *parent) : QAbstractListModel(parent)
{
}

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_messages.count();
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row >= m_messages.count() || row < 0) {
        qWarning() << "Invalid row" << row;
        return QVariant();
    }
    switch(role) {
    case Text:
        return m_messages[row].text;
    case User:
        return QVariant::fromValue(m_messages[row].user.data());
    case Time:
        return m_messages[row].time;
    case Attachments:
        return m_messages[row].attachments;
    default:
        qWarning() << "Unhandled role" << role;
        return QVariant();
    }
}

void MessageListModel::addMessage(const Message &message)
{
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count() + 1);
    m_messages.append(message);
    endInsertRows();
}


QHash<int, QByteArray> MessageListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[Text] = "Text";
    names[User] = "User";
    names[Time] = "Time";
    names[Attachments] = "Attachments";
    return names;
}
