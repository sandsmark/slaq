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
#include "imagescache.h"
#include "debugblock.h"

MessageListModel::MessageListModel(QObject *parent, UsersModel *usersModel, const QString &channelId, bool isThreadModel) : QAbstractListModel(parent),
    m_usersModel(usersModel), m_channelId(channelId), m_isThreadModel(isThreadModel) {
    m_newUserPattern = QRegularExpression(QStringLiteral("<@([A-Z0-9]+)\\|([^>]+)>"));
    m_existingUserPattern = QRegularExpression(QStringLiteral("<@([A-Z0-9]+)>"));
    connect(m_usersModel, &UsersModel::dataChanged, this, &MessageListModel::usersModelChanged);
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
        return message->text;
    case User:
        return QVariant::fromValue(message->user.data());
    case Time:
        return message->time;
    case SlackTimestamp:
        return message->ts;
    case Attachments:
        return QVariant::fromValue(message->attachments);
    case Reactions:
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
    case SameUser:
        return message->isSameUser;
    case TimeDiff:
        return message->timeDiffMs;
    case ThreadReplies:
        return QVariant::fromValue(message->replies);
    case ThreadRepliesModel:
        return QVariant::fromValue(message->messageThread.data());
    case ThreadIsParentMessage:
        return isMessageThreadParent(message);
    case ThreadTs:
        return message->thread_ts;
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

bool MessageListModel::historyLoaded() const
{
    return m_historyLoaded;
}

void MessageListModel::setHistoryLoaded(bool historyLoaded)
{
    m_historyLoaded = historyLoaded;
}

bool MessageListModel::isThreadModel() const
{
    return m_isThreadModel;
}

MessageListModel *MessageListModel::createThread(Message *parentMessage)
{
    if (parentMessage != nullptr) {
        if (parentMessage->messageThread.isNull()) {
            parentMessage->thread_ts = parentMessage->time;
            parentMessage->messageThread = QSharedPointer<MessageListModel>(new MessageListModel(parent(), m_usersModel, m_channelId, true));
            QQmlEngine::setObjectOwnership(parentMessage->messageThread.data(), QQmlEngine::CppOwnership);
            parentMessage->messageThread->prependMessageToModel(parentMessage);
            parentMessage->messageThread->refresh();
            if (!m_MessageThreads.contains(parentMessage->thread_ts)) {
                m_MessageThreads.insert(parentMessage->thread_ts, parentMessage->messageThread);
            }
            m_modelMutex.lock();
            int _index= m_messages.indexOf(parentMessage);
            m_modelMutex.unlock();
            if (_index >= 0) {
                QModelIndex modelIndex = index(_index);
                emit dataChanged(modelIndex, modelIndex, roleNames().keys().toVector());
            }
        }
        return parentMessage->messageThread.data();
    }
    return nullptr;
}

Message *MessageListModel::message(int row)
{
    QMutexLocker locker(&m_modelMutex);
    return m_messages.at(row);
}

Message *MessageListModel::message(const QDateTime &ts)
{
    qDebug() << "searching for" << ts << "at" << this;
    QMutexLocker locker(&m_modelMutex);
    for (int i = 0; i < m_messages.count(); i++) {
        Message* message = m_messages.at(i);
        if (message->time == ts) {
            return message;
        }
        // 1st message in the message thread is parent message
        // so to avoid recursive search - check if the message thread its not current thread
        if (!message->messageThread.isNull() && message->messageThread.data() != this) {
            locker.unlock();
            Message* threadedMsg = message->messageThread->message(ts);
            if (threadedMsg != nullptr) {
                return threadedMsg;
            }
            locker.relock();
        }
    }
    return nullptr;
}

bool MessageListModel::deleteMessage(const QDateTime &ts)
{
    QMutexLocker locker(&m_modelMutex);
    for (int i = 0; i < m_messages.count(); i++) {
        Message* message = m_messages.at(i);
        if (message->time == ts) {
            beginRemoveRows(QModelIndex(), i, i);
            m_messages.removeAt(i);
            endRemoveRows();
            delete message;
            return true;
        } else {
            //also check replies
            if (message->messageThread != nullptr) {
                if (message->messageThread->deleteMessage(ts) == true) {
                    return true;
                }
            }
        }
    }
    return false;
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

void MessageListModel::appendMessageToModel(Message *message)
{
    m_modelMutex.lock();
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(message);
    m_modelMutex.unlock();
    endInsertRows();
}

void MessageListModel::prependMessageToModel(Message *message)
{
    m_modelMutex.lock();
    beginInsertRows(QModelIndex(), 0, 0);
    m_messages.prepend(message);
    m_modelMutex.unlock();
    endInsertRows();
}

void MessageListModel::modelDump()
{
    QString fileName = QString("dump_ch_%1.json").arg(m_channelId);
    QFile dumpFile(fileName);
    dumpFile.open(QIODevice::WriteOnly);
    QJsonArray msgArray;
    for (Message* msg : m_messages) {
        msgArray.append(msg->toJson());
    }
    QJsonDocument jdoc(msgArray);
    dumpFile.write(jdoc.toJson());
    dumpFile.close();
}

void MessageListModel::sortMessages(bool asc)
{
    if (m_messages.size() < 2) {
        return;
    }
    beginResetModel();
    m_modelMutex.lock();
    std::sort(m_messages.begin(), m_messages.end(), Message::compare);
    m_modelMutex.unlock();
    endResetModel();
}

void MessageListModel::refresh()
{
    beginResetModel();
    endResetModel();
}

void MessageListModel::replaceMessage(Message *oldmessage, Message *message)
{
    QMutexLocker locker(&m_modelMutex);
    m_messages.replace(m_messages.indexOf(oldmessage), message);
}

void MessageListModel::preprocessMessage(Message *message)
{
    if (message->user.isNull()
            || (!message->user.isNull() && (message->user_id != message->user->userId()))) {
        message->user = m_usersModel->user(message->user_id);
        if (message->user.isNull()) {
            qWarning() << "user is null for " << message->user_id << message->userName << m_usersModel->users().count();
            //try to construct user from message
            if (!message->user_id.isEmpty()) {
                QPointer<::User> _user = new ::User(message->user_id, message->userName, nullptr);
                QQmlEngine::setObjectOwnership(_user, QQmlEngine::CppOwnership);
                message->user = _user;
                m_usersModel->addUser(_user);
            }
        }
    }

    findNewUsers(message->text);
}

QDateTime MessageListModel::firstMessageTs()
{
    QDateTime _time;
    if (!m_messages.isEmpty()) {
        _time = m_messages.last()->time;
    }
    return _time;
}

void MessageListModel::preprocessFormatting(ChatsModel *chat, Message *message)
{
    updateReactionUsers(message);
    m_formatter.replaceAll(chat, message->text);
    for (QObject* attachmentObj : message->attachments) {
        Attachment* attachment = static_cast<Attachment*>(attachmentObj);
        m_formatter.replaceAll(chat, attachment->text);
        m_formatter.replaceAll(chat, attachment->pretext);
        m_formatter.replaceAll(chat, attachment->fallback);
        m_formatter.replaceAll(chat, attachment->title);
        m_formatter.replaceAll(chat, attachment->footer);
        for (QObject* attachmentFieldObj : attachment->fields) {
            AttachmentField* attachmentField = static_cast<AttachmentField*>(attachmentFieldObj);
            m_formatter.replaceAll(chat, attachmentField->m_title);
            m_formatter.replaceAll(chat, attachmentField->m_value);
        }
    }
}

int MessageListModel::countUnread(const QDateTime &lastRead)
{
    if (!lastRead.isValid()) {
        return 0;
    }
    QMutexLocker locker(&m_modelMutex);
    for (int i = 0; i < m_messages.count(); i++) {
        //qDebug() << "comparing" << i << m_messages.at(i)->time << lastRead;
        if (m_messages.at(i)->time == lastRead) {
            return i;
        }
    }
    return 0;
}

void MessageListModel::usersModelChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight) //TODO: ??
    if (roles.isEmpty()) {
        qWarning() << __PRETTY_FUNCTION__ << "roles are empty";
        return;
    }
    ::User* _user = qvariant_cast<::User*>(m_usersModel->data(topLeft, roles.at(0)));
    if (_user != nullptr) {
        //qWarning() << "USER CHANGED" << _user->userId() << _user->username();
        //got thru messages and inform about user gets changed
        for (int i = 0; i < m_messages.count(); i++) {
            QPointer<::User> user = m_messages.at(i)->user;
            QString _userId = m_messages.at(i)->user_id;
            if (_userId.isEmpty() && !user.isNull()) {
                _userId = user->isBot() ? user->botId() : user->userId();
            }
            if (_userId.isEmpty()) {
                continue;
            }
            if (_userId == _user->userId() || (_userId == _user->botId())) {
                QModelIndex modelIndex = index(i);
                emit dataChanged(modelIndex, modelIndex, QVector<int>() << User);
            }
        }
    }
}

