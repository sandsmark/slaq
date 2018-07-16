#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QDateTime>
#include <QJsonArray>
#include <QMutexLocker>
#include "UsersModel.h"
#include "ChatsModel.h"
#include "MessagesModel.h"
#include "messageformatter.h"

MessageListModel::MessageListModel(QObject *parent, UsersModel *usersModel, const QString &channelId) : QAbstractListModel(parent),
    m_usersModel(usersModel), m_channelId(channelId) {
    m_newUserPattern = QRegularExpression(QStringLiteral("<@([A-Z0-9]+)\\|([^>]+)>"));
}

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_messages.count();
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const
{
    QMutexLocker locker(&m_modelMutex);

    int row = index.row();
    if (row >= m_messages.count() || row < 0) {
        qWarning() << "Invalid row" << row;
        return QVariant();
    }

    const Message* message = m_messages.at(row);
    if (message == nullptr) {
        qWarning() << "No message for row" << row;
        return QVariant();
    }

    switch(role) {
    case Text:
        return m_messages[row]->text;
    case User:
        //qDebug() << "User for row" << row << m_messages[row].user.data();
        return QVariant::fromValue(message->user.data());
    case Time:
        return message->time;
    case SlackTimestamp:
        return message->ts;
    case Attachments:
        //qDebug() << "Attachment for row" << row << m_messages[row]->attachments.count();
        return QVariant::fromValue(message->attachments);
    case Reactions:
        //qDebug() << "Reactions for row" << row << m_messages[row]->reactions.count();
        return QVariant::fromValue(message->reactions);
    case FileShares:
        return QVariant::fromValue(message->fileshares);
    case IsStarred:
        return QVariant::fromValue(message->isStarred);
    case IsChanged:
        return QVariant::fromValue(message->isChanged);
    case SearchChannelName:
        return message->channelName;
    case SearchUserName:
        return message->userName;
    case SearchPermalink:
        return message->permalink;
    default:
        qWarning() << "Unhandled role" << role;
        return QVariant();
    }
}

void MessageListModel::updateReactionUsers(Message* message) {
    foreach(QObject* r, message->reactions) {
        Reaction* reaction = static_cast<Reaction*>(r);
        if (!reaction->users.isEmpty()) {
            reaction->users.clear();
        }
        foreach (const QString& userId, reaction->userIds) {
            ::User* user_ = m_usersModel->user(userId);
            if (user_ != nullptr) {
                reaction->users.append(m_usersModel->user(userId)->username());
            }
        }
        //qDebug() << "reaction users" << reaction->users << "for" << reaction->userIds;
    }
}

Message *MessageListModel::message(const QDateTime &ts)
{
    QMutexLocker locker(&m_modelMutex);
    for (int i = 0; i < m_messages.count(); i++) {
        Message* message = m_messages.at(i);
        if (message->time == ts) {
            return message;
        }
    }
    return nullptr;
}

Message *MessageListModel::message(int row)
{
    QMutexLocker locker(&m_modelMutex);
    return m_messages.at(row);
}

void MessageListModel::clear()
{
    QMutexLocker locker(&m_modelMutex);
    beginResetModel();
    for (Message* msg : m_messages) {
        delete msg;
    }
    m_messages.clear();
    endResetModel();
}

void MessageListModel::preprocessFormatting(Chat *chat, Message *message)
{
    findNewUsers(message->text);
    updateReactionUsers(message);
    m_formatter.replaceAll(message->user, chat, message->text);
    for (QObject* attachmentObj : message->attachments) {
        Attachment* attachment = static_cast<Attachment*>(attachmentObj);
        m_formatter.replaceAll(message->user, chat, attachment->text);
        m_formatter.replaceAll(message->user, chat, attachment->pretext);
        m_formatter.replaceAll(message->user, chat, attachment->fallback);
        m_formatter.replaceAll(message->user, chat, attachment->title);
        m_formatter.replaceAll(message->user, chat, attachment->footer);
        for (QObject* attachmentFieldObj : attachment->fields) {
            AttachmentField* attachmentField = static_cast<AttachmentField*>(attachmentFieldObj);
            m_formatter.replaceAll(message->user, chat, attachmentField->m_title);
            m_formatter.replaceAll(message->user, chat, attachmentField->m_value);
        }
    }
}

