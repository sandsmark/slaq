#pragma once

#include <QString>
#include <QObject>
#include <QtQml>
#include "UsersModel.h"
#include "ChatsModel.h"
#include "searchmessagesmodel.h"
#include "FilesSharesModel.h"

class SearchMessagesModel;
class SlackTeamClient;

class TeamInfo: public QObject {
    Q_OBJECT

    Q_PROPERTY(QString teamId READ teamId WRITE setTeamId NOTIFY teamIdChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString domain READ domain WRITE setDomain NOTIFY domainChanged)
    Q_PROPERTY(QString emailDomain READ emailDomain WRITE setEmailDomain NOTIFY emailDomainChanged)
    Q_PROPERTY(QStringList icons READ icons WRITE setIcons NOTIFY iconsChanged)
    Q_PROPERTY(bool imageDefault READ imageDefault WRITE setImageDefault NOTIFY imageDefaultChanged)
    Q_PROPERTY(QString enterpriseId READ enterpriseId WRITE setEnterpriseId NOTIFY enterpriseIdChanged)
    Q_PROPERTY(QString enterpriseName READ enterpriseName WRITE setEnterpriseName NOTIFY enterpriseNameChanged)
    Q_PROPERTY(QString teamToken READ teamToken WRITE setTeamToken NOTIFY teamTokenChanged)
    Q_PROPERTY(QString lastChannel READ lastChannel WRITE setLastChannel NOTIFY lastChannelChanged)
    Q_PROPERTY(QString selfId READ selfId CONSTANT)
    Q_PROPERTY(User* selfUser READ selfUser CONSTANT)
    Q_PROPERTY(ChatsModel* chats READ chats NOTIFY chatsChanged)
    Q_PROPERTY(UsersModel* users READ users NOTIFY usersChanged)

public:
    explicit TeamInfo(QObject *parent = nullptr);

    bool operator==(const TeamInfo &rhs) const {
        return rhs.m_teamId == m_teamId;
    }

    bool operator!=(const TeamInfo &rhs) const {
        return rhs.m_teamId != m_teamId;
    }

    QString teamId() const { return m_teamId; }
    QString name() const { return m_name; }
    QString domain() const { return m_domain; }
    QStringList icons() const { return m_icons; }
    QString teamToken() const { return m_teamToken; }
    QString emailDomain() const { return m_emailDomain; }
    bool imageDefault() const { return m_imageDefault; }
    QString enterpriseId() const { return m_enterpriseId; }
    QString enterpriseName() const { return m_enterpriseName; }
    QString lastChannel() const { return m_lastChannel; }
    ChatsModel *chats() const;
    UsersModel* users() const;
    SearchMessagesModel *searches() const;

    void parseTeamInfoData(const QJsonObject &teamObj);
    void parseSelfData(const QJsonObject &selfObj);

    QString selfId() const;

    bool teamsEmojisUpdated() const;
    void setTeamsEmojisUpdated(bool teamsEmojisUpdated);

    void createModels(SlackTeamClient *slackClient);
    User* selfUser() const;
    FilesSharesModel *fileSharesModel() const;

public slots:
    void setTeamId(const QString& teamId);
    void setName(const QString& name);
    void setDomain(const QString& domain);
    void setEmailDomain(const QString& emailDomain);
    void setIcons(const QStringList& icons);
    void setImageDefault(bool imageDefault);
    void setEnterpriseId(const QString& enterpriseId);
    void setEnterpriseName(const QString& enterpriseName);
    void setTeamToken(const QString& teamToken);
    void setLastChannel(const QString& lastChannel);
    void addUsersData(const QList<QPointer<User> > &users, bool last);
    void addConversationsData(const QList<Chat *> &chats, bool last);

signals:
    void teamIdChanged(QString teamId);
    void nameChanged(QString name);
    void domainChanged(QString domain);
    void emailDomainChanged(QString emailDomain);
    void iconsChanged(QStringList icons);
    void imageDefaultChanged(bool imageDefault);
    void enterpriseIdChanged(QString enterpriseId);
    void enterpriseNameChanged(QString enterpriseName);
    void teamTokenChanged(QString teamToken);
    void lastChannelChanged(QString lastChannel);
    void chatsChanged(ChatsModel* chats);
    void usersChanged(UsersModel* users);

private:
    QString m_name;
    QString m_domain;
    QString m_emailDomain;
    QStringList m_icons;
    QString m_token;
    QString m_teamId;
    bool m_imageDefault { false };
    QString m_enterpriseId;
    QString m_enterpriseName;
    QString m_teamToken;
    QString m_lastChannel;
    QString m_selfId;

    ChatsModel* m_chats{};
    UsersModel* m_users{};
    SearchMessagesModel* m_searchMessages{};
    FilesSharesModel* m_fileSharesModel{};
    bool m_teamsEmojisUpdated{};
};

QML_DECLARE_TYPE(TeamInfo)

