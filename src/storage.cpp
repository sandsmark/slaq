#include "storage.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QDateTime>
#include <QJsonArray>

void Storage::saveUser(const QVariantMap& user)
{
    userMap.insert(user.value("id").toString(), user);
    userList = userMap.values();
}

Storage::Storage()
{
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

NetworksModel::NetworksModel(QObject *parent) : QAbstractListModel(parent)
{

}

QVariant NetworksModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row >= m_networks.count() || row < 0) {
        qWarning() << "invalid row" << row;
        return QVariant();
    }
    const Network &network = m_networks[m_networkIds[row]];
    switch (role) {
    case Id:
        return network.id;
    case Name:
        return network.name;
    case Chats:
        return QVariant::fromValue(network.chats);
    case Users:
        return QVariant::fromValue(network.users);
    default:
        qWarning() << "Invalid role" << role;
        return QVariant();
    }
}

QHash<int, QByteArray> NetworksModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[Id] = "Id";
    names[Name] = "Name";
    names[Chats] = "Chats";
    names[Users] = "Users";
    return names;

}

QString NetworksModel::addNetwork(const QJsonObject &networkData)
{
    Network network(networkData["team"].toObject());
    if (!network.isValid()) {
        qWarning() << "Invalid network";
        qDebug() << QJsonDocument(networkData).toJson();
        return QString();
    }

    network.users = new UsersModel(this);
    network.users->addUsers(networkData.value(QStringLiteral("users")).toArray());

    network.chats = new ChatsModel(this);
    network.chats->addChats(networkData.value(QStringLiteral("channels")).toArray(), ChatsModel::Channel);
    network.chats->addChats(networkData.value(QStringLiteral("groups")).toArray(), ChatsModel::Group);
    network.chats->addChats(networkData.value(QStringLiteral("chats")).toArray(), ChatsModel::Conversation);

    QQmlEngine::setObjectOwnership(network.users, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(network.chats, QQmlEngine::CppOwnership);

    m_networkIds.append(network.id);
    m_networks.insert(network.id, network);

    return network.id;
}

ChatsModel *NetworksModel::chatsModel(const QString &id)
{
    return m_networks[id].chats;
}

UsersModel *NetworksModel::usersModel(const QString &id)
{
    return m_networks[id].users;
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


//Chat::Chat(const QJsonObject &data, QObject *parent) : QObject(parent),
//    m_messages(new MessageListModel(this))
//{
//    data.insert(QStringLiteral("type"), QVariant(QStringLiteral("im")));
//    data.insert(QStringLiteral("category"), QVariant(QStringLiteral("chat")));
//    m_channelId = data.value(QStringLiteral("id")).toVariant();
////    data.insert(QStringLiteral("userId"), userId);
//    m_name = data.value(QStringLiteral("name"));
//    m_presence = data.value(QStringLiteral("presence"));
//    m_isOpen = data.value(QStringLiteral("is_open")).toBool();
//    m_lastReadId = data.value(QStringLiteral("last_read")).toString();
//    m_unreadCount = data.value(QStringLiteral("unread_count_display")).toInt();
//}

//void Chat::setMembers(const QList<QPointer<User> > &members)
//{
//    m_members.clear();
//    for (QPointer<User> user : members) {
//        if (!user) {
//            qWarning() << "Handled a null user";
//            continue;
//        }
//        const QString userId = user->userId();
//        if (userId.isEmpty()) {
//            qWarning() << "No user id, can't add user";
//            continue;
//        }
//        m_members[userId] = user;
//    }

//    emit membersChanged();
//}

//void Chat::addMember(QPointer<User> user)
//{
//    if (!user) {
//        qWarning() << "Can't add null user";
//        return;
//    }

//    const QString userId = user->userId();
//    if (userId.isEmpty()) {
//        qWarning() << "No user id, can't add user";
//        return;
//    }

//    if (m_members.contains(userId)) {
//        qWarning() << "we already have user" << userId;
//    }

//    m_members[userId] = user;
//    emit membersChanged();
//}

//void Chat::removeMember(const QString &id)
//{
//    if (!m_members.contains(id)) {
//        qDebug() << "We don't have user" << id;
//        return;
//    }

//    m_members.remove(id);
//    emit membersChanged();
//}

//QList<User *> Chat::membersModel()
//{
//    QList<User*> ret;

//    const QStringList userIds = m_members.uniqueKeys();
//    for (const QString &id : userIds) {

//        if (!m_members[id]) {
//            qDebug() << "User" << id << "has gone away";
//            m_members.remove(id);
//            continue;
//        }
//        ret.append(m_members[id]);
//    }

//    return ret;
//}

MessageListModel::MessageListModel(QObject *parent, UsersModel *usersModel) : QAbstractListModel(parent),
    m_usersModel(usersModel)
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
        return QVariant::fromValue(m_messages[row].attachments);
    case Reactions:
        return QVariant::fromValue(m_messages[row].reactions);
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

void MessageListModel::addMessages(const QJsonArray &messages)
{
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count() + messages.count());

    for (const QJsonValue &messageData : messages) {
        const QJsonObject messageObject = messageData.toObject();
        Message message(messageObject);
        message.user = m_usersModel->user(messageObject["user"].toString());

        for (const QJsonValue &reactionValue : messageObject["reactions"].toArray()) {
            const QJsonObject reactionObject = reactionValue.toObject();
            Reaction *reaction = new Reaction(reactionObject, this);
            m_reactions.insert(reactionObject["name"].toString(), reaction);
            message.reactions.append(reaction);
        }

        m_messages.append(std::move(message));
    }

    endInsertRows();
}


QHash<int, QByteArray> MessageListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[Text] = "Text";
    names[User] = "User";
    names[Time] = "Time";
    names[Attachments] = "Attachments";
    names[Reactions] = "Reactions";
    return names;
}

