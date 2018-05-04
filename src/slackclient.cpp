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
#include "storage.h"
#include "messageformatter.h"

SlackClient::SlackClient(QObject *parent) :
    QObject(parent), appActive(true), activeWindow("init"), networkAccessible(QNetworkAccessManager::Accessible),
    m_clientId(QString::fromLatin1(QByteArray::fromBase64("MTE5MDczMjc1MDUuMjUyMzc1NTU3MTU1"))),
    m_clientId2(QString::fromLatin1(QByteArray::fromBase64("MGJlNDA0M2Q2OGIxYjM0MzE4ODk5ZDEwYTNiYmM3ZTY=")))
{
    networkAccessManager = new QNetworkAccessManager(this);
    config = new SlackConfig(this);
    stream = new SlackStream(this);
    reconnectTimer = new QTimer(this);
    networkAccessible = networkAccessManager->networkAccessible();

    connect(networkAccessManager.data(), &QNetworkAccessManager::networkAccessibleChanged, this, &SlackClient::handleNetworkAccessibleChanged);
    connect(reconnectTimer.data(), &QTimer::timeout, this, &SlackClient::reconnect);

    connect(stream.data(), &SlackStream::connected, this, &SlackClient::handleStreamStart);
    connect(stream.data(), &SlackStream::disconnected, this, &SlackClient::handleStreamEnd);
    connect(stream.data(), &SlackStream::messageReceived, this, &SlackClient::handleStreamMessage);

    connect(this, &SlackClient::connected, this, &SlackClient::isOnlineChanged);
    connect(this, &SlackClient::initSuccess, this, &SlackClient::isOnlineChanged);
    connect(this, &SlackClient::disconnected, this, &SlackClient::isOnlineChanged);
}

SlackClient::~SlackClient()
{
    QSettings settings;
    settings.setValue(QStringLiteral("LastChannel"), m_lastChannel);
}

void SlackClient::setAppActive(bool active)
{
    appActive = active;
    clearNotifications();
}

void SlackClient::setActiveWindow(const QString& windowId)
{
    if (windowId == activeWindow) {
        return;
    }

    activeWindow = windowId;
    clearNotifications();

    if (!windowId.isEmpty()) {
        m_lastChannel = windowId;
        emit lastChannelChanged();
    }
}

void SlackClient::clearNotifications()
{
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
    qDebug() << "Network accessible changed" << accessible;
    networkAccessible = accessible;

    if (networkAccessible == QNetworkAccessManager::Accessible) {
        emit networkOn();
    } else {
        emit networkOff();
    }
}

void SlackClient::reconnect()
{
    qDebug() << "Reconnecting";
    emit reconnecting();
    start();
}

void SlackClient::handleStreamStart()
{
    qDebug() << "Stream started";
    emit connected();
}

void SlackClient::handleStreamEnd()
{
    qDebug() << "Stream ended";
    emit reconnecting();
    reconnectTimer->setSingleShot(true);
    reconnectTimer->start(1000);
}

//TODO:
//json of reaction added
//"{\"type\":\"reaction_added\",\"user\":\"U4NH7TD8D\",\"item\":{\"type\":\"message\",\"channel\":\"C09PZTN5S\",\"ts\":\"1525437188.000435\"},\"reaction\":\"slightly_smiling_face\",\"item_user\":\"U3ZC1RYJG\",\"event_ts\":\"1525437431.000236\",\"ts\":\"1525437431.000236\"}"
//"{\"type\":\"pref_change\",\"name\":\"emoji_use\",\"value\":\"{\\\"+1\\\":16,\\\"slightly_smiling_face\\\":11,\\\"stuck_out_tongue_winking_eye\\\":1,\\\"disappointed\\\":2,\\\"stuck_out_tongue\\\":2,\\\"grinning\\\":1,\\\"ok_hand\\\":1,\\\"sunglasses\\\":1,\\\"coffee\\\":1}\",\"event_ts\":\"1525437431.000226\"}"
void SlackClient::handleStreamMessage(const QJsonObject& message)
{
    const QString& type = message.value(QStringLiteral("type")).toString();

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
    }
}

