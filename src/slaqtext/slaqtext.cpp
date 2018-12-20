#include "slaqtext.h"
#include "slaqtext_p.h"
#include <QTextBoundaryFinder>

#include "slaqtextnode_p.h"
#include "QtQuick/private/qquicktextdocument_p.h"

#include <QtQuick/qsgsimplerectnode.h>
#include <private/qv4scopedvalue_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p_p.h>

#include "imagescache.h"
#include "messageformatter.h"

qreal alignedX(const qreal textWidth, const qreal itemWidth, int alignment)
{
    qreal x = 0;
    switch (alignment) {
    case Qt::AlignLeft:
    case Qt::AlignJustify:
        break;
    case Qt::AlignRight:
        x = itemWidth - textWidth;
        break;
    case Qt::AlignHCenter:
        x = (itemWidth - textWidth) / 2;
        break;
    }
    return x;
}

qreal alignedY(const qreal textHeight, const qreal itemHeight, int alignment)
{
    qreal y = 0;
    switch (alignment) {
    case Qt::AlignTop:
        break;
    case Qt::AlignBottom:
        y = itemHeight - textHeight;
        break;
    case Qt::AlignVCenter:
        y = (itemHeight - textHeight) / 2;
        break;
    }
    return y;
}

SlackText::SlackText(QQuickItem* parent)
    : QQuickLabel(parent)
{
    d_ptr = new SlackTextPrivate;
    d_ptr->q_ptr = this;
    Q_D(SlackText);
    d->init();
}

void SlackText::insertImage(QTextCursor& cursor, const QString& url, const QImage& img) {
    Q_D(SlackText);
    d->m_tp->extra->doc->addResource(QTextDocument::ImageResource, QUrl(url), img);
    QTextImageFormat fmt;
    fmt.setWidth(d->m_emojiWidth);
    fmt.setHeight(d->m_emojiHeight);
    fmt.setName(url);
    fmt.setVerticalAlignment(QTextCharFormat::AlignMiddle);
    cursor.insertImage(fmt, QTextFrameFormat::InFlow);
}

void SlackText::onImageLoaded(const QString &id)
{
    //qDebug() << __PRETTY_FUNCTION__ << id;
    Q_D(SlackText);
    ImagesCache* imageCache = ImagesCache::instance();

    int pos = d->m_requestedImages.value(id, -1);
    if (pos >= 0 && d->m_tp->extra.isAllocated() && d->m_tp->extra->doc) {
        d->m_requestedImages.remove(id);
        if (d->m_requestedImages.size() == 0) {
            disconnect(imageCache, &ImagesCache::imageLoaded,
                              this, &SlackText::onImageLoaded);
        }

        QImage img = imageCache->image(id);
        if (img.isNull()) {
            qWarning() << "Still no image for ID:" << id;
            return;
        }
        const QString imgUrl = "image://emoji/"+id;
        bool isundo = d->m_tp->extra->doc->isUndoRedoEnabled();
        d->m_tp->extra->doc->setUndoRedoEnabled(false);
        QTextCursor cursor(d->m_tp->extra->doc);
        cursor.setPosition(pos);
        insertImage(cursor, imgUrl, img);
        d->m_tp->extra->doc->setUndoRedoEnabled(isundo);
        d->m_tp->extra->doc->markContentsDirty(0, d->m_tp->extra->doc->characterCount());
        d->m_lp->updateSize();
        d->m_lp->updateLayout();
    }
}

bool SlackText::markupUpdate(const QString& markupQuote, const QString &markupEndQuote,
                             std::function<void(QTextCursor& from, QString &selText)> markupReplace) {
    Q_D(SlackText);
    QTextCursor fromCursor = d->m_tp->extra->doc->find(markupQuote, 0);
    if (!fromCursor.isNull()) {
        QTextCursor toCursor = d->m_tp->extra->doc->find(markupEndQuote.isEmpty() ?
                                                             markupQuote :
                                                             markupEndQuote,
                                                         fromCursor);
        if (!toCursor.isNull()) {

            QTextCursor brCursor = d->m_tp->extra->doc->find(QChar(QChar::LineSeparator),
                                                             fromCursor);
            if (brCursor.isNull()) {
                brCursor = d->m_tp->extra->doc->find(QChar(QChar::ParagraphSeparator), fromCursor);
            }
            if (brCursor.isNull()) {
                brCursor = d->m_tp->extra->doc->find(QStringLiteral("<br/>"), fromCursor);
            }
            //check if is the same line
            if (brCursor.isNull() || brCursor.position() > toCursor.position()) {
                bool isundo = d->m_tp->extra->doc->isUndoRedoEnabled();
                d->m_tp->extra->doc->setUndoRedoEnabled(false);
                fromCursor.movePosition(QTextCursor::NextCharacter,
                                        QTextCursor::KeepAnchor,
                                        toCursor.position() - fromCursor.position());
                QString selectedText = fromCursor.selectedText();
                selectedText = selectedText.remove(markupQuote);
                markupReplace(fromCursor, selectedText);
                return true;
            }
        }
    }
    return false;
}

