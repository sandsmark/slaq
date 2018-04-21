#ifndef SLACKCLIENT_H
#define SLACKCLIENT_H

#include <QObject>
#include <QPointer>
#include <QImage>
#include <QFile>
#include <QJsonObject>
#include <QUrl>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "slackconfig.h"
#include "slackstream.h"

class SlackClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isOnline READ isOnline NOTIFY isOnlineChanged)
    Q_PROPERTY(bool isDevice READ isDevice CONSTANT)

public:
    explicit SlackClient(QObject *parent = 0);

    Q_INVOKABLE void setAppActive(bool active);
    Q_INVOKABLE void setActiveWindow(QString windowId);

    Q_INVOKABLE QVariantList getChannels();
    Q_INVOKABLE QVariant getChannel(QString channelId);

    bool isOnline() const;
    bool isDevice() const;

    Q_INVOKABLE QString lastChannel() const;

signals:
    void testConnectionFail();
    void testLoginSuccess(QString userId, QString teamId, QString team);
    void testLoginFail();

    void accessTokenSuccess(QString userId, QString teamId, QString team);
    void accessTokenFail();

    void loadMessagesSuccess(QString channelId, QVariantList messages);
    void loadMessagesFail();

    void initFail(const QString &why);
    void initSuccess();

    void reconnectFail();
    void reconnectAccessTokenFail();

    void messageReceived(QVariantMap message);
    void channelUpdated(QVariantMap channel);
    void channelJoined(QVariantMap channel);
    void channelLeft(QVariantMap channel);
    void userUpdated(QVariantMap user);

    void postImageSuccess();
    void postImageFail();

    void networkOff();
    void networkOn();

    void reconnecting();
    void disconnected();
    void connected();

    void isOnlineChanged();

public slots:
    void start();
    void handleStartReply();

    void fetchAccessToken(QUrl url);
    void handleAccessTokenReply();

    void testLogin();
    void handleTestLoginReply();

    void loadMessages(QString type, QString channelId);
    void handleLoadMessagesReply();

    void postMessage(QString channelId, QString content);
    void handlePostMessageReply();

    void postImage(QString channelId, QString imagePath, QString title, QString comment);
    void handlePostImage();

    void markChannel(QString type, QString channelId, QString time);
    void handleMarkChannelReply();

    void joinChannel(QString channelId);
    void handleJoinChannelReply();

    void leaveChannel(QString channelId);
    void handleLeaveChannelReply();

    void leaveGroup(QString groupId);
    void handleLeaveGroupReply();

    void openChat(QString chatId);
    void handleOpenChatReply();

    void closeChat(QString chatId);
    void handleCloseChatReply();

    void handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);

    void handleStreamStart();
    void handleStreamEnd();
    void handleStreamMessage(QJsonObject message);

    void reconnect();

    QUrl avatarUrl(const QString &userId) { return m_userAvatars.value(userId); }

private:
    bool appActive;
    QString activeWindow;

    QNetworkReply *executePost(QString method, const QMap<QString, QString> &data);
    QNetworkReply *executePostWithFile(QString method, const QMap<QString, QString> &, QFile *file);

    QNetworkReply *executeGet(QString method, QMap<QString, QString> params = QMap<QString, QString>());

    bool isOk(const QNetworkReply *reply);
    bool isError(const QJsonObject &data);
    QJsonObject getResult(QNetworkReply *reply);

    void parseMessageUpdate(QJsonObject message);
    void parseChannelUpdate(QJsonObject message);
    void parseChannelJoin(QJsonObject message);
    void parseChannelLeft(QJsonObject message);
    void parseGroupJoin(QJsonObject message);
    void parseChatOpen(QJsonObject message);
    void parseChatClose(QJsonObject message);
    void parsePresenceChange(QJsonObject message);
    void parseNotification(QJsonObject message);

    QVariantMap getMessageData(const QJsonObject message);

    QString getContent(QJsonObject message);
    QVariantList getAttachments(QJsonObject message);
    QVariantList getImages(QJsonObject message);
    QString getAttachmentColor(QJsonObject attachment);
    QVariantList getAttachmentFields(QJsonObject attachment);
    QVariantList getAttachmentImages(QJsonObject attachment);

    QVariantMap parseChannel(QJsonObject data);
    QVariantMap parseGroup(QJsonObject group);

    void parseUsers(QJsonObject data);
    void parseBots(QJsonObject data);
    void parseChannels(QJsonObject data);
    void parseGroups(QJsonObject data);
    void parseChats(QJsonObject data);

    void findNewUsers(const QString &message);

    void sendNotification(QString channelId, QString title, QString content);
    void clearNotifications();

    QVariantMap user(const QJsonObject &data);

    QString historyMethod(QString type);
    QString markMethod(QString type);

    QPointer<QNetworkAccessManager> networkAccessManager;
    QPointer<SlackConfig> config;
    QPointer<SlackStream> stream;
    QPointer<QTimer> reconnectTimer;

    QNetworkAccessManager::NetworkAccessibility networkAccessible;
    QHash<QString, QUrl> m_userAvatars;

    QString m_clientId;
    QString m_clientId2;
};

#endif // SLACKCLIENT_H
