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
VERSION = 0.0.1.2
SRCMOC = .moc
MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .rcc

DEFINES += SLAQ_VERSION=\\\"$$VERSION\\\"
# Translations
TRANSLATIONS += translations/slaq-fi.ts

QT += quick webview xml quickcontrols2 multimedia widgets
CONFIG += c++11
QT += websockets

INCLUDEPATH += src src/slackmodels

# enable for address sanitizer
#QMAKE_CXXFLAGS += -fno-omit-frame-pointer -fsanitize=address -fno-sanitize=vptr
#QMAKE_LIBS += -lasan
#dont forget to add to env: ASAN_OPTIONS=new_delete_type_mismatch=0

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
    src/slackmodels/UsersModel.cpp \
    src/slackmodels/searchmessagesmodel.cpp

OTHER_FILES += translations/*.ts \
    icons/slaq.svg \
    icons/slaq.png \
    slaq.desktop

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
    src/slackmodels/UsersModel.h \
    src/slackmodels/searchmessagesmodel.h

DISTFILES += \
    qml/dialogs/*.qml \
    qml/pages/*.js \
    qml/pages/*.qml \
    qml/*.qml \
    qml/components/*.qml \
    qml/components/TextViewer.qml

target.path = deploy

RESOURCES += \
    qml.qrc \
    data.qrc \
    fonts.qrc

include (src/modelshelper/QtQmlModels.pri)
include (src/qmlsorter/SortFilterProxyModel.pri)

other.files = $${OTHER_FILES}
other.path = deploy

#linux install files
linux: {
desktop.files   = slaq.desktop
desktop.path    = /usr/share/applications

icons.files     = icons/slaq.svg
icons.path      = /usr/share/icons/hicolor/scalable/apps
INSTALLS += desktop icons
}

INSTALLS += target other
