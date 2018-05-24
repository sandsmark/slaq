#include "slackclientthreadspawner.h"

SlackClientThreadSpawner::SlackClientThreadSpawner(QObject *parent) :
    QThread(parent)
  , m_slackClient(nullptr) { }

SlackClient *SlackClientThreadSpawner::slackClient() { return m_slackClient; }

void SlackClientThreadSpawner::startClient()
{
    QMetaObject::invokeMethod(m_slackClient, "startClient", Qt::QueuedConnection);
}

void SlackClientThreadSpawner::testLogin()
{
    QMetaObject::invokeMethod(m_slackClient, "testLogin", Qt::QueuedConnection);
}

void SlackClientThreadSpawner::setAppActive(bool active)
{
    QMetaObject::invokeMethod(m_slackClient, "setAppActive", Qt::QueuedConnection,
                              Q_ARG(bool, active));
}

void SlackClientThreadSpawner::setActiveWindow(const QString &windowId)
{
    QMetaObject::invokeMethod(m_slackClient, "setActiveWindow", Qt::QueuedConnection,
                              Q_ARG(QString, windowId));
}

QVariantList SlackClientThreadSpawner::getChannels()
{
    QVariantList retVal;
    QMetaObject::invokeMethod(m_slackClient, "getChannels", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariantList, retVal));
    return retVal;
}

QVariant SlackClientThreadSpawner::getChannel(const QString &channelId)
{
    QVariant retVal;
    QMetaObject::invokeMethod(m_slackClient, "getChannel", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, retVal),
                              Q_ARG(QString, channelId));
    return retVal;
}

QStringList SlackClientThreadSpawner::getNickSuggestions(const QString &currentText, const int cursorPosition)
{
    QStringList retVal;
    QMetaObject::invokeMethod(m_slackClient, "getNickSuggestions", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QStringList, retVal),
                              Q_ARG(QString, currentText),
                              Q_ARG(int, cursorPosition));
    return retVal;
}

QString SlackClientThreadSpawner::emojiNameByEmoji(const QString &emoji) const
{
    QString retVal;
    QMetaObject::invokeMethod(m_slackClient, "emojiNameByEmoji", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString, retVal),
                              Q_ARG(QString, emoji));
    return retVal;
}

void SlackClientThreadSpawner::loadMessages(const QString &type, const QString &channelId)
{
    QMetaObject::invokeMethod(m_slackClient, "loadMessages", Qt::QueuedConnection,
                              Q_ARG(QString, type), Q_ARG(QString, channelId));
}

bool SlackClientThreadSpawner::isOnline() const
{
    bool retVal;
    QMetaObject::invokeMethod(m_slackClient, "isOnline", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, retVal));
    return retVal;
}

bool SlackClientThreadSpawner::isDevice() const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

QString SlackClientThreadSpawner::lastChannel()
{
    QString retVal;
    QMetaObject::invokeMethod(m_slackClient, "lastChannel", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString, retVal));
    return retVal;
}

QUrl SlackClientThreadSpawner::avatarUrl(const QString &userId)
{
    QUrl retVal;
    QMetaObject::invokeMethod(m_slackClient, "avatarUrl", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QUrl, retVal),
                              Q_ARG(QString, userId));
    return retVal;
}

void SlackClientThreadSpawner::fetchAccessToken(const QUrl &url)
{
    QMetaObject::invokeMethod(m_slackClient, "fetchAccessToken", Qt::QueuedConnection,
                              Q_ARG(QUrl, url));
}

void SlackClientThreadSpawner::markChannel(const QString &type, const QString &channelId, const QString &time)
{
    QMetaObject::invokeMethod(m_slackClient, "markChannel", Qt::QueuedConnection,
                              Q_ARG(QString, type),
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, time));
}