void MessageListModel::addMessage(Message* message)
{
    qDebug() << "adding message:" << message->text;
    QMutexLocker locker(&m_modelMutex);
    if (message->user.isNull()) {
        qWarning() << "user is null for " << message->user_id;
    }
    Q_ASSERT_X(!message->user.isNull(), "addMessage .user is null", "");
    ChatsModel* _chatsModel = static_cast<ChatsModel*>(parent());
    Chat chat;
    if (_chatsModel != nullptr) {
        chat = _chatsModel->chat(m_channelId);
    }
    preprocessFormatting(&chat, message);
    beginInsertRows(QModelIndex(), 0, 0);
    m_messages.prepend(message);
    endInsertRows();
    if (!chat.id.isEmpty() && message->time > chat.lastRead) {
        chat.unreadCountDisplay++;
        _chatsModel->chatChanged(chat);
    }
}

void MessageListModel::updateMessage(Message *message)
{
    for (int i = 0; i < m_messages.count(); i++) {
        m_modelMutex.lock();
        Message* oldmessage = m_messages.at(i);
        m_modelMutex.unlock();
        if (oldmessage->time == message->time) {
            if (message->user.isNull()) {
                message->user = oldmessage->user;
                if (message->user.isNull()) {
                    qWarning() << "user is null for " << message->user_id;
                    Q_ASSERT_X(!message->user.isNull(), "user is null", "");
                }
            }
            Chat chat;
            if (parent() != nullptr) {
                chat = static_cast<ChatsModel*>(parent())->chat(m_channelId);
            }
            preprocessFormatting(&chat, message);
            qDebug() << "updating message:" << message->text;
            m_modelMutex.lock();
            m_messages.replace(i, message);
            m_modelMutex.unlock();
            QModelIndex index = QAbstractListModel::index(i, 0,  QModelIndex());
            emit dataChanged(index, index);
            delete oldmessage;
            break;
        }
    }
}

void MessageListModel::findNewUsers(const QString &message)
{
    //DEBUG_BLOCK

    QRegularExpressionMatchIterator i = m_newUserPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString id = match.captured(1);

        if (m_usersModel->user(id).isNull()) {
            QString name = match.captured(2);
            QPointer<::User> user = new ::User(id, name, this);
            m_usersModel->addUser(user);
        }
    }
}

void MessageListModel::addMessages(const QJsonArray &messages)
{
    qDebug() << "Adding" << messages.count() << "messages";
    QMutexLocker locker(&m_modelMutex);
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count() + messages.count() - 1);

    //assume we have parent;
    ChatsModel* _chatsModel = static_cast<ChatsModel*>(parent());
    Chat chat;
    if (_chatsModel != nullptr) {
        chat = _chatsModel->chat(m_channelId);
    }
    for (const QJsonValue &messageData : messages) {
        const QJsonObject messageObject = messageData.toObject();
        Message* message = new Message;
        //qDebug() << "message obj" << messageObject;
        message->setData(messageObject);
        message->channel_id = m_channelId;
        if (message->user_id.isEmpty()) {
            qWarning() << "user id is empty" << messageObject;
        }
        message->user = m_usersModel->user(message->user_id);
        // TODO: why?
        //m_reactions.insert(reactionObject["name"].toString(), reaction);
        if (message->user.isNull()) {
            qWarning() << "no user for" << message->user_id << m_usersModel;
        }
        Q_ASSERT_X(!message->user.isNull(), "user is null", "");

        preprocessFormatting(&chat, message);
        m_messages.append(std::move(message));
    }

    endInsertRows();
}


QHash<int, QByteArray> MessageListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[Text] = "Text";
    names[User] = "User";
    names[Time] = "Time";
    names[SlackTimestamp] = "SlackTimestamp";
    names[Attachments] = "Attachments";
    names[Reactions] = "Reactions";
    names[FileShares] = "FileShares";
    names[IsStarred] = "IsStarred";
    names[IsChanged] = "IsChanged";
    names[SearchChannelName] = "ChannelName";
    names[SearchUserName] = "UserName";
    names[SearchPermalink] = "Permalink";
    return names;
}

Message::Message() {}

Message::~Message()
{
    qDeleteAll(attachments);
    qDeleteAll(reactions);
    qDeleteAll(fileshares);
}
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


