#!/usr/local/bin/fontforge
#Needs: fonts/Ambrosia.sfd

#Test the fontforge module (but not its types)
import fontforge;

foo = fontforge.getPrefs("DetectDiagonalStems")
fontforge.setPrefs("DetectDiagonalStems",~foo)
fontforge.loadPrefs();
# fontforge.savePrefs() 
fontforge.defaultOtherSubrs();
# fontforge.readOtherSubrsFile(); 

foo = fontforge.hasSpiro()

# fontforge.loadEncodingFile() 
# fontforge.loadNamelist() 
# fontforge.loadNamelistDir() 
# fontforge.loadPlugin() 
# fontforge.loadPluginDir() 
# fontforge.preloadCidmap() 

fontforge.printSetup("lpr")

if ( (fontforge.unicodeFromName("A")!=65) | (fontforge.unicodeFromName("uni030D")!=0x30D) ) :
  raise ValueError, "Wrong return from unicodeFromName"

foo = fontforge.version()

fonts = fontforge.fonts()
if ( len(fonts)!=0 ) :
  raise ValueError, "Wrong return from fontforge.fonts"

fontforge.activeFont()
fontforge.activeGlyph()
fontforge.activeLayer()
fontnames= fontforge.fontsInFile("fonts/Ambrosia.sfd")
if ( len(fontnames)!=1 ) | (fontnames[0]!='Ambrosia' ):
  raise ValueError, "Wrong return from fontforge.fontsInFile"
font = fontforge.open("fonts/Ambrosia.sfd")
morefonts = fontforge.fonts()
if ( len(morefonts)!=1 ) :
  raise ValueError, "Wrong return from fontforge.fonts"

instrs = fontforge.parseTTInstrs("SRP0\nMIRP[min,rnd,black]")
print fontforge.unParseTTInstrs(instrs)
