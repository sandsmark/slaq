#pragma once

#include <QObject>
#include <QtQml>

#include <QString>
#include <QUrl>
#include <QVariantMap>
#include <QPointer>
#include <QAbstractListModel>
#include <QSize>
#include <QDateTime>
#include <QStringList>

#include "UsersModel.h"
#include "messageformatter.h"

class ChatsModel;
class MessageListModel;

namespace {
QDateTime slackToDateTime(const QString& slackts) {
    QDateTime dt;
    QStringList ts_ = slackts.split(".");
    if (ts_.size() >= 1) {
        dt = QDateTime::fromSecsSinceEpoch(ts_.at(0).toLongLong());
    }

    if (ts_.size() == 2) {
        int msecs = ts_.at(1).toInt();
        dt = dt.addMSecs(msecs);
    }
    return dt;
}

QString dateTimeToSlack(const QDateTime& dt) {
    return QString("%1.000").arg(dt.toSecsSinceEpoch()) + QString("%1").arg(dt.time().msec(), 3, 10, QChar('0'));
}
}

class Reaction : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList users MEMBER users CONSTANT/*NOTIFY usersChanged*/)
    Q_PROPERTY(QString emoji MEMBER emoji CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(int usersCount READ usersCount CONSTANT/*NOTIFY usersChanged*/)

public:
    Reaction(QObject *parent = nullptr);
    void setData(const QJsonObject &data);
    int usersCount() { return users.count(); }

signals:
    void usersChanged();

public:
    QStringList users;
    QStringList userIds;
    QString name;
    QString emoji;
};

class AttachmentField: public QObject  {
    Q_OBJECT
    Q_PROPERTY(QString fieldTitle MEMBER m_title CONSTANT)
    Q_PROPERTY(QString fieldValue MEMBER m_value CONSTANT)
    Q_PROPERTY(bool fieldIsShort MEMBER m_isShort CONSTANT)

public:
    AttachmentField(QObject* parent = nullptr);

    void setData(const QJsonObject &data);

    QString m_title;
    QString m_value;
    bool m_isShort { true };
};

class ReplyField: public QObject  {
    Q_OBJECT
    Q_PROPERTY(User* user MEMBER m_user CONSTANT)
    Q_PROPERTY(QDateTime ts MEMBER m_ts CONSTANT)

public:
    ReplyField(QObject* parent = nullptr);
    void setData(const QJsonObject &data);

    User* m_user;
    QString m_userId;
    QDateTime m_ts;
};

//TODO: implement actions
class Attachment: public QObject {

    Q_OBJECT
    Q_PROPERTY(QString titleLink MEMBER titleLink CONSTANT)
    Q_PROPERTY(QString title MEMBER title CONSTANT)
    Q_PROPERTY(QString pretext MEMBER pretext CONSTANT)
    Q_PROPERTY(QString text MEMBER text CONSTANT)
    Q_PROPERTY(QString fallback MEMBER fallback CONSTANT)
    Q_PROPERTY(QString color MEMBER color CONSTANT)
    Q_PROPERTY(QUrl imageUrl MEMBER imageUrl CONSTANT)
    Q_PROPERTY(QString author_name MEMBER author_name CONSTANT)
    Q_PROPERTY(QUrl author_link MEMBER author_link CONSTANT)
    Q_PROPERTY(QUrl author_icon MEMBER author_icon CONSTANT)
    Q_PROPERTY(QUrl thumb_url MEMBER thumb_url CONSTANT)
    Q_PROPERTY(QString footer MEMBER footer CONSTANT)
    Q_PROPERTY(QUrl footer_icon MEMBER footer_icon CONSTANT)

    Q_PROPERTY(QSize imageSize MEMBER imageSize CONSTANT)
    Q_PROPERTY(QList<QObject*> fields MEMBER fields CONSTANT)
    Q_PROPERTY(QDateTime ts MEMBER ts CONSTANT)

public:
    Attachment(QObject* parent = nullptr);
    void setData(const QJsonObject &data);
    QString titleLink;
    QString title;
    QString pretext;
    QString text;
    QString fallback;
    QString color;
    QString author_name;
    QUrl author_link;
    QUrl author_icon;
    QUrl thumb_url;
    QString footer;
    QUrl footer_icon;
    QUrl imageUrl;
    QSize imageSize;
    QDateTime ts;

    QList<QObject*> fields;
};

