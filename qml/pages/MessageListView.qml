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

    //TODO: move messages model to C++
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
        SlackClient.onMessageUpdated.connect(handleMessageUpdated)
    }

    Component.onDestruction: {
        SlackClient.onInitSuccess.disconnect(handleReload)
        SlackClient.onLoadMessagesSuccess.disconnect(handleLoadSuccess)
        SlackClient.onMessageReceived.disconnect(handleMessageReceived)
        SlackClient.onMessageUpdated.disconnect(handleMessageUpdated)
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

    function handleMessageUpdated(message) {
        if (message.channel === channel.id) {
            for (var msgi = 0; msgi < messageListModel.count; msgi++) {
                var msg = messageListModel.get(msgi);
                if (msg.time === message.ts) {
                    var updated = false;
                    var reactions = msg.reactions;
                    var reaction = { "emoji": message.emoji, "reactionscount": 1,
                        "name": message.reaction, "users": message.user }
                    for (var rctni = 0; rctni < reactions.count; rctni++) {
                        var rcn = reactions.get(rctni);
                        if (rcn.name === message.reaction) {
                            if (message.type === "reaction_removed") {
                                reaction.reactionscount = rcn.reactionscount - 1
                            } else {
                                reaction.reactionscount = rcn.reactionscount + 1
                            }
                            if (reaction.reactionscount <= 0) {
                                reactions.remove(rctni, 1);
                            } else {
                                reactions.set(rctni, reaction);
                            }
                            updated = true;
                            break;
                        }
                    }
                    if (updated === false) {
                        reactions.append(reaction)
                    }
                    messageListModel.set(msgi, {"reactions": reactions});
                    break;
                }
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
