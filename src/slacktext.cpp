#include "slacktext.h"
#include "slacktext_p.h"

SlackText::SlackText(QQuickItem *parent) : QQuickText(parent)
{
}

SlackText::~SlackText()
{
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
        d->updateType = SlackTextPrivate::UpdatePaintNode;
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
        d->updateType = SlackTextPrivate::UpdatePaintNode;
        polish();
        update();
    }
    emit selectedTextColorChanged();
}

/*!
    \qmlproperty int QtQuick::TextInput::selectionStart

    The cursor position before the first character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionEnd, cursorPosition, selectedText
*/
int SlackText::selectionStart() const
{
    Q_D(const SlackText);
    return d->lastSelectionStart;
}
/*!
    \qmlproperty int QtQuick::TextInput::selectionEnd

    The cursor position after the last character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionStart, cursorPosition, selectedText
*/
int SlackText::selectionEnd() const
{
    Q_D(const SlackText);
    return d->lastSelectionEnd;
}
/*!
    \qmlmethod QtQuick::TextInput::select(int start, int end)

    Causes the text from \a start to \a end to be selected.

    If either start or end is out of range, the selection is not changed.

    After calling this, selectionStart will become the lesser
    and selectionEnd will become the greater (regardless of the order passed
    to this method).

    \sa selectionStart, selectionEnd
*/
void SlackText::select(int start, int end)
{
    Q_D(SlackText);
    if (start < 0 || end < 0 || start > d->text.length() || end > d->text.length())
        return;
    d->setSelection(start, end-start);
}

/*!
    \qmlproperty string QtQuick::TextInput::selectedText

    This read-only property provides the text currently selected in the
    text input.

    It is equivalent to the following snippet, but is faster and easier
    to use.

    \js
    myTextInput.text.toString().substring(myTextInput.selectionStart,
        myTextInput.selectionEnd);
    \endjs
*/
QString SlackText::selectedText() const
{
    Q_D(const SlackText);
    return d->selectedText();
}


/*!
    \internal

    Moves the cursor to the given position \a pos.   If \a mark is true will
    adjust the currently selected text.
*/
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
    emitCursorPositionChanged();

}

void QQuickTextInput::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickTextInput);

    d->pressPos = event->localPos();

    if (d->sendMouseEventToInputContext(event))
        return;

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
    d->moveCursor(cursor, mark);

    if (d->focusOnPress && !qGuiApp->styleHints()->setFocusOnTouchRelease())
        ensureActiveFocus();

    event->setAccepted(true);
}

void QQuickTextInput::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickTextInput);

    if (d->selectPressed) {
        if (qAbs(int(event->localPos().x() - d->pressPos.x())) > QGuiApplication::styleHints()->startDragDistance())
            setKeepMouseGrab(true);

#if QT_CONFIG(im)
        if (d->composeMode()) {
            // start selection
            int startPos = d->positionAt(d->pressPos);
            int currentPos = d->positionAt(event->localPos());
            if (startPos != currentPos)
                d->setSelection(startPos, currentPos - startPos);
        } else
#endif
        {
            moveCursorSelection(d->positionAt(event->localPos()), d->mouseSelectionMode);
        }
        event->setAccepted(true);
    } else {
        QQuickImplicitSizeItem::mouseMoveEvent(event);
    }
}

void QQuickTextInput::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickTextInput);
    if (d->sendMouseEventToInputContext(event))
        return;
    if (d->selectPressed) {
        d->selectPressed = false;
        setKeepMouseGrab(false);
    }
#if QT_CONFIG(clipboard)
    if (QGuiApplication::clipboard()->supportsSelection()) {
        if (event->button() == Qt::LeftButton) {
            d->copy(QClipboard::Selection);
        } else if (!d->m_readOnly && event->button() == Qt::MidButton) {
            d->deselect();
            d->insert(QGuiApplication::clipboard()->text(QClipboard::Selection));
        }
    }
#endif

    if (d->focusOnPress && qGuiApp->styleHints()->setFocusOnTouchRelease())
        ensureActiveFocus();

    if (!event->isAccepted())
        QQuickImplicitSizeItem::mouseReleaseEvent(event);
}

