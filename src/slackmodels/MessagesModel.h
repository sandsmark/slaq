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
class EmojiInfo;

namespace {
inline quint64 slackTsToInternalTs(const QString& slackts) {
    quint64 dt = 0;
    int ts_extra = 0;
    QStringList ts_ = slackts.split(".", QString::SkipEmptyParts);
    if (ts_.size() >= 1) {
        dt = ts_.at(0).toULongLong();
        if (ts_.size() == 2) {
            ts_extra = ts_.at(1).toInt();
        }
    } else {
        dt = slackts.toLongLong();
    }
    // code unix epoh to 1st 32 bits
    dt = (dt << 32) | ts_extra;
    return dt;
}

inline QString internalTsToSlackTs(quint64 dt) {
    quint64 _dt = dt >> 32;
    int ts_extra = (int)(dt & 0xFFFFFFFF);
    QString s= QString("%1.%2").arg(_dt).arg(ts_extra, 6, 10, QChar('0'));
    //qDebug() << "int to slack" << s << hex << dt;
    return s;
}

inline QDateTime internalTsToDateTime(quint64 dt) {
    quint64 _dt = dt >> 32;;
    int ts_extra = dt & 0xFFFFFFFF;
    return QDateTime::fromSecsSinceEpoch(_dt);
}

inline quint64 internalTsDiff(quint64 ts1, quint64 ts2) {
    return qAbs((ts2 >> 32) - (ts1 >> 32))*1000;
}

static int compareSlackTs(quint64 ts1, quint64 ts2) {
    if (ts1 == ts2) {
        return 0;
    }
    return ts1 > ts2 ? 1 : -1;
}
}

class Reaction : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList users MEMBER m_users NOTIFY usersChanged)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(int usersCount READ usersCount NOTIFY usersChanged)
    Q_PROPERTY(EmojiInfo* emojiInfo MEMBER m_emojiInfo CONSTANT)

public:
    Reaction(QObject *parent = nullptr);
    void setData(const QJsonObject &data);
    int usersCount() { return m_users.count(); }
    QString name() const;

    EmojiInfo *getEmojiInfo() const;
    void setEmojiInfo(EmojiInfo *emojiInfo);
    void setName(const QString &name);
    void appendUser(const QString& userName);

    QStringList m_userIds;
    QStringList m_users;
signals:
    void usersChanged();

private:
    QString m_name;
    EmojiInfo* m_emojiInfo;
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
    Q_PROPERTY(SlackUser* user READ user CONSTANT)
    Q_PROPERTY(quint64 ts MEMBER m_ts CONSTANT)

public:
    ReplyField(QObject* parent = nullptr);
    void setData(const QJsonObject &data);

    QPointer<SlackUser> m_user;
    QString m_userId;
    quint64 m_ts;
    SlackUser* user() const { return m_user.data(); }
};

//TODO: implement actions
class Attachment: public QObject {

    Q_OBJECT
    Q_PROPERTY(QUrl titleLink MEMBER titleLink CONSTANT)
    Q_PROPERTY(QString title MEMBER title CONSTANT)
    Q_PROPERTY(QString pretext MEMBER pretext CONSTANT)
    Q_PROPERTY(QString text MEMBER text CONSTANT)
    Q_PROPERTY(QString fallback MEMBER fallback CONSTANT)
    Q_PROPERTY(QString color MEMBER color CONSTANT)
    Q_PROPERTY(QUrl from_url MEMBER from_url CONSTANT)
    Q_PROPERTY(QUrl original_url MEMBER original_url CONSTANT)
    Q_PROPERTY(QUrl imageUrl MEMBER imageUrl CONSTANT)
    Q_PROPERTY(QSize imageSize MEMBER imageSize CONSTANT)
    Q_PROPERTY(QString author_name MEMBER author_name CONSTANT)
    Q_PROPERTY(QUrl author_link MEMBER author_link CONSTANT)
    Q_PROPERTY(QUrl author_icon MEMBER author_icon CONSTANT)
    Q_PROPERTY(QUrl thumb_url MEMBER thumb_url CONSTANT)
    Q_PROPERTY(QSize thumb_size MEMBER thumb_size CONSTANT)
    Q_PROPERTY(QUrl service_url MEMBER service_url CONSTANT)
    Q_PROPERTY(QString footer MEMBER footer CONSTANT)
    Q_PROPERTY(QString service_name MEMBER service_name CONSTANT)
    Q_PROPERTY(QUrl footer_icon MEMBER footer_icon CONSTANT)

