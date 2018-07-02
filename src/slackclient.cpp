#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QSize>
#include <QStringList>
#include <QRegularExpression>
#include <QFile>
#include <QHttpMultiPart>
#include <QtNetwork/QNetworkConfigurationManager>

#include "slackclient.h"

#include "imagescache.h"

SlackClient::SlackClient(const QString &teamId, const QString &accessToken, QObject *parent) :
    QObject(parent), appActive(true), activeWindow("init"), networkAccessible(QNetworkAccessManager::Accessible),
    m_clientId(QString::fromLatin1(QByteArray::fromBase64("MTE5MDczMjc1MDUuMjUyMzc1NTU3MTU1"))),
    m_clientId2(QString::fromLatin1(QByteArray::fromBase64("MGJlNDA0M2Q2OGIxYjM0MzE4ODk5ZDEwYTNiYmM3ZTY="))),
    m_networksModel(new NetworksModel(this))
{
    DEBUG_BLOCK
    m_teamInfo.setTeamId(teamId);
    config = SlackConfig::instance();
    config->loadTeamInfo(m_teamInfo);
    if (m_teamInfo.teamToken().isEmpty()) {
        m_teamInfo.setTeamToken(accessToken);
    }
    setState(ClientStates::DISCONNECTED);
    qDebug() << "client ctor finished" << m_teamInfo.teamToken() << m_teamInfo.teamId() << m_teamInfo.name();

    QQmlEngine::setObjectOwnership(m_networksModel, QQmlEngine::CppOwnership);
}

SlackClient::~SlackClient() {}

void SlackClient::startConnections()
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "startConnections", Qt::QueuedConnection);
        return;
    }
    networkAccessManager = new QNetworkAccessManager(this);

    stream = new SlackStream(this);
    reconnectTimer = new QTimer(this);
    networkAccessible = networkAccessManager->networkAccessible();

    connect(networkAccessManager.data(), &QNetworkAccessManager::networkAccessibleChanged, this, &SlackClient::handleNetworkAccessibleChanged);
    connect(reconnectTimer.data(), &QTimer::timeout, this, &SlackClient::reconnectClient);

    connect(stream.data(), &SlackStream::connected, this, &SlackClient::handleStreamStart);
    connect(stream.data(), &SlackStream::disconnected, this, &SlackClient::handleStreamEnd);
    connect(stream.data(), &SlackStream::messageReceived, this, &SlackClient::handleStreamMessage);

    connect(this, &SlackClient::connected, this, &SlackClient::isOnlineChanged);
    connect(this, &SlackClient::initSuccess, this, &SlackClient::isOnlineChanged);
    connect(this, &SlackClient::disconnected, this, &SlackClient::isOnlineChanged);
}

void SlackClient::setAppActive(bool active)
{
    DEBUG_BLOCK

    appActive = active;
    clearNotifications();
}

void SlackClient::setActiveWindow(const QString& windowId)
{
    DEBUG_BLOCK

    if (windowId == activeWindow) {
        return;
    }

    activeWindow = windowId;
    clearNotifications();

    if (!windowId.isEmpty()) {
        m_teamInfo.setLastChannel(windowId);
        config->saveTeamInfo(m_teamInfo);
        emit lastChannelChanged(m_teamInfo.teamId());
    }
}

void SlackClient::clearNotifications()
{
    DEBUG_BLOCK

    //  foreach (QObject* object, Notification::notifications()) {
    //      Notification* n = qobject_cast<Notification*>(object);
    //      if (n->hintValue("x-slaq-channel").toString() == activeWindow) {
    //          n->close();
    //      }

    //      delete n;
    //  }
}

void SlackClient::handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible)
{
    DEBUG_BLOCK

    qDebug() << "Network accessible changed" << accessible;
    networkAccessible = accessible;

    if (networkAccessible == QNetworkAccessManager::Accessible) {
        emit networkOn(m_teamInfo.teamId());
    } else {
        setState(ClientStates::DISCONNECTED);
        emit networkOff(m_teamInfo.teamId());
    }
}

void SlackClient::reconnectClient()
{
    DEBUG_BLOCK

    qDebug() << "Reconnecting";
    setState(ClientStates::RECONNECTING);
    emit reconnecting(m_teamInfo.teamId());
    startClient();
}

void SlackClient::handleStreamStart()
{
    DEBUG_BLOCK

    qDebug() << "Stream started";
    setState(ClientStates::CONNECTED);
    emit connected(m_teamInfo.teamId());
}

void SlackClient::handleStreamEnd(){
    DEBUG_BLOCK
    qDebug() << "Stream ended";
    setState(ClientStates::RECONNECTING);
    emit reconnecting(m_teamInfo.teamId());
    reconnectTimer->setSingleShot(true);
    reconnectTimer->start(1000);
}

void SlackClient::handleStreamMessage(const QJsonObject& message)
{
    DEBUG_BLOCK

    const QString& type = message.value(QStringLiteral("type")).toString();

    qDebug() << "stream message";
    qDebug().noquote() << QJsonDocument(message).toJson();

    if (type == QStringLiteral("message")) {
        parseMessageUpdate(message);
    } else if (type == QStringLiteral("group_marked") ||
               type == QStringLiteral("channel_marked") ||
               type == QStringLiteral("im_marked") ||
               type == QStringLiteral("mpim_marked")) {
        parseChannelUpdate(message);
    } else if (type == QStringLiteral("channel_joined")) {
        parseChannelJoin(message);
    } else if (type == QStringLiteral("group_joined")) {
        parseGroupJoin(message);
    } else if (type == QStringLiteral("im_open")) {
        parseChatOpen(message);
    } else if (type == QStringLiteral("im_close")) {
        parseChatClose(message);
    } else if (type == QStringLiteral("channel_left") || type == QStringLiteral("group_left")) {
        parseChannelLeft(message);
    } else if (type == QStringLiteral("presence_change") || type == QStringLiteral("manual_presence_change")) {
        parsePresenceChange(message);
    } else if (type == QStringLiteral("desktop_notification")) {
        parseNotification(message);
    } else if (type == QStringLiteral("reaction_added") || type == QStringLiteral("reaction_removed")) {
        parseReactionUpdate(message);
    } else if (type == QStringLiteral("user_typing")) {
        qDebug() << "user typing" << message;
        emit userTyping(m_teamInfo.teamId(),
                        message.value(QStringLiteral("channel")).toString(),
                        message.value(QStringLiteral("user")).toString());
    } else if (type == QStringLiteral("team_join") || type == QStringLiteral("user_change")) {
        qDebug() << "user joined" << message;
        parseUser(message.value(QStringLiteral("user")).toObject());
        m_storage.updateUsersList();
    } else if (type == QStringLiteral("member_joined_channel")) {
        qDebug() << "user joined to channel" << message.value(QStringLiteral("user")).toString();
    } else {
        qDebug() << "Unhandled message";
        qDebug().noquote() << QJsonDocument(message).toJson();
    }
}

