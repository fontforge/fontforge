"""Preserve FONT_VERSION when opening a standalone BDF or PCF font."""

from pathlib import Path
import sys
import tempfile

import fontforge


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
