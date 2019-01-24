#Needs: fonts/NotoSans-Regular.ttc

import fontforge
import sys

for i, s in enumerate(('Noto Sans', 'Noto Sans UI')):
    for k in (str(i), s):
        f = fontforge.open(sys.argv[1] + '(' + k + ')')
        if f.fullname != s:
            raise ValueError('Expected fontname "' + s + '" instead of "' + f.fontname + '" for key "' + k + '"')
        f.close()
