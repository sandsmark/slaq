#include <algorithm>
#include <QJsonObject>
#include <QJsonValue>
#include <QHash>
#include <QStringList>
#include <QtCore/QTimer>
#include <QEventLoop>
#include <QScopeGuard>

#include <QMetaEnum>
#include "youtubevideourlparser.h"

/**
 * @brief based on https://tyrrrz.me/Blog/Reverse-engineering-YouTube
 */

int httpHeadSync(const QUrl& url, QNetworkAccessManager* qnam, QList<QNetworkReply::RawHeaderPair> &hdrPairs, int timeout = 20000) {
    QTimer timer;
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start(timeout);
    QNetworkRequest req = QNetworkRequest(url);
    for (QNetworkReply::RawHeaderPair hdrPair : hdrPairs) {
        req.setRawHeader(hdrPair.first, hdrPair.second);
    }
    QNetworkReply* reply = qnam->head(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "httpGetSync error:" << reply->error() << "querying" << url;
    }
    reply->deleteLater();
    hdrPairs = reply->rawHeaderPairs();
    return reply->error();
}

QByteArray httpGetSync(const QUrl& url, QNetworkAccessManager* qnam, QList<QNetworkReply::RawHeaderPair> &hdrPairs, int timeout = 20000) {
    QTimer timer;
    QEventLoop loop;
    QByteArray _downloadedData;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start(timeout);
    QNetworkRequest req = QNetworkRequest(url);
    //req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, QNetworkRequest::SameOriginRedirectPolicy);
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
    //    qDebug() << __PRETTY_FUNCTION__ << "\n\n\n" << str << "\n\n";
    const QByteArray& ba = QByteArray::fromPercentEncoding(str.toUtf8().replace("\\u0026", "&").replace("%2525", "%").replace("%25", "%"));
    //    qDebug() << ba1 << "\n\n" << ba << "\n\n\n";
    str = QString::fromUtf8(ba, ba.size());
}

