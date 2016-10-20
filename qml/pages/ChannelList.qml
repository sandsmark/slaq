import QtQuick 2.2
import Sailfish.Silica 1.0
import harbour.slackfish 1.0 as Slack

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
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

        ChannelListView {
            id: listView
            anchors.fill: parent
        }
    }

    ConnectionPanel {}
}
