#ifndef MESSAGEFORMATTER_H
#define MESSAGEFORMATTER_H

#include <QObject>

class MessageFormatter : public QObject
{
    Q_OBJECT
public:
    static void replaceUserInfo(QString &message);
    static void replaceTargetInfo(QString &message);
    static void replaceChannelInfo(QString &message);
    static void replaceSpecialCharacters(QString &message);
    static void replaceLinks(QString &message);
    static void replaceMarkdown(QString &message);
    static void replaceEmoji(QString &message);

signals:

public slots:
};

#endif // MESSAGEFORMATTER_H
