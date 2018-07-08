#include "slackclientthreadspawner.h"
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaContent>

SlackClientThreadSpawner::SlackClientThreadSpawner(QObject *parent) :
    QThread(parent)
{
    QSettings settings;
    m_lastTeam = settings.value(QStringLiteral("LastTeam")).toString();
}

SlackClientThreadSpawner::~SlackClientThreadSpawner() {}

SlackTeamClient *SlackClientThreadSpawner::slackClient(const QString &teamId) {
    //qDebug() << "slackClient" << teamId << m_knownTeams.value(teamId);
    return m_knownTeams.value(teamId, nullptr);
}

QQmlObjectListModel<TeamInfo> *SlackClientThreadSpawner::teamsModel()
{
    return &m_teamsModel;
}

QString SlackClientThreadSpawner::lastTeam() const
{
    return m_lastTeam;
}

bool SlackClientThreadSpawner::isOnline()
{
    return slackClient(m_lastTeam) ? slackClient(m_lastTeam)->isOnline() : false;
}

void SlackClientThreadSpawner::startClient(const QString& teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "startClient", Qt::QueuedConnection);
}

void SlackClientThreadSpawner::testLogin(const QString& teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "testLogin", Qt::QueuedConnection);
}

void SlackClientThreadSpawner::setAppActive(const QString& teamId, bool active)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "setAppActive", Qt::QueuedConnection,
                              Q_ARG(bool, active));
}

void SlackClientThreadSpawner::setActiveWindow(const QString& teamId, const QString &windowId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "setActiveWindow", Qt::QueuedConnection,
                              Q_ARG(QString, windowId));
}

QVariantList SlackClientThreadSpawner::getChannels(const QString& teamId)
{
    QVariantList retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }

    QMetaObject::invokeMethod(_slackClient, "getChannels", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariantList, retVal));
    return retVal;
}

QVariant SlackClientThreadSpawner::getChannel(const QString& teamId, const QString &channelId)
{
    QVariant retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "getChannel", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, retVal),
                              Q_ARG(QString, channelId));
    return retVal;
}

void SlackClientThreadSpawner::searchMessages(const QString &teamId, const QString &searchString)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "searchMessages", Qt::QueuedConnection,
                              Q_ARG(QString, searchString));
}

QStringList SlackClientThreadSpawner::getNickSuggestions(const QString& teamId, const QString &currentText, const int cursorPosition)
{
    QStringList retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "getNickSuggestions", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QStringList, retVal),
                              Q_ARG(QString, currentText),
                              Q_ARG(int, cursorPosition));
    return retVal;
}

void SlackClientThreadSpawner::loadMessages(const QString& teamId, ChatsModel::ChatType type, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "loadMessages", Qt::QueuedConnection,
                              Q_ARG(ChatsModel::ChatType, type), Q_ARG(QString, channelId));
}

bool SlackClientThreadSpawner::isDevice() const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

QString SlackClientThreadSpawner::lastChannel(const QString& teamId)
{
    QString retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "lastChannel", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString, retVal));
    return retVal;
}

QUrl SlackClientThreadSpawner::avatarUrl(const QString& teamId, const QString &userId)
{
    QUrl retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "avatarUrl", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QUrl, retVal),
                              Q_ARG(QString, userId));
    return retVal;
}

QString SlackClientThreadSpawner::userName(const QString& teamId, const QString &userId)
{
    QString retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "userName", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString, retVal),
                              Q_ARG(QString, userId));
    return retVal;
}

bool SlackClientThreadSpawner::handleAccessTokenReply(const QJsonObject &bootData)
{
    QString _accessToken = bootData[QStringLiteral("api_token")].toString();
    if (_accessToken.isEmpty()) {
        qWarning() << "Missing access token";
        return false;
    }

    QString _teamId = bootData[QStringLiteral("team_id")].toString();
    if (_teamId.isEmpty()) {
        qWarning() << "Missing team id";
        return false;
    }

    QString _userId = bootData[QStringLiteral("user_id")].toString();
    if (_userId.isEmpty()) {
        qWarning() << "Missing user id";
        return false;
    }

    QString _teamName = bootData[QStringLiteral("team_url")].toString();
    if (_teamName.isEmpty()) {
        qWarning() << "Missing team name";
        return false;
    }

    qDebug() << "Access token success" << _accessToken << _userId << _teamId << _teamName;

    QMetaObject::invokeMethod(this, "connectToTeam", Qt::QueuedConnection,
                              Q_ARG(QString, _teamId),
                              Q_ARG(QString, _accessToken));
    return true;
}

