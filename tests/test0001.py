#Needs: fonts/Ambrosia.sfd

#Test the fontforge module (but not its types)
import sys, fontforge

foo = fontforge.getPrefs("DetectDiagonalStems")
fontforge.setPrefs("DetectDiagonalStems",~foo)
fontforge.loadPrefs()
# fontforge.savePrefs()
fontforge.defaultOtherSubrs()
# fontforge.readOtherSubrsFile()

foo = fontforge.hasSpiro()

# fontforge.loadEncodingFile()
# fontforge.loadNamelist()
# fontforge.loadNamelistDir()
# fontforge.preloadCidmap()

fontforge.printSetup("lpr")

if (fontforge.unicodeFromName("A")!=65) or (fontforge.unicodeFromName("uni030D")!=0x30D):
  raise ValueError("Wrong return from unicodeFromName")

foo = fontforge.version()

ambrosia = sys.argv[1]

fonts = fontforge.fonts()
if ( len(fonts)!=0 ) :
  raise ValueError("Wrong return from fontforge.fonts")

fontforge.activeFont()
fontforge.activeGlyph()
fontforge.activeLayer()
fontnames= fontforge.fontsInFile(ambrosia)
if len(fontnames)!=1 or fontnames[0]!='Ambrosia':
  raise ValueError("Wrong return from fontforge.fontsInFile")
font = fontforge.open(ambrosia)
morefonts = fontforge.fonts()
if len(morefonts)!=1:
  raise ValueError("Wrong return from fontforge.fonts")

instrs = fontforge.parseTTInstrs("SRP0\nMIRP[min,rnd,black]")
print(fontforge.unParseTTInstrs(instrs))