void SlackClient::parseChatOpen(const QJsonObject& message)
{
    QString id = message.value(QStringLiteral("channel")).toString();
    QVariantMap channel = Storage::channel(id);
    channel.insert(QStringLiteral("isOpen"), QVariant(true));
    Storage::saveChannel(channel);
    emit channelJoined(channel);
}

void SlackClient::parseChatClose(const QJsonObject& message)
{
    QString id = message.value(QStringLiteral("channel")).toString();
    QVariantMap channel = Storage::channel(id);
    channel.insert(QStringLiteral("isOpen"), QVariant(false));
    Storage::saveChannel(channel);
    emit channelLeft(channel);
}

void SlackClient::parseChannelJoin(const QJsonObject& message)
{
    QVariantMap data = parseChannel(message.value(QStringLiteral("channel")).toObject());
    Storage::saveChannel(data);
    emit channelJoined(data);
}

void SlackClient::parseChannelLeft(const QJsonObject& message)
{
    QString id = message.value(QStringLiteral("channel")).toString();
    QVariantMap channel = Storage::channel(id);
    channel.insert(QStringLiteral("isOpen"), QVariant(false));
    Storage::saveChannel(channel);
    emit channelLeft(channel);
}

void SlackClient::parseGroupJoin(const QJsonObject& message)
{
    QVariantMap data = parseGroup(message.value(QStringLiteral("channel")).toObject());
    Storage::saveChannel(data);
    emit channelJoined(data);
}

void SlackClient::parseChannelUpdate(const QJsonObject& message)
{
    QString id = message.value(QStringLiteral("channel")).toString();
    QVariantMap channel = Storage::channel(id);
    channel.insert(QStringLiteral("lastRead"), message.value(QStringLiteral("ts")).toVariant());
    channel.insert(QStringLiteral("unreadCount"), message.value(QStringLiteral("unread_count_display")).toVariant());
    Storage::saveChannel(channel);
    emit channelUpdated(channel);
}

void SlackClient::parseMessageUpdate(const QJsonObject& message)
{
    QVariantMap data = getMessageData(message);

    QString channelId = message.value(QStringLiteral("channel")).toString();
    if (Storage::channelMessagesExist(channelId)) {
        Storage::appendChannelMessage(channelId, data);
    }

    QVariantMap channel = Storage::channel(channelId);

    QString messageTime = data.value(QStringLiteral("time")).toString();
    QString latestRead = channel.value(QStringLiteral("lastRead")).toString();

    if (messageTime > latestRead) {
        int unreadCount = channel.value(QStringLiteral("unreadCount")).toInt() + 1;
        channel.insert(QStringLiteral("unreadCount"), unreadCount);
        Storage::saveChannel(channel);
        emit channelUpdated(channel);
    }

    if (channel.value(QStringLiteral("isOpen")).toBool() == false) {
        if (channel.value(QStringLiteral("type")).toString() == QStringLiteral("im")) {
            openChat(channelId);
        }
    }

    emit messageReceived(data);
}

void SlackClient::parsePresenceChange(const QJsonObject& message)
{
    QVariant userId = message.value(QStringLiteral("user")).toVariant();
    QVariant presence = message.value(QStringLiteral("presence")).toVariant();

    QVariantMap user = Storage::user(userId);
    if (!user.isEmpty()) {
        user.insert(QStringLiteral("presence"), presence);
        Storage::saveUser(user);
        emit userUpdated(user);
    }

    foreach (const auto item, Storage::channels()) {
        QVariantMap channel = item.toMap();

        if (channel.value(QStringLiteral("type")) == QVariant(QStringLiteral("im")) &&
                channel.value(QStringLiteral("userId")) == userId) {
            channel.insert(QStringLiteral("presence"), presence);
            Storage::saveChannel(channel);
            emit channelUpdated(channel);
        }
    }
}

void SlackClient::parseNotification(const QJsonObject& message)
{
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
    if (data.isEmpty()) {
        qWarning() << "No data received";
        return true;
    } else {
        return !data.value(QStringLiteral("ok")).toBool(false);
    }
}

QJsonObject SlackClient::getResult(QNetworkReply *reply)
{
    if (isOk(reply)) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &error);
