.import com.iskrembilen.slaq 1.0 as Slack
.import "Channel.js" as Channel

function init() {
    reloadChannels()
    SlackClient.onInitSuccess.connect(reloadChannels)
    SlackClient.onChannelUpdated.connect(handleChannelUpdate)
    SlackClient.onChannelJoined.connect(handleChannelJoined)
    SlackClient.onChannelLeft.connect(handleChannelLeft)
}

function disconnect() {
    SlackClient.onInitSuccess.disconnect(reloadChannels)
    SlackClient.onChannelUpdated.disconnect(handleChannelUpdate)
    SlackClient.onChannelJoined.disconnect(handleChannelJoined)
    SlackClient.onChannelLeft.disconnect(handleChannelLeft)
}

function reloadChannels() {
    var channels = SlackClient.getChannels().filter(Channel.isOpen)
    channels.sort(compareByCategory)

    channelListModel.clear()
    channels.forEach(function(c) {
        c.section = getChannelSection(c)
        channelListModel.append(c)
    })
}

function handleChannelUpdate(channel) {
    reloadChannels()
}

function handleChannelJoined(channel) {
    reloadChannels()
}

function handleChannelLeft(channel) {
    for (var i = 0; i < channelListModel.count; i++) {
        var current = channelListModel.get(i)

        if (channel.id === current.id) {
            channelListModel.remove(i)
        }
    }
}

function getChannelSection(channel) {
    if (channel.unreadCount > 0) {
        return "unread"
    } else {
        return channel.category
    }
}

function compareChannels(a, b) {
    if (a.unreadCount === 0 && b.unreadCount === 0) {
        return compareByCategory(a, b)
    } else {
        if (a.unreadCount > 0 && b.unreadCount > 0) {
            return Channel.compareByName(a, b)
        } else if (a.unreadCount > 0) {
            return -1
        } else if (b.unreadCount > 0) {
            return 1
        } else {
            return Channel.compareByName(a, b)
        }
    }
}

function compareByCategory(a, b) {
    if (a.category === b.category) {
        return Channel.compareByName(a, b);
    }
    if (a.category === "chat" && b.category === "channel") {
        return 1;
    }
    return -1;
}


