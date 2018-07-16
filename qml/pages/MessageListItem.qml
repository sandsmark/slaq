import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Window 2.3
import QtQuick.Layouts 1.3

import ".."
import "../components"

ItemDelegate {
    id: itemDelegate
    height: column.height
    hoverEnabled: true
    highlighted: hovered
    property bool isSearchResult: false
    property bool emojiSelectorCalled: false

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

            Image {
                id: avatarImage
                height: Theme.avatarSize
                cache: true
                asynchronous: true
                width: height
                source: model.User.avatarUrl
            }

            Column {
                height: childrenRect.height
                width: parent.width
                spacing: Theme.paddingMedium/2

                Row {
                    height: childrenRect.height
                    width: parent.width
                    spacing: Theme.paddingMedium

                    Label {
                        id: nickLabel
                        text: User.fullName
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                    }

                    Label {
                        text: Qt.formatDateTime(model.Time, "yyyy/MM/dd H:mm:ss")
                        font.pointSize: Theme.fontSizeTiny
                        height: nickLabel.height
                        verticalAlignment: "AlignBottom"
                    }

                    Label {
                        id: channelLabel
                        enabled: isSearchResult
                        visible: isSearchResult
                        text: model.ChannelName//model.channel !== undefined ? "#" + model.channel.name : ""
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                    }

                    EmojiButton {
                        id: emojiButton
                        visible: itemDelegate.hovered && !isSearchResult
                        height: Theme.headerSize
                        width: height
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
                }

                TextEdit {
                    id: contentLabel
                    width: parent.width - avatarImage.width - parent.spacing
                    readOnly: true
                    font.pointSize: Theme.fontSizeMedium
                    font.italic: model.IsChanged
                    verticalAlignment: Text.AlignVCenter
                    textFormat: Text.RichText
                    text: model.Text
                    renderType: Text.QtRendering
                    selectByKeyboard: true
                    selectByMouse: true
                    onLinkActivated: handleLink(link)
                    onLinkHovered:  {
                        if (link !== "") {
                            mouseArea.cursorShape = Qt.PointingHandCursor
                        } else {
                            mouseArea.cursorShape = Qt.ArrowCursor
                        }
                    }
                    wrapMode: Text.Wrap
                    //Due to bug in images not rendered until app resize
                    //trigger redraw changing width
                    onTextChanged: {
                        Qt.callLater(function() {
                            var tmp = width
                            width = 0
                            width = tmp
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
                fileshare: FileShares[index]
            }
        }

        Repeater {
            id: attachmentRepeater
            model: Attachments

            Attachment {
                width: column.width
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
