import QtQuick 2.2
import QtQuick.Controls 2.2
import ".."
import com.iskrembilen.slaq 1.0 as Slack

Item {
    id: page

    property bool appActive: Qt.application.state === Qt.ApplicationActive

    onAppActiveChanged: {
        Slack.Client.setAppActive(appActive)
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
