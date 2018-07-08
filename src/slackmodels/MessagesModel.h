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

#include "UsersModel.h"

class Reaction : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList userIds MEMBER userIds NOTIFY usersChanged)
    Q_PROPERTY(QString emoji MEMBER emoji CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(int usersCount READ usersCount NOTIFY usersChanged)

public:
    Reaction(const QJsonObject &data, QObject *parent = nullptr);
    int usersCount() { return userIds.count(); }

signals:
    void usersChanged();

public:
    QStringList userIds;
    QString name;
    QString emoji;
};

class AttachmentField: public QObject  {
    Q_GADGET
    Q_PROPERTY(QString title MEMBER title CONSTANT)
    Q_PROPERTY(QString value MEMBER value CONSTANT)
    Q_PROPERTY(bool isShort MEMBER isShort CONSTANT)

public:
    AttachmentField(QObject* parent = nullptr);

    void setData(const QJsonObject &data);

    QString title;
    QString value;
    bool isShort { false };
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

class FileShare {
    Q_GADGET
    Q_PROPERTY(QString titleLink MEMBER titleLink CONSTANT)
    Q_PROPERTY(QString title MEMBER title CONSTANT)
    Q_PROPERTY(QString pretext MEMBER pretext CONSTANT)
    Q_PROPERTY(QString text MEMBER text CONSTANT)
    Q_PROPERTY(QString fallback MEMBER fallback CONSTANT)
    Q_PROPERTY(QString color MEMBER color CONSTANT)

public:
    FileShare(const QJsonObject &data);

    QString titleLink;
    QString title;
    QString pretext;
    QString text;
    QString fallback;
    QString color;

    QUrl imageUrl;
    QSize imageSize;
};

class Message: public QObject {
    Q_OBJECT

    //Q_PROPERTY(QQmlListProperty<Attachment> attachments READ getAttachments NOTIFY attachmentsChanged)

public:
    Message(QObject* parent = nullptr);
    ~Message();
//    QQmlListProperty<Attachment> getAttachments();
//    static int countAttachments(QQmlListProperty<Attachment> *property);
//    static Attachment *atAttachments(QQmlListProperty<Attachment> *property, int index);

    void setData(const QJsonObject &data);
    QString text;
    QDateTime time;
    QString type;
    QString subtype;
    QString channel_id;
    QString user_id;
    QString team_id;
    bool isStarred;
    QStringList pinnedTo;

    QPointer<User> user;
    QList<Reaction*> reactions;
    QList<QObject*> attachments;
    QList<FileShare*> filechares;

signals:
    void attachmentsChanged(QQmlListProperty<Attachment> attachments);
};

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum MessageFields {
        Text,
        User,
        Time,
        Attachments,
        Reactions,
        FileShares,
        MessageFieldCount
    };

    MessageListModel(QObject *parent, UsersModel *usersModel);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addMessage(Message *message);
    void addMessages(const QJsonArray &messages);

private:
    QList<Message*> m_messages;
    QMap<QString, Reaction*> m_reactions;
    UsersModel *m_usersModel;
};

//QML_DECLARE_TYPE(Attachment)
//Q_DECLARE_METATYPE(Attachment)

