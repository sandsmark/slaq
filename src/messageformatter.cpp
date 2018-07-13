#include "messageformatter.h"

#include <QRegularExpression>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "imagescache.h"

#include "UsersModel.h"
#include "ChatsModel.h"

MessageFormatter::MessageFormatter() :
    m_labelPattern(QRegularExpression (QStringLiteral("<(http[^\\|>]+)\\|([^>]+)>"))),
    m_plainPattern(QRegularExpression(QStringLiteral("<(http[^>]+)>"))),
    m_mailtoPattern(QRegularExpression(QStringLiteral("<(mailto:[^\\|>]+)\\|([^>]+)>"))),
    m_italicPattern(QRegularExpression(QStringLiteral("(^|\\s)_([^_]+)_(\\s|\\.|\\?|!|,|$)"))),
    m_boldPattern(QRegularExpression(QStringLiteral("(^|\\s)\\*([^\\*]+)\\*(\\s|\\.|\\?|!|,|$)"))),
    m_strikePattern(QRegularExpression(QStringLiteral("(^|\\s)~([^~]+)~(\\s|\\.|\\?|!|,|$)"))),
    m_codePattern(QRegularExpression(QStringLiteral("(^|\\s)`([^`]+)`(\\s|\\.|\\?|!|,|$)"))),
    m_codeBlockPattern(QRegularExpression(QStringLiteral("```([^`]+)```"))),
    m_variableLabelPattern(QRegularExpression(QStringLiteral("<!(here|channel|group|everyone)\\|([^>]+)>"))),
    m_variablePattern(QRegularExpression(QStringLiteral("<!(here|channel|group|everyone)>"))),
    m_emojiPattern(QRegularExpression(QStringLiteral(":([\\w\\+\\-]+):(:[\\w\\+\\-]+:)?[\\?\\.!]?")))
{
    m_labelPattern.optimize();
    m_plainPattern.optimize();
    m_mailtoPattern.optimize();
    m_italicPattern.optimize();
    m_boldPattern.optimize();
    m_strikePattern.optimize();
    m_codePattern.optimize();
    m_codeBlockPattern.optimize();
    m_variableLabelPattern.optimize();
    m_variablePattern.optimize();
    m_emojiPattern.optimize();
}

void MessageFormatter::replaceUserInfo(User* user, QString &message)
{
    QRegularExpression userIdPattern("<@" + user->userId() + "(\\|[^>]+)?>");
    QString displayName = "<a href=\"slaq://user/" + user->userId() + "\">@" + user->username() + "</a>";

    message.replace(userIdPattern, displayName);
}

void MessageFormatter::replaceChannelInfo(Chat *chat, QString &message)
{
    QRegularExpression channelIdPattern("<#" + chat->id + "(\\|[^>]+)?>");
    QString displayName = "<a href=\"slaq://channel/" + chat->id + "\">#" + chat->name + "</a>";

    message.replace(channelIdPattern, displayName);
}

void MessageFormatter::replaceLinks(QString &message)
{
    message.replace(m_labelPattern, QStringLiteral("<a href=\"\\1\">\\2</a>"));
    message.replace(m_plainPattern, QStringLiteral("<a href=\"\\1\">\\1</a>"));
    message.replace(m_mailtoPattern, QStringLiteral("<a href=\"\\1\">\\2</a>"));
}

void MessageFormatter::replaceMarkdown(QString &message)
{
    message.replace(m_italicPattern, QStringLiteral("\\1<i>\\2</i>\\3"));
    message.replace(m_boldPattern, QStringLiteral("\\1<b>\\2</b>\\3"));
    message.replace(m_strikePattern, QStringLiteral("\\1<s>\\2</s>\\3"));
    message.replace(m_codePattern, QStringLiteral("\\1<code>\\2</code>\\3"));
    message.replace(m_codeBlockPattern, QStringLiteral("<br/><code>\\1</code><br/>"));

    message.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
}

void MessageFormatter::replaceEmoji(QString &message)
{
    if (!message.contains(':')) {
        return;
    }
    ImagesCache* imageCache = ImagesCache::instance();

    QRegularExpressionMatchIterator i = m_emojiPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString captured = match.captured();
        captured.replace(QStringLiteral(":"), QStringLiteral(""));
        //qDebug() << "captured" << captured;
        EmojiInfo* einfo = imageCache->getEmojiInfo(captured);
        if (einfo != nullptr) {
            if (imageCache->isUnicode() && !(einfo->imagesExist() & EmojiInfo::ImageSlackTeam)) {
                message.replace(":" + captured + ":", imageCache->getEmojiByName(captured));
            } else {
                QString replacement = QString(QStringLiteral("<img src=\"image://emoji/%1\" alt=\"\\1\" align=\"%2\" width=\"%3\" height=\"%4\" />"))
                        .arg(captured)
                        .arg(QStringLiteral("center"))
                        .arg(24)
                        .arg(24);

                message.replace(match.captured(), replacement);
                //qDebug() << "emoji image" << captured << message;
            }
        }
    }
}

void MessageFormatter::replaceAll(User *user, Chat *chat, QString &message)
{
    if (user != nullptr) {
        replaceUserInfo(user, message);
    }
    if (chat != nullptr) {
        replaceChannelInfo(chat, message);
    }
    replaceTargetInfo(message);
    replaceSpecialCharacters(message);
    replaceLinks(message);
    replaceMarkdown(message);
    replaceEmoji(message);
}

void MessageFormatter::replaceTargetInfo(QString &message)
{
    message.replace(m_variableLabelPattern, QStringLiteral("<a href=\"slaq://target/\\1\">\\2</a>"));
    message.replace(m_variablePattern, QStringLiteral("<a href=\"slaq://target/\\1\">@\\1</a>"));
}

void MessageFormatter::replaceSpecialCharacters(QString &message)
{
    message.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    message.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    message.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
}
