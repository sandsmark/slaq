import QtQuick 2.2
import QtQuick.Controls 2.2
import ".."
//import com.iskrembilen.slaq 1.0 as Slack
import "../Channel.js" as Channel

Page {
    id: page

    Flickable {
        anchors.fill: parent

        Label {
            id: header
            text: listView.count !== 0 ? qsTr("Open chat") : qsTr("No available chats")
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

                property var available: SlackClient.getChannels().filter(Channel.isJoinableChat)

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

            delegate: ItemDelegate {
                id: delegate
                icon: Channel.getIcon(model)
                text: model.name

                onClicked: {
                    SlackClient.openChat(model.id)
                    pageStack.replace(Qt.resolvedUrl("Channel.qml"), {"channelId": model.id})
                }
            }
        }
    }

    ConnectionPanel {}
}
