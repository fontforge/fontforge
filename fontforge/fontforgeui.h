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

#ifndef FONTFORGE_PFAEDITUI_H
#define FONTFORGE_PFAEDITUI_H

#include <fontforge-config.h>

#include "ffglib.h"
#include "fontforgevw.h"
#include "gprogress.h"

extern void help(const char *filename, const char *section);

# include "gdraw.h"
# include "ggadget.h"
# include "gwidget.h"
# include "views.h"

extern GCursor ct_magplus, ct_magminus, ct_mypointer,
	ct_circle, ct_square, ct_triangle, ct_pen, ct_hvcircle,
	ct_ruler, ct_knife, ct_rotate, ct_skew, ct_scale, ct_flip,
	ct_3drotate, ct_perspective,
	ct_updown, ct_leftright, ct_nesw, ct_nwse,
	ct_rect, ct_elipse, ct_poly, ct_star, ct_filledrect, ct_filledelipse,
	ct_pencil, ct_shift, ct_line, ct_myhand, ct_setwidth,
	ct_kerning, ct_rbearing, ct_lbearing, ct_eyedropper,
	ct_prohibition, ct_ddcursor, ct_spiroright, ct_spiroleft, ct_g2circle,
	ct_features;
extern GImage GIcon_midtangent, GIcon_midcurve, GIcon_midcorner, GIcon_midhvcurve;
extern GImage GIcon_tangent, GIcon_curve, GIcon_hvcurve, GIcon_corner, GIcon_ruler,
	GIcon_pointer, GIcon_magnify, GIcon_pen, GIcon_knife, GIcon_scale,
	GIcon_flip, GIcon_rotate, GIcon_skew,
	GIcon_3drotate, GIcon_perspective,
	GIcon_squarecap, GIcon_roundcap, GIcon_buttcap,
	GIcon_miterjoin, GIcon_roundjoin, GIcon_beveljoin,
	GIcon_rect, GIcon_elipse, GIcon_rrect, GIcon_poly, GIcon_star,
	GIcon_pencil, GIcon_shift, GIcon_line, GIcon_hand,
	GIcon_press2ptr, GIcon_freehand, GIcon_greyfree,
	GIcon_spirodisabled, GIcon_spiroup, GIcon_spirodown,
	GIcon_spirocurve, GIcon_spirocorner, GIcon_spirog2curve,
	GIcon_spiroleft, GIcon_spiroright;
extern  GImage GIcon_pointer_selected;
extern  GImage GIcon_pointer_selected;
extern  GImage GIcon_magnify_selected;
extern  GImage GIcon_freehand_selected;
extern  GImage GIcon_hand_selected;
extern  GImage GIcon_knife_selected;
extern  GImage GIcon_ruler_selected;
extern  GImage GIcon_pen_selected;
extern  GImage GIcon_spiroup_selected;
extern  GImage GIcon_spirocorner_selected;
extern  GImage GIcon_spirocurve_selected;
extern  GImage GIcon_spirog2curve_selected;
extern  GImage GIcon_spiroright_selected;
extern  GImage GIcon_spiroleft_selected;
extern  GImage GIcon_spirodisabled_selected;
extern  GImage GIcon_spirodown_selected;
extern  GImage GIcon_curve_selected;
extern  GImage GIcon_hvcurve_selected;
extern  GImage GIcon_corner_selected;
extern  GImage GIcon_tangent_selected;
extern  GImage GIcon_scale_selected;
extern  GImage GIcon_rotate_selected;
extern  GImage GIcon_flip_selected;
extern  GImage GIcon_skew_selected;
extern  GImage GIcon_3drotate_selected;
extern  GImage GIcon_perspective_selected;
extern  GImage GIcon_rect_selected;
extern  GImage GIcon_poly_selected;
extern  GImage GIcon_elipse_selected;
extern  GImage GIcon_star_selected;
extern  GImage GIcon_line_selected;
extern  GImage GIcon_pencil_selected;
extern  GImage GIcon_shift_selected;
extern  GImage GIcon_greyfree_selected;
  

extern GImage GIcon_smallskew, GIcon_smallscale, GIcon_smallrotate,
	GIcon_small3drotate, GIcon_smallperspective,
	GIcon_smallflip, GIcon_smalltangent, GIcon_smallcorner,
	GIcon_smallcurve, GIcon_smallmag, GIcon_smallknife, GIcon_smallpen,
	GIcon_smallhvcurve,
	GIcon_smallpointer, GIcon_smallruler, GIcon_smallelipse,
	GIcon_smallrect, GIcon_smallpoly, GIcon_smallstar,
	GIcon_smallpencil, GIcon_smallhand,
	GIcon_smallspirocurve, GIcon_smallspirocorner, GIcon_smallspirog2curve,
	GIcon_smallspiroleft, GIcon_smallspiroright,
	GIcon_FontForgeLogo, GIcon_FontForgeBack, GIcon_FontForgeGuide;
