#ifndef SLACKCONFIG_H
#define SLACKCONFIG_H

#include <QObject>
#include <QSettings>

class TeamInfo;

class SlackConfig : public QObject
{
    Q_OBJECT
public:

    QString userId();
    void setUserInfo(const QString &userId, const QString &teamId, const QString &teamName);
    QList<TeamInfo*> teams();
    void setTeams(const QList<TeamInfo*>& list);

    static void clearWebViewCache();
    static SlackConfig *instance();

    Q_INVOKABLE QString teamId() { return m_currentTeamId; }
    Q_INVOKABLE QString teamName() { return m_currentTeamName; }

    Q_INVOKABLE bool hasUserInfo() { return (!m_currentUserId.isEmpty()
                                 && !m_currentTeamId.isEmpty()
                                 && !m_currentTeamName.isEmpty());
    }

signals:

public slots:
private:
    explicit SlackConfig(QObject *parent = nullptr);

private:
    QSettings m_settings;
    QString m_currentUserId;
    QString m_currentTeamId;
    QString m_currentTeamName;
};

#endif // SLACKCONFIG_H
