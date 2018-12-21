#!/usr/bin/env python
import os
import locale
import sys
import getopt
import subprocess
import re
import shutil
from glob import glob

import common

ignoreErrors = False
debug_build = False
encoding = locale.getdefaultlocale()[1]

def usage():
    print("Usage: %s <existing_slaq_binary> [qmake_path]" % os.path.basename(sys.argv[0]))

def which(program):
    def is_exe(fpath):
        return os.path.exists(fpath) and os.access(fpath, os.X_OK)

    fpath = os.path.dirname(program)
    if fpath:
        if is_exe(program):
            return program
        if common.is_windows_platform():
            if is_exe(program + ".exe"):
                return program  + ".exe"
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
            if common.is_windows_platform():
                if is_exe(exe_file + ".exe"):
                    return exe_file  + ".exe"

    return None

def is_debug(fpath):
    # match all Qt Core dlls from Qt4, Qt5beta2 and Qt5rc1 and later
    # which all have the number at different places
    coredebug = re.compile(r'Qt[1-9]?Core[1-9]?d[1-9]?.dll')
    # bootstrap exception
    if coredebug.search(fpath):
        return True
    output = subprocess.check_output(['dumpbin', '/imports', fpath])
    return coredebug.search(output.decode(encoding)) != None

def op_failed(details = None):
    if details != None:
        print(details)
    if ignoreErrors == False:
        print("Error: operation failed!")
        sys.exit(2)
    else:
        print("Error: operation failed, but proceeding gracefully.")

def is_ignored_windows_file(use_debug, basepath, filename):
    ignore_patterns = ['.lib', '.pdb', '.exp', '.ilk']
    if use_debug:
        ignore_patterns.extend(['libEGL.dll', 'libGLESv2.dll'])
    else:
        ignore_patterns.extend(['libEGLd.dll', 'libGLESv2d.dll'])
    for ip in ignore_patterns:
        if filename.endswith(ip):
            return True
    if filename.endswith('.dll'):
        filepath = os.path.join(basepath, filename)
        if use_debug != is_debug(filepath):
            return True
    return False

def is_ignored_linux_file(use_debug, basepath, filename):
    if filename.lower().endswith(('.debug')):
        return True
    ignore_plugins = ['libqeglfs.so', 'libqminimalegl.so', 'libqoffscreen.so', 'libqwayland-egl.so',
    'libqwayland-xcomposite-egl.so', 'libqwebgl.so', 'libqlinuxfb.so', 'libqminimal.so',
    'libqvnc.so', 'libqwayland-generic.so', 'libqwayland-xcomposite-glx.so']
    for ip in ignore_plugins:
        if filename == ip:
            return True
    return False

def ignored_qt_lib_files(path, filenames):
    if not common.is_windows_platform():
        return [fn for fn in filenames if is_ignored_linux_file(debug_build, path, fn)]
    return [fn for fn in filenames if is_ignored_windows_file(debug_build, path, fn)]

def copy_libs(qt_prefix_path, target_qt_prefix_path, qt_plugin_dir, qt_import_dir, qt_qml_dir, plugins, imports, qmlimports, qtlibs):
    print("copying Qt libraries...")
    if common.is_windows_platform():
        lib_dest = os.path.join(target_qt_prefix_path)
    else:
        lib_dest = os.path.join(target_qt_prefix_path, 'lib')

    if not os.path.exists(lib_dest):
        os.makedirs(lib_dest)

    for library in qtlibs:
        print(library, '->', lib_dest)
        if os.path.islink(library):
            linkto = os.readlink(library)
            print("is link:", library, linkto, os.path.realpath(library))
            shutil.copy(os.path.realpath(library), lib_dest)
            try:
                os.symlink(linkto, os.path.join(lib_dest, os.path.basename(library)))
            except OSError:
                op_failed("Link already exists!")
        else:
            shutil.copy(library, lib_dest)

    print("Copying plugins:", plugins)
    for plugin in plugins:
        target = os.path.join(target_qt_prefix_path, 'plugins', plugin)
        if (os.path.exists(target)):
            shutil.rmtree(target)
        pluginPath = os.path.join(qt_plugin_dir, plugin)
        if (os.path.exists(pluginPath)):
            print('{0} -> {1}'.format(pluginPath, target))
            common.copytree(pluginPath, target, ignore=ignored_qt_lib_files, symlinks=True)

    print("Copying imports:", imports)
    for qtimport in imports:
        target = os.path.join(target_qt_prefix_path, 'imports', qtimport)
        if (os.path.exists(target)):
            shutil.rmtree(target)
        import_path = os.path.join(qt_import_dir, qtimport)
        if os.path.exists(import_path):
            print('{0} -> {1}'.format(import_path, target))
            common.copytree(import_path, target, ignore=ignored_qt_lib_files, symlinks=True)

    if (os.path.exists(qt_qml_dir)):
        print("Copying qt quick 2 imports")
        for qmlimport in qmlimports:
            source = os.path.join(qt_qml_dir, qmlimport)
            target = os.path.join(target_qt_prefix_path, 'qml', qmlimport)
            if (os.path.exists(target)):
                shutil.rmtree(target)
            print('{0} -> {1}'.format(source, target))
            common.copytree(source, target, ignore=ignored_qt_lib_files, symlinks=True)


