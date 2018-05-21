import QtQuick 2.8
import QtQuick.Controls 2.3
import QtQuick.Window 2.3
import QtQuick.Layouts 1.3
import ".."

ItemDelegate {
    id: item
    height: column.height + Theme.paddingMedium
    width: listView.width
    hoverEnabled: true
    highlighted: hovered
    property bool emojiSelectorCalled: false

    property color infoColor: palette.text
    property color textColor: palette.text

    Connections {
        target: emojiSelector
        enabled: item.emojiSelectorCalled
        onEmojiSelected: {
            emojiSelectorCalled = false
            if (emojiSelector.state === "reaction" && emoji !== "") {
                SlackClient.addReaction(channel.id, time, SlackClient.emojiNameByEmoji(emoji));
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
                width: height
                source: SlackClient.avatarUrl(user.id)
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
                        color: textColor
                    }

                    Label {
                        text: new Date(parseInt(time, 10) * 1000).toLocaleString(Qt.locale(), "H:mm")
                        font.pointSize: Theme.fontSizeTiny
                        color: infoColor
                        height: nickLabel.height
                        verticalAlignment: "AlignBottom"
                    }
                    Button {
                        id: emojiButton
                        visible: item.hovered
                        height: 22
                        width: height
                        text: "😎"
                        contentItem: Text {
                            text: emojiButton.text
                            font.family: "Twitter Color Emoji"
                            font.bold: true
                            font.pixelSize: parent.height/2
                            renderType: Text.QtRendering
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        onClicked: {
                            emojiSelector.x = emojiButton.x
                            emojiSelector.y = emojiButton.y
                            emojiSelector.state = "reaction"
                            item.emojiSelectorCalled = true
                            emojiSelector.open()
                        }
                    }
                }

                Label {
                    id: contentLabel
                    width: parent.width - avatarImage.width - parent.spacing
                    font.pointSize: Theme.fontSizeSmall
                    textFormat: Text.StyledText
                    color: textColor
                    visible: text.length > 0
                    text: content
                    onLinkActivated: handleLink(link)
                    wrapMode: Text.Wrap
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
                            height: Theme.headerSize
                            text: emoji

                            width: contentItem.contentWidth + Theme.paddingMedium*2 + countLabel.contentWidth
                            onClicked: {
                                SlackClient.deleteReaction(channel.id, time, name)
                            }

                            contentItem: Text {
                                text: control.text
                                font.family: "Twitter Color Emoji"
                                font.pixelSize: parent.height/2
                                renderType: Text.QtRendering

                            }

                            background: Rectangle {
                                implicitWidth: 100
                                implicitHeight: 40
                                opacity: enabled ? 1 : 0.3
                                border.color: "#bdbdbd"
                                border.width: 1
                                radius: 3
                                Text {
                                    id: countLabel
                                    anchors.right: parent.right; anchors.rightMargin: Theme.paddingSmall
                                    anchors.top: parent.top; anchors.topMargin: Theme.paddingSmall
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
                    source: model.thumbUrl
                    sourceSize: Qt.size(model.thumbSize.width, model.thumbSize.height)
                    visible: !expanded
                }

                AnimatedImage {
                    id: fullImage
                    anchors.fill: parent
                    //to preserve memory, cache is turned off, so to see animation again need to re-expand image
                    //TODO: create settings to change the behavior
                    source: expanded ? model.url : ""
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

                    onClicked: {
                        if (SlackClient.isDevice) {
                            pageStack.push(Qt.resolvedUrl("SlackImage.qml"), {"model": model})
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

    function handleLink(link) {
        if (link.indexOf("slaq://") === 0) {
            console.log("local link", link)
        } else {
            console.log("external link", link)
            Qt.openUrlExternally(link)
        }
    }
}
