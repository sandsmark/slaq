import QtQuick 2.11
import QtQuick.Controls 2.4
import com.iskrembilen 1.0

Control {
    property User user: null
    property bool showNoImage: false
    hoverEnabled: true
    ToolTip.delay: 100
    ToolTip.visible: hovered && user.status !== ""
    ToolTip.text: user != null ? user.status : ""

    Image {
        id: image
        sourceSize: Qt.size(parent.width, parent.height)
        visible: !ImagesCache.isUnicode
        smooth: true
        cache: false
        source: visible && user != null && user.statusEmoji.length > 0 ? "image://emoji/" + user.statusEmoji.slice(1, -1) :
                                                        showNoImage ? "qrc:/icons/no-image.png" : ""
    }
    Label {
        visible: ImagesCache.isUnicode
        text: user != null ? ImagesCache.getEmojiByName(user.statusEmoji.slice(1, -1)) : ""
        font.family: "Twitter Color Emoji"
        font.pixelSize: parent.height - 2
        font.italic: false
        renderType: Text.QtRendering
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
}
