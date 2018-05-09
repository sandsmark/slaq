import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import ".."
import com.iskrembilen.slaq 1.0 as Slack

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
            running: connectionPanel.opened && !Slack.Client.isOnline
            anchors.verticalCenter: parent.verticalCenter
        }

        Button {
            id: reconnectButton
            text: qsTr("Reconnect")
            anchors.verticalCenter: parent.verticalCenter
            onClicked: {
                Slack.Client.reconnect()
            }
        }
    }

    Connections {
        target: Slack.Client
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