void MessageListModel::processChildMessage(Message* message) {
    Message* parent_msg = this->message(message->thread_ts);
    QSharedPointer<MessageListModel> thrdModel = m_MessageThreads.value(message->thread_ts);
    if (thrdModel == nullptr) {
        thrdModel = QSharedPointer<MessageListModel>(new MessageListModel(parent(), m_usersModel, m_channelId, true));
        QQmlEngine::setObjectOwnership(thrdModel.data(), QQmlEngine::CppOwnership);
    }
    if (parent_msg != nullptr) {
        if (parent_msg->messageThread.isNull()) {
            parent_msg->messageThread = thrdModel;
            //add parent message as a 1st message in the thread
            thrdModel->prependMessageToModel(parent_msg);
            thrdModel->sortMessages();
        }
    }
    message->parentMessage = parent_msg;
    thrdModel->prependMessageToModel(message);
    thrdModel->sortMessages();
    if (!m_MessageThreads.contains(message->thread_ts)) {
        m_MessageThreads.insert(message->thread_ts, thrdModel);
    }
}

void MessageListModel::addMessage(Message* message)
{
    preprocessMessage(message);
    //check for thread
    if (isMessageThreadChild(message)) {
        processChildMessage(message);
    } else {
        // check if the message is parent thread
        if (m_MessageThreads.contains(message->time) && message->messageThread == nullptr) {
            message->messageThread = m_MessageThreads.value(message->time);
            message->messageThread->prependMessageToModel(message);
            message->messageThread->sortMessages();
        }
        m_modelMutex.lock();
        if (!m_messages.isEmpty()) {
            Message* prevMsg = prevMsg = m_messages.first();
            message->isSameUser = (prevMsg->user_id == message->user_id);
            message->timeDiffMs = (message->time.toMSecsSinceEpoch() - prevMsg->time.toMSecsSinceEpoch());
            Q_ASSERT_X(prevMsg->time != message->time, __PRETTY_FUNCTION__, "Time should not be equal");
        }
        m_modelMutex.unlock();
        prependMessageToModel(message);
    }
}

