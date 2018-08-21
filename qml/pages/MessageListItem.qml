import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Window 2.3
import QtQuick.Layouts 1.3

import ".."
import "../components"

MouseArea {
    id: itemDelegate
    height: column.height
    hoverEnabled: true
    property bool isSearchResult: false
    property bool isReplies: false
    property bool emojiSelectorCalled: false

    // counts as same if previouse user is same and last message was within 3 minutes
    readonly property bool sameuser: model.SameUser && model.TimeDiff < 180000

    function updateText() {
        var editedText = contentLabel.getText(0, contentLabel.text.length);
        SlackClient.updateMessage(teamRoot.teamId, channelId, editedText, model.Time)
        contentLabel.focus = false
        contentLabel.readOnly = true
        input.forceActiveFocus()
    }

    Connections {
        target: emojiSelector
        enabled: itemDelegate.emojiSelectorCalled
        onEmojiSelected: {
            emojiSelectorCalled = false
            if (emojiSelector.state === "reaction" && emoji !== "") {
                SlackClient.addReaction(teamId, channelId, model.Time, ImagesCache.getNameByEmoji(emoji));
            }
        }
    }

    Column {
        id: column
        width: parent.width - Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 4 : 2) - 20
        anchors.verticalCenter: parent.verticalCenter
        x: Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 2 : 1)

        Row {
            width: parent.width
            height: childrenRect.height
            spacing: Theme.paddingMedium
            leftPadding: sameuser && !(isReplies && model.ThreadIsParentMessage) ? Theme.avatarSize + spacing : 0

            Image {
                id: avatarImage
                y: Theme.paddingMedium/2
                visible: !sameuser || (isReplies && model.ThreadIsParentMessage)
                sourceSize: visible ? Qt.size(Theme.avatarSize, Theme.avatarSize) : Qt.size(0, 0)
                source: visible && model.User != null ? "image://emoji/slack/" + model.User.avatarUrl : "http://www.gravatar.com/avatar/default?d=identicon"
            }

            Column {
                height: childrenRect.height
                width: parent.width
                spacing: 1

                RowLayout {
                    height: emojiButton.implicitHeight
                    spacing: Theme.paddingMedium/2

                    Label {
                        id: nickLabel
                        text: User != null ? User.fullName !== "" ? User.fullName : (User.username !== "" ? User.username : "Private") : "Bot"
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                    }

                    Control {
                        width: 16
                        height: 16
                        visible: User != null && User.statusEmoji.length > 0
                        hoverEnabled: true
                        ToolTip.delay: 100
                        ToolTip.visible: hovered && User.status !== ""
                        ToolTip.text: User.status

                        Image {
                            sourceSize: Qt.size(parent.width, parent.height)
                            visible: !ImagesCache.isUnicode
                            smooth: true
                            cache: false
                            source: visible ? "image://emoji/" + User.statusEmoji.slice(1, -1) : ""
                        }
                        Label {
                            visible: ImagesCache.isUnicode
                            text: User != null ? ImagesCache.getEmojiByName(User.statusEmoji.slice(1, -1)) : ""
                            font.family: "Twitter Color Emoji"
                            font.pixelSize: parent.height - 2
                            renderType: Text.QtRendering
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    Label {
                        text: Qt.formatDateTime(model.Time, "yyyy/MM/dd H:mm:ss")
                        font.pointSize: Theme.fontSizeTiny
                        height: nickLabel.height
                        verticalAlignment: Text.AlignVCenter
                    }

                    Label {
                        id: channelLabel
                        enabled: isSearchResult
                        visible: isSearchResult
                        text: model.ChannelName
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                    }
                    Row {
                        visible: itemDelegate.containsMouse
                        spacing: Theme.paddingSmall
                        EmojiButton {
                            id: emojiButton
                            padding: 0
                            visible: !isSearchResult
                            implicitHeight: nickLabel.paintedHeight * 2
                            implicitWidth: nickLabel.paintedHeight * 2
                            text: "😎"
                            font.bold: true
                            font.pixelSize: parent.height/2
                            onClicked: {
                                emojiSelector.x = emojiButton.x
                                emojiSelector.y = emojiButton.y
                                emojiSelector.state = "reaction"
                                itemDelegate.emojiSelectorCalled = true
                                emojiSelector.open()
                            }
                        }
                        EmojiButton {
                            id: startThreadButton
                            padding: 0
                            visible: !isSearchResult && !itemDelegate.ListView.view.model.isThreadModel()
                            implicitHeight: nickLabel.paintedHeight * 2
                            implicitWidth: nickLabel.paintedHeight * 2
                            text: "\uD83D\uDCAC"
                            font.bold: false
                            font.pixelSize: parent.height/2
                            onClicked: {
                                console.log("index", index)
                                channelRoot.openReplies(itemDelegate.ListView.view.model, index, model)
                            }
                        }
                        EmojiButton {
                            id: trashButton
                            padding: 0
                            visible: !isSearchResult &&
                                     (model.User != null && model.User.userId === teamRoot.slackClient.teamInfo().selfId)
                            implicitHeight: nickLabel.paintedHeight * 2
                            implicitWidth: nickLabel.paintedHeight * 2
                            text: "\uD83D\uDDD1"
                            font.bold: false
                            font.pixelSize: parent.height/2
                            onClicked: {
                                teamRoot.deleteMessage(channelId, model.Time)
                            }
                        }
                        EmojiButton {
                            id: editButton
                            padding: 0
                            visible: !isSearchResult && (model.User != null &&
                                                         model.User.userId === teamRoot.slackClient.teamInfo().selfId)
                            implicitHeight: nickLabel.paintedHeight * 2
                            implicitWidth: nickLabel.paintedHeight * 2
                            text: contentLabel.readOnly ? "✎" : "💾"
                            font.bold: false
                            font.pixelSize: parent.height/2
                            onClicked: {
                                if (contentLabel.readOnly == true) {
                                    contentLabel.readOnly = false
                                    contentLabel.forceActiveFocus();
                                } else {
                                    updateText()
                                }
                            }
                        }
                    }
                }

                TextArea {
                    id: contentLabel
                    width: parent.width - avatarImage.width - parent.spacing
                    height: text === "" ? 0 : implicitHeight
                    readOnly: true
                    font.pixelSize: Theme.fontSizeLarge
                    font.italic: model.IsChanged
                    verticalAlignment: Text.AlignVCenter
                    textFormat: Text.RichText
                    text: model.Text
                    renderType: Text.QtRendering
                    selectByMouse: true
                    onLinkActivated: handleLink(link)
                    activeFocusOnPress: false
                    onLinkHovered:  {
                        if (link !== "") {
                            mouseArea.cursorShape = Qt.PointingHandCursor
                        } else {
                            mouseArea.cursorShape = Qt.ArrowCursor
                        }
                    }
                    onSelectedTextChanged: {
                        if (selectedText !== "") {
                            forceActiveFocus()
                        } else {
                            input.forceActiveFocus()
                        }
                    }
                    onEditingFinished: {
                        //undo editing if new focus is not edit save button
                        if (editButton.focus == false) {
                            undo();
                            readOnly = true
                            input.forceActiveFocus()
                        }
                    }
                    Keys.onReturnPressed: {
                        if (readOnly == false && event.modifiers == 0) {
                            updateText()
                        }
                        event.accepted = false
                    }

                    Keys.onEnterPressed: {
                        if (readOnly == false && event.modifiers == 0) {
                            updateText()
                        }
                        event.accepted = false
                    }
                    wrapMode: Text.WordWrap

                    // To avoid the border on some styles, we only want a textarea to be able to select things
                    background: Item {}

                    //Due to bug in images not rendered until app resize
                    //trigger redraw changing width
                    onTextChanged: {
                        Qt.callLater(function() {
                            var tmp = contentItem.width
                            contentItem.width = 0
                            contentItem.width = tmp
                        })
                    }

                    MouseArea {
                        id: mouseArea
                        enabled: false //we need this just for changing cursor shape
                        anchors.fill: parent
                        propagateComposedEvents: true
                    }
                }

                Item {
                    visible: ThreadReplies.length > 0 && isReplies == false
                    width: repliesRow.implicitWidth
                    height: visible ? repliesRow.implicitHeight : 0
                    Row {
                        id: repliesRow
                        spacing: 5
                        Repeater {
                            id: repliesRepeater
                            model: ThreadReplies
                            Image {
                                source: "image://emoji/slack/" + ThreadReplies[index].user.avatarUrl
                                sourceSize: Qt.size(16, 16)
                            }
                        }
                        Label {
                            text: ThreadReplies.length + " " + qsTr("replies")
                        }
                    }
                    MouseArea {
                        id: repliesMouseArea
                        enabled: true //we need this just for changing cursor shape
                        anchors.fill: parent
                        onClicked: {
                            console.log("clicked replies", model.ThreadRepliesModel, model.ThreadTs)
                            channelRoot.openReplies(itemDelegate.ListView.view.model, index, model)
                        }
                    }
                }

                Row {
                    spacing: 5

                    Repeater {
                        id: reactionsRepeater
                        model: Reactions

                        Reaction {
                            reaction: Reactions[index]
                        }
                    }
                }
            }
        }

        Item {
            height: visible ? Theme.paddingMedium : 0
            width: height
            visible: contentLabel.visible && (fileSharesRepeater.count > 0 || attachmentRepeater.count > 0)
        }

        Column {
            Repeater {
                id: fileSharesRepeater
                model: FileShares

                delegate: FileViewer {
                    width: column.width - x
                    x: Theme.avatarSize + column.spacing
                    fileshare: FileShares[index]
                }
            }
        }

        Column {
            Repeater {
                id: attachmentRepeater
                model: Attachments

                Attachment {
                    width: column.width - x
                    x: Theme.avatarSize + column.spacing
                    attachment: Attachments[index]
                    onLinkClicked: handleLink(link)
                }
            }
        }

    }

    onClicked: {
        if (model.Permalink !== "") {
            Qt.openUrlExternally(model.Permalink)
        }
    }

    function handleLink(link) {
        if (link.indexOf("slaq://") === 0) {
            console.log("local link", link)
        } else {
            console.log("external link", link)
            Qt.openUrlExternally(link)
        }
    }
}
