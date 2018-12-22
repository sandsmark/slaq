#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaContent>

#include "slackclientthreadspawner.h"
#include "teaminfo.h"
#include "searchmessagesmodel.h"
#include "debugblock.h"

#include "qgumbodocument.h"
#include <qgumbonode.h>
#include <qgumboattribute.h>

const QString apiTokenString   = "api_token: \"";
const QString logOutURLString  = "boot_data.logout_url = \"";
const QString versionTSstring  = "version_ts: \"";
const QString versionUIDstring = "version_uid: \"";
const QString teamIDstring = "team_id: '";

SlackClientThreadSpawner::SlackClientThreadSpawner(QObject *parent) :
    QThread(parent)
{
    QSettings settings;
    m_lastTeam = settings.value(QStringLiteral("LastTeam")).toString();
    m_buildTime.setDate(QDate::fromString(QString(__DATE__).simplified(), "MMM d yyyy"));
    m_buildTime.setTime(QTime::fromString(QString(__TIME__), "hh:mm:ss"));
    qDebug() << "build time" << m_buildTime;
}

SlackClientThreadSpawner::~SlackClientThreadSpawner() {}

SlackTeamClient *SlackClientThreadSpawner::slackClient(const QString &teamId) {
    //qDebug() << "slackClient" << teamId << m_knownTeams.value(teamId);
    return m_knownTeams.value(teamId, nullptr);
}

SlackUser *SlackClientThreadSpawner::selfUser(const QString &teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return nullptr;
    }
    return _slackClient->teamInfo()->selfUser();
}

UsersModel *SlackClientThreadSpawner::usersModel(const QString &teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return nullptr;
    }
    return _slackClient->teamInfo()->users();
}

ChatsModel *SlackClientThreadSpawner::chatsModel(const QString &teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return nullptr;
    }
    return _slackClient->teamInfo()->chats();
}

SlackTeamClient::ClientStatus SlackClientThreadSpawner::slackClientStatus(const QString &teamId)
{
    SlackTeamClient::ClientStatus retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return SlackTeamClient::UNDEFINED;
    }
    QMetaObject::invokeMethod(_slackClient, "getStatus", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(SlackTeamClient::ClientStatus, retVal));
    return retVal;
}

QQmlObjectListModel<TeamInfo> *SlackClientThreadSpawner::teamsModel()
{
    return &m_teamsModel;
}

QString SlackClientThreadSpawner::lastTeam() const
{
    return m_lastTeam;
}

bool SlackClientThreadSpawner::isOnline()
{
    return slackClient(m_lastTeam) ? slackClient(m_lastTeam)->isOnline() : false;
}

void SlackClientThreadSpawner::startClient(const QString& teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "startClient", Qt::QueuedConnection);
}

void SlackClientThreadSpawner::testLogin(const QString& teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "testLogin", Qt::QueuedConnection);
}

void SlackClientThreadSpawner::setAppActive(const QString& teamId, bool active)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "setAppActive", Qt::QueuedConnection,
                              Q_ARG(bool, active));
}

void SlackClientThreadSpawner::setActiveWindow(const QString& teamId, const QString &windowId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "setActiveWindow", Qt::QueuedConnection,
                              Q_ARG(QString, windowId));
}


void SlackClientThreadSpawner::searchMessages(const QString &teamId, const QString &searchString, int page)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    //cleanup previous search
    _slackClient->teamInfo()->searches()->setSearchQuery("");
    emit searchStarted();
    QMetaObject::invokeMethod(_slackClient, "searchMessages", Qt::QueuedConnection,
                              Q_ARG(QString, searchString),
                              Q_ARG(int, page));
}

QStringList SlackClientThreadSpawner::getNickSuggestions(const QString& teamId, const QString &currentText, const int cursorPosition)
{
    QStringList retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "getNickSuggestions", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QStringList, retVal),
                              Q_ARG(QString, currentText),
                              Q_ARG(int, cursorPosition));
    return retVal;
}

void SlackClientThreadSpawner::loadMessages(const QString& teamId, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "loadMessages", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

bool SlackClientThreadSpawner::isDevice() const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

QString SlackClientThreadSpawner::lastChannel(const QString& teamId)
{
    QString retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "lastChannel", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString, retVal));
    return retVal;
}

QString SlackClientThreadSpawner::userName(const QString& teamId, const QString &userId)
{
    QString retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "userName", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString, retVal),
                              Q_ARG(QString, userId));
    return retVal;
}

