/* Copyright (C) 2008 by George Williams */
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

#ifndef _PFED_H
# define _PFED_H
/* The PfEd table (from PfaEdit, former name of FontForge) is designed to   */
/*  store information useful for editing the font. This includes stuff like */
/*  guidelines, background layers, spiro layers, comments, lookup names, etc*/
/* The idea is that much useful information can be retrieved even after the */
/*  font has been generated into its final form: ttf/otf                    */
/* It is something of a catch all table composed of many subtables, each of */
/*  which preserves one particular item. I expect more subtables will be    */
/*  added as I, or others, think of more things worth preserving.           */


# ifndef CHR
#  define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))
# endif

/* 'PfEd' table format is as follows...				 */
/* uint32  version number 0x00010000				 */
/* uint32  subtable count					 */
/* struct { uint32 tab, offset } tag/offset for first subtable	 */
/* struct { uint32 tab, offset } tag/offset for second subtable	 */
/* ...								 */

/* 'PfEd' 'fcmt' font comment subtable format			 */
/*  short version number 0/1					 */
/*  short string length						 */
/*  version 0=>String in latin1 (should be ASCII), version 1=>utf8*/

 /* 'PfEd' 'cmnt' glyph comment subtable format			 */
 /*  THIS VERSION IS DEPRECATED IN FAVOR OF VERSION 1 DESCRIBED BELOW */
 /*  short  version number 0					 */
 /*  short  count-of-ranges					 */
 /*  struct { short start-glyph, end-glyph, short offset }	 */
 /*  ...							 */
 /*  foreach glyph >=start-glyph, <=end-glyph(+1)		 */
 /*   uint32 offset		to glyph comment string (in UCS2)*/
 /*  ...							 */
 /*  And one last offset pointing beyong the end of the last string to enable length calculations */
 /*  String table in UCS2 (NUL terminated). All offsets from start*/
 /*   of subtable */

/* 'PfEd' 'cmnt' glyph comment subtable format			 */
/*   New version of 'cmnt' which uses utf8 rather than UCS2	 */
/*  short  version number 1					 */
/*  short  count-of-ranges					 */
/*  struct { short start-glyph, end-glyph, short offset }	 */
/*  ...								 */
/*  foreach glyph >=start-glyph, <=end-glyph(+1)		 */
/*   uint32 offset		to glyph comment string (in utf8)*/
/*  ...								 */
/*  And one last offset pointing beyong the end of the last string to enable length calculations */
/*  String table in utf8 (NUL terminated). All offsets from start*/
/*   of subtable */

/* 'PfEd' 'cvt ' cvt comment subtable format			 */
/*  short  version number 0					 */
/*  short  size of cvt comment array (might be less that cvt)	 */
/*  ushort offset[size] to utf8 strings describing cvt entries	 */
/*           (strings are NUL terminated)			 */
/*  String table in utf8 (NUL terminated). All offsets from start*/
/*   of subtable */

/* 'PfEd' 'colr' glyph colour subtable				 */
/*  short  version number 0					 */
/*  short  count-of-ranges					 */
/*  struct { short start-glyph, end-glyph, uint32 colour (rgb) } */

/* 'PfEd' 'GPOS' GPOS lookup/subtable/anchor names		 */
/*  short  version number 0					 */
/*  short  count-of-lookups					 */
/*  struct { short offset name, offset subtables; }		 */
/* A subtable:							 */
/*  short  count-of-subtables					 */
/*  struct { short offset name, offset anchors; }		 */
/* An anchor							 */
/*  short  count-of-anchors					 */
/*  struct { short offset name; }				 */
/*  string data (utf8, nul terminated)                           */
/* (all offsets from start of subtable)				 */

/* 'PfEd' 'GSUB' GSUB lookup/subtable names			 */
/*  Same as GPOS (anchors will always be NULL)			 */

/* The next two tables 'guid' and 'layr' will make use of the    */
/*  following data type. The glyph_layer.			 */
/*  short contour-count						 */
/*  short image-count (always 0 for now)			 */
/*  struct { ushort offset; ushort name-off}[contour-count]	 */
/*  struct { ??? }[image-count]					 */
/* Each contour consists of one byte of command and a random     */
/*  amount of data dependant on the command.			 */
/* See below for a description of the command verbs		 */
/* string data in utf8 */
/*  (offsets relative to start of glyph_layer structure )	 */

/* 'PfEd' 'guid' Horizontal/Vertical guideline data		 */
/*  short  version number 0					 */
/*  short  # vertical guidelines				 */
/*  short  # horizontal guidelines				 */
/*  short  mbz							 */
/*  offset guide spline data -- specifies all guides as splines  */
/*   May be NULL						 */
/*  struct[# vert guidelines] { short x; short offset name}	 */
/*  struct[# h guidelines] { short y; short offset name}	 */
/*  a glyph_layer (includes all guides, even those listed above) */
/* (all offsets, except those in the glyph_layer, relative to start of subtable) */