void SlackClient::parseChatOpen(const QJsonObject& message)
{
    DEBUG_BLOCK

    QString id = message.value(QStringLiteral("channel")).toString();
    QVariantMap channel = m_storage.channel(id);
    channel.insert(QStringLiteral("isOpen"), QVariant(true));
    m_storage.saveChannel(channel);
    emit channelJoined(m_teamInfo.teamId(), channel);
}

void SlackClient::parseChatClose(const QJsonObject& message)
{
    DEBUG_BLOCK

    QString id = message.value(QStringLiteral("channel")).toString();
    QVariantMap channel = m_storage.channel(id);
    channel.insert(QStringLiteral("isOpen"), QVariant(false));
    m_storage.saveChannel(channel);
    emit channelLeft(m_teamInfo.teamId(), channel);
}

void SlackClient::parseChannelJoin(const QJsonObject& message)
{
    DEBUG_BLOCK

    QVariantMap data = parseChannel(message.value(QStringLiteral("channel")).toObject());
    m_storage.saveChannel(data);
    currentChatsModel()->addChat(message.value(QStringLiteral("channel")).toObject(), ChatsModel::Channel);
    emit channelJoined(m_teamInfo.teamId(), data);
}

void SlackClient::parseChannelLeft(const QJsonObject& message)
{
    DEBUG_BLOCK

    QString id = message.value(QStringLiteral("channel")).toString();
    QVariantMap channel = m_storage.channel(id);
    channel.insert(QStringLiteral("isOpen"), QVariant(false));
    m_storage.saveChannel(channel);
    emit channelLeft(m_teamInfo.teamId(), channel);
}

void SlackClient::parseGroupJoin(const QJsonObject& message)
{
    DEBUG_BLOCK

    QVariantMap data = parseGroup(message.value(QStringLiteral("channel")).toObject());
    m_storage.saveChannel(data);
    emit channelJoined(m_teamInfo.teamId(), data);
}

void SlackClient::parseChannelUpdate(const QJsonObject& message)
{
    DEBUG_BLOCK

    QString id = message.value(QStringLiteral("channel")).toString();
    QVariantMap channel = m_storage.channel(id);
    channel.insert(QStringLiteral("lastRead"), message.value(QStringLiteral("ts")).toVariant());
    channel.insert(QStringLiteral("unreadCount"), message.value(QStringLiteral("unread_count_display")).toVariant());
    m_storage.saveChannel(channel);
    emit channelUpdated(m_teamInfo.teamId(), channel);
}

void SlackClient::parseMessageUpdate(const QJsonObject& message)
{
    DEBUG_BLOCK;

    QVariantMap data;
    const QString& teamId = message.value(QStringLiteral("team_id")).toString();
    const QJsonValue& subtype = message.value(QStringLiteral("subtype"));
    if (subtype.isUndefined()) {
        data = getMessageData(message, teamId);
    } else {
        //TODO(unknown): handle messages threads
        data = getMessageData(message.value(QStringLiteral("message")).toObject(), teamId);
        if (subtype.toString() == "message_changed") {
            data[QStringLiteral("edited")] = true;
        }
    }

    QString channelId = message.value(QStringLiteral("channel")).toString();

    if (!data.value("channel").isValid()) {
        data["channel"] = QVariant(channelId);
    }

    if (m_storage.channelMessagesExist(channelId)) {
        m_storage.appendChannelMessage(channelId, data);
    }

    QVariantMap channel = m_storage.channel(channelId);

    QString messageTime = data.value(QStringLiteral("time")).toString();
    QString latestRead = channel.value(QStringLiteral("lastRead")).toString();

    if (messageTime > latestRead) {
        int unreadCount = channel.value(QStringLiteral("unreadCount")).toInt() + 1;
        channel.insert(QStringLiteral("unreadCount"), unreadCount);
        m_storage.saveChannel(channel);
        emit channelUpdated(m_teamInfo.teamId(), channel);
    }

    if (!channel.value(QStringLiteral("isOpen")).toBool()) {
        if (channel.value(QStringLiteral("type")).toString() == QStringLiteral("im")) {
            openChat(channelId);
        }
    }

    emit messageReceived(m_teamInfo.teamId(), data);
}

void SlackClient::parseReactionUpdate(const QJsonObject &message)
{
    DEBUG_BLOCK

    //"{\"type\":\"reaction_added\",\"user\":\"U4NH7TD8D\",\"item\":{\"type\":\"message\",\"channel\":\"C09PZTN5S\",\"ts\":\"1525437188.000435\"},\"reaction\":\"slightly_smiling_face\",\"item_user\":\"U3ZC1RYJG\",\"event_ts\":\"1525437431.000236\",\"ts\":\"1525437431.000236\"}"
    //"{\"type\":\"reaction_removed\",\"user\":\"U4NH7TD8D\",\"item\":{\"type\":\"message\",\"channel\":\"C0CK10FA9\",\"ts\":\"1526478339.000316\"},\"reaction\":\"+1\",\"item_user\":\"U1WTHK18E\",\"event_ts\":\"1526736040.000047\",\"ts\":\"1526736040.000047\"}"
    QVariantMap data;
    QJsonObject item = message.value(QStringLiteral("item")).toObject();
    const QString& reaction = message.value(QStringLiteral("reaction")).toString();

    QString emojiPrepare = QString(":%1:").arg(reaction);
    m_formatter.replaceEmoji(emojiPrepare);

    data.insert(QStringLiteral("type"), message.value(QStringLiteral("type")).toVariant());
    data.insert(QStringLiteral("reaction"), reaction);
    data.insert(QStringLiteral("emoji"), emojiPrepare);
    data.insert(QStringLiteral("user"),
                m_storage.user(message.value(QStringLiteral("user")).toString()).
                value(QStringLiteral("name")).toString());
    data.insert(QStringLiteral("channel"), item.value(QStringLiteral("channel")).toVariant());
    data.insert(QStringLiteral("ts"), item.value(QStringLiteral("ts")).toVariant());

    emit messageUpdated(m_teamInfo.teamId(), data);
}

void SlackClient::parsePresenceChange(const QJsonObject& message)
{
    DEBUG_BLOCK

    QVariant userId = message.value(QStringLiteral("user")).toVariant();
    QVariant presence = message.value(QStringLiteral("presence")).toVariant();

    QVariantMap user = m_storage.user(userId);
    if (!user.isEmpty()) {
        user.insert(QStringLiteral("presence"), presence);
        m_storage.saveUser(user);
        emit userUpdated(m_teamInfo.teamId(), user);
    }

    foreach (const auto item, m_storage.channels()) {
        QVariantMap channel = item.toMap();

        if (channel.value(QStringLiteral("type")) == QVariant(QStringLiteral("im")) &&
                channel.value(QStringLiteral("userId")) == userId) {
            channel.insert(QStringLiteral("presence"), presence);
            m_storage.saveChannel(channel);
            emit channelUpdated(m_teamInfo.teamId(), channel);
        }
    }
}

