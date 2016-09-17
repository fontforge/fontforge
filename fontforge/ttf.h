/* Copyright (C) 2001-2012 by George Williams */
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

#ifndef FONTFORGE_TTF_H
#define FONTFORGE_TTF_H

#include "psfont.h"		/* for struct fddata */

#define MAC_DELETED_GLYPH_NAME	"<Delete>"

/* Some glyphs have multiple encodings ("A" might be used for Alpha and Cyrillic A) */
struct dup {
    SplineChar *sc;
    int enc;
    int uni;
    struct dup *prev;
};

struct taxis {
    uint32 tag;
    real min, def, max;	/* in user design space */
    int nameid;
    int paircount;
    real *mapfrom;		/* after conversion from [-1,1] */
    real *mapto;		/* secondary conversiont to [-1,1] */
};

struct tinstance {
    int nameid;
    real *coords;	/* Location along axes array[axis_count] */
};

struct tuples {
    real *coords;	/* Location along axes array[axis_count] */
    SplineChar **chars;	/* Varied glyphs, array parallels one in info */
    struct ttf_table *cvt;
    KernClass *khead, *klast, *vkhead, *vklast; /* Varied kern classes */
};

struct variations {
    int axis_count;
    struct taxis *axes;		/* Array of axis_count entries */
    int instance_count;	/* Not master designs, but named interpolations in design space */
    struct tinstance *instances;
    int tuple_count;
    struct tuples *tuples;
};

enum gsub_inusetype { git_normal, git_justinuse, git_findnames };
	
struct macidname {
    int id;
    struct macname *head, *last;
    struct macidname *next;
};

struct savetab {
    uint32 tag;
    uint32 offset;
    int len;
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
    unsigned int is_onebyte:1;			/* Is it a one byte encoding? */
    unsigned int twobytesymbol:1;		/* it had a symbol encoding which we converted to unicode */
    unsigned int complainedbeyondglyfend:1;	/* Don't complain about this more than once */
    unsigned int extensionrequested:1;		/* Only ask once for a copy of a font containing extension subtables */
    unsigned int to_order2:1;			/* We are to leave the font as truetype (order2) splines, else convert to ps */
    unsigned int complainedmultname:1;	/* Don't complain about this more than once */
    unsigned int strokedfont: 1;		/* painttype==2 for otf */
    unsigned int use_typo_metrics: 1;
    unsigned int weight_width_slope_only: 1;
    unsigned int optimized_for_cleartype: 1;
    unsigned int apply_lsb: 1;
    int sfntRevision;
    enum openflags openflags;
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
    int uniqueid;
    real italicAngle;		/* from post table */
    int upos, uwidth;		/* underline pos, width from post table */
    real strokewidth;
    int fstype;
    struct psdict *private;	/* Only for open type cff fonts */
    EncMap *map;
    enum uni_interp uni_interp;
    struct pfminfo pfminfo;
    short os2_version;
    short gasp_version;
    int dupnamestate;
    struct ttflangname *names;
    char *fontcomments, *fontlog;
    char **cvt_names;
    SplineChar **chars;		/* from all over, glyf table for contours */
    				/* 		  cmap table for encodings */
			        /*		  hmtx table for widths */
			        /*		  post table for names */
			        /* Or from	  CFF  table for everything in opentype */
    LayerInfo *layers;
    int layer_cnt;
    BDFFont *bitmaps;
    char *cidregistry, *ordering;
    int supplement;
    real cidfontversion;
    int subfontcnt;
    SplineFont **subfonts;
    char *inuse;		/* What glyphs are used by this font in the ttc */

