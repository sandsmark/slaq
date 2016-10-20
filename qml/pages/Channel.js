.pragma library

function compareByName(a, b) {
    return a.name.localeCompare(b.name)
}

function isOpen(channel) {
    return channel.isOpen
}

function isJoinableChannel(channel) {
    return channel.type === "channel" && !channel.isOpen
}

function isJoinableChat(channel) {
    return channel.type === "im" && !channel.isOpen
}

function getIcon(model) {
    switch (model.type) {
        case "mpim":
        case "channel":
            return "icon-s-group-chat"
        case "group":
            return "icon-s-secure"
        case "im":
            return "icon-s-chat"
    }
}

function getIconColor(model, fallback) {
    switch (model.presence) {
        case "active":
            return "lawngreen"

        case "away":
            return "lightgrey"

        default:
            return fallback
    }
}
