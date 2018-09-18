#!/usr/bin/env python
import argparse
import os
import shutil
import subprocess
import tempfile

import common

def parse_arguments():
    parser = argparse.ArgumentParser(description="Create Slaq package, filtering out debug information files.")
    parser.add_argument('--7z', help='path to 7z binary',
        default='7z.exe' if common.is_windows_platform() else '7z',
        metavar='<7z_binary>', dest='sevenzip')
    parser.add_argument('--debug', help='package only the files with debug information',
                        dest='debug', action='store_true', default=False)
    parser.add_argument('--exclude-toplevel', help='do not include the toplevel source directory itself in the resulting archive, only its contents',
                        dest='exclude_toplevel', action='store_true', default=True)
    parser.add_argument('target_archive', help='output 7z file to create')
    parser.add_argument('source_directory', help='source directory with the Slaq installation')
    return parser.parse_args()

def main():
    arguments = parse_arguments()
    tempdir_base = tempfile.mkdtemp()
    tempdir = os.path.join(tempdir_base, os.path.basename(arguments.source_directory))
    try:
        common.copytree(arguments.source_directory, tempdir, symlinks=True,
            ignore=(common.is_not_debug if arguments.debug else common.is_debug))
        zip_source = os.path.join(tempdir, '*') if arguments.exclude_toplevel else tempdir
        subprocess.check_call([arguments.sevenzip, 'a', '-mx9',
            arguments.target_archive, zip_source])
    finally:
        shutil.rmtree(tempdir_base)
if __name__ == "__main__":
    main()
