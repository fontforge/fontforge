/* -*- coding: utf-8 -*- */
/* Copyright (C) 2000-2012 by George Williams */
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
#include "autohint.h"
#include "autowidth.h"
#include "bitmapchar.h"
#include "bvedit.h"
#include "cvundoes.h"
#include "encoding.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "splinefill.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "namelist.h"
#include <chardata.h>
#include <math.h>
#include <utype.h>
#include <ustring.h>

int accent_offset = 6;
int GraveAcuteCenterBottom = 1;
int PreferSpacingAccents = true;
int CharCenterHighest = 1;

#define BottomAccent	0x300
#define TopAccent	0x345

/* for accents between 0x300 and 345 these are some synonyms */
/* type1 wants accented chars built with accents in the 0x2c? range */
/*  except for grave and acute which live in iso8859-1 range */
/*  this table is ordered on a best try basis */
static const unichar_t accents[][4] = {
    { 0x2cb, 0x300, 0x60 },	/* grave */
    { 0x2ca, 0x301, 0xb4 },	/* acute */
    { 0x2c6, 0x302, 0x5e },	/* circumflex */
    { 0x2dc, 0x303, 0x7e },	/* tilde */
    { 0x2c9, 0x304, 0xaf },	/* macron */
    { 0x305, 0xaf },		/* overline, (macron is suggested as a syn, but it's not quite right) */
    { 0x2d8, 0x306 },		/* breve */
    { 0x2d9, 0x307, '.' },	/* dot above */
    { 0xa8,  0x308 },		/* diaeresis */
    { 0x2c0 },			/* hook above */
    { 0x2da, 0xb0 },		/* ring above */
    { 0x2dd },			/* real acute */
    { 0x2c7 },			/* caron */
    { 0x2c8, 0x384, 0x30d, '\''  },	/* vertical line, tonos */
    { 0x30e, '"' },		/* real vertical line */
    { 0 },			/* real grave */
    { 0 },			/* cand... */		/* 310 */
    { 0 },			/* inverted breve */
    { 0x2bb },			/* turned comma */
    { 0x2bc, 0x313, ',' },	/* comma above */
    { 0x2bd },			/* reversed comma */
    { 0x2bc, 0x315, ',' },	/* comma above right */
    { 0x316, 0x60, 0x2cb },	/* grave below */
    { 0x317, 0xb4, 0x2ca },	/* acute below */
    { 0 },			/* left tack */
    { 0 },			/* right tack */
    { 0 },			/* left angle */
    { 0 },			/* horn, sometimes comma but only if nothing better */
    { 0 },			/* half ring */
    { 0x2d4 },			/* up tack */
    { 0x2d5 },			/* down tack */
    { 0x2d6, 0x31f, '+' },	/* plus below */
    { 0x2d7, 0x320, '-' },	/* minus below */	/* 320 */
    { 0x2b2 },			/* hook */
    { 0 },			/* back hook */
    { 0x323, 0x2d9, '.' },	/* dot below */
    { 0x324, 0xa8 },		/* diaeresis below */
    { 0x325, 0x2da, 0xb0 },	/* ring below */
    { 0x326, 0x2bc, ',' },	/* comma below */
    { 0xb8 },			/* cedilla */
    { 0x2db },			/* ogonek */		/* 0x328 */
    { 0x329, 0x2c8, 0x384, '\''  },	/* vertical line below */
    { 0 },			/* bridge below */
    { 0 },			/* real arch below */
    { 0x32c, 0x2c7 },		/* caron below */
    { 0x32d, 0x2c6, 0x52 },	/* circumflex below */
    { 0x32e, 0x2d8 },		/* breve below */
    { 0 },			/* inverted breve below */
    { 0x330, 0x2dc, 0x7e },	/* tilde below */	/* 0x330 */
    { 0x331, 0xaf, 0x2c9 },	/* macron below */
    { 0x332, '_' },		/* low line */
    { 0 },			/* real low line */
    { 0x334, 0x2dc, 0x7e },	/* tilde overstrike */
    { 0x335, '-' },		/* line overstrike */
    { 0x336, '_' },		/* long line overstrike */
    { 0x337, '/' },		/* short solidus overstrike */
    { 0x338, '/' },		/* long solidus overstrike */	/* 0x338 */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0x340, 0x60, 0x2cb },	/* tone mark, left of circumflex */ /* 0x340 */
    { 0x341, 0xb4, 0x2ca },	/* tone mark, right of circumflex */
    { 0x342, 0x2dc, 0x7e },	/* perispomeni (tilde) */
    { 0x343, 0x2bc, ',' },	/* koronis */
    { 0 },			/* dialytika tonos (two accents) */
    { 0x37a },			/* ypogegrammeni */
    { 0xffff }
};

static int SCMakeDotless(SplineFont *sf, SplineChar *dotless, int layer, BDFFont *bdf, int disp_only, int doit);

int CanonicalCombiner(int uni) {
    /* Translate spacing accents to combiners */
    int j,k;

    /* The above table will use these occasionally, but we don't want to */
    /*  translate them. They aren't accents */
    if ( uni==',' || uni=='\'' || uni=='"' || uni=='~' || uni=='^' || uni=='-' ||
	    uni=='+' || uni=='.' )
return( uni );

    for ( j=0; accents[j][0]!=0xffff; ++j ) {
	for ( k=0; k<4 && accents[j][k]!=0; ++k ) {
	    if ( uni==accents[j][k] ) {
		uni = 0x300+j;
	break;
	    }
	}
	if ( uni>=0x300 && uni<0x370 )
    break;
    }
return( uni );
}

static const char *uc_accent_names[] = {
    "Grave",
    "Acute",
    "Circumflex",
    "Tilde",
    "Macron",
    "Overline",
    "Breve",
    "Dotaccent",
    "Diaeresis",
    NULL,
    "Ring",
    "Acute",
    "Caron"
};

