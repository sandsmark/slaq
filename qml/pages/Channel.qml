import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3

import ".."

Page {
    id: channelRoot

    property string channelId
    property variant channel
    property bool initialized: false
    property url textToShowUrl
    property string textToShowName
    property string textToShowUserName

    title: channel !== undefined ? channel.name : ""
    property var usersTyping: []

    function setChannelActive() {
        console.log("channel active", channelRoot.title)
        SlackClient.setActiveWindow(teamRoot.teamId, channelRoot.channelId)
        channelRoot.channel = SlackClient.getChannel(teamRoot.teamId, channelRoot.channelId)
        input.forceActiveFocus()
        listView.markLatest()
    }

    function showText(url, name, userName) {
        textToShowUrl = url
        textToShowName = name
        textToShowUserName = userName
        downloadManager.append(url, "buffer", SlackClient.teamToken(teamId))
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

    header: Rectangle {
        height: Theme.headerSize
        border.color: "#00050505"
        border.width: 1
        radius: 5
        Label {
            text: "#" + channelRoot.title
            anchors.centerIn: parent
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }
    }

    BusyIndicator {
        id: loader
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

        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label {
                text: "User: " + textToShowUserName + " name: " + textToShowName
            }

            ScrollView {
                id: view
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.margins: Theme.paddingMedium/2
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
        id: listView
        channel: channelRoot.channel
        onChannelChanged: console.log("channel changed", listView.channel)

        anchors {
            top: parent.top; bottom: input.top; left: parent.left; right: parent.right
            margins: Theme.paddingSmall
        }
        onLoadCompleted: {
            loader.visible = false
        }

        onLoadStarted: {
            loader.visible = true
        }
    }

    Rectangle {
        width: listView.width
        height: 50
        opacity: listView.contentY > listView.originY ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 100 } }
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.5) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    Rectangle {
        width: listView.width
        height: 50
        anchors.bottom: listView.bottom
        opacity: listView.contentY - listView.originY < listView.contentHeight - listView.height ? 1 : 0
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

        width: parent.width

        visible: listView.inputEnabled
        placeholder: channel ? qsTr("Message %1%2").arg("#").arg(channel.name) : ""
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

    Connections {
        target: SlackClient
        onUserTyping: {
            if (teamRoot.teamId === teamId && channelId === channel.id) {
                var userName = SlackClient.userName(teamRoot.teamId, userId)

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

    Column {
        id: nickPopup
        anchors {
            bottom: input.top
        }
        visible: false
        x: input.cursorX

        Repeater {
            id: nickSuggestions
            model: input.nickSuggestions

            delegate: Text {
                text: modelData
                font.bold: index === input.currentNickSuggestionIndex
            }
        }
    }

    StackView.onStatusChanged: {
        if (StackView.status === StackView.Active) {
            setChannelActive()
            if (!initialized) {
                initialized = true
                listView.loadMessages()
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
