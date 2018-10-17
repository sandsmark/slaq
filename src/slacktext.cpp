#include "slacktext.h"
#include "slacktext_p.h"
#include <QTextBoundaryFinder>

#include "QtQuick/private/qquicktextnode_p.h"
#include <QtQuick/qsgsimplerectnode.h>
#include <private/qv4scopedvalue_p.h>

DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

SlackText::SlackText(QQuickItem* parent)
: QQuickText(*(new SlackTextPrivate), parent)
{
    Q_D(SlackText);
    d->init();
}

SlackText::SlackText(SlackTextPrivate &dd, QQuickItem *parent)
: QQuickText(dd, parent)
{
    Q_D(SlackText);
    d->init();
}

void SlackText::componentComplete()
{
    Q_D(SlackText);

    QQuickText::componentComplete();

    d->updateLayout();
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

void SlackTextPrivate::init()
{
    Q_Q(SlackText);
#if QT_CONFIG(clipboard)
    if (QGuiApplication::clipboard()->supportsSelection())
        q->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton);
    else
#endif
        q->setAcceptedMouseButtons(Qt::LeftButton);

#if QT_CONFIG(im)
    q->setFlag(QQuickItem::ItemAcceptsInputMethod);
#endif
    q->setFlag(QQuickItem::ItemHasContents);

    lastSelectionStart = 0;
    lastSelectionEnd = 0;
    determineHorizontalAlignment();

    if (!qmlDisableDistanceField()) {
        QTextOption option = layout.textOption();
        option.setUseDesignMetrics(renderType != QQuickText::NativeRendering);
        layout.setTextOption(option);
    }
}


/*!
    \internal

    Moves the cursor to the given position \a pos.   If \a mark is true will
    adjust the currently selected text.
*/
void SlackTextPrivate::moveSelectionCursor(int pos, bool mark)
{
    Q_Q(SlackText);

//    if (pos != m_cursor) {
//        separate();
//        if (m_maskData)
//            pos = pos > m_cursor ? nextMaskBlank(pos) : prevMaskBlank(pos);
//    }
    if (mark) {
        int anchor;
        if (m_selend > m_selstart/* && m_cursor == m_selstart*/)
            anchor = m_selend;
        else if (m_selend > m_selstart/* && m_cursor == m_selend*/)
            anchor = m_selstart;
        /*else
            anchor = m_cursor;*/
        m_selstart = qMin(anchor, pos);
        m_selend = qMax(anchor, pos);
    } else {
        internalDeselect();
    }
    //m_cursor = pos;
    if (mark || m_selDirty) {
        m_selDirty = false;
        emit q->selectionChanged();
    }
    //emitCursorPositionChanged();

}

/*!
    \qmlmethod rect QtQuick::TextInput::positionToRectangle(int pos)

    This function takes a character position and returns the rectangle that the
    cursor would occupy, if it was placed at that character position.

    This is similar to setting the cursorPosition, and then querying the cursor
    rectangle, but the cursorPosition is not changed.
*/
QRectF SlackText::positionToRectangle(int pos) const
{
    Q_D(const SlackText);
//    if (d->m_echoMode == NoEcho)
//        pos = 0;
//#if QT_CONFIG(im)
//    else if (pos > d->m_cursor)
//        pos += d->preeditAreaText().length();
//#endif
    QTextLine l = d->layout.lineForTextPosition(pos);
    if (!l.isValid())
        return QRectF();
    qreal x = l.cursorToX(pos)/* - d->hscroll*/;
    qreal y = l.y()/* - d->vscroll*/;
    qreal w = 1;
//    if (d->overwriteMode) {
//        if (pos < text().length())
//            w = l.cursorToX(pos + 1) - x;
//        else
//            w = QFontMetrics(font()).width(QLatin1Char(' ')); // in sync with QTextLine::draw()
//    }
    return QRectF(x, y, w, l.height());
}

