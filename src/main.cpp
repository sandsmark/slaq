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
#include "teaminfo.h"
#include "QQmlObjectListModel.h"
#include "downloadmanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
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
    qRegisterMetaType<SlackTeamClient*>("SlackClient*");
    qRegisterMetaType<EmojiInfo*>("EmojiInfo*");
    qRegisterMetaType<TeamInfo*>("TeamInfo*");
    qRegisterMetaType<Attachment*>("Attachment*");
    qRegisterMetaType<Message*>("Message*");
    qRegisterMetaType<EmojiCategoryHolder*>("EmojiCategoryHolder*");
    qRegisterMetaType<QList<EmojiInfo*>>("QList<EmojiInfo*>");
    qmlRegisterUncreatableType<ChatsModel>("com.iskrembilen", 1, 0, "ChatsModel", "Only instantiated by c++");
    qmlRegisterUncreatableType<UsersModel>("com.iskrembilen", 1, 0, "UsersModel", "Only instantiated by c++");
    qmlRegisterUncreatableType<Attachment>("com.iskrembilen", 1, 0, "Attachment", "Only instantiated by c++");

    qRegisterMetaType<ChatsModel::ChatType>("ChatsModel::ChatType");
    qRegisterMetaType<Chat*>("Chat*");

    qmlRegisterUncreatableType<MessageListModel>("com.iskrembilen", 1, 0, "MessageListModel", "Only instantiated by c++");
    qmlRegisterUncreatableType<User>("com.iskrembilen", 1, 0, "User", "Only instantiated by c++");
    qmlRegisterUncreatableType<Chat>("com.iskrembilen", 1, 0, "Chat", "Only instantiated by c++");
    qmlRegisterUncreatableType<QQmlObjectListModelBase> ("SlaqQmlModels", 1, 0, "QQmlObjectListModelBase",
                                                         QStringLiteral("!!!"));
    qmlRegisterUncreatableType<EmojiInfo>("SlaqQmlModels", 1, 0, "EmojiInfo", QStringLiteral("!!!"));
    SlackConfig::clearWebViewCache();
    //instantiate ImageCache
    engine.rootContext()->setContextProperty(QStringLiteral("ImagesCache"), ImagesCache::instance());
    engine.addImageProvider(QStringLiteral("emoji"), new EmojiProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("emojiCategoriesModel"),
                                             ImagesCache::instance()->emojiCategoriesModel());
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

    DownloadManager downloadManager;
    FileModel fileModel;
    engine.rootContext()->setContextProperty(QStringLiteral("fileModel"), &fileModel);
    engine.rootContext()->setContextProperty(QStringLiteral("downloadManager"), &downloadManager);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "No root objects?";
        //        return 1;
    }

    return app.exec();
}