void MessageListModel::updateMessage(Message *message)
{
    int _index_to_replace = -1;
    Message* oldmessage = nullptr;

    const bool _isChild = isMessageThreadChild(message);

    m_modelMutex.lock();
    for (int i = 0; i < m_messages.count(); i++) {
        oldmessage = m_messages.at(i);
        if (_isChild && !isThreadModel()) {
            if (oldmessage->thread_ts == message->thread_ts) {
                oldmessage->messageThread->updateMessage(message);
                break;
            }
        } else {
            if (oldmessage->time == message->time) {
                message->messageThread = oldmessage->messageThread;
                if (message->user.isNull()) {
                    message->user = m_usersModel->user(message->user_id);
                    if (message->user.isNull()) {
                        qWarning() << __PRETTY_FUNCTION__ << "user is null for " << message->user_id;
                        //Q_ASSERT_X(!message->user.isNull(), "user is null", "");
                    }
                }
                _index_to_replace = i;
                break;
            }
        }
    }
    m_modelMutex.unlock();
    if (_index_to_replace >= 0) {
        preprocessMessage(message);
        qDebug() << "updating message:" << message->text << message->user_id << message << oldmessage << _index_to_replace;

        if (message->messageThread != nullptr) {
            //replace old parent message with new one in the thread
            message->messageThread->replaceMessage(oldmessage, message);
            message->messageThread->refresh();
        }
        if (message != oldmessage) {
            m_modelMutex.lock();
            m_messages.replace(_index_to_replace, message);
            m_modelMutex.unlock();
            delete oldmessage;
        }

        QModelIndex modelIndex = index(_index_to_replace);
        emit dataChanged(modelIndex, modelIndex, roleNames().keys().toVector());
    }
}

void MessageListModel::findNewUsers(QString& message)
{
    //DEBUG_BLOCK

    QPointer<::User> user;
    QRegularExpressionMatchIterator i = m_newUserPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        const QString& id = match.captured(1);
        user = m_usersModel->user(id);
        if (user.isNull()) {
            QString name = match.captured(2);
            user = new ::User(id, name, nullptr);
            QQmlEngine::setObjectOwnership(user, QQmlEngine::CppOwnership);
            if (QThread::currentThread() != qApp->thread()) {
                user->moveToThread(qApp->thread());
            }
            m_usersModel->addUser(user);
            m_formatter.replaceUserInfo(user.data(), message);
        }
    }
    i = m_existingUserPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        const QString& id = match.captured(1);
        user = m_usersModel->user(id);
        if (!user.isNull()) {
            m_formatter.replaceUserInfo(user.data(), message);
        }
    }
}