void SlackClientThreadSpawner::markChannel(const QString& teamId, const QString &type, const QString &channelId, const QString &time)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "markChannel", Qt::QueuedConnection,
                              Q_ARG(QString, type),
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, time));
}

void SlackClientThreadSpawner::joinChannel(const QString& teamId, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "joinChannel", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::leaveChannel(const QString& teamId, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "leaveChannel", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::leaveGroup(const QString& teamId, const QString &groupId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "leaveGroup", Qt::QueuedConnection,
                              Q_ARG(QString, groupId));
}

void SlackClientThreadSpawner::postMessage(const QString& teamId, const QString &channelId, const QString& content)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "postMessage", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, content));
}

void SlackClientThreadSpawner::postImage(const QString& teamId, const QString &channelId, const QString &imagePath, const QString &title, const QString &comment)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "postImage", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, imagePath),
                              Q_ARG(QString, title),
                              Q_ARG(QString, comment));
}

void SlackClientThreadSpawner::deleteReaction(const QString& teamId, const QString &channelId, const QString &ts, const QString &reaction)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "deleteReaction", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, ts),
                              Q_ARG(QString, reaction));
}

void SlackClientThreadSpawner::addReaction(const QString& teamId, const QString &channelId, const QString &ts, const QString &reaction)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "addReaction", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, ts),
                              Q_ARG(QString, reaction));
}

void SlackClientThreadSpawner::onMessageReceived(Message *message)
{
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
}

void SlackClientThreadSpawner::openChat(const QString& teamId, const QString &chatId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "openChat", Qt::QueuedConnection,
                              Q_ARG(QString, chatId));
}

void SlackClientThreadSpawner::closeChat(const QString& teamId, const QString &chatId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "closeChat", Qt::QueuedConnection,
                              Q_ARG(QString, chatId));
}

void SlackClientThreadSpawner::onTeamInfoChanged(const QString &teamId)
{
    qDebug() << "updated team info for id" << teamId << QThread::currentThreadId();;
}

SlackTeamClient* SlackClientThreadSpawner::createNewClientInstance(const QString &teamId, const QString &accessToken) {
    SlackTeamClient* _slackClient = new SlackTeamClient(teamId, accessToken);
    QQmlEngine::setObjectOwnership(_slackClient, QQmlEngine::CppOwnership);

    connect(_slackClient, &SlackTeamClient::testConnectionFail, this, &SlackClientThreadSpawner::testConnectionFail, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::testLoginSuccess, this, &SlackClientThreadSpawner::testLoginSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::testLoginFail, this, &SlackClientThreadSpawner::testLoginFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::accessTokenSuccess, this, &SlackClientThreadSpawner::accessTokenSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::accessTokenFail, this, &SlackClientThreadSpawner::accessTokenFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::loadMessagesSuccess, this, &SlackClientThreadSpawner::loadMessagesSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::loadMessagesFail, this, &SlackClientThreadSpawner::loadMessagesFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::initFail, this, &SlackClientThreadSpawner::initFail, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::initSuccess, this, &SlackClientThreadSpawner::initSuccess, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::reconnectFail, this, &SlackClientThreadSpawner::reconnectFail, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::reconnectAccessTokenFail, this, &SlackClientThreadSpawner::reconnectAccessTokenFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::messageReceived, this, &SlackClientThreadSpawner::onMessageReceived, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::messageUpdated, this, &SlackClientThreadSpawner::messageUpdated, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::channelUpdated, this, &SlackClientThreadSpawner::channelUpdated, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::channelJoined, this, &SlackClientThreadSpawner::channelJoined, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::channelLeft, this, &SlackClientThreadSpawner::channelLeft, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::userUpdated, this, &SlackClientThreadSpawner::userUpdated, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::postImageSuccess, this, &SlackClientThreadSpawner::postImageSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::postImageFail, this, &SlackClientThreadSpawner::postImageFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::networkOff, this, &SlackClientThreadSpawner::networkOff, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::networkOn, this, &SlackClientThreadSpawner::networkOn, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::reconnecting, this, &SlackClientThreadSpawner::reconnecting, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::disconnected, this, &SlackClientThreadSpawner::disconnected, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::connected, this, &SlackClientThreadSpawner::connected, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::lastChannelChanged, this, &SlackClientThreadSpawner::lastChannelChanged, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::teamInfoChanged, this, &SlackClientThreadSpawner::onTeamInfoChanged, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::isOnlineChanged, this, &SlackClientThreadSpawner::onOnlineChanged, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::searchResultsReady, this, &SlackClientThreadSpawner::searchResultsReady, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::userTyping, this, &SlackClientThreadSpawner::userTyping, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::teamDataChanged, this, &SlackClientThreadSpawner::onTeamDataChanged, Qt::QueuedConnection);

    return _slackClient;
}

