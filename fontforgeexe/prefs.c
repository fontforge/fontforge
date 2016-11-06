/* -*- coding: utf-8 -*- */
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
#include "autotrace.h"
#include "fontforgeui.h"
#include "groups.h"
#include "plugins.h"
#include <charset.h>
#include <gfile.h>
#include <gresource.h>
#include <gresedit.h>
#include <ustring.h>
#include <gkeysym.h>

#include <sys/types.h>
#include <dirent.h>
#include <locale.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#include "ttf.h"

#if HAVE_LANGINFO_H
# include <langinfo.h>
#endif

#define GTimer GTimer_GTK
#include <glib.h>
#undef GTimer

#include "collabclient.h"

#define RAD2DEG	(180/3.1415926535897932)

static void change_res_filename(const char *newname);

#include "gutils/prefs.h"

extern int splash;
extern int adjustwidth;
extern int adjustlbearing;
extern Encoding *default_encoding;
extern int autohint_before_generate;
extern int use_freetype_to_rasterize_fv;
extern int use_freetype_with_aa_fill_cv;
extern int OpenCharsInNewWindow;
extern int ItalicConstrained;
extern int accent_offset;
extern int GraveAcuteCenterBottom;
extern int PreferSpacingAccents;
extern int CharCenterHighest;
extern int ask_user_for_resolution;
extern int stop_at_join;
extern int recognizePUA;
extern float arrowAmount;
extern float arrowAccelFactor;
extern float snapdistance;
extern float snapdistancemeasuretool;
extern int measuretoolshowhorizontolvertical;
extern int xorrubberlines;
extern int snaptoint;
extern float joinsnap;
extern char *BDFFoundry;
extern char *TTFFoundry;
extern char *xuid;
extern char *SaveTablesPref;
/*struct cvshows CVShows = { 1, 1, 1, 1, 1, 0, 1 };*/ /* in charview */
/* int default_fv_font_size = 24; */	/* in fontview */
/* int default_fv_antialias = false */	/* in fontview */
/* int default_fv_bbsized = false */	/* in fontview */
extern int default_fv_row_count;	/* in fontview */
extern int default_fv_col_count;	/* in fontview */
extern int default_fv_showhmetrics;	/* in fontview */
extern int default_fv_showvmetrics;	/* in fontview */
extern int default_fv_glyphlabel;	/* in fontview */
extern int save_to_dir;			/* in fontview, use sfdir rather than sfd */
extern int palettes_docked;		/* in cvpalettes */
extern int cvvisible[2], bvvisible[3];	/* in cvpalettes.c */
extern int maxundoes;			/* in cvundoes */
extern int pref_mv_shift_and_arrow_skip;         /* in metricsview.c */
extern int pref_mv_control_shift_and_arrow_skip; /* in metricsview.c */
extern int mv_type;                              /* in metricsview.c */
extern int prefer_cjk_encodings;	/* in parsettf */
extern int onlycopydisplayed, copymetadata, copyttfinstr;
extern struct cvshows CVShows;
extern int infowindowdistance;		/* in cvruler.c */
extern int oldformatstate;		/* in savefontdlg.c */
extern int oldbitmapstate;		/* in savefontdlg.c */
static int old_ttf_flags=0, old_otf_flags=0;
extern int old_sfnt_flags;		/* in savefont.c */
extern int old_ps_flags;		/* in savefont.c */
extern int old_validate;		/* in savefontdlg.c */
extern int old_fontlog;			/* in savefontdlg.c */
extern int oldsystem;			/* in bitmapdlg.c */
extern int preferpotrace;		/* in autotrace.c */
extern int autotrace_ask;		/* in autotrace.c */
extern int mf_ask;			/* in autotrace.c */
extern int mf_clearbackgrounds;		/* in autotrace.c */
extern int mf_showerrors;		/* in autotrace.c */
extern char *mf_args;			/* in autotrace.c */
static int glyph_2_name_map=0;		/* was in tottf.c, now a flag in savefont options dlg */
extern int coverageformatsallowed;	/* in tottfgpos.c */
extern int debug_wins;			/* in cvdebug.c */
extern int gridfit_dpi, gridfit_depth;	/* in cvgridfit.c */
extern float gridfit_pointsizey;	/* in cvgridfit.c */
extern float gridfit_pointsizex;	/* in cvgridfit.c */
extern int gridfit_x_sameas_y;		/* in cvgridfit.c */
extern int hint_diagonal_ends;		/* in stemdb.c */
extern int hint_diagonal_intersections;	/* in stemdb.c */
extern int hint_bounding_boxes;		/* in stemdb.c */
extern int detect_diagonal_stems;	/* in stemdb.c */
extern float stem_slope_error;		/* in stemdb.c */
extern float stub_slope_error;		/* in stemdb.c */
extern int instruct_diagonal_stems;	/* in nowakowskittfinstr.c */
extern int instruct_serif_stems;		/* in nowakowskittfinstr.c */
extern int instruct_ball_terminals;	/* in nowakowskittfinstr.c */
extern int interpolate_strong;		/* in nowakowskittfinstr.c */
extern int control_counters;		/* in nowakowskittfinstr.c */
extern unichar_t *script_menu_names[SCRIPT_MENU_MAX];
extern char *script_filenames[SCRIPT_MENU_MAX];
static char *xdefs_filename;
extern int new_em_size;				/* in splineutil2.c */
extern int new_fonts_are_order2;		/* in splineutil2.c */
extern int loaded_fonts_same_as_new;		/* in splineutil2.c */
extern int use_second_indic_scripts;		/* in tottfgpos.c */
static char *othersubrsfile = NULL;
extern MacFeat *default_mac_feature_map,	/* from macenc.c */
		*user_mac_feature_map;
extern int updateflex;				/* in charview.c */
extern int default_autokern_dlg;		/* in lookupui.c */
extern int allow_utf8_glyphnames;		/* in lookupui.c */
extern int add_char_to_name_list;		/* in charinfo.c */
extern int clear_tt_instructions_when_needed;	/* in cvundoes.c */
extern int export_clipboard;			/* in cvundoes.c */
extern int cv_width;			/* in charview.c */
extern int cv_height;			/* in charview.c */
extern int cv_show_fill_with_space; /* in charview.c */
extern int interpCPsOnMotion;			/* in charview.c */
extern int DrawOpenPathsWithHighlight;          /* in charview.c */
extern float prefs_cvEditHandleSize;            /* in charview.c */
extern int prefs_cvInactiveHandleAlpha;         /* in charview.c */
extern int mv_width;				/* in metricsview.c */
extern int mv_height;				/* in metricsview.c */
extern int bv_width;				/* in bitmapview.c */
extern int bv_height;				/* in bitmapview.c */
extern int ask_user_for_cmap;			/* in parsettf.c */
extern int mvshowgrid;				/* in metricsview.c */

extern int rectelipse, polystar, regular_star;	/* from cvpalettes.c */
extern int center_out[2];			/* from cvpalettes.c */
extern float rr_radius;				/* from cvpalettes.c */
extern int ps_pointcnt;				/* from cvpalettes.c */
extern float star_percent;			/* from cvpalettes.c */
extern int home_char;				/* from fontview.c */
extern int compact_font_on_open;		/* from fontview.c */
extern int aa_pixelsize;			/* from anchorsaway.c */
extern enum cvtools cv_b1_tool, cv_cb1_tool, cv_b2_tool, cv_cb2_tool; /* cvpalettes.c */
extern int show_kerning_pane_in_class;		/* kernclass.c */
extern int AutoSaveFrequency;			/* autosave.c */
extern int UndoRedoLimitToSave;  /* sfd.c */
extern int UndoRedoLimitToLoad;  /* sfd.c */
extern int prefRevisionsToRetain; /* sfd.c */
extern int prefs_cv_show_control_points_always_initially; /* from charview.c */
extern int prefs_create_dragging_comparison_outline;      /* from charview.c */
extern int prefs_cv_outline_thickness; /* from charview.c */

extern char *pref_collab_last_server_connected_to; /* in collabclient.c */

extern float OpenTypeLoadHintEqualityTolerance;  /* autohint.c */
extern float GenerateHintWidthEqualityTolerance; /* splinesave.c */
extern NameList *force_names_when_opening;
extern NameList *force_names_when_saving;
extern NameList *namelist_for_new_fonts;

extern int default_font_filter_index;
extern struct openfilefilters *user_font_filters;
static int alwaysgenapple=false, alwaysgenopentype=false;

static int gfc_showhidden, gfc_dirplace;
static char *gfc_bookmarks=NULL;

static int prefs_usecairo = true;

static int pointless;

    /* These first three must match the values in macenc.c */
#define CID_Features	101
#define CID_FeatureDel	103
#define CID_FeatureEdit	105

#define CID_Mapping	102
#define CID_MappingDel	104
#define CID_MappingEdit	106

#define CID_ScriptMNameBase	200
#define CID_ScriptMFileBase	(200+SCRIPT_MENU_MAX)
#define CID_ScriptMBrowseBase	(200+2*SCRIPT_MENU_MAX)

#define CID_PrefsBase	1000
#define CID_PrefsOffset	100
#define CID_PrefsBrowseOffset	(CID_PrefsOffset/2)

//////////////////////////////////
// The _oldval_ are used to cache the setting when the prefs window
// is created so that a redraw can be performed only when the
// value has changed.
float prefs_oldval_cvEditHandleSize = 0;
int   prefs_oldval_cvInactiveHandleAlpha = 0;

/* ************************************************************************** */
/* *****************************    mac data    ***************************** */
/* ************************************************************************** */

extern struct macsettingname macfeat_otftag[], *user_macfeat_otftag;

static void UserSettingsFree(void) {

    free( user_macfeat_otftag );
    user_macfeat_otftag = NULL;
}

static int UserSettingsDiffer(void) {
    int i,j;

    if ( user_macfeat_otftag==NULL )
return( false );

    for ( i=0; user_macfeat_otftag[i].otf_tag!=0; ++i );
    for ( j=0; macfeat_otftag[j].otf_tag!=0; ++j );
    if ( i!=j )
return( true );
    for ( i=0; user_macfeat_otftag[i].otf_tag!=0; ++i ) {
	for ( j=0; macfeat_otftag[j].otf_tag!=0; ++j ) {
	    if ( macfeat_otftag[j].mac_feature_type ==
		    user_macfeat_otftag[i].mac_feature_type &&
		    macfeat_otftag[j].mac_feature_setting ==
		    user_macfeat_otftag[i].mac_feature_setting &&
		    macfeat_otftag[j].otf_tag ==
		    user_macfeat_otftag[i].otf_tag )
	break;
	}
	if ( macfeat_otftag[j].otf_tag==0 )
return( true );
    }
return( false );
}

/**************************************************************************** */


/* don't use mnemonics 'C' or 'O' (Cancel & OK) */
enum pref_types { pr_int, pr_real, pr_bool, pr_enum, pr_encoding, pr_string,
	pr_file, pr_namelist, pr_unicode, pr_angle };
struct enums { char *name; int value; };

struct enums fvsize_enums[] = { {NULL, 0} };