    int numtables;
    		/* BASE  */
    uint32 base_start;		/* Offset from sof to start of 'BASE' table */
    		/* CFF  */
    uint32 cff_start;		/* Offset from sof to start of postscript compact font format */
    uint32 cff_length;
    		/* cmap */
    uint32 encoding_start;	/* Offset from sof to start of encoding table */
    uint32 vs_start;		/* Offset within 'cmap' to variant selector table */
		/* gasp */
    uint32 gasp_start;
		/* glyf */
    uint32 glyph_start;		/* Offset from sof to start of glyph table */
    uint32 glyph_length;
		/* GDEF */
    uint32 gdef_start;		/* Offset from sof to start of GDEF table (glyph class defn, ligature carets) */
    uint32 gdef_length;
		/* GPOS */
    uint32 gpos_start;		/* Offset from sof to start of GPOS table */
    uint32 gpos_length;
		/* GSUB */
    uint32 gsub_start;		/* Offset from sof to start of GSUB table */
    uint32 gsub_length;
    uint32 g_bounds;		/* Filled in with g???_start+g???_length */
		/* EBDT, bdat */
    uint32 bitmapdata_start;	/* Offset to start of bitmap data */
    uint32 bitmapdata_length;
		/* EBLT, bloc */
    uint32 bitmaploc_start;	/* Offset to start of bitmap locator data */
    uint32 bitmaploc_length;
		/* gvar, etc. */
    uint32 gvar_start, gvar_len;
    uint32 fvar_start, fvar_len;
    uint32 avar_start, avar_len;
    uint32 cvar_start, cvar_len;
		/* head */
    uint32 head_start;
		/* hhea */
    uint32 hhea_start;
		/* hmtx */
    uint32 hmetrics_start;
		/* JSTF */
    uint32 jstf_start;
    uint32 jstf_length;
		/* kern */
    uint32 kern_start;
		/* loca */
    uint32 glyphlocations_start;/* there are glyph_cnt of these, from maxp tab */
    uint32 loca_length;		/* actually glypn_cnt is wrong. Use the table length (divided by size) instead */
		/* maxp */
    uint32 maxp_start;		/* maximum number of glyphs */
    uint32 maxp_len;
		/* name */
    uint32 copyright_start;	/* copyright and fontname */
		/* post */
    uint32 postscript_start;	/* names for the glyphs, italic angle, etc. */
		/* OS/2 */
    uint32 os2_start;
		/* TYP1 */
    uint32 typ1_start;		/* For Adobe's? Apple's? attempt to stuff a type1 font into an sfnt wrapper */
    uint32 typ1_length;
		/* vhea */
    uint32 vhea_start;
		/* vmtx */
    uint32 vmetrics_start;
		/* VORG */
    uint32 vorg_start;

		/* PfEd -- FontForge/PfaEdit specific info */
    uint32 pfed_start;
		/* TeX  -- TeX table, also non-standard */
    uint32 tex_start;
		/* BDF  -- BDF properties, also non-standard */
    uint32 bdf_start;
		/* FFTM -- FontForge timestamps */
    uint32 fftm_start;

		/* Apple Advanced Typography Tables */
    uint32 prop_start;
    uint32 lcar_start;
    uint32 opbd_start;
    uint32 acnt_start;
    uint32 feat_start;
    uint32 mort_start;
    uint32 morx_start;
    uint32 bsln_start;

		/* MATH Table */
    uint32 math_start;
    uint32 math_length;

		/* Info for instructions */
    uint32 cvt_start, cvt_len;
    uint32 prep_start, prep_len;
    uint32 fpgm_start, fpgm_len;

    unsigned int one_of_many: 1;	/* A TTCF file, or a opentype font with multiple fonts */
    unsigned int obscomplain: 1;	/* We've complained about obsolete format 3 in EBDT table */
    unsigned int cmpcomplain: 1;	/* We've complained about compressed format 4 in EBDT */
    unsigned int unkcomplain: 1;	/* We've complained about unknown formats in EBDT */
    unsigned int comcomplain: 1;	/* We've complained about composit formats in EBDT */
    unsigned int onlystrikes: 1;	/* Only read in the bitmaps, not the outlines */
    unsigned int onlyonestrike: 1;	/* Only read in one bitmap (strike) */
    unsigned int barecff: 1;		/* pay attention to the encoding in the cff file, we won't have a cmap */
    unsigned int wdthcomplain: 1;	/* We've complained about advance widths exceding the max */
    unsigned int bbcomplain: 1;		/* We've complained about glyphs being outside the bounding box */
    unsigned int gbbcomplain: 1;	/* We've complained about points being outside the bounding box */

    int platform, specific;		/* values of the encoding we chose to use */

    int anchor_class_cnt;		/* For GPOS */
    int anchor_merge_cnt;
    AnchorClass *ahead, *alast;

