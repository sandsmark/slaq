#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QtNetwork>
#include <QtCore>
#include <QHash>

class DownloadManager: public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);

    Q_INVOKABLE void append(const QUrl &url, const QString& where, const QString& token);
    Q_INVOKABLE void clearBuffer(const QUrl &url);
    Q_INVOKABLE QByteArray buffer(const QUrl &url);

    struct DownloadRequest {
        QUrl what;
        QString where;
        QString token;
    };

signals:
    void allFinished();
    void finished(const QUrl& url, QNetworkReply::NetworkError error);
    void downloaded(const QUrl& url, qreal progress);

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
    DownloadRequest currentRequest;
    QHash<QUrl, QByteArray> m_downloadedBuffers;

    int downloadedCount = 0;
    int totalCount = 0;
};

#endif
