/* Copyright (C) 2000-2003 by George Williams */
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
#include "pfaedit.h"
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <utype.h>
#include <ustring.h>
#include <chardata.h>
#include <gwidget.h>

#include "ttf.h"

int glyph_2_name_map=0;

/* This file produces a ttf file given a splinefont. */

/* ************************************************************************** */

/* Required tables:
	cmap		encoding
	head		header data
	hhea		more header data
	hmtx		horizontal metrics (widths, lsidebearing)
	maxp		various maxima in the font
	name		various names associated with the font
	post		postscript names and other stuff
Required by windows but not mac
	OS/2		bleah. 
Required for TrueType
	loca		pointers to the glyphs
	glyf		character shapes
Required for OpenType (Postscript)
	CFF 		A complete postscript CFF font here with all its internal tables
Required for bitmaps
	bdat/EBDT	bitmap data
	bloc/EBLC	pointers to bitmaps
	bhed		for apple bitmap only fonts, replaces head
Optional for bitmaps
	EBSC		bitmap scaling table (used in windows "bitmap-only" fonts)
"Advanced Typograpy"
  Apple
	feat		(mapping between morx features and 'name' names)
	kern		(if data are present)
	lcar		(ligature caret, if data present)
	morx		(substitutions, if data present)
	prop		(glyph properties, if data present)
	opbd		(optical bounds, if data present)
  OpenType
	GPOS		(opentype, if kern,anchor data are present)
	GSUB		(opentype, if ligature (other subs) data are present)
	GDEF		(opentype, if anchor data are present)
additional tables
	cvt		for hinting
	gasp		to control when things should be hinted
	fpgm		for hinting (currently only copied and dumped verbatim)
	prep		for hinting (currently only copied and dumped verbatim)
	
*/

const char *ttfstandardnames[258] = {
".notdef",
".null",
"nonmarkingreturn",
"space",
"exclam",
"quotedbl",
"numbersign",
"dollar",
"percent",
"ampersand",
"quotesingle",
"parenleft",
"parenright",
"asterisk",
"plus",
"comma",
"hyphen",
"period",
"slash",
"zero",
"one",
"two",
"three",
"four",
"five",
"six",
"seven",
"eight",
"nine",
"colon",
"semicolon",
"less",
"equal",
"greater",
"question",
"at",
"A",
"B",
"C",
"D",
"E",
"F",
"G",
"H",
"I",
"J",
"K",
"L",
"M",
"N",
"O",
"P",
"Q",
"R",
"S",
"T",
"U",
"V",
"W",
"X",
"Y",
"Z",
"bracketleft",
"backslash",
"bracketright",
"asciicircum",
"underscore",
"grave",
"a",
"b",
"c",
"d",
"e",
"f",
"g",
"h",
"i",
"j",
"k",
"l",
"m",
"n",
"o",
"p",
"q",
"r",
"s",
"t",
"u",
"v",
"w",
"x",
"y",
"z",
"braceleft",
"bar",
"braceright",
"asciitilde",
"Adieresis",
"Aring",
"Ccedilla",
"Eacute",
"Ntilde",
"Odieresis",
"Udieresis",
"aacute",
"agrave",
"acircumflex",
"adieresis",
"atilde",
"aring",
"ccedilla",
"eacute",
"egrave",
"ecircumflex",
"edieresis",
"iacute",
"igrave",
"icircumflex",
"idieresis",
"ntilde",
"oacute",
"ograve",
"ocircumflex",
"odieresis",
"otilde",
"uacute",
"ugrave",
"ucircumflex",
"udieresis",
"dagger",
"degree",
"cent",
"sterling",
"section",
"bullet",
"paragraph",
"germandbls",
"registered",
"copyright",
"trademark",
"acute",
"dieresis",
"notequal",
"AE",
"Oslash",
"infinity",
"plusminus",
"lessequal",
"greaterequal",
"yen",
"mu",
"partialdiff",
"summation",
"product",
"pi",
"integral",
"ordfeminine",
"ordmasculine",
"Omega",
"ae",
"oslash",
"questiondown",
"exclamdown",
"logicalnot",
"radical",
"florin",
"approxequal",
"Delta",
"guillemotleft",
"guillemotright",
"ellipsis",
"nonbreakingspace",
"Agrave",
"Atilde",
"Otilde",
"OE",
"oe",
"endash",
"emdash",
"quotedblleft",
"quotedblright",
"quoteleft",
"quoteright",
"divide",
"lozenge",
"ydieresis",
"Ydieresis",
"fraction",
"currency",
"guilsinglleft",
"guilsinglright",
"fi",
"fl",
"daggerdbl",
"periodcentered",
"quotesinglbase",
"quotedblbase",
"perthousand",
"Acircumflex",
"Ecircumflex",
"Aacute",
"Edieresis",
"Egrave",
"Iacute",
"Icircumflex",
"Idieresis",
"Igrave",
"Oacute",
"Ocircumflex",
"apple",
"Ograve",
"Uacute",
"Ucircumflex",
"Ugrave",
"dotlessi",
"circumflex",
"tilde",
"macron",
"breve",
"dotaccent",
"ring",
"cedilla",
"hungarumlaut",
"ogonek",
"caron",
"Lslash",
"lslash",
"Scaron",
"scaron",
"Zcaron",
"zcaron",
"brokenbar",
"Eth",
"eth",
"Yacute",
"yacute",
"Thorn",
"thorn",
"minus",
"multiply",
"onesuperior",
"twosuperior",
"threesuperior",
"onehalf",
"onequarter",
"threequarters",
"franc",
"Gbreve",
"gbreve",
"Idotaccent",
"Scedilla",
"scedilla",
"Cacute",
"cacute",
"Ccaron",
"ccaron",
"dcroat"
};

static unichar_t uniranges[][2] = {
    { 0x20, 0x7e },
    { 0xa0, 0xff },
    { 0x100, 0x17f },
    { 0x180, 0x24f },
    { 0x250, 0x2af },
    { 0x2b0, 0x2ff },
    { 0x300, 0x36f },
    { 0x370, 0x3ff },
    { 0,0 },		/* Obsolete */
    { 0x400, 0x4ff },
    { 0x530, 0x58f },
    { 0x590, 0x5ff },
    { 0,0 },		/* Obsolete */
    { 0x600, 0x6ff },
    { 0,0 },		/* Obsolete */
    { 0x900, 0x97f },
    { 0x980, 0x9ff },
    { 0xa00, 0xa7f },
    { 0xa80, 0xaff },
    { 0xb00, 0xb7f },
    { 0xb80, 0xbff },		/* bit 20, tamil */
    { 0xc00, 0xc7f },
    { 0xc80, 0xcff },
    { 0xd00, 0xd7f },
    { 0xe00, 0xe7f },
    { 0xe80, 0xeff },		/* bit 25, lao */
    { 0x10a0, 0x10ff },
    { 0, 0 },			/* bit 27, obsolete */
    { 0x1100, 0x11ff },
    { 0x1e00, 0x1eff },
    { 0x1f00, 0x1fff },		/* bit 30, greek extended */
    { 0x2000, 0x206f },

    { 0x2070, 0x209f },		/* bit 32, subscripts, super scripts */
    { 0x20a0, 0x20cf },
    { 0x20d0, 0x20ff },
    { 0x2100, 0x214f },
    { 0x2150, 0x218f },
    { 0x2190, 0x21ff },
    { 0x2200, 0x22ff },
    { 0x2300, 0x237f },
    { 0x2400, 0x243f },		/* bit 40 control pictures */
    { 0x2440, 0x245f },
    { 0x2460, 0x24ff },
    { 0x2500, 0x257f },
    { 0x2580, 0x259f },
    { 0x25a0, 0x25ff },		/* bit 45 geometric shapes */
    { 0x2600, 0x267f },
    { 0x2700, 0x27bf },		/* bit 47 zapf dingbats */
    { 0x3000, 0x303f },
    { 0x3040, 0x309f },
    { 0x30a0, 0x30ff },		/* bit 50 katakana */
    { 0x3100, 0x312f },			/* Also includes 3180-31AF */
    { 0x3130, 0x318f },
    { 0x3190, 0x31ff },
    { 0x3200, 0x32ff },
    { 0x3300, 0x33ff },		/* bit 55 CJK compatability */
    { 0xac00, 0xd7af },		/* bit 56 Hangul */
    { 0, 0 },			/* subranges */
    { 0, 0 },			/* subranges */
    { 0x3400, 0x9fff },		/* bit 59, CJK */ /* also 2E80-29FF */
    { 0xE000, 0xF8FF },		/* bit 60, private use */
    { 0xf900, 0xfaff },
    { 0xfb00, 0xfb4f },
    { 0xfb50, 0xfdff },

    { 0xfe20, 0xfe2f },		/* bit 64 combining half marks */
    { 0xfe30, 0xfe4f },
    { 0xfe50, 0xfe6f },
    { 0xfe70, 0xfeef },
    { 0xff00, 0xffef },
    { 0xfff0, 0xffff },
    { 0x0f00, 0x0fff },		/* bit 70 tibetan */
    { 0x0700, 0x074f },
    { 0x0780, 0x07Bf },
    { 0x0D80, 0x0Dbf },
    { 0x1000, 0x109f },
    { 0x1200, 0x12ff },		/* bit 75 ethiopic */
    { 0x13A0, 0x13ff },
    { 0x1400, 0x167f },
    { 0x1680, 0x169f },
    { 0x16A0, 0x16ff },
    { 0x1780, 0x17ff },		/* bit 80 khmer */
    { 0x1800, 0x18af },
    { 0x2800, 0x28ff },
    { 0xA000, 0xa4af },		/* bit 83, Yi & Yi radicals */
    { 0xffff, 0xffff}
};

static int32 getuint32(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    int ch4 = getc(ttf);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

void putshort(FILE *file,int sval) {
    putc((sval>>8)&0xff,file);
    putc(sval&0xff,file);
}

void putlong(FILE *file,int val) {
    putc((val>>24)&0xff,file);
    putc((val>>16)&0xff,file);
    putc((val>>8)&0xff,file);
    putc(val&0xff,file);
}
#define dumpabsoffset	putlong

static void dumpoffset(FILE *file,int offsize,int val) {
    if ( offsize==1 )
	putc(val,file);
    else if ( offsize==2 )
	putshort(file,val);
    else if ( offsize==3 ) {
	putc((val>>16)&0xff,file);
	putc((val>>8)&0xff,file);
	putc(val&0xff,file);
    } else
	putlong(file,val);
}

static void put2d14(FILE *file,real dval) {
    int val;
    int mant;

    val = floor(dval);
    mant = floor(16384.*(dval-val));
    val = (val<<14) | mant;
    putshort(file,val);
}

void putfixed(FILE *file,real dval) {
    int val;
    int mant;

    val = floor(dval);
    mant = floor(65536.*(dval-val));
    val = (val<<16) | mant;
    putlong(file,val);
}

int ttfcopyfile(FILE *ttf, FILE *other, int pos) {
    int ch;
    int ret = 1;

    if ( pos!=ftell(ttf))
	GDrawIError("File Offset wrong for ttf table, %d expected %d", ftell(ttf), pos );
    rewind(other);
    while (( ch = getc(other))!=EOF )
	putc(ch,ttf);
    if ( ferror(other)) ret = 0;
    if ( fclose(other)) ret = 0;
return( ret );
}

static void FigureFullMetricsEnd(SplineFont *sf,struct glyphinfo *gi) {
    /* We can reduce the size of the width array by removing a run at the end */
    /*  of the same width. So start at the end, find the width of the last */
    /*  character we'll output, then run backwards as long as we've got the */
    /*  same width */
    /* (do same thing for vertical metrics too */
    int i,j, maxc, lasti, lastv;
    int width, vwidth;

    maxc = sf->charcnt;
    for ( j=0; j<sf->subfontcnt; ++j )
	if ( sf->subfonts[j]->charcnt>maxc ) maxc = sf->subfonts[j]->charcnt;
    if ( sf->subfontcnt==0 ) {
	for ( i=maxc-1; i>0; --i )
	    if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i]))
	break;
    } else {
	for ( i=maxc-1; i>0; --i ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( i<sf->subfonts[j]->charcnt && sf->subfonts[j]->chars[i]!=NULL )
	    break;
	    if ( j<sf->subfontcnt && SCWorthOutputting(sf->subfonts[j]->chars[i]))
	break;	/* no duplicates in cid font */
	}
    }

    if ( i>0 ) {
	lasti = lastv = i;
	if ( sf->subfontcnt==0 ) {
	    width = sf->chars[i]->width;
	    vwidth = sf->chars[i]->vwidth;
	    for ( --i; i>0; --i ) {
		if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i])) {
		    if ( sf->chars[i]->width!=width )
	    break;
		    else
			lasti = i;
		}
	    }
	    gi->lasthwidth = lasti;
	    if ( sf->hasvmetrics ) {
		for ( i=lastv-1; i>0; --i ) {
		    if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i])) {
			if ( sf->chars[i]->vwidth!=vwidth )
		break;
			else
			    lastv = i;
		    }
		}
		gi->lastvwidth = lastv;
	    }
	} else {
	    width = sf->subfonts[j]->chars[i]->width;
	    vwidth = sf->subfonts[j]->chars[i]->vwidth;
	    for ( --i; i>0; --i ) {
		for ( j=0; j<sf->subfontcnt; ++j )
		    if ( i<sf->subfonts[j]->charcnt && sf->subfonts[j]->chars[i]!=NULL )
		break;
		if ( j<sf->subfontcnt && SCWorthOutputting(sf->subfonts[j]->chars[i])) {
		    if ( sf->subfonts[j]->chars[i]->width!=width )
	    break;
		    else
			lasti = i;
		}
	    }
	    gi->lasthwidth = lasti;
	    if ( sf->hasvmetrics ) {
		for ( i=lastv-1; i>0; --i ) {
		    for ( j=0; j<sf->subfontcnt; ++j )
			if ( i<sf->subfonts[j]->charcnt && sf->subfonts[j]->chars[i]!=NULL )
		    break;
		    if ( j<sf->subfontcnt && SCWorthOutputting(sf->subfonts[j]->chars[i])) {
			if ( sf->subfonts[j]->chars[i]->vwidth!=vwidth )
		break;
			else
			    lastv = i;
		    }
		}
		gi->lastvwidth = lastv;
	    }
	}
    }
}

static void dumpghstruct(struct glyphinfo *gi,struct glyphhead *gh) {

    putshort(gi->glyphs,gh->numContours);
    putshort(gi->glyphs,gh->xmin);
    putshort(gi->glyphs,gh->ymin);
    putshort(gi->glyphs,gh->xmax);
    putshort(gi->glyphs,gh->ymax);
    if ( gh->xmin<gi->xmin ) gi->xmin = gh->xmin;
    if ( gh->ymin<gi->ymin ) gi->ymin = gh->ymin;
    if ( gh->xmax>gi->xmax ) gi->xmax = gh->xmax;
    if ( gh->ymax>gi->ymax ) gi->ymax = gh->ymax;
}

static void ttfdumpmetrics(SplineChar *sc,struct glyphinfo *gi,DBounds *b) {

    if ( sc->enc<=gi->lasthwidth )
	putshort(gi->hmtx,sc->width);
    putshort(gi->hmtx,b->minx);
    if ( sc->parent->hasvmetrics ) {
	if ( sc->enc<=gi->lastvwidth )
	    putshort(gi->vmtx,sc->vwidth);
	putshort(gi->vmtx,sc->parent->vertical_origin-b->maxy);
    }
    if ( sc->enc==gi->lasthwidth )
	gi->hfullcnt = sc->ttf_glyph+1;
    if ( sc->enc==gi->lastvwidth )
	gi->vfullcnt = sc->ttf_glyph+1;
}

static SplineSet *SCttfApprox(SplineChar *sc) {
    SplineSet *head=NULL, *last, *ss, *tss;
    RefChar *ref;

    for ( ss=sc->splines; ss!=NULL; ss=ss->next ) {
	tss = sc->parent->order2 ? SplinePointListCopy1(ss) : SSttfApprox(ss);
	if ( head==NULL ) head = tss;
	else last->next = tss;
	last = tss;
    }
    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) {
	for ( ss=ref->splines; ss!=NULL; ss=ss->next ) {
	    tss = sc->parent->order2 ? SplinePointListCopy1(ss) : SSttfApprox(ss);
	    if ( head==NULL ) head = tss;
	    else last->next = tss;
	    last = tss;
	}
    }
return( head );
}
    
int SSPointCnt(SplineSet *ss) {
    SplinePoint *sp, *first=NULL;
    int cnt;

    for ( sp=ss->first, cnt=0; sp!=first ; ) {
	if ( sp==ss->first || sp->nonextcp || sp->noprevcp ||
		(sp->dontinterpolate || sp->roundx || sp->roundy) ||
		(sp->prevcp.x+sp->nextcp.x)/2!=sp->me.x ||
		(sp->prevcp.y+sp->nextcp.y)/2!=sp->me.y )
	    ++cnt;
	if ( !sp->nonextcp ) ++cnt;
	if ( sp->next==NULL )
    break;
	if ( first==NULL ) first = sp;
	sp = sp->next->to;
    }
return( cnt );
}

#define _On_Curve	1
#define _X_Short	2
#define _Y_Short	4
#define _Repeat		8
#define _X_Same		0x10
#define _Y_Same		0x20

int SSAddPoints(SplineSet *ss,int ptcnt,BasePoint *bp, char *flags) {
    SplinePoint *sp, *first;

    first = NULL;
    for ( sp=ss->first; sp!=first ; ) {
	if ( sp==ss->first || sp->nonextcp || sp->noprevcp ||
		(sp->dontinterpolate || sp->roundx || sp->roundy) ||
		(sp->prevcp.x+sp->nextcp.x)/2!=sp->me.x ||
		(sp->prevcp.y+sp->nextcp.y)/2!=sp->me.y ) {
	    /* If an on curve point is midway between two off curve points*/
	    /*  it may be omitted and will be interpolated on read in */
	    if ( flags!=NULL ) flags[ptcnt] = _On_Curve;
	    bp[ptcnt].x = rint(sp->me.x);
	    bp[ptcnt].y = rint(sp->me.y);
	    sp->ttfindex = ptcnt++;
	}
	if ( !sp->nonextcp ) {
	    if ( flags!=NULL ) flags[ptcnt] = 0;
	    bp[ptcnt].x = rint(sp->nextcp.x);
	    bp[ptcnt++].y = rint(sp->nextcp.y);
	}
	if ( sp->next==NULL )
    break;
	if ( first==NULL ) first = sp;
	sp = sp->next->to;
    }
return( ptcnt );
}

static void dumppointarrays(struct glyphinfo *gi,BasePoint *bp, char *fs, int pc) {
    BasePoint last;
    int i,flags;
    int lastflag, flagcnt;

    if ( gi->maxp->maxPoints<pc )
	gi->maxp->maxPoints = pc;

	/* flags */
    last.x = last.y = 0;
    lastflag = -1; flagcnt = 0;
    for ( i=0; i<pc; ++i ) {
	flags = 0;
	if ( fs==NULL || fs[i] )
	    flags = _On_Curve;		/* points are on curve */
	if ( last.x==bp[i].x )
	    flags |= _X_Same;
	else if ( bp[i].x-last.x>-256 && bp[i].x-last.x<255 ) {
	    flags |= _X_Short;
	    if ( bp[i].x>=last.x )
		flags |= _X_Same;		/* In this context it means positive */
	}
	if ( last.y==bp[i].y )
	    flags |= _Y_Same;
	else if ( bp[i].y-last.y>-256 && bp[i].y-last.y<255 ) {
	    flags |= _Y_Short;
	    if ( bp[i].y>=last.y )
		flags |= _Y_Same;		/* In this context it means positive */	
	}
	last = bp[i];
	if ( lastflag==-1 ) {
	    lastflag = flags;
	    flagcnt = 0;
	} else if ( flags!=lastflag ) {
	    if ( flagcnt!=0 )
		lastflag |= _Repeat;
	    putc(lastflag,gi->glyphs);
	    if ( flagcnt!=0 )
		putc(flagcnt,gi->glyphs);
	    lastflag = flags;
	    flagcnt = 0;
	} else {
	    if ( ++flagcnt == 255 ) {
		putc(lastflag|_Repeat,gi->glyphs);
		putc(255,gi->glyphs);
		lastflag = -1;
		flagcnt = 0;
	    }
	}
    }
    if ( lastflag!=-1 ) {
	if ( flagcnt!=0 )
	    lastflag |= _Repeat;
	putc(lastflag,gi->glyphs);
	if ( flagcnt!=0 )
	    putc(flagcnt,gi->glyphs);
    }

	/* xcoords */
    last.x = 0;
    for ( i=0; i<pc; ++i ) {
	if ( last.x==bp[i].x )
	    /* Do Nothing */;
	else if ( bp[i].x-last.x>-256 && bp[i].x-last.x<255 ) {
	    if ( bp[i].x>=last.x )
		putc(bp[i].x-last.x,gi->glyphs);
	    else
		putc(last.x-bp[i].x,gi->glyphs);
	} else
	    putshort(gi->glyphs,bp[i].x-last.x);
	last.x = bp[i].x;
    }
	/* ycoords */
    last.y = 0;
    for ( i=0; i<pc; ++i ) {
	if ( last.y==bp[i].y )
	    /* Do Nothing */;
	else if ( bp[i].y-last.y>-256 && bp[i].y-last.y<255 ) {
	    if ( bp[i].y>=last.y )
		putc(bp[i].y-last.y,gi->glyphs);
	    else
		putc(last.y-bp[i].y,gi->glyphs);
	} else
	    putshort(gi->glyphs,bp[i].y-last.y);
	last.y = bp[i].y;
    }
    if ( ftell(gi->glyphs)&1 )		/* Pad the file so that the next glyph */
	putc('\0',gi->glyphs);		/* on a word boundary */
}

static void dumpinstrs(struct glyphinfo *gi,uint8 *instrs,int cnt) {
    int i;

    if ( (gi->flags&ttf_flag_nohints) ) {
	putshort(gi->glyphs,0);
return;
    }
    /* Do we ever want to call AutoHint and AutoInst here? I think not. */

    if ( gi->maxp->maxglyphInstr<cnt ) gi->maxp->maxglyphInstr=cnt;
    putshort(gi->glyphs,cnt);
    for ( i=0; i<cnt; ++i )
	putc( instrs[i],gi->glyphs );
}

