import QtQuick 2.8
import QtQuick.Controls 2.3
import ".."
import "../pages"

Drawer {
    id: searchResultsDialog

    y: Theme.paddingMedium
    bottomMargin: Theme.paddingMedium

    edge: Qt.RightEdge
    width: 0.6 * window.width
    height: window.height - Theme.paddingMedium

    function getDisplayDate(message) {
        return new Date(parseInt(message.ts, 10) * 1000).toLocaleString(Qt.locale(), "MMMM d, yyyy")
    }

    Connections {
        target: SlackClient
        onSearchResultsReady: {
            searchResultsModel.clear()
            messages.forEach(function(message) {
                message.day = getDisplayDate(message)
                searchResultsModel.append(message)
            })

            if (searchResultsModel.count) {
                searchResultsDialog.open()
            }
        }
    }

    ListModel {
        id: searchResultsModel
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: searchResultsModel
        ScrollIndicator.vertical: ScrollIndicator { }
        clip: true
        spacing: Theme.paddingMedium
        delegate: MessageListItem {
            isSearchResult: true
        }
    }
}
