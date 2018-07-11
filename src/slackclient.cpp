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

#include "MessagesModel.h"

SlackTeamClient::SlackTeamClient(const QString &teamId, const QString &accessToken, QObject *parent) :
    QObject(parent), appActive(true), activeWindow("init"), networkAccessible(QNetworkAccessManager::Accessible),
    m_clientId(QString::fromLatin1(QByteArray::fromBase64("MTE5MDczMjc1MDUuMjUyMzc1NTU3MTU1"))),
    m_clientId2(QString::fromLatin1(QByteArray::fromBase64("MGJlNDA0M2Q2OGIxYjM0MzE4ODk5ZDEwYTNiYmM3ZTY=")))/*,
    m_teamsModel(new TeamsModel(this))*/
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

    //QQmlEngine::setObjectOwnership(m_teamsModel, QQmlEngine::CppOwnership);
}

SlackTeamClient::~SlackTeamClient() {}

void SlackTeamClient::startConnections()
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "startConnections", Qt::QueuedConnection);
        return;
    }
    networkAccessManager = new QNetworkAccessManager(this);

    stream = new SlackStream(this);
    reconnectTimer = new QTimer(this);
    networkAccessible = networkAccessManager->networkAccessible();

    connect(networkAccessManager.data(), &QNetworkAccessManager::networkAccessibleChanged, this, &SlackTeamClient::handleNetworkAccessibleChanged);
    connect(reconnectTimer.data(), &QTimer::timeout, this, &SlackTeamClient::reconnectClient);

    connect(stream.data(), &SlackStream::connected, this, &SlackTeamClient::handleStreamStart);
    connect(stream.data(), &SlackStream::disconnected, this, &SlackTeamClient::handleStreamEnd);
    connect(stream.data(), &SlackStream::messageReceived, this, &SlackTeamClient::handleStreamMessage);

    connect(this, &SlackTeamClient::connected, this, &SlackTeamClient::isOnlineChanged);
    connect(this, &SlackTeamClient::initSuccess, this, &SlackTeamClient::isOnlineChanged);
    connect(this, &SlackTeamClient::disconnected, this, &SlackTeamClient::isOnlineChanged);
}

void SlackTeamClient::setAppActive(bool active)
{
    DEBUG_BLOCK

    appActive = active;
    clearNotifications();
}

void SlackTeamClient::setActiveWindow(const QString& windowId)
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

void SlackTeamClient::clearNotifications()
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

void SlackTeamClient::handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible)
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

void SlackTeamClient::reconnectClient()
{
    DEBUG_BLOCK

    qDebug() << "Reconnecting";
    setState(ClientStates::RECONNECTING);
    emit reconnecting(m_teamInfo.teamId());
    startClient();
}

void SlackTeamClient::handleStreamStart()
{
    DEBUG_BLOCK

    qDebug() << "Stream started";
    setState(ClientStates::CONNECTED);
    emit connected(m_teamInfo.teamId());
}

void SlackTeamClient::handleStreamEnd(){
    DEBUG_BLOCK
    qDebug() << "Stream ended";
    setState(ClientStates::RECONNECTING);
    emit reconnecting(m_teamInfo.teamId());
    reconnectTimer->setSingleShot(true);
    reconnectTimer->start(1000);
}

void SlackTeamClient::handleStreamMessage(const QJsonObject& message)
{
    DEBUG_BLOCK

    const QString& type = message.value(QStringLiteral("type")).toString();

    if (type != "pong") {
        qDebug() << "stream message" << type;
    }
//    qDebug().noquote() << QJsonDocument(message).toJson();

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
        emit userTyping(m_teamInfo.teamId(),
                        message.value(QStringLiteral("channel")).toString(),
                        userName(message.value(QStringLiteral("user")).toString()));
    } else if (type == QStringLiteral("user_change")) {
        qDebug() << "user changed" << message;
        QMetaObject::invokeMethod(teamInfo()->users(), "updateUser",
                                  Qt::QueuedConnection,
                                  Q_ARG(QJsonObject, message.value(QStringLiteral("user")).toObject()));
    } else if (type == QStringLiteral("team_join")) {
        qDebug() << "user joined" << message;
        //invoke add user for run in GUI thread
        QMetaObject::invokeMethod(teamInfo()->users(), "addUser",
                                  Qt::QueuedConnection,
                                  Q_ARG(QJsonObject, message.value(QStringLiteral("user")).toObject()));
        //parseUser(message.value(QStringLiteral("user")).toObject());
    } else if (type == QStringLiteral("member_joined_channel")) {
        qDebug() << "user joined to channel" << message.value(QStringLiteral("user")).toString();
    } else if (type == QStringLiteral("pong")) {
    } else if (type == QStringLiteral("hello")) {
    } else {
        qDebug() << "Unhandled message";
        qDebug().noquote() << QJsonDocument(message).toJson();
    }
}

