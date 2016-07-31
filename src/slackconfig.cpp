#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "slackconfig.h"

SlackConfig::SlackConfig(QObject *parent) : QObject(parent), settings(this), currentUserId() {
}

QString SlackConfig::accessToken() {
    return settings.value("user/accessToken").toString();
}

void SlackConfig::setAccessToken(QString accessToken) {
    settings.setValue("user/accessToken", QVariant(accessToken));
}

QString SlackConfig::userId() {
    return currentUserId;
}

void SlackConfig::setUserId(QString userId) {
    currentUserId = userId;
}

void SlackConfig::clearWebViewCache() {
    QStringList dataPaths = QStandardPaths::standardLocations(QStandardPaths::DataLocation);

    if (dataPaths.size()) {
        QDir webData(QDir(dataPaths.at(0)).filePath(".QtWebKit"));
        if (webData.exists()) {
            webData.removeRecursively();
        }
    }
}
