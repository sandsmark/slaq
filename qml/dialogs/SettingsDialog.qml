import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0

Dialog {
    id: settingsDialog
    x: Math.round((window.width - width) / 2)
    y: Math.round(window.height / 6)
    width: Math.round(Math.min(window.width, window.height) / 3 * 2)
    modal: true
    focus: true
    title: qsTr("Settings")

    standardButtons: Dialog.Ok | Dialog.Cancel
    onAccepted: {
        ImagesCache.setEmojiImagesSet(setsBox.displayText)
        settingsDialog.close()
    }

    onRejected: {
        setsBox.currentIndex = setsBox.setIndex
        settingsDialog.close()
    }

    contentItem: ColumnLayout {
        id: settingsColumn
        spacing: 20

        RowLayout {
            spacing: 10

            Label {
                text: qsTr("Emojis:")
            }

            ComboBox {
                id: setsBox
                property int setIndex: 0
                model: ImagesCache.getEmojiImagesSetsNames();
                Component.onCompleted: {
                    setIndex = find(settings.emojisSet, Qt.MatchFixedString)
                    if (setIndex !== -1)
                        currentIndex = setIndex
                }
                Layout.fillWidth: true
            }
        }

        RowLayout {
            spacing: 10

            Label {
                text: qsTr("Theme") + ":"
            }

            ComboBox {
                property int setIndex: 0
                model: [qsTr("Light"), qsTr("Dark"), qsTr("System Default")]
                currentIndex: settings.theme
                onCurrentIndexChanged: settings.theme = currentIndex
                Layout.fillWidth: true
            }
        }
        GroupBox {
            title: qsTr("Performance")
            ColumnLayout {
                clip: true

                CheckBox {
                    id: unloadCb
                    text: qsTr("Unload view on teams switch (Reduce memory usage but slow down switching)")
                    checked: settings.unloadViewOnTeamSwitch
                    onClicked: {
                        settings.unloadViewOnTeamSwitch = checked
                    }
                }

                CheckBox {
                    text: qsTr("Load only last team on startup (faster startup)")
                    enabled: !unloadCb.checked
                    checked: settings.loadOnlyLastTeam || unloadCb.checked
                    onCheckedChanged: {
                        settings.loadOnlyLastTeam = checked
                    }
                }
            }
        }
    }
}
