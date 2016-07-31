import QtQuick 2.0
import Sailfish.Silica 1.0
import QtWebKit 3.0
import harbour.slackfish 1.0 as Slack
import "Settings.js" as Settings

Page {
    id: page

    property string startUrl: "https://slack.com/oauth/authorize?scope=client&client_id=19437480772.45165537666&redirect_uri=http%3A%2F%2Flocalhost%3A3000%2Foauth%2Fcallback"

    SilicaWebView {
        id: webView
        anchors.fill: parent
        url: page.startUrl

        header: PageHeader {
            title: "Sign in with Slack"
        }

        onNavigationRequested: {
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
        pageStack.pop(undefined, PageStackAction.Animated)
    }

    function handleAccessTokenFail() {
        console.log('access token failed')
        webView.visible = true
    }
}
