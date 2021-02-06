# scriptFromUnicode tests…

import sys, fontforge

assert fontforge.scriptFromUnicode(ord('ᜈ')) == "tglg"
assert fontforge.scriptFromUnicode(ord('Q')) == "latn"
assert fontforge.scriptFromUnicode(ord('θ')) == "grek"
assert fontforge.scriptFromUnicode(ord('Щ')) == "cyrl"
assert fontforge.scriptFromUnicode(ord('ツ')) == "kana"
assert fontforge.scriptFromUnicode(ord('ﬡ')) == "hebr"

try:
    fontforge.scriptFromUnicode("derp")
    assert False
except TypeError:
    pass
