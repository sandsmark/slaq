import QtQuick 2.11
import QtQuick.Controls 2.4
import QtMultimedia 5.9
import QtQuick.Layouts 1.3
import ".."
import "../components"

import com.iskrembilen 1.0

ColumnLayout {
    property Attachment attachment: null

    signal linkClicked(string link)

    spacing: Theme.paddingSmall

    SlaqTextTooltips {
        id: pretextLabel
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        chat: channelsList.channelModel
        font.pointSize: Theme.fontSizeSmall
        visible: text.length > 0
        text: attachment.pretext
        onLinkActivated: linkClicked(link)
        height: visible ? implicitHeight : 0
        itemFocusOnUnselect: messageInput
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
            Layout.fillWidth: false
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
                SlaqTextTooltips {
                    Layout.fillWidth: true
                    font.pointSize: Theme.fontSizeSmall
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Bold
                    text: attachment.author_name
                    onLinkActivated: linkClicked(link)
                    itemFocusOnUnselect: messageInput
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.paddingSmall
                visible: attachment.title !== "" || attachment.text !== ""

                ColumnLayout {
                    id: colLayout
                    Layout.fillWidth: true
                    SlaqTextTooltips {
                        id: titleId
                        Layout.fillWidth: true
                        chat: channelsList.channelModel
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                        wrapMode: Text.Wrap
                        text: attachment.title
                        onLinkActivated: linkClicked(link)
                        itemFocusOnUnselect: messageInput
                    }

                    SlaqTextTooltips {
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

            AnimatedImage {
                readonly property bool isThumb: attachment.thumb_url.toString().length > 0
                readonly property bool isImage: attachment.imageUrl.toString().length > 0
                visible: !youtubeLoader.visible && attachment !== null && (isThumb || isImage)
                width: isThumb ? attachment.thumb_size.width : attachment.imageSize.width
                height: visible ? (isThumb ? attachment.thumb_size.height : attachment.imageSize.height) : 0
                asynchronous: true
                fillMode: Image.Stretch
                // AnimatedImage does not support async image provider
                //source: visible ? "image://emoji/slack/" + attachment.imageUrl : ""
                source: visible ? (isImage ? attachment.imageUrl : (isThumb ? attachment.thumb_url : "")) : ""
                onStatusChanged: {
                    if (status == Image.Error) {
                        console.warn("image load error:", source)
                        source = "qrc:/icons/no-image.png"
                    }
                }
            }
            Loader {
                id: youtubeLoader
                property string videoId
                width: attachment.thumb_size.width
                height: attachment.thumb_size.height
                enabled: attachment.service_name === "YouTube"
                visible: enabled
                Connections {
                    target: youtubeParser
                    onUrlParsed: {
                        if (youtubeLoader.videoId == videoId) {
                            console.log("youtube video parsed:", videoId, attachment.from_url)
                            youtubeLoader.setSource("qrc:/qml/components/VideoFileViewer.qml", {
                                                        "directPlay":true,
                                                        "adjustThumbSize":false,
                                                        "previewThumb":attachment.thumb_url,
                                                        "videoUrl":playUrl
                                                    })
                        }
                    }
                }
                Component.onCompleted: {
                    if (attachment.from_url.toString().length > 0 && attachment.service_name === "YouTube") {
                        youtubeLoader.videoId = youtubeParser.parseVideoId(attachment.from_url)
                        if (youtubeLoader.videoId !== "") {
                            console.log("requesting youtube url:", attachment.from_url, youtubeLoader.videoId)
                            youtubeParser.requestVideoUrl(youtubeLoader.videoId)
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
        SlaqTextTooltips {
            font.pointSize: Theme.fontSizeSmall
            chat: channelsList.channelModel
            text: attachment.footer
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            visible: text.length > 0
        }
        SlaqTextTooltips {
            font.pointSize: Theme.fontSizeSmall
            chat: channelsList.channelModel
            text: Qt.formatDateTime(attachment.ts, "dd MMM yyyy hh:mm")
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            visible: attachment.ts > 0
        }
    }
}

