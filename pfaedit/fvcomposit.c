/* Copyright (C) 2000-2002 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "pfaeditui.h"
#include <chardata.h>
#include <math.h>
#include <utype.h>
#include <ustring.h>

#define BottomAccent	0x300
#define TopAccent	0x345

/* for accents between 0x300 and 345 these are some synonyms */
/* postscript wants accented chars built with accents in the 0x2c? range */
/*  except for grave and acute which live in iso8859-1 range */
/*  this table is ordered on a best try basis */
static const unichar_t accents[][3] = {
    { 0x60, 0x2cb },		/* grave */
    { 0xb4, 0x2ca },		/* acute */
    { 0x2c6, 0x5e },		/* circumflex */
    { 0x2dc, 0x7e },		/* tilde */
    { 0xaf, 0x2c9 },		/* macron */
    { 0x305, 0xaf },		/* overline, (macron is suggested as a syn, but it's not quite right) */
    { 0x2d8 },			/* breve */
    { 0x2d9, '.' },		/* dot above */
    { 0xa8 },			/* diaeresis */
    { 0x2c0 },			/* hook above */
    { 0x2da, 0xb0 },		/* ring above */
    { 0x2dd },			/* real acute */
    { 0x2c7 },			/* caron */
    { 0x2c8, 0x384, '\''  },	/* vertical line, tonos */
    { '"' },			/* real vertical line */
    { 0 },			/* real grave */
    { 0 },			/* cand... */		/* 310 */
    { 0 },			/* inverted breve */
    { 0x2bb },			/* turned comma */
    { 0x2bc, ',' },		/* comma above */
    { 0x2bd },			/* reversed comma */
    { 0x2bc, ',' },		/* comma above right */
    { 0x60, 0x2cb },		/* grave below */
    { 0xb4, 0x2ca },		/* acute below */
    { 0 },			/* left tack */
    { 0 },			/* right tack */
    { 0 },			/* left angle */
    { 0 },			/* horn, sometimes comma but only if nothing better */
    { 0 },			/* half ring */
    { 0x2d4 },			/* up tack */
    { 0x2d5 },			/* down tack */
    { 0x2d6, '+' },		/* plus below */
    { 0x2d7, '-' },		/* minus below */	/* 320 */
    { 0x2b2 },			/* hook */
    { 0 },			/* back hook */
    { 0x2d9, '.' },		/* dot below */
    { 0xa8 },			/* diaeresis below */
    { 0x2da, 0xb0 },		/* ring below */
    { 0x2bc, ',' },		/* comma below */
    { 0xb8 },			/* cedilla */
    { 0x2db },			/* ogonek */		/* 0x328 */
    { 0x2c8, 0x384, '\''  },	/* vertical line below */
    { 0 },			/* bridge below */
    { 0 },			/* real arch below */
    { 0x2c7 },			/* caron below */
    { 0x2c6, 0x52 },		/* circumflex below */
    { 0x2d8 },			/* breve below */
    { 0 },			/* inverted breve below */
    { 0x2dc, 0x7e },		/* tilde below */	/* 0x330 */
    { 0xaf, 0x2c9 },		/* macron below */
    { '_' },			/* low line */
    { 0 },			/* real low line */
    { 0x2dc, 0x7e },		/* tilde overstrike */
    { '-' },			/* line overstrike */
    { '_' },			/* long line overstrike */
    { '/' },			/* short solidus overstrike */
    { '/' },			/* long solidus overstrike */	/* 0x338 */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0x60, 0x2cb },		/* tone mark, left of circumflex */ /* 0x340 */
    { 0xb4, 0x2ca },		/* tone mark, right of circumflex */
    { 0x2dc, 0x7e },		/* perispomeni (tilde) */
    { 0x2bc, ',' },		/* koronis */
    { 0 },			/* dialytika tonos (two accents) */
    { 0x37a },			/* ypogegrammeni */
    { 0xffff }
};

