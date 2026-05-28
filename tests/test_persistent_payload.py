# Round-trip persistent payload for font and glyph through SFD save/open.

import os
import tempfile
import fontforge

font_point = fontforge.point((12.5, -3.25), True)
glyph_point = fontforge.point((7.0, 9.5), False)

font_payload = {
    "name": "font-payload",
    "items": [1, 2, 3],
    "nested": {"ok": True, "text": "hello"},
    "pt": font_point,
}

glyph_payload = {
    "name": "glyph-payload",
    "coords": [(1, 2), (3, 4)],
    "value": 42,
    "pt": glyph_point,
}

font = fontforge.font()
glyph = font.createChar(0x41, "A")
glyph.width = 500

contour = fontforge.contour()
contour[:] = [(0, 0), (200, 0), (200, 200), (0, 200)]
contour.closed = True

layer = fontforge.layer()
layer += contour
glyph.setLayer(layer, 1)

font.persistent = font_payload
glyph.persistent = glyph_payload

with tempfile.TemporaryDirectory() as tmpdirname:
    sfd_path = os.path.join(tmpdirname, "persistent_payload_roundtrip.sfd")

    font.save(sfd_path)
    font.close()

    reopened = fontforge.open(sfd_path)
    try:
        persistent_font = reopened.persistent
        if persistent_font["name"] != font_payload["name"]:
            raise ValueError(
                "Font persistent string payload mismatch after reopen")
        if persistent_font["items"] != font_payload["items"]:
            raise ValueError("Font persistent list payload mismatch after reopen")
        if persistent_font["nested"] != font_payload["nested"]:
            raise ValueError(
                "Font persistent nested payload mismatch after reopen")
        if (persistent_font["pt"].x != font_point.x or
                persistent_font["pt"].y != font_point.y or
                persistent_font["pt"].on_curve != font_point.on_curve):
            raise ValueError("Font persistent point payload mismatch after reopen")

        reopened_glyph = reopened["A"]
        persistent_glyph = reopened_glyph.persistent
        if persistent_glyph["name"] != glyph_payload["name"]:
            raise ValueError(
                "Glyph persistent string payload mismatch after reopen")
        if persistent_glyph["coords"] != glyph_payload["coords"]:
            raise ValueError("Glyph persistent coords payload mismatch after reopen")
        if persistent_glyph["value"] != glyph_payload["value"]:
            raise ValueError("Glyph persistent value payload mismatch after reopen")
        if (persistent_glyph["pt"].x != glyph_point.x or
                persistent_glyph["pt"].y != glyph_point.y or
                persistent_glyph["pt"].on_curve != glyph_point.on_curve):
            raise ValueError(
                "Glyph persistent point payload mismatch after reopen")
    finally:
        reopened.close()
