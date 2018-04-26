#include "emojiprovider.h"

EmojiProvider::EmojiProvider()
{
    pool.setMaxThreadCount(QThread::idealThreadCount());
}

QQuickImageResponse *EmojiProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    AsyncImageResponse *response = new AsyncImageResponse(id, requestedSize, QPointer<ImagesCache>(&imageCache));
    pool.start(response);
    return response;
}

AsyncImageResponse::AsyncImageResponse(const QString &id, const QSize &requestedSize, QPointer<ImagesCache> imageCache)
    : m_id(id), m_requestedSize(requestedSize), m_imageCache(imageCache)
{
    setAutoDelete(false);
    connect(imageCache.data(), &ImagesCache::imageLoaded, this, &AsyncImageResponse::onImageLoaded);
}

QQuickTextureFactory *AsyncImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void AsyncImageResponse::run()
{
    //preload database from cache
    if (!m_imageCache->isImagesDatabaseLoaded()) {
        m_imageCache->loadImagesDatabase();
    }
    m_image = m_imageCache->image(m_id);

    if (!m_image.isNull()) {

        if (m_requestedSize.isValid())
            m_image = m_image.scaled(m_requestedSize);

        emit finished();
    }
}

void AsyncImageResponse::onImageLoaded(const QString &id)
{
    if (id == m_id) {
        m_image = m_imageCache->image(m_id);
        if (m_requestedSize.isValid())
            m_image = m_image.scaled(m_requestedSize);
        //qDebug() << "loaded image" << id;
        emit finished();
    }
}
