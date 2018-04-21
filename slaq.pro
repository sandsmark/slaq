# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# App config
TARGET = slaq

# Translations
TRANSLATIONS += translations/slaq-fi.ts

QT += quick widgets webview

# Includes
INCLUDEPATH += ./QtWebsocket

# Check slack config


exists(localconfig.pri) {
    include(localconfig.pri)
} else {
    CLIENT_ID = $$(slack_client_id)
    CLIENT_SECRET = $$(slack_client_secret)
}


if(isEmpty(CLIENT_ID)) {
    error("No client id defined, ether define $slack_client_id or set up localconfig.pri")
}

if(isEmpty(CLIENT_SECRET)) {
    error("No client secret defined, ether define $slack_client_secret or set up localconfig.pri")
}
DEFINES += SLACK_CLIENT_ID=\\\"$${CLIENT_ID}\\\"
DEFINES += SLACK_CLIENT_SECRET=\\\"$${CLIENT_SECRET}\\\"

SOURCES += src/main.cpp \
    src/slackclient.cpp \
    src/slackconfig.cpp \
    src/QtWebsocket/QWsSocket.cpp \
    src/QtWebsocket/QWsFrame.cpp \
    src/QtWebsocket/functions.cpp \
    src/QtWebsocket/QWsHandshake.cpp \
    src/networkaccessmanagerfactory.cpp \
    src/networkaccessmanager.cpp \
    src/slackstream.cpp \
    src/storage.cpp \
    src/messageformatter.cpp \
    src/notificationlistener.cpp \
    src/filemodel.cpp

OTHER_FILES += qml/main.qml \
    qml/cover/CoverPage.qml \
    translations/*.ts \
    slaq.desktop \
    slaq.png

HEADERS += \
    src/slackclient.h \
    src/slackconfig.h \
    src/QtWebsocket/QWsSocket.h \
    src/QtWebsocket/QWsFrame.h \
    src/QtWebsocket/functions.h \
    src/QtWebsocket/QWsHandshake.h \
    src/networkaccessmanagerfactory.h \
    src/networkaccessmanager.h \
    src/slackstream.h \
    src/storage.h \
    src/messageformatter.h \
    src/notificationlistener.h \
    src/filemodel.h

DISTFILES += \
    qml/pages/Settings.js \
    qml/pages/LoginPage.qml \
    qml/pages/ChannelList.qml \
    qml/pages/ChannelList.js \
    qml/pages/Channel.qml \
    qml/pages/MessageListItem.qml \
    qml/pages/MessageInput.qml \
    qml/pages/ConnectionPanel.qml \
    qml/pages/ChannelListView.qml \
    qml/pages/MessageListView.qml \
    qml/pages/About.qml \
    qml/pages/RichTextLabel.qml \
    qml/pages/AttachmentFieldGrid.qml \
    qml/pages/Attachment.qml \
    qml/pages/Spacer.qml \
    qml/pages/ChannelSelect.qml \
    qml/pages/Channel.js \
    qml/pages/Message.js \
    qml/pages/GroupLeaveDialog.qml \
    qml/pages/ChatSelect.qml \
    qml/dialogs/ImagePicker.qml \
    qml/pages/FileSend.qml \
    qml/pages/SlackImage.qml \
    qml/pages/LoadingPage.qml

RESOURCES += \
    qml.qrc
