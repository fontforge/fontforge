/* Copyright (C) 2000-2003 by George Williams */
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
#include "pfaeditui.h"
#include <charset.h>
#include <gfile.h>
#include <gresource.h>
#include <ustring.h>
#include <gkeysym.h>

#include <sys/types.h>
#include <dirent.h>
#include <locale.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#include "ttf.h"

int adjustwidth = true;
int adjustlbearing = true;
int default_encoding = em_iso8859_1;
int autohint_before_rasterize = 1;
int autohint_before_generate = 1;
int ItalicConstrained=true;
int accent_offset = 6;
int GraveAcuteCenterBottom = 1;
int CharCenterHighest = 1;
int ask_user_for_resolution = true;
int stop_at_join = false;
float arrowAmount=1;
float snapdistance=3.5;
float joinsnap=0;
char *BDFFoundry=NULL;
char *TTFFoundry=NULL;
char *xuid=NULL;
char *RecentFiles[RECENT_MAX] = { NULL };
/*struct cvshows CVShows = { 1, 1, 1, 1, 1, 0, 1 };*/ /* in charview */
/* int default_fv_font_size = 24; */	/* in fontview */
/* int default_fv_antialias = false */	/* in fontview */
/* int default_fv_bbsized = false */	/* in fontview */
extern int default_fv_showhmetrics;	/* in fontview */
extern int default_fv_showvmetrics;	/* in fontview */
extern int palettes_docked;		/* in cvpalettes */
extern int maxundoes;			/* in cvundoes */
extern int prefer_cjk_encodings;	/* in parsettf */
/* int local_encoding; */		/* in gresource.c *//* not a charset */
static int prefs_encoding = e_unknown;
int greekfixup = true;
extern int onlycopydisplayed, copymetadata;
extern struct cvshows CVShows;
extern int oldformatstate;		/* in savefontdlg.c */
extern int oldbitmapstate;		/* in savefontdlg.c */
extern int old_ttf_flags;		/* in savefontdlg.c */
extern int old_ps_flags;		/* in savefontdlg.c */
extern int old_otf_flags;		/* in savefontdlg.c */
extern int oldsystem;			/* in bitmapdlg.c */
extern int autotrace_ask;		/* in autotrace.c */
extern int mf_ask;			/* in autotrace.c */
extern int mf_clearbackgrounds;		/* in autotrace.c */
extern int mf_showerrors;		/* in autotrace.c */
extern char *mf_args;			/* in autotrace.c */
extern int glyph_2_name_map;		/* in tottf.c */
extern int coverageformatsallowed;	/* in tottfgpos.c */
unichar_t *script_menu_names[SCRIPT_MENU_MAX];
char *script_filenames[SCRIPT_MENU_MAX];
static char *xdefs_filename;
int new_em_size = 1000;
int new_fonts_are_order2 = false;
int loaded_fonts_same_as_new = false;
#if __Mac
int alwaysgenapple = true;
int alwaysgenopentype = false;
#else
int alwaysgenapple = false;
int alwaysgenopentype = true;
#endif
char *helpdir;

extern int rectelipse, polystar, regular_star;	/* from cvpalettes.c */
extern int center_out;				/* from cvpalettes.c */
extern real rr_radius;				/* from cvpalettes.c */
extern int ps_pointcnt;				/* from cvpalettes.c */
extern real star_percent;			/* from cvpalettes.c */

static int pointless;

#define CID_Features	101
#define CID_Settings	102
#define CID_FeatureDel	103
#define CID_SettingDel	104
#define CID_FeatureEdit	105
#define CID_SettingEdit	106

/* ************************************************************************** */
/* *****************************    mac data    ***************************** */
/* ************************************************************************** */

static struct macfeaturenames {
    int mac_feature_type;
    char *feature_name;
} feature_type_to_name[] = {
    { 0, "All Features" },
    { 1, "Ligatures" },
    { 2, "Cursive Connection" },
    { 3, "Letter Case" },
    { 4, "Vertical Substitution" },
    { 5, "Linguistic Rearrangement" },
    { 6, "Number Spacing" },
    { 8, "Smart Swash" },
    { 9, "Diacritics" },
    { 10, "Vertical Position" },
    { 11, "Fractions" },
    { 13, "Overlapping Characters" },
    { 14, "Typographic Extras" },
    { 15, "Mathematical Extras" },
    { 16, "Ornament Sets" },
    { 17, "Character Alternates" },
    { 18, "Design Complexity" },
    { 19, "Style Options" },
    { 20, "Character Shape" },
    { 21, "Number Case" },
    { 22, "Text Spacing" },
    { 23, "Transliteration" },
    { 24, "Annotation" },
    { 25, "Kana Spacing" },
    { 26, "Ideographic Spacing" },
    { 27, "Unicode Decomposition" },
    { 103, "CJK Roman Spacing" },
    { -1, NULL }
}, *user_feature_type_to_name;

static void UserFeaturesFree(void) {
    int i;

    if ( user_feature_type_to_name==NULL )
return;

    for ( i=0; user_feature_type_to_name[i].feature_name!=NULL ; ++i )
	free( user_feature_type_to_name[i].feature_name );
    free( user_feature_type_to_name );
    user_feature_type_to_name = NULL;
}

static int UserFeaturesDiffer(void) {
    int i,j;

    if ( user_feature_type_to_name==NULL )
return( false );

    for ( i=0; user_feature_type_to_name[i].feature_name!=NULL; ++i );
    for ( j=0; feature_type_to_name[j].feature_name!=NULL; ++j );
    if ( i!=j )
return( true );
    for ( i=0; user_feature_type_to_name[i].feature_name!=NULL; ++i ) {
	for ( j=0; feature_type_to_name[j].feature_name!=NULL; ++j ) {
	    if ( feature_type_to_name[j].mac_feature_type==
		    user_feature_type_to_name[i].mac_feature_type )
	break;
	}
	if ( feature_type_to_name[j].feature_name==NULL ||
		strcmp(feature_type_to_name[j].feature_name,
			user_feature_type_to_name[i].feature_name)!=0 )
return( true );
    }
return( false );
}

struct macsettingname macfeat_otftag[] = {
    { 1, 0, 1, 0, 1, CHR('r','l','i','g'), "Required Ligatures", "No Required Ligatures" },	/* Required ligatures */
    { 1, 2, 3, 0, 1, CHR('l','i','g','a'), "Common Ligatures", "No Common Ligatures" },	/* Common ligatures */
    { 1, 4, 5, 0, 0, CHR('d','l','i','g'), "Rare Ligatures", "No Rare Ligatures" },	/* rare ligatures => discretionary */
    { 1, 4, 5, 0, 0, CHR('h','l','i','g'), "Rare Ligatures", "No Rare Ligatures" },	/* rare ligatures => historic */
    { 1, 4, 5, 0, 0, CHR('a','l','i','g'), "Rare Ligatures", "No Rare Ligatures" },	/* rare ligatures => historic */
    { 1, 6, 7, 0, 0, CHR('M','L','O','G'), "Logos", "No Logos" },
    { 1, 8, 9, 0, 0, CHR('M','R','E','B'), "Rebus Pictures", "No Rebus Pictures" },
    { 1, 10, 11, 0, 0, CHR('M','D','L','G'), "Diphthong Ligatures", "No Diphthong Ligatures" },
    { 1, 12, 13, 0, 0, CHR('M','S','L','G'), "Square Ligatures", "No Square Ligatures" },
    { 1, 14, 15, 0, 0, CHR('M','A','L','G'), "Abbreviated Square Ligatures", "No Abbreviated Square Ligatures" },
    /* 2, 1, partially connected cursive */
    { 2, 2, 0, 0, 1, CHR('i','s','o','l'), "Full Cursive Connection", "No Cursive Connection" },	/* Arabic forms */
    { 2, 2, 0, 0, 1, CHR('c','a','l','t'), "Full Cursive Connection", "No Cursive Connection" },	/* Arabic forms */
    /* 3, 2, all caps */
    /* 3, 3, all lower */
    { 3, 4, 0, 1, 0, CHR('s','m','c','p'), "Small Caps", "Upper and Lower Case" },	/* small caps */
    /* 3, 4, initial caps */
    /* 3, 5, initial caps, small caps */
    { 4, 0, 1, 0, 1, CHR('v','r','t','2'), "Vertical Forms", "No Vertical Forms" },	/* vertical forms => vertical rotation */
    { 4, 0, 1, 0, 1, CHR('v','k','n','a'), "Vertical Forms", "No Vertical Forms" },	/* vertical forms => vertical kana */
    { 6, 0, 1, 1, 0, CHR('t','n','u','m'), "Monospaced Numbers", "Proportional Numbers" },	/* monospace numbers => Tabular numbers */
    { 8, 0, 1, 0, 1, CHR('M','S','W','I'), "Word Initial Swash", "No Word Initial Swash" },
    { 8, 2, 3, 0, 1, CHR('M','S','W','F'), "Word Final Swash", "No Word Final Swash" },
    { 8, 4, 5, 0, 1, CHR('M','S','L','I'), "Line Initial Swash", "No Line Initial Swash" },
    { 8, 6, 7, 0, 1, CHR('M','S','L','F'), "Line Final Swash", "No Line Final Swash" },
    { 8, 8, 9, 0, 0, CHR('M','S','N','F'), "Non-Final Swash", "No Non-Final Swash" },
    { 10, 1, 0, 1, 0, CHR('s','u','p','s'), "Superscript", "Normal Position" },	/* superior vertical position => superscript */
    { 10, 3, 0, 1, 0, CHR('s','u','p','s'), "Superscript", "Normal Position" },	/* ordinal vertical position => superscript */
    { 10, 2, 0, 1, 0, CHR('s','u','b','s'), "Subscript", "Normal Position" },	/* inferior vertical position => subscript */
    { 11, 1, 0, 1, 1, CHR('f','r','a','c'), "Fraction Ligatures", "No Fractions" },	/* vertical fraction => fraction ligature */
    { 16, 1, 0, 1, 0, CHR('o','r','n','m'), "Dingbats", "No Ornaments" },	/* vertical fraction => fraction ligature */
    { 20, 0, -1, 1, 0, CHR('t','r','a','d'), "Traditional Characters", NULL },	/* tradictional characters => traditional forms */
    { 20, 0, -1, 1, 0, CHR('t','n','a','m'), "Traditional Characters", NULL },	/* tradictional characters => traditional names */
    { 20, 1, 0, 1, 0, CHR('s','m','p','l'), "Simplified Characters", "Traditional Characters" },	/* simplified characters */
    { 20, 2, -1, 1, 0, CHR('j','p','7','8'), "JIS 1978 Characters", NULL },	/* jis 1978 */
    { 20, 3, -1, 1, 0, CHR('j','p','8','3'), "JIS 1983 Characters", NULL },	/* jis 1983 */
    { 20, 4, -1, 1, 0, CHR('j','p','9','0'), "JIS 1990 Characters", NULL },	/* jis 1990 */
    { 21, 0, 1, 1, 0, CHR('o','n','u','m'), "Lower Case Number", "Upper Case Number" },	/* lower case number => old style numbers */
    { 22, 0, -1, 1, 1, CHR('p','w','i','d'), "Proportional Text", NULL },	/* proportional text => proportional widths */
    { 22, 1, 0, 1, 0, CHR('M','W','I','D'), "Monospace Text", "Proportional Text" },	/* proportional text => proportional widths */
    { 22, 2, 0, 1, 0, CHR('h','w','i','d'), "Half Width Text", "Proportional Text" },	/* half width text => half widths */
    { 22, 3, 0, 1, 0, CHR('f','w','i','d'), "Full Width Text", "Proportional Text" },	/* full width text => full widths */
    { 25, 0, -1, 1, 1, CHR('f','w','i','d'), "Full Width Kana", NULL },	/* full width kana => full widths */
    { 25, 1, 0, 0, 1, CHR('p','w','i','d'), "Proportional Kana", "Full Width Kana"  },	/* proportional kana => proportional widths */
    { 26, 0, -1, 1, 1, CHR('f','w','i','d'), "Full Width Ideograph", NULL },	/* full width ideograph => full widths */
    { 26, 1, 0, 1, 0, CHR('p','w','i','d'), "Proportional Ideograph", "Full Width Ideograph" },	/* proportional ideograph => proportional widths */
    { 27, 1, 0, 0, 1, CHR('M','U','C','M'), "Compose", "Off" },	/* Unicode decomposition */
    { 103, 0, -1, 1, 1, CHR('h','w','i','d'), "Half Width CJK Roman", NULL },	/* half width cjk roman => half widths */
    { 103, 1, 0, 1, 0, CHR('p','w','i','d'), "Proportional CJK Roman", "Half Width CJK Roman"  },	/* proportional cjk roman => proportional widths */
    { 103, 2, 0, 1, 0, CHR('M','W','I','D'), "Monospace CJK Roman", "Half Width CJK Roman" },	/* proportional text => proportional widths */
    { 103, 3, 0, 1, 0, CHR('f','w','i','d'), "Full Width CJK Roman", "Half Width CJK Roman" },	/* full width cjk roman => full widths */
    { 0, 0, 0 }
}, *user_macfeat_otftag;

