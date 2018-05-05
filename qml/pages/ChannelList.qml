import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."

Rectangle {
    id: page

    color: palette.window

    property bool appActive: Qt.application.state === Qt.ApplicationActive

    onAppActiveChanged: {
        SlackClient.setAppActive(appActive)
    }

//    header: ToolBar {
//        id: topMenu

//    }

    ChannelListView {
        id: listView
        anchors {
            fill: parent
            margins: Theme.paddingMedium
        }
    }


    ConnectionPanel {}
}