void SlackClient::parseNotification(const QJsonObject& message)
{
    DEBUG_BLOCK

    QString channel = message.value(QStringLiteral("subtitle")).toString();
    QString content = message.value(QStringLiteral("content")).toString();

    QString channelId = message.value(QStringLiteral("channel")).toString();
    QString title;

    if (channelId.startsWith(QLatin1Char('C')) || channelId.startsWith(QLatin1Char('G'))) {
        title = QString(tr("New message in %1")).arg(channel);
    } else if (channelId.startsWith("D")) {
        title = QString(tr("New message from %1")).arg(channel);
    } else {
        title = QString(tr("New message"));
    }

    qDebug() << "App state" << appActive << activeWindow;

    if (!appActive || activeWindow != channelId) {
        sendNotification(channelId, title, content);
    }
}

bool SlackClient::isOk(const QNetworkReply *reply)
{
    DEBUG_BLOCK

    if (!reply) {
        return false;
    }
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (statusCode / 100 == 2) {
        return true;
    } else {
        if (statusCode == 429) {
            qDebug() << "too many requests";
            for (const QByteArray &h : reply->rawHeaderList()) {
                qDebug() << h << reply->rawHeader(h);
            }
        }
        qDebug() << statusCode << reply->errorString() << reply;
        return false;
    }
}

bool SlackClient::isError(const QJsonObject &data)
{
    DEBUG_BLOCK

    if (data.isEmpty()) {
        qWarning() << "No data received";
        return true;
    } else {
        return !data.value(QStringLiteral("ok")).toBool(false);
    }
}

QJsonObject SlackClient::getResult(QNetworkReply *reply)
{
    DEBUG_BLOCK

    if (isOk(reply)) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &error);
#if 0 //dump initial
        {
            QFile f("statup_dumps.json");
            if (f.open(QIODevice::Append)) {
                f.write(document.toJson(QJsonDocument::Indented));
                f.close();
            }
        }
#endif
        if (error.error == QJsonParseError::NoError) {
            return document.object();
        } else {
            return QJsonObject();
        }
    } else {
        qWarning() << "bad";
        return QJsonObject();
    }
}

QNetworkReply *SlackClient::executeGet(const QString& method, const QMap<QString, QString>& params)
{
    DEBUG_BLOCK

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), m_teamInfo.teamToken());

    foreach (const QString key, params.keys()) {
        query.addQueryItem(key, params.value(key));
    }

    QUrl url(QStringLiteral("https://slack.com/api/") + method);
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    qDebug() << "GET" << url.toString();
    return networkAccessManager->get(request);
}

QNetworkReply *SlackClient::executePost(const QString& method, const QMap<QString, QString> &data)
{
    DEBUG_BLOCK

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), m_teamInfo.teamToken());

    foreach (const QString key, data.keys()) {
        query.addQueryItem(key, data.value(key));
    }

    QUrl params;
    params.setQuery(query);
    // replace '+' manually, since QUrl does not replacing it
    QByteArray body = params.toEncoded().replace('+', "%2B");
    body.remove(0, 1);

    QUrl url(QStringLiteral("https://slack.com/api/") + method);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, body.length());

    qDebug() << "POST" << url.toString() << body << query.toString();
    return networkAccessManager->post(request, body);
}

QNetworkReply *SlackClient::executePostWithFile(const QString& method, const QMap<QString, QString> &formdata, QFile *file)
{
    DEBUG_BLOCK

    QHttpMultiPart *dataParts = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart tokenPart;
    tokenPart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("from-data; name=\"token\""));
    tokenPart.setBody(m_teamInfo.teamToken().toUtf8());
    dataParts->append(tokenPart);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("from-data; name=\"file\"; filename=\"") + file->fileName() + "\"");
    filePart.setBodyDevice(file);
    dataParts->append(filePart);

    foreach (const QString key, formdata.keys()) {
        QHttpPart data;
        data.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"") +
                       key.toUtf8() + QLatin1Char('"'));
        data.setBody(formdata.value(key).toUtf8());
        dataParts->append(data);
    }

    QUrl url(QStringLiteral("https://slack.com/api/") + method);
    QNetworkRequest request(url);

    qDebug() << "POST" << url << dataParts;

    QNetworkReply *reply = networkAccessManager->post(request, dataParts);
    connect(reply, &QNetworkReply::finished, dataParts, &QObject::deleteLater);

    return reply;
}

void SlackClient::testLogin()
{
    DEBUG_BLOCK

    if (networkAccessible != QNetworkAccessManager::Accessible) {
        qDebug() << "Login failed no network" << networkAccessible;
        emit testConnectionFail(m_teamInfo.teamId());
        return;
    }

    if (m_teamInfo.teamToken().isEmpty()) {
        qDebug() << "Got empty token";
        emit testLoginFail(m_teamInfo.teamId());
        return;
    }

    QNetworkReply *reply = executeGet("auth.test");
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleTestLoginReply);
}

void SlackClient::handleTestLoginReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        emit testLoginFail(m_teamInfo.teamId());
        return;
    }
    qDebug().noquote() << QJsonDocument(data).toJson();

    QString teamId = data.value(QStringLiteral("team_id")).toString();
    QString userId = data.value(QStringLiteral("user_id")).toString();
    QString teamName = data.value(QStringLiteral("team")).toString();
    qDebug() << "Login success" << userId << teamId << teamName;

    config->setUserInfo(userId, teamId, teamName);
    requestTeamInfo();
    requestTeamEmojis();
    emit testLoginSuccess(userId, teamId, teamName);
    //startClient();
}

void SlackClient::searchMessages(const QString &searchString)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("query"), searchString);
    params.insert(QStringLiteral("highlight"), QStringLiteral("false"));
    params.insert(QStringLiteral("sort"), QStringLiteral("timestamp"));

    QNetworkReply *reply = executeGet(QStringLiteral("search.messages"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleSearchMessagesReply);
}

void SlackClient::handleSearchMessagesReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        emit testLoginFail(m_teamInfo.teamId());
        return;
    }
    QVariantList searchResults;
    QJsonObject messages = data.value(QStringLiteral("messages")).toObject();
    int _total = messages.value(QStringLiteral("total")).toInt();
    QJsonArray matches = messages.value(QStringLiteral("matches")).toArray();
    for (const QJsonValue& match : matches) {
        const QJsonObject& matchObj = match.toObject();
        QVariantMap searchResult = getMessageData(matchObj, matchObj.value(QStringLiteral("team")).toString());
        searchResult.insert(QStringLiteral("permalink"), matchObj.value(QStringLiteral("permalink")).toVariant());

        const QJsonObject& channelObj = matchObj.value(QStringLiteral("channel")).toObject();
        QVariantMap channel;
        channel[QStringLiteral("id")] = channelObj.value(QStringLiteral("id"));
        channel[QStringLiteral("name")] = channelObj.value(QStringLiteral("name"));
        searchResult.insert(QStringLiteral("channel"), channel);
        searchResults.append(searchResult);
    }
    qDebug() << "search result. found entries" << _total;
    emit searchResultsReady(m_teamInfo.teamId(), searchResults);
}

