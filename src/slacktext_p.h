#pragma once

#include "QtQuick/private/qquicktextdocument_p.h"
#include <QtQuick/private/qquicktext_p_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p_p.h>

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

class QQuickTextNode;

class SlackTextPrivate
{
    Q_DECLARE_PUBLIC(SlackText)
public:

    //typedef SlackText Public;

    SlackTextPrivate():
        selectionColor(QRgb(0xFF000080))
      , selectedTextColor(QRgb(0xFFFFFFFF))
      , m_selstart(0)
      , m_selend(0)
      , m_maskData(nullptr)
      , mouseSelectionMode(SlackText::SelectCharacters)
      , selectByMouse(true)
      , selectPressed(false)
      , m_selDirty(false)
      , textLayoutDirty(false)
      , persistentSelection(false)
      , m_modified(false)
      , m_dirty(false)
      , m_cursor(0)
      , m_separator(0)
      , m_maxLength(32767)
    {
    }

    ~SlackTextPrivate()
    {
    }

    void init();

    struct MaskInputData {
        enum Casemode { NoCaseMode, Upper, Lower };
        QChar maskChar; // either the separator char or the inputmask
        bool separator;
        Casemode caseMode;
    };

    enum DrawFlags {
        DrawText = 0x01,
        DrawSelections = 0x02,
        DrawAll = DrawText | DrawSelections
    };

    QElapsedTimer tripleClickTimer;
    QSizeF contentSize;
    QPointF pressPos;
    QPointF tripleClickStartPoint;

    QColor selectionColor;
    QColor selectedTextColor;

    QString m_linkHovered;
    QString m_imageHovered;

    int lastSelectionStart;
    int lastSelectionEnd;
    int m_selstart;
    int m_selend;
    int m_cursor;
    int m_maxLength;

    QQuickTextNode *textNode;
    SlackText* q_ptr { nullptr };
    MaskInputData *m_maskData;
    QImage frameImage;
    QHash<QString, int> m_requestedImages;

    SlackText::SelectionMode mouseSelectionMode;

    bool selectByMouse:1;
    bool selectPressed:1;
    bool m_selDirty : 1;
    bool m_separator : 1;
    bool textLayoutDirty:1;
    bool persistentSelection:1;
    bool m_modified : 1;
    bool m_dirty : 1;

    int nextMaskBlank(int pos)
    {
        int c = findInMask(pos, true, false);
        m_separator |= (c != pos);
        return (c != -1 ?  c : m_maxLength);
    }

    int prevMaskBlank(int pos)
    {
        int c = findInMask(pos, false, false);
        m_separator |= (c != pos);
        return (c != -1 ? c : 0);
    }

    static inline SlackTextPrivate *get(SlackText *t) {
        qWarning() << "SlackTextPrivate *get";
        return t->d_func();
    }

    bool hasPendingTripleClick() const {
        return !tripleClickTimer.hasExpired(QGuiApplication::styleHints()->mouseDoubleClickInterval());
    }
    int positionAt(qreal x, qreal y, QTextLine::CursorPosition position);
    int positionAt(const QPointF &point, QTextLine::CursorPosition position = QTextLine::CursorBetweenCharacters) {
        return positionAt(point.x(), point.y(), position);
    }

    void moveSelectionCursor(int pos, bool mark = false);

    bool allSelected() const {
        Q_Q(const SlackText);
        return !q->text().isEmpty() && m_selstart == 0 && m_selend == (int)q->text().length();
    }
    bool hasSelectedText() const {
        Q_Q(const SlackText);
        return !q->text().isEmpty() && m_selend > m_selstart;
    }

    void setSelection(int start, int length);
    int end() const {
        Q_Q(const SlackText);
        return q->text().length();
    }

    inline QString selectedText() const {
        Q_Q(const SlackText);
        return hasSelectedText() ? m_tp->extra->doc->toPlainText().mid(m_selstart, m_selend - m_selstart) : QString();
    }
    QString textBeforeSelection() const {
        Q_Q(const SlackText);
        return hasSelectedText() ? q->text().left(m_selstart) : QString();
    }
    QString textAfterSelection() const {
        Q_Q(const SlackText);
        return hasSelectedText() ? q->text().mid(m_selend) : QString();
    }

    int selectionStart() const { return hasSelectedText() ? m_selstart : -1; }
    int selectionEnd() const { return hasSelectedText() ? m_selend : -1; }

    void copy(QClipboard::Mode mode = QClipboard::Clipboard) const;
    int findInMask(int pos, bool forward, bool findSeparator, QChar searchChar = QChar()) const;

    void removeSelection()
    {
        //removeSelectedText();
        finishChange();
    }

    QString realText() const {
        Q_Q(const SlackText);
        return q->text();
    };

    void deselect() { internalDeselect(); finishChange(); }
    void selectAll() { m_selstart = m_selend = 0; moveSelectionCursor(end(), true); }

    void selectWordAtPos(int);
    void processKeyEvent(QKeyEvent* ev);

    void updateLayout();
private:
    //void removeSelectedText();
    //void internalSetText(const QString &txt, int pos = -1, bool edited = true);
    //void updateDisplayText(bool forceUpdate = false);

//    void internalInsert(const QString &s);
//    void internalDelete(bool wasBackspace = false);
//    void internalRemove(int pos);

    inline void separate() { m_separator = true; }

    inline void internalDeselect()
    {
        m_selDirty |= (m_selend > m_selstart);
        m_selstart = m_selend = 0;
    }


    //bool emitCursorPositionChanged();

    bool finishChange(bool update = false);

    QQuickLabelPrivate* m_lp { nullptr };
    QQuickTextPrivate* m_tp { nullptr };

    //bool separateSelection();
};