bool SlackClientThreadSpawner::handleAccessTokenReply(const QJsonObject &bootData)
{
    //qDebug().noquote() << "boot data:" <<QJsonDocument(bootData).toJson() << QThread::currentThread();
    QString _accessToken = bootData[QStringLiteral("api_token")].toString();
    if (_accessToken.isEmpty()) {
        qWarning() << "Missing access token";
        return false;
    }

    QString _teamId = bootData[QStringLiteral("team_id")].toString();
    if (_teamId.isEmpty()) {
        qWarning() << "Missing team id";
        return false;
    }
#if 0
    {
        QFile f(_teamId + "_boot.json");
        f.open(QFile::WriteOnly);
        f.write(QJsonDocument(bootData).toJson());
        f.close();
    }
#endif
    QString _userId = bootData[QStringLiteral("user_id")].toString();
    if (_userId.isEmpty()) {
        qWarning() << "Missing user id";
        return false;
    }

    QString _teamName = bootData[QStringLiteral("team_url")].toString();
    if (_teamName.isEmpty()) {
        qWarning() << "Missing team name";
        return false;
    }

    qDebug() << "Access token success" << _accessToken << _userId << _teamId << _teamName;

    setLastTeam(_teamId);
    QMetaObject::invokeMethod(m_threadExecutor, "connectToTeam", Qt::QueuedConnection,
                              Q_ARG(QString, _teamId),
                              Q_ARG(QString, _accessToken));
    return true;
}

void SlackClientThreadSpawner::createChat(const QString &teamId, const QString &channelName, bool isPrivate)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "createChat", Qt::QueuedConnection,
                              Q_ARG(QString, channelName),
                              Q_ARG(bool, isPrivate));
}

void SlackClientThreadSpawner::markChannel(const QString& teamId, ChatsModel::ChatType type, const QString &channelId, const QDateTime &time)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "markChannel", Qt::QueuedConnection,
                              Q_ARG(ChatsModel::ChatType, type),
                              Q_ARG(QString, channelId),
                              Q_ARG(QDateTime, time));
}

void SlackClientThreadSpawner::joinChannel(const QString& teamId, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "joinChannel", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::leaveChannel(const QString& teamId, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "leaveChannel", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::archiveChannel(const QString &teamId, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "archiveChannel", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::leaveGroup(const QString& teamId, const QString &groupId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "leaveGroup", Qt::QueuedConnection,
                              Q_ARG(QString, groupId));
}

QString SlackClientThreadSpawner::getChannelName(const QString &teamId, const QString &channelId)
{
    QString retVal;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return retVal;
    }
    QMetaObject::invokeMethod(_slackClient, "getChannelName", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString, retVal),
                              Q_ARG(QString, channelId));
    return retVal;
}

Chat* SlackClientThreadSpawner::getChannel(const QString &teamId, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return nullptr;
    }
    return _slackClient->getChannel(channelId);
}

void SlackClientThreadSpawner::dumpChannel(const QString &teamId, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    MessageListModel* msgs =_chatsModel->messages(channelId);
    if (msgs != nullptr) {
        msgs->modelDump();
    }
}

QString SlackClientThreadSpawner::resourceForFileType(const QString &fileType, const QString& fileName)
{
    QString _lowerFt = fileType;
    _lowerFt = _lowerFt.toLower();
    QFileInfo fi(fileName);
    fi.suffix();
    for (const QString& ftFile: m_fileTypesResDir.entryList()) {
        if (ftFile.contains(_lowerFt) || ftFile.contains(fi.suffix())) {
            return QStringLiteral("qrc:/icons/filetypes/") + ftFile;
        }
    }
    qDebug() << "not found file for type" << fileType;
    return QStringLiteral("qrc:/icons/filetypes/blank.svg");
}

QString SlackClientThreadSpawner::version() const
{
    return qApp->applicationVersion();
}

static QString parseInlineJsValue(const QByteArray& data, const QString& search, QChar end) {
    // get the index of the search value
    int i = data.indexOf(search);
    if (i < 0) {
        qWarning() << search << "not found";
        return "";
    }

    // b is the index of the beginning of the value we want
    int b = i + search.length();

    // get the index of the next double quote character, starting from b
    int ii = data.indexOf(end, b);
    if (ii < 0) {
        qWarning() << "did not find terminating byte in input" << end;
        return "";
    }

    return data.mid(b, ii - b);
}