QString SlackText::preProcessText(const QString& txt) {
    QString _txt = txt;

    //qDebug().noquote().nospace() << "preprocess [" << txt << "]";
    MessageFormatter::replaceLinks(m_chat, _txt);
    QStringList _blocks = _txt.split("```");
    // there must be at least 3 blocks and the numbers of blocks must be odd
    if (_blocks.size() > 2 && _blocks.size() % 2 == 1) {
        _txt = _blocks.at(0);
        for (int i = 1; i < _blocks.size(); i++) {
            if (i % 2 != 0) {
                _txt += "```";
                _txt += _blocks[i].replace(" ", QChar(QChar::Nbsp)).replace("\n", "<br/>");
                _txt += "```";
            } else {
                _txt += _blocks[i].replace("\n", "<br/>");
            }
        }
    } else {
        _txt = _txt.replace("\n", "<br/>");
    }

    return _txt;
}

void SlackText::postProcessText() {
    Q_D(SlackText);

    bool _hasblockquote = false;
    bool _hassinglequote = false;

    if (d->m_dirty && d->m_tp->extra.isAllocated() && d->m_tp->extra->doc) {
        d->m_modified = false;
        const QPalette& palette = QGuiApplication::palette();
        bool singleQuote = false;
        QTextCursor prevCursor(d->m_tp->extra->doc);
        ImagesCache* imageCache = ImagesCache::instance();

        QString searchQuote = "```";

        while (!prevCursor.isNull() && !prevCursor.atEnd()) {
            prevCursor = d->m_tp->extra->doc->find(searchQuote, prevCursor);
            if (prevCursor.isNull()) {
                prevCursor = QTextCursor(d->m_tp->extra->doc);
                searchQuote = "`";
                singleQuote = true;
                prevCursor = d->m_tp->extra->doc->find(searchQuote, prevCursor);
            }

            if (!prevCursor.isNull()) {
                QTextCursor nextCursor = d->m_tp->extra->doc->find(searchQuote, prevCursor);
                if (nextCursor.isNull() && !singleQuote) {
                    qWarning() << "no next cursor found! Assume its will be end of the document";
                    nextCursor = d->m_tp->extra->doc->rootFrame()->lastCursorPosition();
                }
                d->m_modified = true;
                bool isundo = d->m_tp->extra->doc->isUndoRedoEnabled();
                d->m_tp->extra->doc->setUndoRedoEnabled(false);
                prevCursor.movePosition(QTextCursor::NextCharacter,
                                        QTextCursor::KeepAnchor,
                                        nextCursor.position() - prevCursor.position());
                QString selectedText = prevCursor.selectedText();
                //qDebug() << "found quote" << selectedText << singleQuote;
                selectedText = selectedText.remove(searchQuote);
                //prevCursor.removeSelectedText();

                if (singleQuote) {
                    QTextCharFormat chFmt = prevCursor.charFormat();
                    //make some extra space for better visibility
                    selectedText.prepend(" ");
                    selectedText += " ";
                    chFmt.setBackground(QBrush(palette.color(QPalette::AlternateBase)));
                    chFmt.setForeground(QBrush(palette.color(QPalette::HighlightedText)));
                    prevCursor.insertText(selectedText, chFmt);
                    _hassinglequote = true;
                } else {
                    QTextFrameFormat fmt;
                    fmt.setPosition(QTextFrameFormat::InFlow);
                    fmt.setBorderStyle(QTextFrameFormat::BorderStyle_Dashed);
                    fmt.setBorder(1);
                    fmt.setPadding(3);
                    fmt.setBackground(QBrush(palette.color(QPalette::AlternateBase)));
                    fmt.setForeground(QBrush(palette.color(QPalette::HighlightedText)));
                    QTextFrame *codeBlockFrame = prevCursor.insertFrame(fmt);
                    //QTextCursor blockCursor = codeBlockFrame->firstCursorPosition();
                    prevCursor.insertText(selectedText);
                    _hasblockquote = true;
                }
                prevCursor = nextCursor;
                //qDebug() << "frame" << d->m_tp->extra->doc->rootFrame()->frameFormat().width().type();//codeBlockFrame->firstPosition() << codeBlockFrame->lastPosition();
                d->m_tp->extra->doc->setUndoRedoEnabled(isundo);
            }
        }

        //search for emojis
        prevCursor = QTextCursor(d->m_tp->extra->doc);
        searchQuote = ":";
        while (!prevCursor.isNull() && !prevCursor.atEnd()) {
            prevCursor = d->m_tp->extra->doc->find(searchQuote, prevCursor);
            if (!prevCursor.isNull()) {
                QTextCursor nextCursor = d->m_tp->extra->doc->find(searchQuote, prevCursor);
                if (nextCursor.isNull()) {
                    //qWarning() << "no next cursor found! not an emoji";
                    break;
                }

                bool isundo = d->m_tp->extra->doc->isUndoRedoEnabled();
                d->m_tp->extra->doc->setUndoRedoEnabled(false);
                //nextCursor.beginEditBlock();
                prevCursor.movePosition(QTextCursor::NextCharacter,
                                        QTextCursor::KeepAnchor,
                                        nextCursor.position() - prevCursor.position());
                QString selectedText = prevCursor.selectedText();
                selectedText = selectedText.remove(searchQuote);
                EmojiInfo* einfo = imageCache->getEmojiInfo(selectedText);
                if (einfo != nullptr) {
                    d->m_modified = true;
                    prevCursor.removeSelectedText();
                    //qDebug() << "found emoji" << selectedText << einfo << einfo->image();
                    if (imageCache->isUnicode() && !(einfo->imagesExist() & EmojiInfo::ImageSlackTeam)) {
                        prevCursor.insertText(einfo->unified());
                    } else {
                        const QString imgUrl = "image://emoji/"+selectedText;
                        QImage img = imageCache->image(selectedText);
                        if (img.isNull()) {
                            connect(imageCache, &ImagesCache::imageLoaded,
                                              this, &SlackText::onImageLoaded, Qt::UniqueConnection);
                            d->m_requestedImages[selectedText] = prevCursor.position();
                            //qWarning() << "img for" << selectedText << "not ready";
                        } else {
                            insertImage(prevCursor, imgUrl, img);
                        }
                    }
                }
                //qDebug() << "frame" << d->m_tp->extra->doc->rootFrame()->frameFormat().width().type();//codeBlockFrame->firstPosition() << codeBlockFrame->lastPosition();
                //nextCursor.endEditBlock();
                d->m_tp->extra->doc->setUndoRedoEnabled(isundo);
                prevCursor = nextCursor;
            }
        }
        d->m_modified |= markupUpdate("*", "", [=] (QTextCursor& from, QString& selText) {
            QTextCharFormat chFmt = from.charFormat();
            QFont fnt = chFmt.font();
            fnt.setBold(true);
            chFmt.setFont(fnt);
            from.insertText(selText, chFmt);
        });
        d->m_modified |= markupUpdate("_", "", [=] (QTextCursor& from, QString& selText) {
            QTextCharFormat chFmt = from.charFormat();
            QFont fnt = chFmt.font();
            fnt.setItalic(true);
            chFmt.setFont(fnt);
            from.insertText(selText, chFmt);
        });
        d->m_modified |= markupUpdate("~", "", [=] (QTextCursor& from, QString& selText) {
            QTextCharFormat chFmt = from.charFormat();
            QFont fnt = chFmt.font();
            fnt.setStrikeOut(true);
            chFmt.setFont(fnt);
            from.insertText(selText, chFmt);
        });

        if (d->m_modified) {
            //qDebug() << "updating";
            if (_hassinglequote && _hasblockquote) {
                setLineHeight(lineHeight() * 1.2);
            }
            d->m_tp->extra->doc->markContentsDirty(0, d->m_tp->extra->doc->characterCount());
            d->m_lp->updateSize();
            d->m_lp->updateLayout();
            d->m_modified = false;
        }
        d->m_dirty = false;
    }
}

