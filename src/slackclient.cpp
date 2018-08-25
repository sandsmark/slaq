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

#include "zlib.h"

#include "slackclient.h"
#include "imagescache.h"
#include "debugblock.h"

#include "MessagesModel.h"

SlackTeamClient::SlackTeamClient(QObject *spawner, const QString &teamId, const QString &accessToken, QObject *parent) :
    QObject(parent), appActive(true), activeWindow("init"), networkAccessible(QNetworkAccessManager::Accessible), m_spawner(spawner)
{
    DEBUG_BLOCK;
    QQmlEngine::setObjectOwnership(&m_teamInfo, QQmlEngine::CppOwnership);
    m_teamInfo.setTeamId(teamId);
    config = SlackConfig::instance();
    config->loadTeamInfo(m_teamInfo);
    if (m_teamInfo.teamToken().isEmpty()) {
        m_teamInfo.setTeamToken(accessToken);
    }
    setState(ClientStates::DISCONNECTED);
    qDebug() << "client ctor finished" << m_teamInfo.teamToken() << m_teamInfo.teamId() << m_teamInfo.name();
}

SlackTeamClient::~SlackTeamClient() {
    reconnectTimer->stop();
    disconnect(networkAccessManager.data(), &QNetworkAccessManager::networkAccessibleChanged, this, &SlackTeamClient::handleNetworkAccessibleChanged);
    disconnect(reconnectTimer.data(), &QTimer::timeout, this, &SlackTeamClient::reconnectClient);

    disconnect(stream.data(), &SlackStream::connected, this, &SlackTeamClient::handleStreamStart);
    disconnect(stream.data(), &SlackStream::disconnected, this, &SlackTeamClient::handleStreamEnd);
    disconnect(stream.data(), &SlackStream::messageReceived, this, &SlackTeamClient::handleStreamMessage);

    disconnect(this, &SlackTeamClient::connected, this, &SlackTeamClient::isOnlineChanged);
    disconnect(this, &SlackTeamClient::initSuccess, this, &SlackTeamClient::isOnlineChanged);
    disconnect(this, &SlackTeamClient::disconnected, this, &SlackTeamClient::isOnlineChanged);
    delete reconnectTimer;
    delete stream;
    delete networkAccessManager;
}

void SlackTeamClient::startConnections()
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "startConnections", Qt::QueuedConnection);
        return;
    }
    networkAccessManager = new QNetworkAccessManager;

    stream = new SlackStream;
    reconnectTimer = new QTimer;
    networkAccessible = networkAccessManager->networkAccessible();

    connect(networkAccessManager.data(), &QNetworkAccessManager::networkAccessibleChanged, this, &SlackTeamClient::handleNetworkAccessibleChanged);
    connect(reconnectTimer.data(), &QTimer::timeout, this, &SlackTeamClient::reconnectClient);

    connect(stream.data(), &SlackStream::connected, this, &SlackTeamClient::handleStreamStart);
    connect(stream.data(), &SlackStream::disconnected, this, &SlackTeamClient::handleStreamEnd);
    connect(stream.data(), &SlackStream::messageReceived, this, &SlackTeamClient::handleStreamMessage);

    connect(this, &SlackTeamClient::connected, this, &SlackTeamClient::isOnlineChanged);
    connect(this, &SlackTeamClient::initSuccess, this, &SlackTeamClient::isOnlineChanged);
    connect(this, &SlackTeamClient::disconnected, this, &SlackTeamClient::isOnlineChanged);
    // invoke on the main thread
    QMetaObject::invokeMethod(qApp, [this]{ m_teamInfo.createModels(this); });
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

QString SlackTeamClient::getChannelName(const QString &channelId)
{
    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return "";
    }
    Chat* _chat = _chatsModel->chat(channelId);
    if (_chat == nullptr) {
        return "";
    }
    return teamInfo()->chats()->chat(channelId)->name;
}

Chat *SlackTeamClient::getChannel(const QString &channelId)
{
    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return nullptr;
    }
    return teamInfo()->chats()->chat(channelId);
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
    DEBUG_BLOCK;

    if (m_state != ClientStates::DISCONNECTED) {
        return;
    }

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
    setState(ClientStates::DISCONNECTED);
    emit reconnecting(m_teamInfo.teamId());
    reconnectTimer->setSingleShot(true);
    reconnectTimer->start(10000);
}

