import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."
import "../Settings.js" as Settings

Page {
    id: page

    property bool appActive: Qt.application.state === Qt.ApplicationActive

    title: Settings.getUserInfo().teamName
    onAppActiveChanged: {
        SlackClient.setAppActive(appActive)
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
            margins: Theme.paddingMedium
        }
    }
}
