#! /usr/bin/env python3
#
# Converts a prepared Inkscape SVG (all object must be converted to paths, the
# `title` metadata attribute must be a valid glyph name) to a FontForge glyph

import sys
import re
from xml.dom.minidom import parse
from collections import deque
from getopt import getopt
from os.path import join

ascent = None
em_size = None
output_dir = 'output.sfdir'
case_insensitive = False
dx = dy = 0
psnames_to_uni = {#{{{
             'space': 0x0020,
            'exclam': 0x0021,
          'quotedbl': 0x0022,
        'numbersign': 0x0023,
            'dollar': 0x0024,
           'percent': 0x0025,
         'ampersand': 0x0026,
        'quoteright': 0x2019,
         'parenleft': 0x0028,
        'parenright': 0x0029,
          'asterisk': 0x002A,
              'plus': 0x002B,
             'comma': 0x002C,
            'hyphen': 0x002D,
            'period': 0x002E,
             'slash': 0x002F,
              'zero': 0x0030,
               'one': 0x0031,
               'two': 0x0032,
             'three': 0x0033,
              'four': 0x0034,
              'five': 0x0035,
               'six': 0x0036,
             'seven': 0x0037,
             'eight': 0x0038,
              'nine': 0x0039,
             'colon': 0x003A,
         'semicolon': 0x003B,
              'less': 0x003C,
             'equal': 0x003D,
           'greater': 0x003E,
          'question': 0x003F,
                'at': 0x0040,
                 'A': 0x0041,
                 'B': 0x0042,
                 'C': 0x0043,
                 'D': 0x0044,
                 'E': 0x0045,
                 'F': 0x0046,
                 'G': 0x0047,
                 'H': 0x0048,
                 'I': 0x0049,
                 'J': 0x004A,
                 'K': 0x004B,
                 'L': 0x004C,
                 'M': 0x004D,
                 'N': 0x004E,
                 'O': 0x004F,
                 'P': 0x0050,
                 'Q': 0x0051,
                 'R': 0x0052,
                 'S': 0x0053,
                 'T': 0x0054,
                 'U': 0x0055,
                 'V': 0x0056,
                 'W': 0x0057,
                 'X': 0x0058,
                 'Y': 0x0059,
                 'Z': 0x005A,
       'bracketleft': 0x005B,
         'backslash': 0x005C,
      'bracketright': 0x005D,
       'asciicircum': 0x005E,
        'underscore': 0x005F,
         'quoteleft': 0x2018,
                 'a': 0x0061,
                 'b': 0x0062,
                 'c': 0x0063,
                 'd': 0x0064,
                 'e': 0x0065,
                 'f': 0x0066,
                 'g': 0x0067,
                 'h': 0x0068,
                 'i': 0x0069,
                 'j': 0x006A,
                 'k': 0x006B,
                 'l': 0x006C,
                 'm': 0x006D,
                 'n': 0x006E,
                 'o': 0x006F,
                 'p': 0x0070,
                 'q': 0x0071,
                 'r': 0x0072,
                 's': 0x0073,
                 't': 0x0074,
                 'u': 0x0075,
                 'v': 0x0076,
                 'w': 0x0077,
                 'x': 0x0078,
                 'y': 0x0079,
                 'z': 0x007A,
         'braceleft': 0x007B,
               'bar': 0x007C,
        'braceright': 0x007D,
        'asciitilde': 0x007E,
        'exclamdown': 0x00A1,
              'cent': 0x00A2,
          'sterling': 0x00A3,
               'yen': 0x00A5,
            'florin': 0x0192,
           'section': 0x00A7,
          'currency': 0x00A4,
       'quotesingle': 0x0027,
      'quotedblleft': 0x201C,
     'guillemotleft': 0x00AB,
     'guilsinglleft': 0x2039,
    'guilsinglright': 0x203A,
                'fi': 0xFB01,
                'fl': 0xFB02,
            'endash': 0x2013,
            'dagger': 0x2020,
         'daggerdbl': 0x2021,
    'periodcentered': 0x00B7,
         'paragraph': 0x00B6,
            'bullet': 0x2022,
    'quotesinglbase': 0x201A,
      'quotedblbase': 0x201E,
     'quotedblright': 0x201D,
    'guillemotright': 0x00BB,
          'ellipsis': 0x2026,
       'perthousand': 0x2030,
      'questiondown': 0x00BF,
             'grave': 0x0060,
             'acute': 0x00B4,
        'circumflex': 0x02C6,
             'tilde': 0x02DC,
            'macron': 0x00AF,
             'breve': 0x02D8,
         'dotaccent': 0x02D9,
          'dieresis': 0x00A8,
              'ring': 0x02DA,
           'cedilla': 0x00B8,
      'hungarumlaut': 0x02DD,
            'ogonek': 0x02DB,
             'caron': 0x02C7,
            'emdash': 0x2014,
                'AE': 0x00C6,
       'ordfeminine': 0x00AA,
            'Lslash': 0x0141,
            'Oslash': 0x00D8,
                'OE': 0x0152,
      'ordmasculine': 0x00BA,
                'ae': 0x00E6,
          'dotlessi': 0x0131,
            'lslash': 0x0142,
            'oslash': 0x00F8,
                'oe': 0x0153,
        'germandbls': 0x00DF}#}}}