/* greek accents */
/* because of accent unification I can't use the same glyphs for greek and */
/*  latin accents. Annoying. So when unicode decomposes a greek letter to */
/*  use 0x0301 we actually want 0x1ffd and so on */
static unichar_t unicode_greekalts[256][3] = {
/* 1F00 */ { 0x03B1, 0x1FBF, 0 },
/* 1F01 */ { 0x03B1, 0x1FFE, 0 },
/* 1F02 */ { 0x03B1, 0x1FCD, 0 },
/* 1F03 */ { 0x03B1, 0x1FDD, 0 },
/* 1F04 */ { 0x03B1, 0x1FCE, 0 },
/* 1F05 */ { 0x03B1, 0x1FDE, 0 },
/* 1F06 */ { 0x03B1, 0x1FCF, 0 },
/* 1F07 */ { 0x03B1, 0x1FDF, 0 },
/* 1F08 */ { 0x0391, 0x1FBF, 0 },
/* 1F09 */ { 0x0391, 0x1FFE, 0 },
/* 1F0A */ { 0x0391, 0x1FCD, 0 },
/* 1F0B */ { 0x0391, 0x1FDD, 0 },
/* 1F0C */ { 0x0391, 0x1FCE, 0 },
/* 1F0D */ { 0x0391, 0x1FDE, 0 },
/* 1F0E */ { 0x0391, 0x1FCF, 0 },
/* 1F0F */ { 0x0391, 0x1FDF, 0 },
/* 1F10 */ { 0x03B5, 0x1FBF, 0 },
/* 1F11 */ { 0x03B5, 0x1FFE, 0 },
/* 1F12 */ { 0x03B5, 0x1FCD, 0 },
/* 1F13 */ { 0x03B5, 0x1FDD, 0 },
/* 1F14 */ { 0x03B5, 0x1FCE, 0 },
/* 1F15 */ { 0x03B5, 0x1FDE, 0 },
/* 1F16 */ { 0 },
/* 1F17 */ { 0 },
/* 1F18 */ { 0x0395, 0x1FBF, 0 },
/* 1F19 */ { 0x0395, 0x1FFE, 0 },
/* 1F1A */ { 0x0395, 0x1FCD, 0 },
/* 1F1B */ { 0x0395, 0x1FDD, 0 },
/* 1F1C */ { 0x0395, 0x1FCE, 0 },
/* 1F1D */ { 0x0395, 0x1FDE, 0 },
/* 1F1E */ { 0 },
/* 1F1F */ { 0 },
/* 1F20 */ { 0x03B7, 0x1FBF, 0 },
/* 1F21 */ { 0x03B7, 0x1FFE, 0 },
/* 1F22 */ { 0x03B7, 0x1FCD, 0 },
/* 1F23 */ { 0x03B7, 0x1FDD, 0 },
/* 1F24 */ { 0x03B7, 0x1FCE, 0 },
/* 1F25 */ { 0x03B7, 0x1FDE, 0 },
/* 1F26 */ { 0x03B7, 0x1FCF, 0 },
/* 1F27 */ { 0x03B7, 0x1FDF, 0 },
/* 1F28 */ { 0x0397, 0x1FBF, 0 },
/* 1F29 */ { 0x0397, 0x1FFE, 0 },
/* 1F2A */ { 0x0397, 0x1FCD, 0 },
/* 1F2B */ { 0x0397, 0x1FDD, 0 },
/* 1F2C */ { 0x0397, 0x1FCE, 0 },
/* 1F2D */ { 0x0397, 0x1FDE, 0 },
/* 1F2E */ { 0x0397, 0x1FCF, 0 },
/* 1F2F */ { 0x0397, 0x1FDF, 0 },
/* 1F30 */ { 0x03B9, 0x1FBF, 0 },
/* 1F31 */ { 0x03B9, 0x1FFE, 0 },
/* 1F32 */ { 0x03B9, 0x1FCD, 0 },
/* 1F33 */ { 0x03B9, 0x1FDD, 0 },
/* 1F34 */ { 0x03B9, 0x1FCE, 0 },
/* 1F35 */ { 0x03B9, 0x1FDE, 0 },
/* 1F36 */ { 0x03B9, 0x1FCF, 0 },
/* 1F37 */ { 0x03B9, 0x1FDF, 0 },
/* 1F38 */ { 0x0399, 0x1FBF, 0 },
/* 1F39 */ { 0x0399, 0x1FFE, 0 },
/* 1F3A */ { 0x0399, 0x1FCD, 0 },
/* 1F3B */ { 0x0399, 0x1FDD, 0 },
/* 1F3C */ { 0x0399, 0x1FCE, 0 },
/* 1F3D */ { 0x0399, 0x1FDE, 0 },
/* 1F3E */ { 0x0399, 0x1FCF, 0 },
/* 1F3F */ { 0x0399, 0x1FDF, 0 },
/* 1F40 */ { 0x03BF, 0x1FBF, 0 },
/* 1F41 */ { 0x03BF, 0x1FFE, 0 },
/* 1F42 */ { 0x03BF, 0x1FCD, 0 },
/* 1F43 */ { 0x03BF, 0x1FDD, 0 },
/* 1F44 */ { 0x03BF, 0x1FCE, 0 },
/* 1F45 */ { 0x03BF, 0x1FDE, 0 },
/* 1F46 */ { 0 },
/* 1F47 */ { 0 },
/* 1F48 */ { 0x039F, 0x1FBF, 0 },
/* 1F49 */ { 0x039F, 0x1FFE, 0 },
/* 1F4A */ { 0x039F, 0x1FCD, 0 },
/* 1F4B */ { 0x039F, 0x1FDD, 0 },
/* 1F4C */ { 0x039F, 0x1FCE, 0 },
/* 1F4D */ { 0x039F, 0x1FDE, 0 },
/* 1F4E */ { 0 },
/* 1F4F */ { 0 },
/* 1F50 */ { 0x03C5, 0x1FBF, 0 },
/* 1F51 */ { 0x03C5, 0x1FFE, 0 },
/* 1F52 */ { 0x03C5, 0x1FCD, 0 },
/* 1F53 */ { 0x03C5, 0x1FDD, 0 },
/* 1F54 */ { 0x03C5, 0x1FCE, 0 },
/* 1F55 */ { 0x03C5, 0x1FDE, 0 },
/* 1F56 */ { 0x03C5, 0x1FCF, 0 },
/* 1F57 */ { 0x03C5, 0x1FDF, 0 },
/* 1F58 */ { 0 },
/* 1F59 */ { 0x03a5, 0x1FFE, 0 },
/* 1F5A */ { 0 },
/* 1F5B */ { 0x03a5, 0x1FDD, 0 },
/* 1F5C */ { 0 },
/* 1F5D */ { 0x03a5, 0x1FDE, 0 },
/* 1F5E */ { 0 },
/* 1F5F */ { 0x03a5, 0x1FDF, 0 },
/* 1F60 */ { 0x03C9, 0x1FBF, 0 },
/* 1F61 */ { 0x03C9, 0x1FFE, 0 },
/* 1F62 */ { 0x03C9, 0x1FCD, 0 },
/* 1F63 */ { 0x03C9, 0x1FDD, 0 },
/* 1F64 */ { 0x03C9, 0x1FCE, 0 },
/* 1F65 */ { 0x03C9, 0x1FDE, 0 },
/* 1F66 */ { 0x03C9, 0x1FCF, 0 },
/* 1F67 */ { 0x03C9, 0x1FDF, 0 },
/* 1F68 */ { 0x03A9, 0x1FBF, 0 },
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
/* 1FB4 */ { 0x1f71, 0x0345, 0 },
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
/* 1FC4 */ { 0x1f75, 0x0345, 0 },
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

unichar_t adobes_pua_alts[0x200][3] = {	/* Mapped from 0xf600-0xf7ff */
/* U+F600 */ { 0 },
/* U+F601 */ { 0 },
/* U+F602 */ { 0 },
/* U+F603 */ { 0 },
/* U+F604 */ { 0 },
/* U+F605 */ { 0 },
/* U+F606 */ { 0 },
/* U+F607 */ { 0 },
/* U+F608 */ { 0 },
/* U+F609 */ { 0 },
/* U+F60A */ { 0 },
/* U+F60B */ { 0 },
/* U+F60C */ { 0 },
/* U+F60D */ { 0 },
/* U+F60E */ { 0 },
/* U+F60F */ { 0 },
/* U+F610 */ { 0 },
/* U+F611 */ { 0 },
/* U+F612 */ { 0 },
/* U+F613 */ { 0 },
/* U+F614 */ { 0 },
/* U+F615 */ { 0 },
/* U+F616 */ { 0 },
/* U+F617 */ { 0 },
/* U+F618 */ { 0 },
/* U+F619 */ { 0 },
/* U+F61A */ { 0 },
/* U+F61B */ { 0 },
/* U+F61C */ { 0 },
/* U+F61D */ { 0 },
/* U+F61E */ { 0 },
/* U+F61F */ { 0 },
/* U+F620 */ { 0 },
/* U+F621 */ { 0 },
/* U+F622 */ { 0 },
/* U+F623 */ { 0 },
/* U+F624 */ { 0 },
/* U+F625 */ { 0 },
/* U+F626 */ { 0 },
/* U+F627 */ { 0 },
/* U+F628 */ { 0 },
/* U+F629 */ { 0 },
/* U+F62A */ { 0 },
/* U+F62B */ { 0 },
/* U+F62C */ { 0 },
/* U+F62D */ { 0 },
/* U+F62E */ { 0 },
/* U+F62F */ { 0 },
/* U+F630 */ { 0 },
/* U+F631 */ { 0 },
/* U+F632 */ { 0 },
/* U+F633 */ { 0 },
/* U+F634 */ { 0x0020, 0x0326, 0 },
/* U+F635 */ { 0x0020, 0x0326, 0 },
/* U+F636 */ { 0x03d6, 0x0000, 0 },
/* U+F637 */ { 0x0068, 0x0000, 0 },
/* U+F638 */ { 0x0030, 0x0000, 0 },
/* U+F639 */ { 0x0030, 0x0000, 0 },
/* U+F63A */ { 0x0032, 0x0000, 0 },
/* U+F63B */ { 0x0033, 0x0000, 0 },
/* U+F63C */ { 0x0034, 0x0000, 0 },
/* U+F63D */ { 0x0035, 0x0000, 0 },
/* U+F63E */ { 0x0036, 0x0000, 0 },
/* U+F63F */ { 0x0037, 0x0000, 0 },
/* U+F640 */ { 0x0038, 0x0000, 0 },
/* U+F641 */ { 0x0039, 0x0000, 0 },
/* U+F642 */ { 0x0025, 0x0000, 0 },
/* U+F643 */ { 0x0030, 0x0000, 0 },
/* U+F644 */ { 0x0031, 0x0000, 0 },
/* U+F645 */ { 0x0032, 0x0000, 0 },
/* U+F646 */ { 0x0033, 0x0000, 0 },
/* U+F647 */ { 0x0034, 0x0000, 0 },
/* U+F648 */ { 0x0035, 0x0000, 0 },
/* U+F649 */ { 0x0036, 0x0000, 0 },
/* U+F64A */ { 0x0037, 0x0000, 0 },
/* U+F64B */ { 0x0038, 0x0000, 0 },
/* U+F64C */ { 0x0039, 0x0000, 0 },
/* U+F64D */ { 0x20a1, 0x0000, 0 },
/* U+F64E */ { 0x20ac, 0x0000, 0 },
/* U+F64F */ { 0x0192, 0x0000, 0 },
/* U+F650 */ { 0x0023, 0x0000, 0 },
/* U+F651 */ { 0x00a3, 0x0000, 0 },
/* U+F652 */ { 0x00a5, 0x0000, 0 },
/* U+F653 */ { 0x0024, 0x0000, 0 },
/* U+F654 */ { 0x00a2, 0x0000, 0 },
/* U+F655 */ { 0x0030, 0x0000, 0 },
/* U+F656 */ { 0x0031, 0x0000, 0 },
/* U+F657 */ { 0x0032, 0x0000, 0 },
/* U+F658 */ { 0x0033, 0x0000, 0 },
/* U+F659 */ { 0x0034, 0x0000, 0 },
/* U+F65A */ { 0x0035, 0x0000, 0 },
/* U+F65B */ { 0x0036, 0x0000, 0 },
/* U+F65C */ { 0x0037, 0x0000, 0 },
/* U+F65D */ { 0x0038, 0x0000, 0 },
/* U+F65E */ { 0x0039, 0x0000, 0 },
/* U+F65F */ { 0x002c, 0x0000, 0 },
/* U+F660 */ { 0x002e, 0x0000, 0 },
/* U+F661 */ { 0x0030, 0x0000, 0 },
/* U+F662 */ { 0x0031, 0x0000, 0 },
/* U+F663 */ { 0x0032, 0x0000, 0 },
/* U+F664 */ { 0x0033, 0x0000, 0 },
/* U+F665 */ { 0x0034, 0x0000, 0 },
/* U+F666 */ { 0x0035, 0x0000, 0 },
/* U+F667 */ { 0x0036, 0x0000, 0 },
/* U+F668 */ { 0x0037, 0x0000, 0 },
/* U+F669 */ { 0x0038, 0x0000, 0 },
/* U+F66A */ { 0x0039, 0x0000, 0 },
/* U+F66B */ { 0x002c, 0x0000, 0 },
/* U+F66C */ { 0x002e, 0x0000, 0 },
/* U+F66D */ { 0xf761, 0x0306, 0 },
/* U+F66E */ { 0xf761, 0x0304, 0 },
/* U+F66F */ { 0xf761, 0x0328, 0 },
/* U+F670 */ { 0xf7e6, 0x0301, 0 },
/* U+F671 */ { 0xf763, 0x0301, 0 },
/* U+F672 */ { 0xf763, 0x030c, 0 },
/* U+F673 */ { 0xf763, 0x0302, 0 },
/* U+F674 */ { 0xf763, 0x0307, 0 },
/* U+F675 */ { 0xf764, 0x030c, 0 },
/* U+F676 */ { 0x0111, 0x0000, 0 },
/* U+F677 */ { 0xf765, 0x0306, 0 },
/* U+F678 */ { 0xf765, 0x030c, 0 },
/* U+F679 */ { 0xf765, 0x0307, 0 },
/* U+F67A */ { 0xf765, 0x0304, 0 },
/* U+F67B */ { 0x014b, 0x0000, 0 },
/* U+F67C */ { 0xf765, 0x0328, 0 },
/* U+F67D */ { 0xf767, 0x0306, 0 },
/* U+F67E */ { 0xf767, 0x0302, 0 },
/* U+F67F */ { 0xf767, 0x0326, 0 },
/* U+F680 */ { 0xf767, 0x0307, 0 },
/* U+F681 */ { 0x0127, 0x0000, 0 },
/* U+F682 */ { 0xf768, 0x0302, 0 },
/* U+F683 */ { 0xf769, 0x0306, 0 },
/* U+F684 */ { 0xf769, 0xf76a, 0 },
/* U+F685 */ { 0xf769, 0x0304, 0 },
/* U+F686 */ { 0xf769, 0x0328, 0 },
/* U+F687 */ { 0xf769, 0x0303, 0 },
/* U+F688 */ { 0xf76a, 0x0302, 0 },
/* U+F689 */ { 0xf76b, 0x0326, 0 },
/* U+F68A */ { 0xf76c, 0x0301, 0 },
/* U+F68B */ { 0xf76c, 0x030c, 0 },
/* U+F68C */ { 0xf76c, 0x0326, 0 },
/* U+F68D */ { 0xf76c, 0x00b7, 0 },
/* U+F68E */ { 0xf76e, 0x0301, 0 },
/* U+F68F */ { 0xf76e, 0x030c, 0 },
/* U+F690 */ { 0xf76e, 0x0326, 0 },
/* U+F691 */ { 0xf76f, 0x0306, 0 },
/* U+F692 */ { 0xf76f, 0x030b, 0 },
/* U+F693 */ { 0xf76f, 0x0304, 0 },
/* U+F694 */ { 0xf7f8, 0x0301, 0 },
/* U+F695 */ { 0xf772, 0x0301, 0 },
/* U+F696 */ { 0xf772, 0x030c, 0 },
/* U+F697 */ { 0xf772, 0x0326, 0 },
/* U+F698 */ { 0xf773, 0x0301, 0 },
/* U+F699 */ { 0xf773, 0x0327, 0 },
/* U+F69A */ { 0xf773, 0x0302, 0 },
/* U+F69B */ { 0xf773, 0x0326, 0 },
/* U+F69C */ { 0x0167, 0x0000, 0 },
/* U+F69D */ { 0xf774, 0x030c, 0 },
/* U+F69E */ { 0xf774, 0x0326, 0 },
/* U+F69F */ { 0xf775, 0x0306, 0 },
/* U+F6A0 */ { 0xf775, 0x030b, 0 },
/* U+F6A1 */ { 0xf775, 0x0304, 0 },
/* U+F6A2 */ { 0xf775, 0x0328, 0 },
/* U+F6A3 */ { 0xf775, 0x030a, 0 },
/* U+F6A4 */ { 0xf775, 0x0303, 0 },
/* U+F6A5 */ { 0xf777, 0x0301, 0 },
/* U+F6A6 */ { 0xf777, 0x0302, 0 },
/* U+F6A7 */ { 0xf777, 0x0308, 0 },
/* U+F6A8 */ { 0xf777, 0x0300, 0 },
/* U+F6A9 */ { 0xf779, 0x0302, 0 },
/* U+F6AA */ { 0xf779, 0x0300, 0 },
/* U+F6AB */ { 0xf77a, 0x0301, 0 },
/* U+F6AC */ { 0xf77a, 0x0307, 0 },
/* U+F6AD */ { 0xf769, 0x0307, 0 },
/* U+F6AE */ { 0x0028, 0x0000, 0 },
/* U+F6AF */ { 0x0029, 0x0000, 0 },
/* U+F6B0 */ { 0x005b, 0x0000, 0 },
/* U+F6B1 */ { 0x005d, 0x0000, 0 },
/* U+F6B2 */ { 0x007b, 0x0000, 0 },
/* U+F6B3 */ { 0x007d, 0x0000, 0 },
/* U+F6B4 */ { 0x00a1, 0x0000, 0 },
/* U+F6B5 */ { 0x00bf, 0x0000, 0 },
/* U+F6B6 */ { 0x00ab, 0x0000, 0 },
/* U+F6B7 */ { 0x00bb, 0x0000, 0 },
/* U+F6B8 */ { 0x2039, 0x0000, 0 },
/* U+F6B9 */ { 0x203a, 0x0000, 0 },
/* U+F6BA */ { 0x002d, 0x0000, 0 },
/* U+F6BB */ { 0x2013, 0x0000, 0 },
/* U+F6BC */ { 0x2014, 0x0000, 0 },
/* U+F6BD */ { 0x00b7, 0x0000, 0 },
/* U+F6BE */ { 0x006a, 0x0000, 0 },
/* U+F6BF */ { 0x004c, 0x004c, 0 },
/* U+F6C0 */ { 0x006c, 0x006c, 0 },
/* U+F6C1 */ { 0x0053, 0x0327, 0 },
/* U+F6C2 */ { 0x0073, 0x0327, 0 },
/* U+F6C3 */ { 0x0313, 0x0326, 0 },
/* U+F6C4 */ { 0x0433, 0x0000, 0 },
/* U+F6C5 */ { 0x0431, 0x0000, 0 },
/* U+F6C6 */ { 0x0434, 0x0000, 0 },
/* U+F6C7 */ { 0x043f, 0x0000, 0 },
/* U+F6C8 */ { 0x0442, 0x0000, 0 },
/* U+F6C9 */ { 0x02ca, 0x0000, 0 },
/* U+F6CA */ { 0x02c7, 0x0000, 0 },
/* U+F6CB */ { 0x00a8, 0x0000, 0 },
/* U+F6CC */ { 0x00a8, 0x02ca, 0 },
/* U+F6CD */ { 0x00a8, 0x02cb, 0 },
/* U+F6CE */ { 0x02cb, 0x0000, 0 },
/* U+F6CF */ { 0x02dd, 0x0000, 0 },
/* U+F6D0 */ { 0x02c9, 0x0000, 0 },
/* U+F6D1 */ { 0x02d8, 0x0000, 0 },
/* U+F6D2 */ { 0x02c6, 0x0000, 0 },
/* U+F6D3 */ { 0x030f, 0x030f, 0 },
/* U+F6D4 */ { 0x02d8, 0x0000, 0 },
/* U+F6D5 */ { 0x02c6, 0x0000, 0 },
/* U+F6D6 */ { 0x030f, 0x030f, 0 },
/* U+F6D7 */ { 0x00a8, 0x02ca, 0 },
/* U+F6D8 */ { 0x00a8, 0x02cb, 0 },
/* U+F6D9 */ { 0x00a9, 0x0000, 0 },
/* U+F6DA */ { 0x00ae, 0x0000, 0 },
/* U+F6DB */ { 0x2122, 0x0000, 0 },
/* U+F6DC */ { 0x0031, 0x0000, 0 },
/* U+F6DD */ { 0x0052, 0x0070, 0 },
/* U+F6DE */ { 0x002d, 0x0000, 0 },
/* U+F6DF */ { 0x00a2, 0x0000, 0 },
/* U+F6E0 */ { 0x00a2, 0x0000, 0 },
/* U+F6E1 */ { 0x002c, 0x0000, 0 },
/* U+F6E2 */ { 0x002c, 0x0000, 0 },
/* U+F6E3 */ { 0x0024, 0x0000, 0 },
/* U+F6E4 */ { 0x0024, 0x0000, 0 },
/* U+F6E5 */ { 0x002d, 0x0000, 0 },
/* U+F6E6 */ { 0x002d, 0x0000, 0 },
/* U+F6E7 */ { 0x002e, 0x0000, 0 },
/* U+F6E8 */ { 0x002e, 0x0000, 0 },
/* U+F6E9 */ { 0x0061, 0x0000, 0 },
/* U+F6EA */ { 0x0062, 0x0000, 0 },
/* U+F6EB */ { 0x0064, 0x0000, 0 },
/* U+F6EC */ { 0x0065, 0x0000, 0 },
/* U+F6ED */ { 0x0069, 0x0000, 0 },
/* U+F6EE */ { 0x006c, 0x0000, 0 },
/* U+F6EF */ { 0x006d, 0x0000, 0 },
/* U+F6F0 */ { 0x006f, 0x0000, 0 },
/* U+F6F1 */ { 0x0072, 0x0000, 0 },
/* U+F6F2 */ { 0x0073, 0x0000, 0 },
/* U+F6F3 */ { 0x0074, 0x0000, 0 },
/* U+F6F4 */ { 0x02d8, 0x0000, 0 },
/* U+F6F5 */ { 0x02c7, 0x0000, 0 },
/* U+F6F6 */ { 0x02c6, 0x0000, 0 },
/* U+F6F7 */ { 0x02d9, 0x0000, 0 },
/* U+F6F8 */ { 0x02dd, 0x0000, 0 },
/* U+F6F9 */ { 0x0142, 0x0000, 0 },
/* U+F6FA */ { 0xf76f, 0xf765, 0 },
/* U+F6FB */ { 0xf76f, 0x0328, 0 },
/* U+F6FC */ { 0xf772, 0x030a, 0 },
/* U+F6FD */ { 0xf773, 0x030c, 0 },
/* U+F6FE */ { 0xf774, 0x0303, 0 },
/* U+F6FF */ { 0xf77a, 0x030c, 0 },
/* U+F700 */ { 0 },
/* U+F701 */ { 0 },
/* U+F702 */ { 0 },
/* U+F703 */ { 0 },
/* U+F704 */ { 0 },
/* U+F705 */ { 0 },
/* U+F706 */ { 0 },
/* U+F707 */ { 0 },
/* U+F708 */ { 0 },
/* U+F709 */ { 0 },
/* U+F70A */ { 0 },
/* U+F70B */ { 0 },
/* U+F70C */ { 0 },
/* U+F70D */ { 0 },
/* U+F70E */ { 0 },
/* U+F70F */ { 0 },
/* U+F710 */ { 0 },
/* U+F711 */ { 0 },
/* U+F712 */ { 0 },
/* U+F713 */ { 0 },
/* U+F714 */ { 0 },
/* U+F715 */ { 0 },
/* U+F716 */ { 0 },
/* U+F717 */ { 0 },
/* U+F718 */ { 0 },
/* U+F719 */ { 0 },
/* U+F71A */ { 0 },
/* U+F71B */ { 0 },
/* U+F71C */ { 0 },
/* U+F71D */ { 0 },
/* U+F71E */ { 0 },
/* U+F71F */ { 0 },
/* U+F720 */ { 0 },
/* U+F721 */ { 0x0021, 0x0000, 0 },
/* U+F722 */ { 0 },
/* U+F723 */ { 0 },
/* U+F724 */ { 0x0024, 0x0000, 0 },
/* U+F725 */ { 0 },
/* U+F726 */ { 0x0026, 0x0000, 0 },
/* U+F727 */ { 0 },
/* U+F728 */ { 0 },
/* U+F729 */ { 0 },
/* U+F72A */ { 0 },
/* U+F72B */ { 0 },
/* U+F72C */ { 0 },
/* U+F72D */ { 0 },
/* U+F72E */ { 0 },
/* U+F72F */ { 0 },
/* U+F730 */ { 0x0030, 0x0000, 0 },
/* U+F731 */ { 0x0031, 0x0000, 0 },
/* U+F732 */ { 0x0032, 0x0000, 0 },
/* U+F733 */ { 0x0033, 0x0000, 0 },
/* U+F734 */ { 0x0034, 0x0000, 0 },
/* U+F735 */ { 0x0035, 0x0000, 0 },
/* U+F736 */ { 0x0036, 0x0000, 0 },
/* U+F737 */ { 0x0037, 0x0000, 0 },
/* U+F738 */ { 0x0038, 0x0000, 0 },
/* U+F739 */ { 0x0039, 0x0000, 0 },
/* U+F73A */ { 0 },
/* U+F73B */ { 0 },
/* U+F73C */ { 0 },
/* U+F73D */ { 0 },
/* U+F73E */ { 0 },
/* U+F73F */ { 0x003f, 0x0000, 0 },
/* U+F740 */ { 0 },
/* U+F741 */ { 0 },
/* U+F742 */ { 0 },
/* U+F743 */ { 0 },
/* U+F744 */ { 0 },
/* U+F745 */ { 0 },
/* U+F746 */ { 0 },
/* U+F747 */ { 0 },
/* U+F748 */ { 0 },
/* U+F749 */ { 0 },
/* U+F74A */ { 0 },
/* U+F74B */ { 0 },
/* U+F74C */ { 0 },
/* U+F74D */ { 0 },
/* U+F74E */ { 0 },
/* U+F74F */ { 0 },
/* U+F750 */ { 0 },
/* U+F751 */ { 0 },
/* U+F752 */ { 0 },
/* U+F753 */ { 0 },
/* U+F754 */ { 0 },
/* U+F755 */ { 0 },
/* U+F756 */ { 0 },
/* U+F757 */ { 0 },
/* U+F758 */ { 0 },
/* U+F759 */ { 0 },
/* U+F75A */ { 0 },
/* U+F75B */ { 0 },
/* U+F75C */ { 0 },
/* U+F75D */ { 0 },
/* U+F75E */ { 0 },
/* U+F75F */ { 0 },
/* U+F760 */ { 0x0060, 0x0000, 0 },
/* U+F761 */ { 0x0061, 0x0000, 0 },
/* U+F762 */ { 0x0062, 0x0000, 0 },
/* U+F763 */ { 0x0063, 0x0000, 0 },
/* U+F764 */ { 0x0064, 0x0000, 0 },
/* U+F765 */ { 0x0065, 0x0000, 0 },
/* U+F766 */ { 0x0066, 0x0000, 0 },
/* U+F767 */ { 0x0067, 0x0000, 0 },
/* U+F768 */ { 0x0068, 0x0000, 0 },
/* U+F769 */ { 0x0069, 0x0000, 0 },
/* U+F76A */ { 0x006a, 0x0000, 0 },
/* U+F76B */ { 0x006b, 0x0000, 0 },
/* U+F76C */ { 0x006c, 0x0000, 0 },
/* U+F76D */ { 0x006d, 0x0000, 0 },
/* U+F76E */ { 0x006e, 0x0000, 0 },
/* U+F76F */ { 0x006f, 0x0000, 0 },
/* U+F770 */ { 0x0070, 0x0000, 0 },
/* U+F771 */ { 0x0071, 0x0000, 0 },
/* U+F772 */ { 0x0072, 0x0000, 0 },
/* U+F773 */ { 0x0073, 0x0000, 0 },
/* U+F774 */ { 0x0074, 0x0000, 0 },
/* U+F775 */ { 0x0075, 0x0000, 0 },
/* U+F776 */ { 0x0076, 0x0000, 0 },
/* U+F777 */ { 0x0077, 0x0000, 0 },
/* U+F778 */ { 0x0078, 0x0000, 0 },
/* U+F779 */ { 0x0079, 0x0000, 0 },
/* U+F77A */ { 0x007a, 0x0000, 0 },
/* U+F77B */ { 0 },
/* U+F77C */ { 0 },
/* U+F77D */ { 0 },
/* U+F77E */ { 0 },
/* U+F77F */ { 0 },
/* U+F780 */ { 0 },
/* U+F781 */ { 0 },
/* U+F782 */ { 0 },
/* U+F783 */ { 0 },
/* U+F784 */ { 0 },
/* U+F785 */ { 0 },
/* U+F786 */ { 0 },
/* U+F787 */ { 0 },
/* U+F788 */ { 0 },
/* U+F789 */ { 0 },
/* U+F78A */ { 0 },
/* U+F78B */ { 0 },
/* U+F78C */ { 0 },
/* U+F78D */ { 0 },
/* U+F78E */ { 0 },
/* U+F78F */ { 0 },
/* U+F790 */ { 0 },
/* U+F791 */ { 0 },
/* U+F792 */ { 0 },
/* U+F793 */ { 0 },
/* U+F794 */ { 0 },
/* U+F795 */ { 0 },
/* U+F796 */ { 0 },
/* U+F797 */ { 0 },
/* U+F798 */ { 0 },
/* U+F799 */ { 0 },
/* U+F79A */ { 0 },
/* U+F79B */ { 0 },
/* U+F79C */ { 0 },
/* U+F79D */ { 0 },
/* U+F79E */ { 0 },
/* U+F79F */ { 0 },
/* U+F7A0 */ { 0 },
/* U+F7A1 */ { 0x00a1, 0x0000, 0 },
/* U+F7A2 */ { 0x00a2, 0x0000, 0 },
/* U+F7A3 */ { 0 },
/* U+F7A4 */ { 0 },
/* U+F7A5 */ { 0 },
/* U+F7A6 */ { 0 },
/* U+F7A7 */ { 0 },
/* U+F7A8 */ { 0x00a8, 0x0000, 0 },
/* U+F7A9 */ { 0 },
/* U+F7AA */ { 0 },
/* U+F7AB */ { 0 },
/* U+F7AC */ { 0 },
/* U+F7AD */ { 0 },
/* U+F7AE */ { 0 },
/* U+F7AF */ { 0x00af, 0x0000, 0 },
/* U+F7B0 */ { 0 },
/* U+F7B1 */ { 0 },
/* U+F7B2 */ { 0 },
/* U+F7B3 */ { 0 },
/* U+F7B4 */ { 0x00b4, 0x0000, 0 },
/* U+F7B5 */ { 0 },
/* U+F7B6 */ { 0 },
/* U+F7B7 */ { 0 },
/* U+F7B8 */ { 0x00b8, 0x0000, 0 },
/* U+F7B9 */ { 0 },
/* U+F7BA */ { 0 },
/* U+F7BB */ { 0 },
/* U+F7BC */ { 0 },
/* U+F7BD */ { 0 },
/* U+F7BE */ { 0 },
/* U+F7BF */ { 0x00bf, 0x0000, 0 },
/* U+F7C0 */ { 0 },
/* U+F7C1 */ { 0 },
/* U+F7C2 */ { 0 },
/* U+F7C3 */ { 0 },
/* U+F7C4 */ { 0 },
/* U+F7C5 */ { 0 },
/* U+F7C6 */ { 0 },
/* U+F7C7 */ { 0 },
/* U+F7C8 */ { 0 },
/* U+F7C9 */ { 0 },
/* U+F7CA */ { 0 },
/* U+F7CB */ { 0 },
/* U+F7CC */ { 0 },
/* U+F7CD */ { 0 },
/* U+F7CE */ { 0 },
/* U+F7CF */ { 0 },
/* U+F7D0 */ { 0 },
/* U+F7D1 */ { 0 },
/* U+F7D2 */ { 0 },
/* U+F7D3 */ { 0 },
/* U+F7D4 */ { 0 },
/* U+F7D5 */ { 0 },
/* U+F7D6 */ { 0 },
/* U+F7D7 */ { 0 },
/* U+F7D8 */ { 0 },
/* U+F7D9 */ { 0 },
/* U+F7DA */ { 0 },
/* U+F7DB */ { 0 },
/* U+F7DC */ { 0 },
/* U+F7DD */ { 0 },
/* U+F7DE */ { 0 },
/* U+F7DF */ { 0 },
/* U+F7E0 */ { 0xf761, 0x0300, 0 },
/* U+F7E1 */ { 0xf761, 0x0301, 0 },
/* U+F7E2 */ { 0xf761, 0x0302, 0 },
/* U+F7E3 */ { 0xf761, 0x0303, 0 },
/* U+F7E4 */ { 0xf761, 0x0308, 0 },
/* U+F7E5 */ { 0xf761, 0x030a, 0 },
/* U+F7E6 */ { 0xf761, 0xf765, 0 },
/* U+F7E7 */ { 0xf763, 0x0327, 0 },
/* U+F7E8 */ { 0xf765, 0x0300, 0 },
/* U+F7E9 */ { 0xf765, 0x0301, 0 },
/* U+F7EA */ { 0xf765, 0x0302, 0 },
/* U+F7EB */ { 0xf765, 0x0308, 0 },
/* U+F7EC */ { 0xf769, 0x0300, 0 },
/* U+F7ED */ { 0xf769, 0x0301, 0 },
/* U+F7EE */ { 0xf769, 0x0302, 0 },
/* U+F7EF */ { 0xf769, 0x0308, 0 },
/* U+F7F0 */ { 0x00f0, 0x0000, 0 },
/* U+F7F1 */ { 0xf76e, 0x0303, 0 },
/* U+F7F2 */ { 0xf76f, 0x0300, 0 },
/* U+F7F3 */ { 0xf76f, 0x0301, 0 },
/* U+F7F4 */ { 0xf76f, 0x0302, 0 },
/* U+F7F5 */ { 0xf76f, 0x0303, 0 },
/* U+F7F6 */ { 0xf76f, 0x0308, 0 },
/* U+F7F7 */ { 0 },
/* U+F7F8 */ { 0x00f8, 0x0000, 0 },
/* U+F7F9 */ { 0xf775, 0x0300, 0 },
/* U+F7FA */ { 0xf775, 0x0301, 0 },
/* U+F7FB */ { 0xf775, 0x0302, 0 },
/* U+F7FC */ { 0xf775, 0x0308, 0 },
/* U+F7FD */ { 0xf779, 0x0301, 0 },
/* U+F7FE */ { 0x00fe, 0x0000, 0 },
/* U+F7FF */ { 0xf779, 0x0308, 0 }
};

static real SplineSetQuickTop(SplineSet *ss) {
    real max = -1e10;
    SplinePoint *sp;

    for ( ; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->me.y > max ) max = sp->me.y;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    if ( max<-65536 ) max = 0;	/* return a reasonable value for an empty glyph */
return( max );
}

static real SplineCharQuickTop(SplineChar *sc,int layer) {
    RefChar *ref;
    real max, temp;

    max = SplineSetQuickTop(sc->layers[layer].splines);
    for ( ref = sc->layers[layer].refs; ref!=NULL; ref = ref->next )
	if ( (temp =SplineSetQuickTop(ref->layers[0].splines))>max )
	    max = temp;
return( max );
}

int isaccent(int uni) {

    if ( uni<0x10000 && iscombining(uni) )
return( true );
    if ( uni>=0x2b0 && uni<0x2ff )
return( true );
    if ( uni=='.' || uni==',' || uni==0x60 || uni==0x5e || uni==0x7e ||
	    uni==0xa8 || uni==0xaf || uni==0xb8 || uni==0x384 || uni==0x385 ||
	    (uni>=0x1fbd && uni<=0x1fc1) ||
	    (uni>=0x1fcd && uni<=0x1fcf) ||
	    (uni>=0x1fed && uni<=0x1fef) ||
	    (uni>=0x1ffd && uni<=0x1fff) )
return( true );

return( false );
}

static int haschar(SplineFont *sf,unichar_t ch,char *dot) {
    char buffer[200], namebuf[200];

    if ( dot==NULL || ch==-1 )
return( SCWorthOutputting(SFGetChar(sf,ch,NULL)) );
    snprintf(buffer,sizeof(buffer),"%s%s",
	    (char *) StdGlyphName(namebuf,ch,sf->uni_interp,sf->for_new_glyphs),
	    dot);
    if ( SCWorthOutputting(SFGetChar(sf,-1,buffer)) )
return( true );
    else if ( isaccent(ch))
return( SCWorthOutputting(SFGetChar(sf,ch,NULL)) );
    else
return( false );
}

static SplineChar *GetChar(SplineFont *sf,unichar_t ch,char *dot) {
    char buffer[200], namebuf[200];
    SplineChar *sc;
    /* Basically the same as above */

    if ( dot==NULL || ch==-1 )
return( SFGetChar(sf,ch,NULL) );
    snprintf(buffer,sizeof(buffer),"%s%s",
	    (char *) StdGlyphName(namebuf,ch,sf->uni_interp,sf->for_new_glyphs),
	    dot);
    if ( (sc = SFGetChar(sf,-1,buffer)) )
return( sc );
    else if ( isaccent(ch))
return( SFGetChar(sf,ch,NULL) );
    else
return( NULL );
}

static const unichar_t *arabicfixup(SplineFont *sf, const unichar_t *upt, int ini, int final) {
    static unichar_t arabicalts[20];
    unichar_t *apt; const unichar_t *pt;

    for ( apt=arabicalts, pt = upt; *pt!='\0'; ++pt, ++apt ) {
	if ( *pt==' ' ) {
	    *apt = ' ';
	    ini = true;
	} else if ( *pt<0x600 || *pt>0x6ff )
	    *apt = *pt;
	else if ( ini ) {
	    *apt = ArabicForms[*pt-0x600].initial;
	    ini = false;
	} else if ( pt[1]==' ' || (pt[1]=='\0' && final))
	    *apt = ArabicForms[*pt-0x600].final;
	else
	    *apt = ArabicForms[*pt-0x600].medial;
	if ( !haschar(sf,*apt,NULL))
    break;
    }
    if ( *pt=='\0' ) {
	*apt = '\0';
return(arabicalts);
    }
return( upt );
}

static const unichar_t *SFAlternateFromLigature(SplineFont *sf, int base,SplineChar *sc) {
    static unichar_t space[30];
    unichar_t *spt, *send = space+sizeof(space)/sizeof(space[0])-1;
    char *ligstart, *semi, *pt, sch, ch, *dpt;
    int j;
    PST *pst;

    if ( sc==NULL && base>=0 )
	sc = SFGetChar(sf,base,NULL);
    pst = NULL;
    if ( sc!=NULL )
	for ( pst=sc->possub; pst!=NULL && pst->type!=pst_ligature; pst = pst->next );
    if ( sc==NULL || pst==NULL )
return( NULL );

    ligstart=pst->u.lig.components;
    semi = strchr(ligstart,';');
    if ( semi==NULL ) semi = ligstart+strlen(ligstart);
    sch = *semi; *semi = '\0';
    spt = space;
    dpt = NULL;
    for ( pt = ligstart; *pt!='\0'; ) {
	char *start = pt; dpt=NULL;
	for ( ; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	for ( j=0; j<sf->glyphcnt; ++j )
	    if ( sf->glyphs[j]!=NULL && strcmp(sf->glyphs[j]->name,start)==0 )
	break;
	*pt = ch;
	if ( j==sf->glyphcnt && (dpt=strchr(start,'.'))!=NULL && dpt<pt ) {
	    int dch = *dpt; *dpt='\0';
	    for ( j=0; j<sf->glyphcnt; ++j )
		if ( sf->glyphs[j]!=NULL && strcmp(sf->glyphs[j]->name,start)==0 )
	    break;
	    *dpt = dch;
	}
	if ( j>=sf->glyphcnt || sf->glyphs[j]->unicodeenc==-1 || spt>=send ) {
	    *semi = sch;
return( NULL );
	}
	*spt++ = sf->glyphs[j]->unicodeenc;
	if ( ch!='\0' ) ++pt;
    }
    *spt ='\0';
    *semi = sch;

    if ( *space>=0x0600 && *space<=0x6ff ) {
	int ini=0, final=0;
	if ( dpt==NULL || strncmp(dpt,".isolated",strlen(".isolated"))==0 )
	    ini=final = true;
	else if ( strncmp(dpt,".initial",strlen(".initial"))==0 )
	    ini=true;
	else if ( strncmp(dpt,".final",strlen(".final"))==0 )
	    final=true;
return( arabicfixup(sf,space,ini,final));
    }

return( space );
}

const unichar_t *SFGetAlternate(SplineFont *sf, int base,SplineChar *sc,int nocheck) {
    static unichar_t greekalts[5];
    const unichar_t *upt, *pt; unichar_t *gpt;
    char *dot = NULL;

    if ( sc!=NULL && (dot = strchr(sc->name,'.'))!=NULL ) {
	/* agrave.sc should be built from a.sc and grave or grave.sc */
	char *temp = copyn(sc->name,dot-sc->name);
	base = UniFromName(temp,sf->uni_interp,NULL);
	free(temp);
    }

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
    if ( base=='i' || base=='j' ) {
	if ( base=='i' )
	    greekalts[0] = 0x131;
	else if ( haschar(sf,0x237,NULL))
	    greekalts[0] = 0x237;		/* proposed dotlessj */
	else
	    greekalts[0] = 0xf6be;		/* Dotlessj in Adobe's private use area */
	greekalts[1] = 0x307;
	greekalts[2] = 0;
return( greekalts );
    }

    if ( sf->uni_interp==ui_adobe && base>=0xf600 && base<=0xf7ff &&
	    adobes_pua_alts[base-0xf600]!=0 )
return( adobes_pua_alts[base-0xf600]);

    if ( base==-1 || base>=65536 || unicode_alternates[base>>8]==NULL ||
	    (upt = unicode_alternates[base>>8][base&0xff])==NULL )
return( SFAlternateFromLigature(sf,base,sc));

	    /* The definitions of some of the greek letters may make some */
	    /*  linguistic sense, but I can't use it to place the accents */
	    /*  properly. So I redefine them here */
    if ( base>=0x1f00 && base<0x2000 ) {
	gpt = unicode_greekalts[base-0x1f00];
	if ( *gpt && (nocheck ||
		(haschar(sf,*gpt,dot) && (gpt[1]=='\0' || haschar(sf,gpt[1],dot))) ))
return( gpt );
	    /* Similarly for these (korean) jamo */
    } else if (( base>=0x1176 && base<=0x117e ) || (base>=0x119a && base<=0x119c)) {
	greekalts[0] = upt[1]; greekalts[1] = upt[0]; greekalts[2] = 0;
return( greekalts );
    } else if ( base>=0x380 && base<=0x3ff && upt!=NULL ) {
	/* Use precombined accents when possible */
	if ( base==0x390 || base==0x3b0 ) {
	    greekalts[0] = base==0x390?0x3b9:0x3c5;
	    greekalts[1] = 0x385;
	    greekalts[2] = '\0';
	    if ( nocheck || haschar(sf,greekalts[1],dot))
return( greekalts );
	}
	/* In version 3 of unicode tonos gets converted to acute, which it */
	/*  doesn't look like. Convert it back */
	for ( pt=upt ; *pt && *pt!=0x301; ++pt );
	if ( *pt ) {
	    for ( gpt=greekalts ; *upt ; ++upt ) {
		if ( *upt==0x301 )
		    *gpt++ = 0x384 /* I used to use 0x30d, thinking that it was a combining tonos, but greek users tell me it is not */;
		else
		    *gpt++ = *upt;
	    }
return( greekalts );
	}
    } else if (( base>=0xfb50 && base<=0xfdff ) || ( base>=0xfe70 && base<0xfeff ))
return( arabicfixup(sf,upt,isarabisolated(base)||isarabinitial(base),
		isarabisolated(base)||isarabfinal(base)));
    else if ( base==0x0122 || base==0x0136 || base==0x137 ||
	    base==0x013b || base==0x013c || base==0x0145 ||
	    base==0x0146 || base==0x0156 || base==0x0157 ) {
	/* Unicode says these use cedilla, but it looks like a comma below */
	greekalts[0] = base==0x0122 ? 'G' :
		       base==0x0136 ? 'K' :
		       base==0x0137 ? 'k' :
		       base==0x013b ? 'L' :
		       base==0x013c ? 'l' :
		       base==0x0145 ? 'N' :
		       base==0x0146 ? 'n' :
		       base==0x0156 ? 'R' :
			   'r';
	greekalts[1] = 0x326;
	greekalts[2] = '\0';
return( greekalts );
    } else if ( base==0x0123 ) {
	/* Unicode says this uses cedilla, but it looks like a turned comma above */
	greekalts[0] = 'g';
	greekalts[1] = 0x312;
	greekalts[2] = '\0';
return( greekalts );
    } else if ( base==0x010f || base==0x013d || base==0x013e || base==0x0165 ) {
	/* Unicode says these use caron, but it looks like comma above right */
	greekalts[0] =  base==0x010f ? 'd' :
			base==0x013d ? 'L' :
			base==0x013e ? 'l' :
			   't';
	greekalts[1] = 0x315;
	greekalts[2] = '\0';
return( greekalts );
    }

return( upt );
}

static SplineChar *GetGoodAccentGlyph(SplineFont *sf, int uni, int basech,
	int *invert,double ia, char *dot, SplineChar *destination);

int SFIsCompositBuildable(SplineFont *sf,int unicodeenc,SplineChar *sc,int layer) {
    const unichar_t *pt, *all; unichar_t ch, basech;
    SplineChar *one, *two;
    char *dot = NULL;
    int invert = false;

    if ( unicodeenc==0x131 || unicodeenc==0x0237 || unicodeenc==0xf6be )
return( SCMakeDotless(sf,SFGetOrMakeChar(sf,unicodeenc,NULL),layer,NULL,false,false));

    if ( sc!=NULL && (dot = strchr(sc->name,'.'))!=NULL ) {
	/* agrave.sc should be built from a.sc and grave or grave.sc */
	char *temp = copyn(sc->name,dot-sc->name);
	unicodeenc = UniFromName(temp,sf->uni_interp,NULL);
	free(temp);
    }

    if (( all = pt = SFGetAlternate(sf,unicodeenc,sc,false))==NULL )
return( false );

    if ( sc!=NULL )
	one = sc;
    else
	one=SFGetOrMakeChar(sf,unicodeenc,NULL);

    basech = *pt;
    for ( ; *pt; ++pt ) {
	ch = *pt;
	if ( all!=pt && isaccent(ch) ) {	/* first glyph is magic and doesn't go through accent processing */
	    two = GetGoodAccentGlyph(sf,ch,basech,&invert,sf->italicangle,dot,one);
	} else if ( !haschar(sf,ch,dot))
return( false );
	else
	    two = GetChar(sf,ch,dot);
	/* No recursive references */
	/* Cyrillic gamma could refer to Greek gamma, which the entry gives as an alternate */
	if ( one!=NULL && (two==NULL || SCDependsOnSC(two,one)) )
return( false );
    }
return( true );
}

int SFIsRotatable(SplineFont *sf,SplineChar *sc) {
    char *end;
    int cid;

    if ( sf->cidmaster!=NULL && strncmp(sc->name,"vertcid_",8)==0 ) {
	cid = strtol(sc->name+8,&end,10);
	if ( *end=='\0' && SFHasCID(sf,cid)!=-1)
return( true );
    } else if ( sf->cidmaster!=NULL && strstr(sc->name,".vert")!=NULL &&
	    (cid = CIDFromName(sc->name,sf->cidmaster))!= -1 ) {
	if ( SFHasCID(sf,cid)!=-1)
return( true );
    } else if ( strncmp(sc->name,"vertuni",7)==0 && strlen(sc->name)==11 ) {
	int uni = strtol(sc->name+7,&end,16);
	if ( *end=='\0' && SFCIDFindExistingChar(sf,uni,NULL)!=-1 )
return( true );
    } else if ( strncmp(sc->name,"uni",3)==0 && strstr(sc->name,".vert")!=NULL ) {
	int uni = strtol(sc->name+3,&end,16);
	if ( *end=='.' && SFCIDFindExistingChar(sf,uni,NULL)!=-1 )
return( true );
    } else if ( sc->name[0]=='u' && strstr(sc->name,".vert")!=NULL ) {
	int uni = strtol(sc->name+1,&end,16);
	if ( *end=='.' && SFCIDFindExistingChar(sf,uni,NULL)!=-1 )
return( true );
    } else if ( strstr(sc->name,".vert")!=NULL || strstr(sc->name,".vrt2")!=NULL) {
	int ret;
	char *temp;
	end = strchr(sc->name,'.');
	temp = copyn(sc->name,end-sc->name);
	ret = SFFindExistingSlot(sf,-1,temp)!=-1;
	free(temp);
return( ret );
    }
return( false );
}

int hascomposing(SplineFont *sf,int u,SplineChar *sc) {
    const unichar_t *upt = SFGetAlternate(sf,u,sc,false);

    if ( upt!=NULL ) {
	while ( *upt ) {
	    if ( (u==0x13f || u==0x140 ) && *upt==0xb7 )
return( true );	/* b7, centered dot is used as a combining accent for Ldot */
		/* It's spacing for ldot (but we want to pretend it's an accent) */
		/*  and 0x22EF is a ligature of ... */
	    else if ( iscombining(*upt) ||
		    *upt==0x0384 ||	/* tonos */
		    *upt==0x0385 ||	/* dieresis/tonos */
		    *upt==0x1ffe || *upt==0x1fbf || *upt==0x1fcf || *upt==0x1fdf ||
		    *upt==0x1fbd || *upt==0x1fef || *upt==0x1fc0 || *upt==0x1fc1 ||
		    *upt==0x1fee || *upt==0x1ffd || *upt==0x1fbe || *upt==0x1fed ||
		    *upt==0x1fcd || *upt==0x1fdd || *upt==0x1fce || *upt==0x1fde )	/* Special greek accents */
return( true );
	    /* Only build Jongsung out of chosung when doing a build composit */
	    /*  not when doing a build accented (that's what the upt[1]!='\0' */
	    /*  means */
	    if ( *upt>=0x1100 && *upt<0x11c7 && upt[1]!='\0' )
return( true );
	    ++upt;
	}

	if ( u>=0x1f70 && u<0x1f80 )
return( true );			/* Yes. they do work, I don't care what it looks like */
	if ( u==0x0149 )
return( true );
    }
return( false );
}

int SFIsSomethingBuildable(SplineFont *sf,SplineChar *sc, int layer, int onlyaccents) {
    int unicodeenc = sc->unicodeenc;

    if ( onlyaccents &&		/* Don't build greek accents out of latin ones */
	    (unicodeenc==0x1fbd || unicodeenc==0x1fbe || unicodeenc==0x1fbf ||
	     unicodeenc==0x1fef || unicodeenc==0x1ffd || unicodeenc==0x1ffe))
return( false );

    if ((unicodeenc <= 0xffff && iszerowidth(unicodeenc)) ||
	    (unicodeenc>=0x2000 && unicodeenc<=0x2015 ))
return( !onlyaccents );

    if ( SFIsCompositBuildable(sf,unicodeenc,sc,layer))
return( !onlyaccents || hascomposing(sf,sc->unicodeenc,sc) );

    if ( !onlyaccents && SCMakeDotless(sf,sc,layer,NULL,false,false))
return( true );

return( SFIsRotatable(sf,sc));
}

static int SPInRange(SplinePoint *sp, real ymin, real ymax) {
return ( sp->me.y>=ymin && sp->me.y<=ymax );
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

static real _SplineSetFindXRangeAtYExtremum(SplinePointList *spl, DBounds *bounds,
	int findymax, real yextreme) {
    Spline *spline;
    extended t0, t1, t2, t3;
    bigreal y0, y1, y2, y3, x;

    for ( ; spl!=NULL; spl = spl->next ) {
	for ( spline = spl->first->next; spline!=NULL; spline=spline->to->next ) {
	    if (( findymax && 
		    !(yextreme>spline->from->me.y && yextreme>spline->from->nextcp.y &&
		      yextreme>spline->to->me.y && yextreme>spline->to->prevcp.y )) ||
		(!findymax &&
		    !(yextreme<spline->from->me.y && yextreme<spline->from->nextcp.y &&
		      yextreme<spline->to->me.y && yextreme<spline->to->prevcp.y )) ) {
		/* Ok this spline might be bigger(less) than our current */
		/*  extreme value, so check it */
		SplineFindExtrema(&spline->splines[1],&t1,&t2);
		y0 = spline->from->me.y; t0 = 0;
		y3 = spline->to->me.y; t3 = 1;
		if ( t1!=-1 ) {
		    y1 = ((spline->splines[1].a*t1+spline->splines[1].b)*t1+spline->splines[1].c)*t1+spline->splines[1].d;
		    if ( y1>y0 ) { y0 = y1; t0 = t1; }
		}
		if ( t2!=-1 ) {
		    y2 = ((spline->splines[1].a*t2+spline->splines[1].b)*t2+spline->splines[1].c)*t2+spline->splines[1].d;
		    if ( y2>y3 ) { y3 = y2; t3 = t2; }
		}
		if ( (findymax && y0>yextreme+.1) || (!findymax && y0<yextreme-.1) ) {
		    bounds->miny = bounds->maxy = yextreme = y0;
		    bounds->minx = bounds->maxx =
			    ((spline->splines[0].a*t0+spline->splines[0].b)*t0+spline->splines[0].c)*t0+spline->splines[0].d;
		} else if ( (findymax && y0>=yextreme-.1) || (!findymax && y0<=yextreme+1) ) {
		    x = ((spline->splines[0].a*t0+spline->splines[0].b)*t0+spline->splines[0].c)*t0+spline->splines[0].d;
		    if ( x>bounds->maxx ) bounds->maxx = x;
		    else if ( x<bounds->minx ) bounds->minx = x;
		}
		if ( (findymax && y3>yextreme+.1) || (!findymax && y3<yextreme-.1) ) {
		    bounds->miny = bounds->maxy = yextreme = y3;
		    bounds->minx = bounds->maxx =
			    ((spline->splines[0].a*t3+spline->splines[0].b)*t3+spline->splines[0].c)*t3+spline->splines[0].d;
		} else if ( (findymax && y3>=yextreme-.1) || (!findymax && y3<=yextreme+1) ) {
		    x = ((spline->splines[0].a*t3+spline->splines[0].b)*t3+spline->splines[0].c)*t3+spline->splines[0].d;
		    if ( x>bounds->maxx ) bounds->maxx = x;
		    else if ( x<bounds->minx ) bounds->minx = x;
		}
	    }
	    if ( spline->to == spl->first )
	break;
	}
    }
return( yextreme );
}

/* this is called by the accented character routines with bounds set for the */
/*  entire character. Our job is to make a guess at what the top of the */
/*  character looks like so that we can do an optical accent placement */
/* I currently think the best bet is to find the very highest point(s) and */
/*  center on that. I used to find the bounds of the top quarter of the char */
static real SCFindTopXRange(SplineChar *sc,int layer,DBounds *bounds) {
    RefChar *rf;
    real yextreme = -0x80000;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;

    for ( rf=sc->layers[layer].refs; rf!=NULL; rf = rf->next )
	yextreme = _SplineSetFindXRangeAtYExtremum(rf->layers[0].splines,bounds,true,yextreme);

    yextreme = _SplineSetFindXRangeAtYExtremum(sc->layers[layer].splines,bounds,true,yextreme);
    if ( yextreme == -0x80000 ) yextreme = 0;
return( yextreme );
}

static real SCFindBottomXRange(SplineChar *sc,int layer,DBounds *bounds, real ia) {
    RefChar *rf;
    real yextreme = 0x80000;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;

    for ( rf=sc->layers[layer].refs; rf!=NULL; rf = rf->next )
	yextreme = _SplineSetFindXRangeAtYExtremum(rf->layers[0].splines,bounds,false,yextreme);

    yextreme = _SplineSetFindXRangeAtYExtremum(sc->layers[layer].splines,bounds,false,yextreme);
    if ( yextreme == 0x80000 ) yextreme = 0;
return( yextreme );
}

/* the cedilla and ogonec accents do not center on the accent itself but on */
/*  the small part of it that joins at the top */
static real SCFindTopBounds(SplineChar *sc,int layer,DBounds *bounds, real ia) {
    RefChar *rf;
    int ymax = bounds->maxy+1, ymin = ymax-(bounds->maxy-bounds->miny)/20;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;

    for ( rf=sc->layers[layer].refs; rf!=NULL; rf = rf->next )
	_SplineSetFindXRange(rf->layers[0].splines,bounds,ymin,ymax,ia);

    _SplineSetFindXRange(sc->layers[layer].splines,bounds,ymin,ymax,ia);
return( ymin );
}

/* And similarly for the floating hook, and often for grave and acute */
static real SCFindBottomBounds(SplineChar *sc,int layer,DBounds *bounds, real ia) {
    RefChar *rf;
    int ymin = bounds->miny-1, ymax = ymin+(bounds->maxy-bounds->miny)/20;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;

    for ( rf=sc->layers[layer].refs; rf!=NULL; rf = rf->next )
	_SplineSetFindXRange(rf->layers[0].splines,bounds,ymin,ymax,ia);

    _SplineSetFindXRange(sc->layers[layer].splines,bounds,ymin,ymax,ia);
return( ymin );
}

static real SplineCharFindSlantedBounds(SplineChar *sc,int layer,DBounds *bounds, real ia) {
    int ymin, ymax;
    RefChar *rf;

    SplineCharFindBounds(sc,bounds);

    ymin = bounds->miny-1, ymax = bounds->maxy+1;

    if ( ia!=0 ) {
	bounds->minx = bounds->maxx = 0;

	for ( rf=sc->layers[layer].refs; rf!=NULL; rf = rf->next )
	    _SplineSetFindXRange(rf->layers[0].splines,bounds,ymin,ymax,ia);

	_SplineSetFindXRange(sc->layers[layer].splines,bounds,ymin,ymax,ia);
    }
return( ymin );
}

static int SCStemCheck(SplineFont *sf,int layer,int basech,DBounds *bb, DBounds *rbb,int pos) {
    /* cedilla (and ogonec on A) should be centered underneath the left */
    /*  (or right) vertical (or diagonal) stem. Here we try to find that */
    /*  stem */
    StemInfo *h, *best;
    SplineChar *sc;
    DStemInfo *d;

    sc = SFGetChar(sf,basech,NULL);
    if ( sc==NULL )
return( 0x70000000 );
    if ( autohint_before_generate && sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,layer,NULL);
    if ( (best=sc->vstem)!=NULL ) {
	if ( pos & FF_UNICODE_CENTERLEFT ) {
	    for ( h=best->next; h!=NULL && h->start<best->start+best->width; h=h->next )
		if ( h->start+h->width<best->start+best->width )
		    best = h;
	    if ( best->start+best->width/2>(bb->maxx+bb->minx)/2 )
		best = NULL;
	} else if ( pos & FF_UNICODE_CENTERRIGHT ) {
	    while ( best->next!=NULL )
		best = best->next;
	    if ( best->start+best->width/2<(bb->maxx+bb->minx)/2 )
		best = NULL;
	} else {
	    for ( h=best->next; h!=NULL ; h=h->next )
		if ( HIlen(h)>HIlen(best))
		    best = h;
	}
	if ( best!=NULL )
return( best->start + best->width/2 - (rbb->maxx-rbb->minx)/2 - rbb->minx );
    }
    if ( sc->dstem != NULL ) {
        double himin, himax, hibase, roff, temp;
        double lbx = 0, rbx = 0, lbxtest, rbxtest;
        HintInstance *hi;
	for ( d=sc->dstem; d!=NULL ; d=d->next ) {
            if ( d->where == NULL )
        continue;
            if ( d->where->begin < d->where->end ) {
                himin = d->where->begin;
                himax = d->where->end;
            } else {
                himin = d->where->end;
                himax = d->where->begin;
            }
            for ( hi=d->where->next; hi!=NULL; hi=hi->next ) {
                if ( hi->begin < himin ) himin = hi->begin;
                if ( hi->end < himin ) himin = hi->end;
                if ( hi->end > himax ) himax = hi->end;
                if ( hi->begin > himax ) himax = hi->begin;
            }
            hibase = ( d->unit.y > 0 ) ? himin : himax;
            roff =  ( d->right.x - d->left.x ) * d->unit.x + 
                    ( d->right.y - d->left.y ) * d->unit.y;
            lbxtest = d->left.x + d->unit.x * hibase;
            rbxtest = d->right.x + d->unit.x * ( hibase - roff );
            if ( rbxtest < lbxtest ) {
                temp = rbxtest; rbxtest = lbxtest; lbxtest = temp;
            }
            if ( d == sc->dstem ||
                ( ( pos & FF_UNICODE_CENTERLEFT ) && rbxtest < rbx ) ||
                ( ( pos & FF_UNICODE_CENTERRIGHT ) && lbxtest > lbx ) ) {
                lbx = lbxtest; rbx = rbxtest;
            }
        }
        if ( lbx < rbx && (
            ( ( pos & FF_UNICODE_CENTERLEFT ) &&
            ( lbx + rbx )/2 <= ( bb->maxx + bb->minx )/2 ) ||
            ( ( pos & FF_UNICODE_CENTERRIGHT ) &&
            ( lbx + rbx )/2 >= ( bb->maxx + bb->minx )/2 )))
return(( lbx + rbx )/2 - ( rbb->maxx - rbb->minx )/2 - rbb->minx );
    }
return( 0x70000000 );
}

void _SCAddRef(SplineChar *sc,SplineChar *rsc,int layer,real transform[6]) {
    RefChar *ref = RefCharCreate();

    ref->sc = rsc;
    ref->unicode_enc = rsc->unicodeenc;
    ref->orig_pos = rsc->orig_pos;
    ref->adobe_enc = getAdobeEnc(rsc->name);
    ref->next = sc->layers[layer].refs;
    sc->layers[layer].refs = ref;
    memcpy(ref->transform,transform,sizeof(real [6]));
    SCReinstanciateRefChar(sc,ref,layer);
    SCMakeDependent(sc,rsc);
}

void SCAddRef(SplineChar *sc,SplineChar *rsc,int layer, real xoff, real yoff) {
    real transform[6];
    transform[0] = transform[3] = 1;
    transform[1] = transform[2] = 0;
    transform[4] = xoff; transform[5] = yoff;
    _SCAddRef(sc,rsc,layer,transform);
}

static void BCClearAndCopyBelow(BDFFont *bdf,int togid,int fromgid, int ymax) {
    BDFChar *bc, *rbc;

    bc = BDFMakeGID(bdf,togid);
    BCPreserveState(bc);
    BCFlattenFloat(bc);
    BCCompressBitmap(bc);
    if ( bdf->glyphs[fromgid]!=NULL ) {
	rbc = bdf->glyphs[fromgid];
	free(bc->bitmap);
	bc->xmin = rbc->xmin;
	bc->xmax = rbc->xmax;
	bc->ymin = rbc->ymin;
	bc->ymax = ymax;
	bc->bytes_per_line = rbc->bytes_per_line;
	bc->width = rbc->width;
	bc->bitmap = malloc(bc->bytes_per_line*(bc->ymax-bc->ymin+1));
	memcpy(bc->bitmap,rbc->bitmap+(rbc->ymax-ymax)*rbc->bytes_per_line,
		bc->bytes_per_line*(bc->ymax-bc->ymin+1));
    }
}

static void BCAddReference( BDFChar *bc,BDFChar *rbc,int gid,int xoff,int yoff ) {
    BDFRefChar *bcref;

    bcref = calloc( 1,sizeof( BDFRefChar ));
    bcref->bdfc = rbc; bcref->gid = gid;
    bcref->xoff = xoff; bcref->yoff = yoff;
    bcref->next = bc->refs; bc->refs = bcref;
    BCMakeDependent( bc,rbc );
}

static int BCFindGap(BDFChar *bc) {
    int i,y;

    BCFlattenFloat(bc);
    BCCompressBitmap(bc);
    for ( y=bc->ymax; y>=bc->ymin; --y ) {
	for ( i=0; i<bc->bytes_per_line; ++i )
	    if ( bc->bitmap[(bc->ymax-y)*bc->bytes_per_line+i]!=0 )
	break;
	if ( i==bc->bytes_per_line )
return( y );
    }
return( bc->ymax );
}

static int SCMakeBaseReference(SplineChar *sc,SplineFont *sf,int layer,int ch, BDFFont *bdf,int disp_only) {
    SplineChar *rsc;
    BDFChar *bc;
    char *dot;
    char buffer[200], namebuf[200];

    if ( (dot = strchr(sc->name,'.'))!=NULL ) {
	snprintf(buffer,sizeof(buffer),"%s%s",
		(char *) StdGlyphName(namebuf,ch,sf->uni_interp,sf->for_new_glyphs),
		dot);
	rsc = SFGetChar(sf,-1,buffer);
    } else
	rsc = SFGetChar(sf,ch,NULL);

    if ( rsc==NULL && dot==NULL ) {
	if ( ch==0x131 || ch==0xf6be || ch==0x237) {
	    if ( ch==0x131 ) ch='i'; else ch = 'j';
	    rsc = SFGetChar(sf,ch,NULL);
	    if ( rsc!=NULL && !sf->dotlesswarn ) {
/* GT: the string "dotlessi" is the official postscript name for the glyph */
/* GT: containing an "i" without a dot on top of it. The name "dotlessi" */
/* GT: should be left untranslated, though you may wish to explain what it */
/* GT: means */
		ff_post_error( _("Missing Glyph..."),ch=='i'?_("Your font is missing the dotlessi glyph,\nplease add it and remake your accented glyphs"):
/* GT: Adobe decided that a dotless j glyph was needed, assigned a code */
/* GT: point to it in the private use area, and named it "dotlessj". Then */
/* GT: years later the Unicode people decided one was needed and assigned */
/* GT: it U+0237, so that's now the official code point and it is named */
/* GT: "uni0237". The name "dotlessj" is deprecated but still present in */
/* GT: many fonts. Neither "dotlessj" nor "uni0237" should be translated */
/* GT: because they are standard PostScript glyph names. */
/* GT: Again you may wish to explain that these refer to a "j" without a dot */
			_("Your font is missing the uni0237 glyph,\nand the deprecated dotlessj glyph,\nplease add the former and remake your accented glyphs"));
		sf->dotlesswarn = true;
	    }
	}
    }
    if ( rsc==NULL )
return( 0 );
    if ( bdf == NULL || !disp_only ) {
	sc->width = rsc->width;
	if ( ch!=' ' )			/* Some accents are built by combining with space. Don't. The reference won't be displayed or selectable but will be there and might cause confusion... */
	    SCAddRef(sc,rsc,layer,0,0);	/* should be after the call to MakeChar */
    }
    if ( !disp_only ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->glyphs[rsc->orig_pos] != NULL ) {
		if (( bc = bdf->glyphs[sc->orig_pos] ) == NULL ) {
		    bc = BDFMakeGID(bdf,sc->orig_pos);
		    BCClearAll( bc );
		}
		if ( ch!=' ' )
		    BCAddReference( bc,bdf->glyphs[rsc->orig_pos],rsc->orig_pos,0,0 );
	    }
	}
    } else if ( bdf != NULL ) {
	if ( bdf->glyphs[rsc->orig_pos] != NULL ) {
	    if (( bc = bdf->glyphs[sc->orig_pos] ) == NULL ) {
		bc = BDFMakeGID(bdf,sc->orig_pos);
		BCClearAll( bc );
	    }
	    if ( ch!=' ' )
		BCAddReference( bc,bdf->glyphs[rsc->orig_pos],rsc->orig_pos,0,0 );
	}
    }
return( 1 );
}

static SplineChar *GetGoodAccentGlyph(SplineFont *sf, int uni, int basech,
	int *invert,double ia, char *dot, SplineChar *destination) {
    int ach= -1;
    const unichar_t *apt, *end;
    SplineChar *rsc;
    char buffer[200], namebuf[200];

    *invert = false;

    /* cedilla on lower "g" becomes a turned comma above it */
    if ( uni==0x327 && basech=='g' && haschar(sf,0x312, dot))
	uni = 0x312;
    if ( !PreferSpacingAccents && ((haschar(sf,uni, dot) &&
		!SCDependsOnSC(GetChar(sf,uni,dot),destination)) ||
	    (dot!=NULL && haschar(sf,uni, NULL) &&
		!SCDependsOnSC(GetChar(sf,uni,NULL),destination))) )
	ach = uni;
    else if ( uni>=BottomAccent && uni<=TopAccent ) {
	apt = accents[uni-BottomAccent]; end = apt+sizeof(accents[0])/sizeof(accents[0][0]);
	while ( *apt && apt<end &&
		(             GetChar(sf,*apt,dot) ==NULL || SCDependsOnSC(GetChar(sf,*apt,dot),destination)) &&
		(dot==NULL || GetChar(sf,*apt,NULL)==NULL || SCDependsOnSC(GetChar(sf,*apt,NULL),destination)) )
	    ++apt;
	if ( *apt!='\0' && apt<end )
	    ach = *apt;
	else if ( haschar(sf,uni,dot) && !SCDependsOnSC(GetChar(sf,uni,dot),destination))
	    ach = uni;
	else if ( uni==0x31b && haschar(sf,',',NULL))
	    ach = ',';
	else if (( uni == 0x32f || uni == 0x311 ) && haschar(sf,0x2d8,NULL) && ia==0 ) {
	    /* In italic fonts invert is not enough, you must do more */
	    ach = 0x2d8;
	    *invert = true;
	} else if ( (uni == 0x30c || uni == 0x32c ) &&
		(haschar(sf,0x302,dot) || haschar(sf,0x2c6,NULL) || haschar(sf,'^',NULL)) ) {
	    *invert = true;
	    if ( haschar(sf,0x2c6,NULL))
		ach = 0x2c6;
	    else if ( haschar(sf,'^',NULL) )
		ach = '^';
	    else
		ach = 0x302;
	}
    } else
	ach = uni;

    rsc = NULL;
    if ( dot!=NULL ) {
	snprintf(buffer,sizeof(buffer),"%s%s",
		(char *) StdGlyphName(namebuf,ach,sf->uni_interp,sf->for_new_glyphs),
		dot);
	rsc = SFGetChar(sf,-1,buffer);
    }
    if ( rsc==NULL ) {
	rsc = SFGetChar(sf,ach,NULL);
	dot = NULL;
    }

    /* Look to see if there are upper case variants of the accents available */
    if ( dot==NULL && ( isupper(basech) || (basech>=0x400 && basech<=0x52f) )) {
	char *uc_accent;
	SplineChar *test = NULL;
	char buffer[80];
	char *suffixes[4];
	int scnt=0, i;

	if ( rsc!=NULL ) {
	    uc_accent = malloc(strlen(rsc->name)+11);
	    strcpy(uc_accent,rsc->name);
	} else
	    uc_accent = NULL;
	memset(suffixes,0,sizeof(suffixes));
	if ( basech>=0x400 && basech<=0x52f ) {
	    if ( isupper(basech) )
		suffixes[scnt++] = "cyrcap";
	    suffixes[scnt++] = "cyr";
	}
	if ( isupper(basech))
	    suffixes[scnt++] = "cap";

	for ( i=0; test==NULL && i<scnt; ++i ) {
	    if ( uni>=BottomAccent && uni<=TopAccent ) {
		apt = accents[uni-BottomAccent]; end = apt+sizeof(accents[0])/sizeof(accents[0][0]);
		while ( test==NULL && apt<end ) {
		    int acc = *apt ? *apt : uni;
		    sprintf( buffer,"%.70s.%s", StdGlyphName(buffer,acc,ui_none,(NameList *) -1), suffixes[i]);
		    if ( (test = SFGetChar(sf,-1,buffer))!=NULL )
			rsc = test;
		    if ( test==NULL ) {
			sprintf( buffer,"uni%04X.%s", acc, suffixes[i]);
			if ( (test = SFGetChar(sf,-1,buffer))!=NULL )
			    rsc = test;
		    }
		    if ( *apt=='\0' )
		break;
		    ++apt;
		}
	    }
	}
	if ( test==NULL && uni>=BottomAccent && uni<=TopAccent && isupper(basech)) {
	    apt = accents[uni-BottomAccent]; end = apt+sizeof(accents[0])/sizeof(accents[0][0]);
	    while ( test==NULL && apt<end ) {
		int acc = *apt ? *apt : uni;
		sprintf( buffer,"%.70s.%s", StdGlyphName(buffer,acc,ui_none,(NameList *) -1), suffixes[i]);
		if ( islower(buffer[0])) {
		    buffer[0] = toupper(buffer[0]);
		    if ( (test = SFGetChar(sf,-1,buffer))!=NULL )
			rsc = test;
		}
		if ( *apt=='\0' )
	    break;
		++apt;
	    }
	}
	if ( test==NULL && uni>=BottomAccent && uni<BottomAccent+sizeof(uc_accent_names)/sizeof(uc_accent_names[0]) &&
		uc_accent_names[uni-BottomAccent]!=NULL && isupper(basech))
	    if ( (test = SFGetChar(sf,-1,uc_accent_names[uni-BottomAccent]))!=NULL )
		rsc = test;
	if ( test==NULL && uc_accent!=NULL && islower(*uc_accent) && isupper(basech)) {
	    *uc_accent = toupper(*uc_accent);
	    if ( (test=SFGetChar(sf,-1,uc_accent))!=NULL )
		rsc = test;
	}
	free(uc_accent);
    }
    if ( rsc!=NULL && SCDependsOnSC(rsc,destination))
	rsc = NULL;
return( rsc );
}

static void TurnOffUseMyMetrics(SplineChar *sc) {
    /* When building greek polytonic glyphs, or any glyphs where the accents */
    /*  change the metrics, we should turn off "use my metrics" on the base */
    /*  reference */
    RefChar *refs;
    int ly;

    for ( ly=ly_fore; ly<sc->layer_cnt; ++ly )
	for ( refs = sc->layers[ly].refs; refs!=NULL; refs = refs->next )
	    refs->use_my_metrics = false;
}

AnchorClass *AnchorClassMatch(SplineChar *sc1,SplineChar *sc2,AnchorClass *restrict_,
	AnchorPoint **_ap1,AnchorPoint **_ap2 ) {
    AnchorPoint *ap1, *ap2;

    for ( ap1=sc1->anchor; ap1!=NULL ; ap1=ap1->next ) if ( restrict_==(AnchorClass *) -1 || ap1->anchor==restrict_ ) {
	for ( ap2=sc2->anchor; ap2!=NULL; ap2=ap2->next ) if ( restrict_==(AnchorClass *) -1 || ap2->anchor==restrict_ ) {
	    if ( ap1->anchor==ap2->anchor &&
		    ((ap1->type>=at_basechar && ap1->type<=at_basemark && ap2->type==at_mark) ||
		     (ap1->type==at_cexit && ap2->type==at_centry) )) {
		 *_ap1 = ap1;
		 *_ap2 = ap2;
return( ap1->anchor );
	    }
	}
    }
return( NULL );
}

AnchorClass *AnchorClassMkMkMatch(SplineChar *sc1,SplineChar *sc2,
	AnchorPoint **_ap1,AnchorPoint **_ap2 ) {
    AnchorPoint *ap1, *ap2;

    for ( ap1=sc1->anchor; ap1!=NULL ; ap1=ap1->next ) {
	for ( ap2=sc2->anchor; ap2!=NULL; ap2=ap2->next ) {
	    if ( ap1->anchor==ap2->anchor &&
		    ap1->type==at_basemark && ap2->type==at_mark)  {
		 *_ap1 = ap1;
		 *_ap2 = ap2;
return( ap1->anchor );
	    }
	}
    }
return( NULL );
}

AnchorClass *AnchorClassCursMatch(SplineChar *sc1,SplineChar *sc2,
	AnchorPoint **_ap1,AnchorPoint **_ap2 ) {
    AnchorPoint *ap1, *ap2;

    for ( ap1=sc1->anchor; ap1!=NULL ; ap1=ap1->next ) {
	for ( ap2=sc2->anchor; ap2!=NULL; ap2=ap2->next ) {
	    if ( ap1->anchor==ap2->anchor &&
		    ap1->type==at_cexit && ap2->type==at_centry)  {
		 *_ap1 = ap1;
		 *_ap2 = ap2;
return( ap1->anchor );
	    }
	}
    }
return( NULL );
}

static void _BCCenterAccent( BDFFont *bdf, int gid, int rgid, int ch, int basech, int italicoff,
	uint32 pos,	/* unicode char position info, see #define for utype2[] in utype.h */
	real em ) {
    BDFChar *bc, *rbc;
    int ixoff, iyoff, ispacing;
    IBounds ibb, irb;
    
    if (( rbc = bdf->glyphs[rgid] ) != NULL && ( bc = bdf->glyphs[gid] ) != NULL ) {
	if ( (ispacing = (bdf->pixelsize*accent_offset+50)/100)<=1 ) ispacing = 2;
	BCFlattenFloat(rbc);
	BCCompressBitmap(rbc);
	BDFCharQuickBounds( bc,&ibb,0,0,false,true );
	BDFCharQuickBounds( rbc,&irb,0,0,false,true );

	if ( (pos & FF_UNICODE_ABOVE) && (pos & (FF_UNICODE_LEFT|FF_UNICODE_RIGHT)) )
	    iyoff = ibb.maxy - irb.maxy;
	else if ( pos & FF_UNICODE_ABOVE )
	    iyoff = ibb.maxy + ispacing - irb.miny;
	else if ( pos & FF_UNICODE_BELOW ) {
	    iyoff = ibb.miny - irb.maxy;
	    if ( !( pos & FF_UNICODE_TOUCHING) )
		iyoff -= ispacing;
	} else if ( pos & FF_UNICODE_OVERSTRIKE )
	    iyoff = ibb.miny - irb.miny + ((ibb.maxy-ibb.miny)-(irb.maxy-irb.miny))/2;
	else
	    iyoff = ibb.miny - irb.miny;
	if ( isupper(basech) && ch==0x342)
	    ixoff = ibb.minx - irb.minx;
	else if ( pos & FF_UNICODE_LEFT )
	    ixoff = ibb.minx - ispacing - irb.maxx;
	else if ( pos & FF_UNICODE_RIGHT ) {
	    ixoff = ibb.maxx - irb.minx + ispacing/2;
	    if ( !( pos & FF_UNICODE_TOUCHING) )
		ixoff += ispacing;
	} else {
	    if ( pos & FF_UNICODE_CENTERLEFT )
		ixoff = ibb.minx + (ibb.maxx-ibb.minx)/2 - irb.maxx;
	    else if ( pos & FF_UNICODE_LEFTEDGE )
		ixoff = ibb.minx - irb.minx;
	    else if ( pos & FF_UNICODE_CENTERRIGHT )
		ixoff = ibb.minx + (ibb.maxx-ibb.minx)/2 - irb.minx;
	    else if ( pos & FF_UNICODE_RIGHTEDGE )
		ixoff = ibb.maxx - irb.maxx;
	    else
		ixoff = ibb.minx - irb.minx + ((ibb.maxx-ibb.minx)-(irb.maxx-irb.minx))/2;
	}
	ixoff += rint( italicoff*bdf->pixelsize/em );
	BCAddReference( bc,rbc,rgid,ixoff,iyoff );
    }
}

static void _SCCenterAccent(SplineChar *sc,SplineChar *basersc, SplineFont *sf,
	int layer, int ch, BDFFont *bdf, int disp_only,
	SplineChar *rsc, real ia, int basech,
	int invert,	/* invert accent, false==0, true!=0 */
	uint32 pos	/* unicode char position info, see #define for utype2[] in utype.h */ ) {
    real transform[6];
    DBounds bb, rbb, bbb;
    real xoff, yoff;
    real spacing = (sf->ascent+sf->descent)*accent_offset/100;
    real ybase, italicoff;
    const unichar_t *temp;
    int baserch = basech;
    int eta;
    AnchorPoint *ap1, *ap2;

    if ( rsc==NULL || sc==NULL )
return;

    /* When we center an accent on Uhorn, we don't really want it centered */
    /*  on the combination, we want it centered on "U". So if basech is itself*/
    /*  a combo, find what it is based on */
    if ( (temp = SFGetAlternate(sf,basech,NULL,false))!=NULL && haschar(sf,*temp,NULL))
	baserch = *temp;
    /* Similarly in Ø or ø, we really want to base the accents on O or o */
    if ( baserch==0xf8 ) baserch = 'o';
    else if ( baserch==0xd8 ) baserch = 'O';

    SplineCharFindSlantedBounds(rsc,layer,&rbb,ia);
    if ( ch==0x328 || ch==0x327 ) {	/* cedilla and ogonek */
	SCFindTopBounds(rsc,layer,&rbb,ia);
	/* should do more than touch, should overlap a tiny bit... */
	rbb.maxy -= (rbb.maxy-rbb.miny)/30;
    } else if ( ch==0x345 ) {	/* ypogegrammeni */
	SCFindTopBounds(rsc,layer,&rbb,ia);
    } else if ( (GraveAcuteCenterBottom && (ch==0x300 || ch==0x301 || ch==0x30b || ch==0x30f)) || ch==0x309 )	/* grave, acute, hungarian, Floating hook */
	SCFindBottomBounds(rsc,layer,&rbb,ia);
    else if ( basech=='A' && ch==0x30a )
	/* Again, a tiny bit of overlap is usual for Aring */
	rbb.miny += (rbb.maxy-rbb.miny)/30;
    ybase = SplineCharFindSlantedBounds(sc,layer,&bb,ia);
    if ( basersc==NULL ) {
	if ( baserch!=basech && ( basersc = SFGetChar(sf,baserch,NULL))!=NULL )
	    /* Do Nothing */;
	else if ( basersc == NULL || (basech!=-1 && (basech==sc->unicodeenc || ( basersc = SFGetChar(sf,basech,NULL))==NULL )))
	    basersc = sc;
    }
    if ( ia==0 && baserch!=basech && basersc!=NULL ) {
	ybase = SplineCharFindSlantedBounds(basersc,layer,&bbb,ia);
	if ( (ffUnicodeUtype2(ch) & (FF_UNICODE_ABOVE|FF_UNICODE_BELOW)) ) {
	    /* if unicode.org character definition matches ABOVE or BELOW, then... */
	    bbb.maxy = bb.maxy;
	    bbb.miny = bb.miny;
	}
	if ( (ffUnicodeUtype2(ch) & (FF_UNICODE_RIGHT|FF_UNICODE_LEFT)) ) {
	    /* if unicode.org character definition matches RIGHT or LEFT, then... */
	    bbb.maxx = bb.maxx;
	    bbb.minx = bb.minx;
	}
	bb = bbb;
    }

    transform[0] = transform[3] = 1;
    transform[1] = transform[2] = transform[4] = transform[5] = 0;

    italicoff = 0;
    if ( sc->layers[layer].refs!=NULL && sc->layers[layer].refs->next!=NULL &&
	    (AnchorClassMkMkMatch(sc->layers[layer].refs->sc,rsc,&ap1,&ap2)!=NULL ||
	     AnchorClassCursMatch(sc->layers[layer].refs->sc,rsc,&ap1,&ap2)!=NULL) ) {
	/* Do we have a mark to mark attachment to the last anchor we added? */
	/*  (or a cursive exit to entry match?) */
	/*  If so then figure offsets relative to it. */
	xoff = ap1->me.x-ap2->me.x + sc->layers[layer].refs->transform[4];
	yoff = ap1->me.y-ap2->me.y + sc->layers[layer].refs->transform[5];
	pos = ffUnicodeUtype2(ch);	/* init with unicode.org position information */
    } else if ( AnchorClassMatch(basersc,rsc,(AnchorClass *) -1,&ap1,&ap2)!=NULL && ap2->type==at_mark ) {
	xoff = ap1->me.x-ap2->me.x;
	yoff = ap1->me.y-ap2->me.y;
	pos = ffUnicodeUtype2(ch);	/* init with unicode.org position information */
    } else {
 /* try to establish a common line on which all accents lie. The problem being*/
 /*  that an accent above a,e,o will usually be slightly higher than an accent */
 /*  above i or u, similarly for upper case. Letters with ascenders shouldn't */
 /*  have much of a problem because ascenders are (usually) all the same shape*/
 /* Obviously this test is only meaningful for latin,greek,cyrillic alphas */
 /*  hence test for isupper,islower. And I'm assuming greek,cyrillic will */
 /*  be consistant with latin */
	if ( islower(basech) || isupper(basech)) {
	    SplineChar *common = SFGetChar(sf,islower(basech)?'o':'O',NULL);
	    if ( common!=NULL ) {
		real top = SplineCharQuickTop(common,layer);
		if ( bb.maxy<top ) {
		    bb.maxx += tan(ia)*(top-bb.maxy);
		    bb.maxy = top;
		}
	    }
	}
	eta = false;
	if ( ((basech>=0x1f20 && basech<=0x1f27) || basech==0x1f74 || basech==0x1f75 || basech==0x1fc6 || basech==0x3b7 || basech==0x3ae) &&
		ch==0x345 ) {
	    bb.miny = 0;		/* ypogegrammeni rides below baseline, not below bottom stem */
	    eta = true;
	    if ( basersc!=NULL && basersc->vstem!=NULL ) {
		bb.minx = basersc->vstem->start;
		bb.maxx = bb.minx + basersc->vstem->width;
	    } else
		bb.maxx -= (bb.maxx-bb.minx)/3;	/* Should also be centered on left stem of eta, but I don't know how to do that..., hence this hack */
	}

	if ( invert ) {
	    /* yes, this transform does a vertical flip from the vertical midpoint of the breve */
	    transform[3] = -1;
	    transform[5] = rbb.maxy+rbb.miny;
	}
	if ( pos==FF_UNICODE_NOPOSDATAGIVEN ) {
	    /* if here, then we need to initialize some type of position info for the accent */
	    if ( ch<0 || ch>=0x10000 )	/* makeutype.c only built data in utype.c for {0...MAXC} */
		pos = FF_UNICODE_ABOVE;
	    else
		pos = ffUnicodeUtype2(ch);	/* init with unicode.org position information */
	    /* In greek, PSILI and friends are centered above lower case, and kern left*/
	    /*  for upper case */
	    if (( basech>=0x390 && basech<=0x3ff) || (basech>=0x1f00 && basech<=0x1fff)) {
		if ( ( basech==0x1fbf || basech==0x1fef || basech==0x1ffe ) &&
			(ch==0x1fbf || ch==0x1fef || ch==0x1ffe || ch==0x1fbd || ch==0x1ffd )) {
		    pos = FF_UNICODE_ABOVE|FF_UNICODE_RIGHT;
		} else if ( isupper(basech) &&
			(ch==0x313 || ch==0x314 || ch==0x301 || ch==0x300 || ch==0x30d ||
			 ch==0x1ffe || ch==0x1fbf || ch==0x1fcf || ch==0x1fdf ||
			 ch==0x1fbd || ch==0x1fef || ch==0x1ffd ||
			 ch==0x1fcd || ch==0x1fdd || ch==0x1fce || ch==0x1fde ) )
		    pos = FF_UNICODE_ABOVE|FF_UNICODE_LEFT;
		else if ( isupper(basech) && ch==0x1fbe )
		    pos = FF_UNICODE_RIGHT;
		else if ( ch==0x1fcd || ch==0x1fdd || ch==0x1fce || ch==0x1fde ||
			 ch==0x1ffe || ch==0x1fbf || ch==0x1fcf || ch==0x1fdf ||
			 ch==0x384 )
		    pos = FF_UNICODE_ABOVE;
	    } else if ( (basech==0x1ffe || basech==0x1fbf) && (ch==0x301 || ch==0x300))
		pos = FF_UNICODE_RIGHT;
	    else if ( sc->unicodeenc==0x1fbe && ch==0x345 )
		pos = FF_UNICODE_RIGHT;
	    else if ( basech=='l' && ch==0xb7 )
		pos = FF_UNICODE_RIGHT|FF_UNICODE_OVERSTRIKE;
	    else if ( basech=='L' && ch==0xb7 )
		pos = FF_UNICODE_OVERSTRIKE;
	    else if ( ch==0xb7 )
		pos = FF_UNICODE_RIGHT;
	    else if ( basech=='A' && ch==0x30a )	/* Aring usually touches */
		pos = FF_UNICODE_ABOVE|FF_UNICODE_TOUCHING;
	    else if (( basech=='A' || basech=='a' || basech=='E' || basech=='u' ) &&
		    ch == 0x328 )
		pos = FF_UNICODE_BELOW|FF_UNICODE_CENTERRIGHT|FF_UNICODE_TOUCHING;	/* ogonek off to the right for these in polish (but not lc e) */
	    else if (( basech=='N' || basech=='n' || basech=='K' || basech=='k' || basech=='R' || basech=='r' || basech=='H' || basech=='h' ) &&
		    ch == 0x327 )
		pos = FF_UNICODE_BELOW|FF_UNICODE_CENTERLEFT|FF_UNICODE_TOUCHING;	/* cedilla off under left stem for these guys */
	    if ( basech==0x391 && pos==(FF_UNICODE_ABOVE|FF_UNICODE_LEFT) ) {
		bb.minx += (bb.maxx-bb.minx)/4;
	    }
	}
	if ( sc->unicodeenc==0x0149 )
	    pos = FF_UNICODE_ABOVE|FF_UNICODE_LEFT;
	else if ( sc->unicodeenc==0x013d || sc->unicodeenc==0x013e )
	    pos = FF_UNICODE_ABOVE|FF_UNICODE_RIGHT;
	else if ( sc->unicodeenc==0x010f || sc->unicodeenc==0x013d ||
		  sc->unicodeenc==0x013e || sc->unicodeenc==0x0165 )
	    pos = FF_UNICODE_ABOVE|FF_UNICODE_RIGHT;
	else if ( (sc->unicodeenc==0x1fbd || sc->unicodeenc==0x1fbf ||
		sc->unicodeenc==0x1ffe || sc->unicodeenc==0x1fc0 ) &&
		bb.maxy==0 && bb.miny==0 ) {
	    /* Building accents on top of space */
	    bb.maxy = 7*sf->ascent/10;
	}

	if ( (pos & FF_UNICODE_ABOVE) && (pos & (FF_UNICODE_LEFT|FF_UNICODE_RIGHT)) )
	    yoff = bb.maxy - rbb.maxy;
	else if ( pos & FF_UNICODE_ABOVE ) {
	    yoff = bb.maxy - rbb.miny;
	    if ( !( pos & FF_UNICODE_TOUCHING) )
		yoff += spacing;
	} else if ( pos & FF_UNICODE_BELOW ) {
	    yoff = bb.miny - rbb.maxy;
	    if ( !( pos & FF_UNICODE_TOUCHING) )
		yoff -= spacing;
	} else if ( pos & FF_UNICODE_OVERSTRIKE )
	    yoff = bb.miny - rbb.miny + ((bb.maxy-bb.miny)-(rbb.maxy-rbb.miny))/2;
	else /* If neither Above, Below, nor overstrike then should use the same baseline */
	    yoff = bb.miny - rbb.miny;

	if ( pos & (FF_UNICODE_ABOVE|FF_UNICODE_BELOW) ) {
	    /* When we center an accent above an asymetric character like "C" we */
	    /*  should not pick the mid point of the char. Rather we should pick */
	    /*  the highest point (mostly anyway, there are exceptions) */
	    if ( pos & FF_UNICODE_ABOVE ) {
		static DBounds pointless;
		if ( CharCenterHighest ) {
		    if ( basech!='b' && basech!='d' && basech!='h' && basech!='n' && basech!='r' && basech!=0xf8 &&
			    basech!='B' && basech!='D' && basech!='L' && basech!=0xd8 )
			ybase = SCFindTopXRange(sc,layer,&bb);
		    if ( ((basech=='h' && ch==0x307) ||	/* dot over the stem in hdot */
			    basech=='i' || basech=='j' || basech==0x131 || basech==0xf6be || basech==0x237 ||
			    (basech=='k' && ch==0x301) ||
			    (baserch=='L' && (ch==0x301 || ch==0x304)) ||
			    basech=='l' || basech=='t' ) &&
			    (xoff=SCStemCheck(sf,layer,basech,&bb,&pointless,pos))!=0x70000000 )
			bb.minx = bb.maxx = xoff;		/* While on "t" we should center over the stem */
		}
	    } else if ( ( pos & FF_UNICODE_BELOW ) && !eta )
		if ( CharCenterHighest )
		    ybase = SCFindBottomXRange(sc,layer,&bb,ia);
	}

	if ( isupper(basech) && ch==0x342)	/* While this guy rides above PSILI on left */
	    xoff = bb.minx - rbb.minx;
	else if ( pos & FF_UNICODE_LEFT )
	    xoff = bb.minx - spacing - rbb.maxx;
	else if ( pos & FF_UNICODE_RIGHT ) {
	    xoff = bb.maxx - rbb.minx+spacing/2;
	    if ( !( pos & FF_UNICODE_TOUCHING) )
		xoff += spacing;
	} else {
	    if ( (pos & (FF_UNICODE_CENTERLEFT|FF_UNICODE_CENTERRIGHT)) &&
		    (xoff=SCStemCheck(sf,layer,basech,&bb,&rbb,pos))!=0x70000000 )
		/* Done */;
	    else if ( pos & FF_UNICODE_CENTERLEFT )
		xoff = bb.minx + (bb.maxx-bb.minx)/2 - rbb.maxx;
	    else if ( pos & FF_UNICODE_LEFTEDGE )
		xoff = bb.minx - rbb.minx;
	    else if ( pos & FF_UNICODE_CENTERRIGHT )
		xoff = bb.minx + (bb.maxx-bb.minx)/2 - rbb.minx;
	    else if ( pos & FF_UNICODE_RIGHTEDGE )
		xoff = bb.maxx - rbb.maxx;
	    else
		xoff = bb.minx - rbb.minx + ((bb.maxx-bb.minx)-(rbb.maxx-rbb.minx))/2;
	}
	if ( ia!=0 )
	    xoff += (italicoff = tan(-ia)*(rbb.miny+yoff-ybase));
    }	/* Anchor points */
    transform[4] = xoff;
    /*if ( invert ) transform[5] -= yoff; else */transform[5] += yoff;

    if ( bdf == NULL || !disp_only ) {
	_SCAddRef(sc,rsc,layer,transform);
	if ( pos != FF_UNICODE_NOPOSDATAGIVEN && (pos & FF_UNICODE_RIGHT) )
	    SCSynchronizeWidth(sc,sc->width + rbb.maxx-rbb.minx+spacing,sc->width,NULL);
	if ( pos != FF_UNICODE_NOPOSDATAGIVEN && (pos & (FF_UNICODE_LEFT|FF_UNICODE_RIGHT|FF_UNICODE_CENTERLEFT|FF_UNICODE_LEFTEDGE|FF_UNICODE_CENTERRIGHT|FF_UNICODE_RIGHTEDGE)) )
	    TurnOffUseMyMetrics(sc);
    }
    if ( !disp_only ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->glyphs[rsc->orig_pos] != NULL )
		_BCCenterAccent( bdf,sc->orig_pos,rsc->orig_pos,ch,basech,italicoff,pos,sf->ascent+sf->descent );
	}
    } else if ( bdf != NULL && bdf->glyphs[rsc->orig_pos] != NULL )
	_BCCenterAccent( bdf,sc->orig_pos,rsc->orig_pos,ch,basech,italicoff,pos,sf->ascent+sf->descent );
}

