import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import com.iskrembilen 1.0
import ".."
import "../components"

ListView {
    id: msgListView

    clip: true

    spacing: Theme.paddingSmall
    verticalLayoutDirection: ListView.BottomToTop

    signal loadCompleted()
    signal loadStarted()
    property alias atBottom: msgListView.atYEnd

    property bool appActive: Qt.application.state === Qt.ApplicationActive
    property bool inputEnabled: false
    property bool isReplies: false
    property var latestRead
    property var messageInput

    property int lastCount: 0

    onAppActiveChanged: {
        markLatest()
    }

    function markLatest() {
        readTimer.restart()
    }

    function doMarkLatest() {
        if (channelRoot.channel == null)
            return
        if (appActive && teamId === SlackClient.lastTeam
                && SlackClient.lastChannel(teamRoot.teamId) === channelRoot.channel.id
                && count > 0) {
            if (atBottom) {
                currentIndex = 0
                console.log("mark latest", count, channelRoot.channel.name, atBottom)
                SlackClient.markChannel(teamRoot.teamId, channelRoot.channel.type, channelRoot.channel.id)
            }
        } else {
            if (count != lastCount) {
                positionViewAtIndex(currentIndex, ListView.Beginning)
                lastCount = count
                console.log("position", currentIndex, atBottom, msgListView.atYBeginning)
            }
        }
    }

    function loadMessages() {
        console.log("load messages start", channel.name, teamRoot.teamId, channelRoot.channel.type)
        SlackClient.loadMessages(teamRoot.teamId, channelRoot.channel.id)
    }

    ScrollBar.vertical: ScrollBar {
        id: sbar
        policy: ScrollBar.AlwaysOn; minimumSize: 0.05
        anchors.right: parent.right
        //moved to left until https://codereview.qt-project.org/#/c/244603/ is merged
        anchors.rightMargin: 10

        contentItem: Rectangle {
            implicitWidth: 6
            implicitHeight: 100
            radius: width / 2
            color: sbar.pressed ? sbar.palette.dark : sbar.palette.mid
        }
        EmojiButton {
            background: Item {}
            width: parent.width
            font.bold: true
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -(height - contentItem.paintedHeight)/2
            text: "⟱"
            visible: !atBottom
            onClicked: {
                msgListView.positionViewAtIndex(0, ListView.Beginning)
                markLatest()
            }
        }
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
        width: msgListView.width - msgListView.ScrollBar.vertical.width - Theme.paddingSmall
        isReplies: msgListView.isReplies
        messageInput: msgListView.messageInput
    }

    section {
        property: "Time"
        criteria: ViewSection.FullString
        labelPositioning: ViewSection.CurrentLabelAtStart
        delegate: Label {
            text: Qt.formatDate(section, "MMM dd, yyyy")
            width: msgListView.width - msgListView.ScrollBar.vertical.width
            horizontalAlignment: "AlignHCenter"
        }
    }

    onCountChanged: {
        if (atBottom && count > 0) {
            markLatest()
        }
    }

    onMovementEnded: {
        if (atBottom) {
            markLatest()
        }
    }
}