void SlackClientThreadSpawner::getSessionDetails(const QString &url) {
    //now get session details
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:60.0) Gecko/20100101 Firefox/60.0");
    QNetworkReply * sessionDetailsReply = m_qnam.get(request);
    connect(sessionDetailsReply, &QNetworkReply::finished, [this, sessionDetailsReply]() {
        int httpCode = sessionDetailsReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray& data = sessionDetailsReply->readAll();
//        qDebug() << "session details:" << data;
//        qDebug() << "session details http code 2:" << httpCode;
        if (data.size() <= apiTokenString.size()) {
            showError("Slack login error", "Session data error",
                      "Input data too short to contain valid data");
            return;
        }

        // look for the api_token value within the response body buffer
        QString apiToken = parseInlineJsValue(data, apiTokenString, '"');
        if (apiToken.isEmpty()) {
            showError("Slack login error", "Session data error",
                      "Unable to find api token in response");
            return;
        }

        // look for the version_ts value within the response body buffer
        QString versionTS = parseInlineJsValue(data, versionTSstring, '"');
        if (versionTS.isEmpty()) {
            qWarning() << "unable to find version_ts in response";
        }

        // look for the version_uid value within the response body buffer
        QString versionUID = parseInlineJsValue(data, versionUIDstring, '"');
        if (versionUID.isEmpty()) {
            qWarning() << "unable to find version_uid in response";
        }

        QString teamID = parseInlineJsValue(data, teamIDstring, '\'');
        if (teamID.isEmpty()) {
            showError("Slack login error", "Session data error",
                      "Unable to find team id in response");
            return;
        }

        // get the logoutURL value; while noting its format is different than those above
        // because of this format difference we also need to do some later string formatting
        QString logoutURL = parseInlineJsValue(data, logOutURLString, ';');
        logoutURL = logoutURL.remove("+").remove("'").remove('"').replace("\\/", "/");
        if (logoutURL.isEmpty()) {
            qWarning() << "unable to find boot_data.logout_url in response";
        }
        qDebug() << "got slack api details" << teamID << apiToken << "\n" << versionTS << "\n" << versionUID << "\n" << logoutURL;
        delete sessionDetailsReply;
        // check if the team already connected
        if (m_knownTeams.value(teamID) != nullptr) {
            showError("Slack login error", "Team already connected!");
        } else {
            setLastTeam(teamID);
            QMetaObject::invokeMethod(this->m_threadExecutor, "connectToTeam", Qt::QueuedConnection,
                                      Q_ARG(QString, teamID),
                                      Q_ARG(QString, apiToken));
            emit accessTokenSuccess("", teamID, "");
        }
    });

}
void SlackClientThreadSpawner::loginAttempt(const QString &email, const QString &password,
                                            const QString &teamName)
{
    //getting login details
    QString _url = QString("https://%1.slack.com").arg(teamName);
    QNetworkRequest req(_url);
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false);
    req.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:60.0) Gecko/20100101 Firefox/60.0");

    QNetworkReply* reply = m_qnam.get(req);
    //qDebug() << "sending get to" << req.url() << _url;
    connect(reply, &QNetworkReply::finished, [this, reply, email, password, _url]() {
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "http error:" << reply->errorString();
        qDebug() << "http code:" << httpCode;
        if (httpCode == 302) {
            QUrl redirUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (redirUrl.toString().contains("/messages")) {
                this->getSessionDetails(_url + redirUrl.toString());
                //already logged in
            }
        } else if (httpCode == 200) { //not yet logged in
            QString crumb;
            QString signin;
            QString redir;
            QGumboDocument htmlDoc = QGumboDocument::parse(reply->readAll());
            const auto& root = htmlDoc.rootNode();
            const auto& nodes = root.getElementsByTagName(HtmlTag::INPUT);
            for (const auto& node: nodes) {
                //qDebug() << node.tagName() << node.getAttribute("crumb") << node.childNodes().size();
                bool isCrumb = false;
                bool isSignin = false;
                bool isRedir = false;
                for (const auto& attr: node.allAttributes()) {
                    //qDebug() << attr.name() << attr.value();
                    if (attr.name() == "name" && attr.value() == "crumb") {
                        isCrumb = true;
                    }
                    if (attr.name() == "name" && attr.value() == "signin") {
                        isSignin = true;
                    }
                    if (attr.name() == "name" && attr.value() == "redir") {
                        isRedir = true;
                    }
                    if (isCrumb && crumb.isEmpty() && attr.name() == "value") {
                        crumb = attr.value();
                    }
                    if (isSignin && signin.isEmpty() && attr.name() == "value") {
                        signin = attr.value();
                    }
                    if (isRedir && redir.isEmpty() && attr.name() == "value") {
                        redir = attr.value();
                    }
                }
            }
            //qDebug() << "found crumb" << crumb << signin << redir;
            delete reply;

            if (!crumb.isEmpty() && !signin.isEmpty()) {
                //login
                QUrlQuery query;
                query.addQueryItem(QStringLiteral("crumb"), crumb);
                query.addQueryItem(QStringLiteral("email"), email);
                query.addQueryItem(QStringLiteral("password"), password);
                query.addQueryItem(QStringLiteral("redir"), redir);
                query.addQueryItem(QStringLiteral("signin"), signin);

                QUrl params;
                params.setQuery(query);
                // replace '+' manually, since QUrl does not replacing it
                QByteArray body = params.toEncoded().replace('+', "%2B");
                body.remove(0, 1);

                QUrl url(_url);
                QNetworkRequest request(url);
                request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false);
                request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:60.0) Gecko/20100101 Firefox/60.0");
                request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
                request.setHeader(QNetworkRequest::ContentLengthHeader, body.length());

               // qDebug() << "POST" << url.toString() << body;
                QNetworkReply * loginReply = m_qnam.post(request, body);
                connect(loginReply, &QNetworkReply::finished, [this, loginReply, _url]() {
                    int httpCode = loginReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    QUrl redirUrl = loginReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                    const QByteArray& data = loginReply->readAll();
                    delete loginReply;
                    if (httpCode == 200 && data.contains("Sorry, you entered an incorrect email address or password.")) {
                        //login error. wrong credentials
                        qWarning() << "login error";
                        this->showError("Slack login error", "Invalid credentials");
                    } else if (httpCode == 302) {
                        //got smthg like: https://slack.com/checkcookie?redir=https%3A%2F%2Fqtmob.slack.com%2F
                        QNetworkRequest request(redirUrl);
                        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false);
                        request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:60.0) Gecko/20100101 Firefox/60.0");
                        QNetworkReply * loginReply1 = m_qnam.get(request);
                        connect(loginReply1, &QNetworkReply::finished, [this, loginReply1, _url]() {
                            int httpCode = loginReply1->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                            QUrl redirUrl = loginReply1->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                            delete loginReply1;
                            if (httpCode == 302) {
                                //now check if we redirected to /messages
                                QNetworkRequest request(redirUrl);
                                request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false);
                                request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:60.0) Gecko/20100101 Firefox/60.0");
                                QNetworkReply * loginReply2 = m_qnam.get(request);
                                connect(loginReply2, &QNetworkReply::finished, [this, loginReply2, _url]() {
                                    int httpCode = loginReply2->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                                    QUrl redirUrl = loginReply2->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                                    delete loginReply2;
                                    if (httpCode == 302) {
                                        this->getSessionDetails(_url + redirUrl.toString());
                                    } else {
                                        this->showError("Slack login error", "Undefined error");
                                    }
                                });
                            } else {
                                this->showError("Slack login error", "Undefined error");
                            }
                        });
                    } else {
                        this->showError("Slack login error", QString("Cant access slack.com. response code: %1").arg(httpCode));
                    }
                });
            } else {
                this->showError("Slack login error", "Cant retreive login info. Check team name");
            }
        } else {
            this->showError("Slack login error", "Cant retreive login info. Check team name");
        }
    });
}