def add_qt_conf(target_path, qt_prefix_path):
    qtconf_filepath = os.path.join(target_path, 'qt.conf')
    prefix_path = os.path.relpath(qt_prefix_path, target_path).replace('\\', '/')
    print('Creating qt.conf in "{0}":'.format(qtconf_filepath))
    f = open(qtconf_filepath, 'w')
    f.write('[Paths]\n')
    f.write('Prefix={0}\n'.format(prefix_path))
    f.write('Binaries={0}\n'.format('bin' if common.is_linux_platform() else '.'))
    f.write('Libraries={0}\n'.format('lib' if common.is_linux_platform() else '.'))
    f.write('Plugins=plugins\n')
    f.write('Imports=imports\n')
    f.write('Qml2Imports=qml\n')
    f.write('Data=data\n')
    f.close()

def copyPreservingLinks(source, destination):
    if os.path.islink(source):
        linkto = os.readlink(source)
        destFilePath = destination
        if os.path.isdir(destination):
            destFilePath = os.path.join(destination, os.path.basename(source))
        os.symlink(linkto, destFilePath)
    else:
        shutil.copy(source, destination)


def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], 'hi', ['help', 'ignore-errors'])
    except getopt.GetoptError:
        usage()
        sys.exit(2)
    for o, _ in opts:
        if o in ('-h', '--help'):
            usage()
            sys.exit(0)
        if o in ('-i', '--ignore-errors'):
            global ignoreErrors
            ignoreErrors = True
            print("Note: Ignoring all errors")

    slaq_binary = os.path.abspath(args[0])
    if common.is_windows_platform() and not slaq_binary.lower().endswith(".exe"):
        slaq_binary = slaq_binary + ".exe"

    if len(args) < 1 or not os.path.isfile(slaq_binary):
        usage()
        sys.exit(2)

    slaq_binary_path = os.path.dirname(slaq_binary)
    install_dir = os.path.abspath(slaq_binary_path)
    qt_deploy_prefix = install_dir
    qmake_bin = 'qmake'
    if len(args) > 1:
        qmake_bin = args[1]
    qmake_bin = which(qmake_bin)

    if qmake_bin == None:
        print("Cannot find required binary 'qmake'.")
        sys.exit(2)

    chrpath_bin = None
    if common.is_linux_platform():
        chrpath_bin = which('chrpath')
        if chrpath_bin == None:
            print("Cannot find required binary 'chrpath'.")
            sys.exit(2)

    qt_install_info = common.get_qt_install_info(qmake_bin)
    QT_INSTALL_PREFIX = qt_install_info['QT_INSTALL_PREFIX']
    QT_INSTALL_LIBS = qt_install_info['QT_INSTALL_LIBS']
    QT_INSTALL_BINS = qt_install_info['QT_INSTALL_BINS']
    QT_INSTALL_PLUGINS = qt_install_info['QT_INSTALL_PLUGINS']
    QT_INSTALL_IMPORTS = qt_install_info['QT_INSTALL_IMPORTS']
    QT_INSTALL_QML = qt_install_info['QT_INSTALL_QML']

    plugins = ['codecs', 'audio', 'imageformats', 'platformthemes',
               'mediaservice', 'platforms', 'bearer', 'xcbglintegrations']
    imports = ['Qt']
    qmlimports = ['Qt', 'QtQuick', 'QtQml', 'QtMultimedia', 'QtGraphicalEffects', 'QtQuick.2']
    extralibs = ['libstdc++.so']

    if common.is_windows_platform():
        libraries = []
        copy_libs(qt_deploy_prefix, QT_INSTALL_PLUGINS, QT_INSTALL_IMPORTS, QT_INSTALL_QML,
        plugins, imports, qmlimports, libraries)
    else:
        lddlibs = common.ldd([slaq_binary])
        libraries = [lib for lib in lddlibs if QT_INSTALL_LIBS in lib or any(s in lib for s in extralibs) ]
        #add extra lib no included in ldd
        libraries.append(os.path.join(QT_INSTALL_LIBS, "libQt5XcbQpa.so.5"))
        libraries.append(os.path.join(QT_INSTALL_LIBS, "libQt5DBus.so.5"))
        libraries.append(os.path.join(QT_INSTALL_LIBS, "libQt5MultimediaGstTools.so.5"))
        libraries.append(os.path.join(QT_INSTALL_LIBS, "libQt5MultimediaWidgets.so.5"))

        copy_libs(QT_INSTALL_PREFIX, qt_deploy_prefix, QT_INSTALL_PLUGINS, QT_INSTALL_IMPORTS,
        QT_INSTALL_QML, plugins, imports, qmlimports, libraries)

    if not common.is_windows_platform():
        print("fixing rpaths...")
        common.fix_rpaths(install_dir, os.path.join(qt_deploy_prefix, 'lib'), qt_install_info, chrpath_bin)
    add_qt_conf(install_dir, qt_deploy_prefix)

if __name__ == "__main__":
    if common.is_mac_platform():
        print("macOS is not supported by this script, please use macqtdeploy!")
        sys.exit(2)
    else:
        main()
