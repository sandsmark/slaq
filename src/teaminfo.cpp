#include "teaminfo.h"
#include "slackclientthreadspawner.h"
#include "searchmessagesmodel.h"

TeamInfo::TeamInfo(QObject *parent): QObject(parent) {}

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

void TeamInfo::addUsersData(const QList<QPointer<User>>& users, bool last)
{
    //qDebug() << "start" << __PRETTY_FUNCTION__;
#if 0
        {
            QFile f("userlist_dumps_" + name() + ".json");
            if (f.open(QIODevice::Append)) {
                f.write(QJsonDocument(usersData).toJson());
                f.close();
            }
        }
#endif
    m_users->addUsers(users, last);
    if (last) {
        m_users->setUsersFetched(true);
    }
}

void TeamInfo::addConversationsData(const QList<Chat*>& chats, bool last)
{
    if (m_chats == nullptr) {
        return;
    }
    m_chats->addChats(chats, last);
}

bool TeamInfo::teamsEmojisUpdated() const
{
    return m_teamsEmojisUpdated;
}

void TeamInfo::setTeamsEmojisUpdated(bool teamsEmojisUpdated)
{
    m_teamsEmojisUpdated = teamsEmojisUpdated;
}

void TeamInfo::createModels(SlackTeamClient *slackClient)
{
    m_users = new UsersModel;
    QQmlEngine::setObjectOwnership(m_users, QQmlEngine::CppOwnership);
    m_searchMessages = new SearchMessagesModel(nullptr, m_users, "SEARCH");
    QQmlEngine::setObjectOwnership(m_searchMessages, QQmlEngine::CppOwnership);
    m_chats = new ChatsModel(m_selfId, nullptr, m_users);
    QQmlEngine::setObjectOwnership(m_chats, QQmlEngine::CppOwnership);
    connect(m_users, &UsersModel::requestUserInfo, slackClient, &SlackTeamClient::requestUserInfo, Qt::QueuedConnection);
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

SearchMessagesModel *TeamInfo::searches() const
{
    return m_searchMessages;
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
