#include "notificationlistener.h"

#include <QDebug>

NotificationListener::NotificationListener(QObject *parent) : QObject(parent) {
}

void NotificationListener::activate() {
    qDebug() << "Activate notification received received";
    emit activateReceived();
}
