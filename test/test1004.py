#!/usr/local/bin/fontforge
#Needs: fonts/DirectionTest.sfd

import fontforge;

font=fontforge.open("fonts/DirectionTest.sfd");
font.selection.all();
font.selection.select(("less",),"K");

dir=True;
for g in font.selection.byGlyphs :
  c = g.foreground[0];
  if ( dir != c.isClockwise() ) :
    raise ValueError, "Wrong direction";
  dir = not dir;

c = font["K"].foreground[0];
if ( c.isClockwise()!=-1 ) :
  raise ValueError, "Found a direction";

