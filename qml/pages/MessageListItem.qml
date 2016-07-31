import QtQuick 2.1
import Sailfish.Silica 1.0

ListItem {
    id: item
    enabled: false
    contentHeight: column.height + Theme.paddingMedium

    property color infoColor: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
    property color textColor: item.highlighted ? Theme.highlightColor : Theme.primaryColor

    Column {
        id: column
        width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
        anchors.verticalCenter: parent.verticalCenter
        x: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)

        Item {
            width: parent.width
            height: childrenRect.height

            Label {
                text: user.name
                anchors.left: parent.left
                font.pixelSize: Theme.fontSizeTiny
                color: infoColor
            }

            Label {
                anchors.right: parent.right
                text: new Date(parseInt(time, 10) * 1000).toLocaleString(Qt.locale(), "H:mm")
                font.pixelSize: Theme.fontSizeTiny
                color: infoColor
            }
        }

        RichTextLabel {
            id: contentLabel
            width: parent.width
            font.pixelSize: Theme.fontSizeSmall
            color: textColor
            visible: text.length > 0
            value: content
            onLinkActivated: handleLink(link)
        }

        Spacer {
            height: Theme.paddingMedium
            visible: contentLabel.visible && (imageRepeater.count > 0 || attachmentRepeater.count > 0)
        }

        Repeater {
            id: imageRepeater
            model: images

            Image {
                width: parent.width
                height: model.thumbSize.height
                fillMode: Image.PreserveAspectFit
                source: model.thumbUrl
                sourceSize.width: model.thumbSize.width
                sourceSize.height: model.thumbSize.height

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("Image.qml"), {"model": model})
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
        if (link.indexOf("slackfish://") === 0) {
            console.log("local link", link)
        } else {
            console.log("external link", link)
            Qt.openUrlExternally(link)
        }
    }
}
