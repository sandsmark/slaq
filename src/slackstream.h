#ifndef SLACKSTREAM_H
#define SLACKSTREAM_H

#include <QObject>
#include <QJsonObject>
#include <QPointer>
#include <QUrl>
#include <QTimer>
#include <QWebSocket>

class SlackStream : public QObject
{
    Q_OBJECT
public:
    explicit SlackStream(QObject *parent = nullptr);
    virtual ~SlackStream();

    bool isConnected() const { return m_isConnected; }

signals:
    void connected();
    void reconnecting();
    void disconnected();
    void messageReceived(QJsonObject message);

public slots:
    void listen(const QUrl &url);
    void checkConnection();
    void handleListerStart();
    void handleListerEnd();

    void handleMessage(const QString& message);
    void handleBinaryMessage(const QByteArray &message);

    void handleFrame(const QString& message);
    void handleBinaryFrame(const QByteArray& message);

private:
    QPointer<QWebSocket> webSocket;
    QPointer<QTimer> checkTimer;

    bool m_isConnected;
    int m_lastMessageId;
};

#endif // SLACKSTREAM_H
