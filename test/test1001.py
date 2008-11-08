#!/usr/local/bin/fontforge
#Needs: fonts/Caliban.sfd fonts/Zapfino-4.0d3.dfont fonts/Zapfino-4.1d6.dfont

import fontforge;
caliban=fontforge.open("fonts/Caliban.sfd");
print "...Opened Caliban"
caliban.selection.all()
caliban.autoHint();
print "...AutoHinted";
caliban.save("results/Caliban.sfd");
print "...Saved As";
caliban.save("results/Caliban.sfdir");
print "...Saved As dir";
caliban.generate("results/Caliban.ps",bitmap_type="bdf");
print "...Generated type0"
caliban.generate("results/Caliban.ttf",bitmap_type="ms",flags=("pfm",));
print "...Generated ttf w/ ms bitmaps"
caliban.generate("results/Caliban.ttf",bitmap_type="apple");
print "...Generated ttf w/ apple bitmaps"
caliban.generate("results/Caliban.otf",bitmap_type="ms");
print "...Generated otf w/ ms bitmaps"
caliban.generate("results/Caliban.otf",bitmap_type="ttf",flags=("apple","opentype"));
print "...Generated otf w/ both apple and ot modes set (& bitmaps)"
caliban.generate("results/Caliban.dfont",bitmap_type="sbit");
print "...Generated sbit"
caliban.generate("results/Caliban.",bitmap_type="otb");
print "...Generated X11 opentype bitmap"
caliban.generate("results/Caliban.dfont",bitmap_type="dfont");
print "...Generated dfont w/ apple bitmaps"
caliban.layers["Fore"].is_quadratic = 1
caliban.setTableData("cvt ","");
caliban.selection.all()
fontforge.setPrefs("DetectDiagonalStems",1);
caliban.autoHint();
caliban.autoInstr();
print"...AutoInstructed";
caliban.generate("results/Caliban.ttf",bitmap_type="apple");
print "...Generated ttf w/ apple bitmaps (again) and instructions"
caliban.close();

caliban = fontforge.open("results/Caliban.sfd");
print "...Read sfd"
caliban.close();
caliban = fontforge.open("results/Caliban.sfdir");
print "...Read sfdir"
caliban.close();
caliban = fontforge.open("results/Caliban.ps");
print "...Read type0 (if PfaEdit didn't understand /CalibanBase that's ok)"
caliban.close();
caliban = fontforge.open("results/Caliban.ttf");
print "...Read ttf"
caliban.close();
caliban = fontforge.open("results/Caliban.otf");
print "...Read otf"
caliban.close();
caliban = fontforge.open("results/Caliban.dfont");
print "...Read dfont"
caliban.close();
caliban = fontforge.open("results/Caliban.otb");
print "...Read otb"
caliban.close();
caliban = fontforge.open("results/Caliban-10.bdf");
caliban.generate("results/Caliban.dfont",bitmap_type="sbit");
print "...Read bdf & Generated sbit"
caliban.close();

newfont = fontforge.font()
newfont.save("results/foo.sfd");
print "...Saved new font"
newfont.close();

zapfino = fontforge.open("fonts/Zapfino-4.0d3.dfont");
zapfino.generate("results/Zapfino.ttf",flags=("apple",));
zapfino.close()
#We used to get PointCount errors in the above.
zapfino = fontforge.open("results/Zapfino.ttf")
zapfino.close()
print "...Handled Zapfino with control points at points"

print "This font has odd 'morx' tables, we just don't want to crash"
zapfino = fontforge.open("fonts/Zapfino-4.1d6.dfont");
zapfino.generate("results/Zapfino.ttf",flags=("apple",));
zapfino.close()
