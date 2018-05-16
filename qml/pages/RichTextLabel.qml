import QtQuick 2.8
import ".."
import QtQuick.Controls 2.3

Label {
    property string value: ""

    wrapMode: Text.Wrap
    textFormat: Text.RichText
    text: value.length > 0 ? "<style>a:link { color: " + Theme.highlightColor + "; }</style>" + value : ""
}
