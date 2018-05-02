#ifndef IMAGESCACHE_H
#define IMAGESCACHE_H

#include <QObject>
#include <QUrl>
#include <QMap>
#include <QString>
#include <QImage>
#include <QDateTime>
#include <QtNetwork/QNetworkAccessManager>

class ImagesCache : public QObject
{
    Q_OBJECT
public:

    static ImagesCache* instance();
    virtual ~ImagesCache() {}

    struct ImageInfo {
        ImageInfo(): cached(false) {}
        QString pack;
        QString name;
        QString fileName;
        QUrl url;
        bool cached;
    };

    bool isExist(const QString &id);
    bool isCached(const QString &id);
    QImage image(const QString &id);
    bool isImagesDatabaseLoaded() { return _images.size() > 0; }
    void loadImagesDatabase();

signals:
    void imageLoaded(const QString &id);
    void requestImageViaHttp(const QString &id);

public slots:

private slots:
    void onImagesListRequestFinished();
    void onImageRequestedViaHttp(const QString &id);
    void onImageRequestFinished();
private:
    explicit ImagesCache(QObject *parent = nullptr);
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
