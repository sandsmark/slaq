import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.2
import com.iskrembilen 1.0

import "."
import ".."

import "../dialogs"
import "../cover"

Page {
    id: teamRoot

    property var slackClient: null
    property alias pageStack: pageStack
     property alias chatSelect: chatSelect
    property string teamId: ""
    property string teamName: ""
    property string currentChannelId: pageStack.currentItem.channel != undefined ? pageStack.currentItem.channel.id : ""
    property string previousChannelId

    onSlackClientChanged: {
        if (slackClient !== null) {
            console.log("starting test login for team", teamRoot.teamId, teamRoot.teamName)
            SlackClient.testLogin(teamRoot.teamId)
        }
    }

    function setCurrentTeam() {
        if (pageStack.currentItem.setChannelActive) {
            pageStack.currentItem.setChannelActive()
        }
    }

    function deleteMessage(channelId, ts) {
        deleteMessageDialog.channelId = channelId
        deleteMessageDialog.ts = ts
        deleteMessageDialog.open()
    }
    ChatSelect {
        id: chatSelect
    }

    MessageDialog {
        id: deleteMessageDialog
        property string channelId: ""
        property date ts
        icon: StandardIcon.Warning
        standardButtons: StandardButton.Yes | StandardButton.Cancel
        text: qsTr("You going to delete message. Are you sure?")
        onYes: {
            SlackClient.deleteMessage(teamId, channelId, ts)
        }
    }

    Connections {
        target: SlackClient
        onInitSuccess: {
            if (teamRoot.teamId == teamId) {
                var _lastChannelId = SlackClient.lastChannel(teamRoot.teamId);
                console.log("loading page. adding channel component:", _lastChannelId, teamRoot.teamId)
                var channel = SlackClient.getChannel(teamRoot.teamId, _lastChannelId);
                if (channel != null) {
                    pageStack.replace(channelComponent, {"channel" : channel })
                } else {
                    console.warn("No chat found for id", _lastChannelId)
                }

            }
        }

        onTestLoginFail: {
            //TODO: inform main to open login dialog
            //pageStack.push(Qt.resolvedUrl("pages/LoginPage.qml"))
        }
        onChannelJoined: {
            if (teamRoot.teamId == teamId) {
                var _chat = SlackClient.getChannel(teamRoot.teamId, channelId);
                if (_chat != null) {
                    pageStack.replace(channelComponent, {"channel" : _chat })
                }
            }
        }
        onChannelLeft: {
            if (teamRoot.teamId == teamId) {
                var _chat = SlackClient.getGeneralChannel(teamRoot.teamId);
                if (_chat != null) {
                    pageStack.replace(channelComponent, {"channel" : _chat })
                }
            }
        }
    }

    Component {
        id: channelComponent
        Channel {}
    }

    Row {
        anchors.fill: parent
        spacing: Theme.paddingMedium
        ChannelList {
            id: channelsList
            height: parent.height
            width: 200
        }

        StackView {
            id: pageStack
            height: parent.height
            width: parent.width - channelsList.width - Theme.paddingMedium
            transform: Translate {
                x: SlackClient.isDevice ? channelList.item.position * width * 0.33 : 0
            }
            initialItem: Item { BusyIndicator { anchors.centerIn: parent } }
        }
    }
}
