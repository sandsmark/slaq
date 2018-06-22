#include "networkaccessmanager.h"

NetworkAccessManager::NetworkAccessManager(QObject *parent) :
    QNetworkAccessManager(parent)
{
    config = SlackConfig::instance();
}

QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    const QUrl& url = request.url();
    if (url.scheme() == "team") {

        QNetworkRequest copy(request);
        copy.setUrl(QUrl(url.path().remove(0, 1)));

        QString token = config->accessToken(url.host().toUpper());
        if (!token.isEmpty()) {
            copy.setRawHeader(QString("Authorization").toUtf8(), QString("Bearer " + token).toUtf8());
        } else {
            qWarning() << "token for team" << url.host() << "not found";
        }

        return QNetworkAccessManager::createRequest(op, copy, outgoingData);
    } else {
        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    }
}
