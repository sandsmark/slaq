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

        TextArea {
            id: messageInput
            width: parent.width - sendButton.width - uploadButton.width - emojiButton.width - Theme.paddingMedium * 4
            selectByMouse: true
            wrapMode: TextArea.Wrap
            focus: true

            function updateSuggestions() {
                var selectedNick = nickSuggestions[currentNickSuggestionIndex]
                nickSuggestions = SlackClient.getNickSuggestions(teamRoot.teamId, text, cursorPosition)
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

            Connections {
                target: emojiSelector
                onEmojiSelected: {
                    if (emojiSelector.state === "input" && emoji !== "") {
                        messageInput.insert(messageInput.cursorPosition, emoji)
                        messageInput.forceActiveFocus()
                    }
                }
            }

            function doEditingFinished() {
                if (nickPopupVisible) {
                    insertSuggestion()
                    hideNickPopup()
                } else {
                    handleSendMessage()
                }
            }

            onEditingFinished: {
                doEditingFinished()
            }

            Keys.onReturnPressed: {
                if (event.modifiers & Qt.ControlModifier) {
                    doEditingFinished()
                }
            }

            Keys.onEnterPressed: {
                if (event.modifiers & Qt.ControlModifier) {
                    doEditingFinished()
                }
            }

            Keys.onTabPressed: {
                console.log("pressed tab")
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

            onTextChanged: {
                if (nickPopupVisible) {
                    updateSuggestions()
                }
            }
        }

        EmojiButton {
            id: emojiButton
            width: height
            text: "😎"
            font.bold: true
            onClicked: {
                emojiSelector.x = x
                emojiSelector.y = mapToGlobal(x, y).y - emojiSelector.height - emojiButton.height
                emojiSelector.state = "input"
                emojiSelector.open()
            }
        }

        EmojiButton {
            id: sendButton
            font.bold: true
            width: height
            text: "📨"
            enabled: messageInput.text.length > 0
            onClicked: handleSendMessage()
        }

        Button {
            id: uploadButton
            icon.name: "upload-media"
            onClicked: pageStack.push(Qt.resolvedUrl("FileSend.qml"), {"channelId": channelRoot.channelId})
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