void SlackTeamClient::parseChatOpen(const QJsonObject& message)
{
    DEBUG_BLOCK
//TODO: redesign
//    QString id = message.value(QStringLiteral("channel")).toString();
//    QVariantMap channel = m_storage.channel(id);
//    channel.insert(QStringLiteral("isOpen"), QVariant(true));
//    m_storage.saveChannel(channel);
//    emit channelJoined(m_teamInfo.teamId(), channel);
}

void SlackTeamClient::parseChatClose(const QJsonObject& message)
{
    DEBUG_BLOCK
//TODO: redesign
//    QString id = message.value(QStringLiteral("channel")).toString();
//    QVariantMap channel = m_storage.channel(id);
//    channel.insert(QStringLiteral("isOpen"), QVariant(false));
//    m_storage.saveChannel(channel);
//    emit channelLeft(m_teamInfo.teamId(), channel);
}

void SlackTeamClient::parseChannelJoin(const QJsonObject& message)
{
    DEBUG_BLOCK
//TODO: redesign
//    QVariantMap data = parseChannel(message.value(QStringLiteral("channel")).toObject());
//    m_storage.saveChannel(data);
//    currentChatsModel()->addChat(message.value(QStringLiteral("channel")).toObject(), ChatsModel::Channel);
//    emit channelJoined(m_teamInfo.teamId(), data);
}

void SlackTeamClient::parseChannelLeft(const QJsonObject& message)
{
    DEBUG_BLOCK
//TODO: redesign
//    QString id = message.value(QStringLiteral("channel")).toString();
//    QVariantMap channel = m_storage.channel(id);
//    channel.insert(QStringLiteral("isOpen"), QVariant(false));
//    m_storage.saveChannel(channel);
//    emit channelLeft(m_teamInfo.teamId(), channel);
}

void SlackTeamClient::parseGroupJoin(const QJsonObject& message)
{
    DEBUG_BLOCK
//TODO: redesign
//    QVariantMap data = parseGroup(message.value(QStringLiteral("channel")).toObject());
//    m_storage.saveChannel(data);
//    emit channelJoined(m_teamInfo.teamId(), data);
}

void SlackTeamClient::parseChannelUpdate(const QJsonObject& message)
{
    DEBUG_BLOCK
//TODO: redesign
//    QString id = message.value(QStringLiteral("channel")).toString();
//    QVariantMap channel = m_storage.channel(id);
//    channel.insert(QStringLiteral("lastRead"), message.value(QStringLiteral("ts")).toVariant());
//    channel.insert(QStringLiteral("unreadCount"), message.value(QStringLiteral("unread_count_display")).toVariant());
//    m_storage.saveChannel(channel);
//    emit channelUpdated(m_teamInfo.teamId(), channel);
}

