#ifndef STORAGE_H
#define STORAGE_H

#include <QObject>

class Storage : public QObject
{
    Q_OBJECT
public:
    static QVariantMap user(QVariant id);
    static QVariantList users();
    static void saveUser(QVariantMap user);

    static QVariantMap channel(QVariant id);
    static QVariantList channels();
    static void saveChannel(QVariantMap channel);

    static QVariantList channelMessages(QVariant channelId);
    static bool channelMessagesExist(QVariant channelId);
    static void setChannelMessages(QVariant channelId, QVariantList messages);
    static void appendChannelMessage(QVariant channelId, QVariantMap message);
    static void clearChannelMessages();

signals:

public slots:

private:
    static QVariantMap userMap;
    static QVariantMap channelMap;
    static QVariantMap channelMessageMap;
};

#endif // STORAGE_H
