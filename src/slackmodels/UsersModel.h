#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QPointer>
#include <QAbstractListModel>
#include <QSize>
#include <QJsonArray>
#include <QDateTime>
#include <QColor>

class User : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString userId MEMBER m_userId CONSTANT)
    Q_PROPERTY(QString appId MEMBER m_appId CONSTANT)
    Q_PROPERTY(QString username MEMBER m_username CONSTANT)
    Q_PROPERTY(QString fullName MEMBER m_fullName CONSTANT)
    Q_PROPERTY(QString statusEmoji MEMBER m_statusEmoji CONSTANT)
    Q_PROPERTY(QColor color MEMBER m_color CONSTANT)
    Q_PROPERTY(QUrl avatarUrl MEMBER m_avatarUrl CONSTANT)
    Q_PROPERTY(bool isBot MEMBER m_isBot CONSTANT)
    Q_PROPERTY(Presence presence READ presence NOTIFY presenceChanged)

public:
    enum Presence {
        Unknown,
        Active,
        Away,
        Deleted //for bots
    };
    Q_ENUM(Presence)

    User(const QJsonObject &data, QObject *parent);

    void setPresence(const Presence presence);
    Presence presence();

    const QString &userId() { return m_userId; }
    QString username() const;
    QString fullName() const;
    QUrl avatarUrl() const;
    bool isBot() const;

signals:
    void presenceChanged();

private:
    QString m_userId;
    QString m_appId;
    QString m_username;
    QString m_fullName;
    QUrl m_avatarUrl;
    bool m_isBot = false;
    QString m_statusEmoji;
    QColor m_color;
    Presence m_presence = Unknown;
};

class UsersModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Fields {
        UserObject,
        FieldsCount
    };

    enum Presence {
        Unknown,
        Active,
        Away,
        Deleted //for bots
    };
    Q_ENUM(Presence)

    UsersModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const override { return m_users.count(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addUser(User *user);
    void addUsers(const QJsonArray &usersData);
    QPointer<User> user(const QString &id);
    Q_INVOKABLE int fooCount() { return m_users.count(); }

private:
    QMap<QString, QPointer<User>> m_users;
    QStringList m_userIds;
};