void SlackText::componentComplete()
{
    Q_D(SlackText);

    QQuickLabel::setRenderType(QtRendering);
    QQuickLabel::setTextFormat(RichText);
    QQuickLabel::setColor(palette().windowText().color());
    QQuickLabel::setLinkColor(palette().link().color());
    QQuickLabel::componentComplete();
    postProcessText();
    // TODO: code highlight?
}


QColor SlackText::selectionColor() const
{
    Q_D(const SlackText);
    return d->selectionColor;
}

void SlackText::setSelectionColor(const QColor &color)
{
    Q_D(SlackText);
    if (d->selectionColor == color)
        return;

    d->selectionColor = color;
    if (d->hasSelectedText()) {
        d->textLayoutDirty = true;
        d->m_lp->updateType = QQuickTextPrivate::UpdatePaintNode;
        polish();
        update();
    }
    emit selectionColorChanged();
}
/*!
    \qmlproperty color QtQuick::TextInput::selectedTextColor

    The highlighted text color, used in selections.
*/
QColor SlackText::selectedTextColor() const
{
    Q_D(const SlackText);
    return d->selectedTextColor;
}

void SlackText::setSelectedTextColor(const QColor &color)
{
    Q_D(SlackText);
    if (d->selectedTextColor == color)
        return;

    d->selectedTextColor = color;
    if (d->hasSelectedText()) {
        d->textLayoutDirty = true;
        d->m_lp->updateType = QQuickTextPrivate::UpdatePaintNode;
        polish();
        update();
    }
    emit selectedTextColorChanged();
}

int SlackText::selectionStart() const
{
    Q_D(const SlackText);
    return d->lastSelectionStart;
}

