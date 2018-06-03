#ifndef MESSAGEFORMATTER_H
#define MESSAGEFORMATTER_H

#include <QString>
#include <QRegularExpression>

class MessageFormatter
{
public:
    MessageFormatter();

    void replaceUserInfo(const QVariantList &users, QString &message);
    void replaceTargetInfo(QString &message);
    void replaceChannelInfo(const QVariantList &channels, QString &message);
    void replaceSpecialCharacters(QString &message);
    void replaceLinks(QString &message);
    void replaceMarkdown(QString &message);
    void replaceEmoji(QString &message);

signals:

public slots:
private:
    void loadEmojis();

    QRegularExpression m_labelPattern;
    QRegularExpression m_plainPattern;
    QRegularExpression m_mailtoPattern;

    QRegularExpression m_italicPattern;
    QRegularExpression m_boldPattern;
    QRegularExpression m_strikePattern;
    QRegularExpression m_codePattern;
    QRegularExpression m_codeBlockPattern;

    QRegularExpression m_variableLabelPattern;
    QRegularExpression m_variablePattern;
    QRegularExpression m_emojiPattern;
};

#endif // MESSAGEFORMATTER_H
