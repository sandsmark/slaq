import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import com.iskrembilen 1.0
import ".."

ListView {
    id: msgListView

    clip: true

    spacing: Theme.paddingSmall
    verticalLayoutDirection: ListView.BottomToTop

    signal loadCompleted()
    signal loadStarted()
    property bool atBottom: chanScrollView.atYEnd

    property bool appActive: Qt.application.state === Qt.ApplicationActive
    property bool inputEnabled: false
    property bool isReplies: false
    property var latestRead

    signal messageHeightChanged()

    Timer {
        id: heightChangedTimer
        interval: 100
        onTriggered: messageHeightChanged()
        repeat: false
    }

    onAppActiveChanged: {
        markLatest()
    }
    function markLatest() {
        readTimer.restart()
    }

    function doMarkLatest() {
        if (channelRoot.channel == null)
            return
        if (appActive && atBottom
                && teamId === SlackClient.lastTeam
                && SlackClient.lastChannel(teamRoot.teamId) === channelRoot.channel.id) {
            SlackClient.markChannel(teamRoot.teamId, channelRoot.channel.type, channelRoot.channel.id)
        }
    }

    function loadMessages() {
        console.log("load messages start", channel.name, teamRoot.teamId, channelRoot.channel.type)
        SlackClient.loadMessages(teamRoot.teamId, channelRoot.channel.id)
    }

    Timer {
        id: readTimer
        interval: 1000
        triggeredOnStart: false
        running: false
        repeat: false
        onTriggered: {
            doMarkLatest()
        }
    }

    Connections {
        target: SlackClient
        enabled: !isReplies
        onLoadMessagesSuccess: {
            console.log("load messages success", channel.name, teamId, teamRoot.teamId)
            if (teamId === teamRoot.teamId) {
                msgListView.model = teamRoot.slackClient.currentChatsModel().messages(channelId)
                inputEnabled = true
                loadCompleted()
                markLatest()
            }
        }
        onLoadMessagesFail: {
            if (teamId === teamRoot.teamId) {
                console.warn("Load messages fail")
                loadCompleted()
            }
        }
    }

    delegate: MessageListItem {
        width: chanScrollView.width - chanScrollView.ScrollBar.vertical.width - Theme.paddingSmall
        isReplies: msgListView.isReplies
    }

    section {
        property: "Time"
        criteria: ViewSection.FullString
        labelPositioning: ViewSection.CurrentLabelAtStart
        delegate: Label {
            text: Qt.formatDate(section, "MMM dd, yyyy")
            width: chanScrollView.width - chanScrollView.ScrollBar.vertical.width
            horizontalAlignment: "AlignHCenter"
            Component.onCompleted: { reViewHeight = reViewHeight + height }
        }
    }

    onCountChanged: {
        if (atBottom && count > 0) {
            markLatest()
        }
    }
}
