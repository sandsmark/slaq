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
    property bool emojiSelectorCalled: false

    // counts as same if previouse user is same and last message was within 3 minutes
    readonly property bool sameuser: model.SameUser && model.TimeDiff < 180000

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
            leftPadding: sameuser ? Theme.avatarSize + spacing : 0

            Image {
                id: avatarImage
                y: Theme.paddingMedium/2
                visible: !sameuser
                height: Theme.avatarSize
                cache: true
                asynchronous: true
                width: height
                source: visible ? model.User.avatarUrl : ""
            }

            Column {
                height: childrenRect.height
                width: parent.width
                spacing: 0

                RowLayout {
                    height: emojiButton.implicitHeight
                    spacing: Theme.paddingMedium

                    Label {
                        id: nickLabel
                        text: User.fullName
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                    }

                    Label {
                        text: Qt.formatDateTime(model.Time, "yyyy/MM/dd H:mm:ss.zzz")
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
                        spacing: Theme.paddingSmall
                        EmojiButton {
                            id: emojiButton
                            padding: 0
                            visible: itemDelegate.containsMouse && !isSearchResult
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
                            id: trashButton
                            padding: 0
                            visible: itemDelegate.containsMouse && !isSearchResult &&
                                     model.User.userId === teamRoot.slackClient.teamInfo().selfId
                            implicitHeight: nickLabel.paintedHeight * 2
                            implicitWidth: nickLabel.paintedHeight * 2
                            text: "\uD83D\uDDD1"
                            font.bold: false
                            font.pixelSize: parent.height/2
                            onClicked: {
                                teamRoot.deleteMessage(channelId, model.Time)
                            }
                        }
                    }
                }

                TextArea {
                    id: contentLabel
                    width: parent.width - avatarImage.width - parent.spacing
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
                    wrapMode: Text.Wrap

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
            height: Theme.paddingMedium
            width: height
            visible: contentLabel.visible && (fileSharesRepeater.count > 0 || attachmentRepeater.count > 0)
        }

        Repeater {
            id: fileSharesRepeater
            model: FileShares

            delegate: FileViewer {
                width: column.width - x
                x: Theme.avatarSize + column.spacing
                fileshare: FileShares[index]
            }
        }

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
