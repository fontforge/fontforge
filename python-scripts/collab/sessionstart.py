#!/usr/bin/env fontforge

#
# Start a collab session, move a few points around in the font and then exit.
#
# NOTE: The collab server will remain running after we quit
#
import fontforge

f=fontforge.open("../test.sfd")       
f.CollabSessionStart()
fontforge.logWarning( "started session" )

##
## Create a new glyph 'C' in the test font and create a new contour in that glyph
## ... those changes are sent has updates to the original SFD
##
g = f.createChar(-1,'C')
l = g.layers[g.activeLayer]
c = fontforge.contour()
c.moveTo(100,100)   
c.lineTo(100,700)   
c.lineTo(800,700)   
c.lineTo(800,600)   
c.lineTo(200,600)   
c.lineTo(200,200)   
c.lineTo(800,200)   
c.lineTo(800,100)   
c.lineTo(100,100)   
l += c
g.layers[g.activeLayer] = l

#
# Grab a copy of what *we* think the font looks like in case
# it is not what the collab server things it looks like.
# 
fontforge.logWarning( "saving..." )
f.save("/tmp/out-srv.sfd")
f.CollabSessionRunMainLoop()
f.CollabSessionRunMainLoop()

fontforge.logWarning( "script is done. Output at /tmp/out-srv.sfd" )
