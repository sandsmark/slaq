#include "emojiprovider.h"

EmojiProvider::EmojiProvider()
{
    pool.setMaxThreadCount(QThread::idealThreadCount());
}

QQuickImageResponse *EmojiProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    AsyncImageResponse *response = new AsyncImageResponse(id, requestedSize);
    pool.start(response);
    return response;
}

AsyncImageResponse::AsyncImageResponse(const QString &id, const QSize &requestedSize)
    : m_id(id), m_requestedSize(requestedSize)
{
    setAutoDelete(false);
}

QQuickTextureFactory *AsyncImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void AsyncImageResponse::run()
{
    m_image = QImage(50, 50, QImage::Format_RGB32);
    if (m_id == "slow") {
        m_image.fill(Qt::red);
    } else {
        m_image.fill(Qt::blue);
    }
    if (m_requestedSize.isValid())
        m_image = m_image.scaled(m_requestedSize);

    emit finished();
}
