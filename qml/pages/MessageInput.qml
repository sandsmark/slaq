import QtQuick 2.8
import QtQuick.Controls 2.3
import QtQuick.Window 2.3
import ".."
import "../dialogs"

Column {
    id: messageInputColumn
    property alias placeholder: messageInput.placeholderText
    property int cursorX: mapFromItem(messageInput, messageInput.cursorRectangle.x, messageInput.cursorRectangle.y).x

    property bool nickPopupVisible: false
    property var nickSuggestions: []
    property int currentNickSuggestionIndex: 0

    signal sendMessage(string content)
    signal showNickPopup()
    signal hideNickPopup()

    width: parent.width - Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 1 : 0)
    onFocusChanged: if (focus) { messageInput.focus = true; }

    Item {
        height: Theme.paddingLarge
        width: height
    }

    Row {
        width: parent.width
        spacing: Theme.paddingMedium

        TextField {
            id: messageInput
            width: parent.width - sendButton.width - uploadButton.width - Theme.paddingMedium * 3

            function updateSuggestions() {
                var selectedNick = nickSuggestions[currentNickSuggestionIndex]
                nickSuggestions = SlackClient.getNickSuggestions(text, cursorPosition)
                var nickPosition = nickSuggestions.indexOf(selectedNick)
                if (nickPosition > 0) {
                    currentNickSuggestionIndex = nickPosition
                } else {
                    currentNickSuggestionIndex = 0
                }

                if (nickSuggestions.length === 0) {
                    hideNickPopup()
                }
            }

            function insertSuggestion() {
                var lastSpace = text.indexOf(' ', cursorPosition)
                var after = " "
                if (lastSpace >= 0) {
                    after = text.substring(lastSpace)
                }
                var firstSpace = text.lastIndexOf(' ', cursorPosition - 1)
                if (firstSpace < 0) {
                    firstSpace = 0
                } else {
                    firstSpace++
                }

                var before = text.substring(0, firstSpace)

                var cursorBefore = cursorPosition
                text = before + "@" + nickSuggestions[currentNickSuggestionIndex] + after
                cursorPosition = cursorBefore - (lastSpace - firstSpace) + nickSuggestions[currentNickSuggestionIndex].length + 1
            }

            onAccepted: {
                if (nickPopupVisible) {
                    insertSuggestion()
                    hideNickPopup()
                } else {
                    handleSendMessage()
                }
            }

            Keys.onTabPressed: {
                if (nickPopupVisible) {
                    currentNickSuggestionIndex =  (currentNickSuggestionIndex + 1) % nickSuggestions.length
                } else {
                    currentNickSuggestionIndex = 0
                    updateSuggestions()
                    if (nickSuggestions.length == 1) {
                        insertSuggestion()
                    } else if (nickSuggestions.length > 1) {
                        showNickPopup()
                    }
                }
            }

            Keys.onUpPressed: currentNickSuggestionIndex = Math.max(currentNickSuggestionIndex - 1, 0)
            Keys.onDownPressed: currentNickSuggestionIndex = Math.min(currentNickSuggestionIndex + 1, nickSuggestions.length - 1)

            onCursorPositionChanged: {
                if (nickPopupVisible) {
                    updateSuggestions()
                }
            }

            onTextEdited: {
                if (nickPopupVisible) {
                    updateSuggestions()
                }
            }
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

        Qt.inputMethod.hide()
        sendMessage(input)
    }
}
