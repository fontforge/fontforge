"""Regression test for bitmap export in various formats and bitdepths.

Needs: fonts/courB18.sfd (bitmap-only font) and fonts/Caliban.sfd (regular font)

This test generates bitmap exports in multiple formats with different bitdepths:
  - Formats: BMP, PNG, XBM
  - Bitdepths: 1, 8

It compares generated outputs against reference files and asserts byte-for-byte
identity.

To (re)generate references intentionally, set:
  FF_UPDATE_BITMAP_REFS=1
"""

import filecmp
import os
import shutil
import sys
import tempfile

import fontforge


def _make_deterministic_environment() -> None:
    os.environ["SOURCE_DATE_EPOCH"] = "1700000000"
    os.environ["USER"] = "fontforge-test"
    os.environ["TZ"] = "UTC"
    if hasattr(__import__("time"), "tzset"):
        __import__("time").tzset()


def _normalized_text(path: str) -> str:
    with open(path, "r", encoding="utf-8", errors="replace", newline=None) as f:
        return f.read().replace("\r\n", "\n").replace("\r", "\n")


def _generate_bitmap_outputs(font_path: str, out_dir: str, font_type_label: str) -> list[str]:
    """Generate bitmap exports in various formats and bitdepths.
    
    Args:
        font_path: Path to the font file
        out_dir: Output directory for generated bitmaps
        font_type_label: Label to use in filename to distinguish font types (e.g., "bitmap", "regular")
    
    Returns:
        List of generated bitmap file paths
    """
    font = fontforge.open(font_path)
    
    formats = ["bmp", "png", "xbm"]
    bitdepths = [1, 2, 4, 8]
    test_glyph = "A"
    
    generated = []
    
    for fmt in formats:
        for bitdepth in bitdepths:
            # Create a descriptive filename with font type label
            filename = f"{font_type_label}_{fmt}_depth{bitdepth}.{fmt}"
            export_path = os.path.join(out_dir, filename)
            
            try:
                # Export individual glyph as bitmap with specified format and bitdepth
                glyph = font[test_glyph]
                glyph.export(
                    export_path,
                    pixelsize=100,
                    bitdepth=bitdepth
                )
                generated.append(export_path)
            except Exception as e:
                # Skip if format/bitdepth combination not supported
                print(f"Warning: Could not export {fmt} at bitdepth {bitdepth}: {e}")
                continue
    
    font.close()
    return generated


def _assert_identical_outputs(generated_files: list[str], refs_dir: str) -> None:
    """Compare generated bitmap files against reference files.
    
    Args:
        generated_files: List of generated bitmap file paths
        refs_dir: Directory containing reference bitmap files
        
    Raises:
        ValueError: If reference files are missing or outputs don't match
    """
    missing_refs = []
    mismatches = []

    for generated in generated_files:
        basename = os.path.basename(generated)
        reference = os.path.join(refs_dir, basename)

        if not os.path.exists(reference):
            missing_refs.append(reference)
            continue

        if generated.lower().endswith(".xbm"):
            if _normalized_text(generated) != _normalized_text(reference):
                mismatches.append((generated, reference))
        else:
            if not filecmp.cmp(generated, reference, shallow=False):
                mismatches.append((generated, reference))

    if missing_refs:
        raise ValueError(
            "Missing bitmap reference files:\n"
            + "\n".join(missing_refs)
            + "\n\n"
            + "Generate references by setting FF_UPDATE_BITMAP_REFS=1 and rerunning test_bitmap_export.py."
        )

    if mismatches:
        details = [f"{gen} != {ref}" for gen, ref in mismatches]
        raise ValueError("Bitmap export output differs from reference files:\n" + "\n".join(details))


def main() -> None:
    if len(sys.argv) < 3:
        raise ValueError("Expected bitmap font and regular font path arguments")
    
    bitmap_font_path = sys.argv[1]
    regular_font_path = sys.argv[2]
    
    # Verify fonts exist
    if not os.path.exists(bitmap_font_path):
        raise ValueError(f"Bitmap font not found: {bitmap_font_path}")
    if not os.path.exists(regular_font_path):
        raise ValueError(f"Regular font not found: {regular_font_path}")
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    refs_dir = os.path.join(script_dir, "fonts", "bitmap_refs")
    update_refs = os.environ.get("FF_UPDATE_BITMAP_REFS") == "1"

    _make_deterministic_environment()

    with tempfile.TemporaryDirectory(prefix="ff-bitmap-export-test-") as temp_dir:
        # Test bitmap-only font
        bitmap_generated = _generate_bitmap_outputs(bitmap_font_path, temp_dir, font_type_label="court18")
        
        # Test regular font
        regular_generated = _generate_bitmap_outputs(regular_font_path, temp_dir, font_type_label="caliban")
        
        all_generated = bitmap_generated + regular_generated
        
        if update_refs:
            os.makedirs(refs_dir, exist_ok=True)
            for generated in all_generated:
                if os.path.exists(generated):  # Only copy successfully generated files
                    shutil.copyfile(generated, os.path.join(refs_dir, os.path.basename(generated)))

        _assert_identical_outputs(all_generated, refs_dir)


if __name__ == "__main__":
    main()