void SlackTeamClient::handleStreamMessage(const QJsonObject& message)
{
    DEBUG_BLOCK

    const QString& type = message.value(QStringLiteral("type")).toString();

    if (type != "pong") {
        qDebug() << "stream message type" << type;
    }
//    qDebug().noquote() << QJsonDocument(message).toJson();

    if (type == QStringLiteral("message")) {
        parseMessageUpdate(message);
    } else if (type == QStringLiteral("group_marked") ||
               type == QStringLiteral("channel_marked") ||
               type == QStringLiteral("im_marked") ||
               type == QStringLiteral("mpim_marked")) {
        parseChannelUpdate(message);
    } else if (type == QStringLiteral("channel_joined") || type == QStringLiteral("group_joined")) {
        Chat* _chat = new Chat(message.value(QStringLiteral("channel")).toObject());
        QQmlEngine::setObjectOwnership(_chat, QQmlEngine::CppOwnership);
        emit channelJoined(_chat);
    } else if (type == QStringLiteral("im_open")) {
        emit chatJoined(message.value(QStringLiteral("channel")).toString());
    } else if (type == QStringLiteral("im_close")) {
        emit channelLeft(message.value(QStringLiteral("channel")).toString());
    } else if (type == QStringLiteral("channel_left") || type == QStringLiteral("group_left")) {
        emit channelLeft(message.value(QStringLiteral("channel")).toString());
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
        // invoke on the main thread
        QMetaObject::invokeMethod(qApp, [this, message] {
            if (m_teamInfo.users() != nullptr) {
                m_teamInfo.users()->updateUser(message.value(QStringLiteral("user")).toObject());
            }
        });
    } else if (type == QStringLiteral("team_join")) {
        qDebug() << "user joined" << message;
        // invoke on the main thread
        QMetaObject::invokeMethod(qApp, [this, message] {
            if (m_teamInfo.users() != nullptr) {
                m_teamInfo.users()->addUser(message.value(QStringLiteral("user")).toObject());
            }
        });
        //parseUser(message.value(QStringLiteral("user")).toObject());
    } else if (type == QStringLiteral("member_joined_channel")) {
        qDebug() << "user joined to channel" << message.value(QStringLiteral("user")).toString();
    } else if (type == QStringLiteral("pong")) {
    } else if (type == QStringLiteral("hello")) {
    } else if (type == QStringLiteral("emoji_changed")) {
        const QString& subtype = message.value(QStringLiteral("subtype")).toString();
        if (subtype == "add") {
            const QString& name = message.value(QStringLiteral("name")).toString();
            const QString& url = message.value(QStringLiteral("value")).toString();
            addTeamEmoji(name, url);
            ImagesCache::instance()->sendEmojisUpdated();
            m_teamInfo.setTeamsEmojisUpdated(true);
        }
    } else if (type == QStringLiteral("dnd_updated") || type == QStringLiteral("dnd_updated_user")) {
        parseUserDndChange(message);
    } else {
        qDebug() << "Unhandled message";
        qDebug().noquote() << QJsonDocument(message).toJson();
    }
}

void SlackTeamClient::parseChannelUpdate(const QJsonObject& message)
{
    DEBUG_BLOCK;
    //qDebug().noquote() << "channel updated" << QJsonDocument(channelData).toJson();
    const QString& channelId = message.value(QStringLiteral("channel")).toString();
    ChatsModel* _chatsModel = m_teamInfo.chats();
    if (_chatsModel == nullptr) {
        qWarning() << "Chats model not yet allocated";
        return;
    }
    Chat* chat = _chatsModel->chat(channelId);
    if (chat == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "Chat for channel ID" << channelId << "not found";
        return;
    }
    int unreadCountDisplay = message.value(QStringLiteral("unread_count_display")).toInt();
    QDateTime lastRead = slackToDateTime(message.value(QStringLiteral("ts")).toString());
    if (unreadCountDisplay != chat->unreadCountDisplay
            || lastRead != chat->lastRead) {
        chat->unreadCountDisplay = unreadCountDisplay;
        chat->lastRead = lastRead;
        emit channelUpdated(chat);
    }
}

