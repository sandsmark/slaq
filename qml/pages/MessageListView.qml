import QtQuick 2.8
import QtQuick.Controls 2.3
import com.iskrembilen 1.0
import ".."
import "../Message.js" as Message

ListView {
    id: listView

    property alias atBottom: listView.atYEnd
    //property variant channel

    property bool appActive: Qt.application.state === Qt.ApplicationActive
    property bool inputEnabled: false
    property var latestRead

    signal loadCompleted()
    signal loadStarted()
    clip: true

    spacing: Theme.paddingMedium
    verticalLayoutDirection: ListView.BottomToTop

    ScrollBar.vertical: ScrollBar { }

    Timer {
        id: readTimer
        interval: 500
        triggeredOnStart: false
        running: false
        repeat: false
        onTriggered: {
            markLatest()
        }
    }

    Connections {
        target: SlackClient
        onLoadMessagesSuccess: {
            if (teamId === teamRoot.teamId) {
                listView.model = teamRoot.slackClient.currentChatsModel().messages(channelId)
                SlackClient.markChannel(teamRoot.teamId, channelRoot.channelType, channelRoot.channelId)
                inputEnabled = true
                loadCompleted()
            }
        }
    }

    delegate: MessageListItem {
        width: listView.width - listView.ScrollBar.vertical.width
    }

    section {
        property: "Time"
        criteria: ViewSection.FullString
        labelPositioning: ViewSection.CurrentLabelAtStart
        delegate: Label {
            text: Qt.formatDate(section, "MMM dd, yyyy")
            width: listView.width
            horizontalAlignment: "AlignHCenter"
        }
    }

    onAppActiveChanged: {
        readTimer.restart()
    }

    onMovementEnded: {
        if (atBottom) {
            readTimer.restart()
        }
    }

    function markLatest() {
        if (appActive && atBottom
                && teamId === SlackClient.lastTeam
                && SlackClient.lastChannel(teamRoot.teamId) === channelRoot.channelId) {
            SlackClient.markChannel(teamRoot.teamId, channelRoot.channelType, channelRoot.channelId)
        }
    }

    function loadMessages() {
        SlackClient.loadMessages(teamRoot.teamId, channelRoot.channelType, channelRoot.channelId)
    }
}