ChatsModel::ChatsModel(QObject *parent) : QAbstractListModel(parent)
{

}

QVariant ChatsModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row >= m_chatIds.count() || row < 0) {
        qWarning() << "invalid row" << row;
        return QVariant();
    }
    const Chat &chat = m_chats[m_chatIds[row]];
    switch (role) {
    case Id:
        return chat.id;
    case Type:
        return chat.type;
    case Name:
        return chat.name;
    case IsOpen:
        return chat.name;
    case LastReadId:
        return chat.lastReadId;
    case UnreadCount:
        return chat.unreadCount;
    case MembersModel:
        return QVariant::fromValue(chat.membersModel);
    case MessagesModel:
        return QVariant::fromValue(chat.messagesModel);
    default:
        qWarning() << "Invalid role" << role;
        return QVariant();
    }
}

QHash<int, QByteArray> ChatsModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[Id] = "Id";
    names[Type] = "Type";
    names[Name] = "Name";
    names[IsOpen] = "IsOpen";
    names[LastReadId] = "LastReadId";
    names[UnreadCount] = "UnreadCount";
    names[MembersModel] = "MembersModel";
    names[MessagesModel] = "MessagesModel";
    return names;

}

void ChatsModel::addChat(const Chat &chat)
{
    beginInsertRows(QModelIndex(), m_chats.count(), m_chats.count() + 1);
    m_chatIds.append(chat.id);
    m_chats.insert(chat.id, chat);
    endInsertRows();
}

void ChatsModel::addChats(const QJsonArray &chats, const ChatType type)
{
    beginInsertRows(QModelIndex(), m_chats.count(), m_chats.count() + chats.count());
    for (const QJsonValue &value : chats) {
        QJsonObject chatData = value.toObject();

        Chat chat(chatData, type);
        chat.membersModel = new UsersModel(this);
        chat.messagesModel = new MessageListModel(this, chat.membersModel);

        QQmlEngine::setObjectOwnership(chat.membersModel, QQmlEngine::CppOwnership);
        QQmlEngine::setObjectOwnership(chat.messagesModel, QQmlEngine::CppOwnership);

        m_chatIds.append(chat.id);
        m_chats.insert(chat.id, chat);
    }

    endInsertRows();
}

bool ChatsModel::hasChannel(const QString &id)
{
    return m_chatIds.contains(id);
}

MessageListModel *ChatsModel::messages(const QString &id)
{
    return m_chats[id].messagesModel;
}

