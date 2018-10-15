#pragma once

#include <QtQuick/private/qquicktext_p.h>

class SlackText : public QQuickText
{
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(QColor selectedTextColor READ selectedTextColor WRITE setSelectedTextColor NOTIFY selectedTextColorChanged)
    Q_PROPERTY(int selectionStart READ selectionStart NOTIFY selectionStartChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd NOTIFY selectionEndChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectedTextChanged)
    Q_PROPERTY(bool selectByMouse READ selectByMouse WRITE setSelectByMouse NOTIFY selectByMouseChanged)
    Q_PROPERTY(SelectionMode mouseSelectionMode READ mouseSelectionMode WRITE setMouseSelectionMode NOTIFY mouseSelectionModeChanged)
    Q_PROPERTY(bool persistentSelection READ persistentSelection WRITE setPersistentSelection NOTIFY persistentSelectionChanged)

public:
    enum SelectionMode {
        SelectCharacters,
        SelectWords
    };
    Q_ENUM(SelectionMode)

    explicit SlackText(QQuickItem *parent = nullptr);
    virtual ~SlackText();

    Q_INVOKABLE void moveCursorSelection(int pos);
    Q_INVOKABLE void moveCursorSelection(int pos, SelectionMode mode);

    QColor selectionColor() const;
    QColor selectedTextColor() const;
    int selectionStart() const;
    int selectionEnd() const;
    QString selectedText() const;

public Q_SLOTS:
    void selectAll();
    void selectWord();
    void select(int start, int end);
    void deselect();

    void setSelectionColor(const QColor &color);
    void setSelectedTextColor(const QColor &color);

signals:
    void selectionColorChanged(QColor selectionColor);
    void selectedTextColorChanged(QColor selectedTextColor);
    void selectionStartChanged(int selectionStart);

    void selectionEndChanged(int selectionEnd);

    void selectedTextChanged(QString selectedText);

private:
    Q_DISABLE_COPY(SlackText)

    Q_DECLARE_PRIVATE(SlackText)

    QColor m_selectionColor;
    QColor m_selectedTextColor;
    int m_selectionStart;
    int m_selectionEnd;
    QString m_selectedText;
};

Q_DECLARE_TYPEINFO(SlackText, Q_COMPLEX_TYPE);

