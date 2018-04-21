#ifndef NOTIFICATIONLISTENER_H
#define NOTIFICATIONLISTENER_H

#include <QObject>
#include <QQmlApplicationEngine>

class NotificationListener : public QObject
{
    Q_OBJECT
public:
    explicit NotificationListener(QQmlApplicationEngine *engine, QObject *parent = 0);

public slots:
    void activate(const QString &channelId);

private:
    QQmlApplicationEngine *m_engine;
};

#endif // NOTIFICATIONLISTENER_H
