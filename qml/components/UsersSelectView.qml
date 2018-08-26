import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.3

import ".."
import com.iskrembilen 1.0

ColumnLayout {

    Button {
        Layout.fillWidth: true
        text: qsTr("Join selected")
        enabled: usersListView.model.selected
    }

    ListView {
        id: usersListView
        Layout.fillWidth: true
        Layout.fillHeight: true

        property bool channelAddMode: false
        ScrollBar.vertical: ScrollBar { }

        interactive: true
        clip: true

        model: SlackClient.usersModel(teamRoot.teamId)

        delegate: ItemDelegate {
            id: delegate
            text: UserObject.username + " ( "+ UserObject.fullName +" )"
            highlighted: UserObject.selected
            spacing: Theme.paddingMedium
            padding: delegate.height/2 + Theme.paddingMedium
            icon.color: "transparent"
            icon.source: "image://emoji/slack/" + UserObject.avatarUrl
            width: usersListView.width
            Image {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                sourceSize: Qt.size(delegate.height/2, delegate.height/2)
                source: {
                    var presence = UserObject.presence
                    if (presence == User.Away) {
                        return "qrc:/icons/away-icon.png";
                    } else if (presence == User.Dnd) {
                        return "qrc:/icons/dnd-icon.png";
                    } else if (presence == User.Active) {
                        return "qrc:/icons/active-icon.png";
                    }
                    return "qrc:/icons/offline-icon.png";
                }
            }

            onClicked: {
                usersListView.model.setSelected(index)
                //            SlackClient.openChat(teamRoot.teamId, UserObject.userId)
                //            teamRoot.chatSelect.close()
            }
        }
    }
}
