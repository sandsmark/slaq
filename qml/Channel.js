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
    var ret = ""
    switch (model.type) {
        case "mpim":
        case "channel":
            if (model.presence === "active") {
                return "irc-channel-active"
            } else {
                return "irc-channel-inactive"
            }
            break;
//            return "icon-s-group-chat"
        case "group":
            return "icon-s-secure"
        case "im":
            if (model.presence === "active") {
                return "im-user"
            } else {
                return "im-user-inactive"
            }

//            ret = "im-user"
//            return "icon-s-chat"
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
