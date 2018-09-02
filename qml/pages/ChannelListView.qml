import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import ".."
import "../components"

import com.iskrembilen 1.0

ListView {
    id: listView

    ScrollBar.vertical: ScrollBar { }
    GroupLeaveDialog {
        id: leaveDialog
        property string channelId
        onAccepted: {
            SlackClient.closeChat(teamRoot.teamId, channelId)
        }
    }

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
        case ChatsModel.Conversation:
            leaveDialog.name = channelName
            leaveDialog.channelId = channelId
            leaveDialog.open()
            break
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
                    width: parent.height / 1.2
                    height: parent.height / 1.2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: listView.ScrollBar.vertical.width + Theme.paddingSmall
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
            console.time("start_switching")
            var channel = SlackClient.getChannel(teamRoot.teamId, channelId);
            console.warn("got channel", channel.name)
            pageStack.loadChannel(channel)
        }
    }

    delegate: SwipeDelegate {
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
        swipe.enabled: true
        swipe.right: EmojiRoundButton {
            id: deleteLabel
            text: "\uD83D\uDDD1"
            padding: 12
            height: parent.height
            anchors.right: parent.right

            SwipeDelegate.onClicked: leave(model.Type, model.Id, model.Name)

            background: Rectangle {
                color: deleteLabel.SwipeDelegate.pressed ? Qt.darker("tomato", 1.1) : "tomato"
            }
        }

        Rectangle {
            y: parent.height / 3
            x: height / 3

            implicitWidth: parent.height/3
            implicitHeight: parent.height/3

            radius: height/2
            color: model.Type === ChatsModel.Channel ? "green" : "red"
            visible: model.UnreadCountDisplay > 0
            Text {
                anchors.centerIn: parent
                color: palette.base
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
