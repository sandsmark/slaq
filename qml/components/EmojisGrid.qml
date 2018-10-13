import QtQuick 2.11
import QtQuick.Controls 2.4
import SlaqQmlModels 1.0
import ".."

Grid {
    id: grid
    columns: 10
    rows: rep.count/columns + 1
    spacing: 0
    property alias model: rep.model
    Repeater {
        id: rep
        delegate: Rectangle {
            property EmojiInfo einfo: model.modelData != undefined ? model.modelData : rep.model.get(index)
            width: Theme.headerSize
            height: Theme.headerSize
            color: mouseArea.containsMouse ? "#bbbbbb" : "transparent"
            radius: 2
            Image {
                anchors.fill: parent
                anchors.margins: 2
                visible: !ImagesCache.isUnicode || (einfo.imagesExist & EmojiInfo.ImageSlackTeam)
                smooth: true
                cache: false
                source: "image://emoji/" + einfo.shortNames[0]//(model.modelData != undefined ? model.modelData.shortNames[0] : model.shortNames[0])
            }

            Label {
                visible: ImagesCache.isUnicode && !(einfo.imagesExist & EmojiInfo.ImageSlackTeam)
                anchors.fill: parent
                text: einfo.unified
                font.family: "Twitter Color Emoji"
                font.pixelSize: Theme.headerSize - 2
                renderType: Text.QtRendering
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onHoveredChanged: {
                    toolTip.text = einfo.shortNames[0]
                    toolTip.x = mapToItem(emojiPage, x, y).x - toolTip.width/2 + mouseArea.width/2
                    toolTip.y = mapToItem(emojiPage, x, y).y - toolTip.height - Theme.paddingSmall
                    toolTip.visible = containsMouse
                }

                onClicked: {
                    emojiSelected(einfo.unified !== "" ?
                                      einfo.unified : einfo.shortNames[0])
                    ImagesCache.addLastUsedEmoji(SlackClient.lastTeam, einfo.shortNames[0])
                    emojiPopup.close()
                }
            }
        }
    }
}
