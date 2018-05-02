#include "messageformatter.h"

#include <QRegularExpression>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "imagescache.h"

#include "storage.h"


MessageFormatter::MessageFormatter() :
    m_labelPattern(QRegularExpression ("<(http[^\\|>]+)\\|([^>]+)>")),
    m_plainPattern(QRegularExpression("<(http[^>]+)>")),
    m_mailtoPattern(QRegularExpression("<(mailto:[^\\|>]+)\\|([^>]+)>")),
    m_italicPattern(QRegularExpression("(^|\\s)_([^_]+)_(\\s|\\.|\\?|!|,|$)")),
    m_boldPattern(QRegularExpression("(^|\\s)\\*([^\\*]+)\\*(\\s|\\.|\\?|!|,|$)")),
    m_strikePattern(QRegularExpression("(^|\\s)~([^~]+)~(\\s|\\.|\\?|!|,|$)")),
    m_codePattern(QRegularExpression("(^|\\s)`([^`]+)`(\\s|\\.|\\?|!|,|$)")),
    m_codeBlockPattern(QRegularExpression("```([^`]+)```")),
    m_variableLabelPattern(QRegularExpression("<!(here|channel|group|everyone)\\|([^>]+)>")),
    m_variablePattern(QRegularExpression("<!(here|channel|group|everyone)>"))
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

    loadEmojis();
}

#include <QDebug>

void MessageFormatter::replaceUserInfo(QString &message)
{
    foreach (const QVariant &value, Storage::users()) {
        QVariantMap user = value.toMap();
        QString id = user.value("id").toString();
        QString name = user.value("name").toString();

        QRegularExpression userIdPattern("<@" + id + "(\\|[^>]+)?>");
        QString displayName = "<a href=\"slaq://user/" + id + "\">@" + name + "</a>";

        message.replace(userIdPattern, displayName);
    }
}

void MessageFormatter::replaceChannelInfo(QString &message)
{
    foreach (const QVariant &value, Storage::channels()) {
        QVariantMap channel = value.toMap();
        QString id = channel.value("id").toString();
        QString name = channel.value("name").toString();

        QRegularExpression channelIdPattern("<#" + id + "(\\|[^>]+)?>");
        QString displayName = "<a href=\"slaq://channel/" + id + "\">#" + name + "</a>";

        message.replace(channelIdPattern, displayName);
    }
}

void MessageFormatter::replaceLinks(QString &message)
{
    message.replace(m_labelPattern, "<a href=\"\\1\">\\2</a>");
    message.replace(m_plainPattern, "<a href=\"\\1\">\\1</a>");
    message.replace(m_mailtoPattern, "<a href=\"\\1\">\\2</a>");
}

void MessageFormatter::replaceMarkdown(QString &message)
{
    message.replace(m_italicPattern, "\\1<i>\\2</i>\\3");
    message.replace(m_boldPattern, "\\1<b>\\2</b>\\3");
    message.replace(m_strikePattern, "\\1<s>\\2</s>\\3");
    message.replace(m_codePattern, "\\1<code>\\2</code>\\3");
    message.replace(m_codeBlockPattern, "<br/><code>\\1</code><br/>");

    message.replace("\n", "<br/>");
}

void MessageFormatter::replaceEmoji(QString &message)
{
    if (!message.contains(':')) {
        return;
    }

	QRegularExpression emojiPattern(":([\\w\\+\\-]+):(:[\\w\\+\\-]+:)?[\\?\\.!]?");

	QRegularExpressionMatchIterator i = emojiPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString captured = match.captured();
        captured.replace(":", "");
        qDebug() << "captured" << captured;
        if (ImagesCache::instance()->isExist(captured)) {
            QString replacement = QString("<img src=\"image://emoji/%1\" alt=\"\\1\" align=\"%2\" width=\"%3\" height=\"%4\" />")
                    .arg(captured)
                    .arg("middle")
                    .arg(24)
                    .arg(24);

            message.replace(match.captured(), replacement);
        } else {
            if (m_emojis.contains(captured)) {
                message.replace(":" + captured + ":", m_emojis[captured]);
            }
        }
    }
}

void MessageFormatter::loadEmojis()
{
    QFile emojiFile(":/data/emojis.json");
    if (!emojiFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open emoji file for reading";
        return;
    }
    const QJsonObject emojisObject = QJsonDocument::fromJson(emojiFile.readAll()).object();
    if (emojisObject.isEmpty()) {
        qWarning() << "Failed to parse emojis file";
        return;
    }

    for (const QString &emojiName : emojisObject.keys()) {
        QJsonObject emoji = emojisObject[emojiName].toObject();
        m_emojis[emojiName] = emoji["moji"].toString();
        for (const QJsonValue &alias : emoji["aliases"].toArray()) {
            m_emojis[alias.toString().remove(":")] = emoji["moji"].toString();
        }
    }
    // TODO: ascii aliases like :) :(

    qDebug() << "Loaded" << m_emojis.count() << "emojis";
}

void MessageFormatter::replaceTargetInfo(QString &message)
{
    message.replace(m_variableLabelPattern, "<a href=\"slaq://target/\\1\">\\2</a>");
    message.replace(m_variablePattern, "<a href=\"slaq://target/\\1\">@\\1</a>");
}

void MessageFormatter::replaceSpecialCharacters(QString &message)
{
    message.replace("&gt;", ">");
    message.replace("&lt;", "<");
    message.replace("&amp;", "&");
}