//TODO: investigate the following type
//user id is empty QJsonObject({"comment":{"comment":"Okay, now I'm interested here. Light can travel less than a meter in a single nanosecond. Is \"nanosecond latency\" meaning \"measured in nanoseconds\"?","created":1531158771,"id":"FcBLM50VKJ","is_intro":false,"timestamp":1531158771,"user":"U8JRJRKEF"},"file":{"channels":["C21PKDHSL"],"comments_count":2,"created":1531158641,"display_as_bot":false,"editable":false,"external_type":"","filetype":"png","groups":[],"id":"FBM54SBDJ","image_exif_rotation":1,"ims":[],"initial_comment":{"comment":"For example here's a point-to-point laser router with nanosecond latency","created":1531158641,"id":"FcBMBYPSSE","is_intro":true,"timestamp":1531158641,"user":"U4R43AVMM"},"is_external":false,"is_public":true,"mimetype":"image/png","mode":"hosted","name":"image.png","original_h":183,"original_w":276,"permalink":"https://cpplang.slack.com/files/U4R43AVMM/FBM54SBDJ/image.png","permalink_public":"https://slack-files.com/T21Q22G66-FBM54SBDJ-4703532276","pretty_type":"PNG","public_url_shared":false,"size":29293,"thumb_160":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_160.png","thumb_360":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_360.png","thumb_360_h":183,"thumb_360_w":276,"thumb_64":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_64.png","thumb_80":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_80.png","timestamp":1531158641,"title":"image.png","url_private":"https://files.slack.com/files-pri/T21Q22G66-FBM54SBDJ/image.png","url_private_download":"https://files.slack.com/files-pri/T21Q22G66-FBM54SBDJ/download/image.png","user":"U4R43AVMM","username":""},"is_intro":false,"subtype":"file_comment","text":"<@U8JRJRKEF> commented on <@U4R43AVMM>’s file <https://cpplang.slack.com/files/U4R43AVMM/FBM54SBDJ/image.png|image.png>: Okay, now I'm interested here. Light can travel less than a meter in a single nanosecond. Is \"nanosecond latency\" meaning \"measured in nanoseconds\"?","ts":"1531158771.000413","type":"message"})
//no user for "" UsersModel(0x55555a081f80)
//"{\"dnd_suppressed\":true,\"text\":\"... has snoozed notifications. Send one anyway? <slack-action:\\/\\/BSLACKBOT\\/dnd-override\\/420349555508\\/1534958531000100\\/359371993316\\/1534958531.000900|Send notification>\\u200b\",\"username\":\"slackbot\",\"is_ephemeral\":true,\"user\":\"USLACKBOT\",\"type\":\"message\",\"subtype\":\"bot_message\",\"ts\":\"1534958531.000900\",\"channel\":\"DCCA9GBEY\",\"event_ts\":\"1534958531.000200\"}"
void SlackTeamClient::parseMessageUpdate(const QJsonObject& message)
{
    DEBUG_BLOCK;
//TODO: redesign
    const QString& subtype = message.value(QStringLiteral("subtype")).toString();
    const QJsonValue& submessage = message.value(QStringLiteral("message"));
    //channel id missed in sub messages
    const QString& channel_id = message.value(QStringLiteral("channel")).toString();
    Message* message_ = new Message;
    if (submessage.isUndefined()) {
        message_->setData(message);
    } else {
        message_->setData(submessage.toObject());
        message_->channel_id = channel_id;
        if (message_->subtype.isEmpty()) {
            message_->subtype = subtype;
        }
    }
    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }

    //fill up users for replys
    UsersModel* _usersModel = m_teamInfo.users();
    for(QObject* rplyObj : message_->replies) {
        ReplyField* rply = static_cast<ReplyField*>(rplyObj);
        rply->m_user = _usersModel->user(rply->m_userId);
    }

    MessageListModel *_messagesModel = _chatsModel->messages(channel_id);
    if (_messagesModel != nullptr) {
        _messagesModel->preprocessFormatting(_chatsModel, message_);
    }

    if (subtype == "message_changed" || subtype == "message_replied") {
       qDebug().noquote() << "message changed" << QJsonDocument(message).toJson();
       message_->isChanged = true;
       emit messageUpdated(message_);
       //TODO: implement handling all empty subtypes
    } else if (subtype == "message_deleted") {
        const QDateTime& deleted_ts = slackToDateTime(message.value(QStringLiteral("deleted_ts")).toString());
        const QString& channel_id = message.value(QStringLiteral("channel")).toString();
        emit messageDeleted(channel_id, deleted_ts);
    } else if (subtype == "file_comment") {
    } else {
        emit messageReceived(message_);
    }

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
    ChatsModel* _chatsModel = m_teamInfo.chats();
    if (_chatsModel == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "no chats model";
        return;
    }
    MessageListModel* _messagesModel = _chatsModel->messages(channelid);

    if (_messagesModel == nullptr) {
        qWarning() << "No messages for channel id:" << channelid;
        return;
    }
    Message* m = _messagesModel->message(ts);
    if (m != nullptr) {
        qDebug() << "found message at" << ts << "with reactions" << m->reactions.size();
        const QString& reaction = message.value(QStringLiteral("reaction")).toString();
        const QString& type = message.value(QStringLiteral("type")).toString();
        const QString& userid = message.value(QStringLiteral("user")).toString();
        Reaction* r = nullptr;
        //check if the reaction already there
        for (int i = 0; i < m->reactions.size(); i++) {
            Reaction* rea = static_cast<Reaction*>(m->reactions.at(i));
            qDebug() << "search reaction" << rea->name << reaction;
            if (rea->name == reaction) {
                r = rea;
                break;
            }
        }
        if (type == "reaction_added") {
            if (r == nullptr) {
                r = new Reaction;
                QQmlEngine::setObjectOwnership(r, QQmlEngine::CppOwnership);
                QString emojiPrepare = QString(":%1:").arg(reaction);
                qDebug() << "added new reaction" << emojiPrepare;
                MessageFormatter _formatter;
                _formatter.replaceEmoji(emojiPrepare);
                r->emoji = emojiPrepare;
                r->name = reaction;
                m->reactions.append(r);
                qDebug() << "added new reaction" << emojiPrepare << m->reactions.count();
            }
            r->userIds << userid;
        } else if (type == "reaction_removed") {
            if (r == nullptr) {
                qWarning() << "reaction" << reaction << "not found for message on channel" << channelid << "time" << ts;
                return;
            }
            m->reactions.removeOne(r);
            r->deleteLater();
        }
        _messagesModel->preprocessFormatting(_chatsModel, m);
        emit messageUpdated(m);
    } else {
        qWarning() << "message not found for ts" << ts;
    }
}

