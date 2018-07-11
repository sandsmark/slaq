#include "teaminfo.h"
#include "slackclientthreadspawner.h"

extern SlackClientThreadSpawner* g_slackThread;

TeamInfo::TeamInfo(QObject *parent): QObject(parent)
{
}

void TeamInfo::parseTeamInfoData(const QJsonObject &teamObj) {
    QString _id = teamObj.value(QStringLiteral("id")).toString();

    setTeamId(_id);
    setName(teamObj.value(QStringLiteral("name")).toString());
    setDomain(teamObj.value(QStringLiteral("domain")).toString());
    setEmailDomain(teamObj.value(QStringLiteral("email_domain")).toString());
    setEnterpriseId(teamObj.value(QStringLiteral("enterprise_id")).toString());
    setEnterpriseName(teamObj.value(QStringLiteral("enterprise_name")).toString());
    QJsonObject teamIconObj = teamObj.value(QStringLiteral("icon")).toObject();
    setIcons(QStringList() << teamIconObj.value(QStringLiteral("image_34")).toString()
    << teamIconObj.value(QStringLiteral("image_44")).toString()
    << teamIconObj.value(QStringLiteral("image_68")).toString()
    << teamIconObj.value(QStringLiteral("image_88")).toString()
    << teamIconObj.value(QStringLiteral("image_102")).toString()
    << teamIconObj.value(QStringLiteral("image_132")).toString()
    << teamIconObj.value(QStringLiteral("image_230")).toString()
    << teamIconObj.value(QStringLiteral("image_original")).toString());
}

void TeamInfo::parseSelfData(const QJsonObject &selfObj) {
    m_selfId = selfObj.value(QStringLiteral("id")).toString();
}

void TeamInfo::addTeamData(const QJsonObject &teamData)
{
    qDebug() << "start" << __FUNCTION__;

    parseTeamInfoData(teamData.value("team").toObject());
    parseSelfData(teamData.value("self").toObject());
    m_users = new UsersModel;
    m_users->addUsers(teamData.value(QStringLiteral("users")).toArray());
    m_users->addUsers(teamData.value(QStringLiteral("bots")).toArray());

    m_chats = new ChatsModel(m_selfId, nullptr, m_users);
    m_chats->addChats(teamData.value(QStringLiteral("channels")).toArray(), ChatsModel::Channel);
    m_chats->addChats(teamData.value(QStringLiteral("groups")).toArray(), ChatsModel::Group);
    m_chats->addChats(teamData.value(QStringLiteral("ims")).toArray(), ChatsModel::Conversation);

    qDebug() << "Chats count" << m_chats->rowCount();

    QQmlEngine::setObjectOwnership(m_users, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_chats, QQmlEngine::CppOwnership);
}

QString TeamInfo::selfId() const
{
    return m_selfId;
}

ChatsModel *TeamInfo::chats() const
{
    return m_chats;
}

UsersModel *TeamInfo::users() const
{
    return m_users;
}

void TeamInfo::setTeamId(const QString &teamId)
{
    if (m_teamId == teamId)
        return;

    m_teamId = teamId;
    emit teamIdChanged(m_teamId);
}

void TeamInfo::setName(const QString &name)
{
    if (m_name == name)
        return;

    m_name = name;
    emit nameChanged(m_name);
}

void TeamInfo::setDomain(const QString &domain)
{
    if (m_domain == domain)
        return;

    m_domain = domain;
    emit domainChanged(m_domain);
}

void TeamInfo::setEmailDomain(const QString &emailDomain)
{
    if (m_emailDomain == emailDomain)
        return;

    m_emailDomain = emailDomain;
    emit emailDomainChanged(m_emailDomain);
}

void TeamInfo::setIcons(const QStringList &icons)
{
    if (m_icons == icons)
        return;

    m_icons = icons;
    emit iconsChanged(m_icons);
}

void TeamInfo::setImageDefault(bool imageDefault)
{
    if (m_imageDefault == imageDefault)
        return;

    m_imageDefault = imageDefault;
    emit imageDefaultChanged(m_imageDefault);
}

void TeamInfo::setEnterpriseId(const QString &enterpriseId)
{
    if (m_enterpriseId == enterpriseId)
        return;

    m_enterpriseId = enterpriseId;
    emit enterpriseIdChanged(m_enterpriseId);
}

void TeamInfo::setEnterpriseName(const QString &enterpriseName)
{
    if (m_enterpriseName == enterpriseName)
        return;

    m_enterpriseName = enterpriseName;
    emit enterpriseNameChanged(m_enterpriseName);
}

void TeamInfo::setTeamToken(const QString &teamToken) {
    if (m_teamToken == teamToken)
        return;

    m_teamToken = teamToken;
    emit teamTokenChanged(m_teamToken);
}

void TeamInfo::setLastChannel(const QString &lastChannel)
{
    if (m_lastChannel == lastChannel)
        return;

    m_lastChannel = lastChannel;
    emit lastChannelChanged(m_lastChannel);
}
