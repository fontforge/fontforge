import fontforge
import tempfile
import os

font = fontforge.font()

names = (("English (US)", "Designer", "Hello😀"),
         ("Portuguese (Portugal)", "Copyright", "Olá🤠"),
         ("French French", "Version", "🎼 Salut"))

font.sfnt_names = names

with tempfile.TemporaryDirectory() as tmpdirname:
        sfd_full_name = os.path.join(tmpdirname, "Test.sfd")
        font.save(sfd_full_name)

        # Make sure the font is purged from memory
        font.close()

        font2 = fontforge.open(sfd_full_name)

        # Verify each name was successfully stored in SFD
        for name in names:
                assert(name in font2.sfnt_names)