void SlackTeamClient::parseUserDndChange(const QJsonObject& message) {
    QList<QPointer<User>> _users;
    if (m_teamInfo.users() == nullptr) {
        qWarning() << "NO USERS IN TEAMINFO YET!!";
        return;
    }
    const QString& _userId = message.value(QStringLiteral("user")).toString();
    _users.append(m_teamInfo.users()->user(_userId));

    const QJsonObject& dndStatus = message.value(QStringLiteral("dnd_status")).toObject();
    bool _dnd = dndStatus.value("dnd_enabled").toBool(false);
    const QJsonValue& snoozeEnabled = message.value(QStringLiteral("snooze_enabled"));
    if (!snoozeEnabled.isUndefined()) {
        _dnd = snoozeEnabled.toBool();
    }

    QString presence = _dnd ? QStringLiteral("dnd_on") : QStringLiteral("dnd_off");
    emit usersPresenceChanged(_users, presence);
}

void SlackTeamClient::parsePresenceChange(const QJsonObject& message)
{
    DEBUG_BLOCK;

    QList<QPointer<User>> _users;
    if (m_teamInfo.users() == nullptr) {
        qWarning() << "NO USERS IN TEAMINFO YET!!";
        return;
    }
    const QJsonValue& _userValue = message.value(QStringLiteral("user"));
    if (!_userValue.isUndefined()) {
        const QString& userId = _userValue.toString();
        _users.append(m_teamInfo.users()->user(userId));
    }
    const QJsonValue& _usersValue = message.value(QStringLiteral("users"));
    if (!_usersValue.isUndefined()) {
        for (const QJsonValue& jsonUser : _usersValue.toArray()) {
            const QString& userId = jsonUser.toString();
            _users.append(m_teamInfo.users()->user(userId));
        }
    }

    const QString& presence = message.value(QStringLiteral("presence")).toString();
    //qWarning() << "presence" << presence;
    emit usersPresenceChanged(_users, presence);
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

QByteArray gUncompress(const QByteArray &data)
{
    if (data.size() <= 4) {
        qWarning("gUncompress: Input data is truncated");
        return QByteArray();
    }

    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    ret = inflateInit2(&strm, 15 +  32); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
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
        QByteArray baData = reply->readAll();
        //qDebug() << "received" << baData.size() << "bytes" << gUncompress(baData);
        if (baData.isEmpty()) {
            qWarning() << "No data returned";
            return QJsonObject();
        }
        QJsonDocument document = QJsonDocument::fromJson(gUncompress(baData), &error);

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

QNetworkReply *SlackTeamClient::executeGet(const QString& method, const QMap<QString, QString>& params, const QVariant& attribute)
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
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept-Encoding", "gzip, deflate");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    if (attribute.isValid()) {
        request.setAttribute(QNetworkRequest::User, attribute);
    }

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

    if (m_state == CONNECTED && config->hasUserInfo()) {
        qDebug() << "Already logged in";
        handleTestLoginReply();
    } else {
        QNetworkReply *reply = executeGet("auth.test");
        connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleTestLoginReply);
    }
}

void SlackTeamClient::handleTestLoginReply()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply != nullptr) {
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
    }
    requestTeamInfo();
    requestTeamEmojis();

    emit testLoginSuccess(config->userId(), config->teamId(), config->teamName());
    m_status = LOGGEDIN;
}

void SlackTeamClient::searchMessages(const QString &searchString, int page)
{
    DEBUG_BLOCK;

    QMap<QString, QString> params;
    params.insert(QStringLiteral("query"), searchString);
    params.insert(QStringLiteral("highlight"), QStringLiteral("false"));
    params.insert(QStringLiteral("sort"), QStringLiteral("timestamp"));
    params.insert(QStringLiteral("count"), QStringLiteral("100"));
    params.insert(QStringLiteral("page"), QString("%1").arg(page));

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
    QString query = data.value(QStringLiteral("query")).toString();
    const QJsonObject& messages = data.value(QStringLiteral("messages")).toObject();
    const QJsonObject& paging = messages.value(QStringLiteral("paging")).toObject();
    //qDebug().noquote() << "paging" << QJsonDocument(paging).toJson();
    int _total = messages.value(QStringLiteral("total")).toInt();
    int _page = paging.value(QStringLiteral("page")).toInt();
    int _pages = paging.value(QStringLiteral("pages")).toInt();
    QJsonArray matches = messages.value(QStringLiteral("matches")).toArray();
    emit searchMessagesReceived(messages.value(QStringLiteral("matches")).toArray(), _total, query, _page, _pages);
}

void SlackTeamClient::startClient()
{
    DEBUG_BLOCK

    qDebug() << "Start init";
    QMap<QString, QString> params;
    params.insert(QStringLiteral("batch_presence_aware"), QStringLiteral("1"));
    params.insert(QStringLiteral("presence_sub"), QStringLiteral("true"));
    QNetworkReply *reply = executeGet(QStringLiteral("rtm.connect"), params);
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
        setState(ClientStates::DISCONNECTED);
        emit disconnected(m_teamInfo.teamId());

        if (data.isEmpty()) {
            emit initFail(m_teamInfo.teamId(), tr("No data received from server"));
        } else {
            qDebug() << "data error" << data;
            emit initFail(m_teamInfo.teamId(), tr("Wat"));
        }
        return;
    }
    m_teamInfo.parseSelfData(data.value("self").toObject());

    QUrl url(data.value(QStringLiteral("url")).toString());
    stream->listen(url);
    m_status = STARTED;
    qDebug() << "connect success" << QThread::currentThreadId();
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

    QString relevant = currentText.mid(whitespaceBefore, whitespaceAfter- whitespaceBefore);
    if (relevant.startsWith("@")) {
        relevant.remove(0, 1);
    }

    QStringList nicks;
    for (QPointer<User> user : m_teamInfo.users()->users()) {
        const QString nick = user->username();
        if (!nick.isEmpty()) {
            if (relevant.isEmpty()) {
                nicks.append(nick);
            } else if (nick.contains(relevant, Qt::CaseInsensitive)) {
                nicks.append(nick);
            }
        }
    }

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

