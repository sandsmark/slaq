import QtQuick 2.8
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import com.iskrembilen 1.0

import ".."

Page {
    id: page

    property string channelId
    property variant channel
    property bool initialized: false
    property int channelType

    title: channel !== undefined ? channel.Name : ""

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
        onChannelChanged: console.log("channel changed", listView.channel)
        channelType: page.channelType

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
        placeholder: channel ? qsTr("Message %1%2").arg("#").arg(channel.Name) : ""
        onSendMessage: {
            SlackClient.postMessage(teamRoot.teamId, channel.Id, content)
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

    StackView.onStatusChanged: {
        if (StackView.status === StackView.Active) {
            console.log("channel active", page.title)
            SlackClient.setActiveWindow(teamRoot.teamId, page.channelId)
            page.channel = SlackClient.getChannel(teamRoot.teamId, page.channelId)
            input.forceActiveFocus()

            if (!initialized) {
                initialized = true
                listView.loadMessages()
            }
        } else {
            SlackClient.setActiveWindow(teamRoot.teamId, "")
            listView.markLatest()
        }
    }
    StackView.onRemoved: {
        console.log("channels destroying")
        initialized = false
    }
}