//        qDebug().noquote() << document.toJson();

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
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), config->accessToken());

    foreach (const QString key, params.keys()) {
        query.addQueryItem(key, params.value(key));
    }

    QUrl url(QStringLiteral("https://slack.com/api/") + method);
    url.setQuery(query);
    QNetworkRequest request(url);

    qDebug() << "GET" << url.toString();
    return networkAccessManager->get(request);
}

QNetworkReply *SlackClient::executePost(const QString& method, const QMap<QString, QString> &data)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), config->accessToken());

    foreach (const QString key, data.keys()) {
        query.addQueryItem(key, data.value(key));
    }

    QUrl params;
    params.setQuery(query);
    QByteArray body = params.toEncoded(QUrl::EncodeUnicode | QUrl::EncodeReserved);
    body.remove(0, 1);

    QUrl url(QStringLiteral("https://slack.com/api/") + method);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, body.length());

    qDebug() << "POST" << url.toString() << body;
    return networkAccessManager->post(request, body);
}

QNetworkReply *SlackClient::executePostWithFile(const QString& method, const QMap<QString, QString> &formdata, QFile *file)
{
    QHttpMultiPart *dataParts = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart tokenPart;
    tokenPart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("from-data; name=\"token\""));
    tokenPart.setBody(config->accessToken().toUtf8());
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

void SlackClient::fetchAccessToken(const QUrl& resultUrl)
{
    QUrlQuery resultQuery(resultUrl);
    QString code = resultQuery.queryItemValue(QStringLiteral("code"));

    if (code.isEmpty()) {
        emit accessTokenFail();
        return;
    }

    QMap<QString, QString> params;
    params.insert(QStringLiteral("client_id"), m_clientId);
    params.insert(QStringLiteral("client_secret"), m_clientId2);
    params.insert(QStringLiteral("code"), code);

    QNetworkReply *reply = executeGet(QStringLiteral("oauth.access"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleAccessTokenReply);
}

void SlackClient::handleAccessTokenReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        reply->deleteLater();
        emit accessTokenFail();
        return;
    }

    QString accessToken = data.value(QStringLiteral("access_token")).toString();
    QString teamId = data.value(QStringLiteral("team_id")).toString();
    QString userId = data.value(QStringLiteral("user_id")).toString();
    QString teamName = data.value(QStringLiteral("team_name")).toString();
    qDebug() << "Access token success" << accessToken << userId << teamId << teamName;

    config->setAccessToken(accessToken);
    config->setUserId(userId);

    emit accessTokenSuccess(userId, teamId, teamName);

    reply->deleteLater();
}

void SlackClient::testLogin()
{
    if (networkAccessible != QNetworkAccessManager::Accessible) {
        qDebug() << "Login failed no network" << networkAccessible;
        emit testConnectionFail();
        return;
    }

    QString token = config->accessToken();
    if (token.isEmpty()) {
        qDebug() << "Got empty token";
        emit testLoginFail();
        return;
    }

    QNetworkReply *reply = executeGet("auth.test");
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleTestLoginReply);
}

void SlackClient::handleTestLoginReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        reply->deleteLater();
        emit testLoginFail();
        return;
    }

    QString teamId = data.value(QStringLiteral("team_id")).toString();
    QString userId = data.value(QStringLiteral("user_id")).toString();
    QString teamName = data.value(QStringLiteral("team")).toString();
    qDebug() << "Login success" << userId << teamId << teamName;

    config->setUserId(userId);

    emit testLoginSuccess(userId, teamId, teamName);
    reply->deleteLater();
}

void SlackClient::start()
{
    qDebug() << "Start init";
    QNetworkReply *reply = executeGet(QStringLiteral("rtm.start"));
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleStartReply);
}