//TODO: investigate comment type
//user id is empty QJsonObject({"comment":{"comment":"Okay, now I'm interested here. Light can travel less than a meter in a single nanosecond. Is \"nanosecond latency\" meaning \"measured in nanoseconds\"?","created":1531158771,"id":"FcBLM50VKJ","is_intro":false,"timestamp":1531158771,"user":"U8JRJRKEF"},"file":{"channels":["C21PKDHSL"],"comments_count":2,"created":1531158641,"display_as_bot":false,"editable":false,"external_type":"","filetype":"png","groups":[],"id":"FBM54SBDJ","image_exif_rotation":1,"ims":[],"initial_comment":{"comment":"For example here's a point-to-point laser router with nanosecond latency","created":1531158641,"id":"FcBMBYPSSE","is_intro":true,"timestamp":1531158641,"user":"U4R43AVMM"},"is_external":false,"is_public":true,"mimetype":"image/png","mode":"hosted","name":"image.png","original_h":183,"original_w":276,"permalink":"https://cpplang.slack.com/files/U4R43AVMM/FBM54SBDJ/image.png","permalink_public":"https://slack-files.com/T21Q22G66-FBM54SBDJ-4703532276","pretty_type":"PNG","public_url_shared":false,"size":29293,"thumb_160":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_160.png","thumb_360":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_360.png","thumb_360_h":183,"thumb_360_w":276,"thumb_64":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_64.png","thumb_80":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_80.png","timestamp":1531158641,"title":"image.png","url_private":"https://files.slack.com/files-pri/T21Q22G66-FBM54SBDJ/image.png","url_private_download":"https://files.slack.com/files-pri/T21Q22G66-FBM54SBDJ/download/image.png","user":"U4R43AVMM","username":""},"is_intro":false,"subtype":"file_comment","text":"<@U8JRJRKEF> commented on <@U4R43AVMM>’s file <https://cpplang.slack.com/files/U4R43AVMM/FBM54SBDJ/image.png|image.png>: Okay, now I'm interested here. Light can travel less than a meter in a single nanosecond. Is \"nanosecond latency\" meaning \"measured in nanoseconds\"?","ts":"1531158771.000413","type":"message"})
//no user for "" UsersModel(0x55555a081f80)
void SlackTeamClient::parseMessageUpdate(const QJsonObject& message)
{
    DEBUG_BLOCK;
//TODO: redesign
    const QString& subtype = message.value(QStringLiteral("subtype")).toString();
    const QJsonValue& submessage = message.value(QStringLiteral("message"));
    Message* message_ = new Message;
    if (submessage.isUndefined()) {
        message_->setData(message);
    } else {
        message_->setData(submessage.toObject());
        //channel id missed in sub messages
        const QString& channel_id = message.value(QStringLiteral("channel")).toString();
        message_->channel_id = channel_id;
    }
    if (subtype == "message_changed") {
       //qDebug().noquote() << "message changed" << QJsonDocument(message).toJson();
       message_->isChanged = true;
       emit messageUpdated(message_);
    } if (subtype == "message_deleted") {
    } else {
        emit messageReceived(message_);
    }
//    const QString& teamId = message.value(QStringLiteral("team_id")).toString();
//    const QJsonValue& subtype = message.value(QStringLiteral("subtype"));
//    const QJsonValue& innerMessage = message.value(QStringLiteral("message"));
//    if (innerMessage.isUndefined()) {
//        data = getMessageData(message, teamId);
//    } else {
//        //TODO(unknown): handle messages threads
//        data = getMessageData(innerMessage.toObject(), teamId);
//        if (subtype.toString() == "message_changed") {
//            data[QStringLiteral("edited")] = true;
//        }
//    }

//    QString channelId = message.value(QStringLiteral("channel")).toString();

//    if (!data.value("channel").isValid()) {
//        data["channel"] = QVariant(channelId);
//    }

//    if (m_storage.channelMessagesExist(channelId)) {
//        m_storage.appendChannelMessage(channelId, data);
//    }

//    QVariantMap channel = m_storage.channel(channelId);

//    QString messageTime = data.value(QStringLiteral("time")).toString();
//    QString latestRead = channel.value(QStringLiteral("lastRead")).toString();

//    if (messageTime > latestRead) {
//        int unreadCount = channel.value(QStringLiteral("unreadCount")).toInt() + 1;
//        channel.insert(QStringLiteral("unreadCount"), unreadCount);
//        m_storage.saveChannel(channel);
//        emit channelUpdated(m_teamInfo.teamId(), channel);
//    }

//    if (!channel.value(QStringLiteral("isOpen")).toBool()) {
//        if (channel.value(QStringLiteral("type")).toString() == QStringLiteral("im")) {
//            openChat(channelId);
//        }
//    }


}