void MessageListModel::addMessages(const QList<Message*> &messages, bool hasMore, int threadMsgsCount)
{
    DEBUG_BLOCK;
    qDebug() << "Adding" << messages.count() << "messages" << QThread::currentThreadId() << hasMore << threadMsgsCount;

    m_hasMore = hasMore;
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count() + messages.count() - 1 - threadMsgsCount);

    for (Message* message : messages) {
        preprocessMessage(message);
        //check for message thread
        if (isMessageThreadChild(message)) {
            processChildMessage(message);
        } else {
            // check if the message is parent thread
            if (m_MessageThreads.contains(message->time) && message->messageThread == nullptr) {
                message->messageThread = m_MessageThreads.value(message->time);
                message->messageThread->appendMessageToModel(message);
                message->messageThread->sortMessages();
            }

            if (!m_messages.isEmpty()) {
                Message* prevMsg = m_messages.last();
                prevMsg->isSameUser = (prevMsg->user_id == message->user_id);
                prevMsg->timeDiffMs = (prevMsg->time.toMSecsSinceEpoch() - message->time.toMSecsSinceEpoch());
            }
            m_modelMutex.lock();
            m_messages.append(message);
            m_modelMutex.unlock();
        }
    }

    endInsertRows();
}

bool MessageListModel::canFetchMore(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_hasMore;
}

void MessageListModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent)
    qDebug() << "called fetch more" << parent;
    if (m_messages.isEmpty()) {
        emit fetchMoreMessages(m_channelId, QDateTime());
    } else {
        emit fetchMoreMessages(m_channelId, m_messages.last()->time);
    }
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
    names[SameUser] = "SameUser";
    names[TimeDiff] = "TimeDiff";
    names[SearchChannelName] = "ChannelName";
    names[SearchUserName] = "UserName";
    names[SearchPermalink] = "Permalink";
    names[ThreadReplies] = "ThreadReplies";
    names[ThreadRepliesModel] = "ThreadRepliesModel";
    names[ThreadIsParentMessage] = "ThreadIsParentMessage";
    names[ThreadTs] = "ThreadTs";
    return names;
}

Message::Message() {}

Message::~Message()
{
    qDeleteAll(attachments);
    qDeleteAll(reactions);
    qDeleteAll(fileshares);
    qDeleteAll(replies);
}

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
    const QJsonValue thread_ = data.value(QStringLiteral("thread_ts"));
    if (!thread_.isUndefined()) {
        thread_ts = slackToDateTime(thread_.toString());
    }
    isChanged = !data.value(QStringLiteral("edited")).isUndefined();
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

    for (const QJsonValue &repliesValue : data.value("replies").toArray()) {
        ReplyField *msgReply = new ReplyField;
        msgReply->setData(repliesValue.toObject());
        QQmlEngine::setObjectOwnership(msgReply, QQmlEngine::CppOwnership);
        replies.append(msgReply);
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
    const QJsonValue& filesArrayValue = data.value(QStringLiteral("files"));

    //sometimes messages dont have subtype, but have "files" array
    if (subtype == QStringLiteral("file_share") || filesArrayValue.isUndefined() == false) {
        //qDebug().noquote() << "file share json:" << QJsonDocument(data).toJson();
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

Attachment::~Attachment()
{
    qDeleteAll(fields);
}

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
        QQmlEngine::setObjectOwnership(field, QQmlEngine::CppOwnership);
        field->setData((fieldValue.toObject()));
        fields.append(field);
    }
}

Reaction::Reaction(QObject *parent) : QObject(parent) {}

void Reaction::setData(const QJsonObject &data)
{
    //qDebug().noquote() << "reaction" << QJsonDocument(data).toJson();
    name = data.value(QStringLiteral("name")).toString();
    m_emojiInfo = ImagesCache::instance()->getEmojiInfo(name);

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

ReplyField::ReplyField(QObject *parent): QObject(parent) {}

void ReplyField::setData(const QJsonObject &data)
{
    m_userId = data.value(QStringLiteral("user")).toString();
    m_ts = slackToDateTime(data.value(QStringLiteral("ts")).toString());
}
