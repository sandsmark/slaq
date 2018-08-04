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
    property alias model: repliesListView.model
    property string channelName: ""
    property date thread_ts

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

        ListView {
            id: repliesListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollIndicator.vertical: ScrollIndicator { }
            verticalLayoutDirection: ListView.BottomToTop
            clip: true
            spacing: Theme.paddingMedium
            delegate: MessageListItem {
                width: repliesListView.width - repliesListView.ScrollIndicator.vertical.width
                isReplies: true
            }

        }
        MessageInput {
            visible: listView.inputEnabled
            Layout.fillWidth: true
            placeholder: qsTr("Message %1%2").arg("#").arg(channelName)
            onSendMessage: {
                SlackClient.postMessage(teamRoot.teamId, channelId, content, thread_ts)
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
