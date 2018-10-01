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
    Q_PROPERTY(QString fullName READ fullName NOTIFY fullNameChanged)
    Q_PROPERTY(QString firstName READ firstName WRITE setFirstName NOTIFY firstNameChanged)
    Q_PROPERTY(QString lastName READ lastName WRITE setLastName NOTIFY lastNameChanged)
    Q_PROPERTY(QString statusEmoji READ statusEmoji WRITE setStatusEmoji NOTIFY statusEmojiChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QColor color MEMBER m_color CONSTANT)
    Q_PROPERTY(QUrl avatarUrl READ avatarUrl WRITE setAvatarUrl NOTIFY avatarChanged)
    Q_PROPERTY(bool isBot MEMBER m_isBot CONSTANT)
    Q_PROPERTY(Presence presence READ presence NOTIFY presenceChanged)
    Q_PROPERTY(bool selected READ selected NOTIFY selectedChanged)
    Q_PROPERTY(QDateTime snoozeEnds READ snoozeEnds NOTIFY snoozeEndsChanged)

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
    void setUserId(const QString &userId);

    QString email() const;
    void setEmail(const QString &email);

    QString status() const;
    void setStatus(const QString &status);

    QString statusEmoji() const;
    void setStatusEmoji(const QString &statusEmoji);
    void setAvatarUrl(const QUrl &avatarUrl);
    QString firstName() const;
    QString lastName() const;
    QDateTime snoozeEnds() const;

    void setSnoozeEnds(const QDateTime &snoozeEnds);

public slots:
    void setFirstName(const QString& firstName);
    void setLastName(const QString& lastName);

signals:
    void presenceChanged();
    void selectedChanged(bool selected);
    void avatarChanged(QUrl avatarUrl);
    void emailChanged(QString email);
    void fullNameChanged(QString fullName);
    void firstNameChanged(QString firstName);
    void lastNameChanged(QString lastName);
    void statusEmojiChanged(QString statusEmoji);
    void statusChanged(QString status);
    void snoozeEndsChanged(QDateTime snoozeEnds);

private:
    QString m_userId;
    QString m_botId;
    QString m_appId;
    QString m_username;
    QString m_fullName;
    QString m_firstName;
    QString m_lastName;
    QUrl m_avatarUrl;
    bool m_isBot = false;
    QString m_statusEmoji;
    QString m_status;
    QString m_email;
    QColor m_color;
    Presence m_presence = Unknown;
    bool m_selected { false };
    QDateTime m_snoozeEnds;
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
    void createAndRequestInfo(const QString &id);

private:
    QMap<QString, QPointer<User>> m_users;
    QStringList m_userIds;
    bool m_addingUsers { false };
    bool m_usersFetched { false };
    bool m_selected;
    mutable QMutex m_modelMutex;
};