static void UserSettingsFree(void) {
    int i;

    if ( user_macfeat_otftag==NULL )
return;

    for ( i=0; user_macfeat_otftag[i].on_name!=NULL ; ++i ) {
	free( user_macfeat_otftag[i].on_name );
	free( user_macfeat_otftag[i].off_name );
    }
    free( user_macfeat_otftag );
    user_macfeat_otftag = NULL;
}

static int UserSettingsDiffer(void) {
    int i,j;

    if ( user_macfeat_otftag==NULL )
return( false );

    for ( i=0; user_macfeat_otftag[i].on_name!=NULL; ++i );
    for ( j=0; macfeat_otftag[j].on_name!=NULL; ++j );
    if ( i!=j )
return( true );
    for ( i=0; user_macfeat_otftag[i].on_name!=NULL; ++i ) {
	for ( j=0; macfeat_otftag[j].on_name!=NULL; ++j ) {
	    if ( macfeat_otftag[j].mac_feature_type==
		    user_macfeat_otftag[i].mac_feature_type &&
		    macfeat_otftag[j].mac_feature_setting==
		    user_macfeat_otftag[i].mac_feature_setting )
	break;
	}
	if ( macfeat_otftag[j].on_name==NULL ||
		strcmp(macfeat_otftag[j].on_name,
			user_macfeat_otftag[i].on_name)!=0 ||
		macfeat_otftag[j].otf_tag!=user_macfeat_otftag[i].otf_tag ||
		macfeat_otftag[j].off_setting!=user_macfeat_otftag[i].off_setting ||
		macfeat_otftag[j].ismutex!=user_macfeat_otftag[i].ismutex ||
		macfeat_otftag[j].defaultOn!=user_macfeat_otftag[i].defaultOn )
return( true );
	if ( macfeat_otftag[j].off_name==NULL && user_macfeat_otftag[i].off_name==NULL )
	    /* Good */;
	else if ( macfeat_otftag[j].off_name==NULL || user_macfeat_otftag[i].off_name==NULL ||
		strcmp(macfeat_otftag[j].off_name,
			user_macfeat_otftag[i].off_name)!=0 )
return( true );
    }
return( false );
}

/**************************************************************************** */


