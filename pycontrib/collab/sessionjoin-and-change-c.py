#!/usr/bin/env fontforge
#
# Join a collab session, move a few points around in the 'C' glyph and then exit.
# NOTE: The collab server will remain running after we quit
#
import fontforge

base=fontforge.open("../test.sfd")       
f = base.CollabSessionJoin()
fontforge.logWarning( "joined session" )
fontforge.logWarning( "font name: " + f.fullname )

g = f['C']
l = g.layers[g.activeLayer]
c = l[0]

# Make the two points at the opening of the C closer together.
p = c[3]
p.y = 500
c[3] = p

p = c[6]
p.y = 300
c[6] = p

l[0] = c
g.layers[g.activeLayer] = l

fontforge.logWarning( "saving..." )
f.save("/tmp/out.sfd")
f.CollabSessionRunMainLoop()

fontforge.logWarning( "script is done. See /tmp/out.sfd for what we think the font looks like." )
