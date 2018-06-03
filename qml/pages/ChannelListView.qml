import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."
import com.iskrembilen 1.0

import "../ChannelList.js" as ChannelList
import "../Channel.js" as Channel

ListView {
    id: listView

    ScrollBar.vertical: ScrollBar { }

    function setIndex(ind) {
        currentIndex = ind
    }
    interactive: true
    model: SlackClient.currentChatsModel

    ListModel {
        id: channelListModel
    }

    section {
        property: "section"
        criteria: ViewSection.FullString
        delegate: Label {
            text: getSectionName(section)
        }
    }


    delegate: ItemDelegate {
        id: delegate
        text: model.Type === ChatsModel.Conversation ? model.UserObject.fullName : model.Name
        property color textColor: delegate.highlighted ? palette.highlightedText: palette.text
        highlighted: SlackClient.lastChannel(teamRoot.teamId) === model.Id

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

            pageStack.replace(Qt.resolvedUrl("Channel.qml"), {"channelId": model.Id})
        }

        property bool hasActions: model.Type === ChatsModel.Channel || model.type === ChatsModel.Conversation
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
                            var dialog = pageStack.push(Qt.resolvedUrl("GroupLeaveDialog.qml"), {"name": model.Name})
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
        console.log("channel list view component completed",SlackClient.getChannels(teamRoot.teamId).length, teamRoot.teamId)
        ChannelList.init()
    }

    Component.onDestruction: {
        ChannelList.disconnect()
    }

    function getSectionName(section) {
        switch (section) {
            case "unread":
                return qsTr("Unreads")

            case "channel":
                return qsTr("Channels")

            case "chat":
                return qsTr("Direct messages")
        }
    }
}
