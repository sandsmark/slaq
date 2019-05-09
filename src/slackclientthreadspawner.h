#pragma once

#include <QThread>
#include <QMutex>
#include "slackclient.h"
#include "QQmlObjectListModel.h"
#include "networkaccessmanager.h"

class ThreadExecutor;

class SlackClientThreadSpawner : public QThread
{
    Q_OBJECT

    Q_PROPERTY(bool isDevice READ isDevice CONSTANT)
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY onlineChanged)
    Q_PROPERTY(QString lastTeam READ lastTeam WRITE setLastTeam NOTIFY lastTeamChanged)
    Q_PROPERTY(QDateTime buildTime MEMBER m_buildTime CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit SlackClientThreadSpawner(QObject *parent = nullptr);
    virtual ~SlackClientThreadSpawner();

    QString lastTeam() const;
    bool isOnline();

    Q_INVOKABLE SlackTeamClient *slackClient(const QString& teamId);
    Q_INVOKABLE SlackUser *selfUser(const QString& teamId);
    Q_INVOKABLE UsersModel *usersModel(const QString& teamId);
    Q_INVOKABLE ChatsModel *chatsModel(const QString& teamId);
    Q_INVOKABLE SlackTeamClient::ClientStatus slackClientStatus(const QString& teamId);
    Q_INVOKABLE QQmlObjectListModel<TeamInfo>* teamsModel();
    Q_INVOKABLE MessageListModel *getSearchMessages(const QString &teamId);
    Q_INVOKABLE FilesSharesModel *getFilesSharesModel(const QString &teamId);
    Q_INVOKABLE void reconnectClient();
    Q_INVOKABLE Chat *getChannel(const QString& teamId, const QString& channelId);
    Q_INVOKABLE void dumpChannel(const QString& teamId, const QString& channelId);
    Q_INVOKABLE QString resourceForFileType(const QString& fileType, const QString& fileName);

    QString version() const;
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
    void channelCountersUpdated(const QString& teamId, const QString& channelId, int unreadMessages, int unreadPersonalMessages);
    void channelJoined(const QString& teamId, const QString& channelId);
    void channelLeft(const QString& teamId, const QString& channelId);

    void postFileSuccess(const QString& teamId);
    void postFileFail(const QString& teamId);

    void networkOff(const QString& teamId);
    void networkOn(const QString& teamId);

    void reconnecting(const QString& teamId);
    void disconnected(const QString& teamId);
    void connected(const QString& teamId);
    void lastChannelChanged(const QString& teamId);
    void lastTeamChanged(QString lastTeam);
    void teamLeft(const QString& teamId);
    void onlineChanged(bool isOnline);
    void searchResultsReady(const QString& query, int page, int pages);
    void userTyping(const QString& teamId, const QString& channelId, const QString& userName);

    void chatsModelChanged(const QString& teamId, ChatsModel* chatsModel);
    void usersModelChanged(const QString& teamId, UsersModel* usersModel);
    void searchStarted();
    void error(const QJsonObject& err);

public slots:
    // Slack login procedure
    void loginAttempt(const QString& email, const QString& password, const QString& teamName);
    void getSessionDetails(const QString &url);

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
    void createChat(const QString& teamId, const QString &channelName, bool isPrivate);
    void markChannel(const QString& teamId, ChatsModel::ChatType type, const QString& channelId, const QDateTime &time = QDateTime());
    void joinChannel(const QString& teamId, const QString& channelId);
    void leaveChannel(const QString& teamId, const QString& channelId);
    void archiveChannel(const QString& teamId, const QString& channelId);
    void leaveGroup(const QString& teamId, const QString& groupId);
    QString getChannelName(const QString& teamId, const QString& channelId);
    int getTotalUnread(const QString& teamId, ChatsModel::ChatType type, bool personal);
    Chat* getGeneralChannel(const QString& teamId);

    //messages
    void searchMessages(const QString& teamId, const QString& searchString, int page = 0);
    void loadMessages(const QString& teamId, const QString& channelId);
    void postMessage(const QString& teamId, const QString& channelId, const QString& content, const QString &thread_ts = "");
    void updateMessage(const QString& teamId, const QString& channelId, const QString& content,
                       const QDateTime &ts, const QString& slackTs);
    void deleteMessage(const QString& teamId, const QString& channelId, const QDateTime& ts, const QString& slackTs);
    void postFile(const QString& teamId, const QString& channelId, const QString& filePath, const QString& title, const QString& comment);
    void deleteReaction(const QString& teamId, const QString& channelId, const QDateTime& ts, const QString& reaction);
    void addReaction(const QString& teamId, const QString& channelId,
                     const QDateTime& ts, const QString& reaction, const QString& slackTs);

    //from slack thread to main thread
    void onMessageReceived(Message* message);
    void onMessageUpdated(Message* message, bool replace = true);
    void onMessageDeleted(const QString& channelId, quint64 ts);
    void onChannelUpdated(Chat *chat);
    void onChannelJoined(Chat *chat);
    void onChannelLeft(const QString &channelId);
    void onSearchMessagesReceived(const QJsonArray &messages, int total, const QString &query, int page, int pages);

    //chats
    void openChat(const QString& teamId, const QStringList &userIds, const QString &channelId = QString());
    void closeChat(const QString& teamId, const QString& chatId);

    //teams
    void onTeamInfoChanged(const QString& teamId);
    void connectToTeam(const QString& teamId, const QString &accessToken = QString(""));
    void leaveTeam(const QString& teamId);
    void setLastTeam(const QString& lastTeam);
    void appendTeam(const QString& teamId);

    void onOnlineChanged(const QString& teamId);

    void setMediaSource(QObject *mediaPlayer, const QString& teamId, const QString& url);
    QString teamToken(const QString& teamId);

    void onUsersDataChanged(const QList<QPointer<SlackUser> > &users, bool last);
    void onConversationsDataChanged(const QList<Chat *> &chats, bool last);
    void onConversationMembersChanged(const QString &channelId, const QStringList& members, bool last);
    void onUsersPresenceChanged(const QStringList &users, const QString &presence,
                                const QDateTime& snoozeEnds = QDateTime(), bool force = false);
    void onMessagesReceived(const QString &channelId, const QList<Message *> &messages, bool hasMore, const QString &threadTs);

    void sendUserTyping(const QString& teamId, const QString& channelId);
    void updateUserInfo(const QString& teamId, SlackUser *user);
    void updateUserAvatar(const QString& teamId, const QString &filePath, int cropSide = 0, int cropX = 0, int cropY = 0);

    void setPresence(const QString& teamId, bool isAway);
    void setDnD(const QString& teamId, int minutes);
    void cancelDnD(const QString& teamId);
    void onFileSharesReceived(const QList<FileShare*>& shares, int total, int page, int pages);
    void deleteFile(const QString& teamId, const QString& fileId);
    void clearSettingsAndRestartApp(bool clear);
    void showError(const QString& errorDomain, const QString& errorMessage,
                   const QString& errorDetails = "", int timeout = 0);

protected:
    void run() override;

private:
    SlackTeamClient *createNewClientInstance(const QString &teamId, const QString &accessToken = QString(""));
    bool checkForPersonal(const QString &msg, SlackUser *selfUser);

private:
    QQmlObjectListModel<TeamInfo> m_teamsModel;
    QMap<QString, SlackTeamClient*> m_knownTeams;
    QString m_lastTeam;
    bool m_isOnline { false };
    QDateTime m_buildTime;
    ThreadExecutor* m_threadExecutor {nullptr};
    QSettings m_settings;
    QDir m_fileTypesResDir { ":/icons/filetypes" };
    NetworkAccessManager m_qnam;
};

template<typename Callable>
void executeInLoop(Callable &&callable, QObject *object = QCoreApplication::instance())
{
    if (QThread *thread = qobject_cast<QThread *>(object))
        object = QAbstractEventDispatcher::instance(thread);

    QMetaObject::invokeMethod(object, std::forward<Callable>(callable));
    //example:
    //       executeInLoop([&] { clangPchManagerServer.setProgress(progress, total); });
}

class ThreadExecutor: public QObject {
    Q_OBJECT

public:
    explicit ThreadExecutor(SlackClientThreadSpawner* threadSpawner, QObject *parent = nullptr) :
        QObject(parent), m_threadSpawner(threadSpawner) {}
     ~ThreadExecutor() override = default;

public slots:
    void connectToTeam(const QString& teamId, const QString &accessToken = QString(""));
private:
    SlackClientThreadSpawner* m_threadSpawner {nullptr};
};

