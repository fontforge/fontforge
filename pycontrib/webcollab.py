#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
running web collab server hooks
"""

import sys
import os
import subprocess

#putting the folder of this file on the list of import paths
sys.path.reverse();
sys.path.append(os.path.dirname(sys.modules[__name__].__file__))
sys.path.reverse();

uname_os = os.uname()[0];
nodejsPath = "node"
if uname_os == "Darwin": 
    nodejsPath = "/Applications/FontForge.app/Contents/Resources/opt/local/bin/node"

thisScriptDirName = os.path.dirname(os.path.realpath(__file__))
collabpath = os.path.dirname(sys.modules[__name__].__file__) + "/collab/"
FONTFORGE = "fontforge"
StartCommandLine = FONTFORGE + " " + collabpath + "web-test-collab.py"
child = None
childNodejs = None

def ensureChildClosed():
    global child
    global childNodejs
    if child != None:
        child.kill()
    child = None
    if childNodejs != None:
        childNodejs.kill()
    childNodejs = None

def WebServerInCollabModeGoodbye():
    print('webcollab closing...')
    ensureChildClosed()

def startWebServerInCollabMode(data = None, glyphOrFont = None):
    """Start Web server in Collab mode"""
    global child
    global childNodejs
    ensureChildClosed()
    print("FONTFORGE:" + FONTFORGE)
    print("script path:" + collabpath + "web-test-collab.py")
    child = subprocess.Popen( [ FONTFORGE, "-forceuihidden", "-script", collabpath + "web-test-collab.py" ] )
    #
    # start the nodejs server
    oldpwd = os.getcwd()
    os.chdir( thisScriptDirName )
    os.chdir( "../nodejs/collabwebview/" )
    childNodejs = subprocess.Popen( [ nodejsPath, "server.js" ], cwd=os.getcwd() )
    os.chdir( oldpwd )


def stopWebServerInCollabMode(data = None, glyphOrFont = None):
    """Stop Web server in Collab mode"""
    ensureChildClosed()

if fontforge.hasUserInterface():
    print("fontforge.hasUserInterface!")
    fontforge.onAppClosing( WebServerInCollabModeGoodbye )
    fontforge.registerMenuItem(startWebServerInCollabMode, None, None, ("Font","Glyph"),
                                None, "Start Web server in Collab mode");
    fontforge.registerMenuItem(stopWebServerInCollabMode, None, None, ("Font","Glyph"),
                                None, "Stop Web server in Collab mode");

