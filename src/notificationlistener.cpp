#include "notificationlistener.h"

#include <QDebug>
#include <QtQuick/QQuickItem>

NotificationListener::NotificationListener(QQmlApplicationEngine *engine, QObject *parent) : QObject(parent),
    m_engine(engine)
{
}

void NotificationListener::activate(const QString &channelId) {
    qDebug() << "Activate notification received" << channelId;

    if (!m_engine->rootObjects().isEmpty()) {
        QMetaObject::invokeMethod(m_engine->rootObjects().first(), "activateChannel", Q_ARG(QVariant, QVariant(channelId)));
    } else {
        qWarning() << "No root objects";
    }
}
