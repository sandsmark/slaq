import QtQuick 2.11
import QtQuick.Controls 2.4
import ".."

Button {
    property real bgSizeW: Theme.headerSize
    property real bgSizeH: Theme.headerSize
    property real sizeW: Theme.headerSize
    property real sizeH: Theme.headerSize
    property real sizeHoveredW: Theme.headerSize
    property real sizeHoveredH: Theme.headerSize

    property alias source: avatarImage.source

    id: button
    display: AbstractButton.IconOnly
    hoverEnabled: true
    background: Item {
        implicitHeight: bgSizeH
        implicitWidth: bgSizeW
    }
    contentItem: Image {
        id: avatarImage
        x: (bgSizeW - sizeW)/2
        y: (bgSizeH - sizeH)/2
        sourceSize: Qt.size(sizeW, sizeH)
        width: button.hovered ? sizeHoveredW : sizeW
        height: button.hovered ? sizeHoveredH : sizeH
        smooth: true
    }
}
