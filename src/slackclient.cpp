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
#include "debugblock.h"

#include "MessagesModel.h"

const QMap<QString, QString> SlackTeamClient::kSlackErrors = {
    //TODO: add more errors
    { "invalid_auth", "Some aspect of authentication cannot be validated. Either the provided token is invalid or the request originates from an IP address disallowed from making the request." },
    { "not_authed", "No authentication token provided." },
    { "not_admin", "Only admins can update the profile of another user. Some fields, like email may only be updated by an admin." },
    { "request_timeout", "The method was called via a POST request, but the POST data was either missing or truncated." },
    { "fatal_error", "The server could not complete your operation(s). It's possible some aspect of the operation succeeded before the error was raised." },
    { "name_taken", "A channel cannot be created with the given name." },
    { "no_channel", "Value passed for name was empty." },
    { "invalid_name", "Value passed for name was invalid." },
    { "account_inactive", "Authentication token is for a deleted user or workspace." },
    { "token_revoked", "Authentication token is for a deleted user or workspace or the app has been removed." },
    { "no_permission", "The workspace token used in this request does not have the permissions necessary to complete the request. Make sure your app is a member of the conversation it's attempting to post a message to." },
    { "user_is_bot", "This method cannot be called by a bot user." },
    { "org_login_required", "The workspace is undergoing an enterprise migration and will not be available until migration is complete." },
    { "channel_not_found", "Value passed for channel was invalid." },
    { "is_archived", "Channel has been archived." },
    { "already_reacted", "The specified item already has the user/reaction combination." },
    { "too_many_reactions", "The limit for reactions a person may add to the item has been reached." },
    { "no_reaction", "The specified item does not have the user/reaction combination." },
    { "invalid_name_specials", "Value passed for name contained unallowed special characters or upper case characters." },
    { "snooze_not_active", "Snooze is not active for this user and cannot be ended" },
    { "snooze_end_failed", "There was a problem setting the user's Do Not Disturb status" },
    { "too_long", "Do Not Disturb interval too long" },
    { "user_is_restricted", "Cannot join the channel by a restricted user or single channel guest." },
    { "file_not_found", "The file does not exist, or is not visible to the calling user." },
    { "file_deleted", "The file has already been deleted." },
    { "cant_delete_file", "Authenticated user does not have permission to delete this file." },
    { "user_not_found", "Value passed for user was invalid" },
    { "message_not_found", "No message exists with the requested timestamp." }
};

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
    config->saveTeamInfo(m_teamInfo);
}

