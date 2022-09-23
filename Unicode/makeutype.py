# Copyright (C) 2021 by Jeremy Tan

# This file was originally based on makeunicodedata.py from CPython:
# https://github.com/python/cpython/blob/a460ab3134cd5cf3932c2125aec012851268f0cc/Tools/unicode/makeunicodedata.py
# It is licensed under the Python Software Foundation License Version 2.
# For simplicity, everything in this file, excluding the generated source,
# is similarly licensed as such.
# The full license may be found here: https://docs.python.org/3/license.html
# While serving as the basis, it has been heavily modified for FontForge.

# This file generates the Unicode support source files for FontForge.
# It requires Python 3.7 or higher to run.

from __future__ import annotations
from collections import Counter
from data.makeutypedata import VISUAL_ALTS, Pose, get_pose
from functools import partial
from itertools import chain
from typing import Iterator, List, Optional, Set, Tuple

import dataclasses
import enum
import os
import re
import sys

BASE = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.basename(__file__)

UNIDATA_VERSION = "15.0.0"
UNICODE_DATA = "UnicodeData%s.txt"
DERIVED_CORE_PROPERTIES = "DerivedCoreProperties%s.txt"
PROP_LIST = "PropList%s.txt"
BIDI_MIRRORING = "BidiMirroring%s.txt"
BLOCKS = "Blocks%s.txt"
NAMES_LIST = "NamesList%s.txt"
UNICODE_MAX = 0x110000

DATA_DIR = "data"


class UcdTypeFlags(enum.IntFlag):
    FF_UNICODE_ISUNICODEPOINTASSIGNED = 0x1  # all
    FF_UNICODE_ISALPHA = 0x2  # Lu | Ll | Lt | Lm | Lo (letters)
    FF_UNICODE_ISIDEOGRAPHIC = 0x4  # Ideographic (PropList)
    FF_UNICODE_ISLEFTTORIGHT = 0x10  # Bidi L | LRE | LRO
    FF_UNICODE_ISRIGHTTOLEFT = 0x20  # Bidi R | AL | RLE | RLO
    FF_UNICODE_ISLOWER = 0x40  # Lo | Other_Lowercase
    FF_UNICODE_ISUPPER = 0x80  # Lu | Other_Uppercase
    FF_UNICODE_ISDIGIT = 0x100  # Ld
    FF_UNICODE_ISLIGVULGFRAC = 0x200  # dodgy
    FF_UNICODE_ISCOMBINING = 0x400  # Mn | Mc | Me (Marks)
    FF_UNICODE_ISZEROWIDTH = 0x800  # Default_Ignorable_Code_Point
    FF_UNICODE_ISEURONUMERIC = 0x1000  # EN
    FF_UNICODE_ISEURONUMTERM = 0x2000  # ET
    FF_UNICODE_ISARABNUMERIC = 0x4000  # AN
    FF_UNICODE_ISDECOMPOSITIONNORMATIVE = 0x8000  # Has an NFKD decomp
    FF_UNICODE_ISDECOMPCIRCLE = 0x10000  # <circle> compatibility decomp
    FF_UNICODE_ISARABINITIAL = 0x20000  # <initial> compatibility decomp
    FF_UNICODE_ISARABMEDIAL = 0x40000  # <medial> compatibility decomp
    FF_UNICODE_ISARABFINAL = 0x80000  # <final> compatibility decomp
    FF_UNICODE_ISARABISOLATED = 0x100000  # <isolated> compatibility decomp


@dataclasses.dataclass
class UcdRecord:
    # 15 fields from UnicodeData.txt .  See:
    #   https://www.unicode.org/reports/tr44/#UnicodeData.txt
    codepoint: str
    name: str
    general_category: str
    canonical_combining_class: str
    bidi_class: str
    decomposition_type: str
    numeric_value1: str
    numeric_value2: str
    numeric_value3: str
    bidi_mirrored: str
    unicode_1_name: str  # obsolete
    iso_comment: str  # obsolete
    simple_uppercase_mapping: str
    simple_lowercase_mapping: str
    simple_titlecase_mapping: str

    # Binary properties, as a set of those that are true.
    # Taken from multiple files:
    #   https://www.unicode.org/reports/tr44/#DerivedCoreProperties.txt
    #   https://www.unicode.org/reports/tr44/#PropList.txt
    binary_properties: Set[str]

    # Used to get the mirrored character, if any. See:
    #   https://www.unicode.org/reports/tr44/#BidiMirroring.txt
    bidi_mirroring: str

    @staticmethod
    def from_row(row: List[str]) -> UcdRecord:
        return UcdRecord(*row, set(), "")


class UcdFile:
    """
    A file in the standard format of the UCD.

    See: https://www.unicode.org/reports/tr44/#Format_Conventions

    Note that, as described there, the Unihan data files have their
    own separate format.
    """

    @staticmethod
    def open_data(template, version):
        local = os.path.join(DATA_DIR, template % ("-" + version,))
        if not os.path.exists(local):
            import urllib.request

            url = ("https://www.unicode.org/Public/%s/ucd/" + template) % (version, "")
            print("Fetching", url)
            os.makedirs(DATA_DIR, exist_ok=True)
            urllib.request.urlretrieve(url, filename=local)
        if local.endswith(".txt"):
            return open(local, encoding="utf-8")
        else:
            # Unihan.zip
            return open(local, "rb")

    @staticmethod
    def expand_range(char_range: str) -> Iterator[int]:
        """
        Parses ranges of code points, as described in UAX #44:
        https://www.unicode.org/reports/tr44/#Code_Point_Ranges
        """
        if ".." in char_range:
            first, last = [int(c, 16) for c in char_range.split("..")]
        else:
            first = last = int(char_range, 16)
        for char in range(first, last + 1):
            yield char

    def __init__(self, template: str, version: str) -> None:
        self.template = template
        self.version = version

    def records(self) -> Iterator[List[str]]:
        print("Reading", self.template % ("-" + self.version,))
        with self.open_data(self.template, self.version) as file:
            for line in file:
                line = line.split("#", 1)[0].strip()
                if not line:
                    continue
                yield [field.strip() for field in line.split(";")]

    def __iter__(self) -> Iterator[List[str]]:
        return self.records()

    def expanded(self) -> Iterator[Tuple[int, List[str]]]:
        for record in self.records():
            char_range, rest = record[0], record[1:]
            for char in self.expand_range(char_range):
                yield char, rest