static GTextInfo localencodingtypes[] = {
    { (unichar_t *) _STR_Default, NULL, 0, 0, (void *) e_unknown, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Isolatin1, NULL, 0, 0, (void *) e_iso8859_1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin0, NULL, 0, 0, (void *) e_iso8859_15, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin2, NULL, 0, 0, (void *) e_iso8859_2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin3, NULL, 0, 0, (void *) e_iso8859_3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin4, NULL, 0, 0, (void *) e_iso8859_4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin5, NULL, 0, 0, (void *) e_iso8859_9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin6, NULL, 0, 0, (void *) e_iso8859_10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin7, NULL, 0, 0, (void *) e_iso8859_13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin8, NULL, 0, 0, (void *) e_iso8859_14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Isocyrillic, NULL, 0, 0, (void *) e_iso8859_5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Koi8cyrillic, NULL, 0, 0, (void *) e_koi8_r, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isoarabic, NULL, 0, 0, (void *) e_iso8859_6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isogreek, NULL, 0, 0, (void *) e_iso8859_7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isohebrew, NULL, 0, 0, (void *) e_iso8859_8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isothai, NULL, 0, 0, (void *) e_iso8859_11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_MacLatin, NULL, 0, 0, (void *) e_mac, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Win, NULL, 0, 0, (void *) e_win, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Unicode, NULL, 0, 0, (void *) e_unicode, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UTF_8, NULL, 0, 0, (void *) e_utf8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_SJIS, NULL, 0, 0, (void *) e_sjis, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_KoreanWansung, NULL, 0, 0, (void *) e_wansung, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_KoreanJohab, NULL, 0, 0, (void *) e_johab, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ChineseTrad, NULL, 0, 0, (void *) e_big5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ChineseTradHKSCS, NULL, 0, 0, (void *) e_big5hkscs, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

/* don't use mnemonics 'C' or 'O' (Cancel & OK) */
enum pref_types { pr_int, pr_real, pr_bool, pr_enum, pr_encoding, pr_string, pr_file };
struct enums { char *name; int value; };

struct enums fvsize_enums[] = { NULL };

static struct prefs_list {
    char *name;
    enum pref_types type;
    void *val;
    void *(*get)(void);
    void (*set)(void *);
    char mn;
    struct enums *enums;
    unsigned int dontdisplay: 1;
    int popup;
} general_list[] = {
	{ "AutoHint", pr_bool, &autohint_before_rasterize, NULL, NULL, 'A', NULL, 0, _STR_PrefsPopupAH },
	{ "LocalEncoding", pr_encoding, &prefs_encoding, NULL, NULL, 'L', NULL, 0, _STR_PrefsPopupLoc },
	{ "NewCharset", pr_encoding, &default_encoding, NULL, NULL, 'N', NULL, 0, _STR_PrefsPopupForNewFonts },
	{ "NewEmSize", pr_int, &new_em_size, NULL, NULL, 'S', NULL, 0, _STR_PrefsPopupNES },
	{ "NewFontsQuadratic", pr_bool, &new_fonts_are_order2, NULL, NULL, 'Q', NULL, 0, _STR_PrefsPopupNOT },
	{ "LoadedFontsAsNew", pr_bool, &loaded_fonts_same_as_new, NULL, NULL, 'L', NULL, 0, _STR_PrefsPopupLFN },
	{ "GreekFixup", pr_bool, &greekfixup, NULL, NULL, 'G', NULL, 0, _STR_PrefsPopupGF },
	{ "ResourceFile", pr_file, &xdefs_filename, NULL, NULL, 'R', NULL, 0, _STR_PrefsPopupXRF },
	{ "HelpDir", pr_file, &helpdir, NULL, NULL, 'R', NULL, 0, _STR_PrefsPopupHLP },
	{ NULL }
},
  editing_list[] = {
	{ "AutoWidthSync", pr_bool, &adjustwidth, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupAWS },
	{ "AutoLBearingSync", pr_bool, &adjustlbearing, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupALS },
	{ "ItalicConstrained", pr_bool, &ItalicConstrained, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupIC },
	{ "ArrowMoveSize", pr_real, &arrowAmount, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupAA },
	{ "SnapDistance", pr_real, &snapdistance, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupSD },
	{ "JoinSnap", pr_real, &joinsnap, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupJS },
	{ "CopyMetaData", pr_bool, &copymetadata, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupCMD },
	{ "UndoDepth", pr_int, &maxundoes, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupUndo },
	{ "StopAtJoin", pr_bool, &stop_at_join, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupSAJ },
	{ NULL }
},
  accent_list[] = {
	{ "AccentOffsetPercent", pr_int, &accent_offset, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupAO },
	{ "AccentCenterLowest", pr_bool, &GraveAcuteCenterBottom, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupGA },
	{ "CharCenterHighest", pr_bool, &CharCenterHighest, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupCH },
	{ NULL }
},
 args_list[] = {
	{ "AutotraceArgs", pr_string, NULL, GetAutoTraceArgs, SetAutoTraceArgs, '\0', NULL, 0, _STR_PrefsPopupATA },
	{ "AutotraceAsk", pr_bool, &autotrace_ask, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupATK },
	{ "MfArgs", pr_string, &mf_args, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupMFA },
	{ "MfAsk", pr_bool, &mf_ask, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupMFK },
	{ "MfClearBg", pr_bool, &mf_clearbackgrounds, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupMFB },
	{ "MfShowErr", pr_bool, &mf_showerrors, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupMFE },
	{ NULL }
},
 generate_list[] = {
	{ "FoundryName", pr_string, &BDFFoundry, NULL, NULL, 'F', NULL, 0, _STR_PrefsPopupFN },
	{ "TTFFoundry", pr_string, &TTFFoundry, NULL, NULL, 'T', NULL, 0, _STR_PrefsPopupTFN },
	{ "XUID-Base", pr_string, &xuid, NULL, NULL, 'X', NULL, 0, _STR_PrefsPopupXU },
	{ "AskBDFResolution", pr_bool, &ask_user_for_resolution, NULL, NULL, 'B', NULL, 0, _STR_PrefsPopupBR },
	{ "DumpGlyphMap", pr_bool, &glyph_2_name_map, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupG2N },
	{ "PreferCJKEncodings", pr_bool, &prefer_cjk_encodings, NULL, NULL, 'C', NULL, 0, _STR_PrefsPopupPCE },
	{ "HintForGen", pr_bool, &autohint_before_generate, NULL, NULL, 'H', NULL, 0, _STR_PrefsPopupAHG },
	{ "AlwaysGenApple", pr_bool, &alwaysgenapple, NULL, NULL, 'A', NULL, 0, _STR_PrefsPopupAGA },
	{ "AlwaysGenOpenType", pr_bool, &alwaysgenopentype, NULL, NULL, 'A', NULL, 0, _STR_PrefsPopupAGO },
	{ NULL }
},
 hidden_list[] = {
	{ "AntiAlias", pr_bool, &default_fv_antialias, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVShowHmetrics", pr_int, &default_fv_showhmetrics, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVShowVmetrics", pr_int, &default_fv_showvmetrics, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVSize", pr_enum, &default_fv_font_size, NULL, NULL, 'S', fvsize_enums, 1 },
	{ "OnlyCopyDisplayed", pr_bool, &onlycopydisplayed, NULL, NULL, '\0', NULL, 1 },
	{ "PalettesDocked", pr_bool, &palettes_docked, NULL, NULL, '\0', NULL, 1 },
	{ "MarkExtrema", pr_int, &CVShows.markextrema, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultScreenDpiSystem", pr_int, &oldsystem, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultOutputFormat", pr_int, &oldformatstate, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultBitmapFormat", pr_int, &oldbitmapstate, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultTTFflags", pr_int, &old_ttf_flags, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultPSflags", pr_int, &old_ps_flags, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultOTFflags", pr_int, &old_otf_flags, NULL, NULL, '\0', NULL, 1 },
	{ "PageWidth", pr_int, &pagewidth, NULL, NULL, '\0', NULL, 1 },
	{ "PageHeight", pr_int, &pageheight, NULL, NULL, '\0', NULL, 1 },
	{ "PrintType", pr_int, &printtype, NULL, NULL, '\0', NULL, 1 },
	{ "PrintCommand", pr_string, &printcommand, NULL, NULL, '\0', NULL, 1 },
	{ "PageLazyPrinter", pr_string, &printlazyprinter, NULL, NULL, '\0', NULL, 1 },
	{ "ShowRulers", pr_bool, &CVShows.showrulers, NULL, NULL, '\0', NULL, 1, _STR_PrefsPopupRulers },
	{ "RegularStar", pr_bool, &regular_star, NULL, NULL, '\0', NULL, 1 },
	{ "PolyStar", pr_bool, &polystar, NULL, NULL, '\0', NULL, 1 },
	{ "RectEllipse", pr_bool, &rectelipse, NULL, NULL, '\0', NULL, 1 },
	{ "RectEllipseCenterOut", pr_bool, &center_out, NULL, NULL, '\0', NULL, 1 },
	{ "PolyStartPointCnt", pr_int, &ps_pointcnt, NULL, NULL, '\0', NULL, 1 },
	{ "RoundRectRadius", pr_real, &rr_radius, NULL, NULL, '\0', NULL, 1 },
	{ "StarPercent", pr_real, &star_percent, NULL, NULL, '\0', NULL, 1 },
	{ "CoverageFormatsAllowed", pr_int, &coverageformatsallowed, NULL, NULL, '\0', NULL, 1 },
	{ NULL }
},
 oldnames[] = {
	{ "LocalCharset", pr_encoding, &prefs_encoding, NULL, NULL, 'L', NULL, 0, _STR_PrefsPopupLoc },
	{ "DefaultTTFApple", pr_int, &pointless, NULL, NULL, '\0', NULL, 1 },
	{ "AcuteCenterBottom", pr_bool, &GraveAcuteCenterBottom, NULL, NULL, '\0', NULL, 1, _STR_PrefsPopupGA },
	{ NULL }
},
 *prefs_list[] = { general_list, editing_list, args_list, generate_list, hidden_list, NULL },
 *load_prefs_list[] = { general_list, editing_list, args_list, generate_list, hidden_list, oldnames, NULL };

struct visible_prefs_list { int tab_name; struct prefs_list *pl; } visible_prefs_list[] = {
    { _STR_Generic, general_list},
    { _STR_Editing, editing_list},
    { _STR_Accents, accent_list},
    { _STR_PrefsApps, args_list},
    { _STR_PrefsFontInfo, generate_list},
    { 0 }
 };

int GetPrefs(char *name,Val *val) {
    int i,j;

    for ( i=0; prefs_list[i]!=NULL; ++i ) for ( j=0; prefs_list[i][j].name!=NULL; ++j ) {
	if ( strcmp(prefs_list[i][j].name,name)==0 ) {
	    struct prefs_list *pf = &prefs_list[i][j];
	    if ( pf->type == pr_bool || pf->type == pr_int ) {
		val->type = v_int;
		val->u.ival = *((int *) (pf->val));
	    } else if ( pf->type == pr_real ) {
		val->type = v_int;
		val->u.ival = *((float *) (pf->val));
	    } else if ( pf->type == pr_string || pf->type == pr_file ) {
		val->type = v_str;
		val->u.sval = copy( *((char **) (pf->val)));
	    } else
return( false );

return( true );
	}
    }
return( false );
}

int SetPrefs(char *name,Val *val1, Val *val2) {
    int i,j;

    for ( i=0; prefs_list[i]!=NULL; ++i ) for ( j=0; prefs_list[i][j].name!=NULL; ++j ) {
	if ( strcmp(prefs_list[i][j].name,name)==0 ) {
	    struct prefs_list *pf = &prefs_list[i][j];
	    if ( pf->type == pr_bool || pf->type == pr_int ) {
		if ( (val1->type!=v_int && val1->type!=v_unicode) || val2!=NULL )
return( -1 );
		*((int *) (pf->val)) = val1->u.ival;
	    } else if ( pf->type == pr_real ) {
		if ( val1->type!=v_int || (val2!=NULL && val2->type!=v_int ))
return( -1 );
		*((float *) (pf->val)) = (val2==NULL ? val1->u.ival : val1->u.ival / (double) val2->u.ival);
	    } else if ( pf->type == pr_string || pf->type == pr_file ) {
		if ( val1->type!=v_str || val2!=NULL )
return( -1 );
		free( *((char **) (pf->val)));
		*((char **) (pf->val)) = copy( val1->u.sval );
	    } else
return( false );

	    SavePrefs();
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
    if ( getPfaEditDir(buffer)==NULL )
return( NULL );
    sprintf(buffer,"%s/prefs", getPfaEditDir(buffer));
    prefs = copy(buffer);
return( prefs );
}

char *getPfaEditShareDir(void) {
    static char *sharedir=NULL;
    static int set=false;
    char *pt;
    int len;

    if ( set )
return( sharedir );

    set = true;
    pt = strstr(GResourceProgramDir,"/bin");
    if ( pt==NULL )
return( NULL );
    len = (pt-GResourceProgramDir)+strlen("/share/pfaedit")+1;
    sharedir = galloc(len);
    strncpy(sharedir,GResourceProgramDir,pt-GResourceProgramDir);
    strcpy(sharedir+(pt-GResourceProgramDir),"/share/pfaedit");
return( sharedir );
}

static int CheckLangDir(char *full,int sizefull,char *dir, const char *loc) {
    char buffer[100];

    if ( loc==NULL || dir==NULL )
return(false);

    strcpy(buffer,"pfaedit-");
    strcat(buffer,loc);
    strcat(buffer,".ui");

    /*first look for full locale string (pfaedit-en_us.iso8859-1.ui) */
    GFileBuildName(dir,buffer,full,sizefull);
    /* Look for language_territory */
    if ( GFileExists(full))
return( true );
    if ( strlen(loc)>5 ) {
	strcpy(buffer+13,".ui");
	GFileBuildName(dir,buffer,full,sizefull);
	if ( GFileExists(full))
return( true );
    }
    /* Look for language */
    if ( strlen(loc)>2 ) {
	strcpy(buffer+10,".ui");
	GFileBuildName(dir,buffer,full,sizefull);
	if ( GFileExists(full))
return( true );
    }
return( false );
}

static int CheckLangNoLibsDir(char *full,int sizefull,char *dir, const char *loc) {
    int ret;

    if ( loc==NULL || dir==NULL || strstr(dir,"/.libs")==NULL )
return(false);

    dir = copy(dir);
    *strstr(dir,"/.libs") = '\0';

    ret = CheckLangDir(full,sizefull,dir,loc);
    free(dir);
return( ret );
}

static void CheckLang(void) {
    /*const char *loc = setlocale(LC_MESSAGES,NULL);*/ /* This always returns "C" for me, even when it shouldn't be */
    const char *loc = getenv("LC_ALL");
    char buffer[100], full[1024];
    extern int PfaEditNomenChecksum;		/* In pfaedit-ui.c */

    if ( loc==NULL ) loc = getenv("LC_MESSAGES");
    if ( loc==NULL ) loc = getenv("LANG");

    if ( loc==NULL )
return;

    strcpy(buffer,"pfaedit.");
    strcat(buffer,loc);
    strcat(buffer,".ui");
    if ( !CheckLangDir(full,sizeof(full),GResourceProgramDir,loc) &&
	    !CheckLangNoLibsDir(full,sizeof(full),GResourceProgramDir,loc) &&
#ifdef SHAREDIR
	    !CheckLangDir(full,sizeof(full),SHAREDIR,loc) &&
#endif
	    !CheckLangDir(full,sizeof(full),getPfaEditShareDir(),loc) &&
	    !CheckLangDir(full,sizeof(full),"/usr/share/pfaedit",loc) )
return;

    GStringSetResourceFileV(full,PfaEditNomenChecksum);
}

static int encmatch(const char *enc) {
    static struct { char *name; int enc; } encs[] = {
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
	{ "KOI8-R", e_koi8_r },
	{ "CP1252", e_win },
	{ "Big5", e_big5 },
	{ "Big5HKSCS", e_big5hkscs },
	{ "UTF-8", e_utf8 },
	{ "ISO-10646-1", e_unicode },
#if 0
	{ "eucJP", e_euc },
	{ "EUC-JP", e_euc },
	{ "ujis", ??? },
	{ "EUC-KR", e_euckorean },
	{ "ISO-8859-4", e_iso8859_4 },
#endif
	{ NULL }};
    int i;

    for ( i=0; encs[i].name!=NULL; ++i )
	if ( strmatch(enc,encs[i].name)==0 )
return( encs[i].enc );

return( e_unknown );
}

static int DefaultEncoding(void) {
    const char *loc = getenv("LC_ALL");
    int enc;

    if ( loc==NULL ) loc = getenv("LC_CTYPE");
    /*if ( loc==NULL ) loc = getenv("LC_MESSAGES");*/
    if ( loc==NULL ) loc = getenv("LANG");

    if ( loc==NULL )
return( e_iso8859_1 );

    enc = encmatch(loc);
    if ( enc==e_unknown ) {
	loc = strrchr(loc,'.');
	if ( loc==NULL )
return( e_iso8859_1 );
	enc = encmatch(loc+1);
    }
    if ( enc==e_unknown )
return( e_iso8859_1 );

return( enc );
}

static void GreekHack(void) {
    int i;

    if ( greekfixup ) {
	psunicodenames[0x2206] = NULL;		/* Increment */
	psunicodenames[0x2126] = NULL;		/* Ohm sign */
	psunicodenames[0x0394] = "Delta";	/* Delta */
	psunicodenames[0x03A9] = "Omega";	/* Omega */

#if 0		/* I've moved these into the unicode alternate names table */
	psunicodenames[0xf500] = "Alphasmall";
	psunicodenames[0xf501] = "Betasmall";
	psunicodenames[0xf502] = "Gammasmall";
	psunicodenames[0xf503] = "Deltasmall";
	psunicodenames[0xf504] = "Epsilonsmall";
	psunicodenames[0xf505] = "Zetasmall";
	psunicodenames[0xf506] = "Etasmall";
	psunicodenames[0xf507] = "Thetasmall";
	psunicodenames[0xf508] = "Iotasmall";
	psunicodenames[0xf509] = "Kappasmall";
	psunicodenames[0xf50a] = "Lambdasmall";
	psunicodenames[0xf50b] = "Musmall";
	psunicodenames[0xf50c] = "Nusmall";
	psunicodenames[0xf50d] = "Xismall";
	psunicodenames[0xf50e] = "Omicronsmall";
	psunicodenames[0xf50f] = "Pismall";
	psunicodenames[0xf510] = "Rhosmall";
	psunicodenames[0xf511] = NULL,
	psunicodenames[0xf512] = "Sigmasmall";
	psunicodenames[0xf513] = "Tausmall";
	psunicodenames[0xf514] = "Upsilonsmall";
	psunicodenames[0xf515] = "Phismall";
	psunicodenames[0xf516] = "Chismall";
	psunicodenames[0xf517] = "Psismall";
	psunicodenames[0xf518] = "Omegasmall";
	psunicodenames[0xf519] = "Iotadieresissmall";
	psunicodenames[0xf51a] = "Upsilondieresissmall";
#endif
    } else {
	psunicodenames[0x2206] = "Delta";	/* Increment */
	psunicodenames[0x2126] = "Omega";	/* Ohm sign */
	psunicodenames[0x0394] = NULL;		/* Delta */
	psunicodenames[0x03A9] = NULL;		/* Omega */
	for ( i=0xf500; i<=0xf51a; ++i )
	    psunicodenames[i] = NULL;
    }
    /* I'm leaving mu at 00b5 (rather than 03bc) */

#if 0
/* Adobe says 0x03d6 should be called "omega1", but Unicode says it's a varient of Pi */
/* Done now with alternate PS names */
    if ( getenv("PFAEDIT_PI1")!=NULL )
	psunicodenames[0x03d6] = "pi1";
    else
	psunicodenames[0x03d6] = "omega1";
#endif
}

static unichar_t *utf8_copy(char *src) {
    unichar_t *ret = galloc((strlen(src)+1)*sizeof(unichar_t)), *pt=ret;

    while ( *src!='\0' ) {
	int ch1, ch2;
	ch1 = *src++;
	if ( ch1<=0x7f )
	    *pt++ = ch1;
	else if ( (ch1&0xf0)==0xc0 ) {
	    *pt++ = (ch1&0x1f)<<6 | (*src++&0x3f);
	} else {
	    ch2 = *src++;
	    *pt++ = (ch1&0xf)<<6 | ((ch2&0x3f)<<6) | (*src++&0x3f);
	}
    }
    *pt = '\0';
return( ret );
}

static char *cutf8_copy(unichar_t *src) {
    char *ret = galloc(3*u_strlen(src)+1), *pt=ret;

    while ( *src!='\0' ) {
	int ch;
	ch = *src++;
	if ( ch<=0x7f )
	    *pt++ = ch;
	else if ( ch<0x03ff ) {
	    *pt++ = 0xc0|(ch>>6);
	    *pt++ = 0x80|(ch&0x3f);
	} else {
	    *pt++ = 0xe0|(ch>>12);
	    *pt++ = 0x80|((ch>>6)&0x3f);
	    *pt++ = 0x80|(ch&0x3f);
	}
    }
    *pt = '\0';
return( ret );
}

static void ParseMacSetting(char *pt,struct macsettingname *ms) {
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
    end += 4;
    while ( *end==' ' ) ++end;
    if ( *end=='"' ) ++end;
    for ( pt=end; *pt && *pt!='"'; ++pt );
    ms->on_name = copyn(end,pt-end);
    if ( *pt=='"' ) ++pt;

    ms->off_setting = strtol(pt,&end,10);
    while ( *end==' ' ) ++end;
    if ( *end=='"' ) ++end;
    for ( pt=end; *pt && *pt!='"'; ++pt );
    if ( pt!=end )
	ms->off_name = copyn(end,pt-end);
    else
	ms->off_name = NULL;
    if ( *pt=='"' ) ++pt;
    if ( *pt==' ' ) ++pt;
    if ( *pt++ == 'm' ) ms->ismutex = true;
    if ( *pt++ == 'd' ) ms->defaultOn = true;
}

static void DefaultXUID(void) {
    /* Adobe has assigned PfaEdit a base XUID of 1021. Each new user is going */
    /*  to get a couple of random numbers appended to that, hoping that will */
    /*  make for a fairly safe system. */
    int r1, r2;
    char buffer[50];
    struct timeval tv;

    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);
    do {
	r1 = rand()&0x3ff;
    } while ( r1==0 );		/* I reserve "0" for me! */
    gettimeofday(&tv,NULL);
    srandom(tv.tv_usec+1);
    r2 = random();
    sprintf( buffer, "1021 %d %d", r1, r2 );
    xuid = copy(buffer);
}

static void DefaultHelp(void) {
    if ( helpdir==NULL ) {
#ifdef DOCDIR
	helpdir = copy(DOCDIR "/");
#elif defined(SHAREDIR)
	helpdir = copy(SHAREDIR "/../doc/pfaedit/");
#else
	helpdir = copy("/usr/local/share/doc/pfaedit/");
#endif
    }
}

static void DoDefaults(void) {
    DefaultXUID();
    DefaultHelp();
}

void LoadPrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    char line[1100];
    int i, j, ri=0, mn=0, ms=0;
    int mfp=0, mfc=0, msp=0, msc=0;
    char *pt, *end;
    struct prefs_list *pl;

    PfaEditSetFallback();
    LoadPfaEditEncodings();
    CheckLang();
    GreekHack();

    if ( prefs==NULL || (p=fopen(prefs,"r"))==NULL ) {
	DoDefaults();
return;
    }
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
		script_menu_names[mn++] = utf8_copy(pt);
	    else if ( strncmp(line,"MacFeatCnt:",strlen("MacFeatCnt:"))==0 ) {
		sscanf( pt, "%d", &mfc );
		mfp = 0;
		user_feature_type_to_name = gcalloc(mfc+1,sizeof(struct macfeaturenames));
	    } else if ( strncmp(line,"MacFeature:",strlen("MacFeature:"))==0 && mfp<mfc ) {
		user_feature_type_to_name[mfp].mac_feature_type =
			strtol(pt,&end,10);
		for ( pt=end; *pt==' '; ++pt );
		user_feature_type_to_name[mfp++].feature_name = copy(pt);
	    } else if ( strncmp(line,"MacSetCnt:",strlen("MacSetCnt:"))==0 ) {
		sscanf( pt, "%d", &msc );
		msp = 0;
		user_macfeat_otftag = gcalloc(msc+1,sizeof(struct macsettingname));
	    } else if ( strncmp(line,"MacSetting:",strlen("MacSetting:"))==0 && msp<msc ) {
		ParseMacSetting(pt,&user_macfeat_otftag[msp++]);
	    }
    continue;
	}
	switch ( pl->type ) {
	  case pr_encoding:
	    if ( sscanf( pt, "%d", pl->val )!=1 ) {
		Encoding *item;
		for ( item = enclist; item!=NULL && strcmp(item->enc_name,pt)!=0; item = item->next );
		if ( item==NULL )
		    *((int *) (pl->val)) = e_iso8859_1;
		else
		    *((int *) (pl->val)) = item->enc_num;
	    }
	  break;
	  case pr_bool: case pr_int:
	    sscanf( pt, "%d", pl->val );
	  break;
	  case pr_real:
	    sscanf( pt, "%f", pl->val );
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
    GreekHack();
    DefaultHelp();
    if ( prefs_encoding==e_unknown )
	local_encoding = DefaultEncoding();
    else
	local_encoding = prefs_encoding;
    if ( xdefs_filename!=NULL )
	GResourceAddResourceFile(xdefs_filename,GResourceProgramName);
}

void SavePrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    int i, j, val;
    char *temp;
    struct prefs_list *pl;

    if ( prefs==NULL )
return;
    if ( (p=fopen(prefs,"w"))==NULL )
return;

    for ( j=0; prefs_list[j]!=NULL; ++j ) for ( i=0; prefs_list[j][i].name!=NULL; ++i ) {
	pl = &prefs_list[j][i];
	switch ( pl->type ) {
	  case pr_encoding:
	    val = *(int *) (pl->val);
	    if ( val<em_base || val>=em_unicodeplanes )
		fprintf( p, "%s:\t%d\n", pl->name, val );
	    else {
		Encoding *item;
		for ( item = enclist; item!=NULL && item->enc_num!=val; item=item->next );
		fprintf( p, "%s:\t%s\n", pl->name, item==NULL?"-1":item->enc_name );
	    }
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
	fprintf( p, "MenuName:\t%s\n", temp = cutf8_copy(script_menu_names[i]));
	free(temp);
    }
    if ( user_feature_type_to_name!=NULL && UserFeaturesDiffer()) {
	for ( i=0; user_feature_type_to_name[i].feature_name!=NULL; ++i );
	fprintf( p, "MacFeatCnt: %d\n", i );
	for ( i=0; user_feature_type_to_name[i].feature_name!=NULL; ++i ) {
	    fprintf( p, "MacFeature: %d %s\n",
		    user_feature_type_to_name[i].mac_feature_type,
		    user_feature_type_to_name[i].feature_name );
	}
    }
    if ( user_macfeat_otftag!=NULL && UserSettingsDiffer()) {
	for ( i=0; user_macfeat_otftag[i].on_name!=NULL; ++i );
	fprintf( p, "MacSetCnt: %d\n", i );
	for ( i=0; user_macfeat_otftag[i].on_name!=NULL; ++i ) {
	    fprintf( p, "MacSetting: %d,%d %c%c%c%c \"%s\" %d \"%s\" %c%c\n",
		    user_macfeat_otftag[i].mac_feature_type,
		    user_macfeat_otftag[i].mac_feature_setting,
			user_macfeat_otftag[i].otf_tag>>24,
			(user_macfeat_otftag[i].otf_tag>>16)&0xff,
			(user_macfeat_otftag[i].otf_tag>>8)&0xff,
			user_macfeat_otftag[i].otf_tag&0xff,
		    user_macfeat_otftag[i].on_name,
		    user_macfeat_otftag[i].off_setting,
		    user_macfeat_otftag[i].off_name ? user_macfeat_otftag[i].off_name : "",
		    user_macfeat_otftag[i].ismutex ? 'm' : ' ',
		    user_macfeat_otftag[i].defaultOn ? 'd' : ' ');
	}
    }

    fclose(p);
}

struct pref_data {
    int done;
};

static int Prefs_ScriptBrowse(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *tf = GWidgetGetControl(gw,GGadgetGetCid(g)-100);
	const unichar_t *cur = _GGadgetGetTitle(tf); unichar_t *ret;
	static unichar_t filter[] = { '*','.','p','e',  0 };

	if ( *cur=='\0' ) cur=NULL;
	ret = GWidgetOpenFile(GStringGetResource(_STR_CallScript,NULL), cur, filter, NULL,NULL);
	if ( ret==NULL )
return(true);
	GGadgetSetTitle(tf,ret);
	free(ret);
    }
return( true );
}

static int Prefs_BrowseFile(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *tf = GWidgetGetControl(gw,GGadgetGetCid(g)-20000);
	const unichar_t *cur = _GGadgetGetTitle(tf); unichar_t *ret;

	if ( *cur=='\0' ) cur=NULL;
	ret = GWidgetOpenFile(GStringGetResource(_STR_CallScript,NULL), cur, NULL, NULL,NULL);
	if ( ret==NULL )
return(true);
	GGadgetSetTitle(tf,ret);
	free(ret);
    }
return( true );
}

static GTextInfo *Pref_SettingsList(void) {
    struct macsettingname *msn = user_macfeat_otftag ? user_macfeat_otftag : macfeat_otftag;
    GTextInfo *ti;
    int i, len, max=0;
    char *str;

    for ( i=0; msn[i].on_name!=NULL; ++i ) {
	len = strlen( msn[i].on_name) + (msn[i].off_name?strlen(msn[i].off_name):0) ;
	if ( len>max ) max = len;
    }
    ti = gcalloc(i+1,sizeof( GTextInfo ));

    str = galloc(70+max);
    for ( i=0; msn[i].on_name!=NULL; ++i ) {
	sprintf(str,"%3d,%2d %c%c%c%c \"%s\" %d \"%s\" %c%c",
	    msn[i].mac_feature_type, msn[i].mac_feature_setting,
	    msn[i].otf_tag>>24, (msn[i].otf_tag>>16)&0xff, (msn[i].otf_tag>>8)&0xff, msn[i].otf_tag&0xff,
	    msn[i].on_name,
	    msn[i].off_setting, msn[i].off_name?msn[i].off_name:"",
	    msn[i].ismutex ? 'm':' ', msn[i].defaultOn? 'd': ' ');
	ti[i].text = galloc(((len = strlen(str)+1))*sizeof(unichar_t));
	encoding2u_strncpy(ti[i].text,str,len,e_mac);
    }
    free(str);
return( ti );
}

static void GListAddStr(GGadget *list,char *str) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    GTextInfo **replace = galloc((len+2)*sizeof(GTextInfo *));

    replace[len+1] = gcalloc(1,sizeof(GTextInfo));
    for ( i=0; i<len; ++i ) {
	replace[i] = galloc(sizeof(GTextInfo));
	*replace[i] = *ti[i];
	replace[i]->text = u_copy(ti[i]->text);
    }
    replace[i] = gcalloc(1,sizeof(GTextInfo));
    replace[i]->fg = replace[i]->bg = COLOR_DEFAULT;
    replace[i]->text = galloc((strlen(str)+1)*sizeof(unichar_t));
    encoding2u_strncpy(replace[i]->text,str,strlen(str)+1,e_mac);
    GGadgetSetList(list,replace,false);
}

