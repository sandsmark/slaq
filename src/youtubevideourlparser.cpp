#include <algorithm>
#include <QJsonObject>
#include <QJsonValue>
#include <QHash>
#include <QStringList>
#include <QtCore/QTimer>
#include <QEventLoop>

#include "youtubevideourlparser.h"

/**
 * @brief based on https://tyrrrz.me/Blog/Reverse-engineering-YouTube
 */

QByteArray httpGetSync(const QUrl& url, QNetworkAccessManager* qnam, QList<QNetworkReply::RawHeaderPair> &hdrPairs, int timeout = 20000) {
    QTimer timer;
    QEventLoop loop;
    QByteArray _downloadedData;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start(timeout);
    QNetworkRequest req = QNetworkRequest(url);
    for (QNetworkReply::RawHeaderPair hdrPair : hdrPairs) {
        req.setRawHeader(hdrPair.first, hdrPair.second);
    }
    QNetworkReply* reply = qnam->get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() == QNetworkReply::NoError) {
        _downloadedData = reply->readAll();
    } else {
        qWarning() << "httpGetSync error:" << reply->error() << "querying" << url;
    }
    reply->deleteLater();
    hdrPairs = reply->rawHeaderPairs();

    return _downloadedData;
}

void percentDecode(QString& str) {
    str = QUrl::fromPercentEncoding(str.replace("\\u0026", "&").toUtf8());
}

void decypher(QString& str, QList<QPair<QString, int>> ops) {
    if (str.isEmpty())
        return;
    for(QPair<QString, int> cyphOp : ops) {
        if (cyphOp.first == "reverse") {
            QString reverse;
            for(int i = str.size(); i >= 0; --i) reverse.append(str.at(i));
            str = reverse;
        } else if (cyphOp.first == "slice") {
            if (cyphOp.second < 0 || cyphOp.second >= str.size())
                continue;
            str = str.mid(cyphOp.second);
        } else if (cyphOp.first == "swap") {
            if (cyphOp.second < 0 || cyphOp.second >= str.size())
                continue;
            const QChar indChar = str.at(cyphOp.second);
            str = str.replace(cyphOp.second, 1, str.at(0));
            str = str.replace(0, 1, indChar);
        }
    }
}

YoutubeVideoUrlParser::YoutubeVideoUrlParser(QObject *parent) : QObject(parent)
{
    m_youtubeIdRegular.setPattern("youtube\\..+?/watch.*?v=(.*?)(?:&|/|$)");
    m_youtubeIdShort.setPattern("youtu\\.be/(.*?)(?:\\?|&|/|$)");
    m_youtubeIdEmbed.setPattern("youtube\\..+?/embed/(.*?)(?:\\?|&|/|$)");
    //m_youtubePlayerEmbed.setPattern("yt\\.setConfig\\({'PLAYER_CONFIG': (?<Json>\\{[^\\{\\}]*(((?<Open>\\{)[^\\{\\}]*)+((?<Close-Open>\\})[^\\{\\}]*)+)*(?(Open)(?!))\\})");
    m_youtubePlayerEmbed.setPattern("yt\\.setConfig\\({'PLAYER_CONFIG': (?<Json>\\{[^\\{\\}].*})");
    if (!m_youtubePlayerEmbed.isValid()) {
        qWarning() << __PRETTY_FUNCTION__ << m_youtubePlayerEmbed.errorString();
    }
    connect(this, &YoutubeVideoUrlParser::playerConfigChanged, this, &YoutubeVideoUrlParser::onPlayerConfigChanged);
}

