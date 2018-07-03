import QtQuick 2.8
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.4

import ".."

import "../ChannelList.js" as ChannelList
import "../Channel.js" as Channel

ListView {
    id: listView

    ScrollBar.vertical: ScrollBar { }

    function setIndex(ind) {
        currentIndex = ind
    }
    interactive: true

    model: ListModel {
        id: channelListModel
    }

    section {
        property: "section"
        criteria: ViewSection.FullString
        delegate: Label {
            text: getSectionName(section)
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
        text: model.name
        property color textColor: delegate.highlighted ? palette.highlightedText : palette.text
        highlighted: SlackClient.lastChannel(teamRoot.teamId) === model.id

        icon.name: Channel.getIcon(model)
        width: listView.width

        onClicked: {
            if (model.id === SlackClient.lastChannel(teamRoot.teamId)) {
                return
            }

            SlackClient.setActiveWindow(teamRoot.teamId, model.id)

            pageStack.replace(Qt.resolvedUrl("Channel.qml"), {"channelId": model.id})
        }

        property bool hasActions: model.category === "channel" || model.type === "im"
        onPressAndHold: if (hasActions) { actionsMenu.popup() }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: "RightButton"
            onClicked: if (hasActions) { actionsMenu.popup() }
        }

         Menu {
             id: actionsMenu

            MenuItem {
                text: model.category === "channel" ? qsTr("Leave") : qsTr("Close")
                onClicked: {
                    switch (model.type) {
                        case "channel":
                            SlackClient.leaveChannel(teamRoot.teamId, model.id)
                            break

                        case "group":
                            var dialog = pageStack.push(Qt.resolvedUrl("GroupLeaveDialog.qml"), {"name": model.name})
                            dialog.accepted.connect(function() {
                                SlackClient.leaveGroup(teamRoot.teamId, model.id)
                            })
                            break

                        case "im":
                            SlackClient.closeChat(teamRoot.teamId,  model.id)
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
            case "unreadchannel":
                return qsTr("Unread channels")

            case "unreadchat":
                return qsTr("Unread chats")

            case "channel":
                return qsTr("Channels")

            case "chat":
                return qsTr("Direct messages")
        }

        return qsTr("Other")
    }
}
