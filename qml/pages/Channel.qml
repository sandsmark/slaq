import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import com.iskrembilen 1.0

import ".."

Page {
    id: channelRoot

    property string channelId
    property string channelName
    property int channelType
    property bool initialized: false
    property url textToShowUrl: ""
    property string textToShowUserName: ""
    property string textToShowName: ""

    title: channelName
    property var usersTyping: []
    onChannelIdChanged: {
        channelName = SlackClient.getChannelName(teamRoot.teamId, channelRoot.channelId);
    }

    function setChannelActive() {
        console.log("channel active", channelRoot.title);
        SlackClient.setActiveWindow(teamRoot.teamId, channelRoot.channelId);
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

    header: Label {
        text: "#" + channelRoot.title
        horizontalAlignment: "AlignHCenter"
        verticalAlignment: "AlignVCenter"
        height: Theme.headerSize
        font.bold: true
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
        id: listView

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
