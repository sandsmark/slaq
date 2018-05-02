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
#include "messageformatter.h"

class SlackClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isOnline READ isOnline NOTIFY isOnlineChanged)
    Q_PROPERTY(bool isDevice READ isDevice CONSTANT)
    Q_PROPERTY(QString lastChannel READ lastChannel WRITE setActiveWindow NOTIFY lastChannelChanged)

public:
    explicit SlackClient(QObject *parent = 0);
    virtual ~SlackClient();

    Q_INVOKABLE void setAppActive(bool active);
    Q_INVOKABLE void setActiveWindow(const QString &windowId);

    Q_INVOKABLE QVariantList getChannels();
    Q_INVOKABLE QVariant getChannel(const QString& channelId);

    Q_INVOKABLE QStringList getNickSuggestions(const QString &currentText, const int cursorPosition);

    bool isOnline() const;
    bool isDevice() const;

    QString lastChannel();

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
    void lastChannelChanged();

public slots:
    void start();
    void handleStartReply();

    void fetchAccessToken(const QUrl& url);
    void handleAccessTokenReply();

    void testLogin();
    void handleTestLoginReply();

    void loadMessages(const QString& type, const QString& channelId);
    void handleLoadMessagesReply();

    void postMessage(const QString& channelId, QString content);
    void handlePostMessageReply();

    void postImage(const QString& channelId, const QString& imagePath, const QString& title, const QString& comment);
    void handlePostImage();

    void markChannel(const QString& type, const QString& channelId, const QString& time);
    void handleMarkChannelReply();

    void joinChannel(const QString& channelId);
    void handleJoinChannelReply();

    void leaveChannel(const QString& channelId);
    void handleLeaveChannelReply();

    void leaveGroup(const QString& groupId);
    void handleLeaveGroupReply();

    void openChat(const QString& chatId);
    void handleOpenChatReply();

    void closeChat(const QString& chatId);
    void handleCloseChatReply();

    void handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);

    void handleStreamStart();
    void handleStreamEnd();
    void handleStreamMessage(const QJsonObject& message);

    void reconnect();

    QUrl avatarUrl(const QString &userId) { return m_userAvatars.value(userId); }

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
    void parseChannelUpdate(const QJsonObject& message);
    void parseChannelJoin(const QJsonObject& message);
    void parseChannelLeft(const QJsonObject& message);
    void parseGroupJoin(const QJsonObject& message);
    void parseChatOpen(const QJsonObject& message);
    void parseChatClose(const QJsonObject& message);
    void parsePresenceChange(const QJsonObject& message);
    void parseNotification(const QJsonObject& message);

    QVariantMap getMessageData(const QJsonObject& message);

    QString getContent(const QJsonObject& message);
    QVariantList getAttachments(const QJsonObject& message);
    QVariantList getImages(const QJsonObject& message);
    QString getAttachmentColor(const QJsonObject& attachment);
    QVariantList getAttachmentFields(const QJsonObject& attachment);
    QVariantList getAttachmentImages(const QJsonObject& attachment);

    QVariantMap parseChannel(const QJsonObject& data);
    QVariantMap parseGroup(const QJsonObject& group);

    void parseUsers(const QJsonObject& data);
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

    QNetworkAccessManager::NetworkAccessibility networkAccessible;
    QHash<QString, QUrl> m_userAvatars;

    QString m_clientId;
    QString m_clientId2;

    QString m_lastChannel;
    MessageFormatter m_formatter;
};

#endif // SLACKCLIENT_H
