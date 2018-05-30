import QtQuick 2.8
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import ".."

Page {
    id: page

    property string channelId
    property variant channel
    property bool initialized: false

    title: channel !== undefined ? channel.name : ""

    header: Rectangle {
        height: Theme.headerSize
        border.color: "#00050505"
        border.width: 1
        radius: 5
        Label {
            text: "#" + page.title
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

    MessageListView {
        id: listView
        channel: page.channel

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
            SlackClient.postMessage(channel.id, content)
        }

        nickPopupVisible: nickPopup.visible
        onShowNickPopup: {
            nickPopup.visible = true
        }

        onHideNickPopup: {
            nickPopup.visible = false
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

    Component.onCompleted: {
        console.log("before get channel", page.channelId, SlackClient.getChannel(page.channelId).name)
        page.channel = SlackClient.getChannel(page.channelId)
        input.forceActiveFocus()
    }

    StackView.onStatusChanged: {
        if (StackView.status === StackView.Active) {
            console.log("channel active", page.title)
            SlackClient.setActiveWindow(page.channelId)

            if (!initialized) {
                initialized = true
                console.log("channel loading messages", page.title)
                listView.loadMessages()
            }
        } else {
            SlackClient.setActiveWindow("")
            listView.markLatest()
        }
    }
    StackView.onRemoved: {
        console.log("channels destroying")
        initialized = false
    }
}