void SlackClient::startClient()
{
    DEBUG_BLOCK
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "startClient", Qt::QueuedConnection);
        return;
    }

    qDebug() << "Start init";
    QNetworkReply *reply = executeGet(QStringLiteral("rtm.start"));
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleStartReply);
}

void SlackClient::handleStartReply()
{
    DEBUG_BLOCK
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        qDebug() << "Start result error";
        emit disconnected(m_teamInfo.teamId());

        if (data.isEmpty()) {
            emit initFail(m_teamInfo.teamId(), tr("No data received from server"));
        } else {
            qDebug() << "data error" << data;
            emit initFail(m_teamInfo.teamId(), tr("Wat"));
        }
        return;
    }

    m_networkId = m_networksModel->addNetwork(data);
//    qDebug() << "chats count" << m_networksModel->rowCount();
    emit currentChatsModelChanged();
    qDebug() << "start reply, added network" << m_networkId;
//    qDebug() << data.keys();
//    qDebug().noquote() << QJsonDocument(data).toJson();

//    parseUsers(data);
    parseBots(data);
//    parseChannels(data);
//    parseGroups(data);

    QUrl url(data.value(QStringLiteral("url")).toString());
    stream->listen(url);

    m_storage.clearChannelMessages();
    if (m_teamInfo.lastChannel().isEmpty()) {
        m_teamInfo.setLastChannel(lastChannel());
    }
    qDebug() << "init success";
    emit initSuccess(m_teamInfo.teamId());
}

QVariantMap SlackClient::parseChannel(const QJsonObject& channel)
{
    QVariantMap data;
    data.insert(QStringLiteral("id"), channel.value(QStringLiteral("id")).toVariant());
    data.insert(QStringLiteral("type"), QVariant(QStringLiteral("channel")));
    data.insert(QStringLiteral("category"), QVariant(QStringLiteral("channel")));
    data.insert(QStringLiteral("name"), channel.value(QStringLiteral("name")).toVariant());
    data.insert(QStringLiteral("presence"), QVariant(QStringLiteral("none")));
    data.insert(QStringLiteral("isOpen"), channel.value(QStringLiteral("is_member")).toVariant());
    data.insert(QStringLiteral("lastRead"), channel.value(QStringLiteral("last_read")).toVariant());
    data.insert(QStringLiteral("unreadCount"), channel.value(QStringLiteral("unread_count_display")).toVariant());
    data.insert(QStringLiteral("members"), channel.value(QStringLiteral("members")).toVariant());
    data.insert(QStringLiteral("userId"), QVariant());
    return data;
}

QVariantMap SlackClient::parseGroup(const QJsonObject& group)
{
    DEBUG_BLOCK

    QVariantMap data;

    if (group.value(QStringLiteral("is_mpim")).toBool()) {
        data.insert(QStringLiteral("type"), QVariant(QStringLiteral("mpim")));
        data.insert(QStringLiteral("category"), QVariant(QStringLiteral("chat")));

        QStringList members;
        QJsonArray memberList = group.value(QStringLiteral("members")).toArray();
        foreach (const QJsonValue &member, memberList) {
            QVariant memberId = member.toVariant();

            if (memberId != config->userId()) {
                members << m_storage.user(memberId).value(QStringLiteral("name")).toString();
            }
        }
        data.insert(QStringLiteral("name"), QVariant(members.join(QStringLiteral(", "))));
    } else {
        data.insert(QStringLiteral("type"), QVariant(QStringLiteral("group")));
        data.insert(QStringLiteral("category"), QVariant(QStringLiteral("channel")));
        data.insert(QStringLiteral("name"), group.value(QStringLiteral("name")).toVariant());
    }

    data.insert(QStringLiteral("id"), group.value(QStringLiteral("id")).toVariant());
    data.insert(QStringLiteral("presence"), QVariant(QStringLiteral("none")));
    data.insert(QStringLiteral("isOpen"), group.value(QStringLiteral("is_open")).toVariant());
    data.insert(QStringLiteral("lastRead"), group.value(QStringLiteral("last_read")).toVariant());
    data.insert(QStringLiteral("unreadCount"), group.value(QStringLiteral("unread_count_display")).toVariant());
    data.insert(QStringLiteral("userId"), QVariant());
    return data;
}

void SlackClient::parseUser(const QJsonObject& user)
{
    QVariantMap data;
    const QString userId = user.value(QStringLiteral("id")).toString();
    data.insert(QStringLiteral("id"), userId);
    data.insert(QStringLiteral("name"), user.value(QStringLiteral("name")).toVariant());
    data.insert(QStringLiteral("presence"), user.value(QStringLiteral("presence")).toVariant());

    const QJsonObject& profile = user.value(QStringLiteral("profile")).toObject();
    m_userAvatars[userId] = QUrl(profile.value(QStringLiteral("image_72")).toString());

    m_storage.saveUser(data);
}

void SlackClient::parseUsers(const QJsonObject& data)
{
    qDebug() << "parse users";
    foreach (const QJsonValue &value, data.value(QStringLiteral("users")).toArray()) {
        parseUser(value.toObject());
    }
    m_storage.updateUsersList();
}

void SlackClient::parseBots(const QJsonObject& data)
{
    DEBUG_BLOCK

    foreach (const QJsonValue &value, data.value(QStringLiteral("bots")).toArray()) {
        QJsonObject bot = value.toObject();

        QVariantMap data;
        data.insert(QStringLiteral("id"), bot.value(QStringLiteral("id")));
        data.insert(QStringLiteral("name"), bot.value(QStringLiteral("name")));
        data.insert(QStringLiteral("presence"), QVariant(QStringLiteral("active")));
        m_storage.saveUser(data);
    }
}

void SlackClient::parseChannels(const QJsonObject& data)
{
    DEBUG_BLOCK
    qDebug() << "parse channels";

    foreach (const QJsonValue &value, data.value(QStringLiteral("channels")).toArray()) {
        QVariantMap data = parseChannel(value.toObject());
        m_storage.saveChannel(data);
    }
}

void SlackClient::parseGroups(const QJsonObject& data)
{
    DEBUG_BLOCK

    foreach (const QJsonValue &value, data.value(QStringLiteral("groups")).toArray()) {
        QVariantMap data = parseGroup(value.toObject());
        m_storage.saveChannel(data);
    }
}

