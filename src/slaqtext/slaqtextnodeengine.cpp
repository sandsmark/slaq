#include "slaqtextnodeengine_p.h"

#include <QtCore/qpoint.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qrawfont.h>
#include <QtGui/qtextdocument.h>
#include <QtGui/qtextlayout.h>
#include <QtGui/qtextobject.h>
#include <QtGui/qtexttable.h>
#include <QtGui/qtextlist.h>

#include <private/qquicktext_p.h>
#include "slaqtextdocument_p.h"
#include <private/qtextdocumentlayout_p.h>
#include <private/qtextimagehandler_p.h>
#include <private/qrawfont_p.h>
#include <private/qglyphrun_p.h>

QT_BEGIN_NAMESPACE

SlaqTextNodeEngine::BinaryTreeNodeKey::BinaryTreeNodeKey(BinaryTreeNode *node)
    : fontEngine(QRawFontPrivate::get(node->glyphRun.rawFont())->fontEngine)
    , clipNode(node->clipNode)
    , color(node->color.rgba())
    , selectionState(node->selectionState)
{
}

SlaqTextNodeEngine::BinaryTreeNode::BinaryTreeNode(const QGlyphRun &g,
                                                     SelectionState selState,
                                                     const QRectF &brect,
                                                     const Decorations &decs,
                                                     const QColor &c,
                                                     const QColor &bc,
                                                     const QPointF &pos, qreal a)
    : glyphRun(g)
    , boundingRect(brect)
    , selectionState(selState)
    , clipNode(nullptr)
    , decorations(decs)
    , color(c)
    , backgroundColor(bc)
    , position(pos)
    , ascent(a)
    , leftChildIndex(-1)
    , rightChildIndex(-1)
{
    QGlyphRunPrivate *d = QGlyphRunPrivate::get(g);
    ranges.append(qMakePair(d->textRangeStart, d->textRangeEnd));
}


void SlaqTextNodeEngine::BinaryTreeNode::insert(QVarLengthArray<BinaryTreeNode, 16> *binaryTree, const QGlyphRun &glyphRun, SelectionState selectionState,
                                             Decorations decorations, const QColor &textColor,
                                             const QColor &backgroundColor, const QPointF &position)
{
    QRectF searchRect = glyphRun.boundingRect();
    searchRect.translate(position);

    if (qFuzzyIsNull(searchRect.width()) || qFuzzyIsNull(searchRect.height()))
        return;

    decorations |= (glyphRun.underline() ? Decoration::Underline : Decoration::NoDecoration);
    decorations |= (glyphRun.overline()  ? Decoration::Overline  : Decoration::NoDecoration);
    decorations |= (glyphRun.strikeOut() ? Decoration::StrikeOut : Decoration::NoDecoration);
    decorations |= (backgroundColor.isValid() ? Decoration::Background : Decoration::NoDecoration);

    qreal ascent = glyphRun.rawFont().ascent();
    insert(binaryTree, BinaryTreeNode(glyphRun,
                                      selectionState,
                                      searchRect,
                                      decorations,
                                      textColor,
                                      backgroundColor,
                                      position,
                                      ascent));
}

void SlaqTextNodeEngine::BinaryTreeNode::insert(QVarLengthArray<BinaryTreeNode, 16> *binaryTree, const BinaryTreeNode &binaryTreeNode)
{
    int newIndex = binaryTree->size();
    binaryTree->append(binaryTreeNode);
    if (newIndex == 0)
        return;

    int searchIndex = 0;
    forever {
        BinaryTreeNode *node = binaryTree->data() + searchIndex;
        if (binaryTreeNode.boundingRect.left() < node->boundingRect.left()) {
            if (node->leftChildIndex < 0) {
                node->leftChildIndex = newIndex;
                break;
            } else {
                searchIndex = node->leftChildIndex;
            }
        } else {
            if (node->rightChildIndex < 0) {
                node->rightChildIndex = newIndex;
                break;
            } else {
                searchIndex = node->rightChildIndex;
            }
        }
    }
}

void SlaqTextNodeEngine::BinaryTreeNode::inOrder(const QVarLengthArray<BinaryTreeNode, 16> &binaryTree,
                                              QVarLengthArray<int> *sortedIndexes, int currentIndex)
{
    Q_ASSERT(currentIndex < binaryTree.size());

    const BinaryTreeNode *node = binaryTree.data() + currentIndex;
    if (node->leftChildIndex >= 0)
        inOrder(binaryTree, sortedIndexes, node->leftChildIndex);

    sortedIndexes->append(currentIndex);

    if (node->rightChildIndex >= 0)
        inOrder(binaryTree, sortedIndexes, node->rightChildIndex);
}


