#ifndef EMOJIPROVIDER_H
#define EMOJIPROVIDER_H

#include <qqmlengine.h>
#include <qquickimageprovider.h>
#include <QDebug>
#include <QImage>
#include <QThreadPool>
#include <QPointer>
#include "imagescache.h"

class AsyncImageResponse : public QQuickImageResponse, public QRunnable
{
public:
    AsyncImageResponse(const QString &id, const QSize &requestedSize, QPointer<ImagesCache> imageCache);

    QQuickTextureFactory *textureFactory() const;

    void run();

public slots:
    void onImageLoaded(const QString& id);
private:
    QString m_id;
    QSize m_requestedSize;
    QImage m_image;
    QPointer<ImagesCache> m_imageCache;
};

class EmojiProvider : public QQuickAsyncImageProvider
{
public:
    EmojiProvider();

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize);

private:
    QThreadPool pool;
    ImagesCache imageCache;
};

#endif // EMOJIPROVIDER_H