static void SCCenterAccent(SplineChar *sc,SplineChar *basersc, SplineFont *sf,
	int layer, int ch, BDFFont *bdf,int disp_only,
	real ia, int basech, char *dot ) {
    int invert = false;			/* invert accent, false==0, true!=0 */
    SplineChar *rsc = GetGoodAccentGlyph(sf,ch,basech,&invert,ia,dot,sc);

    /* find a location to put an accent on this character */
    _SCCenterAccent(sc,basersc,sf,layer,ch,bdf,disp_only,rsc,ia,basech,invert,FF_UNICODE_NOPOSDATAGIVEN);
}

static void _BCPutRefAfter( BDFFont *bdf,int gid,int rgid,int normal,int under ) {
    BDFChar *bc, *rbc;
    int ispacing;

    if ( bdf->glyphs[rgid]!=NULL && bdf->glyphs[gid]!=NULL ) {
	rbc = bdf->glyphs[rgid];
	bc = bdf->glyphs[gid];
	BCFlattenFloat(rbc);
	BCCompressBitmap(rbc);
	BCCompressBitmap(bc);
	if ( (ispacing = (bdf->pixelsize*accent_offset+50)/100)<=1 ) ispacing = 2;
	if ( normal ) {
	    BCAddReference( bc,rbc,rgid,bc->width,0 );
	    bc->width += rbc->width;
	} else if ( under ) {
	    BCAddReference( bc,rbc,rgid,
		    (bc->xmax+rbc->xmin-rbc->xmax-rbc->xmin)/2,
		    bc->ymin-ispacing-rbc->ymax );
	} else {
	    BCAddReference( bc,rbc,rgid,bc->xmax-ispacing-rbc->xmin,0 );
	}
    }
}

