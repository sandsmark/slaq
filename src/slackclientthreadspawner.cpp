#include "slackclientthreadspawner.h"

SlackClientThreadSpawner::SlackClientThreadSpawner(QObject *parent) :
    QThread(parent)
{
    QSettings settings;
    m_lastTeam = settings.value(QStringLiteral("LastTeam")).toString();
}

SlackClientThreadSpawner::~SlackClientThreadSpawner() {}

SlackClient *SlackClientThreadSpawner::slackClient(const QString &teamId) {
    //qDebug() << "slackClient" << teamId << m_knownTeams.value(teamId);
    return m_knownTeams.value(teamId, nullptr);
}

QQmlGadgetListModel<TeamInfo> *SlackClientThreadSpawner::teamsModel()
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
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "startClient", Qt::QueuedConnection);
}

void SlackClientThreadSpawner::testLogin(const QString& teamId)
{
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "testLogin", Qt::QueuedConnection);
}

void SlackClientThreadSpawner::setAppActive(const QString& teamId, bool active)
{
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "setAppActive", Qt::QueuedConnection,
                              Q_ARG(bool, active));
}

void SlackClientThreadSpawner::setActiveWindow(const QString& teamId, const QString &windowId)
{
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "setActiveWindow", Qt::QueuedConnection,
                              Q_ARG(QString, windowId));
}

QVariantList SlackClientThreadSpawner::getChannels(const QString& teamId)
{
    QVariantList retVal;
    SlackClient* _slackClient = slackClient(teamId);
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
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "getChannel", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, retVal),
                              Q_ARG(QString, channelId));
    return retVal;
}

QStringList SlackClientThreadSpawner::getNickSuggestions(const QString& teamId, const QString &currentText, const int cursorPosition)
{
    QStringList retVal;
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "getNickSuggestions", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QStringList, retVal),
                              Q_ARG(QString, currentText),
                              Q_ARG(int, cursorPosition));
    return retVal;
}

void SlackClientThreadSpawner::loadMessages(const QString& teamId, const QString &type, const QString &channelId)
{
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "loadMessages", Qt::QueuedConnection,
                              Q_ARG(QString, type), Q_ARG(QString, channelId));
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
    SlackClient* _slackClient = slackClient(teamId);
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
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "avatarUrl", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QUrl, retVal),
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

    connectToTeam(_teamId, _accessToken);
//    bool retVal = false;
//    SlackClient* _slackClient = slackClient(teamId);
//    if (_slackClient == nullptr) {
//        return retVal;
//    }
//    QMetaObject::invokeMethod(_slackClient, "handleAccessTokenReply", Qt::BlockingQueuedConnection,
//                              Q_RETURN_ARG(bool, retVal),
//                              Q_ARG(QJsonObject, bootData));
//    return retVal;
    return true;
}

void SlackClientThreadSpawner::markChannel(const QString& teamId, const QString &type, const QString &channelId, const QString &time)
{
    SlackClient* _slackClient = slackClient(teamId);
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
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "joinChannel", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::leaveChannel(const QString& teamId, const QString &channelId)
{
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "leaveChannel", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::leaveGroup(const QString& teamId, const QString &groupId)
{
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "leaveGroup", Qt::QueuedConnection,
                              Q_ARG(QString, groupId));
}

void SlackClientThreadSpawner::postMessage(const QString& teamId, const QString &channelId, const QString& content)
{
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "postMessage", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, content));
}

void SlackClientThreadSpawner::postImage(const QString& teamId, const QString &channelId, const QString &imagePath, const QString &title, const QString &comment)
{
    SlackClient* _slackClient = slackClient(teamId);
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
    SlackClient* _slackClient = slackClient(teamId);
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
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "addReaction", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, ts),
                              Q_ARG(QString, reaction));
}

void SlackClientThreadSpawner::openChat(const QString& teamId, const QString &chatId)
{
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "openChat", Qt::QueuedConnection,
                              Q_ARG(QString, chatId));
}

void SlackClientThreadSpawner::closeChat(const QString& teamId, const QString &chatId)
{
    SlackClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "closeChat", Qt::QueuedConnection,
                              Q_ARG(QString, chatId));
}

