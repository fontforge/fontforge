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
#ifndef _FONTFORGEGTK_H_
#define _FONTFORGEGTK_H_

#include <fontforge/fontforgevw.h>

extern void help(char *filename);

# include <gtk/gtk.h>
# include "gwwv.h"
# include "interface.h"
# include "callbacks.h"
# include "support.h"
# include "viewsgtk.h"
extern GdkCursor *ct_magplus, *ct_magminus, *ct_mypointer,
	*ct_circle, *ct_square, *ct_triangle, *ct_pen,
	*ct_ruler, *ct_knife, *ct_rotate, *ct_skew, *ct_scale, *ct_flip,
	*ct_3drotate, *ct_perspective,
	*ct_updown, *ct_leftright, *ct_nesw, *ct_nwse,
	*ct_rect, *ct_elipse, *ct_poly, *ct_star, *ct_filledrect, *ct_filledelipse,
	*ct_pencil, *ct_shift, *ct_line, *ct_myhand, *ct_setwidth,
	*ct_kerning, *ct_rbearing, *ct_lbearing, *ct_eyedropper,
	*ct_prohibition, *ct_ddcursor, *ct_spiroright, *ct_spiroleft;
extern GdkCursor *ct_pointer, *ct_backpointer, *ct_hand,
	*ct_question, *ct_cross, *ct_4way, *ct_text, *ct_watch, *ct_draganddrop,
	*ct_invisible;

extern int display_has_alpha;	/* Just a guess... */

extern void InitCursors(void);

extern int ErrorWindowExists(void);
extern void ShowErrorWindow(void);
extern struct ui_interface gtk_ui_interface;
extern struct prefs_interface gtk_prefs_interface;
/*extern struct sc_interface gdraw_sc_interface;*/
/*extern struct cv_interface gdraw_cv_interface;*/
/*extern struct bc_interface gdraw_bc_interface;*/
/*extern struct mv_interface gdraw_mv_interface;*/
extern struct fv_interface gtk_fv_interface;
/*extern struct fi_interface gdraw_fi_interface;*/
/*extern struct clip_interface gdraw_clip_interface;*/

extern int ItalicConstrained;
extern char *script_menu_names[SCRIPT_MENU_MAX];
extern char *script_filenames[SCRIPT_MENU_MAX];
extern char *RecentFiles[RECENT_MAX];

extern FontView *fv_list;

extern struct gwwv_filter def_font_filters[], *user_font_filters;
extern int default_font_filter_index;

/* The version of the library that the exe was built against */
extern Library_Version_Configuration exe_library_version_configuration;

#endif
