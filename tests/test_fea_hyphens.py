#Needs: fonts/Ambrosia.sfd

#Test reading feature file with hyphens inside glyph names
import sys, tempfile
import fontforge

fea_contents = """
lookup CALookup {
  lookupflag 0;
    sub [D E F ] [A B C ]' [G H I ]  by [aaa-l bbb-l ccc-l ];
} CALookup;

feature calt {

 script DFLT;
     language dflt ;
      lookup CALookup;
} calt;
"""

with tempfile.NamedTemporaryFile("w+t", suffix=".fea", delete=False) as f:
    f.write(fea_contents)
    f.close()

ambrosia = sys.argv[1]
font = fontforge.open(ambrosia)

n_lookups_before = len(font.gsub_lookups)

font["a"].glyphname = "aaa-l"
font["b"].glyphname = "bbb-l"
font["c"].glyphname = "ccc-l"

font.mergeFeature(f.name)
assert len(font.gsub_lookups) - n_lookups_before == 2
