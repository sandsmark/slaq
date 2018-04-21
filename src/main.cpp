#include <QQuickView>
#include <QApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>

#include "slackclient.h"
#include "networkaccessmanagerfactory.h"
#include "notificationlistener.h"
#include "dbusadaptor.h"
#include "storage.h"
#include "filemodel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QQmlApplicationEngine engine;

    //QSettings settings; settings.remove("user/accessToken");
    SlackConfig::clearWebViewCache();

    qmlRegisterSingletonType<SlackClient>("com.iskrembilen.slaq", 1, 0, "Client", [](QQmlEngine *, QJSEngine*) -> QObject* {
        return new SlackClient;
    });

    engine.rootContext()->setContextProperty("fileModel", new FileModel());
    engine.setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());

    engine.load(QUrl("qrc:/qml/slaq.qml"));
    if  (engine.rootObjects().isEmpty()) {
        qWarning() << "No root objects?";
//        return 1;
    }

//    NotificationListener* listener = new NotificationListener(&engine);
//    new DBusAdaptor(listener);
//    QDBusConnection connection = QDBusConnection::sessionBus();
//    connection.registerService("slaq");
//    connection.registerObject("/", listener);

    return app.exec();
}
