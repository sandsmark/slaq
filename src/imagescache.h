#ifndef IMAGESCACHE_H
#define IMAGESCACHE_H

#include <QObject>
#include <QUrl>
#include <QMap>
#include <QString>
#include <QImage>
#include <QDateTime>
#include <QtNetwork/QNetworkAccessManager>
#include <QMutexLocker>

#include "emojiinfo.h"

class ImagesCache : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isUnicode READ isUnicode NOTIFY isUnicodeChanged)

public:

    static ImagesCache* instance();
    virtual ~ImagesCache();

    bool isExist(const QString &id);
    bool isCached(const QString &id);
    QImage image(const QString &id);
    bool isImagesDatabaseLoaded() { return m_emojiList.size() > 0; }
    void loadImagesDatabase();
    void parseSlackJson();
    Q_INVOKABLE void setEmojiImagesSet(const QString& setName);
    Q_INVOKABLE QString getEmojiImagesSet();
    Q_INVOKABLE int getEmojiImagesSetIndex();
    Q_INVOKABLE QStringList getEmojiImagesSetsNames();
    Q_INVOKABLE QStringList getEmojiCategories();
    Q_INVOKABLE QVariant getEmojisByCategory(const QString &category);
    bool isUnicode() const;
    Q_INVOKABLE QString getEmojiByName(const QString& name) const;
    Q_INVOKABLE QString getNameByEmoji(const QString& emoji) const;
    Q_INVOKABLE QStringList getCategoriesSymbols() const;

signals:
    void imageLoaded(const QString &id);
    void requestImageViaHttp(const QString &id);
    void emojiReaded();
    void emojisSetsIndexChanged(int emojisIndex);
    void isUnicodeChanged(bool isUnicode);

private slots:
    void onImageRequestedViaHttp(const QString &id);
    void onImageRequestFinished();

private:
    explicit ImagesCache(QObject *parent = nullptr);
    void checkImagesPresence();

private:
    QDateTime m_lastUpdate;
    QNetworkAccessManager m_qnam;

    QStringList m_imagesSetsFolders;
    QStringList m_imagesSetsNames;
    QStringList m_categoriesSymbols;
    int m_currentImagesSetIndex; //represents current images set. Might be several in images cache folder
    QString m_cache;
    QHash<QString, EmojiInfo *> m_emojiList;
    QMultiMap<QString, EmojiInfo *> m_emojiCategories;

    QList<QNetworkReply*> m_activeRequests;
    QMutex m_requestsListMutex;
};

#endif // IMAGESCACHE_H
