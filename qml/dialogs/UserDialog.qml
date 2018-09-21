import QtQuick 2.11
import QtQuick.Window 2.3
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3 as QQDialogs

import com.iskrembilen 1.0
import "../pages"
import "../components"
import ".."

Dialog {
    id: userDialog

    property string teamId: ""
    property string selectedFile: ""
    property bool userDataChanged: false

    x: Math.round((window.width - width) / 2)
    y: Math.round(window.height / 6)
    width: Math.round(Math.min(window.width, window.height) / 3 * 2)
    height: Math.round(Math.min(window.width, window.height) / 3 * 2)
    modal: true
    focus: true
    property User user: null

    onAboutToHide: {
        userDataChanged = false
        selectedFile = ""
    }

    Connections {
        target: emojiSelector
        onEmojiSelected: {
            if (emojiSelector.state === "userdialog" && emoji !== "") {
                console.log("selected emoji", emoji, ImagesCache.getNameByEmoji(emoji))
                user.statusEmoji = ":"+ImagesCache.getNameByEmoji(emoji)+":"
                userDataChanged = true
            }
        }
    }

    title: qsTr("User info: ") + user.username

    QQDialogs.FileDialog {
        id: fileDialog
        title: "Please choose an image file"
        visible: false
        nameFilters: [ "Image files (*.jpg *.png *.jpeg *.gif)" ]
        onAccepted: {
            selectedFile = fileUrl
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Save")
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            enabled: selectedFile !== "" || userDataChanged
            onClicked: {
                if (userDataChanged) {
                    SlackClient.updateUserInfo(teamId, user)
                }

                if (selectedFile !== "") {
                    SlackClient.updateUserAvatar(teamId, selectedFile)
                }
                close()
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

    Flickable {
        width: userDialog.availableWidth
        height: userDialog.availableHeight
        enabled: user != null

        contentWidth: column.width
        contentHeight: column.height
        clip: true
        ScrollIndicator.vertical: ScrollIndicator{}
        Column {
            id: column
            width: userDialog.availableWidth - Theme.paddingMedium*2
            x: Theme.paddingMedium
            spacing: Theme.paddingLarge
            Image {
                id: avatar
                x: (parent.width - width)/2
                source: selectedFile === "" ? "image://emoji/slack/" + user.avatarUrl : selectedFile
                width: parent.width / 2
                height: parent.width / 2
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: "lightgray"
                    border.width: 2
                    visible: mouseArea.containsMouse
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Click me")
                    }
                }
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        fileDialog.open()
                    }
                }
            }
            TextField {
                width: parent.width
                placeholderText: qsTr("First name...")
                text: user.firstName
                readOnly: true
                onTextChanged: {
                    if (text != user.firstName) {
                        userDataChanged = true
                        user.firstName = text
                    }
                }
            }
            TextField {
                width: parent.width
                placeholderText: qsTr("Last name...")
                text: user.lastName
                readOnly: true
                onTextChanged: {
                    if (text != user.lastName) {
                        userDataChanged = true
                        user.lastName = text
                    }
                }
            }
            TextField {
                width: parent.width
                placeholderText: qsTr("E-mail...")
                text: user.email
                readOnly: true
                onTextChanged: {
                    if (text != user.email) {
                        userDataChanged = true
                        user.email = text
                    }
                }
            }
            RowLayout {
                width: parent.width
                spacing: Theme.paddingMedium
                TextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Status...")
                    text: user.status
                    onTextChanged: {
                        if (text != user.status) {
                            userDataChanged = true
                            user.status = text
                        }
                    }
                }
                UserStatusEmoji {
                    Layout.minimumWidth: 32
                    Layout.minimumHeight: 32
                    user: userDialog.user
                    showNoImage: true
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            emojiSelector.x = mapToGlobal(x, y).x
                            emojiSelector.y = mapToGlobal(x, y).y - emojiSelector.height - parent.height
                            emojiSelector.state = "userdialog"
                            emojiSelector.open()
                        }
                    }
                }
            }
        }
    }
}
