# PO Language Helpers

This directory contains helper assets for generating locale language/script translations from CLDR.

## Files

- `msgids.csv`: Source list of message IDs (`section,msgid`) used for lookup.
- `build_lang_po_from_cldr.py`: Generates one output PO file from:
  - CSV msgids
  - CLDR English XML (`en.xml`)
  - Existing locale PO (`po/<locale>.po`)
  - CLDR localized XML (`<locale>.xml`)
- `build_lang_po_all_locales.sh`: Batch runner over locales from `po/LINGUAS`.

## Requirements

- Python 3
- `polib` Python package

Install `polib` if needed:

```bash
python3 -m pip install polib
```

## Batch Generation

Run from repository root:

```bash
./po/tools/build_lang_po_all_locales.sh ~/Desktop/common/main
```

Where `~/Desktop/common/main` is the CLDR `main` directory containing `en.xml` and locale XML files.

Outputs are written to:

- `lang/<locale>.po`

Behavior:

- Existing generated output files in `lang/` are overwritten on each run.
- Existing translations already present in `po/<locale>.po` are not overridden.
- Only entries found via CLDR English type lookup and localized CLDR value lookup are emitted.

## Single Locale Generation

```bash
python3 po/tools/build_lang_po_from_cldr.py \
  po/tools/msgids.csv \
  ~/Desktop/common/main/en.xml \
  po/de.po \
  ~/Desktop/common/main/de.xml \
  -o lang/de.po
```
