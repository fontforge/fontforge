"""Preserve FONT_VERSION when opening a standalone BDF or PCF font."""

from pathlib import Path
import struct
import sys
import tempfile

import fontforge


def head_revision(path):
    data = path.read_bytes()
    count = struct.unpack_from(">H", data, 4)[0]
    for index in range(count):
        tag, _, offset, _ = struct.unpack_from(">4sIII", data, 12 + 16 * index)
        if tag in (b"head", b"bhed"):
            return struct.unpack_from(">i", data, offset + 4)[0]
    raise AssertionError("Missing font revision table")


source, pcf = (Path(arg) for arg in sys.argv[1:])
# PCF generated with: bdftopcf -o font-version.pcf font-version.bdf
with tempfile.TemporaryDirectory() as directory:
    directory = Path(directory)
    for path in (source, pcf):
        font = fontforge.open(str(path))
        assert font.version == "2.345", (path, font.version)
        saved = directory / (path.suffix[1:] + ".sfd")
        font.save(str(saved))
        font.close()
        font = fontforge.open(str(saved))
        assert font.version == "2.345"
        font.close()

        font = fontforge.font()
        font.version = "9.999"
        font.importBitmaps(str(path))
        assert font.version == "9.999"
        font.close()

    text = source.read_text(encoding="ascii")
    path = directory / "crlf.bdf"
    path.write_bytes(text.replace("\n", "\r\n").encode("ascii"))
    font = fontforge.open(str(path))
    assert font.version == "2.345"
    font.close()

    for label, value, expected in (
        ("atom", b"release", "release"),
        ("latin1", b'"rev \xe9"', "rev \u00e9"),
        ("date", b'"20240101"', "20240101"),
    ):
        path = directory / (label + ".bdf")
        path.write_bytes(text.encode("ascii").replace(b'"2.345"', value))
        font = fontforge.open(str(path))
        assert font.version == expected, (label, font.version)
        font.close()

    # PCF string tables can contain line breaks, unlike BDF properties.
    pcf_data = pcf.read_bytes()
    assert pcf_data.count(b"2.345\0") == 1
    for label, value in (("cr", b"2\r345\0"), ("lf", b"2\n345\0")):
        path = directory / (label + ".pcf")
        path.write_bytes(pcf_data.replace(b"2.345\0", value))
        font = fontforge.open(str(path))
        assert font.version == "001.000", (label, font.version)
        output = directory / (label + ".pfa")
        font.generate(str(output))
        font.close()
        data = output.read_bytes().split(b"currentfile eexec", 1)[0]
        assert b"%Version: 001.000\n" in data
        assert b"\n345" not in data and b"\r345" not in data

    for label, modified in (
        ("missing", text.replace("STARTPROPERTIES 3", "STARTPROPERTIES 2")
            .replace('FONT_VERSION "2.345"\n', "")),
        ("empty", text.replace('FONT_VERSION "2.345"', 'FONT_VERSION ""')),
    ):
        path = directory / (label + ".bdf")
        path.write_text(modified, encoding="ascii")
        font = fontforge.open(str(path))
        assert font.version == "001.000", (label, font.version)
        font.close()

    # Imported strings also reach Type 1 output; stream and quote them there.
    version = "2.345(" + "a" * 280 + ")"
    path = directory / "quoted-version.bdf"
    path.write_text(text.replace('FONT_VERSION "2.345"',
        'FONT_VERSION "' + version + '"'), encoding="ascii")
    font = fontforge.open(str(path))
    assert font.version == version
    output = directory / "quoted-version.pfa"
    font.generate(str(output))
    font.close()
    data = output.read_text(encoding="ascii")
    escaped = version.replace("(", "\\050").replace(")", "\\051")
    assert "/version (" + escaped + ") readonly def" in data

    # The version label is preserved even when its number does not fit 16.16.
    for label, value, revision in (
        ("plain", "2.345", 0x00025851),
        ("date", "20240101", 0x00010000),
        ("overflow", "1e400", 0x00010000),
        ("high", "32767.5", 0x7fff8000),
        ("highest", "32767.99999999", 0x7fffffff),
        ("limit", "32768", 0x00010000),
        ("hex-limit", "0x1.0p+15", 0x00010000),
        ("negative", "-1.5", 0x00018000),
        ("negative-limit", "-32768", 0x00010000),
        ("negative-overflow", "-1e400", 0x00010000),
        ("utf8", "\u00e9 2.5", 0x00028000),
    ):
        path = directory / ("revision-" + label + ".bdf")
        path.write_bytes(text.encode("ascii").replace(b'"2.345"',
            ('"' + value + '"').encode("utf-8")))
        for suffix, bitmap_type in ((".otb", "otb"), (".ttf", "ttf")):
            font = fontforge.open(str(path))
            assert font.version == value, (label, font.version)
            pinned = label in ("date", "overflow", "limit", "hex-limit",
                "negative-limit", "negative-overflow")
            assert font.sfntRevision == (1.0 if pinned else None), label
            saved = directory / ("revision-" + label + ".sfd")
            font.save(str(saved))
            font.close()
            font = fontforge.open(str(saved))
            assert font.version == value
            assert font.sfntRevision == (1.0 if pinned else None), label
            output = directory / ("revision-" + label + suffix)
            font.generate(str(output), bitmap_type=bitmap_type)
            assert font.version == value
            font.close()
            assert head_revision(output) == revision, (label, suffix, head_revision(output))

    # The existing explicit revision can be cleared after editing the label.
    font = fontforge.open(str(directory / "revision-date.bdf"))
    assert font.sfntRevision == 1.0
    font.version = "3.5"
    font.sfntRevision = None
    output = directory / "edited-revision.otb"
    font.generate(str(output), bitmap_type="otb")
    font.close()
    assert head_revision(output) == 0x00038000

    # WOFF retains its existing numeric-label conversion, including the
    # 16-bit header representation; importing a label adds no WOFF override.
    for label in ("date", "highest", "hex-limit", "negative"):
        imported = fontforge.open(str(directory / ("revision-" + label + ".bdf")))
        manual = fontforge.open(str(source))
        manual.version = imported.version
        manual.sfntRevision = imported.sfntRevision
        headers = []
        for index, font in enumerate((imported, manual)):
            assert font.woffMajor is None
            font.onlybitmaps = False
            output = directory / ("woff-" + label + "-" + str(index) + ".woff")
            font.generate(str(output), bitmap_type="ttf")
            font.close()
            data = output.read_bytes()
            assert data[:4] == b"wOFF"
            headers.append(struct.unpack_from(">HH", data, 20))
        assert headers[0] == headers[1], (label, headers)

    font = fontforge.open(str(directory / "revision-utf8.bdf"))
    # Bitmap-only mode selects a raw SFNT even with a .woff suffix.
    font.onlybitmaps = False
    output = directory / "revision-utf8.woff"
    font.generate(str(output), bitmap_type="ttf")
    font.close()
    data = output.read_bytes()
    assert data[:4] == b"wOFF"
    assert struct.unpack_from(">HH", data, 20) == (2, 5)