void SlackClient::handleStartReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Start result error";
        reply->deleteLater();
        emit disconnected();

        if (data.isEmpty()) {
            emit initFail(tr("No data received from server"));
        } else {
            qDebug() << data;
            emit initFail(tr("Wat"));
        }
        return;
    }

    parseUsers(data);
    parseBots(data);
    parseChannels(data);
    parseGroups(data);
    parseChats(data);

    QUrl url(data.value(QStringLiteral("url")).toString());
    stream->listen(url);

    Storage::clearChannelMessages();
    emit initSuccess();

    reply->deleteLater();
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
    QVariantMap data;

    if (group.value(QStringLiteral("is_mpim")).toBool()) {
        data.insert(QStringLiteral("type"), QVariant(QStringLiteral("mpim")));
        data.insert(QStringLiteral("category"), QVariant(QStringLiteral("chat")));

        QStringList members;
        QJsonArray memberList = group.value(QStringLiteral("members")).toArray();
        foreach (const QJsonValue &member, memberList) {
            QVariant memberId = member.toVariant();

            if (memberId != config->userId()) {
                members << Storage::user(memberId).value(QStringLiteral("name")).toString();
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

void SlackClient::parseUsers(const QJsonObject& data)
{
    foreach (const QJsonValue &value, data.value(QStringLiteral("users")).toArray()) {
        QJsonObject user = value.toObject();

        QVariantMap data;
        const QString userId = user.value(QStringLiteral("id")).toString();
        data.insert(QStringLiteral("id"), userId);
        data.insert(QStringLiteral("name"), user.value(QStringLiteral("name")).toVariant());
        data.insert(QStringLiteral("presence"), user.value(QStringLiteral("presence")).toVariant());

        QJsonObject profile = user.value(QStringLiteral("profile")).toObject();
        m_userAvatars[userId] = QUrl(profile[QStringLiteral("image_512")].toString());

        Storage::saveUser(data);
    }
}

void SlackClient::parseBots(const QJsonObject& data)
{
    foreach (const QJsonValue &value, data.value(QStringLiteral("bots")).toArray()) {
        QJsonObject bot = value.toObject();

        QVariantMap data;
        data.insert(QStringLiteral("id"), bot.value(QStringLiteral("id")));
        data.insert(QStringLiteral("name"), bot.value(QStringLiteral("name")));
        data.insert(QStringLiteral("presence"), QVariant(QStringLiteral("active")));
        Storage::saveUser(data);
    }
}

void SlackClient::parseChannels(const QJsonObject& data)
{
    foreach (const QJsonValue &value, data.value(QStringLiteral("channels")).toArray()) {
        QVariantMap data = parseChannel(value.toObject());
        Storage::saveChannel(data);
    }
}

void SlackClient::parseGroups(const QJsonObject& data)
{
    foreach (const QJsonValue &value, data.value(QStringLiteral("groups")).toArray()) {
        QVariantMap data = parseGroup(value.toObject());
        Storage::saveChannel(data);
    }
}

void SlackClient::parseChats(const QJsonObject& data)
{
    qDebug() << "parse chats"; // << data;
    foreach (const QJsonValue &value, data.value(QStringLiteral("ims")).toArray()) {
        QJsonObject chat = value.toObject();
        QVariantMap data;

        QVariant userId = chat.value(QStringLiteral("user")).toVariant();
        QVariantMap user = Storage::user(userId);

        data.insert(QStringLiteral("type"), QVariant(QStringLiteral("im")));
        data.insert(QStringLiteral("category"), QVariant(QStringLiteral("chat")));
        data.insert(QStringLiteral("id"), chat.value(QStringLiteral("id")).toVariant());
        data.insert(QStringLiteral("userId"), userId);
        data.insert(QStringLiteral("name"), user.value(QStringLiteral("name")));
        data.insert(QStringLiteral("presence"), user.value(QStringLiteral("presence")));
        data.insert(QStringLiteral("isOpen"), chat.value(QStringLiteral("is_open")).toVariant());
        data.insert(QStringLiteral("lastRead"), chat.value(QStringLiteral("last_read")).toVariant());
        data.insert(QStringLiteral("unreadCount"), chat.value(QStringLiteral("unread_count_display")).toVariant());
        Storage::saveChannel(data);
    }
}

QVariantList SlackClient::getChannels()
{
    return Storage::channels();
}

QVariant SlackClient::getChannel(const QString& channelId)
{
    return Storage::channel(QVariant(channelId));
}

QStringList SlackClient::getNickSuggestions(const QString &currentText, const int cursorPosition)
{
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
    for (const QString &memberId : Storage::channel(m_lastChannel).value("members").toStringList()) {
        const QString nick = Storage::user(memberId).value("name").toString();
        if (relevant.isEmpty()) {
            nicks.append(nick);
        } else if (nick.contains(relevant, Qt::CaseInsensitive)) {
            nicks.append(nick);
        }
    }

    return nicks;
}

bool SlackClient::isOnline() const
{
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
    if (m_lastChannel.isEmpty()) {
        QSettings settings;
        m_lastChannel = settings.value(QStringLiteral("LastChannel")).toString();
        if (m_lastChannel.isEmpty() && !Storage::channels().isEmpty()) {
            m_lastChannel = Storage::channels().first().toMap()["id"].toString();
        }

    }
    return m_lastChannel;
}

QString SlackClient::historyMethod(const QString& type)
{
    if (type == QStringLiteral("channel")) {
        return QStringLiteral("channels.history");
    } else if (type == QStringLiteral("group")) {
        return QStringLiteral("groups.history");
    } else if (type == QStringLiteral("mpim")) {
        return QStringLiteral("mpim.history");
    } else if (type == QStringLiteral("im")) {
        return QStringLiteral("im.history");
    } else {
        return "";
    }
}

void SlackClient::joinChannel(const QString& channelId)
{
    QVariantMap channel = Storage::channel(QVariant(channelId));

    QMap<QString, QString> params;
    params.insert(QStringLiteral("name"), channel.value(QStringLiteral("name")).toString());

    QNetworkReply *reply = executeGet("channels.join", params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleJoinChannelReply);
}

void SlackClient::handleJoinChannelReply()
{
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
    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);

    QNetworkReply *reply = executeGet(QStringLiteral("channels.leave"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleLeaveChannelReply);
}

void SlackClient::handleLeaveChannelReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Channel leave failed";
    }

    reply->deleteLater();
}

void SlackClient::leaveGroup(const QString& groupId)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), groupId);

    QNetworkReply *reply = executeGet(QStringLiteral("groups.leave"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleLeaveGroupReply);
}

