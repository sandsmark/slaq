import QtQuick 2.0
import QtWebKit 3.0
import harbour.slackfish 1.0 as Slack
import QtQuick.Controls 2.2
import "../Settings.js" as Settings

Page {
    id: page

//    property string startUrl: "https://google.com"
    property string startUrl: "https://slack.com/oauth/authorize?scope=client&client_id=11907327505.252375557155&redirect_uri=http%3A%2F%2Flocalhost%3A3000%2Foauth%2Fcallback"

    header: Label {
        text: "Sign in with Slack"
        horizontalAlignment: "AlignHCenter"
    }

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

        onNavigationRequested: {
            console.log("navigation request " + request.url.toString())
            if (request.url.toString().indexOf('http://localhost:3000/oauth/callback') !== -1) {
                visible = false
                request.action = WebView.IgnoreRequest
                Slack.Client.fetchAccessToken(request.url)
            } else {
                request.action = WebView.AcceptRequest
            }
        }
    }

    Component.onCompleted: {
        Slack.Client.onAccessTokenSuccess.connect(handleAccessTokenSuccess)
        Slack.Client.onAccessTokenFail.connect(handleAccessTokenFail)
    }

    Component.onDestruction: {
        Slack.Client.onAccessTokenSuccess.disconnect(handleAccessTokenSuccess)
        Slack.Client.onAccessTokenFail.disconnect(handleAccessTokenFail)
    }

    function handleAccessTokenSuccess(userId, teamId, teamName) {
        Settings.setUserInfo(userId, teamId, teamName)
        view.pop(undefined, PageStackAction.Animated)
    }

    function handleAccessTokenFail() {
        console.log('access token failed')
        webView.visible = true
    }
}
