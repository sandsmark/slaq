import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import ".."
import "../pages"

Drawer {
    id: repliesDialog

    y: Theme.paddingMedium
    bottomMargin: Theme.paddingMedium

    edge: Qt.RightEdge
    width: 0.6 * window.width
    height: window.height - Theme.paddingMedium
    property string channelName: ""
    property int parentMessageIndex: -1
    property var parentMessagesModel
    property var parentMessage
    property var modelMsg

    property alias atBottom: repliesListView.atYEnd

    onAboutToShow: {
        if (parentMessageIndex !== -1) {
            parentMessage = parentMessagesModel.messageAt(parentMessageIndex)
            //no thread model. Need to create one
            repliesListView.model = parentMessagesModel.createThread(parentMessage)
            console.log("thread", parentMessagesModel, parentMessageIndex, modelMsg.ThreadTs)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.paddingSmall
        spacing: Theme.paddingMedium
        Label {
            Layout.fillWidth: true
            font.bold: true
            font.pixelSize: Theme.fontSizeHuge
            text: qsTr("Message thread on channel: #") + repliesDialog.channelName
        }
        MessageListView {
            id: repliesListView
            messageInput: msgInput
            Layout.fillWidth: true
            Layout.fillHeight: true
            isReplies: true
        }

        MessageInput {
            id: msgInput
            Layout.fillWidth: true
            placeholder: qsTr("Message %1%2").arg("#").arg(channelName)
            onSendMessage: {
                if (parentMessage != undefined) {
                    if (updating) {
                        SlackClient.updateMessage(teamId, channel.id, content, messageSlackTime)
                        updating = false
                    } else {
                        SlackClient.postMessage(teamRoot.teamId, channel.id, content, modelMsg.ThreadTs)
                    }
                }
            }

            nickPopupVisible: channelRoot.nickPopup.visible
            onShowNickPopup: {
                channelRoot.nickPopup.visible = true
            }

            onHideNickPopup: {
                channelRoot.nickPopup.visible = false
            }
        }
    }
}
