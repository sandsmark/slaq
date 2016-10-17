.pragma library

function getDisplayDate(message) {
    return new Date(parseInt(message.time, 10) * 1000).toLocaleString(Qt.locale(), "MMMM d, yyyy")
}

function compareByTime(a, b) {
    return a.time - b.time
}