static void SCPutRefAfter(SplineChar *sc,SplineFont *sf,int layer, int ch,
	BDFFont *bdf,int disp_only, char *dot) {
    SplineChar *rsc = SFGetChar(sf,ch,NULL);
    int full = sc->unicodeenc, normal = false, under = false/*, stationary=false*/;
    DBounds bb, rbb;
    real spacing = (sf->ascent+sf->descent)*accent_offset/100;
    char buffer[300], namebuf[300];

    if ( bdf == NULL || !disp_only ) {
	if ( dot!=NULL && rsc!=NULL ) {
	    snprintf(buffer,sizeof(buffer),"%s%s", rsc->name, dot );
	    rsc = SFGetChar(sf,-1,buffer);
	} else if ( dot!=NULL ) {
	    snprintf(buffer,sizeof(buffer),"%s%s",
		    (char *) StdGlyphName(namebuf,ch,sf->uni_interp,sf->for_new_glyphs),
		    dot);
	    rsc = SFGetChar(sf,-1,buffer);
	}

	if ( full<0x1100 || full>0x11ff ) {
	    SCAddRef(sc,rsc,layer,sc->width,0);
	    sc->width += rsc->width;
	    normal = true;
    /* these two jamo (same consonant really) ride underneath (except sometimes) */
    /* So should the jungsong */
	} else if (( ch==0x110b && (full!=0x1135 && full!=0x1147 && full!=0x114d)) ||
		    (ch==0x11bc && full!=0x11ee) ||
		    full==0x1182 || full==0x1183 || full==0x1187 || (full==0x118b && ch==0x1173) ||
		    full==0x118d || full==0x1193 || (full>=0x1195 && full<=0x1197) ||
		    full==0x119d || full==0x11a0 ) {
	    SplineCharQuickBounds(sc,&bb);
	    SplineCharQuickBounds(rsc,&rbb);
	    SCAddRef(sc,rsc,layer,(bb.maxx+bb.minx)/2-(rbb.maxx+rbb.minx)/2,bb.miny-spacing-rbb.maxy);
	    under = true;
	} else {	/* Jamo should snuggle right up to one another, and ignore the width */
	    SplineCharQuickBounds(sc,&bb);
	    SplineCharQuickBounds(rsc,&rbb);
	    SCAddRef(sc,rsc,layer,bb.maxx+spacing-rbb.minx,0);
	}
    }
    if ( !disp_only ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    _BCPutRefAfter( bdf,sc->orig_pos,rsc->orig_pos,normal,under );
	}
    } else if ( bdf != NULL )
	_BCPutRefAfter( bdf,sc->orig_pos,rsc->orig_pos,normal,under );
}

