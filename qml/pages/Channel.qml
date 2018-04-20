import QtQuick 2.0
import QtQuick.Controls 2.2
import ".."
import harbour.slackfish 1.0 as Slack

Page {
    id: page

    visible: channelId !== ""
    onChannelIdChanged: console.log("chan id: " + channelId)
    property string channelId
    property variant channel
    property bool initialized: false

    Flickable {
        anchors.fill: parent

        BusyIndicator {
            id: loader
            visible: true
            running: visible
            anchors.centerIn: parent
        }

        MessageListView {
            id: listView
            channel: page.channel
            anchors.fill: parent

            onLoadCompleted: {
                loader.visible = false
            }

            onLoadStarted: {
                loader.visible = true
            }

            Menu {
                id: bottomMenu

                MenuItem {
                    text: qsTr("Upload image")
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("FileSend.qml"), {"channelId": page.channelId})
                    }
                }
            }
        }
    }

    ConnectionPanel {}

    Component.onCompleted: {
        page.channel = Slack.Client.getChannel(page.channelId)
    }

    StackView.onStatusChanged: {
        if (StackView.status === StackView.Active) {
            Slack.Client.setActiveWindow(page.channelId)

            if (!initialized) {
                initialized = true
                listView.loadMessages()
            }
        } else {
            Slack.Client.setActiveWindow("")
            listView.markLatest()
        }
    }
}
