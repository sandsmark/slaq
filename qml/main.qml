import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtQuick.Window 2.3
import QtQuick.Controls.Material 2.4
import QtQuick.Controls.Universal 2.4

import Qt.labs.settings 1.0
import Qt.labs.platform 1.0 as Platform
import com.iskrembilen 1.0
import "."
import "pages"
import "dialogs"
import "components"

ApplicationWindow {
    id: window
    visible: true
    width: Screen.desktopAvailableWidth
    height: Screen.desktopAvailableHeight

    property alias teamsSwipe: teamsSwipe
    property alias emojiSelector: emojiSelector
    property alias settings: settings
    property alias textViewer: textViewer
    property alias fileSend: fileSend

    property int totalUnreadChannelMessages: 0
    property int totalUnreadIMMessages: 0
    property bool completed: false

    readonly property bool isMobile: Qt.platform.os === "android" ||
                                     Qt.platform.os === "ios" ||
                                     Qt.platform.os === "winrt"

    function recalcUnread() {
        var total = 0
        var totalIm = 0
        for (var i = 0; i < tabBar.contentChildren.length; i++) {
            var tabItem = tabBar.contentChildren[i]
            total += tabItem.unreadChannelMessages
            totalIm += tabItem.unreadIMMessages
        }
        window.totalUnreadChannelMessages = total
        window.totalUnreadIMMessages = totalIm
    }

    Action {
        id: openAction
        text: "&Open"
        shortcut: "Ctrl+D"
        onTriggered: {
            console.log("ctrl-d")
            SlackClient.dumpChannel(teamsSwipe.currentItem.item.teamId, teamsSwipe.currentItem.item.currentChannelId);
        }
    }

    Material.theme: settings.theme
    Universal.theme: settings.theme

    Platform.SystemTrayIcon {
        id: trayIcon
        visible: available
        iconSource: window.totalUnreadChannelMessages > 0 && window.totalUnreadIMMessages > 0 ?
                        "qrc:/icons/slaq_ch_im.png" :
                        (window.totalUnreadChannelMessages > 0 ?
                             "qrc:/icons/slaq_ch.png" :
                             (window.totalUnreadIMMessages > 0 ?
                                  "qrc:/icons/slaq_im.png" :
                                  "qrc:/icons/slaq.png"))

        onActivated: {
            if (window.visible) {
                window.hide()
            } else {
                window.showNormal()
                window.raise()
                window.requestActivate()
            }
        }
    }

    Settings {
        id: settings
        property string emojisSet
        property string style
        property alias width: window.width
        property alias height: window.height
        property int theme: Material.System
        property bool loadOnlyLastTeam: true
        property bool unloadViewOnTeamSwitch: false
        property bool cacheSlackImages: true
    }

    property alias leaveDialog: leaveDialog

    GroupLeaveDialog {
        id: leaveDialog
        property string channelId
        property string teamId
        onAccepted: {
            SlackClient.closeChat(teamId, channelId)
        }
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

    SearchResultsList {
        id: searchResultsList
    }

    TextViewer {
        id: textViewer
    }

    FileSend {
        id: fileSend
    }

//    Connections {
//        target: SlackClient
//        onThreadStarted: {
//            console.log("qml: thread started", SlackClient.lastTeam)
//        }
//    }

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
                    background: Rectangle {
                        color: "transparent"
                    }

                    Repeater {
                        model: teamsModel
                        TabButton {
                            id: tabButton
                            hoverEnabled: true
                            ToolTip.delay: 200
                            ToolTip.text: model.name
                            ToolTip.visible: hovered
                            padding: 1
                            property int unreadChannelMessages: 0
                            property int unreadIMMessages: 0
                            background: Item {
                                implicitHeight: Theme.headerSize
                                implicitWidth: Theme.headerSize
                            }
                            contentItem: Item {
                                implicitHeight: Theme.headerSize
                                implicitWidth: Theme.headerSize
                                Image {
                                    anchors.centerIn: parent
                                    width: Theme.headerSize - 2
                                    height: Theme.headerSize - 2
                                    source: model.icons.length > 1 ? "image://emoji/slack/" + model.icons[1] : ""
                                    smooth: true
                                }
                                Rectangle {
                                    color: "green"
                                    visible: unreadChannelMessages > 0
                                    width: Theme.headerSize / 2
                                    height: Theme.headerSize / 2
                                    radius: width/2
                                    anchors.top: parent.top
                                    anchors.right: parent.right
                                    Text {
                                        color: "white"
                                        anchors.centerIn: parent
                                        text: unreadChannelMessages
                                    }
                                }
                                Rectangle {
                                    color: "red"
                                    visible: unreadIMMessages > 0
                                    width: Theme.headerSize / 2
                                    height: Theme.headerSize / 2
                                    radius: width/2
                                    anchors.bottom: parent.bottom
                                    anchors.right: parent.right
                                    Text {
                                        color: "white"
                                        anchors.centerIn: parent
                                        text: unreadIMMessages
                                    }
                                }
                            }
                            onClicked: {
                                SlackClient.lastTeam = model.teamId
                                console.log("set last team", SlackClient.lastTeam)
                                tabBar.currentIndex = index
                            }
                            Connections {
                                target: SlackClient
                                onChannelCountersUpdated: {
                                    if (teamId === model.teamId) {
                                        //unread_messages
                                        if (unreadMessages >= 0) {
                                            var total = SlackClient.getTotalUnread(teamId, ChatsModel.Channel)
                                            var totalIm = SlackClient.getTotalUnread(teamId, ChatsModel.Group)
                                            totalIm += SlackClient.getTotalUnread(teamId, ChatsModel.Conversation)
                                            totalIm += SlackClient.getTotalUnread(teamId, ChatsModel.MultiUserConversation)
                                            tabButton.unreadChannelMessages = total
                                            tabButton.unreadIMMessages = totalIm

                                        } else {
                                            // the chat is not yet initialized, so we only know that there is new message
                                            tabButton.unreadChannelMessages++
                                        }
                                        window.recalcUnread()
                                    }
                                }
                            }

                            onPressAndHold: teamMenu.open();
                            Menu {
                                id: teamMenu
                                MenuItem {
                                    text: qsTr("Leave")
                                    onTriggered: {
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
            RowLayout {
                spacing: 2
                EmojiToolButton {
                    text: "🔍"
                    onClicked: {
                        SlackClient.searchMessages(teamsSwipe.currentItem.item.teamId, searchInput.text)
                    }
                }

                TextField {
                    id: searchInput
                    placeholderText: qsTr("Search...")
                    Layout.fillWidth: true
                    rightPadding: closeButton.width + Theme.paddingSmall
                    leftPadding: Theme.paddingMedium
                    onAccepted: {
                        SlackClient.searchMessages(teamsSwipe.currentItem.item.teamId, searchInput.text)
                    }

                    EmojiToolButton {
                        id: closeButton
                        padding: 0
                        anchors.right: parent.right
                        anchors.rightMargin: -Theme.paddingSmall
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: -2
                        width: searchInput.contentHeight
                        height: searchInput.contentHeight
                        text: "✖️"
                        visible: searchInput.text !== ""
                        onClicked: {
                            searchInput.text = ""
                        }
                    }
                }
            }

            ToolSeparator {}

            EmojiToolButton {
                text: teamsSwipe.currentItem != null && teamsSwipe.currentItem.item != null ?
                          teamsSwipe.currentItem.item.pageStack.depth > 1 ?
                              "🡸" : "🡺" : ""
                onClicked: {
                    if (teamsSwipe.currentItem.item.pageStack.depth > 1) {
                        teamsSwipe.currentItem.item.pageStack.pop()
                        return;
                    }

                    if (channelList.active) {
                        channelList.item.open()
                    }
                }

                visible: SlackClient.isDevice || (teamsSwipe.currentItem != null &&  teamsSwipe.currentItem.item != null) ?
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
                        text: qsTr("Settings")
                        onClicked: {
                            settingsDialog.open()
                        }
                    }
                    MenuItem {
                        text: qsTr("Quit")
                        onClicked: {
                            Qt.quit()
                        }
                    }
                }
            }
        }
    }

    function switchTeam() {
        //make sure we will not switch before settings are readed out
        if (teamsSwipe.currentIndex < 0 ||  completed == false) {
            return;
        }

        var ciItem = repeater.itemAt(teamsSwipe.currentIndex)
        if (settings.unloadViewOnTeamSwitch === false) {
            ciItem.active = true
        } else {
            for (var i = 0; i < repeater.count; i++) {
                if (i == teamsSwipe.currentIndex) {
                    ciItem.active = true
                } else {
                    repeater.itemAt(i).active = false
                }
            }
        }
    }

    Component.onCompleted: {
        completed = true
        switchTeam()
    }

    SwipeView {
        id: teamsSwipe
        anchors.fill: parent
        currentIndex: tabBar.currentIndex
        interactive: false
        onCurrentIndexChanged: {
            switchTeam()
        }

        Repeater {
            id: repeater
            model: teamsModel
            onCountChanged: {
                console.log("count: " + count)
                if (count === 0 && SlackClient.lastTeam === "") {
                    loginDialog.open()
                }
            }

            Loader {
                id: teamloader
                active: false
                asynchronous: true
                property string teamId: model.teamId
                sourceComponent: Team {
                    slackClient: SlackClient.slackClient(model.teamId)
                    teamId: model.teamId
                    teamName: model.name
                }
                SwipeView.onIsCurrentItemChanged: {
                    if (SwipeView.isCurrentItem && item !== null) {
                        item.setCurrentTeam()
                    }
                }
                Component.onCompleted: {
                    if (model.teamId === SlackClient.lastTeam) {
                        tabBar.currentIndex = index
                        teamloader.active = true
                    }
                    if (settings.loadOnlyLastTeam === false && settings.unloadViewOnTeamSwitch === false) {
                        teamloader.active = true
                    }
                }
            }
        }
    }

    ConnectionPanel { id: connectionPanel; }
}
