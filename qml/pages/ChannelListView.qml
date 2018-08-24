import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import ".."
import com.iskrembilen 1.0

ListView {
    id: listView

    property bool channelAddMode: false
    ScrollBar.vertical: ScrollBar { }

    function setIndex(ind) {
        currentIndex = ind
    }
    interactive: true
    clip: true

    Connections {
        target: SlackClient
        onChatsModelChanged: {
            if (teamId === teamRoot.teamId) {
                listView.model = chatsModel
            }
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
        highlighted: teamRoot.currentChannelId === model.Id
        visible: listView.channelAddMode ? (model.IsOpen ? false : true ) : (model.IsOpen ? true : false )
        enabled: visible
        height: visible ? delegate.implicitHeight : 0
        spacing: Theme.paddingSmall
        icon.color: "transparent"
        icon.source: model.PresenceIcon
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
            visible: model.UnreadCountDisplay > 0 && !channelAddMode
            Text {
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: parent.height/1.5
                text: model.UnreadCountDisplay
            }
        }

        width: listView.width

        onClicked: {
            if (channelAddMode) {
                if (model.Type === ChatsModel.Channel) {
                    SlackClient.joinChannel(teamRoot.teamId, model.Id)
                } else if (model.Type === ChatsModel.Conversation) {
                    SlackClient.openChat(teamRoot.teamId, model.Id)
                }
                teamRoot.chatSelect.close()
            } else {
                if (model.Id === SlackClient.lastChannel(teamRoot.teamId)) {
                    console.warn("Clicked on same channel")
                    return
                }
                SlackClient.setActiveWindow(teamRoot.teamId, model.Id)
                chatChangeTimer.channelId = model.Id
                chatChangeTimer.start()
            }
        }

        property bool hasActions: model.Type === ChatsModel.Channel || model.Type === ChatsModel.Conversation
        onPressAndHold: if (hasActions && !channelAddMode) { actionsMenu.popup() }

        MouseArea {
            anchors.fill: parent
            enabled: !channelAddMode
            acceptedButtons: "RightButton"
            onClicked: if (hasActions) { actionsMenu.popup() }
        }

        Menu {
            id: actionsMenu

            MenuItem {
                text: model.Type === ChatsModel.Channel ? qsTr("Leave") : qsTr("Close")
                onClicked: {
                    switch (model.Type) {
                    case ChatsModel.Channel:
                        SlackClient.leaveChannel(teamRoot.teamId, model.Id)
                        break

                    case ChatsModel.Group:
                    case ChatsModel.MultiUserConversation:
                        var dialog = pageStack.push(Qt.resolvedUrl("GroupLeaveDialog.qml"), {"name": model.Name })
                        dialog.accepted.connect(function() {
                            SlackClient.leaveGroup(teamRoot.teamId, model.Id)
                        })
                        break

                    case ChatsModel.Conversation:
                        SlackClient.closeChat(teamRoot.teamId,  model.Id)
                    }
                }
            }
        }
    }
}
