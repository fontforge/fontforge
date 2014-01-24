/* Copyright (C) 2001 by George Williams */
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef struct basepoint {
    double x, y;
} BasePoint;

struct dup {
    int glyph;
    int enc;
    struct dup *prev;
};

struct ttfinfo {
    int emsize;			/* from head table */
    int ascent, descent;	/* from hhea table */
    int glyph_cnt;		/* from maxp table */
    int width_cnt;		/* from the hhea table */
    unsigned int index_to_loc_is_long:1;	/* in head table */
    /* Mac fonts platform=0/1, platform specific enc id, roman=0, english is lang code 0 */
    /* iso platform=2, platform specific enc id, latin1=0/2, no language */
    /* microsoft platform=3, platform specific enc id, 1, english is lang code 0x??09 */
    int numtables;
    		/* cmap */
    int encoding_start;		/* Offset from sof to start of encoding table */
		/* glyf */
    int glyph_start;		/* Offset from sof to start of glyph table */
		/* EBDT */
    int bitmapdata_start;	/* Offset to start of bitmap data */
		/* EBLT */
    int bitmaploc_start;	/* Offset to start of bitmap locator data */
		/* head */
    int head_start;
		/* hhea */
    int hhea_start;
		/* hmtx */
    int hmetrics_start;
		/* kern */
    int kern_start;
		/* loca */
    int glyphlocations_start;	/* there are glyph_cnt of these, from maxp tab */
    int loca_length;		/* actually glypn_cnt is wrong. Use the table length (divided by size) instead */
		/* maxp */
    int maxp_start;		/* maximum number of glyphs */
		/* name */
    int copyright_start;	/* copyright and fontname */
		/* post */
    int postscript_start;	/* names for the glyphs, italic angle, etc. */
    int postscript_len;		/* names for the glyphs, italic angle, etc. */
		/* OS/2 */
    int os2_start;
    int os2_len;

    int fpgm_start;
    int fpgm_len;
    int prep_start;
    int prep_len;

    unsigned int *glyph_offsets;
    char **glyph_names;
    unsigned short *glyph_unicode;
    int *glyph_widths;
    struct dup *dups;
};