int SlackClientThreadSpawner::getTotalUnread(const QString &teamId, ChatsModel::ChatType type, bool personal)
{
    int total = 0;
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return 0;
    }
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return 0;
    }
    if (_chatsModel->rowCount() > 0) {
        for (int i = 0; i < _chatsModel->rowCount(); i++) {
            Chat* chat = _chatsModel->chat(i);
            if (chat == nullptr) {
                qWarning() << "Chat for not found";
                continue;
            }
            if (chat->type == type) {
                if (personal) {
                    total += chat->unreadCountPersonal;
                } else {
                    total += chat->unreadCountDisplay;
                }
            }
        }
    } else {
        total = _chatsModel->unreadsInNull(type, personal);
    }
    return total;
}

Chat *SlackClientThreadSpawner::getGeneralChannel(const QString &teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return nullptr;
    }
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return nullptr;
    }
    return _chatsModel->generalChat();
}

void SlackClientThreadSpawner::postMessage(const QString& teamId, const QString &channelId, const QString& content, const QString &thread_ts)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "postMessage", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, content),
                              Q_ARG(QString, thread_ts));
}

void SlackClientThreadSpawner::updateMessage(const QString &teamId, const QString &channelId, const QString &content,
                                             const QDateTime &ts, const QString &slackTs)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "updateMessage", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, content),
                              Q_ARG(QDateTime, ts),
                              Q_ARG(QString, slackTs));
}

void SlackClientThreadSpawner::deleteMessage(const QString &teamId, const QString &channelId,
                                             const QDateTime &ts, const QString& slackTs)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "deleteMessage", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QDateTime, ts),
                              Q_ARG(QString, slackTs));
}

void SlackClientThreadSpawner::postFile(const QString& teamId, const QString &channelId, const QString &filePath, const QString &title, const QString &comment)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "postFile", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, filePath),
                              Q_ARG(QString, title),
                              Q_ARG(QString, comment));
}

void SlackClientThreadSpawner::deleteReaction(const QString& teamId, const QString &channelId, const QDateTime &ts, const QString &reaction)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "deleteReaction", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QDateTime, ts),
                              Q_ARG(QString, reaction));
}

void SlackClientThreadSpawner::addReaction(const QString& teamId, const QString &channelId,
                                           const QDateTime &ts, const QString &reaction,
                                           const QString &slackTs)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "addReaction", Qt::QueuedConnection,
                              Q_ARG(QString, channelId),
                              Q_ARG(QDateTime, ts),
                              Q_ARG(QString, reaction),
                              Q_ARG(QString, slackTs));
}

inline bool SlackClientThreadSpawner::checkForPersonal(const QString& msg, SlackUser* selfUser) {
    return (selfUser != nullptr && (msg.contains(selfUser->userId())
                                    || msg.contains(selfUser->username())
                                    || msg.contains("<!here")
                                    || msg.contains("<!channel")
                                    || msg.contains("<!group")
                                    || msg.contains("<!everyone")));
}

