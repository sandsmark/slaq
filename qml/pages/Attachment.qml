import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import ".."

import com.iskrembilen 1.0
import SlackComponents 1.0

ColumnLayout {
    property Attachment attachment: null

    signal linkClicked(string link)

    spacing: Theme.paddingSmall

    SlackText {
        id: pretextLabel
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        chat: channelsList.channelModel
        font.pointSize: Theme.fontSizeSmall
        visible: text.length > 0
        text: attachment.pretext
        onLinkActivated: linkClicked(link)
        height: visible ? implicitHeight : 0
    }

    Spacer {
        visible: !pretextLabel.visible
        Layout.fillWidth: true
        height: visible ? Theme.paddingSmall : 0
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: Theme.paddingSmall
        spacing: Theme.paddingMedium

        Rectangle {
            id: color
            radius: 5
            width: 5
            Layout.fillHeight: true
            color: attachment.color === "theme" ? palette.highlight : attachment.color
        }

        ColumnLayout {
            id: attachmentColumn
            Layout.fillWidth: true
            spacing: Theme.paddingSmall

            RowLayout {
                id: authorRow
                Layout.fillWidth: true
                Layout.margins: Theme.paddingSmall
                height: visible ? 16 : 0
                visible: attachment.author_name !== ""
                spacing: Theme.paddingMedium
                Image {
                    source: attachment !== null && attachment.author_icon.toString().length > 0 ? "image://emoji/slack/" + attachment.author_icon : ""
                    sourceSize: Qt.size(16, 16)
                }
                SlackText {
                    Layout.fillWidth: true
                    font.pointSize: Theme.fontSizeSmall
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Bold
                    text: attachment.author_name
                    onLinkActivated: linkClicked(link)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.paddingSmall
                visible: attachment.title !== "" || attachment.text !== ""

                ColumnLayout {
                    id: colLayout
                    Layout.fillWidth: true
                    SlackText {
                        id: titleId
                        Layout.fillWidth: true
                        chat: channelsList.channelModel
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                        wrapMode: Text.Wrap
                        text: attachment.title
                        onLinkActivated: linkClicked(link)
                    }

                    SlackText {
                        id: valueId
                        Layout.fillWidth: true
                        chat: channelsList.channelModel
                        font.pointSize: Theme.fontSizeSmall
                        text: attachment.text
                        itemFocusOnUnselect: messageInput
                        wrapMode: Text.Wrap
                        onLinkActivated: linkClicked(link)
                    }
                }
                Image {
                    visible: attachment !== null && attachment.thumb_url.toString().length > 0
                    source: visible ? "image://emoji/slack/" + attachment.thumb_url : ""
                    sourceSize: Qt.size(Theme.avatarSize, Theme.avatarSize)
                }
            }

            AttachmentFieldGrid {
                fieldList: attachment.fields
            }

            Item {
                width: attachment.imageSize.width
                height: attachment.imageSize.height
                AnimatedImage {
                    anchors.fill: parent
                    asynchronous: true
                    visible: attachment !== null && attachment.imageUrl.toString().length > 0
                    fillMode: Image.PreserveAspectFit
                    // AnimatedImage does not support async image provider
                    //source: visible ? "image://emoji/slack/" + attachment.imageUrl : ""
                    source: visible ? attachment.imageUrl : ""
                    onStatusChanged: {
                        if (status == Image.Error) {
                            source = "qrc:/icons/no-image.png"
                        }
                    }
                }
            }
        }
    }
    RowLayout {
        id: footerRow
        spacing: Theme.paddingMedium
        visible: attachment.footer !== ""

        Image {
            fillMode: Image.PreserveAspectFit
            visible: attachment !== null && attachment.footer_icon.toString().length > 0
            source: visible ? "image://emoji/slack/" + attachment.footer_icon : ""
            sourceSize: Qt.size(16, 16)
        }
        SlackText {
            font.pointSize: Theme.fontSizeSmall
            chat: channelsList.channelModel
            text: attachment.footer
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            visible: text.length > 0
        }
        SlackText {
            font.pointSize: Theme.fontSizeSmall
            chat: channelsList.channelModel
            text: Qt.formatDateTime(attachment.ts, "dd MMM yyyy hh:mm")
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            visible: attachment.ts > 0
        }
    }
}

