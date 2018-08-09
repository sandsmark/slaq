import QtQuick 2.8
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import ".."

GridLayout {
    property alias fieldList: repeater.model

    id: grid
    columns: 2
    width: parent.width

    Repeater {
        id: repeater

        Column {
            Layout.columnSpan: repeater.model[index].fieldIsShort ? 1 : 2
            Layout.preferredWidth: repeater.model[index].fieldIsShort ? grid.width / 2 : grid.width
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            Label {
                font.pointSize: Theme.fontSizeTiny
                font.weight: Font.Bold
                renderType: Text.QtRendering
                textFormat: Text.RichText
                text: repeater.model[index].fieldTitle
            }
            Label {
                font.pointSize: Theme.fontSizeTiny
                font.weight: Font.Normal
                renderType: Text.QtRendering
                textFormat: Text.RichText
                text: repeater.model[index].fieldValue
            }
        }
    }
}
