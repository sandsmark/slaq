import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
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
    property alias loaderIndicator: loaderIndicator
    property string teamId: ""
    property string teamName: ""
    property string currentChannelId: pageStack.item != null && pageStack.item.channel != undefined ? pageStack.item.channel.id : ""
    property string previousChannelId
    property bool loadingData: false

    onSlackClientChanged: {
        if (slackClient !== null) {
            console.log("starting test login for team", teamRoot.teamId, teamRoot.teamName)
            loadingData = true
            SlackClient.testLogin(teamRoot.teamId)
        }
    }

    function setCurrentTeam() {
        if (pageStack.item != null && pageStack.item.setChannelActive) {
            pageStack.item.setChannelActive()
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
                pageStack.loadChannel(channel)
                loadingData = false
            }
        }

        onTestLoginFail: {
            //TODO: inform main to open login dialog
            //pageStack.push(Qt.resolvedUrl("pages/LoginPage.qml"))
        }
        onChannelJoined: {
            if (teamRoot.teamId == teamId) {
                var _chat = SlackClient.getChannel(teamRoot.teamId, channelId);
                pageStack.loadChannel(_chat)
            }
        }
        onChannelLeft: {
            if (teamRoot.teamId == teamId) {
                var _chat = SlackClient.getGeneralChannel(teamRoot.teamId);
                pageStack.loadChannel(_chat)
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: Theme.paddingMedium
        ChannelList {
            id: channelsList
            Layout.fillHeight: true
            Layout.minimumWidth: 200
        }

        Loader {
            id: pageStack
            Layout.fillHeight: true
            Layout.fillWidth: true
            active: false
            asynchronous: true
            sourceComponent: Channel {}
            property Chat _channel: null

            function loadChannel(channel) {
                if (channel == null) {
                    console.warn("Passed NULL channel")
                    return
                }

                console.warn("channel loader start for", channel.name)
                pageStack.active = false
                _channel = channel
                pageStack.active = true
            }

            BusyIndicator {
                id: loaderIndicator
                visible: pageStack.status !== Loader.Ready || teamRoot.loadingData
                running: visible
                anchors.centerIn: parent
            }
            onLoaded: {
                console.log("loaded", item.objectName)
                item.channel = _channel
                console.timeEnd("start_switching")
                console.warn("Loaded", item.channel.name)
            }
        }
    }
}