void SlackClientThreadSpawner::onMessageReceived(Message *message)
{
    auto _slackClient = static_cast<SlackTeamClient*>(sender());

    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();

    if (_chatsModel == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "No chats";
        delete message;
        return;
    }

    Chat* chat = _chatsModel->chat(message->channel_id);

    //qDebug() << "message" << message->text << checkForPersonal(message->text, _slackClient->teamInfo()->selfUser());
    if (chat != nullptr && !chat->id.isEmpty() && message->time > chat->lastRead) {
        if (checkForPersonal(message->text, _slackClient->teamInfo()->selfUser())) {
            chat->unreadCountPersonal++;
        }
        if (message->subtype != "message_changed") {
            chat->unreadCountDisplay++;
            _chatsModel->chatChanged(chat);
            emit channelCountersUpdated(_slackClient->teamInfo()->teamId(), chat->id,
                                        chat->unreadCountDisplay, chat->unreadCountPersonal);
        }
    } else {
        if (!message->channel_id.isEmpty()) {
            qWarning() << __PRETTY_FUNCTION__ << "Chat for channel ID" << message->channel_id << "not found";
            _chatsModel->increaseUnreadsInNull(message->channel_id,
                                               checkForPersonal(message->text, _slackClient->teamInfo()->selfUser()));
            emit channelCountersUpdated(_slackClient->teamInfo()->teamId(),
                                        message->channel_id,
                                        _chatsModel->unreadsInNullChannel(message->channel_id, false),
                                        _chatsModel->unreadsInNullChannel(message->channel_id, true));
        }
    }
    MessageListModel *messages = _chatsModel->messages(message->channel_id);
    if (messages != nullptr) {
        messages->addMessage(message);
    } else {
        qWarning() << "No messages in chat" << message->channel_id;
        delete message;
    }
}

void SlackClientThreadSpawner::onMessagesReceived(const QString& channelId, const QList<Message*>& messages, bool hasMore, const QString &threadTs)
{
    auto _slackClient = static_cast<SlackTeamClient*>(sender());

    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();

    if (_chatsModel == nullptr) {
        qWarning() << "No chats";
        return;
    }
    MessageListModel *messagesModel = _chatsModel->messages(channelId);
    if (messagesModel == nullptr) {
        qWarning() << "No messages in chat" << channelId;
        return;
    }
    Chat* _chat = _chatsModel->chat(channelId);
    QDateTime _lastRead;
    if (_chat != nullptr) {
        qDebug() << "Adding messages for chat" << _chat->name;
        _lastRead = _chat->lastRead;
    }

    messagesModel->addMessages(messages, hasMore, threadTs);
    qDebug() << "unread messages" << messagesModel->countUnread(_lastRead);
}

void SlackClientThreadSpawner::sendUserTyping(const QString &teamId, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "sendUserTyping", Qt::QueuedConnection,
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::updateUserInfo(const QString &teamId, SlackUser *user)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "updateUserInfo", Qt::QueuedConnection,
                              Q_ARG(SlackUser*, user));
}

void SlackClientThreadSpawner::updateUserAvatar(const QString &teamId, const QString &filePath, int cropSide, int cropX, int cropY)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "updateUserAvatar", Qt::QueuedConnection,
                              Q_ARG(QString, filePath),
                              Q_ARG(int, cropSide),
                              Q_ARG(int, cropX),
                              Q_ARG(int, cropY));
}

void SlackClientThreadSpawner::setPresence(const QString &teamId, bool isAway)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "setPresence", Qt::QueuedConnection,
                              Q_ARG(bool, isAway));
}

void SlackClientThreadSpawner::setDnD(const QString &teamId, int minutes)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "setDnD", Qt::QueuedConnection,
                              Q_ARG(int, minutes));
}

void SlackClientThreadSpawner::cancelDnD(const QString &teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "cancelDnD", Qt::QueuedConnection);
}

void SlackClientThreadSpawner::onFileSharesReceived(const QList<FileShare *> &shares, int total, int page, int pages)
{
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());

    TeamInfo* teamInfo = _slackClient->teamInfo();
    ChatsModel* _chatsModel = teamInfo->chats();

    if (_chatsModel == nullptr) {
        qWarning() << "No chats";
        return;
    }
    FilesSharesModel* _filesShares = teamInfo->fileSharesModel();

    _filesShares->addFileShares(shares, total, page, pages);
}

void SlackClientThreadSpawner::deleteFile(const QString &teamId, const QString &fileId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "deleteFile", Qt::QueuedConnection, Q_ARG(QString, fileId));
}

void SlackClientThreadSpawner::clearSettingsAndRestartApp(bool clear)
{
    QSettings s;
    if (clear) {
        s.clear();
    }
    s.sync();
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}

void SlackClientThreadSpawner::showError(const QString &errorDomain, const QString &errorMessage,
                                         const QString &errorDetails, int timeout)
{
    QJsonObject errorData;
    errorData["domain"] = errorDomain;
    errorData["error_str"] = errorMessage;
    errorData["details"] = errorDetails;
    errorData["timeout"] = timeout;
    emit error(errorData);
}

void SlackClientThreadSpawner::onMessageUpdated(Message *message, bool replace)
{
    DEBUG_BLOCK;
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    MessageListModel *messages = _chatsModel->messages(message->channel_id);
    if (messages == nullptr) {
        qWarning() << "No messages in chat" << message->channel_id;
        return;
    }
    messages->updateMessage(message, replace);
}