void YoutubeVideoUrlParser::requestUrl(const QUrl &url)
{
    const QString _videoId = parseVideoId(url.toString());
    //qDebug() << __PRETTY_FUNCTION__ << url << _videoId;

    // 1st: get video embed page
    // TODO: implement video watch page request
    PlayerConfiguration* pc = new PlayerConfiguration;
    pc->videoId = _videoId;
    m_youtubeRequests[_videoId] = pc;
    QNetworkRequest req = QNetworkRequest(QString("https://www.youtube.com/embed/%1?disable_polymer=true&hl=en").arg(_videoId));
    req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    QNetworkReply* embedReply = manager.get(req);
    QObject::connect(embedReply, &QNetworkReply::finished, [this, embedReply, pc]() {
        const QByteArray& data = embedReply->readAll();
        QRegularExpressionMatch embedMath = m_youtubePlayerEmbed.match(QString(data));
        if (embedMath.hasMatch()) {
            const QString& embedJson = embedMath.captured("Json").chopped(1);
            QJsonParseError error;
            QJsonDocument document = QJsonDocument::fromJson(embedJson.toLatin1(), &error);

            if (error.error == QJsonParseError::NoError) {
                QJsonObject obj = document.object();
                const QString& _sts = obj.value("sts").toString();
                const QString& _playerSourceUrl = "https://youtube.com" + obj.value("assets").toObject().value("js").toString();
                pc->playerSourceUrl = _playerSourceUrl;
                //qDebug() << __PRETTY_FUNCTION__ << _sts << _playerSourceUrl;
                const QUrl eurl("https://youtube.googleapis.com/v/" + pc->videoId);
                const QUrl url(QString("https://www.youtube.com/get_video_info?video_id=%1&el=embedded&sts=%2&eurl=%3&hl=en").
                               arg(pc->videoId).arg(_sts).arg(eurl.toEncoded().data()));
                QNetworkRequest req = QNetworkRequest(url);
                req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, QNetworkRequest::SameOriginRedirectPolicy);
                QNetworkReply* reply = manager.get(req);

                QObject::connect(reply, &QNetworkReply::finished, [this, reply, pc]() {
                    if (reply->error() == QNetworkReply::NoError){
                        const QByteArray& data = reply->readAll();
                        // now need to extrcat video_id value
                        QUrlQuery parser(data);
                        if (!parser.hasQueryItem(QStringLiteral("video_id"))) {
                            qWarning() << "No video_id invideo info dic!";
                            return;
                        }
                        QJsonParseError error;
                        const QString& plResp = QUrl::fromPercentEncoding(parser.queryItemValue(QStringLiteral("player_response")).
                                                                          replace("\\u0026", "&").toUtf8());
                        QJsonDocument document = QJsonDocument::fromJson(plResp.toUtf8(), &error);
                        if (error.error == QJsonParseError::NoError) {
                            //qDebug() << "player response" << plResp;
                            const QJsonObject& obj = document.object();
                            const QJsonObject& playabilityStatusO = obj.value(QStringLiteral("playabilityStatus")).toObject();
                            const QJsonObject& streamingDataO = obj.value(QStringLiteral("streamingData")).toObject();
                            qDebug().noquote() << QJsonDocument(streamingDataO).toJson();
                            pc->succeed = true;
                            pc->isLiveStream = obj.value("videoDetails").toObject().value("isLive").toBool(false);
                            int _expsecs = streamingDataO.value("expiresInSeconds").toString().toInt();
                            pc->validUntil = QDateTime::currentDateTime().addSecs(_expsecs);
                            qDebug() << _expsecs << pc->validUntil;
                            if (pc->isLiveStream) {
                                pc->manifestUrl = streamingDataO.value("hlsManifestUrl").toString();
                            } else {
                                pc->manifestUrl = streamingDataO.value("dashManifestUrl").toString();
                                pc->muxedStreamInfosUrlEncoded =
                                        QUrl::fromPercentEncoding(parser.queryItemValue(QStringLiteral("url_encoded_fmt_stream_map")).
                                                                  replace("\\u0026", "&").toUtf8());
                                pc->adaptiveStreamInfosUrlEncoded =
                                        QUrl::fromPercentEncoding(parser.queryItemValue(QStringLiteral("adaptive_fmts")).
                                                                  replace("\\u0026", "&").toUtf8());
                            }
                        } else {
                            qWarning().noquote() << "Error parsing player_response json" << error.error << error.errorString() << plResp;
                        }
                    } else {
                        qWarning() << "Youtube get video info error" << reply->error();
                    }
                    emit playerConfigChanged(pc);
                });
            } else {
                qWarning().noquote() << "Error parsing embed json" << error.error << error.errorString() << embedJson;
            }
        } else {
            qWarning() << "cant parse youtube player embed" << data;
        }
        embedReply->deleteLater();
    });

}

/**
 * Tries to parse video ID from a YouTube video URL.
*/

QString YoutubeVideoUrlParser::parseVideoId(const QString& videoUrl)
{
    QRegularExpressionMatch match = m_youtubeIdRegular.match(videoUrl);
    if (match.hasMatch() && !match.captured(1).isEmpty()) {
        return match.captured(1);
    }
    match = m_youtubeIdShort.match(videoUrl);
    if (match.hasMatch() && !match.captured(1).isEmpty()) {
        return match.captured(1);
    }
    match = m_youtubeIdEmbed.match(videoUrl);
    if (match.hasMatch() && !match.captured(1).isEmpty()) {
        return match.captured(1);
    }
    return QString();
}

