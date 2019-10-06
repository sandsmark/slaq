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
VERSION = 0.0.3.3
SRCMOC = .moc
MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .rcc

exists(extern/qtcreator-breakpad/qtbreakpad.pri) {
    message("Enabling breakpad crash handler")
    include(extern/qtcreator-breakpad/qtbreakpad.pri)
    DEFINES += ENABLE_BREAKPAD
} else {
    message("Breakpad crash handler not enabled")
}

CONFIG(debug, debug|release) {
  DEFINES += SLAQ_DEBUG
} else {
  DEFINES += SLAQ_RELEASE
}

DEFINES += SLAQ_VERSION=\\\"$$VERSION\\\"
win32-msvc* {
    windows: DEFINES += __PRETTY_FUNCTION__=__FUNCSIG__
}
# Translations
TRANSLATIONS += translations/slaq-fi.ts

CONFIG += c++11

QT += quick xml quickcontrols2 multimedia widgets websockets webengine
QT += quick-private core-private gui-private qml-private quickcontrols2-private quicktemplates2-private

INCLUDEPATH += src src/slackmodels
INCLUDEPATH += src/slaqtext

QML_IMPORT_PATH += $$PWD

SOURCES += src/main.cpp \
    src/slackclient.cpp \
    src/slackconfig.cpp \
    src/networkaccessmanagerfactory.cpp \
    src/networkaccessmanager.cpp \
    src/slackstream.cpp \
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
    src/slackmodels/searchmessagesmodel.cpp \
    src/slackmodels/FilesSharesModel.cpp \
    src/slaqtext/slaqtext.cpp \
    src/slaqtext/slaqtextnode.cpp \
    src/slaqtext/slaqtextnodeengine.cpp \
    src/slaqtext/slaqtextdocument.cpp \
    src/youtubevideourlparser.cpp

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
    src/slackmodels/searchmessagesmodel.h \
    src/slackmodels/FilesSharesModel.h \
    src/slaqtext/slaqtext.h \
    src/slaqtext/slaqtext_p.h \
    src/slaqtext/slaqtextnode_p.h \
    src/slaqtext/slaqtextnodeengine_p.h \
    src/slaqtext/slaqtextdocument_p.h \
    src/youtubevideourlparser.h

DISTFILES += \
    qml/dialogs/*.qml \
    qml/pages/*.js \
    qml/pages/*.qml \
    qml/*.qml \
    scripts/*.py \
    qml/components/*.qml \
    qml/components/EmojisGrid.qml \
    qml/dialogs/EditMessageDialog.qml \
    qml/dialogs/EditMessageDialog.qml \
    qml/components/SlaqTextTooltips.qml

RESOURCES += \
    filetypes.qrc \
    qml.qrc \
    data.qrc \
    fonts.qrc

include (src/modelshelper/QtQmlModels.pri)
include (src/qmlsorter/SortFilterProxyModel.pri)

target.path = $$TO_DEPLOY
other.files = $${OTHER_FILES}
other.path = $$TO_DEPLOY

#linux install files
#linux: {
#desktop.files   = slaq.desktop
#desktop.path    = /usr/share/applications

#icons.files     = icons/slaq.svg
#icons.path      = /usr/share/icons/hicolor/scalable/apps
#INSTALLS += desktop icons
#}

INSTALLS += target other

# Create installer if passed `CONFIG+=create-installer`
# Disables webengine usage as well
create-installer {
    include(create-installer.pri)
    DEFINES += NO_WEBENGINE
}