int SlackText::selectionEnd() const
{
    Q_D(const SlackText);
    return d->lastSelectionEnd;
}

void SlackText::select(int start, int end)
{
    Q_D(SlackText);
    if (start < 0 || end < 0 || start > text().length() || end > text().length())
        return;
    d->setSelection(start, end-start);
}

QString SlackText::selectedText() const
{
    Q_D(const SlackText);
    return d->selectedText();
}

void SlackTextPrivate::init()
{
    Q_Q(SlackText);

    m_lp = QQuickLabelPrivate::get(qobject_cast<QQuickLabel *>(q_ptr));
    m_tp = QQuickTextPrivate::get(qobject_cast<QQuickText *>(q_ptr));

#if QT_CONFIG(clipboard)
    if (QGuiApplication::clipboard()->supportsSelection())
        q->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton);
    else
#endif
        q->setAcceptedMouseButtons(Qt::LeftButton);

    const QPalette& palette = QGuiApplication::palette();
    selectionColor = palette.color(QPalette::Highlight);
    selectedTextColor = palette.color(QPalette::HighlightedText);

    lastSelectionStart = 0;
    lastSelectionEnd = 0;
}

int SlackTextPrivate::findInMask(int pos, bool forward, bool findSeparator, QChar searchChar) const
{
    if (pos >= m_maxLength || pos < 0)
        return -1;

    int end = forward ? m_maxLength : -1;
    int step = forward ? 1 : -1;
    int i = pos;

    while (i != end) {
        if (findSeparator) {
            if (m_maskData[i].separator && m_maskData[i].maskChar == searchChar)
                return i;
        } else {
            if (!m_maskData[i].separator) {
                return i;
            }
        }
        i += step;
    }
    return -1;
}

void SlackTextPrivate::moveSelectionCursor(int pos, bool mark)
{
    Q_Q(SlackText);

    if (pos != m_cursor) {
        separate();
        if (m_maskData)
            pos = pos > m_cursor ? nextMaskBlank(pos) : prevMaskBlank(pos);
    }
    if (mark) {
        int anchor;
        if (m_selend > m_selstart && m_cursor == m_selstart)
            anchor = m_selend;
        else if (m_selend > m_selstart && m_cursor == m_selend)
            anchor = m_selstart;
        else
            anchor = m_cursor;
        m_selstart = qMin(anchor, pos);
        m_selend = qMax(anchor, pos);
    } else {
        internalDeselect();
    }
    m_cursor = pos;
    if (mark || m_selDirty) {
        m_selDirty = false;
        emit q->selectionChanged();
    }
}

QRectF SlackText::positionToRectangle(int pos)
{
    Q_D(SlackText);
    QTextLine l = d->m_lp->layout.lineForTextPosition(pos);
    if (!l.isValid())
        return QRectF();
    qreal x = l.cursorToX(pos)/* - d->hscroll*/;
    qreal y = l.y()/* - d->vscroll*/;
    qreal w = 1;
    return QRectF(x, y, w, l.height());
}

void SlackText::positionAt(QQmlV4Function *args)
{
    Q_D(SlackText);

    qreal x = 0;
    qreal y = 0;
    QTextLine::CursorPosition position = QTextLine::CursorBetweenCharacters;

    if (args->length() < 1)
        return;

    int i = 0;
    QV4::Scope scope(args->v4engine());
    QV4::ScopedValue arg(scope, (*args)[0]);
    x = arg->toNumber();

    if (++i < args->length()) {
        arg = (*args)[i];
        y = arg->toNumber();
    }

    if (++i < args->length()) {
        arg = (*args)[i];
        position = QTextLine::CursorPosition(arg->toInt32());
    }

    int pos = d->positionAt(x, y, position);
    const int cursor = d->m_cursor;
    if (pos > cursor) {
#if QT_CONFIG(im)
        const int preeditLength = 0;
        pos = pos > cursor + preeditLength
                ? pos - preeditLength
                : cursor;
#else
        pos = cursor;
#endif
    }
    args->setReturnValue(QV4::Encode(pos));
}

int SlackTextPrivate::positionAt(qreal x, qreal y, QTextLine::CursorPosition position)
{
    Q_Q(SlackText);
    int pos = 0;

    if (m_tp->richText && m_tp->extra.isAllocated() && m_tp->extra->doc) {
        //qDebug() << __PRETTY_FUNCTION__ << y << m_tp->layedOutTextRect.height() << m_tp->availableHeight() << m_tp->extra->topPadding;
        QPointF translatedMousePos = QPointF(x, y);
        translatedMousePos.rx() -= alignedX(m_tp->layedOutTextRect.width(), m_tp->availableWidth(), q->effectiveHAlign());
        translatedMousePos.ry() -= m_tp->extra->topPadding;
        //qDebug() << "corrected mouse pos" << translatedMousePos;
        pos = m_tp->extra->doc->documentLayout()->hitTest(translatedMousePos, Qt::FuzzyHit);
    }
    return pos;

}

