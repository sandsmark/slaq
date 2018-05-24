#include "imagescache.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QQmlEngine>

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <QSettings>

#include <QtNetwork/QNetworkReply>

ImagesCache::ImagesCache(QObject *parent) : QObject(parent)
{
    m_cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/images";
    if (!QDir().mkpath(m_cache)) {
        qWarning() << "Cant create images cache folder" << m_cache;
    }
    m_imagesSetsFolders << "unicode"
                        << "img-apple-160" << "img-apple-64"
                        <<"img-emojione-64"
                       << "img-facebook-64" << "img-facebook-96"
                       << "img-google-136" << "img-google-64"
                       << "img-messenger-128" << "img-messenger-64"
                       <<"img-twitter-64" << "img-twitter-72";
    m_imagesSetsNames << "Unicode"
                      << "Apple 160px" << "Apple 64px"
                      << "EmojiOne 64px"
                      << "Facebook 64px" << "Facebook 96px"
                      << "Google 136px" << "Google 64px"
                      << "Messenger 128px" << "Messenger 64px"
                      << "Twitter 64px" << "Twitter 72px";
    QSettings settings;
    m_currentImagesSetIndex = m_imagesSetsNames.indexOf(settings.value("emojisSet", "Unicode").toString()); //Unicode
    if (m_currentImagesSetIndex < 0) {
        m_currentImagesSetIndex = 0;
    }
    qDebug() << "readed emojis set index" << m_currentImagesSetIndex;

    QThread *thread = QThread::create([&]{
        parseSlackJson();
        checkImagesPresence();
        qDebug() << "database loaded";
    });
    thread->start();
    connect(this, &ImagesCache::requestImageViaHttp, this, &ImagesCache::onImageRequestedViaHttp,
            Qt::QueuedConnection);
}

ImagesCache *ImagesCache::instance()
{
    static ImagesCache imageCache;
    return &imageCache;
}

ImagesCache::~ImagesCache() {
    QSettings settings;
    settings.setValue("emojisSet", m_imagesSetsNames.at(m_currentImagesSetIndex));
}

bool ImagesCache::isExist(const QString &id)
{
    return m_emojiList.contains(id);
}

bool ImagesCache::isCached(const QString& id) {
    return m_emojiList.value(id)->cached();
}

//suppose to be run in non-gui thread
QImage ImagesCache::image(const QString &id)
{
    QImage image_;
    //qDebug() << "request for image" << id << m_emojiList.contains(id) << m_emojiList.value(id)->cached();
    if (m_emojiList.contains(id)) {
        if (m_emojiList.value(id)->cached()) {
            image_.load(m_cache + QDir::separator() +
                        m_imagesSetsFolders.at(m_currentImagesSetIndex) + QDir::separator() +
                        m_emojiList.value(id)->image());
        } else {
            emit requestImageViaHttp(id);
        }
    } else {
        image_.load(QStringLiteral("://icons/smile.gif"));
    }
    return image_;
}

void ImagesCache::onImageRequestedViaHttp(const QString &id)
{
    // doesnt makes sense for unicode
    if (isUnicode()) {
        return;
    }
    QNetworkRequest req_ = QNetworkRequest(QUrl("https://github.com/iamcal/emoji-data/raw/master/"
                                                + m_imagesSetsFolders.at(m_currentImagesSetIndex)
                                                + "/" + m_emojiList.value(id)->image()));

    //qDebug() << "reqesting image" << req_.url();
    req_.setAttribute(QNetworkRequest::User, QVariant(id));
    req_.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply* reply = m_qnam.get(req_);
    m_requestsListMutex.lock();
    m_activeRequests.append(reply);
    m_requestsListMutex.unlock();
    connect(reply, &QNetworkReply::finished, this, &ImagesCache::onImageRequestFinished);
}

