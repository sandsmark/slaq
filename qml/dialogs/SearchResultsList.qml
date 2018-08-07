import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import ".."
import "../pages"

Drawer {
    id: searchResultsDialog

    y: Theme.paddingMedium
    bottomMargin: Theme.paddingMedium

    edge: Qt.RightEdge
    width: 0.6 * window.width
    height: window.height - Theme.paddingMedium
    property string searchQuery: ""
    property int currentPage: -1
    property int totalPages: -1

    Connections {
        target: SlackClient
        onSearchResultsReady: {
            if (listView.model == null) {
                listView.model = SlackClient.getSearchMessages(teamsSwipe.currentItem.item.teamId)
            }
            searchResultsDialog.searchQuery = query
            searchResultsDialog.currentPage = page
            searchResultsDialog.totalPages = pages
            searchResultsDialog.open()
            busy.visible = false
        }
        onSearchStarted: {
            busy.visible = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.paddingSmall
        spacing: Theme.paddingMedium
        Label {
            Layout.fillWidth: true
            font.bold: true
            font.pixelSize: Theme.fontSizeHuge
            text: qsTr("Search results for: ") + searchQuery
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollIndicator.vertical: ScrollIndicator { }
            verticalLayoutDirection: ListView.BottomToTop
            clip: true
            spacing: Theme.paddingMedium
            delegate: MessageListItem {
                width: listView.width - listView.ScrollIndicator.vertical.width
                isSearchResult: true
            }

            BusyIndicator {
                id: busy
                z: listView.z + 10
                anchors.centerIn: parent
                visible: false
                running: visible
            }
        }
    }
}
