#ifndef TEAMINFO_H
#define TEAMINFO_H

#include <QString>
#include <QObject>
#include <QtQml>

class TeamInfo: public QObject {
    Q_OBJECT

    Q_PROPERTY(QString teamId READ teamId WRITE setTeamId NOTIFY teamIdChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString domain READ domain WRITE setDomain NOTIFY domainChanged)
    Q_PROPERTY(QString emailDomain READ emailDomain WRITE setEmailDomain NOTIFY emailDomainChanged)
    Q_PROPERTY(QStringList icons READ icons WRITE setIcons NOTIFY iconsChanged)
    Q_PROPERTY(bool imageDefault READ imageDefault WRITE setImageDefault NOTIFY imageDefaultChanged)
    Q_PROPERTY(QString enterpriseId READ enterpriseId WRITE setEnterpriseId NOTIFY enterpriseIdChanged)
    Q_PROPERTY(QString enterpriseName READ enterpriseName WRITE setEnterpriseName NOTIFY enterpriseNameChanged)
    Q_PROPERTY(QString teamToken READ teamToken WRITE setTeamToken NOTIFY teamTokenChanged)
    Q_PROPERTY(QString lastChannel READ lastChannel WRITE setLastChannel NOTIFY lastCHannelChanged)

public:
    explicit TeamInfo(QObject *parent = nullptr): QObject(parent)
    {
    }

    bool operator==(const TeamInfo &rhs) const {
        return rhs.m_teamId == m_teamId;
    }

    bool operator!=(const TeamInfo &rhs) const {
        return rhs.m_teamId != m_teamId;
    }

    QString teamId() const { return m_teamId; }
    QString name() const { return m_name; }
    QString domain() const { return m_domain; }
    QStringList icons() const { return m_icons; }
    bool image_default() const { return m_image_default; }
    QString teamToken() const { return m_teamToken; }
    QString emailDomain() const { return m_emailDomain; }
    bool imageDefault() const { return m_imageDefault; }
    QString enterpriseId() const { return m_enterpriseId; }
    QString enterpriseName() const { return m_enterpriseName; }
    QString lastChannel() const { return m_lastChannel; }

public slots:
    void setTeamId(const QString& teamId)
    {
        if (m_teamId == teamId)
            return;

        m_teamId = teamId;
        emit teamIdChanged(m_teamId);
    }

    void setName(const QString& name)
    {
        if (m_name == name)
            return;

        m_name = name;
        emit nameChanged(m_name);
    }

    void setDomain(const QString& domain)
    {
        if (m_domain == domain)
            return;

        m_domain = domain;
        emit domainChanged(m_domain);
    }

    void setEmailDomain(const QString& emailDomain)
    {
        if (m_emailDomain == emailDomain)
            return;

        m_emailDomain = emailDomain;
        emit emailDomainChanged(m_emailDomain);
    }

    void setIcons(const QStringList& icons)
    {
        if (m_icons == icons)
            return;

        m_icons = icons;
        emit iconsChanged(m_icons);
    }

    void setImageDefault(bool imageDefault)
    {
        if (m_imageDefault == imageDefault)
            return;

        m_imageDefault = imageDefault;
        emit imageDefaultChanged(m_imageDefault);
    }

    void setEnterpriseId(const QString& enterpriseId)
    {
        if (m_enterpriseId == enterpriseId)
            return;

        m_enterpriseId = enterpriseId;
        emit enterpriseIdChanged(m_enterpriseId);
    }

    void setEnterpriseName(const QString& enterpriseName)
    {
        if (m_enterpriseName == enterpriseName)
            return;

        m_enterpriseName = enterpriseName;
        emit enterpriseNameChanged(m_enterpriseName);
    }

    void setTeamToken(const QString& teamToken) {
        if (m_teamToken == teamToken)
            return;

        m_teamToken = teamToken;
        emit teamTokenChanged(m_teamToken);
    }


    void setLastChannel(const QString& lastChannel)
    {
        if (m_lastChannel == lastChannel)
            return;

        m_lastChannel = lastChannel;
        emit lastCHannelChanged(m_lastChannel);
    }

signals:
    void teamIdChanged(QString teamId);
    void nameChanged(QString name);
    void domainChanged(QString domain);
    void emailDomainChanged(QString emailDomain);
    void iconsChanged(QStringList icons);
    void imageDefaultChanged(bool imageDefault);
    void enterpriseIdChanged(QString enterpriseId);
    void enterpriseNameChanged(QString enterpriseName);
    void teamTokenChanged(QString teamToken);
    void lastCHannelChanged(QString lastChannel);

private:
    QString m_name;
    QString m_domain;
    QString m_emailDomain;
    QStringList m_icons;
    bool m_image_default { false };
    QString m_token;
    QString m_teamId;
    bool m_imageDefault;
    QString m_enterpriseId;
    QString m_enterpriseName;
    QString m_teamToken;
    QString m_lastChannel;
};

QML_DECLARE_TYPE(TeamInfo)

#endif // TEAMINFO_H
