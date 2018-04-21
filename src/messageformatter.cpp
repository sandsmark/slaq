#include "messageformatter.h"

#include <QRegularExpression>

#include "storage.h"

void MessageFormatter::replaceUserInfo(QString &message) {
    foreach (const QVariant &value, Storage::users()) {
        QVariantMap user = value.toMap();
        QString id = user.value("id").toString();
        QString name = user.value("name").toString();

        QRegularExpression userIdPattern("<@" + id + "(\\|[^>]+)?>");
        QString displayName = "<a href=\"slaq://user/"+ id +"\">@" + name + "</a>";

        message.replace(userIdPattern, displayName);
    }
}

void MessageFormatter::replaceChannelInfo(QString &message) {
    foreach (const QVariant &value, Storage::channels()) {
        QVariantMap channel = value.toMap();
        QString id = channel.value("id").toString();
        QString name = channel.value("name").toString();

        QRegularExpression channelIdPattern("<#" + id + "(\\|[^>]+)?>");
        QString displayName = "<a href=\"slaq://channel/"+ id +"\">#" + name + "</a>";

        message.replace(channelIdPattern, displayName);
    }
}

void MessageFormatter::replaceLinks(QString &message) {
    QRegularExpression labelPattern("<(http[^\\|>]+)\\|([^>]+)>");
    message.replace(labelPattern, "<a href=\"\\1\">\\2</a>");

    QRegularExpression plainPattern("<(http[^>]+)>");
    message.replace(plainPattern, "<a href=\"\\1\">\\1</a>");

    QRegularExpression mailtoPattern("<(mailto:[^\\|>]+)\\|([^>]+)>");
    message.replace(mailtoPattern, "<a href=\"\\1\">\\2</a>");
}

void MessageFormatter::replaceMarkdown(QString &message) {
    QRegularExpression italicPattern("(^|\\s)_([^_]+)_(\\s|\\.|\\?|!|,|$)");
    message.replace(italicPattern, "\\1<i>\\2</i>\\3");

    QRegularExpression boldPattern("(^|\\s)\\*([^\\*]+)\\*(\\s|\\.|\\?|!|,|$)");
    message.replace(boldPattern, "\\1<b>\\2</b>\\3");

    QRegularExpression strikePattern("(^|\\s)~([^~]+)~(\\s|\\.|\\?|!|,|$)");
    message.replace(strikePattern, "\\1<s>\\2</s>\\3");

    QRegularExpression codePattern("(^|\\s)`([^`]+)`(\\s|\\.|\\?|!|,|$)");
    message.replace(codePattern, "\\1<code>\\2</code>\\3");

    QRegularExpression codeBlockPattern("```([^`]+)```");
    message.replace(codeBlockPattern, "<br/><code>\\1</code><br/>");

    QRegularExpression newLinePattern("\n");
    message.replace(newLinePattern, "<br/>");
}

void MessageFormatter::replaceEmoji(QString &message) {
    QMap<QString,QString> alternatives;
    alternatives.insert(":slightly_smiling_face:", ":grinning:");
    alternatives.insert(":slightly_frowning_face:", ":confused:");

    foreach (QString from, alternatives.keys()) {
        message.replace(from, alternatives.value(from));
    }

    QRegularExpression emojiPattern(":([\\w\\+\\-]+):(:[\\w\\+\\-]+:)?[\\?\\.!]?");
    message.replace(emojiPattern, "<img src=\"http://www.tortue.me/emoji/\\1.png\" alt=\"\\1\" align=\"bottom\" width=\"32\" height=\"32\" />");
}

void MessageFormatter::replaceTargetInfo(QString &message) {
    QRegularExpression variableLabelPattern("<!(here|channel|group|everyone)\\|([^>]+)>");
    message.replace(variableLabelPattern, "<a href=\"slaq://target/\\1\">\\2</a>");

    QRegularExpression variablePattern("<!(here|channel|group|everyone)>");
    message.replace(variablePattern, "<a href=\"slaq://target/\\1\">@\\1</a>");
}

void MessageFormatter::replaceSpecialCharacters(QString &message) {
    message.replace(QRegularExpression("&gt;"), ">");
    message.replace(QRegularExpression("&lt;"), "<");
    message.replace(QRegularExpression("&amp;"), "&");
}
