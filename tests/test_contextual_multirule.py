import os
import re
import tempfile

import fontforge


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

for subtable, rule, error_type, error_message in [
    (
        "ctx-non-string",
        ("1 2 | 1 @<raise_020> |", object()),
        TypeError,
        "rule 1 must be a string",
    ),
    (
        "ctx-bytes",
        b"1 2 | 1 @<raise_020> |",
        TypeError,
        "rule 0 must be a string",
    ),
    (
        "ctx-item-bytes",
        ("1 2 | 1 @<raise_020> |", b"1 3 | 1 @<raise_040> |"),
        TypeError,
        "rule 1 must be a string",
    ),
    (
        "ctx-bad-parse",
        ("1 2 | 1 @<raise_020> |", "99 3 | 1 @<raise_040> |"),
        TypeError,
        "Error in rule 1:",
    ),
]:
    try:
        font.addContextualSubtable(
            "ctx",
            subtable,
            "class",
            rule,
            bclasses=(None, ("baseA", "baseB"), ("markA",), ("markB",)),
            mclasses=(None, ("target",)),
        )
        raise AssertionError("no error raised, but expected")
    except error_type as error:
        assert error_message in str(error), "wrong error message for correct error type"
    except Exception:
        raise AssertionError("wrong error type")

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
    fea_path = os.path.join(tmpdirname, "ContextClassMultiRule.fea")
    font.save(sfd_path)
    font.close()

    reopened = fontforge.open(sfd_path)
    reopened.generateFeatureFile(fea_path)
    reopened.close()

    with open(fea_path) as fea_file:
        fea = fea_file.read()

    assert re.search(r"@cc2_match_1 = \[\\target \];", fea)
    assert re.search(r"@cc2_back_1 = \[\\baseA \\baseB \];", fea)
    assert re.search(r"@cc2_back_2 = \[\\markA \];", fea)
    assert re.search(r"@cc2_back_3 = \[\\markB \];", fea)
    assert re.search(r"pos @cc2_back_2 @cc2_back_1 @cc2_match_1'lookup raise_020", fea)
    assert re.search(r"pos @cc2_back_3 @cc2_back_1 @cc2_match_1'lookup raise_040", fea)
    assert re.search(r"pos\s+\\target'lookup raise_020", fea)
    assert re.search(r"pos\s+\\baseA'lookup raise_040", fea)
