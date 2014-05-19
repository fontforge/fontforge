#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
running even from the fontforge menu
"""

import sys
import os
import subprocess

#putting the folder of this file on the list of import paths
sys.path.reverse();
sys.path.append(os.path.dirname(sys.modules[__name__].__file__))
sys.path.reverse();

evenpath = os.path.dirname(sys.modules[__name__].__file__) + "/Even"

def runEvenShell(data = None, glyphOrFont = None):
    """Run even"""
    subprocess.Popen([evenpath, ''])
    #subprocess.call([evenpath, ''])

if fontforge.hasUserInterface():
    fontforge.registerMenuItem(runEvenShell, None, None, ("Font","Glyph"),
                                None, "Even Design Tool");

