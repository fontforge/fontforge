/* Copyright (C) 2000-2008 by George Williams */
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
#include "fontforgegtk.h"
#include <fontforge/groups.h>
#include <fontforge/plugins.h>
#include <charset.h>
#include <gfile.h>
#include <ustring.h>
#include <gdk/gdkkeysyms.h>

#include <sys/types.h>
#include <dirent.h>
#include <locale.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#include <fontforge/ttf.h>

#if HAVE_LANGINFO_H
# include <langinfo.h>
#endif

extern int splash;
extern int adjustwidth;
extern int adjustlbearing;
extern Encoding *default_encoding;
extern int autohint_before_generate;
extern int use_freetype_to_rasterize_fv;
extern int OpenCharsInNewWindow;
extern int ItalicConstrained;
extern int accent_offset;
extern int GraveAcuteCenterBottom;
extern int PreferSpacingAccents;
extern int CharCenterHighest;
extern int ask_user_for_resolution;
extern int stop_at_join;
extern int cv_auto_goto;
extern int recognizePUA;
extern float arrowAmount;
extern float arrowAccelFactor;
extern float snapdistance;
extern int snaptoint;
extern float joinsnap;
extern char *BDFFoundry;
extern char *TTFFoundry;
extern char *xuid;
extern char *SaveTablesPref;
extern char *RecentFiles[RECENT_MAX];
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
extern int prefer_cjk_encodings;	/* in parsettf */
extern int onlycopydisplayed, copymetadata, copyttfinstr;
extern struct cvshows CVShows;
extern int infowindowdistance;		/* in cvruler.c */
extern int oldformatstate;		/* in savefontdlg.c */
extern int oldbitmapstate;		/* in savefontdlg.c */
extern int oldbitmapstate;		/* in savefontdlg.c */
static int old_ttf_flags=0, old_otf_flags=0;
extern int old_sfnt_flags;
extern int old_ps_flags;		/* in savefontdlg.c */
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
extern float gridfit_pointsize;		/* in cvgridfit.c */
extern int hint_diagonal_ends;		/* in stemdb.c */
extern int hint_diagonal_intersections;	/* in stemdb.c */
extern int hint_bounding_boxes;		/* in stemdb.c */
extern int detect_diagonal_stems;	/* in stemdb.c */
extern char *script_menu_names[SCRIPT_MENU_MAX];
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
extern int allow_utf8_glyphnames;		/* in charinfo.c */
extern int clear_tt_instructions_when_needed;	/* in charview.c */
extern int ask_user_for_cmap;			/* in parsettf.c */

extern int rectelipse, polystar, regular_star;	/* from cvpalettes.c */
extern int center_out[2];			/* from cvpalettes.c */
extern float rr_radius;				/* from cvpalettes.c */
extern int ps_pointcnt;				/* from cvpalettes.c */
extern float star_percent;			/* from cvpalettes.c */

extern NameList *force_names_when_opening;
extern NameList *force_names_when_saving;
extern NameList *namelist_for_new_fonts;

extern int default_font_filter_index;
extern struct gwwv_filter *user_font_filters;
static int alwaysgenapple=false, alwaysgenopentype=false;

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
	pr_file, pr_namelist };
struct enums { char *name; int value; };