/* 'PfEd' 'layr' layer data					 */
/*  short  version number 0					 */
/*  short  layer-count						 */
/*  struct { short typeflags; short offset-name; uint32 offset-to-layer-data; } */
/*   the layer type is 2=>quadratic, 3=>PostScript, 1=>spiro	 */
/*                     0x102=>quadratic fore, 0x103=>PS fore	 */
/* A layer:							 */
/*  short  count-of-ranges					 */
/*  struct { short start-glyph, end-glyph, uint32 offset }	 */
/*  ...								 */
/*  foreach glyph >=start-glyph, <=end-glyph			 */
/*   uint32 offset		to per-glyph outline data        */
/*  many glyph_layers						 */
/* (all offsets, except those in the glyph_layers, relative to start of subtable) */

 /* Main table tag */
# define PfEd_TAG	CHR('P','f','E','d')

 /* Subtable tags */
# define fcmt_TAG	CHR('f','c','m','t')		/* Font Comment */
# define flog_TAG	CHR('f','l','o','g')		/* Font Log */
# define cmnt_TAG	CHR('c','m','n','t')		/* Glyph Comments */
# define cvtc_TAG	CHR('c','v','t','c')		/* Comments for each cvt entry */
# define colr_TAG	CHR('c','o','l','r')		/* Glyph color flags */
# ifndef GPOS_TAG
#  define GPOS_TAG	CHR('G','P','O','S')		/* Names for GPOS lookups */
#  define GSUB_TAG	CHR('G','S','U','B')		/* Names for GSUB lookups */
# endif
# define guid_TAG	CHR('g','u','i','d')		/* Guideline data */
# define layr_TAG	CHR('l','a','y','r')		/* Any layers which aren't part of the font */
						/* Backgrounds, spiros, etc. */


/* The layer commands used to draw quadratic and cubic layers have two   */
/*  componants: A verb, which says what to do, and a modifier which says */
/*  how the data are stored. So a command looks like (verb)|(modifier)   */
/* A moveto command with byte data looks like (V_MoveTo|V_B)             */
/*  The two commands to end a contour (V_Close and V_End) take no data   */
/*  and use no modifier							 */

/* layr subtable contour construction verb modifiers specifying data types */
# define	V_B	0			/* data are signed bytes */
# define	V_S	1			/* data are signed shorts */
# define	V_F	2			/* data are fixed longs, divide by 256.0 */

/* layr subtable contour construction verbs */
# define	V_MoveTo	0		/* Start contour, absolute data (2 coords) */
# define	V_LineTo	4		/* Straight line, relative data (2 coords) */
# define	V_HLineTo	8		/* Horizontal line, relative (1 coord, x-off) */
# define	V_VLineTo	12		/* Vertical line, relative (1 coord, y-off) */

# define	V_QCurveTo	16		/* Quadratic spline, rel, rel (4 coords, cp, p) */
# define	V_QImplicit	20		/* Quadratic spline, rel (2 coords, cp) */
	/* May only occur after a V_QCurveTo or V_QImplicit (may not start contour) */
	/* Must be followed by a V_QCurveTo or V_QImplicit (this may end contour) */
	/* The on-curve point is implicit by averaging the given cp with the cp in the next verb */
# define	V_QHImplicit	24		/* Quadratic spline, rel (1 coord, cp.x) */
# define	V_QVImplicit	28		/* Quadratic spline, rel (1 coord, cp.y) */

# define	V_CurveTo	32		/* Cubic spline, rel, rel (6 coords, cp1, cp2, p) */
# define	V_VHCurveTo	36		/* Cubic spline, rel, rel (4 coords, cp1.y cp2.* p.x) */
	/* cp1.x == current.y, p.y == cp2.y */
# define	V_HVCurveTo	40		/* Cubic spline, rel, rel (4 coords, cp1.x cp2.* p.y) */

# define	V_Close		44		/* Close path (optionally adding a line) no data */
# define	V_End		45		/* End path (leave open) no data */

#define COM_MOD(com)	((com)&3)
#define COM_VERB(com)	((com)&~3)

/* the layer commands used to draw spiro layers are the standard spiro verbs */
/*  these will always take fixed long data (V_L) as described above */
# ifndef SPIRO_OPEN_CONTOUR
#  define SPIRO_OPEN_CONTOUR	'{'
#  define SPIRO_CORNER		'v'
#  define SPIRO_G4		'o'
#  define SPIRO_G2		'c'
#  define SPIRO_LEFT		'['
#  define SPIRO_RIGHT		']'
#  define SPIRO_END		'z'
# endif
# define SPIRO_CLOSE_CONTOUR	'}'

#endif	/* _PFED_H */