void SlackTeamClient::startConnections()
{
    networkAccessManager = new QNetworkAccessManager(this);

    stream = new SlackStream(this);
    reconnectTimer = new QTimer(this);
#ifndef Q_OS_WIN //reports wrong state on Windows
    networkAccessible = networkAccessManager->networkAccessible();
#endif
    connect(networkAccessManager.data(), &QNetworkAccessManager::networkAccessibleChanged, this, &SlackTeamClient::handleNetworkAccessibleChanged);
    connect(reconnectTimer.data(), &QTimer::timeout, this, &SlackTeamClient::reconnectClient);

    connect(stream.data(), &SlackStream::connected, this, &SlackTeamClient::handleStreamStart);
    connect(stream.data(), &SlackStream::disconnected, this, &SlackTeamClient::handleStreamEnd);
    connect(stream.data(), &SlackStream::messageReceived, this, &SlackTeamClient::handleStreamMessage);

    connect(this, &SlackTeamClient::connected, this, &SlackTeamClient::isOnlineChanged);
    connect(this, &SlackTeamClient::initSuccess, this, &SlackTeamClient::isOnlineChanged);
    connect(this, &SlackTeamClient::disconnected, this, &SlackTeamClient::isOnlineChanged);
    m_teamInfo.createModels(this);
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

    if (type != "pong" && type != "user_typing") {
        qDebug() << "stream message type" << type;
    }
    //qDebug().noquote() << QJsonDocument(message).toJson();

    if (type == QStringLiteral("message")) {
        parseMessageUpdate(message);
    } else if (type == QStringLiteral("group_marked") ||
               type == QStringLiteral("channel_marked") ||
               type == QStringLiteral("im_marked") ||
               type == QStringLiteral("mpim_marked")) {
        parseChannelUpdate(message);
    } else if (type == QStringLiteral("im_created")
               || type == QStringLiteral("channel_created")) {
        createChannelIfNeeded(message.value(QStringLiteral("channel")).toObject());
    } else if (type == QStringLiteral("im_open") ||
               type == QStringLiteral("channel_joined") ||
               type == QStringLiteral("group_joined")) {
        const QJsonValue& chan = message.value(QStringLiteral("channel"));
        const QString& channelId = chan.isString() ? chan.toString() : chan.toObject().value("id").toString();
        ChatsModel* _chatsModel = m_teamInfo.chats();
        if (_chatsModel == nullptr) {
            qWarning() << "Chats model not yet allocated";
            return;
        }
        Chat* _chat = _chatsModel->chat(channelId);
        if (_chat == nullptr) {
            qWarning() << __PRETTY_FUNCTION__ << "Chat for channel ID" << channelId << "not found";
            return;
        }
        //consider chat is opened
        _chat->isOpen = true;
        emit channelJoined(_chat);
    } else if (type == QStringLiteral("im_close")  || type == QStringLiteral("mpim_close")
               || type == QStringLiteral("channel_left") || type == QStringLiteral("group_left") ||
               type == QStringLiteral("group_close")) {
        emit channelLeft(message.value(QStringLiteral("channel")).toString());
    } else if (type == QStringLiteral("presence_change")) {
        parsePresenceChange(message);
    } else if (type == QStringLiteral("manual_presence_change")) {
        parsePresenceChange(message, true);
    } else if (type == QStringLiteral("desktop_notification")) {
        parseNotification(message);
    } else if (type == QStringLiteral("reaction_added") || type == QStringLiteral("reaction_removed")) {
        parseReactionUpdate(message);
    } else if (type == QStringLiteral("user_typing")) {
        emit userTyping(m_teamInfo.teamId(),
                        message.value(QStringLiteral("channel")).toString(),
                        userName(message.value(QStringLiteral("user")).toString()));
    } else if (type == QStringLiteral("user_change")) {
        // invoke on the main thread
        QMetaObject::invokeMethod(qApp, [this, message] {
            if (m_teamInfo.users() != nullptr) {
                m_teamInfo.users()->updateUser(message.value(QStringLiteral("user")).toObject());
            }
        }, Qt::QueuedConnection);
    } else if (type == QStringLiteral("team_join")) {
        // invoke on the main thread
        QMetaObject::invokeMethod(qApp, [this, message] {
            if (m_teamInfo.users() != nullptr) {
                m_teamInfo.users()->addUser(message.value(QStringLiteral("user")).toObject());
            }
        });
    } else if (type == QStringLiteral("member_joined_channel")) {
        QStringList _members(message.value(QStringLiteral("user")).toString());
        qDebug() << "user joined to channel" << _members;
        emit conversationMembersChanged(message.value(QStringLiteral("channel")).toString(),
                                        _members, true);
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
    } else if (type == "error") {
        QJsonObject errorData;
        errorData["domain"] = "Slack RTM error";
        const QJsonObject& errObj = message.value("error").toObject();
        errorData["error_str"] = QString("%1. code: %2").arg(errObj.value("msg").toString()).
                arg(errObj.value("code").toInt());
        emit error(errorData);
    } else if (type == "file_shared") {
        const QString& fileId = message.value(QStringLiteral("file_id")).toString();
        requestSharedFileInfo(fileId);
    } else if (type == "file_deleted") {
        const QString& fileId = message.value(QStringLiteral("file_id")).toString();
        QMetaObject::invokeMethod(qApp, [this, fileId] {
            if (m_teamInfo.fileSharesModel() != nullptr) {
                m_teamInfo.fileSharesModel()->deleteFile(fileId);
            }
        });
    } else if (type == "file_change") {
        const QString& fileId = message.value(QStringLiteral("file_id")).toString();
        requestSharedFileInfo(fileId);
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
    const QDateTime& lastRead = slackToDateTime(message.value(QStringLiteral("ts")).toString());
    if (unreadCountDisplay != chat->unreadCountDisplay
            || lastRead != chat->lastRead) {
        chat->unreadCountDisplay = unreadCountDisplay;
        chat->unreadCountPersonal = 0;
        chat->lastRead = lastRead;
        emit channelUpdated(chat);
    }
}

//TODO: investigate the following type
//user id is empty QJsonObject({"comment":{"comment":"Okay, now I'm interested here. Light can travel less than a meter in a single nanosecond. Is \"nanosecond latency\" meaning \"measured in nanoseconds\"?","created":1531158771,"id":"FcBLM50VKJ","is_intro":false,"timestamp":1531158771,"user":"U8JRJRKEF"},"file":{"channels":["C21PKDHSL"],"comments_count":2,"created":1531158641,"display_as_bot":false,"editable":false,"external_type":"","filetype":"png","groups":[],"id":"FBM54SBDJ","image_exif_rotation":1,"ims":[],"initial_comment":{"comment":"For example here's a point-to-point laser router with nanosecond latency","created":1531158641,"id":"FcBMBYPSSE","is_intro":true,"timestamp":1531158641,"user":"U4R43AVMM"},"is_external":false,"is_public":true,"mimetype":"image/png","mode":"hosted","name":"image.png","original_h":183,"original_w":276,"permalink":"https://cpplang.slack.com/files/U4R43AVMM/FBM54SBDJ/image.png","permalink_public":"https://slack-files.com/T21Q22G66-FBM54SBDJ-4703532276","pretty_type":"PNG","public_url_shared":false,"size":29293,"thumb_160":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_160.png","thumb_360":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_360.png","thumb_360_h":183,"thumb_360_w":276,"thumb_64":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_64.png","thumb_80":"https://files.slack.com/files-tmb/T21Q22G66-FBM54SBDJ-d21d39f46d/image_80.png","timestamp":1531158641,"title":"image.png","url_private":"https://files.slack.com/files-pri/T21Q22G66-FBM54SBDJ/image.png","url_private_download":"https://files.slack.com/files-pri/T21Q22G66-FBM54SBDJ/download/image.png","user":"U4R43AVMM","username":""},"is_intro":false,"subtype":"file_comment","text":"<@U8JRJRKEF> commented on <@U4R43AVMM>’s file <https://cpplang.slack.com/files/U4R43AVMM/FBM54SBDJ/image.png|image.png>: Okay, now I'm interested here. Light can travel less than a meter in a single nanosecond. Is \"nanosecond latency\" meaning \"measured in nanoseconds\"?","ts":"1531158771.000413","type":"message"})
//"{\"dnd_suppressed\":true,\"text\":\"... has snoozed notifications. Send one anyway? <slack-action:\\/\\/BSLACKBOT\\/dnd-override\\/420349555508\\/1534958531000100\\/359371993316\\/1534958531.000900|Send notification>\\u200b\",\"username\":\"slackbot\",\"is_ephemeral\":true,\"user\":\"USLACKBOT\",\"type\":\"message\",\"subtype\":\"bot_message\",\"ts\":\"1534958531.000900\",\"channel\":\"DCCA9GBEY\",\"event_ts\":\"1534958531.000200\"}"
void SlackTeamClient::parseMessageUpdate(const QJsonObject& message)
{
    DEBUG_BLOCK;
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

    //fill up users for replies
    UsersModel* _usersModel = m_teamInfo.users();
    for(QObject* rplyObj : message_->replies) {
        ReplyField* rply = static_cast<ReplyField*>(rplyObj);
        rply->m_user = _usersModel->user(rply->m_userId);
    }

    MessageListModel* _messagesModel = _chatsModel->messages(channel_id);
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
}

void SlackTeamClient::parseReactionUpdate(const QJsonObject &message)
{
    DEBUG_BLOCK;

    //"{\"type\":\"reaction_added\",\"user\":\"U4NH7TD8D\",\"item\":{\"type\":\"message\",\"channel\":\"C09PZTN5S\",\"ts\":\"1525437188.000435\"},\"reaction\":\"slightly_smiling_face\",\"item_user\":\"U3ZC1RYJG\",\"event_ts\":\"1525437431.000236\",\"ts\":\"1525437431.000236\"}"
    //"{\"type\":\"reaction_removed\",\"user\":\"U4NH7TD8D\",\"item\":{\"type\":\"message\",\"channel\":\"C0CK10FA9\",\"ts\":\"1526478339.000316\"},\"reaction\":\"+1\",\"item_user\":\"U1WTHK18E\",\"event_ts\":\"1526736040.000047\",\"ts\":\"1526736040.000047\"}"
    const QJsonObject& item = message.value(QStringLiteral("item")).toObject();
    const QString& ts = item.value(QStringLiteral("ts")).toString();
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
            qDebug() << "search reaction" << rea->name() << reaction;
            if (rea->name() == reaction) {
                r = rea;
                break;
            }
        }
        if (type == "reaction_added") {
            if (r == nullptr) {
                r = new Reaction;
                r->moveToThread(qApp->thread());
                QQmlEngine::setObjectOwnership(r, QQmlEngine::CppOwnership);
                QString emojiPrepare = QString(":%1:").arg(reaction);
                qDebug() << "added new reaction" << emojiPrepare;
                MessageFormatter::replaceEmoji(emojiPrepare);
                r->setEmojiInfo(ImagesCache::instance()->getEmojiInfo(reaction));
                r->setName(reaction);
                m->reactions.append(r);
                qDebug() << "added new reaction" << emojiPrepare << m->reactions.count();
            }
            r->m_userIds << userid;
        } else if (type == "reaction_removed") {
            if (r == nullptr) {
                qWarning() << "reaction" << reaction << "not found for message on channel" << channelid << "time" << ts;
                return;
            }
            r->m_userIds.removeOne(userid);
        }
        _messagesModel->preprocessFormatting(_chatsModel, m);
        emit messageUpdated(m, false);
    } else {
        qWarning() << "message not found for ts" << ts;
    }
}

