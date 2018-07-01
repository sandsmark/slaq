import QtQuick 2.11
import QtMultimedia 5.9
import QtQuick.Controls 2.4
import Qt.labs.platform 1.0 as Platform

import ".."

Item {
    width: loader.item.width + Theme.paddingMedium
    height: loader.item.height + Theme.paddingMedium

    Platform.FileDialog {
        id: fileSaveDialog
        title: "Please choose file name"
        folder: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DownloadLocation)
        fileMode: Platform.FileDialog.SaveFile
        onAccepted: {
            downloadManager.append(model.url_download, file, SlackClient.teamToken(teamId))
        }
    }

    Menu {
        id: fileSharedMenu
        MenuItem {
            text: qsTr("Download")
            onTriggered: {                
                fileSaveDialog.open()
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressAndHold: {
            fileSharedMenu.popup();
        }
        onClicked: {
            if (loader.item) {
                loader.item.clicked()
            }
        }
    }

    Loader {
        id: loader
        anchors.centerIn: parent
        anchors.margins: Theme.paddingMedium/2
        sourceComponent: {
            if (model.mimetype.indexOf("video") !== -1) {
                return videoSharedFileComponent
            } else if (model.mimetype.indexOf("image") !== -1) {
                return imageSharedFileComponent
            } else if (model.mimetype.indexOf("text") !== -1) {
                return textSharedFileComponent
            }
        }
    }

    Component {
        id: textSharedFileComponent

        Text {
            text: model.preview_highlight
            wrapMode: Text.WordWrap
            textFormat: Text.RichText
        }

    }
    Component {
        id: videoSharedFileComponent

        Item {
            id: videoItem
            Image {
                id: thumbVideoImage
                anchors.fill: parent
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                source: "team://" + teamId + "/" + model.thumbUrl
                onStatusChanged: {
                    if (status === Image.Ready) {
                        videoItem.width = sourceSize.width - Theme.paddingLarge * 4  > 360 ? 360 : sourceSize.width
                        videoItem.height = width / (sourceSize.width / sourceSize.height)
                    }
                }
            }
            MediaPlayer {
                id: video
                source: model.url_download
                audioRole: MediaPlayer.VideoRole
                Component.onCompleted: {
                    console.log("video source", model.url_download, model.mimetype, availability)
                    if (availability === MediaPlayer.Available) {
                        SlackClient.setMediaSource(this, teamId, model.url_download)
                    }
                }

                onStatusChanged: {
                    console.log("status for " + source + " : " + status)
                    //                    if (status === MediaPlayer.Loaded) {
                    //                        video.play()
                    //                    }
                }
                onErrorChanged: console.log("error for " + source + " : " + error)
                onHasVideoChanged: console.log("media has video", hasVideo)
                onDurationChanged: console.log("media duration", duration)

            }
            VideoOutput {
                id: voutput
                anchors.fill: parent
                source: video
            }
            function clicked() {
                video.seek(0)
                video.play()
            }
        }
    }

    Component {
        id: imageSharedFileComponent
        Item {
            id: imageItem
            property bool expanded: false
            width: expanded ? listView.width - Theme.paddingLarge * 4 : model.thumbSize.width
            height: expanded ? width / (model.size.width / model.size.height) : model.thumbSize.height

            Image {
                id: thumbImage
                anchors.fill: parent
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                source: "team://" + teamId + "/" + model.thumbUrl
                sourceSize: Qt.size(model.thumbSize.width, model.thumbSize.height)
                visible: !imageItem.expanded
            }

            AnimatedImage {
                id: fullImage
                anchors.fill: parent
                //to preserve memory, cache is turned off, so to see animation again need to re-expand image
                //TODO: create settings to change the behavior
                source: imageItem.expanded ? "team://" + teamId + "/" + model.url : ""
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                visible: imageItem.expanded
                playing: imageItem.expanded
                cache: false
                smooth: true
            }

            ProgressBar {
                anchors.centerIn: parent
                opacity: value < 1
                value: imageItem.expanded ? fullImage.progress : thumbImage.progress
                Behavior on opacity { NumberAnimation { duration: 250 } }
            }

            Rectangle {
                anchors.fill: imageMouseArea
                color: Qt.rgba(1, 1, 1, 0.1)
                visible: imageMouseArea.containsMouse
            }

            function clicked() {
                if (SlackClient.isDevice) {
                    pageStack.push(Qt.resolvedUrl("SlackImage.qml"), {"model": model, "teamId": teamId})
                } else {
                    imageItem.expanded = !imageItem.expanded
                }
            }

            MouseArea {
                id: imageMouseArea
                anchors.fill: parent
                hoverEnabled: true
                enabled: false
                cursorShape: imageItem.expanded ? Qt.ArrowCursor : Qt.WhatsThisCursor
            }
        }
    }
}