int SlaqTextNodeEngine::addText(const QTextBlock &block,
                                  const QTextCharFormat &charFormat,
                                  const QColor &textColor,
                                  const QVarLengthArray<QTextLayout::FormatRange> &colorChanges,
                                  int textPos, int fragmentEnd,
                                  int selectionStart, int selectionEnd)
{
    if (charFormat.foreground().style() != Qt::NoBrush)
        setTextColor(charFormat.foreground().color());
    else
        setTextColor(textColor);

    //qDebug() << "add text" <<  block.text() << m_textColor << selectionStart << selectionEnd << block.position();
    while (textPos < fragmentEnd) {
        int blockRelativePosition = textPos - block.position();
        QTextLine line = block.layout()->lineForTextPosition(blockRelativePosition);
        if (!currentLine().isValid()
                || line.lineNumber() != currentLine().lineNumber()) {
            setCurrentLine(line);
        }

        Q_ASSERT(line.textLength() > 0);
        int lineEnd = line.textStart() + block.position() + line.textLength();

        int len = qMin(lineEnd - textPos, fragmentEnd - textPos);
        Q_ASSERT(len > 0);

        int currentStepEnd = textPos + len;

        addGlyphsForRanges(colorChanges,
                           textPos - block.position(),
                           currentStepEnd - block.position(),
                           selectionStart - block.position(),
                           selectionEnd - block.position());

        textPos = currentStepEnd;
    }
    return textPos;
}

void SlaqTextNodeEngine::addTextDecorations(const QVarLengthArray<TextDecoration> &textDecorations,
                                              qreal offset, qreal thickness)
{
    for (int i=0; i<textDecorations.size(); ++i) {
        TextDecoration textDecoration = textDecorations.at(i);

        {
            QRectF &rect = textDecoration.rect;
            rect.setY(qRound(rect.y()
                             + m_currentLine.ascent()
                             + (m_currentLine.leadingIncluded() ? m_currentLine.leading() : qreal(0.0f))
                             + offset));
            rect.setHeight(thickness);
        }

        m_lines.append(textDecoration);
    }
}

void SlaqTextNodeEngine::processCurrentLine()
{
    // No glyphs, do nothing
    if (m_currentLineTree.isEmpty())
        return;

    // 1. Go through current line and get correct decoration position for each node based on
    // neighbouring decorations. Add decoration to global list
    // 2. Create clip nodes for all selected text. Try to merge as many as possible within
    // the line.
    // 3. Add QRects to a list of selection rects.
    // 4. Add all nodes to a global processed list
    QVarLengthArray<int> sortedIndexes; // Indexes in tree sorted by x position
    BinaryTreeNode::inOrder(m_currentLineTree, &sortedIndexes);

    Q_ASSERT(sortedIndexes.size() == m_currentLineTree.size());

    SelectionState currentSelectionState = Unselected;
    QRectF currentRect;

    Decorations currentDecorations = Decoration::NoDecoration;
    qreal underlineOffset = 0.0;
    qreal underlineThickness = 0.0;

    qreal overlineOffset = 0.0;
    qreal overlineThickness = 0.0;

    qreal strikeOutOffset = 0.0;
    qreal strikeOutThickness = 0.0;

    QRectF decorationRect = currentRect;

    QColor lastColor;
    QColor lastBackgroundColor;

    QVarLengthArray<TextDecoration> pendingUnderlines;
    QVarLengthArray<TextDecoration> pendingOverlines;
    QVarLengthArray<TextDecoration> pendingStrikeOuts;
    if (!sortedIndexes.isEmpty()) {
        QQuickDefaultClipNode *currentClipNode = m_hasSelection ? new QQuickDefaultClipNode(QRectF()) : nullptr;
        bool currentClipNodeUsed = false;
        for (int i=0; i<=sortedIndexes.size(); ++i) {
            BinaryTreeNode *node = nullptr;
            if (i < sortedIndexes.size()) {
                int sortedIndex = sortedIndexes.at(i);
                Q_ASSERT(sortedIndex < m_currentLineTree.size());

                node = m_currentLineTree.data() + sortedIndex;
            }

            if (i == 0)
                currentSelectionState = node->selectionState;

            // Update decorations
            if (currentDecorations != Decoration::NoDecoration) {
                decorationRect.setY(m_position.y() + m_currentLine.y());
                decorationRect.setHeight(m_currentLine.height());

                if (node != nullptr)
                    decorationRect.setRight(node->boundingRect.left());

                TextDecoration textDecoration(currentSelectionState, decorationRect, lastColor);
                if (currentDecorations & Decoration::Underline)
                    pendingUnderlines.append(textDecoration);

                if (currentDecorations & Decoration::Overline)
                    pendingOverlines.append(textDecoration);

                if (currentDecorations & Decoration::StrikeOut)
                    pendingStrikeOuts.append(textDecoration);

                if (currentDecorations & Decoration::Background)
                    m_backgrounds.append(qMakePair(decorationRect, lastBackgroundColor));
            }

            // If we've reached an unselected node from a selected node, we add the
            // selection rect to the graph, and we add decoration every time the
            // selection state changes, because that means the text color changes
            if (node == nullptr || node->selectionState != currentSelectionState) {
                currentRect.setY(m_position.y() + m_currentLine.y());
                currentRect.setHeight(m_currentLine.height());

                if (currentSelectionState == Selected)
                    m_selectionRects.append(currentRect);

                if (currentClipNode != nullptr) {
                    if (!currentClipNodeUsed) {
                        delete currentClipNode;
                    } else {
                        currentClipNode->setIsRectangular(true);
                        currentClipNode->setRect(currentRect);
                        currentClipNode->update();
                    }
                }

                if (node != nullptr && m_hasSelection)
                    currentClipNode = new QQuickDefaultClipNode(QRectF());
                else
                    currentClipNode = nullptr;
                currentClipNodeUsed = false;

                if (node != nullptr) {
                    currentSelectionState = node->selectionState;
                    currentRect = node->boundingRect;

                    // Make sure currentRect is valid, otherwise the unite won't work
                    if (currentRect.isNull())
                        currentRect.setSize(QSizeF(1, 1));
                }
            } else {
                if (currentRect.isNull())
                    currentRect = node->boundingRect;
                else
                    currentRect = currentRect.united(node->boundingRect);
            }

            if (node != nullptr) {
                if (node->selectionState == Selected) {
                    node->clipNode = currentClipNode;
                    currentClipNodeUsed = true;
                }

                decorationRect = node->boundingRect;

                // If previous item(s) had underline and current does not, then we add the
                // pending lines to the lists and likewise for overlines and strikeouts
                if (!pendingUnderlines.isEmpty()
                        && !(node->decorations & Decoration::Underline)) {
                    addTextDecorations(pendingUnderlines, underlineOffset, underlineThickness);

                    pendingUnderlines.clear();

                    underlineOffset = 0.0;
                    underlineThickness = 0.0;
                }

                // ### Add pending when overlineOffset/thickness changes to minimize number of
                // nodes
                if (!pendingOverlines.isEmpty()) {
                    addTextDecorations(pendingOverlines, overlineOffset, overlineThickness);

                    pendingOverlines.clear();

                    overlineOffset = 0.0;
                    overlineThickness = 0.0;
                }

                // ### Add pending when overlineOffset/thickness changes to minimize number of
                // nodes
                if (!pendingStrikeOuts.isEmpty()) {
                    addTextDecorations(pendingStrikeOuts, strikeOutOffset, strikeOutThickness);

                    pendingStrikeOuts.clear();

                    strikeOutOffset = 0.0;
                    strikeOutThickness = 0.0;
                }

                // Merge current values with previous. Prefer greatest thickness
                QRawFont rawFont = node->glyphRun.rawFont();
                if (node->decorations & Decoration::Underline) {
                    if (rawFont.lineThickness() > underlineThickness) {
                        underlineThickness = rawFont.lineThickness();
                        underlineOffset = rawFont.underlinePosition();
                    }
                }

                if (node->decorations & Decoration::Overline) {
                    overlineOffset = -rawFont.ascent();
                    overlineThickness = rawFont.lineThickness();
                }

                if (node->decorations & Decoration::StrikeOut) {
                    strikeOutThickness = rawFont.lineThickness();
                    strikeOutOffset = rawFont.ascent() / -3.0;
                }

                currentDecorations = node->decorations;
                lastColor = node->color;
                lastBackgroundColor = node->backgroundColor;
                m_processedNodes.append(*node);
            }
        }

        if (!pendingUnderlines.isEmpty())
            addTextDecorations(pendingUnderlines, underlineOffset, underlineThickness);

        if (!pendingOverlines.isEmpty())
            addTextDecorations(pendingOverlines, overlineOffset, overlineThickness);

        if (!pendingStrikeOuts.isEmpty())
            addTextDecorations(pendingStrikeOuts, strikeOutOffset, strikeOutThickness);
    }

    m_currentLineTree.clear();
    m_currentLine = QTextLine();
    m_hasSelection = false;
}