#define PREFS_LIST_EMPTY { NULL, 0, NULL, NULL, NULL, '\0', NULL, 0, NULL }
static struct prefs_list {
    char *name;
    	/* In the prefs file the untranslated name will always be used, but */
	/* in the UI that name may be translated. */
    enum pref_types type;
    void *val;
    void *(*get)(void);
    void (*set)(void *);
    char mn;
    struct enums *enums;
    unsigned int dontdisplay: 1;
    char *popup;
} general_list[] = {
/* GT: The following strings have no spaces and an odd capitalization */
/* GT: this is because these strings are used in two different ways, one */
/* GT: translated (which the user sees, and should probably have added spaces,*/
/* GT: and one untranslated which needs the current odd format */
	{ N_("ResourceFile"), pr_file, &xdefs_filename, NULL, NULL, 'R', NULL, 0, N_("When FontForge starts up, it loads the user interface theme from\nthis file. Any changes will only take effect the next time you start FontForge.") },
	{ N_("OtherSubrsFile"), pr_file, &othersubrsfile, NULL, NULL, 'O', NULL, 0, N_("If you wish to replace Adobe's OtherSubrs array (for Type1 fonts)\nwith an array of your own, set this to point to a file containing\na list of up to 14 PostScript subroutines. Each subroutine must\nbe preceded by a line starting with '%%%%' (any text before the\nfirst '%%%%' line will be treated as an initial copyright notice).\nThe first three subroutines are for flex hints, the next for hint\nsubstitution (this MUST be present), the 14th (or 13 as the\nnumbering actually starts with 0) is for counter hints.\nThe subroutines should not be enclosed in a [ ] pair.") },
	{ N_("FreeTypeInFontView"), pr_bool, &use_freetype_to_rasterize_fv, NULL, NULL, 'O', NULL, 0, N_("Use the FreeType rasterizer (when available)\nto rasterize glyphs in the font view.\nThis generally results in better quality.") },
	{ N_("FreeTypeAAFillInOutlineView"), pr_bool, &use_freetype_with_aa_fill_cv, NULL, NULL, 'O', NULL, 0, N_("When filling using freetype in the outline view,\nhave freetype render the glyph antialiased.") },
	{ N_("SplashScreen"), pr_bool, &splash, NULL, NULL, 'S', NULL, 0, N_("Show splash screen on start-up") },
#ifndef _NO_LIBCAIRO
	{ N_("UseCairoDrawing"), pr_bool, &prefs_usecairo, NULL, NULL, '\0', NULL, 0, N_("Use the cairo library for drawing (if available)\nThis makes for prettier (anti-aliased) but slower drawing\nThis applies to any windows created AFTER this is set.\nAlready existing windows will continue as they are.") },
#endif
	{ N_("ExportClipboard"), pr_bool, &export_clipboard, NULL, NULL, '\0', NULL, 0, N_( "If you are running an X11 clipboard manager you might want\nto turn this off. FF can put things into its internal clipboard\nwhich it cannot export to X11 (things like copying more than\none glyph in the fontview). If you have a clipboard manager\nrunning it will force these to be exported with consequent\nloss of data.") },
	{ N_("AutoSaveFrequency"), pr_int, &AutoSaveFrequency, NULL, NULL, '\0', NULL, 0, N_( "The number of seconds between autosaves. If you set this to 0 there will be no autosaves.") },
	{ N_("RevisionsToRetain"), pr_int, &prefRevisionsToRetain, NULL, NULL, '\0', NULL, 0, N_( "When Saving, keep this number of previous versions of the file. file.sfd-01 will be the last saved file, file.sfd-02 will be the file saved before that, and so on. If you set this to 0 then no revisions will be retained.") },
	{ N_("UndoRedoLimitToSave"), pr_int, &UndoRedoLimitToSave, NULL, NULL, '\0', NULL, 0, N_( "The number of undo and redo operations which will be saved in sfd files.\nIf you set this to 0 undo/redo information is not saved to sfd files.\nIf set to -1 then all available undo/redo information is saved without limit.") },
	PREFS_LIST_EMPTY
},
  new_list[] = {
	{ N_("NewCharset"), pr_encoding, &default_encoding, NULL, NULL, 'N', NULL, 0, N_("Default encoding for\nnew fonts") },
	{ N_("NewEmSize"), pr_int, &new_em_size, NULL, NULL, 'S', NULL, 0, N_("The default size of the Em-Square in a newly created font.") },
	{ N_("NewFontsQuadratic"), pr_bool, &new_fonts_are_order2, NULL, NULL, 'Q', NULL, 0, N_("Whether new fonts should contain splines of quadratic (truetype)\nor cubic (postscript & opentype).") },
	{ N_("LoadedFontsAsNew"), pr_bool, &loaded_fonts_same_as_new, NULL, NULL, 'L', NULL, 0, N_("Whether fonts loaded from the disk should retain their splines\nwith the original order (quadratic or cubic), or whether the\nsplines should be converted to the default order for new fonts\n(see NewFontsQuadratic).") },
	PREFS_LIST_EMPTY
},
  open_list[] = {
	{ N_("PreferCJKEncodings"), pr_bool, &prefer_cjk_encodings, NULL, NULL, 'C', NULL, 0, N_("When loading a truetype or opentype font which has both a unicode\nand a CJK encoding table, use this flag to specify which\nshould be loaded for the font.") },
	{ N_("AskUserForCMap"), pr_bool, &ask_user_for_cmap, NULL, NULL, 'O', NULL, 0, N_("When loading a font in sfnt format (TrueType, OpenType, etc.),\nask the user to specify which cmap to use initially.") },
	{ N_("PreserveTables"), pr_string, &SaveTablesPref, NULL, NULL, 'P', NULL, 0, N_("Enter a list of 4 letter table tags, separated by commas.\nFontForge will make a binary copy of these tables when it\nloads a True/OpenType font, and will output them (unchanged)\nwhen it generates the font. Do not include table tags which\nFontForge thinks it understands.") },
	{ N_("SeekCharacter"), pr_unicode, &home_char, NULL, NULL, '\0', NULL, 0, N_("When fontforge opens a (non-sfd) font it will try to display this unicode character in the fontview.")},
	{ N_("CompactOnOpen"), pr_bool, &compact_font_on_open, NULL, NULL, 'O', NULL, 0, N_("When a font is opened, should it be made compact?")},
	{ N_("UndoRedoLimitToLoad"), pr_int, &UndoRedoLimitToLoad, NULL, NULL, '\0', NULL, 0, N_( "The number of undo and redo operations to load from sfd files.\nWith this option you can disregard undo information while loading SFD files.\nIf set to 0 then no undo/redo information is loaded.\nIf set to -1 then all available undo/redo information is loaded without limit.") },
	{ N_("OpenTypeLoadHintEqualityTolerance"), pr_real, &OpenTypeLoadHintEqualityTolerance, NULL, NULL, '\0', NULL, 0, N_( "When importing an OpenType font, for the purposes of hinting spline points might not exactly match boundaries. For example, a point might be -0.0002 instead of exactly 0\nThis setting gives the user some control over this allowing a small tolerance value to be fed into the OpenType loading code.\nComparisons are then not performed for raw equality but for equality within tolerance (e.g., values within the range -0.0002 to 0.0002 will be considered equal to 0 when figuring out hints).") },
	PREFS_LIST_EMPTY
},
  navigation_list[] = {
	{ N_("GlyphAutoGoto"), pr_bool, &cv_auto_goto, NULL, NULL, '\0', NULL, 0, N_("Typing a normal character in the glyph view window changes the window to look at that character.\nEnabling GlyphAutoGoto will disable the shortcut where holding just the ` key will enable Preview mode as long as the key is held.") },
	{ N_("OpenCharsInNewWindow"), pr_bool, &OpenCharsInNewWindow, NULL, NULL, '\0', NULL, 0, N_("When double clicking on a character in the font view\nopen that character in a new window, otherwise\nreuse an existing one.") },
	PREFS_LIST_EMPTY
},
  editing_list[] = {
	{ N_("ItalicConstrained"), pr_bool, &ItalicConstrained, NULL, NULL, '\0', NULL, 0, N_("In the Outline View, the Shift key constrains motion to be parallel to the ItalicAngle rather than constraining it to be vertical.") },
	{ N_("InterpolateCPsOnMotion"), pr_bool, &interpCPsOnMotion, NULL, NULL, '\0', NULL, 0, N_("When moving one end point of a spline but not the other\ninterpolate the control points between the two.") },
	{ N_("SnapDistance"), pr_real, &snapdistance, NULL, NULL, '\0', NULL, 0, N_("When the mouse pointer is within this many pixels\nof one of the various interesting features (baseline,\nwidth, grid splines, etc.) the pointer will snap\nto that feature.") },
	{ N_("SnapDistanceMeasureTool"), pr_real, &snapdistancemeasuretool, NULL, NULL, '\0', NULL, 0, N_("When the measure tool is active and when the mouse pointer is within this many pixels\nof one of the various interesting features (baseline,\nwidth, grid splines, etc.) the pointer will snap\nto that feature.") },
	{ N_("SnapToInt"), pr_bool, &snaptoint, NULL, NULL, '\0', NULL, 0, N_("When the user clicks in the editing window, round the location to the nearest integers.") },
	{ N_("StopAtJoin"), pr_bool, &stop_at_join, NULL, NULL, '\0', NULL, 0, N_("When dragging points in the outline view a join may occur\n(two open contours may connect at their endpoints). When\nthis is On a join will cause FontForge to stop moving the\nselection (as if the user had released the mouse button).\nThis is handy if your fingers are inclined to wiggle a bit.") },
	{ N_("JoinSnap"), pr_real, &joinsnap, NULL, NULL, '\0', NULL, 0, N_("The Edit->Join command will join points which are this close together\nA value of 0 means they must be coincident") },
	{ N_("CopyMetaData"), pr_bool, &copymetadata, NULL, NULL, '\0', NULL, 0, N_("When copying glyphs from the font view, also copy the\nglyphs' metadata (name, encoding, comment, etc).") },
	{ N_("UndoDepth"), pr_int, &maxundoes, NULL, NULL, '\0', NULL, 0, N_("The maximum number of Undoes/Redoes stored in a glyph. Use -1 for infinite Undoes\n(but watch RAM consumption and use the Edit menu's Remove Undoes as needed)") },
	{ N_("UpdateFlex"), pr_bool, &updateflex, NULL, NULL, '\0', NULL, 0, N_("Figure out flex hints after every change") },
	{ N_("AutoKernDialog"), pr_bool, &default_autokern_dlg, NULL, NULL, '\0', NULL, 0, N_("Open AutoKern dialog for new kerning subtables") },
	{ N_("MetricsShiftSkip"), pr_int, &pref_mv_shift_and_arrow_skip, NULL, NULL, '\0', NULL, 0, N_("Number of units to increment/decrement a table value by in the metrics window when shift is held") },
	{ N_("MetricsControlShiftSkip"), pr_int, &pref_mv_control_shift_and_arrow_skip, NULL, NULL, '\0', NULL, 0, N_("Number of units to increment/decrement a table value by in the metrics window when both control and shift is held") },
	PREFS_LIST_EMPTY
},
  editing_interface_list[] = {
	{ N_("ArrowMoveSize"), pr_real, &arrowAmount, NULL, NULL, '\0', NULL, 0, N_("The number of em-units by which an arrow key will move a selected point") },
	{ N_("ArrowAccelFactor"), pr_real, &arrowAccelFactor, NULL, NULL, '\0', NULL, 0, N_("Holding down the Shift key will speed up arrow key motion by this factor") },
	{ N_("DrawOpenPathsWithHighlight"), pr_bool, &DrawOpenPathsWithHighlight, NULL, NULL, '\0', NULL, 0, N_("Open paths should be drawn in a special highlight color to make them more apparent.") },
	{ N_("MeasureToolShowHorizontalVertical"), pr_bool, &measuretoolshowhorizontolvertical, NULL, NULL, '\0', NULL, 0, N_("Have the measure tool show horizontal and vertical distances on the canvas.") },
	{ N_("XORRubberLines"), pr_bool, &xorrubberlines, NULL, NULL, '\0', NULL, 0, N_("Use XOR based rubber lines.") },
	{ N_("EditHandleSize"), pr_real, &prefs_cvEditHandleSize, NULL, NULL, '\0', NULL, 0, N_("The size of the handles showing control points and other interesting points in the glyph editor (default is 5).") },
	{ N_("InactiveHandleAlpha"), pr_int, &prefs_cvInactiveHandleAlpha, NULL, NULL, '\0', NULL, 0, N_("Inactive handles in the glyph editor will be drawn with this alpha value (range: 0-255 default is 255).") },
	{ N_("ShowControlPointsAlways"), pr_bool, &prefs_cv_show_control_points_always_initially, NULL, NULL, '\0', NULL, 0, N_("Always show the control points when editing a glyph.\nThis can be turned off in the menu View/Show, this setting will effect if control points are shown initially.\nChange requires a restart of fontforge.") },
	{ N_("ShowFillWithSpace"), pr_bool, &cv_show_fill_with_space, NULL, NULL, '\0', NULL, 0, N_("Also enable preview mode when the space bar is pressed.") },
	{ N_("OutlineThickness"), pr_int, &prefs_cv_outline_thickness, NULL, NULL, '\0', NULL, 0, N_("Setting above 1 will cause a thick outline to be drawn for glyph paths\n which is only extended inwards from the edge of the glyph.\n See also the ForegroundThickOutlineColor Resource for the color of this outline.") },
	PREFS_LIST_EMPTY
},
  sync_list[] = {
	{ N_("AutoWidthSync"), pr_bool, &adjustwidth, NULL, NULL, '\0', NULL, 0, N_("Changing the width of a glyph\nchanges the widths of all accented\nglyphs based on it.") },
	{ N_("AutoLBearingSync"), pr_bool, &adjustlbearing, NULL, NULL, '\0', NULL, 0, N_("Changing the left side bearing\nof a glyph adjusts the lbearing\nof other references in all accented\nglyphs based on it.") },
	PREFS_LIST_EMPTY
},
 tt_list[] = {
	{ N_("ClearInstrsBigChanges"), pr_bool, &clear_tt_instructions_when_needed, NULL, NULL, 'C', NULL, 0, N_("Instructions in a TrueType font refer to\npoints by number, so if you edit a glyph\nin such a way that some points have different\nnumbers (add points, remove them, etc.) then\nthe instructions will be applied to the wrong\npoints with disasterous results.\n  Normally FontForge will remove the instructions\nif it detects that the points have been renumbered\nin order to avoid the above problem. You may turn\nthis behavior off -- but be careful!") },
	{ N_("CopyTTFInstrs"), pr_bool, &copyttfinstr, NULL, NULL, '\0', NULL, 0, N_("When copying glyphs from the font view, also copy the\nglyphs' truetype instructions.") },
	PREFS_LIST_EMPTY
},
  accent_list[] = {
	{ N_("AccentOffsetPercent"), pr_int, &accent_offset, NULL, NULL, '\0', NULL, 0, N_("The percentage of an em by which an accent is offset from its base glyph in Build Accent") },
	{ N_("AccentCenterLowest"), pr_bool, &GraveAcuteCenterBottom, NULL, NULL, '\0', NULL, 0, N_("When placing grave and acute accents above letters, should\nFontForge center them based on their full width, or\nshould it just center based on the lowest point\nof the accent.") },
	{ N_("CharCenterHighest"), pr_bool, &CharCenterHighest, NULL, NULL, '\0', NULL, 0, N_("When centering an accent over a glyph, should the accent\nbe centered on the highest point(s) of the glyph,\nor the middle of the glyph?") },
	{ N_("PreferSpacingAccents"), pr_bool, &PreferSpacingAccents, NULL, NULL, '\0', NULL, 0, N_("Use spacing accents (Unicode: 02C0-02FF) rather than\ncombining accents (Unicode: 0300-036F) when\nbuilding accented glyphs.") },
	PREFS_LIST_EMPTY
},
 args_list[] = {
	{ N_("PreferPotrace"), pr_bool, &preferpotrace, NULL, NULL, '\0', NULL, 0, N_("FontForge supports two different helper applications to do autotracing\n autotrace and potrace\nIf your system only has one it will use that one, if you have both\nuse this option to tell FontForge which to pick.") },
	{ N_("AutotraceArgs"), pr_string, NULL, GetAutoTraceArgs, SetAutoTraceArgs, '\0', NULL, 0, N_("Extra arguments for configuring the autotrace program\n(either autotrace or potrace)") },
	{ N_("AutotraceAsk"), pr_bool, &autotrace_ask, NULL, NULL, '\0', NULL, 0, N_("Ask the user for autotrace arguments each time autotrace is invoked") },
	{ N_("MfArgs"), pr_string, &mf_args, NULL, NULL, '\0', NULL, 0, N_("Commands to pass to mf (metafont) program, the filename will follow these") },
	{ N_("MfAsk"), pr_bool, &mf_ask, NULL, NULL, '\0', NULL, 0, N_("Ask the user for mf commands each time mf is invoked") },
	{ N_("MfClearBg"), pr_bool, &mf_clearbackgrounds, NULL, NULL, '\0', NULL, 0, N_("FontForge loads large images into the background of each glyph\nprior to autotracing them. You may retain those\nimages to look at after mf processing is complete, or\nremove them to save space") },
	{ N_("MfShowErr"), pr_bool, &mf_showerrors, NULL, NULL, '\0', NULL, 0, N_("MetaFont (mf) generates lots of verbiage to stdout.\nMost of the time I find it an annoyance but it is\nimportant to see if something goes wrong.") },
	PREFS_LIST_EMPTY
},
 fontinfo_list[] = {
	{ N_("FoundryName"), pr_string, &BDFFoundry, NULL, NULL, 'F', NULL, 0, N_("Name used for foundry field in bdf\nfont generation") },
	{ N_("TTFFoundry"), pr_string, &TTFFoundry, NULL, NULL, 'T', NULL, 0, N_("Name used for Vendor ID field in\nttf (OS/2 table) font generation.\nMust be no more than 4 characters") },
	{ N_("NewFontNameList"), pr_namelist, &namelist_for_new_fonts, NULL, NULL, '\0', NULL, 0, N_("FontForge will use this namelist when assigning\nglyph names to code points in a new font.") },
	{ N_("RecognizePUANames"), pr_bool, &recognizePUA, NULL, NULL, 'U', NULL, 0, N_("Once upon a time, Adobe assigned PUA (public use area) encodings\nfor many stylistic variants of characters (small caps, old style\nnumerals, etc.). Adobe no longer believes this to be a good idea,\nand recommends that these encodings be ignored.\n\n The assignments were originally made because most applications\ncould not handle OpenType features for accessing variants. Adobe\nnow believes that all apps that matter can now do so. Applications\nlike Word and OpenOffice still can't handle these features, so\n fontforge's default behavior is to ignore Adobe's current\nrecommendations.\n\nNote: This does not affect figuring out unicode from the font's encoding,\nit just controls determining unicode from a name.") },
	{ N_("UnicodeGlyphNames"), pr_bool, &allow_utf8_glyphnames, NULL, NULL, 'O', NULL, 0, N_("Allow the full unicode character set in glyph names.\nThis does not conform to adobe's glyph name standard.\nSuch names should be for internal use only and\nshould NOT end up in production fonts." ) },
	{ N_("AddCharToNameList"), pr_bool, &add_char_to_name_list, NULL, NULL, 'O', NULL, 0, N_( "When displaying a list of glyph names\n(or sometimes just a single glyph name)\nFontForge will add the unicode character\nthe name refers to in parenthesis after\nthe name. It does this because some names\nare obscure.\nSome people would prefer not to see this,\nso this preference item lets you turn off\n this behavior" ) },
	PREFS_LIST_EMPTY
},
 generate_list[] = {
	{ N_("AskBDFResolution"), pr_bool, &ask_user_for_resolution, NULL, NULL, 'B', NULL, 0, N_("When generating a set of BDF fonts ask the user\nto specify the screen resolution of the fonts\notherwise FontForge will guess depending on the pixel size.") },
	{ N_("AutoHint"), pr_bool, &autohint_before_generate, NULL, NULL, 'H', NULL, 0, N_("AutoHint changed glyphs before generating a font") },

	{ N_("GenerateHintWidthEqualityTolerance"), pr_real, &GenerateHintWidthEqualityTolerance, NULL, NULL, '\0', NULL, 0, N_( "When generating a font, ignore slight rounding errors for hints that should be at the top or bottom of the glyph. For example, you might like to set this to 0.02 so that 19.999 will be considered 20. But only for the hint width value.") },
	
	PREFS_LIST_EMPTY
},
 hints_list[] = {
	{ N_("StandardSlopeError"), pr_angle, &stem_slope_error, NULL, NULL, '\0', NULL, 0, N_("The maximum slope difference which still allows to consider two points \"parallel\".\nEnlarge this to make the autohinter more tolerable to small deviations from straight lines when detecting stem edges.") },
	{ N_("SerifSlopeError"), pr_angle, &stub_slope_error, NULL, NULL, '\0', NULL, 0, N_("Same as above, but for terminals of small features (e. g. serifs), which can deviate more significantly from the horizontal or vertical direction.") },
	{ N_("HintBoundingBoxes"), pr_bool, &hint_bounding_boxes, NULL, NULL, '\0', NULL, 0, N_("FontForge will place vertical or horizontal hints to describe the bounding boxes of suitable glyphs.") },
	{ N_("HintDiagonalEnds"), pr_bool, &hint_diagonal_ends, NULL, NULL, '\0', NULL, 0, N_("FontForge will place vertical or horizontal hints at the ends of diagonal stems.") },
	{ N_("HintDiagonalInter"), pr_bool, &hint_diagonal_intersections, NULL, NULL, '\0', NULL, 0, N_("FontForge will place vertical or horizontal hints at the intersections of diagonal stems.") },
	{ N_("DetectDiagonalStems"), pr_bool, &detect_diagonal_stems, NULL, NULL, '\0', NULL, 0, N_("FontForge will generate diagonal stem hints, which then can be used by the AutoInstr command.") },
	PREFS_LIST_EMPTY
},
 instrs_list[] = {
	{ N_("InstructDiagonalStems"), pr_bool, &instruct_diagonal_stems, NULL, NULL, '\0', NULL, 0, N_("Generate instructions for diagonal stem hints.") },
	{ N_("InstructSerifs"), pr_bool, &instruct_serif_stems, NULL, NULL, '\0', NULL, 0, N_("Try to detect serifs and other elements protruding from base stems and generate instructions for them.") },
	{ N_("InstructBallTerminals"), pr_bool, &instruct_ball_terminals, NULL, NULL, '\0', NULL, 0, N_("Generate instructions for ball terminals.") },
	{ N_("InterpolateStrongPoints"), pr_bool, &interpolate_strong, NULL, NULL, '\0', NULL, 0, N_("Interpolate between stem edges some important points, not affected by other instructions.") },
	{ N_("CounterControl"), pr_bool, &control_counters, NULL, NULL, '\0', NULL, 0, N_("Make sure similar or equal counters remain the same in gridfitted outlines.\nEnabling this option may result in glyph advance widths being\ninconsistently scaled at some PPEMs.") },
	PREFS_LIST_EMPTY
},
 opentype_list[] = {
	{ N_("UseNewIndicScripts"), pr_bool, &use_second_indic_scripts, NULL, NULL, 'C', NULL, 0, N_("MS has changed (in August 2006) the inner workings of their Indic shaping\nengine, and to disambiguate this change has created a parallel set of script\ntags (generally ending in '2') for Indic writing systems. If you are working\nwith the new system set this flag, if you are working with the old unset it.\n(if you aren't doing Indic work, this flag is irrelevant).") },
	PREFS_LIST_EMPTY
},
#ifdef BUILD_COLLAB
 collab_list[] = {
	{ N_("SessionJoinTimeout"), pr_int, &pref_collab_sessionJoinTimeoutMS, NULL, NULL, 'C', NULL, 0, N_("The number of milliseconds to wait for a connection to the collaboration server to happen. FontForge may be unresponsive during this session connection time. (default 1000 which is 1 second)") },
	{ N_("RoundTripMessageMaxTime"), pr_int, &pref_collab_roundTripTimerMS, NULL, NULL, 'C', NULL, 0, N_("The number of milliseconds that are allowed to pass between sending an update to the server and hearing it sent back as a message to all clients. The FontForge user interface may be unresponsive during this time. A change requires a restart of FontForge. (default 2000 which is 2 seconds)") },
	PREFS_LIST_EMPTY
},
#endif
/* These are hidden, so will never appear in preference ui, hence, no "N_(" */
/*  They are controled elsewhere AntiAlias is a menu item in the font window's View menu */
/*  etc. */
 hidden_list[] = {
	{ "AntiAlias", pr_bool, &default_fv_antialias, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultFVShowHmetrics", pr_int, &default_fv_showhmetrics, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultFVShowVmetrics", pr_int, &default_fv_showvmetrics, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultFVSize", pr_int, &default_fv_font_size, NULL, NULL, 'S', NULL, 1, NULL },
	{ "DefaultFVRowCount", pr_int, &default_fv_row_count, NULL, NULL, 'S', NULL, 1, NULL },
	{ "DefaultFVColCount", pr_int, &default_fv_col_count, NULL, NULL, 'S', NULL, 1, NULL },
	{ "DefaultFVGlyphLabel", pr_int, &default_fv_glyphlabel, NULL, NULL, 'S', NULL, 1, NULL },
	{ "SaveToDir", pr_int, &save_to_dir, NULL, NULL, 'S', NULL, 1, NULL },
	{ "OnlyCopyDisplayed", pr_bool, &onlycopydisplayed, NULL, NULL, '\0', NULL, 1, NULL },
	{ "PalettesDocked", pr_bool, &palettes_docked, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultCVWidth", pr_int, &cv_width, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultCVHeight", pr_int, &cv_height, NULL, NULL, '\0', NULL, 1, NULL },
	{ "CVVisible0", pr_bool, &cvvisible[0], NULL, NULL, '\0', NULL, 1, NULL },
	{ "CVVisible1", pr_bool, &cvvisible[1], NULL, NULL, '\0', NULL, 1, NULL },
	{ "BVVisible0", pr_bool, &bvvisible[0], NULL, NULL, '\0', NULL, 1, NULL },
	{ "BVVisible1", pr_bool, &bvvisible[1], NULL, NULL, '\0', NULL, 1, NULL },
	{ "BVVisible2", pr_bool, &bvvisible[2], NULL, NULL, '\0', NULL, 1, NULL },
	{ "MarkExtrema", pr_int, &CVShows.markextrema, NULL, NULL, '\0', NULL, 1, NULL },
	{ "MarkPointsOfInflect", pr_int, &CVShows.markpoi, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ShowRulers", pr_bool, &CVShows.showrulers, NULL, NULL, '\0', NULL, 1, N_("Display rulers in the Outline Glyph View") },
	{ "ShowCPInfo", pr_int, &CVShows.showcpinfo, NULL, NULL, '\0', NULL, 1, NULL },
	{ "CreateDraggingComparisonOutline", pr_int, &prefs_create_dragging_comparison_outline, NULL, NULL, '\0', NULL, 1, NULL },
	{ "InfoWindowDistance", pr_int, &infowindowdistance, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ShowSideBearings", pr_int, &CVShows.showsidebearings, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ShowRefNames", pr_int, &CVShows.showrefnames, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ShowPoints", pr_bool, &CVShows.showpoints, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ShowFilled", pr_int, &CVShows.showfilled, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ShowTabs", pr_int, &CVShows.showtabs, NULL, NULL, '\0', NULL, 1, NULL },
 	{ "SnapOutlines", pr_int, &CVShows.snapoutlines, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ShowAlmostHVLines", pr_bool, &CVShows.showalmosthvlines, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ShowAlmostHVCurves", pr_bool, &CVShows.showalmosthvcurves, NULL, NULL, '\0', NULL, 1, NULL },
	{ "AlmostHVBound", pr_int, &CVShows.hvoffset, NULL, NULL, '\0', NULL, 1, NULL },
	{ "CheckSelfIntersects", pr_bool, &CVShows.checkselfintersects, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ShowDebugChanges", pr_bool, &CVShows.showdebugchanges, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultScreenDpiSystem", pr_int, &oldsystem, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultOutputFormat", pr_int, &oldformatstate, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultBitmapFormat", pr_int, &oldbitmapstate, NULL, NULL, '\0', NULL, 1, NULL },
	{ "SaveValidate", pr_int, &old_validate, NULL, NULL, '\0', NULL, 1, NULL },
	{ "SaveFontLogAsk", pr_int, &old_fontlog, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultSFNTflags", pr_int, &old_sfnt_flags, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultPSflags", pr_int, &old_ps_flags, NULL, NULL, '\0', NULL, 1, NULL },
	{ "PageWidth", pr_int, &pagewidth, NULL, NULL, '\0', NULL, 1, NULL },
	{ "PageHeight", pr_int, &pageheight, NULL, NULL, '\0', NULL, 1, NULL },
	{ "PrintType", pr_int, &printtype, NULL, NULL, '\0', NULL, 1, NULL },
	{ "PrintCommand", pr_string, &printcommand, NULL, NULL, '\0', NULL, 1, NULL },
	{ "PageLazyPrinter", pr_string, &printlazyprinter, NULL, NULL, '\0', NULL, 1, NULL },
	{ "RegularStar", pr_bool, &regular_star, NULL, NULL, '\0', NULL, 1, NULL },
	{ "PolyStar", pr_bool, &polystar, NULL, NULL, '\0', NULL, 1, NULL },
	{ "RectEllipse", pr_bool, &rectelipse, NULL, NULL, '\0', NULL, 1, NULL },
	{ "RectCenterOut", pr_bool, &center_out[0], NULL, NULL, '\0', NULL, 1, NULL },
	{ "EllipseCenterOut", pr_bool, &center_out[1], NULL, NULL, '\0', NULL, 1, NULL },
	{ "PolyStartPointCnt", pr_int, &ps_pointcnt, NULL, NULL, '\0', NULL, 1, NULL },
	{ "RoundRectRadius", pr_real, &rr_radius, NULL, NULL, '\0', NULL, 1, NULL },
	{ "StarPercent", pr_real, &star_percent, NULL, NULL, '\0', NULL, 1, NULL },
	{ "CoverageFormatsAllowed", pr_int, &coverageformatsallowed, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DebugWins", pr_int, &debug_wins, NULL, NULL, '\0', NULL, 1, NULL },
	{ "GridFitDpi", pr_int, &gridfit_dpi, NULL, NULL, '\0', NULL, 1, NULL },
	{ "GridFitDepth", pr_int, &gridfit_depth, NULL, NULL, '\0', NULL, 1, NULL },
	{ "GridFitPointSize", pr_real, &gridfit_pointsizey, NULL, NULL, '\0', NULL, 1, NULL },
	{ "GridFitPointSizeX", pr_real, &gridfit_pointsizex, NULL, NULL, '\0', NULL, 1, NULL },
	{ "GridFitSameAs", pr_int, &gridfit_x_sameas_y, NULL, NULL, '\0', NULL, 1, NULL },
	{ "MVShowGrid", pr_int, &mvshowgrid, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ForceNamesWhenOpening", pr_namelist, &force_names_when_opening, NULL, NULL, '\0', NULL, 1, NULL },
	{ "ForceNamesWhenSaving", pr_namelist, &force_names_when_saving, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultFontFilterIndex", pr_int, &default_font_filter_index, NULL, NULL, '\0', NULL, 1, NULL },
	{ "FCShowHidden", pr_bool, &gfc_showhidden, NULL, NULL, '\0', NULL, 1, NULL },
	{ "FCDirPlacement", pr_int, &gfc_dirplace, NULL, NULL, '\0', NULL, 1, NULL },
	{ "FCBookmarks", pr_string, &gfc_bookmarks, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultMVType",   pr_int, &mv_type, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultMVWidth",  pr_int, &mv_width, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultMVHeight", pr_int, &mv_height, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultBVWidth", pr_int, &bv_width, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultBVHeight", pr_int, &bv_height, NULL, NULL, '\0', NULL, 1, NULL },
	{ "AnchorControlPixelSize", pr_int, &aa_pixelsize, NULL, NULL, '\0', NULL, 1, NULL },
	{ "CollabLastServerConnectedTo", pr_string, &pref_collab_last_server_connected_to, NULL, NULL, '\0', NULL, 1, NULL },
#ifdef _NO_LIBCAIRO
	{ "UseCairoDrawing", pr_bool, &prefs_usecairo, NULL, NULL, '\0', NULL, 0, N_("Use the cairo library for drawing (if available)\nThis makes for prettier (anti-aliased) but slower drawing\nThis applies to any windows created AFTER this is set.\nAlready existing windows will continue as they are.") },
#endif
	{ "CV_B1Tool", pr_int, (int *) &cv_b1_tool, NULL, NULL, '\0', NULL, 1, NULL },
	{ "CV_CB1Tool", pr_int, (int *) &cv_cb1_tool, NULL, NULL, '\0', NULL, 1, NULL },
	{ "CV_B2Tool", pr_int, (int *) &cv_b2_tool, NULL, NULL, '\0', NULL, 1, NULL },
	{ "CV_CB2Tool", pr_int, (int *) &cv_cb2_tool, NULL, NULL, '\0', NULL, 1, NULL },
	{ "XUID-Base", pr_string, &xuid, NULL, NULL, 'X', NULL, 0, N_("If specified this should be a space separated list of integers each\nless than 16777216 which uniquely identify your organization\nFontForge will generate a random number for the final component.") }, /* Obsolete */
	{ "ShowKerningPane", pr_int, (int *) &show_kerning_pane_in_class, NULL, NULL, '\0', NULL, 1, NULL },
	PREFS_LIST_EMPTY
},
 oldnames[] = {
	{ "DumpGlyphMap", pr_bool, &glyph_2_name_map, NULL, NULL, '\0', NULL, 0, N_("When generating a truetype or opentype font it is occasionally\nuseful to know the mapping between truetype glyph ids and\nglyph names. Setting this option will cause FontForge to\nproduce a file (with extension .g2n) containing those data.") },
	{ "DefaultTTFApple", pr_int, &pointless, NULL, NULL, '\0', NULL, 1, NULL },
	{ "AcuteCenterBottom", pr_bool, &GraveAcuteCenterBottom, NULL, NULL, '\0', NULL, 1, N_("When placing grave and acute accents above letters, should\nFontForge center them based on their full width, or\nshould it just center based on the lowest point\nof the accent.") },
	{ "AlwaysGenApple", pr_bool, &alwaysgenapple, NULL, NULL, 'A', NULL, 0, N_("Apple and MS/Adobe differ about the format of truetype and opentype files.\nThis controls the default setting of the Apple checkbox in the\nFile->Generate Font dialog.\nThe main differences are:\n Bitmap data are stored in different tables\n Scaled composite glyphs are treated differently\n Use of GSUB rather than morx(t)/feat\n Use of GPOS rather than kern/opbd\n Use of GDEF rather than lcar/prop\nIf both this and OpenType are set, both formats are generated") },
	{ "AlwaysGenOpenType", pr_bool, &alwaysgenopentype, NULL, NULL, 'O', NULL, 0, N_("Apple and MS/Adobe differ about the format of truetype and opentype files.\nThis controls the default setting of the OpenType checkbox in the\nFile->Generate Font dialog.\nThe main differences are:\n Bitmap data are stored in different tables\n Scaled composite glyphs are treated differently\n Use of GSUB rather than morx(t)/feat\n Use of GPOS rather than kern/opbd\n Use of GDEF rather than lcar/prop\nIf both this and Apple are set, both formats are generated") },
	{ "DefaultTTFflags", pr_int, &old_ttf_flags, NULL, NULL, '\0', NULL, 1, NULL },
	{ "DefaultOTFflags", pr_int, &old_otf_flags, NULL, NULL, '\0', NULL, 1, NULL },
	PREFS_LIST_EMPTY
},
 *prefs_list[] = { general_list, new_list, open_list, navigation_list, sync_list, editing_list, editing_interface_list, accent_list, args_list, fontinfo_list, generate_list, tt_list, opentype_list, hints_list, instrs_list,
 #ifdef BUILD_COLLAB
 collab_list,
 #endif
 hidden_list, NULL },
 *load_prefs_list[] = { general_list, new_list, open_list, navigation_list, sync_list, editing_list, editing_interface_list, accent_list, args_list, fontinfo_list, generate_list, tt_list, opentype_list, hints_list, instrs_list,
 #ifdef BUILD_COLLAB
 collab_list,
 #endif
 hidden_list, oldnames, NULL };

struct visible_prefs_list { char *tab_name; int nest; struct prefs_list *pl; } visible_prefs_list[] = {
    { N_("Generic"), 0, general_list},
    { N_("New Font"), 0, new_list},
    { N_("Open Font"), 0, open_list},
    { N_("Navigation"), 0, navigation_list},
    { N_("Editing"), 0, editing_list},
    { N_("Interface"), 1, editing_interface_list},
    { N_("Synchronize"), 1, sync_list},
    { N_("TT"), 1, tt_list},
    { N_("Accents"), 1, accent_list},
    { N_("Apps"), 1, args_list},
    { N_("Font Info"), 0, fontinfo_list},
    { N_("Generate"), 0, generate_list},
    { N_("PS Hints"), 1, hints_list},
    { N_("TT Instrs"), 1, instrs_list},
    { N_("OpenType"), 1, opentype_list},
#ifdef BUILD_COLLAB
    { N_("Collaboration"), 0, collab_list},
#endif
    { NULL, 0, NULL }
};

static void FileChooserPrefsChanged(void *pointless) {
    SavePrefs(true);
}

static void ProcessFileChooserPrefs(void) {
    unichar_t **b;
    int i;

    GFileChooserSetShowHidden(gfc_showhidden);
    GFileChooserSetDirectoryPlacement(gfc_dirplace);
    if ( gfc_bookmarks==NULL ) {
	b = malloc(8*sizeof(unichar_t *));
	i = 0;
#ifdef __Mac
	b[i++] = uc_copy("~/Library/Fonts/");
#endif
	b[i++] = uc_copy("~/fonts");
#ifdef __Mac
	b[i++] = uc_copy("/Library/Fonts/");
	b[i++] = uc_copy("/System/Library/Fonts/");
#endif
#if __CygWin
	b[i++] = uc_copy("/cygdrive/c/Windows/Fonts/");
#endif
	b[i++] = uc_copy("/usr/X11R6/lib/X11/fonts/");
#ifndef __CygWin		/* I'm not releasing ftp support on cygwin */
	b[i++] = uc_copy("ftp://ctan.org/pub/tex-archive/fonts/");
#endif
	b[i++] = NULL;
	GFileChooserSetBookmarks(b);
    } else {
	char *pt, *start;
	start = gfc_bookmarks;
	for ( i=0; ; ++i ) {
	    pt = strchr(start,';');
	    if ( pt==NULL )
	break;
	    start = pt+1;
	}
	start = gfc_bookmarks;
	b = malloc((i+2)*sizeof(unichar_t *));
	for ( i=0; ; ++i ) {
	    pt = strchr(start,';');
	    if ( pt!=NULL )
		*pt = '\0';
	    b[i] = utf82u_copy(start);
	    if ( pt==NULL )
	break;
	    *pt = ';';
	    start = pt+1;
	}
	b[i+1] = NULL;
	GFileChooserSetBookmarks(b);
    }
    GFileChooserSetPrefsChangedCallback(NULL,FileChooserPrefsChanged);
}

static void GetFileChooserPrefs(void) {
    unichar_t **foo;

    gfc_showhidden = GFileChooserGetShowHidden();
    gfc_dirplace = GFileChooserGetDirectoryPlacement();
    foo = GFileChooserGetBookmarks();
    free(gfc_bookmarks);
    if ( foo==NULL || foo[0]==NULL )
	gfc_bookmarks = NULL;
    else {
	int i,len=0;
	for ( i=0; foo[i]!=NULL; ++i )
	    len += 4*u_strlen(foo[i])+1;
	gfc_bookmarks = malloc(len+10);
	len = 0;
	for ( i=0; foo[i]!=NULL; ++i ) {
	    u2utf8_strcpy(gfc_bookmarks+len,foo[i]);
	    len += strlen(gfc_bookmarks+len);
	    gfc_bookmarks[len++] = ';';
	}
	if ( len>0 )
	    gfc_bookmarks[len-1] = '\0';
	else {
	    free(gfc_bookmarks);
	    gfc_bookmarks = NULL;
	}
    }
}

#define TOPICS	(sizeof(visible_prefs_list)/sizeof(visible_prefs_list[0])-1)

static int PrefsUI_GetPrefs(char *name,Val *val) {
    int i,j;

    /* Support for obsolete preferences */
    alwaysgenapple=(old_sfnt_flags&ttf_flag_applemode)?1:0;
    alwaysgenopentype=(old_sfnt_flags&ttf_flag_otmode)?1:0;

    for ( i=0; prefs_list[i]!=NULL; ++i ) for ( j=0; prefs_list[i][j].name!=NULL; ++j ) {
	if ( strcmp(prefs_list[i][j].name,name)==0 ) {
	    struct prefs_list *pf = &prefs_list[i][j];
	    if ( pf->type == pr_bool || pf->type == pr_int || pf->type == pr_unicode ) {
		val->type = v_int;
		val->u.ival = *((int *) (pf->val));
	    } else if ( pf->type == pr_string || pf->type == pr_file ) {
		val->type = v_str;

		char *tmpstr = pf->val ? *((char **) (pf->val)) : (char *) (pf->get)();
		val->u.sval = copy( tmpstr ? tmpstr : "" );

		if( ! pf->val )
		    free( tmpstr );
	    } else if ( pf->type == pr_encoding ) {
		val->type = v_str;
		if ( *((NameList **) (pf->val))==NULL )
		    val->u.sval = copy( "NULL" );
		else
		    val->u.sval = copy( (*((Encoding **) (pf->val)))->enc_name );
	    } else if ( pf->type == pr_namelist ) {
		val->type = v_str;
		val->u.sval = copy( (*((NameList **) (pf->val)))->title );
	    } else if ( pf->type == pr_real || pf->type == pr_angle ) {
		val->type = v_real;
		val->u.fval = *((float *) (pf->val));
		if ( pf->type == pr_angle )
		    val->u.fval *= RAD2DEG;
	    } else
return( false );

return( true );
	}
    }
return( false );
}

static void CheckObsoletePrefs(void) {
    if ( alwaysgenapple==false ) {
	old_sfnt_flags &= ~ttf_flag_applemode;
    } else if ( alwaysgenapple==true ) {
	old_sfnt_flags |= ttf_flag_applemode;
    }
    if ( alwaysgenopentype==false ) {
	old_sfnt_flags &= ~ttf_flag_otmode;
    } else if ( alwaysgenopentype==true ) {
	old_sfnt_flags |= ttf_flag_otmode;
    }
    if ( old_ttf_flags!=0 )
	old_sfnt_flags = old_ttf_flags | old_otf_flags;
}

static int PrefsUI_SetPrefs(char *name,Val *val1, Val *val2) {
    int i,j;

    /* Support for obsolete preferences */
    alwaysgenapple=-1; alwaysgenopentype=-1;

    for ( i=0; prefs_list[i]!=NULL; ++i ) for ( j=0; prefs_list[i][j].name!=NULL; ++j ) {
	if ( strcmp(prefs_list[i][j].name,name)==0 ) {
	    struct prefs_list *pf = &prefs_list[i][j];
	    if ( pf->type == pr_bool || pf->type == pr_int || pf->type == pr_unicode ) {
		if ( (val1->type!=v_int && val1->type!=v_unicode) || val2!=NULL )
return( -1 );
		*((int *) (pf->val)) = val1->u.ival;
	    } else if ( pf->type == pr_real || pf->type == pr_angle ) {
		if ( val1->type==v_real && val2==NULL )
		    *((float *) (pf->val)) = val1->u.fval;
		else if ( val1->type!=v_int || (val2!=NULL && val2->type!=v_int ))
return( -1 );
		else
		    *((float *) (pf->val)) = (val2==NULL ? val1->u.ival : val1->u.ival / (double) val2->u.ival);
		if ( pf->type == pr_angle )
		    *((float *) (pf->val)) /= RAD2DEG;
	    } else if ( pf->type == pr_string || pf->type == pr_file ) {
		if ( val1->type!=v_str || val2!=NULL )
return( -1 );
		if ( pf->set ) {
		    pf->set( val1->u.sval );
		} else {
		    free( *((char **) (pf->val)));
		    *((char **) (pf->val)) = copy( val1->u.sval );
		}
	    } else if ( pf->type == pr_encoding ) {
		if ( val2!=NULL )
return( -1 );
		else if ( val1->type==v_str && pf->val == &default_encoding) {
		    Encoding *enc = FindOrMakeEncoding(val1->u.sval);
		    if ( enc==NULL )
return( -1 );
		    *((Encoding **) (pf->val)) = enc;
		} else
return( -1 );
	    } else if ( pf->type == pr_namelist ) {
		if ( val2!=NULL )
return( -1 );
		else if ( val1->type==v_str ) {
		    NameList *nl = NameListByName(val1->u.sval);
		    if ( strcmp(val1->u.sval,"NULL")==0 && pf->val != &namelist_for_new_fonts )
			nl = NULL;
		    else if ( nl==NULL )
return( -1 );
		    *((NameList **) (pf->val)) = nl;
		} else
return( -1 );
	    } else
return( false );

	    CheckObsoletePrefs();
	    SavePrefs(true);
return( true );
	}
    }
return( false );
}

static char *getPfaEditPrefs(void) {
    static char *prefs=NULL;
    char buffer[1025];
    char *ffdir;

    if ( prefs!=NULL )
        return prefs;
    ffdir = getFontForgeUserDir(Config);
    if ( ffdir==NULL )
        return NULL;
    sprintf(buffer,"%s/prefs", ffdir);
    free(ffdir);
    prefs = copy(buffer);
    return prefs;
}

static char *PrefsUI_getFontForgeShareDir(void) {
    static char *sharedir=NULL;
    static int set=false;
    char *pt;
    int len;

    if ( set )
return( sharedir );

    set = true;

#if defined(__MINGW32__)

    len = strlen(GResourceProgramDir) + strlen("/share/fontforge") +1;
    sharedir = malloc(len);
    strcpy(sharedir, GResourceProgramDir);
    strcat(sharedir, "/share/fontforge");
    return sharedir;

#else

    pt = strstr(GResourceProgramDir,"/bin");
    if ( pt==NULL ) {
#if defined(SHAREDIR)
	sharedir = copy(SHAREDIR "/fontforge" );
return( sharedir );
#elif defined(PREFIX)
	sharedir = copy( PREFIX "/share/fontforge" );
return( sharedir );
#else
return( NULL );
#endif
    }
    len = (pt-GResourceProgramDir)+strlen("/share/fontforge")+1;
    sharedir = malloc(len);
    strncpy(sharedir,GResourceProgramDir,pt-GResourceProgramDir);
    strcpy(sharedir+(pt-GResourceProgramDir),"/share/fontforge");
return( sharedir );

#endif
}

#  include <charset.h>		/* we still need the charsets & encoding to set local_encoding */
static int encmatch(const char *enc,int subok) {
    static struct { char *name; int enc; } encs[] = {
	{ "US-ASCII", e_usascii },
	{ "ASCII", e_usascii },
	{ "ISO646-NO", e_iso646_no },
	{ "ISO646-SE", e_iso646_se },
	{ "LATIN10", e_iso8859_16 },
	{ "LATIN1", e_iso8859_1 },
	{ "ISO-8859-1", e_iso8859_1 },
	{ "ISO-8859-2", e_iso8859_2 },
	{ "ISO-8859-3", e_iso8859_3 },
	{ "ISO-8859-4", e_iso8859_4 },
	{ "ISO-8859-5", e_iso8859_4 },
	{ "ISO-8859-6", e_iso8859_4 },
	{ "ISO-8859-7", e_iso8859_4 },
	{ "ISO-8859-8", e_iso8859_4 },
	{ "ISO-8859-9", e_iso8859_4 },
	{ "ISO-8859-10", e_iso8859_10 },
	{ "ISO-8859-11", e_iso8859_11 },
	{ "ISO-8859-13", e_iso8859_13 },
	{ "ISO-8859-14", e_iso8859_14 },
	{ "ISO-8859-15", e_iso8859_15 },
	{ "ISO-8859-16", e_iso8859_16 },
	{ "ISO_8859-1", e_iso8859_1 },
	{ "ISO_8859-2", e_iso8859_2 },
	{ "ISO_8859-3", e_iso8859_3 },
	{ "ISO_8859-4", e_iso8859_4 },
	{ "ISO_8859-5", e_iso8859_4 },
	{ "ISO_8859-6", e_iso8859_4 },
	{ "ISO_8859-7", e_iso8859_4 },
	{ "ISO_8859-8", e_iso8859_4 },
	{ "ISO_8859-9", e_iso8859_4 },
	{ "ISO_8859-10", e_iso8859_10 },
	{ "ISO_8859-11", e_iso8859_11 },
	{ "ISO_8859-13", e_iso8859_13 },
	{ "ISO_8859-14", e_iso8859_14 },
	{ "ISO_8859-15", e_iso8859_15 },
	{ "ISO_8859-16", e_iso8859_16 },
	{ "ISO8859-1", e_iso8859_1 },
	{ "ISO8859-2", e_iso8859_2 },
	{ "ISO8859-3", e_iso8859_3 },
	{ "ISO8859-4", e_iso8859_4 },
	{ "ISO8859-5", e_iso8859_4 },
	{ "ISO8859-6", e_iso8859_4 },
	{ "ISO8859-7", e_iso8859_4 },
	{ "ISO8859-8", e_iso8859_4 },
	{ "ISO8859-9", e_iso8859_4 },
	{ "ISO8859-10", e_iso8859_10 },
	{ "ISO8859-11", e_iso8859_11 },
	{ "ISO8859-13", e_iso8859_13 },
	{ "ISO8859-14", e_iso8859_14 },
	{ "ISO8859-15", e_iso8859_15 },
	{ "ISO8859-16", e_iso8859_16 },
	{ "ISO88591", e_iso8859_1 },
	{ "ISO88592", e_iso8859_2 },
	{ "ISO88593", e_iso8859_3 },
	{ "ISO88594", e_iso8859_4 },
	{ "ISO88595", e_iso8859_4 },
	{ "ISO88596", e_iso8859_4 },
	{ "ISO88597", e_iso8859_4 },
	{ "ISO88598", e_iso8859_4 },
	{ "ISO88599", e_iso8859_4 },
	{ "ISO885910", e_iso8859_10 },
	{ "ISO885911", e_iso8859_11 },
	{ "ISO885913", e_iso8859_13 },
	{ "ISO885914", e_iso8859_14 },
	{ "ISO885915", e_iso8859_15 },
	{ "ISO885916", e_iso8859_16 },
	{ "8859_1", e_iso8859_1 },
	{ "8859_2", e_iso8859_2 },
	{ "8859_3", e_iso8859_3 },
	{ "8859_4", e_iso8859_4 },
	{ "8859_5", e_iso8859_4 },
	{ "8859_6", e_iso8859_4 },
	{ "8859_7", e_iso8859_4 },
	{ "8859_8", e_iso8859_4 },
	{ "8859_9", e_iso8859_4 },
	{ "8859_10", e_iso8859_10 },
	{ "8859_11", e_iso8859_11 },
	{ "8859_13", e_iso8859_13 },
	{ "8859_14", e_iso8859_14 },
	{ "8859_15", e_iso8859_15 },
	{ "8859_16", e_iso8859_16 },
	{ "KOI8-R", e_koi8_r },
	{ "KOI8R", e_koi8_r },
	{ "WINDOWS-1252", e_win },
	{ "CP1252", e_win },
	{ "Big5", e_big5 },
	{ "Big-5", e_big5 },
	{ "BigFive", e_big5 },
	{ "Big-Five", e_big5 },
	{ "Big5HKSCS", e_big5hkscs },
	{ "Big5-HKSCS", e_big5hkscs },
	{ "UTF-8", e_utf8 },
	{ "ISO-10646/UTF-8", e_utf8 },
	{ "ISO_10646/UTF-8", e_utf8 },
	{ "UCS2", e_unicode },
	{ "UCS-2", e_unicode },
	{ "UCS-2-INTERNAL", e_unicode },
	{ "ISO-10646", e_unicode },
	{ "ISO_10646", e_unicode },
	/* { "eucJP", e_euc }, */
	/* { "EUC-JP", e_euc }, */
	/* { "ujis", ??? }, */
	/* { "EUC-KR", e_euckorean }, */
	{ NULL, 0 }
    };

    int i;
    char buffer[80];
#if HAVE_ICONV
    static char *last_complaint;

    iconv_t test;
    free(iconv_local_encoding_name);
    iconv_local_encoding_name= NULL;
#endif

    if ( strchr(enc,'@')!=NULL && strlen(enc)<sizeof(buffer)-1 ) {
	strcpy(buffer,enc);
	*strchr(buffer,'@') = '\0';
	enc = buffer;
    }

    for ( i=0; encs[i].name!=NULL; ++i )
	if ( strmatch(enc,encs[i].name)==0 )
return( encs[i].enc );

    if ( subok ) {
	for ( i=0; encs[i].name!=NULL; ++i )
	    if ( strstrmatch(enc,encs[i].name)!=NULL )
return( encs[i].enc );

#if HAVE_ICONV
	/* I only try to use iconv if the encoding doesn't match one I support*/
	/*  loading iconv unicode data takes a while */
	test = iconv_open(enc,FindUnicharName());
	if ( test==(iconv_t) (-1) || test==NULL ) {
	    if ( last_complaint==NULL || strcmp(last_complaint,enc)!=0 ) {
		fprintf( stderr, "Neither FontForge nor iconv() supports your encoding (%s) we will pretend\n you asked for latin1 instead.\n", enc );
		free( last_complaint );
		last_complaint = copy(enc);
	    }
	} else {
	    if ( last_complaint==NULL || strcmp(last_complaint,enc)!=0 ) {
		fprintf( stderr, "FontForge does not support your encoding (%s), it will try to use iconv()\n or it will pretend the local encoding is latin1\n", enc );
		free( last_complaint );
		last_complaint = copy(enc);
	    }
	    iconv_local_encoding_name= copy(enc);
	    iconv_close(test);
	}
#else
	fprintf( stderr, "FontForge does not support your encoding (%s), it will pretend the local encoding is latin1\n", enc );
#endif

return( e_iso8859_1 );
    }
return( e_unknown );
}

static int DefaultEncoding(void) {
    const char *loc;
    int enc;

#if HAVE_LANGINFO_H
    loc = nl_langinfo(CODESET);
    enc = encmatch(loc,false);
    if ( enc!=e_unknown )
return( enc );
#endif
    loc = getenv("LC_ALL");
    if ( loc==NULL ) loc = getenv("LC_CTYPE");
    /*if ( loc==NULL ) loc = getenv("LC_MESSAGES");*/
    if ( loc==NULL ) loc = getenv("LANG");

    if ( loc==NULL )
return( e_iso8859_1 );

    enc = encmatch(loc,false);
    if ( enc==e_unknown ) {
	loc = strrchr(loc,'.');
	if ( loc==NULL )
return( e_iso8859_1 );
	enc = encmatch(loc+1,true);
    }
    if ( enc==e_unknown )
return( e_iso8859_1 );

return( enc );
}

static void DefaultXUID(void) {
    /* Adobe has assigned PfaEdit a base XUID of 1021. Each new user is going */
    /*  to get a couple of random numbers appended to that, hoping that will */
    /*  make for a fairly safe system. */
    /* FontForge will use the same scheme */
    int r1, r2;
    char buffer[50];
    struct timeval tv;

    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);
    do {
	r1 = rand()&0x3ff;
    } while ( r1==0 );		/* I reserve "0" for me! */
    gettimeofday(&tv,NULL);
    g_random_set_seed(tv.tv_usec+1);
    r2 = g_random_int();
    sprintf( buffer, "1021 %d %d", r1, r2 );
    if (xuid != NULL) free(xuid);
    xuid = copy(buffer);
}

static void PrefsUI_SetDefaults(void) {

    DefaultXUID();
    local_encoding = DefaultEncoding();
}

static void ParseMacMapping(char *pt,struct macsettingname *ms) {
    char *end;

    ms->mac_feature_type = strtol(pt,&end,10);
    if ( *end==',' ) ++end;
    ms->mac_feature_setting = strtol(end,&end,10);
    if ( *end==' ' ) ++end;
    ms->otf_tag =
	((end[0]&0xff)<<24) |
	((end[1]&0xff)<<16) |
	((end[2]&0xff)<<8) |
	(end[3]&0xff);
}

static void ParseNewMacFeature(FILE *p,char *line) {
    fseek(p,-(strlen(line)-strlen("MacFeat:")),SEEK_CUR);
    line[strlen("MacFeat:")] ='\0';
    default_mac_feature_map = SFDParseMacFeatures(p,line);
    fseek(p,-strlen(line),SEEK_CUR);
    if ( user_mac_feature_map!=NULL )
	MacFeatListFree(user_mac_feature_map);
    user_mac_feature_map = default_mac_feature_map;
}

static void PrefsUI_LoadPrefs_FromFile( char* filename )
{
    FILE *p;
    char line[1100];
    int i, j, ri=0, mn=0, ms=0, fn=0, ff=0, filt_max=0;
    int msp=0, msc=0;
    char *pt;
    struct prefs_list *pl;

    if ( filename!=NULL && (p=fopen(filename,"r"))!=NULL ) {
	while ( fgets(line,sizeof(line),p)!=NULL ) {
	    if ( *line=='#' )
	continue;
	    pt = strchr(line,':');
	    if ( pt==NULL )
	continue;
	    for ( j=0; load_prefs_list[j]!=NULL; ++j ) {
		for ( i=0; load_prefs_list[j][i].name!=NULL; ++i )
		    if ( strncmp(line,load_prefs_list[j][i].name,pt-line)==0 )
		break;
		if ( load_prefs_list[j][i].name!=NULL )
	    break;
	    }
	    pl = NULL;
	    if ( load_prefs_list[j]!=NULL )
		pl = &load_prefs_list[j][i];
	    for ( ++pt; *pt=='\t'; ++pt );
	    if ( line[strlen(line)-1]=='\n' )
		line[strlen(line)-1] = '\0';
	    if ( line[strlen(line)-1]=='\r' )
		line[strlen(line)-1] = '\0';
	    if ( pl==NULL ) {
		if ( strncmp(line,"Recent:",strlen("Recent:"))==0 && ri<RECENT_MAX )
		    RecentFiles[ri++] = copy(pt);
		else if ( strncmp(line,"MenuScript:",strlen("MenuScript:"))==0 && ms<SCRIPT_MENU_MAX )
		    script_filenames[ms++] = copy(pt);
		else if ( strncmp(line,"MenuName:",strlen("MenuName:"))==0 && mn<SCRIPT_MENU_MAX )
		    script_menu_names[mn++] = utf82u_copy(pt);
		else if ( strncmp(line,"FontFilterName:",strlen("FontFilterName:"))==0 ) {
		    if ( fn>=filt_max )
			user_font_filters = realloc(user_font_filters,((filt_max+=10)+1)*sizeof( struct openfilefilters));
		    user_font_filters[fn].filter = NULL;
		    user_font_filters[fn++].name = copy(pt);
		    user_font_filters[fn].name = NULL;
		} else if ( strncmp(line,"FontFilter:",strlen("FontFilter:"))==0 ) {
		    if ( ff<filt_max )
			user_font_filters[ff++].filter = copy(pt);
		} else if ( strncmp(line,"MacMapCnt:",strlen("MacSetCnt:"))==0 ) {
		    sscanf( pt, "%d", &msc );
		    msp = 0;
		    user_macfeat_otftag = calloc(msc+1,sizeof(struct macsettingname));
		} else if ( strncmp(line,"MacMapping:",strlen("MacMapping:"))==0 && msp<msc ) {
		    ParseMacMapping(pt,&user_macfeat_otftag[msp++]);
		} else if ( strncmp(line,"MacFeat:",strlen("MacFeat:"))==0 ) {
		    ParseNewMacFeature(p,line);
		}
	continue;
	    }
	    switch ( pl->type ) {
	      case pr_encoding:
		{ Encoding *enc = FindOrMakeEncoding(pt);
		    if ( enc==NULL )
			enc = FindOrMakeEncoding("ISO8859-1");
		    if ( enc==NULL )
			enc = &custom;
		    *((Encoding **) (pl->val)) = enc;
		}
	      break;
	      case pr_namelist:
		{ NameList *nl = NameListByName(pt);
		    if ( strcmp(pt,"NULL")==0 && pl->val != &namelist_for_new_fonts )
			*((NameList **) (pl->val)) = NULL;
		    else if ( nl!=NULL )
			*((NameList **) (pl->val)) = nl;
		}
	      break;
	      case pr_bool: case pr_int:
		sscanf( pt, "%d", (int *) pl->val );
	      break;
	      case pr_unicode:
		if ( sscanf( pt, "U+%x", (int *) pl->val )!=1 )
		    if ( sscanf( pt, "u+%x", (int *) pl->val )!=1 )
			sscanf( pt, "%x", (int *) pl->val );
	      break;
	      case pr_real: case pr_angle:
		{ char *end;
		    *((float *) pl->val) = strtod(pt,&end);
		    if (( *end==',' || *end=='.' ) ) {
			*end = (*end=='.')?',':'.';
			*((float *) pl->val) = strtod(pt,NULL);
		    }
		}
		if ( pl->type == pr_angle )
		    *(float *) pl->val /= RAD2DEG;
	      break;
	      case pr_string: case pr_file:
		if ( *pt=='\0' ) pt=NULL;
		if ( pl->val!=NULL )
		    *((char **) (pl->val)) = copy(pt);
		else
		    (pl->set)(copy(pt));
	      break;
	    }
	}
	fclose(p);
    }
}

void Prefs_LoadDefaultPreferences( void )
{
    char filename[PATH_MAX+1];
    char* sharedir = getShareDir();

    snprintf(filename,PATH_MAX,"%s/prefs", sharedir );
    PrefsUI_LoadPrefs_FromFile( filename );
}


static void PrefsUI_LoadPrefs(void)
{
    char *prefs = getPfaEditPrefs();
    FILE *p;
    char line[1100], path[PATH_MAX];
    int i, j, ri=0, mn=0, ms=0, fn=0, ff=0, filt_max=0;
    int msp=0, msc=0;
    char *pt, *real_xdefs_filename = NULL;
    struct prefs_list *pl;

    LoadPluginDir(NULL);
    LoadPfaEditEncodings();
    LoadGroupList();

    if ( prefs!=NULL && (p=fopen(prefs,"r"))!=NULL ) {
	while ( fgets(line,sizeof(line),p)!=NULL ) {
	    if ( *line=='#' )
	continue;
	    pt = strchr(line,':');
	    if ( pt==NULL )
	continue;
	    for ( j=0; load_prefs_list[j]!=NULL; ++j ) {
		for ( i=0; load_prefs_list[j][i].name!=NULL; ++i )
		    if ( strncmp(line,load_prefs_list[j][i].name,pt-line)==0 )
		break;
		if ( load_prefs_list[j][i].name!=NULL )
	    break;
	    }
	    pl = NULL;
	    if ( load_prefs_list[j]!=NULL )
		pl = &load_prefs_list[j][i];
	    for ( ++pt; *pt=='\t'; ++pt );
	    if ( line[strlen(line)-1]=='\n' )
		line[strlen(line)-1] = '\0';
	    if ( line[strlen(line)-1]=='\r' )
		line[strlen(line)-1] = '\0';
	    if ( pl==NULL ) {
		if ( strncmp(line,"Recent:",strlen("Recent:"))==0 && ri<RECENT_MAX )
		    RecentFiles[ri++] = copy(pt);
		else if ( strncmp(line,"MenuScript:",strlen("MenuScript:"))==0 && ms<SCRIPT_MENU_MAX )
		    script_filenames[ms++] = copy(pt);
		else if ( strncmp(line,"MenuName:",strlen("MenuName:"))==0 && mn<SCRIPT_MENU_MAX )
		    script_menu_names[mn++] = utf82u_copy(pt);
		else if ( strncmp(line,"FontFilterName:",strlen("FontFilterName:"))==0 ) {
		    if ( fn>=filt_max )
			user_font_filters = realloc(user_font_filters,((filt_max+=10)+1)*sizeof( struct openfilefilters));
		    user_font_filters[fn].filter = NULL;
		    user_font_filters[fn++].name = copy(pt);
		    user_font_filters[fn].name = NULL;
		} else if ( strncmp(line,"FontFilter:",strlen("FontFilter:"))==0 ) {
		    if ( ff<filt_max )
			user_font_filters[ff++].filter = copy(pt);
		} else if ( strncmp(line,"MacMapCnt:",strlen("MacSetCnt:"))==0 ) {
		    sscanf( pt, "%d", &msc );
		    msp = 0;
		    user_macfeat_otftag = calloc(msc+1,sizeof(struct macsettingname));
		} else if ( strncmp(line,"MacMapping:",strlen("MacMapping:"))==0 && msp<msc ) {
		    ParseMacMapping(pt,&user_macfeat_otftag[msp++]);
		} else if ( strncmp(line,"MacFeat:",strlen("MacFeat:"))==0 ) {
		    ParseNewMacFeature(p,line);
		}
	continue;
	    }
	    switch ( pl->type ) {
	      case pr_encoding:
		{ Encoding *enc = FindOrMakeEncoding(pt);
		    if ( enc==NULL )
			enc = FindOrMakeEncoding("ISO8859-1");
		    if ( enc==NULL )
			enc = &custom;
		    *((Encoding **) (pl->val)) = enc;
		}
	      break;
	      case pr_namelist:
		{ NameList *nl = NameListByName(pt);
		    if ( strcmp(pt,"NULL")==0 && pl->val != &namelist_for_new_fonts )
			*((NameList **) (pl->val)) = NULL;
		    else if ( nl!=NULL )
			*((NameList **) (pl->val)) = nl;
		}
	      break;
	      case pr_bool: case pr_int:
		sscanf( pt, "%d", (int *) pl->val );
	      break;
	      case pr_unicode:
		if ( sscanf( pt, "U+%x", (int *) pl->val )!=1 )
		    if ( sscanf( pt, "u+%x", (int *) pl->val )!=1 )
			sscanf( pt, "%x", (int *) pl->val );
	      break;
	      case pr_real: case pr_angle:
		{ char *end;
		    *((float *) pl->val) = strtod(pt,&end);
		    if (( *end==',' || *end=='.' ) ) {
			*end = (*end=='.')?',':'.';
			*((float *) pl->val) = strtod(pt,NULL);
		    }
		}
		if ( pl->type == pr_angle )
		    *(float *) pl->val /= RAD2DEG;
	      break;
	      case pr_string: case pr_file:
		if ( *pt=='\0' ) pt=NULL;
		if ( pl->val!=NULL )
		    *((char **) (pl->val)) = copy(pt);
		else
		    (pl->set)(copy(pt));
	      break;
	    }
	}
	fclose(p);
    }

    //
    // If the user has no theme set, then use the default
    //
    real_xdefs_filename = xdefs_filename;
    if ( !real_xdefs_filename )
    {
	fprintf(stderr,"no xdefs_filename!\n");
	if (!quiet) {
	    fprintf(stderr,"TESTING: getPixmapDir:%s\n", getPixmapDir() );
	    fprintf(stderr,"TESTING: getShareDir:%s\n", getShareDir() );
	    fprintf(stderr,"TESTING: GResourceProgramDir:%s\n", GResourceProgramDir );
	}
	snprintf(path, PATH_MAX, "%s/%s", getPixmapDir(), "resources" );
	if (!quiet)
	    fprintf(stderr,"trying default theme:%s\n", path );
	real_xdefs_filename = path;
    }
    GResourceAddResourceFile(real_xdefs_filename,GResourceProgramName,true);

    if ( othersubrsfile!=NULL && ReadOtherSubrsFile(othersubrsfile)<=0 )
	fprintf( stderr, "Failed to read OtherSubrs from %s\n", othersubrsfile );

    if ( glyph_2_name_map )
	old_sfnt_flags |= ttf_flag_glyphmap;
    LoadNamelistDir(NULL);
    ProcessFileChooserPrefs();
    GDrawEnableCairo( prefs_usecairo );
}

static void PrefsUI_SavePrefs(int not_if_script) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    int i, j;
    char *temp;
    struct prefs_list *pl;
    extern int running_script;

    if ( prefs==NULL )
return;
    if ( not_if_script && running_script )
return;

    if ( (p=fopen(prefs,"w"))==NULL )
return;

    GetFileChooserPrefs();

    for ( j=0; prefs_list[j]!=NULL; ++j ) for ( i=0; prefs_list[j][i].name!=NULL; ++i ) {
	pl = &prefs_list[j][i];
	switch ( pl->type ) {
	  case pr_encoding:
	    fprintf( p, "%s:\t%s\n", pl->name, (*((Encoding **) (pl->val)))->enc_name );
	  break;
	  case pr_namelist:
	    fprintf( p, "%s:\t%s\n", pl->name, *((NameList **) (pl->val))==NULL ? "NULL" :
		    (*((NameList **) (pl->val)))->title );
	  break;
	  case pr_bool: case pr_int:
	    fprintf( p, "%s:\t%d\n", pl->name, *(int *) (pl->val) );
	  break;
	  case pr_unicode:
	    fprintf( p, "%s:\tU+%04x\n", pl->name, *(int *) (pl->val) );
	  break;
	  case pr_real:
	    fprintf( p, "%s:\t%g\n", pl->name, (double) *(float *) (pl->val) );
	  break;
	  case pr_string: case pr_file:
	    if ( (pl->val)!=NULL )
		temp = *(char **) (pl->val);
	    else
		temp = (char *) (pl->get());
	    if ( temp!=NULL )
		fprintf( p, "%s:\t%s\n", pl->name, temp );
	    if ( (pl->val)==NULL )
		free(temp);
	  break;
	  case pr_angle:
	    fprintf( p, "%s:\t%g\n", pl->name, ((double) *(float *) pl->val) * RAD2DEG );
	  break;
	}
    }

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i )
	fprintf( p, "Recent:\t%s\n", RecentFiles[i]);
    for ( i=0; i<SCRIPT_MENU_MAX && script_filenames[i]!=NULL; ++i ) {
	fprintf( p, "MenuScript:\t%s\n", script_filenames[i]);
	fprintf( p, "MenuName:\t%s\n", temp = u2utf8_copy(script_menu_names[i]));
	free(temp);
    }
    if ( user_font_filters!=NULL ) {
	for ( i=0; user_font_filters[i].name!=NULL; ++i ) {
	    fprintf( p, "FontFilterName:\t%s\n", user_font_filters[i].name);
	    fprintf( p, "FontFilter:\t%s\n", user_font_filters[i].filter);
	}
    }
    if ( user_macfeat_otftag!=NULL && UserSettingsDiffer()) {
	for ( i=0; user_macfeat_otftag[i].otf_tag!=0; ++i );
	fprintf( p, "MacMapCnt: %d\n", i );
	for ( i=0; user_macfeat_otftag[i].otf_tag!=0; ++i ) {
	    fprintf( p, "MacMapping: %d,%d %c%c%c%c\n",
		    user_macfeat_otftag[i].mac_feature_type,
		    user_macfeat_otftag[i].mac_feature_setting,
			(int) (user_macfeat_otftag[i].otf_tag>>24),
			(int) ((user_macfeat_otftag[i].otf_tag>>16)&0xff),
			(int) ((user_macfeat_otftag[i].otf_tag>>8)&0xff),
			(int) (user_macfeat_otftag[i].otf_tag&0xff) );
	}
    }

    if ( UserFeaturesDiffer())
	SFDDumpMacFeat(p,default_mac_feature_map);

    fclose(p);
}

struct pref_data {
    int done;
    struct prefs_list* plist;
};

static int Prefs_ScriptBrowse(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *tf = GWidgetGetControl(gw,GGadgetGetCid(g)-SCRIPT_MENU_MAX);
	char *cur = GGadgetGetTitle8(tf); char *ret;

	if ( *cur=='\0' ) cur=NULL;
	ret = gwwv_open_filename(_("Call Script"), cur, "*.pe", NULL);
	free(cur);
	if ( ret==NULL )
return(true);
	GGadgetSetTitle8(tf,ret);
	free(ret);
    }
return( true );
}

static int Prefs_BrowseFile(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *tf = GWidgetGetControl(gw,GGadgetGetCid(g)-CID_PrefsBrowseOffset);
	char *cur = GGadgetGetTitle8(tf); char *ret;
	struct prefs_list *pl = GGadgetGetUserData(tf);

	ret = gwwv_open_filename(pl->name, *cur=='\0'? NULL : cur, NULL, NULL);
	free(cur);
	if ( ret==NULL )
return(true);
	GGadgetSetTitle8(tf,ret);
	free(ret);
    }
return( true );
}

static GTextInfo *Pref_MappingList(int use_user) {
    struct macsettingname *msn = use_user && user_macfeat_otftag!=NULL ?
	    user_macfeat_otftag :
	    macfeat_otftag;
    GTextInfo *ti;
    int i;
    char buf[60];

    for ( i=0; msn[i].otf_tag!=0; ++i );
    ti = calloc(i+1,sizeof( GTextInfo ));

    for ( i=0; msn[i].otf_tag!=0; ++i ) {
	sprintf(buf,"%3d,%2d %c%c%c%c",
	    msn[i].mac_feature_type, msn[i].mac_feature_setting,
	    (int) (msn[i].otf_tag>>24), (int) ((msn[i].otf_tag>>16)&0xff), (int) ((msn[i].otf_tag>>8)&0xff), (int) (msn[i].otf_tag&0xff) );
	ti[i].text = uc_copy(buf);
    }
return( ti );
}

void GListAddStr(GGadget *list,unichar_t *str, void *ud) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    GTextInfo **replace = malloc((len+2)*sizeof(GTextInfo *));

    replace[len+1] = calloc(1,sizeof(GTextInfo));
    for ( i=0; i<len; ++i ) {
	replace[i] = malloc(sizeof(GTextInfo));
	*replace[i] = *ti[i];
	replace[i]->text = u_copy(ti[i]->text);
    }
    replace[i] = calloc(1,sizeof(GTextInfo));
    replace[i]->fg = replace[i]->bg = COLOR_DEFAULT;
    replace[i]->text = str;
    replace[i]->userdata = ud;
    GGadgetSetList(list,replace,false);
}

void GListReplaceStr(GGadget *list,int index, unichar_t *str, void *ud) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    GTextInfo **replace = malloc((len+2)*sizeof(GTextInfo *));

    for ( i=0; i<len; ++i ) {
	replace[i] = malloc(sizeof(GTextInfo));
	*replace[i] = *ti[i];
	if ( i!=index )
	    replace[i]->text = u_copy(ti[i]->text);
    }
    replace[i] = calloc(1,sizeof(GTextInfo));
    replace[index]->text = str;
    replace[index]->userdata = ud;
    GGadgetSetList(list,replace,false);
}