void SlackTeamClient::parseReactionUpdate(const QJsonObject &message)
{
    DEBUG_BLOCK;

    //"{\"type\":\"reaction_added\",\"user\":\"U4NH7TD8D\",\"item\":{\"type\":\"message\",\"channel\":\"C09PZTN5S\",\"ts\":\"1525437188.000435\"},\"reaction\":\"slightly_smiling_face\",\"item_user\":\"U3ZC1RYJG\",\"event_ts\":\"1525437431.000236\",\"ts\":\"1525437431.000236\"}"
    //"{\"type\":\"reaction_removed\",\"user\":\"U4NH7TD8D\",\"item\":{\"type\":\"message\",\"channel\":\"C0CK10FA9\",\"ts\":\"1526478339.000316\"},\"reaction\":\"+1\",\"item_user\":\"U1WTHK18E\",\"event_ts\":\"1526736040.000047\",\"ts\":\"1526736040.000047\"}"
    const QJsonObject& item = message.value(QStringLiteral("item")).toObject();
    const QDateTime& ts = slackToDateTime(item.value(QStringLiteral("ts")).toString());
    const QString& channelid = item.value(QStringLiteral("channel")).toString();
    MessageListModel* messages = m_teamInfo.chats()->messages(channelid);
    Message* m = messages->message(ts);
    if (m != nullptr) {
        const QString& reaction = message.value(QStringLiteral("reaction")).toString();
        const QString& type = message.value(QStringLiteral("type")).toString();
        const QString& userid = message.value(QStringLiteral("user")).toString();
        Reaction* r = nullptr;
        //check if the reaction already there
        foreach(QObject* reaObj, m->reactions) {
            Reaction* rea = static_cast<Reaction*>(reaObj);
            qDebug() << "search reaction" << rea->name << reaction;
            if (rea->name == reaction) {
                r = rea;
                break;
            }
        }
        if (type == "reaction_added") {
            if (r == nullptr) {
                r = new Reaction;
                QString emojiPrepare = QString(":%1:").arg(reaction);
                MessageFormatter _formatter;
                _formatter.replaceEmoji(emojiPrepare);
                r->emoji = emojiPrepare;
                r->name = reaction;
                m->reactions.append(r);
            }
            r->userIds << userid;
        } else if (type == "reaction_removed") {
            if (r == nullptr) {
                qWarning() << "reaction" << reaction << "not found for message on channel" << channelid << "time" << ts;
                return;
            }
            m->reactions.removeOne(r);
        }
        emit messageUpdated(m);
    } else {
        qWarning() << "message not found for ts" << ts;
    }
}

void SlackTeamClient::parsePresenceChange(const QJsonObject& message)
{
    DEBUG_BLOCK
//TODO: redesign
//    QVariant userId = message.value(QStringLiteral("user")).toVariant();
//    QVariant presence = message.value(QStringLiteral("presence")).toVariant();

//    QVariantMap user = m_storage.user(userId);
//    if (!user.isEmpty()) {
//        user.insert(QStringLiteral("presence"), presence);
//        m_storage.saveUser(user);
//        emit userUpdated(m_teamInfo.teamId(), user);
//    }

//    foreach (const auto item, m_storage.channels()) {
//        QVariantMap channel = item.toMap();

//        if (channel.value(QStringLiteral("type")) == QVariant(QStringLiteral("im")) &&
//                channel.value(QStringLiteral("userId")) == userId) {
//            channel.insert(QStringLiteral("presence"), presence);
//            m_storage.saveChannel(channel);
//            emit channelUpdated(m_teamInfo.teamId(), channel);
//        }
//    }
}

void SlackTeamClient::parseNotification(const QJsonObject& message)
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

bool SlackTeamClient::isOk(const QNetworkReply *reply)
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

bool SlackTeamClient::isError(const QJsonObject &data)
{
    DEBUG_BLOCK

    if (data.isEmpty()) {
        qWarning() << "No data received";
        return true;
    } else {
        return !data.value(QStringLiteral("ok")).toBool(false);
    }
}

QJsonObject SlackTeamClient::getResult(QNetworkReply *reply)
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

QNetworkReply *SlackTeamClient::executeGet(const QString& method, const QMap<QString, QString>& params)
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

QNetworkReply *SlackTeamClient::executePost(const QString& method, const QMap<QString, QString> &data)
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

QNetworkReply *SlackTeamClient::executePostWithFile(const QString& method, const QMap<QString, QString> &formdata, QFile *file)
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

void SlackTeamClient::testLogin()
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
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleTestLoginReply);
}

void SlackTeamClient::handleTestLoginReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        emit testLoginFail(m_teamInfo.teamId());
        return;
    }
    //qDebug().noquote() << QJsonDocument(data).toJson();

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