static void GListReplaceStr(GGadget *list,int index, char *str) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    GTextInfo **replace = galloc((len+2)*sizeof(GTextInfo *));

    replace[len+1] = gcalloc(1,sizeof(GTextInfo));
    for ( i=0; i<len; ++i ) {
	replace[i] = galloc(sizeof(GTextInfo));
	*replace[i] = *ti[i];
	if ( i!=index )
	    replace[i]->text = u_copy(ti[i]->text);
    }
    replace[i] = gcalloc(1,sizeof(GTextInfo));
    replace[index]->text = galloc((strlen(str)+1)*sizeof(unichar_t));
    encoding2u_strncpy(replace[index]->text,str,strlen(str)+1,e_mac);
    GGadgetSetList(list,replace,false);
}

struct setdata {
    GWindow gw;
    GGadget *list;
    GGadget *flist;
    GGadget *feature;
    GGadget *set_code;
    GGadget *set_name;
    GGadget *off_code;
    GGadget *off_name;
    GGadget *def;
    GGadget *mutex;
    GGadget *otf;
    GGadget *ok;
    GGadget *cancel;
    int index;
    int done;
    char *ret;
};

static int set_e_h(GWindow gw, GEvent *event) {
    struct setdata *sd = GDrawGetUserData(gw);
    int i;
    int32 len;
    GTextInfo **ti;
    const unichar_t *ret1; unichar_t *end;
    int on, off, feat, val1, val2;
    char *on_name, *off_name;
    unichar_t ubuf[4];

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
		GWidgetErrorR(_STR_BadNumber,_STR_BadNumber);
return( true );
	    }
	    ret1 = _GGadgetGetTitle(sd->off_code);
	    off = u_strtol(ret1,&end,10);
	    if ( *end!='\0' ) {
		GWidgetErrorR(_STR_BadNumber,_STR_BadNumber);
return( true );
	    }
	    ret1 = _GGadgetGetTitle(sd->feature);
	    feat = u_strtol(ret1,&end,10);
	    if ( *end!='\0' && *end!=' ' ) {
		GWidgetErrorR(_STR_BadNumber,_STR_BadNumber);
return( true );
	    }
	    ti = GGadgetGetList(sd->list,&len);
	    for ( i=0; i<len; ++i ) if ( i!=sd->index ) {
		val1 = u_strtol(ti[i]->text,&end,10);
		val2 = u_strtol(end+1,NULL,10);
		if ( val1==feat && val2==on ) {
		    GWidgetErrorR(_STR_ThisSettingCodeIsAlreadyUsed,_STR_ThisSettingCodeIsAlreadyUsed);
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
		GWidgetErrorR(_STR_TagTooLong,_STR_FeatureTagTooLong);
return( true );
	    }

	    ret1 = _GGadgetGetTitle(sd->set_name);
	    if ( *ret1=='\0' ) {
		GWidgetErrorR(_STR_BadName,_STR_BadName);
return( true );
	    }
	    len = u_strlen(ret1);
	    on_name = galloc(len+1);
	    u2encoding_strncpy(on_name,ret1,len+1,e_mac);

	    ret1 = _GGadgetGetTitle(sd->off_name);
	    if ( *ret1=='\0' && off!=-1 ) {
		GWidgetErrorR(_STR_BadName,_STR_OffNameNull);
		free(on_name);
return( true );
	    }
	    len = u_strlen(ret1);
	    off_name = galloc(len+1);
	    u2encoding_strncpy(off_name,ret1,len+1,e_mac);
	    
	    sd->ret = galloc( strlen(on_name)+strlen(off_name)+70 );
	    sprintf(sd->ret,"%3d,%2d %c%c%c%c \"%s\" %d \"%s\" %c%c",
		    feat, on,
		    ubuf[0], ubuf[1], ubuf[2], ubuf[3],
		    on_name,
		    off, off_name,
		    GGadgetIsChecked(sd->mutex) ? 'm':' ',
		    GGadgetIsChecked(sd->def)? 'd': ' ');
	    sd->done = true;
	    free(on_name); free(off_name);
	}
    }
