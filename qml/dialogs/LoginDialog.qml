import QtQuick 2.11
import QtQuick.Controls 2.4
import Qt.labs.settings 1.0

import ".."

Dialog {
    id: loginDialog
    width: window.width
    height: window.height - window.header.height
    modal: true
    focus: true

    Connections {
        target: SlackClient
        onAccessTokenSuccess: {
            close()
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Login")
            enabled: email.text.length > 0 && password.text.length > 0 && teamName.text.length > 0
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                SlackClient.loginAttempt(email.text, password.text, teamName.text)
            }
        }

        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }

        onRejected: {
            close()
        }
    }

    title: "Sign in to Slack"

    Rectangle {
        anchors {
            fill: parent
            margins: loginDialog.footer.height
        }
        color: "transparent"
        border.color: "black"
        Column {
            anchors {
                fill: parent
                margins: 5
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("In order to be able to login to Slack workspace, user have to do the following steps using browser:\n" +
                           "1. Request invite for the workspace\n"+
                           "2. Go to invite url and create user account\n"+
                           "3. Confirm account and do first login\n")
                font.pixelSize: Theme.fontSizeLarge
            }
            TextField {
                id: email
                width: parent.width
                placeholderText: "Email..."
            }
            TextField {
                id: password
                width: parent.width
                passwordMaskDelay: 300
                echoMode: TextInput.Password
                placeholderText: "Password..."
            }

            TextField {
                id: teamName
                width: parent.width
                placeholderText: "Team/Subdomain name..."
            }
        }
    }
}
