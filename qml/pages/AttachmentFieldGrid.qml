import QtQuick 2.8
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import ".."
import "../components"

Row {
    property alias fieldList: repeater.model

    id: grid
    spacing: Theme.paddingLarge

    Repeater {
        id: repeater

        Column {
            width: parent.width
            property int columnsNUm: repeater.model[index].fieldIsShort ? 1 : 2

            SlaqTextTooltips {
                width: parent.width/columnsNUm
                font.pointSize: Theme.fontSizeTiny
                chat: channelsList.channelModel
                font.weight: Font.Bold
                renderType: Text.QtRendering
                textFormat: Text.RichText
                text: repeater.model[index].fieldTitle
            }
            SlaqTextTooltips {
                width: parent.width/columnsNUm
                font.pointSize: Theme.fontSizeTiny
                chat: channelsList.channelModel
                font.weight: Font.Normal
                renderType: Text.QtRendering
                textFormat: Text.RichText
                text: repeater.model[index].fieldValue
            }
        }
    }
}
