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

    //title: slackClient

    onSlackClientChanged: {
        if (slackClient !== null) {
            console.log("slack client", slackClient)
            SlackClient.testLogin(teamRoot.teamId)
        }
    }

    Connections {
        target: SlackClient
        onInitSuccess: {
            if (teamRoot.teamId == teamId) {
                channelListPermanent.sourceComponent = channelsListComponent
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

    Component {
        id: channelsListComponent
        ChannelList {}
    }
    Component {
        id: loadingPage
        LoadingPage {}
    }

    StackView {
        id: pageStack
        anchors {
            top: parent.top
            left: channelListPermanent.active ? channelListPermanent.right : parent.left
            right: parent.right
            bottom: parent.bottom
        }

        transform: Translate {
            x: SlackClient.isDevice ? channelList.item.position * width * 0.33 : 0
        }
    }

    Loader {
        id: channelListPermanent
        width: active ? Math.min(parent.width * 0.33, 200) : 0
        height: window.height
        opacity: active ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 500 } }
    }
}