static void dumpmissingglyph(SplineFont *sf,struct glyphinfo *gi,int fixedwidth) {
    struct glyphhead gh;
    BasePoint bp[10];
    uint8 instrs[50];
    int stemcvt, stem;

    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);
    gi->maxp->maxContours = 2;

    gh.numContours = 2;
    gh.ymin = 0;
    gh.ymax = 2*(sf->ascent+sf->descent)/3;
    gh.xmax = (sf->ascent+sf->descent)/3;
    gh.xmin = stem = gh.xmax/10;
    gh.xmax += stem;
    if ( gh.ymax>sf->ascent ) gh.ymax = sf->ascent;
    dumpghstruct(gi,&gh);

    stemcvt = TTF_getcvtval(gi->sf,stem);

    bp[0].x = stem;		bp[0].y = 0;
    bp[1].x = stem;		bp[1].y = gh.ymax;
    bp[2].x = gh.xmax;		bp[2].y = gh.ymax;
    bp[3].x = gh.xmax;		bp[3].y = 0;

    bp[4].x = 2*stem;		bp[4].y = stem;
    bp[5].x = gh.xmax-stem;	bp[5].y = stem;
    bp[6].x = gh.xmax-stem;	bp[6].y = gh.ymax-stem;
    bp[7].x = 2*stem;		bp[7].y = gh.ymax-stem;

    instrs[0] = 0xb1;			/* Pushb, 2byte */
    instrs[1] = 1;			/* Point 1 */
    instrs[2] = 0;			/* Point 0 */
    instrs[3] = 0x2f;			/* MDAP, rounded (pt0) */
    instrs[4] = 0x3c;			/* ALIGNRP, (pt1 same pos as pt0)*/
    instrs[5] = 0xb2;			/* Pushb, 3byte */
    instrs[6] = 7;			/* Point 7 */
    instrs[7] = 4;			/* Point 4 */
    instrs[8] = stemcvt;		/* CVT entry for our stem width */
    instrs[9] = 0xe0+0x0d;		/* MIRP, don't set rp0, minimum, rounded, black */
    instrs[10] = 0x32;			/* SHP[rp2] (pt7 same pos as pt4) */
    instrs[11] = 0xb1;			/* Pushb, 2byte */
    instrs[12] = 6;			/* Point 6 */
    instrs[13] = 5;			/* Point 5 */
    instrs[14] = 0xc0+0x1c;		/* MDRP, set rp0, minimum, rounded, grey */
    instrs[15] = 0x3c;			/* ALIGNRP, (pt6 same pos as pt5)*/
    instrs[16] = 0xb2;			/* Pushb, 3byte */
    instrs[17] = 3;			/* Point 3 */
    instrs[18] = 2;			/* Point 2 */
    instrs[19] = stemcvt;		/* CVT entry for our stem width */
    instrs[20] = 0xe0+0x0d;		/* MIRP, dont set rp0, minimum, rounded, black */
    instrs[21] = 0x32;			/* SHP[rp2] (pt3 same pos as pt2) */

    instrs[22] = 0x00;			/* SVTCA, y axis */

    instrs[23] = 0xb1;			/* Pushb, 2byte */
    instrs[24] = 3;			/* Point 3 */
    instrs[25] = 0;			/* Point 0 */
    instrs[26] = 0x2f;			/* MDAP, rounded */
    instrs[27] = 0x3c;			/* ALIGNRP, (pt3 same height as pt0)*/
    instrs[28] = 0xb2;			/* Pushb, 3byte */
    instrs[29] = 5;			/* Point 5 */
    instrs[30] = 4;			/* Point 4 */
    instrs[31] = stemcvt;		/* CVT entry for our stem width */
    instrs[32] = 0xe0+0x0d;		/* MIRP, don't set rp0, minimum, rounded, black */
    instrs[33] = 0x32;			/* SHP[rp2] (pt5 same height as pt4) */
    instrs[34] = 0xb2;			/* Pushb, 3byte */
    instrs[35] = 7;			/* Point 7 */
    instrs[36] = 6;			/* Point 6 */
    instrs[37] = TTF_getcvtval(gi->sf,bp[6].y);	/* CVT entry for top height */
    instrs[38] = 0xe0+0x1c;		/* MIRP, set rp0, minimum, rounded, grey */
    instrs[39] = 0x3c;			/* ALIGNRP (pt7 same height as pt6) */
    instrs[40] = 0xb2;			/* Pushb, 3byte */
    instrs[41] = 1;			/* Point 1 */
    instrs[42] = 2;			/* Point 2 */
    instrs[43] = stemcvt;		/* CVT entry for our stem width */
    instrs[44] = 0xe0+0x0d;		/* MIRP, dont set rp0, minimum, rounded, black */
    instrs[45] = 0x32;			/* SHP[rp2] (pt1 same height as pt2) */

	/* We've touched all points in all dimensions */
	/* Don't need any IUP */

	/* end contours array */
    putshort(gi->glyphs,4-1);
    putshort(gi->glyphs,8-1);
	/* instruction length&instructions */
    dumpinstrs(gi,instrs,46);

    dumppointarrays(gi,bp,NULL,8);

    if ( fixedwidth==-1 )
	putshort(gi->hmtx,gh.xmax + 2*stem);
    else
	putshort(gi->hmtx,fixedwidth);
    putshort(gi->hmtx,stem);
    if ( sf->hasvmetrics ) {
	putshort(gi->vmtx,sf->ascent+sf->descent);
	putshort(gi->vmtx,sf->vertical_origin-gh.ymax);
    }
}

static void dumpblankglyph(struct glyphinfo *gi,SplineFont *sf,int fixedwidth) {
    int advance = gi->next_glyph==1?0:fixedwidth==-1?(sf->ascent+sf->descent)/3:
	    fixedwidth;
    /* These don't get a glyph header, because there are no contours */
    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);
    putshort(gi->hmtx,advance);
    putshort(gi->hmtx,0);
    if ( sf->hasvmetrics ) {
	putshort(gi->vmtx,gi->next_glyph==2?0:(sf->ascent+sf->descent));
	putshort(gi->vmtx,0);
    }
}

static void dumpspace(SplineChar *sc, struct glyphinfo *gi) {
    /* These don't get a glyph header, because there are no contours */
    DBounds b;
    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);
    memset(&b,0,sizeof(b));
    ttfdumpmetrics(sc,gi,&b);
}

static void dumpcomposite(SplineChar *sc, RefChar *refs, struct glyphinfo *gi) {
    struct glyphhead gh;
    DBounds bb;
    int i, ptcnt, ctcnt, flags;
    SplineSet *ss;
    RefChar *ref;
    int any = false;

#if 0
    if ( autohint_before_generate && sc->changedsincelasthinted &&
	    !sc->manualhints )
	if ( !(gi->flags&ttf_flag_nohints) )
	    SplineCharAutoHint(sc,true);
#endif

    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);

    SplineCharFindBounds(sc,&bb);
    gh.numContours = -1;
    gh.xmin = floor(bb.minx); gh.ymin = floor(bb.miny);
    gh.xmax = ceil(bb.maxx); gh.ymax = ceil(bb.maxy);
    dumpghstruct(gi,&gh);

    i=ptcnt=ctcnt=0;
    for ( ref=refs; ref!=NULL; ref=ref->next, ++i ) {
	if ( ref->sc->ttf_glyph==-1 ) {
	    if ( refs->next==NULL || any )
    continue;
	}
	flags = (1<<1)|(1<<2)|(1<<12);	/* Args are always values for me */
					/* Always round args to grid */
	    /* There is some very strange stuff wrongly-documented on the apple*/
	    /*  site about how these should be interpretted when there are */
	    /*  scale factors, or rotations */
	    /* That description does not match the behavior of their rasterizer*/
	    /*  I've reverse engineered something else (see parsettf.c) */
	    /*  http://fonts.apple.com/TTRefMan/RM06/Chap6glyf.html */
	    /* Adobe says that setting bit 12 means that this will not happen */
	    /*  Apple doesn't mention bit 12 though...(but they do support it) */
/* if we want a mac style scaled composite then
	flags = (1<<1)|(1<<2)|(1<<11);
    and if we want an ambiguous composite...
	flags = (1<<1)|(1<<2);
*/
	if ( ref->next!=NULL )
	    flags |= (1<<5);		/* More components */
	else if ( sc->ttf_instrs_len!=0 )	/* Composits also inherit instructions */
	    flags |= (1<<8);		/* Instructions appear after last ref */
	if ( ref->transform[1]!=0 || ref->transform[2]!=0 )
	    flags |= (1<<7);		/* Need a full matrix */
	else if ( ref->transform[0]!=ref->transform[3] )
	    flags |= (1<<6);		/* different xy scales */
	else if ( ref->transform[0]!=1. )
	    flags |= (1<<3);		/* xy scale is same */
	if ( ref->transform[4]<-128 || ref->transform[4]>127 ||
		ref->transform[5]<-128 || ref->transform[5]>127 )
	    flags |= (1<<0);
	putshort(gi->glyphs,flags);
	putshort(gi->glyphs,ref->sc->ttf_glyph==-1?0:ref->sc->ttf_glyph);
	if ( flags&(1<<0) ) {
	    putshort(gi->glyphs,(short)ref->transform[4]);
	    putshort(gi->glyphs,(short)ref->transform[5]);
	} else {
	    putc((char) (ref->transform[4]),gi->glyphs);
	    putc((char) (ref->transform[5]),gi->glyphs);
	}
	if ( flags&(1<<7) ) {
	    put2d14(gi->glyphs,ref->transform[0]);
	    put2d14(gi->glyphs,ref->transform[1]);
	    put2d14(gi->glyphs,ref->transform[2]);
	    put2d14(gi->glyphs,ref->transform[3]);
	} else if ( flags&(1<<6) ) {
	    put2d14(gi->glyphs,ref->transform[0]);
	    put2d14(gi->glyphs,ref->transform[3]);
	} else if ( flags&(1<<3) ) {
	    put2d14(gi->glyphs,ref->transform[0]);
	}
	for ( ss=ref->splines; ss!=NULL ; ss=ss->next ) {
	    ++ctcnt;
	    ptcnt += SSPointCnt(ss);
	}
    }

    if ( sc->ttf_instrs_len!=0 )
	dumpinstrs(gi,sc->ttf_instrs,sc->ttf_instrs_len);

    if ( gi->maxp->maxnumcomponents<i ) gi->maxp->maxnumcomponents = i;
	/* Assume every font has at least one reference character */
	/* PfaEdit will do a transitive closeur so that we end up with */
	/*  a maximum depth of 1 (according to apple) or 2 (according to Opentype) */
    gi->maxp->maxcomponentdepth = /* Apple docs say: 1, Open type docs say: */ 2;
    if ( gi->maxp->maxCompositPts<ptcnt ) gi->maxp->maxCompositPts=ptcnt;
    if ( gi->maxp->maxCompositCtrs<ctcnt ) gi->maxp->maxCompositCtrs=ctcnt;

    ttfdumpmetrics(sc,gi,&bb);
    if ( ftell(gi->glyphs)&1 )		/* Pad the file so that the next glyph */
	putc('\0',gi->glyphs);		/* on a word boundary, can only happen if odd number of instrs */
    RefCharsFreeRef(refs);
}

static void dumpglyph(SplineChar *sc, struct glyphinfo *gi) {
    struct glyphhead gh;
    DBounds bb;
    SplineSet *ss, *ttfss;
    int contourcnt, ptcnt;
    BasePoint *bp;
    char *fs;

/* I haven't seen this documented, but ttf rasterizers are unhappy with a */
/*  glyph that consists of a single point. Glyphs containing two single points*/
/*  are ok, glyphs with a single point and anything else are ok, glyphs with */
/*  a line are ok. But a single point is not ok. Dunno why */
    if (( sc->splines==NULL && sc->refs==NULL ) ||
	    ( sc->refs==NULL &&
	     (sc->splines->first->next==NULL ||
	      sc->splines->first->next->to == sc->splines->first) &&
	     sc->splines->next==NULL) ) {
	dumpspace(sc,gi);
return;
    }

    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);

    if ( sc->ttf_instrs!=NULL )
	initforinstrs(sc);

    contourcnt = ptcnt = 0;
    ttfss = SCttfApprox(sc);
    for ( ss=ttfss; ss!=NULL; ss=ss->next ) {
	++contourcnt;
	ptcnt += SSPointCnt(ss);
    }

    SplineCharFindBounds(sc,&bb);
    gh.numContours = contourcnt;
    gh.xmin = floor(bb.minx); gh.ymin = floor(bb.miny);
    gh.xmax = ceil(bb.maxx); gh.ymax = ceil(bb.maxy);
    dumpghstruct(gi,&gh);
    if ( contourcnt>gi->maxp->maxContours ) gi->maxp->maxContours = contourcnt;
    if ( ptcnt>gi->maxp->maxPoints ) gi->maxp->maxPoints = ptcnt;

    bp = galloc(ptcnt*sizeof(BasePoint));
    fs = galloc(ptcnt);
    ptcnt = contourcnt = 0;
    for ( ss=ttfss; ss!=NULL; ss=ss->next ) {
	ptcnt = SSAddPoints(ss,ptcnt,bp,fs);
	putshort(gi->glyphs,ptcnt-1);
    }

    dumpinstrs(gi,sc->ttf_instrs,sc->ttf_instrs_len);
	
    dumppointarrays(gi,bp,fs,ptcnt);
    SplinePointListsFree(ttfss);
    free(bp);
    free(fs);

    ttfdumpmetrics(sc,gi,&bb);
}

static int AssignTTFGlyph(SplineFont *sf,int32 *bsizes) {
    int i, tg, j;
    BDFFont *bdf;

    tg = 3;
    /* The first three glyphs are magic, glyph 0 might appear in the font */
    /*  but glyph 1,2 never do (they correspond to NUL and CR respectively) */
    /*  We generate them automagically */

    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( j=0; bsizes[j]!=0 && ((bsizes[j]&0xffff)!=bdf->pixelsize || (bsizes[j]>>16)!=BDFDepth(bdf)); ++j );
	if ( bsizes[j]==0 )
    continue;
	for ( i=0; i<bdf->charcnt; ++i ) if ( !IsntBDFChar(bdf->chars[i]) )
	    sf->chars[i]->ttf_glyph = 0;
    }
    /* If we are to use glyph 0, then it will already have the right ttf_glyph*/
    /*  (0), while if we aren't to use it, it again will be right (-1) */
    for ( i=1; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph==0 )
	sf->chars[i]->ttf_glyph = tg++;
return( tg );
}

static int dumpglyphs(SplineFont *sf,struct glyphinfo *gi) {
    int i, cnt;
    RefChar *refs;
    int fixed = gi->fixed_width;

    GProgressChangeStages(2+gi->strikecnt);
    QuickBlues(sf,&gi->bd);
    /*FindBlues(sf,gi->blues,NULL);*/
    GProgressNextStage();

    if ( gi->onlybitmaps )
	cnt = AssignTTFGlyph(sf,gi->bsizes);
    else {
	i=0, cnt=0;
	if ( SCIsNotdef(sf->chars[0],fixed) )
	    sf->chars[i++]->ttf_glyph = cnt++;
	for ( cnt=3; i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i]))
		sf->chars[i]->ttf_glyph = cnt++;
    }

    gi->maxp->numGlyphs = cnt;
    gi->loca = galloc((gi->maxp->numGlyphs+1)*sizeof(uint32));
    gi->next_glyph = 0;
    gi->glyphs = tmpfile();
    gi->hmtx = tmpfile();
    if ( sf->hasvmetrics )
	gi->vmtx = tmpfile();
    FigureFullMetricsEnd(sf,gi);

    i = 0;
    if ( SCIsNotdef(sf->chars[0],fixed) && !gi->onlybitmaps )
	dumpglyph(sf->chars[i++],gi);
    else
	dumpmissingglyph(sf,gi,fixed);
    if ( fixed!=-1 ) {
	gi->lasthwidth = -1;
	gi->hfullcnt = 3;
    }
    dumpblankglyph(gi,sf,fixed);	/* I'm not sure exactly why but there seem */
    dumpblankglyph(gi,sf,fixed);	/* to be a couple of blank glyphs at the start*/
    /* One is for NUL and one for CR I think... but why? */
    for ( cnt=0; i<sf->charcnt; ++i ) {
	if ( gi->onlybitmaps ) {
	    if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph>0 )
		dumpspace(sf->chars[i],gi);
	} else {
	    if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i])) {
		if ( (refs = SCCanonicalRefs(sf->chars[i],false))!=NULL )
		    dumpcomposite(sf->chars[i],refs,gi);
		else
		    dumpglyph(sf->chars[i],gi);
	    }
	}
	if ( !GProgressNext())
return( false );
    }

    /* extra location entry points to end of last glyph */
    gi->loca[gi->next_glyph] = ftell(gi->glyphs);
    gi->glyph_len = ftell(gi->glyphs);
    gi->hmtxlen = ftell(gi->hmtx);
    /* pad out to four bytes */
    if ( gi->hmtxlen&2 ) putshort(gi->hmtx,0);
    if ( gi->loca[gi->next_glyph]&3 ) {
	for ( i=4-(gi->loca[gi->next_glyph]&3); i>0; --i )
	    putc('\0',gi->glyphs);
    }
    if ( sf->hasvmetrics ) {
	gi->vmtxlen = ftell(gi->vmtx);
	if ( gi->vmtxlen&2 ) putshort(gi->vmtx,0);
    }
return( true );
}

/* Standard names for cff */
extern const char *cffnames[];
extern const int nStdStrings;

static int storesid(struct alltabs *at,char *str) {
    int i;
    FILE *news;
    char *pt;
    long pos;

    if ( str!=NULL ) {			/* NULL is the magic string at end of array */
	for ( i=0; cffnames[i]!=NULL; ++i ) {
	    if ( strcmp(cffnames[i],str)==0 )
return( i );
	}
    }

    pos = ftell(at->sidf)+1;
    if ( pos>=65536 && !at->sidlongoffset ) {
	at->sidlongoffset = true;
	news = tmpfile();
	rewind(at->sidh);
	for ( i=0; i<at->sidcnt; ++i )
	    putlong(news,getushort(at->sidh));
	fclose(at->sidh);
	at->sidh = news;
    }
    if ( at->sidlongoffset )
	putlong(at->sidh,pos);
    else
	putshort(at->sidh,pos);

    if ( str!=NULL ) {
	for ( pt=str; *pt; ++pt )
	    putc(*pt,at->sidf);
    }
return( at->sidcnt++ + nStdStrings );
}

static void dumpint(FILE *cfff,int num) {

    if ( num>=-107 && num<=107 )
	putc(num+139,cfff);
    else if ( num>=108 && num<=1131 ) {
	num -= 108;
	putc((num>>8)+247,cfff);
	putc(num&0xff,cfff);
    } else if ( num>=-1131 && num<=-108 ) {
	num = -num;
	num -= 108;
	putc((num>>8)+251,cfff);
	putc(num&0xff,cfff);
    } else if ( num>=-32768 && num<32768 ) {
	putc(28,cfff);
	putc(num>>8,cfff);
	putc(num&0xff,cfff);
    } else {		/* In dict data we have 4 byte ints, in type2 strings we don't */
	putc(29,cfff);
	putc((num>>24)&0xff,cfff);
	putc((num>>16)&0xff,cfff);
	putc((num>>8)&0xff,cfff);
	putc(num&0xff,cfff);
    }
}

static void dumpdbl(FILE *cfff,double d) {
    if ( d-rint(d)>-.00001 && d-rint(d)<.00001 )
	dumpint(cfff,(int) d);
    else {
	/* The type2 strings have a fixed format, but the dict data does not */
	char buffer[20], *pt;
	int sofar,n,odd;
	sprintf( buffer, "%g", d);
	sofar = 0; odd=true;
	putc(30,cfff);		/* Start a double */
	for ( pt=buffer; *pt; ++pt ) {
	    if ( isdigit(*pt) )
		n = *pt-'0';
	    else if ( *pt=='.' )
		n = 0xa;
	    else if ( *pt=='-' )
		n = 0xe;
	    else if (( *pt=='E' || *pt=='e') && pt[1]=='-' ) {
		n = 0xc;
		++pt;
	    } else if ( *pt=='E' || *pt=='e')
		n = 0xb;
	    if ( odd ) {
		sofar = n<<4;
		odd = false;
	    } else {
		putc(sofar|n,cfff);
		sofar=0;
		odd = true;
	    }
	}
	if ( sofar==0 )
	    putc(0xff,cfff);
	else
	    putc(sofar|0xf,cfff);
    }
}

static void dumpoper(FILE *cfff,int oper ) {
    if ( oper!=-1 ) {
	if ( oper>=256 )
	    putc(oper>>8,cfff);
	putc(oper&0xff,cfff);
    }
}

static void dumpdbloper(FILE *cfff,double d, int oper ) {
    dumpdbl(cfff,d);
    dumpoper(cfff,oper);
}

static void dumpintoper(FILE *cfff,int v, int oper ) {
    dumpint(cfff,v);
    dumpoper(cfff,oper);
}

static void dumpsizedint(FILE *cfff,int big,int num, int oper ) {
    if ( big ) {
	putc(29,cfff);
	putc((num>>24)&0xff,cfff);
	putc((num>>16)&0xff,cfff);
	putc((num>>8)&0xff,cfff);
	putc(num&0xff,cfff);
    } else {
	putc(28,cfff);
	putc(num>>8,cfff);
	putc(num&0xff,cfff);
    }
    dumpoper(cfff,oper);
}

static void dumpsid(FILE *cfff,struct alltabs *at,char *str,int oper) {
    if ( str==NULL )
return;
    dumpint(cfff,storesid(at,str));
    dumpoper(cfff,oper);
}

static void DumpStrDouble(char *pt,FILE *cfff,int oper) {
    real d = strtod(pt,NULL);
    dumpdbloper(cfff,d,oper);
}

static void DumpDblArray(real *arr,int n,FILE *cfff, int oper) {
    int mi,i;

    for ( mi=n-1; mi>=0 && arr[mi]==0; --mi );
    if ( mi<0 )
return;
    dumpdbl(cfff,arr[0]);
    for ( i=1; i<=mi; ++i )
	dumpdbl(cfff,arr[i]-arr[i-1]);
    dumpoper(cfff,oper);
}

static void DumpStrArray(char *pt,FILE *cfff,int oper) {
    real d, last=0;
    char *end;

    while ( *pt==' ' ) ++pt;
    if ( *pt=='\0' )
return;
    if ( *pt=='[' ) ++pt;
    while ( *pt==' ' ) ++pt;
    while ( *pt!=']' && *pt!='\0' ) {
	d = strtod(pt,&end);
	if ( pt==end )		/* User screwed up. Should be a number */
    break;
	dumpdbl(cfff,d-last);
	last = d;
	pt = end;
	while ( *pt==' ' ) ++pt;
    }
    dumpoper(cfff,oper);
}

static void dumpcffheader(SplineFont *sf,FILE *cfff) {
    putc('\1',cfff);		/* Major version: 1 */
    putc('\0',cfff);		/* Minor version: 0 */
    putc('\4',cfff);		/* Header size in bytes */
    putc('\4',cfff);		/* Absolute Offset size. */
	/* I don't think there are any absolute offsets that aren't encoded */
	/*  in a dict as numbers (ie. inherently variable sized items) */
}

