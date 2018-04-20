import QtQuick 2.10
import QtQuick.Window 2.2
import QtQuick.Controls 2.2
import ".."

Page {
    id: page

    property double padding: Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 2 : 1)

    header: Label {
        text: qsTr("About")
    }

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
                text: "Slackfish 1.3"
                font.pixelSize: Theme.fontSizeExtraLarge
            }

            Label {
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                wrapMode: Text.Wrap
                text: qsTr("Unoffical Slack client for Sailfish OS")
                font.pixelSize: Theme.fontSizeSmall
            }

            Label {
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Browse channel and post new messages. Channels are updated real-time when new messages are posted.")
            }

            RichTextLabel {
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                font.pixelSize: Theme.fontSizeSmall
                value: qsTr("Source code and issues in <a href='%1'>Github</a>.").arg("https://github.com/markussammallahti/harbour-slackfish")
                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }
}