return( true );
}
    
static char *AskSetting(struct macsettingname *temp,GGadget *list, int index,GGadget *flist) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[17];
    GTextInfo label[17];
    struct setdata sd;
    char buf[20], buf2[20];
    unichar_t *ubuf, *ubuf2, ubuf3[6];
    int32 len, i;
    GTextInfo **ti;

    memset(&sd,0,sizeof(sd));
    sd.list = list;
    sd.flist = flist;
    sd.index = index;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Setting,NULL);
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,240));
    pos.height = GDrawPointsToPixels(NULL,160);
    gw = GDrawCreateTopWindow(NULL,&pos,set_e_h,&sd,&wattrs);
    sd.gw = gw;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    label[0].text = (unichar_t *) _STR_FeatureC;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+4;
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 50; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 170;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].creator = GListButtonCreate;

    label[2].text = (unichar_t *) _STR_Code;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[0].gd.pos.y+24;
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( buf, "%d", temp->mac_feature_setting );
    label[3].text = (unichar_t *) buf;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 40; gcd[3].gd.pos.y = gcd[2].gd.pos.y-4; gcd[3].gd.pos.width = 50;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].creator = GTextFieldCreate;

    label[4].text = (unichar_t *) _STR_Name;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 100; gcd[4].gd.pos.y = gcd[2].gd.pos.y;
    gcd[4].gd.flags = gg_enabled|gg_visible;
    gcd[4].creator = GLabelCreate;

    ubuf = NULL;
    if ( temp->on_name ) {
	len = strlen( temp->on_name );
	ubuf = galloc((len+1)*sizeof(unichar_t));
	encoding2u_strncpy(ubuf,temp->on_name,len+1,e_mac);
	label[5].text = ubuf;
	gcd[5].gd.label = &label[5];
    }
    gcd[5].gd.pos.x = 140; gcd[5].gd.pos.y = gcd[3].gd.pos.y;
    gcd[5].gd.flags = gg_enabled|gg_visible;
    gcd[5].creator = GTextFieldCreate;

    label[6].text = (unichar_t *) _STR_Off;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = gcd[2].gd.pos.y+24;
    gcd[6].gd.flags = gg_enabled|gg_visible;
    gcd[6].creator = GLabelCreate;

    sprintf( buf2, "%d", temp->off_setting );
    label[7].text = (unichar_t *) buf2;
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = 40; gcd[7].gd.pos.y = gcd[6].gd.pos.y-4; gcd[7].gd.pos.width = 50;
    gcd[7].gd.flags = gg_enabled|gg_visible;
    gcd[7].creator = GTextFieldCreate;

    label[8].text = (unichar_t *) "";
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = 100; gcd[8].gd.pos.y = gcd[6].gd.pos.y;
    gcd[8].gd.flags = gg_enabled|gg_visible;
    gcd[8].creator = GLabelCreate;

    ubuf2 = NULL;
    if ( temp->off_name ) {
	len = strlen( temp->off_name );
	ubuf2 = galloc((len+1)*sizeof(unichar_t));
	encoding2u_strncpy(ubuf2,temp->off_name,len+1,e_mac);
	label[9].text = ubuf2;
	gcd[9].gd.label = &label[9];
    }
    gcd[9].gd.pos.x = 140; gcd[9].gd.pos.y = gcd[7].gd.pos.y;
    gcd[9].gd.flags = gg_enabled|gg_visible;
    gcd[9].creator = GTextFieldCreate;

    label[10].text = (unichar_t *) _STR_Default;
    label[10].text_in_resource = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.pos.x = 20; gcd[10].gd.pos.y = gcd[6].gd.pos.y+20;
    gcd[10].gd.flags = gg_enabled|gg_visible|(temp->defaultOn?gg_cb_on:0);
    gcd[10].creator = GCheckBoxCreate;

    label[11].text = (unichar_t *) _STR_Mutex;
    label[11].text_in_resource = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.pos.x = 80; gcd[11].gd.pos.y = gcd[10].gd.pos.y;
    gcd[11].gd.flags = gg_enabled|gg_visible|(temp->ismutex?gg_cb_on:0);
    gcd[11].creator = GCheckBoxCreate;

    label[12].text = (unichar_t *) _STR_TagC;
    label[12].text_in_resource = true;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.pos.x = 5; gcd[12].gd.pos.y = gcd[11].gd.pos.y+22; 
    gcd[12].gd.flags = gg_enabled|gg_visible;
    gcd[12].creator = GLabelCreate;

    ubuf3[0] = temp->otf_tag>>24; ubuf3[1] = (temp->otf_tag>>16)&0xff; ubuf3[2] = (temp->otf_tag>>8)&0xff; ubuf3[3] = temp->otf_tag&0xff; ubuf3[4] = 0;
    label[13].text = ubuf3;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.pos.x = 40; gcd[13].gd.pos.y = gcd[12].gd.pos.y-4; gcd[13].gd.pos.width = 50;
    gcd[13].gd.flags = gg_enabled|gg_visible;
    /*gcd[13].gd.u.list = tags;*/
    gcd[13].creator = GTextFieldCreate;

    gcd[14].gd.pos.x = 13-3; gcd[14].gd.pos.y = gcd[13].gd.pos.y+30;
    gcd[14].gd.pos.width = -1; gcd[14].gd.pos.height = 0;
    gcd[14].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[14].text = (unichar_t *) _STR_OK;
    label[14].text_in_resource = true;
    gcd[14].gd.label = &label[14];
    /*gcd[14].gd.handle_controlevent = Prefs_Ok;*/
    gcd[14].creator = GButtonCreate;

    gcd[15].gd.pos.x = -13; gcd[15].gd.pos.y = gcd[15-1].gd.pos.y+3;
    gcd[15].gd.pos.width = -1; gcd[15].gd.pos.height = 0;
    gcd[15].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[15].text = (unichar_t *) _STR_Cancel;
    label[15].text_in_resource = true;
    gcd[15].gd.label = &label[15];
    /*gcd[15].gd.handle_controlevent = Prefs_Cancel;*/
    gcd[15].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    sd.feature = gcd[1].ret;
    sd.set_code = gcd[3].ret;
    sd.set_name = gcd[5].ret;
    sd.off_code = gcd[7].ret;
    sd.off_name = gcd[9].ret;
    sd.def = gcd[10].ret;
    sd.mutex = gcd[11].ret;
    sd.otf = gcd[13].ret;
    sd.ok = gcd[14].ret;
    sd.cancel = gcd[15].ret;

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

    free(temp->on_name ); free(temp->off_name); free(ubuf); free(ubuf2);
