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

QT += quick webview xml quickcontrols2
CONFIG += c++1x
QT += websockets

SOURCES += src/main.cpp \
    src/slackclient.cpp \
    src/slackconfig.cpp \
    src/networkaccessmanagerfactory.cpp \
    src/networkaccessmanager.cpp \
    src/slackstream.cpp \
    src/storage.cpp \
    src/messageformatter.cpp \
    src/notificationlistener.cpp \
    src/filemodel.cpp \
    src/emojiprovider.cpp \
    src/imagescache.cpp \
    src/slackclientthreadspawner.cpp

OTHER_FILES += qml/main.qml \
    qml/cover/CoverPage.qml \
    translations/*.ts \
    slaq.desktop \
    slaq.png

HEADERS += \
    src/slackclient.h \
    src/slackconfig.h \
    src/networkaccessmanagerfactory.h \
    src/networkaccessmanager.h \
    src/slackstream.h \
    src/storage.h \
    src/messageformatter.h \
    src/notificationlistener.h \
    src/filemodel.h \
    src/emojiprovider.h \
    src/imagescache.h \
    src/slackclientthreadspawner.h \
    src/emojiinfo.h \
    src/teaminfo.h

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
    qml/pages/LoadingPage.qml \
    qml/pages/SettingsDialog.qml

target.path = /usr/bin/

RESOURCES += \
    qml.qrc

include (src/modelshelper/QtQmlModels.pri)
