#include "ChatsModel.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QDateTime>
#include <QJsonArray>

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

ChatsModel::ChatsModel(const QString &selfId, QObject *parent, UsersModel *networkUsers) : QAbstractListModel(parent),
    m_selfId(selfId), m_networkUsers(networkUsers) {}

QString ChatsModel::getSectionName(const Chat& chat) const {
    switch (chat.type) {
    case Channel:
        return chat.unreadCountDisplay > 0 ? tr("Unread channels") : tr("Channels");

    case Group:
        return chat.unreadCountDisplay > 0 ? tr("Unread chats") : tr("Chats");

    case Conversation:
        return chat.unreadCountDisplay > 0 ? tr("Unread DM") : tr("Direct messages");

    }
    return tr("Other");
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
        return chat.readableName.isEmpty() ? chat.name : chat.readableName;
    case IsOpen:
        return chat.isOpen;
    case LastReadId:
        return chat.lastReadId;
    case UnreadCountDisplay:
        return chat.unreadCountDisplay;
    case UnreadCount:
        return chat.unreadCount;
    case Presence:
        return chat.presence;
    case MembersModel:
        return QVariant::fromValue(chat.membersModel.data());
    case MessagesModel:
        return QVariant::fromValue(chat.messagesModel.data());
    case UserObject:
        return QVariant::fromValue(chat.user.data());
    case Section:
        return getSectionName(chat);
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
    names[UnreadCountDisplay] = "UnreadCountDisplay";
    names[MembersModel] = "MembersModel";
    names[MessagesModel] = "MessagesModel";
    names[UserObject] = "UserObject";
    names[Presence] = "Presence";
    names[Section] = "Section";
    return names;

}

void ChatsModel::addChat(const QJsonObject &data, const ChatType type)
{
//    qDebug() << type;
//    qDebug().noquote() << QJsonDocument(data).toJson();

    Chat chat(data, type);

    chat.membersModel = new UsersModel(this);
    chat.messagesModel = new MessageListModel(this, m_networkUsers, chat.id);
    QQmlEngine::setObjectOwnership(chat.membersModel, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(chat.messagesModel, QQmlEngine::CppOwnership);


    if (type == Conversation) {
        chat.user = m_networkUsers->user(data["user"].toString());
    } else {
        for (const QJsonValue &userId : data["members"].toArray()) {
            chat.membersModel->addUser(m_networkUsers->user(userId.toString()));
        }
    }

    if (type != Channel) {
        chat.setReadableName(m_selfId);
    }
    m_chatIds.append(chat.id);

    m_chats.insert(chat.id, chat);
}

void ChatsModel::addChats(const QJsonArray &chats, const ChatType type)
{
    qDebug() << "addChats" << type << chats.count();

    beginInsertRows(QModelIndex(), m_chats.count(), m_chats.count() + chats.count());
    for (const QJsonValue &value : chats) {
        addChat(value.toObject(), type);
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

Chat &ChatsModel::chat(const QString &id)
{
    return m_chats[id];
}

Chat::Chat(const QJsonObject &data, const ChatsModel::ChatType type_)
{
    id = data.value(QStringLiteral("id")).toString();
    type = type_;
    name = data.value(QStringLiteral("name")).toString();
    presence = QStringLiteral("none");
    isOpen = (type == ChatsModel::Channel) ? data.value(QStringLiteral("is_member")).toBool() :
                                             data.value(QStringLiteral("is_open")).toBool();
    lastReadId = data.value(QStringLiteral("last_read")).toString();
    unreadCountDisplay = data.value(QStringLiteral("unread_count_display")).toInt();
    unreadCount = data.value(QStringLiteral("unread_count")).toInt();
}

void Chat::setReadableName(const QString& selfId) {
    QStringList _users;
    for (QPointer<User> user : membersModel->users()) {
        if (user->userId() != selfId) {
            _users << user->username();
        }
    }
    readableName = _users.join(", ");
}