void SlackText::invalidateFontCaches()
{
    QQuickText::invalidateFontCaches();
}

void SlackText::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(SlackText);

    if (d->selectByMouse && event->button() == Qt::LeftButton) {
        //#if QT_CONFIG(im)
        //        d->commitPreedit();
        //#endif
        int cursor = d->positionAt(event->localPos());
        qDebug() << __PRETTY_FUNCTION__ << cursor << event->localPos() << event->pos() << event->windowPos();
        d->selectWordAtPos(cursor);
        event->setAccepted(true);
        if (!d->hasPendingTripleClick()) {
            d->tripleClickStartPoint = event->localPos();
            d->tripleClickTimer.start();
        }
//        d->m_tp->extra->doc->markContentsDirty(0, d->m_tp->extra->doc->characterCount());
//        d->m_lp->updateSize();
//        d->m_lp->updateLayout();
    } else {
        //        if (d->sendMouseEventToInputContext(event))
        //            return;
        QQuickLabel::mouseDoubleClickEvent(event);
    }
}


/*!
    \internal

    Sets the selection to cover the word at the given cursor position.
    The word boundaries are defined by the behavior of QTextLayout::SkipWords
    cursor mode.
*/
void SlackTextPrivate::selectWordAtPos(int cursor)
{
    Q_Q(SlackText);
    int next = cursor + 1;
    if (next > end())
        --next;

    int c = 0;
    int end = 0;
    if (m_lp->richText) {
        QTextDocumentPrivate* td_p = m_tp->extra->doc->docHandle();
        c = td_p->previousCursorPosition(next, QTextLayout::SkipWords);
        end = td_p->nextCursorPosition(c, QTextLayout::SkipWords);
        qDebug() << cursor << next << c << end <<m_tp->extra->doc->toPlainText();
    } else {
        c = m_lp->layout.previousCursorPosition(next, QTextLayout::SkipWords);
        end = m_lp->layout.nextCursorPosition(c, QTextLayout::SkipWords);
    }
    moveSelectionCursor(c, false);
    // ## text layout should support end of words.
    while (end > cursor && m_tp->extra->doc->toPlainText().at(end-1).isSpace())
        --end;
    moveSelectionCursor(end, true);
}


#if QT_CONFIG(clipboard)
/*!
    \internal

    Copies the currently selected text into the clipboard using the given
    \a mode.

    \note If the echo mode is set to a mode other than Normal then copy
    will not work.  This is to prevent using copy as a method of bypassing
    password features of the line control.
*/
void SlackTextPrivate::copy(QClipboard::Mode mode) const
{
    QString t = selectedText();
    if (!t.isEmpty()) {
        qDebug() << "copy to clipboard:" << t << mode;
        QGuiApplication::clipboard()->setText(t, mode);
    }
}

#endif // clipboard

/*!
    \qmlmethod QtQuick::TextInput::copy()

    Copies the currently selected text to the system clipboard.

    \note If the echo mode is set to a mode other than Normal then copy
    will not work.  This is to prevent using copy as a method of bypassing
    password features of the line control.
*/
void SlackText::copy()
{
    Q_D(SlackText);
    d->copy();
}

void SlackText::mousePressEvent(QMouseEvent *event)
{
    Q_D(SlackText);

    d->pressPos = event->localPos();

    if (d->selectByMouse) {
        setKeepMouseGrab(false);
        d->selectPressed = true;
        QPointF distanceVector = d->pressPos - d->tripleClickStartPoint;
        if (d->hasPendingTripleClick()
                && distanceVector.manhattanLength() < QGuiApplication::styleHints()->startDragDistance()) {
            event->setAccepted(true);
            selectAll();
            return;
        }
    }

    bool mark = (event->modifiers() & Qt::ShiftModifier) && d->selectByMouse;
    int cursor = d->positionAt(event->localPos());
    d->moveSelectionCursor(cursor, mark);

    //    if (d->focusOnPress && !qGuiApp->styleHints()->setFocusOnTouchRelease())
    //        ensureActiveFocus();

    QQuickLabel::mousePressEvent(event);
    event->setAccepted(true);
}

void SlackText::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(SlackText);

    if (d->selectPressed) {
        if (qAbs(int(event->localPos().x() - d->pressPos.x())) > QGuiApplication::styleHints()->startDragDistance())
            setKeepMouseGrab(true);
        moveCursorSelection(d->positionAt(event->localPos()), d->mouseSelectionMode);
        event->setAccepted(true);
    } else {
        QQuickLabel::mouseMoveEvent(event);
    }
}

void SlackText::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(SlackText);
    //    if (d->sendMouseEventToInputContext(event))
    //        return;
    if (keepMouseGrab() == false && d->hasSelectedText() == false) {
        QString link = d->m_tp->anchorAt(event->localPos());
        if (!link.isEmpty()) {
            qDebug() << "activating link:" << link << text() << d->m_selstart << d->m_selend;
            emit linkActivated(d->m_tp->extra->activeLink);
            event->setAccepted(true);
            return;
        }
    } else if (d->selectPressed) {
        d->selectPressed = false;
        setKeepMouseGrab(false);
        if (d->hasSelectedText() == true) {
            forceActiveFocus();
        }
    }
