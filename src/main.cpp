#include <QQuickView>
#include <QApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QtWebView>
#include <QNetworkProxyFactory>
#include <QThread>
#include <QTextCodec>
#include <QQuickStyle>
#include <QFontDatabase>

#include "slackclient.h"
#include "networkaccessmanagerfactory.h"
#include "notificationlistener.h"
#include "storage.h"
#include "filemodel.h"
#include "emojiprovider.h"
#include "imagescache.h"
#include "slackclientthreadspawner.h"
#include "emojiinfo.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Martin Sandsmark");
    app.setApplicationName("Slaq");

    int emojiFontId = QFontDatabase::addApplicationFont(":/fonts/TwitterColorEmoji.ttf");
    qDebug() << "emoji fonts:" << QFontDatabase::applicationFontFamilies(emojiFontId);

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    QQuickStyle::setStyle("Material");
    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());

    QtWebView::initialize();
    qDebug() << "GUI thread" << QThread::currentThreadId();
    qRegisterMetaType<SlackClient*>("SlackClient*");
    qRegisterMetaType<EmojiInfo*>("EmojiInfo*");
    qRegisterMetaType<QList<EmojiInfo*>>("QList<EmojiInfo*>");

    SlackConfig::clearWebViewCache();
    //instantiate ImageCache
    engine.rootContext()->setContextProperty("ImagesCache", ImagesCache::instance());
    SlackClientThreadSpawner* _slackThread = new SlackClientThreadSpawner;
    engine.rootContext()->setContextProperty("SlackClient", _slackThread);
    _slackThread->start();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        _slackThread->quit();
    });
    //    NotificationListener* listener = new NotificationListener(&engine);
    //    new DBusAdaptor(listener);
    //    QDBusConnection connection = QDBusConnection::sessionBus();
    //    connection.registerService("slaq");
    //    connection.registerObject("/", listener);

    engine.rootContext()->setContextProperty("fileModel", new FileModel());
    engine.addImageProvider("emoji", new EmojiProvider);
    engine.setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());
    engine.load(QUrl("qrc:/qml/main.qml"));
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "No root objects?";
        //        return 1;
    }

    return app.exec();
}
