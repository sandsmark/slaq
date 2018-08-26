#include "UsersModel.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QDateTime>
#include <QJsonArray>
#include <QApplication>
#include <QThread>
//user parse {
//    "color": "3c8c69",
//    "deleted": false,
//    "id": "U3SFDNEHY",
//    "is_admin": false,
//    "is_app_user": false,
//    "is_bot": false,
//    "is_owner": false,
//    "is_primary_owner": false,
//    "is_restricted": false,
//    "is_ultra_restricted": false,
//    "name": "tafuri",
//    "presence": "away",
//    "profile": {
//        "avatar_hash": "g225e85e1f2c",
//        "display_name": "tafuri",
//        "display_name_normalized": "tafuri",
//        "fields": null,
//        "first_name": "Sebastian",
//        "image_192": "https://secure.gravatar.com/avatar/225e85e1f2c0112175e4db9d1d2f6623.jpg?s=192&d=https%3A%2F%2Fcfr.slack-edge.com%2F7fa9%2Fimg%2Favatars%2Fava_0022-192.png",
//        "image_24": "https://secure.gravatar.com/avatar/225e85e1f2c0112175e4db9d1d2f6623.jpg?s=24&d=https%3A%2F%2Fcfr.slack-edge.com%2F66f9%2Fimg%2Favatars%2Fava_0022-24.png",
//        "image_32": "https://secure.gravatar.com/avatar/225e85e1f2c0112175e4db9d1d2f6623.jpg?s=32&d=https%3A%2F%2Fcfr.slack-edge.com%2F66f9%2Fimg%2Favatars%2Fava_0022-32.png",
//        "image_48": "https://secure.gravatar.com/avatar/225e85e1f2c0112175e4db9d1d2f6623.jpg?s=48&d=https%3A%2F%2Fcfr.slack-edge.com%2F66f9%2Fimg%2Favatars%2Fava_0022-48.png",
//        "image_512": "https://secure.gravatar.com/avatar/225e85e1f2c0112175e4db9d1d2f6623.jpg?s=512&d=https%3A%2F%2Fcfr.slack-edge.com%2F7fa9%2Fimg%2Favatars%2Fava_0022-512.png",
//        "image_72": "https://secure.gravatar.com/avatar/225e85e1f2c0112175e4db9d1d2f6623.jpg?s=72&d=https%3A%2F%2Fcfr.slack-edge.com%2F66f9%2Fimg%2Favatars%2Fava_0022-72.png",
//        "last_name": "Tafuri",
//        "phone": "",
//        "real_name": "Sebastian Tafuri",
//        "real_name_normalized": "Sebastian Tafuri",
//        "skype": "",
//        "status_emoji": "",
//        "status_expiration": 0,
//        "status_text": "",
//        "status_text_canonical": "",
//        "team": "T21Q22G66",
//        "title": ""
//    },
//    "real_name": "Sebastian Tafuri",
//    "team_id": "T21Q22G66",
//    "tz": "Europe/Amsterdam",
//    "tz_label": "Central European Summer Time",
//    "tz_offset": 7200,
//    "updated": 1504672036
//}

User::User(QObject *parent) : QObject(parent) {}

User::User(const User &copy, QObject *parent) : QObject(parent) {
    m_userId = copy.m_userId;
    if (!copy.m_appId.isEmpty()) {
        m_appId = copy.m_appId;
    }
    if (!copy.m_botId.isEmpty()) {
        m_botId = copy.m_botId;
    }
    if (!copy.m_username.isEmpty()) {
        m_username = copy.m_username;
    }
    if (!copy.m_fullName.isEmpty()) {
        m_fullName = copy.m_fullName;
    }
    if (!copy.m_avatarUrl.isEmpty()) {
        m_avatarUrl = copy.m_avatarUrl;
    }
    m_isBot = copy.m_isBot;
    if (!copy.m_statusEmoji.isEmpty()) {
        m_statusEmoji = copy.m_statusEmoji;
    }
    if (!copy.m_status.isEmpty()) {
        m_status = copy.m_status;
    }
    if (!copy.m_email.isEmpty()) {
        m_email = copy.m_email;
    }
    if (copy.m_color != m_color) {
        m_color = copy.m_color;
    }
    if (copy.m_presence != m_presence) {
        m_presence = copy.m_presence;
    }
}

User::User(const QString &id, const QString &name, QObject *parent)
{
    m_userId = id;
    m_username = name;
    m_presence = Active;
}


void User::setData(const QJsonObject &data)
{
    //qDebug().noquote() << "user parse" << QJsonDocument(data).toJson();

    m_userId = data.value(QStringLiteral("id")).toString();
    m_isBot = data.value(QStringLiteral("is_bot")).toBool(false);
    const QJsonObject& profile = data.value(QStringLiteral("profile")).toObject();
    m_fullName = data.value(QStringLiteral("real_name")).toString();
    m_username = data.value(QStringLiteral("name")).toString();
    if (m_fullName.isEmpty()) {
        //qWarning() << "No full name set";
        m_fullName = profile.value(QStringLiteral("real_name")).toString();
    }
    m_avatarUrl = QUrl(profile.value(QStringLiteral("image_72")).toString());
    if (!m_avatarUrl.isValid()) {
        //qWarning() << "No avatar URL";
    }
    m_statusEmoji = profile.value(QStringLiteral("status_emoji")).toString();
    m_status = profile.value(QStringLiteral("status_text")).toString();
    m_email = profile.value(QStringLiteral("email")).toString();
    m_color = QColor("#" + data.value(QStringLiteral("color")).toString());
    m_presence = Unknown;

    if (m_isBot) {
        if (data.value(QStringLiteral("deleted")).toBool()) {
            m_presence = Deleted;
        } else {
            m_presence = Active;
        }
        m_appId = data.value(QStringLiteral("app_id")).toString();
        m_botId = profile.value(QStringLiteral("bot_id")).toString();
    }
}

