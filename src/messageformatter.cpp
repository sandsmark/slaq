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

QRegularExpression MessageFormatter::
m_emojiPattern(QRegularExpression(QStringLiteral(":([\\w\\+\\-]+):?:([skin-tone\\w\\+\\-]+)?:?[\\?\\.!]?"),
                           QRegularExpression::OptimizeOnFirstUsageOption));
QRegularExpression MessageFormatter::
m_labelPattern(QRegularExpression(QStringLiteral("<(.*?)>"),//("<(http[^\\|>]+)\\|([^>]+)>"),
                                  QRegularExpression::OptimizeOnFirstUsageOption));
QRegularExpression MessageFormatter::
m_plainPattern(QRegularExpression(QStringLiteral("<([^>][a-z0-9]+:.*)>"),
                                  QRegularExpression::OptimizeOnFirstUsageOption));
QRegularExpression MessageFormatter::
m_mailtoPattern(QRegularExpression(QStringLiteral("<(mailto:[^\\|>]+)\\|([^>]+)>"),
                                   QRegularExpression::OptimizeOnFirstUsageOption));

MessageFormatter::MessageFormatter() :

    m_italicPattern(QRegularExpression(QStringLiteral("(^|\\s)_([^_]+)_(\\s|\\.|\\?|!|,|$)"))),
    m_boldPattern(QRegularExpression(QStringLiteral("(^|\\s)\\*([^\\*]+)\\*(\\s|\\.|\\?|!|,|$)"))),
    m_strikePattern(QRegularExpression(QStringLiteral("(^|\\s)~([^~]+)~(\\s|\\.|\\?|!|,|$)"))),
    m_codePattern(QRegularExpression(QStringLiteral("(^|\\s)`([^`]+)`(\\s|\\.|\\?|!|,|$)"))),
    m_codeBlockPattern(QRegularExpression(QStringLiteral("```(.*)```"))),
    m_variableLabelPattern(QRegularExpression(QStringLiteral("<!(here|channel|group|everyone)\\|([^>]+)>"))),
    m_variablePattern(QRegularExpression(QStringLiteral("<!(here|channel|group|everyone)>"))),
    m_channelPattern(QRegularExpression(QStringLiteral("<#([A-Z0-9]+)\\|([^>]+)>")))
{
    m_italicPattern.optimize();
    m_boldPattern.optimize();
    m_strikePattern.optimize();
    m_codePattern.optimize();
    m_codeBlockPattern.optimize();
    m_variableLabelPattern.optimize();
    m_variablePattern.optimize();
    m_channelPattern.optimize();
    m_codeBlockPattern.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);
}

void MessageFormatter::replaceUserInfo(SlackUser* user, QString &message)
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

void MessageFormatter::replaceLinks(ChatsModel* chatModel, QString &message)
{
    QRegularExpressionMatchIterator i = m_labelPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QStringRef capture = match.capturedRef(1);
        //qDebug() << "capture" << capture;
        if (!capture.contains("/a") && !capture.contains("a href=")) {
            QVector<QStringRef> hrefs = capture.split("|");
            QString link;
            QString displayName;
            if (hrefs.count() > 1) {
                link = hrefs.at(0).toString();
                displayName = hrefs.at(1).toString();
            } else {
                link = capture.toString();
                displayName = capture.toString();
            }
            if (capture.startsWith("#C")) { //channel link
                link = QString("slaq://channel/%1").arg(hrefs.at(0).mid(1));
                if (hrefs.size() > 1) {
                    displayName = "#"+hrefs.at(1).toString();
                }
            } else if (capture.startsWith("@U") || capture.startsWith("@W")) { //user
                SlackUser* user = chatModel->users()->user(hrefs.at(0).mid(1).toString());
                if (user != nullptr) {
                    link = QString("slaq://user/%1").arg(user->userId());
                    displayName = "@" + user->username();
                }
            } else if (capture.startsWith("!")) {
                const QString trgt = hrefs.at(0).mid(1).toString();
                link = QString("slaq://target/%1").arg(trgt);
                if (hrefs.size() < 2) {
                    displayName = "@" + trgt;
                }
            }
            message.replace(match.captured(),
                            QStringLiteral("<a href=\"%1\">%2</a>").
                            arg(link).arg(displayName));
            //qDebug() << "links:" << match.capturedTexts() << message;
        }
    }
}

void MessageFormatter::replaceMarkdown(QString &message)
{
    message.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
    message.replace(m_italicPattern, QStringLiteral("\\1<i>\\2</i>\\3"));
    message.replace(m_boldPattern, QStringLiteral("\\1<b>\\2</b>\\3"));
    message.replace(m_strikePattern, QStringLiteral("\\1<s>\\2</s>\\3"));
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
                QString replacement = QString(QStringLiteral("<img src=\"image://emoji/%1\" alt=\"\\1\" vertical-align:\"%2\" width=\"%3\" height=\"%4\" />"))
                        .arg(captured)
                        .arg(QStringLiteral("middle"))
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
    //replaceLinks(message); //must be 1st
    replaceChannelInfo(chat, message);
    replaceTargetInfo(message);
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