void YoutubeVideoUrlParser::onPlayerConfigChanged(PlayerConfiguration *playerConfig)
{
    qDebug() << "Player config:"
             << "is success:" << playerConfig->succeed
             << playerConfig->videoId
             << playerConfig->isLiveStream
             << playerConfig->playerSourceUrl
             << playerConfig->validUntil;
//             << playerConfig->manifestUrl
//             << playerConfig->muxedStreamInfosUrlEncoded
//             << playerConfig->adaptiveStreamInfosUrlEncoded;
    if (!playerConfig->succeed) {
        qWarning() << "Player config exctraction was not succeed";
        return;
    }
    const QStringList& muxedStreamInfoList = playerConfig->muxedStreamInfosUrlEncoded.split(",");
    for (const auto muxedStreamInfo : muxedStreamInfoList) {
        QUrlQuery urlq(muxedStreamInfo);
        // Extract info
        int itag = urlq.queryItemValue("itag").toInt();
        QString url = urlq.queryItemValue("url");
        percentDecode(url);
        // Decipher signature if needed
        QString signature = urlq.queryItemValue("s");
        qDebug() << "signature: " << signature;
        QUrl _url = QUrl(url);
        if (!signature.isEmpty()) {
            // Get cipher operations
            QList<QPair<QString, int>> cipherOperations = getCypherOperations(playerConfig);
            // Decipher signature
            decypher(signature, cipherOperations);
            // Set the corresponding parameter in the URL
            const QString& signatureParameter = urlq.queryItemValue("sp").isEmpty() ? "signature" : urlq.queryItemValue("sp");

            // replace query with new signature
            QUrlQuery _urlQuery(_url.query());
            _urlQuery.addQueryItem(signatureParameter, signature);
            _url.setQuery(_urlQuery);
        }

        // Try to extract content length, otherwise get it manually
        int contentLength = QRegularExpression("clen=(\\d+)").match(url).captured(1).toInt();
        qDebug() << "content length from url" << contentLength << url;
        if (contentLength <= 0) {
            QList<QNetworkReply::RawHeaderPair> hdrPairs;
            httpGetSync(_url, &manager, hdrPairs);
            qDebug() << "headers size" << hdrPairs.size();
            for (QNetworkReply::RawHeaderPair hdrPair : hdrPairs) {
                if (hdrPair.first == "ContentLength") {
                    contentLength = hdrPair.second.toInt();
                    break;
                }
            }
            qDebug() << "content length from header" << contentLength;
            // If content length is still not available - stream is gone or faulty
            if (contentLength <= 0)
                continue;
        }
/*
        // Extract container
        var containerRaw = streamInfoDic["type"].SubstringUntil(";").SubstringAfter("/");
        var container = Heuristics.ContainerFromString(containerRaw);

        // Extract audio encoding
        var audioEncodingRaw = streamInfoDic["type"].SubstringAfter("codecs=\"").SubstringUntil("\"").Split(", ").Last();
        var audioEncoding = Heuristics.AudioEncodingFromString(audioEncodingRaw);

        // Extract video encoding
        var videoEncodingRaw = streamInfoDic["type"].SubstringAfter("codecs=\"").SubstringUntil("\"").Split(", ").First();
        var videoEncoding = Heuristics.VideoEncodingFromString(videoEncodingRaw);

        // Determine video quality from itag
        var videoQuality = Heuristics.VideoQualityFromItag(itag);

        // Determine video quality label from video quality
        var videoQualityLabel = Heuristics.VideoQualityToLabel(videoQuality);

        // Determine video resolution from video quality
        var resolution = Heuristics.VideoQualityToResolution(videoQuality);

        // Add to list
        muxedStreamInfoMap[itag] = new MuxedStreamInfo(itag, url, container, contentLength, audioEncoding, videoEncoding,
            videoQualityLabel, videoQuality, resolution);
*/
    }
}