struct setdata {
    GWindow gw;
    GGadget *list;
    GGadget *flist;
    GGadget *feature;
    GGadget *set_code;
    GGadget *otf;
    GGadget *ok;
    GGadget *cancel;
    int index;
    int done;
    unichar_t *ret;
};

static int set_e_h(GWindow gw, GEvent *event) {
    struct setdata *sd = GDrawGetUserData(gw);
    int i;
    int32 len;
    GTextInfo **ti;
    const unichar_t *ret1; unichar_t *end;
    int on, feat, val1, val2;
    unichar_t ubuf[4];
    char buf[40];

    if ( event->type==et_close ) {
	sd->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("prefs.html#Features");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	if ( event->u.control.g == sd->cancel ) {
	    sd->done = true;
	} else if ( event->u.control.g == sd->ok ) {
	    ret1 = _GGadgetGetTitle(sd->set_code);
	    on = u_strtol(ret1,&end,10);
	    if ( *end!='\0' ) {
		ff_post_error(_("Bad Number"),_("Bad Number"));
return( true );
	    }
	    ret1 = _GGadgetGetTitle(sd->feature);
	    feat = u_strtol(ret1,&end,10);
	    if ( *end!='\0' && *end!=' ' ) {
		ff_post_error(_("Bad Number"),_("Bad Number"));
return( true );
	    }
	    ti = GGadgetGetList(sd->list,&len);
	    for ( i=0; i<len; ++i ) if ( i!=sd->index ) {
		val1 = u_strtol(ti[i]->text,&end,10);
		val2 = u_strtol(end+1,NULL,10);
		if ( val1==feat && val2==on ) {
		    static char *buts[3];
		    buts[0] = _("_Yes");
		    buts[1] = _("_No");
		    buts[2] = NULL;
		    if ( gwwv_ask(_("This feature, setting combination is already used"),(const char **) buts,0,1,
			    _("This feature, setting combination is already used\nDo you really wish to reuse it?"))==1 )
return( true );
		}
	    }

	    ret1 = _GGadgetGetTitle(sd->otf);
	    if ( (ubuf[0] = ret1[0])==0 )
		ubuf[0] = ubuf[1] = ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[1] = ret1[1])==0 )
		ubuf[1] = ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[2] = ret1[2])==0 )
		ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[3] = ret1[3])==0 )
		ubuf[3] = ' ';
	    len = u_strlen(ret1);
	    if ( len<2 || len>4 || ubuf[0]>=0x7f || ubuf[1]>=0x7f || ubuf[2]>=0x7f || ubuf[3]>=0x7f ) {
		ff_post_error(_("Tag too long"),_("Feature tags must be exactly 4 ASCII characters"));
return( true );
	    }
	    sprintf(buf,"%3d,%2d %c%c%c%c",
		    feat, on,
		    ubuf[0], ubuf[1], ubuf[2], ubuf[3]);
	    sd->done = true;
	    sd->ret = uc_copy(buf);
	}
    }
