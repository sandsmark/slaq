import QtQuick 2.11

Item {
    width: rect.width
    height: rect.height

    function hovered(mouseArea) {}
    function clicked() {
    }

    Rectangle {
        id: rect
        width: 300
        height: 50
        color: "lightgray"
        Text {
            anchors.centerIn: parent
            text: fileshare.name
            wrapMode: Text.WordWrap
            textFormat: Text.RichText
            renderType: Text.QtRendering
        }
    }
}