void SlackTeamClient::searchMessages(const QString &searchString)
{
    DEBUG_BLOCK;

    QMap<QString, QString> params;
    params.insert(QStringLiteral("query"), searchString);
    params.insert(QStringLiteral("highlight"), QStringLiteral("false"));
    params.insert(QStringLiteral("sort"), QStringLiteral("timestamp"));

    QNetworkReply *reply = executeGet(QStringLiteral("search.messages"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleSearchMessagesReply);
}

void SlackTeamClient::handleSearchMessagesReply()
{
    DEBUG_BLOCK;

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
        //TODO: redesign
//        QVariantMap searchResult = getMessageData(matchObj, matchObj.value(QStringLiteral("team")).toString());
//        searchResult.insert(QStringLiteral("permalink"), matchObj.value(QStringLiteral("permalink")).toVariant());

//        const QJsonObject& channelObj = matchObj.value(QStringLiteral("channel")).toObject();
//        QVariantMap channel;
//        channel[QStringLiteral("id")] = channelObj.value(QStringLiteral("id"));
//        channel[QStringLiteral("name")] = channelObj.value(QStringLiteral("name"));
//        searchResult.insert(QStringLiteral("channel"), channel);
//        searchResults.append(searchResult);
    }
    qDebug() << "search result. found entries" << _total;
    emit searchResultsReady(m_teamInfo.teamId(), searchResults);
}

void SlackTeamClient::startClient()
{
    DEBUG_BLOCK
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "startClient", Qt::QueuedConnection);
        return;
    }

    qDebug() << "Start init";
    QNetworkReply *reply = executeGet(QStringLiteral("rtm.start"));
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleStartReply);
}

void SlackTeamClient::handleStartReply()
{
    DEBUG_BLOCK;

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

    // now time to create models.
    // Qt is strict that model, used by QMLEngine, should be created in same thread
    // by deafault its GUI thread
    // TODO: investigate move QML engine to separate thread or make parsing in non-GUI thread
    // and then provide data to the models
    emit teamDataChanged(data);


//    parseUsers(data);
//    parseBots(data);
//    parseChannels(data);
//    parseGroups(data);

    QUrl url(data.value(QStringLiteral("url")).toString());
    stream->listen(url);

    if (m_teamInfo.lastChannel().isEmpty()) {
        m_teamInfo.setLastChannel(lastChannel());
    }
    qDebug() << "init success";
    emit initSuccess(m_teamInfo.teamId());
}

