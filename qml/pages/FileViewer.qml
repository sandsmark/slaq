import QtQuick 2.11
import QtQuick.Controls 2.4
import Qt.labs.platform 1.0 as Platform

import ".."
import "../components"

Item {
    width: loader.item.width + Theme.paddingMedium
    height: loader.item.height + Theme.paddingMedium

    Platform.FileDialog {
        id: fileSaveDialog
        title: "Please choose file name"
        file: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DownloadLocation) + "/" + model.name
        folder: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DownloadLocation)
        fileMode: Platform.FileDialog.SaveFile
        onAccepted: {
            progressBar.value = 0
            downloadManager.append(model.url_download, file, SlackClient.teamToken(teamId))
        }
    }
    Connections {
        target: downloadManager
        onDownloaded: {
            progressBar.value = progress
        }
    }

    Column {
        anchors.centerIn: parent
        anchors.margins: Theme.paddingMedium/2
        Loader {
            id: loader
            Component.onCompleted: {
                if (model.mimetype.indexOf("video") !== -1) {
                    setSource("qrc:/qml/components/VideoFileViewer.qml")
                } else if (model.mimetype.indexOf("image") !== -1) {
                    setSource("qrc:/qml/components/ImageFileViewer.qml")
                } else if (model.mimetype.indexOf("text") !== -1) {
                    setSource("qrc:/qml/components/TextFileViewer.qml")
                }
            }

            MouseArea {
                id: mouArea
                anchors.fill: parent
                hoverEnabled: true
                z: loader.item.z + 100 //workaround since for Text item
                onContainsMouseChanged: {
                    if (loader.item) {
                        loader.item.hovered(mouArea)
                    }
                }
                onClicked: {
                    if (loader.item) {
                        loader.item.clicked()
                    }
                }
            }
        }

        ProgressBar {
            id: progressBar
            value: 0
            width: loader.item.width
        }
    }

    Control {
        id: overlayControls
        width: loader.item.width
        height: 20
        hoverEnabled: true
        anchors {
            top: parent.top
            left: parent.left
            topMargin: Theme.paddingMedium/2
            leftMargin: Theme.paddingMedium/2
        }
        visible: mouArea.containsMouse || hovered
        Row {
            anchors.fill: parent
            anchors.margins: Theme.paddingMedium/2
            spacing: 5
            Button {
                text: "\u21E9"
                onClicked: fileSaveDialog.open()
            }
        }
    }
}
