import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import ".."
import com.iskrembilen 1.0

ListView {
    id: listView

    ScrollBar.vertical: ScrollBar { }

    interactive: true
    clip: true
    model: SlackClient.chatsModel(teamRoot.teamId)

    delegate: ItemDelegate {
        id: delegate
        text: model.Name
        visible: !model.IsOpen && model.Type === ChatsModel.Channel
        enabled: visible
        height: visible ? delegate.implicitHeight : 0
        spacing: Theme.paddingSmall
        icon.color: "transparent"
        icon.source: model.PresenceIcon
        width: listView.width

        onClicked: {
            SlackClient.joinChannel(teamRoot.teamId, model.Id)
            teamRoot.chatSelect.close()
        }
    }
}