static void BCMakeSpace(BDFFont *bdf, int gid, int width, int em) {
    BDFChar *bc;

    if ( (bc = bdf->glyphs[gid])==NULL ) {
	BDFMakeGID( bdf,gid );
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
	bc->bitmap = calloc(bc->bytes_per_line*(bc->ymax-bc->ymin+1),sizeof(char));
    }
}

static void DoSpaces(SplineFont *sf,SplineChar *sc,int layer,BDFFont *bdf,int disp_only) {
    int width;
    int uni = sc->unicodeenc;
    int em = sf->ascent+sf->descent;
    SplineChar *tempsc;

    if ( iszerowidth(uni))
	width = 0;
    else switch ( uni ) {
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
	tempsc = SFGetChar(sf,'0',NULL);
	if ( tempsc!=NULL && tempsc->layers[layer].splines==NULL && tempsc->layers[layer].refs==NULL ) tempsc = NULL;
	if ( tempsc==NULL ) width = em/2; else width = tempsc->width;
      break;
      case 0x2008:
	tempsc = SFGetChar(sf,'.',NULL);
	if ( tempsc!=NULL && tempsc->layers[layer].splines==NULL && tempsc->layers[layer].refs==NULL ) tempsc = NULL;
	if ( tempsc==NULL ) width = em/4; else width = tempsc->width;
      break;
      case ' ':
	tempsc = SFGetChar(sf,'I',NULL);
	if ( tempsc!=NULL && tempsc->layers[layer].splines==NULL && tempsc->layers[layer].refs==NULL ) tempsc = NULL;
	if ( tempsc==NULL ) width = em/4; else width = tempsc->width;
      break;
      default:
	width = em/3;
      break;
    }

    if ( !disp_only || bdf == NULL ) {
	sc->width = width;
	sc->widthset = true;
	SCCharChangedUpdate(sc,ly_none);
    }

    if ( !disp_only ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    BCMakeSpace( bdf,sc->orig_pos,width,em );
	    BCCharChangedUpdate(bdf->glyphs[sc->orig_pos]);
	}
    } else if ( bdf != NULL ) {
	BCMakeSpace( bdf,sc->orig_pos,width,em );
	BCCharChangedUpdate(bdf->glyphs[sc->orig_pos]);
    }
}

