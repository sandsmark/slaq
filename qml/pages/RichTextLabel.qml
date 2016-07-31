import QtQuick 2.0

import Sailfish.Silica 1.0

Label {
    property string value: ""

    wrapMode: Text.Wrap
    textFormat: Text.RichText
    text: value.length > 0 ? "<style>a:link { color: " + Theme.highlightColor + "; }</style>" + value : ""
}
