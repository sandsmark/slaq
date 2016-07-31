import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    property alias placeholder: messageInput.placeholderText

    signal sendMessage(string content)

    width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 1 : 0)

    Spacer {
        height: Theme.paddingMedium
    }

    Row {
        width: parent.width
        spacing: Theme.paddingSmall

        TextArea {
            id: messageInput
            width: parent.width - sendButton.width
            enabled: enabled
        }

        IconButton {
            id: sendButton
            anchors {
                bottom: parent.bottom
                bottomMargin: 40
            }
            icon.source: "image://theme/icon-m-enter-accept"
            enabled: messageInput.text.length > 0
            onClicked: handleSendMessage()
        }
    }

    function handleSendMessage() {
        var input = messageInput.text
        messageInput.text = ""
        if (input.length < 1) {
            return
        }
        messageInput.focus = false
        Qt.inputMethod.hide()
        sendMessage(input)
    }
}
