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

QString ChatsModel::getSectionName(Chat* chat) const {
    switch (chat->type) {
    case Channel:
        return tr("Channels");

    case Group:
    case MultiUserConversation:
        return tr("Chats");

    case Conversation:
        return tr("Direct messages");

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
    Chat* chat = m_chats[m_chatIds[row]];
    if (chat == nullptr) {
        qWarning() << "Chat for channel ID" << m_chatIds[row] << "not found";
        return QVariant();
    }
    switch (role) {
    case Id:
        return chat->id;
    case Type:
        return chat->type;
    case Name:
        return chat->readableName.isEmpty() ? chat->name : chat->readableName;
    case IsOpen:
        return chat->isOpen;
    case LastRead:
        return chat->lastRead;
    case UnreadCountDisplay:
        return chat->unreadCountDisplay;
    case UnreadCount:
        return chat->unreadCount;
    case Presence:
        return chat->presence;
    case MembersModel:
        return QVariant::fromValue(chat->membersModel.data());
    case MessagesModel:
        return QVariant::fromValue(chat->messagesModel.data());
    case UserObject:
        return QVariant::fromValue(chat->membersModel->users().first().data());
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
    names[LastRead] = "LastRead";
    names[UnreadCount] = "UnreadCount";
    names[UnreadCountDisplay] = "UnreadCountDisplay";
    names[MembersModel] = "MembersModel";
    names[MessagesModel] = "MessagesModel";
    names[UserObject] = "UserObject";
    names[Presence] = "Presence";
    names[Section] = "Section";
    return names;

}

QString ChatsModel::addChat(const QJsonObject &data, const ChatsModel::ChatType type)
{
    beginInsertRows(QModelIndex(), m_chats.count(), m_chats.count());
    QString channelId = doAddChat(data, type);
    endInsertRows();
    return channelId;
}

void ChatsModel::removeChat(const QString &channelId)
{
    int row = m_chatIds.indexOf(channelId);
    if (row < 0) {
        qWarning() << "Channel ID not found" << channelId;
        return;
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_chats.remove(channelId);
    m_chatIds.removeAt(row);
    endRemoveRows();
}

QString ChatsModel::doAddChat(const QJsonObject &data, const ChatType type)
{
//    qDebug() << type;
    //qDebug().noquote() << QJsonDocument(data).toJson();

    Chat* chat = new Chat(data, type);
    if (chat == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "Error allocating memory for Chat";
        return QString();
    }
    chat->membersModel = new UsersModel(this);
    chat->messagesModel = new MessageListModel(this, m_networkUsers, chat->id);
    QQmlEngine::setObjectOwnership(chat, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(chat->membersModel, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(chat->messagesModel, QQmlEngine::CppOwnership);


    if (chat->type == Conversation) {
        chat->membersModel->addUser(m_networkUsers->user(data.value("user").toString()));
        qDebug() << "user for conv" << chat->membersModel->users().first()->userId();
    } else {
        for (const QJsonValueRef &userId : data.value("members").toArray()) {
            chat->membersModel->addUser(m_networkUsers->user(userId.toString()));
        }
    }

    if (chat->type == Conversation || chat->name.startsWith("mpdm")) {
        chat->setReadableName(m_selfId);
    }
    m_chatIds.append(chat->id);

    m_chats.insert(chat->id, chat);
    return chat->id;
}

void ChatsModel::addChats(const QJsonArray &chats, const ChatType type)
{
    qDebug() << "addChats" << type << chats.count();

    beginInsertRows(QModelIndex(), m_chats.count(), m_chats.count() + chats.count() - 1);
    for (const QJsonValue &value : chats) {
        doAddChat(value.toObject(), type);
    }

    endInsertRows();
}

bool ChatsModel::hasChannel(const QString &id)
{
    return m_chatIds.contains(id);
}

MessageListModel *ChatsModel::messages(const QString &id)
{
    return m_chats[id]->messagesModel;
}

UsersModel *ChatsModel::members(const QString &id)
{
    return m_chats[id]->membersModel;
}

Chat* ChatsModel::chat(const QString &id) {
    return m_chats[id];
}

Chat* ChatsModel::chat(int row)
{
    Q_ASSERT_X(m_chats.size() == m_chatIds.size(), "ChatsModel::chat", "m_chats.size() and m_chatIds.size() should be equal");
    //TODO: check why chats size != chat ids size
    return m_chats[m_chatIds[row]];
}

void ChatsModel::chatChanged(Chat *chat)
{
    if (chat->id.isEmpty()) {
        return;
    }
    int row = m_chatIds.indexOf(chat->id);
    m_chats[chat->id] = chat;
    Q_ASSERT_X(m_chats.size() == m_chatIds.size(), "ChatsModel::chatChanged", "m_chats.size() and m_chatIds.size() should be equal");
    QModelIndex index = QAbstractListModel::index(row, 0,  QModelIndex());
    emit dataChanged(index, index);
}

Chat::Chat(const QJsonObject &data, const ChatsModel::ChatType type_, QObject *parent) : QObject (parent)
{
    id = data.value(QStringLiteral("id")).toString();
    type = type_;
    if (data.value(QStringLiteral("is_mpim")).toBool(false)) {
        type = ChatsModel::MultiUserConversation;
    } else if (data.value(QStringLiteral("is_group")).toBool(false)) {
        type = ChatsModel::Group;
    } else if (data.value(QStringLiteral("is_channel")).toBool(false)) {
        type = ChatsModel::Channel;
    } else if (data.value(QStringLiteral("is_im")).toBool(false)) {
        type = ChatsModel::Conversation;
    }

    name = data.value(QStringLiteral("name")).toString();
    presence = QStringLiteral("none");
    isOpen = (type == ChatsModel::Channel) ? data.value(QStringLiteral("is_member")).toBool() :
                                             data.value(QStringLiteral("is_open")).toBool();

    isPrivate = data.value(QStringLiteral("is_private")).toBool(false);
    lastRead = slackToDateTime(data.value(QStringLiteral("last_read")).toString());
    creationDate = slackToDateTime(data.value(QStringLiteral("created")).toString());
    unreadCountDisplay = data.value(QStringLiteral("unread_count_display")).toInt();
    unreadCount = data.value(QStringLiteral("unread_count")).toInt();
    topic = data.value(QStringLiteral("topic")).toObject().value(QStringLiteral("value")).toString();
    purpose = data.value(QStringLiteral("purpose")).toObject().value(QStringLiteral("value")).toString();
    //qDebug() << "new chat" << name << id << type;
}

void Chat::setReadableName(const QString& selfId) {
    QStringList _users;
    for (const QPointer<User>& user : membersModel->users()) {
        if (user->userId() != selfId) {
            _users << user->username();
        }
    }
    readableName = _users.join(", ");
}

