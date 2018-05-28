import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."

Item {
    property int unreadMessageCount: 0
    property color unreadColor: unreadMessageCount === 0 ? palette.text : palette.highlightedText

    Label {
        id: title
        width: parent.width - Theme.paddingLarge * 2
        text: "Slaq"
//        truncationMode: TruncationMode.Fade
        anchors {
            top: parent.top
            left: parent.left
            topMargin: Theme.paddingLarge
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
        }
    }

    Label {
        id: messageCount
        text: unreadMessageCount
        color: unreadColor
        font.pointSize: Theme.fontSizeHuge
        anchors {
            top: title.bottom
            left: parent.left
            topMargin: Theme.paddingMedium
            leftMargin: Theme.paddingLarge
        }
    }

    Label {
        text: "Unread\nmessage" + (unreadMessageCount === 1 ? "" : "s")
        font.pointSize: Theme.fontSizeTiny
        color: unreadColor
        anchors {
            left: messageCount.right
            leftMargin: Theme.paddingSmall
            verticalCenter: messageCount.verticalCenter
        }
    }

    Label {
        id: connectionMessage
        font.pointSize: Theme.fontSizeSmall
        text: ""
        visible: text.length > 0
        width: parent.width - Theme.paddingLarge * 2
        anchors {
            top: messageCount.bottom
            left: parent.left
            topMargin: Theme.paddingLarge
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
        }
    }

//    CoverActionList {
//        id: disconnectedActions
//        enabled: false

//        CoverAction {
//            iconSource: "image://theme/icon-cover-refresh"
//            onTriggered: SlackClient.reconnect()
//        }
//    }

    Component.onCompleted: {
        SlackClient.onInitSuccess.connect(reloadChannelList)
        SlackClient.onChannelUpdated.connect(reloadChannelList)
        SlackClient.onConnected.connect(hideConnectionMessage)
        SlackClient.onReconnecting.connect(showReconnectingMessage)
        SlackClient.onDisconnected.connect(showDisconnectedMessage)
        SlackClient.onNetworkOff.connect(showNoNetworkMessage)
        SlackClient.onNetworkOn.connect(hideNetworkMessage)
    }

    function hideConnectionMessage() {
        connectionMessage.text = ""
        disconnectedActions.enabled = false
    }

    function showReconnectingMessage() {
        connectionMessage.text = qsTr("Reconnecting")
        disconnectedActions.enabled = false
    }

    function showDisconnectedMessage() {
        connectionMessage.text = qsTr("Disconnected")
        disconnectedActions.enabled = true
    }

    function showNoNetworkMessage() {
        connectionMessage.text = qsTr("No network connection")
        disconnectedActions.enabled = false
    }

    function hideNetworkMessage() {
        connectionMessage.text = ""
        disconnectedActions.enabled = false
    }

    function reloadChannelList() {
        title.text = SlackConfig.teamName()
        unreadMessageCount = 0

        SlackClient.getChannels().forEach(function(channel) {
            if (channel.isOpen && channel.unreadCount > 0) {
                unreadMessageCount += channel.unreadCount
            }
        })
    }
}
