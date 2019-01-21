import QtQuick 2.8
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import SlaqQmlModels 1.0

import ".."
import "../components"

Popup {
    id: emojiPopup
    width: Theme.headerSize * 11
    height: 500 + lastUsed.implicitHeight

    property string state: ""
    property string callerTeam: ""

    signal emojiSelected(string emoji)
    property bool dbLoaded: false
    property alias emojiPage: emojiPage

    onDbLoadedChanged: {
        if (dbLoaded && emojiPopup.opened) {
            ldr.active = true
        }
    }

    onAboutToShow: {
        if (dbLoaded) {
            ldr.active = true
        }
        lastUsed.model = ImagesCache.getLastUsedEmojisModel(SlackClient.lastTeam)
    }

    onAboutToHide: {
        emojiSelected("")
    }

    ToolTip {
        id: toolTip
        delay: 100
        timeout: 2000
    }

    Connections {
        target: ImagesCache
        onEmojisDatabaseReaded: {
            emojiPopup.dbLoaded = true
        }

        onEmojisUpdated: {
            console.log("emojis updated")
            ldr.reloading = true
            categoriesTabRepeater.model = undefined
            ldr.sourceComponent = undefined
        }

        onEmojisSetsIndexChanged: {
            ldr.reloading = true
            ldr.sourceComponent = undefined
        }
    }

    Component {
        id: lvComponent
        ListView {
            id: listView
            anchors.fill: parent
            ScrollBar.vertical: ScrollBar { }

            Component.onCompleted: {
                listView.model = emojiCategoriesModel
            }

            interactive: true
            clip: true
            spacing: 10

            delegate: Column {
                id: col
                spacing: 5
                visible: isVisible
                enabled: visible
                height: visible ? title.implicitHeight + grid.implicitHeight + spacing : 0

                Label {
                    id: title
                    font.capitalization: Font.Capitalize
                    text: visibleName
                    font.pixelSize: 24
                    font.bold: true
                }
                EmojisGrid {
                    id: grid
                    model: ImagesCache.getEmojisByCategory(category, SlackClient.lastTeam)
                    width: listView.width
                }
            }
        }
    }

    Page {
        id: emojiPage
        anchors.fill: parent
        header: ToolBar {
            background: Item{}
            RowLayout {
                anchors.fill: parent
                Repeater {
                    id: categoriesTabRepeater
                    model: emojiCategoriesModel
                    EmojiToolButton {
                        text: unicode
                        visible: isVisible
                        font.bold: true
                        Layout.fillWidth: true
                        onClicked: {
                            ldr.item.positionViewAtIndex(index, ListView.Beginning)
                        }
                        hoverEnabled: true
                        ToolTip.delay: 300
                        ToolTip.text: visibleName
                        ToolTip.timeout: 2000
                        ToolTip.visible: hovered
                    }
                }
            }
        }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 3

            EmojisGrid {
                id: lastUsed
            }

            RowLayout {
                spacing: 10

                Label {
                    text: "Select Emojis:"
                }

                ComboBox {
                    id: setsBox
                    property int setIndex: -1
                    model: ImagesCache.getEmojiImagesSetsNames();
                    Component.onCompleted: {
                        setIndex = find(settings.emojisSet, Qt.MatchFixedString)
                        if (setIndex !== -1) {
                            currentIndex = setIndex
                        }
                    }
                    Layout.fillWidth: true
                    onActivated: {
                        if (index !== -1) {
                            ImagesCache.setEmojiImagesSet(textAt(index))
                        }
                    }
                }
            }

            Loader {
                id: ldr
                active: false
                property bool reloading: false
                asynchronous: true
                Layout.fillWidth: true
                Layout.fillHeight: true
                //sourceComponent: lvComponent
                onStatusChanged: {
                    if (reloading && status === Loader.Null) {
                        ldr.sourceComponent = lvComponent
                        categoriesTabRepeater.model = emojiCategoriesModel
                        reloading = false
                    }
                }
            }
        }
    }
}
