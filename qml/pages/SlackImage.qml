import QtQuick 2.8
import QtQuick.Controls 2.3

Page {
    id: page

    property variant model
    property string teamId

    ProgressBar {
        opacity: parent.progress < 1
        value: image.progress
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }

    header: Text {
        text: model ? model.name : ""
    }

    Flickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: page.width

            Image {
                id: image
                source: "team://" + teamId + "/" + model.url
                width: parent.width
                fillMode: Image.PreserveAspectFit
                sourceSize.width: model.size.width
                sourceSize.height: model.size.height
            }
        }
    }
}
