import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."

Column {
    property variant attachment

    signal linkClicked(string link)

    spacing: Theme.paddingSmall

    Label {
        id: pretextLabel
        width: parent.width
        font.pointSize: Theme.fontSizeSmall
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
            color: attachment.indicatorColor === "theme" ? palette.highlight : attachment.indicatorColor
        }

        Column {
            width: parent.width - color.width - Theme.paddingMedium
            spacing: Theme.paddingSmall

            Label {
                width: parent.width
                font.pointSize: Theme.fontSizeSmall
                font.weight: Font.Bold
                text: attachment.title
                visible: text.length > 0
                onLinkActivated: linkClicked(link)
            }

            Label {
                width: parent.width
                font.pointSize: Theme.fontSizeSmall
                text: attachment.content
                wrapMode: Text.Wrap
                visible: text.length > 0
                onLinkActivated: linkClicked(link)
            }

            AttachmentFieldGrid {
                fieldList: attachment.fields
            }

            Repeater {
                model: attachment.images
                anchors.left: parent.left

                Image {
                    fillMode: Image.PreserveAspectFit
                    source: "team://" + teamId + "/" + model.url
                    sourceSize: Qt.size(model.size.width, model.size.height)
                }
            }
        }
    }
}

