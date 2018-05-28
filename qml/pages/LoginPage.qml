import QtQuick 2.8
import QtWebView 1.1
import QtQuick.Controls 2.3

Page {
    id: page

    property string startUrl: "https://slack.com/signin"

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

        onLoadingChanged: {
            runJavaScript("JSON.stringify(boot_data)", function(result){
                if (result !== undefined) {
                    if (SlackClient.handleAccessTokenReply(JSON.parse(result))) {
                        webView.visible = false
                        webView.stop()
                    }
                }
            })
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
        pageStack.pop(undefined, StackView.Transition)
    }

    function handleAccessTokenFail() {
        console.log('access token failed')
        webView.visible = true
    }
}
