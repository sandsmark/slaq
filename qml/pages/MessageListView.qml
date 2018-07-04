import QtQuick 2.8
import QtQuick.Controls 2.3
import com.iskrembilen 1.0
import ".."
import "../Message.js" as Message

ListView {
    id: listView

    property alias atBottom: listView.atYEnd
    property variant channel
    property int channelType

    property bool appActive: Qt.application.state === Qt.ApplicationActive
    property bool inputEnabled: false
    property string latestRead: ""

    signal loadCompleted()
    signal loadStarted()
    clip: true

    spacing: Theme.paddingMedium
    verticalLayoutDirection: ListView.BottomToTop

    ScrollBar.vertical: ScrollBar { }

    Timer {
        id: readTimer
        interval: 2000
        triggeredOnStart: false
        running: false
        repeat: false
        onTriggered: {
            markLatest()
        }
    }

    //TODO: move messages model to C++
//    model: ListModel {
//        id: messageListModel
//    }
    model: SlackClient.currentChatsModel.messages(channelId)

    delegate: MessageListItem {}

    section {
        property: "day"
        criteria: ViewSection.FullString
        labelPositioning: ViewSection.CurrentLabelAtStart
        delegate: Label {
            text: section
            width: listView.width
            horizontalAlignment: "AlignHCenter"
        }
    }

    onAppActiveChanged: {
        if (messageListModel.get(0)) {
            latestRead = messageListModel.get(0).time
            readTimer.restart()
        }
    }

    onMovementEnded: {
        if (atBottom && model.rowCount()) {
            latestRead = model.data(model.index(model.rowCount() - 1, 0)).Time
//            latestRead = messageListModel.get(messageListModel.count - 1).time
            readTimer.restart()
        }
    }

    Component.onCompleted: {
//        SlackClient.onLoadMessagesSuccess.connect(handleLoadSuccess)
//        SlackClient.onMessageReceived.connect(handleMessageReceived)
//        SlackClient.onMessageUpdated.connect(handleMessageUpdated)
    }

    Component.onDestruction: {
//        SlackClient.onLoadMessagesSuccess.disconnect(handleLoadSuccess)
//        SlackClient.onMessageReceived.disconnect(handleMessageReceived)
//        SlackClient.onMessageUpdated.disconnect(handleMessageUpdated)
    }

    function markLatest() {
        if (appActive && atBottom && messageListModel.count
                && teamId === SlackClient.lastTeam
                && SlackClient.lastChannel(teamRoot.teamId) === channel.id) {
            latestRead = messageListModel.get(0).time
            SlackClient.markChannel(teamRoot.teamId, channel.Type, channel.Id, latestRead)
            latestRead = ""
        }
    }

    function loadMessages() {
        console.log(Object.keys(channel))
        console.log(channel.Name)
        console.log(channelType)
        console.log(channel.Id)
        console.log("loading messages", teamRoot.teamId)
        SlackClient.loadMessages(teamRoot.teamId, channel.type, channel.id)
    }

//    function handleLoadSuccess(teamId, channelId, messages) {
//        console.log("loading success", channelId, channel.id)
//        if (teamId === teamRoot.teamId && channelId === channel.id) {
//            messages.sort(Message.compareByTime)
//            messageListModel.clear()
//            messages.forEach(function(message) {
//                message.day = Message.getDisplayDate(message)
//                messageListModel.insert(0, message)
//            })
//            inputEnabled = true
//            loadCompleted()

//            if (messageListModel.count) {
//                latestRead = messageListModel.get(0).time
//                readTimer.restart()
//            }
//        }
//    }

//    function handleMessageUpdated(teamId, message) {
//        if (teamId === teamRoot.teamId && message.channel === channel.id) {
//            for (var msgi = 0; msgi < messageListModel.count; msgi++) {
//                var msg = messageListModel.get(msgi);
//                if (msg.time === message.ts) {
//                    var updated = false;
//                    var reactions = msg.reactions;
//                    var reaction = { "emoji": message.emoji, "reactionscount": 1,
//                        "name": message.reaction, "users": message.user }
//                    for (var rctni = 0; rctni < reactions.count; rctni++) {
//                        var rcn = reactions.get(rctni);
//                        if (rcn.name === message.reaction) {
//                            if (message.type === "reaction_removed") {
//                                reaction.reactionscount = rcn.reactionscount - 1
//                            } else {
//                                reaction.reactionscount = rcn.reactionscount + 1
//                            }
//                            if (reaction.reactionscount <= 0) {
//                                reactions.remove(rctni, 1);
//                            } else {
//                                reactions.set(rctni, reaction);
//                            }
//                            updated = true;
//                            break;
//                        }
//                    }
//                    if (updated === false) {
//                        reactions.append(reaction)
//                    }
//                    messageListModel.set(msgi, {"reactions": reactions});
//                    break;
//                }
//            }
//        }
//    }

//    function handleMessageReceived(teamId, message) {
//        if (teamId === teamRoot.teamId && message.type === "message" && message.channel === channel.id) {
//            if (message.edited !== "") {
//                for (var msgi = 0; msgi < messageListModel.count; msgi++) {
//                    var msg = messageListModel.get(msgi);
//                    if (msg.time === message.time) {
//                        messageListModel.set(msgi, {"content": message.content});
//                        return;
//                    }
//                }
//            } else {
//            var isAtBottom = atBottom
//            message.day = Message.getDisplayDate(message)
//            messageListModel.insert(0, message)

//            if (isAtBottom) {
//                if (appActive) {
//                    latestRead = message.time
//                    readTimer.restart()
//                }
//            }
//        }
//    }
//    }
}