void SlackClient::parseChats(const QJsonObject& data)
{
    DEBUG_BLOCK

    qDebug() << "parse chats"; // << data;
    foreach (const QJsonValue &value, data.value(QStringLiteral("ims")).toArray()) {
        QJsonObject chat = value.toObject();
        QVariantMap data;

        QVariant userId = chat.value(QStringLiteral("user")).toVariant();
        QVariantMap user = m_storage.user(userId);

        data.insert(QStringLiteral("type"), QVariant(QStringLiteral("im")));
        data.insert(QStringLiteral("category"), QVariant(QStringLiteral("chat")));
        data.insert(QStringLiteral("id"), chat.value(QStringLiteral("id")).toVariant());
        data.insert(QStringLiteral("userId"), userId);
        data.insert(QStringLiteral("name"), user.value(QStringLiteral("name")));
        data.insert(QStringLiteral("presence"), user.value(QStringLiteral("presence")));
        data.insert(QStringLiteral("isOpen"), chat.value(QStringLiteral("is_open")).toVariant());
        data.insert(QStringLiteral("lastRead"), chat.value(QStringLiteral("last_read")).toVariant());
        data.insert(QStringLiteral("unreadCount"), chat.value(QStringLiteral("unread_count_display")).toVariant());
        m_storage.saveChannel(data);
    }
}

QVariantList SlackClient::getChannels()
{
    DEBUG_BLOCK

    return m_storage.channels();
}

QVariant SlackClient::getChannel(const QString& channelId)
{
    DEBUG_BLOCK

    return m_storage.channel(QVariant(channelId));
}

QStringList SlackClient::getNickSuggestions(const QString &currentText, const int cursorPosition)
{
    DEBUG_BLOCK

    int whitespaceAfter = currentText.indexOf(' ', cursorPosition, Qt::CaseInsensitive);
    if (whitespaceAfter < cursorPosition) {
        whitespaceAfter = currentText.length();
    }

    int whitespaceBefore = currentText.lastIndexOf(' ', cursorPosition - 1, Qt::CaseInsensitive);
    if (whitespaceBefore < 0) {
        whitespaceBefore = 0;
    } else {
        whitespaceBefore++;
    }

    const QString relevant = currentText.mid(whitespaceBefore, whitespaceAfter- whitespaceBefore);

    QStringList nicks;
    for (const QString &memberId : m_storage.channel(m_teamInfo.lastChannel()).value("members").toStringList()) {
        const QString nick = m_storage.user(memberId).value("name").toString();
        if (relevant.isEmpty()) {
            nicks.append(nick);
        } else if (nick.contains(relevant, Qt::CaseInsensitive)) {
            nicks.append(nick);
        }
    }

    return nicks;
}

ChatsModel *SlackClient::currentChatsModel()
{
    return m_networksModel->chatsModel(m_networkId);
}

bool SlackClient::isOnline() const
{
    DEBUG_BLOCK

            return stream && stream->isConnected();
}

bool SlackClient::isDevice() const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

QString SlackClient::lastChannel()
{
    DEBUG_BLOCK
    QString _lastChannel;
    const QVariantList& chList = m_storage.channels();

    if (!chList.isEmpty()) {
        _lastChannel = chList.first().toMap()[QStringLiteral("id")].toString();
    }

    if (!m_teamInfo.lastChannel().isEmpty()) {
        _lastChannel = m_teamInfo.lastChannel();
    }
    //qDebug() << "last channel" << _lastChannel;
    return _lastChannel;
}

QString SlackClient::historyMethod(const ChatsModel::ChatType type)
{
    DEBUG_BLOCK

    if (type == ChatsModel::Channel) {
        return QStringLiteral("channels.history");
    } else if (type == ChatsModel::Group) {
        return QStringLiteral("groups.history");
    } else if (type == QStringLiteral("mpim")) {
        return QStringLiteral("mpim.history");
    } else if (type == ChatsModel::Conversation) {
        return QStringLiteral("im.history");
    } else {
        return QStringLiteral("");
    }
}

void SlackClient::joinChannel(const QString& channelId)
{
    DEBUG_BLOCK

    QVariantMap channel = m_storage.channel(QVariant(channelId));

    QMap<QString, QString> params;
    params.insert(QStringLiteral("name"), channel.value(QStringLiteral("name")).toString());

    QNetworkReply *reply = executeGet(QStringLiteral("channels.join"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleJoinChannelReply);
}

void SlackClient::handleJoinChannelReply()
{
    DEBUG_BLOCK

    qDebug() << "join reply";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Channel join failed";
    }

    reply->deleteLater();
}

void SlackClient::leaveChannel(const QString& channelId)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);

    QNetworkReply *reply = executeGet(QStringLiteral("channels.leave"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleLeaveChannelReply);
}

void SlackClient::handleLeaveChannelReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Channel leave failed";
    }

    reply->deleteLater();
}

void SlackClient::leaveGroup(const QString& groupId)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), groupId);

    QNetworkReply *reply = executeGet(QStringLiteral("groups.leave"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleLeaveGroupReply);
}

void SlackClient::handleLeaveGroupReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Group leave failed";
    }

    reply->deleteLater();
}

void SlackClient::openChat(const QString& chatId)
{
    DEBUG_BLOCK

    QVariantMap channel = m_storage.channel(QVariant(chatId));

    QMap<QString, QString> params;
    params.insert(QStringLiteral("user"), channel.value(QStringLiteral("userId")).toString());

    QNetworkReply *reply = executeGet(QStringLiteral("im.open"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleOpenChatReply);
}

void SlackClient::handleOpenChatReply()
{
    DEBUG_BLOCK

    qDebug() << "open chat reply";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Chat open failed";
    }

    reply->deleteLater();
}

void SlackClient::closeChat(const QString& chatId)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), chatId);

    QNetworkReply *reply = executeGet(QStringLiteral("im.close"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleCloseChatReply);
}


void SlackClient::handleCloseChatReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Chat close failed";
    }

    reply->deleteLater();
}

void SlackClient::requestTeamInfo()
{
    QNetworkReply *reply = executeGet(QStringLiteral("team.info"));
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleTeamInfoReply);
}

void SlackClient::requestTeamEmojis()
{
    QNetworkReply *reply = executeGet(QStringLiteral("emoji.list"));
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleTeamEmojisReply);
}

