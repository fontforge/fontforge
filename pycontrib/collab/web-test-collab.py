#!/usr/bin/env fontforge
#
# Join the session and output a TTF of the font as it is updated
#   until a key is pressed.
#
import fontforge
import select
import json
import os
import signal
import subprocess

thisScriptDirName = os.path.dirname(os.path.realpath(__file__))


def openURLInBrowser( earl ):
    uname_os = os.uname()[0];
    openURLCommand = "xdg-open"
    if uname_os == "Darwin": 
        openURLCommand = "open"
    #
    # start the browser on the URL
    #
    if uname_os == "Windows": 
        subprocess.Popen([ "cmd", "/c", "start", earl ] )
    else:
        subprocess.Popen([ openURLCommand, earl ] )



myipaddr = "127.0.0.1"

# These are the values for Fedora 18:
webServerRootDir = "/tmp/fontforge-collab/"
webOutputDir = "ffc/"

# These are the values for Ubuntu 13.04:
# webServerRootDir = "/var/www/"
# webOutputDir = "html/ffc/"

webServerOutputDir = webServerRootDir + webOutputDir
if not os.path.exists(webServerOutputDir):
    os.makedirs(webServerOutputDir) 

def keyPressed():
    return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])

def OnCollabUpdate(f):
    seq = f.CollabLastSeq()
    basename = "font-" + str(seq)
    fontFileName = basename + ".ttf"
    fontJsonFileName = basename + ".json"
    fontFileOnDisk = webServerOutputDir + fontFileName
    fontJsonOnDisk = webServerOutputDir + fontJsonFileName
    fontFileURL = "http://" + myipaddr + "/" + webOutputDir + fontFileName
    
    fontforge.logWarning("Got an update!")
    fontforge.logWarning("   something about the font... name: " + str(f.fullname))
    fontforge.logWarning(" last seq: " + str(f.CollabLastSeq()))
    fontforge.logWarning("      glyph:" + str(f.CollabLastChangedName()))
    fontforge.logWarning("      code point:" + str(f.CollabLastChangedCodePoint()))
    f.generate(fontFileOnDisk)
    js = json.dumps({
            "seq": str(f.CollabLastSeq()), 
            "glyph": str(f.CollabLastChangedName()), 
            "codepoint": str(f.CollabLastChangedCodePoint()),
            "earl": str(fontFileURL),
            "end": "null" # this is simply so we dont have to manage keeping the last item with no terminating ,
            }, 
            sort_keys=True, indent=4, separators=(',', ': '))
    print js
    fi = open(fontJsonOnDisk, 'w')
    fi.write(js)

    
newfont=fontforge.font()       
f = newfont.CollabSessionJoin()
fontforge.logWarning( "Joined session, font name: " + f.fullname )

openURLInBrowser( "http://localhost:8000" )

fontforge.logWarning("web-test-collab: starting up...");
f.CollabSessionSetUpdatedCallback( OnCollabUpdate )
while True:
    f.CollabSessionRunMainLoop()
    # if keyPressed(): 
    #     break;

finalOutput = "/tmp/out-final.ttf"
f.generate(finalOutput)
fontforge.logWarning( "Left collab session, final file is at " + finalOutput )
