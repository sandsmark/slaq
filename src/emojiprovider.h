#pragma once

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
    AsyncImageResponse(const QString &id, const QSize &requestedSize);

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
    QThreadPool m_pool;
};

