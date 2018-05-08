#include "imagescache.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QQmlEngine>

#include <QImageReader>
#include <QDomDocument>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QThread>

#include <QtNetwork/QNetworkReply>
#include "qgumbodocument.h"
#include <qgumbonode.h>

ImagesCache::ImagesCache(QObject *parent) : QObject(parent)
{
    m_cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/images";
    if (!QDir().mkpath(m_cache)) {
        qWarning() << "Cant create images cache folder" << m_cache;
    }
    m_imagesJsonFileName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/emojiimages.json";
    //check if images json database exist
    if (!QFile(m_imagesJsonFileName).exists()) {
        qDebug() << "requesting data from SlackMojis";
        //requestSlackMojis();
        requestEmojiCheactSheet();
    }
    QThread *thread = QThread::create([&]{ loadImagesDatabase(); qDebug() << "database loaded";});
    thread->start();
    connect(this, &ImagesCache::requestImageViaHttp, this, &ImagesCache::onImageRequestedViaHttp, Qt::QueuedConnection);
}

ImagesCache *ImagesCache::instance()
{
    static ImagesCache imageCache;
    return &imageCache;
}

bool ImagesCache::isExist(const QString &id)
{
    return m_images.contains(id);
}

void ImagesCache::loadImagesDatabase()
{
    QFile f(m_imagesJsonFileName);
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray& a = f.readAll();
        parseJson(a);
        f.close();
    } else {
        qWarning() << "error opening custom emoji images DB";
    }
}

bool ImagesCache::isCached(const QString& id) {
    return m_images.value(id).cached;
}

