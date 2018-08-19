#ifndef SLACKCLIENT_H
#define SLACKCLIENT_H

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
    void messagesReceived(const QString &channelId, QList<Message*> messages, bool hasMore, int threadMsgsCount);
    void searchMessagesReceived(const QJsonArray& matches, int total, const QString& query, int page, int pages);
    void messageUpdated(Message* message);
    void messageDeleted(const QString& channelId, const QDateTime& ts);

    void channelUpdated(Chat* chat);
    void channelJoined(Chat* chat);
    void channelLeft(const QString& channelId);
    void chatJoined(const QString& channelId);
    void chatLeft(const QString& channelId);

    void userUpdated(const QString& teamId, QVariantMap user);

    void postImageSuccess(const QString& teamId);
    void postImageFail(const QString& teamId);

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
    void usersPresenceChanged(const QList<QPointer<User>>& users, const QString& presence);

    void usersDataChanged(const QList<QPointer<User>>& users, bool last);
    void conversationsDataChanged(const QList<Chat*>& chats, bool last);
    void conversationMembersChanged(const QString &channelId, const QStringList& members, bool last);

public slots:
    void startConnections();
    void startClient();
    void testLogin();
    void searchMessages(const QString& searchString, int page =  1);
    void loadMessages(const QString& channelId, const QDateTime &latest = QDateTime());
    void deleteReaction(const QString &channelId, const QDateTime &ts, const QString &reaction);
    void addReaction(const QString &channelId, const QDateTime &ts, const QString &reaction);
    void postMessage(const QString& channelId, QString content, const QDateTime &thread_ts);
    void updateMessage(const QString& channelId, QString content, const QDateTime &ts);
    void deleteMessage(const QString& channelId, const QDateTime& ts);
    void postImage(const QString& channelId, const QString& imagePath, const QString& title, const QString& comment);
    void markChannel(ChatsModel::ChatType type, const QString& channelId, const QDateTime& time);
    void joinChannel(const QString& channelId);
    void leaveChannel(const QString& channelId);
    void leaveGroup(const QString& groupId);
    void openChat(const QString& chatId);
    void closeChat(const QString& chatId);

    void requestTeamInfo();
    void requestConversationsList(const QString& cursor);
    void requestConversationMembers(const QString& channelId, const QString& cursor);
    void requestUsersList(const QString& cursor);
    void requestTeamEmojis();
    void requestConversationInfo(const QString& channelId);

    QString getChannelName(const QString& channelId);
    Chat *getChannel(const QString& channelId);

    QString userName(const QString &userId);
    QString lastChannel();
    bool isOnline() const;
    bool isDevice() const;
    void onFetchMoreSearchData(const QString& query, int page);
    void parseUserDndChange(const QJsonObject &message);
    SlackTeamClient::ClientStatus getStatus() const;

private slots:
    void handleStartReply();
    void handleTestLoginReply();
    void handleLoadMessagesReply();
    void handleDeleteReactionReply();
    void handleAddReactionReply();
    void handlePostMessageReply();
    void handlePostImage();
    void handleMarkChannelReply();
    void handleJoinChannelReply();
    void handleLeaveChannelReply();
    void handleLeaveGroupReply();
    void handleOpenChatReply();
    void handleCloseChatReply();
    void handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);
    void handleStreamStart();
    void handleStreamEnd();
    void handleStreamMessage(const QJsonObject& message);
    void reconnectClient();
    void handleTeamInfoReply();
    void handleTeamEmojisReply();
    void handleSearchMessagesReply();
    void handleDeleteMessageReply();
    void handleConversationsListReply();
    void handleUsersListReply();
    void handleConversationMembersReply();
    void handleConversationInfoReply();

private:
    bool appActive;
    QString activeWindow;

    QNetworkReply *executePost(const QString& method, const QMap<QString, QString> &data);
    QNetworkReply *executePostWithFile(const QString& method, const QMap<QString, QString> &, QFile *file);

    QNetworkReply *executeGet(const QString& method, const QMap<QString, QString>& params = QMap<QString, QString>(), const QVariant &attribute = QVariant());

    bool isOk(const QNetworkReply *reply);
    bool isError(const QJsonObject &data);
    QJsonObject getResult(QNetworkReply *reply);

    void parseMessageUpdate(const QJsonObject& message);
    void parseReactionUpdate(const QJsonObject& message);
    void parseChannelUpdate(const QJsonObject& message);
    void parsePresenceChange(const QJsonObject& message);
    void parseNotification(const QJsonObject& message);

    void sendNotification(const QString& channelId, const QString& title, const QString& content);
    void clearNotifications();

    QString historyMethod(const ChatsModel::ChatType type);
    QString markMethod(ChatsModel::ChatType type);

    QPointer<QNetworkAccessManager> networkAccessManager;
    QPointer<SlackConfig> config;
    QPointer<SlackStream> stream;
    QPointer<QTimer> reconnectTimer;

    QNetworkAccessManager::NetworkAccessibility networkAccessible;

    TeamInfo m_teamInfo;
    ClientStates m_state { ClientStates::UNINITIALIZED };
    ClientStatus m_status { ClientStatus::UNDEFINED };
    QObject *m_spawner { nullptr };
};

QML_DECLARE_TYPE(SlackTeamClient)

#endif // SLACKCLIENT_H