    KernClass *khead, *klast, *vkhead, *vklast;

    OTLookup *gpos_lookups, *gsub_lookups, *cur_lookups;

    OTLookup *mort_subs_lookup, *mort_pos_lookup2;
    int mort_r2l, mort_tag_mac, mort_feat, mort_setting, mort_is_nested;
    uint16 *morx_classes;
    uint16 *bsln_values;

    int mort_max;

    struct ttf_table *tabs;
    FPST *possub;
    ASM *sm;
    MacFeat *features;
    char *chosenname;
    int macstyle;
    int lookup_cnt;		/* Max lookup in current GPOS/GSUB table */
    int feature_cnt;		/* Max feature in current GPOS/GSUB table */
    struct variations *variations;
    struct macidname *macstrids;
    struct fontdict *fd;	/* For reading in Type42 fonts. Glyph names in postscript section must be associated with glyphs in TTF section */
    int savecnt;
    struct savetab *savetab;
    int32 last_size_pos;
    uint16 design_size;
    uint16 fontstyle_id;
    struct otfname *fontstyle_name;
    uint16 design_range_bottom, design_range_top;
    struct texdata texdata;
    int mark_class_cnt;
    char **mark_classes;		/* glyph name list */
    char **mark_class_names;		/* used within ff (utf8) */
    int mark_set_cnt;
    char **mark_sets;			/* glyph name list */
    char **mark_set_names;		/* used within ff (utf8) */
    uint8 warned_morx_out_of_bounds_glyph;
    int badgid_cnt, badgid_max;		/* Used when parsing apple morx tables*/
    SplineChar **badgids;		/* which use out of range glyph IDs as temporary flags */
    long long creationtime;		/* seconds since 1970 */
    long long modificationtime;
    int gasp_cnt;
    struct gasp *gasp;
    struct MATH *math;
    /* Set of errors we found when loading the font */
    unsigned int bad_ps_fontname: 1;
    unsigned int bad_glyph_data: 1;
    unsigned int bad_cff: 1;
    unsigned int bad_metrics: 1;
    unsigned int bad_cmap: 1;
    unsigned int bad_embedded_bitmap: 1;
    unsigned int bad_gx: 1;
    unsigned int bad_ot: 1;
    unsigned int bad_os2_version: 1;
    unsigned int bad_sfnt_header: 1;
    Layer guidelines;
    struct Base *horiz_base, *vert_base;
    Justify *justify;

    int advanceWidthMax;
    int fbb[4];		/* x,yMin x,yMax*/
    int isFixedPitch;

    uint32 jstf_script;
    uint32 jstf_lang;
    int16 jstf_isShrink, jstf_prio, jstf_lcnt;
    struct otffeatname *feat_names;
    enum gsub_inusetype justinuse;
    long ttfFileSize;
};

struct taboff {
    uint32 tag;	/* Table name */
    uint32 checksum;/* for table */
    uint32 offset;	/* to start of table in file */
    uint32 length;
    FILE *data;
    uint16 dup_of;
    uint16 orderingval;
};

#define MAX_TAB	48
struct tabdir {
    int32 version;	/* 0x00010000 */
    uint16 numtab;
    uint16 searchRange;	/* (Max power of 2 <= numtab) *16 */
    uint16 entrySel;	/* Log2(Max power of 2 <= numtab ) */
    uint16 rangeShift;	/* numtab*16 - searchRange */
    struct taboff tabs[MAX_TAB];/* room for all the tables */
				/* Not in any particular order. */
    struct taboff *ordered[MAX_TAB];	/* Ordered the way the tables should be output in file */
    struct taboff *alpha[MAX_TAB];	/* Ordered alphabetically by tag for the ttf header */
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
	    /* not exactly specified, but FontValidator wants this to match italicangle */
    int16 mbz[5];
    int16 metricformat;	/* 0 */
    uint16 numMetrics;	/* just set to glyph count */
};

struct hmtx {
    uint16 width;	/* NOTE: TTF only allows positive widths!!! */
    int16 lsb;
};

