import QtQuick 2.11
import QtQuick.Controls 2.4
import SortFilterProxyModel 0.2
import com.iskrembilen 1.0

import ".."

ListView {
    id: listView

    Component.onCompleted: {
//        proxyModel.sourceModel = SlackClient.chatsModel(teamRoot.teamId)
//        listView.model = proxyModel
//        proxyModel.sorters = channelSorter
//        proxyModel.filters = filter
    }
    // invalidate filetr
    Connections {
        target: chatSelect.searchInput
        onTextChanged: {
            filter.enabled = false
            filter.enabled = true
        }
    }

    SortFilterProxyModel {
        id: proxyModel
        sourceModel: SlackClient.chatsModel(teamRoot.teamId)
        sorters: channelSorter
        filters: filter
    }

    property var channelSorter: ExpressionSorter {
        expression: { return modelLeft.Name < modelRight.Name; }
        ascendingOrder: true
    }

    property var filter: AllOf {
        ExpressionFilter {
            expression: {
                if (chatSelect.searchInput.text.length <= 0) {
                    return true
                } else {
                    return model.Name.indexOf(chatSelect.searchInput.text) >=0
                }
            }
        }
    }

    ScrollBar.vertical: ScrollBar { }

    interactive: true
    clip: true
    model: proxyModel

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
