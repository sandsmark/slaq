#include "ChatsModel.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QDateTime>
#include <QJsonArray>

ChatsModel::ChatsModel(const QString &selfId, QObject *parent, UsersModel *networkUsers) : QAbstractListModel(parent),
    m_networkUsers(networkUsers), m_selfId(selfId) {}

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
    Chat* chat = m_chats.value(m_chatIds.at(row));
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

QString ChatsModel::addChat(Chat* chat)
{
    beginInsertRows(QModelIndex(), m_chats.count(), m_chats.count());
    QString channelId = doAddChat(chat);
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

QString ChatsModel::doAddChat(Chat *chat)
{
//    qDebug() << type;
    //qDebug() << __PRETTY_FUNCTION__ << chat->id << chat->type;

    chat->membersModel = new UsersModel(this);
    chat->messagesModel = new MessageListModel(this, m_networkUsers, chat->id);
    QQmlEngine::setObjectOwnership(chat, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(chat->membersModel, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(chat->messagesModel, QQmlEngine::CppOwnership);


    if (chat->type == Conversation) {
        chat->membersModel->addUser(m_networkUsers->user(chat->user));
        chat->isOpen = true;
        chat->name = chat->membersModel->users().first()->fullName();
    }

    if (chat->name.startsWith("mpdm")) {
        chat->setReadableName(m_selfId);
    }
    m_chatIds.append(chat->id);
    m_chats.insert(chat->id, chat);
    return chat->id;
}

void ChatsModel::addChats(const QList<Chat*>& chats)
{
    qDebug() << "addChats" << chats.count();

    beginInsertRows(QModelIndex(), m_chats.count(), m_chats.count() + chats.count() - 1);
    for (Chat *chat : chats) {
        doAddChat(chat);
    }
    endInsertRows();
}

void ChatsModel::addMembers(const QString &channelId, const QStringList &members)
{
    Chat* _chat = chat(channelId);
    if (_chat == nullptr) {
        return;
    }
    for (const QString &userId : members) {
        QPointer<User> user = m_networkUsers->user(userId);
        if (!user.isNull()) {
            _chat->membersModel->addUser(user);
        } else {
            qWarning() << "user not found for ID" << userId;
        }

    }
    if (_chat->type == Conversation || _chat->name.startsWith("mpdm")) {
        _chat->setReadableName(m_selfId);
        qDebug() << "readable name" << _chat->id << _chat->readableName;
    }
    chatChanged(_chat);
}

bool ChatsModel::hasChannel(const QString &id)
{
    return m_chatIds.contains(id);
}

MessageListModel *ChatsModel::messages(const QString &id)
{
    if (m_chats.contains(id)) {
        return m_chats.value(id)->messagesModel;
    }
    return nullptr;
}

UsersModel *ChatsModel::members(const QString &id)
{
    if (m_chats.contains(id)) {
        return m_chats.value(id)->membersModel;
    }
    return nullptr;
}

Chat* ChatsModel::chat(const QString &id) {
    return m_chats.value(id);
}

Chat* ChatsModel::chat(int row)
{
    Q_ASSERT_X(m_chats.size() == m_chatIds.size(), "ChatsModel::chat", "m_chats.size() and m_chatIds.size() should be equal");
    //TODO: check why chats size != chat ids size
    return m_chats.value(m_chatIds.at(row));
}

void ChatsModel::chatChanged(Chat *chat)
{
    if (chat == nullptr || chat->id.isEmpty()) {
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
    user = data.value("user").toString();

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
    for (QPointer<User> user : membersModel->users()) {
        if (user->userId() != selfId) {
            if (!user->fullName().isEmpty()) {
                _users << user->fullName();
            } else if (!user->username().isEmpty()) {
                _users << user->username();
            } else {
                _users << user->userId();
            }
        }
    }
    readableName = _users.join(", ");
}