QVariantMap SlackTeamClient::parseChannel(const QJsonObject& channel)
{
    DEBUG_BLOCK;

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

QVariantMap SlackTeamClient::parseGroup(const QJsonObject& group)
{
    DEBUG_BLOCK

    QVariantMap data;
//TODO: redesign
//    if (group.value(QStringLiteral("is_mpim")).toBool()) {
//        data.insert(QStringLiteral("type"), QVariant(QStringLiteral("mpim")));
//        data.insert(QStringLiteral("category"), QVariant(QStringLiteral("chat")));

//        QStringList members;
//        QJsonArray memberList = group.value(QStringLiteral("members")).toArray();
//        foreach (const QJsonValue &member, memberList) {
//            QVariant memberId = member.toVariant();

//            if (memberId != config->userId()) {
//                members << m_storage.user(memberId).value(QStringLiteral("name")).toString();
//            }
//        }
//        data.insert(QStringLiteral("name"), QVariant(members.join(QStringLiteral(", "))));
//    } else {
//        data.insert(QStringLiteral("type"), QVariant(QStringLiteral("group")));
//        data.insert(QStringLiteral("category"), QVariant(QStringLiteral("channel")));
//        data.insert(QStringLiteral("name"), group.value(QStringLiteral("name")).toVariant());
//    }

//    data.insert(QStringLiteral("id"), group.value(QStringLiteral("id")).toVariant());
//    data.insert(QStringLiteral("presence"), QVariant(QStringLiteral("none")));
//    data.insert(QStringLiteral("isOpen"), group.value(QStringLiteral("is_open")).toVariant());
//    data.insert(QStringLiteral("lastRead"), group.value(QStringLiteral("last_read")).toVariant());
//    data.insert(QStringLiteral("unreadCount"), group.value(QStringLiteral("unread_count_display")).toVariant());
//    data.insert(QStringLiteral("userId"), QVariant());
    return data;
}

void SlackTeamClient::parseUser(const QJsonObject& user)
{
    //TODO: redesign
//    QVariantMap data;
//    const QString userId = user.value(QStringLiteral("id")).toString();
//    data.insert(QStringLiteral("id"), userId);
//    data.insert(QStringLiteral("name"), user.value(QStringLiteral("name")).toVariant());
//    data.insert(QStringLiteral("presence"), user.value(QStringLiteral("presence")).toVariant());

//    const QJsonObject& profile = user.value(QStringLiteral("profile")).toObject();
//    m_userAvatars[userId] = QUrl(profile.value(QStringLiteral("image_72")).toString());

//    m_storage.saveUser(data);
}

void SlackTeamClient::parseUsers(const QJsonObject& data)
{
    //TODO: redesign
//    qDebug() << "parse users";
//    foreach (const QJsonValue &value, data.value(QStringLiteral("users")).toArray()) {
//        parseUser(value.toObject());
//    }
//    m_storage.updateUsersList();
}

void SlackTeamClient::parseBots(const QJsonObject& data)
{
    DEBUG_BLOCK

//TODO: redesign
//    foreach (const QJsonValue &value, data.value(QStringLiteral("bots")).toArray()) {
//        QJsonObject bot = value.toObject();

//        QVariantMap data;
//        data.insert(QStringLiteral("id"), bot.value(QStringLiteral("id")));
//        data.insert(QStringLiteral("name"), bot.value(QStringLiteral("name")));
//        data.insert(QStringLiteral("presence"), QVariant(QStringLiteral("active")));
//        m_storage.saveUser(data);
//    }
}

void SlackTeamClient::parseChannels(const QJsonObject& data)
{
    DEBUG_BLOCK
//TODO: redesign
//    qDebug() << "parse channels";

//    foreach (const QJsonValue &value, data.value(QStringLiteral("channels")).toArray()) {
//        QVariantMap data = parseChannel(value.toObject());
//        m_storage.saveChannel(data);
//    }
}

void SlackTeamClient::parseGroups(const QJsonObject& data)
{
    DEBUG_BLOCK
//TODO: redesign
//    foreach (const QJsonValue &value, data.value(QStringLiteral("groups")).toArray()) {
//        QVariantMap data = parseGroup(value.toObject());
//        m_storage.saveChannel(data);
//    }
}

void SlackTeamClient::parseChats(const QJsonObject& data)
{
    DEBUG_BLOCK
//TODO: redesign
//    qDebug() << "parse chats"; // << data;
//    foreach (const QJsonValue &value, data.value(QStringLiteral("ims")).toArray()) {
//        QJsonObject chat = value.toObject();
//        QVariantMap data;

//        QVariant userId = chat.value(QStringLiteral("user")).toVariant();
//        QVariantMap user = m_storage.user(userId);

//        data.insert(QStringLiteral("type"), QVariant(QStringLiteral("im")));
//        data.insert(QStringLiteral("category"), QVariant(QStringLiteral("chat")));
//        data.insert(QStringLiteral("id"), chat.value(QStringLiteral("id")).toVariant());
//        data.insert(QStringLiteral("userId"), userId);
//        data.insert(QStringLiteral("name"), user.value(QStringLiteral("name")));
//        data.insert(QStringLiteral("presence"), user.value(QStringLiteral("presence")));
//        data.insert(QStringLiteral("isOpen"), chat.value(QStringLiteral("is_open")).toVariant());
//        data.insert(QStringLiteral("lastRead"), chat.value(QStringLiteral("last_read")).toVariant());
//        data.insert(QStringLiteral("unreadCount"), chat.value(QStringLiteral("unread_count_display")).toVariant());
//        m_storage.saveChannel(data);
//    }
}

QVariantList SlackTeamClient::getChannels()
{
    DEBUG_BLOCK

    return QVariantList(); //TODO: redesign: m_storage.channels();
}

QVariant SlackTeamClient::getChannel(const QString& channelId)
{
    DEBUG_BLOCK

    return QVariant(); //TODO: redesign: m_storage.channel(QVariant(channelId));
}

QStringList SlackTeamClient::getNickSuggestions(const QString &currentText, const int cursorPosition)
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
//TODO: redesign
//    for (const QString &memberId : m_storage.channel(m_teamInfo.lastChannel()).value("members").toStringList()) {
//        const QString nick = m_storage.user(memberId).value("name").toString();
//        if (relevant.isEmpty()) {
//            nicks.append(nick);
//        } else if (nick.contains(relevant, Qt::CaseInsensitive)) {
//            nicks.append(nick);
//        }
//    }

    return nicks;
}

ChatsModel *SlackTeamClient::currentChatsModel()
{
    DEBUG_BLOCK;
    return m_teamInfo.chats();
}

