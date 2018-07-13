import QtQuick 2.2
import QtQuick.Controls 2.2
import com.iskrembilen 1.0

import ".."

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

                onTextChanged: {}
            }

            model: teamRoot.slackClient.currentChatsModel()

            delegate: ItemDelegate {
                id: delegate
                icon.name: {
                    switch (model.Type) {
                    case ChatsModel.Channel:
                        if (model.presence === "active") {
                            return "irc-channel-active"
                        } else {
                            return "irc-channel-inactive"
                        }
                    case ChatsModel.Group:
                    case ChatsModel.MultiUserConversation:
                        return "icon-s-secure"
                    case ChatsModel.Conversation:
                        if (model.IsOpen === "active") {
                            return "im-user"
                        } else {
                            return "im-user-inactive"
                        }
                    default:
                        console.log("unhandled type: " + model.Type)
                        return ""
                    }
                }
                text: model.Type === ChatsModel.Conversation ? model.UserObject.fullName : model.Name
                visible: !model.IsOpen
                height: visible ? implicitHeight : 0

                onClicked: {
                    if (model.Type === ChatsModel.Channel) {
                        SlackClient.joinChannel(teamRoot.teamId, model.Id)
                    } else if (model.Type === ChatsModel.Conversation) {
                        SlackClient.openChat(teamRoot.teamId, model.Id)
                    }
                    pageStack.replace(Qt.resolvedUrl("Channel.qml"), {"channelId": model.Id})
                }
            }
        }
    }
}