void SlackClientThreadSpawner::onMessageDeleted(const QString &channelId, const QDateTime &ts)
{
    DEBUG_BLOCK;
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    MessageListModel *messages = _chatsModel->messages(channelId);
    if (messages == nullptr) {
        qWarning() << "No messages in chat" << channelId;
        return;
    }
    messages->deleteMessage(ts);
}

void SlackClientThreadSpawner::onChannelUpdated(Chat* chat)
{
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }

    _chatsModel->chatChanged(chat);
    emit channelCountersUpdated(_slackClient->teamInfo()->teamId(), chat->id,
                                chat->unreadCountDisplay, chat->unreadCountPersonal);
}

void SlackClientThreadSpawner::onChannelJoined(Chat* chat)
{
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    QString channelId = _chatsModel->addChat(chat);
    connect(chat->messagesModel.data(), &MessageListModel::fetchMoreMessages,
            _slackClient, &SlackTeamClient::loadMessages, Qt::QueuedConnection);
    emit channelJoined(_slackClient->teamInfo()->teamId(), channelId);
}

void SlackClientThreadSpawner::onChannelLeft(const QString &channelId)
{
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    Chat* chat = _chatsModel->chat(channelId);
    if (chat == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "Chat for channel ID" << channelId << "not found";
        return;
    }
    disconnect(chat->messagesModel.data(), &MessageListModel::fetchMoreMessages,
               _slackClient, &SlackTeamClient::loadMessages);
    chat->isOpen = false;
    _chatsModel->chatChanged(chat);
    emit channelLeft(_slackClient->teamInfo()->teamId(), channelId);
}

void SlackClientThreadSpawner::onSearchMessagesReceived(const QJsonArray& messages, int total,
                                                        const QString& query, int page, int pages)
{
    qDebug() << "onSearchMessagesReceived thread" << QThread::currentThreadId();
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());

    TeamInfo* teamInfo = _slackClient->teamInfo();
    ChatsModel* _chatsModel = teamInfo->chats();

    if (_chatsModel == nullptr) {
        qWarning() << "No chats";
        return;
    }
    SearchMessagesModel* _searchMessages = teamInfo->searches();

    if (_searchMessages->searchQuery() != query) { //new query, cleanup
        _searchMessages->setSearchQuery(query);
        _searchMessages->clear();
        _searchMessages->preallocateTotal(total);
    }
    if (_searchMessages->searchPagesRetrieved().contains(page)) {
        // page for query already retrieved
        return;
    }
    _searchMessages->addSearchMessages(messages, page);

    _searchMessages->searchPageRetrieved(page, messages.count());
    qDebug() << "search ready" << page << pages;
    emit searchResultsReady(query, page, pages);
}

void SlackClientThreadSpawner::openChat(const QString& teamId, const QStringList &userIds, const QString &channelId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "openChat", Qt::QueuedConnection,
                              Q_ARG(QStringList, userIds),
                              Q_ARG(QString, channelId));
}

void SlackClientThreadSpawner::closeChat(const QString& teamId, const QString &chatId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(_slackClient, "closeChat", Qt::QueuedConnection,
                              Q_ARG(QString, chatId));
}

void SlackClientThreadSpawner::onTeamInfoChanged(const QString &teamId)
{
    qDebug() << "updated team info for id" << teamId << QThread::currentThreadId();
}

