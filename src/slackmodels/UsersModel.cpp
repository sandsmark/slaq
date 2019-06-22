#include "UsersModel.h"
#include "messageformatter.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QDateTime>
#include <QJsonArray>
#include <QApplication>
#include <QThread>
#include <QTimer>
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

SlackUser::SlackUser(QObject *parent) : QObject(parent) {}

SlackUser::SlackUser(const SlackUser &copy, QObject *parent) : QObject(parent) {
    copyData(copy);
}

SlackUser::SlackUser(const QString &id, const QString &name, QObject *parent)
{
    m_userId = id;
    m_username = name;
    m_presence = Active;
}


void SlackUser::setData(const QJsonObject &data)
{
    //qDebug().noquote() << "user parse" << QJsonDocument(data).toJson();

    m_userId = data.value(QStringLiteral("id")).toString();
    m_isBot = data.value(QStringLiteral("is_bot")).toBool(false);
    const QJsonObject& profile = data.value(QStringLiteral("profile")).toObject();
    m_fullName = data.value(QStringLiteral("real_name")).toString();
    setUsername(data.value(QStringLiteral("name")).toString());
    if (m_fullName.isEmpty()) {
        //qWarning() << "No full name set";
        m_fullName = profile.value(QStringLiteral("real_name")).toString();
    }
    setAvatarUrl(QUrl(profile.value(QStringLiteral("image_72")).toString()));
    if (!m_avatarUrl.isValid()) {
        //qWarning() << "No avatar URL";
    }
    setStatusEmoji(profile.value(QStringLiteral("status_emoji")).toString());
    setStatus(profile.value(QStringLiteral("status_text")).toString());
    setFirstName(profile.value(QStringLiteral("first_name")).toString());
    setLastName(profile.value(QStringLiteral("last_name")).toString());
    setEmail(profile.value(QStringLiteral("email")).toString());
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

void SlackUser::copyData(const SlackUser &copy)
{
    setUserId(copy.m_userId);
    if (!copy.m_appId.isEmpty()) {
        m_appId = copy.m_appId;
    }
    if (!copy.m_botId.isEmpty()) {
        m_botId = copy.m_botId;
    }
    if (!copy.m_username.isEmpty()) {
        setUsername(copy.m_username);
    }
    if (!copy.m_fullName.isEmpty()) {
        setFullName(copy.m_fullName);
    }
    if (!copy.m_avatarUrl.isEmpty()) {
        setAvatarUrl(copy.m_avatarUrl);
    }
    m_isBot = copy.m_isBot;
    if (!copy.m_statusEmoji.isEmpty()) {
        setStatusEmoji(copy.m_statusEmoji);
    }
    if (!copy.m_status.isEmpty()) {
        setStatus(copy.m_status);
    }
    if (!copy.m_email.isEmpty()) {
        setEmail(copy.m_email);
    }
    if (copy.m_color != m_color) {
        setColor(copy.m_color);
    }
    if (copy.m_presence != m_presence) {
        setPresence(copy.m_presence);
    }
}

void SlackUser::setPresence(const SlackUser::Presence presence, bool force)
{
    //qDebug() << "presence for" << m_userId << m_fullName << presence;
    if (!force && m_presence == Dnd && m_snoozeEnds.isValid() && m_snoozeEnds > QDateTime::currentDateTime()) {
        qWarning() << "User currently in DnD mode" << m_userId << m_username << force;
        return;
    }
    m_presence = presence;
    if (m_presence != Dnd && m_snoozeEnds.isValid()) {
        setSnoozeEnds(QDateTime());
    }
    emit presenceChanged();
}

SlackUser::Presence SlackUser::presence()
{
    return m_presence;
}

QString SlackUser::username() const
{
    //qDebug() << "username" << m_username << "for" << m_userId << this;
    return m_username;
}

QString SlackUser::fullName() const
{
    return m_fullName;
}

QUrl SlackUser::avatarUrl() const
{
    return m_avatarUrl;
}

bool SlackUser::isBot() const
{
    return m_isBot;
}

QString SlackUser::botId() const
{
    return m_botId;
}

bool SlackUser::selected() const
{
    return m_selected;
}

void SlackUser::setSelected(bool selected)
{
    m_selected = selected;
    emit selectedChanged(selected);
}

void SlackUser::setUserId(const QString &userId)
{
    m_userId = userId;
}

QString SlackUser::email() const
{
    return m_email;
}

void SlackUser::setEmail(const QString &email)
{
    m_email = email;
    emit emailChanged(email);
}

QString SlackUser::status() const
{
    return m_status;
}

void SlackUser::setStatus(const QString &status)
{
    m_status = status;
    MessageFormatter::replaceEmoji(m_status);
    emit statusChanged(status);
}

QString SlackUser::statusEmoji() const
{
    return m_statusEmoji;
}

void SlackUser::setStatusEmoji(const QString &statusEmoji)
{
    m_statusEmoji = statusEmoji;
    emit statusEmojiChanged(statusEmoji);
}

void SlackUser::setAvatarUrl(const QUrl &avatarUrl)
{
    m_avatarUrl = avatarUrl;
    emit avatarChanged(avatarUrl);
}

QString SlackUser::firstName() const
{
    return m_firstName;
}

QString SlackUser::lastName() const
{
    return m_lastName;
}

QDateTime SlackUser::snoozeEnds() const
{
    return m_snoozeEnds;
}

void SlackUser::setFirstName(const QString &firstName)
{
    if (m_firstName == firstName)
        return;

    m_firstName = firstName;
    emit firstNameChanged(m_firstName);
}

void SlackUser::setLastName(const QString &lastName)
{
    if (m_lastName == lastName)
        return;

    m_lastName = lastName;
    emit lastNameChanged(m_lastName);
}

void SlackUser::setUsername(QString username)
{
    if (m_username == username)
        return;

    m_username = username;
    emit usernameChanged(m_username);
}

void SlackUser::setColor(QColor color)
{
    if (m_color == color)
        return;

    m_color = color;
    emit colorChanged(m_color);
}

void SlackUser::setFullName(QString fullName)
{
    if (m_fullName == fullName)
        return;

    m_fullName = fullName;
    emit fullNameChanged(m_fullName);
}

QString SlackUser::fullName()
{
    return m_fullName;
}

void SlackUser::setSnoozeEnds(const QDateTime &snoozeEnds)
{
    m_snoozeEnds = snoozeEnds;
    qint64 _msecs = QDateTime::currentDateTime().msecsTo(snoozeEnds);
    if (_msecs > 0) {
        QTimer::singleShot(_msecs, this, [this] {
            qDebug() << "User DnD timer expired" << username();
            m_snoozeEnds = QDateTime();
            this->setPresence(SlackUser::Active, true);
        });
    }
    emit snoozeEndsChanged(snoozeEnds);
}

QColor SlackUser::color() const
{
    return m_color;
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
        qWarning() << "users invalid row" << row;
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

QPointer<SlackUser> UsersModel::updateUser(const QJsonObject &userData)
{
    const QString& userId = userData.value(QStringLiteral("id")).toString();
    QPointer<SlackUser> _user = m_users.value(userId);
    if (_user.isNull()) {
        qWarning() << "no user found for" << userId << "adding new one";
        addUser(userData);
        _user = m_users.value(userId);
    } else {
        _user->setData(userData);
    }
    if (_user.isNull()) {
        qWarning() << "still no user found for" << userId;
        return _user;
    }
    QString _id = _user->userId();
    if (_user->isBot()) {
        _id = _user->botId();
    }
    int row  = m_userIds.indexOf(_id);
    QModelIndex index = QAbstractListModel::index(row, 0,  QModelIndex());
    emit dataChanged(index, index, roleNames().keys().toVector());
    qDebug() << "updated user" << _user->userId() << _user->username();// << _user;
    return _user;
}

void UsersModel::addUser(SlackUser *user)
{
    user->moveToThread(qApp->thread());
    QMutexLocker locker(&m_modelMutex);
    int row  = m_userIds.indexOf(user->userId());
    if (row >= 0) {
        m_users.insert(user->userId(), user);
        QModelIndex index = QAbstractListModel::index(row, 0,  QModelIndex());
        emit dataChanged(index, index, roleNames().keys().toVector());
    } else {
        beginInsertRows(QModelIndex(), m_users.count(), m_users.count());
        if (user->isBot() && !user->botId().isEmpty()) {
            m_userIds.append(user->botId());
            m_users.insert(user->botId(), user);
        } else {
            m_userIds.append(user->userId());
            m_users.insert(user->userId(), user);
        }
        endInsertRows();
        if (user->username().isEmpty()) {
            //user data is empty. request user's info
            qDebug() << "requesting users info for" << user->userId();
            emit requestUserInfo(user);
        }
    }
}

void UsersModel::addUser(const QJsonObject &userData)
{
    SlackUser *user = new SlackUser;
    user->setData(userData);
    QQmlEngine::setObjectOwnership(user, QQmlEngine::CppOwnership);
    addUser(user);
}

void UsersModel::addUsers(const QList<QPointer<SlackUser>>& users, bool last)
{
    int _count = m_users.count();
    qDebug() << "Adding users" << _count << last;

    m_addingUsers = true;
    {
        QMutexLocker locker(&m_modelMutex);
        for (QPointer<SlackUser> user : users) {
            if (m_users.contains(user->userId())) {
                QPointer<SlackUser> _user = m_users.value(user->userId());
                if (_user->username().isEmpty() && !user->username().isEmpty()) {
                    _user->copyData(user.data());
                    int row = m_userIds.indexOf(_user->userId());
                    if (row >= 0) {
                        QModelIndex index = QAbstractListModel::index(row, 0,  QModelIndex());
                        emit dataChanged(index, index, roleNames().keys().toVector());
                    }
                }
                continue;
            }
            int _cnt = m_users.count();
            //if user is a bot as well
            if (user->isBot() && !user->botId().isEmpty()) {
                m_users.insert(user->botId(), user);
                if (_cnt < m_users.count()) { //user added
                    m_userIds.append(user->botId());
                }
            } else {
                m_users.insert(user->userId(), user);
                if (_cnt < m_users.count()) {
                    m_userIds.append(user->userId());
                }
            }
            //qDebug() << "Added user" << user->userId() << this;
        }
    }
    _count = m_users.count() - _count;
    if (_count > 0) {
        beginInsertRows(QModelIndex(), m_userIds.count(), m_userIds.count() + _count - 1);
        endInsertRows();
    }
    if (last) {
        m_addingUsers = false;
    }
    qDebug() << "added users" << m_userIds.count() << m_users.count();
}

QPointer<SlackUser> UsersModel::user(const QString &id)
{
//    if (m_addingUsers) {
//        qWarning() << "NOT ALL USERS ADDED!";
//    }
    createAndRequestInfo(id);
    return m_users.value(id);
}

void UsersModel::createAndRequestInfo(const QString &id)
{
//    if (m_addingUsers) {
//        qWarning() << "NOT ALL USERS ADDED!";
//    }
    if (!m_users.contains(id)) {
        QPointer<SlackUser> _user = new SlackUser;
        QQmlEngine::setObjectOwnership(_user, QQmlEngine::CppOwnership);
        _user->setUserId(id);
        addUser(_user);
    }
}

QStringList UsersModel::selectedUserIds()
{
    QStringList _list;
    for (QPointer<SlackUser> user : m_users){
        if (user->selected()) {
            _list.append(user->userId());
        }
    }
    return _list;
}

bool UsersModel::isSelected() const
{
    for (QPointer<SlackUser> user : m_users){
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

void UsersModel::clearSelections()
{
    for (const QString& userId : m_userIds) {
        QPointer<SlackUser> user = m_users.value(userId);
        if (!user.isNull()) {
            user->setSelected(false);
        } else {
            qWarning() << "null user in the model" << userId;
        }
    }
    m_selected = false;
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

QMap<QString, QPointer<SlackUser> > UsersModel::users() const
{
    return m_users;
}