/*!
    \qmlmethod int QtQuick::TextInput::positionAt(real x, real y, CursorPosition position = CursorBetweenCharacters)

    This function returns the character position at
    x and y pixels from the top left  of the textInput. Position 0 is before the
    first character, position 1 is after the first character but before the second,
    and so on until position text.length, which is after all characters.

    This means that for all x values before the first character this function returns 0,
    and for all x values after the last character this function returns text.length.  If
    the y value is above the text the position will be that of the nearest character on
    the first line and if it is below the text the position of the nearest character
    on the last line will be returned.

    The cursor position type specifies how the cursor position should be resolved.

    \list
    \li TextInput.CursorBetweenCharacters - Returns the position between characters that is nearest x.
    \li TextInput.CursorOnCharacter - Returns the position before the character that is nearest x.
    \endlist
*/

void SlackText::positionAt(QQmlV4Function *args) const
{
    Q_D(const SlackText);

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

int SlackTextPrivate::positionAt(qreal x, qreal y, QTextLine::CursorPosition position) const
{
    Q_Q(const SlackText);
    x += q->leftPadding();
    y += q->topPadding();
    QTextLine line = layout.lineAt(0);
    for (int i = 1; i < layout.lineCount(); ++i) {
        QTextLine nextLine = layout.lineAt(i);

        if (y < (line.rect().bottom() + nextLine.y()) / 2)
            break;
        line = nextLine;
    }
    return line.isValid() ? line.xToCursor(x, position) : 0;
}

void SlackText::mousePressEvent(QMouseEvent *event)
{
    Q_D(SlackText);

    d->pressPos = event->localPos();

//    if (d->sendMouseEventToInputContext(event))
//        return;

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

    event->setAccepted(true);
}

void SlackText::updatePolish()
{
    invalidateFontCaches();
}

void SlackText::invalidateFontCaches()
{
    Q_D(SlackText);

    if (d->layout.engine() != nullptr)
        d->layout.engine()->resetFontEngineCache();
}

void SlackText::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(SlackText);

    if (d->selectByMouse && event->button() == Qt::LeftButton) {
//#if QT_CONFIG(im)
//        d->commitPreedit();
//#endif
        int cursor = d->positionAt(event->localPos());
        d->selectWordAtPos(cursor);
        event->setAccepted(true);
        if (!d->hasPendingTripleClick()) {
            d->tripleClickStartPoint = event->localPos();
            d->tripleClickTimer.start();
        }
    } else {
//        if (d->sendMouseEventToInputContext(event))
//            return;
        QQuickImplicitSizeItem::mouseDoubleClickEvent(event);
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
    int next = cursor + 1;
    if (next > end())
        --next;
    int c = layout.previousCursorPosition(next, QTextLayout::SkipWords);
    moveSelectionCursor(c, false);
    // ## text layout should support end of words.
    int end = layout.nextCursorPosition(c, QTextLayout::SkipWords);
    while (end > cursor && text[end-1].isSpace())
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


void SlackText::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(SlackText);

    if (d->selectPressed) {
        if (qAbs(int(event->localPos().x() - d->pressPos.x())) > QGuiApplication::styleHints()->startDragDistance())
            setKeepMouseGrab(true);
        moveCursorSelection(d->positionAt(event->localPos()), d->mouseSelectionMode);
        event->setAccepted(true);
    } else {
        QQuickImplicitSizeItem::mouseMoveEvent(event);
    }
}

void SlackText::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(SlackText);
//    if (d->sendMouseEventToInputContext(event))
//        return;
    if (d->selectPressed) {
        d->selectPressed = false;
        setKeepMouseGrab(false);
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

//    if (d->focusOnPress && qGuiApp->styleHints()->setFocusOnTouchRelease())
//        ensureActiveFocus();

    if (!event->isAccepted())
        QQuickImplicitSizeItem::mouseReleaseEvent(event);
}

void SlackText::mouseUngrabEvent()
{
    Q_D(SlackText);
    d->selectPressed = false;
    setKeepMouseGrab(false);
}

QSGNode *SlackText::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(SlackText);

    if (d->updateType != SlackTextPrivate::UpdatePaintNode && oldNode != nullptr) {
        // Update done in preprocess() in the nodes
        d->updateType = SlackTextPrivate::UpdateNone;
        return oldNode;
    }

    d->updateType = SlackTextPrivate::UpdateNone;

    QQuickTextNode *node = static_cast<QQuickTextNode *>(oldNode);
    if (node == nullptr)
        node = new QQuickTextNode(this);
    d->textNode = node;

    if (d->textLayoutDirty || oldNode == nullptr) {
        node->setUseNativeRenderer(d->renderType == NativeRendering);
        node->deleteContent();
        node->setMatrix(QMatrix4x4());

        QPointF offset(leftPadding(), topPadding());
        if (d->layout.lineCount() > 0) {
            QFontMetricsF fm(d->font);
            // the y offset is there to keep the baseline constant in case we have script changes in the text.
            //offset += -QPointF(d->hscroll, d->vscroll + d->layout.lineAt(0).ascent() - fm.ascent());
        } /*else {
            offset += -QPointF(d->hscroll, d->vscroll);
        }*/

        if (!d->layout.text().isEmpty()) {
            node->addTextLayout(offset, &d->layout, d->color,
                                QQuickText::Normal, QColor(), QColor(),
                                d->selectionColor, d->selectedTextColor,
                                d->selectionStart(),
                                d->selectionEnd() - 1); // selectionEnd() returns first char after
                                                                 // selection
        }

        d->textLayoutDirty = false;
    }

    invalidateFontCaches();

    return node;
}

