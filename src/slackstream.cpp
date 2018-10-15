#include "slackstream.h"
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>

SlackStream::SlackStream(QObject *parent) :
    QObject(parent), m_isConnected(false), m_lastMessageId(0)
{
    webSocket = new QWebSocket(QStringLiteral("org.slaq"), QWebSocketProtocol::VersionLatest, this);
    checkTimer = new QTimer(this);

    connect(webSocket, &QWebSocket::connected, this, &SlackStream::handleListerStart);
    connect(webSocket, &QWebSocket::disconnected, this, &SlackStream::handleListerEnd);
    connect(webSocket, &QWebSocket::textMessageReceived, this, &SlackStream::handleMessage);
    connect(webSocket, &QWebSocket::textFrameReceived, this, &SlackStream::handleFrame);
    connect(webSocket, &QWebSocket::binaryMessageReceived, this, &SlackStream::handleBinaryMessage);
    connect(webSocket, &QWebSocket::binaryFrameReceived, this, &SlackStream::handleBinaryFrame);
    connect(checkTimer, &QTimer::timeout, this, &SlackStream::checkConnection);
}

SlackStream::~SlackStream()
{
    checkTimer->stop();
    disconnect(webSocket.data(), &QWebSocket::disconnected, this, &SlackStream::handleListerEnd);

    webSocket->abort();
}

void SlackStream::listen(const QUrl& url)
{
    qDebug() << "Connect socket" << url << QThread::currentThread();
    webSocket->open(url);
}

void SlackStream::checkConnection()
{
    //qDebug() << "check connection" << m_isConnected;
    if (m_isConnected) {
        QJsonObject values;
        values.insert(QStringLiteral("type"), QJsonValue(QStringLiteral("ping")));

        //qDebug() << "Check connection" << m_lastMessageId;
        sendMessage(values);
    }
}

void SlackStream::handleListerStart()
{
    qWarning() << "Socket connected";
    m_isConnected = true;
    emit connected();
    checkTimer->start(15000);
}

void SlackStream::handleListerEnd()
{
    qDebug() << "Socket disconnected";
    checkTimer->stop();
    m_isConnected = false;
    m_lastMessageId = 0;
    emit disconnected();
}

void SlackStream::handleMessage(const QString& message)
{
//    if (!message.contains("pong")) {
//        qDebug() << "Got text message" << message;
//    }

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse message" << message;
        return;
    }

    emit messageReceived(document.object());
}

void SlackStream::handleBinaryMessage(const QByteArray& message)
{
    Q_UNUSED(message)
}

void SlackStream::handleFrame(const QString& message)
{
    Q_UNUSED(message)
}

void SlackStream::handleBinaryFrame(const QByteArray &message)
{
    Q_UNUSED(message)
}

void SlackStream::stopStream()
{
    webSocket->abort();
}

void SlackStream::sendBinaryMessage(const QByteArray &message)
{
    if (webSocket->isValid()) {
        webSocket->sendBinaryMessage(message);
    }
}

void SlackStream::sendMessage(QJsonObject &message)
{
    if (!message.contains(QStringLiteral("id"))) {
        message.insert(QStringLiteral("id"), QJsonValue(++m_lastMessageId));
    }
    QJsonDocument document(message);
    const QByteArray& data = document.toJson(QJsonDocument::Compact);
    //qDebug() << "sending" << data;
    webSocket->sendBinaryMessage(data);
}
