#include "networkaccessmanagerfactory.h"

#include <QDebug>

#include "networkaccessmanager.h"
#include "slackconfig.h"

QNetworkAccessManager *NetworkAccessManagerFactory::create(QObject *parent)
{
    NetworkAccessManager *manager = new NetworkAccessManager(parent);
    manager->setCookieJar(SlackConfig::instance()->cookieJar);
    return manager;
}