bool SlackTeamClient::isOnline() const
{
    DEBUG_BLOCK;
    return stream && stream->isConnected();
}

bool SlackTeamClient::isDevice() const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

QString SlackTeamClient::lastChannel()
{
    DEBUG_BLOCK
    QString _lastChannel;
//TODO: redesign
//    const QVariantList& chList = m_storage.channels();

//    if (!chList.isEmpty()) {
//        _lastChannel = chList.first().toMap()[QStringLiteral("id")].toString();
//    }

    if (!m_teamInfo.lastChannel().isEmpty()) {
        _lastChannel = m_teamInfo.lastChannel();
    }
    //qDebug() << "last channel" << _lastChannel;
    return _lastChannel;
}

QString SlackTeamClient::historyMethod(const ChatsModel::ChatType type)
{
    DEBUG_BLOCK

    if (type == ChatsModel::Channel) {
        return QStringLiteral("channels.history");
    } else if (type == ChatsModel::Group) {
        return QStringLiteral("groups.history");
    } else if (type == ChatsModel::MultiUserConversation) {
        return QStringLiteral("mpim.history");
    } else if (type == ChatsModel::Conversation) {
        return QStringLiteral("im.history");
    } else {
        return QStringLiteral("");
    }
}

void SlackTeamClient::joinChannel(const QString& channelId)
{
    DEBUG_BLOCK
//TODO: redesign
//    QVariantMap channel = m_storage.channel(QVariant(channelId));

//    QMap<QString, QString> params;
//    params.insert(QStringLiteral("name"), channel.value(QStringLiteral("name")).toString());

//    QNetworkReply *reply = executeGet(QStringLiteral("channels.join"), params);
//    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleJoinChannelReply);
}

void SlackTeamClient::handleJoinChannelReply()
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

void SlackTeamClient::leaveChannel(const QString& channelId)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);

    QNetworkReply *reply = executeGet(QStringLiteral("channels.leave"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleLeaveChannelReply);
}

void SlackTeamClient::handleLeaveChannelReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Channel leave failed";
    }

    reply->deleteLater();
}

void SlackTeamClient::leaveGroup(const QString& groupId)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), groupId);

    QNetworkReply *reply = executeGet(QStringLiteral("groups.leave"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleLeaveGroupReply);
}

void SlackTeamClient::handleLeaveGroupReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Group leave failed";
    }

    reply->deleteLater();
}

void SlackTeamClient::openChat(const QString& chatId)
{
    DEBUG_BLOCK
//TODO: redesign
//    QVariantMap channel = m_storage.channel(QVariant(chatId));

//    QMap<QString, QString> params;
//    params.insert(QStringLiteral("user"), channel.value(QStringLiteral("userId")).toString());

//    QNetworkReply *reply = executeGet(QStringLiteral("im.open"), params);
//    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleOpenChatReply);
}

void SlackTeamClient::handleOpenChatReply()
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

void SlackTeamClient::closeChat(const QString& chatId)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), chatId);

    QNetworkReply *reply = executeGet(QStringLiteral("im.close"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCloseChatReply);
}


void SlackTeamClient::handleCloseChatReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Chat close failed";
    }

    reply->deleteLater();
}

void SlackTeamClient::requestTeamInfo()
{
    QNetworkReply *reply = executeGet(QStringLiteral("team.info"));
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleTeamInfoReply);
}

void SlackTeamClient::requestTeamEmojis()
{
    QNetworkReply *reply = executeGet(QStringLiteral("emoji.list"));
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleTeamEmojisReply);
}

QString SlackTeamClient::userName(const QString &userId) {
    User* user = teamInfo()->users()->user(userId);
    if (user == nullptr) {
        return "";
    }
    return user->username();
}

void SlackTeamClient::handleTeamInfoReply()
{
    DEBUG_BLOCK;

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

void SlackTeamClient::handleTeamEmojisReply()
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

void SlackTeamClient::loadMessages(const ChatsModel::ChatType type, const QString& channelId)
{
    DEBUG_BLOCK;
    qDebug() << "Loading messages" << type << channelId;
    if (channelId.isEmpty()) {
        qWarning() << "Empty channel id";
        return;
    }

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);

    QNetworkReply *reply = executeGet(historyMethod(type), params);
    reply->setProperty("channelId", channelId);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleLoadMessagesReply);
}

