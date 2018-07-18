import QtQuick 2.11
import QtQuick.Controls 2.4
import Qt.labs.platform 1.0 as Platform
import ".."

Item {
    id: imageItem
    property bool expanded: false
    width: expanded ? listView.width - Theme.paddingLarge * 4 : fileshare.thumb_360_size.width
    height: expanded ? width / (fileshare.original_size.width / fileshare.original_size.height) : fileshare.thumb_360_size.height

    Image {
        id: thumbImage
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: "team://" + teamId + "/" + fileshare.thumb_360
        sourceSize: fileshare.thumb_360_size
        visible: !imageItem.expanded
    }

    AnimatedImage {
        id: fullImage
        anchors.fill: parent
        //to preserve memory, cache is turned off, so to see animation again need to re-expand image
        //TODO: create settings to change the behavior
        source: imageItem.expanded ? "team://" + teamId + "/" + fileshare.url_private : ""
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        visible: imageItem.expanded
        playing: imageItem.expanded
        cache: false
        smooth: true
    }

    ProgressBar {
        anchors.centerIn: parent
        opacity: value < 1
        value: imageItem.expanded ? fullImage.progress : thumbImage.progress
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }

    Rectangle {
        id: fader
        anchors.fill: thumbImage
        color: Qt.rgba(1, 1, 1, 0.1)
        visible: false
    }

    function clicked() {
        if (SlackClient.isDevice) {
            pageStack.push(Qt.resolvedUrl("SlackImage.qml"), {"model": fileshare, "teamId": teamId})
        } else {
            imageItem.expanded = !imageItem.expanded
        }
    }

    function hovered(mouseArea) {
        mouseArea.cursorShape = imageItem.expanded ? Qt.ArrowCursor : Qt.WhatsThisCursor
        fader.visible = mouseArea.containsMouse
    }
}
