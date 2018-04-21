import QtQuick 2.10
import QtQuick.Controls 2.2
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

//    ScrollView {
//    Drawer {
    StackView {
        id: pageStack
        anchors.fill: parent

        initialItem: LoadingPage {
            height: window.height
            width: window.width
        }
    }

    Item {
        id: channelsView
//        anchors {
//            top: parent.top
//            left: parent.left
//            bottom: parent.bottom
//            margins: 10
//        }
        width: parent.width * 0.33
        height: parent.height
        visible: Slack.Client.isOnline
//        interactive: false
//        modal: false

//        background: Rectangle {
//            border.color: channelsView.activeFocus ? "red" : "pink"
//        }

        ChannelListView {

        }



//        Column {
//            Label {
//                text: qsTr("Channels")
//            }

//            Item { height: 5; width: height } // spacer

//            Repeater {
//                id: channelRepeater
//                model: [ "#test1", "#test2" ]

//                delegate: Item {
//                    id: delegate
//                    Row {
//                        id: row
////                        width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
//                        anchors.verticalCenter: parent.verticalCenter
////                        x: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)
////                        spacing: Theme.paddingMedium

////                        SlackImage {
////                        Image {
////                            id: icon
//////                            source: "image://theme/" + Channel.getIcon(model) + "?" + (delegate.highlighted ? currentColor : Channel.getIconColor(model, currentColor))
////                            anchors.verticalCenter: parent.verticalCenter
////                        }

//                        Label {
//                            width: parent.width - icon.width - Theme.paddingMedium
//                            wrapMode: Text.Wrap
//                            anchors.verticalCenter: parent.verticalCenter
//                            font.pixelSize: Theme.fontSizeMedium
//                            font.bold: channelRepeater.model.unreadCount > 0
////                            text: model.name
////                            color: currentColor
//                        }
//                    }
//                }
//            }
//        }
    }
//    CoverPage {

//    }


//    initialItem: CoverPage {
//    }

////    initialItem: Pages.CoverPage {} //Component { Loader { } }
////    cover: Qt.resolvedUrl("cover/CoverPage.qml")
////    allowedOrientations: Orientation.All
////    _defaultPageOrientations: Orientation.All

//    function activateChannel(channelId) {
//        console.log("Navigate to", channelId);
//        pageStack.clear()
//        pageStack.push(Qt.resolvedUrl("pages/ChannelList.qml"), {}, true)
//        activate()
//        pageStack.push(Qt.resolvedUrl("pages/Channel.qml"), {"channelId": channelId})
//    }
}
