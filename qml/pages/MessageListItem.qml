import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Window 2.3
import QtQuick.Layouts 1.3

import SlackComponents 1.0

import ".."
import "../components"

MouseArea {
    id: itemDelegate
    height: column.implicitHeight
    hoverEnabled: true
    property bool isSearchResult: false
    property bool isReplies: false
    property bool emojiSelectorCalled: false

    // counts as same if previouse user is same and last message was within 3 minutes
    readonly property bool sameuser: model.SameUser && model.TimeDiff < 180000

    property var messageInput

    Connections {
        target: emojiSelector
        enabled: itemDelegate.emojiSelectorCalled
        onEmojiSelected: {
            emojiSelectorCalled = false
            if (emojiSelector.state === "reaction" && emoji !== "") {
                SlackClient.addReaction(teamId, channel.id, model.Time,
                                        ImagesCache.getNameByEmoji(emoji),
                                        model.SlackTimestamp);
            }
        }
    }

    Column {
        id: column
        height: implicitHeight
        width: parent.width - Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 4 : 2) - 20

        Row {
            width: parent.width
            height: implicitHeight
            spacing: Theme.paddingMedium
            leftPadding: sameuser && !(isReplies && model.ThreadIsParentMessage) ? Theme.avatarSize + Theme.paddingMedium : 0

            Image {
                id: avatarImage
                visible: !sameuser || (isReplies && model.ThreadIsParentMessage)
                height: visible ? Theme.avatarSize : 0
                sourceSize: visible ? Qt.size(Theme.avatarSize, Theme.avatarSize) : Qt.size(0, 0)
                source: visible && model.User != null ? "image://emoji/slack/" + model.User.avatarUrl :
                                                        "http://www.gravatar.com/avatar/default?d=identicon"
            }


            Column {
                id: columnText
                height: implicitHeight
                width: parent.width
                spacing: 1

                Control {
                    topPadding: Theme.paddingLarge
                    width: parent.width - avatarImage.width - parent.spacing
                    height: contentLabel.text.length == 0 ? topPadding : contentLabel.implicitHeight + topPadding
                    RowLayout {
                        spacing: Theme.paddingMedium/2
                        visible: !sameuser || itemDelegate.containsMouse
                        height: visible ? Theme.paddingLarge : 0
                        anchors.top: parent.top
                        anchors.topMargin: 2

                        Label {
                            id: nameLabel
                            visible: !sameuser
                            text: User != null ? User.fullName.length > 0 ? User.fullName : (User.username !== "" ? User.username : "Private") : "Undefined"
                            font.pixelSize: Theme.fontSizeMedium
                            font.bold: true
                            font.italic: false
                        }

                        Label {
                            id: nickLabel
                            visible: !sameuser
                            text: "(@" + (User != null ? (User.username !== "" ? User.username : "Private") : "Undefined" ) + ")"
                            font.pixelSize: Theme.fontSizeMedium
                            font.italic: false
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    var cursorpos = input.messageInput.cursorPosition
                                    messageInput.messageInput.insert(input.messageInput.cursorPosition, "@" + User.username + " ")
                                    messageInput.messageInput.forceActiveFocus()
                                    messageInput.messageInput.cursorPosition = cursorpos + User.username.length + 2
                                }
                            }
                        }

                        UserStatusEmoji {
                            width: 16
                            height: 16
                            user: User
                            visible: User != null && User.statusEmoji.length > 0 && !sameuser
                        }

                        Label {
                            text: Qt.formatDateTime(model.Time, "yyyy/MM/dd H:mm:ss")
                            font.italic: false
                            font.pixelSize: Theme.fontSizeMedium
                            height: nickLabel.height
                            verticalAlignment: Text.AlignVCenter
                        }

                        Label {
                            id: channelLabel
                            enabled: isSearchResult
                            visible: isSearchResult
                            text: "#"+model.ChannelName
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                            font.italic: false
                        }
                        Row {
                            visible: itemDelegate.containsMouse
                            spacing: Theme.paddingMedium/2
                            EmojiRoundButton {
                                id: emojiButton
                                padding: 0
                                visible: !isSearchResult
                                text: "😎"
                                font.pixelSize: Theme.fontSizeLarge
                                onClicked: {
                                    emojiSelector.x = mapToItem(teamsSwipe, x, y).x
                                    emojiSelector.y = mapToItem(teamsSwipe, x, y).y - emojiSelector.height
                                    emojiSelector.state = "reaction"
                                    itemDelegate.emojiSelectorCalled = true
                                    emojiSelector.open()
                                }
                                background: Item {}
                            }
                            EmojiRoundButton {
                                id: startThreadButton
                                padding: 0
                                visible: !isSearchResult && !itemDelegate.ListView.view.model.isThreadModel()
                                text: "\uD83D\uDCAC"
                                font.pixelSize: Theme.fontSizeLarge
                                onClicked: {
                                    channelRoot.openReplies(itemDelegate.ListView.view.model, index, model)
                                }
                                background: Item {}
                            }
                            EmojiRoundButton {
                                id: trashButton
                                padding: 0
                                visible: !isSearchResult &&
                                         (model.User != null && model.User.userId === teamRoot.slackClient.teamInfo().selfId)
                                text: "\uD83D\uDDD1"
                                font.pixelSize: Theme.fontSizeLarge
                                onClicked: {
                                    teamRoot.deleteMessage(channel.id, model.Time, model.SlackTimestamp)
                                }
                                background: Item {}
                            }
                            EmojiRoundButton {
                                padding: 0
                                id: editButton
                                visible: !isSearchResult && (model.User != null &&
                                                             model.User.userId === teamRoot.slackClient.teamInfo().selfId)
                                text: input.updating ? "💾" : "✎"
                                font.pixelSize: Theme.fontSizeLarge
                                onClicked: {
                                    messageInput.messageInput.text = model.OriginalText
                                    messageInput.updating = true
                                    messageInput.messageTime = model.Time
                                    messageInput.messageSlackTime = model.SlackTimestamp
                                    messageInput.messageInput.forceActiveFocus();
//                                    editMessageDialog.teamId = teamRoot.teamId
//                                    editMessageDialog.channelId = channel.id
//                                    editMessageDialog.messageText = model.OriginalText
//                                    editMessageDialog.open()
                                }
                                background: Item {}
                            }
                        }
                    }
                    SlackText {
                        id: contentLabel
                        y: parent.topPadding
                        color: contentLabel.palette.windowText
                        linkColor: contentLabel.palette.link
                        width: parent.width
                        //readOnly: true
                        font.pixelSize: Theme.fontSizeLarge
                        font.italic: model.IsChanged
                        verticalAlignment: Text.AlignVCenter
                        textFormat: Text.RichText
                        text: model.Text
                        renderType: Text.QtRendering
                        selectByMouse: true
                        onLinkActivated: handleLink(link)
                        //activeFocusOnPress: false
                        onLinkHovered:  {
                            if (link !== "") {
                                mouseArea.cursorShape = Qt.PointingHandCursor
                            } else {
                                mouseArea.cursorShape = Qt.ArrowCursor
                            }
                        }
                        onImageHovered:  {
                            console.log("image", imagelink)
                        }
                        onSelectedTextChanged: {
                            if (selectedText !== "") {
                                forceActiveFocus()
                            } else {
                                messageInput.forceActiveFocus()
                            }
                        }

                        wrapMode: Text.WordWrap

                        // To avoid the border on some styles, we only want a textarea to be able to select things
                        //background: Item {}

                        MouseArea {
                            id: mouseArea
                            enabled: false //we need this just for changing cursor shape
                            anchors.fill: parent
                            propagateComposedEvents: true
                        }
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
            }
        }

        Item {
            height: visible ? Theme.paddingMedium : 0
            width: height
            visible: contentLabel.visible && (fileSharesRepeater.count > 0 || attachmentRepeater.count > 0)
        }

        Column {
            width: parent.width
            leftPadding: Theme.avatarSize + Theme.paddingMedium
            height: implicitHeight
            Repeater {
                id: fileSharesRepeater
                model: FileShares

                delegate: FileViewer {
                    expandedImageWidth: isReplies ? repliesListView.width : msgListView.width
                    fileshare: FileShares[index]
                }
            }
        }

        Column {
            width: parent.width
            height: implicitHeight
            leftPadding: Theme.avatarSize + Theme.paddingMedium

            Repeater {
                id: attachmentRepeater
                model: Attachments

                delegate: Attachment {
                    width: column.width
                    //Layout.maximumWidth: column.width
                    attachment: Attachments[index]
                    onLinkClicked: handleLink(link)
                }
            }
        }

        Item {
            height: visible ? Theme.paddingSmall : 0
            width: height
            visible: reactionsRepeater.count > 0
        }

        Row {
            spacing: 5
            leftPadding: Theme.avatarSize + Theme.paddingMedium

            Repeater {
                id: reactionsRepeater
                model: Reactions

                Reaction {
                    reaction: Reactions[index]
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
