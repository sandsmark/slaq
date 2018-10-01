import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import ".."
import "../components"

import com.iskrembilen 1.0

Page {
    id: page

    property bool appActive: Qt.application.state === Qt.ApplicationActive

    spacing: 5
    property User selfUser: null
    title: teamRoot.teamName
    onAppActiveChanged: {
        SlackClient.setAppActive(teamRoot.teamId, appActive)
    }

    Connections {
        target: SlackClient
        onUsersModelChanged: {
            if (teamId === teamRoot.teamId) {
                selfUser = SlackClient.selfUser(teamRoot.teamId)
                console.log("got user", selfUser)
            }
        }
    }

    header: RowLayout {
        spacing: Theme.paddingMedium
        ImageButton {
            id: button
            bgSizeW: Theme.headerSize + Theme.paddingMedium
            bgSizeH: Theme.headerSize + Theme.paddingMedium
            sizeHoveredW: Theme.headerSize + Theme.paddingMedium/2
            sizeHoveredH: Theme.headerSize + Theme.paddingMedium/2
            source: selfUser != null ? "image://emoji/slack/" + selfUser.avatarUrl : ""
            onClicked: {
                userDialog.user = selfUser
                userDialog.teamId = teamRoot.teamId
                userDialog.open()
            }
        }

        ColumnLayout {
            height: Theme.headerSize
            Layout.fillWidth: true
            RowLayout {
                spacing: 2
                Label {
                    Layout.fillWidth: true
                    text: page.title
                    font.bold: true
                    height: Theme.headerSize
                }

                RoundButton {
                    id: awayButton
                    display: AbstractButton.IconOnly
                    checked: selfUser.presence === User.Away
                    checkable: false
                    implicitWidth: height
                    icon.color: "transparent"
                    icon.source: "qrc:/icons/away-icon.png"
                    onCheckedChanged: {
                        checkable = false
                    }
                    onClicked: {
                        SlackClient.setPresence(teamRoot.teamId, !(selfUser.presence == User.Away))
                    }
                }

                RoundButton {
                    id: dndButton
                    display: AbstractButton.IconOnly
                    checkable: false
                    checked: selfUser.presence === User.Dnd
                    implicitWidth: height
                    icon.color: "transparent"
                    icon.source: "qrc:/icons/dnd-icon.png"
                    onCheckedChanged: { //button should react only on user presence status changes
                        checkable = false
                    }
                    onClicked: {
                        dndDialog.user = selfUser
                        dndDialog.teamId = teamRoot.teamId
                        dndDialog.isDnd = (selfUser.presence == User.Dnd ? true : false)
                        dndDialog.open()
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: selfUser != null ? selfUser.username: ""
                    font.bold: true
                    height: Theme.headerSize
                }
                UserStatusEmoji {
                    id: statusImage
                    user: selfUser
                    width: Theme.headerSize/2
                    height: Theme.headerSize/2
                }
            }
        }
    }

    ChannelListView {
        anchors.fill: parent
    }
}
