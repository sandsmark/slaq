#include <QQuickView>
#include <QGuiApplication>
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
#include "teaminfo.h"
#include "QQmlObjectListModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Martin Sandsmark"));
    app.setApplicationName(QStringLiteral("Slaq"));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    QQuickStyle::setStyle(QStringLiteral("Material"));
    int emojiFontId = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/TwitterColorEmoji.ttf"));
    qDebug() << "emoji fonts:" << QFontDatabase::applicationFontFamilies(emojiFontId);

    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());

    QtWebView::initialize();
    qDebug() << "GUI thread" << QThread::currentThreadId();
    qRegisterMetaType<SlackClient*>("SlackClient*");
    qRegisterMetaType<EmojiInfo*>("EmojiInfo*");
    qRegisterMetaType<TeamInfo*>("TeamInfo*");
    qRegisterMetaType<QList<EmojiInfo*>>("QList<EmojiInfo*>");
    qmlRegisterUncreatableType<QQmlObjectListModelBase> ("SlaqQmlModels", 1, 0, "QQmlObjectListModelBase",
                                                         QStringLiteral("!!!"));
    qmlRegisterUncreatableType<EmojiInfo>("SlaqQmlModels", 1, 0, "EmojiInfo", "!!!");
    SlackConfig::clearWebViewCache();
    //instantiate ImageCache
    engine.rootContext()->setContextProperty(QStringLiteral("ImagesCache"), ImagesCache::instance());
    SlackClientThreadSpawner* _slackThread = new SlackClientThreadSpawner;
    engine.rootContext()->setContextProperty(QStringLiteral("SlackClient"), _slackThread);
    engine.rootContext()->setContextProperty(QStringLiteral("teamsModel"), _slackThread->teamsModel());
    engine.rootContext()->setContextProperty(QStringLiteral("SlackConfig"), SlackConfig::instance());
    _slackThread->start();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        _slackThread->quit();
    });
    //    NotificationListener* listener = new NotificationListener(&engine);
    //    new DBusAdaptor(listener);
    //    QDBusConnection connection = QDBusConnection::sessionBus();
    //    connection.registerService("slaq");
    //    connection.registerObject("/", listener);

    engine.rootContext()->setContextProperty(QStringLiteral("fileModel"), new FileModel());
    engine.addImageProvider(QStringLiteral("emoji"), new EmojiProvider);
    engine.setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "No root objects?";
        //        return 1;
    }

    return app.exec();
}