QList<QPair<QString, int>> YoutubeVideoUrlParser::getCypherOperations(PlayerConfiguration *playerConfig)
{
    // TODO: implement caching
    /*
    // If already in cache - return
    if (_cipherOperationsCache.TryGetValue(playerSourceUrl, out var cached))
        return cached;
*/

    QList<QPair<QString, int>> operations;
    QList<QNetworkReply::RawHeaderPair> _hdrPairs;
    const QByteArray& playerSource = httpGetSync(playerConfig->playerSourceUrl, &manager, _hdrPairs);
    if (playerSource.isEmpty()) {
        qWarning() << "Youtube player source is empty:" << playerConfig->playerSourceUrl;
        return operations;;
    }
    const QString& deciphererFuncName = QRegularExpression("\\bc\\s*&&\\s*d\\.set\\([^,]+,\\s*(?:encodeURIComponent\\s*\\()?\\s*([\\w$]+)\\(").
            match(playerSource).captured(1);

    if (deciphererFuncName.isEmpty()) {
        qWarning() << "Youtube cant find decipherer function name";
        return operations;;
    }
    const QString& deciphererFuncBody = QRegularExpression("(?!h\\.)" + deciphererFuncName + "=function\\(\\w+\\)\\{(.*?)\\}").
            match(playerSource).captured(1);

    if (deciphererFuncBody.isEmpty()) {
        qWarning() << "Youtube cant find decipherer function body";
        return operations;;
    }

    QString reverseFuncName;
    QString sliceFuncName;
    QString swapFuncName;
    const QStringList& cypherStatements = deciphererFuncBody.split(";");

    for (const QString& statement : cypherStatements) {
        if (!reverseFuncName.isEmpty() &&
            !sliceFuncName.isEmpty() &&
            !swapFuncName.isEmpty())
            break;
        const QString& calledFuncName = QRegularExpression("\\w+(?:.|\\[)(\\""?\\w+(?:\\"")?)\\]?\\(").match(statement).captured(1);
        if (calledFuncName.isEmpty())
            continue;
        // Determine cipher function names by signature
        if (QRegularExpression(calledFuncName + ":\\bfunction\\b\\(\\w+\\)").match(playerSource).hasMatch()) {
            reverseFuncName = calledFuncName;
        } else if (QRegularExpression(calledFuncName + ":\\bfunction\\b\\([a],b\\).(\\breturn\\b)?.?\\w+\\.").match(playerSource).hasMatch()) {
            sliceFuncName = calledFuncName;
        } else if (QRegularExpression(calledFuncName + ":\\bfunction\\b\\(\\w+\\,\\w\\).\\bvar\\b.\\bc=a\\b").match(playerSource).hasMatch()) {
            swapFuncName = calledFuncName;
        }
    }
    // Analyze cipher function calls to determine their order and parameters
    for (const QString& statement : cypherStatements) {

        // Get the name of the function called in this statement
        const QString& calledFuncName = QRegularExpression("\\w+(?:.|\\[)(\\""?\\w+(?:\\"")?)\\]?\\(").match(statement).captured(1);
        if (calledFuncName.isEmpty())
            continue;
        // Reverse operation
        if (calledFuncName == reverseFuncName) {
            operations.append(QPair<QString, int>("reverse", 0));
        } else if (calledFuncName == sliceFuncName) {
            int index = QRegularExpression("\\(\\w+,(\\d+)\\)").match(statement).captured(1).toInt();
            operations.append(QPair<QString, int>("slice", index));
        } else if (calledFuncName == swapFuncName) {
            int index = QRegularExpression("\\(\\w+,(\\d+)\\)").match(statement).captured(1).toInt();
            operations.append(QPair<QString, int>("swap", index));
        }
    }
    return operations;
}

void YoutubeVideoUrlParser::finished(QNetworkReply *reply)
{
    qDebug() << __PRETTY_FUNCTION__ << reply->error();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();
    return;
    if (reply->error() == QNetworkReply::NoError/*statusCode >= 200 && statusCode <= 302*/) {
        // This block is based on file youtube.lua from VideoLAN project
        QHash<int, QString> stream_map;
        QRegExp re("\"url_encoded_fmt_stream_map\":\"([^\"]*)\"", Qt::CaseInsensitive, QRegExp::RegExp2);
        QRegExp urls("itag=(\\d+),url=(.*)");

        if (re.indexIn(reply->readAll()) != -1) {
            QString result = re.cap(1);
            for (const QString& line : result.split("\\u0026")) {
                if (urls.indexIn(QUrl::fromPercentEncoding(line.toLocal8Bit())) != -1) {
                    stream_map[urls.cap(1).toInt()] = urls.cap(2);
                }
            }

            // XXX hardcoded
            QList<QString> values = stream_map.values();
            qDebug() << "YoutubeVideoUrlParser" << stream_map;
            emit urlParsed(values.last());
        } else {
            qWarning() << "YoutubeVideoUrlParser no match";
        }
    } else {
        qDebug() << "youtube status code:" << statusCode;
    }
}

void YoutubeVideoUrlParser::error(QNetworkReply::NetworkError error)
{
    qDebug() << "youtube error:" << error;
}
