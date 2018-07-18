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

    header: Label {
        text: page.title
        verticalAlignment: "AlignVCenter"
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        height: Theme.headerSize
    }

    ChannelListView {
        id: listView
        anchors {
            fill: parent
        }
    }
}
