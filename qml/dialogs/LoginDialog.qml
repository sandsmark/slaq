import QtQuick 2.8
import QtWebView 1.1
import QtQuick.Controls 2.3

Dialog {
    id: loginDialog
    width: window.width
    height: window.height
    modal: true
    focus: true

    standardButtons: Dialog.Ok | Dialog.Cancel

    property string startUrl: "https://slack.com/signin"

    title: "Sign in with Slack"

    onOpened: {
        webViewLoader.sourceComponent = webViewComponent
        webViewLoader.active = true
    }
    onClosed: {
        webViewLoader.active = false
        webViewLoader.sourceComponent = undefined
    }

    Rectangle {
        anchors {
            fill: parent
            margins: 5
        }
        color: "transparent"
        border.color: "black"
    }

    Component {
        id: webViewComponent
        WebView {
            id: webView
            url: loginDialog.startUrl

            onLoadingChanged: {
                runJavaScript("JSON.stringify(boot_data)", function(result){
                    if (result !== undefined) {
                        if (SlackClient.handleAccessTokenReply(JSON.parse(result))) {
                            webView.visible = false
                            webView.stop()
                            loginDialog.close();
                        }
                    }
                })
            }
        }
    }

    Loader {
        id: webViewLoader
        active: false
        anchors.fill: parent
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
        loginDialog.close();
    }

    function handleAccessTokenFail() {
        console.log('access token failed')
        webView.visible = true
    }
}
