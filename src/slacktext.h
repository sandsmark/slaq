#pragma once

#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickControls2/private/qtquickcontrols2global_p.h>

class SlackTextPrivate;

class Q_QUICKCONTROLS2_PRIVATE_EXPORT  SlackText : public QQuickLabel
{

    Q_OBJECT

    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(QColor selectedTextColor READ selectedTextColor WRITE setSelectedTextColor NOTIFY selectedTextColorChanged)
    Q_PROPERTY(int selectionStart READ selectionStart NOTIFY selectionStartChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd NOTIFY selectionEndChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectedTextChanged)
    Q_PROPERTY(bool selectByMouse READ selectByMouse WRITE setSelectByMouse NOTIFY selectByMouseChanged)
    Q_PROPERTY(SelectionMode mouseSelectionMode READ mouseSelectionMode WRITE setMouseSelectionMode NOTIFY mouseSelectionModeChanged)
    Q_PROPERTY(bool persistentSelection READ persistentSelection WRITE setPersistentSelection NOTIFY persistentSelectionChanged)

    Q_PROPERTY(qreal padding READ padding WRITE setPadding RESET resetPadding NOTIFY paddingChanged)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding NOTIFY topPaddingChanged)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding NOTIFY leftPaddingChanged)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding NOTIFY rightPaddingChanged)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET resetBottomPadding NOTIFY bottomPaddingChanged)

public:
    enum SelectionMode {
        SelectCharacters,
        SelectWords
    };
    Q_ENUM(SelectionMode)

    explicit SlackText(QQuickItem *parent = nullptr);

    virtual ~SlackText() = default;

    //Auxilliary functions needed to control the TextInput from QML
    Q_INVOKABLE void positionAt(QQmlV4Function *args) const;
    Q_INVOKABLE QRectF positionToRectangle(int pos) const;
    Q_INVOKABLE void moveCursorSelection(int pos);
    Q_INVOKABLE void moveCursorSelection(int pos, SelectionMode mode);

    QColor selectionColor() const;
    QColor selectedTextColor() const;
    int selectionStart() const;
    int selectionEnd() const;
    QString selectedText() const;
    bool selectByMouse() const;
    SelectionMode mouseSelectionMode() const;
    bool persistentSelection() const;

Q_SIGNALS:
    void selectionColorChanged();
    void selectedTextColorChanged();
    void selectionStartChanged();
    void selectionEndChanged();
    void selectedTextChanged();
    void selectByMouseChanged(bool selectByMouse);
    void mouseSelectionModeChanged(SelectionMode mouseSelectionMode);
    void persistentSelectionChanged();

public Q_SLOTS:
    void copy();
    void selectAll();
    void selectWord();
    void select(int start, int end);
    void deselect();
    void setSelectionColor(const QColor &color);
    void setSelectedTextColor(const QColor &color);
    void setSelectByMouse(bool on);
    void setMouseSelectionMode(SelectionMode mouseSelectionMode);
    void setPersistentSelection(bool persistentSelection);


private Q_SLOTS:
    void selectionChanged();

protected:
    void componentComplete() override;
    //SlackText(SlackTextPrivate &dd, QQuickItem *parent = nullptr);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
    //QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    //void updatePolish() override;

private:
    void invalidateFontCaches();

    QColor m_selectionColor;
    QColor m_selectedTextColor;
    int m_selectionStart;
    int m_selectionEnd;
    QString m_selectedText;
    bool m_selectByMouse;
    SelectionMode m_mouseSelectionMode;
    bool m_persistentSelection;

private:
    Q_DECLARE_PRIVATE(SlackText)
    Q_DISABLE_COPY(SlackText)

};

QML_DECLARE_TYPE(SlackText)
