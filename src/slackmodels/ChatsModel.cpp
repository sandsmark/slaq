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
        qWarning() << __PRETTY_FUNCTION__ << "Chat for channel ID" << m_chatIds[row] << "not found";
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
    case IsGeneral:
        return chat->isGeneral;
    case LastRead:
        return chat->lastRead;
    case UnreadCountDisplay:
        return chat->unreadCountDisplay;
    case UnreadCount:
        return chat->unreadCount;
    case Presence:
        if (!chat->membersModel->users().isEmpty()) {
            return chat->membersModel->users().first()->presence();
        } else {
            return User::Unknown;
        }
    case PresenceIcon: {
        if (chat->type == ChatsModel::Channel
                || chat->type == ChatsModel::Group
                || chat->type == ChatsModel::MultiUserConversation) {
            return "qrc:/icons/group-icon.png";
        } else if (chat->type == ChatsModel::Conversation) {
            if (chat->membersModel->users().isEmpty()) {
                return "qrc:/icons/offline-icon.png";
            } else {
                User::Presence presence = chat->membersModel->users().first()->presence();
                if (presence == User::Away) {
                    return "qrc:/icons/away-icon.png";
                } else if (presence == User::Dnd) {
                    return "qrc:/icons/dnd-icon.png";
                } else if (presence == User::Active) {
                    return "qrc:/icons/active-icon.png";
                }
                return "qrc:/icons/offline-icon.png";
            }
        }
        break;
    }
    case MembersModel:
        return QVariant::fromValue(chat->membersModel.data());
    case MessagesModel:
        return QVariant::fromValue(chat->messagesModel.data());
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
    names[IsGeneral] = "IsGeneral";
    names[LastRead] = "LastRead";
    names[UnreadCount] = "UnreadCount";
    names[UnreadCountDisplay] = "UnreadCountDisplay";
    names[MembersModel] = "MembersModel";
    names[MessagesModel] = "MessagesModel";
    names[Presence] = "Presence";
    names[PresenceIcon] = "PresenceIcon";
    names[Section] = "Section";
    return names;

}

QString ChatsModel::addChat(Chat* chat)
{
    QString channelId = chat->id;
    if (m_chatIds.contains(chat->id)) {
        //replace chat
        if (chat->membersModel.isNull()) {
            chat->membersModel = new UsersModel(this);
            QQmlEngine::setObjectOwnership(chat->membersModel, QQmlEngine::CppOwnership);
        }
        if (chat->messagesModel.isNull()) {
            chat->messagesModel = new MessageListModel(this, m_networkUsers, chat->id);
            QQmlEngine::setObjectOwnership(chat->messagesModel, QQmlEngine::CppOwnership);
        }

        chatChanged(chat);
    } else {
        beginInsertRows(QModelIndex(), m_chats.count(), m_chats.count());
        channelId = doAddChat(chat);
        endInsertRows();
    }
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
    //qDebug() << __PRETTY_FUNCTION__ << chat->id << chat->type << chat->name;

    chat->membersModel = new UsersModel(this);
    chat->messagesModel = new MessageListModel(this, m_networkUsers, chat->id);
    QQmlEngine::setObjectOwnership(chat->membersModel, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(chat->messagesModel, QQmlEngine::CppOwnership);


    if (chat->type == Conversation) {
        QPointer<User> _user = m_networkUsers->user(chat->user);
        if (!_user.isNull()) {
            chat->membersModel->addUser(_user);
            chat->name = chat->membersModel->users().first()->fullName();
        }
        chat->isOpen = true;
    }

    if (chat->name.startsWith("mpdm")) {
        chat->setReadableName(m_selfId);
    }
    //check if there is unreads arrived before chat appears
    if (m_unreadNullChats.contains(chat->id)) {
        chat->unreadCountDisplay += m_unreadNullChats.value(chat->id);
        m_unreadNullChats.remove(chat->id);
    }
    m_chatIds.append(chat->id);
    m_chats.insert(chat->id, chat);
    return chat->id;
}

void ChatsModel::addChats(const QList<Chat*>& chats, bool last)
{
    qDebug() << "addChats" << chats.count() << last;

    beginInsertRows(QModelIndex(), m_chats.count(), m_chats.count() + chats.count() - 1);
    for (Chat *chat : chats) {
        if (!m_chatIds.contains(chat->id)) {
            doAddChat(chat);
        } else {
            qFatal("Chat already exists!!");
        }
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
    Q_ASSERT_X(!id.isEmpty(), "ChatsModel::chat", "requested id is empty");
    return m_chats.value(id);
}

Chat* ChatsModel::chat(int row)
{
    Q_ASSERT_X(m_chats.size() == m_chatIds.size(), "ChatsModel::chat", "m_chats.size() and m_chatIds.size() should be equal");
    //TODO: check why chats size != chat ids size
    return m_chats.value(m_chatIds.at(row));
}

Chat *ChatsModel::generalChat()
{
    for(Chat* chat : m_chats.values()) {
        if (chat->isGeneral) {
            return chat;
        }
    }
    return nullptr;
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

void ChatsModel::setPresence(const QStringList &users, const QString &presence)
{
    User::Presence _presence = (presence == "away" ? User::Away : (presence == "dnd_on" ? User::Dnd : User::Active));
    for (Chat* chat : m_chats.values()) {
        if (chat->type == ChatsModel::Conversation && chat->membersModel != nullptr
                && !chat->membersModel->users().isEmpty()) {
            QPointer<User> _user = chat->membersModel->users().first();
            if (!_user.isNull() && (users.contains(_user->userId()) || users.contains(_user->botId()))) {
                _user->setPresence(_presence);
                chatChanged(chat);
            }
        }
    }
}

void ChatsModel::increaseUnreadsInNull(const QString &channelId)
{
    m_unreadNullChats[channelId] = m_unreadNullChats.value(channelId, 0) + 1;
}

int ChatsModel::unreadsInNull(ChatsModel::ChatType type)
{
    int _total = 0;
    for (const QString& id : m_unreadNullChats.keys()) {
        if (type == Channel && id.startsWith("C")) {
            _total += m_unreadNullChats.value(id);
        } else if ((type == Group || type == MultiUserConversation) && id.startsWith("G")) {
            _total += m_unreadNullChats.value(id);
        } else if (type == Conversation && id.startsWith("D")) {
            _total += m_unreadNullChats.value(id);
        }
    }
    return _total;
}

int ChatsModel::unreadsInNullChannel(const QString &channelId)
{
    return m_unreadNullChats.value(channelId, 0);
}

bool Chat::isChatEmpty() const
{
    if (messagesModel.isNull()) {
        qWarning() << "messages model is null";
        return true;
    }
    return (messagesModel->rowCount(QModelIndex()) <= 0);
}

Chat::Chat(const QJsonObject &data, const ChatsModel::ChatType type_, QObject *parent) : QObject (parent)
{
    setData(data, type_);
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

void Chat::setData(const QJsonObject &data, const ChatsModel::ChatType type_)
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

    isGeneral = data.value(QStringLiteral("is_general")).toBool(false);
    name = data.value(QStringLiteral("name")).toString(name);
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
}

