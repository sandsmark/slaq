import QtQuick 2.8
import QtQuick.Controls 2.3

ToolButton {
    id: emojiButton
    contentItem: Label {
        text: emojiButton.text
        font.family: "Twitter Color Emoji"
        font.bold: emojiButton.font.bold
        font.pixelSize: emojiButton.font.pixelSize
        renderType: Text.QtRendering
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
}
