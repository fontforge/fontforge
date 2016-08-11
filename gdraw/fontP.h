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
#ifndef _FONTP_H
#define _FONTP_H

#include "fontforge-config.h"

#ifndef X_DISPLAY_MISSING
# include <X11/Xlib.h>		/* For XFontStruct */
#else
/* Taken from ... */
    /* $TOG: Xlib.h /main/122 1998/03/22 18:22:09 barstow $ */
    /* 

    Copyright 1985, 1986, 1987, 1991, 1998  The Open Group

    All Rights Reserved.

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the name of The Open Group shall not be
    used in advertising or otherwise to promote the sale, use or other dealings
    in this Software without prior written authorization from The Open Group.

    */
    /* $XFree86: xc/lib/X11/Xlib.h,v 3.17 2000/11/28 18:49:27 dawes Exp $ */


    /*
     *	Xlib.h - Header definition and support file for the C subroutine
     *	interface library (Xlib) to the X Window System Protocol (V11).
     *	Structures and symbols starting with "_" are private to the library.
     */
typedef int Atom;
typedef int Bool;

typedef struct {
    short	lbearing;	/* origin to left edge of raster */
    short	rbearing;	/* origin to right edge of raster */
    short	width;		/* advance to next char's origin */
    short	ascent;		/* baseline to top edge of raster */
    short	descent;	/* baseline to bottom edge of raster */
    unsigned short attributes;	/* per char flags (not predefined) */
} XCharStruct;

typedef struct {
    Atom name;
    unsigned long card32;
} XFontProp;

typedef struct {
    int 	fid;		/* Font id for this font */
    unsigned	direction;	/* hint about direction the font is painted */
    unsigned	min_char_or_byte2;/* first character */
    unsigned	max_char_or_byte2;/* last character */
    unsigned	min_byte1;	/* first row that exists */
    unsigned	max_byte1;	/* last row that exists */
    Bool	all_chars_exist;/* flag if all characters have non-zero size*/
    unsigned	default_char;	/* char to print for undefined character */
    int         n_properties;   /* how many properties there are */
    XFontProp	*properties;	/* pointer to array of additional properties*/
    XCharStruct	min_bounds;	/* minimum bounds over all existing char*/
    XCharStruct	max_bounds;	/* maximum bounds over all existing char*/
    XCharStruct	*per_char;	/* first_char to last_char information */
    int		ascent;		/* log. extent above baseline for spacing */
    int		descent;	/* log. descent below baseline for spacing */
} XFontStruct;

typedef struct {		/* normal 16 bit characters are two bytes */
    unsigned char byte1;
    unsigned char byte2;
} XChar2b;
#endif		/* NO X */

#include "gdrawP.h"
#ifndef FONTFORGE_CAN_USE_GDK
#  include "gxdrawP.h"
#else
#  include "ggdkdrawP.h"
#endif
#include "charset.h"

struct fontabbrev {
    char *abbrev;
    enum font_type ft;
    unsigned int italic: 1;
    unsigned int bold: 1;
    unsigned int dont_search: 1;
    unsigned int searched: 1;
    struct font_name *found;
};

#define FONTABBREV_EMPTY { NULL, 0, 0, 0, 0, 0, NULL }


struct kern_info {
    int16 following;		/* second character */
    int16 kern;			/* amount to kern by */
    struct kern_info *next;
};

#define AFM_KERN	0x0001	/* For per_char attributes, has kerning info */
#define AFM_EXISTS	0x0002	/* For per_char attributes, this char exists in the font */

struct font_data {
    struct font_data *next;
    struct font_name *parent;
    int16 point_size;		/* on current monitor */
    int16 weight;
    int16 x_height, cap_height;
    enum font_style style;
    enum charset map;
    unichar_t *charmap_name;	/* only set if non-standard, might be parsed later */
    char *localname;		/* xfontname, postscript fontname, etc. */
    char *fontfile, *metricsfile;	/* for postscript (.ps, .pfb), (.afm) */
    XFontStruct *info;		/* width info for characters, created for postscript, etc. */
    struct kern_info **kerns;	/* not implemented for 2byte fonts yet */

    uint32 scale_metrics_by;	/* Scale the result of postscript width */
		/* after calculating it: multiply by this, divide by 72000 */
		/* normally the scale factor is point_size*printer resolution*/
		/* (ps metrics data in 1000ths of a point) */
		/* but when scaling screen fonts it can be weird */
    unsigned int needsremap: 1;	/* ps font needs to be mapped to 8859-1 */
    unsigned int remapped: 1;	/* mapping has been done */
    unsigned int includenoted: 1;	/* for postscript %%IncludeFont directive */
    unsigned int copiedtoprinter: 1;	/* font description has been copied to the printer */
    unsigned int needsprocessing: 1;	/* font description has not been given to postscript */
    unsigned int copy_from_screen: 1;	/* need to copy the bitmaps from the screen font */
    unsigned int was_scaled: 1;
    unsigned int is_scalable: 1;
    unsigned int configuration_error: 1;/* Server thinks font exists, but when asked for it can't find it */
    struct font_data *screen_font;	/* if the printer doesn't have a font, see if we can do magic with a screen font instead */
    struct font_data *base;		/* of a scaled font */
    uint8 *exists;			/* Bit mask. set bits indicate contains a glyph for that index */
};

#define em_uplane0	(em_max+1)
#define em_uplanemax	(em_max+1+17)
struct font_name {
    struct font_name *next;
    unichar_t *family_name;
    enum font_type ft;
    struct font_data *data[em_uplanemax];/* list of all fonts with this name & type */
					/*  Final 17 are any UnicodePlane fonts */
};

struct font_instance {
    FontRequest rq;		/* identification of this instance */
    PangoFontDescription *pango_fd;
#ifndef _NO_LIBCAIRO
    PangoFontDescription *pangoc_fd;
#endif
};

typedef struct font_state {
    int res;
    struct font_name *font_names[26];
    unsigned int names_loaded: 1;
} FState;

extern struct fontabbrev _gdraw_fontabbrev[];

extern int _GDraw_ClassifyFontName(unichar_t *fontname, int *italic, int *bold);
extern enum charset _GDraw_ParseMapping(unichar_t *setname);
extern int _GDraw_FontFigureWeights(unichar_t *weight_str);
extern struct font_name *_GDraw_HashFontFamily(FState *fonts,unichar_t *name, int prop);
extern void _GDraw_FreeFD(struct font_data *fd);
#endif
