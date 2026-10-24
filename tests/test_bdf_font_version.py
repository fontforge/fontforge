"""Preserve the FONT_VERSION property when opening BDF and PCF fonts."""

from pathlib import Path
import struct
import sys
import tempfile

import fontforge


def sfnt_version(path):
    data = path.read_bytes()
    table_count = struct.unpack_from(">H", data, 4)[0]
    for index in range(table_count):
        tag, _, offset, _ = struct.unpack_from(">4sIII", data, 12 + 16 * index)
        if tag == b"name":
            break
    else:
        raise AssertionError("No name table in generated font")
    _, count, strings = struct.unpack_from(">HHH", data, offset)
    for index in range(count):
        platform, _, language, name, length, start = struct.unpack_from(
            ">HHHHHH", data, offset + 6 + 12 * index)
        if (platform, language, name) == (3, 0x409, 5):
            start += offset + strings
            return data[start:start + length].decode("utf-16-be")
    raise AssertionError("No Windows version name in generated font")


def sfnt_revision(path):
    data = path.read_bytes()
    table_count = struct.unpack_from(">H", data, 4)[0]
    for index in range(table_count):
        tag, _, offset, _ = struct.unpack_from(">4sIII", data, 12 + 16 * index)
        if tag == b"head":
            return struct.unpack_from(">I", data, offset + 4)[0] / 65536
    raise AssertionError("No head table in generated font")


def woff_version(path):
    data = path.read_bytes()
    assert data[:4] == b"wOFF"
    return struct.unpack_from(">HH", data, 20)


source = Path(sys.argv[1])
pcf = Path(sys.argv[2])
integer_source = Path(sys.argv[3])
integer_pcf = Path(sys.argv[4])

