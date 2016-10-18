#ifndef SLACKCLIENT_H
#define SLACKCLIENT_H

#include <QObject>
#include <QPointer>
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
public:
    explicit SlackClient(QObject *parent = 0);

    Q_INVOKABLE QVariantList getChannels();
    Q_INVOKABLE QVariant getChannel(QString channelId);

signals:
    void testConnectionFail();
    void testLoginSuccess(QString userId, QString teamId, QString team);
    void testLoginFail();

    void accessTokenSuccess(QString userId, QString teamId, QString team);
    void accessTokenFail();

    void loadMessagesSuccess(QString channelId, QVariantList messages);
    void loadMessagesFail();

    void initFail();
    void initSuccess();

    void reconnectFail();
    void reconnectAccessTokenFail();

    void messageReceived(QVariantMap message);
    void channelUpdated(QVariantMap channel);
    void channelJoined(QVariantMap channel);
    void userUpdated(QVariantMap user);

    void networkOff();
    void networkOn();

    void reconnecting();
    void disconnected();
    void connected();

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

    void markChannel(QString type, QString channelId, QString time);
    void handleMarkChannelReply();

    void joinChannel(QString channelId);
    void handleJoinChannelReply();

    void handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);


    void handleStreamStart();
    void handleStreamEnd();
    void handleStreamMessage(QJsonObject message);

    void reconnect();

private:
    QNetworkReply* executePost(QString method, QMap<QString,QString> data);
    QNetworkReply* executeGet(QString method, QMap<QString,QString> params = QMap<QString,QString>());

    bool isOk(const QNetworkReply *reply);
    bool isError(const QJsonObject &data);
    QJsonObject getResult(QNetworkReply *reply);

    void parseMessageUpdate(QJsonObject message);
    void parseChannelUpdate(QJsonObject message);
    void parseChannelJoin(QJsonObject message);
    void parsePresenceChange(QJsonObject message);

    QVariantMap getMessageData(const QJsonObject message);

    QString getContent(QJsonObject message);
    QVariantList getAttachments(QJsonObject message);
    QVariantList getImages(QJsonObject message);
    QString getAttachmentColor(QJsonObject attachment);
    QVariantList getAttachmentFields(QJsonObject attachment);
    QVariantList getAttachmentImages(QJsonObject attachment);

    QVariantMap parseChannel(QJsonObject data);

    void parseUsers(QJsonObject data);
    void parseBots(QJsonObject data);
    void parseChannels(QJsonObject data);
    void parseGroups(QJsonObject data);
    void parseChats(QJsonObject data);

    void findNewUsers(const QString &message);

    QVariantMap user(const QJsonObject &data);

    QString historyMethod(QString type);
    QString markMethod(QString type);

    QPointer<QNetworkAccessManager> networkAccessManager;
    QPointer<SlackConfig> config;
    QPointer<SlackStream> stream;
    QPointer<QTimer> reconnectTimer;

    QNetworkAccessManager::NetworkAccessibility networkAccessible;
};

#endif // SLACKCLIENT_H
