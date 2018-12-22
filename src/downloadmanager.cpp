#include "downloadmanager.h"
#include <QDebug>

DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent)
{
}

void DownloadManager::append(const QUrl &url, const QString &where, const QString &token)
{
    if (downloadQueue.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(startNextDownload()));
    }

    DownloadRequest dreq;
    dreq.what = url;
    dreq.where = where;
    dreq.token = token;

    downloadQueue.enqueue(dreq);
    ++totalCount;
}

void DownloadManager::clearBuffer(const QUrl &url)
{
    m_downloadedBuffers[url].clear();
}

QByteArray DownloadManager::buffer(const QUrl &url)
{
    return m_downloadedBuffers.value(url);
}

QString DownloadManager::saveFileName(const QUrl &url)
{
    QString path = url.path();
    QFileInfo fi = QFileInfo(path);
    QString basename = fi.fileName();

    if (basename.isEmpty()) {
        basename = QLatin1String("download");
    }

    if (QFile::exists(basename)) {
        // already exists, don't overwrite
        int i = 0;
        basename += '.';
        while (QFile::exists(basename + QString::number(i))) {
            ++i;
        }

        basename += QString::number(i);
    }


    return fi.absolutePath() + QDir::separator() + basename;
}

void DownloadManager::startNextDownload()
{
    if (downloadQueue.isEmpty()) {
        qDebug("%d/%d files downloaded successfully", downloadedCount, totalCount);
        emit allFinished();
        return;
    }

    currentRequest = downloadQueue.dequeue();

    if (currentRequest.where == "buffer") {
        m_downloadedBuffers[currentRequest.what] = QByteArray();
    } else if (currentRequest.where == "temp") {
        QTemporaryFile *tempFile = new QTemporaryFile();
        if (tempFile == nullptr || !tempFile->open()) {
            qWarning("Problem opening temporary save file for download '%s': %s",
                     currentRequest.what.toEncoded().constData(),
                     qPrintable(output.errorString()));

            startNextDownload();
            return;                 // skip this download
        }
        m_temporaryDownloads[currentRequest.what] = tempFile;
    } else {
        QString filename = saveFileName(currentRequest.where);
        output.setFileName(filename);
        if (!output.open(QIODevice::WriteOnly)) {
            qWarning("Problem opening save file '%s' for download '%s': %s",
                     qPrintable(filename), currentRequest.what.toEncoded().constData(),
                     qPrintable(output.errorString()));

            startNextDownload();
            return;                 // skip this download
        }
    }

    QNetworkRequest request(currentRequest.what);
    request.setRawHeader(QString("Authorization").toUtf8(), QString("Bearer " + currentRequest.token).toUtf8());
    currentDownload = manager.get(request);
    connect(currentDownload, SIGNAL(downloadProgress(qint64,qint64)),
            SLOT(downloadProgress(qint64,qint64)));
    connect(currentDownload, SIGNAL(finished()),
            SLOT(downloadFinished()));
    connect(currentDownload, SIGNAL(readyRead()),
            SLOT(downloadReadyRead()));

    // prepare the output
    qDebug("Downloading %s...", currentRequest.what.toEncoded().constData());
    downloadTime.start();
}

void DownloadManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloaded(currentRequest.what, 1.0 / ((double)bytesTotal/(double)bytesReceived));
}

void DownloadManager::downloadFinished()
{
    emit downloaded(currentRequest.what, 1.0);
    QString _fileName;
    if (currentRequest.where != QLatin1String("buffer")) {
        if (currentRequest.where == QLatin1String("temp")) {
            QTemporaryFile *tf = m_temporaryDownloads.value(currentRequest.what);
            if (tf != nullptr) {
                _fileName = tf->fileName();
                tf->close();
            }
        } else {
            _fileName = output.fileName();
            output.close();
        }
    }

    if (currentDownload->error() || isHttpRedirect()) {
        // download failed
        qWarning("Failed: %s\n", qPrintable(currentDownload->errorString()));
        if (currentRequest.where == QLatin1String("buffer")) {
            m_downloadedBuffers[currentRequest.what].clear();
        } else if (currentRequest.where == QLatin1String("temp")) {
            QTemporaryFile *tf = m_temporaryDownloads.value(currentRequest.what);
            delete tf;
            m_temporaryDownloads.remove(currentRequest.what);
        } else {
            output.remove();
        }
    } else {
        qDebug() << "Succeeded.";
        emit finished(currentRequest.what, _fileName, currentDownload->error());
        ++downloadedCount;
    }

    currentDownload->deleteLater();
    startNextDownload();
}

void DownloadManager::downloadReadyRead()
{
    if (currentRequest.where == QLatin1String("buffer")) {
        m_downloadedBuffers[currentRequest.what].append(currentDownload->readAll());
    }  else if (currentRequest.where == QLatin1String("temp")) {
        QTemporaryFile *tf = m_temporaryDownloads.value(currentRequest.what);
        if (tf != nullptr) {
            tf->write( currentDownload->readAll());
        }
    } else {
        output.write(currentDownload->readAll());
    }
}

bool DownloadManager::isHttpRedirect() const
{
    int statusCode = currentDownload->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return statusCode == 301 || statusCode == 302 || statusCode == 303
           || statusCode == 305 || statusCode == 307 || statusCode == 308;
}

void DownloadManager::reportRedirect()
{
    int statusCode = currentDownload->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QUrl requestUrl = currentDownload->request().url();

    QVariant target = currentDownload->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!target.isValid()) {
        return;
    }
    QUrl redirectUrl = target.toUrl();
    if (redirectUrl.isRelative()) {
        redirectUrl = requestUrl.resolved(redirectUrl);
    }
    qDebug() << "Redirected to: " << redirectUrl.toDisplayString();
}
