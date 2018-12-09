#pragma once

#include <QString>
#include <QRegularExpression>

class SlackUser;
class ChatsModel;
class Chat;

class MessageFormatter
{
public:
    MessageFormatter();

    void replaceUserInfo(SlackUser *user, QString &message);
    void replaceTargetInfo(QString &message);
    void replaceChannelInfo(ChatsModel *chatModel, QString &message);
    void replaceSpecialCharacters(QString &message);
    static void replaceLinks(ChatsModel *chatModel, QString &message);
    void replaceMarkdown(QString &message);
    static void replaceEmoji(QString &message);
    void replaceAll(ChatsModel* chat,  QString &message);
    void doReplaceChannelInfo(Chat *chat, QString &message);

private:
    static QRegularExpression m_labelPattern;
    static QRegularExpression m_plainPattern;
    static QRegularExpression m_mailtoPattern;

    QRegularExpression m_italicPattern;
    QRegularExpression m_boldPattern;
    QRegularExpression m_strikePattern;
    QRegularExpression m_codePattern;
    QRegularExpression m_codeBlockPattern;

    QRegularExpression m_variableLabelPattern;
    QRegularExpression m_variablePattern;
    static QRegularExpression m_emojiPattern;
    QRegularExpression m_channelPattern;
};