struct kp {
    uint16 left;	/* left glyph num */
    uint16 right;	/* right glyph num */
    /* table is ordered by these two above treated as uint32 */
    int16 offset;	/* kern amount */
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
    struct kp *kerns;	/* Array should be nPairs big */
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
    uint16 maxcomponentdepth;
	/* Apple docs say: 0 (if no composits), maximum value 1 (one level of composit) */
	/* OpenType docs say: 1 (if no composits), any depth allowed */
};

struct namerec {
    uint16 platform;	/* 3 => MS */
    uint16 specific;	/* 1 */
    uint16 language;	/* 0x0409 */
    uint16 nameid;	/* 0=>copyright, 1=>family, 2=>weight, 4=>fullname */
			/*  5=>version, 6=>postscript name */
    uint16 strlen;
    uint16 stroff;
};

struct nametab {
    uint16 format;	/* 0 */
    uint16 numrec;	/* 1 */
    uint16 startOfStrings;	/* offset from start of table to start of strings */
    struct namerec nr[6];
};

struct os2 {
    uint16 version;	/* 1 */
    int16 avgCharWid;	/* average all chars (v3) see v2 definition below */
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
			/* 2=>underscore, 4=>negative, 8->outlined, 16=>strikeout */
			/* version 4 of OS/2 */
			/* 128->don't use win_ascent/descent for line spacing */
			/* 256=>family varies on weight width slope only */
			/* 512=>oblique (as opposed to italic) */
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
    /* V3 of OS/2 has no additional data */
    /* V4 of OS/2 has no additional data */

    int v1_avgCharWid;	/* 1&2 Weighted average of the lower case letters and space */
    int v3_avgCharWid;	/* 3&4 average over all non-zero width glyphs */
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
    int xmin, ymin, xmax, ymax;
    BlueData bd;
    int strikecnt;		/* number of bitmaps to dump */
    int lasthwidth, lastvwidth;	/* encoding of last glyph for which we generate a full metrics entry */
    int hfullcnt, vfullcnt;
    int flags;
    int fixed_width;
    int32 *bsizes;
    unsigned int dovariations: 1;
    unsigned int onlybitmaps: 1;
    unsigned int has_instrs: 1;
    unsigned int is_ttf: 1;
    unsigned int ttc_composite_font: 1;
    SplineFont *sf;
    int32 *pointcounts;
    int *bygid;			/* glyph list */
    int gcnt;
    int layer;
};

struct vorg {
    uint16 majorVersion;		/* 1 */
    uint16 minorVersion;		/* 0 */
    short defaultVertOriginY;	/* Y coord of default vertical origin in the design coordinate system */
    uint16 numVertOriginYMetrics;	/* exceptions to the above, elements in following array */
};

struct feat_name {
    int strid;
    struct macname *mn, *smn;
};

struct other_names {
    int strid;
    struct macname *mn;
    struct other_names *next;
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
    FILE *gpos;			/* Used instead of kern for opentype (and other glyph positioning) */
    int gposlen;
    FILE *gsub;			/* Used for ligatures and other substitutions */
    int gsublen;
    FILE *gdef;			/* If we use mark to base we need this to tell the text processor what things are marks (the opentype docs say it is optional. They are wrong) */
    int gdeflen;
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
    FILE *math;
    int mathlen;
    FILE *base;
    int baselen;
    FILE *jstf;
    int jstflen;
    FILE *cvtf;
    int cvtlen;
    FILE *fpgmf;		/* Copied from an original ttf file and dumped out. Never generated */
    int fpgmlen;
    FILE *prepf;		/* Copied from an original ttf file and dumped out. Never generated */
    int preplen;
    FILE *vheadf;
    int vheadlen;
    FILE *vorgf;
    int vorglen;
    FILE *gaspf;
    int gasplen;
    FILE *cfff;
    int cfflen;
    FILE *sidf;
    FILE *sidh;
    FILE *charset;
    FILE *encoding;
    FILE *globalsubrs;
    FILE *private;
    FILE *charstrings;
    FILE *fdselect;
    FILE *fdarray;
    FILE *bdat;		/* might be EBDT */
    int bdatlen;
    FILE *bloc;		/* might be EBLC */
    int bloclen;
    FILE *ebsc;
    int ebsclen;
    FILE *prop;
    int proplen;
    FILE *opbd;
    int opbdlen;
    FILE *acnt;
    int acntlen;
    FILE *lcar;
    int lcarlen;
    FILE *feat;
    int featlen;
    FILE *morx;
    int morxlen;
    FILE *bsln;
    int bslnlen;
    FILE *pfed;
    int pfedlen;
    FILE *tex;
    int texlen;
    FILE *bdf;
    int bdflen;
    FILE *gvar;
    int gvarlen;
    FILE *fvar;
    int fvarlen;
    FILE *cvar;
    int cvarlen;
    FILE *avar;
    int avarlen;
    FILE *fftmf;
    int fftmlen;
    FILE *dsigf;
    int dsiglen;
    FILE *hdmxf;
    int hdmxlen;
    int defwid, nomwid;
    int sidcnt;
    int lenpos;
    int privatelen;
    unsigned int sidlongoffset: 1;
    unsigned int cfflongoffset: 1;
    unsigned int applemode: 1;		/* Where apple & ms differ do things apple's way (bitmaps, name table PostScript) */
    unsigned int opentypemode: 1;	/* Where apple & ms differ do things opentype's way (bitmaps, name table PostScript) */
	    /* If both are set then try to generate both types of tables. Some things can't be fudged though (name table postscript) */
    unsigned int msbitmaps: 1;
    unsigned int applebitmaps: 1;
    unsigned int otbbitmaps: 1;
    unsigned int isotf: 1;
    unsigned int dovariations: 1;	/* Output Apple *var tables (for mm fonts) */
    unsigned int error: 1;
    struct glyphinfo gi;
    int isfixed;
    struct fd2data *fds;
    int next_strid;

