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
        source: fileshare.thumb_video != "" ? "team://" + teamId + "/" + fileshare.thumb_video : "qrc:/icons/video-thumbnail.png"
        visible: video.playbackState === MediaPlayer.StoppedState
        onStatusChanged: {
            if (status === Image.Ready) {
                videoItem.width = sourceSize.width - Theme.paddingLarge * 4  > 360 ? 360 : sourceSize.width
                videoItem.height = width / (sourceSize.width / sourceSize.height)
            }
        }
        onSourceChanged: console.log("thumb video source:" + fileshare.thumb_video + ":")
    }

    MediaPlayer {
        id: video
        source: fileshare.url_private_download
        audioRole: MediaPlayer.VideoRole
        Component.onCompleted: {
            if (availability === MediaPlayer.Available) {
                SlackClient.setMediaSource(this, teamId, fileshare.url_private_download)
            }
        }

        onPositionChanged: slider.value = position
        onStatusChanged: {
            if (status === MediaPlayer.Loaded) {
                slider.to = duration
            }
        }
        onErrorChanged: console.error("error for " + source + " : " + error)

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
                text: video.playbackState === MediaPlayer.PlayingState ? "▯▯" : "▷"
                onClicked: video.playbackState !== MediaPlayer.PlayingState ?
                               video.play() : video.pause()
            }
        }
    }
    function clicked() {
        if (video.playbackState !== MediaPlayer.PlayingState) {
            video.play()
        } else {
            video.pause()
        }
    }
    function hovered(isHovered) {}
}
