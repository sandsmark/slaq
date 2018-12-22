import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3 as DD

DD.MessageDialog {
    id: dialog
    property int timeout: 0

    property Connections conn: Connections {
        target: SlackClient
        onError: {
            showError(err.domain, err.error_str, err.details, err.timeout)
        }
    }

    standardButtons: DD.StandardButton.Ok

    property Timer timer: Timer {
        interval: dialog.timeout
        onTriggered: dialog.close()
    }

    function showError(errorSource, errorText, errorDetails, errorTimeout) {
        title = errorSource
        text = errorText
        detailedText = errorDetails !== undefined ? errorDetails : ""
        timeout = errorTimeout !== undefined ? errorTimeout : 0
        open()
    }

    icon: DD.StandardIcon.Warning
    onVisibleChanged: {
        if (visible && timeout > 0) {
            timer.start()
        }
    }
}
