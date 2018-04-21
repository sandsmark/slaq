import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import ".."
import "../dialogs"

Column {
    id: messageInputColumn
    property alias placeholder: messageInput.placeholderText

    signal sendMessage(string content)

    width: parent.width - Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 1 : 0)

    Item {
        height: Theme.paddingLarge
        width: height
    }

    Row {
        width: parent.width
        spacing: Theme.paddingSmall

        TextArea {
            id: messageInput
            width: parent.width - sendButton.width - uploadButton.width
            enabled: enabled
        }

        Button {
            id: sendButton
            icon.name: "document-send"
            enabled: messageInput.text.length > 0
            onClicked: handleSendMessage()
        }

        Button {
            id: uploadButton
            icon.name: "upload-media"
            enabled: messageInput.text.length > 0
            onClicked: pageStack.push(Qt.resolvedUrl("FileSend.qml"), {"channelId": page.channelId})
        }
    }

    Spacer {
        height: Theme.paddingMedium
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