static SplinePoint *MakeSP(real x, real y, SplinePoint *last,int order2) {
    SplinePoint *new = chunkalloc(sizeof(SplinePoint));

    new->me.x = x; new->me.y = y;
    new->prevcp = new->nextcp = new->me;
    new->noprevcp = new->nonextcp = true;
    new->pointtype = pt_corner;
    if ( last!=NULL )
	SplineMake(last,new,order2);
return( new );
}

static void BCMakeRule(BDFFont *bdf, int gid, int layer) {
    BDFChar *bc, *tempbc;

    if ( (bc = bdf->glyphs[gid])==NULL ) {
	BDFMakeGID(bdf,gid);
    } else {
	BCPreserveState(bc);
	BCFlattenFloat(bc);
	BCCompressBitmap(bc);
	free(bc->bitmap);
	tempbc = SplineCharRasterize(bc->sc,layer,bdf->pixelsize);
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

static void DoRules(SplineFont *sf,SplineChar *sc,int layer,BDFFont *bdf,int disp_only) {
    int width;
    int uni = sc->unicodeenc;
    int em = sf->ascent+sf->descent;
    SplineChar *tempsc;
    DBounds b;
    real lbearing, rbearing, height, ypos;
    SplinePoint *first, *sp;

    switch ( uni ) {
      case '-':
	width = em/3;
      break;
      case 0x2010: case 0x2011:
	tempsc = SFGetChar(sf,'-',NULL);
	if ( tempsc!=NULL && tempsc->layers[layer].splines==NULL && tempsc->layers[layer].refs==NULL ) tempsc = NULL;
	if ( tempsc==NULL ) width = (4*em)/10; else width = tempsc->width;
      break;
      case 0x2012:
	tempsc = SFGetChar(sf,'0',NULL);
	if ( tempsc!=NULL && tempsc->layers[layer].splines==NULL && tempsc->layers[layer].refs==NULL ) tempsc = NULL;
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

    tempsc = SFGetChar(sf,'-',NULL);
    if ( tempsc==NULL || (tempsc->layers[layer].splines==NULL && tempsc->layers[layer].refs==NULL )) {
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
    if ( bdf == NULL || !disp_only ) {
	first = sp = MakeSP(lbearing,ypos,NULL,sc->layers[layer].order2);
	sp = MakeSP(lbearing,ypos+height,sp,sc->layers[layer].order2);
	sp = MakeSP(width-rbearing,ypos+height,sp,sc->layers[layer].order2);
	sp = MakeSP(width-rbearing,ypos,sp,sc->layers[layer].order2);
	SplineMake(sp,first,sc->layers[layer].order2);
	sc->layers[layer].splines = chunkalloc(sizeof(SplinePointList));
	sc->layers[layer].splines->first = sc->layers[layer].splines->last = first;
	sc->layers[layer].splines->start_offset = 0;
	sc->width = width;
	sc->widthset = true;
	SCCharChangedUpdate(sc,layer);
    }

    if ( !disp_only ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    BCMakeRule( bdf,sc->orig_pos,layer );
	    BCCharChangedUpdate(bdf->glyphs[sc->orig_pos]);
	}
    } else if ( bdf != NULL ) {
	BCMakeRule( bdf,sc->orig_pos,layer );
	BCCharChangedUpdate(bdf->glyphs[sc->orig_pos]);
    }
}

static void BCDoRotation(BDFFont *bdf, int gid) {
    BDFChar *from, *to;

    if ( gid>=bdf->glyphcnt || bdf->glyphs[gid]==NULL )
return;
    from = bdf->glyphs[gid];
    to = BDFMakeGID(bdf,gid);
    BCRotateCharForVert(to,from,bdf);
    BCCharChangedUpdate(to);
}

static void DoRotation(SplineFont *sf,SplineChar *sc,int layer,BDFFont *bdf,int disp_only) {
    /* In when laying CJK characters vertically and intermixed latin (greek,etc) */
    /*  characters need to be rotated. Adobe's cid tables call for some */
    /*  pre-rotated characters, so we might as well be prepared to deal with */
    /*  them. Note the rotated and normal characters are often in different */
    /*  subfonts so we can't use references */
    SplineChar *scbase;
    real transform[6];
    SplineSet *last, *temp;
    RefChar *ref;
    char *end;
    int j,cid;

    if ( sf->cidmaster!=NULL && strncmp(sc->name,"vertcid_",8)==0 ) {
	cid = strtol(sc->name+8,&end,10);
	if ( *end!='\0' || (j=SFHasCID(sf,cid))==-1)
return;		/* Can't happen */
	scbase = sf->cidmaster->subfonts[j]->glyphs[cid];
    } else if ( sf->cidmaster!=NULL && strstr(sc->name,".vert")!=NULL &&
	    (cid = CIDFromName(sc->name,sf->cidmaster))!= -1 ) {
	if ( (j=SFHasCID(sf,cid))==-1)
return;		/* Can't happen */
	scbase = sf->cidmaster->subfonts[j]->glyphs[cid];
    } else {
	if ( strncmp(sc->name,"vertuni",7)==0 && strlen(sc->name)==11 ) {
	    char *end;
	    int uni = strtol(sc->name+7,&end,16);
	    if ( *end!='\0' || (cid = SFCIDFindExistingChar(sf,uni,NULL))==-1 )
return;		/* Can't happen */
	} else if ( strncmp(sc->name,"uni",3)==0 && strstr(sc->name,".vert")!=NULL ) {
	    int uni = strtol(sc->name+3,&end,16);
	    if ( *end!='.' || (cid = SFCIDFindExistingChar(sf,uni,NULL))==-1 )
return;
	} else if ( sc->name[0]=='u' && strstr(sc->name,".vert")!=NULL ) {
	    int uni = strtol(sc->name+1,&end,16);
	    if ( *end!='.' || (cid = SFCIDFindExistingChar(sf,uni,NULL))==-1 )
return;
	} else if ( strstr(sc->name,".vert")!=NULL || strstr(sc->name,".vrt2")!=NULL) {
	    char *temp;
	    end = strchr(sc->name,'.');
	    temp = copyn(sc->name,end-sc->name);
	    cid = SFFindExistingSlot(sf,-1,temp);
	    free(temp);
	    if ( cid==-1 )
return;
	}
	if ( sf->cidmaster==NULL )
	    scbase = sf->glyphs[cid];
	else
	    scbase = sf->cidmaster->subfonts[SFHasCID(sf,cid)]->glyphs[cid];
    }

    if ( bdf == NULL || !disp_only ) {
	transform[0] = transform[3] = 0;
	transform[1] = -1; transform[2] = 1;
	transform[4] = scbase->parent->descent; transform[5] = /*scbase->parent->vertical_origin*/ scbase->parent->ascent;

	sc->layers[layer].splines = SplinePointListTransform(SplinePointListCopy(scbase->layers[layer].splines),
		transform, tpt_AllPoints );
	if ( sc->layers[layer].splines==NULL ) last = NULL;
	else for ( last = sc->layers[layer].splines; last->next!=NULL; last = last->next );

	for ( ref = scbase->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	    temp = SplinePointListTransform(SplinePointListCopy(ref->layers[0].splines),
		transform, tpt_AllPoints );
	    if ( last==NULL )
		sc->layers[layer].splines = temp;
	    else
		last->next = temp;
	    if ( temp!=NULL )
		for ( last=temp; last->next!=NULL; last=last->next );
	}
	sc->width = sc->parent->ascent+sc->parent->descent;
	SCCharChangedUpdate(sc,layer);
    }

    if ( !disp_only ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    BCDoRotation( bdf,sc->orig_pos );
	}
    } else if ( bdf != NULL ) {
	BCDoRotation( bdf,sc->orig_pos );
    }
}

static int SCMakeRightToLeftLig(SplineChar *sc,SplineFont *sf,
	int layer, const unichar_t *start,BDFFont *bdf,int disp_only) {
    int cnt = u_strlen(start);
    int ret=false, ch, alt_ch;
    const unichar_t *pt;

    pt = start+cnt-1;
    ch = *pt;
    if ( ch>=0x621 && ch<=0x6ff ) {
	alt_ch = ArabicForms[ch-0x600].final;
	if ( alt_ch!=0 && haschar(sf,alt_ch,NULL))
	    ch = alt_ch;
    }
    if ( SCMakeBaseReference(sc,sf,layer,ch,bdf,disp_only) ) {
	for ( --pt; pt>=start; --pt ) {
	    ch = *pt;
	    if ( ch>=0x621 && ch<=0x6ff ) {
		if ( pt==start )
		    alt_ch = ArabicForms[ch-0x600].initial;
		else
		    alt_ch = ArabicForms[ch-0x600].medial;
		if ( alt_ch!=0 && haschar(sf,alt_ch,NULL))
		    ch = alt_ch;
	    }
	    SCPutRefAfter(sc,sf,layer,ch,bdf,disp_only,NULL);
	}
	ret = true;
    }
return( ret );
}

static void SCBuildHangul(SplineFont *sf,SplineChar *sc, int layer, const unichar_t *pt,
	BDFFont *bdf, int disp_only) {
    SplineChar *rsc;
    int first = true;

    sc->width = 0;
    while ( *pt ) {
	rsc = SFGetChar(sf,*pt++,NULL);
	if ( rsc!=NULL ) {
	    if ( bdf == NULL || !disp_only ) {
		SCAddRef(sc,rsc,layer,0,0);
		if ( rsc->width>sc->width ) sc->width = rsc->width;
	    }
	    if ( !disp_only ) {
		for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		    if ( first )
			BCClearAll(bdf->glyphs[sc->orig_pos]);
		    BCAddReference( BDFMakeGID(bdf,sc->orig_pos),bdf->glyphs[rsc->orig_pos],
			rsc->orig_pos,0,0 );
		}
	    } else if ( bdf != NULL ) {
		if ( first )
		    BCClearAll(bdf->glyphs[sc->orig_pos]);
		BCAddReference( BDFMakeGID(bdf,sc->orig_pos),bdf->glyphs[rsc->orig_pos],
		    rsc->orig_pos,0,0 );
	    }
	    first = false;
	}
    }
}

static int _SCMakeDotless(SplineFont *sf, SplineChar *dotless, int layer, int doit) {
    SplineChar *sc, *xsc;
    BlueData bd;
    SplineSet *head=NULL, *last=NULL, *test, *cur, *next;
    DBounds b;

    sc = SFGetChar(sf,dotless->unicodeenc==0x131?'i':'j',NULL);
    xsc = SFGetChar(sf,'x',NULL);
    if ( sc==NULL || sc->layers[layer].splines==NULL || sc->layers[layer].refs!=NULL || xsc==NULL )
return( false );
    QuickBlues(sf,layer,&bd);
    if ( bd.xheight==0 )
return(false );
    for ( test=sc->layers[layer].splines; test!=NULL; test=test->next ) {
	next = test->next; test->next = NULL;
	SplineSetQuickBounds(test,&b);
	test->next = next;
	if ( b.miny < bd.xheight ) {
	    if ( !doit )
return( true );
	    cur = SplinePointListCopy1(test);
	    if ( last==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
    }
    if ( head==NULL )
return( false );

    SCPreserveLayer(dotless,layer,true);
    SplinePointListsFree(dotless->layers[layer].splines);
    dotless->layers[layer].splines = NULL;
    SCRemoveLayerDependents(dotless,layer);
    dotless->width = sc->width;
    dotless->layers[layer].splines = head;
    SCCharChangedUpdate(dotless,layer);
return( true );
}

static int SCMakeDotless(SplineFont *sf, SplineChar *dotless, int layer, BDFFont *bdf, int disp_only, int doit) {
    SplineChar *sc;
    BDFChar *bc;
    int ret = 0;

    if ( dotless==NULL )
return( ret );
	if ( dotless->unicodeenc!=0x131 && dotless->unicodeenc!=0xf6be && dotless->unicodeenc!=0x237 )
return( ret );
    sc = SFGetChar(sf,dotless->unicodeenc==0x131?'i':'j',NULL);
    if ( sc==NULL )
return( ret );
    if ( bdf==NULL || !disp_only )
	ret = _SCMakeDotless( sf,dotless,layer,doit );

    if ( !disp_only ) {
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if (( bc = bdf->glyphs[sc->orig_pos])!=NULL ) {
		BCClearAndCopyBelow(bdf,dotless->orig_pos,sc->orig_pos,BCFindGap(bc));
	    }
	}
    } else if ( bdf != NULL ) {
	if (( bc = bdf->glyphs[sc->orig_pos])!=NULL ) {
	    BCClearAndCopyBelow(bdf,dotless->orig_pos,sc->orig_pos,BCFindGap(bc));
	}
    }
	    
return( ret );
}

static void SCSetReasonableLBearing(SplineChar *sc,SplineChar *base,int layer) {
    DBounds full, b;
    SplineFont *sf;
    int emsize;
    double xoff;
    RefChar *ref;
    real transform[6];

    /* Hmm. Panov doesn't think this should happen */
return;

    SplineCharLayerFindBounds(sc,layer,&full);
    SplineCharLayerFindBounds(base,layer,&b);

    sf = sc->parent;
    emsize = sf->ascent+sf->descent;

    /* Now don't get excited if we have a very thin glyph (I in a sans-serif) */
    /*  and a centered accent spills off to the left a little */
    if ( full.minx>=0 || full.minx>=b.minx || full.minx>=-(emsize/20) )
return;
    /* ok. let's say we want an lbearing that's the same as that of the base */
    /*  glyph */
    xoff = b.minx-full.minx;
    memset(transform,0,sizeof(transform));
    transform[0] = transform[3] = 1.0;
    transform[4] = xoff;
    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	ref->bb.minx += xoff; ref->bb.maxx += xoff;
	ref->transform[4] += xoff;
	SplinePointListTransform(ref->layers[0].splines,transform,tpt_AllPoints);
    }
    SCSynchronizeWidth(sc,sc->width + xoff,sc->width,NULL);
}

void SCBuildComposit(SplineFont *sf, SplineChar *sc, int layer, BDFFont *bdf, int disp_only ) {
    const unichar_t *pt, *apt; unichar_t ch;
    real ia;
    char *dot;
    /* This does not handle arabic ligatures at all. It would need to reverse */
    /*  the string and deal with <final>, <medial>, etc. info we don't have */

    if ( !SFIsSomethingBuildable(sf,sc,layer,false))
return;
    if ( !disp_only || bdf == NULL ) {
	SCPreserveLayer(sc,layer,true);
	SplinePointListsFree(sc->layers[layer].splines);
	sc->layers[layer].splines = NULL;
	SCRemoveLayerDependents(sc,layer);
	sc->width = 0;
    }
    if ( !disp_only ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->glyphs[sc->orig_pos]!=NULL )
		BCClearAll( bdf->glyphs[sc->orig_pos] );
    } else if ( bdf!=NULL) {
	if ( bdf->glyphs[sc->orig_pos]!=NULL )
	    BCClearAll( bdf->glyphs[sc->orig_pos] );
    }

    if ( iszerowidth(sc->unicodeenc) || (sc->unicodeenc>=0x2000 && sc->unicodeenc<=0x200f )) {
	DoSpaces( sf,sc,layer,bdf,disp_only );
return;
    } else if ( sc->unicodeenc>=0x2010 && sc->unicodeenc<=0x2015 ) {
	DoRules( sf,sc,layer,bdf,disp_only );
return;
    } else if ( SFIsRotatable(sf,sc) ) {
	DoRotation( sf,sc,layer,bdf,disp_only );
return;
    } else if ( sc->unicodeenc==0x131 || sc->unicodeenc==0x237 || sc->unicodeenc==0xf6be ) {
	SCMakeDotless( sf,sc,layer,bdf,disp_only,true );
return;
    }

    if (( ia = sf->italicangle )==0 )
	ia = SFGuessItalicAngle(sf);
    ia *= 3.1415926535897932/180;	/* convert degrees to radians */

    dot = strchr(sc->name,'.');

    pt= SFGetAlternate(sf,sc->unicodeenc,sc,false);
    ch = *pt++;
    if ( dot==NULL && ( ch=='i' || ch=='j' || ch==0x456 )) {
	/* if we're combining i (or j) with an accent that would interfere */
	/*  with the dot, then get rid of the dot. (use dotlessi) */
	for ( apt = pt; *apt && combiningposmask(*apt) != FF_UNICODE_ABOVE; ++apt);
	if ( *apt!='\0' ) {
	    if ( ch=='i' || ch==0x456 ) ch = 0x0131;
	    else if ( ch=='j' ) {
		if ( haschar(sf,0x237,NULL) ) ch = 0x237;		/* Proposed dotlessj */
		else ch = 0xf6be;				/* Adobe's private use dotless j */
	    }
	}
    }
    if ( sc->unicodeenc>=0xac00 && sc->unicodeenc<=0xd7a3 )
	SCBuildHangul(sf,sc,layer,pt-1,bdf,disp_only);
    else if ( isrighttoleft(ch) && !iscombining(*pt)) {
	SCMakeRightToLeftLig(sc,sf,layer,pt-1,bdf,disp_only);
    } else {
	RefChar *base;
	if ( !SCMakeBaseReference(sc,sf,layer,ch,bdf,disp_only) )
return;
	base = sc->layers[layer].refs;
	if ( base==NULL )
	    /* Happens when building glyphs which are themselves marks, like */
	    /* perispomeni which is a tilde over a space, only we don't make */
	    /* a reference to space */;
	else if ( sc->width == base->sc->width )
	    base->use_my_metrics = true;
	while ( iscombining(*pt) || (ch=='L' && *pt==0xb7) ||	/* b7, centered dot is used as a combining accent for Ldot but as a lig for ldot */
		*pt==0x384 || *pt==0x385 || (*pt>=0x1fbd && *pt<=0x1fff ))	/* Special greek accents */
	    SCCenterAccent(sc,base!=NULL?base->sc:NULL,sf,layer,*pt++,bdf,disp_only,ia,ch,dot);
	while ( *pt ) {
	    if ( base!=NULL ) base->use_my_metrics = false;
	    SCPutRefAfter(sc,sf,layer,*pt++,bdf,disp_only,dot);
	}
	/* All along we assumed the base glyph didn't move. This makes       */
	/* positioning easier. But if we add accents to the left we now want */
	/* to move the glyph so we don't have a negative lbearing */
	if ( base!=NULL )
	    SCSetReasonableLBearing(sc,base->sc,layer);
    }
    if ( !disp_only || bdf == NULL )
	SCCharChangedUpdate(sc,layer);
    if ( !disp_only ) {
	for ( bdf=sf->cidmaster?sf->cidmaster->bitmaps:sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->glyphs[sc->orig_pos]!=NULL )
		BCCharChangedUpdate(bdf->glyphs[sc->orig_pos]);
    } else if ( bdf != NULL && bdf->glyphs[sc->orig_pos]!=NULL )
	BCCharChangedUpdate(bdf->glyphs[sc->orig_pos]);
return;
}