void SlaqTextNodeEngine::addImage(const QRectF &rect, const QImage &image, qreal ascent,
                                    SelectionState selectionState,
                                    QTextFrameFormat::Position layoutPosition)
{
    QRectF searchRect = rect;
    if (layoutPosition == QTextFrameFormat::InFlow) {
        if (m_currentLineTree.isEmpty()) {
            qreal y = m_currentLine.ascent() - ascent;
            if (m_currentTextDirection == Qt::RightToLeft)
                searchRect.moveTopRight(m_position + m_currentLine.rect().topRight() + QPointF(0, y));
            else
                searchRect.moveTopLeft(m_position + m_currentLine.position() + QPointF(0, y));
        } else {
            const BinaryTreeNode *lastNode = m_currentLineTree.data() + m_currentLineTree.size() - 1;

            if (lastNode->glyphRun.isRightToLeft()) {
                QPointF lastPos = lastNode->boundingRect.topLeft();
                searchRect.moveTopRight(lastPos - QPointF(0, ascent - lastNode->ascent));
            } else {
                QPointF lastPos = lastNode->boundingRect.topRight();
                searchRect.moveTopLeft(lastPos - QPointF(0, ascent - lastNode->ascent));
            }
        }
    }

    BinaryTreeNode::insert(&m_currentLineTree, searchRect, image, ascent, selectionState);
    m_hasContents = true;
}

