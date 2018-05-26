#ifndef SLACKEMOJISMODEL_H
#define SLACKEMOJISMODEL_H

#include <QObject>
#include <QMultiMap>
#include <QHash>

class EmojiInfo: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString unified READ unified CONSTANT)
    Q_PROPERTY(QString nonqualified READ nonqualified CONSTANT)
    Q_PROPERTY(QString image READ image CONSTANT)
    Q_PROPERTY(QStringList shortNames READ shortNames CONSTANT)
    Q_PROPERTY(QString text READ text CONSTANT)
    Q_PROPERTY(QStringList texts READ texts CONSTANT)
    Q_PROPERTY(QString category READ category CONSTANT)
    Q_PROPERTY(int sortOrder READ sortOrder CONSTANT)
    Q_PROPERTY(ImagesExist imagesExist READ imagesExist CONSTANT)
    Q_PROPERTY(bool cached READ cached WRITE setCached NOTIFY cachedChanged)

    Q_ENUMS(ImageExist)
    Q_FLAGS(ImagesExist)

public:

    enum ImageExist {
        ImageApple =        (1<<0),
        ImageGoogle =       (1<<1),
        ImageTwitter =      (1<<2),
        ImageEmojione =     (1<<3),
        ImageFacebook =     (1<<4),
        ImageMessenger =    (1<<5),
    };
    Q_DECLARE_FLAGS(ImagesExist, ImageExist)

    EmojiInfo(QObject *parent = nullptr):QObject(parent) {}
    QString name() const { return m_name; }
    ImagesExist imagesExist() const { return m_imagesExist; }
    QString unified() const { return m_unified; }
    QString nonqualified() const { return m_nonqualified; }
    QString image() const { return m_image; }
    QStringList shortNames() const { return m_shortNames; }
    QString text() const { return m_text; }
    QStringList texts() const { return m_texts; }
    QString category() const { return m_category; }
    int sortOrder() const { return m_sortOrder; }
    bool cached() const { return m_cached; }

    QString m_name;
    QString m_unified;
    QString m_nonqualified;
    QString m_image;
    QString m_text;
    QStringList m_texts;
    QString m_category;
    ImagesExist m_imagesExist;
    QStringList m_shortNames;
    int m_sortOrder { 0 };
    bool m_cached { false };

public slots:
    void setCached(bool cached)
    {
        if (m_cached == cached)
            return;

        m_cached = cached;
        emit cachedChanged(m_cached);
    }
signals:
    void cachedChanged(bool cached);
};

Q_DECLARE_METATYPE(EmojiInfo*)
Q_DECLARE_OPERATORS_FOR_FLAGS(EmojiInfo::ImagesExist)

#endif // SLACKEMOJISMODEL_H
