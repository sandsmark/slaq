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
        Button {
            display: AbstractButton.IconOnly
            background: Item {
                implicitHeight: Theme.headerSize
                implicitWidth: Theme.headerSize
            }
            contentItem: Image {
                id: avatarImage
                sourceSize: Qt.size(Theme.headerSize, Theme.headerSize)
                width: Theme.headerSize
                height: Theme.headerSize
                smooth: true
                source: selfUser != null ? "image://emoji/slack/" + selfUser.avatarUrl : ""
            }
            onClicked: {
                userDialog.user = selfUser
                userDialog.teamId = teamRoot.teamId
                userDialog.open()
            }
        }
        ColumnLayout {
            height: Theme.headerSize
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: page.title
                font.bold: true
                height: Theme.headerSize
            }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
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
