import QtQuick 2.10
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import ".."

Item {
    id: item
    enabled: false
    height: column.height + Theme.paddingMedium

    property color infoColor: item.highlighted ? palette.highlightedText : palette.text
    property color textColor: item.highlighted ? palette.highlightedText : palette.text

    Column {
        id: column
        width: parent.width - Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 4 : 2)
        anchors.verticalCenter: parent.verticalCenter
        x: Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 2 : 1)

        Item {
            width: parent.width
            height: childrenRect.height

            Label {
                text: user.name
                font.pointSize: Theme.fontSizeTiny
                color: infoColor
            }

            Label {
                anchors.right: parent.right
                text: new Date(parseInt(time, 10) * 1000).toLocaleString(Qt.locale(), "H:mm")
                font.pointSize: Theme.fontSizeTiny
                color: infoColor
            }
        }

        Label {
            id: contentLabel
            width: parent.width
            font.pointSize: Theme.fontSizeSmall
            color: textColor
            visible: text.length > 0
            text: content
            onLinkActivated: handleLink(link)
        }

        Item {
            height: Theme.paddingMedium
            width: height
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
                        pageStack.push(Qt.resolvedUrl("SlackImage.qml"), {"model": model})
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
