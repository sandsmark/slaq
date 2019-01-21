import QtQuick 2.11
import QtQuick.Window 2.3
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3 as QQDialogs

import com.iskrembilen 1.0
import "../pages"
import "../components"
import ".."

LazyLoadDialog {
    id: dialog
    property string teamId: ""
    property User user: null

    sourceComponent: Dialog {
        id: userDialog
        clip: true

        property string selectedFile: ""
        property bool userDataChanged: false

        x: Math.round((window.width - width) / 2)
        y: Math.round(window.height / 6)
        width: Math.round(Math.min(window.width, window.height) / 3 * 2)
        height: Math.round(Math.min(window.width, window.height) / 3 * 2)
        modal: true
        focus: true

        property int crop_x: 0
        property int crop_y: 0
        property int crop_w: 0
        property int x_left: 0
        property int x_right: 0
        property int y_down: 0
        property int y_up: 0

        function calculateAvatar() {
            crop_x = x_left - avatar.start_x
            crop_w = x_right - x_left
            crop_y = avatar.start_y + y_up
        }

        onAboutToHide: {
            userDataChanged = false
            selectedFile = ""
        }

        Connections {
            target: emojiSelector
            onEmojiSelected: {
                if (emojiSelector.state === "userdialog" && emojiSelector.callerTeam === teamId && emoji !== "") {
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
                        calculateAvatar()
                        SlackClient.updateUserAvatar(teamId, selectedFile, crop_w, crop_x, crop_y)
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
            height: userDialog.availableHeight - userDialog.footer.height - userDialog.padding
            enabled: user != null

            contentWidth: column.width
            contentHeight: column.height
            clip: true
            ScrollIndicator.vertical: ScrollIndicator{}
            ColumnLayout {
                id: column
                width: userDialog.availableWidth - Theme.paddingMedium*2
                x: Theme.paddingMedium
                spacing: Theme.paddingLarge

                Image {
                    id: avatar
                    property int start_x: 0
                    property int start_y: 0
                    Layout.alignment: Qt.AlignHCenter
                    source: selectedFile === "" ? "image://emoji/slack/" + user.avatarUrl : selectedFile
                    Layout.minimumWidth: (userDialog.availableWidth - Theme.paddingMedium*2) / 2
                    Layout.minimumHeight: (userDialog.availableWidth - Theme.paddingMedium*2) / 2
                    fillMode: Image.PreserveAspectFit
                    onStatusChanged: {
                        if (avatar.status == Image.Ready) {
                            start_x = (width-paintedWidth)/2
                            start_y = (height-paintedHeight)/2
                            y_down = start_y + paintedHeight
                            y_up = start_y
                            x_left = start_x
                            x_right = x_left + paintedWidth
                            vertSlider.setValues((1.0 - y_down/height), (1.0 - y_up/height))
                        }
                    }
                    Rectangle { color: "gray"; width: parent.width; height: 2; y: y_down }
                    Rectangle { color: "gray"; width: parent.width; height: 2; y: y_up }
                    Rectangle { color: "gray"; width: 2; height: parent.height; x: x_left }
                    Rectangle { color: "gray"; width: 2; height: parent.height; x: x_right }

                    RangeSlider {
                        id: vertSlider
                        orientation: Qt.Vertical
                        anchors.left: parent.right
                        height: parent.height
                        first.onPositionChanged: {
                            if (first.pressed) {
                                y_down = avatar.height*(1.0 - first.value)
                                x_right = y_down
                            }

                        }
                        second.onPositionChanged: {
                            if (second.pressed) {
                                y_up = avatar.height*(1.0 - second.value)
                                x_left = y_up
                            }
                        }
                    }
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: "lightgray"
                        border.width: 2
                        visible: mouseArea.containsMouse
                        Text {
                            color: "#a2a2a2"
                            anchors.centerIn: parent
                            font.bold: true
                            style: Text.Outline
                            text: qsTr("Click to load new")
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
                    Rectangle {
                        id: dragMe
                        x: x_left
                        y: y_up
                        width: x_right - x_left
                        height: y_down - y_up
                        color: "#80b59191"
                        onXChanged: {
                            if (dragMouse.drag.active) {
                                var _w = width
                                x_left = x
                                x_right = x + _w
                            }
                        }
                        onYChanged: {
                            if (dragMouse.drag.active) {
                                var _h = height
                                y_up = y
                                y_down = y + _h
                                vertSlider.setValues(1.0 - y_down/avatar.height, 1.0 - y_up/avatar.height)
                            }
                        }

                        MouseArea {
                            id: dragMouse
                            anchors.fill: parent
                            preventStealing: true
                            propagateComposedEvents: true
                            drag.target: dragMe
                            drag.axis: Drag.XAndYAxis
                            drag.minimumX: avatar.start_x
                            drag.minimumY: avatar.start_y
                            drag.maximumX: avatar.start_x + avatar.paintedWidth - dragMe.width
                            drag.maximumY: avatar.start_y + avatar.paintedHeight - dragMe.height
                        }
                    }
                }
                TextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("First name...")
                    text: user.firstName
                    //readOnly: true
                    onTextChanged: {
                        if (text != user.firstName) {
                            userDataChanged = true
                            user.firstName = text
                        }
                    }
                }
                TextField {
                    Layout.fillWidth: true
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
                    Layout.fillWidth: true
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
                        user: dialog.user
                        showNoImage: true
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                emojiSelector.x = mapToGlobal(x, y).x
                                emojiSelector.y = mapToGlobal(x, y).y - emojiSelector.height - parent.height
                                emojiSelector.state = "userdialog"
                                emojiSelector.callerTeam = teamId
                                emojiSelector.open()
                            }
                        }
                    }
                }
            }
        }
    }
}
