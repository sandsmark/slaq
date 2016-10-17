.pragma library

function compareByName(a, b) {
    return a.name.localeCompare(b.name)
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
