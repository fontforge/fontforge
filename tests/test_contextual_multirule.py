import os
import re
import tempfile

import fontforge


def assert_raises(exception_type, callback):
    try:
        callback()
    except exception_type:
        return
    raise AssertionError("expected %s" % exception_type.__name__)


def make_glyph(font, name, codepoint):
    glyph = font.createChar(codepoint, name)
    pen = glyph.glyphPen()
    pen.moveTo((0, 0))
    pen.lineTo((500, 0))
    pen.lineTo((500, 500))
    pen.lineTo((0, 500))
    pen.closePath()
    glyph.width = 600


font = fontforge.font()
font.encoding = "UnicodeFull"
font.fontname = "ContextClassMultiRule"
font.familyname = "Context Class MultiRule"
font.fullname = "Context Class MultiRule"

make_glyph(font, "baseA", 0xE000)
make_glyph(font, "baseB", 0xE001)
make_glyph(font, "markA", 0xE002)
make_glyph(font, "markB", 0xE003)
make_glyph(font, "target", 0xE004)

features = (("mark", (("DFLT", ("dflt",)),)),)

font.addLookup("raise_020", "gpos_single", (), ())
font.addLookupSubtable("raise_020", "raise_020-1")
font["target"].addPosSub("raise_020-1", 0, 20, 0, 0)

font.addLookup("raise_040", "gpos_single", (), (), "raise_020")
font.addLookupSubtable("raise_040", "raise_040-1")
font["baseA"].addPosSub("raise_040-1", 0, 40, 0, 0)
font["target"].addPosSub("raise_040-1", 0, 40, 0, 0)

font.addLookup("ctx", "gpos_contextchain", (), features, "raise_040")
font.addContextualSubtable(
    "ctx",
    "ctx-1",
    "class",
    (
        "1 2 | 1 @<raise_020> |",
        "1 3 | 1 @<raise_040> |",
    ),
    bclasses=(None, ("baseA", "baseB"), ("markA",), ("markB",)),
    mclasses=(None, ("target",)),
)

assert font.getLookupSubtables("ctx") == ("ctx-1",)

assert_raises(
    ValueError,
    lambda: font.addContextualSubtable(
        "ctx",
        "ctx-nul",
        "class",
        ("1 2 | 1 @<raise_020> |", "1\0 3 | 1 @<raise_040> |"),
        bclasses=(None, ("baseA", "baseB"), ("markA",), ("markB",)),
        mclasses=(None, ("target",)),
    ),
)

assert_raises(
    TypeError,
    lambda: font.addContextualSubtable(
        "ctx",
        "ctx-non-string",
        "class",
        ("1 2 | 1 @<raise_020> |", object()),
        bclasses=(None, ("baseA", "baseB"), ("markA",), ("markB",)),
        mclasses=(None, ("target",)),
    ),
)

assert font.getLookupSubtables("ctx") == ("ctx-1",)

assert_raises(
    TypeError,
    lambda: font.addContextualSubtable(
        "ctx",
        "ctx-bad-parse",
        "class",
        ("1 2 | 1 @<raise_020> |", "99 3 | 1 @<raise_040> |"),
        bclasses=(None, ("baseA", "baseB"), ("markA",), ("markB",)),
        mclasses=(None, ("target",)),
    ),
)

assert font.getLookupSubtables("ctx") == ("ctx-1",)

font.addLookup("glyph_ctx", "gpos_context", (), features, "ctx")
font.addContextualSubtable(
    "glyph_ctx",
    "glyph_ctx-1",
    "glyph",
    (
        "target @<raise_020>",
        "baseA @<raise_040>",
    ),
)

assert font.getLookupSubtables("glyph_ctx") == ("glyph_ctx-1",)

with tempfile.TemporaryDirectory() as tmpdirname:
    sfd_path = os.path.join(tmpdirname, "ContextClassMultiRule.sfd")
    font.save(sfd_path)

    with open(sfd_path) as sfd_file:
        sfd = sfd_file.read()

    assert re.search(
        r'^ChainPos2: class "ctx-1"\s+\d+\s+\d+\s+\d+\s+2$', sfd, re.MULTILINE
    )
    assert re.search(
        r'^ContextPos2: glyph "glyph_ctx-1"\s+0\s+0\s+0\s+2$', sfd, re.MULTILINE
    )
    assert re.search(r"^ String: 6 target$", sfd, re.MULTILINE)
    assert re.search(r"^ String: 5 baseA$", sfd, re.MULTILINE)
