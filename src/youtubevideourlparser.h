#ifndef YOUTUBEVIDEOURLPARSER_H
#define YOUTUBEVIDEOURLPARSER_H

#include <QObject>
#include <QtNetwork>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

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
};

class YoutubeVideoUrlParser : public QObject
{
    Q_OBJECT
public:
    explicit YoutubeVideoUrlParser(QObject *parent = nullptr);

public slots:
    void requestUrl(const QUrl& url);
    QString parseVideoId(const QString &videoUrl);

private slots:
    void onPlayerConfigChanged(PlayerConfiguration* playerConfig);
    QList<QPair<QString, int> > getCypherOperations(PlayerConfiguration* playerConfig);
protected slots:
    void finished(QNetworkReply *reply);
    void error(QNetworkReply::NetworkError error);
signals:
    void urlParsed(const QString& videoUrl);
    void playerConfigChanged(PlayerConfiguration* playerConfig);

private:
    QUrl m_url;
    QNetworkAccessManager manager;
    QRegularExpression m_youtubeIdRegular;
    QRegularExpression m_youtubeIdShort;
    QRegularExpression m_youtubeIdEmbed;
    QRegularExpression m_youtubePlayerEmbed;

    QHash<QString, PlayerConfiguration*> m_youtubeRequests;
};

#endif // YOUTUBEVIDEOURLPARSER_H
