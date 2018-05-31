#include "storage.h"

#include <QDebug>

QVariantMap Storage::userMap = QVariantMap();
QVariantList Storage::userList = QVariantList();
QVariantMap Storage::channelMap = QVariantMap();
QVariantMap Storage::channelMessageMap = QVariantMap();

void Storage::saveUser(const QVariantMap& user)
{
    userMap.insert(user.value("id").toString(), user);
    userList = userMap.values();
}

QVariantMap Storage::user(const QVariant& id)
{
    return userMap.value(id.toString()).toMap();
}

const QVariantList& Storage::users()
{
    return userList;
}

void Storage::saveChannel(const QVariantMap& channel)
{
    channelMap.insert(channel.value("id").toString(), channel);
}

QVariantMap Storage::channel(const QVariant& id)
{
    return channelMap.value(id.toString()).toMap();
}

QVariantList Storage::channels()
{
    return channelMap.values();
}

QVariantList Storage::channelMessages(const QVariant& channelId)
{
    return channelMessageMap.value(channelId.toString()).toList();
}

bool Storage::channelMessagesExist(const QVariant& channelId)
{
    return channelMessageMap.contains(channelId.toString());
}

void Storage::setChannelMessages(const QVariant& channelId, const QVariantList& messages)
{
    channelMessageMap.insert(channelId.toString(), messages);
}

void Storage::appendChannelMessage(const QVariant& channelId, const QVariantMap& message)
{
    QVariantList messages = channelMessages(channelId);
    messages.append(message);
    setChannelMessages(channelId, messages);
}

void Storage::clearChannelMessages()
{
    channelMessageMap.clear();
}

void Storage::clearChannels()
{
    channelMap.clear();
}