void SlackTeamClient::handleLoadMessagesReply()
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
    ChatsModel *chatModel = m_teamInfo.chats();

    if (!chatModel->hasChannel(channelId)) {
        qWarning() << "Team" << m_teamInfo.teamId() << "does not have channel" << channelId;
        return;
    }

    MessageListModel *messageModel = chatModel->messages(channelId);
    messageModel->addMessages(messageList);

    qDebug() << "messages loaded for" << channelId << m_teamInfo.teamId();
    emit loadMessagesSuccess(m_teamInfo.teamId(), channelId);
}

QString SlackTeamClient::markMethod(const QString& type)
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

SlackTeamClient::ClientStates SlackTeamClient::getState() const
{
    return m_state;
}

void SlackTeamClient::setState(ClientStates state)
{
    if (state != m_state) {
        m_state = state;
        emit stateChanged(m_teamInfo.teamId());
    }
}

TeamInfo *SlackTeamClient::teamInfo()
{
    return &m_teamInfo;
}

void SlackTeamClient::markChannel(const QString& type, const QString& channelId, const QString& time)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);
    params.insert(QStringLiteral("ts"), time);

    QNetworkReply *reply = executeGet(markMethod(type), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleMarkChannelReply);
}

void SlackTeamClient::handleMarkChannelReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    //qDebug() << "Mark message result" << data;

    reply->deleteLater();
}

void SlackTeamClient::deleteReaction(const QString& channelId, const QDateTime &ts, const QString& reaction)
{
    DEBUG_BLOCK

    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("name"), reaction);
    data.insert(QStringLiteral("timestamp"), dateTimeToSlack(ts));

    QNetworkReply *reply = executePost(QStringLiteral("reactions.remove"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleDeleteReactionReply);
}

void SlackTeamClient::addReaction(const QString &channelId, const QDateTime &ts, const QString &reaction)
{
    DEBUG_BLOCK

    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("name"), reaction);
    data.insert(QStringLiteral("timestamp"), dateTimeToSlack(ts));

    QNetworkReply *reply = executePost(QStringLiteral("reactions.add"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleAddReactionReply);
}

void SlackTeamClient::postMessage(const QString& channelId, QString content)
{
    DEBUG_BLOCK
    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("text"), content);
    data.insert(QStringLiteral("as_user"), QStringLiteral("true"));
    data.insert(QStringLiteral("parse"), QStringLiteral("full"));

    QNetworkReply *reply = executePost(QStringLiteral("chat.postMessage"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handlePostMessageReply);
}

void SlackTeamClient::handleDeleteReactionReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Delete reaction result" << data;

    reply->deleteLater();
}

void SlackTeamClient::handleAddReactionReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Add reaction result" << data;

    reply->deleteLater();
}

void SlackTeamClient::handlePostMessageReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Post message result" << data;

    reply->deleteLater();
}

void SlackTeamClient::postImage(const QString& channelId, const QString& imagePath,
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

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handlePostImage);
    connect(reply, &QNetworkReply::finished, imageFile, &QObject::deleteLater);
}

void SlackTeamClient::handlePostImage()
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

QVariantMap SlackTeamClient::user(const QJsonObject &data)
{
    DEBUG_BLOCK;
//TODO: redesign
//    QString type = data.value(QStringLiteral("subtype")).toString(QStringLiteral("default"));
//    QVariant userId;

//    if (type == QStringLiteral("bot_message")) {
//        userId = data.value(QStringLiteral("bot_id")).toVariant();
//    } else {
//        userId = data.value(QStringLiteral("user")).toVariant();
//    }

//    QVariantMap userData = m_storage.user(userId);

//    if (userData.isEmpty()) {
//        userData.insert(QStringLiteral("id"), data.value(QStringLiteral("user")).toVariant());
//        userData.insert(QStringLiteral("name"), QVariant(QStringLiteral("Unknown")));
//        userData.insert(QStringLiteral("presence"), QVariant(QStringLiteral("away")));
//    }

//    QString username = data.value(QStringLiteral("username")).toString();
//    if (!username.isEmpty()) {
//        QRegularExpression newUserPattern(QStringLiteral("<@([A-Z0-9]+)\\|([^>]+)>"));
//        username.replace(newUserPattern, QStringLiteral("\\2"));
//        userData.insert(QStringLiteral("name"), username);
//    }

//    return userData;
    return QVariantMap();
}

void SlackTeamClient::sendNotification(const QString& channelId, const QString& title, const QString& text)
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
