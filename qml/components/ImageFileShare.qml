import QtQuick 2.11
import QtQuick.Controls 2.4
import Qt.labs.platform 1.0 as Platform
import ".."

Item {
    id: imageItem
    property bool expanded: false
    width: expanded ? listView.width - Theme.paddingLarge * 4 : model.thumbSize.width
    height: expanded ? width / (model.size.width / model.size.height) : model.thumbSize.height

    Image {
        id: thumbImage
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: "team://" + teamId + "/" + model.thumbUrl
        sourceSize: Qt.size(model.thumbSize.width, model.thumbSize.height)
        visible: !imageItem.expanded
    }

    AnimatedImage {
        id: fullImage
        anchors.fill: parent
        //to preserve memory, cache is turned off, so to see animation again need to re-expand image
        //TODO: create settings to change the behavior
        source: imageItem.expanded ? "team://" + teamId + "/" + model.url : ""
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
            pageStack.push(Qt.resolvedUrl("SlackImage.qml"), {"model": model, "teamId": teamId})
        } else {
            imageItem.expanded = !imageItem.expanded
        }
    }

    function hovered(mouseArea) {
        mouseArea.cursorShape = imageItem.expanded ? Qt.ArrowCursor : Qt.WhatsThisCursor
        fader.visible = mouseArea.containsMouse
    }
}
