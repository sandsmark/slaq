import QtQuick 2.11
import QtQuick.Controls 2.4
import Qt.labs.settings 1.0

LazyLoadDialog {
    sourceComponent: Dialog {
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
                TextField {
                    id: email
                    width: parent.width
                    placeholderText: "Email..."
                }
                TextField {
                    id: password
                    width: parent.width
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
}
