import QtQuick 2.8
import QtQuick.Controls 2.4
import ".."

Page {
    id: page

    property bool appActive: Qt.application.state === Qt.ApplicationActive

    title: teamRoot.teamName
    onAppActiveChanged: {
        SlackClient.setAppActive(teamRoot.teamId, appActive)
    }

    header: Rectangle {
        height: Theme.headerSize
        border.color: "#00050505"
        border.width: 1
        radius: 5
        Label {
            text: page.title
            anchors.centerIn: parent
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }
    }

    ChannelListView {
        id: listView
        anchors {
            fill: parent
        }
    }
}
