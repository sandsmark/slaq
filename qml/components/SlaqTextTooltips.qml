import QtQuick 2.11
import SlackComponents 1.0
import ".."

SlackText {
    id: slaqText
    verticalAlignment: Text.AlignVCenter
    wrapMode: Text.Wrap
    selectByMouse: true
    onLinkActivated: handleLink(link)
    onLinkHovered:  {
        if (link !== "") {
            msgToolTip.text = link
            var coord = mapToItem(teamsSwipe, x, y)
            msgToolTip.x = coord.x - msgToolTip.width/2
            msgToolTip.y = coord.y - (msgToolTip.height + Theme.paddingLarge)
            msgToolTip.open()
        } else {
            msgToolTip.close()
        }
    }
    onImageHovered:  {
        if (imagelink.length <= 0) {
            msgToolTip.close()
            return
        }

        msgToolTip.text = imagelink.replace("image://emoji/", "")
        var coord = mapToItem(teamsSwipe, x, 0)
        msgToolTip.x = coord.x - msgToolTip.width/2
        msgToolTip.y = coord.y - (msgToolTip.height + Theme.paddingSmall)
        msgToolTip.open()
    }
    itemFocusOnUnselect: messageInput
}

