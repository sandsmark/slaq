#ifndef STORAGE_H
#define STORAGE_H

#include <QObject>

class Storage : public QObject
{
    Q_OBJECT
public:
    static QVariantMap user(const QVariant& id);
    static const QVariantList &users();
    static void saveUser(const QVariantMap& user);

    static QVariantMap channel(const QVariant& id);
    static QVariantList channels();
    static void saveChannel(const QVariantMap& channel);

    static QVariantList channelMessages(const QVariant& channelId);
    static bool channelMessagesExist(const QVariant& channelId);
    static void setChannelMessages(const QVariant& channelId, const QVariantList& messages);
    static void appendChannelMessage(const QVariant& channelId, const QVariantMap& message);
    static void clearChannelMessages();

signals:

public slots:

private:
    static QVariantMap userMap;
    static QVariantMap channelMap;
    static QVariantMap channelMessageMap;
    static QVariantList userList;
};

#endif // STORAGE_H
