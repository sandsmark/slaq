#pragma once

#include <QQmlNetworkAccessManagerFactory>
#include <QtNetwork/QNetworkAccessManager>

class NetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    virtual QNetworkAccessManager *create(QObject *parent);
};

