import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."

Popup {
    id: popup
    width: Theme.headerSize * 11
    height: 500

    property string state: ""

    signal emojiSelected(string emoji)

    onAboutToHide: {
        emojiSelected("")
    }

    Connections {
        target: ImagesCache
        onEmojiReaded: {
            ldr.active = true
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
                listView.model = ImagesCache.getEmojiCategories();
            }

            interactive: true
            clip: true
            spacing: 10

            delegate: Column {
                id: col
                spacing: 5
                height: title.implicitHeight + grid.implicitHeight + spacing

                Text {
                    id: title
                    font.capitalization: Font.Capitalize
                    text: modelData
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
                        model: ImagesCache.getEmojisByCategory(modelData)
                        delegate: ItemDelegate {
                            id: control
                            text: model.modelData.unified
                            padding: 0
                            font.family: "Twitter Color Emoji"
                            font.pixelSize: Theme.headerSize - 2
                            display: AbstractButton.IconOnly
                            width: Theme.headerSize
                            height: Theme.headerSize
                            contentItem: Item {
                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    visible: !ImagesCache.isUnicode
                                    smooth: true
                                    cache: false
                                    source: "image://emoji/" + model.modelData.shortNames[0]
                                }

                                Text {
                                    visible: ImagesCache.isUnicode
                                    anchors.fill: parent
                                    text: control.text
                                    font: control.font
                                    renderType: Text.QtRendering
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                            onClicked: {
                                emojiSelected(model.modelData.unified)
                                popup.close()
                            }
                        }
                    }
                }
            }
        }
    }

    //List we have to have possibility to recreate list, so it will be redrawed from scratch
    Loader {
        id: ldr
        active: false
        property bool reloading: false
        asynchronous: true
        anchors.fill: parent
        sourceComponent: lvComponent
        onStatusChanged: {
            if (reloading && status === Loader.Null) {
                console.log("unloaded. loading back")
                ldr.sourceComponent = lvComponent
                reloading = false
            }
        }
    }
}
