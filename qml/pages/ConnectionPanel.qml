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
            id: busyIndicator
            anchors.verticalCenter: parent.verticalCenter
        }

        Button {
            id: reconnectButton
            text: qsTr("Reconnect")
            anchors.verticalCenter: parent.verticalCenter
            enabled: !SlackClient.isOnline
            onClicked: {
                SlackClient.reconnectClient()
            }
        }
    }

    Connections {
        target: SlackClient
        onConnected: {
            statusMessage.text = qsTr("Connected")
            connectionPanel.close()
            busyIndicator.running = false
            reconnectButton.enabled = false
        }

        onReconnecting: {
            statusMessage.text = qsTr("Reconnecting")
            connectionPanel.open()
            busyIndicator.running = true
            reconnectButton.enabled = false
        }

        onDisconnected: {
            statusMessage.text = qsTr("Disconnected")
            connectionPanel.open()
            busyIndicator.running = false
            reconnectButton.enabled = true
        }

        onNetworkOff: {
            statusMessage.text = qsTr("No network connection")
            connectionPanel.open()
            busyIndicator.running = false
            reconnectButton.enabled = false
        }
    }
}