return( sd.ret );
}

static void ChangeSetting(GGadget *list,int index,GGadget *flist) {
    struct macsettingname temp;
    int32 len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    char *str;

    len = u_strlen(ti[index]->text);
    str = galloc(len+3);
    u2encoding_strncpy(str,ti[index]->text,len+1,e_mac);
    ParseMacSetting(str,&temp);
    free(str);
    if ( (str=AskSetting(&temp,list,index,flist))==NULL )
return;
    GListReplaceStr(list,index,str);
}

static int Pref_NewSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list = GWidgetGetControl(gw,CID_Settings);
	GGadget *flist = GWidgetGetControl(GDrawGetParentWindow(gw),CID_Features);
	struct macsettingname temp;
	char *str;

	memset(&temp,0,sizeof(temp));
	temp.mac_feature_type = -1;
	temp.off_setting = 1;
	if ( (str=AskSetting(&temp,list,-1,flist))==NULL )
return( true );
	GListAddStr(list,str);
	free(str);
    }
return( true );
}

static int Pref_DelSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GListDelSelected(GWidgetGetControl(gw,CID_Settings));
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_SettingDel),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_SettingEdit),false);
    }
return( true );
}

static int Pref_EditSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
	GGadget *list = GWidgetGetControl(gw,CID_Settings);
	GGadget *flist = GWidgetGetControl(gw,CID_Features);
	ChangeSetting(list,GGadgetGetFirstListSelectedItem(list),flist);
    }
return( true );
}

static int Pref_SettingSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	GWindow gw = GGadgetGetWindow(g);
	int i, sel_cnt=0;
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected ) ++sel_cnt;
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_SettingDel),sel_cnt!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_SettingEdit),sel_cnt==1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	GGadget *flist = GWidgetGetControl( GDrawGetParentWindow(GGadgetGetWindow(g)),CID_Features);
	ChangeSetting(g,e->u.control.u.list.changed_index!=-1?e->u.control.u.list.changed_index:
		GGadgetGetFirstListSelectedItem(g),flist);
    }
return( true );
}

static GTextInfo *Pref_FeaturesList(void) {
    struct macfeaturenames *mfn = user_feature_type_to_name ? user_feature_type_to_name : feature_type_to_name;
    GTextInfo *ti;
    int i, len, max=0;
    char *str;

    for ( i=0; mfn[i].feature_name!=NULL; ++i ) {
	len = strlen( mfn[i].feature_name);
	if ( len>max ) max = len;
    }
    ti = gcalloc(i+1,sizeof( GTextInfo ));

    str = galloc(30+len);
    for ( i=0; mfn[i].feature_name!=NULL; ++i ) {
	sprintf(str,"%3d %s", mfn[i].mac_feature_type, mfn[i].feature_name);
	ti[i].text = galloc(((len = strlen(str)+1))*sizeof(unichar_t));
	encoding2u_strncpy(ti[i].text,str,len,e_mac);
    }
    free(str);
return( ti );
}

struct featdata {
    GWindow gw;
    GGadget *list;
    GGadget *code;
    GGadget *name;
    GGadget *ok;
    GGadget *cancel;
    int index;
    int done;
    char *ret;
};

static int feat_e_h(GWindow gw, GEvent *event) {
    struct featdata *fd = GDrawGetUserData(gw);
    int i;
    int32 len;
    GTextInfo **ti;
    const unichar_t *ret1; unichar_t *end;
    int val1, val2;
    char buf[20];

    if ( event->type==et_close ) {
	fd->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("prefs.html#Features");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	if ( event->u.control.g == fd->cancel ) {
	    fd->done = true;
	} else if ( event->u.control.g == fd->ok ) {
	    ret1 = _GGadgetGetTitle(fd->code);
	    val1 = u_strtol(ret1,&end,10);
	    if ( *end!='\0' ) {
		GWidgetErrorR(_STR_BadNumber,_STR_BadNumber);
return( true );
	    }
	    ti = GGadgetGetList(fd->list,&len);
	    for ( i=0; i<len; ++i ) if ( i!=fd->index ) {
		val2 = u_strtol(ti[i]->text,NULL,10);
		if ( val2==val1 ) {
		    GWidgetErrorR(_STR_ThisFeatureCodeIsAlreadyUsed,_STR_ThisFeatureCodeIsAlreadyUsed);
return( true );
		}
	    }
	    ret1 = _GGadgetGetTitle(fd->name);
	    if ( *ret1=='\0' ) {
		GWidgetErrorR(_STR_BadName,_STR_BadName);
return( true );
	    }
	    sprintf(buf,"%3d", val1);
	    len = u_strlen(ret1);
	    fd->ret = galloc( strlen(buf)+len+3 );
	    strcpy(fd->ret,buf);
	    strcat(fd->ret," ");
	    i = strlen(fd->ret);
	    u2encoding_strncpy(fd->ret+i,ret1,len+1,e_mac);
	    fd->done = true;
	}
    }
return( true );
}

static char *AskFeature(struct macfeaturenames *temp,GGadget *list, int index) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct featdata fd;
    char buf[20];
    unichar_t *ubuf;
    int len;

    memset(&fd,0,sizeof(fd));
    fd.list = list;
    fd.index = index;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Feature,NULL);
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
    pos.height = GDrawPointsToPixels(NULL,92);
    gw = GDrawCreateTopWindow(NULL,&pos,feat_e_h,&fd,&wattrs);
    fd.gw = gw;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    label[0].text = (unichar_t *) _STR_Code;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+4;
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( buf, "%d", temp->mac_feature_type );
    label[1].text = (unichar_t *) buf;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 40; gcd[1].gd.pos.y = 5;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _STR_Name;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[0].gd.pos.y+24;
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    ubuf = NULL;
    if ( temp->feature_name ) {
	len = strlen( temp->feature_name );
	ubuf = galloc((len+1)*sizeof(unichar_t));
	encoding2u_strncpy(ubuf,temp->feature_name,len+1,e_mac);
	label[3].text = ubuf;
	gcd[3].gd.label = &label[3];
    }
    gcd[3].gd.pos.x = 40; gcd[3].gd.pos.y = gcd[2].gd.pos.y-4;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = 13-3; gcd[4].gd.pos.y = gcd[3].gd.pos.y+30;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _STR_OK;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    /*gcd[4].gd.handle_controlevent = Prefs_Ok;*/
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = -13; gcd[5].gd.pos.y = gcd[5-1].gd.pos.y+3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _STR_Cancel;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    /*gcd[5].gd.handle_controlevent = Prefs_Cancel;*/
    gcd[5].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    fd.code = gcd[1].ret;
    fd.name = gcd[3].ret;
    fd.ok = gcd[4].ret;
    fd.cancel = gcd[5].ret;
    GDrawSetVisible(gw,true);
    GWidgetIndicateFocusGadget(gcd[1].ret);
    while ( !fd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    free(temp->feature_name ); free(ubuf);
return( fd.ret );
}

static void ChangeFeature(GGadget *list,int index) {
    struct macfeaturenames temp;
    int32 len, l;
    GTextInfo **ti = GGadgetGetList(list,&len);
    unichar_t *end;
    char *str;

    temp.mac_feature_type = u_strtol(ti[index]->text,&end,10);
    l = u_strlen(end);
    temp.feature_name = galloc(l);
    u2encoding_strncpy(temp.feature_name,end+1,l,e_mac);
    if ( (str=AskFeature(&temp,list,index))==NULL )
return;
    GListReplaceStr(list,index,str);
}

static int Pref_NewFeat(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	int32 len;
	GGadget *list = GWidgetGetControl(gw,CID_Features);
	GTextInfo **ti = GGadgetGetList(list,&len);
	struct macfeaturenames temp;
	char *str;
	int i, expected=0, val;
	for ( i=0; i<len; ++i ) {
	    val = u_strtol(ti[i]->text,NULL,10);
	    if ( val!=expected )
	break;
	    ++expected;
	}
	temp.mac_feature_type = expected;
	temp.feature_name = copy("");
	if ( (str=AskFeature(&temp,list,-1))==NULL )
return( true );
	GListAddStr(list,str);
    }
return( true );
}

static int Pref_DelFeat(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	GListDelSelected(GWidgetGetControl(gw,CID_Features));
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_FeatureDel),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_FeatureEdit),false);
    }
return( true );
}

static int Pref_EditFeat(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GGadget *list = GWidgetGetControl(GGadgetGetWindow(g),CID_Features);
	ChangeFeature(list,GGadgetGetFirstListSelectedItem(list));
    }
return( true );
}

static int Pref_FeatureSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	GWindow gw = GGadgetGetWindow(g);
	int i, sel_cnt=0;
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected ) ++sel_cnt;
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_FeatureDel),sel_cnt!=0);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_FeatureEdit),sel_cnt==1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	ChangeFeature(g,e->u.control.u.list.changed_index!=-1?e->u.control.u.list.changed_index:
		GGadgetGetFirstListSelectedItem(g));
    }
return( true );
}

static int Prefs_Ok(GGadget *g, GEvent *e) {
    int i, j, mi;
    int err=0, enc;
    struct pref_data *p;
    GWindow gw;
    const unichar_t *ret;
    int lc=-1;
    GTextInfo *ti;
    const unichar_t *names[SCRIPT_MENU_MAX], *scripts[SCRIPT_MENU_MAX];
    struct prefs_list *pl;
    GTextInfo **list;
    int len, maxl, t;
    unichar_t *end;
    char *str;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	p = GDrawGetUserData(gw);
	for ( i=0; i<SCRIPT_MENU_MAX; ++i ) {
	    names[i] = _GGadgetGetTitle(GWidgetGetControl(gw,5000+i));
	    scripts[i] = _GGadgetGetTitle(GWidgetGetControl(gw,5100+i));
	    if ( *names[i]=='\0' ) names[i] = NULL;
	    if ( *scripts[i]=='\0' ) scripts[i] = NULL;
	    if ( scripts[i]==NULL && names[i]!=NULL ) {
		GWidgetErrorR(_STR_MenuNameWithNoScript,_STR_MenuNameWithNoScript);
return( true );
	    } else if ( scripts[i]!=NULL && names[i]==NULL ) {
		GWidgetErrorR(_STR_ScriptWithNoMenuName,_STR_ScriptWithNoMenuName);
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
		GetInt(gw,j*1000+1000+i,pl->name,&err);
	    } else if ( pl->type==pr_int ) {
		GetReal(gw,j*1000+1000+i,pl->name,&err);
	    } else if ( pl->val == &prefs_encoding ) {
		enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,j*1000+1000+i));
		lc = (int) (localencodingtypes[enc].userdata);
		if ( lc!=e_unknown )
		    prefs_encoding = lc;
		else if ( prefs_encoding!=e_unknown ) {
		    lc = DefaultEncoding();
		    prefs_encoding = e_unknown;
		} else
		    lc = local_encoding;
	    }
	}
	if ( err )
