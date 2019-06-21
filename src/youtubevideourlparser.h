#ifndef YOUTUBEVIDEOURLPARSER_H
#define YOUTUBEVIDEOURLPARSER_H

#include <QObject>
#include <QtNetwork>
#include <QRegularExpression>
#include <QRegularExpressionMatch>



class YoutubeVideoUrlParser : public QObject
{
    Q_OBJECT
public:
    enum YoutubeAudioCodec
    {
        UnknownACodec = -1,
        Aac = 0, // MPEG-4 Part 3, Advanced Audio Coding (AAC).
        Vorbis,
        Opus
    };
    Q_ENUM(YoutubeAudioCodec)

    enum YoutubeContainer
    {
        UnknownContainer = -1,
        Mp4 = 0,  // MPEG-4 Part 14 (.mp4).
        WebM, // Web Media (.webm).
        Tgpp  // 3rd Generation Partnership Project (.3gpp).
    };
    Q_ENUM(YoutubeContainer)

    enum YoutubeVideoCodec
    {
        UnknownVCodec = -1,
        Mp4V = 0, // MPEG-4 Part 2.
        H263, // H263.Obsoleted
        H264, // MPEG-4 Part 10, H264, Advanced Video Coding (AVC).
        Vp8,
        Vp9,
        Av1
    };
    Q_ENUM(YoutubeVideoCodec)

    enum YoutubeVideoQuality
    {
        UnknownQuality = -1,
        Low144 = 0,  // Low quality (144p).
        Low240,
        Medium360, // Medium quality (360p).
        Medium480,
        High720, // High quality (720p).
        High1080,
        High1440,
        High2160,
        High2880,
        High3072,
        High4320
    };
    Q_ENUM(YoutubeVideoQuality)

    struct MediaStreamInfo {
        QUrl playableUrl;
        qint64 contentLength;
        int itag = -1;
        YoutubeContainer container = UnknownContainer;
        YoutubeAudioCodec acodec = UnknownACodec;
        YoutubeVideoCodec vcodec = UnknownVCodec;
        YoutubeVideoQuality quality = UnknownQuality;
        QString videoQualityLabel;
        QSize resolution;
        int framerate = -1;
        int bitrate = -1;
    };

    struct PlayerConfiguration
    {
        bool succeed = false;
        QString videoId;
        bool isLiveStream = false;
        QString playerSourceUrl;
        QString manifestUrl;
        QString muxedStreamInfosUrlEncoded;
        QString adaptiveStreamInfosUrlEncoded;
        QDateTime validUntil;
        QMultiMap<int, MediaStreamInfo> streams;
    };

    explicit YoutubeVideoUrlParser(QObject *parent = nullptr);

public slots:
    void requestVideoUrl(const QString& videoId);
    QString parseVideoId(const QString &videoUrl);

    void dump(PlayerConfiguration *pc);
private slots:
    void onPlayerConfigChanged(PlayerConfiguration* playerConfig);
    QList<QPair<QString, int> > getCipherOperations(PlayerConfiguration* playerConfig);

signals:
    void urlParsed(const QString& videoId, const QUrl& playUrl);
    void playerConfigChanged(PlayerConfiguration* playerConfig);

private:
    YoutubeContainer parseContainer(const QString& container);
    YoutubeVideoUrlParser::YoutubeAudioCodec parseAudioCodec(const QString &codec);
    YoutubeVideoUrlParser::YoutubeVideoCodec parseVideoCodec(const QString &codec);
    YoutubeVideoUrlParser::YoutubeVideoQuality itagToQuality(int itag);
    QString videoQualityToLabel(YoutubeVideoUrlParser::YoutubeVideoQuality quality);
    QSize videoQualityToResolution(YoutubeVideoUrlParser::YoutubeVideoQuality quality);
    YoutubeVideoUrlParser::YoutubeVideoQuality videoQualityFromLabel(const QString &label);
    void checkContentLengthAndRedirections(const QUrl& url, MediaStreamInfo& msi);
    void pickBestPossibleVideo(PlayerConfiguration *pc);

private:
    QNetworkAccessManager manager;
    QRegularExpression m_youtubeIdRegular;
    QRegularExpression m_youtubeIdShort;
    QRegularExpression m_youtubeIdEmbed;
    QRegularExpression m_youtubePlayerEmbed;

    QHash<QString, PlayerConfiguration*> m_youtubeRequests;
    QHash<QString, QList<QPair<QString, int>> > m_cipherCache;

};



#endif // YOUTUBEVIDEOURLPARSER_H