struct enums fvsize_enums[] = { {NULL} };

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
	{ N_("ResourceFile"), pr_file, &xdefs_filename, NULL, NULL, 'R', NULL, 0, N_("When FontForge starts up, it loads the user interface theme from\nthis file. Any changes will only take effect the next time you start FontForge.") },
	{ N_("OtherSubrsFile"), pr_file, &othersubrsfile, NULL, NULL, 'O', NULL, 0, N_("If you wish to replace Adobe's OtherSubrs array (for Type1 fonts)\nwith an array of your own, set this to point to a file containing\na list of up to 14 PostScript subroutines. Each subroutine must\nbe preceded by a line starting with '%%%%' (any text before the\nfirst '%%%%' line will be treated as an initial copyright notice).\nThe first three subroutines are for flex hints, the next for hint\nsubstitution (this MUST be present), the 14th (or 13 as the\nnumbering actually starts with 0) is for counter hints.\nThe subroutines should not be enclosed in a [ ] pair.") },
	{ N_("FreeTypeInFontView"), pr_bool, &use_freetype_to_rasterize_fv, NULL, NULL, 'O', NULL, 0, N_("Use the FreeType rasterizer (when available)\nto rasterize glyphs in the font view.\nThis generally results in better quality.") },
	{ N_("SplashScreen"), pr_bool, &splash, NULL, NULL, 'S', NULL, 0, N_("Show splash screen on start-up") },
	{ NULL }
},
  new_list[] = {
	{ N_("NewCharset"), pr_encoding, &default_encoding, NULL, NULL, 'N', NULL, 0, N_("Default encoding for\nnew fonts") },
	{ N_("NewEmSize"), pr_int, &new_em_size, NULL, NULL, 'S', NULL, 0, N_("The default size of the Em-Square in a newly created font.") },
	{ N_("NewFontsQuadratic"), pr_bool, &new_fonts_are_order2, NULL, NULL, 'Q', NULL, 0, N_("Whether new fonts should contain splines of quadratic (truetype)\nor cubic (postscript & opentype).") },
	{ N_("LoadedFontsAsNew"), pr_bool, &loaded_fonts_same_as_new, NULL, NULL, 'L', NULL, 0, N_("Whether fonts loaded from the disk should retain their splines\nwith the original order (quadratic or cubic), or whether the\nsplines should be converted to the default order for new fonts\n(see NewFontsQuadratic).") },
	{ NULL }
},
  open_list[] = {
	{ N_("PreferCJKEncodings"), pr_bool, &prefer_cjk_encodings, NULL, NULL, 'C', NULL, 0, N_("When loading a truetype or opentype font which has both a unicode\nand a CJK encoding table, use this flag to specify which\nshould be loaded for the font.") },
	{ N_("AskUserForCMap"), pr_bool, &ask_user_for_cmap, NULL, NULL, 'O', NULL, 0, N_("When loading a font in sfnt format (TrueType, OpenType, etc.),\nask the user to specify which cmap to use initially.") },
	{ N_("PreserveTables"), pr_string, &SaveTablesPref, NULL, NULL, 'P', NULL, 0, N_("Enter a list of 4 letter table tags, separated by commas.\nFontForge will make a binary copy of these tables when it\nloads a True/OpenType font, and will output them (unchanged)\nwhen it generates the font. Do not include table tags which\nFontForge thinks it understands.") },
	{ NULL }
},
  navigation_list[] = {
	{ N_("GlyphAutoGoto"), pr_bool, &cv_auto_goto, NULL, NULL, '\0', NULL, 0, N_("Typing a normal character in the glyph view window changes the window to look at that character") },
	{ N_("OpenCharsInNewWindow"), pr_bool, &OpenCharsInNewWindow, NULL, NULL, '\0', NULL, 0, N_("When double clicking on a character in the font view\nopen that character in a new window, otherwise\nreuse an existing one.") },
	{ NULL }
},
  editing_list[] = {
	{ N_("ItalicConstrained"), pr_bool, &ItalicConstrained, NULL, NULL, '\0', NULL, 0, N_("In the Outline View, the Shift key constrains motion to be parallel to the ItalicAngle rather than constraining it to be vertical.") },
	{ N_("ArrowMoveSize"), pr_real, &arrowAmount, NULL, NULL, '\0', NULL, 0, N_("The number of em-units by which an arrow key will move a selected point") },
	{ N_("ArrowAccelFactor"), pr_real, &arrowAccelFactor, NULL, NULL, '\0', NULL, 0, N_("Holding down the Shift key will speed up arrow key motion by this factor") },
	{ N_("SnapDistance"), pr_real, &snapdistance, NULL, NULL, '\0', NULL, 0, N_("When the mouse pointer is within this many pixels\nof one of the various interesting features (baseline,\nwidth, grid splines, etc.) the pointer will snap\nto that feature.") },
	{ N_("SnapToInt"), pr_bool, &snaptoint, NULL, NULL, '\0', NULL, 0, N_("When the user clicks in the editing window, round the location to the nearest integers.") },
	{ N_("JoinSnap"), pr_real, &joinsnap, NULL, NULL, '\0', NULL, 0, N_("The Edit->Join command will join points which are this close together\nA value of 0 means they must be coincident") },
	{ N_("StopAtJoin"), pr_bool, &stop_at_join, NULL, NULL, '\0', NULL, 0, N_("When dragging points in the outline view a join may occur\n(two open contours may connect at their endpoints). When\nthis is On a join will cause FontForge to stop moving the\nselection (as if the user had released the mouse button).\nThis is handy if your fingers are inclined to wiggle a bit.") },
	{ N_("CopyMetaData"), pr_bool, &copymetadata, NULL, NULL, '\0', NULL, 0, N_("When copying glyphs from the font view, also copy the\nglyphs' metadata (name, encoding, comment, etc).") },
	{ N_("UndoDepth"), pr_int, &maxundoes, NULL, NULL, '\0', NULL, 0, N_("The maximum number of Undoes/Redoes stored in a glyph. Use -1 for infinite Undoes\n(but watch RAM consumption and use the Edit menu's Remove Undoes as needed)") },
	{ N_("UpdateFlex"), pr_bool, &updateflex, NULL, NULL, '\0', NULL, 0, N_("Figure out flex hints after every change") },
	{ NULL }
},
  sync_list[] = {
	{ N_("AutoWidthSync"), pr_bool, &adjustwidth, NULL, NULL, '\0', NULL, 0, N_("Changing the width of a glyph\nchanges the widths of all accented\nglyphs based on it.") },
	{ N_("AutoLBearingSync"), pr_bool, &adjustlbearing, NULL, NULL, '\0', NULL, 0, N_("Changing the left side bearing\nof a glyph adjusts the lbearing\nof other references in all accented\nglyphs based on it.") },
	{ NULL }
},
 tt_list[] = {
	{ N_("ClearInstrsBigChanges"), pr_bool, &clear_tt_instructions_when_needed, NULL, NULL, 'C', NULL, 0, N_("Instructions in a TrueType font refer to\npoints by number, so if you edit a glyph\nin such a way that some points have different\nnumbers (add points, remove them, etc.) then\nthe instructions will be applied to the wrong\npoints with disasterous results.\n  Normally FontForge will remove the instructions\nif it detects that the points have been renumbered\nin order to avoid the above problem. You may turn\nthis behavior off -- but be careful!") },
	{ N_("CopyTTFInstrs"), pr_bool, &copyttfinstr, NULL, NULL, '\0', NULL, 0, N_("When copying glyphs from the font view, also copy the\nglyphs' metadata (name, encoding, comment, etc).") },
	{ NULL }
},
  accent_list[] = {
	{ N_("AccentOffsetPercent"), pr_int, &accent_offset, NULL, NULL, '\0', NULL, 0, N_("The percentage of an em by which an accent is offset from its base glyph in Build Accent") },
	{ N_("AccentCenterLowest"), pr_bool, &GraveAcuteCenterBottom, NULL, NULL, '\0', NULL, 0, N_("When placing grave and acute accents above letters, should\nFontForge center them based on their full width, or\nshould it just center based on the lowest point\nof the accent.") },
	{ N_("CharCenterHighest"), pr_bool, &CharCenterHighest, NULL, NULL, '\0', NULL, 0, N_("When centering an accent over a glyph, should the accent\nbe centered on the highest point(s) of the glyph,\nor the middle of the glyph?") },
	{ N_("PreferSpacingAccents"), pr_bool, &PreferSpacingAccents, NULL, NULL, '\0', NULL, 0, N_("Use spacing accents (Unicode: 02C0-02FF) rather than\ncombining accents (Unicode: 0300-036F) when\nbuilding accented glyphs.") },
	{ NULL }
},
 args_list[] = {
	{ N_("PreferPotrace"), pr_bool, &preferpotrace, NULL, NULL, '\0', NULL, 0, N_("FontForge supports two different helper applications to do autotracing\n autotrace and potrace\nIf your system only has one it will use that one, if you have both\nuse this option to tell FontForge which to pick.") },
	{ N_("AutotraceArgs"), pr_string, NULL, GetAutoTraceArgs, SetAutoTraceArgs, '\0', NULL, 0, N_("Extra arguments for configuring the autotrace program\n(either autotrace or potrace)") },
	{ N_("AutotraceAsk"), pr_bool, &autotrace_ask, NULL, NULL, '\0', NULL, 0, N_("Ask the user for autotrace arguments each time autotrace is invoked") },
	{ N_("MfArgs"), pr_string, &mf_args, NULL, NULL, '\0', NULL, 0, N_("Commands to pass to mf (metafont) program, the filename will follow these") },
	{ N_("MfAsk"), pr_bool, &mf_ask, NULL, NULL, '\0', NULL, 0, N_("Ask the user for mf commands each time mf is invoked") },
	{ N_("MfClearBg"), pr_bool, &mf_clearbackgrounds, NULL, NULL, '\0', NULL, 0, N_("FontForge loads large images into the background of each glyph\nprior to autotracing them. You may retain those\nimages to look at after mf processing is complete, or\nremove them to save space") },
	{ N_("MfShowErr"), pr_bool, &mf_showerrors, NULL, NULL, '\0', NULL, 0, N_("MetaFont (mf) generates lots of verbiage to stdout.\nMost of the time I find it an annoyance but it is\nimportant to see if something goes wrong.") },
	{ NULL }
},
 fontinfo_list[] = {
	{ N_("FoundryName"), pr_string, &BDFFoundry, NULL, NULL, 'F', NULL, 0, N_("Name used for foundry field in bdf\nfont generation") },
	{ N_("TTFFoundry"), pr_string, &TTFFoundry, NULL, NULL, 'T', NULL, 0, N_("Name used for Vendor ID field in\nttf (OS/2 table) font generation.\nMust be no more than 4 characters") },
	{ N_("NewFontNameList"), pr_namelist, &namelist_for_new_fonts, NULL, NULL, '\0', NULL, 0, N_("FontForge will use this namelist when assigning\nglyph names to code points in a new font.") },
	{ N_("RecognizePUANames"), pr_bool, &recognizePUA, NULL, NULL, 'U', NULL, 0, N_("Once upon a time, Adobe assigned PUA (public use area) encodings\nfor many stylistic variants of characters (small caps, old style\nnumerals, etc.). Adobe no longer believes this to be a good idea,\nand recommends that these encodings be ignored.\n\n The assignments were originally made because most applications\ncould not handle OpenType features for accessing variants. Adobe\nnow believes that all apps that matter can now do so. Applications\nlike Word and OpenOffice still can't handle these features, so\n fontforge's default behavior is to ignore Adobe's current\nrecommendations.\n\nNote: This does not affect figuring out unicode from the font's encoding,\nit just controls determining unicode from a name.") },
	{ N_("UnicodeGlyphNames"), pr_bool, &allow_utf8_glyphnames, NULL, NULL, 'O', NULL, 0, N_("Allow the full unicode character set in glyph names.\nThis does not conform to adobe's glyph name standard.\nSuch names should be for internal use only and\nshould NOT end up in production fonts." ) },
	{ N_("XUID-Base"), pr_string, &xuid, NULL, NULL, 'X', NULL, 0, N_("If specified this should be a space separated list of integers each\nless than 16777216 which uniquely identify your organization\nFontForge will generate a random number for the final component.") },
	{ NULL }
},
 generate_list[] = {
	{ N_("AskBDFResolution"), pr_bool, &ask_user_for_resolution, NULL, NULL, 'B', NULL, 0, N_("When generating a set of BDF fonts ask the user\nto specify the screen resolution of the fonts\notherwise FontForge will guess depending on the pixel size.") },
	{ N_("AutoHint"), pr_bool, &autohint_before_generate, NULL, NULL, 'H', NULL, 0, N_("AutoHint changed glyphs before generating a font") },
	{ NULL }
},
 hints_list[] = {
	{ N_("HintBoundingBoxes"), pr_bool, &hint_bounding_boxes, NULL, NULL, '\0', NULL, 0, N_("FontForge will place vertical or horizontal hints to describe the bounding boxes of suitable glyphs.") },
	{ N_("HintDiagonalEnds"), pr_bool, &hint_diagonal_ends, NULL, NULL, '\0', NULL, 0, N_("FontForge will place vertical or horizontal hints at the ends of diagonal stems.") },
	{ N_("HintDiagonalInter"), pr_bool, &hint_diagonal_intersections, NULL, NULL, '\0', NULL, 0, N_("FontForge will place vertical or horizontal hints at the intersections of diagonal stems.") },
	{ N_("DetectDiagonalStems"), pr_bool, &detect_diagonal_stems, NULL, NULL, '\0', NULL, 0, N_("FontForge will generate diagonal stem hints, which then can be used by the AutoInstr command.") },
	{ NULL }
},
 opentype_list[] = {
	{ N_("UseNewIndicScripts"), pr_bool, &use_second_indic_scripts, NULL, NULL, 'C', NULL, 0, N_("MS has changed (in August 2006) the inner workings of their Indic shaping\nengine, and to disambiguate this change has created a parallel set of script\ntags (generally ending in '2') for Indic writing systems. If you are working\nwith the new system set this flag, if you are working with the old unset it.\n(if you aren't doing Indic work, this flag is irrelevant).") },
	{ NULL }
},
/* These are hidden, so will never appear in preference ui, hence, no "N_(" */
/*  They are controled elsewhere AntiAlias is a menu item in the font window's View menu */
/*  etc. */
 hidden_list[] = {
	{ "AntiAlias", pr_bool, &default_fv_antialias, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVShowHmetrics", pr_int, &default_fv_showhmetrics, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVShowVmetrics", pr_int, &default_fv_showvmetrics, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVSize", pr_int, &default_fv_font_size, NULL, NULL, 'S', NULL, 1 },
	{ "DefaultFVRowCount", pr_int, &default_fv_row_count, NULL, NULL, 'S', NULL, 1 },
	{ "DefaultFVColCount", pr_int, &default_fv_col_count, NULL, NULL, 'S', NULL, 1 },
	{ "DefaultFVGlyphLabel", pr_int, &default_fv_glyphlabel, NULL, NULL, 'S', NULL, 1 },
	{ "SaveToDir", pr_int, &save_to_dir, NULL, NULL, 'S', NULL, 1 },
	{ "OnlyCopyDisplayed", pr_bool, &onlycopydisplayed, NULL, NULL, '\0', NULL, 1 },
	{ "PalettesDocked", pr_bool, &palettes_docked, NULL, NULL, '\0', NULL, 1 },
	{ "CVVisible0", pr_bool, &cvvisible[0], NULL, NULL, '\0', NULL, 1 },
	{ "CVVisible1", pr_bool, &cvvisible[1], NULL, NULL, '\0', NULL, 1 },
	{ "BVVisible0", pr_bool, &bvvisible[0], NULL, NULL, '\0', NULL, 1 },
	{ "BVVisible1", pr_bool, &bvvisible[1], NULL, NULL, '\0', NULL, 1 },
	{ "BVVisible2", pr_bool, &bvvisible[2], NULL, NULL, '\0', NULL, 1 },
	{ "MarkExtrema", pr_int, &CVShows.markextrema, NULL, NULL, '\0', NULL, 1 },
	{ "MarkPointsOfInflect", pr_int, &CVShows.markpoi, NULL, NULL, '\0', NULL, 1 },
	{ "ShowRulers", pr_bool, &CVShows.showrulers, NULL, NULL, '\0', NULL, 1, N_("Display rulers in the Outline Glyph View") },
	{ "ShowCPInfo", pr_int, &CVShows.showcpinfo, NULL, NULL, '\0', NULL, 1 },
	{ "InfoWindowDistance", pr_int, &infowindowdistance, NULL, NULL, '\0', NULL, 1 },
	{ "ShowSideBearings", pr_int, &CVShows.showsidebearings, NULL, NULL, '\0', NULL, 1 },
	{ "ShowPoints", pr_bool, &CVShows.showpoints, NULL, NULL, '\0', NULL, 1 },
	{ "ShowFilled", pr_int, &CVShows.showfilled, NULL, NULL, '\0', NULL, 1 },
	{ "ShowTabs", pr_int, &CVShows.showtabs, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultScreenDpiSystem", pr_int, &oldsystem, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultOutputFormat", pr_int, &oldformatstate, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultBitmapFormat", pr_int, &oldbitmapstate, NULL, NULL, '\0', NULL, 1 },
	{ "SaveValidate", pr_int, &old_validate, NULL, NULL, '\0', NULL, 1 },
	{ "SaveFontLogAsk", pr_int, &old_fontlog, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultSFNTflags", pr_int, &old_sfnt_flags, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultPSflags", pr_int, &old_ps_flags, NULL, NULL, '\0', NULL, 1 },
	{ "PageWidth", pr_int, &pagewidth, NULL, NULL, '\0', NULL, 1 },
	{ "PageHeight", pr_int, &pageheight, NULL, NULL, '\0', NULL, 1 },
	{ "PrintType", pr_int, &printtype, NULL, NULL, '\0', NULL, 1 },
	{ "PrintCommand", pr_string, &printcommand, NULL, NULL, '\0', NULL, 1 },
	{ "PageLazyPrinter", pr_string, &printlazyprinter, NULL, NULL, '\0', NULL, 1 },
	{ "RegularStar", pr_bool, &regular_star, NULL, NULL, '\0', NULL, 1 },
	{ "PolyStar", pr_bool, &polystar, NULL, NULL, '\0', NULL, 1 },
	{ "RectEllipse", pr_bool, &rectelipse, NULL, NULL, '\0', NULL, 1 },
	{ "RectCenterOut", pr_bool, &center_out[0], NULL, NULL, '\0', NULL, 1 },
	{ "EllipseCenterOut", pr_bool, &center_out[1], NULL, NULL, '\0', NULL, 1 },
	{ "PolyStartPointCnt", pr_int, &ps_pointcnt, NULL, NULL, '\0', NULL, 1 },
	{ "RoundRectRadius", pr_real, &rr_radius, NULL, NULL, '\0', NULL, 1 },
	{ "StarPercent", pr_real, &star_percent, NULL, NULL, '\0', NULL, 1 },
	{ "CoverageFormatsAllowed", pr_int, &coverageformatsallowed, NULL, NULL, '\0', NULL, 1 },
	{ "DebugWins", pr_int, &debug_wins, NULL, NULL, '\0', NULL, 1 },
	{ "GridFitDpi", pr_int, &gridfit_dpi, NULL, NULL, '\0', NULL, 1 },
	{ "GridFitDepth", pr_int, &gridfit_depth, NULL, NULL, '\0', NULL, 1 },
	{ "GridFitPointSize", pr_real, &gridfit_pointsize, NULL, NULL, '\0', NULL, 1 },
	{ "ForceNamesWhenOpening", pr_namelist, &force_names_when_opening, NULL, NULL, '\0', NULL, 1 },
	{ "ForceNamesWhenSaving", pr_namelist, &force_names_when_saving, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFontFilterIndex", pr_int, &default_font_filter_index, NULL, NULL, '\0', NULL, 1 },
	{ NULL }
},
 oldnames[] = {
	{ "DumpGlyphMap", pr_bool, &glyph_2_name_map, NULL, NULL, '\0', NULL, 0, N_("When generating a truetype or opentype font it is occasionally\nuseful to know the mapping between truetype glyph ids and\nglyph names. Setting this option will cause FontForge to\nproduce a file (with extension .g2n) containing those data.") },
	{ "DefaultTTFApple", pr_int, &pointless, NULL, NULL, '\0', NULL, 1 },
	{ "AcuteCenterBottom", pr_bool, &GraveAcuteCenterBottom, NULL, NULL, '\0', NULL, 1, N_("When placing grave and acute accents above letters, should\nFontForge center them based on their full width, or\nshould it just center based on the lowest point\nof the accent.") },
	{ "AlwaysGenApple", pr_bool, &alwaysgenapple, NULL, NULL, 'A', NULL, 0, N_("Apple and MS/Adobe differ about the format of truetype and opentype files.\nThis controls the default setting of the Apple checkbox in the\nFile->Generate Font dialog.\nThe main differences are:\n Bitmap data are stored in different tables\n Scaled composite glyphs are treated differently\n Use of GSUB rather than morx(t)/feat\n Use of GPOS rather than kern/opbd\n Use of GDEF rather than lcar/prop\nIf both this and OpenType are set, both formats are generated") },
	{ "AlwaysGenOpenType", pr_bool, &alwaysgenopentype, NULL, NULL, 'O', NULL, 0, N_("Apple and MS/Adobe differ about the format of truetype and opentype files.\nThis controls the default setting of the OpenType checkbox in the\nFile->Generate Font dialog.\nThe main differences are:\n Bitmap data are stored in different tables\n Scaled composite glyphs are treated differently\n Use of GSUB rather than morx(t)/feat\n Use of GPOS rather than kern/opbd\n Use of GDEF rather than lcar/prop\nIf both this and Apple are set, both formats are generated") },
	{ "DefaultTTFflags", pr_int, &old_ttf_flags, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultOTFflags", pr_int, &old_otf_flags, NULL, NULL, '\0', NULL, 1 },
	{ NULL }
},
 *prefs_list[] = { general_list, new_list, open_list, navigation_list, sync_list, editing_list, accent_list, args_list, fontinfo_list, generate_list, tt_list, opentype_list, hints_list, hidden_list, NULL },
 *load_prefs_list[] = { general_list, new_list, open_list, navigation_list, sync_list, editing_list, accent_list, args_list, fontinfo_list, generate_list, tt_list, opentype_list, hints_list, hidden_list, oldnames, NULL };

struct visible_prefs_list { char *tab_name; int nest; struct prefs_list *pl; } visible_prefs_list[] = {
    { N_("Generic"), 0, general_list},
    { N_("New Font"), 0, new_list},
    { N_("Open Font"), 0, open_list},
    { N_("Navigation"), 0, navigation_list},
    { N_("Editing"), 0, editing_list},
    { N_("Synchronize"), 1, sync_list},
    { N_("TT"), 1, tt_list},
    { N_("Accents"), 1, accent_list},
    { N_("Apps"), 1, args_list},
    { N_("Font Info"), 0, fontinfo_list},
    { N_("Generate"), 0, generate_list},
    { N_("PS Hints"), 1, hints_list},
    { N_("OpenType"), 1, opentype_list},
    { 0 }
 };

#define TOPICS	(sizeof(visible_prefs_list)/sizeof(visible_prefs_list[0])-1)

static int PrefsUI_GetPrefs(char *name,Val *val) {
    int i,j;

    /* Support for obsolete preferences */
    alwaysgenapple=(old_sfnt_flags&ttf_flag_applemode)?1:0;
    alwaysgenopentype=(old_sfnt_flags&ttf_flag_otmode)?1:0;
    
    for ( i=0; prefs_list[i]!=NULL; ++i ) for ( j=0; prefs_list[i][j].name!=NULL; ++j ) {
	if ( strcmp(prefs_list[i][j].name,name)==0 ) {
	    struct prefs_list *pf = &prefs_list[i][j];
	    if ( pf->type == pr_bool || pf->type == pr_int ) {
		val->type = v_int;
		val->u.ival = *((int *) (pf->val));
	    } else if ( pf->type == pr_string || pf->type == pr_file ) {
		val->type = v_str;
		val->u.sval = copy( *((char **) (pf->val)));
	    } else if ( pf->type == pr_encoding ) {
		val->type = v_str;
		if ( *((NameList **) (pf->val))==NULL )
		    val->u.sval = copy( "NULL" );
		else
		    val->u.sval = copy( (*((Encoding **) (pf->val)))->enc_name );
	    } else if ( pf->type == pr_namelist ) {
		val->type = v_str;
		val->u.sval = copy( (*((NameList **) (pf->val)))->title );
	    } else if ( pf->type == pr_real ) {
		val->type = v_real;
		val->u.fval = *((float *) (pf->val));
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
	    if ( pf->type == pr_bool || pf->type == pr_int ) {
		if ( (val1->type!=v_int && val1->type!=v_unicode) || val2!=NULL )
return( -1 );
		*((int *) (pf->val)) = val1->u.ival;
	    } else if ( pf->type == pr_real ) {
		if ( val1->type==v_real && val2==NULL )
		    *((float *) (pf->val)) = val1->u.fval;
		else if ( val1->type!=v_int || (val2!=NULL && val2->type!=v_int ))
return( -1 );
		else
		    *((float *) (pf->val)) = (val2==NULL ? val1->u.ival : val1->u.ival / (double) val2->u.ival);
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

    if ( prefs!=NULL )
return( prefs );
    if ( getFontForgeUserDir(Config)==NULL )
return( NULL );
    sprintf(buffer,"%s/prefs", getFontForgeUserDir(Config));
    prefs = copy(buffer);
return( prefs );
}

static char *PrefsUI_getFontForgeShareDir(void) {

#if defined(SHAREDIR)
return( SHAREDIR "/fontforge" );
#elif defined(PREFIX)
return( PREFIX "/share/fontforge" );
#else
return( "share/fontforge" );
#endif
}

#  include <charset.h>		/* we still need the charsets & encoding to set local_encoding */
static int encmatch(const char *enc,int subok) {
    static struct { char *name; int enc; } encs[] = {
	{ "US-ASCII", e_usascii },
	{ "ASCII", e_usascii },
	{ "ISO646-NO", e_iso646_no },
	{ "ISO646-SE", e_iso646_se },
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
	{ NULL }};
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

static void PrefsUI_LoadPrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    char line[1100];
    int i, j, ri=0, mn=0, ms=0, fn=0, ff=0, filt_max=0;
    int msp=0, msc=0;
    char *pt;
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
		    script_menu_names[mn++] = copy(pt);
		else if ( strncmp(line,"FontFilterName:",strlen("FontFilterName:"))==0 ) {
		    if ( fn>=filt_max )
			user_font_filters = realloc(user_font_filters,((filt_max+=10)+1)*sizeof( struct gwwv_filter));
		    user_font_filters[fn].filtfunc = NULL;
		    user_font_filters[fn].wild = NULL;
		    user_font_filters[fn++].name = copy(pt);
		    user_font_filters[fn].name = NULL;
		} else if ( strncmp(line,"FontFilter:",strlen("FontFilter:"))==0 ) {
		    if ( ff<filt_max )
			user_font_filters[ff++].wild = copy(pt);
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
	      case pr_real:
		sscanf( pt, "%f", (float *) pl->val );
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
    if ( othersubrsfile!=NULL && ReadOtherSubrsFile(othersubrsfile)<=0 )
	fprintf( stderr, "Failed to read OtherSubrs from %s\n", othersubrsfile );
	
    if ( glyph_2_name_map ) {
	old_sfnt_flags |= ttf_flag_glyphmap;
    }
    LoadNamelistDir(NULL);
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
	}
    }

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i )
	fprintf( p, "Recent:\t%s\n", RecentFiles[i]);
    for ( i=0; i<SCRIPT_MENU_MAX && script_filenames[i]!=NULL; ++i ) {
	fprintf( p, "MenuScript:\t%s\n", script_filenames[i]);
	fprintf( p, "MenuName:\t%s\n", script_menu_names[i]);
    }
    if ( user_font_filters!=NULL ) {
	for ( i=0; user_font_filters[i].name!=NULL; ++i ) {
	    fprintf( p, "FontFilterName:\t%s\n", user_font_filters[i].name);
	    fprintf( p, "FontFilter:\t%s\n", user_font_filters[i].wild);
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

void DoPrefs(void) {
    gwwv_post_notice("NYI","Preference dlg not yet implemented");
}

void RecentFilesRemember(char *filename) {
    int i;

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i )
	if ( strcmp(RecentFiles[i],filename)==0 )
    break;

    if ( i<RECENT_MAX && RecentFiles[i]!=NULL ) {
	if ( i!=0 ) {
	    filename = RecentFiles[i];
	    RecentFiles[i] = RecentFiles[0];
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

struct prefs_interface gtk_prefs_interface = {
    PrefsUI_SavePrefs,
    PrefsUI_LoadPrefs,
    PrefsUI_GetPrefs,
    PrefsUI_SetPrefs,
    PrefsUI_getFontForgeShareDir,
    PrefsUI_SetDefaults
};
