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
#include "QQmlObjectListModel.h"
#include "storage.h"
#include "debugblock.h"

class SlackClient : public QObject
{
    Q_OBJECT

public:
    explicit SlackClient(const QString& teamId, const QString &accessToken = QString(""), QObject *parent = nullptr);
    virtual ~SlackClient();

    Q_INVOKABLE void setAppActive(bool active);
    Q_INVOKABLE void setActiveWindow(const QString &windowId);

    Q_INVOKABLE QVariantList getChannels();
    Q_INVOKABLE QVariant getChannel(const QString& channelId);
    Q_INVOKABLE QStringList getNickSuggestions(const QString &currentText, const int cursorPosition);
    Q_INVOKABLE TeamInfo *teamInfo();

    enum ClientStates {
        UNINITIALIZED = -1,
        DISCONNECTED = 0,
        CONNECTING,
        CONNECTED,
        RECONNECTING
    };

    ClientStates getState() const;
    void setState(ClientStates state);

signals:
    void testConnectionFail(const QString& teamId);
    void testLoginSuccess(QString userId, QString teamId, QString team);
    void testLoginFail(const QString& teamId);

    void accessTokenSuccess(QString userId, QString teamId, QString team);
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

    void isOnlineChanged(const QString& teamId);
    void lastChannelChanged(const QString& teamId);

    void teamInfoChanged(const QString& teamId);
    void stateChanged(const QString& teamId);
    void searchResultsReady(const QString& teamId, const QVariantList& messages);

public slots:

    void startConnections();
    void startClient();
    void testLogin();
    void searchMessages(const QString& searchString);
    void loadMessages(const QString& type, const QString& channelId);
    void deleteReaction(const QString &channelId, const QString &ts, const QString &reaction);
    void addReaction(const QString &channelId, const QString &ts, const QString &reaction);
    void postMessage(const QString& channelId, QString content);
    void postImage(const QString& channelId, const QString& imagePath, const QString& title, const QString& comment);
    void markChannel(const QString& type, const QString& channelId, const QString& time);
    void joinChannel(const QString& channelId);
    void leaveChannel(const QString& channelId);
    void leaveGroup(const QString& groupId);
    void openChat(const QString& chatId);
    void closeChat(const QString& chatId);
    void requestTeamInfo();

    QUrl avatarUrl(const QString &userId) { return m_userAvatars.value(userId); }
    QString lastChannel();
    bool isOnline() const;

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
    void handleSearchMessagesReply();

private:
    bool appActive;
    QString activeWindow;

    QNetworkReply *executePost(const QString& method, const QMap<QString, QString> &data);
    QNetworkReply *executePostWithFile(const QString& method, const QMap<QString, QString> &, QFile *file);

    QNetworkReply *executeGet(const QString& method, const QMap<QString, QString>& params = QMap<QString, QString>());

    bool isOk(const QNetworkReply *reply);
    bool isError(const QJsonObject &data);
    QJsonObject getResult(QNetworkReply *reply);

    void parseMessageUpdate(const QJsonObject& message);
    void parseReactionUpdate(const QJsonObject& message);
    void parseChannelUpdate(const QJsonObject& message);
    void parseChannelJoin(const QJsonObject& message);
    void parseChannelLeft(const QJsonObject& message);
    void parseGroupJoin(const QJsonObject& message);
    void parseChatOpen(const QJsonObject& message);
    void parseChatClose(const QJsonObject& message);
    void parsePresenceChange(const QJsonObject& message);
    void parseNotification(const QJsonObject& message);

    QVariantMap getMessageData(const QJsonObject& message, const QString& teamId);

    QString getContent(const QJsonObject& message);
    QVariantList getAttachments(const QJsonObject& message);
    QVariantList getImages(const QJsonObject& message);
    QString getAttachmentColor(const QJsonObject& attachment);
    QVariantList getAttachmentFields(const QJsonObject& attachment);
    QVariantList getAttachmentImages(const QJsonObject& attachment);

    QVariantList getReactions(const QJsonObject& message);

    QVariantMap parseChannel(const QJsonObject& data);
    QVariantMap parseGroup(const QJsonObject& group);

    void parseUsers(const QJsonObject& data);
    void parseUser(const QJsonObject &user);
    void parseBots(const QJsonObject& data);
    void parseChannels(const QJsonObject& data);
    void parseGroups(const QJsonObject& data);
    void parseChats(const QJsonObject& data);

    void findNewUsers(const QString &message);

    void sendNotification(const QString& channelId, const QString& title, const QString& content);
    void clearNotifications();

    QVariantMap user(const QJsonObject &data);

    QString historyMethod(const QString& type);
    QString markMethod(const QString& type);

    QPointer<QNetworkAccessManager> networkAccessManager;
    QPointer<SlackConfig> config;
    QPointer<SlackStream> stream;
    QPointer<QTimer> reconnectTimer;
    Storage m_storage;

    QNetworkAccessManager::NetworkAccessibility networkAccessible;
    QHash<QString, QUrl> m_userAvatars;

    QString m_clientId;
    QString m_clientId2;

    MessageFormatter m_formatter;
    TeamInfo m_teamInfo;
    ClientStates m_state { ClientStates::UNINITIALIZED };
    NetworksModel *m_networksModel;
    QString m_networkId;
};

QML_DECLARE_TYPE(SlackClient)

#endif // SLACKCLIENT_H
