import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.slackfish 1.0 as Slack

Page {
    id: page

    property variant channelId
    property double padding: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)
    property string selectedImage: ""
    property bool uploading: false

    SilicaFlickable {
        anchors.fill: parent
        contentWidth: column.width
        contentHeight: column.height

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Upload image")
            }

            Image {
                id: image
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                fillMode: Image.PreserveAspectFit
                source: page.selectedImage
                visible: page.selectedImage.length > 0
            }

            Button {
                text: qsTr("Select image")
                visible: !image.visible
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    var dialog = pageStack.push(Qt.resolvedUrl("../dialogs/ImagePicker.qml"))
                    dialog.accepted.connect(function() {
                        page.selectedImage = dialog.selectedPath
                    })
                }
            }

            TextField {
                id: titleInput
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                label: "Title (optional)"
                placeholderText: label
                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: commentInput.focus = true
            }

            TextField {
                id: commentInput
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                label: "Comment (optional)"
                placeholderText: label
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
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
                label: qsTr("Uploading")
                indeterminate: true
                visible: page.uploading
            }

            Spacer {
                height: Theme.paddingLarge
            }
        }
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