void Message::setData(const QJsonObject &data)
{
    //qDebug() << "message" << data;
    type = data.value(QStringLiteral("type")).toString();
    ts = data.value(QStringLiteral("ts")).toString();
    time = slackToDateTime(ts);
//    qDebug() << ts << time;
//    Q_ASSERT(time.isValid());

    text = data.value(QStringLiteral("text")).toString();
    const QJsonValue chan_ = data.value(QStringLiteral("channel"));
    if (chan_.isString()) {
        channel_id = chan_.toString();
    } else if (chan_.isObject()) {
        const QJsonObject& chanObj = chan_.toObject();
        channel_id = chanObj.value(QStringLiteral("id")).toString();
        channelName = chanObj.value(QStringLiteral("name")).toString();
    }
    user_id = data.value(QStringLiteral("user")).toString();
    subtype = data.value(QStringLiteral("subtype")).toString();
    //search results
    userName = data.value(QStringLiteral("username")).toString();
    permalink = data.value(QStringLiteral("permalink")).toString();
    if (user_id.isEmpty()) {
        const QString& subtype = data.value(QStringLiteral("subtype")).toString(QStringLiteral("default"));
        if (subtype == QStringLiteral("bot_message")) {
            user_id = data.value(QStringLiteral("bot_id")).toString();
        }
    }

    for (const QJsonValue &reactionValue : data.value("reactions").toArray()) {
        Reaction *reaction = new Reaction;
        reaction->setData(reactionValue.toObject());
        QQmlEngine::setObjectOwnership(reaction, QQmlEngine::CppOwnership);
        reactions.append(reaction);
    }

    for (const QJsonValue &attachmentValue : data.value(QStringLiteral("attachments")).toArray()) {
        Attachment* attachment = new Attachment;
        attachment->setData(attachmentValue.toObject());
        QQmlEngine::setObjectOwnership(attachment, QQmlEngine::CppOwnership);
        attachments.append(attachment);
    }

    if (subtype == QStringLiteral("file_share")) {
        //qDebug().noquote() << "file share json:" << QJsonDocument(data).toJson();
        const QJsonValue& filesArrayValue = data.value(QStringLiteral("files"));
        if (filesArrayValue.isUndefined()) {
            const QJsonValue& fileValue = data.value(QStringLiteral("file"));
            if (!fileValue.isUndefined()) {
                FileShare* fileshare = new FileShare;
                fileshare->setData(fileValue.toObject());
                QQmlEngine::setObjectOwnership(fileshare, QQmlEngine::CppOwnership);
                fileshares.append(fileshare);
            }
        } else {
            for (const QJsonValue &filesValue : filesArrayValue.toArray()) {
                FileShare* fileshare = new FileShare;
                fileshare->setData(filesValue.toObject());
                QQmlEngine::setObjectOwnership(fileshare, QQmlEngine::CppOwnership);
                fileshares.append(fileshare);
            }
        }
    }
}

Attachment::Attachment(QObject *parent): QObject(parent) {}

void Attachment::setData(const QJsonObject &data)
{
    titleLink = data.value(QStringLiteral("title_link")).toString();
    title = data.value(QStringLiteral("title")).toString();
    pretext = data.value(QStringLiteral("pretext")).toString();
    text = data.value(QStringLiteral("text")).toString();
    fallback = data.value(QStringLiteral("fallback")).toString();

    author_name = data.value(QStringLiteral("author_name")).toString();
    author_link = QUrl(data.value(QStringLiteral("author_link")).toString());
    author_icon = QUrl(data.value(QStringLiteral("author_icon")).toString());
    thumb_url = QUrl(data.value(QStringLiteral("thumb_url")).toString());
    footer = data.value(QStringLiteral("footer")).toString();
    footer_icon = QUrl(data.value(QStringLiteral("footer_icon")).toString());

    imageSize = QSize(data["image_width"].toInt(), data["image_height"].toInt());
    imageUrl = QUrl(data["image_url"].toString());
    ts = QDateTime::fromSecsSinceEpoch(data.value(QStringLiteral("ts")).toInt());

    color = data.value(QStringLiteral("color")).toString();
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


    foreach (const QJsonValue &fieldValue, data.value(QStringLiteral("fields")).toArray()) {
        AttachmentField* field = new AttachmentField;
        field->setData((fieldValue.toObject()));
        fields.append(field);
    }
}

Reaction::Reaction(QObject *parent) : QObject(parent) {}

void Reaction::setData(const QJsonObject &data)
{
    //qDebug().noquote() << "reaction" << QJsonDocument(data).toJson();
    name = data[QStringLiteral("name")].toString();
    emoji = data[QStringLiteral("emoji")].toString();

    const QJsonArray usersList = data[QStringLiteral("users")].toArray();
    for (const QJsonValue &usersValue : usersList) {
        userIds.append(usersValue.toString());
    }
}

AttachmentField::AttachmentField(QObject *parent): QObject(parent) {}

void AttachmentField::setData(const QJsonObject &data)
{
    m_title = data.value(QStringLiteral("title")).toString();
    m_value = data.value(QStringLiteral("value")).toString();
    m_isShort = data.value(QStringLiteral("short")).toBool();
}

FileShare::FileShare(QObject *parent) : QObject (parent) {}