UsersModel *ChatsModel::members(const QString &id)
{
    return m_chats[id].membersModel;
}

UsersModel::UsersModel(QObject *parent) : QAbstractListModel(parent)
{

}

QVariant UsersModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row >= m_users.count() || row < 0) {
        qWarning() << "invalid row" << row;
        return QVariant();
    }

    switch (role) {
    case UserObject:
        return QVariant::fromValue(m_users[m_userIds[row]]);
    default:
        qWarning() << "Invalid role" << role;
        return QVariant();
    }
}

QHash<int, QByteArray> UsersModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[UserObject] = "UserObject";
    return names;

}

void UsersModel::addUser(User *user)
{
    beginInsertRows(QModelIndex(), m_users.count(), m_users.count() + 1);
    m_userIds.append(user->userId());
    m_users.insert(user->userId(), user);
    endInsertRows();
}

void UsersModel::addUsers(const QJsonArray &usersData)
{
    beginInsertRows(QModelIndex(), m_users.count(), m_users.count() + usersData.count());
    for (const QJsonValue &value : usersData) {
        QJsonObject userData = value.toObject();
        User *user = new User(userData, this);

        if (m_users[user->userId()]) {
            m_users[user->userId()]->deleteLater();
            m_userIds.removeAll(user->userId());
        }

        m_userIds.append(user->userId());
        m_users[user->userId()] = user;
    }

    endInsertRows();
}

User *UsersModel::user(const QString &id)
{
    return m_users[id];
}

ChatsModel::Chat::Chat(const QJsonObject &data, const ChatType type_)
{
    id = data.value(QStringLiteral("id")).toString();
    type = type_;
    name = data.value(QStringLiteral("name")).toString();
    presence = QStringLiteral("none");
    isOpen = data.value(QStringLiteral("is_member")).toBool();
    lastReadId = data.value(QStringLiteral("last_read")).toString();
    unreadCount = data.value(QStringLiteral("unread_count_display")).toInt();
}

NetworksModel::Network::Network(const QJsonObject &data)
{
    id = data["id"].toString();
    name = data["name"].toString();

    QString iconString = data["icon"].toObject()["image_230"].toString();
    icon = QUrl(iconString);
    if (!icon.isValid()) {
        qWarning() << "Invalid icon" << iconString;
    }
}

bool NetworksModel::Network::isValid()
{
    return !id.isEmpty() && !icon.isEmpty();
}

Message::Message(const QJsonObject &data)
{
    type = data.value(QStringLiteral("type")).toString();
    time = QDateTime::fromSecsSinceEpoch(data.value(QStringLiteral("ts")).toDouble());
    text = data.value(QStringLiteral("text")).toString();
}

Message::~Message()
{
    qDeleteAll(attachments);
    attachments.clear();
}

Attachment::Attachment(const QJsonObject &data)
{
    titleLink = data.value(QStringLiteral("title_link")).toString();
    title = data.value(QStringLiteral("title")).toString();
    pretext = data.value(QStringLiteral("pretext")).toString();
    text = data.value(QStringLiteral("text")).toString();
    fallback = data.value(QStringLiteral("fallback")).toString();

    imageSize = QSize(data["image_width"].toInt(), data["image_height"].toInt());
    imageUrl = QUrl(data["image_url"].toString());

    color = data.value(QStringLiteral("color")).toString();
    if (color.isEmpty()) {
        color = QStringLiteral("theme");
    } else if (color == QStringLiteral("good")) {
        color = QStringLiteral("#6CC644");
    } else if (color == QStringLiteral("warning")) {
        color = QStringLiteral("#E67E22");
    } else if (color == QStringLiteral("danger")) {
        color = QStringLiteral("#D00000");
    } else if (!color.startsWith("#")) {
        color = "#" + color;
    }
}

Reaction::Reaction(const QJsonObject &data, QObject *parent) : QObject(parent)
{
    name = data[QStringLiteral("name")].toString();
    emoji = data[QStringLiteral("emoji")].toString();

    const QJsonArray usersList = data[QStringLiteral("users")].toArray();
    for (const QJsonValue &usersValue : usersList) {
        userIds.append(usersValue.toObject()["name"].toString());
    }
}
