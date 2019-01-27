# -*- coding: utf-8 -*-
#Needs: fonts/DejaVuSerif.sfd

import sys, fontforge

plain = 'Comment without em-dash'
uni = u'Comment with — em dash'

f=fontforge.open(sys.argv[1])

g=f['dcroat']
if g.comment != "":
    raise ValueError("Expected comment to start as empty string")

g.comment = plain
if g.comment != plain:
    raise ValueError("Unable to assign and read plain ASCII comment")

g.comment = uni
if g.comment != uni:
    raise ValueError("Unable to assign and read plain ASCII comment")

lname = '\'=>\' Single Substitution'

# These are each string conversion tests
flags = ('right_to_left', 'ignore_bases')
fealist = (("liga",(("latn",("dflt")),)),)

f.addLookup(lname, 'gsub_single', flags, fealist)
f.addLookupSubtable(lname, lname)

g.addPosSub(lname,'LIG')
tup = g.getPosSub(lname)
if not (tup[0][0] == lname and tup[0][2] == 'LIG'):
    raise ValueError("Unable to assign and read plain ASCII PosSub")

dcroat_tti = b"@2\x1e\x04\x10\x1d\x03!\x00\x19\x13z\x1b\x11!\x94\r'\x94\x07\x15z\x17q\x00z\x07`\r\x8c\x02\x12$\x16,\x1d\x14\x10\x03\x03'\x1a\x01,\x18\x005$\x1a\n**\x10\xf4\xec\xf4<\xe42\xfc\x17<\xe4\x1191\x00/\xec\xe4\xec\xfc\xec\x10\xee\x10\xee\xd5<\xee2\x11\x12\x17990@\t\x00+\x10+\x7f+\xb0+\x04\x01]"

if g.ttinstrs != dcroat_tti:
    raise ValueError("Unexpected ttinstrs in glyph dcroat")

tbaseline = (("hang","ideo","romn"),
             (("cyrl","romn",(1405,-288,0),()),
              ("grek","romn",(1405,-288,0),()),
              ("latn","romn",(1405,-288,0),
               (("dflt",-576,1913,
                  (("NoAc",-576,1482), ("ENG ",-576,1482))
               ),)
              )
             )
            )

if f.horizontalBaseline != None:
    raise ValueError("Unexpected horizontalBaseline")
f.horizontalBaseline = tbaseline
if f.horizontalBaseline != tbaseline:
    raise ValueError("Unable to assign and read font baseline")

vrs = (f['a'], f['b'], f['c'])
#vrs = ('a', 'b', 'c')
if g.verticalVariants != None:
    raise ValueError("Unexpected verticalVariants")
g.verticalVariants = vrs
if g.verticalVariants != 'a b c':
    raise ValueError("Unable to assign and read glyph verticalVariants")
