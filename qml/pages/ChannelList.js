.import harbour.slackfish 1.0 as Slack
.import "Channel.js" as Channel

function init() {
    reloadChannels()
    Slack.Client.onInitSuccess.connect(reloadChannels)
    Slack.Client.onChannelUpdated.connect(handleChannelUpdate)
    Slack.Client.onChannelJoined.connect(handleChannelJoined)
}

function reloadChannels() {
    var channels = Slack.Client.getChannels().filter(Channel.isOpen)
    channels.sort(compareChannels)

    channelListModel.clear()
    channels.forEach(function(c) {
        channelListModel.append(c)
    })
}

function handleChannelUpdate(channel) {
    for (var i = 0; i < channelListModel.count; i++) {
        var current = channelListModel.get(i)

        if (channel.id === current.id) {
            channelListModel.set(i, channel)
        }
    }
}

function handleChannelJoined(channel) {
    reloadChannels()
}

function compareChannels(a, b) {
    if (a.category === b.category) {
        return Channel.compareByName(a, b)
    }
    else {
        if (a.category === "channel") {
            return -1
        }
        else if (b.category === "channel") {
            return 1
        }
        else {
            return Channel.compareByName(a, b)
        }
    }
}
