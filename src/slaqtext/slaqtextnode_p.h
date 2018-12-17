#pragma once

#include <QtQuick/qsgnode.h>
#include "private/qquicktext_p.h"
#include <qglyphrun.h>

#include <QtGui/qcolor.h>
#include <QtGui/qtextlayout.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QSGGlyphNode;
class QTextBlock;
class QColor;
class QTextDocument;
class QSGContext;
class QRawFont;
class QSGInternalRectangleNode;
class QSGClipNode;
class QSGTexture;

class SlaqTextNodeEngine;

class SlaqTextNode : public QSGTransformNode
{
public:
    SlaqTextNode(QQuickItem *ownerElement);
    ~SlaqTextNode();

    static bool isComplexRichText(QTextDocument *);

    void deleteContent();
    void addTextLayout(const QPointF &position, QTextLayout *textLayout, const QColor &color = QColor(),
                       QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                       const QColor &anchorColor = QColor(),
                       const QColor &selectionColor = QColor(), const QColor &selectedTextColor = QColor(),
                       int selectionStart = -1, int selectionEnd = -1,
                       int lineStart = 0, int lineCount = -1);
    void addTextDocument(const QPointF &position, QTextDocument *textDocument, const QColor &color = QColor(),
                         QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                         const QColor &anchorColor = QColor(),
                         const QColor &selectionColor = QColor(), const QColor &selectedTextColor = QColor(),
                         int selectionStart = -1, int selectionEnd = -1);

    void setCursor(const QRectF &rect, const QColor &color);
    void clearCursor();
    QSGInternalRectangleNode *cursorNode() const { return m_cursorNode; }

    QSGGlyphNode *addGlyphs(const QPointF &position, const QGlyphRun &glyphs, const QColor &color,
                            QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                            QSGNode *parentNode = 0);
    void addImage(const QRectF &rect, const QImage &image);
    void addRectangleNode(const QRectF &rect, const QColor &color, bool border = false);

    bool useNativeRenderer() const { return m_useNativeRenderer; }
    void setUseNativeRenderer(bool on) { m_useNativeRenderer = on; }

private:
    QSGInternalRectangleNode *m_cursorNode;
    QList<QSGTexture *> m_textures;
    QQuickItem *m_ownerElement;
    bool m_useNativeRenderer;

    friend class QQuickTextEdit;
    friend class QQuickTextEditPrivate;
};

QT_END_NAMESPACE