SlackTeamClient* SlackClientThreadSpawner::createNewClientInstance(const QString &teamId, const QString &accessToken) {
    SlackTeamClient* _slackClient = new SlackTeamClient(this, teamId, accessToken);
    QQmlEngine::setObjectOwnership(_slackClient, QQmlEngine::CppOwnership);

    connect(_slackClient, &SlackTeamClient::testConnectionFail, this, &SlackClientThreadSpawner::testConnectionFail, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::testLoginSuccess, this, &SlackClientThreadSpawner::testLoginSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::testLoginFail, this, &SlackClientThreadSpawner::testLoginFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::loadMessagesSuccess, this, &SlackClientThreadSpawner::loadMessagesSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::loadMessagesFail, this, &SlackClientThreadSpawner::loadMessagesFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::initFail, this, &SlackClientThreadSpawner::initFail, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::initSuccess, this, &SlackClientThreadSpawner::initSuccess, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::reconnectFail, this, &SlackClientThreadSpawner::reconnectFail, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::reconnectAccessTokenFail, this, &SlackClientThreadSpawner::reconnectAccessTokenFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::messageReceived, this, &SlackClientThreadSpawner::onMessageReceived, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::messagesReceived, this, &SlackClientThreadSpawner::onMessagesReceived, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::messageDeleted, this, &SlackClientThreadSpawner::onMessageDeleted, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::messageUpdated, this, &SlackClientThreadSpawner::onMessageUpdated, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::channelUpdated, this, &SlackClientThreadSpawner::onChannelUpdated, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::channelJoined, this, &SlackClientThreadSpawner::onChannelJoined, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::channelLeft, this, &SlackClientThreadSpawner::onChannelLeft, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::searchMessagesReceived, this, &SlackClientThreadSpawner::onSearchMessagesReceived, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::fileSharesReceived, this, &SlackClientThreadSpawner::onFileSharesReceived, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::postFileSuccess, this, &SlackClientThreadSpawner::postFileSuccess, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::postFileFail, this, &SlackClientThreadSpawner::postFileFail, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::networkOff, this, &SlackClientThreadSpawner::networkOff, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::networkOn, this, &SlackClientThreadSpawner::networkOn, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::reconnecting, this, &SlackClientThreadSpawner::reconnecting, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::disconnected, this, &SlackClientThreadSpawner::disconnected, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::connected, this, &SlackClientThreadSpawner::connected, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::lastChannelChanged, this, &SlackClientThreadSpawner::lastChannelChanged, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::teamInfoChanged, this, &SlackClientThreadSpawner::onTeamInfoChanged, Qt::QueuedConnection);

    connect(_slackClient, &SlackTeamClient::isOnlineChanged, this, &SlackClientThreadSpawner::onOnlineChanged, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::userTyping, this, &SlackClientThreadSpawner::userTyping, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::usersDataChanged, this, &SlackClientThreadSpawner::onUsersDataChanged, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::conversationsDataChanged, this, &SlackClientThreadSpawner::onConversationsDataChanged, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::conversationMembersChanged, this, &SlackClientThreadSpawner::onConversationMembersChanged, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::usersPresenceChanged, this, &SlackClientThreadSpawner::onUsersPresenceChanged, Qt::QueuedConnection);
    connect(_slackClient, &SlackTeamClient::error, this, &SlackClientThreadSpawner::error, Qt::QueuedConnection);

    return _slackClient;
}

MessageListModel *SlackClientThreadSpawner::getSearchMessages(const QString& teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient != nullptr) {
        return _slackClient->teamInfo()->searches();
    }
    return nullptr;
}

FilesSharesModel *SlackClientThreadSpawner::getFilesSharesModel(const QString &teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient != nullptr) {
        return _slackClient->teamInfo()->fileSharesModel();
    }
    return nullptr;
}

void SlackClientThreadSpawner::reconnectClient()
{
    for (SlackTeamClient* _slackClient : m_knownTeams.values()) {
        QMetaObject::invokeMethod(_slackClient, "reconnectClient", Qt::QueuedConnection);
    }
}

void SlackClientThreadSpawner::connectToTeam(const QString &teamId, const QString &accessToken)
{
    if (teamId.isEmpty()) {
        return;
    }
    SlackTeamClient* _slackClient = slackClient(teamId);

    if (_slackClient == nullptr) {
        _slackClient = createNewClientInstance(teamId, accessToken);
        _slackClient->moveToThread(this);
        _slackClient->startConnections();
        m_knownTeams[teamId] = _slackClient;
        SlackConfig::instance()->setTeams(m_knownTeams.keys());
    }
    _slackClient->startClient();
}

void SlackClientThreadSpawner::leaveTeam(const QString &teamId)
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "leaveTeam", Qt::QueuedConnection, Q_ARG(QString, teamId));
        return;
    }

    qDebug() << "Leaving team" << teamId;
    if (teamId.isEmpty()) {
        return;
    }
    SlackTeamClient* _slackClient = slackClient(teamId);

    if (_slackClient == nullptr) {
        return;
    }

    m_knownTeams.remove(teamId);

    if (m_lastTeam == teamId) {
        if (!m_knownTeams.isEmpty()) {
            setLastTeam(m_knownTeams.firstKey());
        } else {
            setLastTeam(QString());
        }
    }

    m_teamsModel.remove(_slackClient->teamInfo());

    _slackClient->deleteLater();
    SlackConfig::instance()->setTeams(m_knownTeams.keys());
    emit teamLeft(teamId);
}

void SlackClientThreadSpawner::setLastTeam(const QString &lastTeam)
{
    if (m_lastTeam == lastTeam)
        return;

    m_lastTeam = lastTeam;
    emit lastTeamChanged(m_lastTeam);
    qDebug() << "Setting last team" << lastTeam;

    m_settings.setValue(QStringLiteral("LastTeam"), m_lastTeam);
}

void SlackClientThreadSpawner::appendTeam(const QString &teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }
    m_teamsModel.append(_slackClient->teamInfo());
}

void SlackClientThreadSpawner::onOnlineChanged(const QString &teamId)
{
    if (slackClient(teamId) != nullptr) {
        emit onlineChanged(slackClient(teamId)->isOnline());
    }
}

