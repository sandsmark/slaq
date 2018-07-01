#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QtNetwork>
#include <QtCore>

class DownloadManager: public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);

    Q_INVOKABLE void append(const QUrl &url, const QString& where, const QString& token);


    struct DownloadRequest {
        QUrl what;
        QString where;
        QString token;
    };

signals:
    void finished();
    void downloaded(qreal progress);

private slots:
    void startNextDownload();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void downloadReadyRead();

private:
    QString saveFileName(const QUrl &url);
    bool isHttpRedirect() const;
    void reportRedirect();

    QNetworkAccessManager manager;
    QQueue<DownloadRequest> downloadQueue;
    QNetworkReply *currentDownload = nullptr;
    QFile output;
    QTime downloadTime;

    int downloadedCount = 0;
    int totalCount = 0;
};

#endif
