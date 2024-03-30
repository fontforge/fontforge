#Needs: fonts/Ambrosia.sfd

import sys, fontforge
import tempfile
import os

font = fontforge.open(sys.argv[1])

#
# Test multiple lookups in a contextual substitution rule (Issue #5374)
#
font.addLookup("Single 1", "gsub_single", (), ())
font.addLookupSubtable("Single 1", "Single 1")
font["A"].addPosSub("Single 1", "B")

font.addLookup("Single 2", "gsub_single", (), (), "Single 1")
font.addLookupSubtable("Single 2", "Single 2")
font["C"].addPosSub("Single 2", "D")

font.addLookup("Contextual", "gsub_context", (), (("calt",(("dflt",("dflt")),)),), "Single 2")
font.addContextualSubtable("Contextual", "Contextual_sub", "glyph",
        "A @<Single 1> C @<Single 2>")

with tempfile.TemporaryDirectory() as tmpdirname:
        fea_full_name = os.path.join(tmpdirname, "Ambrosia.sfd")
        font.generateFeatureFile(fea_full_name)

        with open(fea_full_name) as fea_file:
                rules = [line for line in fea_file
                         if r"sub \A'lookup Single1  \C'lookup Single2" in line]

                # Expect to find the rule inside the feature file
                assert rules

#
# Test pair positioning in a contextual chaining lookup (Issue #5383)
#
kern_value = 532
font.addLookup("Pair 1", "gpos_pair", (), ())
font.addLookupSubtable("Pair 1", "Pair 1")
font["E"].addPosSub("Pair 1", "F", -kern_value)

font.addLookup("KernContextual", "gpos_contextchain", (), (("kern",(("dflt",("dflt")),)),), "Pair 1")
font.addContextualSubtable("KernContextual", "KernContextual_sub", "glyph",
        "E | E @<Pair 1> F |")

with tempfile.TemporaryDirectory() as tmpdirname:
        fea_full_name = os.path.join(tmpdirname, "Ambrosia.sfd")
        font.generateFeatureFile(fea_full_name)

        with open(fea_full_name) as fea_file:
                # Check that the kerning value is present
                rules = [line for line in fea_file if str(kern_value) in line]

                # Expect to find the kerning rule inside the feature file
                assert rules