static void dumpcffnames(SplineFont *sf,FILE *cfff) {
    char *pt;

    putshort(cfff,1);		/* One font name */
    putc('\1',cfff);		/* Offset size */
    putc('\1',cfff);		/* Offset to first name */
    putc('\1'+strlen(sf->fontname),cfff);
    for ( pt=sf->fontname; *pt; ++pt )
	putc(*pt,cfff);
}

static void dumpcffcharset(SplineFont *sf,struct alltabs *at) {
    int i;

    putc(0,at->charset);
    /* I always use a format 0 charset. ie. an array of SIDs in random order */

    /* First element must be ".notdef" and is omitted */
    /* So if glyph 0 isn't notdef do something special */
    if ( !SCIsNotdef(sf->chars[0],at->gi.fixed_width) && SCWorthOutputting(sf->chars[0]) )
	putshort(at->charset,storesid(at,sf->chars[0]->name));

    for ( i=1; i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]) )
	    putshort(at->charset,storesid(at,sf->chars[i]->name));
}

static void dumpcffcidset(SplineFont *sf,struct alltabs *at) {
    int cid, k, max=0, start;

    putc(2,at->charset);

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( sf->subfonts[k]->charcnt>max ) max = sf->subfonts[k]->charcnt;

    start = -1;			/* Glyph 0 always maps to CID 0, and is omitted */
    for ( cid = 1; cid<max; ++cid ) {
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    if ( cid<sf->subfonts[k]->charcnt &&
		SCWorthOutputting(sf->subfonts[k]->chars[cid]) )
	break;
	}
	if ( k==sf->subfontcnt ) {
	    if ( start!=-1 ) {
		putshort(at->charset,start);
		putshort(at->charset,cid-1-start);	/* Count of glyphs in range excluding first */
		start = -1;
	    }
	} else {
	    if ( start==-1 ) start = cid;
	}
    }
    if ( start!=-1 ) {
	putshort(at->charset,start);
	putshort(at->charset,cid-1-start);	/* Count of glyphs in range excluding first */
    }
}

static void dumpcfffdselect(SplineFont *sf,struct alltabs *at) {
    int cid, k, max=0, lastfd, cnt;
    int gid;

    putc(3,at->fdselect);
    putshort(at->fdselect,0);		/* number of ranges, fill in later */

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( sf->subfonts[k]->charcnt>max ) max = sf->subfonts[k]->charcnt;

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( SCWorthOutputting(sf->subfonts[k]->chars[0]))
    break;
    if ( k==sf->subfontcnt ) --k;	/* If CID 0 not defined, put it in last font */
    putshort(at->fdselect,0);
    putc(k,at->fdselect);
    lastfd = k;
    cnt = 1;
    for ( cid = gid = 1; cid<max; ++cid ) {
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    if ( cid<sf->subfonts[k]->charcnt &&
		SCWorthOutputting(sf->subfonts[k]->chars[cid]) )
	break;
	}
	if ( k==sf->subfontcnt )
	    /* Doesn't map to a glyph, irrelevant */;
	else {
	    if ( k!=lastfd ) {
		putshort(at->fdselect,gid);
		putc(k,at->fdselect);
		lastfd = k;
		++cnt;
	    }
	    ++gid;
	}
    }
    putshort(at->fdselect,gid);
    fseek(at->fdselect,1,SEEK_SET);
    putshort(at->fdselect,cnt);
    fseek(at->fdselect,0,SEEK_END);
}

static void dumpcffencoding(SplineFont *sf,struct alltabs *at) {
    int i,offset, pos;

    putc(0,at->encoding);
    /* I always use a format 0 encoding. ie. an array of glyph indexes */
    putc(0xff,at->encoding);
    /* And I put in 255 of them (with the encoding for 0 implied, I hope) */

    offset = 0;
    if ( SCIsNotdef(sf->chars[0],at->gi.fixed_width))
	offset = 1;

    for ( i=pos=1; i<sf->charcnt && i<256; ++i )
	if ( SCWorthOutputting(sf->chars[i]) )
	    putc(pos++ +offset,at->encoding);
	else
	    putc(0,at->encoding);
    for ( ; i<256; ++i )
	putc(0,at->encoding);
}

static void _dumpcffstrings(FILE *file, struct pschars *strs) {
    int i, len, offsize;

    /* First figure out the offset size */
    len = 1;
    for ( i=0; i<strs->next; ++i )
	len += strs->lens[i];

    /* Then output the index size and offsets */
    putshort( file, strs->next );
    if ( strs->next!=0 ) {
	offsize = len<=255?1:len<=65535?2:len<=0xffffff?3:4;
	putc(offsize,file);
	len = 1;
	for ( i=0; i<strs->next; ++i ) {
	    dumpoffset(file,offsize,len);
	    len += strs->lens[i];
	}
	dumpoffset(file,offsize,len);

	/* last of all the strings */
	for ( i=0; i<strs->next; ++i ) {
	    uint8 *pt = strs->values[i], *end = pt+strs->lens[i];
	    while ( pt<end )
		putc( *pt++, file );
	}
    }
}

static FILE *dumpcffstrings(struct pschars *strs) {
    FILE *file = tmpfile();
    _dumpcffstrings(file,strs);
    PSCharsFree(strs);
return( file );
}

static void dumpcffprivate(SplineFont *sf,struct alltabs *at,int subfont) {
    char *pt;
    FILE *private = subfont==-1?at->private:at->fds[subfont].private;
    int mi,i,j,cnt, allsame=true, sameval = 0x8000000;
    real bluevalues[14], otherblues[10];
    real snapcnt[12];
    real stemsnaph[12], stemsnapv[12];
    real stdhw[1], stdvw[1];
    int hasblue=0, hash=0, hasv=0, bs;
    int maxw=0;
    uint16 *widths; uint32 *cumwid;
    int nomwid, defwid;

    /* The private dict is not in an index, so no index header. Just the data */

    for ( i=0; i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]) ) {
	    if ( maxw<sf->chars[i]->width ) maxw = sf->chars[i]->width;
	    if ( sameval == 0x8000000 )
		sameval = sf->chars[i]->width;
	    else if ( sameval!=sf->chars[i]->width )
		allsame = false;
	}
    if ( allsame ) {
	nomwid = defwid = sameval;
    } else {
	++maxw;
	if ( maxw>65535 ) maxw = 3*(sf->ascent+sf->descent);
	widths = gcalloc(maxw,sizeof(uint16));
	cumwid = gcalloc(maxw,sizeof(uint32));
	defwid = 0; cnt=0;
	for ( i=0; i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) &&
		    sf->chars[i]->width>=0 &&
		    sf->chars[i]->width<maxw )
		if ( ++widths[sf->chars[i]->width] > cnt ) {
		    defwid = sf->chars[i]->width;
		    cnt = widths[defwid];
		}
	widths[defwid] = 0;
	for ( i=0; i<maxw; ++i )
		for ( j=-107; j<=107; ++j )
		    if ( i+j>=0 && i+j<maxw )
			cumwid[i] += widths[i+j];
	cnt = 0; nomwid = 0;
	for ( i=0; i<maxw; ++i )
	    if ( cnt<cumwid[i] ) {
		cnt = cumwid[i];
		nomwid = i;
	    }
	free(widths); free(cumwid);
    }
    dumpintoper(private,defwid,20);		/* Default Width */
    if ( subfont==-1 )
	at->defwid = defwid;
    else
	at->fds[subfont].defwid = defwid;
    dumpintoper(private,nomwid,21);		/* Nominative Width */
    if ( subfont==-1 )
	at->nomwid = nomwid;
    else
	at->fds[subfont].nomwid = nomwid;

    bs = SplineFontIsFlexible(sf);
    hasblue = PSDictHasEntry(sf->private,"BlueValues")!=NULL;
    hash = PSDictHasEntry(sf->private,"StdHW")!=NULL;
    hasv = PSDictHasEntry(sf->private,"StdVW")!=NULL;
    GProgressChangeStages(2+autohint_before_generate+!hasblue);
    if ( autohint_before_generate ) {
	GProgressChangeLine1R(_STR_AutoHintingFont);
	SplineFontAutoHint(sf);
	GProgressNextStage();
    }

    if ( !hasblue ) {
	FindBlues(sf,bluevalues,otherblues);
	GProgressNextStage();
    }

    stdhw[0] = stdvw[0] = 0;
    if ( !hash ) {
	FindHStems(sf,stemsnaph,snapcnt);
	mi = -1;
	for ( i=0; stemsnaph[i]!=0 && i<12; ++i )
	    if ( mi==-1 ) mi = i;
	    else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
	if ( mi!=-1 ) stdhw[0] = stemsnaph[mi];
    }

    if ( !hasv ) {
	FindVStems(sf,stemsnapv,snapcnt);
	mi = -1;
	for ( i=0; stemsnapv[i]!=0 && i<12; ++i )
	    if ( mi==-1 ) mi = i;
	    else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
	if ( mi!=-1 ) stdvw[0] = stemsnapv[mi];
    }
    GProgressChangeLine1R(_STR_SavingOpenTypeFont);

    if ( hasblue )
	DumpStrArray(PSDictHasEntry(sf->private,"BlueValues"),private,6);
    else
	DumpDblArray(bluevalues,sizeof(bluevalues)/sizeof(bluevalues[0]),private,6);
    if ( (pt=PSDictHasEntry(sf->private,"OtherBlues"))!=NULL )
	DumpStrArray(pt,private,7);
    else if ( !hasblue )
	DumpDblArray(otherblues,sizeof(otherblues)/sizeof(otherblues[0]),private,7);
    if ( (pt=PSDictHasEntry(sf->private,"FamilyBlues"))!=NULL )
	DumpStrArray(pt,private,8);
    if ( (pt=PSDictHasEntry(sf->private,"FamilyOtherBlues"))!=NULL )
	DumpStrArray(pt,private,9);
    if ( (pt=PSDictHasEntry(sf->private,"BlueScale"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+9);
    if ( (pt=PSDictHasEntry(sf->private,"BlueShift"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+10);
    else
	dumpintoper(private,bs,(12<<8)+10);
    if ( (pt=PSDictHasEntry(sf->private,"BlueFuzz"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+11);
    if ( hash ) {
	DumpStrDouble(PSDictHasEntry(sf->private,"StdHW"),private,10);
	if ( (pt=PSDictHasEntry(sf->private,"StemSnapH"))!=NULL )
	    DumpStrArray(pt,private,(12<<8)|12);
    } else {
	if ( stdhw[0]!=0 )
	    dumpdbloper(private,stdhw[0],10);
	DumpDblArray(stemsnaph,sizeof(stemsnaph)/sizeof(stemsnaph[0]),private,(12<<8)|12);
    }
    if ( hasv ) {
	DumpStrDouble(PSDictHasEntry(sf->private,"StdVW"),private,11);
	if ( (pt=PSDictHasEntry(sf->private,"StemSnapV"))!=NULL )
	    DumpStrArray(pt,private,(12<<8)|13);
    } else {
	if ( stdvw[0]!=0 )
	    dumpdbloper(private,stdvw[0],11);
	DumpDblArray(stemsnapv,sizeof(stemsnapv)/sizeof(stemsnapv[0]),private,(12<<8)|13);
    }
    if ( (pt=PSDictHasEntry(sf->private,"ForceBold"))!=NULL ) {
	dumpintoper(private,*pt=='t'||*pt=='T',(12<<8)|14);
    } else if ( sf->weight!=NULL &&
	    (strstrmatch(sf->weight,"Bold")!=NULL ||
	     strstrmatch(sf->weight,"Demi")!=NULL ||
	     strstrmatch(sf->weight,"Fett")!=NULL ||
	     strstrmatch(sf->weight,"Gras")!=NULL ||
	     strstrmatch(sf->weight,"Heavy")!=NULL ||
	     strstrmatch(sf->weight,"Black")!=NULL))
	dumpintoper(private,1,(12<<8)|14);
    if ( (pt=PSDictHasEntry(sf->private,"LanguageGroup"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+17);
    else if ( sf->encoding_name>=em_first2byte && sf->encoding_name<em_unicode )
	dumpintoper(private,1,(12<<8)|17);
    if ( (pt=PSDictHasEntry(sf->private,"ExpansionFactor"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+18);
    dumpsizedint(private,false,ftell(private)+3+1,19);	/* Subrs */

    if ( subfont==-1 )
	at->privatelen = ftell(private);
    else
	at->fds[subfont].privatelen = ftell(private);
}

/* When we exit this the topdict is not complete, we still need to fill in */
/*  values for charset,encoding,charstrings and private. Then we need to go */
/*  back and fill in the table length (at lenpos) */
static void dumpcfftopdict(SplineFont *sf,struct alltabs *at) {
    char *pt, *end;
    FILE *cfff = at->cfff;
    DBounds b;

    putshort(cfff,1);		/* One top dict */
    putc('\2',cfff);		/* Offset size */
    putshort(cfff,1);		/* Offset to topdict */
    at->lenpos = ftell(cfff);
    putshort(cfff,0);		/* placeholder for final position (final offset in index points beyond last element) */
    dumpsid(cfff,at,sf->version,0);
    dumpsid(cfff,at,sf->copyright,1);
    dumpsid(cfff,at,sf->fullname?sf->fullname:sf->fontname,2);
    dumpsid(cfff,at,sf->familyname,3);
    dumpsid(cfff,at,sf->weight,4);
    if ( at->gi.fixed_width!=-1 ) dumpintoper(cfff,1,(12<<8)|1);
    if ( sf->italicangle!=0 ) dumpdbloper(cfff,sf->italicangle,(12<<8)|2);
    if ( sf->upos!=-100 ) dumpdbloper(cfff,sf->upos,(12<<8)|3);
    if ( sf->uwidth!=50 ) dumpdbloper(cfff,sf->uwidth,(12<<8)|4);
    /* We'll never set painttype */
    /* We'll never set CharstringType */
    if ( sf->ascent+sf->descent!=1000 ) {
	dumpdbl(cfff,1.0/(sf->ascent+sf->descent));
	dumpint(cfff,0);
	dumpint(cfff,0);
	dumpdbl(cfff,1.0/(sf->ascent+sf->descent));
	dumpint(cfff,0);
	dumpintoper(cfff,0,(12<<8)|7);
    }
    dumpintoper(cfff, sf->uniqueid?sf->uniqueid:4000000 + (rand()&0x3ffff), 13 );
    SplineFontFindBounds(sf,&b);
    at->gi.xmin = b.minx;
    at->gi.ymin = b.miny;
    at->gi.xmax = b.maxx;
    at->gi.ymax = b.maxy;
    dumpdbl(cfff,floor(b.minx));
    dumpdbl(cfff,floor(b.miny));
    dumpdbl(cfff,ceil(b.maxx));
    dumpdbloper(cfff,ceil(b.maxy),5);
    /* We'll never set StrokeWidth */
    if ( sf->xuid!=NULL ) {
	pt = sf->xuid; if ( *pt=='[' ) ++pt;
	while ( *pt && *pt!=']' ) {
	    dumpint(cfff,strtol(pt,&end,10));
	    for ( pt = end; *pt==' '; ++pt );
	}
	putc(14,cfff);
	if ( sf->changed_since_xuidchanged )
	    SFIncrementXUID(sf);
    }
    /* Offset to charset (oper=15) needed here */
    /* Offset to encoding (oper=16) needed here (not for CID )*/
    /* Offset to charstrings (oper=17) needed here */
    /* Length of, and Offset to private (oper=18) needed here (not for CID )*/
}

static int dumpcffdict(SplineFont *sf,struct alltabs *at) {
    FILE *fdarray = at->fdarray;
    int pstart;
    /* according to the PSRef Man v3, only fontname, fontmatrix and private */
    /*  appear in this dictionary */

    dumpsid(fdarray,at,sf->fontname,(12<<8)|38);
    if ( sf->ascent+sf->descent!=1000 ) {
	dumpdbl(fdarray,1.0/(sf->ascent+sf->descent));
	dumpint(fdarray,0);
	dumpint(fdarray,0);
	dumpdbl(fdarray,1.0/(sf->ascent+sf->descent));
	dumpint(fdarray,0);
	dumpintoper(fdarray,0,(12<<8)|7);
    }
    pstart = ftell(fdarray);
    dumpsizedint(fdarray,false,0,-1);	/* private length */
    dumpsizedint(fdarray,true,0,18);	/* private offset */
return( pstart );
}

static void dumpcffdictindex(SplineFont *sf,struct alltabs *at) {
    int i;
    int pos;

    putshort(at->fdarray,sf->subfontcnt);
    putc('\2',at->fdarray);		/* DICTs aren't very big, and there are at most 255 */
    putshort(at->fdarray,1);		/* Offset to first dict */
    for ( i=0; i<sf->subfontcnt; ++i )
	putshort(at->fdarray,0);	/* Dump offset placeholders (note there's one extra to mark the end) */
    pos = ftell(at->fdarray)-1;
    for ( i=0; i<sf->subfontcnt; ++i ) {
	at->fds[i].fillindictmark = dumpcffdict(sf->subfonts[i],at);
	at->fds[i].eodictmark = ftell(at->fdarray);
	if ( at->fds[i].eodictmark>65536 )
	    GDrawIError("The DICT INDEX got too big, result won't work");
    }
    fseek(at->fdarray,2*sizeof(short)+sizeof(char),SEEK_SET);
    for ( i=0; i<sf->subfontcnt; ++i )
	putshort(at->fdarray,at->fds[i].eodictmark-pos);
    fseek(at->fdarray,0,SEEK_END);
}

static void dumpcffcidtopdict(SplineFont *sf,struct alltabs *at) {
    char *pt, *end;
    FILE *cfff = at->cfff;
    DBounds b;
    int cidcnt=0, k;

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( sf->subfonts[k]->charcnt>cidcnt ) cidcnt = sf->subfonts[k]->charcnt;

    putshort(cfff,1);		/* One top dict */
    putc('\2',cfff);		/* Offset size */
    putshort(cfff,1);		/* Offset to topdict */
    at->lenpos = ftell(cfff);
    putshort(cfff,0);		/* placeholder for final position */
    dumpsid(cfff,at,sf->cidregistry,-1);
    dumpsid(cfff,at,sf->ordering,-1);
    dumpintoper(cfff,sf->supplement,(12<<8)|30);		/* ROS operator must be first */
    dumpdbloper(cfff,sf->cidversion,(12<<8)|31);
    dumpintoper(cfff,cidcnt,(12<<8)|34);
    dumpintoper(cfff, sf->uniqueid?sf->uniqueid:4000000 + (rand()&0x3ffff), (12<<8)|35 );

    dumpsid(cfff,at,sf->copyright,1);
    dumpsid(cfff,at,sf->fullname?sf->fullname:sf->fontname,2);
    dumpsid(cfff,at,sf->familyname,3);
    dumpsid(cfff,at,sf->weight,4);
    /* FontMatrix  (identity here, real ones in sub fonts)*/
    /* Actually there is no fontmatrix in the adobe cid font I'm looking at */
    /*  which means it should default to [.001...] but it doesn't so the */
    /*  docs aren't completely accurate */
    /* I now see I've no idea what the FontMatrix means in a CID keyed font */
    /*  it seems to be ignored everywhere */
#if 0
    dumpdbl(cfff,1.0);
    dumpint(cfff,0);
    dumpint(cfff,0);
    dumpdbl(cfff,1.0);
    dumpint(cfff,0);
    dumpintoper(cfff,0,(12<<8)|7);
#endif

    CIDFindBounds(sf,&b);
    at->gi.xmin = b.minx;
    at->gi.ymin = b.miny;
    at->gi.xmax = b.maxx;
    at->gi.ymax = b.maxy;
    dumpdbl(cfff,floor(b.minx));
    dumpdbl(cfff,floor(b.miny));
    dumpdbl(cfff,ceil(b.maxx));
    dumpdbloper(cfff,ceil(b.maxy),5);
    /* We'll never set StrokeWidth */
    if ( sf->xuid!=NULL ) {
	pt = sf->xuid; if ( *pt=='[' ) ++pt;
	while ( *pt && *pt!=']' ) {
	    dumpint(cfff,strtol(pt,&end,10));
	    for ( pt = end; *pt==' '; ++pt );
	}
	putc(14,cfff);
	if ( sf->changed_since_xuidchanged )
	    SFIncrementXUID(sf);
    }
    dumpint(cfff,0);			/* Docs say a private dict is required and they don't specifically omit CID top dicts */
    dumpintoper(cfff,0,18);		/* But they do say it can be zero */
    /* Offset to charset (oper=15) needed here */
    /* Offset to charstrings (oper=17) needed here */
    /* Offset to FDArray (oper=12,36) needed here */
    /* Offset to FDSelect (oper=12,37) needed here */
}

static void finishup(SplineFont *sf,struct alltabs *at) {
    int strlen, shlen, glen,enclen,csetlen,cstrlen,prvlen;
    int base, eotop, strhead;

    storesid(at,NULL);		/* end the strings index */
    strlen = ftell(at->sidf) + (shlen = ftell(at->sidh));
    glen = sizeof(short);	/* Single entry: 0, no globals */
    enclen = ftell(at->encoding);
    csetlen = ftell(at->charset);
    cstrlen = ftell(at->charstrings);
    prvlen = ftell(at->private);
    base = ftell(at->cfff);
    if ( base+6*3+strlen+glen+enclen+csetlen+cstrlen+prvlen > 32767 ) {
	at->cfflongoffset = true;
	base += 5*5+4;
    } else
	base += 5*3+4;
    strhead = 2+(at->sidcnt>1);
    base += strhead;

    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen,15);
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen,16);
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen+enclen,17);
    dumpsizedint(at->cfff,at->cfflongoffset,at->privatelen,-1);
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen+enclen+cstrlen,18);
    eotop = base-strhead-at->lenpos-1;
    if ( at->cfflongoffset ) {
	fseek(at->cfff,3,SEEK_SET);
	putc(4,at->cfff);
    }
    fseek(at->cfff,at->lenpos,SEEK_SET);
    putshort(at->cfff,eotop);
    fseek(at->cfff,0,SEEK_END);

    /* String Index */
    putshort(at->cfff,at->sidcnt-1);
    if ( at->sidcnt!=1 ) {		/* Everybody gets an added NULL */
	putc(at->sidlongoffset?4:2,at->cfff);
	if ( !ttfcopyfile(at->cfff,at->sidh,base)) at->error = true;
	if ( !ttfcopyfile(at->cfff,at->sidf,base+shlen)) at->error = true;
    }

    /* Global Subrs */
    putshort(at->cfff,0);

    /* Charset */
    if ( !ttfcopyfile(at->cfff,at->charset,base+strlen+glen)) at->error = true;

    /* Encoding */
    if ( !ttfcopyfile(at->cfff,at->encoding,base+strlen+glen+csetlen)) at->error = true;

    /* Char Strings */
    if ( !ttfcopyfile(at->cfff,at->charstrings,base+strlen+glen+csetlen+enclen)) at->error = true;

    /* Private & Subrs */
    if ( !ttfcopyfile(at->cfff,at->private,base+strlen+glen+csetlen+enclen+cstrlen)) at->error = true;
}

static void finishupcid(SplineFont *sf,struct alltabs *at) {
    int strlen, shlen, glen,csetlen,cstrlen,fdsellen,fdarrlen,prvlen;
    int base, eotop, strhead;
    int i;

    storesid(at,NULL);		/* end the strings index */
    strlen = ftell(at->sidf) + (shlen = ftell(at->sidh));
    glen = sizeof(short);	/* Single entry: 0, no globals */
    /* No encodings */
    csetlen = ftell(at->charset);
    fdsellen = ftell(at->fdselect);
    cstrlen = ftell(at->charstrings);
    fdarrlen = ftell(at->fdarray);
    base = ftell(at->cfff);

    at->cfflongoffset = true;
    base += 5*4+4+2;		/* two of the opers below are two byte opers */
    strhead = 2+(at->sidcnt>1);
    base += strhead;

    prvlen = 0;
    for ( i=0; i<sf->subfontcnt; ++i ) {
	fseek(at->fdarray,at->fds[i].fillindictmark,SEEK_SET);
	dumpsizedint(at->fdarray,false,at->fds[i].privatelen,-1);	/* Private len */
	dumpsizedint(at->fdarray,true,base+strlen+glen+csetlen+fdsellen+cstrlen+fdarrlen+prvlen,18);	/* Private offset */
	prvlen += ftell(at->fds[i].private);	/* private & subrs */
    }

    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen,15);	/* charset */
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen,(12<<8)|37);	/* fdselect */
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen+fdsellen,17);	/* charstrings */
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen+fdsellen+cstrlen,(12<<8)|36);	/* fdarray */
    eotop = base-strhead-at->lenpos-1;
    fseek(at->cfff,at->lenpos,SEEK_SET);
    putshort(at->cfff,eotop);
    fseek(at->cfff,0,SEEK_END);

    /* String Index */
    putshort(at->cfff,at->sidcnt-1);
    if ( at->sidcnt!=1 ) {		/* Everybody gets an added NULL */
	putc(at->sidlongoffset?4:2,at->cfff);
	if ( !ttfcopyfile(at->cfff,at->sidh,base)) at->error = true;
	if ( !ttfcopyfile(at->cfff,at->sidf,base+shlen)) at->error = true;
    }

    /* Global Subrs */
    putshort(at->cfff,0);

    /* Charset */
    if ( !ttfcopyfile(at->cfff,at->charset,base+strlen+glen)) at->error = true;

    /* FDSelect */
    if ( !ttfcopyfile(at->cfff,at->fdselect,base+strlen+glen+csetlen)) at->error = true;

    /* Char Strings */
    if ( !ttfcopyfile(at->cfff,at->charstrings,base+strlen+glen+csetlen+fdsellen)) at->error = true;

    /* FDArray (DICT Index) */
    if ( !ttfcopyfile(at->cfff,at->fdarray,base+strlen+glen+csetlen+fdsellen+cstrlen)) at->error = true;

    /* Private & Subrs */
    prvlen = 0;
    for ( i=0; i<sf->subfontcnt; ++i ) {
	int temp = ftell(at->fds[i].private);
	if ( !ttfcopyfile(at->cfff,at->fds[i].private,
		base+strlen+glen+csetlen+fdsellen+cstrlen+fdarrlen+prvlen)) at->error = true;
	prvlen += temp;
    }

    free(at->fds);
}

static int dumpcffhmtx(struct alltabs *at,SplineFont *sf,int bitmaps) {
    DBounds b;
    SplineChar *sc;
    int i,cnt;
    int dovmetrics = sf->hasvmetrics;
    int width = at->gi.fixed_width;

    at->gi.hmtx = tmpfile();
    if ( dovmetrics )
	at->gi.vmtx = tmpfile();
    FigureFullMetricsEnd(sf,&at->gi);
    if ( SCIsNotdef(sf->chars[0],width)) {
	putshort(at->gi.hmtx,sf->chars[0]->width);
	SplineCharFindBounds(sf->chars[0],&b);
	putshort(at->gi.hmtx,b.minx);
	if ( dovmetrics ) {
	    putshort(at->gi.vmtx,sf->chars[0]->vwidth);
	    putshort(at->gi.vmtx,sf->vertical_origin-b.miny);
	}
	i = 1;
    } else {
	i = 0;
	putshort(at->gi.hmtx,width==-1?sf->ascent+sf->descent:width);
	putshort(at->gi.hmtx,0);
	if ( dovmetrics ) {
	    putshort(at->gi.vmtx,sf->ascent+sf->descent);
	    putshort(at->gi.vmtx,0);
	}
    }
    cnt = 1;
    if ( bitmaps ) {
	if ( width==-1 ) width = (sf->ascent+sf->descent)/3;
	putshort(at->gi.hmtx,width);
	putshort(at->gi.hmtx,0);
	if ( dovmetrics ) {
	    putshort(at->gi.vmtx,sf->ascent+sf->descent);
	    putshort(at->gi.vmtx,0);
	}
	putshort(at->gi.hmtx,width);
	putshort(at->gi.hmtx,0);
	if ( dovmetrics ) {
	    putshort(at->gi.vmtx,sf->ascent+sf->descent);
	    putshort(at->gi.vmtx,0);
	}
	cnt = 3;
    }
    for ( i=1; i<sf->charcnt; ++i ) {
	sc = sf->chars[i];
	if ( SCWorthOutputting(sc)) {
	    if ( i<=at->gi.lasthwidth )
		putshort(at->gi.hmtx,sc->width);
	    SplineCharFindBounds(sc,&b);
	    putshort(at->gi.hmtx,b.minx);
	    if ( dovmetrics ) {
		if ( i<=at->gi.lasthwidth )
		    putshort(at->gi.vmtx,sc->vwidth);
		putshort(at->gi.vmtx,sf->vertical_origin-b.maxy);
	    }
	    ++cnt;
	    if ( i==at->gi.lasthwidth )
		at->gi.hfullcnt = cnt;
	    if ( i==at->gi.lastvwidth )
		at->gi.vfullcnt = cnt;
	}
    }
    at->gi.hmtxlen = ftell(at->gi.hmtx);
    if ( at->gi.hmtxlen&2 ) putshort(at->gi.hmtx,0);
    if ( dovmetrics ) {
	at->gi.vmtxlen = ftell(at->gi.vmtx);
	if ( at->gi.vmtxlen&2 ) putshort(at->gi.vmtx,0);
    }

    at->gi.maxp->numGlyphs = cnt;
return( true );
}

static void dumpcffcidhmtx(struct alltabs *at,SplineFont *_sf) {
    DBounds b;
    SplineChar *sc;
    int cid,i,cnt=0,max;
    SplineFont *sf;
    int dovmetrics = _sf->hasvmetrics;

    at->gi.hmtx = tmpfile();
    if ( dovmetrics )
	at->gi.vmtx = tmpfile();
    FigureFullMetricsEnd(_sf,&at->gi);

    max = 0;
    for ( i=0; i<_sf->subfontcnt; ++i )
	if ( max<_sf->subfonts[i]->charcnt )
	    max = _sf->subfonts[i]->charcnt;
    for ( cid = 0; cid<max; ++cid ) {
	for ( i=0; i<_sf->subfontcnt; ++i ) {
	    sf = _sf->subfonts[i];
	    if ( cid<sf->charcnt && SCWorthOutputting(sf->chars[cid]))
	break;
	}
	if ( i!=_sf->subfontcnt ) {
	    sc = sf->chars[cid];
	    if ( cid<=at->gi.lasthwidth )
		putshort(at->gi.hmtx,sc->width);
	    SplineCharFindBounds(sc,&b);
	    putshort(at->gi.hmtx,b.minx);
	    if ( dovmetrics ) {
		if ( cid<=at->gi.lasthwidth )
		    putshort(at->gi.vmtx,sc->vwidth);
		putshort(at->gi.vmtx,sf->vertical_origin-b.maxy);
	    }
	    ++cnt;
	    if ( cid==at->gi.lasthwidth )
		at->gi.hfullcnt = cnt;
	    if ( cid==at->gi.lastvwidth )
		at->gi.vfullcnt = cnt;
	} else if ( cid==0 /*&& i==_sf->subfontcnt*/ ) {
	    /* Create a dummy entry for .notdef */
	    putshort(at->gi.hmtx,sf->ascent+sf->descent);
	    putshort(at->gi.hmtx,0);
	    ++cnt;
	    if ( dovmetrics ) {
		putshort(at->gi.vmtx,sf->ascent+sf->descent);
		putshort(at->gi.vmtx,0);
	    }
	}
    }
    at->gi.hmtxlen = ftell(at->gi.hmtx);
    if ( at->gi.hmtxlen&2 ) putshort(at->gi.hmtx,0);
    if ( dovmetrics ) {
	at->gi.vmtxlen = ftell(at->gi.vmtx);
	if ( at->gi.vmtxlen&2 ) putshort(at->gi.vmtx,0);
    }

    at->gi.maxp->numGlyphs = cnt;
}

static int dumptype2glyphs(SplineFont *sf,struct alltabs *at) {
    int i;
    struct pschars *subrs;

    at->cfff = tmpfile();
    at->sidf = tmpfile();
    at->sidh = tmpfile();
    at->charset = tmpfile();
    at->encoding = tmpfile();
    at->private = tmpfile();

    dumpcffheader(sf,at->cfff);
    dumpcffnames(sf,at->cfff);
    dumpcffcharset(sf,at);
    dumpcffencoding(sf,at);
    GProgressChangeStages(2+at->gi.strikecnt);
    dumpcffprivate(sf,at,-1);
    if ((subrs = SplineFont2Subrs2(sf))==NULL )
return( false );
    _dumpcffstrings(at->private,subrs);
    GProgressNextStage();
    at->charstrings = dumpcffstrings(SplineFont2Chrs2(sf,at->nomwid,at->defwid,subrs));
    PSCharsFree(subrs);
    if ( at->charstrings == NULL )
return( false );
    dumpcfftopdict(sf,at);
    finishup(sf,at);

    at->cfflen = ftell(at->cfff);
    if ( at->cfflen&3 ) {
	for ( i=4-(at->cfflen&3); i>0; --i )
	    putc('\0',at->cfff);
    }

    dumpcffhmtx(at,sf,false);
return( true );
}

static int dumpcidglyphs(SplineFont *sf,struct alltabs *at) {
    int i;

    at->cfff = tmpfile();
    at->sidf = tmpfile();
    at->sidh = tmpfile();
    at->charset = tmpfile();
    at->fdselect = tmpfile();
    at->fdarray = tmpfile();

    at->fds = gcalloc(sf->subfontcnt,sizeof(struct fd2data));
    for ( i=0; i<sf->subfontcnt; ++i ) {
	at->fds[i].private = tmpfile();
	dumpcffprivate(sf->subfonts[i],at,i);
	if ( (at->fds[i].subrs = SplineFont2Subrs2(sf->subfonts[i]))==NULL )
return( false );
	_dumpcffstrings(at->fds[i].private,at->fds[i].subrs);
    }

    dumpcffheader(sf,at->cfff);
    dumpcffnames(sf,at->cfff);
    dumpcffcidset(sf,at);
    dumpcfffdselect(sf,at);
    dumpcffdictindex(sf,at);
    if ( (at->charstrings = dumpcffstrings(CID2Chrs2(sf,at->fds)))==NULL )
return( false );
    for ( i=0; i<sf->subfontcnt; ++i )
	PSCharsFree(at->fds[i].subrs);
    dumpcffcidtopdict(sf,at);
    finishupcid(sf,at);

    at->cfflen = ftell(at->cfff);
    if ( at->cfflen&3 ) {
	for ( i=4-(at->cfflen&3); i>0; --i )
	    putc('\0',at->cfff);
    }

    dumpcffcidhmtx(at,sf);
return( true );
}

static int AnyWidthMDs(SplineFont *sf) {
    int i;
    MinimumDistance *md;

    if ( sf->subfontcnt!=0 ) {
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( AnyWidthMDs(sf->subfonts[i]))
return( true );
    } else {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    for ( md=sf->chars[i]->md; md!=NULL; md = md->next )
		if ( md->sp2==NULL )
return( true );
	}
    }
return( false );
}

