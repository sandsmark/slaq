import QtQuick 2.11
import QtQuick.Controls 2.4
import com.iskrembilen 1.0
import ".."

ListView {
    id: listView

    property alias atBottom: listView.atYEnd

    property bool appActive: Qt.application.state === Qt.ApplicationActive
    property bool inputEnabled: false
    property var latestRead

    signal loadCompleted()
    signal loadStarted()
    clip: true

    spacing: Theme.paddingSmall
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
            console.log("load messages success", channelName, teamId, teamRoot.teamId)
            if (teamId === teamRoot.teamId) {
                listView.model = teamRoot.slackClient.currentChatsModel().messages(channelId)
                SlackClient.markChannel(teamRoot.teamId, channelRoot.channelType, channelRoot.channelId)
                inputEnabled = true
                loadCompleted()
            }
        }
        onLoadMessagesFail: {
            if (teamId === teamRoot.teamId) {
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

    onCountChanged: {
        if (atBottom && count > 0) {
            readTimer.restart()
        }
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
        console.log("load messages start", channelName, teamRoot.teamId, channelRoot.channelType)
        SlackClient.loadMessages(teamRoot.teamId, channelRoot.channelId)
    }
}