return( true );
}

static unichar_t *AskSetting(struct macsettingname *temp,GGadget *list, int index,GGadget *flist) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[17];
    GTextInfo label[17];
    struct setdata sd;
    char buf[20];
    unichar_t ubuf3[6];
    int32 len, i;
    GTextInfo **ti;

    memset(&sd,0,sizeof(sd));
    sd.list = list;
    sd.flist = flist;
    sd.index = index;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Mapping");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,240));
    pos.height = GDrawPointsToPixels(NULL,120);
    gw = GDrawCreateTopWindow(NULL,&pos,set_e_h,&sd,&wattrs);
    sd.gw = gw;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    label[0].text = (unichar_t *) _("_Feature:");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+4;
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 50; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 170;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].creator = GListButtonCreate;

    label[2].text = (unichar_t *) _("Setting");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[0].gd.pos.y+26;
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( buf, "%d", temp->mac_feature_setting );
    label[3].text = (unichar_t *) buf;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[2].gd.pos.y-4; gcd[3].gd.pos.width = 50;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].creator = GTextFieldCreate;

    label[4].text = (unichar_t *) _("_Tag:");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[3].gd.pos.y+26;
    gcd[4].gd.flags = gg_enabled|gg_visible;
    gcd[4].creator = GLabelCreate;

    ubuf3[0] = temp->otf_tag>>24; ubuf3[1] = (temp->otf_tag>>16)&0xff; ubuf3[2] = (temp->otf_tag>>8)&0xff; ubuf3[3] = temp->otf_tag&0xff; ubuf3[4] = 0;
    label[5].text = ubuf3;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = gcd[3].gd.pos.x; gcd[5].gd.pos.y = gcd[4].gd.pos.y-4; gcd[5].gd.pos.width = 50;
    gcd[5].gd.flags = gg_enabled|gg_visible;
    /*gcd[5].gd.u.list = tags;*/
    gcd[5].creator = GTextFieldCreate;

    gcd[6].gd.pos.x = 13-3; gcd[6].gd.pos.y = gcd[5].gd.pos.y+30;
    gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[6].text = (unichar_t *) _("_OK");
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    /*gcd[6].gd.handle_controlevent = Prefs_Ok;*/
    gcd[6].creator = GButtonCreate;

    gcd[7].gd.pos.x = -13; gcd[7].gd.pos.y = gcd[7-1].gd.pos.y+3;
    gcd[7].gd.pos.width = -1; gcd[7].gd.pos.height = 0;
    gcd[7].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[7].text = (unichar_t *) _("_Cancel");
    label[7].text_is_1byte = true;
    label[7].text_in_resource = true;
    gcd[7].gd.label = &label[7];
    gcd[7].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    sd.feature = gcd[1].ret;
    sd.set_code = gcd[3].ret;
    sd.otf = gcd[5].ret;
    sd.ok = gcd[6].ret;
    sd.cancel = gcd[7].ret;

    ti = GGadgetGetList(flist,&len);
    GGadgetSetList(sd.feature,ti,true);
    for ( i=0; i<len; ++i ) {
	int val = u_strtol(ti[i]->text,NULL,10);
	if ( val==temp->mac_feature_type ) {
	    GGadgetSetTitle(sd.feature,ti[i]->text);
    break;
	}
    }

    GDrawSetVisible(gw,true);
    GWidgetIndicateFocusGadget(gcd[1].ret);
    while ( !sd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

return( sd.ret );
}

