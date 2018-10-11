import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import ".."

import com.iskrembilen 1.0

Column {
    property Attachment attachment: null

    signal linkClicked(string link)

    spacing: Theme.paddingSmall

    Label {
        id: pretextLabel
        width: parent.width
        wrapMode: Text.WordWrap
        font.pointSize: Theme.fontSizeSmall
        visible: text.length > 0
        text: attachment.pretext
        onLinkActivated: linkClicked(link)
        height: visible ? implicitHeight : 0
    }

    Spacer {
        visible: !pretextLabel.visible
        height: visible ? Theme.paddingSmall : 0
    }

    Row {
        width: parent.width - Theme.paddingMedium
        spacing: Theme.paddingMedium

        Rectangle {
            id: color
            radius: 5
            width: 5
            height: parent.height
            color: attachment.color === "theme" ? palette.highlight : attachment.color
        }

        ColumnLayout {
            width: parent.width - color.width - Theme.paddingMedium
            spacing: Theme.paddingSmall

            Row {
                id: authorRow
                width: parent.width - Theme.paddingMedium
                height: visible ? 16 : 0
                visible: attachment.author_name !== ""
                spacing: Theme.paddingMedium
                Image {
                    source: attachment !== null && attachment.author_icon.toString().length > 0 ? "image://emoji/slack/" + attachment.author_icon : ""
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
                width: parent.width - Theme.paddingMedium
                visible: attachment.title !== "" || attachment.text !== ""
                height: visible ? implicitHeight : 0

                ColumnLayout {
                    id: colLayout
                    Layout.fillWidth: true
                    Label {
                        id: titleId
                        Layout.fillWidth: true
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                        text: attachment.title
                        onLinkActivated: linkClicked(link)
                    }

                    Label {
                        id: valueId
                        Layout.fillWidth: true
                        font.pointSize: Theme.fontSizeSmall
                        text: attachment.text
                        wrapMode: Text.Wrap
                        onLinkActivated: linkClicked(link)
                    }
                }
                Image {
                    source: attachment !== null && attachment.thumb_url.toString().length > 0 ? "image://emoji/slack/" + attachment.thumb_url : ""
                    sourceSize: Qt.size(colLayout.height, colLayout.height)
                }
            }

            AttachmentFieldGrid {
                fieldList: attachment.fields
            }

            Item {
                //anchors.left: parent.left
                width: attachment.imageSize.width
                height: attachment.imageSize.height
                AnimatedImage {
                    visible: attachment.isAnimated
                    fillMode: Image.PreserveAspectFit
                    source: "team://" + teamId + "/" + attachment.imageUrl
                }
                Image {
                    visible: !attachment.isAnimated
                    fillMode: Image.PreserveAspectFit
                    source: attachment !== null && attachment.imageUrl.toString().length > 0 ? "image://emoji/slack/" + attachment.imageUrl : ""
                    sourceSize:  attachment.imageSize
                }
            }
        }
    }
    Row {
        id: footerRow
        anchors.left: parent.left
        spacing: Theme.paddingMedium
        visible: attachment.footer !== ""
        height: visible ? implicitHeight : 0

        Image {
            fillMode: Image.PreserveAspectFit
            source: attachment !== null && attachment.footer_icon.length > 0 ? "image://emoji/slack/" + attachment.footer_icon : ""
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