// TODO(unknown): reimplement for thread wrapper
void SlackClientThreadSpawner::onTeamInfoChanged(const QString &teamId)
{
    Q_UNUSED(teamId);
//    QList<TeamInfo *> list = m_slackClient->getKnownTeams();
//    m_teamsModel.clear();
//    for (TeamInfo *ti : list) {
//        m_teamsModel.append(*ti);
//    }
}

SlackClient* SlackClientThreadSpawner::createNewClientInstance(const QString &teamId, const QString &accessToken) {
    SlackClient* _slackClient = new SlackClient(teamId, accessToken);
    QQmlEngine::setObjectOwnership(_slackClient, QQmlEngine::CppOwnership);

    connect(_slackClient, &SlackClient::testConnectionFail, this, &SlackClientThreadSpawner::testConnectionFail, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::testLoginSuccess, this, &SlackClientThreadSpawner::testLoginSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::testLoginFail, this, &SlackClientThreadSpawner::testLoginFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::accessTokenSuccess, this, &SlackClientThreadSpawner::accessTokenSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::accessTokenFail, this, &SlackClientThreadSpawner::accessTokenFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::loadMessagesSuccess, this, &SlackClientThreadSpawner::loadMessagesSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::loadMessagesFail, this, &SlackClientThreadSpawner::loadMessagesFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::initFail, this, &SlackClientThreadSpawner::initFail, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::initSuccess, this, &SlackClientThreadSpawner::initSuccess, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::reconnectFail, this, &SlackClientThreadSpawner::reconnectFail, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::reconnectAccessTokenFail, this, &SlackClientThreadSpawner::reconnectAccessTokenFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::messageReceived, this, &SlackClientThreadSpawner::messageReceived, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::messageUpdated, this, &SlackClientThreadSpawner::messageUpdated, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::channelUpdated, this, &SlackClientThreadSpawner::channelUpdated, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::channelJoined, this, &SlackClientThreadSpawner::channelJoined, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::channelLeft, this, &SlackClientThreadSpawner::channelLeft, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::userUpdated, this, &SlackClientThreadSpawner::userUpdated, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::postImageSuccess, this, &SlackClientThreadSpawner::postImageSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::postImageFail, this, &SlackClientThreadSpawner::postImageFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::networkOff, this, &SlackClientThreadSpawner::networkOff, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::networkOn, this, &SlackClientThreadSpawner::networkOn, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::reconnecting, this, &SlackClientThreadSpawner::reconnecting, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::disconnected, this, &SlackClientThreadSpawner::disconnected, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::connected, this, &SlackClientThreadSpawner::connected, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::lastChannelChanged, this, &SlackClientThreadSpawner::lastChannelChanged, Qt::QueuedConnection);
    connect(_slackClient, &SlackClient::teamInfoChanged, this, &SlackClientThreadSpawner::onTeamInfoChanged, Qt::QueuedConnection);

    connect(_slackClient, &SlackClient::isOnlineChanged, this, &SlackClientThreadSpawner::onOnlineChanged, Qt::QueuedConnection);

    return _slackClient;
}

void SlackClientThreadSpawner::connectToTeam(const QString &teamId, const QString &accessToken)
{
    if (teamId.isEmpty()) {
        return;
    }
    SlackClient* _slackClient = slackClient(teamId);

    if (_slackClient == nullptr) {
        _slackClient = createNewClientInstance(teamId, accessToken);
        _slackClient->moveToThread(this);
        _slackClient->startConnections();
        qDebug() << "before added slack client" << _slackClient->teamInfo()->teamId() << _slackClient->teamInfo()->teamToken();
        m_knownTeams[teamId] = _slackClient;
        m_teamsModel.append(*(_slackClient->teamInfo()));
        qDebug() << "added slack client" << teamId << m_teamsModel.count();
    }
    _slackClient->startClient();
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

void SlackClientThreadSpawner::run()
{
    for (const QString& teamId : SlackConfig::instance()->teams()) {
        SlackClient* _slackClient = createNewClientInstance(teamId);
        _slackClient->startConnections();

        qDebug() << "before added slack client" << _slackClient->teamInfo()->teamId() << _slackClient->teamInfo()->teamToken();
        m_knownTeams[teamId] = _slackClient;
        m_teamsModel.append(*(_slackClient->teamInfo()));
        qDebug() << "added slack client" << teamId << m_teamsModel.count();
//        if (m_lastTeam == teamId) {
//            _slackClient->startClient();
//        }
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
