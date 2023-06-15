#!/usr/bin/env python
##
 #######################################################################################################################
 #
 #  Copyright (c) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
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



# This script is used to download the glslang, SPIRV-Tools, and SPIRV-Headers from github.

# __future__ must be at the beginning of the file
from __future__ import print_function

import sys
import os
import string

from optparse import OptionParser

TargetDir = os.getcwd() + "/"; # target directory the source code downloaded to.

class GitRepo:
    def __init__(self, httpsUrl, moduleName, extractDir):
        self.httpsUrl   = httpsUrl
        self.moduleName = moduleName
        self.extractDir = extractDir

    def GetRevision(self):
        SrcFile = TargetDir + "../CHANGES";
        if not os.path.exists(SrcFile):
            print("Error: " + SrcFile + " does not exist!!!");
            exit(1);

        revFile = open(SrcFile,'r');
        lines = revFile.readlines();
        found = False;
        for line in lines:
            if (found == True):
                if (line.find("Commit:") == 0):
                    self.revision = line[7:];
                    break;
            if (self.moduleName.lower() in line.lower()):
                found = True;
            else:
                found = False;
        revFile.close();

        if (found == False):
            print("Error: Revision is not gotten from " + SrcFile + " correctly, please check it!!!")
            exit(1)
        else:
            print("Get the revision of " + self.extractDir + ": " + self.revision);

    def CheckOut(self):
        fullDstPath = os.path.join(TargetDir, self.extractDir)
        if not os.path.exists(fullDstPath):
            os.system("git clone " + self.httpsUrl + " " + fullDstPath);

        os.chdir(fullDstPath);
        os.system("git fetch");
        os.system("git checkout " + self.revision);

PACKAGES = [
    GitRepo("https://github.com/KhronosGroup/glslang.git",       "glslang",       "glslang"),
    GitRepo("https://github.com/KhronosGroup/SPIRV-Tools.git",   "spirv-tools",   "SPIRV-tools"),
    GitRepo("https://github.com/KhronosGroup/SPIRV-Headers.git", "spirv-headers", "SPIRV-tools/external/spirv-headers"),
    GitRepo("https://github.com/KhronosGroup/SPIRV-Cross.git",   "spirv-cross",   "SPIRV-cross"),
]

def GetOpt():
    global TargetDir;

    parser = OptionParser()

    parser.add_option("-t", "--targetdir", action="store",
                  type="string",
                  dest="targetdir",
                  help="the target directory source code downloaded to")

    (options, args) = parser.parse_args()

    if options.targetdir:
        print("The source code is downloaded to %s" % (options.targetdir));
        TargetDir = options.targetdir;
    else:
        print("The target directory is not specified, using default: " + TargetDir);

def DownloadSourceCode():
    global TargetDir;

    os.chdir(TargetDir);

    # Touch the spvgen CMakeLists.txt to ensure that the new directories get used.
    os.utime('../CMakeLists.txt', None)

    for pkg in PACKAGES:
        pkg.GetRevision()
        pkg.CheckOut()

GetOpt();
DownloadSourceCode();