#if QT_CONFIG(clipboard)
    if (QGuiApplication::clipboard()->supportsSelection()) {
        if (event->button() == Qt::LeftButton) {
            d->copy(QClipboard::Selection);
        } else if (event->button() == Qt::MidButton) {
            d->deselect();
        }
    }
#endif

    if (!event->isAccepted())
        QQuickLabel::mouseReleaseEvent(event);
}

void SlackText::mouseUngrabEvent()
{
    Q_D(SlackText);
    d->selectPressed = false;
    setKeepMouseGrab(false);
}

void SlackText::hoverEnterEvent(QHoverEvent *event)
{
    //qDebug() << __PRETTY_FUNCTION__ << event->pos();
    QQuickLabel::hoverEnterEvent(event);
}

void SlackText::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(SlackText);
    //qDebug() << __PRETTY_FUNCTION__ << event->pos();
    //
    QString link;
    QString imglink;
    QPointF translatedMousePos = event->posF();
    translatedMousePos.rx() -= leftPadding();
    translatedMousePos.ry() -= topPadding() + alignedY(d->m_tp->layedOutTextRect.height() + d->m_tp->lineHeightOffset(),
                                                       d->m_tp->availableHeight(), d->m_tp->vAlign);
    if (d->m_tp->richText && d->m_tp->extra.isAllocated() && d->m_tp->extra->doc) {
        translatedMousePos.rx() -= alignedX(d->m_tp->layedOutTextRect.width(), d->m_tp->availableWidth(),
                                            effectiveHAlign());
        link = d->m_tp->extra->doc->documentLayout()->anchorAt(translatedMousePos);
        imglink = d->m_tp->extra->doc->documentLayout()->imageAt(translatedMousePos);
    }
    if (link != d->m_linkHovered) {
        //qDebug() << link << d->m_linkHovered;
        d->m_linkHovered = link;
        emit linkHovered(link, translatedMousePos.x(), translatedMousePos.y());
        if (link.isEmpty()) {
            qApp->restoreOverrideCursor();
        } else {
            qApp->setOverrideCursor(QCursor(Qt::PointingHandCursor));
        }
    }
    if (imglink != d->m_imageHovered) {
        d->m_imageHovered = imglink;
        emit imageHovered(imglink, translatedMousePos.x(), translatedMousePos.y());
    }
    QQuickLabel::hoverMoveEvent(event);
}

void SlackText::hoverLeaveEvent(QHoverEvent *event)
{
    //qDebug() << __PRETTY_FUNCTION__ << event->pos();
    Q_D(SlackText);
    QQuickLabel::hoverLeaveEvent(event);
    d->m_linkHovered = "";
    emit linkHovered(d->m_linkHovered, 0, 0);
    d->m_imageHovered = "";
    emit imageHovered(d->m_imageHovered, 0, 0);
    qApp->restoreOverrideCursor();
}

/**
 * @brief SlackText::updatePaintNode. Direct copy of QQuickText one with select modifications. Keep in sync
 * @param oldNode
 * @param data
 * @return
 */

QSGNode *SlackText::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(SlackText);

    if (d->m_lp->text.isEmpty()) {
        delete oldNode;
        return nullptr;
    }

    if (d->m_lp->updateType != QQuickTextPrivate::UpdatePaintNode && oldNode != nullptr) {
        // Update done in preprocess() in the nodes
        d->m_lp->updateType = QQuickTextPrivate::UpdateNone;
        return oldNode;
    }

    d->m_tp->updateType = QQuickTextPrivate::UpdateNone;

    const qreal dy = alignedY(d->m_lp->layedOutTextRect.height() + d->m_lp->lineHeightOffset(), d->m_lp->availableHeight(), d->m_lp->vAlign) + topPadding();

    //qDebug() << dy << topPadding() << d->m_lp->vAlign;

    SlaqTextNode *node = nullptr;
    if (!oldNode)
        node = new SlaqTextNode(this);
    else
        node = static_cast<SlaqTextNode *>(oldNode);

    node->setUseNativeRenderer(d->m_lp->renderType == NativeRendering);
    node->deleteContent();
    node->setMatrix(QMatrix4x4());

    const QColor color = QColor::fromRgba(d->m_lp->color);
    const QColor styleColor = QColor::fromRgba(d->m_lp->styleColor);
    const QColor linkColor = QColor::fromRgba(d->m_lp->linkColor);

    if (d->m_lp->richText) {
        const qreal dx = alignedX(d->m_lp->layedOutTextRect.width(), d->m_lp->availableWidth(), effectiveHAlign()) + leftPadding();
        d->m_lp->ensureDoc();
        //qDebug() << QPointF(dx, dy) << d->m_lp->style << styleColor << color << d->selectionStart() << d->selectionEnd() << d->selectionColor << d->selectedTextColor;
        node->addTextDocument(QPointF(dx, dy), d->m_tp->extra->doc, color,
                              d->m_lp->style, styleColor, linkColor,
                              d->selectionColor, d->selectedTextColor,
                              d->selectionStart(), d->selectionEnd() - 1);
    }
    // The font caches have now been initialized on the render thread, so they have to be
    // invalidated before we can use them from the main thread again.
    invalidateFontCaches();

    return node;

}

