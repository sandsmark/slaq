#include "networkaccessmanager.h"

NetworkAccessManager::NetworkAccessManager(QObject *parent): QNetworkAccessManager(parent) {
    config = new SlackConfig(this);
}

QNetworkReply* NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData) {
    if (request.url().host() == "files.slack.com") {
        QNetworkRequest copy(request);

        QString token = config->accessToken();
        if (!token.isEmpty()) {
            copy.setRawHeader(QString("Authorization").toUtf8(), QString("Bearer " + token).toUtf8());
        }

        return QNetworkAccessManager::createRequest(op, copy, outgoingData);
    }
    else {
        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    }
}
