import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import ".."
import com.iskrembilen.slaq 1.0 as Slack

Page {
    id: page

    property string channelId
    property variant channel
    property bool initialized: false

    title: channel.name

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
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: input.top
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
            Slack.Client.postMessage(channel.id, content)
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
        page.channel = Slack.Client.getChannel(page.channelId)
        input.forceActiveFocus()
    }

    StackView.onStatusChanged: {
        if (StackView.status === StackView.Active) {
            Slack.Client.setActiveWindow(page.channelId)

            if (!initialized) {
                initialized = true
                listView.loadMessages()
            }
        } else {
            Slack.Client.setActiveWindow("")
            listView.markLatest()
        }
    }
}
