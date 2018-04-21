import QtQuick 2.10
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import com.iskrembilen.slaq 1.0 as Slack

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

    SystemPalette {
        id: palette
    }

    Component {
        id: channelComponent
        Channel {}
    }


    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: pageStack.depth > 0 ? ("›") : "<"
                onClicked: channelList.item.open()
//                visible: Slack.Client.isDevice
            }

            Label {
                text: pageStack.currentItem.title
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

            ToolButton {
                text: qsTr("⋮")
                onClicked: menu.open()
            }
        }
    }

    StackView {
        id: pageStack
        anchors {
            top: parent.top
            left: channelListPermanent.active ? channelListPermanent.right : parent.left
            right: parent.right
            bottom: parent.bottom
        }

        initialItem: LoadingPage {
            height: window.height
            width: window.width
        }

        transform: Translate {
            x: Slack.Client.isDevice ? channelList.item.position * width * 0.33 : 0
        }

    }

    Loader {
        id: channelList
        active: Slack.Client.isDevice

        sourceComponent:  Drawer {
            width: parent.width * 0.66
            height: window.height

            ChannelList {
                anchors.fill: parent
            }
        }
    }

    Loader {
        id: channelListPermanent
        width: active ? parent.width * 0.33 : 0
        active: false
        height: window.height
        onWidthChanged: console.log(width)
        opacity: active ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 500 } }

        sourceComponent:   ChannelList {
            width: Slack.Client.isOnline ? parent.width * 0.33 : 0
            height: window.height
        }
    }
}
