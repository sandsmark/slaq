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
                listView.model = teamRoot.slackClient.currentChatsModel()
            }
        }
    }

    onModelChanged: console.log("channels list model", model)

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

            color: Material.background
        }
    }


    delegate: ItemDelegate {
        id: delegate
        text: model.Type === ChatsModel.Conversation ? model.UserObject.fullName : model.Name
        property color textColor: delegate.highlighted ? palette.highlightedText : palette.text
        highlighted: SlackClient.lastChannel(teamRoot.teamId) === model.Id
        visible: model.IsOpen
        enabled: visible
        height: model.IsOpen ? delegate.implicitHeight : 0
//        icon.name: Channel.getIcon(model)
        icon.name: {
            switch (model.Type) {
            case ChatsModel.Channel:
                if (model.presence === "active") {
                    return "irc-channel-active"
                } else {
                    return "irc-channel-inactive"
                }
            case ChatsModel.Group:
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

                        case "group":
                            var dialog = pageStack.push(Qt.resolvedUrl("GroupLeaveDialog.qml"), {"name": model.Name })
                            dialog.accepted.connect(function() {
                                SlackClient.leaveGroup(teamRoot.teamId, model.Id)
                            })
                            break

                        case "im":
                            SlackClient.closeChat(teamRoot.teamId,  model.Id)
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        console.log("channel list view component completed", teamRoot.teamId)
        //ChannelList.init()
    }

    Component.onDestruction: {
        //ChannelList.disconnect()
    }
}
