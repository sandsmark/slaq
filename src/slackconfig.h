#ifndef SLACKCONFIG_H
#define SLACKCONFIG_H

#include <QObject>
#include <QSettings>

class SlackConfig : public QObject
{
    Q_OBJECT
public:
    explicit SlackConfig(QObject *parent = 0);

    QString accessToken();
    void setAccessToken(QString accessToken);
    QString userId();
    void setUserId(QString userId);

    static void clearWebViewCache();

signals:

public slots:

private:
    QSettings settings;
    QString currentUserId;
};

#endif // SLACKCONFIG_H
