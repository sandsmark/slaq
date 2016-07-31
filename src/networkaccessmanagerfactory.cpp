#include "networkaccessmanagerfactory.h"

#include <QDebug>

#include "networkaccessmanager.h"

QNetworkAccessManager *NetworkAccessManagerFactory::create(QObject *parent)
{
    NetworkAccessManager *manager = new NetworkAccessManager(parent);
    return manager;
}
