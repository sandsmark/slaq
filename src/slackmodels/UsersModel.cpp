#include "UsersModel.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QDateTime>
#include <QJsonArray>

User::User(const QJsonObject &data, QObject *parent) : QObject(parent)
{
//    qDebug().noquote() << QJsonDocument(data).toJson();

    m_userId = data.value(QStringLiteral("id")).toString();
    if (m_userId.isEmpty()) {
        qWarning() << "No user id";
        return;
    }

    if (m_userId.startsWith('B')) {
        m_isBot = true;
    }

    if (m_isBot) {
        if (data.value(QStringLiteral("deleted")).toBool()) {
            m_presence = Deleted;
        } else {
            m_presence = Active;
        }
        QJsonObject icons = data.value(QStringLiteral("icons")).toObject();
        m_avatarUrl = QUrl(icons[QStringLiteral("image_72")].toString());
        m_appId = data.value(QStringLiteral("app_id")).toString();
    } else {
        m_fullName = data.value(QStringLiteral("real_name")).toString();
        if (m_fullName.isEmpty()) {
            //qWarning() << "No full name set";
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
        m_avatarUrl = QUrl(profile[QStringLiteral("image_72")].toString());
        if (!m_avatarUrl.isValid()) {
            //qWarning() << "No avatar URL";
        }
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
        QQmlEngine::setObjectOwnership(user, QQmlEngine::CppOwnership);

        if (m_users[user->userId()]) {
            m_users[user->userId()]->deleteLater();
            m_userIds.removeAll(user->userId());
        }

        m_userIds.append(user->userId());
        m_users[user->userId()] = user;
        //qDebug() << "Added user" << user->userId() << this;
    }

    endInsertRows();
}

QPointer<User> UsersModel::user(const QString &id)
{
    return m_users.value(id);
}


