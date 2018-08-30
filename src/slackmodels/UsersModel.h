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
#include <QMutexLocker>

class User : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString userId MEMBER m_userId CONSTANT)
    Q_PROPERTY(QString botId MEMBER m_botId CONSTANT)
    Q_PROPERTY(QString appId MEMBER m_appId CONSTANT)
    Q_PROPERTY(QString username MEMBER m_username CONSTANT)
    Q_PROPERTY(QString fullName MEMBER m_fullName CONSTANT)
    Q_PROPERTY(QString statusEmoji MEMBER m_statusEmoji CONSTANT)
    Q_PROPERTY(QString status MEMBER m_status CONSTANT)
    Q_PROPERTY(QString email MEMBER m_email CONSTANT)
    Q_PROPERTY(QColor color MEMBER m_color CONSTANT)
    Q_PROPERTY(QUrl avatarUrl MEMBER m_avatarUrl CONSTANT)
    Q_PROPERTY(bool isBot MEMBER m_isBot CONSTANT)
    Q_PROPERTY(Presence presence READ presence NOTIFY presenceChanged)
    Q_PROPERTY(bool selected READ selected NOTIFY selectedChanged)

public:
    enum Presence {
        Unknown,
        Active,
        Away,
        Dnd,
        Deleted //for bots
    };
    Q_ENUM(Presence)

    User(QObject *parent = nullptr);
    User(const User& copy, QObject *parent = nullptr);
    User(const QString& id, const QString& name, QObject *parent = nullptr);

    void setData(const QJsonObject &data);

    void setPresence(const Presence presence);
    Presence presence();

    const QString &userId() { return m_userId; }
    QString username() const;
    QString fullName() const;
    QUrl avatarUrl() const;
    bool isBot() const;
    QString botId() const;
    bool selected() const;
    void setSelected(bool selected);

signals:
    void presenceChanged();
    void selectedChanged(bool selected);

private:
    QString m_userId;
    QString m_botId;
    QString m_appId;
    QString m_username;
    QString m_fullName;
    QUrl m_avatarUrl;
    bool m_isBot = false;
    QString m_statusEmoji;
    QString m_status;
    QString m_email;
    QColor m_color;
    Presence m_presence = Unknown;
    bool m_selected { false };
};

class UsersModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool selected READ selected NOTIFY selectedChanged)
public:
    enum Fields {
        UserObject,
        FieldsCount
    };

    UsersModel(QObject *parent = nullptr);
    ~UsersModel() override;

    int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const override { return m_users.count(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QMap<QString, QPointer<User> > users() const;

    bool usersFetched() const;
    void setUsersFetched(bool usersFetched);
    bool selected() const;

signals:
    void requestUserInfo(User* user);
    void selectedChanged(bool selected);

public slots:
    void addUser(User *user);
    void updateUser(const QJsonObject &userData);
    void addUser(const QJsonObject &userData);
    void addUsers(const QList<QPointer<User>> &users, bool last);
    QPointer<User> user(const QString &id);
    QStringList selectedUserIds();
    bool isSelected() const;
    void setSelected(int index);
    void clearSelections();

private:
    QMap<QString, QPointer<User>> m_users;
    QStringList m_userIds;
    bool m_addingUsers { false };
    bool m_usersFetched { false };
    bool m_selected;
    mutable QMutex m_modelMutex;
};

