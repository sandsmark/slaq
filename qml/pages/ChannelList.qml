import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import ".."
import "../components"

import com.iskrembilen 1.0

Page {
    id: page

    property bool appActive: Qt.application.state === Qt.ApplicationActive

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
                //avatarImage.source = selfUser.avatarUrl
            }
        }
    }

    header: RowLayout {
        spacing: Theme.paddingMedium
        Button {
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
            Label {
                text: page.title
                verticalAlignment: "AlignVCenter"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                height: Theme.headerSize
            }
            RowLayout {
                Label {
                    text: selfUser.username
                    verticalAlignment: "AlignVCenter"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
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
