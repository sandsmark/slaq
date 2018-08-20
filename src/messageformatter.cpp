#include "messageformatter.h"

#include <QRegularExpression>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGuiApplication>
#include <QPalette>
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
    m_emojiPattern(QRegularExpression(QStringLiteral(":([\\w\\+\\-]+):?:([skin-tone\\w\\+\\-]+)?:?[\\?\\.!]?"))),
    m_channelPattern(QRegularExpression(QStringLiteral("<#([A-Z0-9]+)|([^>]+)>")))
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
    m_channelPattern.optimize();
}

void MessageFormatter::replaceUserInfo(User* user, QString &message)
{
    if (user == nullptr) {
        qWarning() << "no user for message" << message;
        return;
    }
    QRegularExpression userIdPattern("<@" + user->userId() + "(\\|[^>]+)?>");
    QString displayName = "<a href=\"slaq://user/" + user->userId() + "\">@" + user->username() + "</a>";
    message.replace(userIdPattern, displayName);
}

void MessageFormatter::replaceChannelInfo(ChatsModel *chatModel, QString &message)
{
    if (chatModel == nullptr) {
        qWarning() << "no chat found for message" << message;
        return;
    }
    Chat* chat = nullptr;
    QRegularExpressionMatchIterator i = m_channelPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        const QString& id = match.captured(1);
        if (!id.isEmpty()) {
            chat = chatModel->chat(id);
            if (chat != nullptr) {
                doReplaceChannelInfo(chat, message);
            } else {
                qWarning() << "channel not found" << id;
            }
        }
    }
}

void MessageFormatter::doReplaceChannelInfo(Chat *chat, QString &message)
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
    const QPalette& palette = QGuiApplication::palette();
    const QString& blockStyleBg = palette.color(QPalette::Highlight).name(QColor::HexRgb);
    const QString& blockStyleFg = palette.color(QPalette::HighlightedText).name(QColor::HexRgb);

    message.replace(m_italicPattern, QStringLiteral("\\1<i>\\2</i>\\3"));
    message.replace(m_boldPattern, QStringLiteral("\\1<b>\\2</b>\\3"));
    message.replace(m_strikePattern, QStringLiteral("\\1<s>\\2</s>\\3"));
    message.replace(m_codeBlockPattern, QStringLiteral("<blockquote><table style=\"background-color:%1; color:%2; word-wrap:break-word;\"><tr><td>\\1</td></tr></table></blockquote>").arg(blockStyleBg).arg(blockStyleFg));
    message.replace(m_codePattern, QStringLiteral("\\1<span style=\"background-color:%1; color:%2; white-space:pre;\"> \\2 </span>\\3").arg(blockStyleBg).arg(blockStyleFg));
    message.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
}

void MessageFormatter::replaceEmoji(QString &message)
{
    if (!message.contains(':')) {
        return;
    }
    ImagesCache* imageCache = ImagesCache::instance();

    QRegularExpressionMatchIterator i = m_emojiPattern.globalMatch(message);
    if (!i.isValid()) {
        qWarning() << "error parsing" << message << m_emojiPattern.errorString();
        return;
    }
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString captured = match.captured(1);
        //captured.replace(QStringLiteral(":"), QStringLiteral(""));
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
        } else {
            qWarning() << "Emoji not found for" << captured << "message:" << message;
        }
    }
}

void MessageFormatter::replaceAll(ChatsModel *chat, QString &message)
{
    replaceChannelInfo(chat, message);
    replaceTargetInfo(message);
    replaceMarkdown(message);
    replaceEmoji(message);
    replaceLinks(message);
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
