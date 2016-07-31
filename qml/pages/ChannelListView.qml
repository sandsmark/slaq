import QtQuick 2.0
import Sailfish.Silica 1.0
import "Settings.js" as Settings
import "ChannelList.js" as ChannelList

SilicaListView {
    id: listView
    spacing: Theme.paddingMedium

    function reload() {
        ChannelList.reloadChannels()
    }

    VerticalScrollDecorator {}

    header: PageHeader {
        title: Settings.getUserInfo().teamName
    }

    model: ListModel {
        id: channelListModel
    }

    section {
        property: "category"
        criteria: ViewSection.FullString
        delegate: SectionHeader {
            text: getSectionName(section)
        }
    }

    delegate: BackgroundItem {
        id: delegate
        height: row.height + Theme.paddingLarge
        property color infoColor: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        property color textColor: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
        property color currentColor: model.unreadCount > 0 ? textColor : infoColor

        Row {
            id: row
            width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
            anchors.verticalCenter: parent.verticalCenter
            x: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)
            spacing: Theme.paddingMedium

            Image {
                id: icon
                source: "image://theme/" + getChannelIcon(model) + "?" + (delegate.highlighted ? currentColor : getChannelIconColor(model, currentColor))
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                width: parent.width - icon.width - Theme.paddingMedium
                wrapMode: Text.Wrap
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: Theme.fontSizeMedium
                font.bold: model.unreadCount > 0
                text: model.name
                color: currentColor
            }
        }

        onClicked: {
            pageStack.push(Qt.resolvedUrl("Channel.qml"), {"channel": model})
        }
    }

    Component.onCompleted: {
        ChannelList.init()
    }

    function getChannelIcon(model) {
        switch (model.type) {
            case "mpim":
            case "channel":
                return "icon-s-group-chat"
            case "group":
                return "icon-s-secure"
            case "im":
                return "icon-s-chat"
        }
    }

    function getChannelIconColor(model, color) {
        switch (model.presence) {
            case "active":
                return "lawngreen"

            case "away":
                return "lightgrey"

            default:
                return color
        }
    }

    function getSectionName(category) {
        switch (category) {
            case "channel":
                return qsTr("Channels")

            case "chat":
                return qsTr("Direct messages")
        }
    }
}
