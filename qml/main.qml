import QtQuick 2.8
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtQuick.Window 2.3

import Qt.labs.settings 1.0

import "ChannelList.js" as ChannelList
import "Channel.js" as Channel
import "."

import "pages"
import "dialogs"
import "cover"

ApplicationWindow {
    id: window
    visible: true
    width: 800
    height: 600

    property alias pageStack: pageStack
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

    color: palette.window

    EmojiSelector  {
        id: emojiSelector
    }

    Component {
        id: channelComponent
        Channel {}
    }

    Component {
        id: channelsListComponent
        ChannelList {}
    }

    Connections {
        target: SlackClient
        onThreadStarted: {
            console.log("qml: thread started")
            pageStack.push(loadingPage)
        }
        onInitSuccess: {
            channelListPermanent.sourceComponent = channelsListComponent
            pageStack.push(loadingPage)
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
                text: pageStack.depth > 1 ? "‹" : "›"
                onClicked: {
                    if (pageStack.depth > 1) {
                        pageStack.pop()
                        return;
                    }

                    if (channelList.active) {
                        channelList.item.open()
                    }
                }

                visible: SlackClient.isDevice || pageStack.depth > 1
                enabled: visible
            }
            RowLayout {
                Layout.fillWidth: true
                Repeater {
                    model: teamsModel
                    ToolButton {
                        hoverEnabled: true
                        ToolTip.text: name
                        icon.source: "image://emoji/icon/" + icons[icons.length - 2]
                        icon.color: "transparent"
                        onClicked: {
                            pageStack.clear()
                            channelListPermanent.sourceComponent = undefined
                            pageStack.push(loadingPage)
                            SlackClient.connectToTeam(teamId)
                        }
                    }
                }

                ToolButton {
                    text: "➕"
                    onClicked: {
                        pageStack.clear()
                        channelListPermanent.sourceComponent = undefined
                        SlackClient.connectToTeam("")
                        pageStack.push(Qt.resolvedUrl("pages/LoginPage.qml"))
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
                            pageStack.push(Qt.resolvedUrl("pages/About.qml"))
                        }
                    }
                    MenuItem {
                        text: qsTr("Open chat")
                        onClicked: {
                            pageStack.push(Qt.resolvedUrl("pages/ChatSelect.qml"))
                        }
                    }
                    MenuItem {
                        text: qsTr("Join channel")
                        onClicked: {
                            pageStack.push(Qt.resolvedUrl("pages/ChannelSelect.qml"))
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

    ConnectionPanel { id: connectionPanel; }

    Component {
        id: loadingPage
        LoadingPage {}
    }

    StackView {
        id: pageStack
        anchors {
            top: parent.top
            left: channelListPermanent.active ? channelListPermanent.right : parent.left
            right: parent.right
            bottom: parent.bottom
        }

        transform: Translate {
            x: SlackClient.isDevice ? channelList.item.position * width * 0.33 : 0
        }

    }

//    Loader {
//        id: channelList
//        active: SlackClient.isDevice

//        sourceComponent:  Drawer {
//            width: parent.width * 0.66
//            height: window.height

//            ChannelList {
//                anchors.fill: parent
//            }
//        }
//    }

    Loader {
        id: channelListPermanent
        width: active ? Math.min(parent.width * 0.33, 200) : 0
        active: false
        height: window.height
        opacity: active ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 500 } }
    }
}
