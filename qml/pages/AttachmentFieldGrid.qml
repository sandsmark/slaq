import QtQuick 2.8
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import ".."

RowLayout {
    property alias fieldList: repeater.model
    Layout.fillWidth: true

    id: grid
    spacing: Theme.paddingLarge

    Repeater {
        id: repeater

        ColumnLayout {
            Layout.columnSpan: repeater.model[index].fieldIsShort ? 1 : 2
            Layout.preferredWidth: repeater.model[index].fieldIsShort ? grid.implicitWidth / 2 : grid.implicitWidth
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
