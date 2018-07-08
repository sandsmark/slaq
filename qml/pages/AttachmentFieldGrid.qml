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

//        onModelChanged: {
//            console.log("attachment fields", model)
//        }

        Label {
            Layout.columnSpan: model.isShort ? 1 : 2
            Layout.preferredWidth: model.isShort ? grid.width / 2 : grid.width
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            font.pointSize: Theme.fontSizeTiny
            font.weight: model.isShort ? Font.Bold : Font.Normal
            text: model.title != undefined ? model.title : model.value != undefined ? model.value : ""
        }
    }
}
