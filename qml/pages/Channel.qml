import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import com.iskrembilen 1.0
import SlackComponents 1.0

import ".."
import "../dialogs"

Page {
    id: channelRoot

    objectName: "channelPage"

    property Chat channel: null
    property alias nickPopup: nickPopup

    property var usersTyping: []
    onChannelChanged: {
        if (channel != null) {
            title = (channel.type === ChatsModel.Conversation ||
                     channel.type === ChatsModel.MultiUserConversation ||
                     channel.type === ChatsModel.Group)
                    && channel.readableName != "" ? channel.readableName : channel.name
            console.log("channel", channel.id, channel.name, channel.readableName, channel.isPrivate, channel.type)
            setChannelActive()
            messagesListView.loadMessages()
        }
    }

    function setChannelActive() {
        if (channel == null)
            return
        console.log("channel active", channelRoot.title);
        SlackClient.setActiveWindow(teamRoot.teamId, channelRoot.channel.id);
        input.forceActiveFocus()
        messagesListView.markLatest()
    }

    function openReplies(model, messageindex, msg) {
        replieslist.parentMessagesModel = model
        replieslist.modelMsg = msg
        replieslist.parentMessageIndex = messageindex
        replieslist.channelName = channel.name
        replieslist.open()
    }

    Connections {
        target: SlackClient
        onUserTyping: {
            if (channelRoot.channel == null) {
                return
            }

            if (teamRoot.teamId === teamId && channelId === channelRoot.channel.id) {
                if (usersTyping.indexOf(userName) === -1) {
                    usersTyping.push(userName)
                }
                if (usersTyping.length > 0) {
                    userTypingLabel.text = "User(s) " + usersTyping.join(", ") + " typing..."
                    removeDelayTimer.restart()
                }
            }
        }
    }

    header: RowLayout {
        spacing: 10
        width: parent.width
        height: channelTitle.paintedHeight > Theme.headerSize ? channelTitle.paintedHeight + Theme.paddingMedium : Theme.headerSize
        Label {
            text: "#" + channelRoot.title
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Layout.fillHeight: true
            font.bold: true
        }
        SlackText {
            id: channelTitle
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            font.bold: false
            chat: channelsList.channelModel
            text: channelRoot.channel != null ? channelRoot.channel.topic : ""
            wrapMode: Text.Wrap
            onLinkHovered:  {
                if (link !== "") {
                    msgToolTip.text = link
                    var coord = mapToItem(teamsSwipe, x, 0)
                    msgToolTip.x = coord.x - msgToolTip.width/2
                    msgToolTip.y = coord.y - (msgToolTip.height + Theme.paddingSmall)
                    msgToolTip.open()
                } else {
                    msgToolTip.close()
                }
            }
            onLinkActivated: {
                if (link.indexOf("slaq://") === 0) {
                    if (link.indexOf("slaq://channel") === 0) {
                        var id = link.replace("slaq://channel/", "")
                        SlackClient.joinChannel(teamRoot.teamId, id)
                    }
                } else {
                    Qt.openUrlExternally(link)
                }
            }
        }
    }

    RepliesList {
        id: replieslist
    }

    Popup {
        id: nickPopup
        visible: false
        modal: false
        width: 200
        height: messagesListView.height
        x: input.cursorX
        y: messagesListView.y
        focus: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            width: parent.width
            height: parent.height - nickPopup.padding
            radius: 10
            color: palette.base

            border.color: "lightGray"
        }

        ListView {
            id: nickSuggestionsView
            anchors.fill: parent
            anchors.margins: 5
            model: input.nickSuggestions
            clip: true
            ScrollBar.vertical: ScrollBar {}

            delegate: Label {
                text: modelData
                font.bold: index === input.currentNickSuggestionIndex
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        input.currentNickSuggestionIndex = index
                        input.insertSuggestion()
                        nickPopup.close()
                    }
                }
            }
        }
    }

    contentItem: ColumnLayout {
        width: parent.width
        spacing: Theme.paddingSmall

        MessageListView {
            id: messagesListView
            Layout.fillHeight: true
            Layout.fillWidth: true
            messageInput: input

            onLoadCompleted: {
                loaderIndicator.visible = false
            }

            onLoadStarted: {
                loaderIndicator.visible = true
            }

            Rectangle {
                width: parent.width - parent.ScrollBar.vertical.width - 10
                height: 50
                opacity: messagesListView.contentY > messagesListView.originY ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 100 } }
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.5) }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }

            Rectangle {
                width: parent.width - parent.ScrollBar.vertical.width - 10
                height: 50
                anchors.bottom: parent.bottom
                opacity: messagesListView.contentY - messagesListView.originY < messagesListView.contentHeight - messagesListView.height ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 100 } }
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.5) }
                }
            }
        }

        MessageInput {
            id: input

            onUpPressed: {
                messagesListView.currentIndex = 0
                var myListItemObject = messagesListView.currentItem
                if (myListItemObject !== null) {
                    myListItemObject.editMessage()
                }
            }

            enabled: messagesListView.inputEnabled
            placeholder: qsTr("Message %1%2").arg("#").arg(channel.name)
            onSendMessage: {
                if (updating) {
                    SlackClient.updateMessage(teamId, channel.id, content,
                                              messageTime, messageSlackTime)
                    updating = false
                } else {
                    SlackClient.postMessage(teamRoot.teamId, channel.id, content)
                }
            }

            nickPopupVisible: nickPopup.visible
            onShowNickPopup: {
                nickPopup.visible = true
            }

            onHideNickPopup: {
                nickPopup.visible = false
            }
        }

        Label {
            id: userTypingLabel
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.paddingSmall
            font.pixelSize: Theme.fontSizeSmall
            Timer {
                id: removeDelayTimer
                interval: 2000
                onTriggered: {
                    userTypingLabel.text = ""
                    usersTyping = []
                }
            }
        }
    }

    Component.onDestruction: {
        //SlackClient.setActiveWindow(teamRoot.teamId, "")
        if (channel == null)
            return

        console.log("channel destroying", channel.name)
    }
}
