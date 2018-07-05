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
#include <QMutableListIterator>

ImagesCache::ImagesCache(QObject *parent) : QObject(parent)
{
    m_cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/images";
    QDir iconsCacheDir(m_cache + QDir::separator() + "icons");
    if (!iconsCacheDir.exists()) {
        iconsCacheDir.mkpath(iconsCacheDir.path());
    }
    QDir teamsEmojisCacheDir(m_cache + QDir::separator() + "teams_emojis");
    if (!teamsEmojisCacheDir.exists()) {
        teamsEmojisCacheDir.mkpath(teamsEmojisCacheDir.path());
    }

    m_imagesSetsFolders << QStringLiteral("unicode")
                        << QStringLiteral("img-apple-160") << QStringLiteral("img-apple-64")
                        << QStringLiteral("img-emojione-64")
                       << QStringLiteral("img-facebook-64") << QStringLiteral("img-facebook-96")
                       << QStringLiteral("img-google-136") << QStringLiteral("img-google-64")
                       << QStringLiteral("img-messenger-128") << QStringLiteral("img-messenger-64")
                       << QStringLiteral("img-twitter-64") << QStringLiteral("img-twitter-72");
    m_imagesSetsNames << QStringLiteral("Unicode")
                      << QStringLiteral("Apple 160px") << QStringLiteral("Apple 64px")
                      << QStringLiteral("EmojiOne 64px")
                      << QStringLiteral("Facebook 64px") << QStringLiteral("Facebook 96px")
                      << QStringLiteral("Google 136px") << QStringLiteral("Google 64px")
                      << QStringLiteral("Messenger 128px") << QStringLiteral("Messenger 64px")
                      << QStringLiteral("Twitter 64px") << QStringLiteral("Twitter 72px");
    // emoji categories names taken from
    // https://raw.githubusercontent.com/iamcal/emoji-data/master/categories.json
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategoryPeople,
                                                           QStringLiteral("🙂"), tr("People"),
                                                           QStringLiteral("Smileys & People"), true ));
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategoryNature,
                                                           QStringLiteral("🐶"), tr("Nature"),
                                                           QStringLiteral("Animals & Nature"), true ));
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategoryFoodAndDrink,
                                                           QStringLiteral("🥂"), tr("Food & Drink"),
                                                           QStringLiteral("Food & Drink"), true ));
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategoryActivity,
                                                           QStringLiteral("🏈"), tr("Activity"),
                                                           QStringLiteral("Activities"), true ));
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategoryTravelAndPlaces,
                                                           QStringLiteral("✈️"), tr("Travel & Places"),
                                                           QStringLiteral("Travel & Places"), true ));
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategoryObjects,
                                                           QStringLiteral("🔦"), tr("Objects"),
                                                           QStringLiteral("Objects"), true ));
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategorySymbols,
                                                           QStringLiteral("©️"), tr("Symbols"),
                                                           QStringLiteral("Symbols"), true ));
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategoryFlags,
                                                           QStringLiteral("🏴"), tr("Flags"),
                                                           QStringLiteral("Flags"), true ));
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategorySkinTone,
                                                           QStringLiteral(""), tr(""),
                                                           QStringLiteral("Skin Tones"), false ));
    m_EmojiCategoriesModel.append(new EmojiCategoryHolder( EmojiInfo::EmojiCategoryCustom,
                                                           QStringLiteral("♻️"), tr("Custom"),
                                                           QStringLiteral(""), true ));
    QSettings settings;
    const QString imagesSet = settings.value(QStringLiteral("emojisSet"), "Unicode").toString();
    QThread *thread = QThread::create([&]{
        parseSlackJson();
    });
    thread->start();
    setEmojiImagesSet(imagesSet);
    QDir emojisCacheDir(m_cache + QDir::separator() + m_imagesSetsFolders.at(m_currentImagesSetIndex));
    if (!emojisCacheDir.exists()) {
        emojisCacheDir.mkpath(emojisCacheDir.path());
    }

    qDebug() << "readed emojis set index" << m_currentImagesSetIndex;

    connect(this, &ImagesCache::requestImageViaHttp, this, &ImagesCache::onImageRequestedViaHttp,
            Qt::QueuedConnection);
}