void FileShare::setData(const QJsonObject &data)
{
    m_id = data.value(QStringLiteral("id")).toString();
    m_created = slackToDateTime(data.value(QStringLiteral("created")).toString());
    m_timestamp = slackToDateTime(data.value(QStringLiteral("timestamp")).toString());
    m_name = data.value(QStringLiteral("name")).toString();
    m_title = data.value(QStringLiteral("title")).toString();
    m_mimetype = data.value(QStringLiteral("mimetype")).toString();
    m_filetype = data.value(QStringLiteral("filetype")).toString();
    m_pretty_type = data.value(QStringLiteral("pretty_type")).toString();
    QPointer<User> m_user;//" : "U2147483697",
    const QString& _mode = data.value(QStringLiteral("mode")).toString();
    if (_mode == "hosted") {
        m_mode = Hosted;
    } if (_mode == "external") {
        m_mode = External;
    } if (_mode == "snippet") {
        m_mode = Snipped;
    } if (_mode == "post") {
        m_mode = Post;
    }
    m_editable = data.value(QStringLiteral("external_type")).toBool(true);
    m_is_external = data.value(QStringLiteral("external_type")).toBool(false);
    m_external_type = data.value(QStringLiteral("external_type")).toString();
    m_username = data.value(QStringLiteral("username")).toString();
    m_size = (quint64)data.value(QStringLiteral("size")).toInt(0);
    m_url_private = QUrl(data.value(QStringLiteral("url_private")).toString());
    m_url_private_download = QUrl(data.value(QStringLiteral("url_private_download")).toString());
    m_thumb_video = QUrl(data.value(QStringLiteral("thumb_video")).toString());
    m_thumb_64 = QUrl(data.value(QStringLiteral("thumb_64")).toString());
    m_thumb_80 = QUrl(data.value(QStringLiteral("thumb_80")).toString());
    m_thumb_360 = QUrl(data.value(QStringLiteral("thumb_360")).toString());
    m_thumb_360_gif = QUrl(data.value(QStringLiteral("thumb_360_gif")).toString());
    m_thumb_360_size = QSize(data.value(QStringLiteral("thumb_360_w")).toInt(100),
                             data.value(QStringLiteral("thumb_360_h")).toInt(100));
    m_thumb_480 = QUrl(data.value(QStringLiteral("thumb_480")).toString());
    m_thumb_480_size = QSize(data.value(QStringLiteral("thumb_480_w")).toInt(100),
                             data.value(QStringLiteral("thumb_480_h")).toInt(100));
    m_original_size = QSize(data.value(QStringLiteral("original_w")).toInt(100),
                            data.value(QStringLiteral("original_h")).toInt(100));
    m_thumb_160 = QUrl(data.value(QStringLiteral("thumb_160")).toString());
    m_permalink = QUrl(data.value(QStringLiteral("permalink")).toString());
    m_permalink_public = QUrl(data.value(QStringLiteral("permalink_public")).toString());
    m_edit_link = QUrl(data.value(QStringLiteral("edit_link")).toString());
    m_preview = data.value(QStringLiteral("preview")).toString();
    m_preview_highlight = data.value(QStringLiteral("preview_highlight")).toString();
    m_lines = data.value(QStringLiteral("lines")).toInt(0);
    m_lines_more = data.value(QStringLiteral("lines_more")).toInt(0);
    m_is_public = data.value(QStringLiteral("external_type")).toBool(true);
    m_public_url_shared = data.value(QStringLiteral("external_type")).toBool(true);
    m_display_as_bot = data.value(QStringLiteral("external_type")).toBool(false);
    foreach (const QJsonValue &channelValue, data.value(QStringLiteral("channels")).toArray()) {
        m_channels << channelValue.toString();
    }
    foreach (const QJsonValue &groupValue, data.value(QStringLiteral("groups")).toArray()) {
        m_groups << groupValue.toString();
    }
    foreach (const QJsonValue &imValue, data.value(QStringLiteral("ims")).toArray()) {
        m_ims << imValue.toString();
    }
    m_initial_comment = data.value(QStringLiteral("initial_comment")).toString();
    m_num_stars = data.value(QStringLiteral("num_stars")).toInt(0);
    m_is_starred = data.value(QStringLiteral("external_type")).toBool(false);
    foreach (const QJsonValue &pinnedlValue, data.value(QStringLiteral("pinned_to")).toArray()) {
        m_pinned_to << pinnedlValue.toString();
    }

    for (const QJsonValue &reactionValue : data.value("reactions").toArray()) {
        Reaction *reaction = new Reaction;
        reaction->setData(reactionValue.toObject());
        QQmlEngine::setObjectOwnership(reaction, QQmlEngine::CppOwnership);
        m_reactions.append(reaction);
    }
    m_comments_count = data.value(QStringLiteral("m_comments_count")).toInt(0);
}
