import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."
import "../Message.js" as Message

ListView {
    id: listView

    property alias atBottom: listView.atYEnd
    property variant channel

    property bool appActive: Qt.application.state === Qt.ApplicationActive
    property bool inputEnabled: false
    property string latestRead: ""

    signal loadCompleted()
    signal loadStarted()
    clip: true

    spacing: Theme.paddingLarge

    ScrollBar.vertical: ScrollBar { }

    Timer {
        id: readTimer
        interval: 5000
        triggeredOnStart: false
        running: false
        repeat: false
        onTriggered: {
            markLatest()
        }
    }

    model: ListModel {
        id: messageListModel
    }

    delegate: MessageListItem {}

    section {
        property: "day"
        criteria: ViewSection.FullString
        delegate: Label {
            text: section
            width: listView.width
            horizontalAlignment: "AlignHCenter"
        }
    }

    onAppActiveChanged: {
        if (appActive && atBottom && messageListModel.count) {
            latestRead = messageListModel.get(messageListModel.count - 1).time
            readTimer.restart()
        }
    }

    onMovementEnded: {
        if (atBottom && messageListModel.count) {
            latestRead = messageListModel.get(messageListModel.count - 1).time
            readTimer.restart()
        }
    }

    Component.onCompleted: {
        SlackClient.onInitSuccess.connect(handleReload)
        SlackClient.onLoadMessagesSuccess.connect(handleLoadSuccess)
        SlackClient.onMessageReceived.connect(handleMessageReceived)
    }

    Component.onDestruction: {
        SlackClient.onInitSuccess.disconnect(handleReload)
        SlackClient.onLoadMessagesSuccess.disconnect(handleLoadSuccess)
        SlackClient.onMessageReceived.disconnect(handleMessageReceived)
    }

    function markLatest() {
        if (latestRead != "") {
            SlackClient.markChannel(channel.type, channel.id, latestRead)
            latestRead = ""
        }
    }

    function handleReload() {
        console.log("Handling reloading")
        inputEnabled = false
        loadStarted()
        loadMessages()
    }

    function loadMessages() {
        console.log("loading messages")
        SlackClient.loadMessages(channel.type, channel.id)
    }

    function handleLoadSuccess(channelId, messages) {
        console.log("loading success")
        if (channelId === channel.id) {
            messages.sort(Message.compareByTime)
            messageListModel.clear()
            messages.forEach(function(message) {
                message.day = Message.getDisplayDate(message)
                messageListModel.append(message)
            })
            listView.positionViewAtEnd()
            inputEnabled = true
            loadCompleted()

            if (messageListModel.count) {
                latestRead = messageListModel.get(messageListModel.count - 1).time
                readTimer.restart()
            }
        }
    }

    function handleMessageReceived(message) {
        console.log("message received")
        if (message.type === "message" && message.channel === channel.id) {
            var isAtBottom = atBottom
            message.day = Message.getDisplayDate(message)
            messageListModel.append(message)

            if (isAtBottom) {
                listView.positionViewAtEnd()

                if (appActive) {
                    latestRead = message.time
                    readTimer.restart()
                }
            }
        }
    }
}
