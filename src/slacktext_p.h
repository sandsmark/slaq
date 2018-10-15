#pragma once

#include "slacktext.h"
#include <QtQuick/private/qquicktext_p_p.h>

#include <QtQml/qqml.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qpointer.h>
#include <QtCore/qbasictimer.h>
#include <QtGui/qclipboard.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpalette.h>
#include <QtGui/qtextlayout.h>
#include <QtGui/qstylehints.h>
#include <private/qlazilyallocated_p.h>

#include "qplatformdefs.h"


QT_BEGIN_NAMESPACE

class QQuickTextNode;
class QInputControl;

class Q_QUICK_PRIVATE_EXPORT SlackTextPrivate : public QQuickTextPrivate
{
public:
    Q_DECLARE_PUBLIC(SlackText)

    typedef SlackText Public;

    SlackTextPrivate():
        selectionColor(QRgb(0xFF000080))
        , selectedTextColor(QRgb(0xFFFFFFFF))
        , m_selstart(0)
        , m_selend(0)
        , mouseSelectionMode(SlackText::SelectCharacters)
        , selectByMouse(false)
        , selectPressed(false)
        , persistentSelection(false)
    {
    }

    ~SlackTextPrivate()
    {
    }

    void mirrorChange() override;
    bool sendMouseEventToInputContext(QMouseEvent *event);
#if QT_CONFIG(im)
    Qt::InputMethodHints effectiveInputMethodHints() const;
#endif
    void handleFocusEvent(QFocusEvent *event);


    enum DrawFlags {
        DrawText = 0x01,
        DrawSelections = 0x02,
        DrawCursor = 0x04,
        DrawAll = DrawText | DrawSelections | DrawCursor
    };

    QElapsedTimer tripleClickTimer;
    QSizeF contentSize;
    QPointF pressPos;
    QPointF tripleClickStartPoint;

    QColor selectionColor;
    QColor selectedTextColor;

    int lastSelectionStart;
    int lastSelectionEnd;
    int m_selstart;
    int m_selend;

    UpdateType updateType;
    SlackText::SelectionMode mouseSelectionMode;

    bool selectByMouse:1;
    bool selectPressed:1;
    bool m_selDirty : 1;

    static inline SlackTextPrivate *get(SlackText *t) {
        return t->d_func();
    }

    void moveSelectionCursor(int pos, bool mark = false);

    bool allSelected() const { return !text.isEmpty() && m_selstart == 0 && m_selend == (int)text.length(); }
    bool hasSelectedText() const { return !text.isEmpty() && m_selend > m_selstart; }

    void setSelection(int start, int length);

    inline QString selectedText() const { return hasSelectedText() ? text.mid(m_selstart, m_selend - m_selstart) : QString(); }
    QString textBeforeSelection() const { return hasSelectedText() ? text.left(m_selstart) : QString(); }
    QString textAfterSelection() const { return hasSelectedText() ? text.mid(m_selend) : QString(); }

    int selectionStart() const { return hasSelectedText() ? m_selstart : -1; }
    int selectionEnd() const { return hasSelectedText() ? m_selend : -1; }


    void removeSelection()
    {
        removeSelectedText();
        finishChange(priorState);
    }

    QString realText() const;

    void deselect() { internalDeselect(); finishChange(); }
    void selectAll() { m_selstart = m_selend = 0; moveCursor(text.length(), true); }

    void selectWordAtPos(int);

#if QT_CONFIG(im)
    void processInputMethodEvent(QInputMethodEvent *event);
#endif
    void processKeyEvent(QKeyEvent* ev);

    void updateLayout();
    void updateBaselineOffset();

    qreal getImplicitWidth() const override;


private:
    void removeSelectedText();
    void internalSetText(const QString &txt, int pos = -1, bool edited = true);
    void updateDisplayText(bool forceUpdate = false);

    void internalInsert(const QString &s);
    void internalDelete(bool wasBackspace = false);
    void internalRemove(int pos);

    inline void internalDeselect()
    {
        m_selDirty |= (m_selend > m_selstart);
        m_selstart = m_selend = 0;
    }

    void internalUndo(int until = -1);
    void internalRedo();
    void emitUndoRedoChanged();

    bool emitCursorPositionChanged();

    bool finishChange(int validateFromState = -1, bool update = false, bool edited = true);

    bool separateSelection();
};

QT_END_NAMESPACE

#endif // QQUICKTEXTINPUT_P_P_H