void SlackClient::handleTeamInfoReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        qDebug() << "Team info failed";
    }
    //qDebug() << "teaminfo:" << data;
    QJsonObject teamObj = data.value(QStringLiteral("team")).toObject();

    QString _id = teamObj.value(QStringLiteral("id")).toString();

    m_teamInfo.setTeamId(_id);
    m_teamInfo.setName(teamObj.value(QStringLiteral("name")).toString());
    m_teamInfo.setDomain(teamObj.value(QStringLiteral("domain")).toString());
    m_teamInfo.setEmailDomain(teamObj.value(QStringLiteral("email_domain")).toString());
    m_teamInfo.setEnterpriseId(teamObj.value(QStringLiteral("enterprise_id")).toString());
    m_teamInfo.setEnterpriseName(teamObj.value(QStringLiteral("enterprise_name")).toString());
    QJsonObject teamIconObj = teamObj.value(QStringLiteral("icon")).toObject();
    m_teamInfo.setIcons(QStringList() << teamIconObj.value(QStringLiteral("image_34")).toString()
    << teamIconObj.value(QStringLiteral("image_44")).toString()
    << teamIconObj.value(QStringLiteral("image_68")).toString()
    << teamIconObj.value(QStringLiteral("image_88")).toString()
    << teamIconObj.value(QStringLiteral("image_102")).toString()
    << teamIconObj.value(QStringLiteral("image_132")).toString()
    << teamIconObj.value(QStringLiteral("image_230")).toString()
    << teamIconObj.value(QStringLiteral("image_original")).toString());

    config->saveTeamInfo(m_teamInfo);
    emit teamInfoChanged(m_teamInfo.teamId());
}

void SlackClient::handleTeamEmojisReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        qDebug() << "Team info failed";
    }

    ImagesCache* imagesCache = ImagesCache::instance();
    QJsonObject teamEmojisArr = data.value(QStringLiteral("emoji")).toObject();
    //qDebug() << "team emojis:" << data << teamEmojisArr.count();
    for (const QString &name : teamEmojisArr.keys()) {
        QString emoji_url = teamEmojisArr.value(name).toString();
        if (emoji_url.startsWith("alias:")) {
            imagesCache->addEmojiAlias(name, emoji_url.remove(0, 6));
        } else {
            EmojiInfo *einfo = new EmojiInfo;
            einfo->m_name = name;
            einfo->m_shortNames << name;
            einfo->m_image = emoji_url;
            einfo->m_imagesExist |= EmojiInfo::ImageSlackTeam;
            einfo->m_category = EmojiInfo::EmojiCategoryCustom;
            einfo->m_teamId = m_teamInfo.teamId();
            //qDebug() << "edding emoji" << einfo->m_shortNames << einfo->m_image << einfo->m_category;
            imagesCache->addEmoji(einfo);
        }
    }
    imagesCache->sendEmojisUpdated();
}

void SlackClient::loadMessages(const ChatsModel::ChatType type, const QString& channelId)
{
    DEBUG_BLOCK;
    qDebug() << "Loading message" << type << channelId;
    if (channelId.isEmpty()) {
        qWarning() << "Empty channel id";
        return;
    }

    if (m_storage.channelMessagesExist(channelId)) {
        QVariantList messages = m_storage.channelMessages(channelId);
        emit loadMessagesSuccess(m_teamInfo.teamId(), channelId, messages);
        return;
    }

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);

    QNetworkReply *reply = executeGet(historyMethod(type), params);
    reply->setProperty("channelId", channelId);

    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleLoadMessagesReply);
}

void SlackClient::handleLoadMessagesReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        emit loadMessagesFail(m_teamInfo.teamId());
        return;
    }

    QJsonArray messageList = data.value(QStringLiteral("messages")).toArray();

    QString channelId = reply->property("channelId").toString();
    ChatsModel *chatModel = m_networksModel->chatsModel(m_networkId);
    if (!chatModel) {
        qWarning() << "Network" << m_networkId << "does not exist";
        return;
    }

    if (!chatModel->hasChannel(channelId)) {
        qWarning() << "Network" << m_networkId << "does not have channel" << channelId;
        return;
    }

    MessageListModel *messageModel = chatModel->messages(channelId);
    messageModel->addMessages(messageList);

    foreach (const QJsonValue &value, messageList) {
        QJsonObject messageData = value.toObject();
//        Message message(messageData);
//        message.user = chatModel->members(channelId)->user(messageData[QStringLiteral("user")]);

        messages << getMessageData(messageData, m_teamInfo.teamId());
    }
    m_storage.setChannelMessages(channelId, messages);

    emit loadMessagesSuccess(m_teamInfo.teamId(), channelId, messages);
}

QString SlackClient::markMethod(const QString& type)
{
    DEBUG_BLOCK

    if (type == QStringLiteral("channel")) {
        return QStringLiteral("channels.mark");
    } else if (type == QStringLiteral("group")) {
        return QStringLiteral("groups.mark");
    } else if (type == QStringLiteral("mpim")) {
        return QStringLiteral("mpim.mark");
    } else if (type == QStringLiteral("im")) {
        return QStringLiteral("im.mark");
    } else {
        return "";
    }
}

SlackClient::ClientStates SlackClient::getState() const
{
    return m_state;
}

void SlackClient::setState(ClientStates state)
{
    if (state != m_state) {
        m_state = state;
        emit stateChanged(m_teamInfo.teamId());
    }
}

TeamInfo *SlackClient::teamInfo()
{
    return &m_teamInfo;
}

void SlackClient::markChannel(const QString& type, const QString& channelId, const QString& time)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);
    params.insert(QStringLiteral("ts"), time);

    QNetworkReply *reply = executeGet(markMethod(type), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleMarkChannelReply);
}

void SlackClient::handleMarkChannelReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    //qDebug() << "Mark message result" << data;

    reply->deleteLater();
}

void SlackClient::deleteReaction(const QString& channelId, const QString& ts, const QString& reaction)
{
    DEBUG_BLOCK

    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("name"), reaction);
    data.insert(QStringLiteral("timestamp"), ts);

    QNetworkReply *reply = executePost(QStringLiteral("reactions.remove"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleDeleteReactionReply);
}

void SlackClient::addReaction(const QString &channelId, const QString &ts, const QString &reaction)
{
    DEBUG_BLOCK

    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("name"), reaction);
    data.insert(QStringLiteral("timestamp"), ts);

    QNetworkReply *reply = executePost(QStringLiteral("reactions.add"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleAddReactionReply);
}

void SlackClient::postMessage(const QString& channelId, QString content)
{
    DEBUG_BLOCK

    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("text"), content);
    data.insert(QStringLiteral("as_user"), QStringLiteral("true"));
    data.insert(QStringLiteral("parse"), QStringLiteral("full"));

    QNetworkReply *reply = executePost(QStringLiteral("chat.postMessage"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handlePostMessageReply);
}

void SlackClient::handleDeleteReactionReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Delete reaction result" << data;

    reply->deleteLater();
}

void SlackClient::handleAddReactionReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Add reaction result" << data;

    reply->deleteLater();
}

void SlackClient::handlePostMessageReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Post message result" << data;

    reply->deleteLater();
}

