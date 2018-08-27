import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import com.iskrembilen 1.0

import ".."
import "../components"

Drawer {
    id: chatSelect

    property alias searchInput: searchInput
    y: Theme.paddingMedium
    bottomMargin: Theme.paddingMedium

    edge: Qt.RightEdge
    width: 0.3 * window.width
    height: window.height - Theme.paddingMedium
    property string state: "none"

    onClosed: state = "none"

    Component {
        id: channelsListComponent
        ChannelsSelectView {}
    }

    Component {
        id: usersListComponent
        UsersSelectView {}
    }

    ColumnLayout {
        anchors.fill: parent
        TextField {
            id: searchInput
            Layout.fillWidth: true
            Layout.margins: 5
            placeholderText: qsTr("Search...")
        }
        Loader {
            id: loader
            Layout.fillHeight: true
            Layout.fillWidth: true
            sourceComponent: {
                if (chatSelect.state === "none")
                    return undefined;
                else if (chatSelect.state === "channels")
                    return channelsListComponent;
                else if (chatSelect.state === "users")
                    return usersListComponent;
            }
        }
    }
}
