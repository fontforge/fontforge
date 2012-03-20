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
#ifndef _PSDRAW_H
#define _PSDRAW_H

#include <stdio.h>

#include "gdrawP.h"

typedef struct gpswindow {
    GGC *ggc;
    struct gpsdisplay *display;
    int (*eh)(GWindow,GEvent *);	/* not meaningful */
    GRect pos;				/* Page size minus margins */
    GWindow parent;			/* not meaningful */
    void *user_data;
    void *widget_data;
    FILE *output_file;
    unsigned int is_visible: 1;		/* Not meaningful here */
    unsigned int is_pixmap: 1;		/* Not meaningful here */
    unsigned int is_toplevel: 1;
    unsigned int is_dying: 1;
    unsigned int is_popup: 1;
    unsigned int disable_expose_requests: 1;
    unsigned int usecairo: 1;		/* should be 0, not meaningful */
    unsigned int usepango: 1;		/* should be 0, not meaningful */
    FILE *init_file;			/* Postscript style guides want all */
					/* fonts downloaded at start, but we */
			                /* don't know what we'll need then */
			                /* so this file contains inits and */
			                /* fonts, other file everything else */
			                /* they get merged at end. If we can't*/
			                /* open a temp file we merge the two */
			                /* works, but isn't nice */
/* current state */
    int pnt_cnt;			/* Init to -1 */
    long cur_x, cur_y;
    long line_x, line_y;
    unsigned int buffered_line: 1;
    int16 cur_dash_len, cur_skip_len;
    int16 cur_line_width;
    int16 cur_dash_offset;
    int16 cur_ts;
    struct font_data *cur_font;
    Color cur_fg, cur_bg;
/* Previous state saved by gsave */
    int last_dash_len, last_skip_len;
    int last_line_width;
    int last_dash_offset;
    int last_ts;
    struct font_data *last_font;
    Color last_fg, last_bg;
    GRect cur_clip;
    int res;
    uint32 ascii85encode;
    int16 ascii85n, ascii85bytes_per_line;
    uint32 page_cnt;
} *GPSWindow;

#define STROKE_CACHE	20

typedef struct gpsdisplay /* : GDisplay */ {
    struct displayfuncs *funcs;
    void *semaphore;			/* To lock the display against multiple threads */
    struct font_state *fontstate;
    int16 res;
    int16 scale_screen_by;		/* When converting screen pixels to printer pixels */
    GWindow groot;			/* Current printer window (can only be */
					/*  one thing being printed at a time) */
    Color def_background, def_foreground;
    uint16 mykey_state;			/* Useless here */
    uint16 mykey_keysym;
    uint16 mykey_mask;
    unsigned int mykeybuild: 1;
    /* display specific data */
    unsigned int landscape: 1;
    unsigned int use_lpr: 1;		/* else use lp */
    unsigned int print_to_file: 1;	/* Don't queue it */
    unsigned int do_color: 1;
    unsigned int do_transparent: 1;
    unsigned int eps: 1;
    float scale;
    float xwidth;		/* page size, in inches */
    float yheight;
    float lmargin, rmargin;	/* unused space, to be subtracted from size */
    float tmargin, bmargin;
    int16 num_copies;			/* -#3 (lpr)/ -n3 (lp) */
    int16 linear_thumb_cnt;
    char *printer_name;			/* -Pname (lpr)/ -dname (lp) */
    char *lpr_args;
    char *filename;			/* Temp file for lp(r), else user specified */
    char *tempname;			/* Temp file to be merged with above later */
    enum printer_units last_units;
} GPSDisplay;

extern void _GPSDraw_Image(GWindow, GImage *, GRect *src, int32 x, int32 y);
extern void _GPSDraw_TileImage(GWindow, GImage *, GRect *src, int32 x, int32 y);
extern void _GPSDraw_ImageMagnified(GWindow, GImage *, GRect *src, int32 x, int32 y, int32 width, int32 height);

extern void *_GPSDraw_LoadFontMetrics(GDisplay *gdisp, struct font_data *fd);
extern struct font_data *_GPSDraw_ScaleFont(GDisplay *gdisp, struct font_data *fd, FontRequest *rq);
extern struct font_data *_GPSDraw_StylizeFont(GDisplay *gdisp, struct font_data *fd, FontRequest *rq);
extern void _GPSDraw_ProcessFont(GPSWindow ps, struct font_data *fd);

extern double _GSPDraw_YPos(GPSWindow ps,int y);
extern double _GSPDraw_XPos(GPSWindow ps,int x);
extern double _GSPDraw_Distance(GPSWindow ps,int x);
extern void _GPSDraw_SetColor(GPSWindow ps,Color fg);
extern void _GPSDraw_CopyFile(FILE *to, FILE *from);
extern void _GPSDraw_ResetFonts(struct font_state *fonts);
extern void _GPSDraw_ListNeededFonts(GPSWindow ps);
extern int _GPSDraw_InitFonts(struct font_state *fonts);
extern void _GPSDraw_InitPatterns(GPSWindow);
extern void _GPSDraw_FlushPath(GPSWindow ps);
extern void _GPSDraw_SetClip(GPSWindow ps);
#endif