//suppose to be run in non-gui thread
QImage ImagesCache::image(const QString &id)
{
    QImage image_;
    //qDebug() << "request for image" << id << _images.contains(id) << _images.value(id).cached;
    if (m_images.contains(id)) {
        if (m_images.value(id).cached) {
            image_.load(m_cache + QDir::separator() + m_images.value(id).fileName);
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
    QNetworkRequest req_ = QNetworkRequest(QUrl(m_images.value(id).url));
    req_.setAttribute(QNetworkRequest::User, QVariant(id));
    QNetworkReply* reply = m_qnam.get(req_);
    connect(reply, &QNetworkReply::finished, this, &ImagesCache::onImageRequestFinished);
}

bool ImagesCache::parseSlackMojis(const QByteArray &data)
{
    QGumboDocument doc = QGumboDocument::parse(data.data());
    QGumboNode root = doc.rootNode();
    QGumboNodes nodes = root.getElementsByClassName(QStringLiteral("group"));
    //    qDebug() << "groups" << nodes.size();

    for (const QGumboNode& node: nodes) {
        QString title_ = node.getElementsByClassName(QStringLiteral("title")).at(0).innerText().trimmed();
        //qDebug() << "title" << title_;
        QGumboNodes emojis = node.getElementsByClassName(QStringLiteral("emojis")).front().children();
        //qDebug() << "emojis" << emojis.size();
        for (const auto& emoji: emojis) {
            QGumboNode aclass = emoji.getElementsByTagName(HtmlTag::A).front();
            ImageInfo info_;
            info_.pack = title_;
            info_.name = emoji.getAttribute(QStringLiteral("title")).trimmed();
            QString url_ = aclass.getAttribute(QStringLiteral("href")).trimmed();
            if (!url_.startsWith(QStringLiteral("http"))) {
                url_.prepend("https://slackmojis.com");
            }
            info_.url = QUrl(url_);
            info_.fileName = aclass.getAttribute(QStringLiteral("download")).trimmed();
            info_.cached = false; //to check later
            m_images[info_.name] = info_;
            //qDebug() << "adding emoji:" << info_.pack << info_.name << info_.url << info_.fileName;
        }

    }
    return true;
}

bool ImagesCache::parseEmojiCheatSheet(const QByteArray &data)
{
    auto doc = QGumboDocument::parse(data.data());
    QGumboNode root = doc.rootNode();
    QGumboNodes nodes = root.getElementsByTagName(HtmlTag::UL);
    //qDebug() << "emojis" << nodes.size();

    for (const QGumboNode& node: nodes) {
        const QStringList & list_ = node.classList();
        if (list_.isEmpty()) {
            continue;
        }
        QString title_ = node.classList().at(0).trimmed();
        //qDebug() << "title" << title_;
        QGumboNodes emojis = node.getElementsByClassName(QStringLiteral("emoji"));
        QGumboNodes emojisnames = node.getElementsByClassName(QStringLiteral("name"));
        if (emojis.size() != emojisnames.size()) {
            qDebug() << "emojis parse something wrong" << emojis.size() << emojisnames.size();
            continue;
        }
        for (ulong i = 0; i < emojis.size(); i++) {
            const QGumboNode& emoji = emojis.at(i);
            const QGumboNode& name = emojisnames.at(i);

            ImageInfo info_;
            info_.pack = title_;
            info_.name = name.innerText().trimmed();
            QString url_ = emoji.getAttribute(QStringLiteral("data-src")).trimmed();
            if (!url_.startsWith(QStringLiteral("http"))) {
                url_.prepend("https://www.webpagefx.com/tools/emoji-cheat-sheet/");
            }
            info_.url = QUrl(url_);
            info_.fileName = info_.url.fileName();
            info_.cached = false; //to check later
            m_images[info_.name] = info_;
            //qDebug() << "adding emoji:" << info_.pack << info_.name << info_.url << info_.fileName;
        }
    }
    return (m_images.size() > 0);
}

void ImagesCache::requestSlackMojis()
{
    QNetworkReply *reply = m_qnam.get(QNetworkRequest(QUrl(QStringLiteral("https://slackmojis.com/"))));
    connect(reply, &QNetworkReply::finished, this, &ImagesCache::onImagesListRequestFinished);
}

void ImagesCache::requestEmojiCheactSheet()
{
    QNetworkReply *reply = m_qnam.get(QNetworkRequest(QUrl(QStringLiteral("https://www.webpagefx.com/tools/emoji-cheat-sheet/"))));
    connect(reply, &QNetworkReply::finished, this, &ImagesCache::onImagesListRequestFinished);
}

void ImagesCache::parseJson(const QByteArray &data) {

    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject obj = jsonDoc.object();

    qDebug() << "Images DB version" << obj.value("version").toString();
    m_lastUpdate = QDateTime::fromString(obj.value(QStringLiteral("updated")).toString(), "yyyy-MM-ddThh:mm:ss.zzzZ");
    QJsonArray packsList = obj.value(QStringLiteral("packs")).toArray();

    foreach (const QJsonValue& value, packsList) {
        QJsonObject obj1 = value.toObject();
        if (!obj1.isEmpty()) {
            QString packname = obj1.value("name").toString();

            QJsonArray array = obj1.value("images").toArray();
            foreach (const QJsonValue &ivalue, array) {
                const QJsonObject &imageObj = ivalue.toObject();
                if (!imageObj.isEmpty()) {
                    ImageInfo info_;
                    info_.pack = packname;
                    info_.name = imageObj.value("name").toString();
                    info_.url = QUrl(imageObj.value("url").toString());
                    info_.fileName = imageObj.value("filename").toString();
                    info_.cached = false; //to check later
                    m_images[info_.name] = info_;
                    //qDebug() << "appended" << iData.name << iData.url << iData.category;
                }
            }
        }
    }
}

void ImagesCache::saveJson()
{
    QJsonObject o;
    o.insert("version", QJsonValue("1.0"));
    o.insert("updated", QJsonValue(QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss.zzzZ")));

    QJsonArray packs;

    QMap<QString, QJsonArray> packsMap;
    foreach(ImageInfo info, m_images) {
        QJsonObject imgo;
        imgo.insert("name", info.name);
        imgo.insert("filename", info.fileName);
        imgo.insert("url", info.url.toString());
        packsMap[info.pack].append(imgo);
    }

    foreach(QString pack, packsMap.uniqueKeys()) {
        QJsonObject packo;
        packo.insert("name", pack);
        packo.insert("images", packsMap.value(pack));
        packs.append(packo);
    }

    o["packs"] = packs;

    QJsonDocument jdoc = QJsonDocument(o);
    QByteArray ba = jdoc.toJson(QJsonDocument::Compact);
    QFile f(m_imagesJsonFileName);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(ba);
        f.close();
    } else {
        qWarning() << "error opening custom images DB for saving";
    }
}

void ImagesCache::onImagesListRequestFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    const QByteArray &arr = reply->readAll();
    bool parsed_ = false;
    if(reply->error() == QNetworkReply::NoError){
        //qDebug() << "readed" << arr;
        if (reply->url().host().contains(QStringLiteral("slackmojis.com"))) {
            parsed_ = parseSlackMojis(arr);
        } else {
            parsed_ = parseEmojiCheatSheet(arr);
        }
        if (parsed_) {
            saveJson();
        } else {
            qDebug() << "Nothing gets parsed";
        }
    } else {
        qDebug() << "Error request images data" << reply->error() << reply->url();
        qDebug() << "error content: " << arr;
    }
}

void ImagesCache::onImageRequestFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if(reply->error() == QNetworkReply::NoError){
        const QByteArray &arr = reply->readAll();
        QString id = reply->request().attribute(QNetworkRequest::User).toString();
        if (!id.isEmpty()) {
            QFile f(m_cache + QDir::separator() + m_images.value(id).fileName);
            f.open(QIODevice::WriteOnly);
            f.write(arr);
            f.close();
            m_images[id].cached = true;
            emit imageLoaded(id);
        } else {
            qWarning() << "id is empty";
        }
        //qDebug() << "readed" << arr;
    } else {
        qDebug() << "Error request images data" << reply->error() << reply->url();
    }
}

