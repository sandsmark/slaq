import QtQuick 2.0
import com.iskrembilen.slaq 1.0 as Slack
import QtQuick.Controls 2.2
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
        Slack.Client.onTestLoginSuccess.connect(handleLoginTestSuccess)
        Slack.Client.onTestLoginFail.connect(handleLoginTestFail)
        Slack.Client.onInitSuccess.connect(handleInitSuccess)
        Slack.Client.onInitFail.connect(handleInitFail)
        Slack.Client.onTestConnectionFail.connect(handleConnectionFail)

        errorMessage = ""
        if (firstView || Settings.hasUserInfo()) {
            firstView = false
            initLoading()
        } else {
            loading = false
            errorMessage = qsTr("Not logged in")
        }
    }

    Component.onDestruction: {
        Slack.Client.onTestLoginSuccess.disconnect(handleLoginTestSuccess)
        Slack.Client.onTestLoginFail.disconnect(handleLoginTestFail)
        Slack.Client.onInitSuccess.disconnect(handleInitSuccess)
        Slack.Client.onInitFail.disconnect(handleInitFail)
        Slack.Client.onTestConnectionFail.disconnect(handleConnectionFail)
    }

    function initLoading() {
        loading = true

        if (Settings.hasUserInfo()) {
            loadMessage = qsTr("Loading")
            Slack.Client.start()
        }
        else {
            Slack.Client.testLogin()
        }
    }

    function handleLoginTestSuccess(userId, teamId, teamName) {
        loadMessage = qsTr("Loading")
        Settings.setUserInfo(userId, teamId, teamName)
        Slack.Client.start()
    }

    function handleLoginTestFail() {
        pageStack.push(Qt.resolvedUrl("LoginPage.qml"))
    }

    function handleInitSuccess() {
        pageStack.replace(channelComponent, {"channelId" : Slack.Client.lastChannel() })
        if (Slack.Client.isDevice) {
            channelList.item.open()
        } else {
            channelListPermanent.active = true
        }
    }

    function handleInitFail() {
        loading = false
        errorMessage = qsTr("Error loading team information")
        initButton.visible = true
    }

    function handleConnectionFail() {
        loading = false
        errorMessage = qsTr("No network connection")
        initButton.visible = true
    }
}
