#!/usr/bin/env fontforge
#
# Join the session and save the current font as /tmp/out.sfd
#
import fontforge

base=fontforge.open("../test.sfd")       
f = base.CollabSessionJoin()
fontforge.logWarning( "joined session" )

f.save("/tmp/out.sfd")

fontforge.logWarning( "script is done. See /tmp/out.sfd " )
