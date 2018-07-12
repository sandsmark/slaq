import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
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
            radius: 5
            width: 5
            height: parent.height
            color: attachment.color === "theme" ? palette.highlight : attachment.color
        }

        Column {
            width: parent.width - color.width - Theme.paddingMedium
            spacing: Theme.paddingSmall

            Row {
                id: authorRow
                width: parent.width
                height: visible ? 16 : 0
                visible: attachment.author_name !== ""
                spacing: Theme.paddingMedium
                Image {
                    width: 16
                    height: 16
                    source: attachment.author_icon
                    sourceSize: Qt.size(16, 16)
                }
                Label {
                    width: parent.width
                    font.pointSize: Theme.fontSizeSmall
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Bold
                    text: attachment.author_name
                    onLinkActivated: linkClicked(link)
                }
            }

            RowLayout {
                width: parent.width
                ColumnLayout {
                    Layout.maximumWidth: value.paintedWidth
                    Label {
                        id: title
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                        text: attachment.title
                        onLinkActivated: linkClicked(link)
                    }

                    Label {
                        id: value
                        font.pointSize: Theme.fontSizeSmall
                        text: attachment.text
                        wrapMode: Text.Wrap
                        onLinkActivated: linkClicked(link)
                    }
                }
                Image {
                    width: 75
                    height: 75
                    source: attachment.thumb_url
                    sourceSize: Qt.size(75, 75)
                }
            }

            AttachmentFieldGrid {
                fieldList: attachment.fields
            }

            Image {
                anchors.left: parent.left
                fillMode: Image.PreserveAspectFit
                source: attachment.imageUrl
                //source: "team://" + teamId + "/" + attachment.imageUrl
                sourceSize:  attachment.imageSize
            }
        }
    }
    Row {
        id: footerRow
        anchors.left: parent.left
        spacing: Theme.paddingMedium
        Image {
            fillMode: Image.PreserveAspectFit
            source: attachment.footer_icon
            sourceSize: Qt.size(16, 16)
        }
        Label {
            font.pointSize: Theme.fontSizeSmall
            text: attachment.footer
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            visible: text.length > 0
        }
        Label {
            font.pointSize: Theme.fontSizeSmall
            text: Qt.formatDateTime(attachment.ts, "dd MMM yyyy hh:mm")
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            visible: attachment.ts > 0
        }
    }
}