void QQuickTextInput::mouseUngrabEvent()
{
    Q_D(QQuickTextInput);
    d->selectPressed = false;
    setKeepMouseGrab(false);
}

QSGNode *QQuickTextInput::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QQuickTextInput);

    if (d->updateType != QQuickTextInputPrivate::UpdatePaintNode && oldNode != nullptr) {
        // Update done in preprocess() in the nodes
        d->updateType = QQuickTextInputPrivate::UpdateNone;
        return oldNode;
    }

    d->updateType = QQuickTextInputPrivate::UpdateNone;

    QQuickTextNode *node = static_cast<QQuickTextNode *>(oldNode);
    if (node == nullptr)
        node = new QQuickTextNode(this);
    d->textNode = node;

    const bool showCursor = !isReadOnly() && d->cursorItem == nullptr && d->cursorVisible && d->m_blinkStatus;

    if (!d->textLayoutDirty && oldNode != nullptr) {
        if (showCursor)
            node->setCursor(cursorRectangle(), d->color);
        else
            node->clearCursor();
    } else {
        node->setUseNativeRenderer(d->renderType == NativeRendering);
        node->deleteContent();
        node->setMatrix(QMatrix4x4());

        QPointF offset(leftPadding(), topPadding());
        if (d->autoScroll && d->m_textLayout.lineCount() > 0) {
            QFontMetricsF fm(d->font);
            // the y offset is there to keep the baseline constant in case we have script changes in the text.
            offset += -QPointF(d->hscroll, d->vscroll + d->m_textLayout.lineAt(0).ascent() - fm.ascent());
        } else {
            offset += -QPointF(d->hscroll, d->vscroll);
        }

        if (!d->m_textLayout.text().isEmpty()
#if QT_CONFIG(im)
                || !d->m_textLayout.preeditAreaText().isEmpty()
#endif
                ) {
            node->addTextLayout(offset, &d->m_textLayout, d->color,
                                QQuickText::Normal, QColor(), QColor(),
                                d->selectionColor, d->selectedTextColor,
                                d->selectionStart(),
                                d->selectionEnd() - 1); // selectionEnd() returns first char after
                                                                 // selection
        }

        if (showCursor)
                node->setCursor(cursorRectangle(), d->color);

        d->textLayoutDirty = false;
    }

    invalidateFontCaches();

    return node;
}

/*!
    \qmlmethod QtQuick::TextInput::deselect()

    Removes active text selection.
*/
void QQuickTextInput::deselect()
{
    Q_D(QQuickTextInput);
    d->deselect();
}

/*!
    \qmlmethod QtQuick::TextInput::selectAll()

    Causes all text to be selected.
*/
void QQuickTextInput::selectAll()
{
    Q_D(QQuickTextInput);
    d->setSelection(0, text().length());
}

/*!
    \qmlmethod QtQuick::TextInput::selectWord()

    Causes the word closest to the current cursor position to be selected.
*/
void QQuickTextInput::selectWord()
{
    Q_D(QQuickTextInput);
    d->selectWordAtPos(d->m_cursor);
}

/*!
    \qmlproperty bool QtQuick::TextInput::selectByMouse

    Defaults to false.

    If true, the user can use the mouse to select text in some
    platform-specific way. Note that for some platforms this may
    not be an appropriate interaction (it may conflict with how
    the text needs to behave inside a \l Flickable, for example).
*/
bool QQuickTextInput::selectByMouse() const
{
    Q_D(const QQuickTextInput);
    return d->selectByMouse;
}

void QQuickTextInput::setSelectByMouse(bool on)
{
    Q_D(QQuickTextInput);
    if (d->selectByMouse != on) {
        d->selectByMouse = on;
        emit selectByMouseChanged(on);
    }
}

/*!
    \qmlproperty enumeration QtQuick::TextInput::mouseSelectionMode

    Specifies how text should be selected using a mouse.

    \list
    \li TextInput.SelectCharacters - The selection is updated with individual characters. (Default)
    \li TextInput.SelectWords - The selection is updated with whole words.
    \endlist

    This property only applies when \l selectByMouse is true.
*/

QQuickTextInput::SelectionMode QQuickTextInput::mouseSelectionMode() const
{
    Q_D(const QQuickTextInput);
    return d->mouseSelectionMode;
}

