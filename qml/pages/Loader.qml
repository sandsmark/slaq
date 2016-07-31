import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.slackfish 1.0 as Slack
import "Settings.js" as Settings

Page {
    id: page

    property bool firstView: true
    property bool loading: true
    property string errorMessage: ""
    property string loadMessage: ""

    SilicaFlickable {
        anchors.fill: parent

        PageHeader { title: "Slackfish" }

        PullDownMenu {
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
            color: Theme.highlightColor
            text: page.loadMessage
            font.pixelSize: Theme.fontSizeLarge
        }

        BusyIndicator {
            id: loader
            visible: loading && !errorMessageLabel.visible
            running: visible
            size: BusyIndicatorSize.Large
            anchors.centerIn: parent
        }

        Label {
            id: errorMessageLabel
            anchors.centerIn: parent
            color: Theme.highlightColor
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

    onStatusChanged: {
        if (status === PageStatus.Active) {
            errorMessage = ""
            if (firstView || Settings.hasUserInfo()) {
                firstView = false
                initLoading()
            }
            else {
                loading = false
                errorMessage = qsTr("Not logged in")
            }
        }
    }

    Component.onCompleted: {
        Slack.Client.onTestLoginSuccess.connect(handleLoginTestSuccess)
        Slack.Client.onTestLoginFail.connect(handleLoginTestFail)
        Slack.Client.onInitSuccess.connect(handleInitSuccess)
        Slack.Client.onInitFail.connect(handleInitFail)
        Slack.Client.onTestConnectionFail.connect(handleConnectionFail)
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
        pageStack.replace(Qt.resolvedUrl("ChannelList.qml"))
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
