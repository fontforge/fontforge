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
 
#include "psfont.h"		/* for struct fddata */

/* Some glyphs have multiple encodings ("A" might be used for Alpha and Cyrillic A) */
struct dup {
    SplineChar *sc;
    int enc;
    int uni;
    struct dup *prev;
};

struct ttfinfo {
    int emsize;			/* ascent + descent? from the head table */
    int ascent, descent;	/* from the hhea table */
				/* not the usWinAscent from the OS/2 table */
    int vertical_origin;	/* if vmetrics are present */
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
		/* GPOS */
    int gpos_start;		/* Offset from sof to start of GPOS table */
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
		/* vhea */
    int vhea_start;
		/* vmtx */
    int vmetrics_start;
		/* VORG */
    int vorg_start;

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

struct tabdir {
    int32 version;	/* 0x00010000 */
    uint16 numtab;
    uint16 searchRange;	/* (Max power of 2 <= numtab) *16 */
    uint16 entrySel;	/* Log2(Max power of 2 <= numtab ) */
    uint16 rangeShift;	/* numtab*16 - searchRange */
    struct taboff {
	uint32 tag;	/* Table name */
	uint32 checksum;/* for table */
	uint32 offset;	/* to start of table in file */
	uint32 length;
    } tabs[20];		/* room for all the above tables */
};

struct glyphhead {
    int16 numContours;
    int16 xmin;
    int16 ymin;
    int16 xmax;
    int16 ymax;
};

struct head {
    int32 version;	/* 0x00010000 */
    int32 revision;	/* 0 */
    uint32 checksumAdj;	/* set to 0, sum entire font, store 0xb1b0afba-sum */
    uint32 magicNum;	/* 0x5f0f3cf5 */
    uint16 flags;	/* 1 */
    uint16 emunits;	/* sf->ascent+sf->descent */
    int32 createtime[2];/* number of seconds since 1904 */
    int32 modtime[2];
    int16 xmin;		/* min for entire font */
    int16 ymin;
    int16 xmax;
    int16 ymax;
    uint16 macstyle;	/* 1=>Bold, 2=>Italic */
    uint16 lowestreadable;	/* size in pixels. Say about 10? */
    int16 dirhint;	/* 0=>mixed directional characters, */
    int16 locais32;	/* is the location table 32bits or 16, 0=>16, 1=>32 */
    int16 glyphformat;	/* 0 */
    uint16 mbz;		/* padding */
};

struct hhead {
    int32 version;	/* 0x00010000 */
    int16 ascender;	/* sf->ascender */
    int16 descender;	/* -sf->descender */
    int16 linegap;	/* 0 */
    int16 maxwidth;	/* of all characters */
    int16 minlsb;	/* How is this different from xmin above? */
    int16 minrsb;
    int16 maxextent;	/* How is this different from xmax above? */
    int16 caretSlopeRise;/* Uh... let's say 1? */
    int16 caretSlopeRun;/* Uh... let's say 0 */
    int16 mbz[5];
    int16 metricformat;	/* 0 */
    uint16 numMetrics;	/* just set to glyph count */
};

struct hmtx {
    uint16 width;	/* NOTE: TTF only allows positive widths!!! */
    int16 lsb;
};

struct kern {
    uint16 version;	/* 0 */
    uint16 ntab;	/* 1, number of subtables */
    /* first (and only) subtable */
    uint16 stversion;	/* 0 */
    uint16 length;	/* length of subtable beginning at &stversion */
    uint16 coverage;	/* 1, (set of flags&format) */
    uint16 nPairs;	/* number of kern pairs */
    uint16 searchRange;	/* (Max power of 2 <= nPairs) *6 */
    uint16 entrySel;	/* Log2(Max power of 2 <= nPairs ) */
    uint16 rangeShift;	/* numtab*6 - searchRange */
    struct kp {
	uint16 left;	/* left glyph num */
	uint16 right;	/* right glyph num */
	/* table is ordered by these two above treated as uint32 */
	int16 offset;	/* kern amount */
    } *kerns;		/* Array should be nPairs big */
};

struct maxp {
    int32 version;	/* 0x00010000 */
    uint16 numGlyphs;
    uint16 maxPoints;	/* max number of points in a simple glyph */
    uint16 maxContours;	/* max number of paths in a simple glyph */
    uint16 maxCompositPts;
    uint16 maxCompositCtrs;
    uint16 maxZones;	/* 1 */
    uint16 maxTwilightPts;	/* 0 */
    uint16 maxStorage;	/* 0 */
    uint16 maxFDEFs;	/* 0 */
    uint16 maxIDEFs;	/* 0 */
    uint16 maxStack;	/* 0 */
    uint16 maxglyphInstr;/* 0 */
    uint16 maxnumcomponents;	/* Maximum number of refs in any composit */
    uint16 maxcomponentdepth;	/* 0 (if no composits), 1 else */
    uint16 mbz;		/* pad out to a 4byte boundary */
};

struct nametab {
    uint16 format;	/* 0 */
    uint16 numrec;	/* 1 */
    uint16 startOfStrings;	/* offset from start of table to start of strings */
    struct namerec {
	uint16 platform;	/* 3 => MS */
	uint16 specific;	/* 1 */
	uint16 language;	/* 0x0409 */
	uint16 nameid;		/* 0=>copyright, 1=>family, 2=>weight, 4=>fullname */
				/*  5=>version, 6=>postscript name */
	uint16 strlen;
	uint16 stroff;
    } nr[6];
};

struct os2 {
    uint16 version;	/* 1 */
    int16 avgCharWid;	/* average width of a-z and space, if no lower case, then average all chars */
    uint16 weightClass;	/* 100=>thin, 200=>extra-light, 300=>light, 400=>normal, */
			/* 500=>Medium, 600=>semi-bold, 700=>bold, 800=>extra-bold, */
			/* 900=>black */
    uint16 widthClass;	/* 75=>condensed, 100, 125=>expanded */
    int16 fstype;	/* 0x0008 => allow embedded editing */
    int16 ysubXSize;	/* emsize/5 */
    int16 ysubYSize;	/* emsize/5 */
    int16 ysubXOff;	/* 0 */
    int16 ysubYOff;	/* emsize/5 */
    int16 ysupXSize;	/* emsize/5 */
    int16 ysupYSize;	/* emsize/5 */
    int16 ysupXOff;	/* 0 */
    int16 ysupYOff;	/* emsize/5 */
    int16 yStrikeoutSize;	/* 102/2048 *emsize */
    int16 yStrikeoutPos;	/* 530/2048 *emsize */
    int16 sFamilyClass;	/* ??? 0 */
	/* high order byte is the "class", low order byte the sub class */
	/* class = 0 => no classification */
	/* class = 1 => old style serifs */
	/*	subclass 0, no class; 1 ibm rounded; 2 garalde; 3 venetian; 4 mod venitian; 5 dutch modern; 6 dutch trad; 7 contemporary; 8 caligraphic; 15 misc */
	/* class = 2 => transitional serifs */
	/*	subclass 0, no class; 1 drect line; 2 script; 15 misc */
	/* class = 3 => modern serifs */
	/*	subclass: 1, italian; 2, script */
	/* class = 4 => clarendon serifs */
	/*	subclass: 1, clarendon; 2, modern; 3 trad; 4 newspaper; 5 stub; 6 monotone; 7 typewriter */
	/* class = 5 => slab serifs */
	/*	subclass: 1, monotone; 2, humanist; 3 geometric; 4 swiss; 5 typewriter */
	/* class = 7 => freeform serifs */
	/*	subclass: 1, modern */
	/* class = 8 => sans serif */
	/*	subclass: 1, ibm neogrotesque; 2 humanist; 3 low-x rounded; 4 high-x rounded; 5 neo-grotesque; 6 mod neo-grot; 9 typewriter; 10 matrix */
	/* class = 9 => ornamentals */
	/*	subclass: 1, engraver; 2 black letter; 3 decorative; 4 3D */
	/* class = 10 => scripts */
	/*	subclass: 1, uncial; 2 brush joined; 3 formal joined; 4 monotone joined; 5 calligraphic; 6 brush unjoined; 7 formal unjoined; 8 monotone unjoined */
	/* class = 12 => symbolic */
	/*	subclass: 3 mixed serif; 6 old style serif; 7 neo-grotesque sans; */
    char panose[10];	/* can be set to zero */
    uint32 unicoderange[4];	
	/* 1<<0=>ascii, 1<<1 => latin1, 2=>100-17f, 3=>180-24f, 4=>250-2af */
	/* 5=> 2b0-2ff, 6=>300-36f, ... */
    char achVendID[4];	/* can be zero */
    uint16 fsSel;	/* 1=> italic, 32=>bold, 64 => regular */
    uint16 firstcharindex; /* minimum unicode encoding */
    uint16 lastcharindex;  /* maximum unicode encoding */
    uint16 ascender;	/* font ascender height (not ascent) */
    uint16 descender;	/* font descender height */
    uint16 linegap;	/* 0 */
    uint16 winascent;	/* ymax */
    uint16 windescent;	/* ymin */
    uint32 ulCodePage[2];
	/* 1<<0 => latin1, 1<<1=>latin2, cyrillic, greek, turkish, hebrew, arabic */
	/* 1<<30 => mac, 1<<31 => symbol */
    /* OTF stuff (version 2 of OS/2) */
    short xHeight;
    short capHeight;
    short defChar;
    short breakChar;
    short maxContext;
};

struct post {
    int32 formattype;		/* 0x00020000 */
    int32 italicAngle;		/* in fixed format */
    int16 upos;
    int16 uwidth;
    uint32 isfixed;
    uint32 minmem42;
    uint32 maxmem42;
    uint32 minmem1;
    uint32 maxmem1;
    uint16 numglyphs;
    uint16 glyphnameindex[1];
};

struct glyphinfo {
    struct maxp *maxp;		/* this one is given to dumpglyphs, rest blank */
    uint32 *loca;
    FILE *glyphs;
    FILE *hmtx;
    int hmtxlen;
    FILE *vmtx;
    int vmtxlen;
    int next_glyph;
    int glyph_len;
    short *cvt;
    int cvtmax;
    int cvtcur;
    int xmin, ymin, xmax, ymax;
    BlueData bd;
    int strikecnt;		/* number of bitmaps to dump */
    int fudge;
    int lasthwidth, lastvwidth;	/* encoding of last glyph for which we generate a full metrics entry */
    int hfullcnt, vfullcnt;
};

struct vorg {
    ushort majorVersion;		/* 1 */
    ushort minorVersion;		/* 0 */
    short defaultVertOriginY;	/* Y coord of default vertical origin in the design coordinate system */
    ushort numVertOriginYMetrics;	/* exceptions to the above, elements in following array */
#if 0
    struct {
	ushort glyphindex;		/* ordered */
	short vertOrigin;
    } origins[];
#endif
};

struct alltabs {
    struct tabdir tabdir;
    struct head head;
    struct hhead hhead;
    struct hhead vhead;
    struct maxp maxp;
    struct os2 os2;
    struct vorg vorg;
    FILE *loca;
    int localen;
    FILE *name;
    int namelen;
    FILE *post;
    int postlen;
    FILE *gpos;			/* Used instead of kern for opentype */
    int gposlen;
    FILE *gsub;			/* Used for ligatures */
    int gsublen;
    FILE *kern;
    int kernlen;
    FILE *cmap;
    int cmaplen;
    FILE *headf;
    int headlen;
    FILE *hheadf;
    int hheadlen;
    FILE *maxpf;
    int maxplen;
    FILE *os2f;
    int os2len;
    FILE *cvtf;
    int cvtlen;
    FILE *vheadf;
    int vheadlen;
    FILE *vorgf;
    int vorglen;
    FILE *cfff;
    int cfflen;
    FILE *sidf;
    FILE *sidh;
    FILE *charset;
    FILE *encoding;
    FILE *private;
    FILE *charstrings;
    FILE *fdselect;
    FILE *fdarray;
    FILE *bdat;		/* might be EBDT */
    int bdatlen;
    FILE *bloc;		/* might be EBLC */
    int bloclen;
    int defwid, nomwid;
    int sidcnt;
    int lenpos;
    int privatelen;
    unsigned int sidlongoffset: 1;
    unsigned int cfflongoffset: 1;
    unsigned int msbitmaps: 1;
    unsigned int error: 1;
    struct glyphinfo gi;
    int isfixed;
    struct fd2data *fds;
};

struct subhead { uint16 first, cnt, delta, rangeoff; };	/* a sub header in 8/16 cmap table */
