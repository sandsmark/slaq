import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3 as QQDialogs
import ".."
import "../pages"

LazyLoadDialog {
    property string teamId
    property string channelId
    property var messageTime
    property string messageSlackTime
    property string messageText

    sourceComponent: Dialog {
        id: page
        title: qsTr("Edit message")

        property string selectedFile: ""
        property bool uploading: false
        width: window.isMobile ? window.width : window.width / 2
        height: window.isMobile ? window.height : window.height / 2
        x: (window.width - width)/2
        y: (window.height - height)/2

        function updateText() {
            var editedText = updatedMessageInput.getText(0, updatedMessageInput.text.length);
            SlackClient.updateMessage(teamId, channelId, editedText, messageTime, messageSlackTime)
            //messageInput.forceActiveFocus()
            close()
        }

        footer: DialogButtonBox {
            Button {
                text: qsTr("Update")
                enabled: messageInput.text.length > 0
                //DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: {
                    updateText()
                }
            }
            Button {
                text: qsTr("Cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }

            onRejected: {
                close()
            }
        }

        modal: true

        contentItem: Flickable {
            width: page.availableWidth
            height: page.availableHeight
            anchors.margins: 5
            contentWidth: column.width
            contentHeight: column.height
            clip: true
            Column {
                id: column
                width: page.width
                spacing: Theme.paddingMedium

                TextArea {
                    id: updatedMessageInput
                    width: parent.width
                    placeholderText: "Message..."
                    text: messageText
                    onEditingFinished: updateText()
                }

//                TextField {
//                    id: commentInput
//                    width: parent.width
//                    placeholderText: "Comment (optional)"
//                    onAccepted: focus = false
//                }

//                ProgressBar {
//                    width: parent.width
//                    indeterminate: true
//                    visible: page.uploading
//                }

//                Spacer {
//                    height: Theme.paddingLarge
//                }
            }
        }

//        QQDialogs.FileDialog {
//            id: fileDialog
//            title: "Please choose an image file"
//            folder: shortcuts.home
//            visible: false
//            onAccepted: {
//                page.selectedFile = fileUrl
//            }
//            onRejected: {
//                page.selectedFile = fileUrl
//            }
//        }

    }
}
