!exists(extern/qtcreator-breakpad/qtbreakpad.pri) {
    error("Please check out the extern/qtcreator-breakpad submodule")
}

CONFIG(debug, debug|release) {
  TO_DEPLOY = $$PWD/build/Debug/
} else {
  CONFIG += force_debug_info
  CONFIG += separate_debug_info
  TO_DEPLOY = $$PWD/to_deploy/
}

win*: {
    contains(QT_ARCH, i386) {
        ## Win32
        message("Win32 build")
        OTHER_FILES += $$clean_path($$[QT_INSTALL_PREFIX]/../../Tools/OpenSSL/Win_x86/bin)/*.dll
    } else {
        ##Win64
        message("Win64 build")
        OTHER_FILES += $$clean_path($$[QT_INSTALL_PREFIX]/../../Tools/OpenSSL/Win_x64/bin)/*.dll
    }
}

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