int SCAppendAccent(SplineChar *sc,int layer,
	char *glyph_name,	/* unicode char name */
	int uni,		/* unicode char value */
	uint32 pos		/* unicode char position info, see #define for (uint32)(utype2[]) */ ) {
    SplineFont *sf = sc->parent;
    double ia;
    int basech;
    RefChar *ref, *last=NULL;
    int invert = false;		/* invert accent, false==0, true!=0 */
    SplineChar *asc;
    int i;
    const unichar_t *apt, *end;
    char *pt;

    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	last = ref;
    if ( last==NULL )
return( 1 );				/* No base character reference found */
    basech = last->sc->unicodeenc;

    if (( ia = sf->italicangle )==0 )
	ia = SFGuessItalicAngle(sf);
    ia *= 3.1415926535897932/180;	/* convert degrees to radians */

    SCPreserveLayer(sc,layer,true);

    asc = SFGetChar(sf,uni,glyph_name);
    if ( asc!=NULL && uni==-1 )
	uni = asc->unicodeenc;
    else if ( asc==NULL && uni!=-1 )
	asc = GetGoodAccentGlyph(sf,uni,basech,&invert,ia,NULL,sc);
    if ( asc==NULL )
return( 2 );				/* Could not find that accent */
    if ( uni==-1 && (pt=strchr(asc->name,'.'))!=NULL && pt-asc->name<100 ) {
	char buffer[101];
	strncpy(buffer,asc->name,pt-asc->name);
	buffer[(pt-asc->name)] = '\0';
	uni = UniFromName(buffer,ui_none,NULL);
    }

    if ( uni<=BottomAccent || uni>=TopAccent ) {
	/* Find the real combining character that maps to this glyph */
	/* that's where we store positioning info */
	for ( i=BottomAccent; i<=TopAccent; ++i ) {
	    apt = accents[i-BottomAccent]; end = apt+sizeof(accents[0])/sizeof(accents[0][0]);
	    while ( apt<end && *apt!=uni )
		++apt;
	    if ( apt<end ) {
		uni = i;
	break;
	    }
	}
    }

    _SCCenterAccent(sc,last->sc,sf,layer,uni,NULL,false,asc,ia,basech,invert,pos);
return( 0 );
}
