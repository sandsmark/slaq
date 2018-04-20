import QtQuick 2.2
import QtQuick.Controls 2.2
import ".."
import harbour.slackfish 1.0 as Slack

Page {
    id: page

    property bool appActive: Qt.application.state === Qt.ApplicationActive

    onAppActiveChanged: {
        Slack.Client.setAppActive(appActive)
    }

    header: ToolBar {
        id: topMenu

        MenuItem {
            text: qsTr("About")
            onClicked: {
                pageStack.push(Qt.resolvedUrl("About.qml"))
            }
        }
        MenuItem {
            text: qsTr("Open chat")
            onClicked: {
                pageStack.push(Qt.resolvedUrl("ChatSelect.qml"))
            }
        }
        MenuItem {
            text: qsTr("Join channel")
            onClicked: {
                pageStack.push(Qt.resolvedUrl("ChannelSelect.qml"))
            }
        }
    }

    Flickable {
        anchors.fill: parent

        ChannelListView {
            id: listView
            anchors.fill: parent
        }
    }

    ConnectionPanel {}
}
