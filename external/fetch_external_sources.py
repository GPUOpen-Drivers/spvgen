##
 #######################################################################################################################
 #
 #  Copyright (c) 2018 Advanced Micro Devices, Inc. All Rights Reserved.
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

#!/usr/bin/python

# This script is used to download the glslang, SPIRV-Tools, and SPIRV-Headers from github.

import sys
import os
import string

from optparse import OptionParser

TargetDir = os.getcwd() + "/"; # target directory the source code downloaded to.

class GitRepo:
    def __init__(self, httpsUrl, moduleName, defaultRevision, extractDir):
        self.httpsUrl   = httpsUrl
        self.moduleName = moduleName
        self.revision   = defaultRevision
        self.extractDir = extractDir

    def GetRevision(self):
        SrcFile = TargetDir + "../CHANGES";
        if not os.path.exists(SrcFile):
            print SrcFile + " does not exist, default revision is " + self.revision;
            return;

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
            print "Warning: Revision is not gotten from " + SrcFile + " correctly, please check it!!!"
        else:
            print "Get the revision of " + self.extractDir + ": " + self.revision;

    def CheckOut(self):
        fullDstPath = os.path.join(TargetDir, self.extractDir)
        if not os.path.exists(fullDstPath):
            os.system("git clone " + self.httpsUrl + " " + fullDstPath);
        else:
            print "Warning: " + fullDstPath + " exist already, will not download from github again"
            return;

        os.chdir(fullDstPath);
        os.system("git pull");
        os.system("git checkout " + self.revision);

PACKAGES = [
    GitRepo("https://github.com/KhronosGroup/SPIRV-Tools.git", "spirv-tools", "72524db", "SPIRV-tools"),
    GitRepo("https://github.com/KhronosGroup/glslang.git", "glslang", "46e0731", "glslang"),
    GitRepo("https://github.com/KhronosGroup/SPIRV-Headers.git", "spirv-headers", "3a4dbdd", "SPIRV-tools/external/SPIRV-Headers"),
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
        print("The target directroy is not specified, using default: " + TargetDir);

def DownloadSourceCode():
    global TargetDir;

    os.chdir(TargetDir);

    for pkg in PACKAGES:
        pkg.GetRevision()
        pkg.CheckOut()

GetOpt();
DownloadSourceCode();
