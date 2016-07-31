#ifndef SLACKSTREAM_H
#define SLACKSTREAM_H

#include <QObject>
#include <QJsonObject>
#include <QPointer>
#include <QUrl>
#include <QTimer>

#include "QtWebsocket/QWsSocket.h"

class SlackStream : public QObject
{
    Q_OBJECT
public:
    explicit SlackStream(QObject *parent = 0);
    ~SlackStream();

signals:
    void connected();
    void reconnecting();
    void disconnected();
    void messageReceived(QJsonObject message);

public slots:
    void listen(QUrl url);
    void checkConnection();
    void handleListerStart();
    void handleListerEnd();
    void handleMessage(QString message);

private:
    QPointer<QtWebsocket::QWsSocket> webSocket;
    QPointer<QTimer> checkTimer;

    bool isConnected;
    int lastMessageId;
};

#endif // SLACKSTREAM_H
