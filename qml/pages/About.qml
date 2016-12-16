import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property double padding: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)

    SilicaFlickable {
        anchors.fill: parent

        contentWidth: column.width
        contentHeight: column.height

        Column {
            id: column

            width: page.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("About")
            }

            Label {
                x: page.padding
                text: "Slackfish 1.2"
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeExtraLarge
            }

            Label {
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                wrapMode: Text.Wrap
                text: qsTr("Unoffical Slack client for Sailfish OS")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
            }

            Label {
                x: page.padding
                width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Browse channel and post new messages. Channels are updated real-time when new messages are posted.")
                color: Theme.primaryColor
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