void QQuickTextInput::setMouseSelectionMode(SelectionMode mode)
{
    Q_D(QQuickTextInput);
    if (d->mouseSelectionMode != mode) {
        d->mouseSelectionMode = mode;
        emit mouseSelectionModeChanged(mode);
    }
}

/*!
    \qmlproperty bool QtQuick::TextInput::persistentSelection

    Whether the TextInput should keep its selection when it loses active focus to another
    item in the scene. By default this is set to false;
*/

bool QQuickTextInput::persistentSelection() const
{
    Q_D(const QQuickTextInput);
    return d->persistentSelection;
}

void QQuickTextInput::setPersistentSelection(bool on)
{
    Q_D(QQuickTextInput);
    if (d->persistentSelection == on)
        return;
    d->persistentSelection = on;
    emit persistentSelectionChanged();
}

void QQuickTextInput::moveCursorSelection(int position)
{
    Q_D(QQuickTextInput);
    d->moveCursor(position, true);
}

/*!
    \qmlmethod QtQuick::TextInput::moveCursorSelection(int position, SelectionMode mode = TextInput.SelectCharacters)

    Moves the cursor to \a position and updates the selection according to the optional \a mode
    parameter.  (To only move the cursor, set the \l cursorPosition property.)

    When this method is called it additionally sets either the
    selectionStart or the selectionEnd (whichever was at the previous cursor position)
    to the specified position. This allows you to easily extend and contract the selected
    text range.

    The selection mode specifies whether the selection is updated on a per character or a per word
    basis.  If not specified the selection mode will default to TextInput.SelectCharacters.

    \list
    \li TextInput.SelectCharacters - Sets either the selectionStart or selectionEnd (whichever was at
    the previous cursor position) to the specified position.
    \li TextInput.SelectWords - Sets the selectionStart and selectionEnd to include all
    words between the specified position and the previous cursor position.  Words partially in the
    range are included.
    \endlist

    For example, take this sequence of calls:

    \code
        cursorPosition = 5
        moveCursorSelection(9, TextInput.SelectCharacters)
        moveCursorSelection(7, TextInput.SelectCharacters)
    \endcode

    This moves the cursor to position 5, extend the selection end from 5 to 9
    and then retract the selection end from 9 to 7, leaving the text from position 5 to 7
    selected (the 6th and 7th characters).

    The same sequence with TextInput.SelectWords will extend the selection start to a word boundary
    before or on position 5 and extend the selection end to a word boundary on or past position 9.
*/
void QQuickTextInput::moveCursorSelection(int pos, SelectionMode mode)
{
    Q_D(QQuickTextInput);

    if (mode == SelectCharacters) {
        d->moveCursor(pos, true);
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
            const QString text = this->text();
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
            const QString text = this->text();
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

void QQuickTextInput::selectionChanged()
{
    Q_D(QQuickTextInput);
    d->textLayoutDirty = true; //TODO: Only update rect in selection
    d->updateType = QQuickTextInputPrivate::UpdatePaintNode;
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
}

/*!
    \internal

    Sets \a length characters from the given \a start position as selected.
    The given \a start position must be within the current text for
    the line control.  If \a length characters cannot be selected, then
    the selection will extend to the end of the current text.
*/
void QQuickTextInputPrivate::setSelection(int start, int length)
{
    Q_Q(QQuickTextInput);
#if QT_CONFIG(im)
    commitPreedit();
#endif

    if (start < 0 || start > m_text.length()) {
        qWarning("QQuickTextInputPrivate::setSelection: Invalid start position");
        return;
    }

    if (length > 0) {
        if (start == m_selstart && start + length == m_selend && m_cursor == m_selend)
            return;
        m_selstart = start;
        m_selend = qMin(start + length, m_text.length());
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
        emitCursorPositionChanged();
        return;
    }
    emit q->selectionChanged();
    emitCursorPositionChanged();
#if QT_CONFIG(im)
    q->updateInputMethod(Qt::ImCursorRectangle | Qt::ImAnchorRectangle | Qt::ImCursorPosition | Qt::ImAnchorPosition
                       | Qt::ImCurrentSelection);
#endif
}