class FileShare: public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id MEMBER m_id CONSTANT)
    Q_PROPERTY(QDateTime created MEMBER m_created CONSTANT)
    Q_PROPERTY(QDateTime timestamp MEMBER m_timestamp CONSTANT)
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)
    Q_PROPERTY(QString title MEMBER m_title CONSTANT)
    Q_PROPERTY(QString mimetype MEMBER m_mimetype CONSTANT)
    Q_PROPERTY(QString filetype MEMBER m_filetype CONSTANT)
    Q_PROPERTY(QString pretty_type MEMBER m_pretty_type CONSTANT)
    Q_PROPERTY(QPointer<User> user MEMBER m_user CONSTANT)
    Q_PROPERTY(FileShareModes mode MEMBER m_mode CONSTANT)
    Q_PROPERTY(bool editable MEMBER m_editable CONSTANT)
    Q_PROPERTY(bool is_external MEMBER m_is_external CONSTANT)
    Q_PROPERTY(QString external_type MEMBER m_external_type CONSTANT)
    Q_PROPERTY(QString username MEMBER m_username CONSTANT)
    Q_PROPERTY(quint64 size MEMBER m_size CONSTANT)
    Q_PROPERTY(QUrl url_private MEMBER m_url_private CONSTANT)
    Q_PROPERTY(QUrl url_private_download MEMBER m_url_private_download CONSTANT)
    Q_PROPERTY(QUrl thumb_video MEMBER m_thumb_video CONSTANT)
    Q_PROPERTY(QUrl thumb_64 MEMBER m_thumb_64 CONSTANT)
    Q_PROPERTY(QUrl thumb_80 MEMBER m_thumb_80 CONSTANT)
    Q_PROPERTY(QUrl thumb_160 MEMBER m_thumb_160 CONSTANT)
    Q_PROPERTY(QUrl thumb_360 MEMBER m_thumb_360 CONSTANT)
    Q_PROPERTY(QUrl thumb_360_gif MEMBER m_thumb_360_gif CONSTANT)
    Q_PROPERTY(QUrl thumb_480 MEMBER m_thumb_480 CONSTANT)
    Q_PROPERTY(QSize thumb_360_size MEMBER m_thumb_360_size CONSTANT)
    Q_PROPERTY(QSize thumb_480_size MEMBER m_thumb_480_size CONSTANT)
    Q_PROPERTY(QSize original_size MEMBER m_original_size CONSTANT)
    Q_PROPERTY(QUrl permalink MEMBER m_permalink CONSTANT)
    Q_PROPERTY(QUrl permalink_public MEMBER m_permalink_public CONSTANT)
    Q_PROPERTY(QUrl edit_link MEMBER m_edit_link CONSTANT)
    Q_PROPERTY(QString preview MEMBER m_preview CONSTANT)
    Q_PROPERTY(QString preview_highlight MEMBER m_preview_highlight CONSTANT)
    Q_PROPERTY(int lines MEMBER m_lines CONSTANT)
    Q_PROPERTY(int lines_more MEMBER m_lines_more CONSTANT)
    Q_PROPERTY(bool is_public MEMBER m_is_public CONSTANT)
    Q_PROPERTY(bool public_url_shared MEMBER m_public_url_shared CONSTANT)
    Q_PROPERTY(bool display_as_bot MEMBER m_display_as_bot CONSTANT)
    Q_PROPERTY(QStringList channels MEMBER m_channels CONSTANT)
    Q_PROPERTY(QStringList groups MEMBER m_groups CONSTANT)
    Q_PROPERTY(QStringList ims MEMBER m_ims CONSTANT)
    Q_PROPERTY(QString initial_comment MEMBER m_initial_comment CONSTANT)
    Q_PROPERTY(int num_stars MEMBER m_num_stars CONSTANT)
    Q_PROPERTY(bool is_starred MEMBER m_is_starred CONSTANT)
    Q_PROPERTY(QStringList pinned_to MEMBER m_pinned_to CONSTANT)
    Q_PROPERTY(QList<QObject*> reactions MEMBER m_reactions CONSTANT)
    Q_PROPERTY(int comments_count MEMBER m_comments_count CONSTANT)

public:
    FileShare(QObject* parent = nullptr);
    void setData(const QJsonObject &data);

    enum FileShareModes {
        Hosted,
        External,
        Snipped,
        Post
    };
    Q_ENUM(FileShareModes)

    QString m_id;
    QDateTime m_created;
    QDateTime m_timestamp;
    QString m_name;
    QString m_title;
    QString m_mimetype;
    QString m_filetype;
    QString m_pretty_type;
    QPointer<User> m_user;
    FileShareModes m_mode { Hosted };
    bool m_editable { true };
    bool m_is_external {false };
    QString m_external_type;
    QString m_username;
    quint64 m_size { 0 };
    QUrl m_url_private;
    QUrl m_url_private_download;
    QUrl m_thumb_64;
    QUrl m_thumb_80;
    QUrl m_thumb_360;
    QUrl m_thumb_360_gif;
    QSize m_thumb_360_size;
    QUrl m_thumb_480;
    QSize m_thumb_480_size;
    QSize m_original_size;
    QUrl m_thumb_160;
    QUrl m_thumb_video;
    QUrl m_permalink;
    QUrl m_permalink_public;
    QUrl m_edit_link;
    QString m_preview;
    QString m_preview_highlight;
    int m_lines { 0 };
    int m_lines_more { 0 };
    bool m_is_public { true };
    bool m_public_url_shared { false };
    bool m_display_as_bot { false };
    QStringList m_channels;
    QStringList m_groups;
    QStringList m_ims;
    QString m_initial_comment;
    int m_num_stars { 0 };
    bool m_is_starred { false };
    QStringList m_pinned_to;
    QList<QObject*> m_reactions;
    int m_comments_count { 0 };
};

