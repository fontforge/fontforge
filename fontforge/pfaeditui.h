/* Copyright (C) 2000-2007 by George Williams */
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
#ifndef _PFAEDITUI_H_
#define _PFAEDITUI_H_

#include "pfaedit.h"

extern void help(char *filename);
extern int FontForgeMain( int argc, char **argv );

#if defined(FONTFORGE_CONFIG_GTK)
# include <gtk/gtk.h>
# include "interface.h"
# include "callbacks.h"
# include "support.h"
# include "views.h"
extern GdkCursor *ct_magplus, *ct_magminus, *ct_mypointer,
	*ct_circle, *ct_square, *ct_triangle, *ct_pen,
	*ct_ruler, *ct_knife, *ct_rotate, *ct_skew, *ct_scale, *ct_flip,
	*ct_3drotate, *ct_perspective,
	*ct_updown, *ct_leftright, *ct_nesw, *ct_nwse,
	*ct_rect, *ct_elipse, *ct_poly, *ct_star, *ct_filledrect, *ct_filledelipse,
	*ct_pencil, *ct_shift, *ct_line, *ct_myhand, *ct_setwidth,
	*ct_kerning, *ct_rbearing, *ct_lbearing, *ct_eyedropper,
	*ct_prohibition, *ct_ddcursor;

/*
extern GImage GIcon_midtangent, GIcon_midcurve, GIcon_midcorner;
extern GImage GIcon_tangent, GIcon_curve, GIcon_corner, GIcon_ruler,
	GIcon_pointer, GIcon_magnify, GIcon_pen, GIcon_knife, GIcon_scale,
	GIcon_flip, GIcon_rotate, GIcon_skew,
	GIcon_squarecap, GIcon_roundcap, GIcon_buttcap,
	GIcon_miterjoin, GIcon_roundjoin, GIcon_beveljoin,
	GIcon_rect, GIcon_elipse, GIcon_rrect, GIcon_poly, GIcon_star,
	GIcon_pencil, GIcon_shift, GIcon_line, GIcon_hand,
	GIcon_press2ptr, GIcon_freehand, GIcon_greyfree;
extern GImage GIcon_smallskew, GIcon_smallscale, GIcon_smallrotate,
	GIcon_smallflip, GIcon_smalltangent, GIcon_smallcorner,
	GIcon_smallcurve, GIcon_smallmag, GIcon_smallknife, GIcon_smallpen,
	GIcon_smallpointer, GIcon_smallruler, GIcon_smallelipse,
	GIcon_smallrect, GIcon_smallpoly, GIcon_smallstar,
	GIcon_smallpencil, GIcon_smallhand,
	GIcon_FontForgeLogo, GIcon_FontForgeBack, GIcon_FontForgeGuide;
extern GImage GIcon_sel2ptr, GIcon_rightpointer, GIcon_angle, GIcon_distance,
	GIcon_selectedpoint, GIcon_mag;
extern GImage GIcon_menumark;
extern GImage GIcon_rmoverlap, GIcon_exclude, GIcon_intersection, GIcon_findinter;
extern GImage GIcon_outline, GIcon_inline, GIcon_shadow, GIcon_wireframe;
extern GImage def_image, red_image, blue_image, green_image, magenta_image,
	yellow_image, cyan_image, white_image;
extern GImage GIcon_continue, GIcon_stepout, GIcon_stepover, GIcon_stepinto,
	GIcon_watchpnt, GIcon_menudelta, GIcon_exit;
extern GImage GIcon_Stopped, GIcon_Stop;
extern GWindow logo_icon;

extern GTextInfo encodingtypes[];
extern GTextInfo *EncodingTypesFindEnc(GTextInfo *encodingtypes, Encoding *enc);
extern Encoding *ParseEncodingNameFromList(GGadget *listfield);
extern GTextInfo *GetEncodingTypes(void);
*/

extern void InitCursors(void);

/*
extern real GetReal(GWindow gw,int cid,char *name,int *err);
extern int GetInt(GWindow gw,int cid,char *name,int *err);
extern void Protest(char *label);
extern real GetCalmReal8(GWindow gw,int cid,char *namer,int *err);
extern real GetReal8(GWindow gw,int cid,char *namer,int *err);
extern int GetInt8(GWindow gw,int cid,char *namer,int *err);
extern void Protest8(char *labelr);
*/

/* I would like these to be const ints, but gcc doesn't treat them as consts */
/*
#define et_sb_halfup et_sb_thumbrelease+1
#define et_sb_halfdown  et_sb_thumbrelease+2
*/

#elif defined( FONTFORGE_CONFIG_GDRAW )

# include "gdraw.h"
# include "gwidget.h"
# include "ggadget.h"
# include "views.h"

