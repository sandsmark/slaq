import QtQuick 2.12
import QtMultimedia 5.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt.labs.platform 1.0 as Platform
import ".."
Item {
    id: videoItem
    property url previewThumb
    property url videoUrl
    // set if playbacks from Slack servers, which requires extra headers
    property bool directPlay: false
    property bool adjustThumbSize: true
    Image {
        id: thumbVideoImage
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: previewThumb
        visible: video.playbackState === MediaPlayer.StoppedState && previewThumb.toString().length > 0
        onStatusChanged: {
            if (status === Image.Ready && videoItem.adjustThumbSize) {
                videoItem.width = sourceSize.width - Theme.paddingLarge * 4  > 360 ?
                            360 : sourceSize.width
                videoItem.height = width / (sourceSize.width / sourceSize.height)
            }
        }
        onSourceChanged: console.log("thumb video source:" + source)
    }

    Connections {
        target: downloadManager
        onFinished: {
            if (url === videoUrl) {
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
                enabled: videoUrl.toString().length > 0
                text: video.playbackState === MediaPlayer.PlayingState ? "▯▯" : "▷"
                onClicked: videoItem.clicked()
            }
        }
    }

    function clicked() {
        if (video.playbackState !== MediaPlayer.PlayingState) {
            if (video.playbackState === MediaPlayer.StoppedState) {
                if (directPlay)
                    video.source = videoUrl
                else
                    SlackClient.setMediaSource(video, teamId, videoUrl)
            }
            video.play()
        } else {
            video.pause()
        }
    }
    function hovered(isHovered) {}
}
