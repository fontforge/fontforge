#!/usr/bin/env fontforge

import fontforge

f=fontforge.open("../test.sfd")       
fontforge.logWarning( "font name: " + f.fullname )
