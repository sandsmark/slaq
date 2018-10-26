#pragma once

#include <QObject>
#include <QPointer>
#include <QImage>
#include <QFile>
#include <QJsonObject>
#include <QUrl>
#include <QTimer>
#include <QThread>
#include <QtQml>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "storage.h"
#include "slackconfig.h"
#include "slackstream.h"
#include "messageformatter.h"
#include "teaminfo.h"
#include "storage.h"

class SlackTeamClient : public QObject
{
    Q_OBJECT


public:
    explicit SlackTeamClient(QObject *spawner, const QString& teamId, const QString &accessToken = QString(""), QObject *parent = nullptr);
    virtual ~SlackTeamClient();

    Q_INVOKABLE void setAppActive(bool active);
    Q_INVOKABLE void setActiveWindow(const QString &windowId);

    Q_INVOKABLE QStringList getNickSuggestions(const QString &currentText, const int cursorPosition);
    Q_INVOKABLE TeamInfo *teamInfo();

    enum ClientStates {
        UNINITIALIZED = -1,
        DISCONNECTED = 0,
        CONNECTING,
        CONNECTED,
        RECONNECTING
    };
    Q_ENUM(ClientStates)

    Q_INVOKABLE ClientStates getState() const;
    void setState(ClientStates state);

    enum ClientStatus {
        UNDEFINED = -1,
        STARTED = 0,
        LOGGEDIN,
        LOGINFAILED,
        INITED
    };
    Q_ENUM(ClientStatus)

    Q_INVOKABLE ChatsModel *currentChatsModel();

signals:
    void testConnectionFail(const QString& teamId);
    void testLoginSuccess(QString userId, QString teamId, QString team);
    void testLoginFail(const QString& teamId);

    void accessTokenSuccess(QString userId, QString teamId, QString team);
    void accessTokenFail(const QString& teamId);

    void loadMessagesSuccess(const QString& teamId, QString channelId);
    void loadMessagesFail(const QString& teamId);

    void initFail(const QString& teamId, const QString &why);
    void initSuccess(const QString& teamId);

    void reconnectFail(const QString& teamId);
    void reconnectAccessTokenFail(const QString& teamId);

    // signals to main thread
    void messageReceived(Message* message);
    void messagesReceived(const QString &channelId, const QList<Message*>& messages, bool hasMore, int threadMsgsCount);
    void searchMessagesReceived(const QJsonArray& matches, int total, const QString& query, int page, int pages);
    void messageUpdated(Message* message, bool replace = true);
    void messageDeleted(const QString& channelId, const QDateTime& ts);
    void error(QJsonObject err);
    void fileSharesReceived(const QList<FileShare*>& shares, int total, int page, int pages);

    void channelUpdated(Chat* chat);
    void channelJoined(Chat* chat);
    void channelLeft(const QString& channelId);
    void chatJoined(const QString& channelId);
    void chatLeft(const QString& channelId);

    void postFileSuccess(const QString& teamId);
    void postFileFail(const QString& teamId);

    void networkOff(const QString& teamId);
    void networkOn(const QString& teamId);

    void reconnecting(const QString& teamId);
    void disconnected(const QString& teamId);
    void connected(const QString& teamId);

    void isOnlineChanged(const QString& teamId);
    void lastChannelChanged(const QString& teamId);

    void teamInfoChanged(const QString& teamId);
    void stateChanged(const QString& teamId);

    void userTyping(const QString& teamId, const QString& channelId, const QString& userName);
    void usersPresenceChanged(const QStringList& users, const QString& presence, const QDateTime& snoozeEnds = QDateTime(), bool force = false);

    void usersDataChanged(const QList<QPointer<User>>& users, bool last);
    void conversationsDataChanged(const QList<Chat*>& chats, bool last);
    void conversationMembersChanged(const QString &channelId, const QStringList& members, bool last);

public slots:
    void startConnections();
    void startClient();
    void testLogin();

    void deleteReaction(const QString &channelId, const QDateTime &ts, const QString &reaction);
    void addReaction(const QString &channelId, const QDateTime &ts, const QString &reaction);

    void searchMessages(const QString& searchString, int page =  1);
    void loadMessages(const QString& channelId, const QDateTime &latest = QDateTime());
    void postMessage(const QString& channelId, QString content, const QDateTime &thread_ts);
    void updateMessage(const QString& channelId, QString content, const QDateTime &ts, const QString& slackTs);
    void deleteMessage(const QString& channelId, const QDateTime& ts);

