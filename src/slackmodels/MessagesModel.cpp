#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QDateTime>
#include <QJsonArray>

#include "MessagesModel.h"
#include "messageformatter.h"

MessageListModel::MessageListModel(QObject *parent, UsersModel *usersModel) : QAbstractListModel(parent),
    m_usersModel(usersModel)
{
}

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_messages.count();
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row >= m_messages.count() || row < 0) {
        qWarning() << "Invalid row" << row;
        return QVariant();
    }
    switch(role) {
    case Text:
        return m_messages[row]->text;
    case User:
        //qDebug() << "User for row" << row << m_messages[row].user.data();
        return QVariant::fromValue(m_messages[row]->user.data());
    case Time:
        return m_messages[row]->time;
    case Attachments:
        //qDebug() << "Attachment for row" << row << m_messages[row]->attachments.count();
        return QVariant::fromValue(m_messages[row]->attachments);
    case Reactions:
        return QVariant::fromValue(m_messages[row]->reactions);
    case FileShares:
        return QVariant::fromValue(m_messages[row]->filechares);
    case IsStarred:
        return QVariant::fromValue(m_messages[row]->isStarred);
    case IsChanged:
        return QVariant::fromValue(m_messages[row]->isChanged);
    default:
        qWarning() << "Unhandled role" << role;
        return QVariant();
    }
}

void MessageListModel::addMessage(Message* message)
{
    beginInsertRows(QModelIndex(),0, 1);
    qDebug() << "adding message:" << message->text;
    m_messages.insert(0, message);
    endInsertRows();
}

void MessageListModel::updateMessage(Message *message)
{
    for (int i = 0; i < m_messages.count(); i++) {
        Message* oldmessage = m_messages.at(i);
        if (oldmessage->time == message->time) {
            if (message->user.isNull()) {
                message->user = oldmessage->user;
            }
            m_messages.replace(i, message);
            QModelIndex index = QAbstractListModel::index (i, 0,  QModelIndex());
            emit dataChanged(index, index);
            break;
        }
    }
}

void MessageListModel::addMessages(const QJsonArray &messages)
{
    qDebug() << "Adding" << messages.count() << "messages";
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count() + messages.count());

    for (const QJsonValue &messageData : messages) {
        const QJsonObject messageObject = messageData.toObject();
        Message* message = new Message;
        message->setData(messageObject);
        if (message->user_id.isEmpty()) {
            qWarning() << "user id is empty" << messageObject;
        }
        message->user = m_usersModel->user(message->user_id);
        // TODO: why?
        //m_reactions.insert(reactionObject["name"].toString(), reaction);
        if (message->user.isNull()) {
            qWarning() << "no user for" << message->user_id << m_usersModel;
        }
        //Q_ASSERT(!message.user.isNull());

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
    names[Attachments] = "Attachments";
    names[Reactions] = "Reactions";
    names[FileShares] = "FileShares";
    names[IsStarred] = "IsStarred";
    names[IsChanged] = "IsChanged";
    return names;
}

Message::Message(QObject *parent) : QObject(parent)
{}

