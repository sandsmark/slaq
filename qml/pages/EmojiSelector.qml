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

    ListView {
        id: listView
        anchors.fill: parent
        ScrollBar.vertical: ScrollBar { }

        model: SlackClient.getEmojiCategories();
        interactive: true
        clip: true
        spacing: 10

        delegate: Column {
            id: col
            spacing: 5
            height: title.implicitHeight + grid.implicitHeight + spacing
            Component.onCompleted: {
                grid.forceLayout()
            }

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
                    model: SlackClient.getEmojisByCategory(modelData)

                    delegate: ItemDelegate {
                        id: control
                        text: modelData
                        //text: "\xE2\x9D\x93"//"<li>&#x33;&#x20E3;</li>"
                        padding: 0
                        font.family: "Twitter Color Emoji"
                        font.pixelSize: Theme.headerSize - 2
                        //font.bold: true
                        display: AbstractButton.TextOnly
                        width: Theme.headerSize
                        height: Theme.headerSize
                        contentItem: Text {
                            text: control.text
                            font: control.font
                            //textFormat: Text.RichText
                            //renderType: Text.NativeRendering
                            renderType: Text.QtRendering
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        onClicked: {
                            emojiSelected(modelData)
                            popup.close()
                        }
                    }
                }
            }
        }
    }
}
