#pragma once

#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickControls2/private/qtquickcontrols2global_p.h>

#include "ChatsModel.h"

class SlackTextPrivate;

class SlackText : public QQuickLabel
{

    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)

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

    Q_PROPERTY(QString hoveredLink READ hoveredLink NOTIFY linkHovered)
    Q_PROPERTY(QString hoveredImage READ hoveredImage NOTIFY imageHovered)

    Q_PROPERTY(QJSValue fontInfo READ fontInfo NOTIFY fontInfoChanged)
    Q_PROPERTY(QSizeF advance READ advance NOTIFY contentSizeChanged)
    Q_PROPERTY(ChatsModel* chat READ chat WRITE setChat NOTIFY chatChanged)
    Q_PROPERTY(QQuickItem* itemFocusOnUnselect READ itemFocusOnUnselect WRITE setItemFocusOnUnselect NOTIFY itemFocusOnUnselectChanged)

public:
    enum SelectionMode {
        SelectCharacters,
        SelectWords
    };
    Q_ENUM(SelectionMode)

    explicit SlackText(QQuickItem *parent = nullptr);

    virtual ~SlackText() = default;

    //Auxilliary functions needed to control the TextInput from QML
    Q_INVOKABLE void positionAt(QQmlV4Function *args);
    Q_INVOKABLE QRectF positionToRectangle(int pos);
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

    QString text() const;
    void setText(const QString &txt);

    TextFormat textFormat() const;
    void setTextFormat(TextFormat format);

    QString hoveredLink() const;
    QString hoveredImage() const;
    ChatsModel* chat() const;
    QQuickItem* itemFocusOnUnselect() const;

Q_SIGNALS:

    void textChanged(const QString &text);
    void textFormatChanged(QQuickText::TextFormat textFormat);
    void selectionColorChanged();
    void selectedTextColorChanged();
    void selectionStartChanged();
    void selectionEndChanged();
    void selectedTextChanged();
    void selectByMouseChanged(bool selectByMouse);
    void mouseSelectionModeChanged(SelectionMode mouseSelectionMode);
    void persistentSelectionChanged();

    void linkHovered(const QString &link);
    void imageHovered(const QString &imagelink, qreal x, qreal y);
    void chatChanged(ChatsModel* chat);
    void itemFocusOnUnselectChanged(QQuickItem* itemFocusOnUnselect);

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
    void setChat(ChatsModel* chat);
    void setItemFocusOnUnselect(QQuickItem* itemFocusOnUnselect);

private Q_SLOTS:
    void selectionChanged();
    void postProcessText();
    QString preProcessText(const QString &txt);
    void onImageLoaded(const QString &id);

protected:
    void componentComplete() override;
    //SlackText(SlackTextPrivate &dd, QQuickItem *parent = nullptr);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    //void updatePolish() override;

    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;

private:
    void invalidateFontCaches();
    void insertImage(QTextCursor &cursor, const QString &url, const QImage &img);
    bool markupUpdate(const QString &markupQuote, const QString &markupEndQuote,
                      std::function<void(QTextCursor& from, QString &selText)> markupReplace);

    QColor m_selectionColor;
    QColor m_selectedTextColor;
    int m_selectionStart;
    int m_selectionEnd;
    QString m_selectedText;
    bool m_selectByMouse;
    SelectionMode m_mouseSelectionMode;
    bool m_persistentSelection;
    ChatsModel* m_chat {nullptr};

private:
    SlackTextPrivate* d_ptr { nullptr };
    Q_DECLARE_PRIVATE(SlackText)
    Q_DISABLE_COPY(SlackText)
};

QML_DECLARE_TYPE(SlackText)