void ImagesCache::parseSlackJson()
{
    QFile emojiFile(":/data/emojis-slack.json");
    if (!emojiFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open emojis-slack file for reading";
        return;
    }
    const QJsonArray emojisArray = QJsonDocument::fromJson(emojiFile.readAll()).array();
    if (emojisArray.isEmpty()) {
        qWarning() << "Failed to parse emojis file";
        return;
    }

    for (const QJsonValue& value: emojisArray) {
        QJsonObject obj = value.toObject();
        if (!obj.isEmpty()) {
            EmojiInfo* einfo = new EmojiInfo;
            einfo->m_name = obj.value("name").toString();
            QStringList s_ = obj.value("unified").toString().split("-");

            //parse unified unicode to unicode chars sequences, representing different symbols
            for(const QString& s: s_) {
                quint32 i_unicode = s.toUInt(nullptr, 16);
                einfo->m_unified += QString::fromUcs4(&i_unicode, 1);
            }

            einfo->m_nonqualified = obj.value("non_qualified").toString();
            einfo->m_image = obj.value("image").toString();
            einfo->m_shortNames << obj.value("short_name").toString();
            for (const QJsonValue& snvalue: obj.value("short_names").toArray()) {
                const QString sn = snvalue.toString();
                if (!einfo->m_shortNames.contains(sn)) {
                    einfo->m_shortNames << sn;
                }
            }
            einfo->m_text = obj.value("text").toString();
            for (const QJsonValue& tvalue: obj.value("texts").toArray()) {
                const QString txt = tvalue.toString();
                if (!einfo->m_texts.contains(txt)) {
                    einfo->m_texts << txt;
                }
            }
            einfo->m_category = obj.value("category").toString();
            einfo->m_sortOrder = obj.value("sort_order").toInt();
            if (obj.value("has_img_apple").toBool()) {
                einfo->m_imagesExist |= EmojiInfo::ImageApple;
            }
            if (obj.value("has_img_google").toBool()) {
                einfo->m_imagesExist |= EmojiInfo::ImageGoogle;
            }
            if (obj.value("has_img_twitter").toBool()) {
                einfo->m_imagesExist |= EmojiInfo::ImageTwitter;
            }
            if (obj.value("has_img_emojione").toBool()) {
                einfo->m_imagesExist |= EmojiInfo::ImageEmojione;
            }
            if (obj.value("has_img_facebook").toBool()) {
                einfo->m_imagesExist |= EmojiInfo::ImageFacebook;
            }
            if (obj.value("has_img_messenger").toBool()) {
                einfo->m_imagesExist |= EmojiInfo::ImageMessenger;
            }

            m_emojiCategories.insert(einfo->m_category, einfo);
            m_emojiList[einfo->m_shortNames.at(0)] = einfo;
        }
    }
    qDebug() << "readed" << m_emojiList.count() << "emoji icons";
    QMetaObject::invokeMethod(this, "emojiReaded", Qt::QueuedConnection);
}

void ImagesCache::setEmojiImagesSet(const QString& setName)
{
    m_currentImagesSetIndex = m_imagesSetsNames.indexOf(setName);
    checkImagesPresence();
    m_requestsListMutex.lock();
    for(QNetworkReply* reply: m_activeRequests) {
        reply->abort();
    }
    m_requestsListMutex.unlock();
    emit emojisSetsIndexChanged(m_currentImagesSetIndex);
    emit isUnicodeChanged(isUnicode());
    qDebug() << "image set index" << m_currentImagesSetIndex;
}

QString ImagesCache::getEmojiImagesSet()
{
    return m_imagesSetsNames.at(m_currentImagesSetIndex);
}

int ImagesCache::getEmojiImagesSetIndex()
{
    return m_currentImagesSetIndex;
}

QStringList ImagesCache::getEmojiImagesSetsNames()
{
    return m_imagesSetsNames;
}

void ImagesCache::onImageRequestFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    m_requestsListMutex.lock();
    m_activeRequests.removeOne(reply);
    m_requestsListMutex.unlock();
    if(reply->error() == QNetworkReply::NoError){
        const QByteArray &arr = reply->readAll();
        const QString& id = reply->request().attribute(QNetworkRequest::User).toString();
        if (!id.isEmpty()) {
            QFile f(m_cache + QDir::separator() +
                    m_imagesSetsFolders.at(m_currentImagesSetIndex) + QDir::separator() +
                    m_emojiList.value(id)->image());
            f.open(QIODevice::WriteOnly);
            f.write(arr);
            f.close();
            m_emojiList.value(id)->setCached(true);
            emit imageLoaded(id);
        } else {
            qWarning() << "id is empty";
        }
        //qDebug() << "readed" << arr;
    } else {
        qDebug() << "Error request images data" << reply->error() << reply->url();
    }
}

void ImagesCache::checkImagesPresence()
{
    // doesnt makes sense for unicode
    if (isUnicode()) {
        return;
    }
    QDir imagesCacheDir(m_cache + QDir::separator() + m_imagesSetsFolders.at(m_currentImagesSetIndex));
    if (!imagesCacheDir.exists()) {
        imagesCacheDir.mkpath(imagesCacheDir.path());
        return;
    }
    const QList<EmojiInfo *> vals = m_emojiList.values();
    for (const QFileInfo& fi: imagesCacheDir.entryInfoList()) {
        for (EmojiInfo *ei: vals) {
            if (ei->image() == fi.fileName() && fi.size() > 0) {
                ei->setCached(true);
                break;
            }
        }
    }
}

//QML model data
QStringList ImagesCache::getEmojiCategories()
{
    return m_emojiCategories.uniqueKeys();
}

QVariant ImagesCache::getEmojisByCategory(const QString &category)
{
    //QML uderstands only list of QObject's
    QList<QObject*> dataList;
    for (QObject *o : m_emojiCategories.values(category)) {
        dataList.append(o);
    }
    return QVariant::fromValue(dataList);
}

bool ImagesCache::isUnicode() const
{
    return (m_currentImagesSetIndex == 0);
}