static void sethead(struct head *head,SplineFont *_sf) {
    time_t now;
    uint32 now1904[4];
    uint32 year[2];
    int i, lr, rl, j;
    SplineFont *sf = _sf;

    head->version = 0x00010000;
    if ( _sf->subfontcnt!=0 ) {
	int val, mant;
	val = floor(_sf->cidversion);
	mant = floor(65536.*(_sf->cidversion-val));
	head->revision = (val<<16) | mant;
    } else if ( _sf->version!=NULL ) {
	char *pt=_sf->version;
	double dval;
	int val, mant;
	while ( *pt && !isdigit(*pt) && *pt!='.' ) ++pt;
	if ( *pt ) {
	    dval = strtod(pt,NULL);
	    val = floor(dval);
	    mant = floor(65536.*(dval-val));
	    head->revision = (val<<16) | mant;
	}
    }
    head->checksumAdj = 0;
    head->magicNum = 0x5f0f3cf5;
    head->flags = 3;
    if ( AnyWidthMDs(_sf))
	head->flags = 0x13;		/* baseline at 0, lsbline at 0, instructions change metrics */
    head->emunits = sf->ascent+sf->descent;
    head->macstyle = MacStyleCode(sf,NULL);
    head->lowestreadable = 8;
    head->locais32 = 1;
    lr = rl = 0;
    j = 0;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[j];
	for ( i=0; i<sf->charcnt; ++i ) {
	    if ( SCWorthOutputting(sf->chars[i]) ) {
		int uni = sf->chars[i]->unicodeenc ;
		if ( SCRightToLeft(sf->chars[i]) )
		    rl = 1;
		else if (( uni!=-1 && uni<0x10000 && islefttoright(uni)) ||
			 (uni>=0x10300 && uni<0x107ff))
		    lr = 1;
	    }
	}
	++j;
    } while ( j<_sf->subfontcnt );
    sf = _sf;
    /* I assume we've always got some neutrals (spaces, punctuation) */
    if ( lr && rl )
	head->dirhint = 0;
    else if ( rl )
	head->dirhint = -2;
    else
	head->dirhint = 2;
    if ( rl )
	head->flags |= (1<<9);		/* Apple documents this */
    /* if there are any indic characters, set bit 10 */

    time(&now);		/* seconds since 1970, need to convert to seconds since 1904 */
    now1904[0] = now1904[1] = now1904[2] = now1904[3] = 0;
    year[0] = 60*60*24*365;
    year[1] = year[0]>>16; year[0] &= 0xffff;
    for ( i=4; i<70; ++i ) {
	now1904[3] += year[0];
	now1904[2] += year[1];
	if ( (i&3)==0 )
	    now1904[3] += 60*60*24;
	now1904[2] += now1904[3]>>16;
	now1904[3] &= 0xffff;
	now1904[1] += now1904[2]>>16;
	now1904[2] &= 0xffff;
    }
    now1904[3] += now&0xffff;
    now1904[2] += now>>16;
    now1904[2] += now1904[3]>>16;
    now1904[3] &= 0xffff;
    now1904[1] += now1904[2]>>16;
    now1904[2] &= 0xffff;
    head->modtime[1] = head->createtime[1] = (now1904[2]<<16)|now1904[3];
    head->modtime[0] = head->createtime[0] = (now1904[0]<<16)|now1904[1];
}

static void sethhead(struct hhead *hhead,struct hhead *vhead,struct alltabs *at, SplineFont *_sf) {
    int i, width, rbearing, height, bbearing;
    SplineFont *sf=NULL;
    DBounds bb;
    int j;
    /* Might as well fill in the vhead even if we don't use it */
    /*  we just won't dump it out if we don't want it */

    hhead->version = 0x00010000;
    hhead->ascender = _sf->ascent;
    hhead->descender = -_sf->descent;
    hhead->linegap = _sf->pfminfo.linegap;

    vhead->version = 0x00011000;
    vhead->ascender = (_sf->ascent+_sf->descent)/2;
    vhead->descender = -vhead->ascender;
    vhead->linegap = _sf->pfminfo.linegap;

    width = 0x80000000; rbearing = 0x7fffffff; height = 0x80000000; bbearing=0x7fffffff;
    j=0;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[j];
	for ( i=0; i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i])) { /* I don't check for duplicates here. Shouldn't matter, they are duplicates, won't change things */
	    SplineCharFindBounds(sf->chars[i],&bb);
	    if ( sf->chars[i]->width>width ) width = sf->chars[i]->width;
	    if ( sf->chars[i]->vwidth>height ) height = sf->chars[i]->vwidth;
	    if ( sf->chars[i]->width-bb.maxx < rbearing ) rbearing = sf->chars[i]->width-bb.maxx;
	    if ( sf->chars[i]->vwidth-bb.maxy < bbearing ) bbearing = sf->chars[i]->vwidth-bb.maxy;
	}
	++j;
    } while ( j<_sf->subfontcnt );
    at->isfixed = at->gi.fixed_width!=-1;
    hhead->maxwidth = width;
    hhead->minlsb = at->head.xmin;
    hhead->minrsb = rbearing;
    /* Apple's ftxvalidator says the the min sidebearing should be 0 even if it isn't */
    if ( hhead->minlsb>0 ) hhead->minlsb = 0;
    if ( hhead->minrsb>0 ) hhead->minrsb = 0;
    hhead->maxextent = at->head.xmax;
    hhead->caretSlopeRise = 1;

    vhead->maxwidth = height;
    vhead->minlsb = at->head.ymin;
    vhead->minrsb = bbearing;
    vhead->maxextent = at->head.ymax;
    vhead->caretSlopeRise = 1;

    hhead->numMetrics = at->gi.hfullcnt;
    vhead->numMetrics = at->gi.vfullcnt;
}

static void setvorg(struct vorg *vorg, SplineFont *sf) {
    vorg->majorVersion = 1;
    vorg->minorVersion = 0;
    vorg->defaultVertOriginY = sf->vertical_origin;
    vorg->numVertOriginYMetrics = 0;
}

static void OS2WeightCheck(struct pfminfo *pfminfo,char *weight) {
    if ( strstrmatch(weight,"medi")!=NULL ) {
	pfminfo->weight = 500;
	pfminfo->panose[2] = 6;
    } else if ( strstrmatch(weight,"demi")!=NULL ||
		strstrmatch(weight,"halb")!=NULL ||
		(strstrmatch(weight,"semi")!=NULL &&
		    strstrmatch(weight,"bold")!=NULL) ) {
	pfminfo->weight = 600;
	pfminfo->panose[2] = 7;
    } else if ( strstrmatch(weight,"bold")!=NULL ||
		strstrmatch(weight,"fett")!=NULL ||
		strstrmatch(weight,"gras")!=NULL ) {
	pfminfo->weight = 700;
	pfminfo->panose[2] = 8;
    } else if ( strstrmatch(weight,"heavy")!=NULL ) {
	pfminfo->weight = 800;
	pfminfo->panose[2] = 9;
    } else if ( strstrmatch(weight,"black")!=NULL ) {
	pfminfo->weight = 900;
	pfminfo->panose[2] = 10;
    } else if ( strstrmatch(weight,"nord")!=NULL ) {
	pfminfo->weight = 950;
	pfminfo->panose[2] = 11;
    } else if ( strstrmatch(weight,"thin")!=NULL ) {
	pfminfo->weight = 100;
	pfminfo->panose[2] = 2;
    } else if ( strstrmatch(weight,"extra")!=NULL ||
	    strstrmatch(weight,"light")!=NULL ) {
	pfminfo->weight = 200;
	pfminfo->panose[2] = 3;
    } else if ( strstrmatch(weight,"light")!=NULL ) {
	pfminfo->weight = 300;
	pfminfo->panose[2] = 4;
    }
}

void SFDefaultOS2Info(struct pfminfo *pfminfo,SplineFont *_sf,char *fontname) {
    int samewid= -1;
    SplineFont *sf;
    char *weight = _sf->cidmaster==NULL ? _sf->weight : _sf->cidmaster->weight;

    if ( !pfminfo->pfmset ) {
	memset(pfminfo,'\0',sizeof(*pfminfo));
	samewid = CIDOneWidth(_sf);
	sf = _sf;

	pfminfo->pfmfamily = 0x10;
	pfminfo->panose[0] = 2;
	if ( samewid>0 )
	    pfminfo->pfmfamily = 0x30;
	else if ( strstrmatch(fontname,"sans")!=NULL )
	    pfminfo->pfmfamily = 0x20;
	else if ( strstrmatch(fontname,"script")!=NULL ) {
	    pfminfo->pfmfamily = 0x40;
	    pfminfo->panose[0] = 3;
	}
	if ( samewid==-1 )
	    pfminfo->pfmfamily |= 0x1;	/* Else it assumes monospace */

	pfminfo->weight = 400;
	pfminfo->panose[2] = 5;
/* urw uses 4 character abreviations */
	if ( weight!=NULL )
	    OS2WeightCheck(pfminfo,weight);
	OS2WeightCheck(pfminfo,fontname);

	pfminfo->width = 5;
	pfminfo->panose[3] = 3;
	if ( strstrmatch(fontname,"ultra")!=NULL &&
		strstrmatch(fontname,"condensed")!=NULL ) {
	    pfminfo->width = 1;
	    pfminfo->panose[3] = 8;
	} else if ( strstrmatch(fontname,"extra")!=NULL &&
		strstrmatch(fontname,"condensed")!=NULL ) {
	    pfminfo->width = 2;
	    pfminfo->panose[3] = 8;
	} else if ( strstrmatch(fontname,"semi")!=NULL &&
		strstrmatch(fontname,"condensed")!=NULL ) {
	    pfminfo->width = 4;
	    pfminfo->panose[3] = 6;
	} else if ( strstrmatch(fontname,"condensed")!=NULL ||
		strstrmatch(fontname,"narrow")!=NULL ) {
	    pfminfo->width = 3;
	    pfminfo->panose[3] = 6;
	} else if ( strstrmatch(fontname,"ultra")!=NULL &&
		strstrmatch(fontname,"expanded")!=NULL ) {
	    pfminfo->width = 9;
	    pfminfo->panose[3] = 7;
	} else if ( strstrmatch(fontname,"extra")!=NULL &&
		strstrmatch(fontname,"expanded")!=NULL ) {
	    pfminfo->width = 8;
	    pfminfo->panose[3] = 7;
	} else if ( strstrmatch(fontname,"semi")!=NULL &&
		strstrmatch(fontname,"expanded")!=NULL ) {
	    pfminfo->width = 6;
	    pfminfo->panose[3] = 5;
	} else if ( strstrmatch(fontname,"expanded")!=NULL ) {
	    pfminfo->width = 7;
	    pfminfo->panose[3] = 5;
	}
	if ( samewid>0 )
	    pfminfo->panose[3] = 9;
	if ( sf->subfonts!=NULL ) sf = sf->subfonts[0];
	pfminfo->linegap = pfminfo->vlinegap =
		rint(.09*(sf->ascent+sf->descent));
    }
}

