#pragma once

#include <QObject>
#include <QUrl>
#include <QMap>
#include <QString>
#include <QImage>
#include <QDateTime>
#include <QMutexLocker>

#include "QQmlObjectListModel.h"
#include "emojiinfo.h"
#include "networkaccessmanager.h"

class ImagesCache : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isUnicode READ isUnicode NOTIFY isUnicodeChanged)

public:

    static ImagesCache* instance();
    virtual ~ImagesCache() = default;

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
    Q_INVOKABLE QVariant getEmojisByCategory(int category, const QString &teamId);
    Q_INVOKABLE QVariant getLastUsedEmojisModel(const QString &teamId);
    bool isUnicode() const;
    Q_INVOKABLE QString getEmojiByName(const QString& name) const;
    Q_INVOKABLE QString getNameByEmoji(const QString& emoji) const;

    void addEmoji(EmojiInfo *einfo, bool visibleCategory = true);
    void sendEmojisUpdated();
    void addEmojiAlias(const QString &emojiName, const QString &emojiAlias);
    EmojiInfo* getEmojiInfo(const QString& name);

    QQmlObjectListModel<EmojiCategoryHolder>* emojiCategoriesModel();

public slots:
    void addLastUsedEmoji(const QString &teamId, const QString &emojiName);
    QStringList getLastUsedEmojisList(const QString &teamId);
    void setLastUsedEmojisList(const QString &teamId, const QStringList &emojis);
signals:
    void imageLoaded(const QString &id);
    void requestImageViaHttp(const QString &id);
    void emojisDatabaseReaded();
    void emojisUpdated();
    void emojisSetsIndexChanged(int emojisIndex);
    void isUnicodeChanged(bool isUnicode);

private slots:
    void onImageRequestedViaHttp(const QString &id);
    void onImageRequestFinished();
    void checkImagesPresence();

private:
    explicit ImagesCache(QObject *parent = nullptr);


private:
    QDateTime m_lastUpdate;
    NetworkAccessManager m_qnam;

    QStringList m_imagesSetsFolders;
    QStringList m_imagesSetsNames;
    int m_currentImagesSetIndex {-1}; //represents current images set. Might be several in images cache folder
    QString m_cache;
    QHash<QString, QImage> m_requestedImages;
    QHash<QString, EmojiInfo *> m_emojiList;
    QMultiMap<EmojiInfo::EmojiCategories, EmojiInfo *> m_emojiCategories;
    QSet<QString> m_iconsCached;

    QList<QNetworkReply*> m_activeRequests;
    QMutex m_requestsListMutex;

    QQmlObjectListModel<EmojiCategoryHolder> m_EmojiCategoriesModel;
    bool m_cacheSlackImages { true };
    QMap<QString, QQmlObjectListModel<EmojiInfo>*> m_lastUsedEmojisModels;

    QMutex m_mutex;
};
