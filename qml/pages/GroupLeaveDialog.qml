import QtQuick 2.2
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property string name

    property double padding: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)

    Column {
        width: parent.width

        DialogHeader {
            title: qsTr("Leave %1").arg(dialog.name)
        }

        Label {
            x: dialog.padding
            width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            text: qsTr("If you leave the private channel, you will no longer be able to see any of its messages. To rejoin the private channel, you will have to be re-invited.

Are you sure you wish to leave?")
            color: Theme.primaryColor
        }
    }
}
