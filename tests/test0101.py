#Needs: fonts/LucidaGrande.ttc

#Test the fontforge module (but not its types)
import sys, fontforge
font = fontforge.open(sys.argv[1])
