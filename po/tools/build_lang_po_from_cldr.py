#!/usr/bin/env python3

import argparse
import csv
import os
import xml.etree.ElementTree as ET

import polib


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Build a PO file with missing translations by mapping CSV msgids "
            "through CLDR English and localized XML files."
        )
    )
    parser.add_argument("csv_path", help="Path to CSV file (for example: po/tools/msgids.csv)")
    parser.add_argument("en_xml_path", help="Path to CLDR English XML (for example: ~/Desktop/common/main/en.xml)")
    parser.add_argument("po_path", help="Path to existing PO file (for example: po/de.po)")
    parser.add_argument("loc_xml_path", help="Path to localized CLDR XML (for example: ~/Desktop/common/main/de.xml)")
    parser.add_argument(
        "-o",
        "--output",
        default="de-lang.po",
        help="Output PO path (default: de-lang.po)",
    )
    return parser.parse_args()


def expand_path(path):
    return os.path.abspath(os.path.expanduser(path))


def strip_prefix(msgid):
    if "|" in msgid:
        return msgid.split("|", 1)[1]
    return msgid


def build_en_type_index(en_xml_path):
    root = ET.parse(en_xml_path).getroot()
    index = {"languages": {}, "scripts": {}}

    for node in root.findall(".//localeDisplayNames/languages/language"):
        if "alt" in node.attrib:
            continue
        text = (node.text or "").strip()
        typ = node.attrib.get("type", "").strip()
        if text and typ and text not in index["languages"]:
            index["languages"][text] = typ

    for node in root.findall(".//localeDisplayNames/scripts/script"):
        if "alt" in node.attrib:
            continue
        text = (node.text or "").strip()
        typ = node.attrib.get("type", "").strip()
        if text and typ and text not in index["scripts"]:
            index["scripts"][text] = typ

    return index


def build_loc_value_index(loc_xml_path):
    root = ET.parse(loc_xml_path).getroot()
    index = {"languages": {}, "scripts": {}}

    for node in root.findall(".//localeDisplayNames/languages/language"):
        if "alt" in node.attrib:
            continue
        typ = node.attrib.get("type", "").strip()
        text = (node.text or "").strip()
        if typ and text and typ not in index["languages"]:
            index["languages"][typ] = text

    for node in root.findall(".//localeDisplayNames/scripts/script"):
        if "alt" in node.attrib:
            continue
        typ = node.attrib.get("type", "").strip()
        text = (node.text or "").strip()
        if typ and text and typ not in index["scripts"]:
            index["scripts"][typ] = text

    return index


def load_or_create_output_po(output_path):
    if os.path.exists(output_path):
        return polib.pofile(output_path)

    po = polib.POFile()
    po.metadata = {
        "MIME-Version": "1.0",
        "Content-Type": "text/plain; charset=UTF-8",
        "Content-Transfer-Encoding": "8bit",
    }
    return po


def has_translation(po_file, msgid):
    entry = po_file.find(msgid)
    if entry is None:
        return False
    return bool((entry.msgstr or "").strip())


def main():
    args = parse_args()

    csv_path = expand_path(args.csv_path)
    en_xml_path = expand_path(args.en_xml_path)
    po_path = expand_path(args.po_path)
    loc_xml_path = expand_path(args.loc_xml_path)
    output_path = expand_path(args.output)

    source_po = polib.pofile(po_path)
    output_po = load_or_create_output_po(output_path)

    en_types = build_en_type_index(en_xml_path)
    loc_values = build_loc_value_index(loc_xml_path)

    added = 0
    skipped_existing = 0
    skipped_lookup = 0

    with open(csv_path, newline="", encoding="utf-8") as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            section = (row.get("section") or "").strip()
            msgid = (row.get("msgid") or "").strip()

            if section not in ("languages", "scripts") or not msgid:
                continue

            if has_translation(source_po, msgid):
                skipped_existing += 1
                continue

            if has_translation(output_po, msgid):
                # Keep existing generated/manual value in output file.
                skipped_existing += 1
                continue

            lookup_text = strip_prefix(msgid)
            cldr_type = en_types[section].get(lookup_text)
            if not cldr_type:
                cldr_type = en_types[section].get(msgid)
            if not cldr_type:
                skipped_lookup += 1
                continue

            loc_text = loc_values[section].get(cldr_type)
            if not loc_text:
                skipped_lookup += 1
                continue

            output_po.append(polib.POEntry(msgid=msgid, msgstr=loc_text))
            added += 1

    output_po.save(output_path)

    print(f"Wrote: {output_path}")
    print(f"Added entries: {added}")
    print(f"Skipped existing translations: {skipped_existing}")
    print(f"Skipped missing CLDR lookup/translation: {skipped_lookup}")


if __name__ == "__main__":
    main()