static void ChangeSetting(GGadget *list,int index,GGadget *flist) {
    struct macsettingname temp;
    int32 len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    char *str;
    unichar_t *ustr;

    str = cu_copy(ti[index]->text);
    ParseMacMapping(str,&temp);
    free(str);
    if ( (ustr=AskSetting(&temp,list,index,flist))==NULL )
return;
    GListReplaceStr(list,index,ustr,NULL);
}

static int Pref_NewMapping(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_Mapping);
	GGadget *flist = GWidgetGetControl(GDrawGetParentWindow(gw),CID_Features);
	struct macsettingname temp;
	unichar_t *str;

	memset(&temp,0,sizeof(temp));
	temp.mac_feature_type = -1;
	if ( (str=AskSetting(&temp,list,-1,flist))==NULL )
return( true );
	GListAddStr(list,str,NULL);
	/*free(str);*/
    }
return( true );
}

static int Pref_DelMapping(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GListDelSelected(GWidgetGetControl(gw,CID_Mapping));
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MappingDel),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MappingEdit),false);
    }
return( true );
}

static int Pref_EditMapping(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
	GGadget *list = GWidgetGetControl(gw,CID_Mapping);
	GGadget *flist = GWidgetGetControl(gw,CID_Features);
	ChangeSetting(list,GGadgetGetFirstListSelectedItem(list),flist);
    }
