import QtQuick 2.10
import QtQuick.Controls 2.2
import ".."
import com.iskrembilen.slaq 1.0 as Slack
import "../Channel.js" as Channel

Page {
    id: page

    Flickable {
        anchors.fill: parent

        Label {
            id: header
            text: qsTr("Join channel")
        }

        Text {
            enabled: listView.count === 0
            text: qsTr("No available channels")
        }

        ListView {
            id: listView
            spacing: Theme.paddingMedium

            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            ScrollBar.vertical: ScrollBar { }

            // Prevent losing focus from search field
            currentIndex: -1

            header: TextField {
                id: searchField
                width: parent.width
                focus: true
                placeholderText: qsTr("Search")

                onTextChanged: {
                    channelListModel.update()
                }
            }

            model: ListModel {
                id: channelListModel

                property var available: Slack.Client.getChannels().filter(Channel.isJoinableChannel)

                Component.onCompleted: update()

                function matchesSearch(channel) {
                    return listView.headerItem.text === "" || channel.name.toLowerCase().indexOf(listView.headerItem.text.toLowerCase()) >= 0
                }

                function update() {
                    var channels = available.filter(matchesSearch)
                    channels.sort(Channel.compareByName)

                    clear()
                    channels.forEach(function(c) {
                        append(c)
                    })
                }
            }

            delegate: MouseArea {
                id: delegate
                height: row.height + Theme.paddingLarge
                property color textColor: delegate.highlighted ? palette.highlight : palette.alternateBase

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
                        font.pointSize: Theme.fontSizeMedium
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
}