/* greek accents */
/* because of accent unification I can't use the same glyphs for greek and */
/*  latin accents. Annoying. So when unicode decomposes a greek letter to */
/*  use 0x0301 we actually want 0x1ffd and so on */
static unichar_t unicode_greekalts[256][3] = {
/* 1F00 */ { 0x03B1, 0x1FBD, 0 },
/* 1F01 */ { 0x03B1, 0x1FFE, 0 },
/* 1F02 */ { 0x03B1, 0x1FCD, 0 },
/* 1F03 */ { 0x03B1, 0x1FDD, 0 },
/* 1F04 */ { 0x03B1, 0x1FCE, 0 },
/* 1F05 */ { 0x03B1, 0x1FDE, 0 },
/* 1F06 */ { 0x03B1, 0x1FCF, 0 },
/* 1F07 */ { 0x03B1, 0x1FDF, 0 },
/* 1F08 */ { 0x0391, 0x1FBD, 0 },
/* 1F09 */ { 0x0391, 0x1FFE, 0 },
/* 1F0A */ { 0x0391, 0x1FCD, 0 },
/* 1F0B */ { 0x0391, 0x1FDD, 0 },
/* 1F0C */ { 0x0391, 0x1FCE, 0 },
/* 1F0D */ { 0x0391, 0x1FDE, 0 },
/* 1F0E */ { 0x0391, 0x1FCF, 0 },
/* 1F0F */ { 0x0391, 0x1FDF, 0 },
/* 1F10 */ { 0x03B5, 0x1FBD, 0 },
/* 1F11 */ { 0x03B5, 0x1FFE, 0 },
/* 1F12 */ { 0x03B5, 0x1FCD, 0 },
/* 1F13 */ { 0x03B5, 0x1FDD, 0 },
/* 1F14 */ { 0x03B5, 0x1FCE, 0 },
/* 1F15 */ { 0x03B5, 0x1FDE, 0 },
/* 1F16 */ { 0 },
/* 1F17 */ { 0 },
/* 1F18 */ { 0x0395, 0x1FBD, 0 },
/* 1F19 */ { 0x0395, 0x1FFE, 0 },
/* 1F1A */ { 0x0395, 0x1FCD, 0 },
/* 1F1B */ { 0x0395, 0x1FDD, 0 },
/* 1F1C */ { 0x0395, 0x1FCE, 0 },
/* 1F1D */ { 0x0395, 0x1FDE, 0 },
/* 1F1E */ { 0 },
/* 1F1F */ { 0 },
/* 1F20 */ { 0x03B7, 0x1FBD, 0 },
/* 1F21 */ { 0x03B7, 0x1FFE, 0 },
/* 1F22 */ { 0x03B7, 0x1FCD, 0 },
/* 1F23 */ { 0x03B7, 0x1FDD, 0 },
/* 1F24 */ { 0x03B7, 0x1FCE, 0 },
/* 1F25 */ { 0x03B7, 0x1FDE, 0 },
/* 1F26 */ { 0x03B7, 0x1FCF, 0 },
/* 1F27 */ { 0x03B7, 0x1FDF, 0 },
/* 1F28 */ { 0x0397, 0x1FBD, 0 },
/* 1F29 */ { 0x0397, 0x1FFE, 0 },
/* 1F2A */ { 0x0397, 0x1FCD, 0 },
/* 1F2B */ { 0x0397, 0x1FDD, 0 },
/* 1F2C */ { 0x0397, 0x1FCE, 0 },
/* 1F2D */ { 0x0397, 0x1FDE, 0 },
/* 1F2E */ { 0x0397, 0x1FCF, 0 },
/* 1F2F */ { 0x0397, 0x1FDF, 0 },
/* 1F30 */ { 0x03B9, 0x1FBD, 0 },
/* 1F31 */ { 0x03B9, 0x1FFE, 0 },
/* 1F32 */ { 0x03B9, 0x1FCD, 0 },
/* 1F33 */ { 0x03B9, 0x1FDD, 0 },
/* 1F34 */ { 0x03B9, 0x1FCE, 0 },
/* 1F35 */ { 0x03B9, 0x1FDE, 0 },
/* 1F36 */ { 0x03B9, 0x1FCF, 0 },
/* 1F37 */ { 0x03B9, 0x1FDF, 0 },
/* 1F38 */ { 0x0399, 0x1FBD, 0 },
/* 1F39 */ { 0x0399, 0x1FFE, 0 },
/* 1F3A */ { 0x0399, 0x1FCD, 0 },
/* 1F3B */ { 0x0399, 0x1FDD, 0 },
/* 1F3C */ { 0x0399, 0x1FCE, 0 },
/* 1F3D */ { 0x0399, 0x1FDE, 0 },
/* 1F3E */ { 0x0399, 0x1FCF, 0 },
/* 1F3F */ { 0x0399, 0x1FDF, 0 },
/* 1F40 */ { 0x03BF, 0x1FBD, 0 },
/* 1F41 */ { 0x03BF, 0x1FFE, 0 },
/* 1F42 */ { 0x03BF, 0x1FCD, 0 },
/* 1F43 */ { 0x03BF, 0x1FDD, 0 },
/* 1F44 */ { 0x03BF, 0x1FCE, 0 },
/* 1F45 */ { 0x03BF, 0x1FDE, 0 },
/* 1F46 */ { 0 },
/* 1F47 */ { 0 },
/* 1F48 */ { 0x039F, 0x1FBD, 0 },
/* 1F49 */ { 0x039F, 0x1FFE, 0 },
/* 1F4A */ { 0x039F, 0x1FCD, 0 },
/* 1F4B */ { 0x039F, 0x1FDD, 0 },
/* 1F4C */ { 0x039F, 0x1FCE, 0 },
/* 1F4D */ { 0x039F, 0x1FDE, 0 },
/* 1F4E */ { 0 },
/* 1F4F */ { 0 },
/* 1F50 */ { 0x03C5, 0x1FBD, 0 },
/* 1F51 */ { 0x03C5, 0x1FFE, 0 },
/* 1F52 */ { 0x03C5, 0x1FCD, 0 },
/* 1F53 */ { 0x03C5, 0x1FDD, 0 },
/* 1F54 */ { 0x03C5, 0x1FCE, 0 },
/* 1F55 */ { 0x03C5, 0x1FDE, 0 },
/* 1F56 */ { 0x03C5, 0x1FCF, 0 },
/* 1F57 */ { 0x03C5, 0x1FDF, 0 },
/* 1F58 */ { 0 },
/* 1F59 */ { 0x03D2, 0x1FFE, 0 },
/* 1F5A */ { 0 },
/* 1F5B */ { 0x03D2, 0x1FDD, 0 },
/* 1F5C */ { 0 },
/* 1F5D */ { 0x03D2, 0x1FDE, 0 },
/* 1F5E */ { 0 },
/* 1F5F */ { 0x03D2, 0x1FDF, 0 },
/* 1F60 */ { 0x03C9, 0x1FBD, 0 },
/* 1F61 */ { 0x03C9, 0x1FFE, 0 },
/* 1F62 */ { 0x03C9, 0x1FCD, 0 },
/* 1F63 */ { 0x03C9, 0x1FDD, 0 },
/* 1F64 */ { 0x03C9, 0x1FCE, 0 },
/* 1F65 */ { 0x03C9, 0x1FDE, 0 },
/* 1F66 */ { 0x03C9, 0x1FCF, 0 },
/* 1F67 */ { 0x03C9, 0x1FDF, 0 },
/* 1F68 */ { 0x03A9, 0x1FBD, 0 },
/* 1F69 */ { 0x03A9, 0x1FFE, 0 },
/* 1F6A */ { 0x03A9, 0x1FCD, 0 },
/* 1F6B */ { 0x03A9, 0x1FDD, 0 },
/* 1F6C */ { 0x03A9, 0x1FCE, 0 },
/* 1F6D */ { 0x03A9, 0x1FDE, 0 },
/* 1F6E */ { 0x03A9, 0x1FCF, 0 },
/* 1F6F */ { 0x03A9, 0x1FDF, 0 },
/* 1F70 */ { 0x03B1, 0x1FEF, 0 },
/* 1F71 */ { 0x03B1, 0x1FFD, 0 },
/* 1F72 */ { 0x03B5, 0x1FEF, 0 },
/* 1F73 */ { 0x03B5, 0x1FFD, 0 },
/* 1F74 */ { 0x03B7, 0x1FEF, 0 },
/* 1F75 */ { 0x03B7, 0x1FFD, 0 },
/* 1F76 */ { 0x03B9, 0x1FEF, 0 },
/* 1F77 */ { 0x03B9, 0x1FFD, 0 },
/* 1F78 */ { 0x03BF, 0x1FEF, 0 },
/* 1F79 */ { 0x03BF, 0x1FFD, 0 },
/* 1F7A */ { 0x03C5, 0x1FEF, 0 },
/* 1F7B */ { 0x03C5, 0x1FFD, 0 },
/* 1F7C */ { 0x03C9, 0x1FEF, 0 },
/* 1F7D */ { 0x03C9, 0x1FFD, 0 },
/* 1F7E */ { 0 },
/* 1F7F */ { 0 },
/* 1F80 */ { 0x1F00, 0x0345, 0 },
/* 1F81 */ { 0x1F01, 0x0345, 0 },
/* 1F82 */ { 0x1F02, 0x0345, 0 },
/* 1F83 */ { 0x1F03, 0x0345, 0 },
/* 1F84 */ { 0x1F04, 0x0345, 0 },
/* 1F85 */ { 0x1F05, 0x0345, 0 },
/* 1F86 */ { 0x1F06, 0x0345, 0 },
/* 1F87 */ { 0x1F07, 0x0345, 0 },
/* 1F88 */ { 0x1F08, 0x1FBE, 0 },
/* 1F89 */ { 0x1F09, 0x1FBE, 0 },
/* 1F8A */ { 0x1F0A, 0x1FBE, 0 },
/* 1F8B */ { 0x1F0B, 0x1FBE, 0 },
/* 1F8C */ { 0x1F0C, 0x1FBE, 0 },
/* 1F8D */ { 0x1F0D, 0x1FBE, 0 },
/* 1F8E */ { 0x1F0E, 0x1FBE, 0 },
/* 1F8F */ { 0x1F0F, 0x1FBE, 0 },
/* 1F90 */ { 0x1F20, 0x0345, 0 },
/* 1F91 */ { 0x1F21, 0x0345, 0 },
/* 1F92 */ { 0x1F22, 0x0345, 0 },
/* 1F93 */ { 0x1F23, 0x0345, 0 },
/* 1F94 */ { 0x1F24, 0x0345, 0 },
/* 1F95 */ { 0x1F25, 0x0345, 0 },
/* 1F96 */ { 0x1F26, 0x0345, 0 },
/* 1F97 */ { 0x1F27, 0x0345, 0 },
/* 1F98 */ { 0x1F28, 0x1FBE, 0 },
/* 1F99 */ { 0x1F29, 0x1FBE, 0 },
/* 1F9A */ { 0x1F2A, 0x1FBE, 0 },
/* 1F9B */ { 0x1F2B, 0x1FBE, 0 },
/* 1F9C */ { 0x1F2C, 0x1FBE, 0 },
/* 1F9D */ { 0x1F2D, 0x1FBE, 0 },
/* 1F9E */ { 0x1F2E, 0x1FBE, 0 },
/* 1F9F */ { 0x1F2F, 0x1FBE, 0 },
/* 1FA0 */ { 0x1F60, 0x0345, 0 },
/* 1FA1 */ { 0x1F61, 0x0345, 0 },
/* 1FA2 */ { 0x1F62, 0x0345, 0 },
/* 1FA3 */ { 0x1F63, 0x0345, 0 },
/* 1FA4 */ { 0x1F64, 0x0345, 0 },
/* 1FA5 */ { 0x1F65, 0x0345, 0 },
/* 1FA6 */ { 0x1F66, 0x0345, 0 },
/* 1FA7 */ { 0x1F67, 0x0345, 0 },
/* 1FA8 */ { 0x1F68, 0x1FBE, 0 },
/* 1FA9 */ { 0x1F69, 0x1FBE, 0 },
/* 1FAA */ { 0x1F6A, 0x1FBE, 0 },
/* 1FAB */ { 0x1F6B, 0x1FBE, 0 },
/* 1FAC */ { 0x1F6C, 0x1FBE, 0 },
/* 1FAD */ { 0x1F6D, 0x1FBE, 0 },
/* 1FAE */ { 0x1F6E, 0x1FBE, 0 },
/* 1FAF */ { 0x1F6F, 0x1FBE, 0 },
/* 1FB0 */ { 0x03B1, 0x0306, 0 },
/* 1FB1 */ { 0x03B1, 0x0304, 0 },
/* 1FB2 */ { 0x1F70, 0x0345, 0 },
/* 1FB3 */ { 0x03B1, 0x0345, 0 },
/* 1FB4 */ { 0x03AC, 0x0345, 0 },
/* 1FB5 */ { 0 },
/* 1FB6 */ { 0x03B1, 0x1FC0, 0 },
/* 1FB7 */ { 0x1FB6, 0x0345, 0 },
/* 1FB8 */ { 0x0391, 0x0306, 0 },
/* 1FB9 */ { 0x0391, 0x0304, 0 },
/* 1FBA */ { 0x0391, 0x1FEF, 0 },
/* 1FBB */ { 0x0391, 0x1FFD, 0 },
/* 1FBC */ { 0x0391, 0x1FBE, 0 },
/* 1FBD */ { 0x0020, 0x0313, 0 },
/* 1FBE */ { 0x0020, 0x0345, 0 },
/* 1FBF */ { 0x0020, 0x0313, 0 },
/* 1FC0 */ { 0x0020, 0x0342, 0 },
/* 1FC1 */ { 0x00A8, 0x1FC0, 0 },
/* 1FC2 */ { 0x1F74, 0x0345, 0 },
/* 1FC3 */ { 0x03B7, 0x0345, 0 },
/* 1FC4 */ { 0x03AE, 0x0345, 0 },
/* 1FC5 */ { 0 },
/* 1FC6 */ { 0x03B7, 0x1FC0, 0 },
/* 1FC7 */ { 0x1FC6, 0x0345, 0 },
/* 1FC8 */ { 0x0395, 0x1FEF, 0 },
/* 1FC9 */ { 0x0395, 0x1FFD, 0 },
/* 1FCA */ { 0x0397, 0x1FEF, 0 },
/* 1FCB */ { 0x0397, 0x1FFD, 0 },
/* 1FCC */ { 0x0397, 0x1FBE, 0 },
/* 1FCD */ { 0x1FBF, 0x1FEF, 0 },
/* 1FCE */ { 0x1FBF, 0x1FFD, 0 },
/* 1FCF */ { 0x1FBF, 0x1FC0, 0 },
/* 1FD0 */ { 0x03B9, 0x0306, 0 },
/* 1FD1 */ { 0x03B9, 0x0304, 0 },
/* 1FD2 */ { 0x03B9, 0x1FED, 0 },
/* 1FD3 */ { 0x03B9, 0x1FEE, 0 },
/* 1FD4 */ { 0 },
/* 1FD5 */ { 0 },
/* 1FD6 */ { 0x03B9, 0x1FC0, 0 },
/* 1FD7 */ { 0x03B9, 0x1FC1, 0 },
/* 1FD8 */ { 0x0399, 0x0306, 0 },
/* 1FD9 */ { 0x0399, 0x0304, 0 },
/* 1FDA */ { 0x0399, 0x1FEF, 0 },
/* 1FDB */ { 0x0399, 0x1FFD, 0 },
/* 1FDC */ { 0 },
/* 1FDD */ { 0x1FFE, 0x1FEF, 0 },
/* 1FDE */ { 0x1FFE, 0x1FFD, 0 },
/* 1FDF */ { 0x1FFE, 0x1FC0, 0 },
/* 1FE0 */ { 0x03C5, 0x0306, 0 },
/* 1FE1 */ { 0x03C5, 0x0304, 0 },
/* 1FE2 */ { 0x03C5, 0x1FED, 0 },
/* 1FE3 */ { 0x03C5, 0x1FEE, 0 },
/* 1FE4 */ { 0x03C1, 0x1FBF, 0 },
/* 1FE5 */ { 0x03C1, 0x1FFE, 0 },
/* 1FE6 */ { 0x03C5, 0x1FC0, 0 },
/* 1FE7 */ { 0x03C5, 0x1FC1, 0 },
/* 1FE8 */ { 0x03A5, 0x0306, 0 },
/* 1FE9 */ { 0x03A5, 0x0304, 0 },
/* 1FEA */ { 0x03A5, 0x1FEF, 0 },
/* 1FEB */ { 0x03A5, 0x1FFD, 0 },
/* 1FEC */ { 0x03A1, 0x1FFE, 0 },
/* 1FED */ { 0x00A8, 0x1FEF, 0 },
/* 1FEE */ { 0x0385, 0 },
/* 1FEF */ { 0x0060, 0 },
/* 1FF0 */ { 0 },
/* 1FF1 */ { 0 },
/* 1FF2 */ { 0x1F7C, 0x0345, 0 },
/* 1FF3 */ { 0x03C9, 0x0345, 0 },
/* 1FF4 */ { 0x03CE, 0x0345, 0 },
/* 1FF5 */ { 0 },
/* 1FF6 */ { 0x03C9, 0x1FC0, 0 },
/* 1FF7 */ { 0x1FF6, 0x0345, 0 },
/* 1FF8 */ { 0x039F, 0x1FEF, 0 },
/* 1FF9 */ { 0x039F, 0x1FFD, 0 },
/* 1FFA */ { 0x03A9, 0x1FEF, 0 },
/* 1FFB */ { 0x03A9, 0x1FFD, 0 },
/* 1FFC */ { 0x03A9, 0x1FBE, 0 },
/* 1FFD */ { 0x00B4, 0 },
/* 1FFE */ { 0x0020, 0x0314, 0 },
/* 1FFF */ { 0 },
};

