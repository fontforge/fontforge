#Needs: fonts/Ambrosia.sfd

#Test the fontforge module (but not its types)
import sys, fontforge

namelist = sys.argv[1]
fontforge.loadNamelist(namelist)
