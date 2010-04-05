#!/usr/local/bin/fontforge
#Needs: fonts/StrokeTests.sfd

import fontforge;
import psMat;

font=fontforge.open("fonts/StrokeTests.sfd");
font.selection.all();
triangle=fontforge.unitShape(3).transform(psMat.scale(25))
font.stroke("polygonal",triangle);
font.close();

font=fontforge.open("fonts/StrokeTests.sfd");
font.selection.all();
square=fontforge.unitShape(4).transform(psMat.scale(25))
font.stroke("polygonal",square);
font.close();

font=fontforge.open("fonts/StrokeTests.sfd");
font.selection.all();
pent=fontforge.unitShape(5).transform(psMat.scale(25))
font.stroke("polygonal",pent);
font.close();
