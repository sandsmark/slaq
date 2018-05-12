import QtQuick 2.2
import QtQuick.Controls 2.2
import ".."
import com.iskrembilen.slaq 1.0 as Slack
import "../Settings.js" as Settings

Page {
    id: page

    property bool appActive: Qt.application.state === Qt.ApplicationActive

    title: Settings.getUserInfo().teamName
    onAppActiveChanged: {
        Slack.Client.setAppActive(appActive)
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
