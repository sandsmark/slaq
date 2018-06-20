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
    Q_PROPERTY(EmojiCategories category READ category CONSTANT)
    Q_PROPERTY(int sortOrder READ sortOrder CONSTANT)
    Q_PROPERTY(ImagesExist imagesExist READ imagesExist CONSTANT)
    Q_PROPERTY(bool cached READ cached WRITE setCached NOTIFY cachedChanged)
    Q_PROPERTY(QString teamId READ teamId CONSTANT)

    Q_ENUMS(ImageExist)
    Q_FLAGS(ImagesExist)

    Q_ENUMS(EmojiCategory)
    Q_FLAGS(EmojiCategories)

public:

    enum ImageExist {
        ImageApple =        (1<<0),
        ImageGoogle =       (1<<1),
        ImageTwitter =      (1<<2),
        ImageEmojione =     (1<<3),
        ImageFacebook =     (1<<4),
        ImageMessenger =    (1<<5),
        ImageSlackTeam =    (1<<6),
    };
    Q_DECLARE_FLAGS(ImagesExist, ImageExist)

    enum EmojiCategory {
        EmojiCategoryPeople,
        EmojiCategoryNature,
        EmojiCategoryFoodAndDrink,
        EmojiCategoryActivity,
        EmojiCategoryTravelAndPlaces,
        EmojiCategoryObjects,
        EmojiCategorySymbols,
        EmojiCategoryFlags,
        EmojiCategorySkinTone,
        EmojiCategoryCustom = 99
    };
    Q_DECLARE_FLAGS(EmojiCategories, EmojiCategory)



    explicit EmojiInfo(QObject *parent = nullptr):QObject(parent) {}
    QString name() const { return m_name; }
    ImagesExist imagesExist() const { return m_imagesExist; }
    QString unified() const { return m_unified; }
    QString nonqualified() const { return m_nonqualified; }
    QString image() const { return m_image; }
    QStringList shortNames() const { return m_shortNames; }
    QString text() const { return m_text; }
    QStringList texts() const { return m_texts; }
    EmojiCategories category() const { return m_category; }
    int sortOrder() const { return m_sortOrder; }
    bool cached() const { return m_cached; }

    QString m_name;
    QString m_unified;
    QString m_nonqualified;
    QString m_image;
    QString m_text;
    QStringList m_texts;
    EmojiCategories m_category;
    ImagesExist m_imagesExist;
    QStringList m_shortNames;
    int m_sortOrder { 0 };
    bool m_cached { false };
    QString m_teamId;

    QString teamId() const
    {
        return m_teamId;
    }

public slots:
    void setCached(bool cached)
    {
        if (m_cached == cached) {
            return;
        }

        m_cached = cached;
        emit cachedChanged(m_cached);
    }
signals:
    void cachedChanged(bool cached);
};

class EmojiCategoryHolder: public QObject
{
    Q_OBJECT

    Q_PROPERTY(EmojiInfo::EmojiCategories category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(QString unicode READ unicode WRITE setUnicode NOTIFY unicodeChanged)
    Q_PROPERTY(QString visibleName READ visibleName WRITE setVisibleName NOTIFY visibleNameChanged)
    Q_PROPERTY(QString dbName READ dbName WRITE setDbName NOTIFY dbNameChanged)
    Q_PROPERTY(bool isVisible READ isVisible WRITE setIsVisible NOTIFY isVisibleChanged)

    EmojiInfo::EmojiCategories m_category;
    QString m_unicode;
    QString m_visibleName;
    QString m_dbName;
    bool m_isVisible{true};

public:
    explicit EmojiCategoryHolder(QObject *parent = nullptr): QObject(parent) {}
    explicit EmojiCategoryHolder(EmojiInfo::EmojiCategories category,
                                 const QString& unicode,
                                 const QString& visibleName,
                                 const QString& dbName,
                                 bool isVisible,
                                 QObject *parent = nullptr): QObject(parent) {
        setCategory(category);
        setUnicode(unicode);
        setVisibleName(visibleName);
        setDbName(dbName);
        setIsVisible(isVisible);
    }

    EmojiInfo::EmojiCategories category() const { return m_category; }
    QString unicode() const { return m_unicode; }
    QString visibleName() const { return m_visibleName; }
    QString dbName() const { return m_dbName; }
    bool isVisible() const { return m_isVisible; }

public slots:
    void setCategory(EmojiInfo::EmojiCategories category)
    {
        if (m_category == category) {
            return;
        }

        m_category = category;
        emit categoryChanged(m_category);
    }

    void setUnicode(const QString& unicode)
    {
        if (m_unicode == unicode) {
            return;
        }

        m_unicode = unicode;
        emit unicodeChanged(m_unicode);
    }

    void setVisibleName(const QString& visibleName)
    {
        if (m_visibleName == visibleName) {
            return;
        }

        m_visibleName = visibleName;
        emit visibleNameChanged(m_visibleName);
    }

    void setDbName(const QString& dbName)
    {
        if (m_dbName == dbName) {
            return;
        }

        m_dbName = dbName;
        emit dbNameChanged(m_dbName);
    }

    void setIsVisible(bool isVisible)
    {
        if (m_isVisible == isVisible) {
            return;
        }

        m_isVisible = isVisible;
        emit isVisibleChanged(m_isVisible);
    }

signals:
    void categoryChanged(EmojiInfo::EmojiCategories category);
    void unicodeChanged(QString unicode);
    void visibleNameChanged(QString visibleName);
    void dbNameChanged(QString dbName);
    void isVisibleChanged(bool isVisible);
};

Q_DECLARE_METATYPE(EmojiInfo*)
Q_DECLARE_METATYPE(EmojiCategoryHolder*)
Q_DECLARE_OPERATORS_FOR_FLAGS(EmojiInfo::ImagesExist)
Q_DECLARE_OPERATORS_FOR_FLAGS(EmojiInfo::EmojiCategories)

#endif // SLACKEMOJISMODEL_H
