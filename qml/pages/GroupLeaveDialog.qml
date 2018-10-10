import QtQuick 2.11
import QtQuick.Controls 2.4

import ".."

Dialog {
    id: dialog

    property string name
    property string question: qsTr("Are you sure you wish to leave ") + name + "?"
    standardButtons: Dialog.Ok | Dialog.Cancel

    x: (window.width - dialog.width)/2
    y: (window.height - dialog.height)/2
    title: qsTr("Group leave")

    modal: true
    contentItem: Rectangle {
        color: palette.base
        Label {
            text: question
        }
    }
}
