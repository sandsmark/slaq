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
    if (teamInfo.teamId().isEmpty()) {
        teamInfo.setTeamId(m_settings.value(QStringLiteral("id")).toString());
    }
    if (teamInfo.name().isEmpty()) {
        teamInfo.setName(m_settings.value(QStringLiteral("name")).toString());
    }
    if (teamInfo.icons().isEmpty()) {
        teamInfo.setIcons(m_settings.value(QStringLiteral("icons")).toStringList());
    }
    if (teamInfo.teamToken().isEmpty()) {
        teamInfo.setTeamToken(m_settings.value(QStringLiteral("token")).toString());
    }
    if (teamInfo.lastChannel().isEmpty()) {
        teamInfo.setLastChannel(m_settings.value(QStringLiteral("lastChannel")).toString());
    }
    m_settings.endGroup();
    m_teamsTokens[teamInfo.teamId()] = teamInfo.teamToken();
}

void SlackConfig::saveTeamInfo(const TeamInfo &teamInfo)
{
    if (teamInfo.teamId().isEmpty()) {
        qWarning()  << "team id EMPTY!!!!!";
    }
    m_settings.beginGroup(teamInfo.teamId());
    m_settings.setValue(QStringLiteral("id"), teamInfo.teamId());
    m_settings.setValue(QStringLiteral("name"), teamInfo.name());
    m_settings.setValue(QStringLiteral("icons"), teamInfo.icons());
    m_settings.setValue(QStringLiteral("token"), teamInfo.teamToken());
    m_settings.setValue(QStringLiteral("lastChannel"), teamInfo.lastChannel());
    m_settings.endGroup();
    m_teamsTokens[teamInfo.teamId()] = teamInfo.teamToken();
}

QStringList SlackConfig::teams()
{
    return m_settings.value(QStringLiteral("teamsList")).toStringList();
}

void SlackConfig::setTeams(const QStringList &teams)
{
    m_settings.setValue(QStringLiteral("teamsList"), teams);
}

QString SlackConfig::accessToken(const QString &teamId)
{
    return m_teamsTokens.value(teamId);
}

void SlackConfig::clearWebViewCache()
{
    QStringList dataPaths = QStandardPaths::standardLocations(QStandardPaths::DataLocation);

    if (!dataPaths.isEmpty()) {
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
