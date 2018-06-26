import QtQuick 2.8
import QtQuick.Controls 2.3
import "../ChannelList.js" as ChannelList
import "../Channel.js" as Channel
import "."

import "../dialogs"
import "../cover"

Page {
    id: teamRoot

    property var slackClient: null
    property alias pageStack: pageStack
    property string teamId
    property string teamName

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
                pageStack.replace(channelComponent, {"channelId" : _lastChannel,
                                      "channel" : SlackClient.getChannel(teamRoot.teamId, _lastChannel) })
            }
        }

        onTestLoginFail: {
            //TODO: inform main to open login dialog
            //pageStack.push(Qt.resolvedUrl("pages/LoginPage.qml"))
        }
    }
    Component {
        id: channelComponent
        Channel {}
    }

    Row {
        anchors.fill: parent
        spacing: 0
        ChannelList {
            id: channelsList
            height: parent.height
            width: 200
        }

        StackView {
            id: pageStack
            height: parent.height
            width: parent.width - channelsList.width
            transform: Translate {
                x: SlackClient.isDevice ? channelList.item.position * width * 0.33 : 0
            }

            initialItem: Item { BusyIndicator { anchors.centerIn: parent } }
        }
    }

}
