#ifndef EMOJIPROVIDER_H
#define EMOJIPROVIDER_H

#include <qqmlengine.h>
#include <qquickimageprovider.h>
#include <QDebug>
#include <QImage>
#include <QThreadPool>
#include "imagescache.h"

class AsyncImageResponse : public QQuickImageResponse, public QRunnable
{
    public:
        AsyncImageResponse(const QString &id, const QSize &requestedSize);

        QQuickTextureFactory *textureFactory() const;

        void run();

        QString m_id;
        QSize m_requestedSize;
        QImage m_image;
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
