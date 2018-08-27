import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.3
import com.iskrembilen 1.0
import SortFilterProxyModel 0.2

import ".."


ColumnLayout {
    anchors.margins: 5
    Component.onCompleted: {
        proxyModel.sourceModel = SlackClient.usersModel(teamRoot.teamId)
        usersListView.model = proxyModel
        proxyModel.sourceModel.clearSelections()
        proxyModel.sorters = userSorter
        proxyModel.filters = filter
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
    }

    property var userSorter: ExpressionSorter {
        expression: { return modelLeft.UserObject.username < modelRight.UserObject.username; }
        ascendingOrder: true
    }
    property var fullNameSorter: ExpressionSorter {
        expression: { return modelLeft.UserObject.fullName < modelRight.UserObject.fullName; }
        ascendingOrder: true
    }

    property var filter: AllOf {
        ExpressionFilter {
            expression: {
                if (chatSelect.searchInput.text.length <= 0) {
                    return true
                } else {
                    return (model.UserObject.fullName.indexOf(chatSelect.searchInput.text) >=0 ||
                            model.UserObject.username.indexOf(chatSelect.searchInput.text) >=0)
                }
            }
        }
    }

    ButtonGroup {
        id: tabPositionGroup
    }

    GroupBox {
        id: sortGroup
        title: qsTr("Sort by")
        padding: 2
        Layout.margins: 5
        Layout.fillWidth: true
        RowLayout {
            Layout.fillWidth: true
            RadioButton {
                id: userSortButton
                text: qsTr("User")
                Layout.fillWidth: true
                checked: true
                ButtonGroup.group: tabPositionGroup
                onClicked: proxyModel.sorters = userSorter
            }
            RadioButton {
                id: fullNameSortButton
                text: qsTr("Full name")
                Layout.fillWidth: true
                ButtonGroup.group: tabPositionGroup
                onClicked: proxyModel.sorters = fullNameSorter
            }
        }
    }


    RowLayout {
        Layout.fillWidth: true
        Layout.margins: 5
        Button {
            id: joinButton
            Layout.fillWidth: true
            text: qsTr("Join selected")
            enabled: proxyModel.sourceModel.selected
            onClicked: {
                SlackClient.openChat(teamRoot.teamId, proxyModel.sourceModel.selectedUserIds())
                proxyModel.sourceModel.clearSelections()
                teamRoot.chatSelect.close()
            }
        }
        Button {
            id: clearButton
            Layout.fillWidth: true
            text: qsTr("Clear")
            enabled: proxyModel.sourceModel.selected
            onClicked: {
                proxyModel.sourceModel.clearSelections()
            }
        }
    }

    ListView {
        id: usersListView
        Layout.fillWidth: true
        Layout.fillHeight: true

        property bool channelAddMode: false
        ScrollBar.vertical: ScrollBar { }

        interactive: true
        clip: true

        delegate: CheckDelegate {
            id: delegate
            implicitHeight: Theme.itemSize
            text: UserObject.username + " ( "+ UserObject.fullName +" )"
            checkState: UserObject.selected ? Qt.Checked : Qt.Unchecked
            indicator: Item {
                implicitWidth: Theme.itemSize
                implicitHeight: Theme.itemSize
                x: delegate.width - width - delegate.rightPadding
                visible: delegate.checked
                y: delegate.topPadding + delegate.availableHeight / 2 - height / 2
                EmojiButton {
                    background: Item {}
                    anchors.centerIn: parent
                    font.pixelSize: Theme.itemSize/2
                    text: "🗸"
                }
            }

            spacing: Theme.paddingMedium
            padding: delegate.height/2 + Theme.paddingMedium
            icon.color: "transparent"
            icon.source: "image://emoji/slack/" + UserObject.avatarUrl
            width: usersListView.width
            Image {
                id: image
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
                proxyModel.sourceModel.setSelected(index)
            }
        }
    }
}
