import QtQuick 2.8
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import SlaqQmlModels 1.0
import ".."

Popup {
    id: popup
    width: Theme.headerSize * 11
    height: 500

    property string state: ""

    signal emojiSelected(string emoji)
    property bool dbLoaded: false

    onDbLoadedChanged: {
        if (dbLoaded && popup.opened) {
            ldr.active = true
        }
    }

    onAboutToShow: {
        if (dbLoaded) {
            ldr.active = true
        }
    }

    onAboutToHide: {
        emojiSelected("")
    }

    Connections {
        target: ImagesCache
        onEmojisDatabaseReaded: {
            popup.dbLoaded = true
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
                Grid {
                    id: grid
                    width: listView.width
                    columns: 10
                    rows: rep.count/columns + 1
                    spacing: 0
                    Repeater {
                        id: rep
                        model: ImagesCache.getEmojisByCategory(category, SlackClient.lastTeam)
                        delegate: Rectangle {
                            width: Theme.headerSize
                            height: Theme.headerSize
                            color: mouseArea.containsMouse ? "#bbbbbb" : "transparent"
                            radius: 2
                            Image {
                                anchors.fill: parent
                                anchors.margins: 2
                                visible: !ImagesCache.isUnicode || (model.modelData.imagesExist & EmojiInfo.ImageSlackTeam)
                                smooth: true
                                cache: false
                                source: "image://emoji/" + model.modelData.shortNames[0]
                            }

                            Label {
                                visible: ImagesCache.isUnicode && !(model.modelData.imagesExist & EmojiInfo.ImageSlackTeam)
                                anchors.fill: parent
                                text: model.modelData.unified
                                font.family: "Twitter Color Emoji"
                                font.pixelSize: Theme.headerSize - 2
                                renderType: Text.QtRendering
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                            ToolTip {
                                delay: 300
                                text: model.modelData.shortNames[0]
                                timeout: 2000
                                visible: mouseArea.containsMouse
                            }

                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    emojiSelected(model.modelData.unified !== "" ?
                                                      model.modelData.unified : model.modelData.shortNames[0])
                                    popup.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Page {
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
                sourceComponent: lvComponent
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