    struct feat_name *feat_name;
    struct other_names *other_names;
    struct macname2 *ordered_feat;

    int next_lookup;	/* for doing nested lookups in contextual features */
    short *gn_sid;
    enum fontformat format;
    int fontstyle_name_strid;	/* For GPOS 'size' */
    SplineFont *sf;
    EncMap *map;
    struct ttf_table *oldcvt;
    unsigned oldcvtlen;
};

struct subhead { uint16 first, cnt, delta, rangeoff; };	/* a sub header in 8/16 cmap table */

enum touchflags { tf_x=1, tf_y=2, tf_d=4, tf_endcontour=0x80, tf_startcontour=0x40 };

struct ct_branch {
    uint16 classnum;
    struct contexttree *branch;
};

struct ct_subs {
    struct fpst_rule *rule;
    struct contexttree *branch;/* if the rule ends here this will be null */
    uint16 thisclassnum;
};

struct contexttree {
    int depth;
    int branch_cnt;	/* count of subbranches of this node */
    struct ct_branch *branches;
    struct fpst_rule *ends_here;
    int rule_cnt;	/* count of rules which are active here */
    struct ct_subs *rules;
    int pending_pos;
    OTLookup *applymarkedsubs;
    OTLookup *applycursubs;
    uint16 marked_index, cur_index;
    uint8 markme;
    int state, next_state;
    struct contexttree *parent;
};

	/* TrueType Composite glyph flags */
#define _ARGS_ARE_WORDS	1
#define _ARGS_ARE_XY	2
#define _ROUND		4		/* round offsets so componant is on grid */
#define _SCALE		8
/* 0x10 is reserved */
#define _MORE		0x20
#define _XY_SCALE	0x40
#define _MATRIX		0x80
#define _INSTR		0x100
#define _USE_MY_METRICS	0x200
#define _OVERLAP_COMPOUND	0x400	/* Used in Apple GX fonts */
	    /* Means the components overlap (which? this one and what other?) */
/* Described in OpenType specs, not by Apple */
/* amusingly, Apple supports but MS does not */
/* MS says they support this after Win 2000 */
#define _SCALED_OFFSETS		0x800	/* Use Apple definition of offset interpretation */
#define _UNSCALED_OFFSETS	0x1000	/* Use MS definition */

extern int ttfFixupRef(SplineChar **chars,int i);
extern const char *cffnames[];
extern const int nStdStrings;

    /* Open type Advanced Typography Tables */
