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
    //need for quick access
    m_categoriesSymbols << "🏈" << "🐶"<< "🏴" << "🥂" << "🔦" << "🏿" << "🙂" << "©️" << "✈️";
    QSettings settings;
    const QString imagesSet = settings.value("emojisSet", "Unicode").toString();
    setEmojiImagesSet(imagesSet);
    qDebug() << "readed emojis set index" << m_currentImagesSetIndex;

    connect(this, &ImagesCache::requestImageViaHttp, this, &ImagesCache::onImageRequestedViaHttp,
            Qt::QueuedConnection);
}

ImagesCache *ImagesCache::instance()
{
    static ImagesCache imageCache;
    return &imageCache;
}

ImagesCache::~ImagesCache() {
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
    if (id.startsWith(QStringLiteral("icon/"))) {
        //icons ids started with icon/following by url:
        // icon/https://slack-files2.s3-us-west-2.amazonaws.com/avatars/2017-10-19/259793453543_4b3c2e1f2d0926ea6415_original.png
        QString iconPath = id;
        iconPath.remove(0, 5);
        QUrl iconUrl(iconPath);

        if (m_iconsCached.contains(iconUrl.fileName())) {
            image_.load(m_cache + QDir::separator() + QStringLiteral("icons")
                        + QDir::separator() + iconUrl.fileName());
        } else {
            emit requestImageViaHttp(id);
        }
    } else if (m_emojiList.contains(id)) {
        //qDebug() << "request for image" << id << m_emojiList.contains(id) << m_emojiList.value(id)->cached();
        if (m_emojiList.value(id)->cached()) {
            EmojiInfo* einfo = m_emojiList.value(id);
            QString _path;
            if (einfo->imagesExist() & EmojiInfo::ImageSlackTeam) {
                _path = m_cache + QDir::separator() + "teams_emojis" +
                        QDir::separator() + QUrl(einfo->image()).fileName();

            } else {
                _path = m_cache + QDir::separator() +
                        m_imagesSetsFolders.at(m_currentImagesSetIndex) + QDir::separator() +
                        m_emojiList.value(id)->image();
            }
            image_.load(_path);
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
    QUrl url;
    if (id.startsWith(QStringLiteral("icon/"))) {
        QString iconPath = id;
        iconPath.remove(0, 5);
        url.setUrl(iconPath);
    } else {
        EmojiInfo* einfo = m_emojiList.value(id);
        if (einfo == nullptr) {
            return;
        }
        if (isUnicode() && !(einfo->imagesExist() & EmojiInfo::ImageSlackTeam)) {
            return;
        }
        if (einfo->imagesExist() & EmojiInfo::ImageSlackTeam) {
            url.setUrl(einfo->image());
        } else {
            url.setUrl("https://github.com/iamcal/emoji-data/raw/master/"
                       + m_imagesSetsFolders.at(m_currentImagesSetIndex)
                       + "/" + einfo->image());
        }
    }
    QNetworkRequest req_ = QNetworkRequest(url);

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

            addEmoji(einfo);
        }
    }
    qDebug() << "readed" << m_emojiList.count() << "emoji icons";
    QMetaObject::invokeMethod(this, "emojiReaded", Qt::QueuedConnection);
}

