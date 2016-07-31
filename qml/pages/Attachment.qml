import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    property variant attachment

    signal linkClicked(string link)

    spacing: Theme.paddingSmall

    RichTextLabel {
        id: pretextLabel
        width: parent.width
        font.pixelSize: Theme.fontSizeSmall
        visible: text.length > 0
        value: attachment.pretext
        onLinkActivated: linkClicked(link)
    }

    Spacer {
        height: Theme.paddingSmall
        visible: !pretextLabel.visible
    }

    Row {
        width: parent.width
        spacing: Theme.paddingMedium

        Rectangle {
            id: color
            width: Theme.paddingSmall
            height: parent.height
            color: attachment.indicatorColor === "theme" ? Theme.highlightColor : attachment.indicatorColor
        }

        Column {
            width: parent.width - color.width - Theme.paddingMedium
            spacing: Theme.paddingSmall

            RichTextLabel {
                width: parent.width
                font.pixelSize: Theme.fontSizeSmall
                font.weight: Font.Bold
                value: attachment.title
                visible: text.length > 0
                onLinkActivated: linkClicked(link)
            }

            RichTextLabel {
                width: parent.width
                font.pixelSize: Theme.fontSizeSmall
                value: attachment.content
                visible: text.length > 0
                onLinkActivated: linkClicked(link)
            }

            AttachmentFieldGrid {
                fieldList: attachment.fields
            }

            Repeater {
                model: attachment.images

                Image {
                    width: parent.width
                    fillMode: Image.PreserveAspectFit
                    source: model.url
                    sourceSize.width: model.size.width
                    sourceSize.height: model.size.height
                }
            }
        }
    }
}