void SlackClient::handleLeaveGroupReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Group leave failed";
    }

    reply->deleteLater();
}

void SlackClient::openChat(const QString& chatId)
{
    QVariantMap channel = Storage::channel(QVariant(chatId));

    QMap<QString, QString> params;
    params.insert(QStringLiteral("user"), channel.value(QStringLiteral("userId")).toString());

    QNetworkReply *reply = executeGet(QStringLiteral("im.open"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleOpenChatReply);
}

void SlackClient::handleOpenChatReply()
{
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
    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), chatId);

    QNetworkReply *reply = executeGet(QStringLiteral("im.close"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleCloseChatReply);
}

void SlackClient::handleCloseChatReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Chat close failed";
    }

    reply->deleteLater();
}

void SlackClient::loadMessages(const QString& type, const QString& channelId)
{
    if (Storage::channelMessagesExist(channelId)) {
        QVariantList messages = Storage::channelMessages(channelId);
        emit loadMessagesSuccess(channelId, messages);
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
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        emit loadMessagesFail();
        return;
    }

    QJsonArray messageList = data.value(QStringLiteral("messages")).toArray();
    QVariantList messages;

    foreach (const QJsonValue &value, messageList) {
        QJsonObject message = value.toObject();
        messages << getMessageData(message);
    }

    QString channelId = reply->property("channelId").toString();
    Storage::setChannelMessages(channelId, messages);

    emit loadMessagesSuccess(channelId, messages);
}

QString SlackClient::markMethod(const QString& type)
{
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

void SlackClient::markChannel(const QString& type, const QString& channelId, const QString& time)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);
    params.insert(QStringLiteral("ts"), time);

    QNetworkReply *reply = executeGet(markMethod(type), params);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handleMarkChannelReply);
}

void SlackClient::handleMarkChannelReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Mark message result" << data;

    reply->deleteLater();
}

void SlackClient::postMessage(const QString& channelId, QString content)
{
    content.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    content.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    content.replace(QLatin1Char('<'), QStringLiteral("&lt;"));

    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("text"), content);
    data.insert(QStringLiteral("as_user"), QStringLiteral("true"));
    data.insert(QStringLiteral("parse"), QStringLiteral("full"));

    QNetworkReply *reply = executePost(QStringLiteral("chat.postMessage"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackClient::handlePostMessageReply);
}

void SlackClient::handlePostMessageReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Post message result" << data;

    reply->deleteLater();
}

