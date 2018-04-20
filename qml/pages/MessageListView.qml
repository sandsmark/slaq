import QtQuick 2.10
import QtQuick.Controls 2.2
import ".."
import harbour.slackfish 1.0 as Slack
import "../Message.js" as Message

ListView {
    property alias atBottom: listView.atYEnd
    property variant channel

    property bool appActive: Qt.application.state === Qt.ApplicationActive
    property bool inputEnabled: false
    property string latestRead: ""

    signal loadCompleted()
    signal loadStarted()

    id: listView
    anchors.fill: parent
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

    header: Label {
        text: channel.name
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
            background: Rectangle { color: SystemPalette.alternateBase }
        }
    }

    footer: MessageInput {
        visible: inputEnabled
        placeholder: qsTr("Message %1%2").arg("#").arg(channel.name)
        onSendMessage: {
            Slack.Client.postMessage(channel.id, content)
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
        Slack.Client.onInitSuccess.connect(handleReload)
        Slack.Client.onLoadMessagesSuccess.connect(handleLoadSuccess)
        Slack.Client.onMessageReceived.connect(handleMessageReceived)
    }

    Component.onDestruction: {
        Slack.Client.onInitSuccess.disconnect(handleReload)
        Slack.Client.onLoadMessagesSuccess.disconnect(handleLoadSuccess)
        Slack.Client.onMessageReceived.disconnect(handleMessageReceived)
    }

    function markLatest() {
        if (latestRead != "") {
            Slack.Client.markChannel(channel.type, channel.id, latestRead)
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
        Slack.Client.loadMessages(channel.type, channel.id)
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
