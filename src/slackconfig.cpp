#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "slackconfig.h"
#include "teaminfo.h"

SlackConfig::SlackConfig(QObject *parent) :
    QObject(parent), m_settings(this), m_currentUserId()
{
}

QString SlackConfig::userId()
{
    return m_currentUserId;
}

void SlackConfig::setUserInfo(const QString &userId, const QString &teamId, const QString &teamName)
{
    m_currentUserId = userId;
    m_currentTeamId = teamId;
    m_currentTeamName = teamName;
}

void SlackConfig::loadTeamInfo(TeamInfo &teamInfo)
{
    m_settings.beginGroup(teamInfo.teamId());
    if (teamInfo.m_id.isEmpty()) {
        teamInfo.m_id = m_settings.value(QStringLiteral("id")).toString();
    }
    if (teamInfo.m_name.isEmpty()) {
        teamInfo.m_name = m_settings.value(QStringLiteral("name")).toString();
    }
    if (teamInfo.m_icons.isEmpty()) {
        teamInfo.m_icons = m_settings.value(QStringLiteral("icons")).toStringList();
    }
    if (teamInfo.m_token.isEmpty()) {
        teamInfo.m_token = m_settings.value(QStringLiteral("token")).toString();
    }
    if (teamInfo.m_channel.isEmpty()) {
        teamInfo.m_channel = m_settings.value(QStringLiteral("lastChannel")).toString();
    }
    m_settings.endGroup();
}

void SlackConfig::saveTeamInfo(const TeamInfo &teamInfo)
{
    m_settings.beginGroup(teamInfo.teamId());
    m_settings.setValue(QStringLiteral("id"), teamInfo.teamId());
    m_settings.setValue(QStringLiteral("name"), teamInfo.name());
    m_settings.setValue(QStringLiteral("icons"), teamInfo.icons());
    m_settings.setValue(QStringLiteral("token"), teamInfo.teamToken());
    m_settings.setValue(QStringLiteral("lastChannel"), teamInfo.channel());
    m_settings.endGroup();
}

QStringList SlackConfig::teams()
{
    return m_settings.value(QStringLiteral("teamsList")).toStringList();
}

void SlackConfig::setTeams(const QStringList &teams)
{
    m_settings.setValue(QStringLiteral("teamsList"), teams);
}

void SlackConfig::clearWebViewCache()
{
    QStringList dataPaths = QStandardPaths::standardLocations(QStandardPaths::DataLocation);

    if (dataPaths.size()) {
        QDir webData(QDir(dataPaths.at(0)).filePath(QStringLiteral(".QtWebKit")));
        if (webData.exists()) {
            webData.removeRecursively();
        }
    }
}

SlackConfig *SlackConfig::instance()
{
    static SlackConfig slackConfig;
    return &slackConfig;
}
