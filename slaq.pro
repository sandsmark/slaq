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

QT += quick webview xml quickcontrols2 multimedia widgets
CONFIG += c++1x
QT += websockets

INCLUDEPATH += src src/slackmodels

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
    src/slackclientthreadspawner.cpp \
    src/downloadmanager.cpp \
    src/teaminfo.cpp \
    src/slackmodels/ChatsModel.cpp \
    src/slackmodels/MessagesModel.cpp \
    src/slackmodels/UsersModel.cpp

OTHER_FILES += translations/*.ts \
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
    src/teaminfo.h \
    src/downloadmanager.h \
    src/debugblock.h \
    src/slackmodels/ChatsModel.h \
    src/slackmodels/MessagesModel.h \
    src/slackmodels/UsersModel.h

DISTFILES += \
    qml/dialogs/*.qml \
    qml/pages/*.js \
    qml/pages/*.qml \
    qml/*.qml \
    qml/components/*.qml

target.path = /usr/bin/

RESOURCES += \
    qml.qrc \
    data.qrc \
    fonts.qrc

include (src/modelshelper/QtQmlModels.pri)
