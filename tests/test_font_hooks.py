import fontforge

def on_new_font(new_font):
    new_font.createChar(-1, "dummy_char")

fontforge.hooks["newFontHook"] = on_new_font

# Create a new font. We expect to find the dummy character which was created
# by the hook.
font = fontforge.font()
char_idx = font.findEncodingSlot("dummy_char")
assert char_idx != -1
