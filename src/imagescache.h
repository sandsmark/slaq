#ifndef IMAGESCACHE_H
#define IMAGESCACHE_H

#include <QObject>
#include <QUrl>
#include <QMap>
#include <QString>
#include <QDateTime>
#include <QtNetwork/QNetworkAccessManager>

class ImagesCache : public QObject
{
    Q_OBJECT
public:
    explicit ImagesCache(QObject *parent = nullptr);
    virtual ~ImagesCache() {}

    struct ImageInfo {
        QString pack;
        QString name;
        QUrl url;
        bool cached;
    };

signals:

public slots:

private slots:
    void onImagesRequestFinished();
private:
    void loadImagesDatabase();
    void parseJson(const QByteArray &data);
    bool parseSlackMojis(const QByteArray &data);
    void requestSlackMojis();
    void saveJson();

private:
    QMap<QString, ImageInfo> _images;
    QDateTime _lastUpdate;
    QNetworkAccessManager _qnam;

    QString _cache;
    QString _imagesJsonFileName;
};

#endif // IMAGESCACHE_H
