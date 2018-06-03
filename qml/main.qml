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
            Rectangle {
                color: SlackClient.isOnline ? "green" : "red"
                Layout.leftMargin: Theme.paddingMedium
                height: parent.height/3
                width: height
                radius: width/2
                MouseArea {
                    anchors.fill: parent
                    onClicked: connectionPanel.open()
                }
            }

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

            RowLayout {
                Layout.fillWidth: true
                TabBar {
                    id: tabBar
                    Layout.fillWidth: true
                    currentIndex: teamsSwipe.currentIndex
                    Repeater {
                        model: teamsModel
                        TabButton {
                            hoverEnabled: true
                            ToolTip.text: name
                            icon.source: "image://emoji/icon/" + icons[icons.length - 2]
                            icon.color: "transparent"
//                            onClicked: {
//                                if (model.teamId !== SlackClient.lastTeam) {
//                                    SlackClient.connectToTeam(model.teamId)
//                                }
//                            }
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
                            teamsSwipe.currentItem.pageStack.push(Qt.resolvedUrl("pages/ChatSelect.qml"))
                        }
                    }
                    MenuItem {
                        text: qsTr("Join channel")
                        onClicked: {
                            teamsSwipe.currentItem.pageStack.push(Qt.resolvedUrl("pages/ChannelSelect.qml"))
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
                    //SwipeView.isCurrentItem || SwipeView.isNextItem || SwipeView.isPreviousItem
                sourceComponent: Team {
                    slackClient: SlackClient.slackClient(model.teamId)
                    teamId: model.teamId
                    Component.onCompleted: console.log("created: " + index + " teamid: " + model.teamId + " name " + name)
                    Component.onDestruction: console.log("destroyed:", index)
                }
                onLoaded: {
                    SlackClient.lastTeam = item.teamId
                }
            }
        }
    }

    ConnectionPanel { id: connectionPanel; }
}
