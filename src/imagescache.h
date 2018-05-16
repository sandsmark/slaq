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
    bool isImagesDatabaseLoaded() { return m_images.size() > 0; }
    void loadImagesDatabase();

signals:
    void imageLoaded(const QString &id);
    void requestImageViaHttp(const QString &id);

public slots:

private slots:
    void onImagesListRequestFinished();
    void onImageRequestedViaHttp(const QString &id);
    void onImageRequestFinished();
    void requestEmojiCheactSheet();
    void requestSlackMojis();

private:
    explicit ImagesCache(QObject *parent = nullptr);
    void parseJson(const QByteArray &data);
    bool parseSlackMojis(const QByteArray &data);
    bool parseEmojiCheatSheet(const QByteArray &data);
    void saveJson();

private:
    QMap<QString, ImageInfo> m_images;
    QDateTime m_lastUpdate;
    QNetworkAccessManager m_qnam;

    QString m_cache;
    QString m_imagesJsonFileName;
};

#endif // IMAGESCACHE_H