Message::~Message()
{
    qDeleteAll(attachments);
    attachments.clear();
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


void Message::setData(const QJsonObject &data)
{
    //qDebug() << "message" << data;
    type = data.value(QStringLiteral("type")).toString();
    time = QDateTime::fromSecsSinceEpoch(data.value(QStringLiteral("ts")).toString().toDouble());
    text = data.value(QStringLiteral("text")).toString();
    channel_id = data.value(QStringLiteral("channel")).toString();
    user_id = data.value(QStringLiteral("user")).toString();
    if (user_id.isEmpty()) {
        const QString& subtype = data.value(QStringLiteral("subtype")).toString(QStringLiteral("default"));
        if (subtype == QStringLiteral("bot_message")) {
            user_id = data.value(QStringLiteral("bot_id")).toString();
        }
    }

    subtype = data.value(QStringLiteral("subtype")).toString();
    for (const QJsonValue &reactionValue : data.value("reactions").toArray()) {
        Reaction *reaction = new Reaction(reactionValue.toObject());
        QQmlEngine::setObjectOwnership(reaction, QQmlEngine::CppOwnership);
        reactions.append(reaction);
    }

    foreach (const QJsonValue &attachmentValue, data.value(QStringLiteral("attachments")).toArray()) {
        Attachment* attachment = new Attachment;
        attachment->setData(attachmentValue.toObject());
        QQmlEngine::setObjectOwnership(attachment, QQmlEngine::CppOwnership);
        attachments.append(attachment);
    }
}

//QString titleLink = attachmentObject.value(QStringLiteral("title_link")).toString();
//QString title = attachmentObject.value(QStringLiteral("title")).toString();
//QString pretext = attachment.value(QStringLiteral("pretext")).toString();
//QString text = attachment.value(QStringLiteral("text")).toString();
//QString fallback = attachment.value(QStringLiteral("fallback")).toString();
//QString color = getAttachmentColor(attachment);
//QVariantList fields = getAttachmentFields(attachment);
//QVariantList images = getAttachmentImages(attachment);

//m_formatter.replaceLinks(pretext);
//m_formatter.replaceLinks(text);
//m_formatter.replaceLinks(fallback);
//m_formatter.replaceEmoji(text);
//m_formatter.replaceEmoji(pretext);
//m_formatter.replaceEmoji(fallback);
//m_formatter.replaceSpecialCharacters(text);
//m_formatter.replaceSpecialCharacters(pretext);
//m_formatter.replaceSpecialCharacters(fallback);

//int index = text.indexOf(' ', 250);
//if (index > 0) {
//    text = text.left(index) + "...";
//}

//if (!title.isEmpty() && !titleLink.isEmpty()) {
//    title = "<a href=\"" + titleLink + "\">" + title + "</a>";
//}

//data.insert(QStringLiteral("title"), QVariant(title));
//data.insert(QStringLiteral("pretext"), QVariant(pretext));
//data.insert(QStringLiteral("content"), QVariant(text));
//data.insert(QStringLiteral("fallback"), QVariant(fallback));
//data.insert(QStringLiteral("indicatorColor"), QVariant(color));
//data.insert(QStringLiteral("fields"), QVariant(fields));
//data.insert(QStringLiteral("images"), QVariant(images));

Attachment::Attachment(QObject *parent): QObject(parent)
{
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
    MessageFormatter formatter;
    formatter.replaceLinks(pretext);
    formatter.replaceLinks(text);
    formatter.replaceLinks(fallback);
    formatter.replaceEmoji(text);
    formatter.replaceEmoji(pretext);
    formatter.replaceEmoji(fallback);
    formatter.replaceSpecialCharacters(text);
    formatter.replaceSpecialCharacters(pretext);
    formatter.replaceSpecialCharacters(fallback);

    const QJsonArray& fieldList = data.value(QStringLiteral("fields")).toArray();

    foreach (const QJsonValue &fieldValue, fieldList) {
        AttachmentField* field = new AttachmentField;
        field->setData((fieldValue.toObject()));

        if (!field->title.isEmpty()) {
            formatter.replaceLinks(field->title);
            formatter.replaceMarkdown(field->title);
            formatter.replaceSpecialCharacters(field->title);
        }

        if (!field->value.isEmpty()) {
            formatter.replaceLinks(field->value);
            formatter.replaceMarkdown(field->value);
            formatter.replaceSpecialCharacters(field->value);
        }
        fields.append(field);
    }
}

Reaction::Reaction(const QJsonObject &data, QObject *parent) : QObject(parent)
{
    name = data[QStringLiteral("name")].toString();
    emoji = data[QStringLiteral("emoji")].toString();

    const QJsonArray usersList = data[QStringLiteral("users")].toArray();
    for (const QJsonValue &usersValue : usersList) {
        userIds.append(usersValue.toObject()["name"].toString());
    }
}

AttachmentField::AttachmentField(QObject *parent): QObject(parent)
{
}

void AttachmentField::setData(const QJsonObject &data)
{
    title = data.value(QStringLiteral("title")).toString();
    value = data.value(QStringLiteral("value")).toString();
    isShort = data.value(QStringLiteral("short")).toBool();
}