class NamesList:
    def __init__(self, template: str, version: str) -> None:
        self.template = template
        self.version = version

    def records(self) -> Iterator[List[str]]:
        with UcdFile.open_data(self.template, self.version) as file:
            parts = []
            for line in file:
                line = line.split(";", 1)[0]
                if not line:
                    continue
                line = [field.strip() for field in line.split("\t")]
                if not line:
                    continue
                if line[0] and parts:
                    yield parts
                    parts.clear()
                parts.append(line)
            if parts:
                yield parts

    def __iter__(self) -> Iterator[List[str]]:
        return self.records()


# --------------------------------------------------------------------
# the following support code is taken from the unidb utilities
# Copyright (c) 1999-2000 by Secret Labs AB

# load a unicode-data file from disk


class UnicodeData:
    table: List[Optional[UcdRecord]]  # index is codepoint; None means unassigned

    def __init__(self, version):
        table = [None] * UNICODE_MAX
        for s in UcdFile(UNICODE_DATA, version):
            char = int(s[0], 16)
            table[char] = UcdRecord.from_row(s)

        # See Unicode Chapter 4.8 "Name Derivation Rule Prefix Strings"
        special_name_ranges = {
            "HANGUL SYLLABLE": [],
            "CJK UNIFIED IDEOGRAPH": [],
            "TANGUT IDEOGRAPH": [],
        }

        # expand first-last ranges
        field = None
        for i in range(0, UNICODE_MAX):
            # The file UnicodeData.txt has its own distinct way of
            # expressing ranges.  See:
            #   https://www.unicode.org/reports/tr44/#Code_Point_Ranges
            s = table[i]
            if s:
                if s.name[-6:] == "First>":
                    s.name = ""
                    field = dataclasses.astuple(s)[:15]
                elif s.name[-5:] == "Last>":
                    field_range = (int(field[0], 16), int(s.codepoint, 16))
                    if s.name.startswith("<Hangul Syllable"):
                        special_name_ranges["HANGUL SYLLABLE"].append(field_range)
                    elif s.name.startswith("<CJK Ideograph"):
                        special_name_ranges["CJK UNIFIED IDEOGRAPH"].append(field_range)
                    elif s.name.startswith("<Tangut Ideograph"):
                        special_name_ranges["TANGUT IDEOGRAPH"].append(field_range)
                    s.name = ""
                    field = None
            elif field:
                table[i] = UcdRecord.from_row(("%X" % i,) + field[1:])

        blocks = []
        for block_range, block in UcdFile(BLOCKS, version):
            start, end = [int(c, 16) for c in block_range.split("..")]
            first_char = next((i for i in range(start, end + 1) if table[i]), start)
            num_assigned = sum(1 for i in range(start, end + 1) if table[i])
            blocks.append((start, end, first_char, num_assigned, block))
        planes = [
            (0x0, 0xFFFD, "Basic Multilingual Plane"),
            (0x10000, 0x1FFFD, "Supplementary Multilingual Plane"),
            (0x20000, 0x2FFFD, "Supplementary Ideographic Plane"),
            (0x30000, 0x3FFFD, "Tertiary Ideographic Plane"),
            (0xE0000, 0xEFFFD, "Supplementary Special-purpose Plane"),
            (0xF0000, 0xFFFFD, "Supplementary Private Use Area-A"),
            (0x100000, 0x10FFFD, "Supplementary Private Use Area-B"),
        ]
        planes = [
            (s, e, s, sum(1 for i in range(s, e + 1) if table[i]), n)
            for s, e, n in planes
        ]
        for i in range(0x40000, 0xE0000, 0x10000):
            if all(x is None for x in table[i : i + 0x10000]):
                planes.append(
                    (i, i + 0x10000 - 3, i, 0, f"<Unassigned Plane {i//0x10000}>")
                )
            else:
                raise ValueError(
                    f"Assigned codepoints in plane {i//0x10000}, give this a name"
                )

        max_name, max_annot = 0, 0
        special_names = {}
        names = {}
        for l in NamesList(NAMES_LIST, version):
            if l[0][0].isalnum():
                char = int(l[0][0], 16)
                name = [x[1] for x in l]
                assert (len(x) == 2 for x in l)  # should only be two parts on the line
                m = re.match(
                    r"^(CJK COMPATIBILITY IDEOGRAPH|KHITAN SMALL SCRIPT CHARACTER|NUSHU CHARACTER)-[A-F0-9]+$",
                    name[0],
                )
                if name[0].startswith("<") or m:
                    name[0] = ""
                    if m:
                        specials = special_names.setdefault(m.group(1), set())
                        specials.add(char)
                if any(l for l in name):
                    nlines = len(name)
                    name = "\n".join(name).encode("utf-8") + b"\0"
                    name_split = name.split(b"\n", 1)
                    max_name = max(max_name, len(name_split[0]) + 1)
                    if len(name_split) > 1:
                        # each line expands to tab + prettified start (3 bytes)
                        max_annot = max(max_annot, len(name_split[1]) + nlines * 3 + 1)
                    names[char] = name

        for k, v in special_names.items():
            ranges = []
            min_range, prev_char = None, None
            for char in sorted(v):
                if min_range is None:
                    min_range = char
                elif char != prev_char + 1:
                    ranges.append((min_range, prev_char))
                    min_range = char
                prev_char = char
            if min_range is not None:
                ranges.append((min_range, prev_char))
            special_name_ranges[k] = ranges

        # public attributes
        self.filename = UNICODE_DATA % ""
        self.table = table
        self.chars = list(range(UNICODE_MAX))  # unicode 3.2
        self.names = names
        self.special_name_ranges = special_name_ranges
        self.max_name = ((max_name + 31) // 32) * 32
        self.max_annot = ((max_annot + 31) // 32) * 32
        self.blocks = sorted(blocks, key=lambda x: x[0])
        self.planes = sorted(planes, key=lambda x: x[0])

        for char, (p,) in UcdFile(DERIVED_CORE_PROPERTIES, version).expanded():
            if table[char]:
                # Some properties (e.g. Default_Ignorable_Code_Point)
                # apply to unassigned code points; ignore them
                table[char].binary_properties.add(p)

        for char, (p,) in UcdFile(PROP_LIST, version).expanded():
            if table[char]:
                table[char].binary_properties.add(p)

        for data in UcdFile(BIDI_MIRRORING, version):
            c = int(data[0], 16)
            table[c].bidi_mirroring = data[1]


# --------------------------------------------------------------------
# unicode character type tables


def makeutype(unicode, trace):
    FILE = "utype.c"

    print("--- Preparing", FILE, "...")

    # extract simple case mappings
    case_dummy = (0, 0, 0, 0)
    case_table = [case_dummy]
    case_cache = {case_dummy: 0}
    case_index = [0] * len(unicode.chars)
    # extract unicode type info
    type_dummy = (0, 0)
    type_table = [type_dummy]
    type_cache = {type_dummy: 0}
    type_index = [0] * len(unicode.chars)
    # use switch statements for properties with low instance counts
    switches = {}

    for char in unicode.chars:
        record = unicode.table[char]
        if record:
            # extract database properties
            category = record.general_category
            bidirectional = record.bidi_class
            properties = record.binary_properties
            flags = UcdTypeFlags.FF_UNICODE_ISUNICODEPOINTASSIGNED
            if category in ["Mn", "Mc", "Me"]:
                flags |= UcdTypeFlags.FF_UNICODE_ISCOMBINING
            if category == "Nd":
                flags |= UcdTypeFlags.FF_UNICODE_ISDIGIT
            if category == "Zs" or bidirectional in ("WS", "B", "S"):
                switches.setdefault("space", []).append(char)
            if category == "Lt":
                switches.setdefault("title", []).append(char)
            if category in ["Lm", "Lt", "Lu", "Ll", "Lo"]:
                flags |= UcdTypeFlags.FF_UNICODE_ISALPHA
            if "Lowercase" in properties:
                flags |= UcdTypeFlags.FF_UNICODE_ISLOWER
            if "Uppercase" in properties:
                flags |= UcdTypeFlags.FF_UNICODE_ISUPPER
            if "Ideographic" in properties:
                flags |= UcdTypeFlags.FF_UNICODE_ISIDEOGRAPHIC
            if "ASCII_Hex_Digit" in properties:
                switches.setdefault("hexdigit", []).append(char)
            if "Default_Ignorable_Code_Point" in properties:
                flags |= UcdTypeFlags.FF_UNICODE_ISZEROWIDTH

            # bidi flags
            if bidirectional in ("L", "LRE", "LRO"):
                flags |= UcdTypeFlags.FF_UNICODE_ISLEFTTORIGHT
            elif bidirectional in ("R", "AL", "RLE", "RLO"):
                flags |= UcdTypeFlags.FF_UNICODE_ISRIGHTTOLEFT
            elif bidirectional == "EN":
                flags |= UcdTypeFlags.FF_UNICODE_ISEURONUMERIC
            elif bidirectional == "AN":
                flags |= UcdTypeFlags.FF_UNICODE_ISARABNUMERIC
            elif bidirectional == "ES":
                switches.setdefault("euronumsep", []).append(char)
            elif bidirectional == "CS":
                switches.setdefault("commonsep", []).append(char)
            elif bidirectional == "ET":
                flags |= UcdTypeFlags.FF_UNICODE_ISEURONUMTERM

            # This is questionable... But ok
            if any(x in record.name for x in ("LIGATURE", "VULGAR", "FRACTION")):
                if char != 0x2044:  # FRACTION SLASH
                    flags |= UcdTypeFlags.FF_UNICODE_ISLIGVULGFRAC

            if record.decomposition_type:
                flags |= UcdTypeFlags.FF_UNICODE_ISDECOMPOSITIONNORMATIVE

                if record.decomposition_type.startswith("<circle>"):
                    flags |= UcdTypeFlags.FF_UNICODE_ISDECOMPCIRCLE
                elif record.decomposition_type.startswith("<initial>"):
                    flags |= UcdTypeFlags.FF_UNICODE_ISARABINITIAL
                elif record.decomposition_type.startswith("<medial>"):
                    flags |= UcdTypeFlags.FF_UNICODE_ISARABMEDIAL
                elif record.decomposition_type.startswith("<final>"):
                    flags |= UcdTypeFlags.FF_UNICODE_ISARABFINAL
                elif record.decomposition_type.startswith("<isolated>"):
                    flags |= UcdTypeFlags.FF_UNICODE_ISARABISOLATED

            if record.simple_uppercase_mapping:
                upper = int(record.simple_uppercase_mapping, 16)
            else:
                upper = char
            if record.simple_lowercase_mapping:
                lower = int(record.simple_lowercase_mapping, 16)
            else:
                lower = char
            if record.simple_titlecase_mapping:
                title = int(record.simple_titlecase_mapping, 16)
            else:
                title = upper
            if record.bidi_mirroring:
                mirror = int(record.bidi_mirroring, 16)
            else:
                mirror = char

            # No special casing or case folding
            if upper == lower == title == mirror:
                upper = lower = title = mirror = 0
            else:
                upper = upper - char
                lower = lower - char
                title = title - char
                mirror = mirror - char
                assert (
                    abs(upper) <= 2147483647
                    and abs(lower) <= 2147483647
                    and abs(title) <= 2147483647
                    and abs(mirror) <= 2147483647
                )

            # pose info
            ccc = int(record.canonical_combining_class)
            pose = get_pose(char, ccc, with_ccc=True)

            # construct the case mapping entry
            item = (upper, lower, title, mirror)
            i = case_cache.get(item)
            if i is None:
                case_cache[item] = i = len(case_table)
                case_table.append(item)
            case_index[char] = i

            # construct the type mapping entry
            item = (flags, pose)
            i = type_cache.get(item)
            if i is None:
                type_cache[item] = i = len(type_table)
                type_table.append(item)
            type_index[char] = i

    # You want to keep this generally below 255 so the index can fit in one
    # byte. Once it exceeds you double the size requirement for index2.
    print(len(case_table), "unique case mapping entries")
    print(len(type_table), "unique character type entries")
    for k, v in switches.items():
        print(len(v), k, "code points")

    print("--- Writing", FILE, "...")

    with open(FILE, "w") as fp:
        fprint = partial(print, file=fp)
        alignment = max(len(flag.name) for flag in UcdTypeFlags)

        License(extra="Contributions: Werner Lemberg, Khaled Hosny, Joe Da Silva").dump(
            fp
        )
        fprint("#include <assert.h>")
        fprint("#include <utype.h>")
        fprint()
        for flag in UcdTypeFlags:
            fprint("#define %-*s 0x%x" % (alignment, flag.name, flag.value))
        fprint()
        fprint("struct utypecasing {")
        fprint("    /* deltas from the current character */")
        fprint("    int32_t upper, lower, title, mirror;")
        fprint("};")
        fprint()
        fprint("struct utypeflags {")
        fprint("    uint32_t flags; /* One or more of the above flags */")
        fprint("    uint32_t pose; /* Positioning info */")
        fprint("};")
        fprint()
        fprint("/* a list of unique simple case mappings */")
        fprint("static const struct utypecasing casing_data[] = {")
        for item in case_table:
            fprint("    {%d, %d, %d, %d}," % item)
        fprint("};")
        fprint()
        fprint("/* a list of unique type flags */")
        fprint("static const struct utypeflags type_data[] = {")
        for item in type_table:
            fprint("    {%d, %d}," % item)
        fprint("};")
        fprint()

        case_ind = Index("casing", "struct utypecasing", case_index)
        type_ind = Index("type", "struct utypeflags", type_index)

        case_ind.dump(fp, trace, accessor=False)
        type_ind.dump(fp, trace, accessor=False)

        case_ind.dump(fp, trace, index=False)
        type_ind.dump(fp, trace, index=False)

        for flag in UcdTypeFlags:
            fprint("int %s(unichar_t ch) {" % flag.name.lower())
            fprint("    return get_type(ch)->flags & %s;" % (flag.name,))
            fprint("}")
            fprint()

        for name, flag in [
            ("ff_unicode_isideoalpha", "FF_UNICODE_ISALPHA | FF_UNICODE_ISIDEOGRAPHIC"),
            ("ff_unicode_isalnum", "FF_UNICODE_ISALPHA | FF_UNICODE_ISDIGIT"),
        ]:
            fprint("int %s(unichar_t ch) {" % name)
            fprint("    return get_type(ch)->flags & (%s);" % flag)
            fprint("}")
            fprint()

        for k in sorted(switches.keys()):
            Switch("ff_unicode_is" + k, switches[k]).dump(fp)

        fprint("int ff_unicode_pose(unichar_t ch) {")
        fprint("   return get_type(ch)->pose & ~0xff;")
        fprint("}")
        fprint()
        fprint("int ff_unicode_combiningclass(unichar_t ch) {")
        fprint("    return get_type(ch)->pose & 0xff;")
        fprint("}")
        fprint()

        conv_types = ("lower", "upper", "title", "mirror")
        for n in conv_types:
            fprint("unichar_t ff_unicode_to%s(unichar_t ch) {" % n)
            fprint("    const struct utypecasing* rec = get_casing(ch);")
            if n == "mirror":
                fprint("    if (rec->mirror == 0) {")
                fprint("        return 0;")  # special case...
                fprint("    }")
            fprint("    return (unichar_t)(((int32_t)ch) + rec->%s);" % n)
            fprint("}")
            fprint()

    return (
        [("int", x.name.lower()) for x in UcdTypeFlags]
        + [
            ("int", "ff_unicode_is" + k)
            for k in chain(["ideoalpha", "alnum"], switches)
        ]
        + [("int", "ff_unicode_pose"), ("int", "ff_unicode_combiningclass")]
        + [("unichar_t", "ff_unicode_to" + n) for n in conv_types]
    )


def makeunialt(unicode, trace):
    FILE = "unialt.c"

    print("--- Preparing", FILE, "...")

    unialt_data = [0]
    unialt_index = [0] * len(unicode.chars)
    unialt_chars = set()  # excl. visual alts

    for char in unicode.chars:
        record = unicode.table[char]
        if not record:
            continue
        elif not record.decomposition_type and char not in VISUAL_ALTS:
            continue

        # Get category and decomposition data
        if record.decomposition_type:
            decomp = record.decomposition_type.split()
            if len(decomp) > 19:
                raise Exception(
                    "character %x has a decomposition too large for nfd_nfkd" % char
                )
            if decomp[0].startswith("<"):
                decomp.pop(0)  # subset of category info stored in utype flags
            decomp = [int(s, 16) for s in decomp]
            unialt_chars.add(char)
        else:
            decomp = VISUAL_ALTS[char]
            assert len(decomp) <= 19

        # content
        decomp = decomp + [0]
        unialt_index[char] = len(unialt_data)
        unialt_data.extend(decomp)

    # NFKD forms always take precedence over visual alts
    print(
        "Ignored visual alts:",
        ", ".join("U+%04x" % x for x in VISUAL_ALTS.keys() & unialt_chars),
    )
    print(len(unialt_data), "unique decomposition entries")

    print("--- Writing", FILE, "...")

    with open(FILE, "w") as fp:
        fprint = partial(print, file=fp)

        License().dump(fp)

        fprint("#include <assert.h>")
        fprint("#include <utype.h>")
        fprint()

        fprint("/* decomposition + visual alternates data */")
        Array("unialt_data", unialt_data).dump(fp, trace)

        Index("unialt", "unichar_t", unialt_index).dump(fp, trace)

        fprint("int ff_unicode_hasunialt(unichar_t ch) {")
        fprint("   return *get_unialt(ch) != 0;")
        fprint("}")
        fprint()

        fprint("const unichar_t* ff_unicode_unialt(unichar_t ch) {")
        fprint("    const unichar_t* ptr = get_unialt(ch);")
        fprint("    if (!*ptr) {")
        fprint("        return NULL;")
        fprint("    }")
        fprint("    return ptr;")
        fprint("}")
        fprint()

        return [
            ("int", "ff_unicode_hasunialt"),
            ("const unichar_t*", "ff_unicode_unialt"),
        ]


def makearabicforms(unicode, trace):
    FILE = "ArabicForms.c"

    print("--- Preparing", FILE, "...")

    # Could compress this a little with a single stage lookup but oh well
    table = []
    lookup = {x.name: i for i, x in enumerate(unicode.table) if x and x.name}

    for char in range(0x600, 0x700):
        record = unicode.table[char]
        initial, medial, final, isolated, isletter, joindual = 0, 0, 0, 0, 0, 0
        required_lig_with_alef = char == 0x644  # 0x644 == LAM

        if record is not None:
            if not record.name.startswith("ARABIC LETTER "):
                # No op (not a letter, no fancy forms)
                initial = medial = final = isolated = char
            else:
                isletter = 1
                initial = lookup.get(record.name + " INITIAL FORM", char)
                medial = lookup.get(record.name + " MEDIAL FORM", char)
                final = lookup.get(record.name + " FINAL FORM", char)
                isolated = lookup.get(record.name + " ISOLATED FORM", char)
                joindual = int(initial != char and medial != char)
        table.append(
            (
                initial,
                medial,
                final,
                isolated,
                isletter,
                joindual,
                required_lig_with_alef,
            )
        )

    print("--- Writing", FILE, "...")

    with open(FILE, "w") as fp:
        fprint = partial(print, file=fp)
        License(extra="Contributions: Khaled Hosny, Joe Da Silva").dump(fp)
        fprint("#include <utype.h>")
        fprint()
        fprint("static struct arabicforms arabic_forms[] = {")
        fprint(
            "    /* initial, medial, final, isolated, isletter, joindual, required_lig_with_alef */"
        )
        for i, form in enumerate(table):
            fp.write("    { 0x%04x, 0x%04x, 0x%04x, 0x%04x, %d, %d, %d }," % form)
            fprint(f"    /* 0x{0x600+i:04x} */" if (i % 32) == 0 else "")
        fprint("};")
        fprint()

        fprint("const struct arabicforms* arabicform(unichar_t ch) {")
        fprint("    if (ch >= 0x600 && ch <= 0x6ff) {")
        fprint("        return &arabic_forms[ch - 0x600];")
        fprint("    }")
        fprint("    return NULL;")
        fprint("}")


def makeuninames(unicode, trace):
    FILE = "uninames_data.h"

    print("--- Preparing", FILE, "...")

    # Chosen empirically, tweak as desired. Only ascii is allowed due to
    # how the lexicon is encoded. Generally longer replacements go first
    # so shorter replacements don't prevent them from working.
    # As of Unicode 15 this is roughly 468kb excluding the size of the
    # offset tables
    regexes = [
        (100, re.compile(rb"[\x20-\x7F]{3,}[ -]")),
        (100, re.compile(rb"(?:[\x21-\x7F]+[ -]+){5}")),
        (150, re.compile(rb"(?:[\x21-\x7F]+[ -]+){4}")),
        (250, re.compile(rb"(?:[\x21-\x7F]+[ -]+){3}")),
        (750, re.compile(rb"(?:[\x21-\x7F]+[ -]+){2}")),
        (3400, re.compile(rb"(?:[\x21-\x7F]+[ -]+){1}")),
        (5000, re.compile(rb"\b(?:[a-z]{3,}|[\x21-\x60\x7B-\x7F]{3,})\b")),
    ]
    # The initial character on annotation lines are excluded from the phrasebook
    # This allows us to substitute them for fancier characters
    annot_re = re.compile(rb"\n(.) ")

    # Max realisable saving for given replacement. Other replacements may
    # preclude realising the full benefit. Each replacement costs 2 bytes,
    # plus space in the lexicon (equal to length of replacement).
    def benefit(w: bytes, f: int) -> int:
        return (len(w) - 2) * f - len(w)

    # Lexicon encoding scheme: 0b10xxxxxx 0b1xxxxxxx (13 usable bits)
    # Decoder must also be utf-8 aware, and skip valid utf-8 sequences.
    # Possible alternative: Allow any value > 0x20 for second byte, to keep
    # newlines/nul chars in the raw string (permits strstr). (14 usable bits)
    intermediate = {x: annot_re.sub(rb"\n", y) for x, y in unicode.names.items()}
    names = {**unicode.names}
    replacements = set()
    wordlist = {}
    for count, regex in regexes:
        replacements.update(
            x[0]
            for x in Counter(
                part for name in intermediate.values() for part in regex.findall(name)
            ).most_common(count)
            if benefit(*x) > 0
        )

        for char, name in intermediate.items():
            wn = names[char]
            for part in regex.findall(name):
                if part in replacements:
                    i = wordlist.get(part)
                    if i is None:
                        i = len(wordlist)
                        assert i < 2 ** 13, "lexicon overflow; adjust regexes"
                        i = bytes((0x80 | ((i >> 7) & 0x3F), 0x80 | (i & 0x7F)))
                        wordlist[part] = i
                    wn = wn.replace(part, i)
                    name = name.replace(part, b"")
            intermediate[char] = name
            names[char] = wn

    lexicon = b""
    lexicon_offset = []
    lexicon_shifts = []
    lexicon_shift = 12  # 2^12=4096; midpoint of the lexicon

    current_offset = 0
    # build a lexicon string
    for w in wordlist.keys():
        # encoding: high bit indicates last character in word (assumes ascii)
        assert w.isascii()
        ww = w[:-1] + bytes([int(w[-1]) | 0x80])
        offset = len(lexicon)
        offset_index = len(lexicon_offset) >> lexicon_shift
        if offset_index + 1 > len(lexicon_shifts):
            current_offset = offset
            while len(lexicon_shifts) < offset_index:
                lexicon_shifts.append(lexicon_shifts[-1])
            lexicon_shifts.append(current_offset)
        offset -= current_offset
        lexicon_offset.append(offset)
        lexicon += ww

    lexicon = list(lexicon)
    assert getsize(lexicon) == 1
    assert getsize(lexicon_offset) < 4
    assert getsize(lexicon_shifts) < 4

    phrasebook = [0]
    phrasebook_offset = [0] * len(unicode.chars)
    phrasebook_shifts = []
    phrasebook_shift = 11  # Empirical choice to keep offsets below 64k
    phrasebook_shift_cap = 63  # Highly concentrated in the first/second planes
    current_offset = 0

    for char in unicode.chars:
        name = names.get(char)
        if name is not None:
            name = annot_re.sub(rb"\n\1", name)
            offset = len(phrasebook)
            offset_index = min(char >> phrasebook_shift, phrasebook_shift_cap)
            if offset_index + 1 > len(phrasebook_shifts):
                current_offset = offset - 1
                while len(phrasebook_shifts) < offset_index:
                    phrasebook_shifts.append(phrasebook_shifts[-1])
                phrasebook_shifts.append(current_offset)
            offset -= current_offset
            assert offset > 0  # retain special meaning of 0 being no entry
            phrasebook_offset[char] = offset
            phrasebook += list(name)

    assert getsize(phrasebook) == 1
    assert getsize(phrasebook_offset) < 4
    # print("lexicon_offset", len(lexicon_offset))
    # print("lexicon", len(lexicon))
    # print("phrasebook", len(phrasebook))
    # print("Lexicon+phrasebook data size", len(phrasebook)+len(lexicon))
    # return

    print("Number of lexicon entries:", len(lexicon_offset))
    print("--- Writing", FILE, "...")

    with open(FILE, "w") as fp:
        fprint = partial(print, file=fp)

        License(copyright="2021 by Jeremy Tan").dump(fp)
        fprint("#ifndef FONTFORGE_UNINAMES_DATA_H")
        fprint("#define FONTFORGE_UNINAMES_DATA_H")
        fprint()

        fprint("#include <intl.h>")
        fprint("#include <ustring.h>")
        fprint("#include <utype.h>")
        fprint()

        fprint("/* Basic definitions */")
        fprint("#define MAX_NAME_LENGTH", unicode.max_name)
        fprint("#define MAX_ANNOTATION_LENGTH", unicode.max_annot)
        fprint()

        fprint("/* unicode ranges data */")
        fprint("static const struct unicode_range unicode_blocks[] = {")
        for block in unicode.blocks:
            fprint('    {0x%04X, 0x%04X, 0x%04X, %d, N_("%s")},' % block)
        fprint("};")
        fprint("static const struct unicode_range unicode_planes[] = {")
        for plane in unicode.planes:
            fprint('    {0x%04X, 0x%04X, 0x%04X, %d, N_("%s")},' % plane)
        fprint("};")
        fprint()

        fprint("/* lexicon data */")
        fprint("#define LEXICON_SHIFT", lexicon_shift)
        Array("lexicon_data", lexicon).dump(fp, trace)
        Array("lexicon_offset", lexicon_offset).dump(fp, trace)
        Array("lexicon_shift", lexicon_shifts).dump(fp, trace)

        index1, index2, shift = splitbins(phrasebook_offset, 1)

        fprint("/* phrasebook data */")
        fprint("#define PHRASEBOOK_SHIFT1", shift)
        fprint("#define PHRASEBOOK_SHIFT2", phrasebook_shift)
        fprint("#define PHRASEBOOK_SHIFT2_CAP", phrasebook_shift_cap)
        Array("phrasebook_data", phrasebook).dump(fp, trace)
        Array("phrasebook_index1", index1).dump(fp, trace)
        Array("phrasebook_index2", index2).dump(fp, trace)
        Array("phrasebook_shift", phrasebook_shifts).dump(fp, trace)

        # fmt: off
        jamo_l = ["G", "GG", "N", "D", "DD", "R", "M", "B", "BB", "S", "SS",
            "", "J", "JJ", "C", "K", "T", "P", "H"]
        jamo_v = ["A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O", "WA",
            "WAE", "OE", "YO", "U", "WEO", "WE", "WI","YU", "EU", "YI", "I"]
        jamo_t = ["", "G", "GG", "GS", "N", "NJ", "NH", "D", "L", "LG", "LM",
            "LB", "LS", "LT", "LP", "LH", "M","B", "BS", "S", "SS", "NG",
            "J", "C", "K", "T", "P", "H"]
        # fmt: on

        fprint("/* Unicode chapter 3.12 on Hangul Character Name Generation */")
        fprint("#define JAMO_V_COUNT", len(jamo_v))
        fprint("#define JAMO_T_COUNT", len(jamo_t))
        fprint("#define JAMO_N_COUNT (JAMO_V_COUNT * JAMO_T_COUNT)")
        Array("jamo_l", jamo_l).dump(fp, trace)
        Array("jamo_v", jamo_v).dump(fp, trace)
        Array("jamo_t", jamo_t).dump(fp, trace)

        fprint("static char* get_derived_name(unichar_t ch) {")

        # This follows NR1, see Unicode chapter 4, table 4.8
        hangul_range = unicode.special_name_ranges["HANGUL SYLLABLE"]
        assert hangul_range == [(0xAC00, 0xD7A3)]
        fprint("    if (ch >= 0xAC00 && ch <= 0xD7A3) {")
        fprint("        ch -= 0xAC00;")
        fprint("        int l = ch / JAMO_N_COUNT;")
        fprint("        int v = (ch % JAMO_N_COUNT) / JAMO_T_COUNT;")
        fprint("        int t = ch % JAMO_T_COUNT;")
        fprint('        return smprintf("HANGUL SYLLABLE %s%s%s",')
        fprint("            jamo_l[l], jamo_v[v], jamo_t[t]);")
        fprint("    }")

        # The rest follows NR2
        items = sorted(unicode.special_name_ranges.items(), key=lambda x: x[1][0])
        for name, ranges in items:
            if name == "HANGUL SYLLABLE":
                continue
            cond = "    if ("
            for i, (start, end) in enumerate(ranges):
                if i > 0:
                    cond += " || "
                range_cond = "ch >= 0x%X && ch <= 0x%X" % (start, end)
                if len(ranges) > 1:
                    range_cond = "(" + range_cond + ")"
                if len(cond) + len(range_cond) > 78:
                    fprint(cond.rstrip())
                    cond = "        " + range_cond
                else:
                    cond += range_cond
            fprint(cond + ") {")
            fprint('        return smprintf("%s-%%X", ch);' % name)
            fprint("    }")
        fprint("    return NULL;")
        fprint("}")
        fprint()

        fprint("#endif /* FONTFORGE_UNINAMES_DATA_H */")

    return (
        phrasebook,
        phrasebook_offset,
        phrasebook_shifts,
        lexicon,
        lexicon_offset,
        lexicon_shifts,
        lexicon_shift,
    )


def makeutypeheader(utype_funcs):
    FILE = "utype.h"
    utype_funcs = [(t, n, n.replace("ff_unicode_", "")) for t, n in utype_funcs]

    print("--- Writing", FILE, "...")

    with open(os.path.join(BASE, "..", "inc", FILE), "w") as fp:
        fprint = partial(print, file=fp)

        License().dump(fp)
        fprint("#ifndef FONTFORGE_UNICODE_UTYPE2_H")
        fprint("#define FONTFORGE_UNICODE_UTYPE2_H")
        fprint()
        fprint(
            "#include <ctype.h> /* Include here so we can control it. If a system header includes it later bad things happen */"
        )
        fprint(
            '#include "basics.h" /* Include here so we can use pre-defined int types to correctly size constant data arrays. */'
        )
        fprint()

        fprint("/* Unicode basics */")
        fprint('#define UNICODE_VERSION "%s"' % UNIDATA_VERSION)
        fprint("#define UNICODE_MAX 0x%x" % UNICODE_MAX)
        fprint()

        alignment = len("FF_UNICODE_") + max(len(x.name) for x in Pose)
        fprint("/* Pose flags */")
        fprint("#define %-*s -1" % (alignment, "FF_UNICODE_NOPOSDATAGIVEN"))
        for c in Pose:
            fprint("#define %-*s 0x%x" % (alignment, "FF_UNICODE_" + c.name, c))
        fprint()

        for rettype, fn, _ in utype_funcs:
            fprint("extern %s %s(unichar_t ch);" % (rettype, fn))
        fprint()

        for _, fn, realfn in utype_funcs:
            fprint("#undef %s" % realfn)
        fprint()

        alignment = max(len(x) + 4 for _, _, x in utype_funcs)
        for _, fn, realfn in utype_funcs:
            fprint("#define %-*s %s((ch))" % (alignment, realfn + "(ch)", fn))
        fprint()

        fprint("struct arabicforms {")
        fprint("    unsigned short initial, medial, final, isolated;")
        fprint("    unsigned int isletter: 1;")
        fprint("    unsigned int joindual: 1;")
        fprint("    unsigned int required_lig_with_alef: 1;")
        fprint("};")
        fprint()
        fprint("extern const struct arabicforms* arabicform(unichar_t ch);")
        fprint()

        fprint("struct unicode_range {")
        fprint("    unichar_t start;")
        fprint("    unichar_t end;")
        fprint("    unichar_t first_char;")
        fprint("    int num_assigned;")
        fprint("    const char *name;")
        fprint("};")
        fprint()

        fprint("extern char* uniname_name(unichar_t ch);")
        fprint("extern char* uniname_annotation(unichar_t ch, int prettify);")
        fprint("extern char* uniname_formal_alias(unichar_t ch);")
        fprint("extern const struct unicode_range* uniname_block(unichar_t ch);")
        fprint("extern const struct unicode_range* uniname_plane(unichar_t ch);")
        fprint("extern const struct unicode_range* uniname_blocks(int *sz);")
        fprint("extern const struct unicode_range* uniname_planes(int *sz);")
        fprint()

        fprint("#endif /* FONTFORGE_UNICODE_UTYPE2_H */")


# stuff to deal with arrays of unsigned integers


class Array:
    def __init__(self, name, data):
        self.name = name
        self.data = data
        self.is_str = isinstance(data[0], str) if data else False

    def dump(self, file, trace=0):
        # write data to file, as a C array
        file.write("static const ")
        if self.is_str:
            file.write("char *")
        else:
            size = getsize(self.data)
            if trace:
                print(self.name + ":", size * len(self.data), "bytes", file=sys.stderr)
            if size == 1:
                file.write("unsigned char")
            elif size == 2:
                file.write("unsigned short")
            else:
                file.write("unsigned int")
        file.write(" " + self.name + "[] = {\n")
        if self.data:
            s = "    "
            for item in self.data:
                i = str(int(item)) + ", " if not self.is_str else f'"{item}", '
                if len(s) + len(i) > 78:
                    file.write(s.rstrip() + "\n")
                    s = "    " + i
                else:
                    s = s + i
            if s.strip():
                file.write(s.rstrip() + "\n")
        file.write("};\n\n")


class Index:
    def __init__(self, prefix, rectype, index):
        self.prefix = prefix
        self.rectype = rectype
        self.index = index

    def dump(self, file, trace, index=True, accessor=True):
        fprint = partial(print, file=file)
        if index:
            index1, index2, shift = splitbins(self.index, trace)
            fprint("/* %s indexes */" % self.prefix)
            fprint("#define %s_SHIFT" % self.prefix.upper(), shift)
            Array(self.prefix + "_index1", index1).dump(file, trace)
            Array(self.prefix + "_index2", index2).dump(file, trace)
        if accessor:
            prefix, rectype = self.prefix, self.rectype
            func = f"get_{self.prefix.lower()}"
            shift = self.prefix.upper() + "_SHIFT"

            # fmt: off
            fprint(f"static const {rectype}* {func}(unichar_t ch) {{")
            fprint(f"    int index = 0;")
            fprint(f"    if (ch < UNICODE_MAX) {{")
            fprint(f"        index = {prefix}_index1[ch >> {shift}];")
            fprint(f"        index = {prefix}_index2[(index << {shift}) + (ch & ((1 << {shift}) - 1))];")
            fprint(f"    }}")
            fprint(f"    assert(index >= 0 && (size_t)index < sizeof({prefix}_data)/sizeof({prefix}_data[0]));")
            fprint(f"    return &{prefix}_data[index];")
            fprint(f"}}")
            fprint()
            # fmt: on


class Switch:
    def __init__(self, name, data):
        self.name = name
        self.data = data

    def dump(self, file):
        fprint = partial(print, file=file)
        fprint("int %s(unichar_t ch) {" % self.name)
        fprint("    switch (ch) {")

        for codepoint in sorted(self.data):
            fprint("    case 0x%04X:" % (codepoint,))
        fprint("        return 1;")

        fprint("    }")
        fprint("    return 0;")
        fprint("}")
        fprint()


class License:
    def __init__(self, copyright=None, extra=None):
        self.copyright = copyright or "2000-2012 by George Williams"
        self.extra = f"/* {extra} */\n" if extra else ""

    def dump(self, file):
        file.write(
            f"""/* This is a GENERATED file - from {SCRIPT} with Unicode {UNIDATA_VERSION} */

/* Copyright (C) {self.copyright} */
{self.extra}/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

"""
        )


def getsize(data: List[int]) -> int:
    # return smallest possible integer size for the given array
    maxdata = max(data)
    if maxdata < 256:
        return 1
    elif maxdata < 65536:
        return 2
    else:
        return 4


def splitbins(t: List[int], trace=0) -> Tuple[List[int], List[int], int]:
    """t, trace=0 -> (t1, t2, shift).  Split a table to save space.

    t is a sequence of ints.  This function can be useful to save space if
    many of the ints are the same.  t1 and t2 are lists of ints, and shift
    is an int, chosen to minimize the combined size of t1 and t2 (in C
    code), and where for each i in range(len(t)),
        t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    where mask is a bitmask isolating the last "shift" bits.

    If optional arg trace is non-zero (default zero), progress info
    is printed to sys.stderr.  The higher the value, the more info
    you'll get.
    """

    if trace:

        def dump(t1, t2, shift, bytes):
            print(
                "%d+%d bins at shift %d; %d bytes" % (len(t1), len(t2), shift, bytes),
                file=sys.stderr,
            )

        print("Size of original table:", len(t) * getsize(t), "bytes", file=sys.stderr)
    n = len(t) - 1  # last valid index
    maxshift = 0  # the most we can shift n and still have something left
    if n > 0:
        while n >> 1:
            n >>= 1
            maxshift += 1
    del n
    bytes = sys.maxsize  # smallest total size so far
    t = tuple(t)  # so slices can be dict keys
    for shift in range(maxshift + 1):
        t1 = []
        t2 = []
        size = 2 ** shift
        bincache = {}
        for i in range(0, len(t), size):
            bin = t[i : i + size]
            index = bincache.get(bin)
            if index is None:
                index = len(t2)
                bincache[bin] = index
                t2.extend(bin)
            t1.append(index >> shift)
        # determine memory size
        b = len(t1) * getsize(t1) + len(t2) * getsize(t2)
        if trace > 1:
            dump(t1, t2, shift, b)
        if b < bytes:
            best = t1, t2, shift
            bytes = b
    t1, t2, shift = best
    if trace:
        print("Best:", end=" ", file=sys.stderr)
        dump(t1, t2, shift, bytes)
    if __debug__:
        # exhaustively verify that the decomposition is correct
        mask = ~((~0) << shift)  # i.e., low-bit mask of shift bits
        for i in range(len(t)):
            assert t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    return best


def maketables(trace=0):
    print("--- Reading", UNICODE_DATA % "", "...")

    unicode = UnicodeData(UNIDATA_VERSION)
    utype_funcs = makeutype(unicode, trace)
    unialt_funcs = makeunialt(unicode, trace)
    makearabicforms(unicode, trace)
    makeuninames(unicode, trace)
    makeutypeheader(utype_funcs + unialt_funcs)


if __name__ == "__main__":
    maketables(1)
