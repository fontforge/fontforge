import sys
import fontforge

font = fontforge.open(sys.argv[1])

font.addLookup("'calt' Reverse chaining test", "gsub_reversecchain", (),
               (("calt", (("DFLT", ("DFLT",)), ("latn", ("dflt",)))),))

font.addContextualSubtable("'calt' Reverse chaining test", "reverse subtable1",
                           "reversecoverage",
                           "| [A B C D] => [a b c d] | [l m n o]")
# This test succeeds if things don't crash.
font.save("Caliban-reversechain.sfd")
