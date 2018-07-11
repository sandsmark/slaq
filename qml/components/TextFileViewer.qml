import QtQuick 2.11

Item {
    width: textEdit.width
    height: textEdit.height

    function hovered(mouseArea) {}
    function clicked() {
        channelRoot.showText(fileshare.url_private_download, fileshare.name, fileshare.username)
    }

    Text {
        id: textEdit
        width: paintedWidth
        height: paintedHeight
        text: fileshare.preview_highlight
        wrapMode: Text.WordWrap
        textFormat: Text.RichText
        renderType: Text.QtRendering
    }
}

