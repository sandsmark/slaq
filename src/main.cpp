#include <QQuickView>
#include <QApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QtWebView>
#include <QNetworkProxyFactory>
#include <QThread>

#include "slackclient.h"
#include "networkaccessmanagerfactory.h"
#include "notificationlistener.h"
#include "storage.h"
#include "filemodel.h"
#include "emojiprovider.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Martin Sandsmark");
    app.setApplicationName("Slaq");

    QNetworkProxyFactory::setUseSystemConfiguration(true);
    QQmlApplicationEngine engine;

    QtWebView::initialize();

    SlackConfig::clearWebViewCache();

    qmlRegisterSingletonType<SlackClient>("com.iskrembilen.slaq", 1, 0, "Client", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return new SlackClient;
    });

    qDebug() << "GUI thread" << QThread::currentThreadId();

    engine.rootContext()->setContextProperty("fileModel", new FileModel());
    engine.setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());
    engine.addImageProvider("emoji", new EmojiProvider);

    engine.load(QUrl("qrc:/qml/main.qml"));
    if (engine.rootObjects().isEmpty()) {
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