QQmlObjectListModel<EmojiCategoryHolder>* ImagesCache::emojiCategoriesModel()
{
    return &m_EmojiCategoriesModel;
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
    //qDebug() << "request for image id" << id;
    QImage image_;
    QString path_;
    bool cached_ = false;
    EmojiInfo* einfo = nullptr;

    if (id.startsWith(QStringLiteral("icon/"))) {
        //icons ids started with icon/following by url:
        // icon/https://slack-files2.s3-us-west-2.amazonaws.com/avatars/2017-10-19/259793453543_4b3c2e1f2d0926ea6415_original.png
        QString iconPath = id;
        iconPath.remove(0, 5);
        QUrl iconUrl(iconPath);

        path_ = m_cache + QDir::separator() + QStringLiteral("icons")
                + QDir::separator() + iconUrl.fileName();
        cached_ = m_iconsCached.contains(path_);
    } else {
        einfo = m_emojiList.value(id);
        if (einfo != nullptr) {
            //qDebug() << "image is" << id << m_emojiList.contains(id) << m_emojiList.value(id)->cached();
            if (einfo->imagesExist() & EmojiInfo::ImageSlackTeam) {
                path_ = m_cache + QDir::separator() + "teams_emojis" +
                        QDir::separator() + QUrl(einfo->image()).fileName();

            } else {
                path_ = m_cache + QDir::separator() +
                        m_imagesSetsFolders.at(m_currentImagesSetIndex) + QDir::separator() +
                        m_emojiList.value(id)->image();
            }

            cached_ = m_emojiList.value(id)->cached();
        } else {
            qWarning() << "invalid id" << id;
            return image_;
        }
    }

    if (!cached_) {
        cached_ = QFile::exists(path_);
    }

    if (cached_) {
        //qDebug() << "loading image" << path_;
        if (!image_.load(path_)) {
            qWarning() << "Error loading image" << path_;
        }
    } else {
        emit requestImageViaHttp(id);
    }

    if (id.startsWith(QStringLiteral("icon/"))) {
        if (cached_) {
            m_iconsCached.insert(path_);
        }
    } else {
        einfo->setCached(cached_);
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
            einfo->m_name = obj.value(QStringLiteral("name")).toString();
            QStringList s_ = obj.value(QStringLiteral("unified")).toString().split(QStringLiteral("-"));

            //parse unified unicode to unicode chars sequences, representing different symbols
            for(const QString& s: s_) {
                quint32 i_unicode = s.toUInt(nullptr, 16);
                einfo->m_unified += QString::fromUcs4(&i_unicode, 1);
            }

            einfo->m_nonqualified = obj.value(QStringLiteral("non_qualified")).toString();
            einfo->m_image = obj.value(QStringLiteral("image")).toString();
            einfo->m_shortNames << obj.value(QStringLiteral("short_name")).toString();
            foreach (const auto& snvalue, obj.value(QStringLiteral("short_names")).toArray()) {
                const QString sn = snvalue.toString();
                if (!einfo->m_shortNames.contains(sn)) {
                    einfo->m_shortNames << sn;
                }
            }
            einfo->m_text = obj.value(QStringLiteral("text")).toString();
            foreach (const auto& tvalue, obj.value(QStringLiteral("texts")).toArray()) {
                const QString txt = tvalue.toString();
                if (!einfo->m_texts.contains(txt)) {
                    einfo->m_texts << txt;
                }
            }
            const QString& emCat = obj.value(QStringLiteral("category")).toString();

            for (EmojiCategoryHolder* ech : m_EmojiCategoriesModel) {
                if (emCat == ech->dbName()) {
                    einfo->m_category = ech->category();
                    break;
                }
            }
            einfo->m_sortOrder = obj.value(QStringLiteral("sort_order")).toInt();
            if (obj.value(QStringLiteral("has_img_apple")).toBool()) {
                einfo->m_imagesExist.setFlag(EmojiInfo::ImageApple);
            }
            if (obj.value(QStringLiteral("has_img_google")).toBool()) {
                einfo->m_imagesExist.setFlag(EmojiInfo::ImageGoogle);
            }
            if (obj.value(QStringLiteral("has_img_twitter")).toBool()) {
                einfo->m_imagesExist.setFlag(EmojiInfo::ImageTwitter);
            }
            if (obj.value(QStringLiteral("has_img_emojione")).toBool()) {
                einfo->m_imagesExist.setFlag(EmojiInfo::ImageEmojione);
            }
            if (obj.value(QStringLiteral("has_img_facebook")).toBool()) {
                einfo->m_imagesExist.setFlag(EmojiInfo::ImageFacebook);
            }
            if (obj.value(QStringLiteral("has_img_messenger")).toBool()) {
                einfo->m_imagesExist.setFlag(EmojiInfo::ImageMessenger);
            }

            addEmoji(einfo, !(einfo->category().testFlag(EmojiInfo::EmojiCategorySkinTone)));
        }
    }
    qDebug() << "readed" << m_emojiList.count() << "emoji icons";
    QMetaObject::invokeMethod(this, "emojisDatabaseReaded", Qt::QueuedConnection);
}

