import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import "../pages"
import ".."
import "../components"
import com.iskrembilen 1.0

LazyLoadDialog {
    property string teamId
    property bool isDnd: false
    property User user: null

    sourceComponent: Dialog {
        id: dndDialog
        property date currentDate: new Date()
        x: Math.round((window.width - width) / 2)
        y: Math.round(window.height / 6)
        width: Math.round(Math.min(window.width, window.height) / 3 * 2)
        height: Math.round(Math.min(window.width, window.height) / 3 * 2)
        modal: true
        focus: true
        property int initialHrs: -1
        property int initialMin: -1
        onVisibleChanged: {
            if (visible) {
                currentDate: new Date()
                initialHrs = currentDate.getHours()
                initialMin = currentDate.getMinutes()
                updateModel()
            }
        }

        standardButtons: Dialog.Ok|Dialog.Cancel

        title: qsTr("Set or add time for DnD snooze..")

        function formatNumber(number) {
            return number < 10 && number >= 0 ? "0" + number : number.toString()
        }

        function updateModel() {
            // Populate the model with days of the month. For example: [0, ..., 30]
            var previousIndex = dayTumbler.currentIndex
            var array = []
            var newDays = datePicker.days[monthTumbler.currentIndex]
            for (var i = 1; i <= newDays; ++i)
                array.push(i)
            dayTumbler.model = array
            console.log("update model", newDays - 1, previousIndex)
            dayTumbler.currentIndex = Math.min(newDays - 1, previousIndex)
        }

        contentItem: ColumnLayout {
            id: column
            anchors.centerIn: parent
            spacing: Theme.paddingLarge

            Button {
                visible: isDnd
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Cancel DnD mode")
                onClicked: {
                    SlackClient.cancelDnD(teamId)
                    dndDialog.close()
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Label {
                    text: isDnd ? "Set new interval:" : "Set interval:"
                    font.pointSize: Theme.fontSizeLarge
                }
                SpinBox {
                    id: spinMinutes
                    from: 1
                    to: 999
                }
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: isDnd ? qsTr("or set new time:") : qsTr("or set time:")
                font.pointSize: Theme.fontSizeLarge
            }

            Frame {
                id: frame
                padding: 0
                Layout.alignment: Qt.AlignHCenter
                RowLayout {
                    RowLayout {
                        id: datePicker

                        Layout.leftMargin: 20

                        property alias dayTumbler: dayTumbler
                        property alias monthTumbler: monthTumbler
                        property alias yearTumbler: yearTumbler

                        readonly property var days: [31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

                        Tumbler {
                            id: dayTumbler
                            currentIndex: currentDate.getDate() - 1
                            delegate: TumblerDelegate {
                                text: formatNumber(modelData)
                            }
                        }
                        Tumbler {
                            id: monthTumbler

                            onCurrentIndexChanged: updateModel()
                            currentIndex: currentDate.getMonth()

                            model: 12
                            delegate: TumblerDelegate {
                                text: window.locale.standaloneMonthName(modelData, Locale.ShortFormat)
                            }
                        }
                        Tumbler {
                            id: yearTumbler

                            // This array is populated with the next three years. For example: [2018, 2019, 2020]
                            readonly property var years: (function() {
                                var currentYear = currentDate.getFullYear()
                                return [0, 1, 2].map(function(value) { return value + currentYear; })
                            })()

                            model: years
                            delegate: TumblerDelegate {
                                text: formatNumber(modelData)
                            }
                        }
                    }
                    RowLayout {
                        id: rowTumbler
                        Layout.fillWidth: true
                        spacing: 0
                        Tumbler {
                            id: hoursTumbler
                            model: 24
                            currentIndex: initialHrs
                            delegate: TumblerDelegate {
                                text: formatNumber(modelData)
                            }
                        }

                        Tumbler {
                            id: minutesTumbler
                            model: 60
                            currentIndex: initialMin
                            delegate: TumblerDelegate {
                                text: formatNumber(modelData)
                            }
                        }
                    }

                }
            }
        }
        onAccepted: {
            var minutes = 0;
            var _currdate = new Date()
            var day = dayTumbler.currentIndex + 1
            var month = monthTumbler.currentIndex
            var year = yearTumbler.years[yearTumbler.currentIndex]
            var _proposedDate = new Date(year, month, day, hoursTumbler.currentIndex, minutesTumbler.currentIndex)

            console.log(_proposedDate, _currdate)
            //check if time was set with tumblers
            if (_proposedDate - _currdate >= 60000) {
                minutes = (Math.ceil((_proposedDate - _currdate) / 60000) * 60) / 60;
            } else {
                minutes = spinMinutes.value
            }

            console.log("set DnD for " + minutes + " minutes")
            SlackClient.setDnD(teamId, minutes)
        }
    }
}
