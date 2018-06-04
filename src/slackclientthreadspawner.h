#ifndef THREADSPAWNER_H
#define THREADSPAWNER_H

#include <QThread>
#include <QMutex>
#include "slackclient.h"

class SlackClientThreadSpawner : public QThread
{
    Q_OBJECT

    Q_PROPERTY(bool isDevice READ isDevice CONSTANT)
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY onlineChanged)
    Q_PROPERTY(QString lastTeam READ lastTeam WRITE setLastTeam NOTIFY lastTeamChanged)

public:
    explicit SlackClientThreadSpawner(QObject *parent = nullptr);

    virtual ~SlackClientThreadSpawner();
    Q_INVOKABLE SlackClient *slackClient(const QString& teamId);

    Q_INVOKABLE QQmlObjectListModel<TeamInfo>* teamsModel();

    QString lastTeam() const;

    bool isOnline();

signals:
    void threadStarted();

    //signals from thread
    void testConnectionFail(const QString& teamId);
    void testLoginSuccess(QString userId, const QString& teamId, QString team);
    void testLoginFail(const QString& teamId);

    void accessTokenSuccess(QString userId, const QString& teamId, QString team);
    void accessTokenFail(const QString& teamId);

    void loadMessagesSuccess(const QString& teamId, QString channelId, QVariantList messages);
    void loadMessagesFail(const QString& teamId);

    void initFail(const QString& teamId, const QString &why);
    void initSuccess(const QString& teamId);

    void reconnectFail(const QString& teamId);
    void reconnectAccessTokenFail(const QString& teamId);

    void messageReceived(const QString& teamId, QVariantMap message);
    void messageUpdated(const QString& teamId, QVariantMap message);
    void channelUpdated(const QString& teamId, QVariantMap channel);
    void channelJoined(const QString& teamId, QVariantMap channel);
    void channelLeft(const QString& teamId, QVariantMap channel);
    void userUpdated(const QString& teamId, QVariantMap user);

    void postImageSuccess(const QString& teamId);
    void postImageFail(const QString& teamId);

    void networkOff(const QString& teamId);
    void networkOn(const QString& teamId);

    void reconnecting(const QString& teamId);
    void disconnected(const QString& teamId);
    void connected(const QString& teamId);
    void lastChannelChanged(const QString& teamId);
    void lastTeamChanged(QString lastTeam);
    void onlineChanged(bool isOnline);

public slots:
    void startClient(const QString& teamId);
    void testLogin(const QString& teamId);

    void setAppActive(const QString& teamId, bool active);
    void setActiveWindow(const QString& teamId, const QString &windowId);

    QStringList getNickSuggestions(const QString& teamId, const QString &currentText, const int cursorPosition);

    bool isDevice() const;
    QString lastChannel(const QString& teamId);

    QUrl avatarUrl(const QString& teamId, const QString &userId);
    bool handleAccessTokenReply(const QJsonObject &bootData);

    //channels
    void markChannel(const QString& teamId, const QString& type, const QString& channelId, const QString& time);
    void joinChannel(const QString& teamId, const QString& channelId);
    void leaveChannel(const QString& teamId, const QString& channelId);
    void leaveGroup(const QString& teamId, const QString& groupId);
    QVariantList getChannels(const QString& teamId);
    QVariant getChannel(const QString& teamId, const QString& channelId);

    //messages
    void loadMessages(const QString& teamId, const QString& type, const QString& channelId);
    void postMessage(const QString& teamId, const QString& channelId, const QString& content);
    void postImage(const QString& teamId, const QString& channelId, const QString& imagePath, const QString& title, const QString& comment);
    void deleteReaction(const QString& teamId, const QString& channelId, const QString& ts, const QString& reaction);
    void addReaction(const QString& teamId, const QString& channelId, const QString& ts, const QString& reaction);

    //chats
    void openChat(const QString& teamId, const QString& chatId);
    void closeChat(const QString& teamId, const QString& chatId);

    //teams
    void onTeamInfoChanged(const QString& teamId);
    void connectToTeam(const QString& teamId, const QString &accessToken = QString(""));
    void leaveTeam(const QString& teamId);

    void setLastTeam(const QString& lastTeam);
    void onOnlineChanged(const QString& teamId);

protected:
    void run();

private:
    SlackClient *createNewClientInstance(const QString &teamId, const QString &accessToken = QString(""));

private:
    QQmlObjectListModel<TeamInfo> m_teamsModel;
    QHash<QString, SlackClient*> m_knownTeams;
    QString m_lastTeam;
    bool m_isOnline;
};

#endif

