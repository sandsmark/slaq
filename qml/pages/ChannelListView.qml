import QtQuick 2.10
import com.iskrembilen.slaq 1.0 as Slack
import QtQuick.Controls 2.3
import ".."

import "../Settings.js" as Settings
import "../ChannelList.js" as ChannelList
import "../Channel.js" as Channel

ListView {
    id: listView

    ScrollBar.vertical: ScrollBar { }

    model: ListModel {
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
        text: model.name
        property color textColor: delegate.highlighted ? palette.base : palette.base
        highlighted: model.unreadCount > 0

        icon.name: Channel.getIcon(model)

        onClicked: {
            pageStack.push(Qt.resolvedUrl("Channel.qml"), {"channelId": model.id})
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
                            Slack.Client.leaveChannel(model.id)
                            break

                        case "group":
                            var dialog = pageStack.push(Qt.resolvedUrl("GroupLeaveDialog.qml"), {"name": model.name})
                            dialog.accepted.connect(function() {
                                Slack.Client.leaveGroup(model.id)
                            })
                            break

                        case "im":
                            Slack.Client.closeChat(model.id)
                    }
                }
            }
        }
    }

    Component.onCompleted: {
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