static int haschar(SplineFont *sf,int ch) {
    int i;

    if ( sf->encoding_name==em_unicode || (ch<0x100 && sf->encoding_name==em_iso8859_1))
return( ch<sf->charcnt && sf->chars[ch]!=NULL &&
	(sf->chars[ch]->splines!=NULL || sf->chars[ch]->refs!=NULL || sf->chars[ch]->widthset) );

    for ( i=sf->charcnt-1; i>=0; --i ) if ( sf->chars[i]!=NULL )
	if ( sf->chars[i]->unicodeenc == ch )
    break;
return( i!=-1 && (sf->chars[i]->splines!=NULL || sf->chars[i]->refs!=NULL || (ch==0x20 && sf->chars[i]->widthset)) );
}

static SplineChar *findchar(SplineFont *sf,int ch) {
    int i;

    if ( sf->encoding_name==em_unicode || (ch<0x100 && sf->encoding_name==em_iso8859_1))
return( sf->chars[ch] );

    for ( i=sf->charcnt-1; i>=0; --i ) if ( sf->chars[i]!=NULL )
	if ( sf->chars[i]->unicodeenc == ch )
    break;
    if ( i<0 )
return( NULL );

return( sf->chars[i] );
}

static unichar_t *SFAlternateFromLigature(SplineFont *sf, int base) {
    static unichar_t space[30];
    unichar_t *spt, *send = space+sizeof(space)/sizeof(space[0])-1;
    SplineChar *sc;
    char *ligstart, *semi, *pt, sch, ch;
    int j;

    if ( base<0 || base>=sf->charcnt || (sc=sf->chars[base])==NULL ||
	    sc->lig==NULL )
return( NULL );

    ligstart=sc->lig->components;
    semi = strchr(ligstart,';');
    if ( semi==NULL ) semi = ligstart+strlen(ligstart);
    sch = *semi; *semi = '\0';
    spt = space;
    for ( pt = ligstart; *pt!='\0'; ) {
	char *start = pt;
	for ( ; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	for ( j=0; j<sf->charcnt; ++j )
	    if ( sf->chars[j]!=NULL && strcmp(sf->chars[j]->name,start)==0 )
	break;
	*pt = ch;
	if ( j>=sf->charcnt || sf->chars[j]->unicodeenc==-1 || spt>=send ) {
	    *semi = sch;
return( NULL );
	}
	*spt++ = sf->chars[j]->unicodeenc;
	if ( ch!='\0' ) ++pt;
    }
    *spt ='\0';
    *semi = sch;
return( space );
}

const unichar_t *SFGetAlternate(SplineFont *sf, int base) {
    static unichar_t greekalts[5];
    const unichar_t *upt, *pt; unichar_t *gpt;

    if ( base==-1 )
return( NULL );
    if ( base>=0xac00 && base<=0xd7a3 ) { /* Hangul syllables */
	greekalts[0] = (base-0xac00)/(21*28) + 0x1100;
	greekalts[1] = ((base-0xac00)/28)%21 + 0x1161;
	if ( (base-0xac00)%28==0 )
	    greekalts[2] = 0;
	else
	    greekalts[2] = (base-0xac00)%28 -1 + 0x11a8;
	greekalts[3] = 0;
return( greekalts );
    }
    if ( unicode_alternates[base>>8]==NULL ||
	    (upt = unicode_alternates[base>>8][base&0xff])==NULL )
return( SFAlternateFromLigature(sf,base));

	    /* The definitions of some of the greek letters may make some */
	    /*  linguistic sense, but I can't use it to place the accents */
	    /*  properly. So I redefine them here */
    if ( base>=0x1f00 && base<0x2000 ) {
	gpt = unicode_greekalts[base-0x1f00];
	if ( *gpt && haschar(sf,*gpt) && (gpt[1]=='\0' || haschar(sf,gpt[1])) )
return( gpt );
    } else if ( base>=0x380 && base<=0x3ff && upt!=NULL ) {
	/* Use precombined accents when possible */
	if ( base==0x390 || base==0x3b0 ) {
	    greekalts[0] = base==0x390?0x3b1:0x3c5;
	    greekalts[1] = 0x385;
	    greekalts[2] = '\0';
	    if ( haschar(sf,greekalts[1]))
return( greekalts );
	}
	/* In version 3 of unicode tonos gets converted to accute, which it */
	/*  doesn't look like. Convert it back */
	for ( pt=upt ; *pt && *pt!=0x301; ++pt );
	if ( *pt ) {
	    for ( gpt=greekalts ; *upt ; ++upt ) {
		if ( *upt==0x301 )
		    *gpt++ = 0x30d;
		else
		    *gpt++ = *upt;
	    }
return( greekalts );
	}
    }
return( upt );
}

int SFIsCompositBuildable(SplineFont *sf,int unicodeenc) {
    const unichar_t *pt, *apt, *end; unichar_t ch;
    SplineChar *one, *two;

    if (( pt = SFGetAlternate(sf,unicodeenc))==NULL )
return( false );

    one=findchar(sf,unicodeenc);
    
    for ( ; *pt; ++pt ) {
	ch = *pt;
	if ( !haschar(sf,ch)) {
	    if ( ch>=BottomAccent && ch<=TopAccent ) {
		apt = accents[ch-BottomAccent]; end = apt+3;
		while ( apt<end && *apt && !haschar(sf,*apt)) ++apt;
		if ( apt==end || *apt=='\0' ) {
		    /* check for caron */
		    /*  and for inverted breve */
		    if ( (ch == 0x30c || ch == 0x32c ) &&
			    (haschar(sf,0x302) || haschar(sf,0x2c6) || haschar(sf,'^')) )
			/* It's ok */;
		    if ( ch==0x31b && haschar(sf,','))
			ch = ',';
		    else if ( (ch != 0x32f && ch != 0x311 ) || !haschar(sf,0x2d8))
return( false );
		    else
			ch = 0x2d8;
		} else
		    ch = *apt;
	    } else
return( false );
	}
	/* No recursive references */
	/* Cyrillic gamma could refer to Greek gamma, which the entry gives as an alternate */
	if ( one!=NULL && (two=findchar(sf,ch))!=NULL && SCDependsOnSC(two,one))
return( false );
    }
return( true );
}

int SFIsRotatable(SplineFont *sf,SplineChar *sc) {
    if ( sf->cidmaster!=NULL && strncmp(sc->name,"vertcid_",8)==0 ) {
	char *end;
	int cid = strtol(sc->name+8,&end,10);
	if ( *end=='\0' && SFHasCID(sf,cid)!=-1)
return( true );
    } else if ( strncmp(sc->name,"vertuni",7)==0 && strlen(sc->name)==11 ) {
	char *end;
	int uni = strtol(sc->name+7,&end,16);
	if ( *end=='\0' && SFCIDFindExistingChar(sf,uni,NULL)!=-1 )
return( true );
    }
return( false );
}

int SFIsSomethingBuildable(SplineFont *sf,SplineChar *sc) {
    int unicodeenc = sc->unicodeenc;

    if ( iszerowidth(unicodeenc) ||
	    (unicodeenc>=0x2000 && unicodeenc<=0x2015 ))
return( true );

    if ( SFIsCompositBuildable(sf,unicodeenc))
return( true );

return( SFIsRotatable(sf,sc));
}

static int SPInRange(SplinePoint *sp, real ymin, real ymax) {
    if ( sp->me.y>=ymin && sp->me.y<=ymax )
return( true );
    if ( sp->prev!=NULL )
	if ( sp->prev->from->me.y>=ymin && sp->prev->from->me.y<=ymax )
return( true );
    if ( sp->next!=NULL )
	if ( sp->next->to->me.y>=ymin && sp->next->to->me.y<=ymax )
return( true );
return( false );
}

static void _SplineSetFindXRange(SplinePointList *spl, DBounds *bounds,
	real ymin, real ymax, real ia) {
    Spline *spline;
    real xadjust, tia = tan(ia);

    for ( ; spl!=NULL; spl = spl->next ) {
	if ( SPInRange(spl->first,ymin,ymax) ) {
	    xadjust = spl->first->me.x + tia*(spl->first->me.y-ymin);
	    if ( bounds->minx==0 && bounds->maxx==0 ) {
		bounds->minx = bounds->maxx = xadjust;
	    } else {
		if ( xadjust<bounds->minx ) bounds->minx = xadjust;
		if ( xadjust>bounds->maxx ) bounds->maxx = xadjust;
	    }
	}
	for ( spline = spl->first->next; spline!=NULL && spline->to!=spl->first; spline=spline->to->next ) {
	    if ( SPInRange(spline->to,ymin,ymax) ) {
	    xadjust = spline->to->me.x + tia*(spline->to->me.y-ymin);
		if ( bounds->minx==0 && bounds->maxx==0 ) {
		    bounds->minx = bounds->maxx = xadjust;
		} else {
		    if ( xadjust<bounds->minx ) bounds->minx = xadjust;
		    if ( xadjust>bounds->maxx ) bounds->maxx = xadjust;
		}
	    }
	}
    }
}

/* this is called by the accented character routines with bounds set for the */
/*  entire character. Our job is to make a guess at what the top of the */
/*  character looks like so that we can do an optical accent placement */
/* I tried having "top" be the top tenth of the character but that was too small */
static real SCFindTopXRange(SplineChar *sc,DBounds *bounds, real ia) {
    RefChar *rf;
    int ymax = bounds->maxy+1, ymin = ymax-(bounds->maxy-bounds->miny)/4;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;

    for ( rf=sc->refs; rf!=NULL; rf = rf->next )
	_SplineSetFindXRange(rf->splines,bounds,ymin,ymax,ia);

    _SplineSetFindXRange(sc->splines,bounds,ymin,ymax,ia);
return( ymin );
}

static real SCFindBottomXRange(SplineChar *sc,DBounds *bounds, real ia) {
    RefChar *rf;
    int ymin = bounds->miny-1, ymax = ymin+(bounds->maxy-bounds->miny)/4;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;

    for ( rf=sc->refs; rf!=NULL; rf = rf->next )
	_SplineSetFindXRange(rf->splines,bounds,ymin,ymax,ia);

    _SplineSetFindXRange(sc->splines,bounds,ymin,ymax,ia);
return( ymin );
}

/* the cedilla and ogonec accents do not center on the accent itself but on */
/*  the small part of it that joins at the top */
static real SCFindTopBounds(SplineChar *sc,DBounds *bounds, real ia) {
    RefChar *rf;
    int ymax = bounds->maxy+1, ymin = ymax-(bounds->maxy-bounds->miny)/20;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;

    for ( rf=sc->refs; rf!=NULL; rf = rf->next )
	_SplineSetFindXRange(rf->splines,bounds,ymin,ymax,ia);

    _SplineSetFindXRange(sc->splines,bounds,ymin,ymax,ia);
return( ymin );
}

/* And similarly for the floating hook */
static real SCFindBottomBounds(SplineChar *sc,DBounds *bounds, real ia) {
    RefChar *rf;
    int ymin = bounds->miny-1, ymax = ymin+(bounds->maxy-bounds->miny)/20;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;

    for ( rf=sc->refs; rf!=NULL; rf = rf->next )
	_SplineSetFindXRange(rf->splines,bounds,ymin,ymax,ia);

    _SplineSetFindXRange(sc->splines,bounds,ymin,ymax,ia);
return( ymin );
}

static real SplineCharFindSlantedBounds(SplineChar *sc,DBounds *bounds, real ia) {
    int ymin, ymax;
    RefChar *rf;
    BlueData bd;

    SplineCharFindBounds(sc,bounds);

    QuickBlues(sc->parent,&bd);
    if ( bounds->maxy >= bd.xheight-bd.xheight/25 && bounds->maxy <= bd.xheight+bd.xheight/25 )
	bounds->maxy = bd.xheight;
    else if ( bounds->maxy >= bd.caph-bd.xheight/25 && bounds->maxy <= bd.caph+bd.xheight/25 )
	bounds->maxy = bd.caph;
    else if ( bounds->maxy >= bd.ascent-bd.xheight/25 && bounds->maxy <= bd.ascent+bd.xheight/25 )
	bounds->maxy = bd.ascent;

    ymin = bounds->miny-1, ymax = bounds->maxy+1;

    if ( ia!=0 ) {
	bounds->minx = bounds->maxx = 0;

	for ( rf=sc->refs; rf!=NULL; rf = rf->next )
	    _SplineSetFindXRange(rf->splines,bounds,ymin,ymax,ia);

	_SplineSetFindXRange(sc->splines,bounds,ymin,ymax,ia);
    }
return( ymin );
}

static int SCStemCheck(SplineFont *sf,int basech,DBounds *bb, DBounds *rbb,int pos) {
    /* cedilla (and ogonec on A) should be centered underneath the left */
    /*  (or right) vertical (or diagonal) stem. Here we try to find that */
    /*  stem */
    StemInfo *h, *best;
    SplineChar *sc;
    DStemInfo *d, *dbest;

    sc = findchar(sf,basech);
    if ( sc==NULL )
return( 0x70000000 );
    if ( sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,true);
    if ( (best=sc->vstem)!=NULL ) {
	if ( pos&____CENTERLEFT ) {
	    for ( h=best->next; h!=NULL && h->start<best->start+best->width; h=h->next )
		if ( h->start+h->width<best->start+best->width )
		    best = h;
	    if ( best->start+best->width/2>(bb->maxx+bb->minx)/2 )
		best = NULL;
	} else {
	    while ( best->next!=NULL )
		best = best->next;
	    if ( best->start+best->width/2<(bb->maxx+bb->minx)/2 )
		best = NULL;
	}
	if ( best!=NULL )
return( best->start + best->width/2 - (rbb->maxx-rbb->minx)/2 - rbb->minx );
    }
    if ( (dbest=sc->dstem)!=NULL ) {
	if ( pos&____CENTERLEFT ) {
	    for ( d=dbest->next; d!=NULL ; d=d->next )
		if ( d->rightedgebottom.x<dbest->rightedgebottom.x )
		    dbest = d;
	    if ( (dbest->leftedgebottom.x+dbest->rightedgebottom.x)/2>(bb->maxx+bb->minx)/2 )
		dbest = NULL;
	} else {
	    for ( d=dbest->next; d!=NULL ; d=d->next )
		if ( d->leftedgebottom.x>dbest->leftedgebottom.x )
		    dbest = d;
	    if ( (dbest->leftedgebottom.x+dbest->rightedgebottom.x)/2<(bb->maxx+bb->minx)/2 )
		dbest = NULL;
	}
	if ( dbest!=NULL )
return( (dbest->leftedgebottom.x+dbest->rightedgebottom.x)/2 - (rbb->maxx-rbb->minx)/2 - rbb->minx );
    }
return( 0x70000000 );
}

static void _SCAddRef(SplineChar *sc,SplineChar *rsc,real transform[6]) {
    RefChar *ref = chunkalloc(sizeof(RefChar));

    ref->sc = rsc;
    ref->unicode_enc = rsc->unicodeenc;
    ref->local_enc = rsc->enc;
    ref->adobe_enc = getAdobeEnc(rsc->name);
    ref->next = sc->refs;
    sc->refs = ref;
    memcpy(ref->transform,transform,sizeof(real [6]));
    SCReinstanciateRefChar(sc,ref);
    SCMakeDependent(sc,rsc);
}

static void SCAddRef(SplineChar *sc,SplineChar *rsc,real xoff, real yoff) {
    real transform[6];
    transform[0] = transform[3] = 1;
    transform[1] = transform[2] = 0;
    transform[4] = xoff; transform[5] = yoff;
    _SCAddRef(sc,rsc,transform);
}

static void BCClearAndCopy(BDFFont *bdf,int toenc,int fromenc) {
    BDFChar *bc, *rbc;

    bc = BDFMakeChar(bdf,toenc);
    BCPreserveState(bc);
    BCFlattenFloat(bc);
    BCCompressBitmap(bc);
    if ( bdf->chars[fromenc]!=NULL ) {
	rbc = bdf->chars[fromenc];
	free(bc->bitmap);
	bc->xmin = rbc->xmin;
	bc->xmax = rbc->xmax;
	bc->ymin = rbc->ymin;
	bc->ymax = rbc->ymax;
	bc->bytes_per_line = rbc->bytes_per_line;
	bc->width = rbc->width;
	bc->bitmap = galloc(bc->bytes_per_line*(bc->ymax-bc->ymin+1));
	memcpy(bc->bitmap,rbc->bitmap,bc->bytes_per_line*(bc->ymax-bc->ymin+1));
    }
}

static int SCMakeBaseReference(SplineChar *sc,SplineFont *sf,int ch, int copybmp) {
    SplineChar *rsc;
    BDFFont *bdf;

    rsc = findchar(sf,ch);
    if ( rsc==NULL ) {
	if ( ch==0x131 || ch==0xf6be ) {
	    if ( ch==0x131 ) ch='i'; else ch = 'j';
	    rsc = findchar(sf,ch);
	    if ( rsc!=NULL && !sf->dotlesswarn ) {
		GWidgetErrorR( _STR_MissingChar,ch=='i'?_STR_Missingdotlessi:_STR_Missingdotlessj);
		sf->dotlesswarn = true;
	    }
	}
	if ( rsc==NULL )
return( 0 );
    }
    sc->width = rsc->width;
    if ( copybmp ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    BCClearAndCopy(bdf,sc->enc,rsc->enc);
	}
    }
    if ( ch!=' ' )		/* Some accents are built by combining with space. Don't. The reference won't be displayed or selectable but will be there and might cause confusion... */
	SCAddRef(sc,rsc,0,0);	/* should be after the call to MakeChar */
return( 1 );
}

