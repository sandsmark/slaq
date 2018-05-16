import QtQuick 2.8
import QtQuick.Window 2.3
import QtQuick.Controls 2.3
import ".."

Page {
    id: page

    title: qsTr("About")

    Flickable {
        anchors.fill: parent

        contentWidth: column.width
        contentHeight: column.height

        Column {
            id: column

            width: page.width
            spacing: Theme.paddingLarge

            Label {
                x: page.padding
                text: "Slaq 1.3"
                font.pointSize: Theme.fontSizeHuge
            }

            Label {
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                wrapMode: Text.Wrap
                text: qsTr("Unoffical Slack client which doesn't eat all your RAM")
                font.pointSize: Theme.fontSizeSmall
            }

            Label {
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                wrapMode: Text.Wrap
                font.pointSize: Theme.fontSizeSmall
                text: qsTr("Browse channel and post new messages. Channels are updated real-time when new messages are posted.")
            }

            RichTextLabel {
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                font.pointSize: Theme.fontSizeSmall
                value: qsTr("Source code and issues in <a href='%1'>Github</a>.").arg("https://github.com/sandsmark/slaq")
                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }
}