void SlackText::deselect()
{
    Q_D(SlackText);
    d->deselect();
}

void SlackText::selectAll()
{
    Q_D(SlackText);
    d->setSelection(0, text().length());
}

void SlackText::selectWord()
{
    Q_D(SlackText);
    d->selectWordAtPos(d->m_cursor);
}

bool SlackText::selectByMouse() const
{
    Q_D(const SlackText);
    return d->selectByMouse;
}

void SlackText::setSelectByMouse(bool on)
{
    Q_D(SlackText);
    if (d->selectByMouse != on) {
        d->selectByMouse = on;
        emit selectByMouseChanged(on);
    }
}

SlackText::SelectionMode SlackText::mouseSelectionMode() const
{
    Q_D(const SlackText);
    return d->mouseSelectionMode;
}

void SlackText::setMouseSelectionMode(SelectionMode mode)
{
    Q_D(SlackText);
    if (d->mouseSelectionMode != mode) {
        d->mouseSelectionMode = mode;
        emit mouseSelectionModeChanged(mode);
    }
}

bool SlackText::persistentSelection() const
{
    Q_D(const SlackText);
    return d->persistentSelection;
}

void SlackText::setTextFormat(TextFormat format)
{
    QQuickLabel::setTextFormat(format);
}

QQuickText::TextFormat SlackText::textFormat() const
{
    return QQuickLabel::textFormat();
}

QString SlackText::text() const
{
    return QQuickLabel::text();
}

void SlackText::setText(const QString &txt)
{
    Q_D(SlackText);
    //preprocess text to replace CRs and spaces for frames, otherwize wordwarp not working
    QString _txt = preProcessText(txt);
    QQuickLabel::setText(_txt.isEmpty() ? txt : _txt);
    d->m_dirty = true;
    postProcessText();
}

QString SlackText::hoveredLink() const
{
    Q_D(const SlackText);
    return d->m_linkHovered;
}

QString SlackText::hoveredImage() const
{
    Q_D(const SlackText);
    return d->m_imageHovered;
}

ChatsModel *SlackText::chat() const
{
    return m_chat;
}

QQuickItem *SlackText::itemFocusOnUnselect() const
{
    Q_D(const SlackText);
    return d->m_itemFocusOnUnselect;
}

qreal SlackText::emojiWidth() const
{
    Q_D(const SlackText);
    return d->m_emojiWidth;
}

qreal SlackText::emojiHeight() const
{
    Q_D(const SlackText);
    return d->m_emojiHeight;
}

void SlackText::setPersistentSelection(bool on)
{
    Q_D(SlackText);
    if (d->persistentSelection == on)
        return;
    d->persistentSelection = on;
    emit persistentSelectionChanged();
}

void SlackText::setChat(ChatsModel *chat)
{
    if (m_chat == chat)
        return;

    m_chat = chat;
    emit chatChanged(m_chat);
}

void SlackText::setItemFocusOnUnselect(QQuickItem *itemFocusOnUnselect)
{
    Q_D(SlackText);
    if (d->m_itemFocusOnUnselect == itemFocusOnUnselect)
        return;

    d->m_itemFocusOnUnselect = itemFocusOnUnselect;
    emit itemFocusOnUnselectChanged(d->m_itemFocusOnUnselect);
}

void SlackText::setEmojiWidth(qreal emojiWidth)
{
    Q_D(SlackText);
    if (qFuzzyCompare(d->m_emojiWidth, emojiWidth))
        return;

    d->m_emojiWidth = emojiWidth;
    emit emojiWidthChanged(d->m_emojiWidth);
}

void SlackText::setEmojiHeight(qreal emojiHeight)
{
    Q_D(SlackText);
    if (qFuzzyCompare(d->m_emojiHeight, emojiHeight))
        return;

    d->m_emojiHeight = emojiHeight;
    emit emojiHeightChanged(d->m_emojiHeight);
}

void SlackText::moveCursorSelection(int position)
{
    Q_D(SlackText);
    d->moveSelectionCursor(position, true);
}

