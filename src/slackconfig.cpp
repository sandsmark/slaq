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

QList<TeamInfo *> SlackConfig::teams()
{
    QList<TeamInfo *> list;
    int size = m_settings.beginReadArray(QStringLiteral("teams"));
     for (int i = 0; i < size; ++i) {
         m_settings.setArrayIndex(i);
         TeamInfo * teaminfo = new TeamInfo;
         teaminfo->m_id = m_settings.value(QStringLiteral("id")).toString();
         teaminfo->m_name = m_settings.value(QStringLiteral("name")).toString();
         teaminfo->m_icons = m_settings.value(QStringLiteral("icons")).toStringList();
         teaminfo->m_token = m_settings.value(QStringLiteral("token")).toString();
         teaminfo->m_channel = m_settings.value(QStringLiteral("lastChannel")).toString();
         list.append(teaminfo);
     }
     m_settings.endArray();
    return list;
}

void SlackConfig::setTeams(const QList<TeamInfo *> &list)
{
    m_settings.beginWriteArray(QStringLiteral("teams"));
     for (int i = 0; i < list.size(); ++i) {
         m_settings.setArrayIndex(i);
         m_settings.setValue(QStringLiteral("id"), list.at(i)->teamId());
         m_settings.setValue(QStringLiteral("name"), list.at(i)->name());
         m_settings.setValue(QStringLiteral("icons"), list.at(i)->icons());
         m_settings.setValue(QStringLiteral("token"), list.at(i)->teamToken());
         m_settings.setValue(QStringLiteral("lastChannel"), list.at(i)->channel());
     }
     m_settings.endArray();
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
