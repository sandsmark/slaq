#include <QtQuick>
#include <sailfishapp.h>

#include "slackclient.h"
#include "networkaccessmanagerfactory.h"
#include "storage.h"

static QObject *slack_client_provider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    SlackClient *client = new SlackClient();
    return client;
}

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    //QSettings settings; settings.remove("user/accessToken");
    SlackConfig::clearWebViewCache();

    qmlRegisterSingletonType<SlackClient>("harbour.slackfish", 1, 0, "Client", slack_client_provider);

    view->setSource(SailfishApp::pathTo("qml/harbour-slackfish.qml"));
    view->engine()->setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());
    view->showFullScreen();

    int result = app->exec();

    qDebug() << "Application terminating";
    return result;
}