return( true );
}

static int Pref_MappingSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	GWindow gw = GGadgetGetWindow(g);
	int i, sel_cnt=0;
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected ) ++sel_cnt;
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MappingDel),sel_cnt!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_MappingEdit),sel_cnt==1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	GGadget *flist = GWidgetGetControl( GDrawGetParentWindow(GGadgetGetWindow(g)),CID_Features);
	ChangeSetting(g,e->u.control.u.list.changed_index!=-1?e->u.control.u.list.changed_index:
		GGadgetGetFirstListSelectedItem(g),flist);
    }
return( true );
}

static int Pref_DefaultMapping(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GGadget *list = GWidgetGetControl(GGadgetGetWindow(g),CID_Mapping);
	GTextInfo *ti, **arr;
	uint16 cnt;

	ti = Pref_MappingList(false);
	arr = GTextInfoArrayFromList(ti,&cnt);
	GGadgetSetList(list,arr,false);
	GTextInfoListFree(ti);
    }
return( true );
}

static int Prefs_Ok(GGadget *g, GEvent *e) {
    int i, j, mi;
    int err=0, enc;
    struct pref_data *p;
    GWindow gw;
    const unichar_t *ret;
    const unichar_t *names[SCRIPT_MENU_MAX], *scripts[SCRIPT_MENU_MAX];
    struct prefs_list *pl;
    GTextInfo **list;
    int32 len;
    int maxl, t;
    char *str;
    real dangle;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	p = GDrawGetUserData(gw);
	for ( i=0; i<SCRIPT_MENU_MAX; ++i ) {
	    names[i] = _GGadgetGetTitle(GWidgetGetControl(gw,CID_ScriptMNameBase+i));
	    scripts[i] = _GGadgetGetTitle(GWidgetGetControl(gw,CID_ScriptMFileBase+i));
	    if ( *names[i]=='\0' ) names[i] = NULL;
	    if ( *scripts[i]=='\0' ) scripts[i] = NULL;
	    if ( scripts[i]==NULL && names[i]!=NULL ) {
		ff_post_error(_("Menu name with no associated script"),_("Menu name with no associated script"));
return( true );
	    } else if ( scripts[i]!=NULL && names[i]==NULL ) {
		ff_post_error(_("Script with no associated menu name"),_("Script with no associated menu name"));
return( true );
	    }
	}
	for ( i=mi=0; i<SCRIPT_MENU_MAX; ++i ) {
	    if ( names[i]!=NULL ) {
		names[mi] = names[i];
		scripts[mi] = scripts[i];
		++mi;
	    }
	}
	for ( j=0; visible_prefs_list[j].tab_name!=0; ++j ) for ( i=0; visible_prefs_list[j].pl[i].name!=NULL; ++i ) {
	    pl = &visible_prefs_list[j].pl[i];
	    /* before assigning values, check for any errors */
	    /* if any errors, then NO values should be assigned, in case they cancel */
	    if ( pl->dontdisplay )
	continue;
	    if ( pl->type==pr_int ) {
		GetInt8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	    } else if ( pl->type==pr_real ) {
		GetReal8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	    } else if ( pl->type==pr_angle ) {
		dangle = GetReal8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
		if ( dangle > 90 || dangle < 0 ) {
		    GGadgetProtest8(pl->name);
		    err = true;
		}
	    } else if ( pl->type==pr_unicode ) {
		GetUnicodeChar8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	    }
	}
	if ( err )
return( true );

	for ( j=0; visible_prefs_list[j].tab_name!=0; ++j ) for ( i=0; visible_prefs_list[j].pl[i].name!=NULL; ++i ) {
	    pl = &visible_prefs_list[j].pl[i];
	    if ( pl->dontdisplay )
	continue;
	    switch( pl->type ) {
	      case pr_int:
	        *((int *) (pl->val)) = GetInt8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	      break;
	      case pr_unicode:
	        *((int *) (pl->val)) = GetUnicodeChar8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	      break;
	      case pr_bool:
	        *((int *) (pl->val)) = GGadgetIsChecked(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
	      break;
	      case pr_real:
	        *((float *) (pl->val)) = GetReal8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	      break;
	      case pr_encoding:
		{ Encoding *e;
		    e = ParseEncodingNameFromList(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
		    if ( e!=NULL )
			*((Encoding **) (pl->val)) = e;
		    enc = 1;	/* So gcc doesn't complain about unused. It is unused, but why add the ifdef and make the code even messier? Sigh. icc complains anyway */
		}
	      break;
	      case pr_namelist:
		{ NameList *nl;
		  GTextInfo *ti = GGadgetGetListItemSelected(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
		  if ( ti!=NULL ) {
			char *name = u2utf8_copy(ti->text);
			nl = NameListByName(name);
			free(name);
			if ( nl!=NULL && nl->uses_unicode && !allow_utf8_glyphnames)
			    ff_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set, but there are names in this namelist which use characters outside that range."));
			else if ( nl!=NULL )
			    *((NameList **) (pl->val)) = nl;
		    }
		}
	      break;
	      case pr_string: case pr_file:
	        ret = _GGadgetGetTitle(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
		if ( pl->val!=NULL ) {
		    free( *((char **) (pl->val)) );
		    *((char **) (pl->val)) = NULL;
		    if ( ret!=NULL && *ret!='\0' )
			*((char **) (pl->val)) = /* u2def_*/ cu_copy(ret);
		} else {
		    char *cret = cu_copy(ret);
		    (pl->set)(cret);
		    free(cret);
		}
	      break;
	      case pr_angle:
	        *((float *) (pl->val)) = GetReal8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err)/RAD2DEG;
	      break;
	    }
	}
	for ( i=0; i<SCRIPT_MENU_MAX; ++i ) {
	    free(script_menu_names[i]); script_menu_names[i] = NULL;
	    free(script_filenames[i]); script_filenames[i] = NULL;
	}
	for ( i=0; i<mi; ++i ) {
	    script_menu_names[i] = u_copy(names[i]);
	    script_filenames[i] = u2def_copy(scripts[i]);
	}

	list = GGadgetGetList(GWidgetGetControl(gw,CID_Mapping),&len);
	UserSettingsFree();
	user_macfeat_otftag = malloc((len+1)*sizeof(struct macsettingname));
	user_macfeat_otftag[len].otf_tag = 0;
	maxl = 0;
	for ( i=0; i<len; ++i ) {
	    t = u_strlen(list[i]->text);
	    if ( t>maxl ) maxl = t;
	}
	str = malloc(maxl+3);
	for ( i=0; i<len; ++i ) {
	    u2encoding_strncpy(str,list[i]->text,maxl+1,e_mac);
	    ParseMacMapping(str,&user_macfeat_otftag[i]);
	}
	free(str);

	Prefs_ReplaceMacFeatures(GWidgetGetControl(gw,CID_Features));

	if ( xuid!=NULL ) {
	    char *pt;
	    for ( pt=xuid; *pt==' ' ; ++pt );
	    if ( *pt=='[' ) {	/* People who know PS well, might want to put brackets arround the xuid base array, but I don't want them */
		pt = copy(pt+1);
		if (xuid != NULL) free( xuid );
		xuid = pt;
	    }
	    for ( pt=xuid+strlen(xuid)-1; pt>xuid && *pt==' '; --pt );
	    if ( pt >= xuid && *pt==']' ) *pt = '\0';
	}

	p->done = true;
	PrefsUI_SavePrefs(true);
	if ( maxundoes==0 ) { FontView *fv;
	    for ( fv=fv_list ; fv!=NULL; fv=(FontView *) (fv->b.next) )
		SFRemoveUndoes(fv->b.sf,NULL,NULL);
	}
	if ( othersubrsfile!=NULL && ReadOtherSubrsFile(othersubrsfile)<=0 )
	    fprintf( stderr, "Failed to read OtherSubrs from %s\n", othersubrsfile );
	GDrawEnableCairo(prefs_usecairo);

	int force_redraw_charviews = 0;
	if( prefs_oldval_cvEditHandleSize != prefs_cvEditHandleSize )
	    force_redraw_charviews = 1;
	if( prefs_oldval_cvInactiveHandleAlpha != prefs_cvInactiveHandleAlpha )
	    force_redraw_charviews = 1;


	if( force_redraw_charviews )
	{
	    FontView *fv;
	    for ( fv=fv_list ; fv!=NULL; fv=(FontView *) (fv->b.next) )
	    {
		FVRedrawAllCharViews( fv );
	    }
	}
    }
return( true );
}

static int Prefs_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct pref_data *p = GDrawGetUserData(GGadgetGetWindow(g));
	MacFeatListFree(GGadgetGetUserData((GWidgetGetControl(
		GGadgetGetWindow(g),CID_Features))));
	p->done = true;
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct pref_data *p = GDrawGetUserData(gw);
	p->done = true;
	if(GWidgetGetControl(gw,CID_Features)) {
	    MacFeatListFree(GGadgetGetUserData((GWidgetGetControl(gw,CID_Features))));
	}
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("prefs.html");
return( true );
	}
return( false );
    }
return( true );
}

static void PrefsInit(void) {
    static int done = false;
    int i;

    if ( done )
return;
    done = true;
    for ( i=0; visible_prefs_list[i].tab_name!=NULL; ++i )
	visible_prefs_list[i].tab_name = _(visible_prefs_list[i].tab_name);
}

