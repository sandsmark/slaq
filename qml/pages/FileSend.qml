import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3 as QQDialogs
import ".."

Dialog {
    id: page

    title: qsTr("Sending file...")
    property string teamId
    property string channelId
    property string selectedFile: ""
    property bool uploading: false
    width: window.isMobile ? window.width : window.width / 2
    height: window.isMobile ? window.height : window.height / 2
    x: (window.width - width)/2
    y: (window.height - height)/2

    footer: DialogButtonBox {
        Button {
            text: qsTr("Select file")
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                fileDialog.open()
            }
        }
        Button {
            text: qsTr("Send")
            enabled: !page.uploading && selectedFile !== ""
            //DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                page.uploading = true
                SlackClient.postFile(teamId, channelId, page.selectedFile, titleInput.text, commentInput.text)
            }
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }

        onRejected: {
            close()
        }
    }

    modal: true

    contentItem: Flickable {
        width: page.availableWidth
        height: page.availableHeight
        anchors.margins: 5
        contentWidth: column.width
        contentHeight: column.height
        clip: true
        Column {
            id: column
            width: page.width
            spacing: Theme.paddingMedium

            Label {
                width: parent.width
                text: qsTr("Cant preview file: ") + page.selectedFile
                visible: image.status == Image.Error
            }

            Image {
                id: image
                width: parent.width
                fillMode: Image.PreserveAspectFit
                source: page.selectedFile
                visible: page.selectedFile !== ""
            }

            TextField {
                id: titleInput
                width: parent.width
                placeholderText: "Title (optional)"
                onAccepted: commentInput.focus = true
            }

            TextField {
                id: commentInput
                width: parent.width
                placeholderText: "Comment (optional)"
                onAccepted: focus = false
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

    QQDialogs.FileDialog {
        id: fileDialog
        title: "Please choose an image file"
        folder: shortcuts.home
        visible: false
        onAccepted: {
            page.selectedFile = fileUrl
        }
        onRejected: {
            page.selectedFile = fileUrl
        }
    }

    Component.onCompleted: {
        SlackClient.onPostFileFail.connect(handleFilePostFail)
        SlackClient.onPostFileSuccess.connect(handleFilePostSuccess)
    }

    Component.onDestruction: {
    }

    function handleFilePostFail() {
        console.warn("post image fail")
    }

    function handleFilePostSuccess() {
        close()
    }
}