static void SCCenterAccent(SplineChar *sc,SplineFont *sf,int ch, int copybmp,
	real ia, int basech ) {
    const unichar_t *apt = accents[ch-BottomAccent], *end = apt+3;
    int ach= -1;
    int invert = false;
    SplineChar *rsc;
    real transform[6];
    DBounds bb, rbb, bbb;
    real xoff, yoff;
    extern int accent_offset;	/* in prefs.c */
    real spacing = (sf->ascent+sf->descent)*accent_offset/100;
    BDFChar *bc, *rbc;
    int ixoff, iyoff, ispacing, pos;
    BDFFont *bdf;
    real ybase, italicoff;
    const unichar_t *temp;
    SplineChar *basersc;
    int baserch = basech;

    /* When we center an accent on Uhorn, we don't really want it centered */
    /*  on the combination, we want it centered on "U". So if basech is itself*/
    /*  a combo, find what it is based on */
    if ( (temp = SFGetAlternate(sf,basech))!=NULL && haschar(sf,*temp))
	baserch = *temp;
    
    /* cedilla on lower "g" becomes a turned comma above it */
    if ( ch==0x327 && basech=='g' && haschar(sf,0x312))
	ch = 0x312;
    if ( ch>=BottomAccent && ch<=TopAccent ) {
	while ( *apt && apt<end && !haschar(sf,*apt)) ++apt;
	if ( *apt!='\0' && apt<end )
	    ach = *apt;
	else if ( haschar(sf,ch))
	    ach = ch;
	else if ( ch==0x31b && haschar(sf,','))
	    ach = ',';
	else if (( ch == 0x32f || ch == 0x311 ) && haschar(sf,0x2d8)) {
	    ach = 0x2d8;
	    invert = true;
	} else if ( (ch == 0x30c || ch == 0x32c ) &&
		(haschar(sf,0x302) || haschar(sf,0x2c6) || haschar(sf,'^')) ) {
	    invert = true;
	    if ( haschar(sf,0x2c6))
		ach = 0x2c6;
	    else if ( haschar(sf,'^') )
		ach = '^';
	    else
		ach = 0x302;
	}
    } else
	ach = ch;
    rsc = findchar(sf,ach);
    SplineCharFindSlantedBounds(rsc,&rbb,ia);
    if ( ch==0x328 || ch==0x327 ) {
	SCFindTopBounds(rsc,&rbb,ia);
	/* should do more than touch, should overlap a tiny bit... */
	rbb.maxy -= (rbb.maxy-rbb.miny)/30;
    } else if ( ch==0x309 )
	SCFindBottomBounds(rsc,&rbb,ia);
    else if ( basech=='A' && ch==0x30a )
	/* Again, a tiny bit of overlap is usual for Aring */
	rbb.miny += (rbb.maxy-rbb.miny)/30;
    if ( ia!=0 || ( basersc = findchar(sf,baserch))==NULL )
	basersc = sc;
    ybase = SplineCharFindSlantedBounds(sc,&bb,ia);
    if ( ia==0 ) {
	ybase = SplineCharFindSlantedBounds(basersc,&bbb,ia);
	if ( ____utype2[1+ch]&(____ABOVE|____BELOW) ) {
	    bbb.maxy = bb.maxy;
	    bbb.miny = bb.miny;
	}
	if ( ____utype2[1+ch]&(____RIGHT|____LEFT) ) {
	    bbb.maxx = bb.maxx;
	    bbb.minx = bb.minx;
	}
	bb = bbb;
    }
    if ( basech>=0x1f20 && basech<=0x1f27 && ch==0x345 ) {
	bb.miny = 0;		/* ypogegrammeni rides below baseline, not below bottom stem */
	bb.minx -= (bb.maxx-bb.minx)/3;	/* Should also be centered on left stem of eta, but I don't know how to do that..., hence this hack */
    }

    transform[0] = transform[3] = 1;
    transform[1] = transform[2] = transform[4] = transform[5] = 0;
    if ( invert ) {
	/* this transform does a vertical flip from the vertical midpoint of the breve */
	transform[3] = -1;
	transform[5] = rbb.maxy+rbb.miny;
    }
    pos = ____utype2[1+ch];
    /* In greek, PSILI and friends are centered above lower case, and kern left*/
    /*  for upper case */
    if (( basech>=0x390 && basech<=0x3ff) || (basech>=0x1f00 && basech<=0x1fff)) {
	if ( ( basech==0x1fbf || basech==0x1fef || basech==0x1ffe ) &&
		(ch==0x1fbf || ch==0x1fef || ch==0x1ffe || ch==0x1fbd || ch==0x1ffd )) {
	    pos = ____ABOVE|____RIGHT;
	} else if ( isupper(basech) &&
		(ch==0x313 || ch==0x314 || ch==0x301 || ch==0x300 || ch==0x30d ||
		 ch==0x1ffe || ch==0x1fbf || ch==0x1fcf || ch==0x1fdf ||
		 ch==0x1fbd || ch==0x1fef || ch==0x1ffd ||
		 ch==0x1fcd || ch==0x1fdd || ch==0x1fce || ch==0x1fde ) )
	    pos = ____ABOVE|____LEFT;
	else if ( isupper(basech) && ch==0x1fbe )
	    pos = ____RIGHT;
	else if ( ch==0x1fcd || ch==0x1fdd || ch==0x1fce || ch==0x1fde ||
		 ch==0x1ffe || ch==0x1fbf || ch==0x1fcf || ch==0x1fdf )
	    pos = ____ABOVE;
    } else if ( (basech==0x1ffe || basech==0x1fbf) && (ch==0x301 || ch==0x300))
	pos = ____RIGHT;
    else if ( sc->unicodeenc==0x1fbe && ch==0x345 )
	pos = ____RIGHT;
    else if ( basech=='l' && ch==0xb7 )
	pos = ____RIGHT|____OVERSTRIKE;
    else if ( ch==0xb7 )
	pos = ____OVERSTRIKE;
    else if ( basech=='A' && ch==0x30a )	/* Aring usually touches */
	pos = ____ABOVE|____TOUCHING;
    else if (( basech=='A' || basech=='a' || basech=='E' || basech=='u' ) &&
	    ch == 0x328 )
	pos = ____BELOW|____CENTERRIGHT|____TOUCHING;	/* ogonek off to the right for these in polish (but not lc e) */
    else if (( basech=='N' || basech=='n' || basech=='K' || basech=='k' || basech=='R' || basech=='r' || basech=='H' || basech=='h' ) &&
	    ch == 0x327 )
	pos = ____BELOW|____CENTERLEFT|____TOUCHING;	/* cedilla off under left stem for these guys */
    if ( basech==0x391 && pos==(____ABOVE|____LEFT) ) {
	bb.minx += (bb.maxx-bb.minx)/4;
    }
    if ( (sc->unicodeenc==0x1fbd || sc->unicodeenc==0x1fbf ||
	    sc->unicodeenc==0x1ffe || sc->unicodeenc==0x1fc0 ) &&
	    bb.maxy==0 && bb.miny==0 ) {
	/* Building accents on top of space */
	bb.maxy = 7*sf->ascent/10;
    }

    if ( (pos&____ABOVE) && (pos&(____LEFT|____RIGHT)) )
	yoff = bb.maxy - rbb.maxy;
    else if ( pos&____ABOVE )
	yoff = bb.maxy + spacing - rbb.miny;
    else if ( pos&____BELOW ) {
	yoff = bb.miny - rbb.maxy;
	if ( !( pos&____TOUCHING) )
	    yoff -= spacing;
    } else if ( pos&____OVERSTRIKE )
	yoff = bb.miny - rbb.miny + ((bb.maxy-bb.miny)-(rbb.maxy-rbb.miny))/2;
    else /* If neither Above, Below, nor overstrike then should use the same baseline */
	yoff = bb.miny - rbb.miny;

    if ( pos&(____ABOVE|____BELOW) ) {
	if ( !sf->serifcheck ) SFHasSerifs(sf);
	if ( sf->issans ) {
	    if ( ia==0 ) {
		if ( pos&____ABOVE )
		    ybase = SCFindTopXRange(basersc,&bbb,ia);
		else if ( pos&____BELOW )
		    ybase = SCFindBottomXRange(basersc,&bbb,ia);
		bb.maxx = bbb.maxx;
		bb.minx = bbb.minx;
	    } else {
		if ( pos&____ABOVE )
		    ybase = SCFindTopXRange(sc,&bb,ia);
		else if ( pos&____BELOW )
		    ybase = SCFindBottomXRange(sc,&bb,ia);
	    }
	}
    }

    if ( isupper(basech) && ch==0x342)	/* While this guy rides above PSILI on left */
	xoff = bb.minx - rbb.minx;
    else if ( pos&____LEFT )
	xoff = bb.minx - spacing - rbb.maxx;
    else if ( pos&____RIGHT ) {
	xoff = bb.maxx - rbb.minx+spacing/2;
	if ( !( pos&____TOUCHING) )
	    xoff += spacing;
    } else {
	if ( (pos&(____CENTERLEFT|____CENTERRIGHT)) &&
		(xoff=SCStemCheck(sf,basech,&bb,&rbb,pos))!=0x70000000 )
	    /* Done */;
	else if ( pos&____CENTERLEFT )
	    xoff = bb.minx + (bb.maxx-bb.minx)/2 - rbb.maxx;
	else if ( pos&____LEFTEDGE )
	    xoff = bb.minx - rbb.minx;
	else if ( pos&____CENTERRIGHT )
	    xoff = bb.minx + (bb.maxx-bb.minx)/2 - rbb.minx;
	else if ( pos&____RIGHTEDGE )
	    xoff = bb.maxx - rbb.maxx;
	else
	    xoff = bb.minx - rbb.minx + ((bb.maxx-bb.minx)-(rbb.maxx-rbb.minx))/2;
    }
    italicoff = 0;
    if ( ia!=0 )
	xoff += (italicoff = tan(-ia)*(rbb.miny+yoff-ybase));
    transform[4] = xoff;
    if ( invert ) transform[5] -= yoff; else transform[5] += yoff;
    _SCAddRef(sc,rsc,transform);
    if ( pos&____RIGHT )
	SCSynchronizeWidth(sc,sc->width + rbb.maxx-rbb.minx+spacing,sc->width,sf->fv);

    if ( copybmp ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->chars[rsc->enc]!=NULL && bdf->chars[sc->enc]!=NULL ) {
		if ( (ispacing = (bdf->pixelsize*accent_offset+50)/100)<=1 ) ispacing = 2;
		rbc = bdf->chars[rsc->enc];
		BCFlattenFloat(rbc);
		BCCompressBitmap(rbc);
		bc = bdf->chars[sc->enc];
		BCCompressBitmap(bc);
		if ( (pos&____ABOVE) && (pos&(____LEFT|____RIGHT)) )
		    iyoff = bc->ymax - rbc->ymax;
		else if ( pos&____ABOVE )
		    iyoff = bc->ymax + ispacing - rbc->ymin;
		else if ( pos&____BELOW ) {
		    iyoff = bc->ymin - rbc->ymax;
		    if ( !( pos&____TOUCHING) )
			iyoff -= ispacing;
		} else if ( pos&____OVERSTRIKE )
		    iyoff = bc->ymin - rbc->ymin + ((bc->ymax-bc->ymin)-(rbc->ymax-rbc->ymin))/2;
		else
		    iyoff = bc->ymin - rbc->ymin;
		if ( isupper(basech) && ch==0x342)
		    xoff = bc->xmin -  rbc->xmin;
		else if ( pos&____LEFT )
		    ixoff = bc->xmin - ispacing - rbc->xmax;
		else if ( pos&____RIGHT ) {
		    ixoff = bc->xmax - rbc->xmin + ispacing/2;
		    if ( !( pos&____TOUCHING) )
			ixoff += ispacing;
		} else {
		    if ( pos&____CENTERLEFT )
			ixoff = bc->xmin + (bc->xmax-bc->xmin)/2 - rbc->xmax;
		    else if ( pos&____LEFTEDGE )
			ixoff = bc->xmin - rbc->xmin;
		    else if ( pos&____CENTERRIGHT )
			ixoff = bc->xmin + (bc->xmax-bc->xmin)/2 - rbc->xmin;
		    else if ( pos&____RIGHTEDGE )
			ixoff = bc->xmax - rbc->xmax;
		    else
			ixoff = bc->xmin - rbc->xmin + ((bc->xmax-bc->xmin)-(rbc->xmax-rbc->xmin))/2;
		}
		ixoff += rint(italicoff*bdf->pixelsize/(real) (sf->ascent+sf->descent));
		BCPasteInto(bc,rbc,ixoff,iyoff, invert, false);
	    }
	}
    }
}