void SlackTeamClient::onFetchMoreSearchData(const QString &query, int page)
{
    searchMessages(query, page);
}

QString SlackTeamClient::lastChannel()
{
    DEBUG_BLOCK
    QString _lastChannel = m_teamInfo.lastChannel();
    if (_lastChannel.isEmpty()) {
        ChatsModel* _chatsModel = m_teamInfo.chats();
        if (_chatsModel != nullptr) {
            Chat* _generalChat = _chatsModel->generalChat();
            if (_generalChat != nullptr) {
                qDebug() << "taken general chat";
                _lastChannel = _generalChat->id;
            } else if (_chatsModel->rowCount() > 0){
                qDebug() << "taken 1st chat";
                _lastChannel = _chatsModel->chat(0)->id;
            }
        }
    } else {
        qDebug() << "taken last chat for team" << m_teamInfo.name();
    }

    qDebug() << "last channel" << _lastChannel;
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
    DEBUG_BLOCK;

    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }

    Chat* chat = _chatsModel->chat(channelId);

    if(chat == nullptr || chat->id.isEmpty()) {
        qWarning() << "Invalid channel ID provided" << channelId;
        return;
    }

    QMap<QString, QString> params;
    params.insert(QStringLiteral("name"), "#"+chat->name);

    QNetworkReply *reply = executeGet(QStringLiteral("channels.join"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleJoinChannelReply);
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

    QNetworkReply *reply = executePost(QStringLiteral("conversations.leave"), params);
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
    DEBUG_BLOCK;

    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    Chat* chat = _chatsModel->chat(chatId);
    if (chat == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "Chat for channel ID" << chatId << "not found";
        return;
    }
    if (chat->membersModel->users().first().isNull()) {
        qWarning() << "No user for chat" << chatId;
        return;
    }
    QMap<QString, QString> params;
    params.insert(QStringLiteral("user"), chat->membersModel->users().first()->userId());

    QNetworkReply *reply = executeGet(QStringLiteral("im.open"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleOpenChatReply);
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

    QNetworkReply *reply = executePost(QStringLiteral("conversations.close"), params);
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
    if (m_teamInfo.domain().isEmpty()) {
        QNetworkReply *reply = executeGet(QStringLiteral("team.info"));
        connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleTeamInfoReply);
    } else {
        handleTeamInfoReply();
    }
}

void SlackTeamClient::requestConversationsList(const QString& cursor)
{
    if (m_teamInfo.chats() == nullptr || m_teamInfo.chats()->rowCount() == 0 || !cursor.isEmpty()) {
        QMap<QString, QString> params;
        params.insert(QStringLiteral("limit"), "1000");
        params.insert(QStringLiteral("types"), "public_channel,private_channel,mpim,im");
        if (!cursor.isEmpty()) {
            params.insert(QStringLiteral("cursor"), cursor);
        }
        QNetworkReply *reply = executeGet(QStringLiteral("conversations.list"), params);
        connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleConversationsListReply);
    } else {
        handleConversationsListReply();
    }
}

void SlackTeamClient::requestConversationMembers(const QString &channelId, const QString &cursor)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("limit"), "1000");
    params.insert(QStringLiteral("channel"), channelId);
    if (!cursor.isEmpty()) {
        params.insert(QStringLiteral("cursor"), cursor);
    }
    QNetworkReply *reply = executeGet(QStringLiteral("conversations.members"), params, channelId);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleConversationMembersReply);
}

void SlackTeamClient::requestUsersList(const QString& cursor)
{
    qDebug() << __PRETTY_FUNCTION__ << m_teamInfo.users()->users().count();
    if (m_teamInfo.users() == nullptr || !m_teamInfo.users()->usersFetched() || !cursor.isEmpty()) {
        QMap<QString, QString> params;
        params.insert(QStringLiteral("limit"), "1000");
        if (!cursor.isEmpty()) {
            params.insert(QStringLiteral("cursor"), cursor);
        }
        QNetworkReply *reply = executeGet(QStringLiteral("users.list"), params);
        connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleUsersListReply);
    } else {
        handleUsersListReply();
    }
}

void SlackTeamClient::requestTeamEmojis()
{
    if (m_teamInfo.teamsEmojisUpdated() == false) {
        QNetworkReply *reply = executeGet(QStringLiteral("emoji.list"));
        connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleTeamEmojisReply);
    }
}

void SlackTeamClient::requestConversationInfo(const QString &channelId)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);
    QNetworkReply *reply = executeGet(QStringLiteral("conversations.info"), params, channelId);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleConversationInfoReply);
}

