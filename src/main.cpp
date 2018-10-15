#include <QQuickView>
#include <QApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QtWebEngine>
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
    qSetMessagePattern("[%{time h:mm:ss.zzz} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{file}:%{line} - %{message}");
    QApplication app(argc, argv);

    QtWebEngine::initialize();
    app.setOrganizationName(QStringLiteral("Martin Sandsmark"));
    app.setApplicationName(QStringLiteral("Slaq"));
    app.setApplicationVersion(QString(SLAQ_VERSION));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    QSettings settings;
    QQuickStyle::setStyle(settings.value("style", QStringLiteral("Material")).toString());
    settings.setValue("style", QQuickStyle::name());

    int emojiFontId = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/TwitterColorEmoji.ttf"));
    qDebug() << "emoji fonts:" << QFontDatabase::applicationFontFamilies(emojiFontId);

    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(new NetworkAccessManagerFactory());

    qDebug() << "GUI thread" << QThread::currentThread();
    qRegisterMetaType<SlackTeamClient*>("SlackClient*");
    qRegisterMetaType<EmojiInfo*>("EmojiInfo*");
    qRegisterMetaType<TeamInfo*>("TeamInfo*");
    qRegisterMetaType<Attachment*>("Attachment*");
    qRegisterMetaType<Message*>("Message*");
    qRegisterMetaType<Message>("Message");
    qRegisterMetaType<QList<Message*>>("QList<Message*>");
    qRegisterMetaType<EmojiCategoryHolder*>("EmojiCategoryHolder*");
    qRegisterMetaType<QList<EmojiInfo*>>("QList<EmojiInfo*>");
    //qRegisterMetaType<QQmlObjectListModel<EmojiInfo>>("QQmlObjectListModel<EmojiInfo>");
    qRegisterMetaType<ChatsModel::ChatType>("ChatsModel::ChatType");
    qRegisterMetaType<SlackTeamClient::ClientStatus>("SlackTeamClient::ClientStatus");
    qRegisterMetaType<Chat*>("Chat*");
    qRegisterMetaType<User*>("User*");
    qRegisterMetaType<FileShare*>("FileShare*");

    qmlRegisterUncreatableType<ChatsModel>("com.iskrembilen", 1, 0, "ChatsModel", "Only instantiated by c++");
    qmlRegisterUncreatableType<UsersModel>("com.iskrembilen", 1, 0, "UsersModel", "Only instantiated by c++");
    qmlRegisterUncreatableType<Attachment>("com.iskrembilen", 1, 0, "Attachment", "Only instantiated by c++");
    qmlRegisterUncreatableType<MessageListModel>("com.iskrembilen", 1, 0, "MessageListModel", "Only instantiated by c++");
    qmlRegisterUncreatableType<User>("com.iskrembilen", 1, 0, "User", "Only instantiated by c++");
    qmlRegisterUncreatableType<Chat>("com.iskrembilen", 1, 0, "Chat", "Only instantiated by c++");
    qmlRegisterUncreatableType<SlackTeamClient>("com.iskrembilen", 1, 0, "SlackTeamClient", "Only instantiated by c++");
    qmlRegisterUncreatableType<FileShare>("com.iskrembilen", 1, 0, "FileShare", "Only instantiated by c++");
    qmlRegisterUncreatableType<FilesSharesModel>("com.iskrembilen", 1, 0, "FilesSharesModel", "Only instantiated by c++");

    qmlRegisterUncreatableType<QQmlObjectListModelBase> ("SlaqQmlModels", 1, 0, "QQmlObjectListModelBase",
                                                         QStringLiteral("!!!"));
    qmlRegisterUncreatableType<EmojiInfo>("SlaqQmlModels", 1, 0, "EmojiInfo", QStringLiteral("!!!"));

    //SlackConfig::clearWebViewCache();
    engine.rootContext()->setContextProperty("availableStyles", QQuickStyle::availableStyles());
    //instantiate ImageCache
    engine.rootContext()->setContextProperty(QStringLiteral("ImagesCache"), ImagesCache::instance());
    engine.addImageProvider(QStringLiteral("emoji"), new EmojiProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("emojiCategoriesModel"),
                                             ImagesCache::instance()->emojiCategoriesModel());
    QPointer<SlackClientThreadSpawner> _slackThread = new SlackClientThreadSpawner(qApp);

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

    int ret = app.exec();
    if (_slackThread) {
        _slackThread->wait();
    }
    return ret;
}