static void SCPutRefAfter(SplineChar *sc,SplineFont *sf,int ch, int copybmp) {
    SplineChar *rsc = findchar(sf,ch);
    BDFFont *bdf;
    BDFChar *bc, *rbc;

    SCAddRef(sc,rsc,sc->width,0);
    sc->width += rsc->width;
    if ( copybmp ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->chars[rsc->enc]!=NULL && bdf->chars[sc->enc]!=NULL ) {
		rbc = bdf->chars[rsc->enc];
		bc = bdf->chars[sc->enc];
		BCFlattenFloat(rbc);
		BCCompressBitmap(rbc);
		BCCompressBitmap(bc);
		BCPasteInto(bc,rbc,bc->width,0,false, false);
		bc->width += rbc->width;
	    }
	}
    }
}

static void DoSpaces(SplineFont *sf,SplineChar *sc,int copybmp,FontView *fv) {
    int width;
    int enc = sc->unicodeenc;
    int em = sf->ascent+sf->descent;
    SplineChar *tempsc;
    BDFChar *bc;
    BDFFont *bdf;

    if ( iszerowidth(enc))
	width = 0;
    else switch ( enc ) {
      case 0x2000: case 0x2002:
	width = em/2;
      break;
      case 0x2001: case 0x2003:
	width = em;
      break;
      case 0x2004:
	width = em/3;
      break;
      case 0x2005:
	width = em/4;
      break;
      case 0x2006:
	width = em/6;
      break;
      case 0x2009:
	width = em/10;
      break;
      case 0x200a:
	width = em/100;
      break;
      case 0x2007:
	tempsc = findchar(sf,'0');
	if ( tempsc!=NULL && tempsc->splines==NULL && tempsc->refs==NULL ) tempsc = NULL;
	if ( tempsc==NULL ) width = em/2; else width = tempsc->width;
      break;
      case 0x2008:
	tempsc = findchar(sf,'.');
	if ( tempsc!=NULL && tempsc->splines==NULL && tempsc->refs==NULL ) tempsc = NULL;
	if ( tempsc==NULL ) width = em/4; else width = tempsc->width;
      break;
      case ' ':
	tempsc = findchar(sf,'I');
	if ( tempsc!=NULL && tempsc->splines==NULL && tempsc->refs==NULL ) tempsc = NULL;
	if ( tempsc==NULL ) width = em/4; else width = tempsc->width;
      break;
      default:
	width = em/3;
      break;
    }

    sc->width = width;
    sc->widthset = true;
    if ( copybmp ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( (bc = bdf->chars[sc->enc])==NULL ) {
		BDFMakeChar(bdf,sc->enc);
	    } else {
		BCPreserveState(bc);
		BCFlattenFloat(bc);
		BCCompressBitmap(bc);
		free(bc->bitmap);
		bc->xmin = 0;
		bc->xmax = 1;
		bc->ymin = 0;
		bc->ymax = 1;
		bc->bytes_per_line = 1;
		bc->width = rint(width*bdf->pixelsize/(real) em);
		bc->bitmap = gcalloc(bc->bytes_per_line*(bc->ymax-bc->ymin+1),sizeof(char));
	    }
	}
    }
    SCCharChangedUpdate(sc);
    if ( copybmp ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->chars[sc->enc]!=NULL )
		BCCharChangedUpdate(bdf->chars[sc->enc],fv);
    }
}

