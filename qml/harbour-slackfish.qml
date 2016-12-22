import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    initialPage: Component { Loader { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: Orientation.All
    _defaultPageOrientations: Orientation.All

    function activateChannel(channelId) {
        console.log("Navigate to", channelId);
        pageStack.clear()
        pageStack.push(Qt.resolvedUrl("pages/ChannelList.qml"), {}, true)
        activate()
        pageStack.push(Qt.resolvedUrl("pages/Channel.qml"), {"channelId": channelId})
    }
}
