import QtQuick 2.11
import QtQuick.Controls 2.11
import QtMultimedia 5.11
import QtQuick.Layouts 1.11
import ".."
import "../components"

import com.iskrembilen 1.0

Column {
    id: attachRoot
    property Attachment attachment: null

    signal linkClicked(string link)

    spacing: Theme.paddingSmall

    SlaqTextTooltips {
        id: pretextLabel
        width: parent.width
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
        width: parent.width
        height: visible ? Theme.paddingSmall : 0
    }

    Row {
        width: parent.width - Theme.paddingSmall*2
        spacing: Theme.paddingMedium

        Rectangle {
            id: color
            radius: 5
            width: 5
            height: attachmentColumn.implicitHeight
            color: attachment.color === "theme" ? palette.highlight : attachment.color
        }

        Column {
            id: attachmentColumn
            width: parent.width - color.width - parent.spacing*2
            spacing: Theme.paddingSmall

            Row {
                id: authorRow
                width: parent.width - Theme.paddingSmall*2
                height: visible ? 16 : 0
                visible: attachment.author_name !== ""
                spacing: Theme.paddingMedium
                Image {
                    source: attachment !== null && attachment.author_icon.toString().length > 0 ? "image://emoji/slack/" + attachment.author_icon : ""
                    sourceSize: Qt.size(16, 16)
                }
                SlaqTextTooltips {
                    width: parent.width
                    font.pointSize: Theme.fontSizeSmall
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Bold
                    text: attachment.author_name
                    wrapMode: Text.Wrap
                    onLinkActivated: linkClicked(link)
                    itemFocusOnUnselect: messageInput
                }
            }

            Row {
                width: parent.width - Theme.paddingSmall*2
                visible: attachment.title !== "" || attachment.text !== ""
                spacing: Theme.paddingMedium

                Column {
                    id: colLayout
                    width: Math.min(Math.max(titleId.implicitWidth, valueId.implicitWidth) + parent.spacing,
                                    parent.width - Theme.avatarSize - parent.spacing)
                    SlaqTextTooltips {
                        id: titleId
                        width: parent.width
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
                        width: parent.width
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
                    sourceSize: Qt.size(Theme.avatarSize, Theme.avatarSize/ (attachment.thumb_size.width / attachment.thumb_size.height))
                }
            }

            AttachmentFieldGrid {
                width: parent.width
                fieldList: attachment.fields
            }

            Item {
                id: imgRoot
                readonly property bool isThumb: attachment.thumb_url.toString().length > 0
                readonly property bool isImage: attachment.imageUrl.toString().length > 0
                readonly property bool isVideo: attachment.service_name === "YouTube"
                readonly property real src_width: isVideo && isThumb ? attachment.thumb_size.width : attachment.imageSize.width
                readonly property real src_height: isVideo && isThumb ? attachment.thumb_size.height : attachment.imageSize.height

                width: src_width - Theme.paddingLarge * 4  > 640 ? 640 : src_width
                height: width / (src_width / src_height)

                AnimatedImage {
                    anchors.fill: parent
                    visible: !imgRoot.isVideo && attachment !== null && imgRoot.isImage

                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    // AnimatedImage does not support async image provider
                    //source: visible ? "image://emoji/slack/" + attachment.imageUrl : ""
                    source: visible ? attachment.imageUrl : ""
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
                    anchors.fill: parent
                    enabled: imgRoot.isVideo
                    visible: enabled
                    active: teamRoot.currentChannelId == channel.id && teamId === teamsSwipe.currentItem.item.teamId
                    Timer {
                        id: expirationTimer
                        onTriggered: {
                            youtubeLoader.source = undefined
                            if (youtubeLoader.videoId !== "") {
                                console.log("requesting youtube url:", attachment.from_url, youtubeLoader.videoId)
                                youtubeParser.requestVideoUrl(youtubeLoader.videoId)
                            }
                        }
                    }

                    Connections {
                        target: youtubeParser
                        onUrlParsed: {
                            if (youtubeLoader.videoId == videoId) {
                                console.log("youtube video parsed:", videoId, attachment.from_url)
                                expirationTimer.interval = youtubeParser.expiredIn(youtubeLoader.videoId)*1000
                                expirationTimer.start()
                                youtubeLoader.setSource("qrc:/qml/components/VideoFileViewer.qml", {
                                                            "directPlay":true,
                                                            "adjustThumbSize":false,
                                                            "previewThumb":attachment.thumb_url,
                                                            "videoUrl":playUrl
                                                        })
                                //console.log("playably url", playUrl)
                            }
                        }
                    }
                    Component.onCompleted: {
                        if (attachment.from_url.toString().length > 0 && attachment.service_name === "YouTube") {
                            youtubeLoader.videoId = youtubeParser.parseVideoId(attachment.from_url)
                            if (youtubeLoader.videoId !== "") {
                                console.log("requesting youtube url:", attachment.from_url, youtubeLoader.videoId)
                                youtubeParser.requestVideoUrl(youtubeLoader.videoId)
                            } else {
                                console.warn("Youtube error. Cannot parse youtube link", attachment.from_url)
                            }
                        }
                    }
                }
            }
        }
    }

    Row {
        id: footerRow
        width: parent.width
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

