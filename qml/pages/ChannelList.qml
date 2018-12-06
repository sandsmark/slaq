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

    Menu {
        id: userMenu
        Action {
            text: qsTr("Photo/Status")
            onTriggered: {
                userDialog.user = selfUser
                userDialog.teamId = teamRoot.teamId
                userDialog.open()
            }
        }
        Menu {
            title: qsTr("Presence")
            MenuItem {
                text: qsTr("Away")
                checked: selfUser != null && selfUser.presence == User.Away
                checkable: true
                icon.source: "qrc:/icons/away-icon.png"
                icon.color: "transparent"
                onTriggered: {
                    if (selfUser.presence == User.Dnd) {
                        SlackClient.cancelDnD(teamRoot.teamId)
                    }
                    SlackClient.setPresence(teamRoot.teamId, !(selfUser.presence == User.Away))
                }
            }
            MenuItem {
                text: qsTr("DnD")
                checkable: true
                checked: selfUser != null ? selfUser.presence === User.Dnd : false
                icon.source: "qrc:/icons/dnd-icon.png"
                icon.color: "transparent"
                onTriggered: {
                    dndDialog.user = selfUser
                    dndDialog.teamId = teamRoot.teamId
                    dndDialog.isDnd = (selfUser.presence == User.Dnd ? true : false)
                    dndDialog.open()
                }
            }
        }
    }

    header: RowLayout {
        spacing: Theme.paddingMedium
        ImageButton {
            id: button
            bgSizeW: Theme.headerSize + Theme.paddingMedium
            bgSizeH: Theme.headerSize + Theme.paddingMedium
            source: selfUser != null ? "image://emoji/slack/" + selfUser.avatarUrl : ""
            onClicked: {
                userMenu.open()
            }
        }

        ColumnLayout {
            height: Theme.headerSize
            Layout.fillWidth: true
            RowLayout {
                spacing: Theme.paddingTiny
                Label {
                    Layout.fillWidth: true
                    text: page.title
                    font.bold: true
                    height: Theme.headerSize
                }

                EmojiRoundButton {
                    text: "📂"
                    padding: 0
                    radius: Theme.headerSize/2
                    onClicked: {
                        filesSharesList.open()
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
