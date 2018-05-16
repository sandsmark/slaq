import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."
import "../Settings.js" as Settings

Page {
    id: page

    property bool firstView: true
    property bool loading: true
    property string errorMessage: ""
    property string loadMessage: "Loading"

    title: "Connecting"

    Flickable {
        anchors.fill: parent

        Menu {
            enabled: !page.loading

            MenuItem {
                text: qsTr("Login")
                onClicked: pageStack.push(Qt.resolvedUrl("LoginPage.qml"))
            }
        }

        Label {
            visible: loader.visible
            anchors.bottom: loader.top
            anchors.horizontalCenter: loader.horizontalCenter
            anchors.bottomMargin: Theme.paddingLarge
            text: page.loadMessage
            font.pointSize: Theme.fontSizeLarge
        }

        BusyIndicator {
            id: loader
            visible: loading && !errorMessageLabel.visible
            running: visible
            anchors.centerIn: parent
        }

        Text {
            id: errorMessageLabel
            anchors.centerIn: parent
            visible: text.length > 0
            text: page.errorMessage
        }

        Button {
            id: initButton
            visible: false
            anchors.top: errorMessageLabel.bottom
            anchors.horizontalCenter: errorMessageLabel.horizontalCenter
            anchors.topMargin: Theme.paddingLarge
            text: qsTr("Retry")
            onClicked: {
                initButton.visible = false
                errorMessage = ""
                initLoading()
            }
        }
    }

    Component.onCompleted: {
        SlackClient.onTestLoginSuccess.connect(handleLoginTestSuccess)
        SlackClient.onTestLoginFail.connect(handleLoginTestFail)
        SlackClient.onInitSuccess.connect(handleInitSuccess)
        SlackClient.onInitFail.connect(handleInitFail)
        SlackClient.onTestConnectionFail.connect(handleConnectionFail)

        errorMessage = ""
        if (firstView || Settings.hasUserInfo()) {
            firstView = false
        } else {
            loading = false
            errorMessage = qsTr("Not logged in")
        }
    }

    Component.onDestruction: {
        SlackClient.onTestLoginSuccess.disconnect(handleLoginTestSuccess)
        SlackClient.onTestLoginFail.disconnect(handleLoginTestFail)
        SlackClient.onInitSuccess.disconnect(handleInitSuccess)
        SlackClient.onInitFail.disconnect(handleInitFail)
        SlackClient.onTestConnectionFail.disconnect(handleConnectionFail)
    }

    StackView.onActivating: {
        initLoading()
    }

    function initLoading() {
        loading = true

        if (Settings.hasUserInfo()) {
            loadMessage = qsTr("Loading")
            SlackClient.startClient()
        } else {
            SlackClient.testLogin()
        }
    }

    function handleLoginTestSuccess(userId, teamId, teamName) {
        loadMessage = qsTr("Loading")
        Settings.setUserInfo(userId, teamId, teamName)
        SlackClient.startClient()
    }

    function handleLoginTestFail() {
        pageStack.push(Qt.resolvedUrl("LoginPage.qml"))
    }

    function handleInitSuccess() {
        pageStack.replace(channelComponent, {"channelId" : SlackClient.lastChannel })
        if (SlackClient.isDevice) {
            channelList.item.open()
        } else {
            channelListPermanent.active = true
        }
    }

    function handleInitFail(why) {
        loading = false
        errorMessage = qsTr("Error connecting: ") + why
        initButton.visible = true
    }

    function handleConnectionFail() {
        loading = false
        errorMessage = qsTr("No network connection")
        initButton.visible = true
    }
}
