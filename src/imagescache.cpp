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
    _cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/images";
    if (!QDir().mkpath(_cache)) {
        qWarning() << "Cant create images cache folder" << _cache;
    }
    _imagesJsonFileName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/emojiimages.json";
    //check if images json database exist
    if (!QFile(_imagesJsonFileName).exists()) {
        qDebug() << "requesting data from SlackMojis";
        requestSlackMojis();
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
    return _images.contains(id);
}

void ImagesCache::loadImagesDatabase()
{
    QFile f(_imagesJsonFileName);
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray& a = f.readAll();
        parseJson(a);
        f.close();
    } else {
        qWarning() << "error opening custom emoji images DB";
    }
}

bool ImagesCache::isCached(const QString& id) {
    return _images.value(id).cached;
}

//suppose to be run in non-gui thread
QImage ImagesCache::image(const QString &id)
{
    QImage image_;
    //qDebug() << "request for image" << id << _images.contains(id) << _images.value(id).cached;
    if (_images.contains(id)) {
        if (_images.value(id).cached) {
            image_.load(_cache + QDir::separator() + _images.value(id).fileName);
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
    QNetworkRequest req_ = QNetworkRequest(QUrl(_images.value(id).url));
    req_.setAttribute(QNetworkRequest::User, QVariant(id));
    QNetworkReply* reply = _qnam.get(req_);
    connect(reply, &QNetworkReply::finished, this, &ImagesCache::onImageRequestFinished);
}

bool ImagesCache::parseSlackMojis(const QByteArray &data)
{
    auto doc = QGumboDocument::parse(data.data());
    auto root = doc.rootNode();
    auto nodes = root.getElementsByClassName("group");
    //    qDebug() << "groups" << nodes.size();

    for (const auto& node: nodes) {
        QString title_ = node.getElementsByClassName("title").at(0).innerText().trimmed();
        //qDebug() << "title" << title_;
        auto emojis = node.getElementsByClassName("emojis").front().children();
        //qDebug() << "emojis" << emojis.size();
        for (const auto& emoji: emojis) {
            auto aclass = emoji.getElementsByTagName(HtmlTag::A).front();
            ImageInfo info_;
            info_.pack = title_;
            info_.name = emoji.getAttribute("title").trimmed();
            QString url_ = aclass.getAttribute("href").trimmed();
            if (!url_.startsWith("http")) {
                url_.prepend("https://slackmojis.com");
            }
            info_.url = QUrl(url_);
            info_.fileName = aclass.getAttribute("download").trimmed();
            info_.cached = false; //to check later
            _images[info_.name] = info_;
            //qDebug() << "adding emoji:" << info_.pack << info_.name << info_.url << info_.fileName;
        }

    }
    return true;
}

void ImagesCache::requestSlackMojis()
{
    QNetworkReply *reply = _qnam.get(QNetworkRequest(QUrl("https://slackmojis.com/")));
    connect(reply, &QNetworkReply::finished, this, &ImagesCache::onImagesListRequestFinished);
}

void ImagesCache::parseJson(const QByteArray &data) {

    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject obj = jsonDoc.object();

    qDebug() << "Images DB version" << obj.value("version").toString();
    _lastUpdate = QDateTime::fromString(obj.value("updated").toString(), "yyyy-MM-ddThh:mm:ss.zzzZ");
    QJsonArray packsList = obj.value("packs").toArray();

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
                    _images[info_.name] = info_;
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
    foreach(ImageInfo info, _images) {
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
    QFile f(_imagesJsonFileName);
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
    if(reply->error() == QNetworkReply::NoError){
        //qDebug() << "readed" << arr;
        parseSlackMojis(arr);
        saveJson();
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
            QFile f(_cache + QDir::separator() + _images.value(id).fileName);
            f.open(QIODevice::WriteOnly);
            f.write(arr);
            f.close();
            _images[id].cached = true;
            emit imageLoaded(id);
        } else {
            qWarning() << "id is empty";
        }
        //qDebug() << "readed" << arr;
    } else {
        qDebug() << "Error request images data" << reply->error() << reply->url();
    }
}