return( true );

	if ( lc!=-1 ) {
	    local_encoding = lc;		/* must be done early, else strings don't convert */
	}

	for ( j=0; visible_prefs_list[j].tab_name!=0; ++j ) for ( i=0; visible_prefs_list[j].pl[i].name!=NULL; ++i ) {
	    pl = &visible_prefs_list[j].pl[i];
	    if ( pl->dontdisplay )
	continue;
	    switch( pl->type ) {
	      case pr_int:
	        *((int *) (pl->val)) = GetInt(gw,j*1000+1000+i,pl->name,&err);
	      break;
	      case pr_bool:
	        *((int *) (pl->val)) = GGadgetIsChecked(GWidgetGetControl(gw,j*1000+1000+i));
	      break;
	      case pr_real:
	        *((float *) (pl->val)) = GetReal(gw,j*1000+1000+i,pl->name,&err);
	      break;
	      case pr_encoding:
		if ( pl->val==&prefs_encoding )
	continue;
		enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,j*1000+1000+i));
		ti = GGadgetGetListItem(GWidgetGetControl(gw,j*1000+1000+i),enc);
		*((int *) (pl->val)) = (int) (ti->userdata);
	      break;
	      case pr_string: case pr_file:
	        ret = _GGadgetGetTitle(GWidgetGetControl(gw,j*1000+1000+i));
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

	list = GGadgetGetList(GWidgetGetControl(gw,CID_Features),&len);
	UserFeaturesFree();
	user_feature_type_to_name = galloc((len+1)*sizeof(struct macfeaturenames));
	user_feature_type_to_name[len].feature_name = NULL;
	user_feature_type_to_name[len].mac_feature_type = -1;
	for ( i=0; i<len; ++i ) {
	    user_feature_type_to_name[i].mac_feature_type = u_strtol(list[i]->text,&end,10);
	    while ( *end==' ' ) ++end;
	    user_feature_type_to_name[i].feature_name = galloc(u_strlen(end)+2);
	    u2encoding_strncpy(user_feature_type_to_name[i].feature_name,end,u_strlen(end)+1,
		e_mac);
	}

	list = GGadgetGetList(GWidgetGetControl(gw,CID_Settings),&len);
	UserSettingsFree();
	user_macfeat_otftag = galloc((len+1)*sizeof(struct macsettingname));
	user_macfeat_otftag[len].on_name = NULL;
	maxl = 0;
	for ( i=0; i<len; ++i ) {
	    t = u_strlen(list[i]->text);
	    if ( t>maxl ) maxl = t;
	}
	str = galloc(maxl+3);
	for ( i=0; i<len; ++i ) {
	    u2encoding_strncpy(str,list[i]->text,maxl+1,e_mac);
	    ParseMacSetting(str,&user_macfeat_otftag[i]);
	}
	free(str);

	p->done = true;
	SavePrefs();
	if ( maxundoes==0 ) { FontView *fv;
	    for ( fv=fv_list ; fv!=NULL; fv=fv->next )
		SFRemoveUndoes(fv->sf,NULL);
	}
    }
return( true );
}

static int Prefs_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct pref_data *p = GDrawGetUserData(GGadgetGetWindow(g));
	p->done = true;
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct pref_data *p = GDrawGetUserData(gw);
	p->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("prefs.html");
return( true );
	}
return( false );
    }
return( true );
}

