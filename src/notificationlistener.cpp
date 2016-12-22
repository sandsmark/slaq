#include "notificationlistener.h"

#include <QDebug>
#include <QtQuick/QQuickItem>

NotificationListener::NotificationListener(QQuickView *view, QObject *parent) : QObject(parent) {
  this->view = view;
}

void NotificationListener::activate(const QString &channelId) {
    qDebug() << "Activate notification received" << channelId;
    QMetaObject::invokeMethod(view->rootObject(), "activateChannel", Q_ARG(QVariant, QVariant(channelId)));
}
