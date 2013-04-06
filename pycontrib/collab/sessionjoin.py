#!/usr/bin/env fontforge
#
# Join the session and output a TTF of the font as it is updated
#   until a key is pressed.
#
import fontforge
import select

def keyPressed():
    return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])

def OnCollabUpdate(f):
    fontforge.logWarning("Got an update!")
    fontforge.logWarning("   something about the font... name: " + f.fullname )
    f.generate("/tmp/out.ttf")

f=fontforge.open("test.sfd")       
fontforge.logWarning( "font name: " + f.fullname )
f2 = f.CollabSessionJoin()
fontforge.logWarning( "joined session" )
fontforge.logWarning( "f2 name: " + f2.fullname )

f2.CollabSessionSetUpdatedCallback( OnCollabUpdate )
while True:
    f2.CollabSessionRunMainLoop()
    if keyPressed(): 
        break;

f2.generate("/tmp/out-final.ttf")
fontforge.logWarning( "script is done." )