void User::setPresence(const User::Presence presence)
{
    //qDebug() << "presence for" << m_userId << m_fullName << presence;
    m_presence = presence;
    emit presenceChanged();
}

User::Presence User::presence()
{
    return m_presence;
}

QString User::username() const
{
    return m_username;
}

QString User::fullName() const
{
    return m_fullName;
}

QUrl User::avatarUrl() const
{
    return m_avatarUrl;
}

bool User::isBot() const
{
    return m_isBot;
}

QString User::botId() const
{
    return m_botId;
}

bool User::selected() const
{
    return m_selected;
}

void User::setSelected(bool selected)
{
    m_selected = selected;
    emit selectedChanged(selected);
}

UsersModel::UsersModel(QObject *parent) : QAbstractListModel(parent)
{}

UsersModel::~UsersModel() {
    qWarning() << "Deleting User Model";
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
        return QVariant::fromValue(m_users.value(m_userIds.at(row)).data());
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

void UsersModel::updateUser(const QJsonObject &userData)
{
    const QString& userId = userData.value(QStringLiteral("id")).toString();
    User *olduser = m_users[userId];
    if (olduser == nullptr) {
        qWarning() << "no user found for" << userId;
        return;
    }
    User *user = new User(olduser);
    user->setData(userData);
    if (QThread::currentThread() != qApp->thread()) {
        user->moveToThread(qApp->thread());
    }
    QQmlEngine::setObjectOwnership(user, QQmlEngine::CppOwnership);
    if (m_users[user->userId()]) {
        m_users[user->userId()]->deleteLater();
    }
    m_users[user->userId()] = user;
    int row  = m_userIds.indexOf(user->userId());
    QModelIndex index = QAbstractListModel::index (row, 0,  QModelIndex());
    emit dataChanged(index, index);
}

void UsersModel::addUser(User *user)
{
    if (QThread::currentThread() != qApp->thread()) {
        user->moveToThread(qApp->thread());
    }
    beginInsertRows(QModelIndex(), m_users.count(), m_users.count());
    m_userIds.append(user->userId());
    m_users.insert(user->userId(), user);
    endInsertRows();
    if (user->username().isEmpty()) {
        //user data is empty. request user's info
        qDebug() << "requesting users info";
        emit requestUserInfo(user);
    }
}

void UsersModel::addUser(const QJsonObject &userData)
{
    User *user = new User;
    user->setData(userData);
    QQmlEngine::setObjectOwnership(user, QQmlEngine::CppOwnership);
    addUser(user);
}

void UsersModel::addUsers(const QList<QPointer<User>>& users, bool last)
{
    qDebug() << "Adding users" << users.count() << last;
    m_addingUsers = true;
    beginInsertRows(QModelIndex(), m_users.count(), m_users.count() + users.count() - 1);
    for (QPointer<User> user : users) {
        //user.data()->moveToThread(QApplication::instance()->thread());
        if (m_users.contains(user->userId())) {
            m_users.value(user->userId())->deleteLater();
            m_userIds.removeAll(user->userId());
        }

        m_userIds.append(user->userId());
        m_users[user->userId()] = user;

        //if user is a bot as well
        if (user->isBot() || !user->botId().isEmpty()) {
            if (m_users.contains(user->botId())) {
                m_users.value(user->botId())->deleteLater();
                m_userIds.removeAll(user->botId());
            }

            m_userIds.append(user->botId());
            m_users[user->botId()] = user;
        }
        //qDebug() << "Added user" << user->userId() << this;
    }

    endInsertRows();
    if (last) {
        m_addingUsers = false;
    }
}

QPointer<User> UsersModel::user(const QString &id)
{
    if (m_addingUsers) {
        qWarning() << "NOT ALL USERS ADDED!";
    }
    return m_users.value(id);
}

QStringList UsersModel::selectedUserIds()
{
    QStringList _list;
    for (QPointer<User> user : m_users){
        if (user->selected()) {
            _list.append(user->userId());
        }
    }
    return _list;
}

bool UsersModel::isSelected() const
{
    for (QPointer<User> user : m_users){
        if (user->selected()) {
            return true;
        }
    }
    return false;
}

void UsersModel::setSelected(int index)
{
    QString _id = m_userIds.at(index);
    m_users.value(_id)->setSelected(!m_users.value(_id)->selected());
    m_selected = isSelected();
    emit selectedChanged(m_selected);
}

bool UsersModel::usersFetched() const
{
    return m_usersFetched;
}

void UsersModel::setUsersFetched(bool usersFetched)
{
    m_usersFetched = usersFetched;
}

bool UsersModel::selected() const
{
    return m_selected;
}

QMap<QString, QPointer<User> > UsersModel::users() const
{
    return m_users;
}


