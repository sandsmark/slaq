import QtQuick 2.11
import QtQuick.Controls 2.4
import ".."

Button {
    //TODO: check for Theme
    id: control

    property variant reaction

    hoverEnabled: true
    ToolTip.text: reaction.users.join(", ")
    ToolTip.delay: 500
    ToolTip.timeout: 5000
    ToolTip.visible: hovered
    text: reaction.emoji
    height: Theme.headerSize
    width: (ImagesCache.isUnicode ? contentItem.contentWidth : Theme.headerSize - 4)
           + Theme.paddingMedium*2
           + countLabel.contentWidth

    onClicked: {
        SlackClient.deleteReaction(teamId, channelId, Time, reaction.name)
    }

    contentItem: Item {
        Image {
            anchors.centerIn: parent
            width: Theme.headerSize - 4
            height: Theme.headerSize - 4
            smooth: true
            visible: !ImagesCache.isUnicode
            source: "image://emoji/" + reaction.name
        }

        Label {
            visible: ImagesCache.isUnicode
            anchors.centerIn: parent
            text: control.text
            font.family: "Twitter Color Emoji"
            font.pixelSize: Theme.headerSize - 6
            renderType: Text.QtRendering
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
    }

    background: Rectangle {
        color: palette.base
        implicitWidth: 100
        implicitHeight: parent.height
        opacity: enabled ? 1 : 0.3
        border.color: "#bdbdbd"
        border.width: 1
        radius: 3
        Label {
            id: countLabel
            anchors.right: parent.right; anchors.rightMargin: Theme.paddingSmall
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: Theme.fontSizeSmall
            renderType: Text.QtRendering
            text: reaction.usersCount
        }
    }
}