void decypher(QString& str, QList<QPair<QString, int>> ops) {
    if (str.isEmpty())
        return;
    for(QPair<QString, int> cyphOp : ops) {
        if (cyphOp.first == "reverse") {
            QString reverse;
            reverse.reserve(str.size());
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

QString midInner(const QString& s, const QString& before, const QString& after) {
    int ind_before = s.indexOf(before);
    if (ind_before == -1) {
        //qWarning() << "cant find" << before;
        return "";
    }
    ind_before += before.size();
    int ind_after = s.indexOf(after, ind_before);
    if (ind_after == -1) {
        //qWarning() << "cant find" << after;
        return "";
    }
    // video/mp4;
    return s.mid(ind_before, ind_after - ind_before);
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

void YoutubeVideoUrlParser::pickBestPossibleVideo(PlayerConfiguration* pc) {
    for (const MediaStreamInfo& msi : pc->streams) {
        if (msi.quality >= High720 && msi.container != UnknownContainer &&
                msi.acodec != UnknownACodec && msi.vcodec != UnknownVCodec) { //TODO: check settings or best possible
            qDebug() << "best video for id:" << pc->videoId << msi.quality << msi.resolution;
            emit urlParsed(pc->videoId, msi.playableUrl);
            return;
        }
    }
    qWarning() << "Youtube: cant pick best video with audio. Try without audio";
    for (const MediaStreamInfo& msi : pc->streams) {
        if (msi.quality >= High720 && msi.container != UnknownContainer && msi.vcodec != UnknownVCodec) {
            qDebug() << "best video for id:" << pc->videoId << msi.quality << msi.resolution;
            emit urlParsed(pc->videoId, msi.playableUrl);
            return;
        }
    }
}

void YoutubeVideoUrlParser::dump(PlayerConfiguration* pc) {
    qDebug().noquote() << "Dump for:";
    qDebug().noquote() << "  video ID" << pc->videoId;
    qDebug().noquote() << "  valid till" << pc->validUntil;
    for (const MediaStreamInfo& msi : pc->streams) {
        qDebug().noquote() << "    playable url:" << msi.playableUrl;
        qDebug().noquote() << "    content length:" << msi.contentLength;
        qDebug().noquote() << "    container:" << msi.container;
        qDebug().noquote() << "    acodec:" << msi.acodec;
        qDebug().noquote() << "    vcodec:" << msi.vcodec;
        qDebug().noquote() << "    resolution:" << msi.resolution;
        qDebug().noquote() << "    bitrate:" << msi.bitrate;
        qDebug().noquote() << "    framerate:" << msi.framerate;
        qDebug().noquote() << "    quality:" << msi.quality;
    }
}

bool YoutubeVideoUrlParser::isNotExpired(const QString &videoId)
{
    PlayerConfiguration* pc = m_youtubeRequests.value(videoId);
    if (pc != nullptr) {
        return (QDateTime::currentMSecsSinceEpoch() < pc->validUntil.toMSecsSinceEpoch());
    }
    return false;
}

int YoutubeVideoUrlParser::expiredIn(const QString &videoId)
{
    PlayerConfiguration* pc = m_youtubeRequests.value(videoId);
    if (pc != nullptr) {
        int _expireSecs = (pc->validUntil.toMSecsSinceEpoch() - QDateTime::currentMSecsSinceEpoch())/1000;
        if (_expireSecs <= 0) {
            qWarning() << "Youtube expiration time" << pc->validUntil
                       << "less than current time" << QDateTime::currentDateTime();
            _expireSecs = 3600;
        }
        return _expireSecs;
    }
    return 0;
}

void YoutubeVideoUrlParser::requestVideoUrl(const QString &videoId)
{
    //qDebug() << __PRETTY_FUNCTION__ << url << _videoId;

    PlayerConfiguration* pc = m_youtubeRequests.value(videoId);
    if (pc != nullptr) { // check if request already sent
        if (pc->succeed) {
            pickBestPossibleVideo(pc);
        }
        return;
    }
    // 1st: get video embed page
    pc = new PlayerConfiguration;

    pc->succeed = false;
    pc->videoId = videoId;//parseVideoId(url.toString());
    m_youtubeRequests[videoId] = pc;

    //lambdas
    QNetworkRequest req = QNetworkRequest(QString("https://www.youtube.com/embed/%1?disable_polymer=true&hl=en").arg(pc->videoId));
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
                        //                        const QString& plResp = QUrl::fromPercentEncoding(parser.queryItemValue(QStringLiteral("player_response")).
                        //                                                                          replace("\\u0026", "&").toUtf8());
                        const QString& plResp = parser.queryItemValue(QStringLiteral("player_response"));//.replace("\\u0026", "&");
                        //qWarning().noquote() << "player response" << plResp;
                        QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromPercentEncoding(plResp.toUtf8()), &error);
                        if (error.error == QJsonParseError::NoError) {
                            //qDebug() << "player response" << plResp;
                            const QJsonObject& obj = document.object();
                            const QJsonObject& playabilityStatusO = obj.value(QStringLiteral("playabilityStatus")).toObject();
                            const QJsonObject& streamingDataO = obj.value(QStringLiteral("streamingData")).toObject();
                            //qDebug().noquote() << QJsonDocument(streamingDataO).toJson();
                            pc->succeed = true;
                            pc->isLiveStream = obj.value("videoDetails").toObject().value("isLive").toBool(false);
                            int _expsecs = streamingDataO.value("expiresInSeconds").toString().toInt();
                            pc->validUntil = QDateTime::currentDateTime().addSecs(_expsecs);
                            //qDebug() << _expsecs << pc->validUntil;
                            if (pc->isLiveStream) {
                                pc->manifestUrl = streamingDataO.value("hlsManifestUrl").toString();
                            } else {
                                pc->manifestUrl = streamingDataO.value("dashManifestUrl").toString();
                                pc->muxedStreamInfosUrlEncoded = parser.queryItemValue(QStringLiteral("url_encoded_fmt_stream_map"));
                                pc->adaptiveStreamInfosUrlEncoded = parser.queryItemValue(QStringLiteral("adaptive_fmts"));
                                //qDebug().noquote() << "pc->muxedStreamInfosUrlEncoded" << pc->muxedStreamInfosUrlEncoded;
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
                emit playerConfigChanged(pc);
            }
        } else {
            qWarning() << "cant parse youtube player embed" << data;
            emit playerConfigChanged(pc);
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

void YoutubeVideoUrlParser::checkContentLengthAndRedirections(const QUrl &url, YoutubeVideoUrlParser::MediaStreamInfo &msi)
{
    // make request anyway to check for redirected playable url
    QList<QNetworkReply::RawHeaderPair> hdrPairs;
    httpHeadSync(url, &manager, hdrPairs);
    for (QNetworkReply::RawHeaderPair hdrPair : hdrPairs) {
        if (hdrPair.first == "Content-Length") {
            qint64 _contentLength = hdrPair.second.toLongLong();
            if (_contentLength != msi.contentLength && _contentLength > 0)
                msi.contentLength = _contentLength;
        }
        if (hdrPair.first == "Location") {
            //redirected url
            msi.playableUrl = QUrl(hdrPair.second);
            qDebug() << "u2be redirected" << msi.playableUrl;
        }
    }
    qDebug() << "content length" << msi.contentLength;
}

void YoutubeVideoUrlParser::onPlayerConfigChanged(PlayerConfiguration *playerConfig)
{
    if (!playerConfig)
        return;
    qDebug() << "Player config:"
             << "is success:" << playerConfig->succeed
             << playerConfig->videoId
             << playerConfig->isLiveStream
             << playerConfig->validUntil;
    //             << playerConfig->muxedStreamInfosUrlEncoded
    //             << playerConfig->adaptiveStreamInfosUrlEncoded;
    if (!playerConfig->succeed) {
        qWarning() << "Player config exctraction was not succeed";
        return;
    }

    //qDebug().noquote() << "muxed stream info" << playerConfig->muxedStreamInfosUrlEncoded;
    // get muxed stream infos
    // split by %2C whish is ',', because its percent encoded
    const QStringList& muxedStreamInfoList = playerConfig->muxedStreamInfosUrlEncoded.split("%2C");
    for (const auto muxedStreamInfo : muxedStreamInfoList) {
        QUrlQuery streamInfoDic(muxedStreamInfo);
        // Extract info
        int itag = streamInfoDic.queryItemValue("itag").toInt();
        QString url = streamInfoDic.queryItemValue("url");
        percentDecode(url);
        // Decipher signature if needed
        QString signature = streamInfoDic.queryItemValue("s");
        qDebug() << "signature (muxed): " << signature;
        QUrl _url = url;
        if (!signature.isEmpty()) {
            // Get cipher operations
            QList<QPair<QString, int>> cipherOperations = getCipherOperations(playerConfig);
            // Decipher signature
            decypher(signature, cipherOperations);
            // Set the corresponding parameter in the URL
            const QString& signatureParameter = streamInfoDic.hasQueryItem("sp") ?
                        "signature" :
                        streamInfoDic.queryItemValue("sp", QUrl::FullyDecoded);

            // replace query with new signature
            QUrlQuery _urlQuery(_url.query());
            _urlQuery.addQueryItem(signatureParameter, signature);
            _url.setQuery(_urlQuery);
        }
        MediaStreamInfo msi;
        msi.playableUrl = _url;
        // Try to extract content length, otherwise get it manually
        msi.contentLength = QRegularExpression("clen=(\\d+)").match(url).captured(1).toLongLong();
        qDebug() << "content length from url" << msi.contentLength;
        checkContentLengthAndRedirections(_url, msi);
        // If content length is still not available - stream is gone or faulty
        if (msi.contentLength <= 0)
            continue;
        QString containerType = streamInfoDic.queryItemValue("type", QUrl::FullyDecoded);
        percentDecode(containerType);
        const QString& containerRaw = midInner(containerType, "/", ";");
        const QString& codecsListRaw = midInner(containerType, "codecs=\"", "\"");
        const QStringList& codecsList = codecsListRaw.split(",+");
        const QString& audioEncodingRaw = codecsList.last();
        const QString& videoEncodingRaw = codecsList.first();
        //qDebug().noquote() << "youtube encodings:" << audioEncodingRaw << videoEncodingRaw << codecsListRaw << containerType << containerRaw;


        msi.itag = itag;
        msi.container = parseContainer(containerRaw);
        msi.acodec = parseAudioCodec(audioEncodingRaw);
        msi.vcodec = parseVideoCodec(videoEncodingRaw);
        msi.quality = itagToQuality(itag);
        if (msi.quality == UnknownQuality) {
            qWarning().noquote() << "Unknown quality from itag" << msi.itag << "parsed from" << streamInfoDic.queryItemValue("itag", QUrl::FullyDecoded);
        }
        msi.videoQualityLabel = videoQualityToLabel(msi.quality);
        msi.resolution = videoQualityToResolution(msi.quality);
        playerConfig->streams.insertMulti(itag, msi);
    }

    // Get adaptive stream infos
    const QStringList& adaptiveStreamInfoList = playerConfig->adaptiveStreamInfosUrlEncoded.split(",");
    //qDebug().noquote() << "adaptiveStreamInfoList" << adaptiveStreamInfoList;

    for (const auto adaptiveStreamInfo : adaptiveStreamInfoList) {
        QUrlQuery streamInfoDic(adaptiveStreamInfo);
        MediaStreamInfo msi;
        msi.itag = streamInfoDic.queryItemValue("itag", QUrl::FullyDecoded).toInt();
        QString url = streamInfoDic.queryItemValue("url", QUrl::FullyDecoded);
        percentDecode(url);
        msi.bitrate = streamInfoDic.queryItemValue("bitrate", QUrl::FullyDecoded).toInt();
        QString signature = streamInfoDic.queryItemValue("s", QUrl::FullyDecoded);
        qDebug() << "signature (adaptive): " << signature;
        QUrl _url = url;

        if (!signature.isEmpty()) {
            // Get cipher operations
            QList<QPair<QString, int>> cipherOperations = getCipherOperations(playerConfig);
            // Decipher signature
            decypher(signature, cipherOperations);
            // Set the corresponding parameter in the URL
            const QString& signatureParameter = streamInfoDic.hasQueryItem("sp") ? "signature" : streamInfoDic.queryItemValue("sp");

            // replace query with new signature
            QUrlQuery _urlQuery(_url.query());
            _urlQuery.addQueryItem(signatureParameter, signature);
            _url.setQuery(_urlQuery);
        }
        msi.playableUrl = _url;
        // Try to extract content length, otherwise get it manually
        msi.contentLength = streamInfoDic.queryItemValue("clen", QUrl::FullyDecoded).toLongLong();
        qDebug() << "content length from url" << msi.contentLength;
        // make request anyway to check for redirected playable url
        checkContentLengthAndRedirections(_url, msi);
        // If content length is still not available - stream is gone or faulty
        if (msi.contentLength <= 0)
            continue;
        QString containerType = streamInfoDic.queryItemValue("type", QUrl::FullyDecoded);
        percentDecode(containerType);
        const QString& containerRaw = midInner(containerType, "/", ";");
        msi.container = parseContainer(containerRaw);
        const QString& codecRaw = midInner(containerType, "codecs=\"", "\"");
        if (containerType.startsWith("audio/", Qt::CaseInsensitive)) {
            // audio only
            msi.acodec = parseAudioCodec(codecRaw);
            //qDebug() << "audio only" << msi.acodec << msi.container;
        } else {
            // video only

            // Extract video encoding
            msi.vcodec = codecRaw.compare("unknown", Qt::CaseInsensitive) == 0 ? Av1 : parseVideoCodec(codecRaw);
            // Extract video quality label and video quality
            const QString& videoQualityLabel = streamInfoDic.queryItemValue("quality_label", QUrl::FullyDecoded);
            msi.quality = videoQualityFromLabel(videoQualityLabel);
            if (msi.quality == UnknownQuality) {
                qWarning().noquote() << "Unknown quality from label" << videoQualityLabel;
            }

            // Extract resolution
            const QStringList& sizeList = streamInfoDic.queryItemValue("size", QUrl::FullyDecoded).split("x");
            if (sizeList.size() == 2) {
                int width = sizeList.at(0).toInt();
                int height = sizeList.at(1).toInt();
                msi.resolution = QSize(width, height);
            }
            // Extract framerate
            msi.framerate = streamInfoDic.queryItemValue("fps", QUrl::FullyDecoded).toInt();
            //qDebug() << "video only" << msi.vcodec << msi.container << msi.quality << msi.resolution << "fps:" << msi.framerate << streamInfoDic.toString(QUrl::FullyDecoded);
        }
        // distinguish audio only or video only bycheck audio or video codec is set
        playerConfig->streams.insertMulti(msi.itag, msi);
    }

    // Parse dash manifest. Dash mainfest set only for non live streams
    if (!playerConfig->isLiveStream && !playerConfig->manifestUrl.isEmpty()) {
        // Extract signature
        QString dashManifestUrl = playerConfig->manifestUrl;
        QUrl _url = dashManifestUrl;
        QString signature = QRegularExpression("/s/(.*?)(?:/|$)").match(dashManifestUrl).captured(1);

        // Decipher signature if needed
        if (!signature.isEmpty()) {
            // Get cipher operations
            QList<QPair<QString, int>> cipherOperations = getCipherOperations(playerConfig);
            // Decipher signature
            decypher(signature, cipherOperations);

            // replace query with new signature
            QUrlQuery _urlQuery(_url.query());
            _urlQuery.addQueryItem("signature", signature);
            _url.setQuery(_urlQuery);
        }
        QList<QNetworkReply::RawHeaderPair> hdrPairs;
        const QByteArray& dashManifestXml = httpGetSync(_url, &manager, hdrPairs);
        //qDebug().noquote().nospace() << "dashManifestXml:" << dashManifestXml;
        QXmlStreamReader dashXml(dashManifestXml);
        dashXml.setNamespaceProcessing(false);
        while (!dashXml.atEnd()) {
            dashXml.readNext();
            if (dashXml.tokenType() == QXmlStreamReader::StartElement && dashXml.name() == QStringLiteral("Representation")) {
                bool _isvalid = true; //consider as valid. Check for sourceUrl attribute later
                bool _isaudio = false;
                MediaStreamInfo _msi;
                _msi.itag = dashXml.attributes().value("id").toInt();
                _msi.bitrate = dashXml.attributes().value("bandwidth").toInt();
                int _w = dashXml.attributes().value("width").toInt();
                int _h = dashXml.attributes().value("height").toInt();
                _msi.resolution = QSize(_w, _h);
                QString codec = dashXml.attributes().value("codecs").toString();

                while (!(dashXml.tokenType() == QXmlStreamReader::EndElement && dashXml.name() == QStringLiteral("Representation"))) {
                    dashXml.readNext();
                    if (dashXml.tokenType() == QXmlStreamReader::StartElement && dashXml.name() == QStringLiteral("AudioChannelConfiguration")) {
                        _isaudio = true;
                    }
                    if (dashXml.tokenType() == QXmlStreamReader::StartElement && dashXml.name() == QStringLiteral("Initialization")) {
                        if (dashXml.attributes().value("sourceURL").contains("sq/")) {
                            _isvalid = false;
                        }
                    }
                    if (dashXml.tokenType() == QXmlStreamReader::StartElement && dashXml.name() == QStringLiteral("BaseURL")) {
                        dashXml.readNext();
                        _msi.playableUrl = QUrl(QByteArray::fromPercentEncoding(dashXml.text().toUtf8()));
                        _msi.contentLength = QRegularExpression("clen[/=](\\d+)").match(_msi.playableUrl.toString()).captured(1).toInt();
                        const QString& containerRaw = QRegularExpression("mime[/=]\\w*%2F([\\w\\d]*)").match(_msi.playableUrl.toString()).captured(1);
                        _msi.container = parseContainer(containerRaw);

                        //qDebug() << "playable url" << _msi.playableUrl;
                    }
                }
                if (_isvalid) {
                    if (_isaudio)
                        _msi.acodec = parseAudioCodec(codec);
                    else
                        _msi.vcodec = parseVideoCodec(codec);
                    _msi.quality = itagToQuality(_msi.itag);
                    playerConfig->streams.insertMulti(_msi.itag, _msi);
                }
            }
        }
    }
    pickBestPossibleVideo(playerConfig);
    dump(playerConfig);
}

QList<QPair<QString, int>> YoutubeVideoUrlParser::getCipherOperations(PlayerConfiguration *playerConfig)
{
    if (!playerConfig)
        return QList<QPair<QString, int>>();
    // If already in cache - return
    if (!m_cipherCache.contains(playerConfig->playerSourceUrl))
        return m_cipherCache.value(playerConfig->playerSourceUrl);

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
    m_cipherCache[playerConfig->playerSourceUrl] = operations;
    return operations;
}

YoutubeVideoUrlParser::YoutubeContainer YoutubeVideoUrlParser::parseContainer(const QString &container)
{
    if (container.contains("mp4", Qt::CaseInsensitive))
        return YoutubeContainer::Mp4;
    if (container.contains("webm", Qt::CaseInsensitive))
        return YoutubeContainer::WebM;
    if (container.contains("3gpp", Qt::CaseInsensitive))
        return YoutubeContainer::Tgpp;
    return YoutubeContainer::UnknownContainer;
}

YoutubeVideoUrlParser::YoutubeAudioCodec YoutubeVideoUrlParser::parseAudioCodec(const QString &codec)
{
    if (codec.contains("mp4a", Qt::CaseInsensitive))
        return YoutubeAudioCodec::Aac;
    if (codec.contains("vorbis", Qt::CaseInsensitive))
        return YoutubeAudioCodec::Vorbis;
    if (codec.contains("opus", Qt::CaseInsensitive))
        return YoutubeAudioCodec::Opus;
    return YoutubeAudioCodec::UnknownACodec;

}

YoutubeVideoUrlParser::YoutubeVideoCodec YoutubeVideoUrlParser::parseVideoCodec(const QString &codec)
{
    if (codec.contains("mp4v", Qt::CaseInsensitive))
        return YoutubeVideoCodec::Mp4V;
    if (codec.contains("avc1", Qt::CaseInsensitive))
        return YoutubeVideoCodec::H264;
    if (codec.contains("vp8", Qt::CaseInsensitive))
        return YoutubeVideoCodec::Vp8;
    if (codec.contains("vp9", Qt::CaseInsensitive))
        return YoutubeVideoCodec::Vp9;
    if (codec.contains("av01", Qt::CaseInsensitive))
        return YoutubeVideoCodec::Av1;
    return YoutubeVideoCodec::UnknownVCodec;

}

YoutubeVideoUrlParser::YoutubeVideoQuality YoutubeVideoUrlParser::itagToQuality(int itag)
{
    static QMap<int, YoutubeVideoQuality> itagToVideoQualityMap =
    {
        {5, Low144},
        {6, Low240},
        {13, Low144},
        {17, Low144},
        {18, Medium360},
        {22, High720},
        {34, Medium360},
        {35, Medium480},
        {36, Low240},
        {37, High1080},
        {38, High3072},
        {43, Medium360},
        {44, Medium480},
        {45, High720},
        {46, High1080},
        {59, Medium480},
        {78, Medium480},
        {82, Medium360},
        {83, Medium480},
        {84, High720},
        {85, High1080},
        {91, Low144},
        {92, Low240},
        {93, Medium360},
        {94, Medium480},
        {95, High720},
        {96, High1080},
        {100, Medium360},
        {101, Medium480},
        {102, High720},
        {132, Low240},
        {151, Low144},
        {133, Low240},
        {134, Medium360},
        {135, Medium480},
        {136, High720},
        {137, High1080},
        {138, High4320},
        {160, Low144},
        {212, Medium480},
        {213, Medium480},
        {214, High720},
        {215, High720},
        {216, High1080},
        {217, High1080},
        {264, High1440},
        {266, High2160},
        {298, High720},
        {299, High1080},
        {399, High1080},
        {398, High720},
        {397, Medium480},
        {396, Medium360},
        {395, Low240},
        {394, Low144},
        {167, Medium360},
        {168, Medium480},
        {169, High720},
        {170, High1080},
        {218, Medium480},
        {219, Medium480},
        {242, Low240},
        {243, Medium360},
        {244, Medium480},
        {245, Medium480},
        {246, Medium480},
        {247, High720},
        {248, High1080},
        {271, High1440},
        {272, High2160},
        {278, Low144},
        {302, High720},
        {303, High1080},
        {308, High1440},
        {313, High2160},
        {315, High2160},
        {330, Low144},
        {331, Low240},
        {332, Medium360},
        {333, Medium480},
        {334, High720},
        {335, High1080},
        {336, High1440},
        {337, High2160}
    };
    return itagToVideoQualityMap.value(itag, UnknownQuality);
}

QString YoutubeVideoUrlParser::videoQualityToLabel(YoutubeVideoUrlParser::YoutubeVideoQuality quality)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<YoutubeVideoUrlParser::YoutubeVideoQuality>();
    // Convert to string, strip non-digits and add "p"
    return QString(metaEnum.valueToKey(quality)).remove(QRegularExpression("[^0-9.]")) + "p";
}


QSize YoutubeVideoUrlParser::videoQualityToResolution(YoutubeVideoUrlParser::YoutubeVideoQuality quality)
{
    static QMap<YoutubeVideoQuality, QSize> videoQualityToResolutionMap =
    {
        {Low144, QSize(256, 144)},
        {Low240, QSize(426, 240)},
        {Medium360, QSize(640, 360)},
        {Medium480, QSize(854, 480)},
        {High720, QSize(1280, 720)},
        {High1080, QSize(1920, 1080)},
        {High1440, QSize(2560, 1440)},
        {High2160, QSize(3840, 2160)},
        {High2880, QSize(5120, 2880)},
        {High3072, QSize(4096, 3072)},
        {High4320, QSize(7680, 4320)}
    };

    return videoQualityToResolutionMap.value(quality, QSize(0, 0));
}

YoutubeVideoUrlParser::YoutubeVideoQuality YoutubeVideoUrlParser::videoQualityFromLabel(const QString& label)
{
    if (label.startsWith("144p", Qt::CaseInsensitive))
        return Low144;

    if (label.startsWith("240p", Qt::CaseInsensitive))
        return Low240;

    if (label.startsWith("360p", Qt::CaseInsensitive))
        return Medium360;

    if (label.startsWith("480p", Qt::CaseInsensitive))
        return Medium480;

    if (label.startsWith("720p", Qt::CaseInsensitive))
        return High720;

    if (label.startsWith("1080p", Qt::CaseInsensitive))
        return High1080;

    if (label.startsWith("1440p", Qt::CaseInsensitive))
        return High1440;

    if (label.startsWith("2160p", Qt::CaseInsensitive))
        return High2160;

    if (label.startsWith("2880p", Qt::CaseInsensitive))
        return High2880;

    if (label.startsWith("3072p", Qt::CaseInsensitive))
        return High3072;

    if (label.startsWith("4320p", Qt::CaseInsensitive))
        return High4320;

    // Unrecognized
    return UnknownQuality;
}
