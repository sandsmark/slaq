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
VERSION = 0.0.2.5
SRCMOC = .moc
MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .rcc

include(src/QGumboParser/QGumboParser.pri)
include(src/qtbreakpad/qtbreakpad.pri)

CONFIG(debug, debug|release) {
  DEFINES += SLAQ_DEBUG
  TO_DEPLOY = $$PWD/build/Debug/
} else {
  CONFIG += force_debug_info
  CONFIG += separate_debug_info
  DEFINES += SLAQ_RELEASE
  TO_DEPLOY = $$PWD/to_deploy/
}

DEFINES += SLAQ_VERSION=\\\"$$VERSION\\\"
windows: DEFINES += __PRETTY_FUNCTION__=__FUNCSIG__
# Translations
TRANSLATIONS += translations/slaq-fi.ts

CONFIG += c++11

QT += quick xml quickcontrols2 multimedia widgets websockets
QT += quick-private core-private gui-private qml-private quickcontrols2-private quicktemplates2-private

linux: QMAKE_LFLAGS += -fuse-ld=gold
INCLUDEPATH += src src/slackmodels
INCLUDEPATH += src/slaqtext
INCLUDEPATH += src/QGumboParser

QML_IMPORT_PATH =
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
    src/slaqtext/slaqtextdocument.cpp

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
    src/slaqtext/slaqtextdocument_p.h

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

win* {
OTHER_FILES += \
    3rdparty/ssl/win32/*.dll \

}
RESOURCES += \
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

contains(QT_ARCH, i386): ARCHITECTURE = x86
else: ARCHITECTURE = $$QT_ARCH

macx: PLATFORM = "mac"
else:win*: PLATFORM = "windows"
else:linux-*: PLATFORM = "linux-$${ARCHITECTURE}"
else: PLATFORM = "unknown"

BASENAME = $$(INSTALL_BASENAME)
isEmpty(BASENAME): BASENAME = slaq-$${PLATFORM}-$${VERSION}

macx {
# not yet supported
#    APPBUNDLE = "$$OUT_PWD/bin/$${IDE_APP_TARGET}.app"
#    BINDIST_SOURCE = "$$OUT_PWD/bin/$${IDE_APP_TARGET}.app"
#    deploylibs.commands = $$PWD/scripts/deployqtHelper_mac.sh \"$${APPBUNDLE}\" \"$$[QT_INSTALL_BINS]\" \"$$[QT_INSTALL_TRANSLATIONS]\" \"$$[QT_INSTALL_PLUGINS]\" \"$$[QT_INSTALL_IMPORTS]\" \"$$[QT_INSTALL_QML]\"
#    codesign.commands = codesign --deep -s \"$(SIGNING_IDENTITY)\" $(SIGNING_FLAGS) \"$${APPBUNDLE}\"
#    dmg.commands = python -u \"$$PWD/scripts/makedmg.py\" \"$${BASENAME}.dmg\" \"Slaq\" \"$$IDE_SOURCE_TREE\" \"$$OUT_PWD/bin\"
    #dmg.depends = deployqt
    QMAKE_EXTRA_TARGETS += codesign dmg
} else:linux-*: {
    BINDIST_SOURCE = "$${TO_DEPLOY}"
    BINDIST_EXCLUDE_ARG = "--exclude-toplevel"
    DEBUG_INFO_SOURCE = "$${TO_DEPLOY}"
    DEBUG_INFO_ARG = "--exclude-toplevel --debug"
    deploylibs.commands = python -u $$PWD/scripts/deploylibs.py -i \"$${TO_DEPLOY}/slaq\" \"$(QMAKE)\"
    deploylibs.depends = install
} else:win*: {
    BINDIST_SOURCE = "$${TO_DEPLOY}"
    BINDIST_EXCLUDE_ARG = "--exclude-toplevel"
}

INSTALLER_ARCHIVE_FROM_ENV = $$(INSTALLER_ARCHIVE)
isEmpty(INSTALLER_ARCHIVE_FROM_ENV) {
    INSTALLER_ARCHIVE = $$TO_DEPLOY/$${BASENAME}-installer-archive.7z
} else {
    INSTALLER_ARCHIVE = $$TO_DEPLOY/$$(INSTALLER_ARCHIVE)
}

DEBUG_INFO_ARCHIVE = $$TO_DEPLOY/$${BASENAME}-debuginfo-archive.7z

bindist_installer.commands = python -u $$PWD/scripts/createDistPackage.py $$BINDIST_EXCLUDE_ARG $${INSTALLER_ARCHIVE} \"$$BINDIST_SOURCE\"
debuginfodist_installer.commands = python -u $$PWD/scripts/createDistPackage.py $$DEBUG_INFO_ARG $${DEBUG_INFO_ARCHIVE} \"$$DEBUG_INFO_SOURCE\"

win* {
    deploylibs.commands ~= s,/,\\\\,g
    bindist.commands ~= s,/,\\\\,g
    bindist_installer.commands ~= s,/,\\\\,g
}

QMAKE_EXTRA_TARGETS += deploylibs bindist_installer debuginfodist_installer

INSTALLS += target other
