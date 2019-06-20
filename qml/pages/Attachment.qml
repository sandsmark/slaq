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

            Item {
                readonly property bool isThumb: attachment.thumb_url.toString().length > 0
                readonly property bool isImage: attachment.imageUrl.toString().length > 0
                visible: attachment !== null && (isThumb || isImage > 0)
                width: isThumb ? attachment.thumb_size.width : attachment.imageSize.width
                height: isThumb ? attachment.thumb_size.height : attachment.imageSize.height
                //                AnimatedImage {
                //                    anchors.fill: parent
                //                    asynchronous: true

                //                    fillMode: Image.PreserveAspectFit
                //                    // AnimatedImage does not support async image provider
                //                    //source: visible ? "image://emoji/slack/" + attachment.imageUrl : ""
                //                    source: parent.isImage ? attachment.imageUrl : (parent.isThumb ? attachment.thumb_url : "")
                //                    onStatusChanged: {
                //                        if (status == Image.Error) {
                //                            source = "qrc:/icons/no-image.png"
                //                        }
                //                    }
                //                }
                Item {
                    anchors.fill: parent

                    MediaPlayer {
                        id: mediaplayer
                        //source: "https://www.youtube.com/embed/QZE_mWONFaA?feature=oembed&autoplay=1&iv_load_policy=3"
                    }

                    VideoOutput {
                        anchors.fill: parent
                        source: mediaplayer
                    }

                    MouseArea {
                        id: playArea
                        anchors.fill: parent
                        onPressed: mediaplayer.play();
                    }
                    Connections {
                        target: youtubeParser
                        onUrlParsed: {
                            console.log("youtube video parsed", videoUrl, attachment.from_url)
                            if (attachment.from_url == videoUrl) {
                                mediaplayer.source = playUrl
                            }
                        }
                    }

                    Component.onCompleted: {
                        if (attachment.from_url.toString().length > 0 && attachment.service_name === "YouTube") {
                            console.log("requesting youtube url:", attachment.from_url)
                            youtubeParser.requestUrl(attachment.from_url)
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