# PCF fixtures generated with bdftopcf from xfonts-utils 1:7.7+6:
# bdftopcf -o font-version.pcf font-version.bdf
# bdftopcf -o font-version-integer.pcf font-version-integer.bdf
with tempfile.TemporaryDirectory() as directory:
    directory = Path(directory)
    for path in (source, pcf):
        font = fontforge.open(str(path))
        assert font.version == "2.345", (path, font.version)
        assert font.bitmapSizes == (8,)

        otb = directory / (path.suffix[1:] + ".otb")
        font.generate(str(otb), bitmap_type="otb")
        assert sfnt_version(otb) == "Version 2.345"
        assert abs(sfnt_revision(otb) - 2.345) < 1 / 65536

        woff = directory / (path.suffix[1:] + ".woff")
        font.generate(str(woff))
        assert woff_version(woff) == (2, 345)

        saved = directory / (path.suffix[1:] + ".sfd")
        font.save(str(saved))
        font.close()
        font = fontforge.open(str(saved))
        assert font.version == "2.345"
        font.close()
    for path in (integer_source, integer_pcf):
        font = fontforge.open(str(path))
        assert font.version == "2", (path, font.version)
        assert font.bitmapSizes == (8,)

        otb = directory / (path.stem + ".otb")
        font.generate(str(otb), bitmap_type="otb")
        assert sfnt_version(otb) == "Version 2"
        assert abs(sfnt_revision(otb) - 2.0) < 1 / 65536

        woff = directory / (path.stem + ".woff")
        font.generate(str(woff))
        assert woff_version(woff) == (2, 0)

        saved = directory / (path.stem + ".sfd")
        font.save(str(saved))
        font.close()
        font = fontforge.open(str(saved))
        assert font.version == "2"
        font.close()
    for path in (source, pcf, integer_source, integer_pcf):
        font = fontforge.font()
        font.version = "9.999"
        font.importBitmaps(str(path))
        assert font.version == "9.999"
        assert font.bitmapSizes == (8,)
        font.close()

    missing = directory / "missing-version.bdf"
    text = source.read_text(encoding="ascii")
    text = text.replace("STARTPROPERTIES 3", "STARTPROPERTIES 2")
    text = text.replace('FONT_VERSION "2.345"\n', "")
    missing.write_text(text, encoding="ascii")
    font = fontforge.open(str(missing))
    assert font.version == "001.000"
    font.close()

    for label, version, name, revision, woff in (
        ("long-numeric", "12.345", "Version 12.345", 12.345, (12, 345)),
        ("large-numeric", "20240115", "Version 20240115", 1.0, (1, 0)),
        ("leading-dot", ".5", "Version .5", 0.5, (0, 5)),
        ("plus-dot-plus-minor", "+.+5", "Version +.+5", 0.0, (0, 5)),
        ("leading-dot-plus-minor", ".+5", "Version .+5", 0.0, (0, 5)),
        ("leading-dot-negative-minor", ".-5", "Version .-5", 0.0, (0, 0)),
        ("negative-dot-plus-minor", "-.+5", "Version -.+5", 1.0, (1, 0)),
        ("negative-dot-only", "-.", "Version -.", 1.0, (1, 0)),
        ("plus-leading", "+1.5", "Version +1.5", 1.5, (1, 5)),
        ("hyphen-prefix", "Release-1.5", "Version Release-1.5", 1.5, (1, 5)),
        ("plus-prefix", "Release+1.5", "Version Release+1.5", 1.5, (1, 5)),
        ("multiple-dots", "1.2.3", "Version 1.2.3", 1.2, (1, 2)),
        ("trailing-text", "2.345abc", "Version 2.345abc", 2.345, (2, 345)),
        ("negative", "-2.345", "Version -2.345", 1.0, (1, 0)),
        ("internal-space", "2 .345", "Version 2 .345", 2.0, (2, 0)),
        ("dot-only", ".", "Version .", 0.0, (0, 0)),
        ("none", "None", None, 1.0, (1, 0)),
    ):
        variant = directory / (label + ".bdf")
        text = source.read_text(encoding="ascii")
        original = 'FONT_VERSION "2.345"\n'
        assert text.count(original) == 1
        text = text.replace(original, f'FONT_VERSION "{version}"\n')
        variant.write_text(text, encoding="ascii")
        font = fontforge.open(str(variant))
        assert font.version == version

        otb = directory / (label + ".otb")
        font.generate(str(otb), bitmap_type="otb")
        if name is not None:
            assert sfnt_version(otb) == name
        assert abs(sfnt_revision(otb) - revision) < 1 / 65536

        output_woff = directory / (label + ".woff")
        font.generate(str(output_woff))
        assert woff_version(output_woff) == woff
        font.close()

    for label, version_name in (
        ("oversized-name", "Version 70000.1"),
        ("negative-name", "Version -2.1"),
        ("leading-dot-name", "Version .5"),
        ("leading-plus-dot-name", "Version +.5"),
        ("plus-integer-name", "Version +1.5"),
        ("oversized-minor-name", "Version 2.70000"),
        ("multiple-dots-name", "Version 1.2.3"),
        ("trailing-name", "Version 2.1 beta"),
    ):
        font = fontforge.open(str(source))
        font.sfnt_names = (("English (US)", "Version", version_name),)
        otb = directory / (label + ".otb")
        font.generate(str(otb), bitmap_type="otb")
        assert sfnt_version(otb) == version_name
        expected_revision = {
            "multiple-dots-name": 1.2,
            "trailing-name": 2.1,
            "leading-dot-name": 0.5,
            "leading-plus-dot-name": 0.5,
            "plus-integer-name": 1.5,
            "oversized-minor-name": 2.7,
        }.get(label, 2.345)
        assert abs(sfnt_revision(otb) - expected_revision) < 1 / 65536
        woff = directory / (label + ".woff")
        font.generate(str(woff))
        expected_woff = {
            "multiple-dots-name": (1, 2),
            "trailing-name": (2, 1),
            "plus-integer-name": (1, 5),
            "oversized-minor-name": (2, 0),
        }.get(label, (2, 345))
        assert woff_version(woff) == expected_woff
        font.close()

    font = fontforge.open(str(source))
    font.sfnt_names = (("English (US)", "Version", "Version 1.0000152587"),)
    otb = directory / "rounded-name.otb"
    font.generate(str(otb), bitmap_type="otb")
    assert abs(sfnt_revision(otb) - (1 + 1 / 65536)) < 1 / 65536
    woff = directory / "rounded-name.woff"
    font.generate(str(woff))
    assert woff_version(woff) == (1, 0)
    font.close()

    negative_minor = directory / "negative-minor.bdf"
    text = source.read_text(encoding="ascii").replace(
        'FONT_VERSION "2.345"\n', 'FONT_VERSION "2.-5"\n')
    negative_minor.write_text(text, encoding="ascii")
    font = fontforge.open(str(negative_minor))
    assert font.version == "2.-5"
    otb = directory / "negative-minor.otb"
    font.generate(str(otb), bitmap_type="otb")
    assert abs(sfnt_revision(otb) - 2.0) < 1 / 65536
    woff = directory / "negative-minor.woff"
    font.generate(str(woff))
    assert woff_version(woff) == (2, 0)
    font.close()

    font = fontforge.open(str(source))
    font.sfnt_names = (("English (US)", "Version", "Version 2.-5"),)
    otb = directory / "negative-minor-name.otb"
    font.generate(str(otb), bitmap_type="otb")
    assert abs(sfnt_revision(otb) - 2.0) < 1 / 65536
    woff = directory / "negative-minor-name.woff"
    font.generate(str(woff))
    assert woff_version(woff) == (2, 0)
    font.close()

    integer_min_version = directory / "integer-min-version.bdf"
    text = source.read_text(encoding="ascii").replace(
        'FONT_VERSION "2.345"\n', "FONT_VERSION -2147483648\n")
    integer_min_version.write_text(text, encoding="ascii")
    font = fontforge.open(str(integer_min_version))
    assert font.version == "-2147483648"
    otb = directory / "integer-min-version.otb"
    font.generate(str(otb), bitmap_type="otb")
    assert abs(sfnt_revision(otb) - 1.0) < 1 / 65536
    woff = directory / "integer-min-version.woff"
    font.generate(str(woff))
    assert woff_version(woff) == (1, 0)
    font.close()

    for label, major, minor, expected in (
        ("bad-explicit-major", 70000, 345, (1, 345)),
        ("bad-explicit-minor", 2, 70000, (2, 0)),
        ("negative-explicit-major", -2, 345, (1, 345)),
        ("negative-explicit-minor", 2, -345, (2, 0)),
    ):
        font = fontforge.open(str(source))
        font.woffMajor = major
        font.woffMinor = minor
        output_woff = directory / (label + ".woff")
        font.generate(str(output_woff))
        assert woff_version(output_woff) == expected
        font.close()

    original = b'FONT_VERSION "2.345"\n'
    for label, value, expected in (
        ("padded", b'  2.345  ', "2.345"),
        ("latin1", b'\xe92.345', "\u00e92.345"),
        ("empty", b'', "001.000"),
    ):
        replacement = b'FONT_VERSION "' + value + b'"\n'
        data = source.read_bytes()
        assert data.count(original) == 1
        variant = directory / (label + ".bdf")
        variant.write_bytes(data.replace(original, replacement))
        font = fontforge.open(str(variant))
        assert font.version == expected, (label, font.version)
        if label != "empty":
            otb = directory / (label + ".otb")
            font.generate(str(otb), bitmap_type="otb")
            assert sfnt_version(otb) == "Version " + expected
            assert abs(sfnt_revision(otb) - 2.345) < 1 / 65536
            woff = directory / (label + ".woff")
            font.generate(str(woff))
            assert woff_version(woff) == (2, 345)
        font.close()

    control_value = directory / "control-value.bdf"
    data = source.read_bytes()
    assert data.count(original) == 1
    control_value.write_bytes(data.replace(original, b'FONT_VERSION "2.\t345"\n'))
    font = fontforge.open(str(control_value))
    assert font.version == "001.000"
    font.close()

    overlong_control = directory / "overlong-control-value.bdf"
    data = source.read_bytes()
    assert data.count(original) == 1
    overlong_control.write_bytes(data.replace(original, b'FONT_VERSION "2.\xc1\xbf345"\n'))
    font = fontforge.open(str(overlong_control))
    assert font.version == "001.000"
    font.close()

    oversized_minor = directory / "oversized-minor.bdf"
    data = source.read_bytes()
    assert data.count(original) == 1
    oversized_minor.write_bytes(data.replace(original, b'FONT_VERSION "2.70000"\n'))
    font = fontforge.open(str(oversized_minor))
    assert font.version == "2.70000"
    otb = directory / "oversized-minor.otb"
    font.generate(str(otb), bitmap_type="otb")
    assert abs(sfnt_revision(otb) - 2.7) < 1 / 65536
    woff = directory / "oversized-minor.woff"
    font.generate(str(woff))
    assert woff_version(woff) == (2, 0)
    font.close()

    negative_zero = directory / "negative-zero.bdf"
    data = source.read_bytes()
    assert data.count(original) == 1
    negative_zero.write_bytes(data.replace(original, b'FONT_VERSION "-0.0"\n'))
    font = fontforge.open(str(negative_zero))
    assert font.version == "-0.0"
    otb = directory / "negative-zero.otb"
    font.generate(str(otb), bitmap_type="otb")
    assert abs(sfnt_revision(otb) - 1.0) < 1 / 65536
    woff = directory / "negative-zero.woff"
    font.generate(str(woff))
    assert woff_version(woff) == (1, 0)
    font.close()

    unsafe_pfa_version = "2.345) /injected true def (" + "A" * 250
    unsafe_pfa_source = directory / "unsafe-pfa-version.bdf"
    data = source.read_bytes()
    assert data.count(original) == 1
    unsafe_pfa_source.write_bytes(
        data.replace(original, ('FONT_VERSION "' + unsafe_pfa_version + '"\n').encode("ascii"))
    )
    font = fontforge.open(str(unsafe_pfa_source))
    assert font.version == unsafe_pfa_version, (font.version, len(font.version))
    pfa = directory / "unsafe-pfa-version.pfa"
    font.generate(str(pfa))
    font.close()
    pfa_data = pfa.read_bytes()
    version_comment = b"\n%Version: " + unsafe_pfa_version.encode("ascii") + b"\n"
    assert version_comment in pfa_data
    safe_pfa_version = (
        b"/version (2.345\\051 /injected true def \\050" + b"A" * 250 +
        b") readonly def"
    )
    assert safe_pfa_version in pfa_data
