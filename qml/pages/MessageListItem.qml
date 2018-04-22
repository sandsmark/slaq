import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Window 2.2
import com.iskrembilen.slaq 1.0 as Slack
import QtQuick.Layouts 1.3
import ".."

Item {
    id: item
    height: column.height + Theme.paddingMedium
    width: listView.width

    property color infoColor: item.highlighted ? palette.highlightedText : palette.text
    property color textColor: item.highlighted ? palette.highlightedText : palette.text

    Column {
        id: column
        width: parent.width - Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 4 : 2)
        anchors.verticalCenter: parent.verticalCenter
        x: Theme.paddingLarge * (Screen.devicePixelRatio > 90 ? 2 : 1)

        Row {
            width: parent.width
            height: childrenRect.height
            spacing: Theme.paddingMedium

            Image {
                height: Theme.avatarSize
                width: height
                source: Slack.Client.avatarUrl(user.id)
            }

            Column {
                height: childrenRect.height
                width: parent.width
                spacing: Theme.paddingMedium/2

                Row {
                    height: childrenRect.height
                    width: parent.width
                    spacing: Theme.paddingMedium

                    Label {
                        id: nickLabel
                        text: user.name
                        font.pointSize: Theme.fontSizeSmall
                        font.bold: true
                        color: textColor
                    }

                    Label {
                        text: new Date(parseInt(time, 10) * 1000).toLocaleString(Qt.locale(), "H:mm")
                        font.pointSize: Theme.fontSizeTiny
                        color: infoColor
                        height: nickLabel.height
                        verticalAlignment: "AlignBottom"
                    }
                }
                Label {
                    id: contentLabel
                    width: parent.width
                    font.pointSize: Theme.fontSizeSmall
                    color: textColor
                    visible: text.length > 0
                    text: content
                    onLinkActivated: handleLink(link)
                    wrapMode: Text.Wrap
                }
            }
        }

        Item {
            height: Theme.paddingMedium
            width: height
        }


        Item {
            height: Theme.paddingMedium
            width: height
            visible: contentLabel.visible && (imageRepeater.count > 0 || attachmentRepeater.count > 0)
        }

        Repeater {
            id: imageRepeater
            model: images

            Item {
                property bool expanded: false
                width: expanded ? listView.width - Theme.paddingLarge * 4 : model.thumbSize.width
                height: expanded ? width / (model.size.width / model.size.height) : model.thumbSize.height

                Image {
                    id: thumbImage
                    anchors.fill: parent
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: model.thumbUrl
                    sourceSize: Qt.size(model.thumbSize.width, model.thumbSize.height)
                    visible: !expanded
                }

                AnimatedImage {
                    id: fullImage
                    anchors.fill: parent
                    //to preserve memory, cache is turned off, so to see animation again need to re-expand image
                    //TODO: create settings to change the behavior
                    source: expanded ? model.url : ""
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    visible: expanded
                    playing: expanded
                    cache: false
                    smooth: true
                }

                ProgressBar {
                    anchors.centerIn: parent
                    opacity: value < 1
                    value: expanded ? fullImage.progress : thumbImage.progress
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                Rectangle {
                    anchors.fill: imageMouseArea
                    color: Qt.rgba(1, 1, 1, 0.1)
                    visible: imageMouseArea.containsMouse
                }

                MouseArea {
                    id: imageMouseArea
                    anchors.fill: parent
                    hoverEnabled: true

                    onClicked: {
                        if (Slack.Client.isDevice) {
                            pageStack.push(Qt.resolvedUrl("SlackImage.qml"), {"model": model})
                        } else {
                            parent.expanded = !parent.expanded
                        }
                    }
                }
            }
        }

        Repeater {
            id: attachmentRepeater
            model: attachments

            Attachment {
                width: column.width
                attachment: model
                onLinkClicked: handleLink(link)
            }
        }
    }

    function handleLink(link) {
        if (link.indexOf("slaq://") === 0) {
            console.log("local link", link)
        } else {
            console.log("external link", link)
            Qt.openUrlExternally(link)
        }
    }
}