extern GCursor ct_magplus, ct_magminus, ct_mypointer,
	ct_circle, ct_square, ct_triangle, ct_pen,
	ct_ruler, ct_knife, ct_rotate, ct_skew, ct_scale, ct_flip,
	ct_3drotate, ct_perspective,
	ct_updown, ct_leftright, ct_nesw, ct_nwse,
	ct_rect, ct_elipse, ct_poly, ct_star, ct_filledrect, ct_filledelipse,
	ct_pencil, ct_shift, ct_line, ct_myhand, ct_setwidth,
	ct_kerning, ct_rbearing, ct_lbearing, ct_eyedropper,
	ct_prohibition, ct_ddcursor;
extern GImage GIcon_midtangent, GIcon_midcurve, GIcon_midcorner;
extern GImage GIcon_tangent, GIcon_curve, GIcon_corner, GIcon_ruler,
	GIcon_pointer, GIcon_magnify, GIcon_pen, GIcon_knife, GIcon_scale,
	GIcon_flip, GIcon_rotate, GIcon_skew,
	GIcon_3drotate, GIcon_perspective,
	GIcon_squarecap, GIcon_roundcap, GIcon_buttcap,
	GIcon_miterjoin, GIcon_roundjoin, GIcon_beveljoin,
	GIcon_rect, GIcon_elipse, GIcon_rrect, GIcon_poly, GIcon_star,
	GIcon_pencil, GIcon_shift, GIcon_line, GIcon_hand,
	GIcon_press2ptr, GIcon_freehand, GIcon_greyfree;
extern GImage GIcon_smallskew, GIcon_smallscale, GIcon_smallrotate,
	GIcon_small3drotate, GIcon_smallperspective,
	GIcon_smallflip, GIcon_smalltangent, GIcon_smallcorner,
	GIcon_smallcurve, GIcon_smallmag, GIcon_smallknife, GIcon_smallpen,
	GIcon_smallpointer, GIcon_smallruler, GIcon_smallelipse,
	GIcon_smallrect, GIcon_smallpoly, GIcon_smallstar,
	GIcon_smallpencil, GIcon_smallhand,
	GIcon_FontForgeLogo, GIcon_FontForgeBack, GIcon_FontForgeGuide;
extern GImage GIcon_lock;
extern GImage GIcon_menumark;
extern GImage GIcon_rmoverlap, GIcon_exclude, GIcon_intersection, GIcon_findinter;
extern GImage GIcon_outline, GIcon_inline, GIcon_shadow, GIcon_wireframe;
extern GImage def_image, red_image, blue_image, green_image, magenta_image,
	yellow_image, cyan_image, white_image;
extern GImage GIcon_continue, GIcon_stepout, GIcon_stepover, GIcon_stepinto,
	GIcon_watchpnt, GIcon_menudelta, GIcon_exit;
extern GImage GIcon_Stopped, GIcon_Stop;
extern GWindow logo_icon;
extern GImage GIcon_sel2ptr, GIcon_rightpointer, GIcon_angle, GIcon_distance,
	GIcon_selectedpoint, GIcon_mag;
extern GImage GIcon_up, GIcon_down;
extern GImage OFL_logo;

extern GTextInfo encodingtypes[];
extern GTextInfo *EncodingTypesFindEnc(GTextInfo *encodingtypes, Encoding *enc);
extern Encoding *ParseEncodingNameFromList(GGadget *listfield);
extern GTextInfo *GetEncodingTypes(void);

extern void InitCursors(void);
extern void InitToolIconClut(Color bg);

extern real GetReal(GWindow gw,int cid,char *name,int *err);
extern int GetInt(GWindow gw,int cid,char *name,int *err);
extern void Protest(char *label);
extern real GetCalmReal8(GWindow gw,int cid,char *namer,int *err);
extern real GetReal8(GWindow gw,int cid,char *namer,int *err);
extern int GetInt8(GWindow gw,int cid,char *namer,int *err);
extern void Protest8(char *labelr);

/* I would like these to be const ints, but gcc doesn't treat them as consts */
#define et_sb_halfup et_sb_thumbrelease+1
#define et_sb_halfdown  et_sb_thumbrelease+2
#else
# include "views.h"
#endif

extern FontView *fv_list;

extern struct openfilefilters { char *name, *filter; } def_font_filters[], *user_font_filters;
extern int default_font_filter_index;

#if !defined( FONTFORGE_CONFIG_CAPSLOCK_FOR_ALT ) || FONTFORGE_CONFIG_CAPSLOCK_FOR_ALT==0
# define ksm_alt	ksm_meta
#elif FONTFORGE_CONFIG_CAPSLOCK_FOR_ALT-2 == 0	/* I use this peculiar construction just in case it is defined as the empty string */
# define ksm_alt	(ksm_meta|ksm_capslock)
#else
# define ksm_alt	ksm_capslock
#endif

#endif