void SlackText::moveCursorSelection(int pos, SelectionMode mode)
{
    Q_D(SlackText);

    if (d->m_tp->extra->doc == nullptr) {
        return;
    }
    const QString text = d->m_tp->extra->doc->toPlainText();
    //qDebug() << pos << mode << d->m_cursor;
    if (mode == SelectCharacters) {
        d->moveSelectionCursor(pos, true);
    } else if (pos != d->m_cursor){
        const int cursor = d->m_cursor;
        int anchor;
        if (!d->hasSelectedText())
            anchor = d->m_cursor;
        else if (d->selectionStart() == d->m_cursor)
            anchor = d->selectionEnd();
        else
            anchor = d->selectionStart();

        if (anchor < pos || (anchor == pos && cursor < pos)) {

            QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text);
            finder.setPosition(anchor);

            const QTextBoundaryFinder::BoundaryReasons reasons = finder.boundaryReasons();
            if (anchor < text.length() && (reasons == QTextBoundaryFinder::NotAtBoundary
                                           || (reasons & QTextBoundaryFinder::EndOfItem))) {
                finder.toPreviousBoundary();
            }
            anchor = finder.position() != -1 ? finder.position() : 0;

            finder.setPosition(pos);
            if (pos > 0 && !finder.boundaryReasons())
                finder.toNextBoundary();
            const int cursor = finder.position() != -1 ? finder.position() : text.length();

            d->setSelection(anchor, cursor - anchor);
        } else if (anchor > pos || (anchor == pos && cursor > pos)) {
            QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text);
            finder.setPosition(anchor);

            const QTextBoundaryFinder::BoundaryReasons reasons = finder.boundaryReasons();
            if (anchor > 0 && (reasons == QTextBoundaryFinder::NotAtBoundary
                               || (reasons & QTextBoundaryFinder::StartOfItem))) {
                finder.toNextBoundary();
            }
            anchor = finder.position() != -1 ? finder.position() : text.length();

            finder.setPosition(pos);
            if (pos < text.length() && !finder.boundaryReasons())
                finder.toPreviousBoundary();
            const int cursor = finder.position() != -1 ? finder.position() : 0;

            d->setSelection(anchor, cursor - anchor);
        }
    }
}

void SlackText::selectionChanged()
{
    Q_D(SlackText);
    d->textLayoutDirty = true; //TODO: Only update rect in selection
    d->m_lp->updateType = QQuickTextPrivate::UpdatePaintNode;
    polish();
    update();
    emit selectedTextChanged();

    if (d->lastSelectionStart != d->selectionStart()) {
        d->lastSelectionStart = d->selectionStart();
        if (d->lastSelectionStart == -1)
            d->lastSelectionStart = d->m_cursor;
        emit selectionStartChanged();
    }
    if (d->lastSelectionEnd != d->selectionEnd()) {
        d->lastSelectionEnd = d->selectionEnd();
        if (d->lastSelectionEnd == -1)
            d->lastSelectionEnd = d->m_cursor;
        emit selectionEndChanged();
    }
    if (d->hasSelectedText() == false && d->m_itemFocusOnUnselect != nullptr) {
        d->m_itemFocusOnUnselect->forceActiveFocus();
    }
}

void SlackTextPrivate::setSelection(int start, int length)
{
    Q_Q(SlackText);

    if (start < 0 || start > m_tp->extra->doc->toPlainText().length()) {
        qWarning("SlackTextPrivate::setSelection: Invalid start position");
        return;
    }

    if (length > 0) {
        if (start == m_selstart && start + length == m_selend && m_cursor == m_selend)
            return;
        m_selstart = start;
        m_selend = qMin(start + length, m_tp->extra->doc->toPlainText().length());
        m_cursor = m_selend;
    } else if (length < 0){
        if (start == m_selend && start + length == m_selstart && m_cursor == m_selstart)
            return;
        m_selstart = qMax(start + length, 0);
        m_selend = start;
        m_cursor = m_selstart;
    } else if (m_selstart != m_selend) {
        m_selstart = 0;
        m_selend = 0;
        m_cursor = start;
    } else {
        m_cursor = start;
        return;
    }
    emit q->selectionChanged();
}

bool SlackTextPrivate::finishChange(bool update)
{
    Q_Q(SlackText);

    Q_UNUSED(update)

    bool alignmentChanged = false;
    bool textChanged = false;

    if (m_selDirty) {
        m_selDirty = false;
        emit q->selectionChanged();
    }

    return true;
}

void SlackTextPrivate::updateLayout()
{
    Q_Q(SlackText);

    if (!q->isComponentComplete())
        return;

    m_lp->updateLayout();
}

void SlackText::keyPressEvent(QKeyEvent* ev)
{
    Q_D(SlackText);
    // Don't allow MacOSX up/down support, and we don't allow a completer.
    bool ignore = (ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down) && ev->modifiers() == Qt::NoModifier;
    if (ignore) {
        ev->ignore();
    } else {
        d->processKeyEvent(ev);
    }
    if (!ev->isAccepted())
        QQuickLabel::keyPressEvent(ev);
}

void SlackTextPrivate::processKeyEvent(QKeyEvent* event)
{
    Q_Q(SlackText);

    if (event == QKeySequence::Copy) {
        copy();
        event->accept();
    } else {
        event->ignore();
    }
}
