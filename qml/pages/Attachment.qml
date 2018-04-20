import QtQuick 2.10
import QtQuick.Controls 2.2
import ".."

Column {
    property variant attachment

    signal linkClicked(string link)

    spacing: Theme.paddingSmall

    Label {
        id: pretextLabel
        width: parent.width
        font.pixelSize: Theme.fontSizeSmall
        visible: text.length > 0
        text: attachment.pretext
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
            color: attachment.indicatorColor === "theme" ? SystemPalette.highlight : attachment.indicatorColor
        }

        Column {
            width: parent.width - color.width - Theme.paddingMedium
            spacing: Theme.paddingSmall

            Label {
                width: parent.width
                font.pixelSize: Theme.fontSizeSmall
                font.weight: Font.Bold
                text: attachment.title
                visible: text.length > 0
                onLinkActivated: linkClicked(link)
            }

            Label {
                width: parent.width
                font.pixelSize: Theme.fontSizeSmall
                text: attachment.content
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