void SlaqTextNodeEngine::addTextObject(const QTextBlock &block, const QPointF &position, const QTextCharFormat &format,
                                         SelectionState selectionState,
                                         QTextDocument *textDocument, int pos,
                                         QTextFrameFormat::Position layoutPosition)
{
    QTextObjectInterface *handler = textDocument->documentLayout()->handlerForObject(format.objectType());
    if (handler != nullptr) {
        QImage image;
        QSizeF size = handler->intrinsicSize(textDocument, pos, format);

        if (format.objectType() == QTextFormat::ImageObject) {
            QTextImageFormat imageFormat = format.toImageFormat();
            if (SlaqTextDocumentWithImageResources *imageDoc = static_cast<SlaqTextDocumentWithImageResources *>(textDocument)) {
                image = imageDoc->image(imageFormat);

                if (image.isNull())
                    return;
            } else {
                QTextImageHandler *imageHandler = static_cast<QTextImageHandler *>(handler);
                image = imageHandler->image(textDocument, imageFormat);
            }
        }

        if (image.isNull()) {
            image = QImage(size.toSize(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            {
                QPainter painter(&image);
                handler->drawObject(&painter, image.rect(), textDocument, pos, format);
            }
        }

        qreal ascent;
        QTextLine line = block.layout()->lineForTextPosition(pos);
        switch (format.verticalAlignment())
        {
        case QTextCharFormat::AlignTop:
            ascent = line.ascent();
            break;
        case QTextCharFormat::AlignMiddle: {
            QFontMetrics m(format.font());
            if (line.isValid()) {
                ascent = line.ascent() + (line.height() - size.height());
            } else {
                ascent = (size.height() - m.xHeight()) / 2;
            }
            break;
        }
        case QTextCharFormat::AlignBottom:
            ascent = size.height() - line.descent();

            break;
        case QTextCharFormat::AlignBaseline:
        default:
            ascent = size.height();
        }
        //qDebug() << __PRETTY_FUNCTION__ << ascent << currentLine().ascent() << currentLine().descent() << size << m_currentLineTree.isEmpty();

        addImage(QRectF(position, size), image, ascent, selectionState, layoutPosition);
    }
}

void SlaqTextNodeEngine::addUnselectedGlyphs(const QGlyphRun &glyphRun)
{
    BinaryTreeNode::insert(&m_currentLineTree,
                           glyphRun,
                           Unselected,
                           Decoration::NoDecoration,
                           m_textColor,
                           m_backgroundColor,
                           m_position);
}

void SlaqTextNodeEngine::addSelectedGlyphs(const QGlyphRun &glyphRun)
{
    int currentSize = m_currentLineTree.size();
    BinaryTreeNode::insert(&m_currentLineTree,
                           glyphRun,
                           Selected,
                           Decoration::NoDecoration,
                           m_textColor,
                           m_backgroundColor,
                           m_position);
    m_hasSelection = m_hasSelection || m_currentLineTree.size() > currentSize;
}

void SlaqTextNodeEngine::addGlyphsForRanges(const QVarLengthArray<QTextLayout::FormatRange> &ranges,
                                              int start, int end,
                                              int selectionStart, int selectionEnd)
{
    int currentPosition = start;
    int remainingLength = end - start;
    for (int j=0; j<ranges.size(); ++j) {
        const QTextLayout::FormatRange &range = ranges.at(j);
        if (range.start + range.length >= currentPosition
                && range.start < currentPosition + remainingLength) {

            if (range.start > currentPosition) {
                addGlyphsInRange(currentPosition, range.start - currentPosition,
                                 QColor(), QColor(), selectionStart, selectionEnd);
            }
            int rangeEnd = qMin(range.start + range.length, currentPosition + remainingLength);
            QColor rangeColor;
            if (range.format.hasProperty(QTextFormat::ForegroundBrush))
                rangeColor = range.format.foreground().color();
            else if (range.format.isAnchor())
                rangeColor = m_anchorColor;
            QColor rangeBackgroundColor = range.format.hasProperty(QTextFormat::BackgroundBrush)
                    ? range.format.background().color()
                    : QColor();

            addGlyphsInRange(range.start, rangeEnd - range.start,
                             rangeColor, rangeBackgroundColor,
                             selectionStart, selectionEnd);

            currentPosition = range.start + range.length;
            remainingLength = end - currentPosition;

        } else if (range.start > currentPosition + remainingLength || remainingLength <= 0) {
            break;
        }
    }

    if (remainingLength > 0) {
        addGlyphsInRange(currentPosition, remainingLength, QColor(), QColor(),
                         selectionStart, selectionEnd);
    }

}

void SlaqTextNodeEngine::addGlyphsInRange(int rangeStart, int rangeLength,
                                            const QColor &color, const QColor &backgroundColor,
                                            int selectionStart, int selectionEnd)
{
    QColor oldColor;
    if (color.isValid()) {
        oldColor = m_textColor;
        m_textColor = color;
    }

    QColor oldBackgroundColor = m_backgroundColor;
    if (backgroundColor.isValid()) {
        oldBackgroundColor = m_backgroundColor;
        m_backgroundColor = backgroundColor;
    }

    bool hasSelection = selectionEnd >= 0
            && selectionStart <= selectionEnd;

    QTextLine &line = m_currentLine;

    int rangeEnd = rangeStart + rangeLength;
    if (!hasSelection || (selectionStart > rangeEnd || selectionEnd < rangeStart)) {
        QList<QGlyphRun> glyphRuns = line.glyphRuns(rangeStart, rangeLength);
        for (int j=0; j<glyphRuns.size(); ++j) {
            const QGlyphRun &glyphRun = glyphRuns.at(j);
            addUnselectedGlyphs(glyphRun);
        }
    } else {
        //qDebug() << selectionStart << selectionEnd << rangeStart << rangeLength << m_textColor << m_backgroundColor;
        if (rangeStart < selectionStart) {
            int length = qMin(selectionStart - rangeStart, rangeLength);
            QList<QGlyphRun> glyphRuns = line.glyphRuns(rangeStart, length);
            for (int j=0; j<glyphRuns.size(); ++j) {
                const QGlyphRun &glyphRun = glyphRuns.at(j);
                addUnselectedGlyphs(glyphRun);
            }
        }

        if (rangeEnd > selectionStart) {
            int start = qMax(selectionStart, rangeStart);
            int length = qMin(selectionEnd - start + 1, rangeEnd - start);
            QList<QGlyphRun> glyphRuns = line.glyphRuns(start, length);

            for (int j=0; j<glyphRuns.size(); ++j) {
                const QGlyphRun &glyphRun = glyphRuns.at(j);
                addSelectedGlyphs(glyphRun);
            }
        }

        if (selectionEnd >= rangeStart && selectionEnd < rangeEnd) {
            int start = selectionEnd + 1;
            int length = rangeEnd - selectionEnd - 1;
            QList<QGlyphRun> glyphRuns = line.glyphRuns(start, length);
            for (int j=0; j<glyphRuns.size(); ++j) {
                const QGlyphRun &glyphRun = glyphRuns.at(j);
                addUnselectedGlyphs(glyphRun);
            }
        }
    }

    if (backgroundColor.isValid())
        m_backgroundColor = oldBackgroundColor;

    if (oldColor.isValid())
        m_textColor = oldColor;
}

void SlaqTextNodeEngine::addBorder(const QRectF &rect, qreal border,
                                     QTextFrameFormat::BorderStyle borderStyle,
                                     const QBrush &borderBrush)
{
    QColor color = borderBrush.color();

    // Currently we don't support other styles than solid
    Q_UNUSED(borderStyle);

    m_backgrounds.append(qMakePair(QRectF(rect.left(), rect.top(), border, rect.height() + border), color));
    m_backgrounds.append(qMakePair(QRectF(rect.left() + border, rect.top(), rect.width(), border), color));
    m_backgrounds.append(qMakePair(QRectF(rect.right(), rect.top() + border, border, rect.height() - border), color));
    m_backgrounds.append(qMakePair(QRectF(rect.left() + border, rect.bottom(), rect.width(), border), color));
}

void SlaqTextNodeEngine::addFrameDecorations(QTextDocument *document, QTextFrame *frame)
{
    QTextDocumentLayout *documentLayout = qobject_cast<QTextDocumentLayout *>(document->documentLayout());
    if (Q_UNLIKELY(!documentLayout))
        return;

    QTextFrameFormat frameFormat = frame->format().toFrameFormat();
    QTextTable *table = qobject_cast<QTextTable *>(frame);

    QRectF boundingRect = table == nullptr
            ? documentLayout->frameBoundingRect(frame)
            : documentLayout->tableBoundingRect(table);

    QBrush bg = frame->frameFormat().background();
    if (bg.style() != Qt::NoBrush)
        m_backgrounds.append(qMakePair(boundingRect, bg.color()));

    if (!frameFormat.hasProperty(QTextFormat::FrameBorder))
        return;

    qreal borderWidth = frameFormat.border();
    if (qFuzzyIsNull(borderWidth))
        return;

    QBrush borderBrush = frameFormat.borderBrush();
    QTextFrameFormat::BorderStyle borderStyle = frameFormat.borderStyle();
    if (borderStyle == QTextFrameFormat::BorderStyle_None)
        return;

    addBorder(boundingRect.adjusted(frameFormat.leftMargin(), frameFormat.topMargin(),
                                    -frameFormat.rightMargin(), -frameFormat.bottomMargin()),
              borderWidth, borderStyle, borderBrush);
    if (table != nullptr) {
        int rows = table->rows();
        int columns = table->columns();

        for (int row=0; row<rows; ++row) {
            for (int column=0; column<columns; ++column) {
                QTextTableCell cell = table->cellAt(row, column);

                QRectF cellRect = documentLayout->tableCellBoundingRect(table, cell);
                addBorder(cellRect.adjusted(-borderWidth, -borderWidth, 0, 0), borderWidth,
                          borderStyle, borderBrush);
            }
        }
    }
}

uint qHash(const SlaqTextNodeEngine::BinaryTreeNodeKey &key)
{
    // Just use the default hash for pairs
    return qHash(qMakePair(key.fontEngine, qMakePair(key.clipNode,
                                                     qMakePair(key.color, key.selectionState))));
}

void SlaqTextNodeEngine::mergeProcessedNodes(QList<BinaryTreeNode *> *regularNodes,
                                               QList<BinaryTreeNode *> *imageNodes)
{
    QHash<BinaryTreeNodeKey, QList<BinaryTreeNode *> > map;

    for (int i = 0; i < m_processedNodes.size(); ++i) {
        BinaryTreeNode *node = m_processedNodes.data() + i;

        if (node->image.isNull()) {
            BinaryTreeNodeKey key(node);

            QList<BinaryTreeNode *> &nodes = map[key];
            if (nodes.isEmpty())
                regularNodes->append(node);

            nodes.append(node);
        } else {
            imageNodes->append(node);
        }
    }

    for (int i = 0; i < regularNodes->size(); ++i) {
        BinaryTreeNode *primaryNode = regularNodes->at(i);
        BinaryTreeNodeKey key(primaryNode);

        const QList<BinaryTreeNode *> &nodes = map.value(key);
        Q_ASSERT(nodes.first() == primaryNode);

        int count = 0;
        for (int j = 0; j < nodes.size(); ++j)
            count += nodes.at(j)->glyphRun.glyphIndexes().size();

        if (count != primaryNode->glyphRun.glyphIndexes().size()) {
            QGlyphRun &glyphRun = primaryNode->glyphRun;
            QVector<quint32> glyphIndexes = glyphRun.glyphIndexes();
            glyphIndexes.reserve(count);

            QVector<QPointF> glyphPositions = glyphRun.positions();
            glyphPositions.reserve(count);

            QRectF glyphBoundingRect = glyphRun.boundingRect();

            for (int j = 1; j < nodes.size(); ++j) {
                BinaryTreeNode *otherNode = nodes.at(j);
                glyphIndexes += otherNode->glyphRun.glyphIndexes();
                primaryNode->ranges += otherNode->ranges;
                glyphBoundingRect = glyphBoundingRect.united(otherNode->boundingRect);

                QVector<QPointF> otherPositions = otherNode->glyphRun.positions();
                for (int k = 0; k < otherPositions.size(); ++k)
                    glyphPositions += otherPositions.at(k) + (otherNode->position - primaryNode->position);
            }

            Q_ASSERT(glyphPositions.size() == count);
            Q_ASSERT(glyphIndexes.size() == count);

            glyphRun.setGlyphIndexes(glyphIndexes);
            glyphRun.setPositions(glyphPositions);
            glyphRun.setBoundingRect(glyphBoundingRect);
        }
    }
}

void  SlaqTextNodeEngine::addToSceneGraph(SlaqTextNode *parentNode,
                                            QQuickText::TextStyle style,
                                            const QColor &styleColor)
{
    if (m_currentLine.isValid())
        processCurrentLine();

    QList<BinaryTreeNode *> nodes;
    QList<BinaryTreeNode *> imageNodes;
    mergeProcessedNodes(&nodes, &imageNodes);

    for (int i = 0; i < m_backgrounds.size(); ++i) {
        const QRectF &rect = m_backgrounds.at(i).first;
        const QColor &color = m_backgrounds.at(i).second;

        parentNode->addRectangleNode(rect, color);
    }

    // Add all text with unselected color first
    for (int i = 0; i < nodes.size(); ++i) {
        const BinaryTreeNode *node = nodes.at(i);
        parentNode->addGlyphs(node->position, node->glyphRun, node->color, style, styleColor, nullptr);
    }

    for (int i = 0; i < imageNodes.size(); ++i) {
        const BinaryTreeNode *node = imageNodes.at(i);
        if (node->selectionState == Unselected)
            parentNode->addImage(node->boundingRect, node->image);
    }

    // Then, prepend all selection rectangles to the tree
    for (int i = 0; i < m_selectionRects.size(); ++i) {
        const QRectF &rect = m_selectionRects.at(i);

        parentNode->addRectangleNode(rect, m_selectionColor);
    }

    // Add decorations for each node to the tree.
    for (int i = 0; i < m_lines.size(); ++i) {
        const TextDecoration &textDecoration = m_lines.at(i);

        QColor color = textDecoration.selectionState == Selected
                ? m_selectedTextColor
                : textDecoration.color;

        parentNode->addRectangleNode(textDecoration.rect, color);
    }

    // Finally add the selected text on top of everything
    for (int i = 0; i < nodes.size(); ++i) {
        const BinaryTreeNode *node = nodes.at(i);
        QQuickDefaultClipNode *clipNode = node->clipNode;
        if (clipNode != nullptr && clipNode->parent() == nullptr)
            parentNode->appendChildNode(clipNode);

        if (node->selectionState == Selected) {
            QColor color = m_selectedTextColor;
            int previousNodeIndex = i - 1;
            int nextNodeIndex = i + 1;
            const BinaryTreeNode *previousNode = previousNodeIndex < 0 ? 0 : nodes.at(previousNodeIndex);
            while (previousNode != nullptr && qFuzzyCompare(previousNode->boundingRect.left(), node->boundingRect.left()))
                previousNode = --previousNodeIndex < 0 ? 0 : nodes.at(previousNodeIndex);

            const BinaryTreeNode *nextNode = nextNodeIndex == nodes.size() ? 0 : nodes.at(nextNodeIndex);

            if (previousNode != nullptr && previousNode->selectionState == Unselected)
                parentNode->addGlyphs(previousNode->position, previousNode->glyphRun, color, style, styleColor, clipNode);

            if (nextNode != nullptr && nextNode->selectionState == Unselected)
                parentNode->addGlyphs(nextNode->position, nextNode->glyphRun, color, style, styleColor, clipNode);

            // If the previous or next node completely overlaps this one, then we have already drawn the glyphs of
            // this node
            bool drawCurrent = true;
            if (previousNode != nullptr || nextNode != nullptr) {
                for (int i = 0; i < node->ranges.size(); ++i) {
                    const QPair<int, int> &range = node->ranges.at(i);

                    int rangeLength = range.second - range.first + 1;
                    if (previousNode != nullptr) {
                        for (int j = 0; j < previousNode->ranges.size(); ++j) {
                            const QPair<int, int> &otherRange = previousNode->ranges.at(j);

                            if (range.first < otherRange.second && range.second > otherRange.first) {
                                int start = qMax(range.first, otherRange.first);
                                int end = qMin(range.second, otherRange.second);
                                rangeLength -= end - start + 1;
                                if (rangeLength == 0)
                                    break;
                            }
                        }
                    }

                    if (nextNode != nullptr && rangeLength > 0) {
                        for (int j = 0; j < nextNode->ranges.size(); ++j) {
                            const QPair<int, int> &otherRange = nextNode->ranges.at(j);

                            if (range.first < otherRange.second && range.second > otherRange.first) {
                                int start = qMax(range.first, otherRange.first);
                                int end = qMin(range.second, otherRange.second);
                                rangeLength -= end - start + 1;
                                if (rangeLength == 0)
                                    break;
                            }
                        }
                    }

                    if (rangeLength > 0) {
                        drawCurrent = true;
                        break;
                    }
                }
            } else {
                drawCurrent = true;
            }

            if (drawCurrent)
                parentNode->addGlyphs(node->position, node->glyphRun, color, style, styleColor, clipNode);
        }
    }

    for (int i = 0; i < imageNodes.size(); ++i) {
        const BinaryTreeNode *node = imageNodes.at(i);
        if (node->selectionState == Selected) {
            parentNode->addImage(node->boundingRect, node->image);
            if (node->selectionState == Selected) {
                QColor color = m_selectionColor;
                color.setAlpha(128);
                parentNode->addRectangleNode(node->boundingRect, color);
            }
        }
    }
}

void SlaqTextNodeEngine::mergeFormats(QTextLayout *textLayout, QVarLengthArray<QTextLayout::FormatRange> *mergedFormats)
{
    Q_ASSERT(mergedFormats != nullptr);
    if (textLayout == nullptr)
        return;

    QVector<QTextLayout::FormatRange> additionalFormats = textLayout->formats();
    for (int i=0; i<additionalFormats.size(); ++i) {
        QTextLayout::FormatRange additionalFormat = additionalFormats.at(i);
        if (additionalFormat.format.hasProperty(QTextFormat::ForegroundBrush)
         || additionalFormat.format.hasProperty(QTextFormat::BackgroundBrush)
         || additionalFormat.format.isAnchor()) {
            // Merge overlapping formats
            if (!mergedFormats->isEmpty()) {
                QTextLayout::FormatRange *lastFormat = mergedFormats->data() + mergedFormats->size() - 1;

                if (additionalFormat.start < lastFormat->start + lastFormat->length) {
                    QTextLayout::FormatRange *mergedRange = nullptr;

                    int length = additionalFormat.length;
                    if (additionalFormat.start > lastFormat->start) {
                        lastFormat->length = additionalFormat.start - lastFormat->start;
                        length -= lastFormat->length;

                        mergedFormats->append(QTextLayout::FormatRange());
                        mergedRange = mergedFormats->data() + mergedFormats->size() - 1;
                        lastFormat = mergedFormats->data() + mergedFormats->size() - 2;
                    } else {
                        mergedRange = lastFormat;
                    }

                    mergedRange->format = lastFormat->format;
                    mergedRange->format.merge(additionalFormat.format);
                    mergedRange->start = additionalFormat.start;

                    int end = qMin(additionalFormat.start + additionalFormat.length,
                                   lastFormat->start + lastFormat->length);

                    mergedRange->length = end - mergedRange->start;
                    length -= mergedRange->length;

                    additionalFormat.start = end;
                    additionalFormat.length = length;
                }
            }

            if (additionalFormat.length > 0)
                mergedFormats->append(additionalFormat);
        }
    }

}

void SlaqTextNodeEngine::addTextBlock(QTextDocument *textDocument, const QTextBlock &block, const QPointF &position, const QColor &textColor, const QColor &anchorColor, int selectionStart, int selectionEnd)
{
    Q_ASSERT(textDocument);
#if QT_CONFIG(im)
    int preeditLength = block.isValid() ? block.layout()->preeditAreaText().length() : 0;
    int preeditPosition = block.isValid() ? block.layout()->preeditAreaPosition() : -1;
#endif

    setCurrentTextDirection(block.textDirection());

    QVarLengthArray<QTextLayout::FormatRange> colorChanges;
    mergeFormats(block.layout(), &colorChanges);

    QPointF blockPosition = textDocument->documentLayout()->blockBoundingRect(block).topLeft() + position;
    if (QTextList *textList = block.textList()) {
        QPointF pos = blockPosition;
        QTextLayout *layout = block.layout();
        if (layout->lineCount() > 0) {
            QTextLine firstLine = layout->lineAt(0);
            Q_ASSERT(firstLine.isValid());

            setCurrentLine(firstLine);

            QRectF textRect = firstLine.naturalTextRect();
            pos += textRect.topLeft();
            if (block.textDirection() == Qt::RightToLeft)
                pos.rx() += textRect.width();

            const QTextCharFormat charFormat = block.charFormat();
            QFont font(charFormat.font());
            QFontMetricsF fontMetrics(font);
            QTextListFormat listFormat = textList->format();

            QString listItemBullet;
            switch (listFormat.style()) {
            case QTextListFormat::ListCircle:
                listItemBullet = QChar(0x25E6); // White bullet
                break;
            case QTextListFormat::ListSquare:
                listItemBullet = QChar(0x25AA); // Black small square
                break;
            case QTextListFormat::ListDecimal:
            case QTextListFormat::ListLowerAlpha:
            case QTextListFormat::ListUpperAlpha:
            case QTextListFormat::ListLowerRoman:
            case QTextListFormat::ListUpperRoman:
                listItemBullet = textList->itemText(block);
                break;
            default:
                listItemBullet = QChar(0x2022); // Black bullet
                break;
            };

            QSizeF size(fontMetrics.width(listItemBullet), fontMetrics.height());
            qreal xoff = fontMetrics.width(QLatin1Char(' '));
            if (block.textDirection() == Qt::LeftToRight)
                xoff = -xoff - size.width();
            setPosition(pos + QPointF(xoff, 0));

            QTextLayout layout;
            layout.setFont(font);
            layout.setText(listItemBullet); // Bullet
            layout.beginLayout();
            QTextLine line = layout.createLine();
            line.setPosition(QPointF(0, 0));
            layout.endLayout();

            QList<QGlyphRun> glyphRuns = layout.glyphRuns();
            for (int i=0; i<glyphRuns.size(); ++i)
                addUnselectedGlyphs(glyphRuns.at(i));
        }
    }

    int textPos = block.position();
    QTextBlock::iterator blockIterator = block.begin();

    while (!blockIterator.atEnd()) {
        QTextFragment fragment = blockIterator.fragment();
        QString text = fragment.text();
        if (text.isEmpty())
            continue;

        QTextCharFormat charFormat = fragment.charFormat();
        QFont font(charFormat.font());
        QFontMetricsF fontMetrics(font);

        int fontHeight = fontMetrics.descent() + fontMetrics.ascent();
        int valign = charFormat.verticalAlignment();
        if (valign == QTextCharFormat::AlignSuperScript)
            setPosition(QPointF(blockPosition.x(), blockPosition.y() - fontHeight / 2));
        else if (valign == QTextCharFormat::AlignSubScript)
            setPosition(QPointF(blockPosition.x(), blockPosition.y() + fontHeight / 6));
        else
            setPosition(blockPosition);

        if (text.contains(QChar::ObjectReplacementCharacter)) {
            QTextFrame *frame = qobject_cast<QTextFrame *>(textDocument->objectForFormat(charFormat));
            if (frame && frame->frameFormat().position() == QTextFrameFormat::InFlow) {
                int blockRelativePosition = textPos - block.position();
                QTextLine line = block.layout()->lineForTextPosition(blockRelativePosition);
                if (!currentLine().isValid()
                        || line.lineNumber() != currentLine().lineNumber()) {
                    setCurrentLine(line);
                }

                SlaqTextNodeEngine::SelectionState selectionState =
                        (selectionStart < textPos + text.length()
                         && selectionEnd >= textPos)
                        ? SlaqTextNodeEngine::Selected
                        : SlaqTextNodeEngine::Unselected;

                addTextObject(block, QPointF(), charFormat, selectionState, textDocument, textPos);
            }
            textPos += text.length();
        } else {
            if (charFormat.foreground().style() != Qt::NoBrush)
                setTextColor(charFormat.foreground().color());
            else if (charFormat.isAnchor())
                setTextColor(anchorColor);
            else
                setTextColor(textColor);

            int fragmentEnd = textPos + fragment.length();
#if QT_CONFIG(im)
            if (preeditPosition >= 0
                    && (preeditPosition + block.position()) >= textPos
                    && (preeditPosition + block.position()) <= fragmentEnd) {
                fragmentEnd += preeditLength;
            }
#endif
            if (charFormat.background().style() != Qt::NoBrush) {
                QTextLayout::FormatRange additionalFormat;
                additionalFormat.start = textPos - block.position();
                additionalFormat.length = fragmentEnd - textPos;
                additionalFormat.format = charFormat;
                colorChanges << additionalFormat;
            }

            textPos = addText(block, charFormat, textColor, colorChanges, textPos, fragmentEnd,
                                                 selectionStart, selectionEnd);
        }

        ++blockIterator;
    }

#if QT_CONFIG(im)
    if (preeditLength >= 0 && textPos <= block.position() + preeditPosition) {
        setPosition(blockPosition);
        textPos = block.position() + preeditPosition;
        QTextLine line = block.layout()->lineForTextPosition(preeditPosition);
        if (!currentLine().isValid()
                || line.lineNumber() != currentLine().lineNumber()) {
            setCurrentLine(line);
        }
        textPos = addText(block, block.charFormat(), textColor, colorChanges,
                                             textPos, textPos + preeditLength,
                                             selectionStart, selectionEnd);
    }
#endif

    setCurrentLine(QTextLine()); // Reset current line because the text layout changed
    m_hasContents = true;
}


QT_END_NAMESPACE

