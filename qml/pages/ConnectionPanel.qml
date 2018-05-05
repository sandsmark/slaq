import QtQuick 2.8
import QtQuick.Controls 2.3
import QtQuick.Window 2.3
import ".."

Drawer {
    id: connectionPanel
    width: parent.width
    height: content.height + Theme.paddingLarge

    edge: Qt.BottomEdge

    Column {
        id: content
        width: parent.width - Theme.paddingLarge * (Screen.devicePixelRatio >= 90 ? 4 : 2)
        anchors.centerIn: parent
        spacing: Theme.paddingMedium

        Row {
            id: reconnectingMessage
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingMedium

            BusyIndicator {
                running: reconnectingMessage.visible
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                text: qsTr("Reconnecting")
            }
        }

        Label {
            id: disconnectedMessage
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Disconnected")
        }

        Button {
            id: reconnectButton
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Reconnect")
            onClicked: {
                SlackClient.reconnect()
            }
        }
    }

    Component.onCompleted: {
        SlackClient.onConnected.connect(hideConnectionPanel)
        SlackClient.onReconnecting.connect(showReconnectingMessage)
        SlackClient.onDisconnected.connect(showDisconnectedMessage)
        SlackClient.onNetworkOff.connect(showNoNetworkMessage)
        SlackClient.onNetworkOn.connect(hideConnectionPanel)
    }

    Component.onDestruction: {
        SlackClient.onConnected.disconnect(hideConnectionPanel)
        SlackClient.onReconnecting.disconnect(showReconnectingMessage)
        SlackClient.onDisconnected.disconnect(showDisconnectedMessage)
        SlackClient.onNetworkOff.disconnect(showNoNetworkMessage)
        SlackClient.onNetworkOn.disconnect(hideConnectionPanel)
    }

    function hideConnectionPanel() {
        connectionPanel.close()
    }

    function showReconnectingMessage() {
        disconnectedMessage.visible = false
        reconnectButton.visible = false
        reconnectingMessage.visible = true
        connectionPanel.show()
    }

    function showDisconnectedMessage() {
        disconnectedMessage.text = qsTr("Disconnected")
        disconnectedMessage.visible = true
        reconnectButton.visible = true
        reconnectingMessage.visible = false
        connectionPanel.show()
    }

    function showNoNetworkMessage() {
        disconnectedMessage.text = qsTr("No network connection")
        disconnectedMessage.visible = true
        reconnectButton.visible = false
        reconnectingMessage.visible = false
        connectionPanel.show()
    }
}
