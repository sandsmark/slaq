#include "qquicktextdocument.h"
#include "slaqtextdocument_p.h"

#include "private/qquicktextedit_p.h"
#include "private/qquicktextedit_p_p.h"
#include "private/qquicktext_p_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/private/qquickpixmapcache_p.h>

QT_BEGIN_NAMESPACE


SlaqTextDocumentWithImageResources::SlaqTextDocumentWithImageResources(QQuickItem *parent)
: QTextDocument(parent), outstanding(0)
{
    setUndoRedoEnabled(false);
    documentLayout()->registerHandler(QTextFormat::ImageObject, this);
    connect(this, SIGNAL(baseUrlChanged(QUrl)), this, SLOT(reset()));
}

SlaqTextDocumentWithImageResources::~SlaqTextDocumentWithImageResources()
{
    if (!m_resources.isEmpty())
        qDeleteAll(m_resources);
}

QVariant SlaqTextDocumentWithImageResources::loadResource(int type, const QUrl &name)
{
    QVariant resource = QTextDocument::loadResource(type, name);
    if (resource.isNull() && type == QTextDocument::ImageResource) {
        QQmlContext *context = qmlContext(parent());
        QUrl url = baseUrl().resolved(name);
        QQuickPixmap *p = loadPixmap(context, url);
        resource = p->image();
    }

    return resource;
}

void SlaqTextDocumentWithImageResources::requestFinished()
{
    outstanding--;
    if (outstanding == 0) {
        markContentsDirty(0, characterCount());
        emit imagesLoaded();
    }
}

QSizeF SlaqTextDocumentWithImageResources::intrinsicSize(
        QTextDocument *, int, const QTextFormat &format)
{
    if (format.isImageFormat()) {
        QTextImageFormat imageFormat = format.toImageFormat();

        const int width = qRound(imageFormat.width());
        const bool hasWidth = imageFormat.hasProperty(QTextFormat::ImageWidth) && width > 0;
        const int height = qRound(imageFormat.height());
        const bool hasHeight = imageFormat.hasProperty(QTextFormat::ImageHeight) && height > 0;

        QSizeF size(width, height);
        if (!hasWidth || !hasHeight) {
            QVariant res = resource(QTextDocument::ImageResource, QUrl(imageFormat.name()));
            QImage image = res.value<QImage>();
            if (image.isNull()) {
                if (!hasWidth)
                    size.setWidth(16);
                if (!hasHeight)
                    size.setHeight(16);
                return size;
            }
            QSize imgSize = image.size();

            if (!hasWidth) {
                if (!hasHeight)
                    size.setWidth(imgSize.width());
                else
                    size.setWidth(qRound(height * (imgSize.width() / (qreal) imgSize.height())));
            }
            if (!hasHeight) {
                if (!hasWidth)
                    size.setHeight(imgSize.height());
                else
                    size.setHeight(qRound(width * (imgSize.height() / (qreal) imgSize.width())));
            }
        }
        return size;
    }
    return QSizeF();
}

void SlaqTextDocumentWithImageResources::drawObject(
        QPainter *, const QRectF &, QTextDocument *, int, const QTextFormat &)
{
}

QImage SlaqTextDocumentWithImageResources::image(const QTextImageFormat &format) const
{
    QVariant res = resource(QTextDocument::ImageResource, QUrl(format.name()));
    return res.value<QImage>();
}

void SlaqTextDocumentWithImageResources::reset()
{
    clearResources();
    markContentsDirty(0, characterCount());
}

QQuickPixmap *SlaqTextDocumentWithImageResources::loadPixmap(
        QQmlContext *context, const QUrl &url)
{

    QHash<QUrl, QQuickPixmap *>::Iterator iter = m_resources.find(url);

    if (iter == m_resources.end()) {
        QQuickPixmap *p = new QQuickPixmap(context->engine(), url);
        iter = m_resources.insert(url, p);

        if (p->isLoading()) {
            p->connectFinished(this, SLOT(requestFinished()));
            outstanding++;
        }
    }

    QQuickPixmap *p = *iter;
    if (p->isError()) {
        if (!errors.contains(url)) {
            errors.insert(url);
            qmlWarning(parent()) << p->error();
        }
    }
    return p;
}

void SlaqTextDocumentWithImageResources::clearResources()
{
    for (QQuickPixmap *pixmap : qAsConst(m_resources))
        pixmap->clear(this);
    qDeleteAll(m_resources);
    m_resources.clear();
    outstanding = 0;
}

void SlaqTextDocumentWithImageResources::setText(const QString &text)
{
    clearResources();

#if QT_CONFIG(texthtmlparser)
    setHtml(text);
#else
    setPlainText(text);
#endif
}

QSet<QUrl> SlaqTextDocumentWithImageResources::errors;

QT_END_NAMESPACE
