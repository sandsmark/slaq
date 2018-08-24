import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Window 2.3
import QtQuick.Layouts 1.3
import ".."
import "../dialogs"

ColumnLayout {
    id: messageInputColumn
    property alias placeholder: messageInput.placeholderText
    property int cursorX: mapFromItem(messageInput, messageInput.cursorRectangle.x, messageInput.cursorRectangle.y).x

    property bool nickPopupVisible: false
    property var nickSuggestions: []
    property int currentNickSuggestionIndex: 0

    signal sendMessage(string content)
    signal showNickPopup()
    signal hideNickPopup()

    Layout.fillWidth: true
    Layout.leftMargin: Theme.paddingLarge/2
    Layout.rightMargin: Theme.paddingLarge/2
    onFocusChanged: if (focus) { messageInput.focus = true; }

    function insertSuggestion() {
        var lastSpace = messageInput.text.indexOf(' ', messageInput.cursorPosition)
        var after = " "
        if (lastSpace >= 0) {
            after = messageInput.text.substring(lastSpace)
        }
        var firstSpace = messageInput.text.lastIndexOf(' ', messageInput.cursorPosition - 1)
        if (firstSpace < 0) {
            firstSpace = 0
        } else {
            firstSpace++
        }
        var firstAt = messageInput.text.lastIndexOf('@', messageInput.cursorPosition - 1)
        if (firstAt >= 0) {
            firstAt++
        }

        var cursorBefore = messageInput.cursorPosition
        var before = ""
        if (firstAt != -1) {
            before = messageInput.text.substring(0, firstAt)
            messageInput.text = before + nickSuggestions[currentNickSuggestionIndex] + after
            messageInput.cursorPosition = cursorBefore + nickSuggestions[currentNickSuggestionIndex].length + 1
        } else {
            before = messageInput.text.substring(0, firstSpace)
            messageInput.text = before + "@" + nickSuggestions[currentNickSuggestionIndex] + after
            messageInput.cursorPosition = cursorBefore - (lastSpace - firstSpace) + nickSuggestions[currentNickSuggestionIndex].length + 1
        }
    }

    Item {
        height: Theme.paddingLarge
        width: height
    }

    RowLayout {
        width: parent.width
        spacing: Theme.paddingMedium

        TextArea {
            id: messageInput
            Layout.fillWidth: true
            selectByMouse: true
            wrapMode: TextArea.Wrap
            focus: true
            persistentSelection: true

            function updateSuggestions() {
                nickSuggestions = SlackClient.getNickSuggestions(teamRoot.teamId, text, cursorPosition)
                var selectedNick = nickSuggestions[currentNickSuggestionIndex]
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

            // Enter - sends message,
            // Shift-Enter - new line
            Keys.onReturnPressed: {
                if (event.modifiers == 0) {
                    doEditingFinished()
                    event.accepted = true
                } else {
                    event.accepted = false
                }
            }

            Keys.onEnterPressed: {
                if (event.modifiers == 0) {
                    doEditingFinished()
                    event.accepted = true
                } else {
                    event.accepted = false
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
                event.accepted = true
            }

            Keys.onUpPressed: {
                if (nickPopupVisible) {
                    currentNickSuggestionIndex = Math.max(currentNickSuggestionIndex - 1, 0)
                    event.accepted = true
                } else {
                    event.accepted = false
                }
            }
            Keys.onDownPressed: {
                if (nickPopupVisible) {
                    currentNickSuggestionIndex = Math.min(currentNickSuggestionIndex + 1, nickSuggestions.length - 1)
                    event.accepted = true
                } else {
                    event.accepted = false
                }
            }

            onCursorPositionChanged: {
                if (nickPopupVisible) {
                    updateSuggestions()
                }
            }

            onTextChanged: {
                if (nickPopupVisible) {
                    updateSuggestions()
                } else {
                    if (text[cursorPosition - 1] === "@") {
                        currentNickSuggestionIndex = 0
                        updateSuggestions()
                        showNickPopup()
                    }
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
            onClicked: pageStack.push(Qt.resolvedUrl("FileSend.qml"), {"channelId": channelRoot.channel.id})
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