extern void otf_dumpgpos(struct alltabs *at, SplineFont *sf);
extern void otf_dumpgsub(struct alltabs *at, SplineFont *sf);
extern void otf_dumpgdef(struct alltabs *at, SplineFont *sf);
extern void otf_dumpbase(struct alltabs *at, SplineFont *sf);
extern void otf_dumpjstf(struct alltabs *at, SplineFont *sf);
extern void otf_dump_dummydsig(struct alltabs *at, SplineFont *sf);
extern int gdefclass(SplineChar *sc);

    /* Apple Advanced Typography Tables */
extern void aat_dumpacnt(struct alltabs *at, SplineFont *sf);
extern void ttf_dumpkerns(struct alltabs *at, SplineFont *sf);
extern void aat_dumplcar(struct alltabs *at, SplineFont *sf);
extern void aat_dumpmorx(struct alltabs *at, SplineFont *sf);
extern void aat_dumpopbd(struct alltabs *at, SplineFont *sf);
extern void aat_dumpprop(struct alltabs *at, SplineFont *sf);
extern void aat_dumpbsln(struct alltabs *at, SplineFont *sf);
extern int LookupHasDefault(OTLookup *otl);
extern int scriptsHaveDefault(struct scriptlanglist *sl);
extern int FPSTisMacable(SplineFont *sf, FPST *fpst);
extern uint32 MacFeatureToOTTag(int featureType,int featureSetting);
extern int OTTagToMacFeature(uint32 tag, int *featureType,int *featureSetting);
extern uint16 *props_array(SplineFont *sf,struct glyphinfo *gi);
extern int haslrbounds(SplineChar *sc, PST **left, PST **right);
extern int16 *PerGlyphDefBaseline(SplineFont *sf,int *def_baseline);
extern void FigureBaseOffsets(SplineFont *sf,int def_bsln,int offsets[32]);

    /* Apple variation tables */
extern int ContourPtNumMatch(MMSet *mm, int gid);
extern int16 **SCFindDeltas(MMSet *mm, int gid, int *_ptcnt);
extern int16 **CvtFindDeltas(MMSet *mm, int *_ptcnt);
extern void ttf_dumpvariations(struct alltabs *at, SplineFont *sf);

extern struct macsettingname {
    int mac_feature_type;
    int mac_feature_setting;
    uint32 otf_tag;
} macfeat_otftag[], *user_macfeat_otftag;

    /* TrueType instructions */
extern struct ttf_table *SFFindTable(SplineFont *sf,uint32 tag);
extern int32 memlong(uint8 *data,int table_len, int offset);
extern int memushort(uint8 *data,int table_len, int offset);
extern void memputshort(uint8 *data,int offset,uint16 val);
extern int TTF__getcvtval(SplineFont *sf,int val);
extern int TTF_getcvtval(SplineFont *sf,int val);
extern void SCinitforinstrs(SplineChar *sc);
extern int SSAddPoints(SplineSet *ss,int ptcnt,BasePoint *bp, char *flags);
extern int Macable(SplineFont *sf, OTLookup *otl);

    /* Used by both otf and apple */
extern int LigCaretCnt(SplineChar *sc);
extern uint16 *ClassesFromNames(SplineFont *sf,char **classnames,int class_cnt,
	int numGlyphs, SplineChar ***glyphs, int apple_kc);
extern SplineChar **SFGlyphsFromNames(SplineFont *sf,char *names);


extern void AnchorClassOrder(SplineFont *sf);
extern SplineChar **EntryExitDecompose(SplineFont *sf,AnchorClass *ac,
	struct glyphinfo *gi);
extern void AnchorClassDecompose(SplineFont *sf,AnchorClass *_ac, int classcnt, int *subcnts,
	SplineChar ***marks,SplineChar ***base,
	SplineChar ***lig,SplineChar ***mkmk,
	struct glyphinfo *gi);

extern void cvt_unix_to_1904( long long time, int32 result[2]);


    /* Non-standard tables */
	/* My PfEd table for FontForge/PfaEdit specific info */
extern void pfed_dump(struct alltabs *at, SplineFont *sf);
extern void pfed_read(FILE *ttf,struct ttfinfo *info);
	/* The TeX table, to contain stuff the TeX people want */
extern void tex_dump(struct alltabs *at, SplineFont *sf);
extern void tex_read(FILE *ttf,struct ttfinfo *info);
	/* The BDF table, to contain bdf properties the X people want */