void SlackTeamClient::requestUserInfo(User *user)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("user"), user->userId());
    QNetworkReply *reply = executeGet(QStringLiteral("users.info"), params);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleUsersInfoReply);
}

QString SlackTeamClient::userName(const QString &userId) {
    if (teamInfo()->users() == nullptr) {
        return "";
    }

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
    if (reply != nullptr) {
        QJsonObject data = getResult(reply);
        reply->deleteLater();

        if (isError(data)) {
            qDebug() << "Team info failed" << data;
        } else {
            m_teamInfo.parseTeamInfoData(data.value("team").toObject());
            config->saveTeamInfo(m_teamInfo);
            emit teamInfoChanged(m_teamInfo.teamId());
        }
    }
    qDebug() << __PRETTY_FUNCTION__ << "teaminfo:" << m_teamInfo.name() << m_teamInfo.teamId();
    requestUsersList("");
}

void SlackTeamClient::addTeamEmoji(const QString& name, const QString& url) {
    ImagesCache* imagesCache = ImagesCache::instance();
    EmojiInfo *einfo = new EmojiInfo;
    einfo->m_name = name;
    einfo->m_shortNames << name;
    einfo->m_image = url;
    einfo->m_imagesExist |= EmojiInfo::ImageSlackTeam;
    einfo->m_category = EmojiInfo::EmojiCategoryCustom;
    einfo->m_teamId = m_teamInfo.teamId();
    //qDebug() << "edding emoji" << einfo->m_shortNames << einfo->m_image << einfo->m_category;
    imagesCache->addEmoji(einfo);
}

void SlackTeamClient::handleTeamEmojisReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        qDebug() << "Team emoji failed";
    }

    ImagesCache* imagesCache = ImagesCache::instance();
    QJsonObject teamEmojisArr = data.value(QStringLiteral("emoji")).toObject();
    //qDebug() << "team emojis:" << data << teamEmojisArr.count();
    for (const QString &name : teamEmojisArr.keys()) {
        QString emoji_url = teamEmojisArr.value(name).toString();
        if (emoji_url.startsWith("alias:")) {
            imagesCache->addEmojiAlias(name, emoji_url.remove(0, 6));
        } else {
            addTeamEmoji(name, emoji_url);
        }
    }
    imagesCache->sendEmojisUpdated();
    m_teamInfo.setTeamsEmojisUpdated(true);
}

void SlackTeamClient::loadMessages(const QString& channelId, const QDateTime& latest)
{
    DEBUG_BLOCK;
    qDebug() << "Loading messages" << channelId << latest << sender();
    if (channelId.isEmpty()) {
        qWarning() << __PRETTY_FUNCTION__ << "Empty channel id";
        return;
    }
    QDateTime _latest = latest;
    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    Chat* chat = _chatsModel->chat(channelId);
    if (chat == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "Chat for channel ID" << channelId << "not found";
        return;
    }
    if (!_latest.isValid()) {
        MessageListModel* mesgs = _chatsModel->messages(channelId);
        //check if send from QML (sender == null)
        if (sender() == nullptr && mesgs->historyLoaded()) {
            // requested from QML with no timestamp and there is messages in the model
            // so emit that we already have messages
            // reduced number of requests when switching models
            emit loadMessagesSuccess(m_teamInfo.teamId(), channelId);
            return;
        }
        if (mesgs != nullptr) {
            _latest = mesgs->firstMessageTs();
        }
    }
    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);
    params.insert(QStringLiteral("count"), "50");
    if (_latest.isValid()) {
        params.insert(QStringLiteral("latest"), dateTimeToSlack(_latest));
        params.insert(QStringLiteral("inclusive"), "0");
    } else {
        qWarning() << "NO LATEST!!";
    }

    QNetworkReply *reply = executeGet(historyMethod(chat->type), params);
    reply->setProperty("channelId", channelId);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleLoadMessagesReply, Qt::QueuedConnection);
}

void SlackTeamClient::handleLoadMessagesReply()
{
    DEBUG_BLOCK;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        emit loadMessagesFail(m_teamInfo.teamId());
        return;
    }

    QJsonArray messageList = data.value(QStringLiteral("messages")).toArray();
    bool _hasMore = data.value(QStringLiteral("has_more")).toBool(false);

    QString channelId = reply->property("channelId").toString();
    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    if (!_chatsModel->hasChannel(channelId)) {
        qWarning() << __PRETTY_FUNCTION__ << "Team" << m_teamInfo.teamId() << "does not have channel" << channelId;
        return;
    }

    MessageListModel *messageModel = _chatsModel->messages(channelId);
    if (messageModel == nullptr) {
        emit loadMessagesFail(m_teamInfo.teamId());
        return;
    }
#if 0
        {
            QFile f("msglist_dumps_" + m_teamInfo.name() + "_" + channelId + ".json");
            if (f.open(QIODevice::Append)) {
                f.write(QJsonDocument(data).toJson());
                f.close();
            }
        }
