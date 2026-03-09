import os
import sys
import tempfile
import zipfile

import fontforge

source_path = sys.argv[1]

source_font = fontforge.open(source_path)
expected_fontname = source_font.fontname
expected_familyname = source_font.familyname
expected_fullname = source_font.fullname
expected_em = source_font.em
expected_glyph_count = len(source_font)
source_font.close()

with tempfile.TemporaryDirectory(prefix="fontforge-zip-open-") as temp_dir:
    zip_path = os.path.join(temp_dir, os.path.basename(source_path) + ".zip")

    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        zf.write(source_path, arcname=os.path.basename(source_path))

    if not os.path.exists(zip_path):
        raise ValueError("ZIP file was not created")

    zipped_font = fontforge.open(zip_path)

    if zipped_font.fontname != expected_fontname:
        raise ValueError(
            f"Unexpected fontname from zipped font: {zipped_font.fontname}"
        )

    if zipped_font.familyname != expected_familyname:
        raise ValueError(
            f"Unexpected familyname from zipped font: {zipped_font.familyname}"
        )

    if zipped_font.fullname != expected_fullname:
        raise ValueError(
            f"Unexpected fullname from zipped font: {zipped_font.fullname}"
        )

    if zipped_font.em != expected_em:
        raise ValueError(
            f"Unexpected EM from zipped font: {zipped_font.em}"
        )

    if len(zipped_font) != expected_glyph_count:
        raise ValueError(
            "Unexpected glyph count from zipped font: "
            f"{len(zipped_font)}"
        )

    zipped_font.close()
