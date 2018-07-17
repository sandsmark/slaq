import QtQuick 2.8
import QtQuick.Controls 2.4

import "."
import ".."

import "../dialogs"
import "../cover"

Page {
    id: teamRoot

    property var slackClient: null
    property alias pageStack: pageStack
    property string teamId
    property string teamName
    property string previousChannelId

    onSlackClientChanged: {
        if (slackClient !== null) {
            console.log("slack client", slackClient)
            SlackClient.testLogin(teamRoot.teamId)
        }
    }

    function setCurrentTeam() {
        pageStack.currentItem.setChannelActive()
    }

    Connections {
        target: SlackClient
        onInitSuccess: {
            if (teamRoot.teamId == teamId) {
                var _lastChannel = SlackClient.lastChannel(teamRoot.teamId);
                console.log("loading page. adding channel component:", _lastChannel, teamRoot.teamId)
                previousChannelId = _lastChannel
                pageStack.replace(channelComponent, {"channelId" : _lastChannel })

            }
        }

        onTestLoginFail: {
            //TODO: inform main to open login dialog
            //pageStack.push(Qt.resolvedUrl("pages/LoginPage.qml"))
        }
        onChannelJoined: {
            if (teamRoot.teamId == teamId) {
                previousChannelId = pageStack.currentItem.channelId
                pageStack.replace(channelComponent, {"channelId" : channelId })
            }
        }
        onChannelLeft: {
            if (teamRoot.teamId == teamId) {
                pageStack.replace(channelComponent, {"channelId" : previousChannelId })
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