void ImagesCache::setEmojiImagesSet(const QString& setName)
{
    m_currentImagesSetIndex = m_imagesSetsNames.indexOf(setName);
    if (m_currentImagesSetIndex < 0 || m_currentImagesSetIndex >= m_imagesSetsNames.size()) {
        m_currentImagesSetIndex = 0;
    }
    QSettings settings;
    settings.setValue(QStringLiteral("emojisSet"), m_imagesSetsNames.at(m_currentImagesSetIndex));
    //check asyncronously
    QThread *thread = QThread::create([&]{
        if (m_emojiList.isEmpty()) {
            parseSlackJson();
        }
        checkImagesPresence();
        m_requestsListMutex.lock();
        for(QNetworkReply* reply: m_activeRequests) {
            reply->abort();
        }
        m_requestsListMutex.unlock();
        QMetaObject::invokeMethod(this, "emojisSetsIndexChanged",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, m_currentImagesSetIndex));
        QMetaObject::invokeMethod(this, "isUnicodeChanged",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, isUnicode()));

    });
    thread->start();
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
            QFile f;
            if (id.startsWith(QStringLiteral("icon/"))) {
                QString iconPath = id;
                iconPath.remove(0, 5);
                QUrl url(iconPath);
                const QString& filename = url.fileName();
                f.setFileName(m_cache + QDir::separator() +
                              "icons" + QDir::separator() +
                              filename);
                if (!m_iconsCached.contains(filename)) {
                    m_iconsCached << filename;
                }

            } else {
                EmojiInfo* einfo = m_emojiList.value(id);
                if (einfo == nullptr) {
                    qWarning() << "id is not found" << id;
                    return;
                }

                if (einfo->imagesExist() & EmojiInfo::ImageSlackTeam) {
                    QUrl url(einfo->image());
                    const QString& filename = url.fileName();
                    f.setFileName(m_cache + QDir::separator() +
                                  "teams_emojis" + QDir::separator() +
                                  filename);
                } else {
                    f.setFileName(m_cache + QDir::separator() +
                                  m_imagesSetsFolders.at(m_currentImagesSetIndex) + QDir::separator() +
                                  einfo->image());
                }
                einfo->setCached(true);
            }
            f.open(QIODevice::WriteOnly);
            f.write(arr);
            f.close();

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
    QDir iconsCacheDir(m_cache + QDir::separator() + "icons");
    if (!iconsCacheDir.exists()) {
        iconsCacheDir.mkpath(iconsCacheDir.path());
    }
    QDir teamsEmojisCacheDir(m_cache + QDir::separator() + "teams_emojis");
    if (!teamsEmojisCacheDir.exists()) {
        teamsEmojisCacheDir.mkpath(teamsEmojisCacheDir.path());
    }
    //readout icons
    for (const QFileInfo& fi: iconsCacheDir.entryInfoList()) {
        if (fi.isFile() && fi.size() > 0) {
            m_iconsCached << fi.fileName();
        }
    }
    // doesnt makes sense for unicode

    const QList<EmojiInfo *> vals = m_emojiList.values();
    for (EmojiInfo *ei: vals) {
        ei->setCached(false);
    }
    QDir imagesCacheDir(m_cache + QDir::separator() + m_imagesSetsFolders.at(m_currentImagesSetIndex));
    if (!imagesCacheDir.exists()) {
        imagesCacheDir.mkpath(imagesCacheDir.path());
        return;
    }

    for (EmojiInfo *ei: vals) {
        for (const QFileInfo& fi: imagesCacheDir.entryInfoList()) {
            if (ei->image() == fi.fileName() && fi.size() > 0) {
                ei->setCached(true);
                break;
            }
        }
    }
}

QStringList ImagesCache::getCategoriesSymbols() const
{
    return m_categoriesSymbols;
}

void ImagesCache::addEmoji(EmojiInfo *einfo, bool visibleCategory)
{
    if (visibleCategory) {
        m_emojiCategories.insert(einfo->m_category, einfo);
    }
    for (const QString& key : einfo->m_shortNames) {
        m_emojiList[key] = einfo;
    }
}

void ImagesCache::sendEmojisUpdated()
{
    QMetaObject::invokeMethod(this, "emojisUpdated", Qt::QueuedConnection);
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

QString ImagesCache::getEmojiByName(const QString &name) const
{
    EmojiInfo *ei = m_emojiList.value(name, nullptr);
    if (ei) {
        return ei->unified();
    }
    return "";
}

QString ImagesCache::getNameByEmoji(const QString &emoji) const
{
    const QList<EmojiInfo*>& eiList = m_emojiList.values();
    for (EmojiInfo* ei : eiList) {
        if (ei->unified() == emoji) {
            return ei->shortNames().at(0);
        }
    }
    return "";
}