static SplinePoint *MakeSP(real x, real y, SplinePoint *last) {
    SplinePoint *new = chunkalloc(sizeof(SplinePoint));

    new->me.x = x; new->me.y = y;
    new->prevcp = new->nextcp = new->me;
    new->noprevcp = new->nonextcp = true;
    new->pointtype = pt_corner;
    if ( last!=NULL )
	SplineMake(last,new);
return( new );
}

static void DoRules(SplineFont *sf,SplineChar *sc,int copybmp,FontView *fv) {
    int width;
    int enc = sc->unicodeenc;
    int em = sf->ascent+sf->descent;
    SplineChar *tempsc;
    BDFChar *bc, *tempbc;
    BDFFont *bdf;
    DBounds b;
    real lbearing, rbearing, height, ypos;
    SplinePoint *first, *sp;

    switch ( enc ) {
      case '-':
	width = em/3;
      break;
      case 0x2010: case 0x2011:
	tempsc = findchar(sf,'-');
	if ( tempsc!=NULL && tempsc->splines==NULL && tempsc->refs==NULL ) tempsc = NULL;
	if ( tempsc==NULL ) width = (4*em)/10; else width = tempsc->width;
      break;
      case 0x2012:
	tempsc = findchar(sf,'0');
	if ( tempsc!=NULL && tempsc->splines==NULL && tempsc->refs==NULL ) tempsc = NULL;
	if ( tempsc==NULL ) width = em/2; else width = tempsc->width;
      break;
      case 0x2013:
	width = em/2;
      break;
      case 0x2014: case 0x30fc:
	width = em;
      break;
      case 0x2015:		/* French quotation dash */
	width = 2*em;
      break;
      default:
	width = em/3;
      break;
    }

    tempsc = findchar(sf,'-');
    if ( tempsc==NULL || (tempsc->splines==NULL && tempsc->refs==NULL )) {
	height = em/10;
	lbearing = rbearing = em/10;
	if ( lbearing+rbearing>2*width/3 )
	    lbearing = rbearing = width/4;
	ypos = em/4;
    } else {
	SplineCharFindBounds(tempsc,&b);
	height = b.maxy-b.miny;
	rbearing = tempsc->width - b.maxx;
	lbearing = b.minx;
	ypos = b.miny;
    }
    first = sp = MakeSP(lbearing,ypos,NULL);
    sp = MakeSP(lbearing,ypos+height,sp);
    sp = MakeSP(width-rbearing,ypos+height,sp);
    sp = MakeSP(width-rbearing,ypos,sp);
    SplineMake(sp,first);
    sc->splines = chunkalloc(sizeof(SplinePointList));
    sc->splines->first = sc->splines->last = first;
    sc->width = width;
    sc->widthset = true;

    if ( copybmp ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( (bc = bdf->chars[sc->enc])==NULL ) {
		BDFMakeChar(bdf,sc->enc);
	    } else {
		BCPreserveState(bc);
		BCFlattenFloat(bc);
		BCCompressBitmap(bc);
		free(bc->bitmap);
		tempbc = SplineCharRasterize(sc,bdf->pixelsize);
		bc->xmin = tempbc->xmin;
		bc->xmax = tempbc->xmax;
		bc->ymin = tempbc->ymin;
		bc->ymax = tempbc->ymax;
		bc->bytes_per_line = tempbc->bytes_per_line;
		bc->width = tempbc->width;
		bc->bitmap = tempbc->bitmap;
		free(tempbc);
	    }
	}
    }
    SCCharChangedUpdate(sc);
    if ( copybmp ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->chars[sc->enc]!=NULL )
		BCCharChangedUpdate(bdf->chars[sc->enc],fv);
    }
}