void SlackClientThreadSpawner::connectToTeam(const QString &teamId, const QString &accessToken)
{
    if (teamId.isEmpty()) {
        return;
    }
    SlackTeamClient* _slackClient = slackClient(teamId);

    if (_slackClient == nullptr) {
        _slackClient = createNewClientInstance(teamId, accessToken);
        _slackClient->moveToThread(this);
        _slackClient->startConnections();
        m_knownTeams[teamId] = _slackClient;
        m_teamsModel.append(_slackClient->teamInfo());
    }
    _slackClient->startClient();
}

void SlackClientThreadSpawner::leaveTeam(const QString &teamId)
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "leaveTeam", Qt::QueuedConnection, Q_ARG(QString, teamId));
        return;
    }

    if (teamId.isEmpty()) {
        return;
    }
    SlackTeamClient* _slackClient = slackClient(teamId);

    if (_slackClient == nullptr) {
        return;
    }

    m_knownTeams.remove(teamId);
    m_teamsModel.remove(_slackClient->teamInfo());
    _slackClient->deleteLater();
    SlackConfig::instance()->setTeams(m_knownTeams.keys());
}

void SlackClientThreadSpawner::setLastTeam(const QString &lastTeam)
{
    if (m_lastTeam == lastTeam)
        return;

    m_lastTeam = lastTeam;
    emit lastTeamChanged(m_lastTeam);
}

void SlackClientThreadSpawner::onOnlineChanged(const QString &teamId)
{
    if (teamId == m_lastTeam && slackClient(teamId) != nullptr) {
        emit onlineChanged(slackClient(teamId)->isOnline());
    }
}

void SlackClientThreadSpawner::setMediaSource(QObject *mediaPlayer, const QString &teamId,
                                              const QString &url)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }

    QMediaPlayer *mPlayer = qvariant_cast<QMediaPlayer *>(mediaPlayer->property("mediaObject"));
    if (mPlayer == nullptr) {
        qWarning() << "no media player found";
        return;
    }
    QNetworkRequest request;
    request.setUrl(url);

    QString token = _slackClient->teamInfo()->teamToken();
    if (!token.isEmpty()) {
        request.setRawHeader(QString("Authorization").toUtf8(), QString("Bearer " + token).toUtf8());
    } else {
        qWarning() << "token for team" << _slackClient->teamInfo()->name() << "not found";
        return;
    }

    qDebug() << "set media content" << request.url();
    mPlayer->setMedia(QMediaContent(request));
}

QString SlackClientThreadSpawner::teamToken(const QString &teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return QStringLiteral("");
    }
    return _slackClient->teamInfo()->teamToken();
}

// this method should be run on GUI thread since QML engine cant connect to models, created in thread, differs from QML Engine
void SlackClientThreadSpawner::onTeamDataChanged(const QJsonObject &teamData)
{
    DEBUG_BLOCK
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    _slackClient->teamInfo()->addTeamData(teamData);
    emit chatsModelChanged(_slackClient->teamInfo()->teamId());
}

void SlackClientThreadSpawner::run()
{
    for (const QString& teamId : SlackConfig::instance()->teams()) {
        connectToTeam(teamId);
    }
    //make sure signal will be delivered to QML
    QMetaObject::invokeMethod(this, "threadStarted", Qt::QueuedConnection);
    // Start QT event loop for this thread
    this->exec();
    SlackConfig::instance()->setTeams(m_knownTeams.keys());
    qDeleteAll(m_knownTeams.values());
    QSettings settings;
    settings.setValue(QStringLiteral("LastTeam"), m_lastTeam);
    qDebug() << "closed thread";
}
