import QtQuick 2.11
import QtQuick.Controls 2.3
import QtQuick.Window 2.3
import QtQuick.Layouts 1.3
import ".."

MouseArea {
    id: item
    height: column.height
    width: listView.width
    hoverEnabled: true
    property bool isSearchResult: false
    property bool emojiSelectorCalled: false

    Connections {
        target: emojiSelector
        enabled: item.emojiSelectorCalled
        onEmojiSelected: {
            emojiSelectorCalled = false
            if (emojiSelector.state === "reaction" && emoji !== "") {
                SlackClient.addReaction(teamId, channel.id, time, ImagesCache.getNameByEmoji(emoji));
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
                source: SlackClient.avatarUrl(teamId, user.id)
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
                        text: user.name
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                    }

                    Label {
                        text: new Date(parseInt(time, 10) * 1000).toLocaleString(Qt.locale(), "H:mm")
                        font.pointSize: Theme.fontSizeTiny
                        height: nickLabel.height
                        verticalAlignment: "AlignVCenter"
                    }

                    Label {
                        id: channelLabel
                        enabled: isSearchResult
                        visible: isSearchResult
                        text: model.channel !== undefined ? "#" + model.channel.name : ""
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                    }

                    EmojiButton {
                        id: emojiButton
                        visible: item.containsMouse && !isSearchResult
                        height: Theme.headerSize
                        width: height
                        text: "😎"
                        font.bold: true
                        font.pixelSize: parent.height/2
                        onClicked: {
                            emojiSelector.x = emojiButton.x
                            emojiSelector.y = emojiButton.y
                            emojiSelector.state = "reaction"
                            item.emojiSelectorCalled = true
                            emojiSelector.open()
                        }
                    }
                }

                TextArea {
                    id: contentLabel
                    width: parent.width - avatarImage.width - parent.spacing
                    readOnly: true
                    font.pointSize: Theme.fontSizeSmall
                    textFormat: Text.RichText
                    text: content
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
                        model: reactions

                        Button {
                            //TODO: check for Theme
                            id: control

                            hoverEnabled: true
                            ToolTip.text: users
                            ToolTip.delay: 500
                            ToolTip.timeout: 5000
                            ToolTip.visible: hovered
                            text: emoji
                            height: Theme.headerSize
                            width: (ImagesCache.isUnicode ? contentItem.contentWidth : Theme.headerSize - 4)
                                   + Theme.paddingMedium*2
                                   + countLabel.contentWidth

                            onClicked: {
                                SlackClient.deleteReaction(teamId, channel.id, time, name)
                            }

                            contentItem: Item {
                                Image {
                                    anchors.centerIn: parent
                                    width: Theme.headerSize - 4
                                    height: Theme.headerSize - 4
                                    smooth: true
                                    visible: !ImagesCache.isUnicode
                                    source: "image://emoji/" + name
                                }

                                Text {
                                    visible: ImagesCache.isUnicode
                                    anchors.centerIn: parent
                                    text: control.text
                                    font.family: "Twitter Color Emoji"
                                    font.pixelSize: Theme.headerSize - 6
                                    renderType: Text.QtRendering
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }

                            background: Rectangle {
                                color: "#eaf4f5"
                                implicitWidth: 100
                                implicitHeight: parent.height
                                opacity: enabled ? 1 : 0.3
                                border.color: "#bdbdbd"
                                border.width: 1
                                radius: 3
                                Text {
                                    id: countLabel
                                    anchors.right: parent.right; anchors.rightMargin: Theme.paddingSmall
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.pointSize: Theme.fontSizeSmall
                                    renderType: Text.QtRendering
                                    color: "#0f0f0f"
                                    text: reactionscount
                                }
                            }
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
            visible: contentLabel.visible && (imageRepeater.count > 0 || attachmentRepeater.count > 0)
        }

        Repeater {
            id: imageRepeater
            model: images

            Item {
                property bool expanded: false
                width: expanded ? listView.width - Theme.paddingLarge * 4 : model.thumbSize.width
                height: expanded ? width / (model.size.width / model.size.height) : model.thumbSize.height

                Image {
                    id: thumbImage
                    anchors.fill: parent
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "team://" + teamId + "/" + model.thumbUrl
                    sourceSize: Qt.size(model.thumbSize.width, model.thumbSize.height)
                    visible: !expanded
                }

                AnimatedImage {
                    id: fullImage
                    anchors.fill: parent
                    //to preserve memory, cache is turned off, so to see animation again need to re-expand image
                    //TODO: create settings to change the behavior
                    source: expanded ? "team://" + teamId + "/" + model.url : ""
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    visible: expanded
                    playing: expanded
                    cache: false
                    smooth: true
                }

                ProgressBar {
                    anchors.centerIn: parent
                    opacity: value < 1
                    value: expanded ? fullImage.progress : thumbImage.progress
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                Rectangle {
                    anchors.fill: imageMouseArea
                    color: Qt.rgba(1, 1, 1, 0.1)
                    visible: imageMouseArea.containsMouse
                }

                MouseArea {
                    id: imageMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: parent.expanded ? Qt.ArrowCursor : Qt.WhatsThisCursor

                    onClicked: {
                        if (SlackClient.isDevice) {
                            pageStack.push(Qt.resolvedUrl("SlackImage.qml"), {"model": model, "teamId": teamId})
                        } else {
                            parent.expanded = !parent.expanded
                        }
                    }
                }
            }
        }

        Repeater {
            id: attachmentRepeater
            model: attachments

            Attachment {
                width: column.width
                attachment: model
                onLinkClicked: handleLink(link)
            }
        }
    }

    onClicked: {
        if (permalink !== "") {
            Qt.openUrlExternally(permalink)
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
