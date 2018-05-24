import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."

import "../Settings.js" as Settings
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
        }
    }


    delegate: ItemDelegate {
        id: delegate
        text: model.name
        property color textColor: delegate.highlighted ? palette.highlightedText: palette.text
        highlighted: SlackClient.lastChannel === model.id

        icon.name: Channel.getIcon(model)
        width: listView.width

        onClicked: {
            SlackClient.setActiveWindow(model.id)

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
                            SlackClient.leaveChannel(model.id)
                            break

                        case "group":
                            var dialog = pageStack.push(Qt.resolvedUrl("GroupLeaveDialog.qml"), {"name": model.name})
                            dialog.accepted.connect(function() {
                                SlackClient.leaveGroup(model.id)
                            })
                            break

                        case "im":
                            SlackClient.closeChat(model.id)
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