    Q_PROPERTY(QString video_html MEMBER video_html CONSTANT)
    Q_PROPERTY(QSize video_html_size MEMBER video_html_size CONSTANT)
    Q_PROPERTY(QList<QObject*> fields MEMBER fields CONSTANT)
    Q_PROPERTY(QDateTime ts MEMBER ts CONSTANT)
    Q_PROPERTY(bool isAnimated MEMBER m_isAnimated CONSTANT)

public:
    Attachment(QObject* parent = nullptr);
    ~Attachment() override;
    void setData(const QJsonObject &data);
    QUrl titleLink;
    QString title;
    QString pretext;
    QString text;
    QString fallback;
    QString color;
    QString author_name;
    QString service_name;
    QUrl from_url;
    QUrl original_url;
    QUrl service_url;
    QUrl author_link;
    QUrl author_icon;
    QUrl thumb_url;
    QSize thumb_size;
    QString footer;
    QUrl footer_icon;
    QUrl imageUrl;
    QSize imageSize;
    QString video_html;
    QSize video_html_size;
    QDateTime ts;
    bool m_isAnimated { false };
    QList<QObject*> fields;
};

class FileShare: public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id MEMBER m_id CONSTANT)
    Q_PROPERTY(QDateTime created MEMBER m_created CONSTANT)
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)
    Q_PROPERTY(QString title MEMBER m_title CONSTANT)
    Q_PROPERTY(QString mimetype MEMBER m_mimetype CONSTANT)
    Q_PROPERTY(QString filetype MEMBER m_filetype CONSTANT)
    Q_PROPERTY(QString pretty_type MEMBER m_pretty_type CONSTANT)
    Q_PROPERTY(SlackUser* user MEMBER m_user CONSTANT)
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
    Q_PROPERTY(bool preview_is_truncated MEMBER m_preview_is_truncated CONSTANT)

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
    QString m_name;
    QString m_title;
    QString m_mimetype;
    QString m_filetype;
    QString m_pretty_type;
    QPointer<SlackUser> m_user;
    QString m_userId;
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
    bool m_preview_is_truncated { false };
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
    Message() = default;
    ~Message() = default;
    void setData(const QJsonObject &data);

    QString text;
    QString originalText;
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
    quint64 time;
    QString thread_ts;
    quint64 thread_time;
    QUrl permalink;

    QPointer<SlackUser> user;

    QList<QObject*> reactions;
    QList<QObject*> attachments;
    QList<QObject*> fileshares;
    QList<QObject*> replies;

    bool isChanged { false };
    bool isStarred { false };
    bool isSameUser { false }; //indicates that previuos message has same user
    qint64 timeDiffMs { 0 }; // time difference with previous message
    QSharedPointer<MessageListModel> messageThread;
    Message* parentMessage { nullptr };

    static bool compare(const Message* a, const Message* b) { return compareSlackTs(a->time, b->time) > 0; }

    QJsonObject toJson() {
        QJsonObject jo;
        jo["text"] = originalText;
        jo["type"] = type;
        jo["subtype"] = subtype;
        jo["channel_id"] = channel_id;
        jo["channel_name"] = channelName;
        jo["user_id"] = user_id;
        jo["team_id"] = team_id;
        jo["userName"] = userName;
        jo["time_slack"] = ts;
        jo["ts"] = QString("%1").arg(time, 0, 16);
        jo["thread_ts"] = thread_ts;
        return jo;
    }
};

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum MessageFields {
        Text,
        Subtype,
        OriginalText,
        User,
        Time,
        Timestamp,
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
    ~MessageListModel() override = default;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    inline bool isMessageThreadParent(const Message* msg) const { return !isMessageThreadChild(msg); }
    inline bool isMessageThreadChild(const Message* msg) const { return (!msg->thread_ts.isEmpty() && msg->thread_ts != msg->ts); }

    bool historyLoaded() const;
    void setHistoryLoaded(bool historyLoaded);

public slots:
    void addMessage(Message *message);
    void updateMessage(Message *message, bool replace = true);
    void addMessages(const QList<Message *> &messages, bool hasMore, const QString &threadTs,
                     bool isThread = false);
    bool deleteMessage(quint64 ts);

    Message* message(const QString &ts);
    Message* messageAt(int row);
    Message* message(quint64 ts);

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
    QString firstMessageTs() const;
    bool isThreadModel() const;
    MessageListModel* createThread(Message* parentMessage);
    void processChildMessage(Message *message);
    void updateReactionUsers(Message *message);
    int countUnread(quint64 lastRead);
    void usersModelChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    quint64 lastMessage() const;
    void requestMissedMessages();
protected:
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

signals:
    void fetchMoreMessages(const QString& channelId, const QString& latest,
                           const QString& oldest, const QString& threadTs);
private:
    void findNewUsers(QString &message);


protected:
    QList<Message*> m_messages;
    UsersModel *m_usersModel;
    mutable QMutex m_modelMutex;

private:
    QMap<QString, Reaction*> m_reactions;
    QMap<QString, QSharedPointer<MessageListModel>> m_MessageThreads;

    QString m_channelId;
    MessageFormatter m_formatter;
    QRegularExpression m_newUserPattern;
    QRegularExpression m_existingUserPattern;
    bool m_hasMore { false }; //indicator if there is more data in the channel's history
    bool m_isThreadModel { false };
    bool m_historyLoaded { false };
};
