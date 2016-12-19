#ifndef NOTIFICATIONLISTENER_H
#define NOTIFICATIONLISTENER_H

#include <QObject>

class NotificationListener : public QObject
{
  Q_OBJECT
public:
  explicit NotificationListener(QObject *parent = 0);

signals:
  void activateReceived();

public slots:
  void activate();
};

#endif // NOTIFICATIONLISTENER_H
