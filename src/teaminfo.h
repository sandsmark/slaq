#ifndef TEAMINFO_H
#define TEAMINFO_H

#include <QString>
#include <QObject>
#include <QtQml>

class TeamInfo {
    Q_GADGET

    Q_PROPERTY(QString teamId READ teamId CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString domain READ domain CONSTANT)
    Q_PROPERTY(QString email_domain READ email_domain CONSTANT)
    Q_PROPERTY(QStringList icons READ icons CONSTANT)
    Q_PROPERTY(bool image_default READ image_default CONSTANT)
    Q_PROPERTY(QString enterprise_id READ enterprise_id CONSTANT)
    Q_PROPERTY(QString enterprise_name READ enterprise_name CONSTANT)
    Q_PROPERTY(QString teamToken READ teamToken CONSTANT)
    Q_PROPERTY(QString channel READ channel CONSTANT)

public:
    TeamInfo& operator=(const TeamInfo& other) // copy assignment
    {
        if (this != &other) { // self-assignment check expected
            if (other.m_id != m_id) {
                m_id = other.m_id;
                m_name = other.m_name;
                m_domain = other.m_domain;
                m_email_domain = other.m_email_domain;
                m_icons = other.m_icons;
                m_image_default = other.m_image_default;
                m_enterprise_id = other.m_enterprise_id;
                m_enterprise_name = other.m_enterprise_name;
                m_token = other.m_token;
                m_channel = other.m_channel;
            }
        }
        return *this;
    }

    bool operator==(const TeamInfo &rhs) const {
        return rhs.m_id == m_id;
    }

    bool operator!=(const TeamInfo &rhs) const {
        return rhs.m_id != m_id;
    }

    QString m_id;
    QString m_name;
    QString m_domain;
    QString m_email_domain;
    QStringList m_icons;
    bool m_image_default { false };
    QString m_enterprise_id;
    QString m_enterprise_name;
    QString m_token;
    QString m_channel;

    QString teamId() const { return m_id; }
    QString name() const { return m_name; }
    QString domain() const { return m_domain; }
    QString email_domain() const { return m_email_domain; }
    QStringList icons() const { return m_icons; }
    bool image_default() const { return m_image_default; }
    QString enterprise_id() const { return m_enterprise_id; }
    QString enterprise_name() const { return m_enterprise_name; }
    QString teamToken() const { return m_token; }
    QString channel() const { return m_channel; }
};

QML_DECLARE_TYPE(TeamInfo)

#endif // TEAMINFO_H
