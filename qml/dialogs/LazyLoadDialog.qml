import QtQuick 2.11
import QtQuick.Controls 2.4

Loader {
    id: loader
    active: false
    function open() {
        if (loader.active == false) {
            loader.active = true
        } else {
            loader.item.open()
        }
    }
    onStatusChanged: {
        if (loader.status == Loader.Ready) {
            loader.item.open()
        }
    }
}
