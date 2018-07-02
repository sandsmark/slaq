import QtQuick 2.0

Text {
    text: model.preview_highlight
    wrapMode: Text.WordWrap
    textFormat: Text.RichText

    function hovered(isHovered) {}
}