void OS2FigureCodePages(SplineFont *sf, uint32 CodePage[2]) {
    SplineFont *_sf;
    int i, k;

    CodePage[0] = CodePage[1] = 0;
    if ( sf->encoding_name==em_iso8859_1 || sf->encoding_name==em_iso8859_15 ||
	    sf->encoding_name==em_win )
	CodePage[0] |= 1<<0;	/* latin1 */
    else if ( sf->encoding_name==em_iso8859_2 )
	CodePage[0] |= 1<<1;	/* latin2 */
    else if ( sf->encoding_name==em_iso8859_9 )
	CodePage[0] |= 1<<4;	/* turkish */
    else if ( sf->encoding_name==em_iso8859_4 )
	CodePage[0] |= 1<<7;	/* baltic */
    else if ( sf->encoding_name==em_iso8859_5 || sf->encoding_name==em_koi8_r )
	CodePage[0] |= 1<<2;	/* cyrillic */
    else if ( sf->encoding_name==em_iso8859_7 )
	CodePage[0] |= 1<<3;	/* greek */
    else if ( sf->encoding_name==em_iso8859_8 )
	CodePage[0] |= 1<<5;	/* hebrew */
    else if ( sf->encoding_name==em_iso8859_6 )
	CodePage[0] |= 1<<6;	/* arabic */
    else if ( sf->encoding_name==em_iso8859_11 )
	CodePage[0] |= 1<<16;	/* thai */
    else if ( sf->encoding_name==em_jis201 || sf->encoding_name==em_jis208 ||
	    sf->encoding_name==em_jis212 || sf->encoding_name==em_sjis )
	CodePage[0] |= 1<<17;	/* japanese */
    else if ( sf->encoding_name==em_gb2312 )
	CodePage[0] |= 1<<18;	/* simplified chinese */
    else if ( sf->encoding_name==em_ksc5601 || sf->encoding_name==em_wansung )
	CodePage[0] |= 1<<19;	/* korean wansung */
    else if ( sf->encoding_name==em_big5 || sf->encoding_name==em_big5hkscs )
	CodePage[0] |= 1<<20;	/* traditional chinese */
    else if ( sf->encoding_name==em_johab )
	CodePage[0] |= 1<<21;	/* korean johab */
    else if ( sf->encoding_name==em_mac )
	CodePage[0] |= 1<<29;	/* mac */
    else if ( sf->encoding_name==em_symbol )
	CodePage[0] |= 1<<31;	/* symbol */

    k=0; _sf = sf;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    if ( sf->chars[i]->unicodeenc==0xde )
		CodePage[0] |= 1<<0;		/* latin1 */
	    else if ( sf->chars[i]->unicodeenc==0x13d )
		CodePage[0] |= 1<<1;		/* latin2 */
	    else if ( sf->chars[i]->unicodeenc==0x411 )
		CodePage[0] |= 1<<2;		/* cyrillic */
	    else if ( sf->chars[i]->unicodeenc==0x386 )
		CodePage[0] |= 1<<3;		/* greek */
	    else if ( sf->chars[i]->unicodeenc==0x130 )
		CodePage[0] |= 1<<4;		/* turkish */
	    else if ( sf->chars[i]->unicodeenc==0x5d0 )
		CodePage[0] |= 1<<5;		/* hebrew */
	    else if ( sf->chars[i]->unicodeenc==0x631 )
		CodePage[0] |= 1<<6;		/* arabic */
	    else if ( sf->chars[i]->unicodeenc==0x157 )
		CodePage[0] |= 1<<7;		/* baltic */
	    else if ( sf->chars[i]->unicodeenc==0xe45 )
		CodePage[0] |= 1<<16;		/* thai */
	    else if ( sf->chars[i]->unicodeenc==0x30a8 )
		CodePage[0] |= 1<<17;		/* japanese */
	    else if ( sf->chars[i]->unicodeenc==0x3105 )
		CodePage[0] |= 1<<18;		/* simplified chinese */
	    else if ( sf->chars[i]->unicodeenc==0x3131 )
		CodePage[0] |= 1<<19;		/* korean wansung */
	    else if ( sf->chars[i]->unicodeenc==0xacf4 )
		CodePage[0] |= 1<<21;		/* korean Johab */
	    else if ( sf->chars[i]->unicodeenc==0x21d4 )
		CodePage[0] |= 1<<31;		/* symbol */
	}
	++k;
    } while ( k<_sf->subfontcnt );

    if ( CodePage[0]==0 )
	CodePage[0] |= 1;
}

static void setos2(struct os2 *os2,struct alltabs *at, SplineFont *_sf,
	enum fontformat format) {
    int i,j,cnt1,cnt2,first,last,avg1,avg2,k;
    SplineFont *sf = _sf;
    char *pt;

    os2->version = 1;
    os2->weightClass = sf->pfminfo.weight;
    os2->widthClass = sf->pfminfo.width;
    os2->fstype = 0xc;
    if ( sf->pfminfo.fstype!=-1 )
	os2->fstype = sf->pfminfo.fstype;
    os2->ysubYSize = os2->ysubXSize = os2->ysubYOff = (sf->ascent+sf->descent)/5;
    os2->ysubXOff = os2->ysupXOff = 0;
    os2->ysupYSize = os2->ysupXSize = os2->ysupYOff = (sf->ascent+sf->descent)/5;
    os2->yStrikeoutSize = 102*(sf->ascent+sf->descent)/2048;
    os2->yStrikeoutPos = 530*(sf->ascent+sf->descent)/2048;
    os2->fsSel = (at->head.macstyle&1?32:0)|(at->head.macstyle&2?1:0);
    if ( sf->fullname!=NULL && strstrmatch(sf->fullname,"outline")!=NULL )
	os2->fsSel |= 8;
    if ( os2->fsSel==0 ) os2->fsSel = 64;		/* Regular */
    os2->ascender = os2->winascent = at->head.ymax;
    os2->descender = at->head.ymin;
    os2->windescent = -at->head.ymin;
    os2->linegap = sf->pfminfo.linegap;

    avg1 = avg2 = last = 0; first = 0x10000;
    cnt1 = cnt2 = 0;
    k = 0;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc!=-1 ) {
	    for ( j=0; j<sizeof(uniranges)/sizeof(uniranges[0]); ++j )
		if ( sf->chars[i]->unicodeenc>=uniranges[j][0] &&
			sf->chars[i]->unicodeenc<=uniranges[j][1] ) {
		    os2->unicoderange[j>>5] |= (1<<(j&31));
	    break;
		}
	    if ( sf->chars[i]->unicodeenc<first ) first = sf->chars[i]->unicodeenc;
	    if ( sf->chars[i]->unicodeenc>last ) last = sf->chars[i]->unicodeenc;
	    avg2 += sf->chars[i]->width; ++cnt2;
	    if ( sf->chars[i]->unicodeenc==' ' ||
		    (sf->chars[i]->unicodeenc>='a' && sf->chars[i]->unicodeenc<='z')) {
		avg1 += sf->chars[i]->width; ++cnt1;
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
    if ( format==ff_ttfsym )	/* MS Symbol font has this set to zero. Does it matter? */
	memset(os2->unicoderange,0,sizeof(os2->unicoderange));
    sf = _sf;

    if ( TTFFoundry!=NULL )
	strncpy(os2->achVendID,TTFFoundry,4);
    else
	memcpy(os2->achVendID,"PfEd",4);
    for ( pt=os2->achVendID; pt<os2->achVendID && *pt!='\0'; ++pt );
    while ( pt<os2->achVendID ) *pt++ = ' ';	/* Pad with spaces not NUL */

    if ( cnt1==27 )
	os2->avgCharWid = avg1/cnt1;
    else if ( cnt2!=0 )
	os2->avgCharWid = avg2/cnt2;
    memcpy(os2->panose,sf->pfminfo.panose,sizeof(os2->panose));
    if ( format==ff_ttfsym ) {
	os2->ulCodePage[0] = 0x80000000;
	os2->ulCodePage[1] = 0;
	first = 255; last = 0;
	if ( sf->chars!=NULL && sf->chars[0]!=NULL && sf->chars[0]->ttf_glyph!=-1 &&
		!SCIsNotdef(sf->chars[0],at->gi.fixed_width) )
	    first = 0;
	for ( i=1; i<sf->charcnt && i<255; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
	    if ( i<first ) first = i;
	    if ( i>last ) last = i;
	}
	os2->firstcharindex = 0xf000 + first;
	os2->lastcharindex  = 0xf000 + last;
    } else {
	os2->firstcharindex = first;
	os2->lastcharindex = last;
	OS2FigureCodePages(sf, os2->ulCodePage);
	if ( os2->ulCodePage[0]==0 )
	    os2->ulCodePage[0] |= 1;
    }

    if ( format==ff_otf || format==ff_otfcid ) {
	BlueData bd;

	QuickBlues(sf,&bd);		/* This handles cid fonts properly */
	os2->version = 2;
	os2->xHeight = bd.xheight;
	os2->capHeight = bd.caph;
	os2->defChar = ' ';
	os2->breakChar = ' ';
	os2->maxContext = 1;	/* Kerning will set this to 2, ligature to whatever */
    }
}

static void redoloca(struct alltabs *at) {
    int i;

    at->loca = tmpfile();
    if ( at->head.locais32 ) {
	for ( i=0; i<=at->maxp.numGlyphs; ++i )
	    putlong(at->loca,at->gi.loca[i]);
	at->localen = sizeof(int32)*(at->maxp.numGlyphs+1);
    } else {
	for ( i=0; i<=at->maxp.numGlyphs; ++i )
	    putshort(at->loca,at->gi.loca[i]/2);
	at->localen = sizeof(int16)*(at->maxp.numGlyphs+1);
	if ( ftell(at->loca)&2 )
	    putshort(at->loca,0);
    }
    free(at->gi.loca);
}

static void redohead(struct alltabs *at) {
    at->headf = tmpfile();

    putlong(at->headf,at->head.version);
    putlong(at->headf,at->head.revision);
    putlong(at->headf,at->head.checksumAdj);
    putlong(at->headf,at->head.magicNum);
    putshort(at->headf,at->head.flags);
    putshort(at->headf,at->head.emunits);
    putlong(at->headf,at->head.createtime[0]);
    putlong(at->headf,at->head.createtime[1]);
    putlong(at->headf,at->head.modtime[0]);
    putlong(at->headf,at->head.modtime[1]);
    putshort(at->headf,at->head.xmin);
    putshort(at->headf,at->head.ymin);
    putshort(at->headf,at->head.xmax);
    putshort(at->headf,at->head.ymax);
    putshort(at->headf,at->head.macstyle);
    putshort(at->headf,at->head.lowestreadable);
    putshort(at->headf,at->head.dirhint);
    putshort(at->headf,at->head.locais32);
    putshort(at->headf,at->head.glyphformat);

    at->headlen = ftell(at->headf);
    if ( (at->headlen&2)!=0 )
	putshort(at->headf,0);
}

static void redohhead(struct alltabs *at,int isv) {
    int i;
    struct hhead *head;
    FILE *f;

    if ( !isv ) {
	f = at->hheadf = tmpfile();
	head = &at->hhead;
    } else {
	f = at->vheadf = tmpfile();
	head = &at->vhead;
    }

    putlong(f,head->version);
    putshort(f,head->ascender);
    putshort(f,head->descender);
    putshort(f,head->linegap);
    putshort(f,head->maxwidth);
    putshort(f,head->minlsb);
    putshort(f,head->minrsb);
    putshort(f,head->maxextent);
    putshort(f,head->caretSlopeRise);
    putshort(f,head->caretSlopeRun);
    for ( i=0; i<5; ++i )
	putshort(f,head->mbz[i]);
    putshort(f,head->metricformat);
    putshort(f,head->numMetrics);

    if ( !isv ) {
	at->hheadlen = ftell(f);
	if ( (at->hheadlen&2)!=0 )
	    putshort(f,0);
    } else {
	at->vheadlen = ftell(f);
	if ( (at->vheadlen&2)!=0 )
	    putshort(f,0);
    }
}

static void redovorg(struct alltabs *at) {

    at->vorgf = tmpfile();
    putshort(at->vorgf,at->vorg.majorVersion);
    putshort(at->vorgf,at->vorg.minorVersion);
    putshort(at->vorgf,at->vorg.defaultVertOriginY);
    putshort(at->vorgf,at->vorg.numVertOriginYMetrics);

    at->vorglen = ftell(at->vorgf);
    if ( (at->vorglen&2)!=0 )
	putshort(at->vorgf,0);
}

static void redomaxp(struct alltabs *at,enum fontformat format) {
    at->maxpf = tmpfile();

    putlong(at->maxpf,at->maxp.version);
    putshort(at->maxpf,at->maxp.numGlyphs);
    if ( format!=ff_otf && format!=ff_otfcid ) {
	putshort(at->maxpf,at->maxp.maxPoints);
	putshort(at->maxpf,at->maxp.maxContours);
	putshort(at->maxpf,at->maxp.maxCompositPts);
	putshort(at->maxpf,at->maxp.maxCompositCtrs);
	putshort(at->maxpf,at->maxp.maxZones);
	putshort(at->maxpf,at->maxp.maxTwilightPts);
	putshort(at->maxpf,at->maxp.maxStorage);
	putshort(at->maxpf,at->maxp.maxFDEFs);
	putshort(at->maxpf,at->maxp.maxIDEFs);
	putshort(at->maxpf,at->maxp.maxStack);
	putshort(at->maxpf,at->maxp.maxglyphInstr);
	putshort(at->maxpf,at->maxp.maxnumcomponents);
	putshort(at->maxpf,at->maxp.maxcomponentdepth);
    }

    at->maxplen = ftell(at->maxpf);
    if ( (at->maxplen&2)!=0 )
	putshort(at->maxpf,0);
}

static void redoos2(struct alltabs *at) {
    int i;
    at->os2f = tmpfile();

    putshort(at->os2f,at->os2.version);
    putshort(at->os2f,at->os2.avgCharWid);
    putshort(at->os2f,at->os2.weightClass);
    putshort(at->os2f,at->os2.widthClass);
    putshort(at->os2f,at->os2.fstype);
    putshort(at->os2f,at->os2.ysubXSize);
    putshort(at->os2f,at->os2.ysubYSize);
    putshort(at->os2f,at->os2.ysubXOff);
    putshort(at->os2f,at->os2.ysubYOff);
    putshort(at->os2f,at->os2.ysupXSize);
    putshort(at->os2f,at->os2.ysupYSize);
    putshort(at->os2f,at->os2.ysupXOff);
    putshort(at->os2f,at->os2.ysupYOff);
    putshort(at->os2f,at->os2.yStrikeoutSize);
    putshort(at->os2f,at->os2.yStrikeoutPos);
    putshort(at->os2f,at->os2.sFamilyClass);
    for ( i=0; i<10; ++i )
	putc(at->os2.panose[i],at->os2f);
    for ( i=0; i<4; ++i )
	putlong(at->os2f,at->os2.unicoderange[i]);
    for ( i=0; i<4; ++i )
	putc(at->os2.achVendID[i],at->os2f);
    putshort(at->os2f,at->os2.fsSel);
    putshort(at->os2f,at->os2.firstcharindex);
    putshort(at->os2f,at->os2.lastcharindex);
    putshort(at->os2f,at->os2.ascender);
    putshort(at->os2f,at->os2.descender);
    putshort(at->os2f,at->os2.linegap);
    putshort(at->os2f,at->os2.winascent);
    putshort(at->os2f,at->os2.windescent);
    putlong(at->os2f,at->os2.ulCodePage[0]);
    putlong(at->os2f,at->os2.ulCodePage[1]);

    if ( at->os2.version>=2 ) {
	putshort(at->os2f,at->os2.xHeight);
	putshort(at->os2f,at->os2.capHeight);
	putshort(at->os2f,at->os2.defChar);
	putshort(at->os2f,at->os2.breakChar);
	putshort(at->os2f,at->os2.maxContext);
    }

    at->os2len = ftell(at->os2f);
    if ( (at->os2len&2)!=0 )
	putshort(at->os2f,0);
}

static void dumpgasp(struct alltabs *at) {

    at->gaspf = tmpfile();
    putshort(at->gaspf,0);	/* Version number */
    putshort(at->gaspf,1);	/* One range */
    putshort(at->gaspf,0xffff);	/* Upper bound on pixels/em for this range */
    putshort(at->gaspf,0x2);	/* Grey scale, no gridfitting */
				    /* No hints, so no grids to fit */
    at->gasplen = ftell(at->gaspf);
	/* This table is always 32 bit aligned */
}

static void dumpmacstr(FILE *file,unichar_t *str) {
    int ch;
    unsigned char *table;

    do {
	ch = *str++;
	if ( ch==0 )
	    putc('\0',file);
	else if ( (ch>>8)>=mac_from_unicode.first && (ch>>8)<=mac_from_unicode.last &&
		(table = mac_from_unicode.table[(ch>>8)-mac_from_unicode.first])!=NULL &&
		table[ch&0xff]!=0 )
	    putc(table[ch&0xff],file);
	else
	    putc('?',file);	/* if we were to omit an unencoded char all our position calculations would be off */
    } while ( ch!='\0' );
}

static void dumpstr(FILE *file,char *str) {
    do {
	putc(*str,file);
    } while ( *str++!='\0' );
}

static void dumpc2ustr(FILE *file,char *str) {
    do {
	putc('\0',file);
	putc(*str,file);
    } while ( *str++!='\0' );
}

static void dumpustr(FILE *file,unichar_t *str) {
    do {
	putc(*str>>8,file);
	putc(*str&0xff,file);
    } while ( *str++!='\0' );
}

static void dumppstr(FILE *file,char *str) {
    putc(strlen(str),file);
    fwrite(str,sizeof(char),strlen(str),file);
}

#if 0
/* Languages on the mac presumably imply an encoding, but that encoding is not*/
/*  listed in the language table in the name table docs. I think it is safe to*/
/*  guess that these first 10 languages all use the MacRoman encoding that */
/*  is designed for western europe. I could handle that... */
/* The complexities of locale (british vs american english) don't seem to be */
/*  present on the mac */
/* I don't think I'll make use of this table for the mac though... */
static struct { int mslang, maclang, enc, used; } mactrans[] = {
    { 0x09, 0, em_mac },		/* English */
    { 0x0c, 1, em_mac },		/* French */
    { 0x07, 2, em_mac },		/* German */
    { 0x10, 3, em_mac },		/* Italian */
    { 0x13, 4, em_mac },		/* Dutch */
    { 0x1d, 5, em_mac },		/* Swedish */
    { 0x0a, 6, em_mac },		/* Spanish */
    { 0x06, 7, em_mac },		/* Danish */
    { 0x16, 8, em_mac },		/* Portuguese */
    { 0x14, 9, em_mac },		/* Norwegian */
    { 0 }};
#endif

/* Oh. If the encoding is symbol (platform=3, specific=0) then Windows won't */
/*  accept the font unless the name table also has entries for (3,0). I'm not */
/*  sure if this is the case for the CJK encodings (docs don't mention that) */
/*  but let's do it just in case */
void DefaultTTFEnglishNames(struct ttflangname *dummy, SplineFont *sf) {
    time_t now;
    struct tm *tm;
    char buffer[200];

    if ( dummy->names[ttf_copyright]==NULL || *dummy->names[ttf_copyright]=='\0' )
	dummy->names[ttf_copyright] = uc_copy(sf->copyright);
    if ( dummy->names[ttf_family]==NULL || *dummy->names[ttf_family]=='\0' )
	dummy->names[ttf_family] = uc_copy(sf->familyname);
    if ( dummy->names[ttf_subfamily]==NULL || *dummy->names[ttf_subfamily]=='\0' )
	dummy->names[ttf_subfamily] = uc_copy(SFGetModifiers(sf));
    if ( dummy->names[ttf_uniqueid]==NULL || *dummy->names[ttf_uniqueid]=='\0' ) {
	time(&now);
	tm = localtime(&now);
	sprintf( buffer, "%s : %s : %d-%d-%d",
		BDFFoundry?BDFFoundry:TTFFoundry?TTFFoundry:"PfaEdit 1.0",
		sf->fullname!=NULL?sf->fullname:sf->fontname,
		tm->tm_mday, tm->tm_mon, tm->tm_year+1900 );
	dummy->names[ttf_uniqueid] = uc_copy(buffer);
    }
    if ( dummy->names[ttf_fullname]==NULL || *dummy->names[ttf_fullname]=='\0' )
	dummy->names[ttf_fullname] = uc_copy(sf->fullname);
    if ( dummy->names[ttf_version]==NULL || *dummy->names[ttf_version]=='\0' ) {
	if ( sf->subfontcnt!=0 )
	    sprintf( buffer, "Version %f ", sf->cidversion );
	else if ( sf->version!=NULL )
	    sprintf(buffer,"Version %.20s ", sf->version);
	else
	    strcpy(buffer,"Version 1.0" );
	dummy->names[ttf_version] = uc_copy(buffer);
    }
    if ( dummy->names[ttf_postscriptname]==NULL || *dummy->names[ttf_postscriptname]=='\0' )
	dummy->names[ttf_postscriptname] = uc_copy(sf->fontname);
}

/* There's an inconsistancy here. Apple's docs say there most be only one */
/*  nameid==6 and that name must be ascii (presumably plat=1, spec=0, lang=0) */
/* The opentype docs say there must be two (psl=1,0,0 & psl=3,1,0x409) any */
/*  others are to be ignored */
static void dumpnames(struct alltabs *at, SplineFont *sf,enum fontformat format) {
    int pos=0,i,j;
    struct ttflangname dummy, *cur, *useng;
    int strcnt=0, cnt=0;
    int posses[ttf_namemax];
    int maxlen, len, enc, specific;
    char *space;
    int extra=0;

    if ( sf->encoding_name==em_ksc5601 || sf->encoding_name==em_wansung ||
	    sf->encoding_name==em_jis208 || sf->encoding_name==em_sjis ||
	    sf->encoding_name==em_big5 || sf->encoding_name==em_big5hkscs ||
	    sf->encoding_name==em_johab ||
	    format == ff_ttfsym )
	extra = 1;
    memset(&dummy,'\0',sizeof(dummy));
    useng = NULL;
    for ( cur=sf->names; cur!=NULL; cur=cur->next ) {
	if ( cur->lang!=0x409 ) {
	    for ( i=0; i<ttf_namemax; ++i )
		if ( cur->names[i]!=NULL ) strcnt += extra+1;
	} else {
	    dummy = *cur;
	    useng = cur;
	}
    }
    DefaultTTFEnglishNames(&dummy, sf);
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) {
	if ( i==6 && at->applemode ) ++strcnt;	/* Apple insists there only be 1 */
	else strcnt+=3+extra;
    }
    /* once of mac roman encoding, once for mac unicode and once for windows unicode 409 */

    /* The examples I've seen of the feature table only contain mac roman english */
    /*  but I'm including apple unicode too */
    if ( at->feat_name!=NULL ) {
	for ( i=0; at->feat_name[i].strid!=0; ++i )
	    strcnt += 2;
    }

    at->name = tmpfile();
    putshort(at->name,0);	/* format */
    putshort(at->name,strcnt);	/* numrec */
    putshort(at->name,(3+strcnt*6)*sizeof(int16));	/* offset to strings */
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) {
	if (i==6 && at->applemode )
	    /* No output, but increment the string table point none the less */
	    pos += 2*u_strlen(dummy.names[i])+2;
	else {
	    putshort(at->name,0);	/* apple unicode */
	    putshort(at->name,3);	/* 3 => Unicode 2.0 semantics */ /* 0 ("default") is also a reasonable value */
	    putshort(at->name,0);	/*  */
	    putshort(at->name,i);
	    putshort(at->name,2*u_strlen(dummy.names[i]));
	    putshort(at->name,pos);
	    posses[i] = pos;
	    pos += 2*u_strlen(dummy.names[i])+2;
	    ++cnt;
	}
    }
    if ( at->feat_name!=NULL ) {
	for ( i=0; at->feat_name[i].strid!=0; ++i ) {
	    putshort(at->name,0);	/* apple unicode */
	    putshort(at->name,3);	/* Unicode 2.0 */
	    putshort(at->name,0);	/*  */
	    putshort(at->name,at->feat_name[i].strid);
	    putshort(at->name,2*strlen(at->feat_name[i].name));
	    putshort(at->name,pos);
	    pos += 2*strlen(at->feat_name[i].name)+2;
	    ++cnt;
	}
    }
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) {
	/* here is the one place we always have postscript, nameid==6 */
	putshort(at->name,1);	/* apple */
	putshort(at->name,0);	/*  */
	putshort(at->name,0);	/* Roman alphabet */
	putshort(at->name,i);
	putshort(at->name,u_strlen(dummy.names[i]));
	putshort(at->name,pos);
	pos += u_strlen(dummy.names[i])+1;
	++cnt;
    }
    if ( at->feat_name!=NULL ) {
	for ( i=0; at->feat_name[i].strid!=0; ++i ) {
	    putshort(at->name,1);	/* apple */
	    putshort(at->name,0);	/*  */
	    putshort(at->name,0);	/* Roman alphabet */
	    putshort(at->name,at->feat_name[i].strid);
	    putshort(at->name,strlen(at->feat_name[i].name));
	    putshort(at->name,pos);
	    pos += strlen(at->feat_name[i].name)+1;
	    ++cnt;
	}
    }
    if ( format==ff_ttfsym ) {
	for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL && (i!=6 || !at->applemode) ) {
	    putshort(at->name,3);	/* MS platform */
	    putshort(at->name,0);
	    putshort(at->name,0x0409);	/* american english language */
	    putshort(at->name,i);
	    putshort(at->name,2*u_strlen(dummy.names[i]));
	    putshort(at->name,posses[i]);
	++cnt;
	}

	for ( cur=sf->names; cur!=NULL; cur=cur->next ) if ( cur->lang!=0x409 ) {
	    for ( i=0; i<ttf_namemax; ++i ) if ( cur->names[i]!=NULL ) {
		putshort(at->name,3);	/* MS platform */
		putshort(at->name,0);	/* not symbol */
		putshort(at->name,cur->lang);/* american english language */
		putshort(at->name,i);
		putshort(at->name,2*u_strlen(cur->names[i]));
		putshort(at->name,pos);
		pos += 2*u_strlen(cur->names[i])+2;
	++cnt;
	    }
	}
    }

    maxlen = 0;
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL && (i!=6 || !at->applemode) ) {
	putshort(at->name,3);	/* MS platform */
	putshort(at->name,1);
	putshort(at->name,0x0409);	/* american english language */
	putshort(at->name,i);
	len = u_strlen(dummy.names[i]);
	if ( len>maxlen ) maxlen = len;
	putshort(at->name,2*len);
	putshort(at->name,posses[i]);
	++cnt;
    }
    for ( cur=sf->names; cur!=NULL; cur=cur->next ) if ( cur->lang!=0x409 ) {
	for ( i=0; i<ttf_namemax; ++i ) if ( cur->names[i]!=NULL ) {
	    putshort(at->name,3);	/* MS platform */
	    putshort(at->name,1);	/* unicode */
	    putshort(at->name,cur->lang);
	    putshort(at->name,i);
	    len = u_strlen(cur->names[i]);
	    if ( len>maxlen ) maxlen = len;
	    putshort(at->name,2*len);
	    putshort(at->name,pos);
	    pos += 2*u_strlen(cur->names[i])+2;
	++cnt;
	}
    }

    enc = em_unicode; space = NULL;
    if ( sf->encoding_name==em_ksc5601 || sf->encoding_name==em_wansung ||
	    sf->encoding_name==em_jis208 || sf->encoding_name==em_sjis ||
	    sf->encoding_name==em_big5 || sf->encoding_name==em_big5hkscs ||
	    sf->encoding_name==em_johab ) {
	specific =  sf->encoding_name==em_ksc5601 ? 5 :	/* Wansung, korean */
		    sf->encoding_name==em_wansung ? 5 :	/* Wansung, korean */
		    sf->encoding_name==em_jis208 ? 2 :	/* SJIS */
		    sf->encoding_name==em_sjis ? 2 :	/* SJIS */
		    sf->encoding_name==em_big5 ? 4 :	/* Big5, traditional Chinese */
		    sf->encoding_name==em_big5hkscs ? 4 :
		    /* sf->encoding_name==em_johab*/ 6;	/* Korean */
	enc =	    sf->encoding_name==em_ksc5601 ? e_wansung :	/* Wansung, korean */
		    sf->encoding_name==em_wansung ? e_wansung :	/* Wansung, korean */
		    sf->encoding_name==em_jis208 ? e_sjis :	/* SJIS */
		    sf->encoding_name==em_sjis ? e_sjis :	/* SJIS */
		    sf->encoding_name==em_big5 ? e_big5 :	/* Big5, traditional Chinese */
		    sf->encoding_name==em_big5hkscs ? e_big5hkscs :
		    /* sf->encoding_name==em_johab*/ e_johab;	/* Korean */
	maxlen = 3*maxlen+10;
	space = galloc(maxlen);
	for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL && (i!=6 || !at->applemode) ) {
	    putshort(at->name,3);	/* MS platform */
	    putshort(at->name,specific);
	    putshort(at->name,0x0409);	/* american english language */
	    putshort(at->name,i);
	    putshort(at->name,strlen(u2encoding_strncpy(space,dummy.names[i],maxlen,enc)));
	    putshort(at->name,pos);
	    pos += strlen(space)+1;
	++cnt;
	}
	for ( cur=sf->names; cur!=NULL; cur=cur->next ) if ( cur->lang!=0x409 ) {
	    for ( i=0; i<ttf_namemax; ++i ) if ( cur->names[i]!=NULL ) {
		putshort(at->name,3);	/* MS platform */
		putshort(at->name,specific);
		putshort(at->name,cur->lang);
		putshort(at->name,i);
		putshort(at->name,strlen(u2encoding_strncpy(space,cur->names[i],maxlen,enc)));
		putshort(at->name,pos);
		pos += strlen(space)+1;
	++cnt;
	    }
	}
    }
    if ( cnt!=strcnt )
	fprintf(stderr, "String count wrong in 'name' table, is %d should be %d\n", cnt, strcnt );
    if ( ftell(at->name)!=(3+strcnt*6)*sizeof(int16) )
	fprintf(stderr, "Table offset wrong in 'name' table, is %d should be %d\n", ftell(at->name), (3+strcnt*6)*sizeof(short) );

    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL )
	dumpustr(at->name,dummy.names[i]);
    if ( at->feat_name!=NULL ) {
	for ( i=0; at->feat_name[i].strid!=0; ++i )
	    dumpc2ustr(at->name,at->feat_name[i].name);
    }
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL )
	dumpmacstr(at->name,dummy.names[i]);
    if ( at->feat_name!=NULL ) {
	for ( i=0; at->feat_name[i].strid!=0; ++i )
	    dumpstr(at->name,at->feat_name[i].name);
    }
    if ( format==ff_ttfsym ) {
	for ( cur=sf->names; cur!=NULL; cur=cur->next ) if ( cur->lang!=0x409 )
	    for ( i=0; i<ttf_namemax; ++i ) if ( cur->names[i]!=NULL )
		dumpustr(at->name,cur->names[i]);
    }
    for ( cur=sf->names; cur!=NULL; cur=cur->next ) if ( cur->lang!=0x409 )
	for ( i=0; i<ttf_namemax; ++i ) if ( cur->names[i]!=NULL )
	    dumpustr(at->name,cur->names[i]);
    if ( enc!=em_unicode ) {
	for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL )
	    dumpstr(at->name,u2encoding_strncpy(space,dummy.names[i],maxlen,enc));
	for ( cur=sf->names; cur!=NULL; cur=cur->next ) if ( cur->lang!=0x409 )
	    for ( i=0; i<ttf_namemax; ++i ) if ( cur->names[i]!=NULL )
		dumpstr(at->name,u2encoding_strncpy(space,cur->names[i],maxlen,enc));
	free(space);
    }
    at->namelen = ftell(at->name);
    if ( (at->namelen&3)!=0 )
	for ( j= 4-(at->namelen&3); j>0; --j )
	    putc('\0',at->name);
    for ( i=0; i<ttf_namemax; ++i )
	if ( useng==NULL || dummy.names[i]!=useng->names[i] )
	    free( dummy.names[i]);
    free( at->feat_name );
}

