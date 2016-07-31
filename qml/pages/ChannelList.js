.import harbour.slackfish 1.0 as Slack

function init() {
    reloadChannels()
    Slack.Client.onChannelUpdated.connect(handleChannelUpdate)
}

function reloadChannels() {
    var channels = Slack.Client.getChannels()
    channels.sort(compareChannels)

    channelListModel.clear()
    channels.forEach(function(channel) {
        if (channel.isOpen) {
            channelListModel.append(channel)
        }
    })
}

function handleChannelUpdate(channel) {
    console.log("updated", channel.id, channel.name)

    for (var i = 0; i < channelListModel.count; i++) {
        var current = channelListModel.get(i)

        if (channel.id === current.id) {
            channelListModel.set(i, channel)
        }
    }
}

function compareChannels(a, b) {
    if (a.category === b.category) {
        return a.name.localeCompare(b.name)
    }
    else {
        if (a.category === "channel") {
            return -1
        }
        else if (b.category === "channel") {
            return 1
        }
        else {
            return a.name.localeCompare(b.name)
        }
    }
}
