import QtQuick 2.11
import QtQuick.Controls 2.4

import ".."

Drawer {
    id: channelTextView

    property url textToShowUrl: ""
    property string textToShowUserName: ""
    property string textToShowName: ""

    function showText(url, name, userName, teamId) {
        textToShowUrl = url
        textToShowName = name
        textToShowUserName = userName
        downloadManager.append(url, "buffer", SlackClient.teamToken(teamId))
    }

    width: drawerTextArea.paintedWidth > 0.66 * window.width ?
               0.66 * window.width : drawerTextArea.paintedWidth + Theme.paddingMedium
    height: window.height
    edge: Qt.RightEdge

    Connections {
        target: downloadManager
        onFinished: {
            if (url === textToShowUrl) {
                drawerTextArea.text = downloadManager.buffer(url)
                downloadManager.clearBuffer(url)
                channelTextView.open()
            }
        }
    }

    Page {
        anchors.fill: parent
        padding: Theme.paddingMedium/2
        header: Rectangle {
            height: Theme.headerSize
            border.color: "#00050505"
            border.width: 1
            radius: 5
            color: palette.base
            Row {
                anchors.centerIn: parent
                spacing: 10
                Label {
                    text: "@" + textToShowUserName
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    color: palette.link
                }
                Label {
                    text: textToShowName
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        contentItem: ScrollView {
            id: view

            TextArea {
                id: drawerTextArea
                readOnly: true
                selectByKeyboard: true
                selectByMouse: true
            }
        }
    }
}