void DoPrefs(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData *pgcd, gcd[5], sgcd[45], mgcd[3], mfgcd[9], msgcd[9];
    GGadgetCreateData mfboxes[3], *mfarray[14];
    GGadgetCreateData mpboxes[3], *mparray[14];
    GGadgetCreateData sboxes[2], *sarray[50];
    GGadgetCreateData mboxes[3], *varray[5], *harray[8];
    GTextInfo *plabel, **list, label[5], slabel[45], *plabels[TOPICS+5], mflabels[9], mslabels[9];
    GTabInfo aspects[TOPICS+5], subaspects[3];
    GGadgetCreateData **hvarray, boxes[2*TOPICS];
    struct pref_data p;
    int i, gc, sgc, j, k, line, line_max, y, y2, ii, si;
    int32 llen;
    char buf[20];
    int gcnt[20];
    static unichar_t nullstr[] = { 0 };
    struct prefs_list *pl;
    char *tempstr;
    FontRequest rq;
    GFont *font;

    PrefsInit();

    MfArgsInit();
    for ( k=line_max=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	for ( i=line=gcnt[k]=0; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	    if ( visible_prefs_list[k].pl[i].dontdisplay )
	continue;
	    gcnt[k] += 2;
	    if ( visible_prefs_list[k].pl[i].type==pr_bool ) ++gcnt[k];
	    else if ( visible_prefs_list[k].pl[i].type==pr_file ) ++gcnt[k];
	    else if ( visible_prefs_list[k].pl[i].type==pr_angle ) ++gcnt[k];
	    ++line;
	}
	if ( visible_prefs_list[k].pl == args_list ) {
	    gcnt[k] += 6;
	    line += 6;
	}
	if ( line>line_max ) line_max = line;
    }

    prefs_oldval_cvEditHandleSize = prefs_cvEditHandleSize;
    prefs_oldval_cvInactiveHandleAlpha = prefs_cvInactiveHandleAlpha;

    memset(&p,'\0',sizeof(p));
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Preferences");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,350));
    pos.height = GDrawPointsToPixels(NULL,line_max*26+69);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);

    memset(sgcd,0,sizeof(sgcd));
    memset(slabel,0,sizeof(slabel));
    memset(&mfgcd,0,sizeof(mfgcd));
    memset(&msgcd,0,sizeof(msgcd));
    memset(&mflabels,0,sizeof(mflabels));
    memset(&mslabels,0,sizeof(mslabels));
    memset(&mfboxes,0,sizeof(mfboxes));
    memset(&mpboxes,0,sizeof(mpboxes));
    memset(&sboxes,0,sizeof(sboxes));
    memset(&boxes,0,sizeof(boxes));

    GCDFillMacFeat(mfgcd,mflabels,250,default_mac_feature_map, true, mfboxes, mfarray);

    sgc = 0;

    msgcd[sgc].gd.pos.x = 6; msgcd[sgc].gd.pos.y = 6;
    msgcd[sgc].gd.pos.width = 250; msgcd[sgc].gd.pos.height = 16*12+10;
    msgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic | gg_list_multiplesel;
    msgcd[sgc].gd.cid = CID_Mapping;
    msgcd[sgc].gd.u.list = Pref_MappingList(true);
    msgcd[sgc].gd.handle_controlevent = Pref_MappingSel;
    msgcd[sgc++].creator = GListCreate;
    mparray[0] = &msgcd[sgc-1];

    msgcd[sgc].gd.pos.x = 6; msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y+msgcd[sgc-1].gd.pos.height+10;
    msgcd[sgc].gd.flags = gg_visible | gg_enabled;
    mslabels[sgc].text = (unichar_t *) S_("MacMap|_New...");
    mslabels[sgc].text_is_1byte = true;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.handle_controlevent = Pref_NewMapping;
    msgcd[sgc++].creator = GButtonCreate;
    mparray[4] = GCD_Glue; mparray[5] = &msgcd[sgc-1];

    msgcd[sgc].gd.pos.x = msgcd[sgc-1].gd.pos.x+10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y;
    msgcd[sgc].gd.flags = gg_visible ;
    mslabels[sgc].text = (unichar_t *) _("_Delete");
    mslabels[sgc].text_is_1byte = true;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.cid = CID_MappingDel;
    msgcd[sgc].gd.handle_controlevent = Pref_DelMapping;
    msgcd[sgc++].creator = GButtonCreate;
    mparray[5] = GCD_Glue; mparray[6] = &msgcd[sgc-1];

    msgcd[sgc].gd.pos.x = msgcd[sgc-1].gd.pos.x+10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y;
    msgcd[sgc].gd.flags = gg_visible ;
    mslabels[sgc].text = (unichar_t *) _("_Edit...");
    mslabels[sgc].text_is_1byte = true;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.cid = CID_MappingEdit;
    msgcd[sgc].gd.handle_controlevent = Pref_EditMapping;
    msgcd[sgc++].creator = GButtonCreate;
    mparray[7] = GCD_Glue; mparray[8] = &msgcd[sgc-1];

    msgcd[sgc].gd.pos.x = msgcd[sgc-1].gd.pos.x+10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y;
    msgcd[sgc].gd.flags = gg_visible | gg_enabled;
    mslabels[sgc].text = (unichar_t *) S_("MacMapping|Default");
    mslabels[sgc].text_is_1byte = true;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.handle_controlevent = Pref_DefaultMapping;
    msgcd[sgc++].creator = GButtonCreate;
    mparray[9] = GCD_Glue; mparray[10] = &msgcd[sgc-1];
    mparray[11] = GCD_Glue; mparray[12] = NULL;

    mpboxes[2].gd.flags = gg_enabled|gg_visible;
    mpboxes[2].gd.u.boxelements = mparray+4;
    mpboxes[2].creator = GHBoxCreate;
    mparray[1] = GCD_Glue;
    mparray[2] = &mpboxes[2];
    mparray[3] = NULL;

    mpboxes[0].gd.flags = gg_enabled|gg_visible;
    mpboxes[0].gd.u.boxelements = mparray;
    mpboxes[0].creator = GVBoxCreate;

    sgc = 0;
    y2=5;
    si = 0;

    slabel[sgc].text = (unichar_t *) _("Menu Name");
    slabel[sgc].text_is_1byte = true;
    sgcd[sgc].gd.label = &slabel[sgc];
    sgcd[sgc].gd.popup_msg = (unichar_t *) _("You may create a script menu containing up to 10 frequently used scripts.\nEach entry in the menu needs both a name to display in the menu and\na script file to execute. The menu name may contain any unicode characters.\nThe button labeled \"...\" will allow you to browse for a script file.");
    sgcd[sgc].gd.pos.x = 8;
    sgcd[sgc].gd.pos.y = y2;
    sgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    sgcd[sgc++].creator = GLabelCreate;
    sarray[si++] = &sgcd[sgc-1];

    slabel[sgc].text = (unichar_t *) _("Script File");
    slabel[sgc].text_is_1byte = true;
    sgcd[sgc].gd.label = &slabel[sgc];
    sgcd[sgc].gd.popup_msg = (unichar_t *) _("You may create a script menu containing up to 10 frequently used scripts\nEach entry in the menu needs both a name to display in the menu and\na script file to execute. The menu name may contain any unicode characters.\nThe button labeled \"...\" will allow you to browse for a script file.");
    sgcd[sgc].gd.pos.x = 110;
    sgcd[sgc].gd.pos.y = y2;
    sgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    sgcd[sgc++].creator = GLabelCreate;
    sarray[si++] = &sgcd[sgc-1];
    sarray[si++] = GCD_Glue;
    sarray[si++] = NULL;

    y2 += 14;

    for ( i=0; i<SCRIPT_MENU_MAX; ++i ) {
	sgcd[sgc].gd.pos.x = 8; sgcd[sgc].gd.pos.y = y2;
	sgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_text_xim;
	slabel[sgc].text = script_menu_names[i]==NULL?nullstr:script_menu_names[i];
	sgcd[sgc].gd.label = &slabel[sgc];
	sgcd[sgc].gd.cid = i+CID_ScriptMNameBase;
	sgcd[sgc++].creator = GTextFieldCreate;
	sarray[si++] = &sgcd[sgc-1];

	sgcd[sgc].gd.pos.x = 110; sgcd[sgc].gd.pos.y = y2;
	sgcd[sgc].gd.flags = gg_visible | gg_enabled;
	slabel[sgc].text = (unichar_t *) (script_filenames[i]==NULL?"":script_filenames[i]);
	slabel[sgc].text_is_1byte = true;
	sgcd[sgc].gd.label = &slabel[sgc];
	sgcd[sgc].gd.cid = i+CID_ScriptMFileBase;
	sgcd[sgc++].creator = GTextFieldCreate;
	sarray[si++] = &sgcd[sgc-1];

	sgcd[sgc].gd.pos.x = 210; sgcd[sgc].gd.pos.y = y2;
	sgcd[sgc].gd.flags = gg_visible | gg_enabled;
	slabel[sgc].text = (unichar_t *) _("...");
	slabel[sgc].text_is_1byte = true;
	sgcd[sgc].gd.label = &slabel[sgc];
	sgcd[sgc].gd.cid = i+CID_ScriptMBrowseBase;
	sgcd[sgc].gd.handle_controlevent = Prefs_ScriptBrowse;
	sgcd[sgc++].creator = GButtonCreate;
	sarray[si++] = &sgcd[sgc-1];
	sarray[si++] = NULL;

	y2 += 26;
    }
    sarray[si++] = GCD_Glue; sarray[si++] = GCD_Glue; sarray[si++] = GCD_Glue;
    sarray[si++] = NULL;
    sarray[si++] = NULL;

    sboxes[0].gd.flags = gg_enabled|gg_visible;
    sboxes[0].gd.u.boxelements = sarray;
    sboxes[0].creator = GHVBoxCreate;

    memset(&mgcd,0,sizeof(mgcd));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&subaspects,'\0',sizeof(subaspects));
    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&aspects,'\0',sizeof(aspects));
    aspects[0].selected = true;

    for ( k=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	pgcd = calloc(gcnt[k]+4,sizeof(GGadgetCreateData));
	plabel = calloc(gcnt[k]+4,sizeof(GTextInfo));
	hvarray = calloc((gcnt[k]+6)*5+2,sizeof(GGadgetCreateData *));

	aspects[k].text = (unichar_t *) visible_prefs_list[k].tab_name;
	aspects[k].text_is_1byte = true;
	aspects[k].gcd = &boxes[2*k];
	aspects[k].nesting = visible_prefs_list[k].nest;
	plabels[k] = plabel;

	gc = si = 0;
	for ( i=line=0, y=5; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	    pl = &visible_prefs_list[k].pl[i];
	    if ( pl->dontdisplay )
	continue;
	    plabel[gc].text = (unichar_t *) _(pl->name);
	    plabel[gc].text_is_1byte = true;
	    pgcd[gc].gd.label = &plabel[gc];
	    pgcd[gc].gd.mnemonic = '\0';
	    pgcd[gc].gd.popup_msg = (unichar_t *) _(pl->popup);
	    pgcd[gc].gd.pos.x = 8;
	    pgcd[gc].gd.pos.y = y + 6;
	    pgcd[gc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    pgcd[gc++].creator = GLabelCreate;
	    hvarray[si++] = &pgcd[gc-1];

	    plabel[gc].text_is_1byte = true;
	    pgcd[gc].gd.label = &plabel[gc];
	    pgcd[gc].gd.mnemonic = '\0';
	    pgcd[gc].gd.popup_msg = (unichar_t *) _(pl->popup);
	    pgcd[gc].gd.pos.x = 110;
	    pgcd[gc].gd.pos.y = y;
	    pgcd[gc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    pgcd[gc].data = pl;
	    pgcd[gc].gd.cid = k*CID_PrefsOffset+CID_PrefsBase+i;
	    switch ( pl->type ) {
	      case pr_bool:
		plabel[gc].text = (unichar_t *) _("On");
		pgcd[gc].gd.pos.y += 3;
		pgcd[gc++].creator = GRadioCreate;
		hvarray[si++] = &pgcd[gc-1];
		pgcd[gc] = pgcd[gc-1];
		pgcd[gc].gd.pos.x += 50;
		pgcd[gc].gd.cid = 0;
		pgcd[gc].gd.label = &plabel[gc];
		plabel[gc].text = (unichar_t *) _("Off");
		plabel[gc].text_is_1byte = true;
		hvarray[si++] = &pgcd[gc];
		hvarray[si++] = GCD_Glue;
		if ( *((int *) pl->val))
		    pgcd[gc-1].gd.flags |= gg_cb_on;
		else
		    pgcd[gc].gd.flags |= gg_cb_on;
		++gc;
		y += 22;
	      break;
	      case pr_int:
		sprintf(buf,"%d", *((int *) pl->val));
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	      case pr_unicode:
		/*sprintf(buf,"U+%04x", *((int *) pl->val));*/
		{ char *pt; pt=buf; pt=utf8_idpb(pt,*((int *)pl->val),UTF8IDPB_NOZERO); *pt='\0'; }
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	      case pr_real:
		sprintf(buf,"%g", *((float *) pl->val));
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	      case pr_encoding:
		pgcd[gc].gd.u.list = GetEncodingTypes();
		pgcd[gc].gd.label = EncodingTypesFindEnc(pgcd[gc].gd.u.list,
			*(Encoding **) pl->val);
		for ( ii=0; pgcd[gc].gd.u.list[ii].text!=NULL ||pgcd[gc].gd.u.list[ii].line; ++ii )
		    if ( pgcd[gc].gd.u.list[ii].userdata!=NULL &&
			    (strcmp(pgcd[gc].gd.u.list[ii].userdata,"Compacted")==0 ||
			     strcmp(pgcd[gc].gd.u.list[ii].userdata,"Original")==0 ))
			pgcd[gc].gd.u.list[ii].disabled = true;
		pgcd[gc].creator = GListFieldCreate;
		pgcd[gc].gd.pos.width = 160;
		if ( pgcd[gc].gd.label==NULL ) pgcd[gc].gd.label = &encodingtypes[0];
		++gc;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_ColSpan; hvarray[si++] = GCD_ColSpan;
		y += 28;
	      break;
	      case pr_namelist:
	        { char **nlnames = AllNamelistNames();
		int cnt;
		GTextInfo *namelistnames;
		for ( cnt=0; nlnames[cnt]!=NULL; ++cnt);
		namelistnames = calloc(cnt+1,sizeof(GTextInfo));
		for ( cnt=0; nlnames[cnt]!=NULL; ++cnt) {
		    namelistnames[cnt].text = (unichar_t *) nlnames[cnt];
		    namelistnames[cnt].text_is_1byte = true;
		    if ( strcmp(_((*(NameList **) (pl->val))->title),nlnames[cnt])==0 ) {
			namelistnames[cnt].selected = true;
			pgcd[gc].gd.label = &namelistnames[cnt];
		    }
		}
		pgcd[gc].gd.u.list = namelistnames;
		pgcd[gc].creator = GListButtonCreate;
		pgcd[gc].gd.pos.width = 160;
		++gc;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_ColSpan; hvarray[si++] = GCD_ColSpan;
		y += 28;
	      } break;
	      case pr_string: case pr_file:
		if ( pl->set==SetAutoTraceArgs || ((char **) pl->val)==&mf_args )
		    pgcd[gc].gd.pos.width = 160;
		if ( pl->val!=NULL )
		    tempstr = *((char **) (pl->val));
		else
		    tempstr = (char *) ((pl->get)());
		if ( tempstr!=NULL )
		    plabel[gc].text = /* def2u_*/ uc_copy( tempstr );
		else if ( ((char **) pl->val)==&BDFFoundry )
		    plabel[gc].text = /* def2u_*/ uc_copy( "FontForge" );
		else
		    plabel[gc].text = /* def2u_*/ uc_copy( "" );
		plabel[gc].text_is_1byte = false;
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		if ( pl->type==pr_file ) {
		    pgcd[gc] = pgcd[gc-1];
		    pgcd[gc-1].gd.pos.width = 140;
		    hvarray[si++] = GCD_ColSpan;
		    pgcd[gc].gd.pos.x += 145;
		    pgcd[gc].gd.cid += CID_PrefsBrowseOffset;
		    pgcd[gc].gd.label = &plabel[gc];
		    plabel[gc].text = (unichar_t *) "...";
		    plabel[gc].text_is_1byte = true;
		    pgcd[gc].gd.handle_controlevent = Prefs_BrowseFile;
		    pgcd[gc++].creator = GButtonCreate;
		    hvarray[si++] = &pgcd[gc-1];
		} else if ( pl->set==SetAutoTraceArgs || ((char **) pl->val)==&mf_args ) {
		    hvarray[si++] = GCD_ColSpan;
		    hvarray[si++] = GCD_Glue;
		} else {
		    hvarray[si++] = GCD_Glue;
		    hvarray[si++] = GCD_Glue;
		}
		y += 26;
		if ( pl->val==NULL )
		    free(tempstr);
	      break;
	      case pr_angle:
		sprintf(buf,"%g", *((float *) pl->val) * RAD2DEG);
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		plabel[gc].text = (unichar_t *) U_("°");
		plabel[gc].text_is_1byte = true;
		pgcd[gc].gd.label = &plabel[gc];
		pgcd[gc].gd.pos.x = pgcd[gc-1].gd.pos.x+gcd[gc-1].gd.pos.width+2; pgcd[gc].gd.pos.y = pgcd[gc-1].gd.pos.y;
		pgcd[gc].gd.flags = gg_enabled|gg_visible;
		pgcd[gc++].creator = GLabelCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	    }
	    ++line;
	    hvarray[si++] = NULL;
	}
	if ( visible_prefs_list[k].pl == args_list ) {
	    static char *text[] = {
/* GT: See the long comment at "Property|New" */
/* GT: This and the next few strings show a limitation of my widget set which */
/* GT: cannot handle multi-line text labels. These strings should be concatenated */
/* GT: (after striping off "Prefs_App|") together, translated, and then broken up */
/* GT: to fit the dialog. There is an extra blank line, not used in English, */
/* GT: into which your text may extend if needed. */
		N_("Prefs_App|Normally FontForge will find applications by searching for"),
		N_("Prefs_App|them in your PATH environment variable, if you want"),
		N_("Prefs_App|to alter that behavior you may set an environment"),
		N_("Prefs_App|variable giving the full path spec of the application."),
		N_("Prefs_App|FontForge recognizes BROWSER, MF and AUTOTRACE."),
		N_("Prefs_App| "), /* A blank line */
		NULL };
	    y += 8;
	    for ( i=0; text[i]!=0; ++i ) {
		plabel[gc].text = (unichar_t *) S_(text[i]);
		plabel[gc].text_is_1byte = true;
		pgcd[gc].gd.label = &plabel[gc];
		pgcd[gc].gd.pos.x = 8;
		pgcd[gc].gd.pos.y = y;
		pgcd[gc].gd.flags = gg_visible | gg_enabled;
		pgcd[gc++].creator = GLabelCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_ColSpan; hvarray[si++] = GCD_ColSpan;
		hvarray[si++] = NULL;
		y += 12;
	    }
	}
	if ( y>y2 ) y2 = y;
	hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
	hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
	hvarray[si++] = NULL;
	hvarray[si++] = NULL;
	boxes[2*k].gd.flags = gg_enabled|gg_visible;
	boxes[2*k].gd.u.boxelements = hvarray;
	boxes[2*k].creator = GHVBoxCreate;
    }

    aspects[k].text = (unichar_t *) _("Script Menu");
    aspects[k].text_is_1byte = true;
    aspects[k++].gcd = sboxes;

    subaspects[0].text = (unichar_t *) _("Features");
    subaspects[0].text_is_1byte = true;
    subaspects[0].gcd = mfboxes;

    subaspects[1].text = (unichar_t *) _("Mapping");
    subaspects[1].text_is_1byte = true;
    subaspects[1].gcd = mpboxes;

    mgcd[0].gd.pos.x = 4; gcd[0].gd.pos.y = 6;
    mgcd[0].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-20;
    mgcd[0].gd.pos.height = y2;
    mgcd[0].gd.u.tabs = subaspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].creator = GTabSetCreate;

    aspects[k].text = (unichar_t *) _("Mac");
    aspects[k].text_is_1byte = true;
    aspects[k++].gcd = mgcd;

    gc = 0;

    gcd[gc].gd.pos.x = gcd[gc].gd.pos.y = 2;
    gcd[gc].gd.pos.width = pos.width-4; gcd[gc].gd.pos.height = pos.height-2;
    gcd[gc].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[gc++].creator = GGroupCreate;

    gcd[gc].gd.pos.x = 4; gcd[gc].gd.pos.y = 6;
    gcd[gc].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-8;
    gcd[gc].gd.pos.height = y2+20+18+4;
    gcd[gc].gd.u.tabs = aspects;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
    gcd[gc++].creator = GTabSetCreate;
    varray[0] = &gcd[gc-1]; varray[1] = NULL;

    y = gcd[gc-1].gd.pos.y+gcd[gc-1].gd.pos.height;

    gcd[gc].gd.pos.x = 30-3; gcd[gc].gd.pos.y = y+5-3;
    gcd[gc].gd.pos.width = -1; gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[gc].text = (unichar_t *) _("_OK");
    label[gc].text_is_1byte = true;
    label[gc].text_in_resource = true;
    gcd[gc].gd.mnemonic = 'O';
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.handle_controlevent = Prefs_Ok;
    gcd[gc++].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &gcd[gc-1]; harray[2] = GCD_Glue; harray[3] = GCD_Glue;

    gcd[gc].gd.pos.x = -30; gcd[gc].gd.pos.y = gcd[gc-1].gd.pos.y+3;
    gcd[gc].gd.pos.width = -1; gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[gc].text = (unichar_t *) _("_Cancel");
    label[gc].text_is_1byte = true;
    label[gc].text_in_resource = true;
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.mnemonic = 'C';
    gcd[gc].gd.handle_controlevent = Prefs_Cancel;
    gcd[gc++].creator = GButtonCreate;
    harray[4] = GCD_Glue; harray[5] = &gcd[gc-1]; harray[6] = GCD_Glue; harray[7] = NULL;

    memset(mboxes,0,sizeof(mboxes));
    mboxes[2].gd.flags = gg_enabled|gg_visible;
    mboxes[2].gd.u.boxelements = harray;
    mboxes[2].creator = GHBoxCreate;
    varray[2] = &mboxes[2];
    varray[3] = NULL;
    varray[4] = NULL;

    mboxes[0].gd.pos.x = mboxes[0].gd.pos.y = 2;
    mboxes[0].gd.flags = gg_enabled|gg_visible;
    mboxes[0].gd.u.boxelements = varray;
    mboxes[0].creator = GHVGroupCreate;

    y = GDrawPointsToPixels(NULL,y+37);
    gcd[0].gd.pos.height = y-4;

    GGadgetsCreate(gw,mboxes);
    GTextInfoListFree(mfgcd[0].gd.u.list);
    GTextInfoListFree(msgcd[0].gd.u.list);

    GHVBoxSetExpandableRow(mboxes[0].ret,0);
    GHVBoxSetExpandableCol(mboxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(mfboxes[0].ret,0);
    GHVBoxSetExpandableCol(mfboxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(mpboxes[0].ret,0);
    GHVBoxSetExpandableCol(mpboxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(sboxes[0].ret,gb_expandglue);
    for ( k=0; k<TOPICS; ++k )
	GHVBoxSetExpandableRow(boxes[2*k].ret,gb_expandglue);

    memset(&rq,0,sizeof(rq));
    rq.utf8_family_name = MONO_UI_FAMILIES;
    rq.point_size = 12;
    rq.weight = 400;
    font = GDrawInstanciateFont(gw,&rq);
    GGadgetSetFont(mfgcd[0].ret,font);
    GGadgetSetFont(msgcd[0].ret,font);
    GHVBoxFitWindow(mboxes[0].ret);

    for ( k=0; visible_prefs_list[k].tab_name!=0; ++k ) for ( gc=0,i=0; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	GGadgetCreateData *gcd = aspects[k].gcd[0].gd.u.boxelements[0];
	pl = &visible_prefs_list[k].pl[i];
	if ( pl->dontdisplay )
    continue;
	switch ( pl->type ) {
	  case pr_bool:
	    ++gc;
	  break;
	  case pr_encoding: {
	    GGadget *g = gcd[gc+1].ret;
	    list = GGadgetGetList(g,&llen);
	    for ( j=0; j<llen ; ++j ) {
		if ( list[j]->text!=NULL &&
			(void *) (intpt) ( *((int *) pl->val)) == list[j]->userdata )
		    list[j]->selected = true;
		else
		    list[j]->selected = false;
	    }
	    if ( gcd[gc+1].gd.u.list!=encodingtypes )
		GTextInfoListFree(gcd[gc+1].gd.u.list);
	  } break;
	  case pr_namelist:
	    free(gcd[gc+1].gd.u.list);
	  break;
	  case pr_string: case pr_file: case pr_int: case pr_real: case pr_unicode: case pr_angle:
	    free(plabels[k][gc+1].text);
	    if ( pl->type==pr_file || pl->type==pr_angle )
		++gc;
	  break;
	}
	gc += 2;
    }

    for ( k=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	free(aspects[k].gcd->gd.u.boxelements[0]);
	free(aspects[k].gcd->gd.u.boxelements);
	free(plabels[k]);
    }

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !p.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

void RecentFilesRemember(char *filename) {
    int i,j;

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i )
	if ( strcmp(RecentFiles[i],filename)==0 )
    break;

    if ( i<RECENT_MAX && RecentFiles[i]!=NULL ) {
	if ( i!=0 ) {
	    filename = RecentFiles[i];
	    for ( j=i; j>0; --j )
		RecentFiles[j] = RecentFiles[j-1];
	    RecentFiles[0] = filename;
	}
    } else {
	if ( RecentFiles[RECENT_MAX-1]!=NULL )
	    free( RecentFiles[RECENT_MAX-1]);
	for ( i=RECENT_MAX-1; i>0; --i )
	    RecentFiles[i] = RecentFiles[i-1];
	RecentFiles[0] = copy(filename);
    }

    PrefsUI_SavePrefs(true);
}

void LastFonts_Save(void) {
    FontView *fv, *next;
    char buffer[1024];
    char *ffdir = getFontForgeUserDir(Config);
    FILE *preserve = NULL;

    if ( ffdir ) {
        sprintf(buffer, "%s/FontsOpenAtLastQuit", ffdir);
        preserve = fopen(buffer,"w");
        free(ffdir);
    }

    for ( fv = fv_list; fv!=NULL; fv = next ) {
        next = (FontView *) (fv->b.next);
        if ( preserve ) {
            SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:fv->b.sf;
            fprintf(preserve, "%s\n", sf->filename?sf->filename:sf->origname);
        }
    }

    if ( preserve )
        fclose(preserve);
}

struct prefs_interface gdraw_prefs_interface = {
    PrefsUI_SavePrefs,
    PrefsUI_LoadPrefs,
    PrefsUI_GetPrefs,
    PrefsUI_SetPrefs,
    PrefsUI_getFontForgeShareDir,
    PrefsUI_SetDefaults
};

static void change_res_filename(const char *newname) {
    free(xdefs_filename);
    xdefs_filename = copy( newname );
    SavePrefs(true);
}

void DoXRes(void) {
    extern GResInfo fontview_ri;

    MVColInit();
    CVColInit();
    GResEdit(&fontview_ri,xdefs_filename,change_res_filename);
}

struct prefs_list pointer_dialog_list[] = {
    { N_("ArrowMoveSize"), pr_real, &arrowAmount, NULL, NULL, '\0', NULL, 0, N_("The number of em-units by which an arrow key will move a selected point") },
    { N_("ArrowAccelFactor"), pr_real, &arrowAccelFactor, NULL, NULL, '\0', NULL, 0, N_("Holding down the Shift key will speed up arrow key motion by this factor") },
    { N_("InterpolateCPsOnMotion"), pr_bool, &interpCPsOnMotion, NULL, NULL, '\0', NULL, 0, N_("When moving one end point of a spline but not the other\ninterpolate the control points between the two.") },
    PREFS_LIST_EMPTY
};

struct prefs_list ruler_dialog_list[] = {
	{ N_("SnapDistanceMeasureTool"), pr_real, &snapdistancemeasuretool, NULL, NULL, '\0', NULL, 0, N_("When the measure tool is active and when the mouse pointer is within this many pixels\nof one of the various interesting features (baseline,\nwidth, grid splines, etc.) the pointer will snap\nto that feature.") },
	{ N_("MeasureToolShowHorizontalVertical"), pr_bool, &measuretoolshowhorizontolvertical, NULL, NULL, '\0', NULL, 0, N_("Have the measure tool show horizontal and vertical distances on the canvas.") },
    PREFS_LIST_EMPTY
};

static int PrefsSubSet_Ok(GGadget *g, GEvent *e) {
    GWindow gw = GGadgetGetWindow(g);
    struct pref_data *p = GDrawGetUserData(GGadgetGetWindow(g));
    struct prefs_list* plist = p->plist;
    struct prefs_list* pl = plist;
    int i=0,j=0;
    int err=0, enc;
    const unichar_t *ret;

    p->done = true;

    for ( i=0, pl=plist; pl->name; ++i, ++pl ) {
	switch( pl->type ) {
	case pr_int:
	    *((int *) (pl->val)) = GetInt8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	    break;
	case pr_unicode:
	    *((int *) (pl->val)) = GetUnicodeChar8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	    break;
	case pr_bool:
	    *((int *) (pl->val)) = GGadgetIsChecked(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
	    break;
	case pr_real:
	    *((float *) (pl->val)) = GetReal8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err);
	    break;
	case pr_encoding:
	{ Encoding *e;
		e = ParseEncodingNameFromList(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
		if ( e!=NULL )
		    *((Encoding **) (pl->val)) = e;
		enc = 1;	/* So gcc doesn't complain about unused. It is unused, but why add the ifdef and make the code even messier? Sigh. icc complains anyway */
	}
	break;
	case pr_namelist:
	{ NameList *nl;
	    GTextInfo *ti = GGadgetGetListItemSelected(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
	    if ( ti!=NULL ) {
		char *name = u2utf8_copy(ti->text);
		nl = NameListByName(name);
		free(name);
		if ( nl!=NULL && nl->uses_unicode && !allow_utf8_glyphnames)
		    ff_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set, but there are names in this namelist which use characters outside that range."));
		else if ( nl!=NULL )
		    *((NameList **) (pl->val)) = nl;
	    }
	}
	break;
	case pr_string: case pr_file:
	    ret = _GGadgetGetTitle(GWidgetGetControl(gw,j*CID_PrefsOffset+CID_PrefsBase+i));
	    if ( pl->val!=NULL ) {
		free( *((char **) (pl->val)) );
		*((char **) (pl->val)) = NULL;
		if ( ret!=NULL && *ret!='\0' )
		    *((char **) (pl->val)) = /* u2def_*/ cu_copy(ret);
	    } else {
		char *cret = cu_copy(ret);
		(pl->set)(cret);
		free(cret);
	    }
	    break;
	case pr_angle:
	    *((float *) (pl->val)) = GetReal8(gw,j*CID_PrefsOffset+CID_PrefsBase+i,pl->name,&err)/RAD2DEG;
	    break;
	}
    }

    return( true );
}

static void PrefsSubSetDlg(CharView *cv,char* windowTitle,struct prefs_list* plist) {
    struct prefs_list* pl = plist;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData *pgcd, gcd[20], sgcd[45], mgcd[3], mfgcd[9], msgcd[9];
    GGadgetCreateData mfboxes[3], *mfarray[14];
    GGadgetCreateData mpboxes[3];
    GGadgetCreateData sboxes[2];
    GGadgetCreateData mboxes[3], mboxes2[5], *varray[5], *harray[8];
    GTextInfo *plabel, label[20], slabel[45], mflabels[9], mslabels[9];
    GTabInfo aspects[TOPICS+5], subaspects[3];
    GGadgetCreateData **hvarray, boxes[2*TOPICS];
    struct pref_data p;
    int line = 0,line_max = 3;
    int i = 0, gc = 0, ii, y=0, si=0, k=0;
    char buf[20];
    char *tempstr;

    PrefsInit();
    MfArgsInit();

    line_max=0;
    for ( i=0, pl=plist; pl->name; ++i, ++pl ) {
	    ++line_max;
    }

    int itemCount = 100;
    pgcd = calloc(itemCount,sizeof(GGadgetCreateData));
    plabel = calloc(itemCount,sizeof(GTextInfo));
    hvarray = calloc((itemCount)*5,sizeof(GGadgetCreateData *));
    memset(&p,'\0',sizeof(p));
    memset(&wattrs,0,sizeof(wattrs));
    memset(sgcd,0,sizeof(sgcd));
    memset(slabel,0,sizeof(slabel));
    memset(&mfgcd,0,sizeof(mfgcd));
    memset(&msgcd,0,sizeof(msgcd));
    memset(&mflabels,0,sizeof(mflabels));
    memset(&mslabels,0,sizeof(mslabels));
    memset(&mfboxes,0,sizeof(mfboxes));
    memset(&mpboxes,0,sizeof(mpboxes));
    memset(&sboxes,0,sizeof(sboxes));
    memset(&boxes,0,sizeof(boxes));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&subaspects,'\0',sizeof(subaspects));
    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&aspects,'\0',sizeof(aspects));
    GCDFillMacFeat(mfgcd,mflabels,250,default_mac_feature_map, true, mfboxes, mfarray);

    p.plist = plist;
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = windowTitle;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,340));
    pos.height = GDrawPointsToPixels(NULL,line_max*26+25);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);



    for ( i=0, pl=plist; pl->name; ++i, ++pl ) {

	    plabel[gc].text = (unichar_t *) _(pl->name);
	    plabel[gc].text_is_1byte = true;
	    pgcd[gc].gd.label = &plabel[gc];
	    pgcd[gc].gd.mnemonic = '\0';
	    pgcd[gc].gd.popup_msg = (unichar_t *) 0;//_(pl->popup);
	    pgcd[gc].gd.pos.x = 8;
	    pgcd[gc].gd.pos.y = y + 6;
	    pgcd[gc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    pgcd[gc++].creator = GLabelCreate;
	    hvarray[si++] = &pgcd[gc-1];

	    plabel[gc].text_is_1byte = true;
	    pgcd[gc].gd.label = &plabel[gc];
	    pgcd[gc].gd.mnemonic = '\0';
	    pgcd[gc].gd.popup_msg = (unichar_t *) 0;//_(pl->popup);
	    pgcd[gc].gd.pos.x = 110;
	    pgcd[gc].gd.pos.y = y;
	    pgcd[gc].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    pgcd[gc].data = pl;
	    pgcd[gc].gd.cid = k*CID_PrefsOffset+CID_PrefsBase+i;
	    switch ( pl->type ) {
	      case pr_bool:
		plabel[gc].text = (unichar_t *) _("On");
		pgcd[gc].gd.pos.y += 3;
		pgcd[gc++].creator = GRadioCreate;
		hvarray[si++] = &pgcd[gc-1];
		pgcd[gc] = pgcd[gc-1];
		pgcd[gc].gd.pos.x += 50;
		pgcd[gc].gd.cid = 0;
		pgcd[gc].gd.label = &plabel[gc];
		plabel[gc].text = (unichar_t *) _("Off");
		plabel[gc].text_is_1byte = true;
		hvarray[si++] = &pgcd[gc];
		hvarray[si++] = GCD_Glue;
		if ( *((int *) pl->val))
		    pgcd[gc-1].gd.flags |= gg_cb_on;
		else
		    pgcd[gc].gd.flags |= gg_cb_on;
		++gc;
		y += 22;
	      break;
	      case pr_int:
		sprintf(buf,"%d", *((int *) pl->val));
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	      case pr_unicode:
		/*sprintf(buf,"U+%04x", *((int *) pl->val));*/
		{ char *pt; pt=buf; pt=utf8_idpb(pt,*((int *)pl->val),UTF8IDPB_NOZERO); *pt='\0'; }
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	      case pr_real:
		sprintf(buf,"%g", *((float *) pl->val));
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue; hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	      case pr_encoding:
		pgcd[gc].gd.u.list = GetEncodingTypes();
		pgcd[gc].gd.label = EncodingTypesFindEnc(pgcd[gc].gd.u.list,
			*(Encoding **) pl->val);
		for ( ii=0; pgcd[gc].gd.u.list[ii].text!=NULL ||pgcd[gc].gd.u.list[ii].line; ++ii )
		    if ( pgcd[gc].gd.u.list[ii].userdata!=NULL &&
			    (strcmp(pgcd[gc].gd.u.list[ii].userdata,"Compacted")==0 ||
			     strcmp(pgcd[gc].gd.u.list[ii].userdata,"Original")==0 ))
			pgcd[gc].gd.u.list[ii].disabled = true;
		pgcd[gc].creator = GListFieldCreate;
		pgcd[gc].gd.pos.width = 160;
		if ( pgcd[gc].gd.label==NULL ) pgcd[gc].gd.label = &encodingtypes[0];
		++gc;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_ColSpan; hvarray[si++] = GCD_ColSpan;
		y += 28;
	      break;
	      case pr_namelist:
	        { char **nlnames = AllNamelistNames();
		int cnt;
		GTextInfo *namelistnames;
		for ( cnt=0; nlnames[cnt]!=NULL; ++cnt);
		namelistnames = calloc(cnt+1,sizeof(GTextInfo));
		for ( cnt=0; nlnames[cnt]!=NULL; ++cnt) {
		    namelistnames[cnt].text = (unichar_t *) nlnames[cnt];
		    namelistnames[cnt].text_is_1byte = true;
		    if ( strcmp(_((*(NameList **) (pl->val))->title),nlnames[cnt])==0 ) {
			namelistnames[cnt].selected = true;
			pgcd[gc].gd.label = &namelistnames[cnt];
		    }
		}
		pgcd[gc].gd.u.list = namelistnames;
		pgcd[gc].creator = GListButtonCreate;
		pgcd[gc].gd.pos.width = 160;
		++gc;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_ColSpan; hvarray[si++] = GCD_ColSpan;
		y += 28;
	      } break;
	      case pr_string: case pr_file:
		if ( pl->set==SetAutoTraceArgs || ((char **) pl->val)==&mf_args )
		    pgcd[gc].gd.pos.width = 160;
		if ( pl->val!=NULL )
		    tempstr = *((char **) (pl->val));
		else
		    tempstr = (char *) ((pl->get)());
		if ( tempstr!=NULL )
		    plabel[gc].text = /* def2u_*/ uc_copy( tempstr );
		else if ( ((char **) pl->val)==&BDFFoundry )
		    plabel[gc].text = /* def2u_*/ uc_copy( "FontForge" );
		else
		    plabel[gc].text = /* def2u_*/ uc_copy( "" );
		plabel[gc].text_is_1byte = false;
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		if ( pl->type==pr_file ) {
		    pgcd[gc] = pgcd[gc-1];
		    pgcd[gc-1].gd.pos.width = 140;
		    hvarray[si++] = GCD_ColSpan;
		    pgcd[gc].gd.pos.x += 145;
		    pgcd[gc].gd.cid += CID_PrefsBrowseOffset;
		    pgcd[gc].gd.label = &plabel[gc];
		    plabel[gc].text = (unichar_t *) "...";
		    plabel[gc].text_is_1byte = true;
		    pgcd[gc].gd.handle_controlevent = Prefs_BrowseFile;
		    pgcd[gc++].creator = GButtonCreate;
		    hvarray[si++] = &pgcd[gc-1];
		} else if ( pl->set==SetAutoTraceArgs || ((char **) pl->val)==&mf_args ) {
		    hvarray[si++] = GCD_ColSpan;
		    hvarray[si++] = GCD_Glue;
		} else {
		    hvarray[si++] = GCD_Glue;
		    hvarray[si++] = GCD_Glue;
		}
		y += 26;
		if ( pl->val==NULL )
		    free(tempstr);
	      break;
	      case pr_angle:
		sprintf(buf,"%g", *((float *) pl->val) * RAD2DEG);
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		hvarray[si++] = &pgcd[gc-1];
		plabel[gc].text = (unichar_t *) U_("°");
		plabel[gc].text_is_1byte = true;
		pgcd[gc].gd.label = &plabel[gc];
		pgcd[gc].gd.pos.x = pgcd[gc-1].gd.pos.x+gcd[gc-1].gd.pos.width+2; pgcd[gc].gd.pos.y = pgcd[gc-1].gd.pos.y;
		pgcd[gc].gd.flags = gg_enabled|gg_visible;
		pgcd[gc++].creator = GLabelCreate;
		hvarray[si++] = &pgcd[gc-1];
		hvarray[si++] = GCD_Glue;
		y += 26;
	      break;
	    }
	    ++line;
	    hvarray[si++] = NULL;

    }

    harray[4] = 0;
    harray[5] = 0;
    harray[6] = 0;
    harray[7] = 0;

    gcd[gc].gd.pos.x = 30-3;
    gcd[gc].gd.pos.y = y+5-3;
    gcd[gc].gd.pos.width = -1;
    gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[gc].text = (unichar_t *) _("_OK");
    label[gc].text_is_1byte = true;
    label[gc].text_in_resource = true;
    gcd[gc].gd.mnemonic = 'O';
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.handle_controlevent = PrefsSubSet_Ok;
    gcd[gc++].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &gcd[gc-1]; harray[2] = GCD_Glue; harray[3] = GCD_Glue;


    memset(mboxes,0,sizeof(mboxes));
    memset(mboxes2,0,sizeof(mboxes2));

    mboxes[2].gd.pos.x = 2;
    mboxes[2].gd.pos.y = 20;
    mboxes[2].gd.flags = gg_enabled|gg_visible;
    mboxes[2].gd.u.boxelements = harray;
    mboxes[2].creator = GHBoxCreate;

    mboxes[0].gd.pos.x = mboxes[0].gd.pos.y = 2;
    mboxes[0].gd.flags = gg_enabled|gg_visible;
    mboxes[0].gd.u.boxelements = hvarray;
    mboxes[0].creator = GHVGroupCreate;

    varray[0] = &mboxes[0];
    varray[1] = &mboxes[2];
    varray[2] = 0;
    varray[3] = 0;
    varray[4] = 0;

    /* varray[0] = &mboxes[2]; */
    /* varray[1] = 0;//&mboxes[2]; */
    /* varray[2] = 0; */
    /* varray[3] = 0; */
    /* varray[4] = 0; */

    mboxes2[0].gd.pos.x = 4;
    mboxes2[0].gd.pos.y = 4;
    mboxes2[0].gd.flags = gg_enabled|gg_visible;
    mboxes2[0].gd.u.boxelements = varray;
    mboxes2[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,mboxes2);


    GDrawSetVisible(gw,true);
    while ( !p.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}


void PointerDlg(CharView *cv) {
    PrefsSubSetDlg( cv, _("Arrow Options"), pointer_dialog_list );
}

void RulerDlg(CharView *cv) {
    PrefsSubSetDlg( cv, _("Ruler Options"), ruler_dialog_list );
}

