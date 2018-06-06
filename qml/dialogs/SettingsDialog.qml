import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0

Dialog {
    id: settingsDialog
    x: Math.round((window.width - width) / 2)
    y: Math.round(window.height / 6)
    width: Math.round(Math.min(window.width, window.height) / 3 * 2)
    modal: true
    focus: true
    title: "Settings"

    standardButtons: Dialog.Ok | Dialog.Cancel
    onAccepted: {
        ImagesCache.setEmojiImagesSet(setsBox.displayText)
        settingsDialog.close()
    }
    onRejected: {
        setsBox.currentIndex = setsBox.setIndex
        settingsDialog.close()
    }

    contentItem: ColumnLayout {
        id: settingsColumn
        spacing: 20

        RowLayout {
            spacing: 10

            Label {
                text: "Emojis:"
            }

            ComboBox {
                id: setsBox
                property int setIndex: 0
                model: ImagesCache.getEmojiImagesSetsNames();
                Component.onCompleted: {
                    setIndex = find(settings.emojisSet, Qt.MatchFixedString)
                    if (setIndex !== -1)
                        currentIndex = setIndex
                }
                Layout.fillWidth: true
            }
        }
    }
}