void SlackTeamClient::parseUserDndChange(const QJsonObject& message) {
    QStringList _userIds;

    qDebug().noquote() << "dnd message" << QJsonDocument(message).toJson();
    _userIds << message.value(QStringLiteral("user")).toString();

    const QJsonObject& dndStatus = message.value(QStringLiteral("dnd_status")).toObject();
    bool _dnd = dndStatus.value(QStringLiteral("dnd_enabled")).toBool(false);
    const QJsonValue& snoozeEnabled = dndStatus.value(QStringLiteral("snooze_enabled"));
    QString presence = _dnd ? QStringLiteral("dnd_on") : QStringLiteral("dnd_off");
    QDateTime snoozeEnd;
    if (!snoozeEnabled.isUndefined()) {
        _dnd = snoozeEnabled.toBool();
        int snoole_endtime = dndStatus.value("snooze_endtime").toInt(0);
        snoozeEnd = QDateTime::fromSecsSinceEpoch(snoole_endtime);
        presence = _dnd ? QStringLiteral("dnd_on") : QStringLiteral("dnd_off");
    }
    //qDebug() << "presence" << _userIds << _dnd << presence << snoozeEnabled << snoozeEnd;
    emit usersPresenceChanged(_userIds, presence, snoozeEnd);
}

void SlackTeamClient::parsePresenceChange(const QJsonObject& message, bool force)
{
    DEBUG_BLOCK;

    //qDebug().noquote() << QJsonDocument(message).toJson() << force;

    QStringList _userIds;
    const QJsonValue& _userValue = message.value(QStringLiteral("user"));
    if (!_userValue.isUndefined()) {
        _userIds << _userValue.toString();
    } else {
        const QJsonValue& _usersValue = message.value(QStringLiteral("users"));
        if (!_usersValue.isUndefined()) {
            for (const auto& jsonUser : _usersValue.toArray()) {
                _userIds << jsonUser.toString();
            }
        }
    }
    //if no users mentioned, its for self user
    if (_userIds.isEmpty()) {
        _userIds << m_teamInfo.selfId();
    }

    const QString& presence = message.value(QStringLiteral("presence")).toString();
    //qWarning() << "presence" << presence << force;
    emit usersPresenceChanged(_userIds, presence, QDateTime(), force);
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

bool SlackTeamClient::isError(QJsonObject &data)
{
    DEBUG_BLOCK

    if (data.isEmpty()) {
        qWarning() << "No data received";
        return true;
    }

    bool ok = data.value(QStringLiteral("ok")).toBool(false);

    if (ok) {
        return false;
    }

    qDebug() << "error" << data;
    data["domain"] = "Slack error";
    const QString& sl_err = data.value("error").toString();
    data["error_str"] = kSlackErrors.value(sl_err);
    if (data.value("error_str").toString().isEmpty()) {
        data["error_str"] = sl_err;
    }
    emit error(data);
    return true;
}

QJsonObject SlackTeamClient::getResult(QNetworkReply *reply)
{
    DEBUG_BLOCK;

    QJsonObject errorJson;
    if (!isOk(reply)) {
        qWarning() << "network error";
        errorJson["ok"] = false;
        errorJson["domain"] = "Network error";
        errorJson["error_str"] = reply->errorString() +
                QString(". HTTP code: %1").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
        return errorJson;
    }

    QJsonParseError error;
    const QByteArray& baData = reply->readAll();

    if (baData.isEmpty()) {
        qWarning() << "No data returned";
        return QJsonObject();
    }
    QJsonDocument document = QJsonDocument::fromJson(baData, &error);

    if (error.error != QJsonParseError::NoError) {
        return QJsonObject();
    }

    return document.object();
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
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         QVariant(int(QNetworkRequest::AlwaysNetwork)));
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    if (attribute.isValid()) {
        request.setAttribute(QNetworkRequest::User, attribute);
    }

    qDebug() << "GET" << url.toString();
    return networkAccessManager->get(request);
}

