import QtQuick 2.2
import Sailfish.Silica 1.0
import harbour.slackfish 1.0 as Slack

Page {
    id: page

    property bool shouldReload: false

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
        }

        ChannelListView {
            id: listView
            anchors.fill: parent
        }
    }

    ConnectionPanel {}

    onStatusChanged: {
        if (status === PageStatus.Active && shouldReload) {
            listView.reload()
        }
    }

    Component.onCompleted: {
        Slack.Client.onInitSuccess.connect(handleReload)
    }

    Component.onDestruction: {
        Slack.Client.onInitSuccess.disconnect(handleReload)
    }

    function handleReload() {
        if (status === PageStatus.Active) {
            listView.reload()
        } else {
            shouldReload = true
        }
    }
}
