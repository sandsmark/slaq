#include "slackstream.h"

#include <QJsonDocument>
#include <QJsonObject>

SlackStream::SlackStream(QObject *parent) :
    QObject(parent), m_isConnected(false), m_lastMessageId(0)
{
    webSocket = new QWebSocket("", QWebSocketProtocol::VersionLatest);
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
    disconnect(webSocket, SIGNAL(disconnected()), this, SLOT(handleListerEnd()));

    if (!webSocket.isNull()) {
        handleListerEnd();
        webSocket->deleteLater();
    }
}

void SlackStream::listen(const QUrl& url)
{
    qDebug() << "Connect socket" << url;
    webSocket->open(url);
}

void SlackStream::checkConnection()
{
    qDebug() << "check connection" << m_isConnected;
    if (m_isConnected) {
        QJsonObject values;
        values.insert("id", QJsonValue(++m_lastMessageId));
        values.insert("type", QJsonValue(QString("ping")));

        qDebug() << "Check connection" << m_lastMessageId;

        QJsonDocument document(values);
        QByteArray data = document.toJson(QJsonDocument::Compact);
        webSocket->sendBinaryMessage(data);
    }
}

void SlackStream::handleListerStart()
{
    qDebug() << "Socket connected";
    emit connected();
    m_isConnected = true;
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
    //qDebug() << "Got text message" << message;

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
