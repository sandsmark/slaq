import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import com.iskrembilen 1.0

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

    header: Row {
        spacing: 10
        Label {
            text: "#" + channelRoot.title
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            height: Theme.headerSize
            font.bold: true
        }
        Label {
            text: channelRoot.channel != null ? channelRoot.channel.topic : ""
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            height: Theme.headerSize
            font.bold: false
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

            onLoadCompleted: {
                loaderIndicator.visible = false
            }

            onLoadStarted: {
                loaderIndicator.visible = true
            }

            Rectangle {
                width: parent.width - parent.ScrollBar.vertical.width
                height: 50
                opacity: messagesListView.contentY > messagesListView.originY ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 100 } }
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.5) }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }

            Rectangle {
                width: parent.width - parent.ScrollBar.vertical.width
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

            enabled: messagesListView.inputEnabled
            placeholder: qsTr("Message %1%2").arg("#").arg(channel.name)
            onSendMessage: {
                SlackClient.postMessage(teamRoot.teamId, channel.id, content)
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