void SlackClient::postImage(const QString& channelId, const QString& imagePath,
                            const QString& title, const QString& comment)
{
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
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Post image result" << data;

    if (isError(data)) {
        emit postImageFail();
    } else {
        emit postImageSuccess();
    }

    reply->deleteLater();
}

QVariantMap SlackClient::getMessageData(const QJsonObject& message)
{
    QVariantMap data;
    data.insert(QStringLiteral("type"), message.value(QStringLiteral("type")).toVariant());
    data.insert(QStringLiteral("time"), message.value(QStringLiteral("ts")).toVariant());
    data.insert(QStringLiteral("channel"), message.value(QStringLiteral("channel")).toVariant());
    data.insert(QStringLiteral("user"), user(message));
    data.insert(QStringLiteral("attachments"), getAttachments(message));
    data.insert(QStringLiteral("images"), getImages(message));
    data.insert(QStringLiteral("content"), QVariant(getContent(message)));

    return data;
}

QVariantMap SlackClient::user(const QJsonObject &data)
{

    QString type = data.value(QStringLiteral("subtype")).toString(QStringLiteral("default"));
    QVariant userId;

    if (type == QStringLiteral("bot_message")) {
        userId = data.value(QStringLiteral("bot_id")).toVariant();
    } else {
        userId = data.value(QStringLiteral("user")).toVariant();
    }

    QVariantMap userData = Storage::user(userId);

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
    QString content = message.value(QStringLiteral("text")).toString();

    findNewUsers(content);
    m_formatter.replaceUserInfo(content);
    m_formatter.replaceTargetInfo(content);
    m_formatter.replaceChannelInfo(content);
    m_formatter.replaceLinks(content);
    m_formatter.replaceSpecialCharacters(content);
    m_formatter.replaceMarkdown(content);
    m_formatter.replaceEmoji(content);

    return content;
}

QVariantList SlackClient::getImages(const QJsonObject& message)
{
    QVariantList images;

    if (message.value(QStringLiteral("subtype")).toString() == QStringLiteral("file_share")) {
        QStringList imageTypes;
        imageTypes << QStringLiteral("jpg");
        imageTypes << QStringLiteral("png");
        imageTypes << QStringLiteral("gif");

        QJsonObject file = message.value(QStringLiteral("file")).toObject();
        QString fileType = file.value(QStringLiteral("filetype")).toString();

        if (imageTypes.contains(fileType)) {
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
            fileData.insert(QStringLiteral("name"), file.value(QStringLiteral("name")).toVariant());
            fileData.insert(QStringLiteral("url"), file.value(QStringLiteral("url_private")).toVariant());
            fileData.insert(QStringLiteral("size"), imageSize);
            fileData.insert(QStringLiteral("thumbSize"), thumbSize);
            fileData.insert(QStringLiteral("thumbUrl"), file.value("thumb_" + thumbItem).toVariant());

            images.append(fileData);
            qDebug() << fileData;
        }
    }

    return images;
}

QVariantList SlackClient::getAttachments(const QJsonObject& message)
{
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

                QVariantMap titleData;
                titleData.insert(QStringLiteral("isTitle"), QVariant(true));
                titleData.insert(QStringLiteral("isShort"), QVariant(isShort));
                titleData.insert(QStringLiteral("content"), QVariant(title));
                fields.append(titleData);
            }

            if (!value.isEmpty()) {
                m_formatter.replaceLinks(value);
                m_formatter.replaceMarkdown(value);

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

void SlackClient::findNewUsers(const QString &message)
{
    QRegularExpression newUserPattern(QStringLiteral("<@([A-Z0-9]+)\\|([^>]+)>"));

    QRegularExpressionMatchIterator i = newUserPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString id = match.captured(1);

        if (Storage::user(id).isEmpty()) {
            QString name = match.captured(2);
            QVariantMap data;
            data.insert(QStringLiteral("id"), QVariant(id));
            data.insert(QStringLiteral("name"), QVariant(name));
            data.insert(QStringLiteral("presence"), QVariant(QStringLiteral("active")));
            Storage::saveUser(data);
        }
    }
}

void SlackClient::sendNotification(const QString& channelId, const QString& title, const QString& text)
{
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