extern int ttf_bdf_dump(SplineFont *sf,struct alltabs *at,int32 *sizes);
extern void ttf_bdf_read(FILE *ttf,struct ttfinfo *info);
	/* The FFTM table, to some timestamps I'd like */
extern int ttf_fftm_dump(SplineFont *sf,struct alltabs *at);

    /* The MATH table */
extern void otf_dump_math(struct alltabs *at, SplineFont *sf);
extern void otf_read_math(FILE *ttf,struct ttfinfo *info);
extern void otf_read_math_used(FILE *ttf,struct ttfinfo *info);
extern void GuessNamesFromMATH(FILE *ttf,struct ttfinfo *info);

    /* Parsing advanced typography */
extern void readmacfeaturemap(FILE *ttf,struct ttfinfo *info);
extern void readttfkerns(FILE *ttf,struct ttfinfo *info);
extern void readttfmort(FILE *ttf,struct ttfinfo *info);
extern void readttfmort_glyphsused(FILE *ttf,struct ttfinfo *info);
extern void readttfopbd(FILE *ttf,struct ttfinfo *info);
extern void readttflcar(FILE *ttf,struct ttfinfo *info);
extern void readttfprop(FILE *ttf,struct ttfinfo *info);
extern void readttfbsln(FILE *ttf,struct ttfinfo *info);
extern void readttfgsubUsed(FILE *ttf,struct ttfinfo *info);
extern void GuessNamesFromGSUB(FILE *ttf,struct ttfinfo *info);
extern void readttfgpossub(FILE *ttf,struct ttfinfo *info,int gpos);
extern void readttfgdef(FILE *ttf,struct ttfinfo *info);
extern void readttfbase(FILE *ttf,struct ttfinfo *info);
extern void readttfjstf(FILE *ttf,struct ttfinfo *info);

extern void VariationFree(struct ttfinfo *info);
extern void readttfvariations(struct ttfinfo *info, FILE *ttf);

extern struct otfname *FindAllLangEntries(FILE *ttf, struct ttfinfo *info, int id );

/* Known font parameters for 'TeX ' table (fontdims, spacing params, whatever you want to call them) */
    /* Used by all fonts */
#define	TeX_Slant	CHR('S','l','n','t')
#define TeX_Space	CHR('S','p','a','c')
#define TeX_Stretch	CHR('S','t','r','e')
#define TeX_Shrink	CHR('S','h','n','k')
#define TeX_XHeight	CHR('X','H','g','t')
#define TeX_Quad	CHR('Q','u','a','d')
    /* Used by text fonts */
#define TeX_ExtraSp	CHR('E','x','S','p')
    /* Used by all math fonts */
#define TeX_MathSp	CHR('M','t','S','p')
    /* Used by math fonts */
#define TeX_Num1	CHR('N','u','m','1')
#define TeX_Num2	CHR('N','u','m','2')
#define TeX_Num3	CHR('N','u','m','3')
#define TeX_Denom1	CHR('D','n','m','1')
#define TeX_Denom2	CHR('D','n','m','2')
#define TeX_Sup1	CHR('S','u','p','1')
#define TeX_Sup2	CHR('S','u','p','2')
#define TeX_Sup3	CHR('S','u','p','3')
#define TeX_Sub1	CHR('S','u','b','1')
#define TeX_Sub2	CHR('S','u','b','2')
#define TeX_SupDrop	CHR('S','p','D','p')
#define TeX_SubDrop	CHR('S','b','D','p')
#define TeX_Delim1	CHR('D','l','m','1')
#define TeX_Delim2	CHR('D','l','m','2')
#define TeX_AxisHeight	CHR('A','x','H','t')
    /* Used by math extension fonts */
#define TeX_DefRuleThick	CHR('R','l','T','k')
#define TeX_BigOpSpace1		CHR('B','O','S','1')
#define TeX_BigOpSpace2		CHR('B','O','S','2')
#define TeX_BigOpSpace3		CHR('B','O','S','3')
#define TeX_BigOpSpace4		CHR('B','O','S','4')
#define TeX_BigOpSpace5		CHR('B','O','S','5')

extern void SFDummyUpCIDs(struct glyphinfo *gi,SplineFont *sf);

#endif /* FONTFORGE_TTF_H */
