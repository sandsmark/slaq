import QtQuick 2.11
import QtWebEngine 1.7
import QtQuick.Controls 2.4

Dialog {
    id: loginDialog
    width: window.width
    height: window.height - window.header.height
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

    Connections {
        target: SlackClient
        onAccessTokenSuccess: {
            webViewLoader.item.visible = false
            webViewLoader.item.stop()
            close()
        }
    }

    Rectangle {
        anchors {
            fill: parent
            margins: loginDialog.footer.height
        }
        color: "transparent"
        border.color: "black"

        Loader {
            id: webViewLoader
            anchors {
                fill: parent
                margins: 5
            }
            active: false
            onStatusChanged: {
                if (webViewLoader.status == Loader.Loading) {
                    SlackClient.onAccessTokenSuccess.connect(handleAccessTokenSuccess)
                    SlackClient.onAccessTokenFail.connect(handleAccessTokenFail)
                }
                if (webViewLoader.status == Loader.Null) {
                    SlackClient.onAccessTokenSuccess.disconnect(handleAccessTokenSuccess)
                    SlackClient.onAccessTokenFail.disconnect(handleAccessTokenFail)
                }
            }
        }
    }

    Component {
        id: webViewComponent
        WebEngineView {
            id: webView
            url: loginDialog.startUrl

            onLoadingChanged: {
                runJavaScript("window.localStorage.getItem(\"localConfig_v2\")", function(result) { try {
                    if (result === undefined) {
                        return
                    }
                    var localConfig = JSON.parse(result)
                    // Don't bother update the C++
                    var teamId = webView.url.toString().split('/')[4]
                    var bootConfig = {
                        'api_token': localConfig['teams'][teamId]['token'],
                        'team_id': teamId,
                        'user_id' :'dummyTODO', // not used
                        'team_url' :'dummyTODO', // not used
                    }
                    SlackClient.handleAccessTokenReply(bootConfig)
                } catch (err) { console.log(" error: " + err); } } )
            }
        }
    }

    function handleAccessTokenSuccess(userId, teamId, teamName) {
        loginDialog.close();
    }

    function handleAccessTokenFail() {
        console.warn('access token failed')
        webView.visible = true
    }
}