void SlackClientThreadSpawner::setMediaSource(QObject *mediaPlayer, const QString &teamId,
                                              const QString &url)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return;
    }

    QMediaPlayer *mPlayer = qvariant_cast<QMediaPlayer *>(mediaPlayer->property("mediaObject"));
    if (mPlayer == nullptr) {
        qWarning() << "no media player found";
        return;
    }
    QNetworkRequest request;
    request.setUrl(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QString token = _slackClient->teamInfo()->teamToken();
    if (!token.isEmpty()) {
        request.setRawHeader(QString("Authorization").toUtf8(), QString("Bearer " + token).toUtf8());
    } else {
        qWarning() << "token for team" << _slackClient->teamInfo()->name() << "not found";
        return;
    }

    qDebug() << "set media content" << request.url();
    mPlayer->setMedia(QMediaContent(request));
}

QString SlackClientThreadSpawner::teamToken(const QString &teamId)
{
    SlackTeamClient* _slackClient = slackClient(teamId);
    if (_slackClient == nullptr) {
        return QStringLiteral("");
    }
    return _slackClient->teamInfo()->teamToken();
}

void SlackClientThreadSpawner::onUsersDataChanged(const QList<QPointer<SlackUser>>& users, bool last) {
    DEBUG_BLOCK;
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    _slackClient->teamInfo()->addUsersData(users, last);
    //query conversations only when last batch of users gets loaded
    if (last) {
        emit usersModelChanged(_slackClient->teamInfo()->teamId(), _slackClient->teamInfo()->users());
        QMetaObject::invokeMethod(_slackClient, "requestConversationsList", Qt::QueuedConnection, Q_ARG(QString, ""));
        connect(_slackClient->teamInfo()->fileSharesModel(), &FilesSharesModel::fetchMoreData,
                _slackClient, &SlackTeamClient::requestSharedFiles, Qt::QueuedConnection);
    }
}

void SlackClientThreadSpawner::onConversationsDataChanged(const QList<Chat*>& chats, bool last)
{
    DEBUG_BLOCK;
    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    if (_slackClient == nullptr) {
        return;
    }
    TeamInfo* _teamInfo = _slackClient->teamInfo();
    _teamInfo->addConversationsData(chats, last);
    // request extra info about IM chats
    for (Chat* chat : chats) {
        if (chat->type == ChatsModel::Conversation) {
            QMetaObject::invokeMethod(_slackClient, "requestConversationInfo", Qt::QueuedConnection, Q_ARG(QString, chat->id));
        }
        if (chat->isOpen || chat->type != ChatsModel::Channel) {
            //connect only to opened chats and conversations
            emit channelCountersUpdated(_teamInfo->teamId(), chat->id,
                                        chat->unreadCountDisplay, chat->unreadCountPersonal);
            connect(chat->messagesModel.data(), &MessageListModel::fetchMoreMessages,
                    _slackClient, &SlackTeamClient::loadMessages, Qt::QueuedConnection);
        }
    }
    if (last) {
        emit chatsModelChanged(_teamInfo->teamId(), _teamInfo->chats());
        connect(_teamInfo->searches(), &SearchMessagesModel::fetchMoreData,
                _slackClient, &SlackTeamClient::onFetchMoreSearchData, Qt::QueuedConnection);
    }
}

void SlackClientThreadSpawner::onConversationMembersChanged(const QString& channelId, const QStringList &members, bool last)
{
    DEBUG_BLOCK;

    Q_UNUSED(last);

    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    _chatsModel->addMembers(channelId, members);
}

void SlackClientThreadSpawner::onUsersPresenceChanged(const QStringList &users, const QString& presence,
                                                      const QDateTime &snoozeEnds, bool force)
{
    DEBUG_BLOCK;

    SlackTeamClient* _slackClient = static_cast<SlackTeamClient*>(sender());
    ChatsModel* _chatsModel = _slackClient->teamInfo()->chats();
    if (_chatsModel == nullptr) {
        return;
    }
    _chatsModel->setPresence(users, presence, snoozeEnds, force);
}

void SlackClientThreadSpawner::run()
{
    m_threadExecutor = new ThreadExecutor(this);
    for (const QString& teamId : SlackConfig::instance()->teams()) {
        connectToTeam(teamId);
    }

    if (SlackConfig::instance()->teams().isEmpty()) {
        qDebug() << "teams empty";
        setLastTeam(QString());
    }

    //make sure signal will be delivered to QML
    QMetaObject::invokeMethod(this, "threadStarted", Qt::QueuedConnection);

    //updating teams model
    //make sure it runs in GUI thread
    QMetaObject::invokeMethod(qApp, [this] {
        for (SlackTeamClient* _slackClient : m_knownTeams.values()) {
            m_teamsModel.append(_slackClient->teamInfo());
        }
    });

    // Start QT event loop for this thread
    this->exec();

    qDebug() << "closed thread" << m_lastTeam;
}

void ThreadExecutor::connectToTeam(const QString &teamId, const QString &accessToken)
{
    if (m_threadSpawner) {
        m_threadSpawner->connectToTeam(teamId, accessToken);
        m_threadSpawner->testLogin(teamId);
        QMetaObject::invokeMethod(m_threadSpawner, "appendTeam", Qt::QueuedConnection, Q_ARG(QString, teamId));
    }
}
