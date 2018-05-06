import QtQuick 2.8
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import Qt.labs.settings 1.0

import "Settings.js" as Settings
import "ChannelList.js" as ChannelList
import "Channel.js" as Channel
import "."

//import QtQuick.Window 2.3
import "pages"
import "dialogs"
import "cover"

ApplicationWindow {
    id: window
    visible: true
    width: 800
    height: 600

    property alias pageStack: pageStack
    Settings {
        property alias width: window.width
        property alias height: window.height
    }

    color: palette.window

    Component {
        id: channelComponent
        Channel {}
    }

    Connections {
        target: SlackClient
        onThreadStarted: {
            console.log("qml: thread started")
            pageStack.push(loadingPage)
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

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

            Label {
                text: pageStack.currentItem ? pageStack.currentItem.title : ""
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

            ToolButton {
                text: qsTr("⋮")
                onClicked: menu.open()

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
                }
            }
        }
    }

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
        onWidthChanged: console.log(width)
        opacity: active ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 500 } }

        sourceComponent: ChannelList {
            width: SlackClient.isOnline ? Math.min(parent.width * 0.33, 200) : 0
            height: window.height
        }
    }
}
