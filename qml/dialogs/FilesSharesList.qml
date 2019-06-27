import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import Qt.labs.platform 1.0 as Platform

import ".."
import "../pages"
import "../components"

import com.iskrembilen 1.0

Drawer {
    id: filesSharesDialog

    y: Theme.paddingMedium
    bottomMargin: Theme.paddingMedium

    edge: Qt.RightEdge
    width: 0.5 * window.width
    height: window.height - Theme.paddingMedium
    property int currentPage: -1
    property int totalPages: -1
    property string teamId: teamsSwipe.currentItem !== null ? teamsSwipe.currentItem.item.teamId : ""
    property User selfUser: null
    onTeamIdChanged: listView.model = undefined

    function fetchData() {
        if (radioGroup.checkedButton === allFilesButton) {
            listView.model.retreiveFilesFor("", "")
        } else if (radioGroup.checkedButton === myFilesButton) {
            listView.model.retreiveFilesFor("", selfUser.userId)
        } else if (radioGroup.checkedButton === channelFilesButton) {
            listView.model.retreiveFilesFor(teamsSwipe.currentItem.item.currentChannelId, "")
        }
    }

    onOpened: {
        selfUser = SlackClient.selfUser(teamId)
        if (listView.model === undefined) {
            listView.model = SlackClient.getFilesSharesModel(teamId)
            fetchData()
        }
    }

    Platform.FileDialog {
        id: fileSaveDialog
        title: "Please choose file name"
        property string fileName: ""
        property url downloadUrl: ""

        file: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DownloadLocation) + "/" + fileName
        fileMode: Platform.FileDialog.SaveFile
        currentFiles: [fileName]
        folder: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DownloadLocation)
        onAccepted: {
            progressBar.value = 0
            downloadManager.append(downloadUrl, file, SlackClient.teamToken(teamId))
        }
    }

    Connections {
        target: downloadManager
        onDownloaded: {
            console.log("downloading", progress, url.toString(), fileSaveDialog.downloadUrl)
            if (url === fileSaveDialog.downloadUrl) {
                progressBar.value = progress
            }
        }
    }

    ButtonGroup {
        id: radioGroup
        onCheckedButtonChanged: {
            if (filesSharesDialog.opened) {
                fetchData()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.paddingSmall
        spacing: Theme.paddingMedium

        RowLayout {
            Layout.fillWidth: true
            spacing: 0
            Button {
                id: allFilesButton
                Layout.fillWidth: true
                checkable: true
                ButtonGroup.group: radioGroup
                text: qsTr("All files")
            }
            Button {
                id: myFilesButton
                Layout.fillWidth: true
                checkable: true
                ButtonGroup.group: radioGroup
                checked: true
                text: qsTr("My files")
            }
            Button {
                id: channelFilesButton
                Layout.fillWidth: true
                checkable: true
                ButtonGroup.group: radioGroup
                text: qsTr("Channel files")
            }
        }

        ProgressBar {
            id: progressBar
            value: 0
            Layout.fillWidth: true
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollIndicator.vertical: ScrollIndicator { }
            verticalLayoutDirection: ListView.BottomToTop
            clip: true
            spacing: Theme.paddingSmall
            delegate: ItemDelegate {
                id: delegate
                padding: Theme.paddingTiny
                width: listView.width - listView.ScrollIndicator.vertical.width
                height: Theme.avatarSize
                property FileShare fileShare: model.FileShareObject
                hoverEnabled: true
                spacing: 0
                enabled: fileShare != null && fileShare != undefined
                visible: enabled

                contentItem: RowLayout {
                    height: delegate.availableHeight
                    width: delegate.availableWidth

                    Image {
                        sourceSize: Qt.size(delegate.availableHeight, delegate.availableHeight)
                        width: delegate.availableHeight
                        height: delegate.availableHeight
                        source: fileShare.thumb_64.toString().length > 0 ?
                                    "team://" + teamId + "/" + fileShare.thumb_64 :
                                    SlackClient.resourceForFileType(fileShare.filetype, fileShare.name)
                        RoundButton {
                            id: downloadButton
                            hoverEnabled: true
                            visible: delegate.hovered
                            anchors.fill: parent
                            padding: 12
                            font.pixelSize: Theme.fontSizeLarge
                            text: "\u21E9"
                            onClicked: {
                                fileSaveDialog.fileName = fileShare.name
                                fileSaveDialog.downloadUrl = fileShare.url_private_download
                                fileSaveDialog.open()
                            }
                        }
                    }

                    ColumnLayout {
                        height: parent.height
                        Layout.fillWidth: true

                        RowLayout {
                            Label {
                                wrapMode: Text.Wrap
                                Layout.fillWidth: !name.visible
                                text: fileShare.title
                            }
                            Label {
                                id: name
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                visible: fileShare.title !== fileShare.name
                                text: "[" + fileShare.name + "]."
                            }
                        }
                        RowLayout {
                            Label {
                                font.pixelSize: Theme.fontSizeSmall
                                text: qsTr("Created by")
                            }

                            Image {
                                source: fileShare.user.avatarUrl
                                sourceSize: Qt.size(delegate.availableHeight/2, delegate.availableHeight/2)
                            }
                            Label {
                                font.pixelSize: Theme.fontSizeSmall
                                text: "@" + fileShare.user.username
                                color: palette.link
                            }
                            Label {
                                font.pixelSize: Theme.fontSizeSmall
                                text: " at " + Qt.formatDateTime(fileShare.created, "dd MMM yyyy hh:mm")
                            }
                            Label {
                                wrapMode: Text.Wrap
                                text: qsTr("Size: ") + fileShare.size + qsTr(" bytes")
                            }
                        }
                    }
                    EmojiRoundButton {
                        id: trashButton
                        visible:  (selfUser != null && selfUser.userId === fileShare.user.userId)
                        text: "\uD83D\uDDD1"
                        font.pixelSize: Theme.fontSizeLarge
                        onClicked: {
                            SlackClient.deleteFile(teamId, fileShare.id)
                        }
                    }
                }
            }
        }
    }
}
