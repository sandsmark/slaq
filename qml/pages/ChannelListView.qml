import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import ".."
import "../components"

import com.iskrembilen 1.0

ListView {
    id: listView

    ScrollBar.vertical: ScrollBar { }

    function setIndex(ind) {
        currentIndex = ind
    }
    interactive: true
    clip: true
    model: SlackClient.chatsModel(teamRoot.teamId)

    function leave(chatType, channelId, channelName) {
        switch (chatType) {
        case ChatsModel.Channel:
            SlackClient.leaveChannel(teamRoot.teamId, channelId)
            break

        case ChatsModel.Group:
        case ChatsModel.MultiUserConversation:
            var dialog = pageStack.push(Qt.resolvedUrl("GroupLeaveDialog.qml"), {"name": channelName })
            dialog.accepted.connect(function() {
                SlackClient.leaveGroup(teamRoot.teamId, channelId)
            })
            break

        case ChatsModel.Conversation:
            SlackClient.closeChat(teamRoot.teamId,  channelId)
        }
    }

    section {
        property: "Section"
        criteria: ViewSection.FullString
        delegate: Label {
            text: section
            padding: Theme.paddingMedium

            background: Rectangle {
                width: listView.width
                color: palette.base
                EmojiRoundButton {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    text: "➕"
                    onClicked: {
                        if (section == qsTr("Channels")) {
                            teamRoot.chatSelect.state = "channels"
                        } else {
                            teamRoot.chatSelect.state = "users"
                        }
                        teamRoot.chatSelect.open()
                    }
                }
            }
        }
    }

    Timer {
        id: chatChangeTimer
        interval: 32
        repeat: false
        property string channelId: ""
        onTriggered: {
            var channel = SlackClient.getChannel(teamRoot.teamId, channelId);
            if (channel != null) {
                pageStack.replace(Qt.resolvedUrl("Channel.qml"), {"channel": channel})
            } else {
                console.warn("No chat found for id", channelId)
            }
        }
    }

    delegate: ItemDelegate {
        id: delegate
        text: model.Name
        hoverEnabled: true
        highlighted: teamRoot.currentChannelId === model.Id
        visible: model.IsOpen
        enabled: visible
        height: visible ? delegate.implicitHeight : 0
        spacing: Theme.paddingSmall
        icon.color: "transparent"
        icon.source: model.PresenceIcon

        EmojiRoundButton {
            visible: delegate.hovered
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: listView.ScrollBar.vertical.width + Theme.paddingSmall
            text: "\uD83D\uDDD1"
            onClicked: {
                leave(model.Type, model.Id, model.Name)
            }
        }

        Rectangle {
            anchors.right: parent.right
            anchors.rightMargin: listView.ScrollBar.vertical.visible ?
                                     listView.ScrollBar.vertical.width + Theme.paddingMedium :
                                     Theme.paddingMedium
            anchors.verticalCenter: parent.verticalCenter
            implicitWidth: parent.height/2
            implicitHeight: parent.height/2
            radius: height/2
            color: model.Type === ChatsModel.Channel ? "green" : "red"
            visible: model.UnreadCountDisplay > 0
            Text {
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: parent.height/1.5
                text: model.UnreadCountDisplay
            }
        }

        width: listView.width

        onClicked: {
            if (model.Id === SlackClient.lastChannel(teamRoot.teamId)) {
                console.warn("Clicked on same channel")
                return
            }
            SlackClient.setActiveWindow(teamRoot.teamId, model.Id)
            chatChangeTimer.channelId = model.Id
            chatChangeTimer.start()
        }

        property bool hasActions: model.Type === ChatsModel.Channel || model.Type === ChatsModel.Conversation
        onPressAndHold: if (hasActions) { actionsMenu.popup() }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: "RightButton"
            onClicked: if (hasActions) { actionsMenu.popup() }
        }

        Menu {
            id: actionsMenu

            MenuItem {
                text: model.Type === ChatsModel.Channel ? qsTr("Leave") : qsTr("Close")
                onClicked: {
                    leave(model.Type, model.Id, model.Name)
                }
            }
        }
    }
}
