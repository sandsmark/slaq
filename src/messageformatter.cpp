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

    loadEmojis();
}

#include <QDebug>

void MessageFormatter::replaceUserInfo(QString &message)
{
    foreach (const QVariant &value, Storage::users()) {
        QVariantMap user = value.toMap();
        QString id = user.value(QStringLiteral("id")).toString();
        QString name = user.value(QStringLiteral("name")).toString();

        QRegularExpression userIdPattern("<@" + id + "(\\|[^>]+)?>");
        QString displayName = "<a href=\"slaq://user/" + id + "\">@" + name + "</a>";

        message.replace(userIdPattern, displayName);
    }
}

void MessageFormatter::replaceChannelInfo(QString &message)
{
    foreach (const QVariant &value, Storage::channels()) {
        QVariantMap channel = value.toMap();
        QString id = channel.value(QStringLiteral("id")).toString();
        QString name = channel.value(QStringLiteral("name")).toString();

        QRegularExpression channelIdPattern("<#" + id + "(\\|[^>]+)?>");
        QString displayName = "<a href=\"slaq://channel/" + id + "\">#" + name + "</a>";

        message.replace(channelIdPattern, displayName);
    }
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

    QRegularExpressionMatchIterator i = m_emojiPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString captured = match.captured();
        captured.replace(QStringLiteral(":"), QStringLiteral(""));
        //qDebug() << "captured" << captured;
        if (m_emojis.contains(captured)) {
            message.replace(":" + captured + ":", m_emojis[captured]);
        } else if (ImagesCache::instance()->isExist(captured)) {
            QString replacement = QString(QStringLiteral("<img src=\"image://emoji/%1\" alt=\"\\1\" align=\"%2\" width=\"%3\" height=\"%4\" />"))
                    .arg(captured)
                    .arg(QStringLiteral("middle"))
                    .arg(24)
                    .arg(24);

            message.replace(match.captured(), replacement);
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
        const QString& emojiCategory = emoji.value(QStringLiteral("category")).toString();
        const QString& moji_ = emoji.value(QStringLiteral("moji")).toString();
        m_emojis[emojiName] = moji_;
        if (!m_emojiCategories.contains(emojiCategory, moji_)) {
            m_emojiCategories.insertMulti(emojiCategory, moji_);
        }
        for (const QJsonValue &alias : emoji["aliases"].toArray()) {            
            m_emojis[alias.toString().remove(QStringLiteral(":"))] = moji_;
        }
    }
    // TODO: ascii aliases like :) :(

    qDebug() << "Loaded" << m_emojis.count() << "emojis";
    qDebug() << "Loaded emoji categories:" << m_emojiCategories.uniqueKeys();
}

QString MessageFormatter::emojiNameByEmoji(const QString& emoji) const {
    foreach(const QString &key, m_emojis.keys()) {
        if (m_emojis.value(key) == emoji) {
            return key;
        }
    }
    return QString("");
}


QMultiMap<QString, QString> MessageFormatter::emojiCategories() const
{
    return m_emojiCategories;
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