/*!
    \qmlmethod QtQuick::TextInput::deselect()

    Removes active text selection.
*/
void SlackText::deselect()
{
    Q_D(SlackText);
    d->deselect();
}

/*!
    \qmlmethod QtQuick::TextInput::selectAll()

    Causes all text to be selected.
*/
void SlackText::selectAll()
{
    Q_D(SlackText);
    d->setSelection(0, text().length());
}

/*!
    \qmlmethod QtQuick::TextInput::selectWord()

    Causes the word closest to the current cursor position to be selected.
*/
void SlackText::selectWord()
{
    Q_D(SlackText);
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

/*!
    \qmlproperty enumeration QtQuick::TextInput::mouseSelectionMode

    Specifies how text should be selected using a mouse.

    \list
    \li TextInput.SelectCharacters - The selection is updated with individual characters. (Default)
    \li TextInput.SelectWords - The selection is updated with whole words.
    \endlist

    This property only applies when \l selectByMouse is true.
*/

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

/*!
    \qmlproperty bool QtQuick::TextInput::persistentSelection

    Whether the TextInput should keep its selection when it loses active focus to another
    item in the scene. By default this is set to false;
*/

bool SlackText::persistentSelection() const
{
    Q_D(const SlackText);
    return d->persistentSelection;
}

void SlackText::setPersistentSelection(bool on)
{
    Q_D(SlackText);
    if (d->persistentSelection == on)
        return;
    d->persistentSelection = on;
    emit persistentSelectionChanged();
}

void SlackText::moveCursorSelection(int position)
{
    Q_D(SlackText);
    d->moveSelectionCursor(position, true);
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
void SlackText::moveCursorSelection(int pos, SelectionMode mode)
{
    Q_D(SlackText);

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

void SlackText::selectionChanged()
{
    Q_D(SlackText);
    d->textLayoutDirty = true; //TODO: Only update rect in selection
    d->updateType = SlackTextPrivate::UpdatePaintNode;
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
void SlackTextPrivate::setSelection(int start, int length)
{
    Q_Q(SlackText);

    if (start < 0 || start > text.length()) {
        qWarning("QQuickTextInputPrivate::setSelection: Invalid start position");
        return;
    }

    if (length > 0) {
        if (start == m_selstart && start + length == m_selend && m_cursor == m_selend)
            return;
        m_selstart = start;
        m_selend = qMin(start + length, text.length());
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
        //emitCursorPositionChanged();
        return;
    }
    emit q->selectionChanged();
    //emitCursorPositionChanged();
}

/*!
    \internal

    Completes a change to the line control text.  If the change is not valid
    will undo the line control state back to the given \a validateFromState.

    If \a edited is true and the change is valid, will emit textEdited() in
    addition to textChanged().  Otherwise only emits textChanged() on a valid
    change.

    The \a update value is currently unused.
*/
bool SlackTextPrivate::finishChange(bool update)
{
    Q_Q(SlackText);

    Q_UNUSED(update)
//#if QT_CONFIG(im)
//    bool inputMethodAttributesChanged = m_textDirty || m_selDirty;
//#endif
    bool alignmentChanged = false;
    bool textChanged = false;

//    if (m_textDirty) {
//        // do validation
//        bool wasValidInput = m_validInput;
//        bool wasAcceptable = m_acceptableInput;
//        m_validInput = true;
//        m_acceptableInput = true;
//#if QT_CONFIG(validator)
//        if (m_validator) {
//            QString textCopy = m_text;
//            if (m_maskData)
//                textCopy = maskString(0, m_text, true);
//            int cursorCopy = m_cursor;
//            QValidator::State state = m_validator->validate(textCopy, cursorCopy);
//            if (m_maskData)
//                textCopy = m_text;
//            m_validInput = state != QValidator::Invalid;
//            m_acceptableInput = state == QValidator::Acceptable;
//            if (m_validInput && !m_maskData) {
//                if (m_text != textCopy) {
//                    internalSetText(textCopy, cursorCopy);
//                    return true;
//                }
//                m_cursor = cursorCopy;
//            }
//        }
//#endif
//        if (m_maskData)
//            checkIsValid();

//        if (validateFromState >= 0 && wasValidInput && !m_validInput) {
//            if (m_transactions.count())
//                return false;
//            internalUndo(validateFromState);
//            m_history.resize(m_undoState);
//            m_validInput = true;
//            m_acceptableInput = wasAcceptable;
//            m_textDirty = false;
//        }

//        if (m_textDirty) {
//            textChanged = true;
//            m_textDirty = false;
//#if QT_CONFIG(im)
//            m_preeditDirty = false;
//#endif
//            alignmentChanged = determineHorizontalAlignment();
//            if (edited)
//                emit q->textEdited();
//            emit q->textChanged();
//        }

//        updateDisplayText(alignmentChanged);

//        if (m_acceptableInput != wasAcceptable)
//            emit q->acceptableInputChanged();
//    }
//#if QT_CONFIG(im)
//    if (m_preeditDirty) {
//        m_preeditDirty = false;
//        if (determineHorizontalAlignment()) {
//            alignmentChanged = true;
//            updateLayout();
//        }
//    }
//#endif

    if (m_selDirty) {
        m_selDirty = false;
        emit q->selectionChanged();
    }

//#if QT_CONFIG(im)
//    inputMethodAttributesChanged |= (m_cursor != m_lastCursorPos);
//    if (inputMethodAttributesChanged)
//        q->updateInputMethod();
//#endif
//    emitUndoRedoChanged();

//    if (!emitCursorPositionChanged() && (alignmentChanged || textChanged))
//        q->updateCursorRectangle();

    return true;
}

void SlackTextPrivate::updateLayout()
{
    Q_Q(SlackText);

    if (!q->isComponentComplete())
        return;

    QQuickTextPrivate::updateLayout();
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

//qreal SlackTextPrivate::getImplicitWidth() const
//{
//    Q_Q(const SlackText);
//    if (!requireImplicitWidth) {
//        SlackTextPrivate *d = const_cast<SlackTextPrivate *>(this);
//        d->requireImplicitWidth = true;

//        if (q->isComponentComplete()) {
//            // One time cost, only incurred if implicitWidth is first requested after
//            // componentComplete.
//            QTextLayout layout(text);

//            QTextOption option = layout.textOption();
//            option.setTextDirection(layoutDirection);
//            option.setFlags(QTextOption::IncludeTrailingSpaces);
//            option.setWrapMode(QTextOption::WrapMode(wrapMode));
//            option.setAlignment(Qt::Alignment(q->effectiveHAlign()));
//            layout.setTextOption(option);
//            layout.setFont(font);
//            layout.beginLayout();

//            QTextLine line = layout.createLine();
//            line.setLineWidth(INT_MAX);
//            d->implicitWidth = qCeil(line.naturalTextWidth()) + q->leftPadding() + q->rightPadding();

//            layout.endLayout();
//        }
//    }
//    return implicitWidth;
//}
