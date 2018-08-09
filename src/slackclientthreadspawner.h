#ifndef THREADSPAWNER_H
#define THREADSPAWNER_H

#include <QThread>
#include <QMutex>
#include "slackclient.h"
#include "QQmlObjectListModel.h"

class SlackClientThreadSpawner : public QThread
{
    Q_OBJECT

    Q_PROPERTY(bool isDevice READ isDevice CONSTANT)
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY onlineChanged)
    Q_PROPERTY(QString lastTeam READ lastTeam WRITE setLastTeam NOTIFY lastTeamChanged)

public:
    explicit SlackClientThreadSpawner(QObject *parent = nullptr);
    virtual ~SlackClientThreadSpawner();

    QString lastTeam() const;
    bool isOnline();

    Q_INVOKABLE SlackTeamClient *slackClient(const QString& teamId);
    Q_INVOKABLE QQmlObjectListModel<TeamInfo>* teamsModel();
    Q_INVOKABLE MessageListModel *getSearchMessages(const QString &teamId);
    Q_INVOKABLE void reconnectClient();
    Q_INVOKABLE Chat *getChannel(const QString& teamId, const QString& channelId);
    Q_INVOKABLE void dumpChannel(const QString& teamId, const QString& channelId);

signals:
    void threadStarted();

    //signals from thread
    void testConnectionFail(const QString& teamId);
    void testLoginSuccess(QString userId, const QString& teamId, QString team);
    void testLoginFail(const QString& teamId);

    void accessTokenSuccess(QString userId, const QString& teamId, QString team);
    void accessTokenFail(const QString& teamId);

    void loadMessagesSuccess(const QString& teamId, QString channelId);
    void loadMessagesFail(const QString& teamId);

    void initFail(const QString& teamId, const QString &why);
    void initSuccess(const QString& teamId);

    void reconnectFail(const QString& teamId);
    void reconnectAccessTokenFail(const QString& teamId);

    void messageReceived(const QString& teamId, QVariantMap message);
    void messageUpdated(const QString& teamId, QVariantMap message);
    void channelCountersUpdated(const QString& teamId, const QString& channelId, int unread_messages);
    void channelJoined(const QString& teamId, const QString& channelId);
    void channelLeft(const QString& teamId, const QString& channelId);
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
    void searchResultsReady(const QString& query, int page, int pages);
    void userTyping(const QString& teamId, const QString& channelId, const QString& userName);

    void chatsModelChanged(const QString& teamId, ChatsModel* chatsModel);
    void searchStarted();

public slots:
    void startClient(const QString& teamId);
    void testLogin(const QString& teamId);

    void setAppActive(const QString& teamId, bool active);
    void setActiveWindow(const QString& teamId, const QString &windowId);

    QStringList getNickSuggestions(const QString& teamId, const QString &currentText, const int cursorPosition);

    bool isDevice() const;
    QString lastChannel(const QString& teamId);

    QString userName(const QString& teamId, const QString &userId);
    bool handleAccessTokenReply(const QJsonObject &bootData);

    //channels
    void markChannel(const QString& teamId, ChatsModel::ChatType type, const QString& channelId, const QDateTime &time = QDateTime());
    void joinChannel(const QString& teamId, const QString& channelId);
    void leaveChannel(const QString& teamId, const QString& channelId);
    void leaveGroup(const QString& teamId, const QString& groupId);
    QString getChannelName(const QString& teamId, const QString& channelId);
    int getTotalUnread(const QString& teamId, ChatsModel::ChatType type);

    //messages
    void searchMessages(const QString& teamId, const QString& searchString, int page = 0);
    void loadMessages(const QString& teamId, const QString& channelId);
    void postMessage(const QString& teamId, const QString& channelId, const QString& content, const QDateTime &thread_ts = QDateTime());
    void updateMessage(const QString& teamId, const QString& channelId, const QString& content, const QDateTime &ts);
    void deleteMessage(const QString& teamId, const QString& channelId, const QDateTime& ts);
    void postImage(const QString& teamId, const QString& channelId, const QString& imagePath, const QString& title, const QString& comment);
    void deleteReaction(const QString& teamId, const QString& channelId, const QDateTime& ts, const QString& reaction);
    void addReaction(const QString& teamId, const QString& channelId, const QDateTime& ts, const QString& reaction);

    //from slack thread to main thread
    void onMessageReceived(Message* message);
    void onMessageUpdated(Message* message);
    void onMessageDeleted(const QString& channelId, const QDateTime& ts);
    void onChannelUpdated(Chat *chat);
    void onChannelJoined(Chat *chat);
    void onChannelLeft(const QString &channelId);
    void onSearchMessagesReceived(const QJsonArray &messages, int total, const QString &query, int page, int pages);

    //chats
    void openChat(const QString& teamId, const QString& chatId);
    void closeChat(const QString& teamId, const QString& chatId);

    //teams
    void onTeamInfoChanged(const QString& teamId);
    void connectToTeam(const QString& teamId, const QString &accessToken = QString(""));
    void leaveTeam(const QString& teamId);

    void setLastTeam(const QString& lastTeam);
    void onOnlineChanged(const QString& teamId);

    void setMediaSource(QObject *mediaPlayer, const QString& teamId, const QString& url);
    QString teamToken(const QString& teamId);

    void onChatJoined(const QJsonObject &data);

    void onUsersDataChanged(const QList<QPointer<User> > &users, bool last);
    void onConversationsDataChanged(const QList<Chat *> &chats, bool last);
    void onConversationMembersChanged(const QString &channelId, const QStringList& members, bool last);

protected:
    void run();

private:
    SlackTeamClient *createNewClientInstance(const QString &teamId, const QString &accessToken = QString(""));

private:
    QQmlObjectListModel<TeamInfo> m_teamsModel;
    QMap<QString, SlackTeamClient*> m_knownTeams;
    QString m_lastTeam;
    bool m_isOnline { false };
};

#endif