struct Message {
private:
    Q_GADGET
    Q_PROPERTY(QDateTime threadts MEMBER thread_ts)
public:
    Message();
    ~Message();
    void setData(const QJsonObject &data);

    QString text;
    QString ts;
    QString type;
    QString subtype;
    QString channel_id;
    QString user_id;
    QString team_id;
    //available only in search results
    QString channelName;
    QString userName;

    QStringList pinnedTo;
    QDateTime time;
    QDateTime thread_ts;
    QUrl permalink;

    QPointer<User> user;

    QList<QObject*> reactions;
    QList<QObject*> attachments;
    QList<QObject*> fileshares;
    QList<QObject*> replies;

    bool isChanged { false };
    bool isStarred { false };
    bool isSameUser { false }; //indicates that previuos message has same user
    qint64 timeDiffMs { 0 }; // time difference with previous message
    QSharedPointer<MessageListModel> messageThread;

    static bool compare(const Message* a, const Message* b) { return a->time > b->time; }

    QJsonObject toJson() {
        QJsonObject jo;
        jo["text"] = text;
        jo["type"] = type;
        jo["subtype"] = subtype;
        jo["channel_id"] = channel_id;
        jo["channel_name"] = channelName;
        jo["user_id"] = user_id;
        jo["team_id"] = team_id;
        jo["userName"] = userName;
        jo["time_slack"] = dateTimeToSlack(time);
        return jo;
    }
};

Q_DECLARE_METATYPE(Message)

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum MessageFields {
        Text,
        User,
        Time,
        SlackTimestamp,
        Attachments,
        Reactions,
        FileShares,
        IsStarred,
        IsChanged,
        MessageFieldCount,
        SameUser,
        TimeDiff,
        SearchChannelName,
        SearchUserName,
        SearchPermalink,
        ThreadReplies,
        ThreadRepliesModel,
        ThreadIsParentMessage,
        ThreadTs
    };

    MessageListModel(QObject *parent, UsersModel *usersModel, const QString& channelId, bool isThreadModel = false);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    inline bool isMessageThreadParent(const Message* msg) const { return !isMessageThreadChild(msg); }
    inline bool isMessageThreadChild(const Message* msg) const { return (msg->thread_ts.isValid() && msg->thread_ts != msg->time); }

public slots:
    void addMessage(Message *message);
    void updateMessage(Message *message);
    void addMessages(const QList<Message *> &messages, bool hasMore);
    Message* message(const QDateTime& ts);
    bool deleteMessage(const QDateTime& ts);
    Message* message(int row);
    void clear();
    void appendMessageToModel(Message *message);
    void prependMessageToModel(Message *message);
    void modelDump();
    void sortMessages(bool asc = false);
    void refresh();
    void replaceMessage(Message* oldmessage, Message* message);
    void preprocessMessage(Message *message);

    // to provide for channel history in case if history fetched after new messages comes via RTM
    // avoid duplicates
    QDateTime firstMessageTs();
    bool isThreadModel() const;
    MessageListModel* createThread(Message* parentMessage);
    void processChildMessage(Message *message);
    void preprocessFormatting(ChatsModel *chat, Message *message);

protected:
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

signals:
    void fetchMoreMessages(const QString& channelId, const QDateTime& latest);
private:
    void findNewUsers(QString &message);
    void updateReactionUsers(Message *message);

protected:
    QList<Message*> m_messages;
    UsersModel *m_usersModel;
    mutable QMutex m_modelMutex;

private:
    QMap<QString, Reaction*> m_reactions;
    QMap<QDateTime, QSharedPointer<MessageListModel>> m_MessageThreads;

    QString m_channelId;
    MessageFormatter m_formatter;
    QRegularExpression m_newUserPattern;
    QRegularExpression m_existingUserPattern;
    bool m_hasMore { false }; //indicator if there is more data in the channel's history
    bool m_isThreadModel { false };
};