static void DoRotation(SplineFont *sf,SplineChar *sc,int copybmp,FontView *fv) {
    /* In when laying CJK characters vertically and intermixed latin (greek,etc) */
    /*  characters need to be rotated. Adobe's cid tables call for some */
    /*  pre-rotated characters, so we might as well be prepared to deal with */
    /*  them. Note the rotated and normal characters are often in different */
    /*  subfonts so we can't use references */
    SplineChar *scbase;
    real transform[6];
    SplineSet *last, *temp;
    RefChar *ref;
    BDFFont *bdf;

    if ( sf->cidmaster!=NULL && strncmp(sc->name,"vertcid_",8)==0 ) {
	char *end;
	int cid = strtol(sc->name+8,&end,10), j;
	if ( *end!='\0' || (j=SFHasCID(sf,cid))==-1)
return;		/* Can't happen */
	scbase = sf->cidmaster->subfonts[j]->chars[cid];
    } else if ( strncmp(sc->name,"vertuni",7)==0 && strlen(sc->name)==11 ) {
	char *end;
	int uni = strtol(sc->name+7,&end,16), index;
	if ( *end!='\0' || (index = SFCIDFindExistingChar(sf,uni,NULL))==-1 )
return;		/* Can't happen */
	if ( sf->cidmaster==NULL )
	    scbase = sf->chars[index];
	else
	    scbase = sf->cidmaster->subfonts[SFHasCID(sf,index)]->chars[index];
    }

    if ( scbase->parent->vertical_origin==0 )
	scbase->parent->vertical_origin = scbase->parent->ascent;
    transform[0] = transform[3] = 0;
    transform[1] = -1; transform[2] = 1;
    transform[4] = scbase->parent->descent; transform[5] = scbase->parent->vertical_origin;

    sc->splines = SplinePointListTransform(SplinePointListCopy(scbase->splines),
	    transform, true );
    if ( sc->splines==NULL ) last = NULL;
    else for ( last = sc->splines; last->next!=NULL; last = last->next );

    for ( ref = scbase->refs; ref!=NULL; ref=ref->next ) {
	temp = SplinePointListTransform(SplinePointListCopy(ref->splines),
	    transform, true );
	if ( last==NULL )
	    sc->splines = temp;
	else
	    last->next = temp;
	if ( temp!=NULL )
	    for ( last=temp; last->next!=NULL; last=last->next );
    }
    sc->width = sc->parent->ascent+sc->parent->descent;
    SCCharChangedUpdate(sc);
    if ( copybmp ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    BDFChar *from, *to;
	    if ( scbase->enc>=bdf->charcnt || sc->enc>=bdf->charcnt ||
		    bdf->chars[scbase->enc]==NULL )
	continue;
	    from = bdf->chars[scbase->enc];
	    to = BDFMakeChar(bdf,sc->enc);
	    BCRotateCharForVert(to,from,bdf);
	    BCCharChangedUpdate(to,fv);
	}
    }
}

