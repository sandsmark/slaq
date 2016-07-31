import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.slackfish 1.0 as Slack

SilicaListView {
    property alias atBottom: listView.atYEnd
    property variant channel

    property bool appActive: Qt.application.active
    property bool inputEnabled: false
    property string latestRead: ""

    signal loadCompleted()
    signal loadStarted()

    id: listView
    anchors.fill: parent
    spacing: Theme.paddingLarge

    VerticalScrollDecorator {}

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

    header: PageHeader {
        title: channel.name
    }

    model: ListModel {
        id: messageListModel
    }

    delegate: MessageListItem {}

    section {
        property: "day"
        criteria: ViewSection.FullString
        delegate: SectionHeader {
            text: section
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
        inputEnabled = false
        loadStarted()
        loadMessages()
    }

    function loadMessages() {
        Slack.Client.loadMessages(channel.type, channel.id)
    }

    function handleLoadSuccess(channelId, messages) {
        if (channelId === channel.id) {
            messages.sort(function(a, b) {
                return a.time - b.time
            })
            messageListModel.clear()
            messages.forEach(function(message) {
                message.day = getMessageDay(message)
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
        if (message.type === "message" && message.channel === channel.id) {
            var isAtBottom = atBottom
            message.day = getMessageDay(message)
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

    function getMessageDay(message) {
        return new Date(parseInt(message.time, 10) * 1000).toLocaleString(Qt.locale(), "MMMM d, yyyy")
    }

    /*
    onMovementEnded: {
        var index = -1;

        if (atBottom) {
            index = messageListModel.count - 1
            console.log("Botton", index)
        }
        else {
            index = listView.indexAt(contentX, contentY + height - footerItem.height)
            console.log("First", index)

            if (index === -1) {
                index = listView.indexAt(contentX, contentY + height - footerItem.height + 10)
                console.log("Second", index)
            }

            if (index === -1) {
                index = listView.indexAt(contentX, contentY + height - footerItem.height + 5)
                console.log("Third", index)
            }
        }

        if (index >= 0) {
            console.log("Move end", index)
            latestRead = messageListModel.get(index).time
            readTimer.start()
        }
    }
    */
}


