import QtQuick 2.0
import Sailfish.Silica 1.0
import "../dialogs"

Column {
    id:messageInputColumn
    property alias placeholder: messageInput.placeholderText

    signal sendMessage(string content)
    signal sendPicture(string filename, string content)

    property var hasAttachment: false;
    property var attachementFile: "";

    width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 1 : 0)

    Spacer {
        height: Theme.paddingMedium
    }

    Row {
        width: parent.width
        spacing: Theme.paddingSmall
        IconButton {
            id: imageButton
            anchors {
                bottom: parent.bottom
                bottomMargin: 40
            }
            icon.source: "image://theme/icon-m-image"
            onClicked: {
                pageStack.push(imagepicker)
                imagepicker.selected.connect(handleImageSelect);
            }
            highlighted: hasAttachment
        }

        TextArea {
            id: messageInput
            width: parent.width - sendButton.width - imageButton.width
            enabled: enabled
        }

        IconButton {
            id: sendButton
            anchors {
                bottom: parent.bottom
                bottomMargin: 40
            }
            icon.source: "image://theme/icon-m-enter-accept"
            enabled: (messageInput.text.length > 0) || (hasAttachment)
            onClicked: handleSendMessage()
        }
    }

    function handleImageSelect(){
        console.log("attachment selected");
        hasAttachment = true
    }

    function handleSendMessage() {
        var input = messageInput.text
        messageInput.text = ""
        if (input.length < 1 && !hasAttachment) {
            return
        }

        if(hasAttachment){
            sendPicture(imagepicker.selectedPath, input);
            hasAttachment=false;
            imagepicker.reset
        } else {
            messageInput.focus = false
            Qt.inputMethod.hide()
            sendMessage(input)
        }
    }


}//column
