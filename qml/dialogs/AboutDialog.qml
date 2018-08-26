import QtQuick 2.11
import QtQuick.Window 2.3
import QtQuick.Controls 2.4
import "../pages"
import ".."

Dialog {
    id: aboutDialog
    x: Math.round((window.width - width) / 2)
    y: Math.round(window.height / 6)
    width: Math.round(Math.min(window.width, window.height) / 3 * 2)
    height: Math.round(Math.min(window.width, window.height) / 3 * 2)
    modal: true
    focus: true

    standardButtons: Dialog.Ok

    title: qsTr("About")

    Flickable {
        anchors.fill: parent

        contentWidth: column.width
        contentHeight: column.height

        Column {
            id: column

            width: aboutDialog.width
            spacing: Theme.paddingLarge

            Label {
                x: aboutDialog.padding
                text: "Slaq 1.3"
                font.pointSize: Theme.fontSizeHuge
            }

            Label {
                x: aboutDialog.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                wrapMode: Text.Wrap
                text: qsTr("Unoffical Slack client which doesn't eat all your RAM")
                font.pointSize: Theme.fontSizeSmall
            }

            Label {
                x: aboutDialog.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                wrapMode: Text.Wrap
                font.pointSize: Theme.fontSizeSmall
                text: qsTr("Browse channel and post new messages. Channels are updated real-time when new messages are posted.")
            }

            RichTextLabel {
                x: aboutDialog.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                font.pointSize: Theme.fontSizeSmall
                value: qsTr("Source code and issues in <a href='%1'>Github</a>.").arg("https://github.com/sandsmark/slaq")
                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }
}
