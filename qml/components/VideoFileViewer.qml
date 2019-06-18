import QtQuick 2.11
import QtMultimedia 5.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import Qt.labs.platform 1.0 as Platform
import ".."
Item {
    id: videoItem
    Image {
        id: thumbVideoImage
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: fileshare.thumb_video.toString().length > 0 ?
                    "team://" + teamId + "/" + fileshare.thumb_video :
                    "qrc:/icons/video-thumbnail.png"
        visible: video.playbackState === MediaPlayer.StoppedState
        onStatusChanged: {
            if (status === Image.Ready) {
                videoItem.width = sourceSize.width - Theme.paddingLarge * 4  > 360 ?
                            360 : sourceSize.width
                videoItem.height = width / (sourceSize.width / sourceSize.height)
            }
        }
        onSourceChanged: console.log("thumb video source:" + source + ":")
    }

    Connections {
        target: downloadManager
        onFinished: {
            if (url === fileshare.url_private_download) {
                console.log("set temp source", url, fileName)
                video.source = "file://"+fileName
                playButton.enabled = true
            }
        }
    }

    MediaPlayer {
        id: video
        audioRole: MediaPlayer.VideoRole
        onPositionChanged: slider.value = position
        onDurationChanged: {
            console.log("duration", duration)
            if (duration > 0) {
                slider.enabled = true
                slider.to = duration
            } else {
                slider.enabled = false
            }
        }
        onErrorChanged: {
            console.error("error for " + source + " : " + error + " playback state: " + playbackState)
            if (error === MediaPlayer.ResourceError) {
                errorDialog.showError("Media player", "Error playing video. Please, download the video and play locally")
                //fallback to temporary file download
                //TODO: disabled temporarily
//                playButton.enabled = false
//                downloadManager.append(fileshare.url_private_download,
//                                       Platform.StandardPaths.writableLocation(
//                                           Platform.StandardPaths.TempLocation)
//                                       + "/" + fileshare.name,
//                                       SlackClient.teamToken(teamId))
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.paddingMedium/2
        VideoOutput {
            id: voutput
            Layout.fillHeight: true
            Layout.fillWidth: true
            source: video
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 5
            Slider {
                id: slider
                Layout.fillWidth: true
                from: 0
                onPositionChanged: video.seek(value)
            }

            Button {
                id: playButton
                text: video.playbackState === MediaPlayer.PlayingState ? "▯▯" : "▷"
                onClicked: videoItem.clicked()
            }
        }
    }

    function clicked() {
        if (video.playbackState !== MediaPlayer.PlayingState) {
            if (video.playbackState === MediaPlayer.StoppedState) {
                SlackClient.setMediaSource(video, teamId, fileshare.url_private_download)
            }
            video.play()
        } else {
            video.pause()
        }
    }
    function hovered(isHovered) {}
}