static void dumppost(struct alltabs *at, SplineFont *sf, enum fontformat format) {
    int pos, i,j;
    int shorttable = (format==ff_otf || format==ff_otfcid ||
	    (at->gi.flags&ttf_flag_shortps));

    at->post = tmpfile();

    putlong(at->post,shorttable?0x00030000:0x00020000);	/* formattype */
    putfixed(at->post,sf->italicangle);
    putshort(at->post,sf->upos);
    putshort(at->post,sf->uwidth);
    putlong(at->post,at->isfixed);
    putlong(at->post,0);		/* no idea about memory */
    putlong(at->post,0);		/* no idea about memory */
    putlong(at->post,0);		/* no idea about memory */
    putlong(at->post,0);		/* no idea about memory */
    if ( !shorttable ) {
	putshort(at->post,at->maxp.numGlyphs);

	putshort(at->post,0);		/* glyph 0 is named .notdef */
	putshort(at->post,1);		/* glyphs 1&2 are tab and cr */
	putshort(at->post,2);		/* or something */
	i=1;
	if ( sf->chars[0]!=NULL && strcmp(sf->chars[0]->name,".notdef")!=0 )
	    i=0;
	for ( pos=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
	    if ( strcmp(sf->chars[i]->name,".notdef")==0 )
		putshort(at->post,0);
	    else {
		for ( j=127-32+3; j<258; ++j )
		    if ( strcmp(sf->chars[i]->name,ttfstandardnames[j])==0 )
		break;
		if ( j!=258 )
		    putshort(at->post,j);
		else {
		    putshort(at->post,pos+258);
		    ++pos;
		}
	    }
	}
	if ( pos!=0 ) {
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
		if ( strcmp(sf->chars[i]->name,".notdef")==0 )
		    /* Do Nothing */;
		else {
		    for ( j=127-32+3; j<258; ++j )
			if ( strcmp(sf->chars[i]->name,ttfstandardnames[j])==0 )
		    break;
		    if ( j!=258 )
			/* Do Nothing */;
		    else
			dumppstr(at->post,sf->chars[i]->name);
		}
	    }
	}
    }
    at->postlen = ftell(at->post);
    if ( (at->postlen&3)!=0 )
	for ( j= 4-(at->postlen&3); j>0; --j )
	    putc('\0',at->post);
}

static void SetTo(uint32 *avail,uint16 *sfind,int to,int from) {
    avail[to] = avail[from];
    if ( sfind!=NULL ) sfind[to] = sfind[from];
}

static FILE *Needs816Enc(SplineFont *sf,int *tlen) {
    int i, j, complained, pos, k, subheadindex, jj;
    uint16 table[256];
    struct subhead subheads[128];
    uint16 *glyphs;
    uint16 tempglyphs[256];
    int base, lbase, basebound, subheadcnt, planesize, plane0size;
    int base2, base2bound;
    FILE *sub;

    *tlen = 0;
    if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 )
return( NULL );
    if ( sf->encoding_name!=em_big5 && sf->encoding_name!=em_big5hkscs &&
		    sf->encoding_name!=em_johab &&
		    sf->encoding_name!=em_sjis && sf->encoding_name!=em_wansung )
return( NULL );

    base2 = -1; base2bound = -2;
    if ( sf->encoding_name==em_big5 ) {
	base = 0xa1;
	basebound = 0xf9;	/* wcl-02.ttf's cmap claims to go up to fc, but everything after f9 is invalid (according to what I know of big5, f9 should be the end) */
	subheadcnt = basebound-base+1;
	lbase = 0x40;
	planesize = 191;
    } else if ( sf->encoding_name==em_big5hkscs ) {
	base = 0x81;
	basebound = 0xfe;
	subheadcnt = basebound-base+1;
	lbase = 0x40;
	planesize = 191;
    } else if ( sf->encoding_name==em_wansung ) {
	base = 0xa1;
	basebound = 0xfd;
	lbase = 0xa1;
	subheadcnt = basebound-base+1;
	planesize = 0xfe - lbase +1;
    } else if ( sf->encoding_name==em_johab ) {
	base = 0x84;
	basebound = 0xf9;
	lbase = 0x31;
	subheadcnt = basebound-base+1;
	planesize = 0xfe -0x31+1;	/* Stupid gcc bug, thinks 0xfe- is ambiguous (exponant) */
    } else if ( sf->encoding_name==em_sjis ) {
	base = 129;
	basebound = 159;
	lbase = 64;
	planesize = 252 - lbase +1;
	base2 = 0xe0;
	/* SJIS supports "user defined characters" between 0xf040 and 0xfcfc */
	/*  there probably won't be any, but allow space for them if there are*/
	for ( base2bound=0xfc00; base2bound>0xefff; --base2bound )
	    if ( base2bound<sf->charcnt && SCWorthOutputting(sf->chars[base2bound]))
	break;
	base2bound >>= 8;
	subheadcnt = basebound-base + 1 + base2bound-base2 + 1;
    } else {
	fprintf( stderr, "Unsupported 8/16 encoding %d\n", sf->encoding_name );
return( NULL );
    }
    plane0size = base2==-1? base : base2;
    i=0;
    if ( base2!=-1 ) {
	for ( i=basebound; i<base2 && i<sf->charcnt; ++i )
	    if ( sf->chars[i]!=NULL && (i==0 || (i==13 && sf->chars[i]->ttf_glyph==2)))
	continue;	/* Ignore .notdef, nonmarking return */
	    else if ( SCWorthOutputting(sf->chars[i]))
	break;
	if ( i==base2 || i==sf->charcnt )
	    i = 0;
    }
    if ( i==0 ) {
	for ( i=0; i<base && i<sf->charcnt; ++i )
	    if ( sf->chars[i]!=NULL && (i==0 || (i==13 && sf->chars[i]->ttf_glyph==2)))
	continue;
	    else if ( SCWorthOutputting(sf->chars[i]))
	break;
    }
#if 0
    if ( i==base || i==sf->charcnt )
return( NULL );		/* Doesn't have the single byte entries */
	/* Can use the normal 16 bit encoding scheme */
	/* Always do an 8/16, and we'll do a unicode table too now... */
#endif

    if ( base2!=-1 ) {
	for ( i=base; i<=basebound && i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i])) {
		GWidgetErrorR(_STR_BadEncoding,_STR_ExtraneousSingleByte,i);
	break;
	    }
	if ( i==basebound+1 )
	    for ( i=base2; i<256 && i<sf->charcnt; ++i )
		if ( SCWorthOutputting(sf->chars[i])) {
		    GWidgetErrorR(_STR_BadEncoding,_STR_ExtraneousSingleByte,i);
	    break;
		}
    } else {
	for ( i=base; i<256 && i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i])) {
		GWidgetErrorR(_STR_BadEncoding,_STR_ExtraneousSingleByte,i);
	break;
	    }
    }
    for ( i=256; i<(base<<8) && i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i])) {
	    GWidgetErrorR(_STR_BadEncoding,_STR_OutOfEncoding,i);
    break;
	}
    if ( i==(base<<8) && base2==-1 )
	for ( i=((basebound+1)<<8); i<0x10000 && i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i])) {
		GWidgetErrorR(_STR_BadEncoding,_STR_OutOfEncoding,i);
	break;
	    }

    memset(table,'\0',sizeof(table));
    for ( i=base; i<=basebound; ++i )
	table[i] = 8*(i-base+1);
    for ( i=base2; i<=base2bound; ++i )
	table[i] = 8*(i-base2+basebound-base+1+1);
    memset(subheads,'\0',sizeof(subheads));
    subheads[0].first = 0; subheads[0].cnt = plane0size;
    for ( i=1; i<subheadcnt+1; ++i ) {
	subheads[i].first = lbase;
	subheads[i].cnt = planesize;
    }
    glyphs = gcalloc(subheadcnt*planesize+plane0size,sizeof(uint16));
    subheads[0].rangeoff = 0;
    for ( i=0; i<plane0size && i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL && SCDuplicate(sf->chars[i])->ttf_glyph!=-1)
	    glyphs[i] = SCDuplicate(sf->chars[i])->ttf_glyph;
    
    pos = 1;

    complained = false;
    subheadindex = 1;
    for ( jj=0; jj<2 || (base2==-1 && jj<1); ++jj )
	    for ( j=((jj==0?base:base2)<<8); j<=((jj==0?basebound:base2bound)<<8); j+= 0x100 ) {
	for ( i=0; i<lbase; ++i )
	    if ( !complained && SCWorthOutputting(sf->chars[i+j])) {
		GWidgetErrorR(_STR_BadEncoding,_STR_NotNormallyEncoded,i+j);
		complained = true;
	    }
	if ( sf->encoding_name==em_big5 ) {
	    /* big5 has a gap here. Does johab? */
	    for ( i=0x7f; i<0xa1; ++i )
		if ( !complained && SCWorthOutputting(sf->chars[i+j])) {
		    GWidgetErrorR(_STR_BadEncoding,_STR_NotNormallyEncoded,i+j);
		    complained = true;
		}
	}
	memset(tempglyphs,0,sizeof(tempglyphs));
	for ( i=0; i<planesize; ++i )
	    if ( sf->chars[j+lbase+i]!=NULL && SCDuplicate(sf->chars[j+lbase+i])->ttf_glyph!=-1 )
		tempglyphs[i] = SCDuplicate(sf->chars[j+lbase+i])->ttf_glyph;
	for ( i=1; i<pos; ++i ) {
	    int delta = 0;
	    for ( k=0; k<planesize; ++k )
		if ( tempglyphs[k]==0 && glyphs[plane0size+(i-1)*planesize+k]==0 )
		    /* Still matches */;
		else if ( tempglyphs[k]==0 || glyphs[plane0size+(i-1)*planesize+k]==0 )
	    break;  /* Doesn't match */
		else if ( delta==0 )
		    delta = (uint16) (tempglyphs[k]-glyphs[plane0size+(i-1)*planesize+k]);
		else if ( tempglyphs[k]==(uint16) (glyphs[plane0size+(i-1)*planesize+k]+delta) )
		    /* Still matches */;
		else
	    break;
	    if ( k==planesize ) {
		subheads[subheadindex].delta = delta;
		subheads[subheadindex].rangeoff = plane0size+(i-1)*planesize;
	break;
	    }
	}
	if ( subheads[subheadindex].rangeoff==0 ) {
	    memcpy(glyphs+(pos-1)*planesize+plane0size,tempglyphs,planesize*sizeof(uint16));
	    subheads[subheadindex].rangeoff = plane0size+(pos++-1)*planesize ;
	}
	++subheadindex;
    }

    /* fixup offsets */
    /* my rangeoffsets are indexes into the glyph array. That's nice and */
    /*  simple. Unfortunately ttf says they are offsets from the current */
    /*  location in the file (sort of) so we now fix them up. */
    for ( i=0; i<subheadcnt+1; ++i )
	subheads[i].rangeoff = subheads[i].rangeoff*sizeof(uint16) +
		(subheadcnt-i)*sizeof(struct subhead) + sizeof(uint16);

    sub = tmpfile();
    if ( sub==NULL )
return( NULL );

    putshort(sub,2);		/* 8/16 format */
    putshort(sub,0);		/* Subtable length, we'll come back and fix this */
    putshort(sub,0);		/* version/language, not meaningful in ms systems */
    for ( i=0; i<256; ++i )
	putshort(sub,table[i]);
    for ( i=0; i<subheadcnt+1; ++i ) {
	putshort(sub,subheads[i].first);
	putshort(sub,subheads[i].cnt);
	putshort(sub,subheads[i].delta);
	putshort(sub,subheads[i].rangeoff);
    }
    for ( i=0; i<(pos-1)*planesize+plane0size; ++i )
	putshort(sub,glyphs[i]);
    free(glyphs);

    *tlen = ftell(sub);
    fseek(sub,2,SEEK_SET);
    putshort(sub,*tlen);	/* Length, I said we'd come back to it */
    rewind( sub );
return( sub );
}

static FILE *NeedsUCS4Table(SplineFont *sf,int *ucs4len) {
    int i=0,j,group;
    FILE *format12;
    SplineChar *sc;
    
    if ( sf->encoding_name==em_unicode4 ) {
	for ( i=0x10000; i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]))
	break;
    } else if ( sf->encoding_name>=em_unicodeplanes && sf->encoding_name<=em_unicodeplanesmax ) {
	for ( i=0; i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]))
	break;
    } else
return( NULL );
	if ( i>=sf->charcnt )
return(NULL);

    format12 = tmpfile();
    if ( format12==NULL )