void SlackClientThreadSpawner::joinChannel(const QString &channelId)
{
    QMetaObject::invokeMethod(m_slackClient, "joinChannel", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::leaveChannel(const QString &channelId)
{
    QMetaObject::invokeMethod(m_slackClient, "leaveChannel", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::leaveGroup(const QString &groupId)
{
    QMetaObject::invokeMethod(m_slackClient, "leaveGroup", Qt::QueuedConnection,
                              Q_ARG(QString, groupId));
}

void SlackClientThreadSpawner::postMessage(const QString &channelId, const QString& content)
{
    QMetaObject::invokeMethod(m_slackClient, "postMessage", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, content));
}

void SlackClientThreadSpawner::postImage(const QString &channelId, const QString &imagePath, const QString &title, const QString &comment)
{
    QMetaObject::invokeMethod(m_slackClient, "postImage", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, imagePath),
                              Q_ARG(QString, title),
                              Q_ARG(QString, comment));
}

void SlackClientThreadSpawner::deleteReaction(const QString &channelId, const QString &ts, const QString &reaction)
{
    QMetaObject::invokeMethod(m_slackClient, "deleteReaction", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, ts),
                              Q_ARG(QString, reaction));
}

void SlackClientThreadSpawner::addReaction(const QString &channelId, const QString &ts, const QString &reaction)
{
    QMetaObject::invokeMethod(m_slackClient, "addReaction", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, ts),
                              Q_ARG(QString, reaction));
}

void SlackClientThreadSpawner::openChat(const QString &chatId)
{
    QMetaObject::invokeMethod(m_slackClient, "openChat", Qt::QueuedConnection,
                              Q_ARG(QString, chatId));
}

void SlackClientThreadSpawner::closeChat(const QString &chatId)
{
    QMetaObject::invokeMethod(m_slackClient, "closeChat", Qt::QueuedConnection,
                              Q_ARG(QString, chatId));
}

void SlackClientThreadSpawner::run()
{
    m_slackClient = new SlackClient;

    connect(m_slackClient, &SlackClient::testConnectionFail, this, &SlackClientThreadSpawner::testConnectionFail, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::testLoginSuccess, this, &SlackClientThreadSpawner::testLoginSuccess, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::testLoginFail, this, &SlackClientThreadSpawner::testLoginFail, Qt::QueuedConnection);

    connect(m_slackClient, &SlackClient::accessTokenSuccess, this, &SlackClientThreadSpawner::accessTokenSuccess, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::accessTokenFail, this, &SlackClientThreadSpawner::accessTokenFail, Qt::QueuedConnection);

    connect(m_slackClient, &SlackClient::loadMessagesSuccess, this, &SlackClientThreadSpawner::loadMessagesSuccess, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::loadMessagesFail, this, &SlackClientThreadSpawner::loadMessagesFail, Qt::QueuedConnection);

    connect(m_slackClient, &SlackClient::initFail, this, &SlackClientThreadSpawner::initFail, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::initSuccess, this, &SlackClientThreadSpawner::initSuccess, Qt::QueuedConnection);

    connect(m_slackClient, &SlackClient::reconnectFail, this, &SlackClientThreadSpawner::reconnectFail, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::reconnectAccessTokenFail, this, &SlackClientThreadSpawner::reconnectAccessTokenFail, Qt::QueuedConnection);

    connect(m_slackClient, &SlackClient::messageReceived, this, &SlackClientThreadSpawner::messageReceived, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::messageUpdated, this, &SlackClientThreadSpawner::messageUpdated, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::channelUpdated, this, &SlackClientThreadSpawner::channelUpdated, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::channelJoined, this, &SlackClientThreadSpawner::channelJoined, Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::channelLeft, this, &SlackClientThreadSpawner::channelLeft , Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::userUpdated, this, &SlackClientThreadSpawner::userUpdated , Qt::QueuedConnection);

    connect(m_slackClient, &SlackClient::postImageSuccess, this, &SlackClientThreadSpawner::postImageSuccess , Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::postImageFail, this, &SlackClientThreadSpawner::postImageFail , Qt::QueuedConnection);

    connect(m_slackClient, &SlackClient::networkOff, this, &SlackClientThreadSpawner::networkOff , Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::networkOn, this, &SlackClientThreadSpawner::networkOn, Qt::QueuedConnection);

    connect(m_slackClient, &SlackClient::reconnecting, this, &SlackClientThreadSpawner::reconnecting , Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::disconnected, this, &SlackClientThreadSpawner::disconnected , Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::connected, this, &SlackClientThreadSpawner::connected , Qt::QueuedConnection);

    connect(m_slackClient, &SlackClient::isOnlineChanged, this, &SlackClientThreadSpawner::isOnlineChanged , Qt::QueuedConnection);
    connect(m_slackClient, &SlackClient::lastChannelChanged, this, &SlackClientThreadSpawner::lastChannelChanged , Qt::QueuedConnection);

    //make sure signal will be delivered to QML
    QMetaObject::invokeMethod(this, "threadStarted", Qt::QueuedConnection);
    // Start QT event loop for this thread
    this->exec();
    delete m_slackClient;
    m_slackClient = nullptr;
    qDebug() << "closed thread";
}
