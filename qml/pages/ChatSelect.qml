import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import com.iskrembilen 1.0

import ".."

Drawer {
    id: page
    y: Theme.paddingMedium
    bottomMargin: Theme.paddingMedium

    edge: Qt.RightEdge
    width: 0.3 * window.width
    height: window.height - Theme.paddingMedium

    ColumnLayout {
        anchors.fill: parent
        ChannelListView {
            id: channelsListView
            channelAddMode: true
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
