import QtQuick 2.8
import QtWebView 1.5
import QtQuick.Controls 2.3
import "../Settings.js" as Settings

Page {
    id: page

    property string startUrl: "https://slack.com/oauth/authorize?scope=client&client_id=11907327505.252375557155&redirect_uri=http%3A%2F%2Flocalhost%3A3000%2Foauth%2Fcallback"

    title: "Sign in with Slack"

    Rectangle {
        anchors {
            fill: parent
            margins: 5
        }
        color: "transparent"
        border.color: "black"
    }

    WebView {
        id: webView
        anchors.fill: parent
        url: page.startUrl
        onUrlChanged: console.log("uirl changeD " + url)

        onLoadingChanged: {
            console.log("navigation request " + loadRequest.url.toString())
            if (loadRequest.url.toString().indexOf('http://localhost:3000/oauth/callback') !== -1) {
                webView.stop()
                visible = false
                SlackClient.fetchAccessToken(loadRequest.url)
            }
        }
    }

    Component.onCompleted: {
        SlackClient.onAccessTokenSuccess.connect(handleAccessTokenSuccess)
        SlackClient.onAccessTokenFail.connect(handleAccessTokenFail)
    }

    Component.onDestruction: {
        SlackClient.onAccessTokenSuccess.disconnect(handleAccessTokenSuccess)
        SlackClient.onAccessTokenFail.disconnect(handleAccessTokenFail)
    }

    function handleAccessTokenSuccess(userId, teamId, teamName) {
        Settings.setUserInfo(userId, teamId, teamName)
        pageStack.pop(undefined, StackView.Transition)
    }

    function handleAccessTokenFail() {
        console.log('access token failed')
        webView.visible = true
    }
}
