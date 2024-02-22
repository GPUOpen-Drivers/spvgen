#!/usr/bin/env python
##
 #######################################################################################################################
 #
 #  Copyright (c) 2024 Advanced Micro Devices, Inc. All Rights Reserved.
 #
 #  Permission is hereby granted, free of charge, to any person obtaining a copy
 #  of this software and associated documentation files (the "Software"), to deal
 #  in the Software without restriction, including without limitation the rights
 #  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 #  copies of the Software, and to permit persons to whom the Software is
 #  furnished to do so, subject to the following conditions:
 #
 #  The above copyright notice and this permission notice shall be included in all
 #  copies or substantial portions of the Software.
 #
 #  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 #  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 #  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 #  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 #  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 #  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 #  SOFTWARE.
 #
 #######################################################################################################################



"""Module fetch external source."""

# This script is used to download the glslang, SPIRV-Tools, and SPIRV-Headers from github.

# __future__ must be at the beginning of the file
from __future__ import print_function

import os
import sys
import argparse

target_dir = os.getcwd() + "/" # target directory the source code downloaded to.

class GitRepo:
    """Class representing a git repo"""
    def __init__(self, https_url, module_name, extract_dir):
        self.https_url   = https_url
        self.module_name = module_name
        self.extract_dir = extract_dir
        self.revision    = ''

    def get_revision(self):
        """Function get revision."""
        src_file = globals()['target_dir'] + "../CHANGES"
        if not os.path.exists(src_file):
            print("Error:", src_file, "does not exist!!!")
            sys.exit(1)

        with open(src_file,'r', encoding="utf-8") as rev_file:
            lines = rev_file.readlines()
            found = False
            for line in lines:
                if found:
                    if line.find("Commit:") == 0:
                        # line is in format: "Commit: HASH". Save only hash.
                        self.revision = line[8:]
                        break
                found = self.module_name.lower() in line.lower()
            rev_file.close()

        if not found:
            print("Error: Revision is not gotten from", src_file, "correctly, please check it!!!")
            sys.exit(1)
        else:
            print("Get the revision of", self.extract_dir + ":", self.revision)

    def checkout(self):
        """Function checkout code."""
        full_dst_path = os.path.join(globals()['target_dir'], self.extract_dir)
        if not os.path.exists(full_dst_path):
            os.system("git clone " + self.https_url + " " + full_dst_path)

        os.chdir(full_dst_path)
        os.system("git fetch")
        os.system("git checkout " + self.revision)

PACKAGES = [
    GitRepo("https://github.com/KhronosGroup/glslang.git",
            "glslang",
            "glslang"),

    GitRepo("https://github.com/KhronosGroup/SPIRV-Tools.git",
            "spirv-tools",
            "SPIRV-tools"),

    GitRepo("https://github.com/KhronosGroup/SPIRV-Headers.git",
            "spirv-headers",
            "SPIRV-tools/external/spirv-headers"),

    GitRepo("https://github.com/KhronosGroup/SPIRV-Cross.git",
            "spirv-cross",
            "SPIRV-cross"),
]

def get_opt():
    """Class for parser arguments."""
    parser = argparse.ArgumentParser(description='Fetching external source.')

    parser.add_argument("-t", "--target_dir", action="store",
                  type=str,
                  default=None,
                  dest="target_dir",
                  help="the target directory source code downloaded to")

    args = parser.parse_args()

    if args.target_dir:
        print(f"The source code is downloaded to {args.target_dir}")
        globals()['target_dir'] = args.target_dir
    else:
        print("The target directory is not specified, using default:", globals()['target_dir'])

def download_source_code():
    """Class for download source code."""
    os.chdir(globals()['target_dir'])

    # Touch the spvgen CMakeLists.txt to ensure that the new directories get used.
    os.utime('../CMakeLists.txt', None)

    for pkg in PACKAGES:
        pkg.get_revision()
        pkg.checkout()

get_opt()
download_source_code()
