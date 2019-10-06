#pragma once

#include <QObject>
#include <QSettings>
#include <QPointer>
#include <QNetworkCookieJar>

class TeamInfo;

struct CookieJar : public QNetworkCookieJar
{

    // QNetworkCookieJar interface
public:
    CookieJar(QObject *parent = nullptr);

    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url) override {
        const bool ret = QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
        saveCookies();
        return ret;
    }

    bool insertCookie(const QNetworkCookie &cookie) override {
        const bool ret = QNetworkCookieJar::insertCookie(cookie);
        if (m_initialized) {
            saveCookies();
        }
        return ret;
    }

    bool updateCookie(const QNetworkCookie &cookie) override {
        const bool ret = QNetworkCookieJar::updateCookie(cookie);
        saveCookies();
        return ret;
    }

    bool deleteCookie(const QNetworkCookie &cookie) override {
        const bool ret = QNetworkCookieJar::deleteCookie(cookie);
        saveCookies();
        return ret;
    }

    void saveCookies();

private:
    bool m_initialized = false;
};

class SlackConfig : public QObject
{
    Q_OBJECT

public:
    ~SlackConfig();

    QString userId();
    void setUserInfo(const QString &userId, const QString &teamId, const QString &teamName);
    void loadTeamInfo(TeamInfo& teamInfo);
    void saveTeamInfo(const TeamInfo& teamInfo);

    QPointer<CookieJar> cookieJar;

    static void clearWebViewCache();
    static SlackConfig *instance();

    Q_INVOKABLE QString teamId() { return m_currentTeamId; }
    Q_INVOKABLE QString teamName() { return m_currentTeamName; }

    Q_INVOKABLE bool hasUserInfo() { return (!m_currentUserId.isEmpty()
                                 && !m_currentTeamId.isEmpty()
                                 && !m_currentTeamName.isEmpty());
    }

    QStringList teams();
    void setTeams(const QStringList& teams);
    QString accessToken(const QString& teamId);

public slots:
    void onCookieAdded(const QNetworkCookie &cookie);

private:
    explicit SlackConfig(QObject *parent = nullptr);

private:
    QSettings m_settings;
    QString m_currentUserId;
    QString m_currentTeamId;
    QString m_currentTeamName;
    QHash<QString, QString> m_teamsTokens;
};

