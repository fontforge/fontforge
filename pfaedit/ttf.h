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

/* Some glyphs have multiple encodings ("A" might be used for Alpha and Cyrillic A) */
struct dup {
    SplineChar *sc;
    int enc;
    struct dup *prev;
};

struct ttfinfo {
    int emsize;			/* ascent + descent? from the head table */
    int ascent, descent;	/* from the hhea table */
				/* not the usWinAscent from the OS/2 table */
    int width_cnt;		/* from the hhea table, in the hmtx table */
    int glyph_cnt;		/* from maxp table (or cff table) */
    unsigned int index_to_loc_is_long:1;	/* in head table */
    unsigned int is_ttc:1;			/* Is it a font collection? */
    /* Mac fonts platform=0/1, platform specific enc id, roman=0, english is lang code 0 */
    /* iso platform=2, platform specific enc id, latin1=0/2, no language */
    /* microsoft platform=3, platform specific enc id, 1, english is lang code 0x??09 */
    char *copyright;		/* from the name table, nameid=0 */
    char *familyname;		/* nameid=1 */
    char *fullname;		/* nameid=4 */
    char *weight;
    char *version;		/* nameid=5 */
    char *fontname;		/* postscript font name, nameid=6 */
    char *xuid;			/* Only for open type cff fonts */
    real italicAngle;		/* from post table */
    int upos, uwidth;		/* underline pos, width from post table */
    int fstype;
    struct psdict *private;	/* Only for open type cff fonts */
    enum charset encoding_name;/* from cmap */
    struct pfminfo pfminfo;
    struct ttflangname *names;
    SplineChar **chars;		/* from all over, glyf table for contours */
    				/* 		  cmap table for encodings */
			        /*		  hmtx table for widths */
			        /*		  post table for names */
			        /* Or from	  CFF  table for everything in opentype */
    BDFFont *bitmaps;
    char *cidregistry, *ordering;
    int supplement;
    real cidfontversion;
    int subfontcnt;
    SplineFont **subfonts;
    char *inuse;		/* What glyphs are used by this font in the ttc */

    int numtables;
    		/* CFF  */
    int cff_start;		/* Offset from sof to start of postscript compact font format */
    int cff_length;
    		/* cmap */
    int encoding_start;		/* Offset from sof to start of encoding table */
		/* glyf */
    int glyph_start;		/* Offset from sof to start of glyph table */
		/* GSUB */
    int gsub_start;		/* Offset from sof to start of GSUB table */
		/* EBDT, bdat */
    int bitmapdata_start;	/* Offset to start of bitmap data */
		/* EBLT, bloc */
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
		/* OS/2 */
    int os2_start;

    struct dup *dups;
    unsigned int one_of_many: 1;	/* A TTCF file, or a opentype font with multiple fonts */
    unsigned int obscomplain: 1;	/* We've complained about obsolete format 3 in EBDT table */
    unsigned int cmpcomplain: 1;	/* We've complained about compressed format 4 in EBDT */
    unsigned int unkcomplain: 1;	/* We've complained about unknown formats in EBDT */
    unsigned int comcomplain: 1;	/* We've complained about composit formats in EBDT */
    unsigned int onlystrikes: 1;	/* Only read in the bitmaps, not the outlines */
    unsigned int onlyonestrike: 1;	/* Only read in one bitmap (strike) */
};

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))
