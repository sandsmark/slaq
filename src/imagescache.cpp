#include "imagescache.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QQmlEngine>

#include <QDomDocument>
#include <QDebug>
#include <QFile>
#include <QStandardPaths>

#include <QtNetwork/QNetworkReply>
#include "qgumbodocument.h"
#include <qgumbonode.h>

ImagesCache::ImagesCache(QObject *parent) : QObject(parent)
{
    _cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    _imagesJsonFileName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/emojiimages.json";
}

void ImagesCache::loadImagesDatabase()
{
    QFile f(_imagesJsonFileName);
    qDebug() << "cache location" << _cache << f.fileName();
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray& a = f.readAll();
        parseJson(a);
        f.close();
    } else {
        qWarning() << "error opening custom emoji images DB";
    }
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
            ImageInfo info_;
            info_.pack = title_;
            info_.name = emoji.getAttribute("title").trimmed();
            info_.url = QUrl(emoji.getElementsByTagName(HtmlTag::A).front().getAttribute("href").trimmed());
            info_.cached = false; //to check later
            _images[info_.name] = info_;
            //qDebug() << "adding emoji:" << info_.pack << info_.name << info_.url;
        }

    }
    return true;
}

void ImagesCache::requestSlackMojis()
{
    QNetworkReply *reply = _qnam.get(QNetworkRequest(QUrl("https://slackmojis.com/")));
    connect(reply, &QNetworkReply::finished, this, &ImagesCache::onImagesRequestFinished);
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

void ImagesCache::onImagesRequestFinished()
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

