#ifndef STORAGE_H
#define STORAGE_H

//#include <QObject>
#include <QVariantMap>

class Storage
{
    //Q_OBJECT
public:
    QVariantMap user(const QVariant& id);
    const QVariantList &users();
    void saveUser(const QVariantMap& user);
    void updateUsersList();

    QVariantMap channel(const QVariant& id);
    QVariantList channels();
    void saveChannel(const QVariantMap& channel);

    QVariantList channelMessages(const QVariant& channelId);
    bool channelMessagesExist(const QVariant& channelId);
    void setChannelMessages(const QVariant& channelId, const QVariantList& messages);
    void appendChannelMessage(const QVariant& channelId, const QVariantMap& message);
    void clearChannelMessages();
    void clearChannels();
signals:

public slots:

private:
    QVariantMap userMap;
    QVariantMap channelMap;
    QVariantMap channelMessageMap;
    QVariantList userList;
};

#endif // STORAGE_H
