import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3 as DD

DD.MessageDialog {
    id: dialog
    property int timeout: 0

    standardButtons: DD.StandardButton.Ok

    property Timer timer: Timer {
        interval: dialog.timeout
        onTriggered: dialog.close()
    }

    function showError(errorSource, errorText, errorDetails, errorTimeout) {
        title = errorSource
        text = errorText
        detailedText = errorDetails
        timeout = errorTimeout
        open()
    }

    icon: DD.StandardIcon.Warning
    onVisibleChanged: {
        if (visible && timeout > 0) {
            timer.start()
        }
    }
}