return( NULL );

    putshort(format12,12);		/* Subtable format */
    putshort(format12,0);		/* padding */
    putlong(format12,0);		/* Length, we'll come back to this */
    putlong(format12,0);		/* language */
    putlong(format12,0);		/* Number of groups, we'll come back to this */

    group = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]->unicodeenc!=-1 ) {
	sc = SCDuplicate(sf->chars[i]);
	for ( j=i+1; j<sf->charcnt && SCWorthOutputting(sf->chars[j]) &&
		sf->chars[j]->unicodeenc!=-1 &&
		SCDuplicate(sf->chars[j])->ttf_glyph==sc->ttf_glyph+j-i; ++j );
	--j;
	putlong(format12,i);		/* start char code */
	putlong(format12,j);		/* end char code */
	putlong(format12,sc->ttf_glyph);
	++group;
    }
    *ucs4len = ftell(format12);
    fseek(format12,4,SEEK_SET);
    putlong(format12,*ucs4len);		/* Length, I said we'd come back to it */
    putlong(format12,0);		/* language */
    putlong(format12,group);		/* Number of groups */
    rewind( format12 );
return( format12 );
}

static int figureencoding(SplineFont *sf,int i) {
#if 1
    /* We used to be able to generate a format 4 table for various cjk encodings */
    /*  (if they didn't have any single byte characters). Now we always generate */
    /*  a format2 (8/16) table for cjk AND a format 4 (unicode) table. So this */
    /*  routine is now just used in a unicode context no matter what the font's */
    /*  encoding might be */
    if ( sf->chars[i]->unicodeenc>=65536 )	/* format 4 doesn't support 4byte encodings, we have an additional format 12 table for that */
return( -1 );

return( sf->chars[i]->unicodeenc );
#else
    switch ( sf->encoding_name ) {
      default:				/* Unicode */
	  if ( sf->chars[i]->unicodeenc>=65536 )	/* format 4 doesn't support 4byte encodings, we have an additional format 12 table for that */
return( -1 );

return( sf->chars[i]->unicodeenc );
      case em_big5:			/* Taiwan, Hong Kong */
      case em_big5hkscs:		/* ?Hong Kong? */
      case em_johab:			/* Korea */
      case em_wansung:			/* Korea */
      case em_sjis:			/* Japan */
return( i>65536?-1:i );
      case em_ksc5601:			/* Wansung */
	if ( (i/96)>=94 )
return( -1 );

return( ((i/96)<<8) + (i%96) + 0xa1a0 );
      case em_jis208: {			/* SJIS */
	int ch1, ch2, ro, co;
	ch1 = i/96; ch2 = i%96;
	if ( ch1>=94 )
return( -1 );
	ro = ch1<95 ? 112 : 176;
	co = (ch1&1) ? (ch2>95?32:31) : 126;

return(  ((((ch1+1)>>1) + ro )<<8 )    |    (ch2+co) );
      }
    }
#endif
}

static void dumpcmap(struct alltabs *at, SplineFont *_sf,enum fontformat format) {
    uint16 *sfind = NULL;
    int i,j,k,l,charcnt,enccnt;
    int segcnt, cnt=0, delta, rpos;
    struct cmapseg { uint16 start, end; uint16 delta; uint16 rangeoff; } *cmapseg;
    uint16 *ranges;
    char table[256];
    SplineFont *sf = _sf;
    SplineChar *sc, notdef, nonmarkingreturn;
    extern int greekfixup;
    int alreadyprivate = false;

    if ( sf->subfontcnt==0 && sf->chars[0]==NULL ) {	/* Encode the default notdef char at 0 */
	memset(&notdef,0,sizeof(notdef));
	notdef.unicodeenc = -1;
	notdef.name = ".notdef";
	notdef.parent = sf;
	sf->chars[0] = &notdef;
    }
    if ( sf->subfontcnt==0 && sf->chars[13]==NULL && !at->isotf ) {	/* Encode the default notdef char at 0 */
	memset(&nonmarkingreturn,0,sizeof(notdef));
	nonmarkingreturn.unicodeenc = 13;
	nonmarkingreturn.name = "nonmarkingreturn";
	nonmarkingreturn.parent = sf;
	nonmarkingreturn.ttf_glyph = 2;
	sf->chars[13] = &nonmarkingreturn;
    }

    at->cmap = tmpfile();

    /* MacRoman encoding table */ /* Not going to bother with making this work for cid fonts */
    memset(table,'\0',sizeof(table));
    for ( i=0; i<sf->charcnt && i<256; ++i ) {
	sc = SCDuplicate(SFFindExistingCharMac(sf,unicode_from_mac[i]));
	if ( sc!=NULL && sc->ttf_glyph!=-1 && sc->ttf_glyph<256 )
	    table[i] = sc->ttf_glyph;
    }
    table[0] = table[8] = table[13] = table[29] = 1;
    table[9] = table[32]==0 ? 1 : table[32];

    if ( format==ff_ttfsym ) {
	int acnt=0, pcnt=0;
	int space = table[9];
	for ( i=0; i<sf->charcnt; ++i ) {
	    if ( sf->chars[i]!=&notdef && sf->chars[i]!=&nonmarkingreturn &&
		    sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 &&
		    !SCIsNotdef(sf->chars[i],at->gi.fixed_width) && i<=0xffff ) {
		if ( i>=0xf000 && i<=0xf0ff )
		    ++acnt;
		else if ( i>=0x20 && i<=0xff )
		    ++pcnt;
	    }
	}
	alreadyprivate = acnt>pcnt;
	memset(table,'\0',sizeof(table));
	if ( !alreadyprivate ) {
	    for ( i=0; i<sf->charcnt && i<256; ++i ) {
		sc = SCDuplicate(sf->chars[i]);
		if ( sc!=NULL && sc->ttf_glyph!=-1 && sc->ttf_glyph<256 )
		    table[sf->chars[i]->enc] = sc->ttf_glyph;
	    }
	} else {
	    for ( i=0xf000; i<=0xf0ff && i<sf->charcnt; ++i ) {
		sc = SCDuplicate(sf->chars[i]);
		if ( sc!=NULL && sc->ttf_glyph!=-1 &&
			sc->ttf_glyph<256 )
		    table[sf->chars[i]->enc-0xf000] = sc->ttf_glyph;
	    }
	}
	table[0] = table[8] = table[13] = table[29] = 1;
	table[9] = space;
	/* if the user has read in a ttf symbol file then it will already have */
	/*  the right private use encoding, and we don't want to mess it up. */
	/*  The alreadyprivate flag should detect this case */
	if ( !alreadyprivate ) {
	    for ( i=0; i<sf->charcnt && i<256; ++i ) {
		if ( sf->chars[i]!=&notdef && sf->chars[i]!=&nonmarkingreturn &&
			sf->chars[i]!=NULL && !SCIsNotdef(sf->chars[i],at->gi.fixed_width)) {
		    sf->chars[i]->enc = sf->chars[i]->unicodeenc;
		    sf->chars[i]->unicodeenc = 0xf000 + i;
		}
	    }
	    for ( ; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
		sf->chars[i]->enc = sf->chars[i]->unicodeenc;
		sf->chars[i]->unicodeenc = -1;
	    }
	}
    }

    {
	uint32 *avail = galloc(65536*sizeof(uint32));
	int ucs4len=0, cjklen=0;
	FILE *format12 = NeedsUCS4Table(sf,&ucs4len);
	FILE *format2  = Needs816Enc(sf,&cjklen);
	int hasmac;
	long ucs4pos, mspos, cjkpos;
	memset(avail,0xff,65536*sizeof(uint32));
	if ( _sf->subfontcnt!=0 ) sfind = gcalloc(65536,sizeof(uint16));
	charcnt = _sf->charcnt;
	if ( _sf->subfontcnt!=0 ) {
	    for ( k=0; k<_sf->subfontcnt; ++k )
		if ( _sf->subfonts[k]->charcnt > charcnt )
		    charcnt =  _sf->subfonts[k]->charcnt;
	}
	for ( i=0; i<charcnt; ++i ) {
	    k = 0;
	    do {
		sf = (_sf->subfontcnt==0 ) ? _sf : _sf->subfonts[k];
		if ( i<sf->charcnt &&
			sf->chars[i]!=NULL && SCDuplicate(sf->chars[i])->ttf_glyph!=-1 &&
			(l = figureencoding(sf,i))!=-1 ) {
		    avail[l] = i;
		    if ( sfind!=NULL ) sfind[l] = k;
		    ++cnt;
	    break;
		}
		++k;
	    } while ( k<_sf->subfontcnt );
	}
	if ( _sf->encoding_name!=em_jis208 && _sf->encoding_name!=em_ksc5601 &&
		_sf->encoding_name!=em_sjis && _sf->encoding_name!=em_wansung &&
		_sf->encoding_name!=em_big5 && _sf->encoding_name!=em_big5hkscs &&
		_sf->encoding_name!=em_johab &&
		!greekfixup ) {
	    /* Duplicate glyphs for greek */	/* Only meaningful if unicode */
	    if ( avail[0xb5]==0xffffffff && avail[0x3bc]!=0xffffffff )
		SetTo(avail,sfind,0xb5,0x3bc);
	    else if ( avail[0x3bc]==0xffffffff && avail[0xb5]!=0xffffffff )
		SetTo(avail,sfind,0x3bc,0xb5);
	    if ( avail[0x394]==0xffffffff && avail[0x2206]!=0xffffffff )
		SetTo(avail,sfind,0x394,0x2206);
	    else if ( avail[0x2206]==0xffffffff && avail[0x394]!=0xffffffff )
		SetTo(avail,sfind,0x2206,0x394);
	    if ( avail[0x3a9]==0xffffffff && avail[0x2126]!=0xffffffff )
		SetTo(avail,sfind,0x3a9,0x2126);
	    else if ( avail[0x2126]==0xffffffff && avail[0x3a9]!=0xffffffff )
		SetTo(avail,sfind,0x2126,0x3a9);
	}

	j = -1;
	for ( i=segcnt=0; i<65536; ++i ) {
	    if ( avail[i]!=0xffffffff && j==-1 ) {
		j=i;
		++segcnt;
	    } else if ( j!=-1 && avail[i]==0xffffffff )
		j = -1;
	}
	cmapseg = gcalloc(segcnt+1,sizeof(struct cmapseg));
	ranges = galloc(cnt*sizeof(int16));
	j = -1;
	for ( i=segcnt=0; i<65536; ++i ) {
	    if ( avail[i]!=0xffffffff && j==-1 ) {
		j=i;
		cmapseg[segcnt].start = j;
		++segcnt;
	    } else if ( j!=-1 && avail[i]==0xffffffff ) {
		cmapseg[segcnt-1].end = i-1;
		j = -1;
	    }
	}
	if ( j!=-1 )
	    cmapseg[segcnt-1].end = i-1;
	/* create a dummy segment to mark the end of the table */
	cmapseg[segcnt].start = cmapseg[segcnt].end = 0xffff;
	cmapseg[segcnt++].delta = 1;
	rpos = 0;
	for ( i=0; i<segcnt-1; ++i ) {
	    l = avail[cmapseg[i].start];
	    sc = sfind==NULL ? _sf->chars[l] : _sf->subfonts[sfind[cmapseg[i].start]]->chars[l];
	    delta = sc->ttf_glyph-cmapseg[i].start;
	    for ( j=cmapseg[i].start; j<=cmapseg[i].end; ++j ) {
		l = avail[j];
		sc = sfind==NULL ? _sf->chars[l] : _sf->subfonts[sfind[j]]->chars[l];
		sc = SCDuplicate(sc);
		if ( delta != sc->ttf_glyph-j )
	    break;
	    }
	    if ( j>cmapseg[i].end )
		cmapseg[i].delta = delta;
	    else {
		cmapseg[i].rangeoff = (rpos + (segcnt-i)) * sizeof(int16);
		for ( j=cmapseg[i].start; j<=cmapseg[i].end; ++j ) {
		    l = avail[j];
		    sc = sfind==NULL ? _sf->chars[l] : _sf->subfonts[sfind[j]]->chars[l];
		    sc = SCDuplicate(sc);
		    ranges[rpos++] = sc->ttf_glyph;
		}
	    }
	}
	free(avail);
	if ( _sf->subfontcnt!=0 ) free(sfind);

	/* Two/Three/Four encoding table pointers, one for ms, one for mac */
	/*  usually one for mac big, just a copy of ms */
	/* plus we may have a format12 encoding for ucs4, mac doesn't support */
	/* plus we may have a format2 encoding for cjk, sometimes I know the codes for the mac... */
	hasmac = 1;
	enccnt = 3;
	if ( format12!=NULL )
	    enccnt = 4;
	else if ( format==ff_ttfsym ) {
	    enccnt = 2;
	    hasmac = 0;
	} else if ( format2!=NULL ) {
	    if ( sf->encoding_name==em_johab ) {
		enccnt=4;
	    } else {
		enccnt=5;
		hasmac=3;
	    }
	}
	putshort(at->cmap,0);		/* version */
	putshort(at->cmap,enccnt);	/* num tables */

	mspos = 2*sizeof(uint16)+enccnt*(2*sizeof(uint16)+sizeof(uint32));
	ucs4pos = mspos+(8+4*segcnt+rpos)*sizeof(int16);
	cjkpos = ucs4pos+ucs4len;
	if ( hasmac&1 ) {
	    /* big mac table, just a copy of the ms table */
	    putshort(at->cmap,0);	/* mac unicode platform */
	    putshort(at->cmap,3);	/* Unicode 2.0 */
	    putlong(at->cmap,mspos);
	}
	putshort(at->cmap,1);		/* mac platform */
	putshort(at->cmap,0);		/* plat specific enc, script=roman */
	    /* Even the symbol font on the mac claims a mac roman encoding */
	    /* although it actually contains a symbol encoding. There is an*/
	    /* "RSymbol" language listed for Mac (specific=8) but it isn't used*/
	putlong(at->cmap,ucs4pos+ucs4len);	/* offset from tab start to sub tab start */
	if ( format2!=NULL && (hasmac&2) ) {
	    /* big mac table, just a copy of the ms table */
	    putshort(at->cmap,1);	/* mac platform */
	    putshort(at->cmap,
		sf->encoding_name==em_jis208? 1 :	/* SJIS */
		sf->encoding_name==em_sjis? 1 :		/* SJIS */
		sf->encoding_name==em_ksc5601? 3 :	/* Korean */
		sf->encoding_name==em_wansung? 3 :	/* Korean */
		2 );					/* Big5 */
	    putlong(at->cmap,cjkpos);
	}

	putshort(at->cmap,3);		/* ms platform */
	putshort(at->cmap,		/* plat specific enc */
		format==ff_ttfsym ? 0 :
		 1 );					/* Unicode */
	putlong(at->cmap,mspos);		/* offset from tab start to sub tab start */

	if ( format2!=NULL ) {
	    putshort(at->cmap,3);		/* ms platform */
	    putshort(at->cmap,		/* plat specific enc */
		    sf->encoding_name==em_ksc5601 ? 5 :	/* Wansung, korean */
		    sf->encoding_name==em_wansung ? 5 :	/* Wansung, korean */
		    sf->encoding_name==em_jis208 ? 2 :	/* SJIS */
		    sf->encoding_name==em_sjis ? 2 :	/* SJIS */
		    sf->encoding_name==em_big5 ? 4 :	/* Big5, traditional Chinese */
		    sf->encoding_name==em_big5hkscs ? 4 :
		    /*sf->encoding_name==em_johab ?*/ 6);/* Korean */
	    putlong(at->cmap,cjkpos);		/* offset from tab start to sub tab start */
	}

	if ( format12!=NULL ) {
	    putshort(at->cmap,3);		/* ms platform */
	    putshort(at->cmap,10);		/* plat specific enc, ucs4 */
	    putlong(at->cmap,ucs4pos);		/* offset from tab start to sub tab start */
	}

	putshort(at->cmap,4);		/* format */
	putshort(at->cmap,(8+4*segcnt+rpos)*sizeof(int16));
	putshort(at->cmap,0);		/* language/version */
	putshort(at->cmap,2*segcnt);	/* segcnt */
	for ( j=0,i=1; i<=segcnt; i<<=1, ++j );
	putshort(at->cmap,i);		/* 2*2^floor(log2(segcnt)) */
	putshort(at->cmap,j-1);
	putshort(at->cmap,2*segcnt-i);
	for ( i=0; i<segcnt; ++i )
	    putshort(at->cmap,cmapseg[i].end);
	putshort(at->cmap,0);
	for ( i=0; i<segcnt; ++i )
	    putshort(at->cmap,cmapseg[i].start);
	for ( i=0; i<segcnt; ++i )
	    putshort(at->cmap,cmapseg[i].delta);
	for ( i=0; i<segcnt; ++i )
	    putshort(at->cmap,cmapseg[i].rangeoff);
	for ( i=0; i<rpos; ++i )
	    putshort(at->cmap,ranges[i]);
	free(ranges);
	free(cmapseg);

	if ( format12!=NULL ) {
	    if ( !ttfcopyfile(at->cmap,format12,ucs4pos)) at->error = true;
	}
	if ( format2!=NULL ) {
	    if ( !ttfcopyfile(at->cmap,format2,cjkpos)) at->error = true;
	}
    }

    /* Mac table */
    putshort(at->cmap,0);		/* format */
    putshort(at->cmap,262);	/* length = 256bytes + 6 header bytes */
    putshort(at->cmap,0);		/* language = english */
    for ( i=0; i<256; ++i )
	putc(table[i],at->cmap);

    at->cmaplen = ftell(at->cmap);
    if ( (at->cmaplen&2)!=0 )
	putshort(at->cmap,0);

    if ( format==ff_ttfsym ) {
	if ( !alreadyprivate ) {
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=&notdef && sf->chars[i]!=&nonmarkingreturn && sf->chars[i]!=NULL ) {
		sf->chars[i]->unicodeenc = sf->chars[i]->enc;
		sf->chars[i]->enc = i;
	    }
	}
    }
    if ( sf->subfontcnt==0 && sf->chars[0]==&notdef )
	sf->chars[0] = NULL;
    if ( sf->subfontcnt==0 && sf->chars[13]==&nonmarkingreturn )
	sf->chars[13] = NULL;
}

static int32 filecheck(FILE *file) {
    uint32 sum = 0, chunk;

    rewind(file);
    while ( 1 ) {
	chunk = getuint32(file);
	if ( feof(file))
    break;
	sum += chunk;
    }
return( sum );
}
    
static void AbortTTF(struct alltabs *at, SplineFont *sf) {
    int i;

    if ( at->loca!=NULL )
	fclose(at->loca);
    if ( at->name!=NULL )
	fclose(at->name);
    if ( at->post!=NULL )
	fclose(at->post);
    if ( at->gpos!=NULL )
	fclose(at->gpos);
    if ( at->gsub!=NULL )
	fclose(at->gsub);
    if ( at->gdef!=NULL )
	fclose(at->gdef);
    if ( at->kern!=NULL )
	fclose(at->kern);
    if ( at->cmap!=NULL )
	fclose(at->cmap);
    if ( at->headf!=NULL )
	fclose(at->headf);
    if ( at->hheadf!=NULL )
	fclose(at->hheadf);
    if ( at->maxpf!=NULL )
	fclose(at->maxpf);
    if ( at->os2f!=NULL )
	fclose(at->os2f);
    if ( at->cvtf!=NULL )
	fclose(at->cvtf);
    if ( at->vheadf!=NULL )
	fclose(at->vheadf);
    if ( at->vorgf!=NULL )
	fclose(at->vorgf);
    if ( at->cfff!=NULL )
	fclose(at->cfff);

    if ( at->gi.glyphs!=NULL )
	fclose(at->gi.glyphs);
    if ( at->gi.hmtx!=NULL )
	fclose(at->gi.hmtx);
    if ( at->gi.vmtx!=NULL )
	fclose(at->gi.vmtx);
    if ( at->fpgmf!=NULL )
	fclose(at->fpgmf);
    if ( at->prepf!=NULL )
	fclose(at->prepf);
    if ( at->gaspf!=NULL )
	fclose(at->gaspf);

    if ( at->sidf!=NULL )
	fclose(at->sidf);
    if ( at->sidh!=NULL )
	fclose(at->sidh);
    if ( at->charset!=NULL )
	fclose(at->charset);
    if ( at->encoding!=NULL )
	fclose(at->encoding);
    if ( at->private!=NULL )
	fclose(at->private);
    if ( at->charstrings!=NULL )
	fclose(at->charstrings);
    if ( at->fdselect!=NULL )
	fclose(at->fdselect);
    if ( at->fdarray!=NULL )
	fclose(at->fdarray);
    if ( at->bdat!=NULL )
	fclose(at->bdat);
    if ( at->bloc!=NULL )
	fclose(at->bloc);
    if ( at->ebsc!=NULL )
	fclose(at->ebsc);

    if ( at->prop!=NULL )
	fclose(at->prop);
    if ( at->opbd!=NULL )
	fclose(at->opbd);
    if ( at->acnt!=NULL )
	fclose(at->acnt);
    if ( at->lcar!=NULL )
	fclose(at->lcar);
    if ( at->feat!=NULL )
	fclose(at->feat);
    if ( at->morx!=NULL )
	fclose(at->morx);

    for ( i=0; i<sf->subfontcnt; ++i ) {
	if ( at->fds[i].private!=NULL )
	    fclose(at->fds[i].private);
    }
    if ( at->fds!=NULL )
	free( at->fds );
}

static int SFHasInstructions(SplineFont *sf) {
    int i;

    if ( sf->subfontcnt!=0 )
return( false );		/* Truetype doesn't support cid keyed fonts */

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	if ( sf->chars[i]->ttf_instrs!=NULL )
return( true );
    }
return( false );
}

static void MaxpFromTable(struct alltabs *at,struct ttf_table *maxp ) {
    if ( maxp==NULL || maxp->len<13*sizeof(uint16) )
return;
    /* We can figure out the others ourselves, but these depend on the contents */
    /*  of uninterpretted tables */
    at->maxp.maxZones = memushort(maxp->data,7*sizeof(uint16));
    at->maxp.maxTwilightPts = memushort(maxp->data,8*sizeof(uint16));
    at->maxp.maxStorage = memushort(maxp->data,9*sizeof(uint16));
    at->maxp.maxFDEFs = memushort(maxp->data,10*sizeof(uint16));
    at->maxp.maxIDEFs = memushort(maxp->data,11*sizeof(uint16));
    at->maxp.maxStack = memushort(maxp->data,12*sizeof(uint16));
}

static FILE *dumpstoredtable(SplineFont *sf,uint32 tag,int *len) {
    struct ttf_table *tab = SFFindTable(sf,tag);
    FILE *out;

    if ( tab==NULL ) {
	*len = 0;
return( NULL );
    }

    out = tmpfile();
    fwrite(tab->data,1,tab->len,out);
    if ( (tab->len&1))
	putc('\0',out);
    if ( (tab->len+1)&2 )
	putshort(out,0);
    *len = tab->len;
return( out );
}

