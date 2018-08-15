import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import com.iskrembilen 1.0

import ".."
import "../dialogs"

Page {
    id: channelRoot

    objectName: "channelPage"
    property string channelId
    property string channelName
    property int channelType
    property bool initialized: false
    property url textToShowUrl: ""
    property string textToShowUserName: ""
    property string textToShowName: ""
    property Chat channel: null
    property alias nickPopup: nickPopup

    property var usersTyping: []
    onChannelIdChanged: {
        channel = SlackClient.getChannel(teamRoot.teamId, channelRoot.channelId);
        title = (channel.type === ChatsModel.Conversation ||
                 channel.type === ChatsModel.MultiUserConversation ||
                 channel.type === ChatsModel.Group)
                && channel.readableName != "" ? channel.readableName : channel.name
        console.log("channel", channel.id, channel.name, channel.readableName, channel.isPrivate)
    }

    function setChannelActive() {
        console.log("channel active", channelRoot.title);
        SlackClient.setActiveWindow(teamRoot.teamId, channelRoot.channelId);
        input.forceActiveFocus()
        messagesListView.markLatest()
    }

    function showText(url, name, userName) {
        textToShowUrl = url
        textToShowName = name
        textToShowUserName = userName
        downloadManager.append(url, "buffer", SlackClient.teamToken(teamId))
    }

    function openReplies(model, messageindex, msg) {
        replieslist.parentMessagesModel = model
        replieslist.modelMsg = msg
        replieslist.parentMessageIndex = messageindex
        replieslist.channelName = channel.name
        replieslist.open()
    }

    RepliesList {
        id: replieslist
    }

    Connections {
         target: downloadManager
         onFinished: {
             if (url === textToShowUrl) {
                 drawerTextArea.text = downloadManager.buffer(url)
                 downloadManager.clearBuffer(url)
                 channelTextView.open()
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

    BusyIndicator {
        id: loaderIndicator
        visible: true
        running: visible
        anchors.centerIn: parent
    }

    Drawer {
        id: channelTextView
        width: drawerTextArea.paintedWidth > 0.66 * channelRoot.availableWidth ?
                   0.66 * channelRoot.availableWidth : drawerTextArea.paintedWidth + Theme.paddingMedium
        height: channelRoot.availableHeight
        edge: Qt.RightEdge

        Page {
            anchors.fill: parent
            padding: Theme.paddingMedium/2
            header: Rectangle {
                height: Theme.headerSize
                border.color: "#00050505"
                border.width: 1
                radius: 5
                Row {
                    anchors.centerIn: parent
                    spacing: 10
                    Label {
                        text: "@" + textToShowUserName
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        color: palette.link
                    }
                    Label {
                        text: textToShowName
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            contentItem: ScrollView {
                id: view

                TextArea {
                    id: drawerTextArea
                    readOnly: true
                    selectByKeyboard: true
                    selectByMouse: true
                }
            }
        }
    }

    MessageListView {
        id: messagesListView

        anchors {
            top: parent.top; bottom: input.top; left: parent.left; right: parent.right
            margins: Theme.paddingSmall
        }
        onLoadCompleted: {
            loaderIndicator.visible = false
        }

        onLoadStarted: {
            loaderIndicator.visible = true
        }
    }

    Rectangle {
        width: messagesListView.width
        height: 50
        opacity: messagesListView.contentY > messagesListView.originY ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 100 } }
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.5) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    Rectangle {
        width: messagesListView.width
        height: 50
        anchors.bottom: messagesListView.bottom
        opacity: messagesListView.contentY - messagesListView.originY < messagesListView.contentHeight - messagesListView.height ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 100 } }
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.5) }
        }
    }

    Rectangle {
        anchors {
            top: nickPopup.top
            left: nickPopup.left
            right: nickPopup.right
            bottom: input.verticalCenter
            margins: -10
        }
        radius: 10
        visible: nickPopup.visible

        border.color: "lightGray"
    }

    MessageInput {
        id: input

        anchors {
            bottom: parent.bottom
        }
        anchors.left: parent.left; anchors.leftMargin: Theme.paddingLarge/2
        anchors.right: parent.right; anchors.rightMargin: Theme.paddingLarge/2

        visible: messagesListView.inputEnabled
        placeholder: qsTr("Message %1%2").arg("#").arg(channelName)
        onSendMessage: {
            SlackClient.postMessage(teamRoot.teamId, channelId, content)
        }

        nickPopupVisible: nickPopup.visible
        onShowNickPopup: {
            nickPopup.visible = true
        }

        onHideNickPopup: {
            nickPopup.visible = false
        }
    }

    Connections {
        target: SlackClient
        onUserTyping: {
            if (teamRoot.teamId === teamId && channelId === channelRoot.channelId) {
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

    Label {
        id: userTypingLabel
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.paddingSmall
        font.pixelSize: Theme.fontSizeMedium

        Timer {
            id: removeDelayTimer
            interval: 2000
            onTriggered: {
                userTypingLabel.text = ""
                usersTyping = []
            }
        }
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

        ListView {
            id: nickSuggestionsView
            width: parent.width
            height: parent.height - nickPopup.padding
            model: input.nickSuggestions
            clip: true
            ScrollBar.vertical: ScrollBar {}

            delegate: Text {
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

    StackView.onStatusChanged: {
        if (StackView.status === StackView.Active) {
            setChannelActive()
            if (!initialized) {
                initialized = true
                messagesListView.loadMessages()
            }

        } else {
            SlackClient.setActiveWindow(teamRoot.teamId, "")
        }
    }
    StackView.onRemoved: {
        console.log("channels destroying")
        initialized = false
    }
}
