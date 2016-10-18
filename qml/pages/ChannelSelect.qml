import QtQuick 2.2
import Sailfish.Silica 1.0
import harbour.slackfish 1.0 as Slack
import "Channel.js" as Channel

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent

        SilicaListView {
            id: listView
            spacing: Theme.paddingMedium
            anchors.fill: parent

            VerticalScrollDecorator {}

            ViewPlaceholder {
                enabled: listView.count === 0
                text: "No available channels"
            }

            header: PageHeader {
                title: qsTr("Join channel")
            }

            model: ListModel {
                id: channelListModel
            }

            delegate: BackgroundItem {
                id: delegate
                height: row.height + Theme.paddingLarge
                property color textColor: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

                Row {
                    id: row
                    width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                    anchors.verticalCenter: parent.verticalCenter
                    x: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)
                    spacing: Theme.paddingMedium

                    Image {
                        id: icon
                        source: "image://theme/" + Channel.getIcon(model) + "?" + textColor
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Label {
                        width: parent.width - icon.width - Theme.paddingMedium
                        wrapMode: Text.Wrap
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: Theme.fontSizeMedium
                        text: model.name
                        color: textColor
                    }
                }

                onClicked: {
                    Slack.Client.joinChannel(model.id)
                    pageStack.replace(Qt.resolvedUrl("Channel.qml"), {"channelId": model.id})
                }
            }
        }
    }

    ConnectionPanel {}

    Component.onCompleted: {
        var channels = Slack.Client.getChannels().filter(Channel.isJoinableChannel)
        channels.sort(Channel.compareByName)
        channels.forEach(function(c) {
            channelListModel.append(c)
        })
    }
}
