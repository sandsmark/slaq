import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.slackfish 1.0 as Slack

Page {
    id: page

    property string channelId
    property variant channel
    property bool initialized: false

    SilicaFlickable {
        anchors.fill: parent

        BusyIndicator {
            id: loader
            visible: true
            running: visible
            size: BusyIndicatorSize.Large
            anchors.centerIn: parent
        }

        MessageListView {
            id: listView
            channel: page.channel
            anchors.fill: parent

            onLoadCompleted: {
                loader.visible = false
            }

            onLoadStarted: {
                loader.visible = true
            }
        }
    }

    ConnectionPanel {}

    Component.onCompleted: {
        page.channel = Slack.Client.getChannel(page.channelId)
    }

    onStatusChanged: {
        if (status === PageStatus.Active && !initialized) {
            initialized = true
            listView.loadMessages()
        }
        else if (status === PageStatus.Deactivating) {
            listView.markLatest()
        }
    }
}