class Point():
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def add(self, pt):
        self.x = self.x + pt.x
        self.y = self.y + pt.y

    def __str__(self):
        return '{}, {}'.format(self.x, self.y)

def putspline(cmd, indent, fobj, *lst):
    cmd = cmd.lower()
    while indent > 0:
        print(' ', end='', file=fobj)
        indent = indent - 1
    for pt in lst:
        print('{} {}'.format(round(pt.x + dx), round(ascent - pt.y - dy)), end=' ', file=fobj)
    print('{} {}'.format(cmd, 0 if cmd == 'c' else 1), file=fobj)

def getfloat(queue):
    return float(queue.popleft())

def getpoint(queue):
    return Point(getfloat(queue), getfloat(queue))

def handle_transform(elt):
    global dx, dy
    if 'transform' in elt.attributes:
        for pair in re.findall('translate\(([^,]+),([^)]+)\)', elt.attributes['transform'].value):
            mp = map(float, pair)
            dx = dx + next(mp, 0)
            dy = dy + next(mp, 0)

opts, args = getopt(sys.argv[1:], 'hia:e:o:', ['help', 'case-insensitive-fs', 'ascent=', 'em-size=', 'output-dir='])

for opt, val in opts:
    if opt in ('-a', '--ascent'):
        ascent = int(val)
    elif opt in ('-e', '--em-size'):
        em_size = int(val)
    elif opt in ('-o', '--output-dir'):
        output_dir = val
    elif opt in ('-i', '--case-insensitive-fs'):
        case_insensitive = True
    elif opt in ('-h', '--help'):
        print('svg2glyph [-h|-i|-a ASCENT|-e EM_SIZE|-o OUTPUT_DIR] SVG_FILE ...')
        sys.exit(0)

gid = 0

for fname in args:
    root = parse(fname)
    name = root.getElementsByTagName('title')[0].childNodes[0].data
    dx = dy = 0
    groups = root.getElementsByTagName('g')[0]
    handle_transform(groups)
    if name in psnames_to_uni:
        uni = psnames_to_uni[name]
    else:
        uni = int(re.match('^uni([a-f0-9]{4})$', name, re.I).groups()[0], 16)
    gname = name
    if case_insensitive and (re.match('^[A-Z]$', name) or re.match('^[AO]E$', name)):
        gname = '_{}'.format(name.lower())
    paths = groups.getElementsByTagName('path')
    glyph = join(output_dir, '{}.glyph'.format(gname))
    dest = open(glyph, 'w')
    print('{} -> {}'.format(fname, glyph))
    print('StartChar: {}'.format(name), file=dest)
    print('Encoding: {} {} {}'.format(uni, uni, gid), file=dest)
    print('Width: {}'.format(em_size), file=dest)
    print('SplineSet', file=dest)
    cmd = last_cmd = None
    for pth in paths:
        handle_transform(pth)
        pos = init_pos = None
        data = pth.attributes['d'].value
        queue = deque(re.findall('([mlhvcsqtaz]|-?\d+(?:\.\d+)?(?:e-?\d+)?)', data, re.I))
        indent = 0
        while len(queue) > 0:
            if cmd == None or re.match('[mlhvcsqtaz]', queue[0], re.I):
                last_cmd = cmd
                cmd = queue.popleft()
            if cmd == 'M' or cmd == 'm':
                pt = getpoint(queue)
                if pos == None:
                    init_pos = pos = pt
                elif cmd == 'M':
                    pos = pt
                elif cmd == 'm':
                    pos.add(pt)
                if last_cmd == 'Z' or last_cmd == 'z':
                    init_pos = pos
                putspline(cmd, indent, dest, pos)
                cmd = chr(ord(cmd) - 1)
                indent = 1
            elif cmd == 'L' or cmd == 'l':
                pt = getpoint(queue)
                if cmd == 'l':
                    pt.add(pos)
                putspline(cmd, indent, dest, pt)
                pos = pt
            elif cmd == 'C' or cmd == 'c':
                p1 = getpoint(queue)
                p2 = getpoint(queue)
                pt = getpoint(queue)
                if cmd == 'c':
                    p1.add(pos)
                    p2.add(pos)
                    pt.add(pos)
                putspline(cmd, indent, dest, p1, p2, pt)
                pos = pt
            elif cmd == 'Z' or cmd == 'z':
                putspline('l', indent, dest, init_pos)
                pos = init_pos
                indent = 0
            else:
                print("Unsupported instruction: '{}'.".format(cmd))
                cmd = queue.popleft()
    print('EndSplineSet', file=dest)
    print('EndChar', file=dest)
    dest.close()
    gid = gid + 1
