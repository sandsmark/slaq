import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2
import ".."

GridLayout {
    property alias fieldList: repeater.model

    id: grid
    columns: 2
    width: parent.width

    Repeater {
        id: repeater

        Label {
            Layout.columnSpan: model.isShort ? 1 : 2
            Layout.preferredWidth: model.isShort ? grid.width / 2 : grid.width
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            font.pointSize: Theme.fontSizeTiny
            font.weight: model.isTitle ? Font.Bold : Font.Normal
            text: model.content
        }
    }
}