void SlackClient::postImage(const QString& channelId, const QString& imagePath,
                            const QString& title, const QString& comment)
{
    DEBUG_BLOCK

    QMap<QString, QString> data;
    data.insert(QStringLiteral("channels"), channelId);

    if (!title.isEmpty()) {
        data.insert(QStringLiteral("title"), title);
    }

    if (!comment.isEmpty()) {
        data.insert(QStringLiteral("initial_comment"), comment);
    }

    QFile *imageFile = new QFile(imagePath);
    if (!imageFile->open(QFile::ReadOnly)) {
        qWarning() << "image file not readable" << imagePath;
        return;
    }

    qDebug() << "sending image" << imagePath;
    QNetworkReply *reply = executePostWithFile(QStringLiteral("files.upload"), data, imageFile);

    connect(reply, &QNetworkReply::finished, this, &SlackClient::handlePostImage);
    connect(reply, &QNetworkReply::finished, imageFile, &QObject::deleteLater);
}

void SlackClient::handlePostImage()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Post image result" << data;

    if (isError(data)) {
        emit postImageFail(m_teamInfo.teamId());
    } else {
        emit postImageSuccess(m_teamInfo.teamId());
    }

    reply->deleteLater();
}

QVariantMap SlackClient::getMessageData(const QJsonObject& message, const QString& teamId)
{
    DEBUG_BLOCK

    QVariantMap data;
    data.insert(QStringLiteral("type"), message.value(QStringLiteral("type")).toVariant());
    data.insert(QStringLiteral("time"), message.value(QStringLiteral("ts")).toVariant());
    data.insert(QStringLiteral("channel"), message.value(QStringLiteral("channel")).toVariant());
    data.insert(QStringLiteral("user"), user(message));
    data.insert(QStringLiteral("attachments"), getAttachments(message));
    data.insert(QStringLiteral("fileShares"), getFileShares(message));
    data.insert(QStringLiteral("content"), QVariant(getContent(message)));
    data.insert(QStringLiteral("reactions"), getReactions(message));
    data.insert(QStringLiteral("teamid"), teamId);
    data.insert(QStringLiteral("permalink"), "");

    bool _edited = false;
    if (!message.value(QStringLiteral("edited")).isUndefined()) {
        _edited = true;
    }
    data.insert(QStringLiteral("edited"), _edited);

    return data;
}

QVariantMap SlackClient::user(const QJsonObject &data)
{
    DEBUG_BLOCK

    QString type = data.value(QStringLiteral("subtype")).toString(QStringLiteral("default"));
    QVariant userId;

    if (type == QStringLiteral("bot_message")) {
        userId = data.value(QStringLiteral("bot_id")).toVariant();
    } else {
        userId = data.value(QStringLiteral("user")).toVariant();
    }

    QVariantMap userData = m_storage.user(userId);

    if (userData.isEmpty()) {
        userData.insert(QStringLiteral("id"), data.value(QStringLiteral("user")).toVariant());
        userData.insert(QStringLiteral("name"), QVariant(QStringLiteral("Unknown")));
        userData.insert(QStringLiteral("presence"), QVariant(QStringLiteral("away")));
    }

    QString username = data.value(QStringLiteral("username")).toString();
    if (!username.isEmpty()) {
        QRegularExpression newUserPattern(QStringLiteral("<@([A-Z0-9]+)\\|([^>]+)>"));
        username.replace(newUserPattern, QStringLiteral("\\2"));
        userData.insert(QStringLiteral("name"), username);
    }

    return userData;
}

QString SlackClient::getContent(const QJsonObject& message)
{
    DEBUG_BLOCK

    QString content = message.value(QStringLiteral("text")).toString();

    findNewUsers(content);
    m_formatter.replaceUserInfo(m_storage.users(), content);
    m_formatter.replaceTargetInfo(content);
    m_formatter.replaceChannelInfo(m_storage.channels(), content);
    m_formatter.replaceLinks(content);
    m_formatter.replaceSpecialCharacters(content);
    m_formatter.replaceMarkdown(content);
    m_formatter.replaceEmoji(content);

    return content;
}

QVariantList SlackClient::getFileShares(const QJsonObject& message)
{
    DEBUG_BLOCK

    QVariantList images;

    if (message.value(QStringLiteral("subtype")).toString() == QStringLiteral("file_share")) {
        //qDebug() << "file share json:" << message;

        QJsonObject file = message.value(QStringLiteral("file")).toObject();
        QString fileType = file.value(QStringLiteral("filetype")).toString();

        QString thumbItem = file.contains(QStringLiteral("thumb_480")) ?
                    QStringLiteral("480") :
                    QStringLiteral("360");

        QVariantMap thumbSize;
        thumbSize.insert(QStringLiteral("width"), file.value("thumb_" + thumbItem + "_w").toVariant());
        thumbSize.insert(QStringLiteral("height"), file.value("thumb_" + thumbItem + "_h").toVariant());

        QVariantMap imageSize;
        imageSize.insert(QStringLiteral("width"), file.value("original_w").toVariant());
        imageSize.insert(QStringLiteral("height"), file.value("original_h").toVariant());

        QVariantMap fileData;
        fileData.insert(QStringLiteral("filetype"), fileType);
        fileData.insert(QStringLiteral("name"), file.value(QStringLiteral("name")).toVariant());
        fileData.insert(QStringLiteral("url"), file.value(QStringLiteral("url_private")).toVariant());
        fileData.insert(QStringLiteral("url_download"), file.value(QStringLiteral("url_private_download")).toVariant());
        fileData.insert(QStringLiteral("size"), imageSize);
        fileData.insert(QStringLiteral("thumbSize"), thumbSize);
        fileData.insert(QStringLiteral("preview_highlight"), file.value(QStringLiteral("preview_highlight")).toVariant());
        if (!file.value(QStringLiteral("thumb_video")).isUndefined()) {
            fileData.insert(QStringLiteral("thumbUrl"), file.value("thumb_video").toVariant());
        } else {
            fileData.insert(QStringLiteral("thumbUrl"), file.value("thumb_" + thumbItem).toVariant());
        }
        fileData.insert(QStringLiteral("mimetype"), file.value("mimetype").toVariant());

        images.append(fileData);
        //qDebug() << "images" << fileData;
    }

    return images;
}

QVariantList SlackClient::getAttachments(const QJsonObject& message)
{
    DEBUG_BLOCK

    QJsonArray attachementList = message.value(QStringLiteral("attachments")).toArray();
    QVariantList attachments;

    foreach (const QJsonValue &value, attachementList) {
        QJsonObject attachment = value.toObject();
        QVariantMap data;

        QString titleLink = attachment.value(QStringLiteral("title_link")).toString();
        QString title = attachment.value(QStringLiteral("title")).toString();
        QString pretext = attachment.value(QStringLiteral("pretext")).toString();
        QString text = attachment.value(QStringLiteral("text")).toString();
        QString fallback = attachment.value(QStringLiteral("fallback")).toString();
        QString color = getAttachmentColor(attachment);
        QVariantList fields = getAttachmentFields(attachment);
        QVariantList images = getAttachmentImages(attachment);

        m_formatter.replaceLinks(pretext);
        m_formatter.replaceLinks(text);
        m_formatter.replaceLinks(fallback);
        m_formatter.replaceEmoji(text);
        m_formatter.replaceEmoji(pretext);
        m_formatter.replaceEmoji(fallback);
        m_formatter.replaceSpecialCharacters(text);
        m_formatter.replaceSpecialCharacters(pretext);
        m_formatter.replaceSpecialCharacters(fallback);

        int index = text.indexOf(' ', 250);
        if (index > 0) {
            text = text.left(index) + "...";
        }

        if (!title.isEmpty() && !titleLink.isEmpty()) {
            title = "<a href=\"" + titleLink + "\">" + title + "</a>";
        }

        data.insert(QStringLiteral("title"), QVariant(title));
        data.insert(QStringLiteral("pretext"), QVariant(pretext));
        data.insert(QStringLiteral("content"), QVariant(text));
        data.insert(QStringLiteral("fallback"), QVariant(fallback));
        data.insert(QStringLiteral("indicatorColor"), QVariant(color));
        data.insert(QStringLiteral("fields"), QVariant(fields));
        data.insert(QStringLiteral("images"), QVariant(images));

        attachments.append(data);
    }

    return attachments;
}