#endif

    QList<Message*> _mlist;
    UsersModel* _usersModel = m_teamInfo.users();
    int threadMsgsCount = 0;
    for (const QJsonValue &messageData : messageList) {
        const QJsonObject messageObject = messageData.toObject();
        if (messageObject.value(QStringLiteral("subtype")).toString() == "file_comment") {
            qWarning() << "file comment. skipping for now";
            continue; //TODO: not yet supported
        }
        Message* message = new Message;
        //qDebug() << "message obj" << messageObject;
        message->setData(messageObject);

        if (message->channel_id.isEmpty()) {
            message->channel_id = channelId;
        }
        if (!message->user_id.isEmpty()) {
            message->user = _usersModel->user(message->user_id);
        } else {
            qWarning() << "user id is empty" << messageObject;
        }

        //fill up users for replys
        UsersModel* _usersModel = m_teamInfo.users();
        for(QObject* rplyObj : message->replies) {
            ReplyField* rply = static_cast<ReplyField*>(rplyObj);
            rply->m_user = _usersModel->user(rply->m_userId);
        }
        messageModel->preprocessFormatting(_chatsModel, message);
        if (messageModel->isMessageThreadChild(message)) {
            threadMsgsCount++;
        }
        _mlist.append(message);
    }
    emit messagesReceived(channelId, _mlist, _hasMore, threadMsgsCount);
    messageModel->setHistoryLoaded(true);

    qDebug() << "messages loaded for" << channelId << _chatsModel->chat(channelId)->name << m_teamInfo.teamId() << m_teamInfo.name() << _mlist.count();
    emit loadMessagesSuccess(m_teamInfo.teamId(), channelId);
}

QString SlackTeamClient::markMethod(ChatsModel::ChatType type)
{
    DEBUG_BLOCK

    if (type == ChatsModel::Channel) {
        return QStringLiteral("channels.mark");
    } else if (type == ChatsModel::Group) {
        return QStringLiteral("groups.mark");
    } else if (type == ChatsModel::MultiUserConversation) {
        return QStringLiteral("mpim.mark");
    } else if (type == ChatsModel::Conversation) {
        return QStringLiteral("im.mark");
    } else {
        return "";
    }
}