QNetworkReply *SlackTeamClient::executePost(const QString& method, const QByteArray &data)
{
    DEBUG_BLOCK

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), m_teamInfo.teamToken());

    QUrl url(QStringLiteral("https://slack.com/api/") + method);
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=utf-8"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, data.length());

    qDebug() << "POST (2)" << url.toString() << data;// << query.toString();
    return networkAccessManager->post(request, data);
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

QNetworkReply *SlackTeamClient::executePostWithFile(const QString& method, const QMap<QString, QString> &formdata,
                                                    QFile *file, const QString& fileFormData)
{
    DEBUG_BLOCK;

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), m_teamInfo.teamToken());
    QHttpMultiPart *dataParts = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    //set content type
    QFileInfo _fi(*file);
    const QString& ext = _fi.completeSuffix().toLower();
    if (ext == "gif") {
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/gif");
    } else if (ext == "png") {
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
    } else if (ext == "jpg" || ext == "jpeg") {
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
    }
    if (fileFormData.isEmpty()) {
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"file\"; filename=\"") + file->fileName() + "\"");
    } else {
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, fileFormData);
    }

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
    url.setQuery(query);
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
            qWarning() << "Test login failed";
            m_status = LOGINFAILED;
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

void SlackTeamClient::requestSharedFiles(int page, const QString& channelId, const QString& userId)
{
    DEBUG_BLOCK;

    QMap<QString, QString> params;
    if (!channelId.isEmpty()) {
        params.insert(QStringLiteral("channel"), channelId);
    }
    if (!userId.isEmpty()) {
        params.insert(QStringLiteral("user"), userId);
    }
    params.insert(QStringLiteral("count"), QStringLiteral("100"));
    params.insert(QStringLiteral("page"), QString("%1").arg(page));

    QNetworkReply *reply = executeGet(QStringLiteral("files.list"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleTeamFilesReply);
}

void SlackTeamClient::requestSharedFileInfo(const QString &fileId)
{
    DEBUG_BLOCK;

    QMap<QString, QString> params;
    params.insert(QStringLiteral("file"), fileId);

    QNetworkReply *reply = executeGet(QStringLiteral("files.info"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleTeamFilesReply);
}

void SlackTeamClient::handleTeamFilesReply()
{
    DEBUG_BLOCK;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    reply->deleteLater();

    if (isError(data)) {
        return;
    }
    UsersModel* _users = m_teamInfo.users();
    QList<FileShare*> _list;
    const QJsonObject& paging = data.value(QStringLiteral("paging")).toObject();
    //qDebug().noquote() << "paging" << QJsonDocument(paging).toJson();
    int _total = paging.value(QStringLiteral("total")).toInt(1);
    int _page = paging.value(QStringLiteral("page")).toInt(1);
    int _pages = paging.value(QStringLiteral("pages")).toInt(1);
    const QJsonValue& jsonfilesVal = data.value(QStringLiteral("files"));
    if (!jsonfilesVal.isUndefined()) {
        const QJsonArray& jsonfiles = jsonfilesVal.toArray();
        for (const QJsonValue& jsonfile : jsonfiles) {
            //qDebug().noquote() << "file share" << QJsonDocument(jsonfile.toObject()).toJson();
            FileShare* fileshare = new FileShare;
            fileshare->setData(jsonfile.toObject());
            QQmlEngine::setObjectOwnership(fileshare, QQmlEngine::CppOwnership);
            fileshare->moveToThread(qApp->thread());
            fileshare->m_user = _users->user(fileshare->m_userId);
            _list.append(fileshare);
        }
    } else {
        const QJsonValue& jsonfileVal = data.value(QStringLiteral("file"));
        if (!jsonfileVal.isUndefined()) {
            //qDebug().noquote() << "file share" << QJsonDocument(jsonfile.toObject()).toJson();
            FileShare* fileshare = new FileShare;
            fileshare->setData(jsonfileVal.toObject());
            QQmlEngine::setObjectOwnership(fileshare, QQmlEngine::CppOwnership);
            fileshare->moveToThread(qApp->thread());
            fileshare->m_user = _users->user(fileshare->m_userId);
            _list.append(fileshare);
        }
    }
    qDebug() << "file shares for team:" << _list.size() << _page << _pages << _total;
    emit fileSharesReceived(_list, _total, _page, _pages);
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

    qDebug() << "Start init" << QThread::currentThread();
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
    //subscribe on self presence events
    QJsonObject presenceRequest;
    presenceRequest.insert(QStringLiteral("type"), QJsonValue(QStringLiteral("presence_sub")));
    presenceRequest.insert(QStringLiteral("ids"), QJsonValue(m_teamInfo.selfId()));
    stream->sendMessage(presenceRequest);
    requestUserInfoById(m_teamInfo.selfId());

    QUrl url(data.value(QStringLiteral("url")).toString());
    stream->listen(url);
    m_status = STARTED;
    qDebug() << "connect success" << QThread::currentThread();
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
    for (QPointer<SlackUser> user : m_teamInfo.users()->users()) {
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
        return QStringLiteral("conversations.history");
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

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);

    QNetworkReply *reply = executePost(QStringLiteral("conversations.join"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::leaveChannel(const QString& channelId)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);

    QNetworkReply *reply = executePost(QStringLiteral("conversations.leave"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::archiveChannel(const QString &channelId)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), channelId);

    QNetworkReply *reply = executePost(QStringLiteral("conversations.archive"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::openChat(const QStringList& userIds, const QString& channelId)
{
    DEBUG_BLOCK;

    QMap<QString, QString> params;
    if (!channelId.isEmpty()) {
        //resume chat
        params.insert(QStringLiteral("channel"), channelId);
    } else {
        params.insert(QStringLiteral("users"), userIds.join(","));
    }
    params.insert(QStringLiteral("return_im"), "true");

    QNetworkReply *reply = executePost(QStringLiteral("conversations.open"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::handleCreateChatReply()
{
    DEBUG_BLOCK

    qDebug() << "create chat reply";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    QJsonObject data = getResult(reply);

    if (isError(data)) {
        qDebug() << "Chat open failed" << data;
        return;
    }

    createChannelIfNeeded(data.value("channel").toObject());
}

void SlackTeamClient::createChat(const QString& channelName, bool isPrivate)
{
    DEBUG_BLOCK;

    QMap<QString, QString> params;
    params.insert(QStringLiteral("name"), channelName);
    params.insert(QStringLiteral("is_private"), isPrivate ? "true" : "false");

    QNetworkReply *reply = executePost(QStringLiteral("conversations.create"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCreateChatReply);
}

void SlackTeamClient::closeChat(const QString& chatId)
{
    DEBUG_BLOCK

    QMap<QString, QString> params;
    params.insert(QStringLiteral("channel"), chatId);

    QNetworkReply *reply = executePost(QStringLiteral("conversations.close"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::requestTeamInfo()
{
    if (m_teamInfo.domain().isEmpty() || m_teamInfo.name().isEmpty() || m_teamInfo.icons().isEmpty()) {
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
    qDebug() << __PRETTY_FUNCTION__ << m_teamInfo.users()->users().count() << "cursor" << cursor;
    if (m_teamInfo.users() == nullptr || !m_teamInfo.users()->usersFetched() || !cursor.isEmpty()) {
        QMap<QString, QString> params;
        params.insert(QStringLiteral("limit"), "10000");
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

void SlackTeamClient::requestUserInfoById(const QString& userId)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("user"), userId);
    QNetworkReply *reply = executeGet(QStringLiteral("users.info"), params);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleUsersInfoReply);
}

void SlackTeamClient::requestUserInfo(SlackUser *user)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("user"), user->userId());
    QNetworkReply *reply = executeGet(QStringLiteral("users.info"), params);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleUsersInfoReply);
}

void SlackTeamClient::updateUserInfo(SlackUser *user)
{
    QMap<QString, QString> params;

    QJsonObject profile;
    //TODO: to change this, app must have scope: users.profile.write
    profile["first_name"] = user->firstName();
    profile["last_name"] = user->lastName();
    profile["email"] = user->email();
    profile["status_text"] = user->status();
    profile["status_emoji"] = user->statusEmoji();
    params.insert(QStringLiteral("profile"), QJsonDocument(profile).toJson(QJsonDocument::Compact));
    QNetworkReply *reply = executePost(QStringLiteral("users.profile.set"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::updateUserAvatar(const QString& filePath, int cropSide, int cropX, int cropY)
{
    QMap<QString, QString> data;

    if (cropSide > 0) {
        data.insert(QStringLiteral("crop_w"), QString("%1").arg(cropSide));
    }
    if (cropSide > 0) {
        data.insert(QStringLiteral("crop_x"), QString("%1").arg(cropX));
    }
    if (cropSide > 0) {
        data.insert(QStringLiteral("crop_y"), QString("%1").arg(cropY));
    }

    QFile *file = new QFile(QUrl(filePath).toString(QUrl::RemoveScheme));
    if (!file->open(QFile::ReadOnly)) {
        qWarning() << "image file not readable" << file->fileName();
        emit postFileFail(m_teamInfo.teamId());
        return;
    }

    QString _fileFormData = QStringLiteral("form-data; name=\"image\"; filename=\"") + file->fileName() + "\"";
    qDebug() << "sending picture image image" << filePath;
    QNetworkReply *reply = executePostWithFile(QStringLiteral("users.setPhoto"), data, file, _fileFormData);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handlePostFile);
    connect(reply, &QNetworkReply::finished, file, &QObject::deleteLater);
}

void SlackTeamClient::setPresence(bool isAway)
{
    QMap<QString, QString> params;

    params.insert(QStringLiteral("presence"), isAway ? "away" : "auto");
    QNetworkReply *reply = executePost(QStringLiteral("users.setPresence"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::setDnD(int minutes)
{
    QMap<QString, QString> params;

    params.insert(QStringLiteral("num_minutes"), QString("%1").arg(minutes));
    QNetworkReply *reply = executePost(QStringLiteral("dnd.setSnooze"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::cancelDnD()
{
    QMap<QString, QString> params;
    QNetworkReply *reply = executePost(QStringLiteral("dnd.endSnooze"), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

QString SlackTeamClient::userName(const QString &userId) {
    if (teamInfo()->users() == nullptr) {
        return "";
    }

    SlackUser* user = teamInfo()->users()->user(userId);
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
        }
    }
    config->saveTeamInfo(m_teamInfo);
    emit teamInfoChanged(m_teamInfo.teamId());
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
    //qDebug() << "adding emoji" << einfo->m_shortNames << einfo->m_image << einfo->m_category;
    imagesCache->addEmoji(einfo);
}

void SlackTeamClient::requestDnDInfo(const QString &userId)
{
    QMap<QString, QString> params;
    params.insert(QStringLiteral("user"), userId);
    QNetworkReply *reply = executeGet(QStringLiteral("dnd.info"), params, userId);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleDnDInfoReply);
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

void SlackTeamClient::loadMessages(const QString& channelId, const QString& latest,
                                   const QString &threadTs)
{
    DEBUG_BLOCK;
    qDebug() << "Loading messages" << channelId << latest << sender();
    if (channelId.isEmpty()) {
        qWarning() << __PRETTY_FUNCTION__ << "Empty channel id";
        return;
    }
    QString _latest = latest;
    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    Chat* chat = _chatsModel->chat(channelId);
    if (chat == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "Chat for channel ID" << channelId << "not found";
        return;
    }
    if (_latest.isEmpty() && threadTs.isEmpty() == true) {
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
    if (threadTs.isEmpty() == false) {
        params.insert(QStringLiteral("ts"), threadTs);
    }
    if (_latest.isEmpty() == false) {
        params.insert(QStringLiteral("latest"), _latest);
        params.insert(QStringLiteral("inclusive"), "0");
    }

    QNetworkReply *reply = executeGet(threadTs.isEmpty() ? "conversations.history" :
                                                           "conversations.replies",
                                      params);
    reply->setProperty("threadTs", threadTs);
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
    QString threadTs = reply->property("threadTs").toString();
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
        const QJsonObject& messageObject = messageData.toObject();
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

        //fill up users for replies
        UsersModel* _usersModel = m_teamInfo.users();
        for(QObject* rplyObj : message->replies) {
            ReplyField* rply = static_cast<ReplyField*>(rplyObj);
            rply->m_user = _usersModel->user(rply->m_userId);
        }
        messageModel->preprocessFormatting(_chatsModel, message);
        _mlist.append(message);
    }
    emit messagesReceived(channelId, _mlist, _hasMore, threadTs);
    messageModel->setHistoryLoaded(true);

    qDebug() << "messages loaded for" << channelId << _chatsModel->chat(channelId)->name << m_teamInfo.teamId() << m_teamInfo.name() << _mlist.count() << "thread ts" << threadTs;
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

void SlackTeamClient::sendUserTyping(const QString &channelId)
{
    QJsonObject typingRequest;
    typingRequest.insert(QStringLiteral("type"), QJsonValue(QStringLiteral("typing")));
    typingRequest.insert(QStringLiteral("channel"), QJsonValue(channelId));
    stream->sendMessage(typingRequest);
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
    QString dt;// = time;
    ChatsModel* _chatsModel = teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    if (dt.isEmpty()) {
        auto messagesModel = _chatsModel->messages(channelId);
        if (messagesModel != nullptr) {
            dt = messagesModel->lastMessage();
        } else {
            qDebug() << "message model not ready for the channel" << channelId;
        }
    }
    if (dt.isEmpty()) {
        qWarning() << "Cant find timestamp for the channel" << channelId;
        return;
    }
    params.insert(QStringLiteral("ts"), dt);

    QNetworkReply *reply = executeGet(markMethod(type), params);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::deleteReaction(const QString& channelId, const QDateTime &ts, const QString& reaction)
{
    DEBUG_BLOCK

    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("name"), reaction);
    data.insert(QStringLiteral("timestamp"), dateTimeToSlack(ts));

    QNetworkReply *reply = executePost(QStringLiteral("reactions.remove"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::addReaction(const QString &channelId, const QDateTime &ts,
                                  const QString &reaction,
                                  const QString &slackTs)
{
    DEBUG_BLOCK

    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("name"), reaction);
    if (!slackTs.isEmpty()) {
        data.insert(QStringLiteral("timestamp"), slackTs);
    } else {
        data.insert(QStringLiteral("timestamp"), dateTimeToSlack(ts));
    }
    QNetworkReply *reply = executePost(QStringLiteral("reactions.add"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::postMessage(const QString& channelId, QString content, const QString &thread_ts)
{
    DEBUG_BLOCK;
    QMap<QString, QString> data;
    QNetworkReply *reply = nullptr;
    data.insert(QStringLiteral("channel"), channelId);
    if (!thread_ts.isEmpty()) {
        data.insert(QStringLiteral("thread_ts"), thread_ts);
    }

    if (content.startsWith("/me ")) {
        content.remove("/me ");
        data.insert(QStringLiteral("text"), content);
        reply = executePost(QStringLiteral("chat.meMessage"), data);
    } else {
        data.insert(QStringLiteral("text"), content);
        data.insert(QStringLiteral("as_user"), QStringLiteral("true"));
        data.insert(QStringLiteral("parse"), QStringLiteral("full"));
        reply = executePost(QStringLiteral("chat.postMessage"), data);
    }


    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::updateMessage(const QString &channelId, QString content,
                                    const QDateTime &ts, const QString &slackTs)
{
    DEBUG_BLOCK;
    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    data.insert(QStringLiteral("text"), content);
    data.insert(QStringLiteral("as_user"), QStringLiteral("true"));
    data.insert(QStringLiteral("parse"), QStringLiteral("full"));
    if (!slackTs.isEmpty()) {
        data.insert(QStringLiteral("ts"), slackTs);
    } else {
        data.insert(QStringLiteral("ts"), dateTimeToSlack(ts));
    }

    QNetworkReply *reply = executePost(QStringLiteral("chat.update"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::deleteMessage(const QString &channelId, const QDateTime &ts, const QString &slackTs)
{
    DEBUG_BLOCK;
    QMap<QString, QString> data;
    data.insert(QStringLiteral("channel"), channelId);
    if (!slackTs.isEmpty()) {
        data.insert(QStringLiteral("ts"), slackTs);
    } else {
        data.insert(QStringLiteral("ts"), dateTimeToSlack(ts));
    }
    data.insert(QStringLiteral("as_user"), QStringLiteral("true"));

    QNetworkReply *reply = executePost(QStringLiteral("chat.delete"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
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
#if 0
        {
            QFile f("chatslist_dumps_" + m_teamInfo.name() + ".json");
            if (f.open(QIODevice::Append)) {
                f.write(QJsonDocument(data).toJson());
                f.close();
            }
        }
#endif
    }
    emit conversationsDataChanged(_chats, cursor.isEmpty());

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
        stream->sendMessage(presenceRequest);
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
    QList<QPointer<SlackUser>> _users;
    QString cursor;
    if (reply != nullptr) {
        const QJsonObject& data = getResult(reply);
        cursor = data.value("response_metadata").toObject().value("next_cursor").toString();
        //qDebug() << __PRETTY_FUNCTION__ << "result" << data;
        reply->deleteLater();
        for (const QJsonValue& userValue : data.value("members").toArray()) {
            QPointer<SlackUser> user = new SlackUser;
            user->moveToThread(qApp->thread());
            QQmlEngine::setObjectOwnership(user, QQmlEngine::CppOwnership);
            user->setData(userValue.toObject());
            _users.append(user);
        }
        qDebug() << "got" << _users.count() << "users";
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

void SlackTeamClient::handleDnDInfoReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    const QJsonObject& data = getResult(reply);
    qDebug().noquote() << __PRETTY_FUNCTION__ << "result" << data;
    const QString& userId = reply->request().attribute(QNetworkRequest::User).toString();
    //qDebug() << __PRETTY_FUNCTION__ << "channel id" << channelId;
    reply->deleteLater();
    if (m_teamInfo.selfUser() != nullptr && userId == m_teamInfo.selfUser()->userId()) {
        const QJsonValue& snoozeEnabled = data.value(QStringLiteral("snooze_enabled"));
        if (!snoozeEnabled.isUndefined()) {
            bool _dnd = snoozeEnabled.toBool();
            if (_dnd) {
                int snoole_endtime = data.value("snooze_endtime").toInt(0);
                QDateTime snoozeEnd = QDateTime::fromSecsSinceEpoch(snoole_endtime);
                //in GUI thread
                QMetaObject::invokeMethod(qApp, [this, snoozeEnd] {
                    m_teamInfo.selfUser()->setPresence(SlackUser::Dnd, true);
                    m_teamInfo.selfUser()->setSnoozeEnds(snoozeEnd);
                });
            }
        }
    }
}

void SlackTeamClient::handleUsersInfoReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    //qDebug().noquote() << __PRETTY_FUNCTION__ << "result" << data;
    reply->deleteLater();
    // invoke on the main thread but dnd info must be requested from this thread
    // after user gets updated
    if (!isError(data)) {
        QMetaObject::invokeMethod(qApp, [this, data] {
            if (m_teamInfo.users() != nullptr) {
                const QJsonObject& userObj = data.value(QStringLiteral("user")).toObject();
                m_teamInfo.users()->updateUser(userObj);
                const QString& userId = userObj.value("id").toString();
                QMetaObject::invokeMethod(this, [this, userId] {
                    if (m_teamInfo.selfUser() != nullptr && m_teamInfo.selfUser()->userId() == userId) {
                        requestDnDInfo(m_teamInfo.selfId());
                    }
                }, Qt::QueuedConnection);
            }
        }, Qt::QueuedConnection);
    }
}

void SlackTeamClient::createChannelIfNeeded(const QJsonObject &channel)
{
    const QString& channelId = channel.value("id").toString();

    ChatsModel* _chatsModel = m_teamInfo.chats();
    if (_chatsModel == nullptr) {
        qWarning() << "Chats model not yet allocated";
        return;
    }
    Chat* _chat = _chatsModel->chat(channelId);
    if (_chat == nullptr) {
        _chat = new Chat(channel);
        QQmlEngine::setObjectOwnership(_chat, QQmlEngine::CppOwnership);
        // invoke on the main thread
        QMetaObject::invokeMethod(qApp, [_chatsModel, _chat] {
            _chatsModel->addChat(_chat);
        });
    }

}

void SlackTeamClient::handleCommonReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QJsonObject data = getResult(reply);
    bool error = isError(data);
    qDebug() << "Common result" << data << error;
    reply->deleteLater();
}

void SlackTeamClient::postFile(const QString& channelId, const QString& filePath,
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

    QFile *file = new QFile(QUrl(filePath).toString(QUrl::RemoveScheme));
    if (!file->open(QFile::ReadOnly)) {
        qWarning() << "image file not readable" << file->fileName();
        emit postFileFail(m_teamInfo.teamId());
        return;
    }

    qDebug() << "sending image" << filePath;
    QNetworkReply *reply = executePostWithFile(QStringLiteral("files.upload"), data, file);

    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handlePostFile);
    connect(reply, &QNetworkReply::finished, file, &QObject::deleteLater);
}

void SlackTeamClient::deleteFile(const QString &fileId)
{
    QMap<QString, QString> data;
    data.insert(QStringLiteral("file"), fileId);
    QNetworkReply *reply = executePost(QStringLiteral("files.delete"), data);
    connect(reply, &QNetworkReply::finished, this, &SlackTeamClient::handleCommonReply);
}

void SlackTeamClient::handlePostFile()
{
    DEBUG_BLOCK

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QJsonObject data = getResult(reply);
    qDebug() << "Post file result" << data;

    if (isError(data)) {
        emit postFileFail(m_teamInfo.teamId());
    } else {
        emit postFileSuccess(m_teamInfo.teamId());
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