static char *standardnames[258] = {
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

const unsigned short unicode_from_mac[] = {
  0x0000, 0x00d0, 0x00f0, 0x0141, 0x0142, 0x0160, 0x0161, 0x00dd,
  0x00fd, 0x0009, 0x000a, 0x00de, 0x00fe, 0x000d, 0x017d, 0x017e,
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x00bd, 0x00bc, 0x00b9,
  0x00be, 0x00b3, 0x00b2, 0x00a6, 0x00ad, 0x00d7, 0x001e, 0x001f,
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
  0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
  0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
  0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
  0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
  0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
  0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1,
  0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8,
  0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3,
  0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc,
  0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df,
  0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8,
  0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211,
  0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x2126, 0x00e6, 0x00f8,
  0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab,
  0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153,
  0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca,
  0x00ff, 0x0178, 0x2044, 0x00a4, 0x2039, 0x203a, 0xfb01, 0xfb02,
  0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1,
  0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4,
  0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc,
  0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7
};

static int getushort(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

static int getlong(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    int ch4 = getc(ttf);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

static double getfixed(FILE *ttf) {
    int val = getlong(ttf);
    int mant = val&0xffff;
    /* This oddity may be needed to deal with the first 16 bits being signed */
    /*  and the low-order bits unsigned */
return( (double) (val>>16) + (mant/65536.0) );
}

static double get2dot14(FILE *ttf) {
    int val = getushort(ttf);
    int mant = val&0x3fff;
    /* This oddity may be needed to deal with the first 2 bits being signed */
    /*  and the low-order bits unsigned */
return( (double) ((val<<16)>>(16+14)) + (mant/16384.0) );
}

static void readttfhead(FILE *ttf, struct ttfinfo *info) {
    int i;

    fseek(ttf,info->head_start+4*4+2,SEEK_SET);		/* skip over the version number and a bunch of junk */
    info->emsize = getushort(ttf);
    for ( i=0; i<15; ++i )
	getushort(ttf);
    info->index_to_loc_is_long = getushort(ttf);
    if ( info->index_to_loc_is_long )
	info->glyph_cnt = info->loca_length/4-1;
}

static void readttfhhea(FILE *ttf,struct ttfinfo *info) {
    /* Here I want ascent, descent and the number of horizontal metrics */
    int i;

    fseek(ttf,info->hhea_start+4,SEEK_SET);		/* skip over the version number */
    info->ascent = getushort(ttf);
    info->descent = -(short) getushort(ttf);

    /* fontographer puts the max ascender/min descender here instead. idiots */
    if ( info->ascent==0 && info->descent==0 )
	info->ascent = .8*info->emsize;
    if ( info->ascent+info->descent!=info->emsize )
	info->ascent = info->ascent * ((double) info->emsize)/(info->ascent+info->descent);
    info->descent = info->emsize-info->ascent;

    for ( i=0; i<13; ++i )
	getushort(ttf);
    info->width_cnt = getushort(ttf);
}

static void readttfpost(FILE *ttf, struct ttfinfo *info) {
    int format;
    int gc;
    int i, j, ind, extras;
    int len;
    unsigned short *indexes;
    char **names, *name;

    fseek(ttf,info->postscript_start,SEEK_SET);
    format = getlong(ttf);
    getlong(ttf);
    getlong(ttf);
    getlong(ttf);
    getlong(ttf);
    getlong(ttf);
    getlong(ttf);
    getlong(ttf);
    if ( format==0x00020000 ) {
	gc = getushort(ttf);
	indexes = calloc(65536,sizeof(unsigned short));
	names = calloc(info->glyph_cnt,sizeof(char *));
	extras = 0;
	for ( i=0; i<gc; ++i ) {
	    ind = getushort(ttf);
	    if ( ind>=258 ) ++extras;
	    indexes[ind] = i;
	}
	for ( i=0; i<258; ++i ) if ( indexes[i]!=0 )
	    names[indexes[i]] = standardnames[i];
	for ( i=258; i<258+extras; ++i ) {
	    if ( indexes[i]==0 )
	break;
	    len = getc(ttf);
	    name = malloc(len+1);
	    for ( j=0; j<len; ++j )
		name[j] = getc(ttf);
	    name[j] = '\0';
	    names[indexes[i]] = name;
	}
	if ( names[0]==NULL )
	    names[0] = standardnames[0];
	free(indexes);
	info->glyph_names = names;
    }
}

static struct dup *makedup(int glyph, int uni, struct dup *prev) {
    struct dup *d = calloc(1,sizeof(struct dup));

    d->glyph = glyph;
    d->enc = uni;
    d->prev = prev;
return( d );
}

static void readttfencodings(FILE *ttf,struct ttfinfo *info) {
    int i,j;
    int nencs, version;
    int enc = 0;
    int platform, specific;
    int offset, encoff;
    int format, len;
    const unsigned short *table;
    int segCount;
    unsigned short *endchars, *startchars, *delta, *rangeOffset, *glyphs;
    int index;

    fseek(ttf,info->encoding_start,SEEK_SET);
    version = getushort(ttf);
    nencs = getushort(ttf);
    if ( version!=0 && nencs==0 )
	nencs = version;		/* Sometimes they are backwards */
    for ( i=0; i<nencs; ++i ) {
	platform = getushort(ttf);
	specific = getushort(ttf);
	offset = getlong(ttf);
	if ( platform==3 && specific==1 ) {
	    enc = 1;
	    encoff = offset;
	} else if ( platform==1 && specific==0 && enc!=1 ) {
	    enc = 2;
	    encoff = offset;
	}
    }
    if ( enc!=0 ) {
	info->glyph_unicode = calloc(info->glyph_cnt,sizeof(unsigned short));
	fseek(ttf,info->encoding_start+encoff,SEEK_SET);
	format = getushort(ttf);
	len = getushort(ttf);
	/* version = */ getushort(ttf);
	if ( format==0 ) {
	    table = unicode_from_mac;
	    for ( i=0; i<256; ++i ) {
		info->glyph_unicode[i] = table[i];
		if ( i<32 && table[i]==0 )
		    info->glyph_unicode[i] = i;
	    }
	} else if ( format==4 ) {
	    segCount = getushort(ttf)/2;
	    /* searchRange = */ getushort(ttf);
	    /* entrySelector = */ getushort(ttf);
	    /* rangeShift = */ getushort(ttf);
	    endchars = malloc(segCount*sizeof(unsigned short));
	    for ( i=0; i<segCount; ++i )
		endchars[i] = getushort(ttf);
	    if ( getushort(ttf)!=0 )
		fprintf(stderr,"Expected 0 in true type font");
	    startchars = malloc(segCount*sizeof(unsigned short));
	    for ( i=0; i<segCount; ++i )
		startchars[i] = getushort(ttf);
	    delta = malloc(segCount*sizeof(unsigned short));
	    for ( i=0; i<segCount; ++i )
		delta[i] = getushort(ttf);
	    rangeOffset = malloc(segCount*sizeof(unsigned short));
	    for ( i=0; i<segCount; ++i )
		rangeOffset[i] = getushort(ttf);
	    len -= 8*sizeof(unsigned short) +
		    4*segCount*sizeof(unsigned short);
	    /* that's the amount of space left in the subtable and it must */
	    /*  be filled with glyphIDs */
	    glyphs = malloc(len);
	    for ( i=0; i<len/2; ++i )
		glyphs[i] = getushort(ttf);
	    for ( i=0; i<segCount; ++i ) {
		if ( rangeOffset[i]==0 ) {
		    /* It's ok for a glyph to have several different encodings*/
		    /* "A" might work for "Alpha" for example */
		    for ( j=startchars[i]; j<=endchars[i]; ++j )
			if ( info->glyph_unicode[(unsigned short) (j+delta[i])]!=0 )
			    info->dups = makedup((unsigned short) (j+delta[i]),j,info->dups);
			else
			    info->glyph_unicode[(unsigned short) (j+delta[i])] = j;
		} else if ( rangeOffset[i]!=0xffff ) {
		    /* It isn't explicitly mentioned by a rangeOffset of 0xffff*/
		    /*  means no glyph */
		    for ( j=startchars[i]; j<=endchars[i]; ++j ) {
			index = glyphs[ (i-segCount+rangeOffset[i]/2) +
					    j-startchars[i] ];
			if ( index!=0 ) {
			    index = (unsigned short) (index+delta[i]);
			    if ( info->glyph_unicode[index]!=0 )
				info->dups = makedup(index,j,info->dups);
			    else
				info->glyph_unicode[index] = j;
			}
		    }
		}
	    }
	    free(glyphs);
	    free(rangeOffset);
	    free(delta);
	    free(startchars);
	    free(endchars);
	} else if ( format==6 ) {
	    /* Apple's unicode format */
	    int first, count;
	    first = getushort(ttf);
	    count = getushort(ttf);
	    for ( i=0; i<count; ++i )
		info->glyph_unicode[getushort(ttf)] = first+i;
	} else if ( format==2 ) {
	    fprintf(stderr,"I don't support mixed 8/16 bit characters (no shit jis for me)");
	} else if ( format==8 ) {
	    fprintf(stderr,"I don't support mixed 16/32 bit characters (no unicode surogates)");
	} else if ( format==10 || format==12 ) {
	    fprintf(stderr,"I don't support 32 bit characters");
	}
    }
}

static void readttflocations(FILE *ttf,struct ttfinfo *info) {
    int i;
    info->glyph_offsets = malloc((info->glyph_cnt+1)*sizeof(unsigned int));

    /* First we read all the locations. This might not be needed, they may */
    /*  just follow one another, but nothing I've noticed says that so let's */
    /*  be careful */
    fseek(ttf,info->glyphlocations_start,SEEK_SET);
    if ( info->index_to_loc_is_long ) {
	for ( i=0; i<=info->glyph_cnt ; ++i )
	    info->glyph_offsets[i] = getlong(ttf);
    } else {
	for ( i=0; i<=info->glyph_cnt ; ++i )
	    info->glyph_offsets[i] = 2*getushort(ttf);
    }
}

static void readttfwidths(FILE *ttf,struct ttfinfo *info) {
    int i,j;

    fseek(ttf,info->hmetrics_start,SEEK_SET);
    info->glyph_widths = calloc(info->glyph_cnt,sizeof(int));
    for ( i=0; i<info->width_cnt && i<info->glyph_cnt; ++i ) {
	info->glyph_widths[i] = getushort(ttf);
	/* lsb = */ getushort(ttf);
    }
    for ( j=i; j<info->glyph_cnt; ++j )
	info->glyph_widths[j] = info->glyph_widths[i-1];
}

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

static int readttfheader(FILE *ttf, struct ttfinfo *info) {
    int i;
    int tag, checksum, offset, length, sr, es, rs;
    double version;

    version = getfixed(ttf);
    info->numtables = getushort(ttf);
    sr = getushort(ttf),
    es = getushort(ttf),
    rs = getushort(ttf);

    for ( i=0; i<info->numtables; ++i ) {
	tag = getlong(ttf);
	checksum = getlong(ttf);
	offset = getlong(ttf);
	length = getlong(ttf);
	switch ( tag ) {
	  case CHR('c','m','a','p'):
	    info->encoding_start = offset;
	  break;
	  case CHR('g','l','y','f'):
	    info->glyph_start = offset;
	  break;
	  case CHR('E','B','D','T'):
	    info->bitmapdata_start = offset;
	  break;
	  case CHR('E','B','L','T'):
	    info->bitmaploc_start = offset;
	  break;
	  case CHR('h','e','a','d'):
	    info->head_start = offset;
	    if ( length!=54 ) fprintf( stderr, "> ! Unexpected length for head table, was %d expected 54 ! <\n", length );
	  break;
	  case CHR('h','h','e','a'):
	    info->hhea_start = offset;
	    if ( length!=36 ) fprintf( stderr, "> ! Unexpected length for hhea table, was %d expected 36 ! <\n", length );
	  break;
	  case CHR('h','m','t','x'):
	    info->hmetrics_start = offset;
	  break;
	  case CHR('k','e','r','n'):
	    info->kern_start = offset;
	  break;
	  case CHR('l','o','c','a'):
	    info->glyphlocations_start = offset;
	    info->loca_length = length;
	    info->glyph_cnt = length/2-1;	/* we don't always have a maxp table. the minus one is because there is one extra entry to give the length of the last glyph */
	  break;
	  case CHR('m','a','x','p'):
	    info->maxp_start = offset;
	    if ( length!=32 ) fprintf( stderr, "> ! Unexpected length for maxp table, was %d expected 32 ! <\n", length );
	  break;
	  case CHR('n','a','m','e'):
	    info->copyright_start = offset;
	  break;
	  case CHR('p','o','s','t'):
	    info->postscript_start = offset;
	    info->postscript_len = length;
	  break;
	  case CHR('O','S','/','2'):
	    info->os2_start = offset;
	    info->os2_len = length;
	    if ( length!=78 && length!=86 )
		fprintf( stderr, "> ! Unexpected length for OS/2 table, was %d expected either 78 or 86 ! <\n", length );
	  break;
	  case CHR('p','r','e','p'):
	    info->prep_start = offset;
	    info->prep_len = length;
	  break;
	  case CHR('f','p','g','m'):
	    info->fpgm_start = offset;
	    info->fpgm_len = length;
	  break;
	}
    }
    if ( info->encoding_start==0 || info->glyph_start==0 || info->head_start==0 ||
	    info->hhea_start==0 || info->hmetrics_start==0 || info->glyphlocations_start == 0 ||
	    info->maxp_start==0 || info->copyright_start==0 || info->postscript_start==0 ||
	    info->os2_start==0 ) {
	fprintf( stderr, "Required table(s) missing: " );
	if ( info->encoding_start==0 ) fprintf(stderr,"cmap ");
	if ( info->glyph_start==0 ) fprintf(stderr,"glyf ");
	if ( info->head_start==0 ) fprintf(stderr,"head ");
	if ( info->hhea_start==0 ) fprintf(stderr,"hhea ");
	if ( info->hmetrics_start==0 ) fprintf(stderr,"hmtx ");
	if ( info->glyphlocations_start==0 ) fprintf(stderr,"loca ");
	if ( info->maxp_start==0 ) fprintf(stderr,"maxp ");
	if ( info->copyright_start==0 ) fprintf(stderr,"name ");
	if ( info->postscript_start==0 ) fprintf(stderr,"post ");
	if ( info->os2_start==0 ) fprintf(stderr,"OS/2");
	fprintf(stderr,"\n");
	exit(1);
    }
    readttfhead(ttf,info);
    readttfhhea(ttf,info);
    readttfpost(ttf,info);
    readttfencodings(ttf,info);
    readttflocations(ttf,info);
    readttfwidths(ttf,info);
return( 1 );
}

static char *glyph2name(int glyph, struct ttfinfo *info) {
    char buffer[20];

    if ( info->glyph_names!=NULL )
	if ( info->glyph_names[glyph]!=NULL )
return( info->glyph_names[glyph] );
    sprintf( buffer,"glyph%d", glyph);
return( strdup(buffer));
}

static int name2glyph(char *name, struct ttfinfo *info) {
    int i;

    if ( info->glyph_names!=NULL ) {
	for ( i=0; i<info->glyph_cnt; ++i ) if ( info->glyph_names[i]!=NULL )
	    if ( strcmp(name,info->glyph_names[i])==0 )
return( i );
    }
    fprintf( stderr, "Could not find a glyph named %s\n", name );
    exit(1);
return( -1 );
}

static void DumpEpsHeader(FILE *eps, struct ttfinfo *info, int glyph,
	double xmin, double ymin, double xmax, double ymax ) {
    time_t now;
    struct tm *tm;

    fprintf( eps, "%%!PS-Adobe-3.0 EPSF-3.0\n" );
    fprintf( eps, "%%%%BoundingBox: %g %g %g %g\n", xmin, ymin, xmax, ymax );
    fprintf( eps, "%%%%Pages: 0\n" );
    fprintf( eps, "%%%%Title: Glyph %d ", glyph );
    if ( info->glyph_names!=NULL && info->glyph_names[glyph]!=NULL )
	fprintf( eps, " Name: %s", info->glyph_names[glyph]);
    if ( info->glyph_unicode!=NULL && info->glyph_unicode[glyph]!=0 )
	fprintf(eps, " Unicode %04x", info->glyph_unicode[glyph]);
    fprintf( eps, "\n" );
    fprintf( eps, "%%%%Creator: ttf2eps\n" );
    time(&now);
    tm = localtime(&now);
    fprintf( eps, "%%%%CreationDate: %d:%02d %d-%d-%d\n", tm->tm_hour, tm->tm_min,
	    tm->tm_mday, tm->tm_mon+1, 1900+tm->tm_year );
    fprintf( eps, "%%%%EndComments\n" );
    fprintf( eps, "%%%%EndProlog\n" );
    fprintf( eps, "%%Number of units in Em: %d\n", info->emsize );
    fprintf( eps, "%%Ascent: %d\n", info->ascent );
    fprintf( eps, "%%Descent: %d\n", info->descent );
    if ( info->glyph_widths!=NULL )
	fprintf( eps, "%%Character Width %d\n", info->glyph_widths[glyph]);
    if ( info->glyph_names!=NULL && info->glyph_names[glyph]!=NULL )
	fprintf( eps, "%%%%Page: \"%s\" 1\n", info->glyph_names[glyph] );
    else
	fprintf( eps, "%%%%Page: \"Glyph%d\" 1\n", glyph );
}

static void DoDumpGlyph(FILE *ttf, FILE *eps, struct ttfinfo *info, int glyph,
	double trans[6], int top);

#define _ARGS_ARE_WORDS	1
#define _ARGS_ARE_XY	2
#define _ROUND		4		/* to grid? What's that? */
#define _SCALE		8
#define _XY_SCALE	0x40
#define _MATRIX		0x80
#define _MORE		0x20
#define _INSTR		0x100
#define _MY_METRICS	0x200


#define _On_Curve	1
#define _X_Short	2
#define _Y_Short	4
#define _Repeat		8
#define _X_Same		0x10
#define _Y_Same		0x20

static void DumpCompositGlyph(FILE *ttf,FILE *eps,struct ttfinfo *info,double trans[6]) {
    int flags, arg1, arg2;
    int sub_glyph;
    int oldpos;
    double transform[6], restrans[6];

    do {
	flags = getushort(ttf);
	sub_glyph = getushort(ttf);
	transform[0] = transform[3] = 1;
	transform[1] = transform[2] = transform[4] = transform[5] = 0;
	if ( flags&_ARGS_ARE_WORDS ) {
	    arg1 = (short) getushort(ttf);
	    arg2 = (short) getushort(ttf);
	} else {
	    arg1 = (signed char) getc(ttf);
	    arg2 = (signed char) getc(ttf);
	}
	if ( flags & _ARGS_ARE_XY ) {
	    transform[4] = arg1;
	    transform[5] = arg2;
	} else
	    fprintf(stderr,"Don't know how to deal with !(flags&_ARGS_ARE_XY)" );
	if ( flags & _SCALE )
	    transform[0] = transform[3] = get2dot14(ttf);
	else if ( flags & _XY_SCALE ) {
	    transform[0] = get2dot14(ttf);
	    transform[3] = get2dot14(ttf);
	} else if ( flags & _MATRIX ) {
	    transform[0] = get2dot14(ttf);
	    transform[1] = get2dot14(ttf);
	    transform[2] = get2dot14(ttf);
	    transform[3] = get2dot14(ttf);
	}
	restrans[0] = transform[0]*trans[0] +
		transform[1]*trans[2];
	restrans[1] = transform[0]*trans[1] +
		transform[1]*trans[3];
	restrans[2] = transform[2]*trans[0] +
		transform[3]*trans[2];
	restrans[3] = transform[2]*trans[1] +
		transform[3]*trans[3];
	restrans[4] = transform[4]*trans[0] +
		transform[5]*trans[2] +
		trans[4];
	restrans[5] = transform[4]*trans[1] +
		transform[5]*trans[3] +
		trans[5];
	oldpos = ftell(ttf);
	DoDumpGlyph(ttf, eps, info, sub_glyph, restrans, 0);
	fseek(ttf,oldpos,SEEK_SET);
    } while ( flags&_MORE );
    /* I'm ignoring many things that I don't understand here */
    /* Instructions, USE_MY_METRICS, ROUND, what happens if ARGS AREN'T XY */
}

static void TransformPoint(BasePoint *bp,double trans[6]) {
    double x;
    x = trans[0]*bp->x+trans[2]*bp->y+trans[4];
    bp->y=trans[1]*bp->x+trans[3]*bp->y+trans[5];
    bp->x = x;
}

static void DumpSpline(FILE *eps, BasePoint *from, BasePoint *cp, BasePoint *to) {
    double b, c, d;
    BasePoint fn, tp;

    d = from->x;
    c = 2*cp->x - 2*from->x;
    b = to->x+from->x-2*cp->x;
    fn.x = d+c/3;
    tp.x = fn.x + (c+b)/3;

    d = from->y;
    c = 2*cp->y - 2*from->y;
    b = to->y+from->y-2*cp->y;
    fn.y = d+c/3;
    tp.y = fn.y + (c+b)/3;

    fprintf( eps, " %g %g %g %g %g %g curveto\n", fn.x, fn.y, tp.x, tp.y,
	    to->x, to->y);
}

static void DumpLine(FILE *eps,BasePoint *from,BasePoint *to ) {
    fprintf( eps, " %g %g lineto\n", to->x, to->y );
}

static void DumpMoveTo(FILE *eps,BasePoint *to ) {
    fprintf( eps, " %g %g moveto\n", to->x, to->y );
}

static void DumpSimpleGlyph(FILE *ttf,FILE *eps,struct ttfinfo *info,
	double trans[6],int path_cnt) {
    int i, path, start, last_off;
    unsigned short *endpt = malloc((path_cnt+1)*sizeof(unsigned short));
    BasePoint mid, last, begin;
    char *flags;
    BasePoint *pts;
    int j, tot, len;
    int last_pos, first;

    for ( i=0; i<path_cnt; ++i )
	endpt[i] = getushort(ttf);
    tot = endpt[path_cnt-1]+1;
    pts = malloc(tot*sizeof(BasePoint));

    /* ignore instructions */
    len = getushort(ttf);
    for ( i=0; i<len; ++i )
	getc(ttf);

    flags = malloc(tot);
    for ( i=0; i<tot; ++i ) {
	flags[i] = getc(ttf);
	if ( flags[i]&_Repeat ) {
	    int cnt = getc(ttf);
	    for ( j=0; j<cnt; ++j )
		flags[i+j+1] = flags[i];
	    i += cnt;
	}
    }
    if ( i>tot )
	fprintf(stderr,"Flag count is wrong (or total is): %d %d", i, tot );

    last_pos = 0;
    for ( i=0; i<tot; ++i ) {
	if ( flags[i]&_X_Short ) {
	    int off = getc(ttf);
	    if ( !(flags[i]&_X_Same ) )
		off = -off;
	    pts[i].x = last_pos + off;
	} else if ( flags[i]&_X_Same )
	    pts[i].x = last_pos;
	else
	    pts[i].x = last_pos + (short) getushort(ttf);
	last_pos = pts[i].x;
    }

    last_pos = 0;
    for ( i=0; i<tot; ++i ) {
	if ( flags[i]&_Y_Short ) {
	    int off = getc(ttf);
	    if ( !(flags[i]&_Y_Same ) )
		off = -off;
	    pts[i].y = last_pos + off;
	} else if ( flags[i]&_Y_Same )
	    pts[i].y = last_pos;
	else
	    pts[i].y = last_pos + (short) getushort(ttf);
	last_pos = pts[i].y;
    }

    for ( i=0; i<tot; ++i )
	TransformPoint(&pts[i],trans);

    for ( path=i=0; path<path_cnt; ++path ) {
	start = i;
	first = 1;
	while ( i<=endpt[path] ) {
	    if ( flags[i]&_On_Curve ) {
		if ( first )
		    DumpMoveTo(eps,&pts[i]);
		else if ( last_off )
		    DumpSpline(eps,&last,&pts[i-1],&pts[i]);
		else
		    DumpLine(eps,&last,&pts[i]);
		first = 0;
		last = pts[i];
		last_off = 0;
	    } else if ( last_off ) {
		/* two off curve points get a third on curve point created */
		/* half-way between them. Now isn't that special */
		mid.x = (pts[i].x+pts[i-1].x)/2;
		mid.y = (pts[i].y+pts[i-1].y)/2;
		if ( first )
		    DumpMoveTo(eps,&mid);
		else
		    DumpSpline(eps,&last,&pts[i-1],&mid);
		first = 0;
		last = mid;
		/* last_off continues to be true */
	    } else {
		last_off = 1;
	    }
	    ++i;
	}
	if ( !(flags[start]&_On_Curve) && !(flags[i-1]&_On_Curve) ) {
	    mid.x = (pts[start].x+pts[i-1].x)/2;
	    mid.y = (pts[start].y+pts[i-1].y)/2;
	    DumpSpline(eps,&last,&pts[i-1],&mid);
	    if ( flags[start+1]&_On_Curve )
		begin = pts[start+1];
	    else {
		begin.x = (pts[start].x+pts[start+1].x)/2;
		begin.y = (pts[start].y+pts[start+1].y)/2;
	    }
	    DumpSpline(eps,&mid,&pts[start],&begin);
	} else if ( !(flags[i-1]&_On_Curve))
	    DumpSpline(eps,&last,&pts[i-1],&pts[start]);
	else if ( !(flags[start]&_On_Curve) )
	    DumpSpline(eps,&last,&pts[start],&pts[start+1]);
	fprintf( eps,"closepath\n" );
    }
    free(endpt);
    free(flags);
    free(pts);
    fprintf( eps,"fill\n" );
}

static void DoDumpGlyph(FILE *ttf, FILE *eps, struct ttfinfo *info, int glyph,
	double trans[6], int top) {
    int start, end;
    int path_cnt=-1;
    double xmin, xmax, ymin, ymax;

    start = info->glyph_offsets[glyph];
    end = info->glyph_offsets[glyph+1];
    xmin = xmax = ymin = ymax = 0;
    if ( start!=end ) {
	fseek(ttf,info->glyph_start+start,SEEK_SET);
	path_cnt = (short) getushort(ttf);
	xmin = (short) getushort(ttf);
	ymin = (short) getushort(ttf);
	xmax = (short) getushort(ttf);
	ymax = (short) getushort(ttf);
    }
    if ( top ) {
	DumpEpsHeader(eps,info,glyph,xmin,ymin,xmax,ymax);
	fprintf( eps,"newpath\n" );
    } else {
	fprintf( eps, "%% Glyph %d ", glyph );
	if ( info->glyph_names!=NULL && info->glyph_names[glyph]!=NULL )
	    fprintf( eps, " Name: %s", info->glyph_names[glyph]);
	if ( info->glyph_unicode!=NULL && info->glyph_unicode[glyph]!=0 )
	    fprintf(eps, " Unicode %04x", info->glyph_unicode[glyph]);
    }
    if ( start==end )
return;
    if ( path_cnt<0 )
	DumpCompositGlyph(ttf,eps,info,trans);
    else
	DumpSimpleGlyph(ttf,eps,info,trans,path_cnt);
}

static void DumpGlyph(FILE *ttf, struct ttfinfo *info, int glyph) {
    char *name = glyph2name(glyph,info);
    char buffer[300];
    FILE *eps;
    double trans[6];

    sprintf(buffer,"%s.eps", name);
    eps = fopen(buffer,"w");
    if ( eps==NULL ) {
	fprintf( stderr, "Could not open %s for writing\n", buffer );
	exit(1);
    }
    trans[0] = trans[3] = 1;
    trans[1] = trans[2] = trans[4] = trans[5] = 0;
    DoDumpGlyph(ttf, eps, info, glyph, trans, 1);
    fprintf( eps, "%%%%EOF\n" );
    fclose(eps);
}

int main(int argc, char **argv) {
    FILE *ttf;
    struct ttfinfo info;
    char *filename=NULL;
    int doall=0, i, val;
    struct spec { enum { is_glyph, is_unicode, is_name } type; char *arg; struct spec *next; } *specs=NULL, *temp;
    char *end;
    struct dup *dup;

    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]!='-' ) {
	    if ( filename == NULL )
		filename = argv[i];
	    else {
		fprintf( stderr, "Must have exactly one truetype filename argument\n" );
  exit(1);
	    }
	} else if ( strcmp(argv[i],"-all")==0 )
	    doall=1;
	else if ( strcmp(argv[i],"-glyph")==0 ||
		  strcmp(argv[i],"-unicode")==0 ||
		  strcmp(argv[i],"-uni")==0 ||
		  strcmp(argv[i],"-name")==0 ) {
	    temp = calloc(1,sizeof(struct spec));
	    temp->type = strcmp(argv[i],"-glyph")==0? is_glyph :
			 strcmp(argv[i],"-name")==0? is_name : is_unicode;
	    temp->arg = argv[++i];
	    temp->next = specs;
	    specs = temp;
	} else {
	    fprintf( stderr, "Unknown argument %s\n", argv[i]);
	    fprintf( stderr, "Usage: %s [-all] {-glyph num | -name name | -unicode hex | -uni hex} truetypefile\n", argv[0] );
	    exit(1);
	}
    }
    if ( filename==NULL ) {
	fprintf( stderr, "Must have exactly one truetype filename argument\n" );
return( 1 );
    }
    ttf = fopen(filename,"r");
    if ( ttf==NULL ) {
	fprintf( stderr, "Can't open %s\n", filename);
return( 1 );
    }

    memset(&info,'\0',sizeof(info));
    readttfheader(ttf,&info);
    if ( doall )
	for ( i=0; i<info.glyph_cnt; ++i )
	    DumpGlyph(ttf,&info,i);
    else {
	for ( temp=specs; temp!=NULL; temp=temp->next ) {
	    if ( temp->type==is_glyph ) {
		val = strtol(temp->arg,&end,10);
		if ( *end!='\0' || val>=info.glyph_cnt || val<0 ) {
		    fprintf( stderr, "Bad glyph index %s\n", temp->arg);
		    exit(1);
		}
	    } else if ( temp->type==is_unicode ) {
		val = strtol(temp->arg,&end,16);
		if ( *end!='\0' || val<0 || val>65535 || info.glyph_unicode==NULL ) {
		    fprintf( stderr, "Bad Unicode value %s\n", temp->arg);
		    exit(1);
		}
		for ( i=0; i<info.glyph_cnt && info.glyph_unicode[i]!=val; ++i );
		if ( i==info.glyph_cnt ) {
		    for ( dup = info.dups; dup!=NULL && dup->enc!=val; dup = dup->prev );
		    if ( dup==NULL ) {
			fprintf( stderr, "This Unicode value %04X not encoded in this font\n", val );
			exit(1);
		    }
		    i = dup->glyph;
		}
		val = i;
	    } else if ( temp->type==is_name ) {
		val = name2glyph(temp->arg,&info);
	    }
	    DumpGlyph(ttf,&info,val);
	}
    }
return( 0 );
}
