#pragma once

#include <QtQuick/private/qquicktext_p_p.h>

#include <QtQml/qqml.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qpointer.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpalette.h>
#include <QtGui/qtextlayout.h>
#include <QtGui/qstylehints.h>
#include <QtGui/qclipboard.h>
#include <private/qlazilyallocated_p.h>

#include "qplatformdefs.h"

#include "slacktext.h"

QT_BEGIN_NAMESPACE

class QQuickTextNode;
class QInputControl;

class SlackTextPrivate : public QQuickTextPrivate
{
public:
    Q_DECLARE_PUBLIC(SlackText)

    //typedef SlackText Public;

    SlackTextPrivate():
        selectionColor(QRgb(0xFF000080))
      , selectedTextColor(QRgb(0xFFFFFFFF))
      , m_selstart(0)
      , m_selend(0)
      , mouseSelectionMode(SlackText::SelectCharacters)
      , selectByMouse(false)
      , selectPressed(false)
      , persistentSelection(false)
      , m_cursor(0)
    {
    }

    ~SlackTextPrivate()
    {
    }

    //void mirrorChange() override;
    void init();

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
    int m_cursor;

    QQuickTextNode *textNode;

    UpdateType updateType;
    SlackText::SelectionMode mouseSelectionMode;

    bool selectByMouse:1;
    bool selectPressed:1;
    bool m_selDirty : 1;
    bool textLayoutDirty:1;
    bool persistentSelection:1;

    static inline SlackTextPrivate *get(SlackText *t) {
        return t->d_func();
    }
    bool hasPendingTripleClick() const {
        return !tripleClickTimer.hasExpired(QGuiApplication::styleHints()->mouseDoubleClickInterval());
    }
    int positionAt(qreal x, qreal y, QTextLine::CursorPosition position) const;
    int positionAt(const QPointF &point, QTextLine::CursorPosition position = QTextLine::CursorBetweenCharacters) const {
        return positionAt(point.x(), point.y(), position);
    }

    void moveSelectionCursor(int pos, bool mark = false);

    bool allSelected() const { return !text.isEmpty() && m_selstart == 0 && m_selend == (int)text.length(); }
    bool hasSelectedText() const { return !text.isEmpty() && m_selend > m_selstart; }

    void setSelection(int start, int length);
    int end() const { return text.length(); }

    inline QString selectedText() const { return hasSelectedText() ? text.mid(m_selstart, m_selend - m_selstart) : QString(); }
    QString textBeforeSelection() const { return hasSelectedText() ? text.left(m_selstart) : QString(); }
    QString textAfterSelection() const { return hasSelectedText() ? text.mid(m_selend) : QString(); }

    int selectionStart() const { return hasSelectedText() ? m_selstart : -1; }
    int selectionEnd() const { return hasSelectedText() ? m_selend : -1; }

    void copy(QClipboard::Mode mode = QClipboard::Clipboard) const;

    void removeSelection()
    {
        //removeSelectedText();
        finishChange();
    }

    QString realText() const { return text; };

    void deselect() { internalDeselect(); finishChange(); }
    void selectAll() { m_selstart = m_selend = 0; moveSelectionCursor(text.length(), true); }

    void selectWordAtPos(int);
    void processKeyEvent(QKeyEvent* ev);

    void updateLayout();
    //void updateBaselineOffset();

    //qreal getImplicitWidth() const override;


private:
    //void removeSelectedText();
    //void internalSetText(const QString &txt, int pos = -1, bool edited = true);
    //void updateDisplayText(bool forceUpdate = false);

//    void internalInsert(const QString &s);
//    void internalDelete(bool wasBackspace = false);
//    void internalRemove(int pos);

    inline void internalDeselect()
    {
        m_selDirty |= (m_selend > m_selstart);
        m_selstart = m_selend = 0;
    }


    //bool emitCursorPositionChanged();

    bool finishChange(bool update = false);

    //bool separateSelection();
};

QT_END_NAMESPACE