void DoPrefs(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData *pgcd, gcd[5], sgcd[45], mgcd[3], mfgcd[8], msgcd[8];
    GTextInfo *plabel, **list, label[5], slabel[45], *plabels[10], mflabels[8], mslabels[8];
    GTabInfo aspects[9], subaspects[3];
    struct pref_data p;
    int i, gc, sgc, j, k, line, line_max, llen, y, y2, ii;
    char buf[20];
    int gcnt[20];
    static unichar_t nullstr[] = { 0 };
    struct prefs_list *pl;
    char *tempstr;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    FontRequest rq;
    GFont *font;

    MfArgsInit();
    for ( k=line_max=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	for ( i=line=gcnt[k]=0; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	    if ( visible_prefs_list[k].pl[i].dontdisplay )
	continue;
	    gcnt[k] += 2;
	    if ( visible_prefs_list[k].pl[i].type==pr_bool ) ++gcnt[k];
	    else if ( visible_prefs_list[k].pl[i].type==pr_file ) ++gcnt[k];
	    ++line;
	}
	if ( visible_prefs_list[k].pl == args_list ) {
	    gcnt[k] += 6;
	    line += 6;
	}
	if ( line>line_max ) line_max = line;
    }

    memset(&p,'\0',sizeof(p));
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Prefs,NULL);
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,290));
    pos.height = GDrawPointsToPixels(NULL,line_max*26+69);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);

    memset(sgcd,0,sizeof(sgcd));
    memset(slabel,0,sizeof(slabel));
    memset(&mfgcd,0,sizeof(mfgcd));
    memset(&msgcd,0,sizeof(msgcd));
    memset(&mflabels,0,sizeof(mflabels));
    memset(&mslabels,0,sizeof(mslabels));

    sgc = 0;

    mfgcd[sgc].gd.pos.x = 6; mfgcd[sgc].gd.pos.y = 6;
    mfgcd[sgc].gd.pos.width = 250; mfgcd[sgc].gd.pos.height = 16*12+10;
    mfgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic | gg_list_multiplesel;
    mfgcd[sgc].gd.cid = CID_Features;
    mfgcd[sgc].gd.u.list = Pref_FeaturesList();
    mfgcd[sgc].gd.handle_controlevent = Pref_FeatureSel;
    mfgcd[sgc++].creator = GListCreate;

    mfgcd[sgc].gd.pos.x = 6; mfgcd[sgc].gd.pos.y = mfgcd[sgc-1].gd.pos.y+mfgcd[sgc-1].gd.pos.height+10;
    mfgcd[sgc].gd.flags = gg_visible | gg_enabled;
    mflabels[sgc].text = (unichar_t *) _STR_NewDDD;
    mflabels[sgc].text_in_resource = true;
    mfgcd[sgc].gd.label = &mflabels[sgc];
    /*mfgcd[sgc].gd.cid = CID_AnchorRename;*/
    mfgcd[sgc].gd.handle_controlevent = Pref_NewFeat;
    mfgcd[sgc++].creator = GButtonCreate;

    mfgcd[sgc].gd.pos.x = mfgcd[sgc-1].gd.pos.x+20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    mfgcd[sgc].gd.pos.y = mfgcd[sgc-1].gd.pos.y;
    mfgcd[sgc].gd.flags = gg_visible ;
    mflabels[sgc].text = (unichar_t *) _STR_Delete;
    mflabels[sgc].text_in_resource = true;
    mfgcd[sgc].gd.label = &mflabels[sgc];
    mfgcd[sgc].gd.cid = CID_FeatureDel;
    mfgcd[sgc].gd.handle_controlevent = Pref_DelFeat;
    mfgcd[sgc++].creator = GButtonCreate;

    mfgcd[sgc].gd.pos.x = mfgcd[sgc-1].gd.pos.x+20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    mfgcd[sgc].gd.pos.y = mfgcd[sgc-1].gd.pos.y;
    mfgcd[sgc].gd.flags = gg_visible ;
    mflabels[sgc].text = (unichar_t *) _STR_EditDDD;
    mflabels[sgc].text_in_resource = true;
    mfgcd[sgc].gd.label = &mflabels[sgc];
    mfgcd[sgc].gd.cid = CID_FeatureEdit;
    mfgcd[sgc].gd.handle_controlevent = Pref_EditFeat;
    mfgcd[sgc++].creator = GButtonCreate;

    sgc = 0;

    msgcd[sgc].gd.pos.x = 6; msgcd[sgc].gd.pos.y = 6;
    msgcd[sgc].gd.pos.width = 250; msgcd[sgc].gd.pos.height = 16*12+10;
    msgcd[sgc].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic | gg_list_multiplesel;
    msgcd[sgc].gd.cid = CID_Settings;
    msgcd[sgc].gd.u.list = Pref_SettingsList();
    msgcd[sgc].gd.handle_controlevent = Pref_SettingSel;
    msgcd[sgc++].creator = GListCreate;

    msgcd[sgc].gd.pos.x = 6; msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y+msgcd[sgc-1].gd.pos.height+10;
    msgcd[sgc].gd.flags = gg_visible | gg_enabled;
    mslabels[sgc].text = (unichar_t *) _STR_NewDDD;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    /*msgcd[sgc].gd.cid = CID_AnchorRename;*/
    msgcd[sgc].gd.handle_controlevent = Pref_NewSet;
    msgcd[sgc++].creator = GButtonCreate;

    msgcd[sgc].gd.pos.x = msgcd[sgc-1].gd.pos.x+20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y;
    msgcd[sgc].gd.flags = gg_visible ;
    mslabels[sgc].text = (unichar_t *) _STR_Delete;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.cid = CID_SettingDel;
    msgcd[sgc].gd.handle_controlevent = Pref_DelSet;
    msgcd[sgc++].creator = GButtonCreate;

    msgcd[sgc].gd.pos.x = msgcd[sgc-1].gd.pos.x+20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    msgcd[sgc].gd.pos.y = msgcd[sgc-1].gd.pos.y;
    msgcd[sgc].gd.flags = gg_visible ;
    mslabels[sgc].text = (unichar_t *) _STR_EditDDD;
    mslabels[sgc].text_in_resource = true;
    msgcd[sgc].gd.label = &mslabels[sgc];
    msgcd[sgc].gd.cid = CID_SettingEdit;
    msgcd[sgc].gd.handle_controlevent = Pref_EditSet;
    msgcd[sgc++].creator = GButtonCreate;

    sgc = 0;
    y2=5;

    slabel[sgc].text = (unichar_t *) _STR_MenuName;
    slabel[sgc].text_in_resource = true;
    sgcd[sgc].gd.label = &slabel[sgc];
    sgcd[sgc].gd.popup_msg = GStringGetResource(_STR_ScriptMenuPopup,NULL);
    sgcd[sgc].gd.pos.x = 8;
    sgcd[sgc].gd.pos.y = y2;
    sgcd[sgc].gd.flags = gg_visible | gg_enabled;
    sgcd[sgc++].creator = GLabelCreate;

    slabel[sgc].text = (unichar_t *) _STR_ScriptFile;
    slabel[sgc].text_in_resource = true;
    sgcd[sgc].gd.label = &slabel[sgc];
    sgcd[sgc].gd.popup_msg = GStringGetResource(_STR_ScriptMenuPopup,NULL);
    sgcd[sgc].gd.pos.x = 110;
    sgcd[sgc].gd.pos.y = y2;
    sgcd[sgc].gd.flags = gg_visible | gg_enabled;
    sgcd[sgc++].creator = GLabelCreate;

    y2 += 14;

    for ( i=0; i<SCRIPT_MENU_MAX; ++i ) {
	sgcd[sgc].gd.pos.x = 8; sgcd[sgc].gd.pos.y = y2;
	sgcd[sgc].gd.flags = gg_visible | gg_enabled;
	slabel[sgc].text = script_menu_names[i]==NULL?nullstr:script_menu_names[i];
	sgcd[sgc].gd.label = &slabel[sgc];
	sgcd[sgc].gd.cid = i+5000;
	sgcd[sgc++].creator = GTextFieldCreate;

	sgcd[sgc].gd.pos.x = 110; sgcd[sgc].gd.pos.y = y2;
	sgcd[sgc].gd.flags = gg_visible | gg_enabled;
	slabel[sgc].text = (unichar_t *) (script_filenames[i]==NULL?"":script_filenames[i]);
	slabel[sgc].text_is_1byte = true;
	sgcd[sgc].gd.label = &slabel[sgc];
	sgcd[sgc].gd.cid = i+5100;
	sgcd[sgc++].creator = GTextFieldCreate;

	sgcd[sgc].gd.pos.x = 210; sgcd[sgc].gd.pos.y = y2;
	sgcd[sgc].gd.flags = gg_visible | gg_enabled;
	slabel[sgc].text = (unichar_t *) _STR_BrowseForFile;
	slabel[sgc].text_in_resource = true;
	sgcd[sgc].gd.label = &slabel[sgc];
	sgcd[sgc].gd.cid = i+5200;
	sgcd[sgc].gd.handle_controlevent = Prefs_ScriptBrowse;
	sgcd[sgc++].creator = GButtonCreate;

	y2 += 26;
    }

    memset(&mgcd,0,sizeof(mgcd));
    memset(&subaspects,'\0',sizeof(subaspects));
    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&aspects,'\0',sizeof(aspects));
    aspects[0].selected = true;

    for ( k=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	pgcd = gcalloc(gcnt[k]+4,sizeof(GGadgetCreateData));
	plabel = gcalloc(gcnt[k]+4,sizeof(GTextInfo));

	aspects[k].text = (unichar_t *) visible_prefs_list[k].tab_name;
	aspects[k].text_in_resource = true;
	aspects[k].gcd = pgcd;
	plabels[k] = plabel;

	gc = 0;
	for ( i=line=0, y=5; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	    pl = &visible_prefs_list[k].pl[i];
	    if ( pl->dontdisplay )
	continue;
	    plabel[gc].text = (unichar_t *) pl->name;
	    plabel[gc].text_is_1byte = true;
	    pgcd[gc].gd.label = &plabel[gc];
	    pgcd[gc].gd.mnemonic = pl->mn;
	    pgcd[gc].gd.popup_msg = GStringGetResource(pl->popup,NULL);
	    pgcd[gc].gd.pos.x = 8;
	    pgcd[gc].gd.pos.y = y + 6;
	    pgcd[gc].gd.flags = gg_visible | gg_enabled;
	    pgcd[gc++].creator = GLabelCreate;

	    plabel[gc].text_is_1byte = true;
	    pgcd[gc].gd.label = &plabel[gc];
	    pgcd[gc].gd.mnemonic = pl->mn;
	    pgcd[gc].gd.popup_msg = GStringGetResource(pl->popup,NULL);
	    pgcd[gc].gd.pos.x = 110;
	    pgcd[gc].gd.pos.y = y;
	    pgcd[gc].gd.flags = gg_visible | gg_enabled;
	    pgcd[gc].gd.cid = k*1000+1000+i;
	    switch ( pl->type ) {
	      case pr_bool:
		plabel[gc].text = (unichar_t *) "On";
		pgcd[gc].gd.pos.y += 3;
		pgcd[gc++].creator = GRadioCreate;
		pgcd[gc] = pgcd[gc-1];
		pgcd[gc].gd.pos.x += 50;
		pgcd[gc].gd.cid = 0;
		pgcd[gc].gd.label = &plabel[gc];
		plabel[gc].text = (unichar_t *) "Off";
		plabel[gc].text_is_1byte = true;
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
		y += 26;
	      break;
	      case pr_real:
		sprintf(buf,"%g", *((float *) pl->val));
		plabel[gc].text = (unichar_t *) copy( buf );
		pgcd[gc++].creator = GTextFieldCreate;
		y += 26;
	      break;
	      case pr_encoding:
		if ( pl->val==&prefs_encoding ) {
		    pgcd[gc].gd.u.list = localencodingtypes;
		    pgcd[gc].gd.label = EncodingTypesFindEnc(localencodingtypes,
			    *(int *) pl->val);
		} else {
		    pgcd[gc].gd.u.list = GetEncodingTypes();
		    pgcd[gc].gd.label = EncodingTypesFindEnc(pgcd[gc].gd.u.list,
			    *(int *) pl->val);
		}
		for ( ii=0; pgcd[gc].gd.u.list[ii].text!=NULL ||pgcd[gc].gd.u.list[ii].line; ++ii )
		    if ( pgcd[gc].gd.u.list[ii].userdata==(void *) em_unicodeplanes )
			pgcd[gc].gd.u.list[ii].disabled = true;
		pgcd[gc].gd.pos.width = 160;
		if ( pgcd[gc].gd.label==NULL ) pgcd[gc].gd.label = &encodingtypes[0];
		pgcd[gc++].creator = GListButtonCreate;
		y += 28;
	      break;
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
		    plabel[gc].text = /* def2u_*/ uc_copy( "PfaEdit" );
		else
		    plabel[gc].text = /* def2u_*/ uc_copy( "" );
		plabel[gc].text_is_1byte = false;
		pgcd[gc++].creator = GTextFieldCreate;
		if ( pl->type==pr_file ) {
		    pgcd[gc] = pgcd[gc-1];
		    pgcd[gc-1].gd.pos.width = 140;
		    pgcd[gc].gd.pos.x += 145;
		    pgcd[gc].gd.cid += 20000;
		    pgcd[gc].gd.label = &plabel[gc];
		    plabel[gc].text = (unichar_t *) "...";
		    plabel[gc].text_is_1byte = true;
		    pgcd[gc].gd.handle_controlevent = Prefs_BrowseFile;
		    pgcd[gc++].creator = GButtonCreate;
		}
		y += 26;
		if ( pl->val==NULL )
		    free(tempstr);
	      break;
	    }
	    ++line;
	}
	if ( visible_prefs_list[k].pl == args_list ) {
	    static int text[] = { _STR_PrefsAppNotice1, _STR_PrefsAppNotice2,
				_STR_PrefsAppNotice3, _STR_PrefsAppNotice4,
			        _STR_PrefsAppNotice5, _STR_PrefsAppNotice6, 0 };
	    y += 8;
	    for ( i=0; text[i]!=0; ++i ) {
		plabel[gc].text = (unichar_t *) text[i];
		plabel[gc].text_in_resource = true;
		pgcd[gc].gd.label = &plabel[gc];
		pgcd[gc].gd.pos.x = 8;
		pgcd[gc].gd.pos.y = y;
		pgcd[gc].gd.flags = gg_visible | gg_enabled;
		pgcd[gc++].creator = GLabelCreate;
		y += 12;
	    }
	}
	if ( y>y2 ) y2 = y;
    }

    aspects[k].text = (unichar_t *) _STR_ScriptMenu;
    aspects[k].text_in_resource = true;
    aspects[k++].gcd = sgcd;

    subaspects[0].text = (unichar_t *) _STR_Features;
    subaspects[0].text_in_resource = true;
    subaspects[0].gcd = mfgcd;

    subaspects[1].text = (unichar_t *) _STR_Settings;
    subaspects[1].text_in_resource = true;
    subaspects[1].gcd = msgcd;

    mgcd[0].gd.pos.x = 4; gcd[0].gd.pos.y = 6;
    mgcd[0].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-20;
    mgcd[0].gd.pos.height = y2;
    mgcd[0].gd.u.tabs = subaspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].creator = GTabSetCreate;

    aspects[k].text = (unichar_t *) _STR_Mac;
    aspects[k].text_in_resource = true;
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
    gcd[gc].gd.flags = gg_visible | gg_enabled;
    gcd[gc++].creator = GTabSetCreate;

    y = gcd[gc-1].gd.pos.y+gcd[gc-1].gd.pos.height;

    gcd[gc].gd.pos.x = 30-3; gcd[gc].gd.pos.y = y+5-3;
    gcd[gc].gd.pos.width = -1; gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[gc].text = (unichar_t *) _STR_OK;
    label[gc].text_in_resource = true;
    gcd[gc].gd.mnemonic = 'O';
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.handle_controlevent = Prefs_Ok;
    gcd[gc++].creator = GButtonCreate;

    gcd[gc].gd.pos.x = -30; gcd[gc].gd.pos.y = gcd[gc-1].gd.pos.y+3;
    gcd[gc].gd.pos.width = -1; gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[gc].text = (unichar_t *) _STR_Cancel;
    label[gc].text_in_resource = true;
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.mnemonic = 'C';
    gcd[gc].gd.handle_controlevent = Prefs_Cancel;
    gcd[gc++].creator = GButtonCreate;

    y = GDrawPointsToPixels(NULL,y+37);
    gcd[0].gd.pos.height = y-4;

    GGadgetsCreate(gw,gcd);
    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = 12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GGadgetSetFont(mfgcd[0].ret,font);
    GGadgetSetFont(msgcd[0].ret,font);
    GTextInfoListFree(mfgcd[0].gd.u.list);
    GTextInfoListFree(msgcd[0].gd.u.list);
    if ( y!=pos.height )
	GDrawResize(gw,pos.width,y );

    for ( k=0; visible_prefs_list[k].tab_name!=0; ++k ) for ( gc=0,i=0; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	pl = &visible_prefs_list[k].pl[i];
	if ( pl->dontdisplay )
    continue;
	switch ( pl->type ) {
	  case pr_bool:
	    ++gc;
	  break;
	  case pr_encoding: {
	    GGadget *g = aspects[k].gcd[gc+1].ret;
	    list = GGadgetGetList(g,&llen);
	    for ( j=0; j<llen ; ++j ) {
		if ( list[j]->text!=NULL &&
			(void *) ( *((int *) pl->val)) == list[j]->userdata )
		    list[j]->selected = true;
		else
		    list[j]->selected = false;
	    }
	    if ( aspects[k].gcd[gc+1].gd.u.list!=encodingtypes && aspects[k].gcd[gc+1].gd.u.list!=localencodingtypes )
		GTextInfoListFree(aspects[k].gcd[gc+1].gd.u.list);
	  } break;
	  case pr_string: case pr_file: case pr_int: case pr_real:
	    free(plabels[k][gc+1].text);
	    if ( pl->type==pr_file )
		++gc;
	  break;
	}
	gc += 2;
    }

    for ( k=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	free(aspects[k].gcd);
	free(plabels[k]);
    }

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !p.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GreekHack();
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
    SavePrefs();
}

char *FeatureNameFromType(int featureType) {
    int i;
    struct macfeaturenames *mfn = user_feature_type_to_name ? user_feature_type_to_name : feature_type_to_name;

    for ( i=0; mfn[i].feature_name!=NULL; ++i )
	if ( mfn[i].mac_feature_type==featureType )
return( mfn[i].feature_name );

return( "Unknown Feature" );
}