static int initTables(struct alltabs *at, SplineFont *sf,enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags) {
    int i, j, pos, aborted;
    BDFFont *bdf;

    if ( sf->encoding_name == em_symbol && format==ff_ttf )
	format = ff_ttfsym;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	sf->chars[i]->ttf_glyph = -1;

    SFDefaultOS2Info(&sf->pfminfo,sf,sf->fontname);

    memset(at,'\0',sizeof(struct alltabs));
    at->gi.xmin = at->gi.ymin = 15000;
    at->gi.sf = sf;
    if ( bf!=bf_ttf && bf!=bf_sfnt_dfont)
	bsizes = NULL;
    if ( bsizes!=NULL ) {
	for ( i=j=0; bsizes[i]!=0; ++i ) {
	    for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(bsizes[i]&0xffff) || BDFDepth(bdf)!=(bsizes[i]>>16)); bdf=bdf->next );
	    if ( bdf!=NULL )
		bsizes[j++] = bsizes[i];
	}
	bsizes[j] = 0;
	for ( i=0; bsizes[i]!=0; ++i );
	at->gi.strikecnt = i;
	if ( i==0 ) bsizes=NULL;
    }
    at->applemode = (flags&ttf_flag_applemode)?1:0;
    at->msbitmaps = bsizes!=NULL && !at->applemode;
    at->gi.onlybitmaps = format==ff_none;
    at->gi.flags = flags;
    at->gi.bsizes = bsizes;
    at->gi.fixed_width = CIDOneWidth(sf);
    at->isotf = format==ff_otf || format==ff_otfcid;

    at->maxp.version = 0x00010000;
    if ( format==ff_otf || format==ff_otfcid || (format==ff_none && at->applemode) )
	at->maxp.version = 0x00005000;
    at->maxp.maxnumcomponents = 1;
    at->maxp.maxcomponentdepth = 1;
    at->maxp.maxZones = 2;		/* 1 would probably do, don't use twilight */
    at->maxp.maxFDEFs = 1;		/* Not even 1 */
    at->maxp.maxStorage = 1;		/* Not even 1 */
    at->maxp.maxStack = 64;		/* A guess, it's probably more like 8 */
    if ( format==ff_otf || format==ff_otfcid || (format==ff_none && at->applemode) )
	at->maxp.version = 0x00005000;
    else
	MaxpFromTable(at,SFFindTable(sf,CHR('m','a','x','p')));
    at->gi.maxp = &at->maxp;
    if ( format==ff_otf )
	aborted = !dumptype2glyphs(sf,at);
    else if ( format==ff_otfcid )
	aborted = !dumpcidglyphs(sf,at);
    else if ( format==ff_none && at->applemode ) {
	AssignTTFGlyph(sf,bsizes);
	aborted = !dumpcffhmtx(at,sf,true);
    } else
	/* if format==ff_none the following will put out lots of space glyphs */
	aborted = !dumpglyphs(sf,&at->gi);
    if ( bsizes!=NULL && !aborted )
	ttfdumpbitmap(sf,at,bsizes);
    if ( bsizes!=NULL && format==ff_none && at->msbitmaps )
	ttfdumpbitmapscaling(sf,at,bsizes);
    if ( aborted ) {
	AbortTTF(at,sf);
return( false );
    }
    at->head.xmin = at->gi.xmin;
    at->head.ymin = at->gi.ymin;
    at->head.xmax = at->gi.xmax;
    at->head.ymax = at->gi.ymax;
    sethead(&at->head,sf);
    sethhead(&at->hhead,&at->vhead,at,sf);
    setvorg(&at->vorg,sf);
    setos2(&at->os2,at,sf,format);	/* should precede kern/ligature output */
    if ( at->gi.glyph_len<0x20000 )
	at->head.locais32 = 0;
    if ( format!=ff_otf && format!=ff_otfcid && (format!=ff_none || at->msbitmaps) )
	redoloca(at);
    redohead(at);
    redohhead(at,false);
    if ( sf->hasvmetrics ) {
	redohhead(at,true);
	redovorg(at);		/* I know, VORG is only meaningful in a otf font and I dump it out in ttf too. Well, it will help ME read the font back in, and it won't bother anyone else. So there. */
    }
    redomaxp(at,format);
    if ( !at->applemode ) {
	otf_orderlangs(sf);
	otf_dumpgpos(at,sf);
	otf_dumpgsub(at,sf);
	otf_dumpgdef(at,sf);
    } else {
	aat_dumpacnt(at,sf);		/* Placeholder for now */
	ttf_dumpkerns(at,sf);
	aat_dumplcar(at,sf);
	aat_dumpmorx(at,sf);		/* Sets the feat table too */
	aat_dumpopbd(at,sf);
	aat_dumpprop(at,sf);
    }
    dumpnames(at,sf,format);		/* Must be after dumpmorx which may create extra names */
    redoos2(at);
    if ( format!=ff_otf && format!=ff_otfcid && format!=ff_none ) {
	if ( !SFHasInstructions(sf))
	    dumpgasp(at);
	at->fpgmf = dumpstoredtable(sf,CHR('f','p','g','m'),&at->fpgmlen);
	at->prepf = dumpstoredtable(sf,CHR('p','r','e','p'),&at->preplen);
	at->cvtf = dumpstoredtable(sf,CHR('c','v','t',' '),&at->cvtlen);
    }
    dumppost(at,sf,format);
    dumpcmap(at,sf,format);

    if ( format==ff_otf || format==ff_otfcid ) {
	at->tabdir.version = CHR('O','T','T','O');
    } else if ( at->applemode ) {
	at->tabdir.version = CHR('t','r','u','e');
    } else {
	at->tabdir.version = 0x00010000;
    }

    i = 0;
    pos = 0;

    if ( format==ff_otf || format==ff_otfcid ) {
	at->tabdir.tabs[i].tag = CHR('C','F','F',' ');
	at->tabdir.tabs[i].checksum = filecheck(at->cfff);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->cfflen;
	pos += ((at->cfflen+3)>>2)<<2;
    }

    if ( at->bdat!=NULL && at->msbitmaps ) {
	at->tabdir.tabs[i].tag = CHR('E','B','D','T');
	at->tabdir.tabs[i].checksum = filecheck(at->bdat);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->bdatlen;
	pos += ((at->bdatlen+3)>>2)<<2;
    }

    if ( at->bloc!=NULL && at->msbitmaps ) {
	at->tabdir.tabs[i].tag = CHR('E','B','L','C');
	at->tabdir.tabs[i].checksum = filecheck(at->bloc);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->bloclen;
	pos += ((at->bloclen+3)>>2)<<2;
    }

    if ( at->ebsc!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('E','B','S','C');
	at->tabdir.tabs[i].checksum = filecheck(at->ebsc);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->ebsclen;
	pos += ((at->ebsclen+3)>>2)<<2;
    }

    if ( at->gdef!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('G','D','E','F');
	at->tabdir.tabs[i].checksum = filecheck(at->gdef);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gposlen;
	pos += ((at->gdeflen+3)>>2)<<2;
    }

    if ( at->gpos!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('G','P','O','S');
	at->tabdir.tabs[i].checksum = filecheck(at->gpos);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gposlen;
	pos += ((at->gposlen+3)>>2)<<2;
    }

    if ( at->gsub!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('G','S','U','B');
	at->tabdir.tabs[i].checksum = filecheck(at->gsub);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gsublen;
	pos += ((at->gsublen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('O','S','/','2');
    at->tabdir.tabs[i].checksum = filecheck(at->os2f);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->os2len;
    pos += ((at->os2len+3)>>2)<<2;

    if ( at->vorgf!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('V','O','R','G');
	at->tabdir.tabs[i].checksum = filecheck(at->vorgf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->vorglen;
	pos += ((at->vorglen+3)>>2)<<2;
    }

    if ( at->acnt!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('a','c','n','t');
	at->tabdir.tabs[i].checksum = filecheck(at->acnt);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->acntlen;
	pos += ((at->acntlen+3)>>2)<<2;
    }

    if ( at->bdat!=NULL && !at->msbitmaps ) {
	at->tabdir.tabs[i].tag = CHR('b','d','a','t');
	at->tabdir.tabs[i].checksum = filecheck(at->bdat);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->bdatlen;
	pos += ((at->bdatlen+3)>>2)<<2;
    }

    if ( format==ff_none && !at->msbitmaps ) {
	/* Bitmap only fonts get a bhed table rather than a head */
	at->tabdir.tabs[i].tag = CHR('b','h','e','d');
	at->tabdir.tabs[i].checksum = filecheck(at->headf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->headlen;
	pos += ((at->headlen+3)>>2)<<2;
    }

    if ( at->bloc!=NULL && !at->msbitmaps ) {
	at->tabdir.tabs[i].tag = CHR('b','l','o','c');
	at->tabdir.tabs[i].checksum = filecheck(at->bloc);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->bloclen;
	pos += ((at->bloclen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('c','m','a','p');
    at->tabdir.tabs[i].checksum = filecheck(at->cmap);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->cmaplen;
    pos += ((at->cmaplen+3)>>2)<<2;

    if ( format!=ff_otf && format!=ff_otfcid && (format!=ff_none || at->msbitmaps) ) {
	if ( format!=ff_none && at->cvtf!=NULL ) {
	    at->tabdir.tabs[i].tag = CHR('c','v','t',' ');
	    at->tabdir.tabs[i].checksum = filecheck(at->cvtf);
	    at->tabdir.tabs[i].offset = pos;
	    at->tabdir.tabs[i++].length = at->cvtlen;
	    pos += ((at->cvtlen+3)>>2)<<2;
	}
    }

    if ( at->feat!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('f','e','a','t');
	at->tabdir.tabs[i].checksum = filecheck(at->feat);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->featlen;
	pos += ((at->featlen+3)>>2)<<2;
    }

    if ( format!=ff_otf && format!=ff_otfcid && (format!=ff_none || at->msbitmaps) ) {
	if ( at->fpgmf!=NULL ) {
	    at->tabdir.tabs[i].tag = CHR('f','p','g','m');
	    at->tabdir.tabs[i].checksum = filecheck(at->fpgmf);
	    at->tabdir.tabs[i].offset = pos;
	    at->tabdir.tabs[i++].length = at->fpgmlen;
	    pos += ((at->fpgmlen+3)>>2)<<2;
	}

	if ( at->gaspf!=NULL ) {
	    at->tabdir.tabs[i].tag = CHR('g','a','s','p');
	    at->tabdir.tabs[i].checksum = filecheck(at->gaspf);
	    at->tabdir.tabs[i].offset = pos;
	    at->tabdir.tabs[i++].length = at->gasplen;
	    pos += ((at->gasplen+3)>>2)<<2;
	}

	at->tabdir.tabs[i].tag = CHR('g','l','y','f');
	at->tabdir.tabs[i].checksum = filecheck(at->gi.glyphs);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gi.glyph_len;
	pos += ((at->gi.glyph_len+3)>>2)<<2;
    }

    if ( format!=ff_none || at->msbitmaps ) {
	at->tabdir.tabs[i].tag = CHR('h','e','a','d');
	at->tabdir.tabs[i].checksum = filecheck(at->headf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->headlen;
	pos += ((at->headlen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('h','h','e','a');
    at->tabdir.tabs[i].checksum = filecheck(at->hheadf);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->hheadlen;
    pos += ((at->hheadlen+3)>>2)<<2;

    at->tabdir.tabs[i].tag = CHR('h','m','t','x');
    at->tabdir.tabs[i].checksum = filecheck(at->gi.hmtx);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->gi.hmtxlen;
    pos += ((at->gi.hmtxlen+3)>>2)<<2;

    if ( at->kern!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('k','e','r','n');
	at->tabdir.tabs[i].checksum = filecheck(at->kern);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->kernlen;
	pos += ((at->kernlen+3)>>2)<<2;
    }

    if ( at->lcar!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('l','c','a','r');
	at->tabdir.tabs[i].checksum = filecheck(at->lcar);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->lcarlen;
	pos += ((at->lcarlen+3)>>2)<<2;
    }

    if ( format!=ff_otf && format!=ff_otfcid  && (format!=ff_none || at->msbitmaps) ) {
	at->tabdir.tabs[i].tag = CHR('l','o','c','a');
	at->tabdir.tabs[i].checksum = filecheck(at->loca);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->localen;
	pos += ((at->localen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('m','a','x','p');
    at->tabdir.tabs[i].checksum = filecheck(at->maxpf);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->maxplen;
    pos += ((at->maxplen+3)>>2)<<2;

    if ( at->morx!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('m','o','r','x');
	at->tabdir.tabs[i].checksum = filecheck(at->morx);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->morxlen;
	pos += ((at->morxlen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('n','a','m','e');
    at->tabdir.tabs[i].checksum = filecheck(at->name);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->namelen;
    pos += ((at->namelen+3)>>2)<<2;

    if ( at->opbd!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('o','p','b','d');
	at->tabdir.tabs[i].checksum = filecheck(at->opbd);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->opbdlen;
	pos += ((at->opbdlen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('p','o','s','t');
    at->tabdir.tabs[i].checksum = filecheck(at->post);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->postlen;
    pos += ((at->postlen+3)>>2)<<2;

    if ( format!=ff_otf && format!=ff_otfcid && (format!=ff_none || at->msbitmaps) ) {
	if ( at->prepf!=NULL ) {
	    at->tabdir.tabs[i].tag = CHR('p','r','e','p');
	    at->tabdir.tabs[i].checksum = filecheck(at->prepf);
	    at->tabdir.tabs[i].offset = pos;
	    at->tabdir.tabs[i++].length = at->preplen;
	    pos += ((at->preplen+3)>>2)<<2;
	}
    }

    if ( at->prop!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('p','r','o','p');
	at->tabdir.tabs[i].checksum = filecheck(at->prop);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->proplen;
	pos += ((at->proplen+3)>>2)<<2;
    }

    if ( at->vheadf!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('v','h','e','a');
	at->tabdir.tabs[i].checksum = filecheck(at->vheadf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->vheadlen;
	pos += ((at->vheadlen+3)>>2)<<2;

	at->tabdir.tabs[i].tag = CHR('v','m','t','x');
	at->tabdir.tabs[i].checksum = filecheck(at->gi.vmtx);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gi.vmtxlen;
	pos += ((at->gi.vmtxlen+3)>>2)<<2;
    }
    if ( i>=sizeof(at->tabdir.tabs)/sizeof(at->tabdir.tabs[0]))
	GDrawIError("Miscalculation of number of tables needed. Up sizeof tabs array in struct tabdir" );

    at->tabdir.numtab = i;
    at->tabdir.searchRange = (i<16?8:i<32?16:i<64?32:64)*16;
    at->tabdir.entrySel = (i<16?3:i<32?4:i<64?5:6);
    at->tabdir.rangeShift = at->tabdir.numtab*16-at->tabdir.searchRange;
    for ( i=0; i<at->tabdir.numtab; ++i )
	at->tabdir.tabs[i].offset += sizeof(int32)+4*sizeof(int16) + at->tabdir.numtab*4*sizeof(int32);
return( true );
}

static void dumpttf(FILE *ttf,struct alltabs *at, enum fontformat format) {
    int32 checksum;
    int i, head_index=-1;
    /* I can't use fwrite because I (may) have to byte swap everything */

    putlong(ttf,at->tabdir.version);
    putshort(ttf,at->tabdir.numtab);
    putshort(ttf,at->tabdir.searchRange);
    putshort(ttf,at->tabdir.entrySel);
    putshort(ttf,at->tabdir.rangeShift);
    for ( i=0; i<at->tabdir.numtab; ++i ) {
	putlong(ttf,at->tabdir.tabs[i].tag);
	putlong(ttf,at->tabdir.tabs[i].checksum);
	putlong(ttf,at->tabdir.tabs[i].offset);
	putlong(ttf,at->tabdir.tabs[i].length);
    }

    i=0;
    if ( format==ff_otf || format==ff_otfcid)
	if ( !ttfcopyfile(ttf,at->cfff,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->bdat!=NULL && at->msbitmaps ) {
	if ( !ttfcopyfile(ttf,at->bdat,at->tabdir.tabs[i++].offset)) at->error = true;
	if ( !ttfcopyfile(ttf,at->bloc,at->tabdir.tabs[i++].offset)) at->error = true;
    }
    if ( at->ebsc!=NULL )
	if ( !ttfcopyfile(ttf,at->ebsc,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->gdef!=NULL )
	if ( !ttfcopyfile(ttf,at->gdef,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->gpos!=NULL )
	if ( !ttfcopyfile(ttf,at->gpos,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->gsub!=NULL )
	if ( !ttfcopyfile(ttf,at->gsub,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->os2f,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->vorgf!=NULL )
	if ( !ttfcopyfile(ttf,at->vorgf,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->acnt!=NULL )
	if ( !ttfcopyfile(ttf,at->acnt,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->bdat!=NULL && !at->msbitmaps ) {
	if ( !ttfcopyfile(ttf,at->bdat,at->tabdir.tabs[i++].offset)) at->error = true;
	if ( format==ff_none ) {
	    head_index = i;
	    if ( !ttfcopyfile(ttf,at->headf,at->tabdir.tabs[i++].offset)) at->error = true;
	}
	if ( !ttfcopyfile(ttf,at->bloc,at->tabdir.tabs[i++].offset)) at->error = true;
    }
    if ( !ttfcopyfile(ttf,at->cmap,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( format!=ff_otf && format!= ff_otfcid && (format!=ff_none || at->msbitmaps) ) {
	if ( format!=ff_none && at->cvtf!=NULL ) {
	    if ( !ttfcopyfile(ttf,at->cvtf,at->tabdir.tabs[i++].offset)) at->error = true;
	}
    }
    if ( at->feat!=NULL )
	if ( !ttfcopyfile(ttf,at->feat,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( format!=ff_otf && format!= ff_otfcid && (format!=ff_none || at->msbitmaps) ) {
	if ( at->fpgmf!=NULL ) {
	    if ( !ttfcopyfile(ttf,at->fpgmf,at->tabdir.tabs[i++].offset)) at->error = true;
	}
	if ( at->gaspf!=NULL )
	    if ( !ttfcopyfile(ttf,at->gaspf,at->tabdir.tabs[i++].offset)) at->error = true;
	if ( !ttfcopyfile(ttf,at->gi.glyphs,at->tabdir.tabs[i++].offset)) at->error = true;
    }
    if ( format!=ff_none || at->msbitmaps ) {
	head_index = i;
	if ( !ttfcopyfile(ttf,at->headf,at->tabdir.tabs[i++].offset)) at->error = true;
    }
    if ( !ttfcopyfile(ttf,at->hheadf,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->gi.hmtx,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->kern!=NULL )
	if ( !ttfcopyfile(ttf,at->kern,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->lcar!=NULL )
	if ( !ttfcopyfile(ttf,at->lcar,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( format!=ff_otf && format!=ff_otfcid && (format!=ff_none || at->msbitmaps) )
	if ( !ttfcopyfile(ttf,at->loca,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->maxpf,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->morx!=NULL )
	if ( !ttfcopyfile(ttf,at->morx,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->name,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->opbd!=NULL )
	if ( !ttfcopyfile(ttf,at->opbd,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->post,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->prepf!=NULL )
	if ( !ttfcopyfile(ttf,at->prepf,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->prop!=NULL )
	if ( !ttfcopyfile(ttf,at->prop,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->vheadf!=NULL ) {
	if ( !ttfcopyfile(ttf,at->vheadf,at->tabdir.tabs[i++].offset)) at->error = true;
	if ( !ttfcopyfile(ttf,at->gi.vmtx,at->tabdir.tabs[i++].offset)) at->error = true;
    }

    checksum = filecheck(ttf);
    checksum = 0xb1b0afba-checksum;
    fseek(ttf,at->tabdir.tabs[head_index].offset+2*sizeof(int32),SEEK_SET);
    putlong(ttf,checksum);

    /* ttfcopyfile closed all the files (except ttf) */
}

static void DumpGlyphToNameMap(char *fontname,SplineFont *sf) {
    char *d, *e;
    char *newname = galloc(strlen(fontname)+10);
    FILE *file;
    int i,k,max;
    SplineChar *sc;

    strcpy(newname,fontname);
    d = strrchr(newname,'/');
    if ( d==NULL ) d=newname;
    e = strrchr(d,'.');
    if ( e==NULL ) e = newname+strlen(newname);
    strcpy(e,".g2n");

    file = fopen(newname,"w");
    if ( file==NULL ) {
	fprintf(stderr, "Failed to open glyph to name map file for writing: %s\n", newname );
	free(newname);
return;
    }

    if ( sf->subfontcnt==0 )
	max = sf->charcnt;
    else {
	for ( k=max=0; k<sf->subfontcnt; ++k )
	    if ( sf->subfonts[k]->charcnt > max )
		max = sf->subfonts[k]->charcnt;
    }
    for ( i=0; i<max; ++i ) {
	if ( sf->subfontcnt==0 )
	    sc = sf->chars[i];
	else {
	    for ( k=0; k<sf->subfontcnt; ++k ) if ( i<sf->subfonts[k]->charcnt )
		if ( (sc=sf->subfonts[k]->chars[i])!=NULL )
	    break;
	}
	if ( sc!=NULL && sc->ttf_glyph!=-1 ) {
	    fprintf( file, "GLYPHID %d\tPSNAME %s", sc->ttf_glyph, sc->name );
	    if ( sc->unicodeenc!=-1 )
		fprintf( file, "\tUNICODE %04X", sc->unicodeenc );
	    putc('\n',file);
	}
    }
    fclose(file);
    free(newname);
}

int _WriteTTFFont(FILE *ttf,SplineFont *sf,enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags) {
    struct alltabs at;
    char *oldloc;

    oldloc = setlocale(LC_NUMERIC,"C");		/* TrueType probably doesn't need this, but OpenType does for floats in dictionaries */
    if ( format==ff_otfcid ) {
	if ( sf->cidmaster ) sf = sf->cidmaster;
    } else {
	if ( sf->subfontcnt!=0 ) sf = sf->subfonts[0];
    }
    if ( initTables(&at,sf,format,bsizes,bf,flags))
	dumpttf(ttf,&at,format);
    setlocale(LC_NUMERIC,oldloc);
    if ( at.error || ferror(ttf))
return( 0 );

return( 1 );
}

int WriteTTFFont(char *fontname,SplineFont *sf,enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags) {
    FILE *ttf;
    int ret;

    if (( ttf=fopen(fontname,"w+"))==NULL )
return( 0 );
    ret = _WriteTTFFont(ttf,sf,format,bsizes,bf,flags);
    if ( ret && glyph_2_name_map )
	DumpGlyphToNameMap(fontname,sf);
    if ( fclose(ttf)==-1 )
return( 0 );
return( ret );
}

/* Fontograpgher also generates: fpgm, hdmx, prep */
