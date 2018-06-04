import QtQuick 2.8
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtQuick.Window 2.3

import Qt.labs.settings 1.0

import "."
import "pages"
import "dialogs"

ApplicationWindow {
    id: window
    visible: true
    width: 800
    height: 600

    property alias teamsSwipe: teamsSwipe
    property alias emojiSelector: emojiSelector
    property alias settings: settings

    Settings {
        id: settings
        property string emojisSet
        property alias width: window.width
        property alias height: window.height
    }

    SettingsDialog {
        id: settingsDialog
    }

    LoginDialog {
        id: loginDialog
    }

    AboutDialog {
        id: aboutDialog
    }

    color: palette.window

    EmojiSelector  {
        id: emojiSelector
    }

    Connections {
        target: SlackClient
        onThreadStarted: {
            console.log("qml: thread started")
        }
    }

    header: ToolBar {
        id: toolbar
        RowLayout {
            anchors.fill: parent
            spacing: 0
            ToolButton {
                Layout.leftMargin: Theme.headerSize/3
                background: Rectangle {
                    anchors.centerIn: parent
                    color: SlackClient.isOnline ? "green" : "red"
                    implicitHeight: Theme.headerSize/3
                    implicitWidth: Theme.headerSize/3
                    radius: implicitWidth/2
                }

                onClicked: connectionPanel.open()
            }

            ToolSeparator {}

            RowLayout {
                TabBar {
                    id: tabBar
                    spacing: 2
                    currentIndex: teamsSwipe.currentIndex
                    Repeater {
                        model: teamsModel
                        TabButton {
                            hoverEnabled: true
                            ToolTip.delay: 200
                            ToolTip.text: model.name
                            ToolTip.visible: hovered
                            background: Item {
                                implicitHeight: Theme.headerSize
                                implicitWidth: Theme.headerSize
                            }
                            contentItem: Image {
                                anchors.centerIn: parent
                                sourceSize { width: Theme.headerSize - 2; height: Theme.headerSize - 2 } // ### TODO: resize the image
                                source: model.icons.length > 1 ? "image://emoji/icon/" + model.icons[model.icons.length - 2] : ""
                                smooth: true
                            }
                            onPressAndHold: teamMenu.open();
                            Menu {
                                id: teamMenu
                                MenuItem {
                                    text: qsTr("Leave")
                                    onClicked: {
                                        SlackClient.leaveTeam(model.teamId)
                                    }
                                }
                            }
                        }
                    }
                }

                ToolButton {
                    text: "➕"
                    font.family: "Twitter Color Emoji"
                    onClicked: {
                        loginDialog.open()
                    }
                }
            }
            ToolSeparator {}

            Item { Layout.fillWidth: true }
            ToolButton {
                text: teamsSwipe.currentItem !== null ?
                          teamsSwipe.currentItem.item.pageStack.depth > 1 ?
                              "‹" : "›" : ""
                onClicked: {
                    if (teamsSwipe.currentItem.item.pageStack.depth > 1) {
                        teamsSwipe.currentItem.item.pageStack.pop()
                        return;
                    }

                    if (channelList.active) {
                        channelList.item.open()
                    }
                }

                visible: SlackClient.isDevice || teamsSwipe.currentItem !== null ?
                             teamsSwipe.currentItem.item.pageStack.depth > 1 : false
                enabled: visible
            }
            ToolButton {
                text: qsTr("⋮")
                onClicked: menu.open()

                Layout.alignment: Qt.AlignRight

                Menu {
                    id: menu

                    MenuItem {
                        text: qsTr("About")
                        onClicked: {
                            aboutDialog.open()
                        }
                    }
                    MenuItem {
                        text: qsTr("Open chat")
                        onClicked: {
                            teamsSwipe.currentItem.item.pageStack.push(Qt.resolvedUrl("pages/ChatSelect.qml"))
                        }
                    }
                    MenuItem {
                        text: qsTr("Join channel")
                        onClicked: {
                            teamsSwipe.currentItem.item.pageStack.push(Qt.resolvedUrl("pages/ChannelSelect.qml"))
                        }
                    }
                    MenuItem {
                        text: qsTr("Settings")
                        onClicked: {
                            settingsDialog.open()
                        }
                    }
                }
            }
        }
    }

    SwipeView {
        id: teamsSwipe
        anchors.fill: parent
        currentIndex: tabBar.currentIndex
        Repeater {
            model: teamsModel
            Loader {
                active: true
                sourceComponent: Team {
                    slackClient: SlackClient.slackClient(model.teamId)
                    teamId: model.teamId
                    teamName: name
                }
                onLoaded: {
                    SlackClient.lastTeam = item.teamId
                }
            }
        }
    }

    ConnectionPanel { id: connectionPanel; }
}