SlackTeamClient::ClientStatus SlackTeamClient::getStatus() const
{
    qDebug() << "status" << m_teamInfo.name() << m_status;
    return m_status;
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

void SlackTeamClient::markChannel(ChatsModel::ChatType type, const QString& channelId, const QDateTime &time)
{
    DEBUG_BLOCK;

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);
    QDateTime dt = time;
    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    if (dt.isNull() || !dt.isValid()) {
        auto messagesModel = _chatsModel->messages(channelId);
        if (messagesModel != nullptr) {
            if (messagesModel->rowCount(QModelIndex()) > 0) {
                dt = messagesModel->message(0)->time;
            }
        } else {
            qDebug() << "message model not ready for the channel" << channelId;
        }
    }
    if (!dt.isValid()) {
        qWarning() << "Cant find timestamp for the channel" << channelId;
        return;
    }
    //chatsModel->chat(channelId)
    params.insert(QStringLiteral("ts"), dateTimeToSlack(dt));

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

void SlackTeamClient::postMessage(const QString& channelId, QString content, const QDateTime &thread_ts)
{
    DEBUG_BLOCK;
    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("text"), content);
    data.insert(QStringLiteral("as_user"), QStringLiteral("true"));
    data.insert(QStringLiteral("parse"), QStringLiteral("full"));
    if (thread_ts.isValid()) {
        data.insert(QStringLiteral("thread_ts"), dateTimeToSlack(thread_ts));
    }

    QNetworkReply *reply = executePost(QStringLiteral("chat.postMessage"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handlePostMessageReply);
}

void SlackTeamClient::updateMessage(const QString &channelId, QString content, const QDateTime &ts)
{
    DEBUG_BLOCK;
    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("text"), content);
    data.insert(QStringLiteral("as_user"), QStringLiteral("true"));
    data.insert(QStringLiteral("parse"), QStringLiteral("full"));
    data.insert(QStringLiteral("ts"), dateTimeToSlack(ts));

    QNetworkReply *reply = executePost(QStringLiteral("chat.update"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handlePostMessageReply);
}

void SlackTeamClient::deleteMessage(const QString &channelId, const QDateTime &ts)
{
    DEBUG_BLOCK;
    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("ts"), dateTimeToSlack(ts));
    data.insert(QStringLiteral("as_user"), QStringLiteral("true"));

    QNetworkReply *reply = executePost(QStringLiteral("chat.delete"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleDeleteMessageReply);
}

void SlackTeamClient::handleDeleteMessageReply()
{
    DEBUG_BLOCK;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    qDebug() << "Delete message result" << data;
    reply->deleteLater();
}

void SlackTeamClient::handleConversationsListReply()
{
    DEBUG_BLOCK;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QList<Chat*> _chats;
    QString cursor;
    if (reply != nullptr) {
        const QJsonObject& data = getResult(reply);
        //qDebug() << __PRETTY_FUNCTION__ << "result" << data;
        cursor = data.value("response_metadata").toObject().value("next_cursor").toString();
        reply->deleteLater();
        for (const QJsonValue& chatValue : data.value("channels").toArray()) {
            Chat* chat = new Chat(chatValue.toObject());
            QQmlEngine::setObjectOwnership(chat, QQmlEngine::CppOwnership);
            if (chat == nullptr) {
                qWarning() << __PRETTY_FUNCTION__ << "Error allocating memory for Chat";
            }
            _chats.append(chat);
        }
    }
    emit conversationsDataChanged(_chats, cursor.isEmpty());

#if 0
        {
            QFile f("chatslist_dumps_" + m_teamInfo.name() + ".json");
            if (f.open(QIODevice::Append)) {
                f.write(QJsonDocument(data).toJson());
                f.close();
            }
        }
#endif

    QJsonArray presenceIds;

    for (Chat* chat : _chats) {
        if (chat->type != ChatsModel::Conversation && chat->type != ChatsModel::Channel) {
            requestConversationMembers(chat->id, "");
        }
        if (chat->type == ChatsModel::Conversation && !chat->user.isEmpty()) {
            presenceIds.append(QJsonValue(chat->user));
        }
    }
    //create presence status request
    if (presenceIds.count() > 0) {
        QJsonObject presenceRequest;
        presenceRequest.insert(QStringLiteral("type"), QJsonValue(QStringLiteral("presence_sub")));
        presenceRequest.insert(QStringLiteral("ids"), QJsonValue(presenceIds));
        QJsonDocument document(presenceRequest);
        stream->sendBinaryMessage(document.toJson(QJsonDocument::Compact));
    }

    if (!cursor.isEmpty()) {
        requestConversationsList(cursor);
    } else {
        if (m_teamInfo.lastChannel().isEmpty()) {
            m_teamInfo.setLastChannel(lastChannel());
        }
        emit initSuccess(m_teamInfo.teamId());
        m_status = INITED;
    }
}

void SlackTeamClient::handleUsersListReply()
{
    DEBUG_BLOCK;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QList<QPointer<User>> _users;
    QString cursor;
    if (reply != nullptr) {
        const QJsonObject& data = getResult(reply);
        cursor = data.value("response_metadata").toObject().value("next_cursor").toString();
        //qDebug() << __PRETTY_FUNCTION__ << "result" << data;
        reply->deleteLater();
        for (const QJsonValue& userValue : data.value("members").toArray()) {
            QPointer<User> user = new User(nullptr);
            QQmlEngine::setObjectOwnership(user, QQmlEngine::CppOwnership);
            user->setData(userValue.toObject());
            _users.append(user);
        }
        if (!cursor.isEmpty()) {
            requestUsersList(cursor);
        }
    }
    emit usersDataChanged(_users, cursor.isEmpty());
}

void SlackTeamClient::handleConversationMembersReply()
{
    DEBUG_BLOCK;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    const QJsonObject& data = getResult(reply);
    QString cursor = data.value("response_metadata").toObject().value("next_cursor").toString();
    //qDebug() << __PRETTY_FUNCTION__ << "result" << data;
    QString channelId = reply->request().attribute(QNetworkRequest::User).toString();
    //qDebug() << __PRETTY_FUNCTION__ << "channel id" << channelId;
    reply->deleteLater();
    QStringList _members;

    for (const QJsonValue& memberjson : data.value("members").toArray()) {
        _members.append(memberjson.toString());
    }

    emit conversationMembersChanged(channelId, _members, cursor.isEmpty());
    if (!cursor.isEmpty()) {
        requestConversationMembers(channelId, cursor);
    }
}

void SlackTeamClient::handleConversationInfoReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    const QJsonObject& data = getResult(reply);
    //qDebug().noquote() << __PRETTY_FUNCTION__ << "result" << data;
    const QString& channelId = reply->request().attribute(QNetworkRequest::User).toString();
    //qDebug() << __PRETTY_FUNCTION__ << "channel id" << channelId;
    reply->deleteLater();
    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    Chat* chat = _chatsModel->chat(channelId);
    if (chat != nullptr) {
        chat->setData(data.value("channel").toObject());
        emit channelUpdated(chat);
    }
}

void SlackTeamClient::handleUsersInfoReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    const QJsonObject& data = getResult(reply);
    qDebug().noquote() << __PRETTY_FUNCTION__ << "result" << data;
    reply->deleteLater();
    // invoke on the main thread
    QMetaObject::invokeMethod(qApp, [this, data] {
        if (m_teamInfo.users() != nullptr) {
            m_teamInfo.users()->updateUser(data.value(QStringLiteral("user")).toObject());
        }
    });

}
void SlackTeamClient::handleDeleteReactionReply()
{
    DEBUG_BLOCK;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    qDebug() << "Delete reaction result" << data;
    reply->deleteLater();
}

void SlackTeamClient::handleAddReactionReply()
{
    DEBUG_BLOCK;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    qDebug() << "Add reaction result" << data;
    reply->deleteLater();
}

void SlackTeamClient::handlePostMessageReply()
{
    DEBUG_BLOCK;
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    qDebug() << "Post/update message result" << data;
    reply->deleteLater();
}

void SlackTeamClient::postImage(const QString& channelId, const QString& imagePath,
                            const QString& title, const QString& comment)
{
    DEBUG_BLOCK;

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