    void postFile(const QString& channelId, const QString& filePath, const QString& title, const QString& comment);
    void deleteFile(const QString& fileId);

    void createChat(const QString &channelName, bool isPrivate);
    void markChannel(ChatsModel::ChatType type, const QString& channelId, const QDateTime& time);
    void joinChannel(const QString& channelId);
    void leaveChannel(const QString& channelId);
    void archiveChannel(const QString& channelId);
    void openChat(const QStringList &userIds, const QString &channelId = QString());
    void closeChat(const QString& chatId);
    QString getChannelName(const QString& channelId);
    Chat *getChannel(const QString& channelId);

    void requestTeamInfo();
    void requestConversationsList(const QString& cursor);
    void requestConversationMembers(const QString& channelId, const QString& cursor);
    void requestUsersList(const QString& cursor);
    void requestTeamEmojis();
    void requestConversationInfo(const QString& channelId);
    void requestUserInfo(User* user);
    void requestUserInfoById(const QString& userId);
    void updateUserInfo(User* user);
    void updateUserAvatar(const QString &filePath, int cropSide = 0, int cropX = 0, int cropY = 0);
    void setPresence(bool isAway);
    void setDnD(int minutes);
    void cancelDnD();

    QString userName(const QString &userId);
    QString lastChannel();
    bool isOnline() const;
    bool isDevice() const;
    void onFetchMoreSearchData(const QString& query, int page);
    void parseUserDndChange(const QJsonObject &message);
    SlackTeamClient::ClientStatus getStatus() const;
    void sendUserTyping(const QString& channelId);
    void requestSharedFiles(int page, const QString &channelId = QString(), const QString &userId = QString());
    void requestSharedFileInfo(const QString &fileId);

private slots:
    void handleStartReply();
    void handleTestLoginReply();
    void handleLoadMessagesReply();
    void handleCommonReply();
    void handlePostFile();
    void handleCreateChatReply();
    void handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);
    void handleStreamStart();
    void handleStreamEnd();

    /**
     * @brief handleStreamMessage
     * @param message Json object with message body
     *
     * Handles all RTM messages for Slack Team
     */
    void handleStreamMessage(const QJsonObject& message);
    void reconnectClient();
    void handleTeamInfoReply();
    void handleTeamEmojisReply();
    void handleSearchMessagesReply();
    void handleConversationsListReply();
    void handleUsersListReply();
    void handleConversationMembersReply();
    void handleConversationInfoReply();
    void handleUsersInfoReply();
    void handleDnDInfoReply();

    void createChannelIfNeeded(const QJsonObject &channel);
    void handleTeamFilesReply();

private:
    bool appActive;
    QString activeWindow;

    QNetworkReply *executePost(const QString& method, const QMap<QString, QString> &data);
    QNetworkReply *executePostWithFile(const QString& method, const QMap<QString, QString> &,
                                       QFile *file, const QString &fileFormData = QString());
    QNetworkReply *executePost(const QString &method, const QByteArray &data);
    QNetworkReply *executeGet(const QString& method, const QMap<QString,
                              QString>& params = QMap<QString, QString>(),
                              const QVariant &attribute = QVariant());

    bool isOk(const QNetworkReply *reply);
    bool isError(QJsonObject &data);
    QJsonObject getResult(QNetworkReply *reply);

    void parseMessageUpdate(const QJsonObject& message);
    void parseReactionUpdate(const QJsonObject& message);
    void parseChannelUpdate(const QJsonObject& message);
    void parsePresenceChange(const QJsonObject& message, bool force = false);
    void parseNotification(const QJsonObject& message);

    void sendNotification(const QString& channelId, const QString& title, const QString& content);
    void clearNotifications();

    QString historyMethod(const ChatsModel::ChatType type);
    QString markMethod(ChatsModel::ChatType type);
    void addTeamEmoji(const QString &name, const QString &url);
    void requestDnDInfo(const QString &userId);

private:
    QPointer<QNetworkAccessManager> networkAccessManager;
    QPointer<SlackConfig> config;
    QPointer<SlackStream> stream;
    QPointer<QTimer> reconnectTimer;

    QNetworkAccessManager::NetworkAccessibility networkAccessible;

    TeamInfo m_teamInfo;
    ClientStates m_state { ClientStates::UNINITIALIZED };
    ClientStatus m_status { ClientStatus::UNDEFINED };
    QObject *m_spawner { nullptr };

    static const QMap<QString, QString> kSlackErrors;

};

QML_DECLARE_TYPE(SlackTeamClient)