extern GImage GIcon_lock;
extern GImage GIcon_menumark;
extern GImage GIcon_rmoverlap, GIcon_exclude, GIcon_intersection, GIcon_findinter;
extern GImage GIcon_bold, GIcon_italic, GIcon_condense, GIcon_oblique, GIcon_smallcaps,
	GIcon_subsup, GIcon_changexheight, GIcon_styles;
extern GImage GIcon_outline, GIcon_inline, GIcon_shadow, GIcon_wireframe;
extern GImage GIcon_u45fItalic, GIcon_u452Italic,
    GIcon_u448Italic, GIcon_u446Italic, GIcon_u444Italic,
    GIcon_u442Italic, GIcon_u43fItalic, GIcon_u43cItalic, GIcon_u438Italic,
    GIcon_u436Italic, GIcon_u434Italic, GIcon_u433Italic, GIcon_u432Italic,
    GIcon_zItalic, GIcon_yItalic, GIcon_xItalic, GIcon_wItalic, GIcon_vItalic,
    GIcon_pItalic, GIcon_kItalic, GIcon_gItalic, GIcon_f2Italic, GIcon_fItalic,
    GIcon_aItalic, GIcon_FlatSerif, GIcon_SlantSerif, GIcon_PenSerif,
    GIcon_TopSerifs, GIcon_BottomSerifs, GIcon_DiagSerifs;
extern GImage def_image, red_image, blue_image, green_image, magenta_image,
	yellow_image, cyan_image, white_image, customcolor_image;
extern GImage GIcon_continue, GIcon_stepout, GIcon_stepover, GIcon_stepinto,
	GIcon_watchpnt, GIcon_menudelta, GIcon_exit;
extern GImage GIcon_Stopped, GIcon_Stop;
extern GWindow logo_icon;
extern GImage GIcon_sel2ptr, GIcon_rightpointer, GIcon_angle, GIcon_distance,
	GIcon_selectedpoint, GIcon_mag;
extern GImage GIcon_up, GIcon_down;
extern GImage OFL_logo;

extern GMenuItem2 cvtoollist[], cvspirotoollist[];

extern GTextInfo encodingtypes[];
extern GTextInfo *EncodingTypesFindEnc(GTextInfo *encodingtypes, Encoding *enc);
extern Encoding *ParseEncodingNameFromList(GGadget *listfield);
extern GTextInfo *GetEncodingTypes(void);
extern void cvtoollist_check(GWindow gw,struct gmenuitem *mi,GEvent *e);

extern void InitCursors(void);
extern void InitToolIconClut(Color bg);
extern void InitToolIcons(void); /* needs image cache already working */

extern int ErrorWindowExists(void);
extern void ShowErrorWindow(void);
extern struct ui_interface gdraw_ui_interface;
extern struct prefs_interface gdraw_prefs_interface;
extern struct sc_interface gdraw_sc_interface;
extern struct cv_interface gdraw_cv_interface;
extern struct bc_interface gdraw_bc_interface;
extern struct mv_interface gdraw_mv_interface;
extern struct fv_interface gdraw_fv_interface;
extern struct fi_interface gdraw_fi_interface;
extern struct clip_interface gdraw_clip_interface;

extern int ItalicConstrained;
extern unichar_t *script_menu_names[SCRIPT_MENU_MAX];
extern char *script_filenames[SCRIPT_MENU_MAX];

/* The number of files displayed in the "File->Recent" menu */
#define RECENT_MAX	10
extern char *RecentFiles[RECENT_MAX];

/* I would like these to be const ints, but gcc doesn't treat them as consts */
#define et_sb_halfup et_sb_thumbrelease+1
#define et_sb_halfdown  et_sb_thumbrelease+2

extern FontView *fv_list;

extern struct openfilefilters { char *name, *filter; } def_font_filters[], *user_font_filters;
extern int default_font_filter_index;

#define SERIF_UI_FAMILIES	"dejavu serif,times,caslon,serif,clearlyu,unifont,unifont upper"
#define SANS_UI_FAMILIES	"dejavu sans,helvetica,caliban,sans,clearlyu,unifont,unifont upper"
#define MONO_UI_FAMILIES	"courier,monospace,clearlyu,unifont,unifont upper"
#define FIXED_UI_FAMILIES	"monospace,fixed,clearlyu,unifont,unifont upper"

#define isprivateuse(enc) ((enc)>=0xe000 && (enc)<=0xf8ff)
#define issurrogate(enc) ((enc)>=0xd800 && (enc)<=0xd8ff)

#endif /* FONTFORGE_PFAEDITUI_H */
