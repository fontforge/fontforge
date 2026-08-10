import gdb
import gdb.printing


class FFTagPrinter:
    def __init__(self, val):
        self.val = val

    def _char_at(self, index):
        try:
            raw = int(self.val['arr']['_M_elems'][index])
        except Exception:
            raw = int(self.val['arr']['__elems_'][index])
        return raw & 0xFF

    def to_string(self):
        chars = []
        for i in range(4):
            code = self._char_at(i)
            if code == 0:
                chars.append(' ')
            elif 32 <= code <= 126:
                chars.append(chr(code))
            else:
                chars.append('\\x%02x' % code)
        return "ff::Tag('%s')" % ''.join(chars)


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('fontforge')
    pp.add_printer('ff::Tag', '^ff::Tag$', FFTagPrinter)
    return pp


objfile = gdb.current_objfile()
if objfile is not None:
    gdb.printing.register_pretty_printer(objfile, build_pretty_printer(), replace=True)
else:
    gdb.printing.register_pretty_printer(gdb, build_pretty_printer(), replace=True)
