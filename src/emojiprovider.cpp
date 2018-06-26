#include "emojiprovider.h"

EmojiProvider::EmojiProvider()
{
    m_pool.setMaxThreadCount(QThread::idealThreadCount());
}

QQuickImageResponse *EmojiProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    AsyncImageResponse *response = new AsyncImageResponse(id, requestedSize);
    m_pool.start(response);
    return response;
}

AsyncImageResponse::AsyncImageResponse(const QString &id, const QSize &requestedSize)
    : m_id(id), m_requestedSize(requestedSize), m_imageCache(ImagesCache::instance())
{
    setAutoDelete(false);
    connect(m_imageCache, &ImagesCache::imageLoaded, this, &AsyncImageResponse::onImageLoaded);
}

QQuickTextureFactory *AsyncImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void AsyncImageResponse::run()
{
    m_image = m_imageCache->image(m_id);

    if (!m_image.isNull()) {

        if (m_requestedSize.isValid()) {
            m_image = m_image.scaled(m_requestedSize);
        }

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
