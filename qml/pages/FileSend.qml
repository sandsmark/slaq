import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.3

import com.iskrembilen.slaq 1.0 as Slack

import ".."

Page {
    id: page

    property variant channelId
    padding: Theme.paddingLarge
    property string selectedImage: ""
    property bool uploading: false

    title: qsTr("Upload image")

    Flickable {
        anchors.fill: parent
        contentWidth: column.width
        contentHeight: column.height

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingMedium

            Image {
                id: image
                x: page.padding
                width: parent.width - Theme.paddingLarge * 2
                fillMode: Image.PreserveAspectFit
                source: page.selectedImage
                visible: page.selectedImage.length > 0
            }

            Button {
                text: qsTr("Select image")
                visible: !image.visible
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    fileDialog.open()
                }
            }

            TextField {
                id: titleInput
                x: page.padding
                width: parent.width - Theme.paddingLarge * 2
                placeholderText: "Title (optional)"
                onAccepted: commentInput.focus = true
            }

            TextField {
                id: commentInput
                x: page.padding
                width: parent.width - Theme.paddingLarge * 2
                placeholderText: "Comment (optional)"
                onAccepted: focus = false
            }

            Button {
                text: qsTr("Send")
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: image.visible
                visible: !page.uploading
                onClicked: {
                    page.uploading = true
                    Slack.Client.postImage(channelId, page.selectedImage, titleInput.text, commentInput.text)
                }
            }

            ProgressBar {
                width: parent.width
                indeterminate: true
                visible: page.uploading
            }

            Spacer {
                height: Theme.paddingLarge
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose an image file"
        folder: shortcuts.home
        visible: false
        onAccepted: {
            page.selectedImage = fileUrl
        }
        onRejected: {
            page.selectedImage = fileUrl
        }
        Component.onCompleted: visible = true
    }


    Component.onCompleted: {
        Slack.Client.onPostImageFail.connect(handleImagePostFail)
        Slack.Client.onPostImageSuccess.connect(handleImagePostSuccess)
    }

    Component.onDestruction: {
        Slack.Client.onPostImageFail.disconnect(handleImagePostFail)
        Slack.Client.onPostImageSuccess.disconnect(handleImagePostSuccess)
    }

    function handleImagePostFail() {
        console.log("post image fail")
    }

    function handleImagePostSuccess() {
        console.log("post image success")
        pageStack.pop()
    }
}