static int SCMakeRightToLeftLig(SplineChar *sc,SplineFont *sf,
	const unichar_t *start,int copybmp) {
    int cnt = u_strlen(start);
    GBiText bd;
    int ret, ch;
    unichar_t *pt, *end, *freeme=NULL;
    static unichar_t final_lamalef[] = { 0xFE8E, 0xfee0, 0 }, isolated_lamalef[] = { 0xFE8E ,0xfedf, 0 };
    /* Bugs: Doesn't handle accents, and there are some in arabic ligs */

    /* Deal with arabic ligatures which are not isolated */
    if ( isarabinitial(sc->unicodeenc) || isarabmedial(sc->unicodeenc) ||
	    isarabfinal(sc->unicodeenc)) {
	++cnt;
	if ( isarabmedial(sc->unicodeenc)) ++cnt;
	freeme = galloc((cnt+1)*sizeof(unichar_t));
	if ( isarabinitial(sc->unicodeenc)) {
	    u_strcpy(freeme,start);
	    freeme[cnt-2] = 0x200d; freeme[cnt-1] = 0;
	} else {
	    *freeme = 0x200d;
	    u_strcpy(freeme+1,start);
	    if ( isarabmedial(sc->unicodeenc)) {
		freeme[cnt-2] = 0x200d;
		freeme[cnt-1] = 0;
	    }
	}
	start = freeme;
    }

    ++cnt;		/* for EOS */
    bd.text = galloc(cnt*sizeof(unichar_t));
    bd.level = galloc(cnt*sizeof(uint8));
    bd.override = galloc(cnt*sizeof(uint8));
    bd.type = galloc(cnt*sizeof(uint16));
    bd.original = galloc(cnt*sizeof(unichar_t *));
    --cnt;
    bd.len = cnt;
    bd.base_right_to_left = true;
    GDrawBiText1(&bd,start,cnt);
    GDrawBiText2(&bd,0,cnt);

    ret = false;
    pt = bd.text; end = pt+cnt;
    if ( *pt==0x200d ) ++pt;
	/* The arabic encoder knows about two ligs. So if we pass in either of*/
	/*  those two we will get out itself. We want it decomposed so undo */
	/*  the process */
    if ( sc->unicodeenc==0xfedf )
	pt = isolated_lamalef;
    else if ( sc->unicodeenc==0xfee0 )
	pt = final_lamalef;
    if ( SCMakeBaseReference(sc,sf,*pt,copybmp) ) {
	for ( ++pt; pt<end; ++pt ) if ( *pt!=0x200d ) {
	    ch = *pt;
	    /* Arabic characters may get transformed into initial/medial/final*/
	    /*  and we don't know that those characters exist in the font. We */
	    /*  do know that the "unformed" character exists (because we checked)*/
	    /*  so go back to using it if we must */
	    if ( !haschar(sf,ch) ) {
		const unichar_t *temp = SFGetAlternate(sf,ch);
		if ( temp!=NULL ) ch = *temp;
	    }
	    SCPutRefAfter(sc,sf,ch,copybmp);
	}
	ret = true;
    }
    free(bd.text); free(bd.level); free(bd.override); free(bd.type);
    free(bd.original);
return( ret );
}

static void SCBuildHangul(SplineFont *sf,SplineChar *sc, const unichar_t *pt, int copybmp) {
    SplineChar *rsc;
    int first = true;
    BDFFont *bdf;

    sc->width = 0;
    while ( *pt ) {
	rsc = findchar(sf,*pt++);
	if ( rsc!=NULL ) {
	    SCAddRef(sc,rsc,0,0);
	    if ( rsc->width>sc->width ) sc->width = rsc->width;
	    if ( copybmp ) {
		for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		    if ( first )
			BCClearAndCopy(bdf,sc->enc,rsc->enc);
		    else if ( bdf->chars[rsc->enc]!=NULL )
			BCPasteInto(BDFMakeChar(bdf,sc->enc),bdf->chars[rsc->enc],
				0,0, false, false);
		}
	    }
	    first = false;
	}
    }
}

void SCBuildComposit(SplineFont *sf, SplineChar *sc, int copybmp,FontView *fv) {
    const unichar_t *pt, *apt; unichar_t ch;
    BDFFont *bdf;
    real ia;
    /* This does not handle arabic ligatures at all. It would need to reverse */
    /*  the string and deal with <final>, <medial>, etc. info we don't have */

    if ( !SFIsSomethingBuildable(sf,sc))
return;
    SCPreserveState(sc,true);
    SplinePointListsFree(sc->splines);
    sc->splines = NULL;
    SCRemoveDependents(sc);
    sc->width = 0;

    if ( iszerowidth(sc->unicodeenc) || (sc->unicodeenc>=0x2000 && sc->unicodeenc<=0x200f )) {
	DoSpaces(sf,sc,copybmp,fv);
return;
    } else if ( sc->unicodeenc>=0x2010 && sc->unicodeenc<=0x2015 ) {
	DoRules(sf,sc,copybmp,fv);
return;
    } else if ( SFIsRotatable(sf,sc) ) {
	DoRotation(sf,sc,copybmp,fv);
return;
    }

    if (( ia = sf->italicangle )==0 )
	ia = SFGuessItalicAngle(sf);
    ia *= 3.1415926535897932/180;

    pt= SFGetAlternate(sf,sc->unicodeenc);
    ch = *pt++;
    if ( ch=='i' || ch=='j' || ch==0x456 ) {
	/* if we're combining i (or j) with an accent that would interfere */
	/*  with the dot, then get rid of the dot. (use dotlessi) */
	for ( apt = pt; *apt && combiningposmask(*apt)!=____ABOVE; ++apt);
	if ( *apt!='\0' ) {
	    if ( ch=='i' || ch==0x456 ) ch = 0x0131;
	    else if ( ch=='j' ) ch = 0xf6be;
	}
    }
    if ( sc->unicodeenc>=0xac00 && sc->unicodeenc<=0xd7a3 )
	SCBuildHangul(sf,sc,pt-1,copybmp);
    else if ( isrighttoleft(ch) && !iscombining(*pt)) {
	SCMakeRightToLeftLig(sc,sf,pt-1,copybmp);
    } else {
	if ( !SCMakeBaseReference(sc,sf,ch,copybmp) )
return;
	while ( iscombining(*pt) || (ch!='l' && *pt==0xb7) ||	/* b7, centered dot is used as a combining accent for Ldot but as a lig for ldot */
		*pt==0x385 || (*pt>=0x1fbd && *pt<=0x1fff ))	/* Special greek accents */
	    SCCenterAccent(sc,sf,*pt++,copybmp,ia, ch);
	while ( *pt )
	    SCPutRefAfter(sc,sf,*pt++,copybmp);
    }
    SCCharChangedUpdate(sc);
    if ( copybmp ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->chars[sc->enc]!=NULL )
		BCCharChangedUpdate(bdf->chars[sc->enc],fv);
    }
return;
}
