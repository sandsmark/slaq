#ifndef THREADSPAWNER_H
#define THREADSPAWNER_H

#include <QThread>
#include <QMutex>
#include "slackclient.h"

class SlackClientThreadSpawner : public QThread
{
    Q_OBJECT

    Q_PROPERTY(bool isOnline READ isOnline NOTIFY isOnlineChanged)
    Q_PROPERTY(bool isDevice READ isDevice CONSTANT)
    Q_PROPERTY(QString lastChannel READ lastChannel WRITE setActiveWindow NOTIFY lastChannelChanged)

public:
    explicit SlackClientThreadSpawner(QObject *parent = nullptr);

    virtual ~SlackClientThreadSpawner() {}
    SlackClient *slackClient();

signals:
    void threadStarted();

    //signals from thread
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
    void startClient();
    void testLogin();

    void setAppActive(bool active);
    void setActiveWindow(const QString &windowId);


    QStringList getNickSuggestions(const QString &currentText, const int cursorPosition);


    bool isOnline() const;
    bool isDevice() const;
    QString lastChannel();

    QUrl avatarUrl(const QString &userId);
    //channels
    void markChannel(const QString& type, const QString& channelId, const QString& time);
    void joinChannel(const QString& channelId);
    void leaveChannel(const QString& channelId);
    void leaveGroup(const QString& groupId);
    QVariantList getChannels();
    QVariant getChannel(const QString& channelId);

    //messages
    void loadMessages(const QString& type, const QString& channelId);
    void postMessage(const QString& channelId, const QString& content);
    void postImage(const QString& channelId, const QString& imagePath, const QString& title, const QString& comment);

    //chats
    void openChat(const QString& chatId);
    void closeChat(const QString& chatId);

protected:
    void run();

private:
    SlackClient *m_slackClient;
};

#endif

