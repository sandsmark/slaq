#pragma once


#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>
#include <QtGui/qimage.h>
#include <QtGui/qtextdocument.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qtextlayout.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickPixmap;
class QQmlContext;

class Q_AUTOTEST_EXPORT SlaqTextDocumentWithImageResources : public QTextDocument, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    SlaqTextDocumentWithImageResources(QQuickItem *parent);
    virtual ~SlaqTextDocumentWithImageResources();

    void setText(const QString &);
    int resourcesLoading() const { return outstanding; }

    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) override;
    void drawObject(QPainter *p, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format) override;

    QImage image(const QTextImageFormat &format) const;

public Q_SLOTS:
    void clearResources();

Q_SIGNALS:
    void imagesLoaded();

protected:
    QVariant loadResource(int type, const QUrl &name) override;

    QQuickPixmap *loadPixmap(QQmlContext *context, const QUrl &name);

private Q_SLOTS:
    void reset();
    void requestFinished();

private:
    QHash<QUrl, QQuickPixmap *> m_resources;

    int outstanding;
    static QSet<QUrl> errors;
};

QT_END_NAMESPACE
