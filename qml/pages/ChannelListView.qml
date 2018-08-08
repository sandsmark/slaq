import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import ".."
import com.iskrembilen 1.0

ListView {
    id: listView

    ScrollBar.vertical: ScrollBar { }

    function setIndex(ind) {
        currentIndex = ind
    }
    interactive: true

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
                color: Material.primary
            }
        }
    }


    delegate: ItemDelegate {
        id: delegate
        text: model.Name
        property color textColor: delegate.highlighted ? palette.highlightedText : palette.text
        highlighted: teamRoot.currentChannelId === model.Id
        visible: model.IsOpen
        enabled: visible
        height: model.IsOpen ? delegate.implicitHeight : 0
        icon.name: {
            switch (model.Type) {
            case ChatsModel.Channel:
                if (model.presence === "active") {
                    return "irc-channel-active"
                } else {
                    return "irc-channel-inactive"
                }
            case ChatsModel.Group:
            case ChatsModel.MultiUserConversation:
                return "icon-s-secure"
            case ChatsModel.Conversation:
                if (model.IsOpen === "active") {
                    return "im-user"
                } else {
                    return "im-user-inactive"
                }
            default:
                console.log("unhandled type: " + model.Type)
                return ""
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
                return
            }

            SlackClient.setActiveWindow(teamRoot.teamId, model.Id)

            //            var channel = List
            //            console.log(Object.keys(channel))
            //            console.log(channel.Name)
            //            console.log(channel.Type)
            //            console.log(channel.Id)

            pageStack.replace(Qt.resolvedUrl("Channel.qml"), {"channelId": model.Id, "channelName": model.Name, "channelType": model.Type})
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
