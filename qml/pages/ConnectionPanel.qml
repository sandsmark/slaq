import QtQuick 2.8
import QtQuick.Controls 2.4
import QtQuick.Window 2.3
import ".."

Drawer {
    id: connectionPanel
    width: parent.width
    height: content.height + Theme.paddingLarge

    edge: Qt.TopEdge

    Row {
        id: content
        width: parent.width - Theme.paddingLarge * (Screen.devicePixelRatio >= 90 ? 4 : 2)
        anchors.centerIn: parent
        spacing: Theme.paddingMedium

        Label {
            id: statusMessage
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Disconnected")
        }

        BusyIndicator {
            running: !SlackClient.isOnline
            anchors.verticalCenter: parent.verticalCenter
        }

        Button {
            id: reconnectButton
            text: qsTr("Reconnect")
            anchors.verticalCenter: parent.verticalCenter
            enabled: !SlackClient.isOnline
            visible: enabled
            onClicked: {
                SlackClient.reconnect()
            }
        }
    }

    Connections {
        target: SlackClient
        onConnected: {
            statusMessage.text = qsTr("Connected")
            connectionPanel.close()
        }

        onReconnecting: {
            statusMessage.text = qsTr("Reconnecting")
            connectionPanel.open()
        }

        onDisconnected: {
            statusMessage.text = qsTr("Disconnected")
            connectionPanel.open()

        }

        onNetworkOff: {
            statusMessage.text = qsTr("No network connection")
            connectionPanel.open()
        }
    }
}