void ImagesCache::setEmojiImagesSet(const QString& setName)
{
    m_currentImagesSetIndex = m_imagesSetsNames.indexOf(setName);
    if (m_currentImagesSetIndex < 0 || m_currentImagesSetIndex >= m_imagesSetsNames.size()) {
        m_currentImagesSetIndex = 0;
    }
    QSettings settings;
    settings.setValue(QStringLiteral("emojisSet"), m_imagesSetsNames.at(m_currentImagesSetIndex));
    m_requestsListMutex.lock();
    foreach(QNetworkReply* reply, m_activeRequests) {
        reply->abort();
    }
    m_requestsListMutex.unlock();
    QMetaObject::invokeMethod(this, "emojisSetsIndexChanged",
                              Qt::QueuedConnection,
                              Q_ARG(int, m_currentImagesSetIndex));
    QMetaObject::invokeMethod(this, "isUnicodeChanged",
                              Qt::QueuedConnection,
                              Q_ARG(bool, isUnicode()));
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

                if (einfo->imagesExist().testFlag(EmojiInfo::ImageSlackTeam)) {
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
    QThread *thread = QThread::create([&]{
        QDir iconsCacheDir(m_cache + QDir::separator() + "icons");
        if (!iconsCacheDir.exists()) {
            iconsCacheDir.mkpath(iconsCacheDir.path());
        }
        //readout icons
        foreach (const QFileInfo& fi, iconsCacheDir.entryInfoList()) {
            if (fi.isFile() && fi.size() > 0) {
                m_iconsCached << fi.fileName();
            }
        }

        const QList<EmojiInfo *> vals = m_emojiList.values();
        for (EmojiInfo *ei: vals) {
            ei->setCached(false);
        }
        QDir emojisCacheDir(m_cache + QDir::separator() + m_imagesSetsFolders.at(m_currentImagesSetIndex));
        if (!emojisCacheDir.exists()) {
            emojisCacheDir.mkpath(emojisCacheDir.path());
            return;
        }
        QDir teamsEmojisCacheDir(m_cache + QDir::separator() + "teams_emojis");
        if (!teamsEmojisCacheDir.exists()) {
            teamsEmojisCacheDir.mkpath(teamsEmojisCacheDir.path());
        }

        QFileInfoList emojisFiList = emojisCacheDir.entryInfoList();
        emojisFiList.append(teamsEmojisCacheDir.entryInfoList());
        QMutableListIterator<QFileInfo> it(emojisFiList);
        for (EmojiInfo *ei: vals) {
            it.toFront();
            while (it.hasNext()) {
                const QFileInfo& fi = it.next();
                if (ei->image() == fi.fileName() && fi.size() > 0) {
                    ei->setCached(true);
                    it.remove();
                    break;
                }
            }
        }
    });
    thread->start();
}

void ImagesCache::addEmojiAlias(const QString& emojiName, const QString& emojiAlias) {
    EmojiInfo* einfo = m_emojiList.value(emojiAlias);
    if (einfo == nullptr) {
        return;
    }
    einfo->m_shortNames << emojiName;
    m_emojiList[emojiName] = einfo;
}

EmojiInfo* ImagesCache::getEmojiInfo(const QString &name)
{
    return m_emojiList.value(name);
}

void ImagesCache::addEmoji(EmojiInfo *einfo, bool visibleCategory)
{
    if (visibleCategory) {
        m_emojiCategories.insert(einfo->m_category, einfo);
    }
    foreach (const QString& key, einfo->m_shortNames) {
        m_emojiList[key] = einfo;
    }
}

void ImagesCache::sendEmojisUpdated()
{
    QMetaObject::invokeMethod(this, "emojisUpdated", Qt::QueuedConnection);
}

// parameter set as int due to Qt bug:
// https://bugreports.qt.io/browse/QTBUG-58454
QVariant ImagesCache::getEmojisByCategory(int category, const QString &teamId)
{
    //QML uderstands only list of QObject's
    QList<QObject*> dataList;
    foreach (EmojiInfo *einfo, m_emojiCategories.values(static_cast<EmojiInfo::EmojiCategories>(category))) {
        if (einfo->teamId().isEmpty() || einfo->teamId() == teamId) {
            dataList.append(static_cast<QObject*>(einfo));
        }
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
    if (ei != nullptr) {
        return ei->unified();
    }
    return QStringLiteral("");
}

QString ImagesCache::getNameByEmoji(const QString &emoji) const
{
    const QList<EmojiInfo*>& eiList = m_emojiList.values();

    for (EmojiInfo* ei : eiList) {
        const QStringList& _shortnames = ei->shortNames();
        if (_shortnames.isEmpty()) {
            continue;
        }
        if (ei->unified() == emoji || _shortnames.contains(emoji)) {
            return _shortnames.at(0);
        }
    }
    return QStringLiteral("");
}