QString SlackClient::getAttachmentColor(const QJsonObject& attachment)
{
    DEBUG_BLOCK

    QString color = attachment.value(QStringLiteral("color")).toString();

    if (color.isEmpty()) {
        color = QStringLiteral("theme");
    } else if (color == QStringLiteral("good")) {
        color = QStringLiteral("#6CC644");
    } else if (color == QStringLiteral("warning")) {
        color = QStringLiteral("#E67E22");
    } else if (color == QStringLiteral("danger")) {
        color = QStringLiteral("#D00000");
    } else if (!color.startsWith("#")) {
        color = "#" + color;
    }

    return color;
}

QVariantList SlackClient::getAttachmentFields(const QJsonObject& attachment)
{
    DEBUG_BLOCK

    QVariantList fields;
    if (attachment.contains(QStringLiteral("fields"))) {
        QJsonArray fieldList = attachment.value(QStringLiteral("fields")).toArray();

        foreach (const QJsonValue &fieldValue, fieldList) {
            QJsonObject field = fieldValue.toObject();
            QString title = field.value(QStringLiteral("title")).toString();
            QString value = field.value(QStringLiteral("value")).toString();
            bool isShort = field.value(QStringLiteral("short")).toBool();

            if (!title.isEmpty()) {
                m_formatter.replaceLinks(title);
                m_formatter.replaceMarkdown(title);
                m_formatter.replaceSpecialCharacters(title);

                QVariantMap titleData;
                titleData.insert(QStringLiteral("isTitle"), QVariant(true));
                titleData.insert(QStringLiteral("isShort"), QVariant(isShort));
                titleData.insert(QStringLiteral("content"), QVariant(title));
                fields.append(titleData);
            }

            if (!value.isEmpty()) {
                m_formatter.replaceLinks(value);
                m_formatter.replaceMarkdown(value);
                m_formatter.replaceSpecialCharacters(title);

                QVariantMap valueData;
                valueData.insert(QStringLiteral("isTitle"), QVariant(false));
                valueData.insert(QStringLiteral("isShort"), QVariant(isShort));
                valueData.insert(QStringLiteral("content"), QVariant(value));
                fields.append(valueData);
            }
        }
    }

    return fields;
}

QVariantList SlackClient::getAttachmentImages(const QJsonObject& attachment)
{
    DEBUG_BLOCK

    QVariantList images;

    if (attachment.contains(QStringLiteral("image_url"))) {
        QVariantMap size;
        size.insert(QStringLiteral("width"), attachment.value(QStringLiteral("image_width")));
        size.insert(QStringLiteral("height"), attachment.value(QStringLiteral("image_height")));

        QVariantMap image;
        image.insert(QStringLiteral("url"), attachment.value(QStringLiteral("image_url")));
        image.insert(QStringLiteral("size"), size);

        images.append(image);
    }

    return images;
}

QVariantList SlackClient::getReactions(const QJsonObject &message)
{
    DEBUG_BLOCK

    QJsonArray reactionsList = message.value(QStringLiteral("reactions")).toArray();
    QVariantList reactions;

    foreach (const QJsonValue &value, reactionsList) {
        QJsonObject reaction = value.toObject();
        QVariantMap data;

        QString name = reaction.value(QStringLiteral("name")).toString();
        int count = reaction.value(QStringLiteral("count")).toInt();
        QJsonArray usersList = reaction.value(QStringLiteral("users")).toArray();
        QString users;
        for (int i = 0; i < usersList.count(); i++) {
            const QJsonValue &uservalue = usersList.at(i);
            users.append(m_storage.user(uservalue.toString()).value("name").toString());
            if (i < usersList.count() - 1) {
                users.append("\n");
            }
        }

        QString emojiPrepare = QString(":%1:").arg(name);
        m_formatter.replaceEmoji(emojiPrepare);

        data.insert(QStringLiteral("name"), QVariant(name));
        data.insert(QStringLiteral("emoji"), QVariant(emojiPrepare));
        data.insert(QStringLiteral("reactionscount"), QVariant(count));
        data.insert(QStringLiteral("users"), QVariant(users));

        reactions.append(data);
    }

    return reactions;
}

void SlackClient::findNewUsers(const QString &message)
{
    DEBUG_BLOCK

    QRegularExpression newUserPattern(QStringLiteral("<@([A-Z0-9]+)\\|([^>]+)>"));

    QRegularExpressionMatchIterator i = newUserPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString id = match.captured(1);

        if (m_storage.user(id).isEmpty()) {
            QString name = match.captured(2);
            QVariantMap data;
            data.insert(QStringLiteral("id"), QVariant(id));
            data.insert(QStringLiteral("name"), QVariant(name));
            data.insert(QStringLiteral("presence"), QVariant(QStringLiteral("active")));
            m_storage.saveUser(data);
        }
    }
}

void SlackClient::sendNotification(const QString& channelId, const QString& title, const QString& text)
{
    DEBUG_BLOCK

    Q_UNUSED(title);

    QString body = text.length() > 100 ? text.left(97) + "..." : text;
    QString preview = text.length() > 40 ? text.left(37) + "..." : text;

    QVariantList arguments;
    arguments.append(channelId);

    //    Notification notification;
    //    notification.setAppName("Slaq");
    //    notification.setAppIcon("slaq");
    //    notification.setBody(body);
    //    notification.setPreviewSummary(title);
    //    notification.setPreviewBody(preview);
    //    notification.setCategory("chat");
    //    notification.setHintValue("x-slaq-channel", channelId);
    //    notification.setHintValue("x-nemo-feedback", "chat_exists");
    //    notification.setHintValue("x-nemo-priority", 100);
    //    notification.setHintValue("x-nemo-display-on", true);
    //    notification.setRemoteAction(Notification::remoteAction("default", "", "slaq", "/", "slaq", "activate", arguments));
    //    notification.publish();
}
