/* Copyright (C) 2001-2002 by George Williams */
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
#include "charset.h"
#include "gfile.h"
#include "gresource.h"
#include "ustring.h"
#include <gkeysym.h>

#include <sys/types.h>
#include <dirent.h>
#include <locale.h>

int adjustwidth = true;
int adjustlbearing = true;
int default_encoding = em_iso8859_1;
int autohint_before_rasterize = 1;
int ItalicConstrained=true;
int accent_offset = 6;
int GraveAcuteCenterBottom = 1;
int ask_user_for_resolution = true;
float arrowAmount=1;
float snapdistance=3.5;
char *BDFFoundry=NULL;
char *TTFFoundry=NULL;
char *xuid=NULL;
char *RecentFiles[RECENT_MAX] = { NULL };
/*struct cvshows CVShows = { 1, 1, 1, 1, 1, 0, 1 };*/ /* in charview */
/* int default_fv_font_size = 24; */	/* in fontview */
/* int default_fv_antialias = false */	/* in fontview */
extern int default_fv_showhmetrics;	/* in fontview */
extern int default_fv_showvmetrics;	/* in fontview */
extern int palettes_docked;		/* in cvpalettes */
/* int local_encoding; */		/* in gresource.c *//* not a charset */
static int prefs_encoding = e_unknown;
int greekfixup = true;
extern int onlycopydisplayed, copymetadata;
extern struct cvshows CVShows;
extern int oldformatstate;		/* in savefontdlg.c */
extern int oldbitmapstate;		/* in savefontdlg.c */
extern int oldpsstate;			/* in savefontdlg.c */
extern int oldttfhintstate;		/* in savefontdlg.c */
/*extern int oldttfapplestate;		/* in savefontdlg.c */
extern int oldsystem;			/* in bitmapdlg.c */
extern int autotrace_ask;		/* in autotrace.c */
extern int mf_ask;			/* in autotrace.c */
extern int mf_clearbackgrounds;		/* in autotrace.c */
extern int mf_showerrors;		/* in autotrace.c */
extern char *mf_args;			/* in autotrace.c */
extern int glyph_2_name_map;		/* in tottf.c */
unichar_t *script_menu_names[SCRIPT_MENU_MAX];
char *script_filenames[SCRIPT_MENU_MAX];

static int pointless;

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
    { (unichar_t *) _STR_Mac, NULL, 0, 0, (void *) e_mac, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Win, NULL, 0, 0, (void *) e_win, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Unicode, NULL, 0, 0, (void *) e_unicode, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UTF_8, NULL, 0, 0, (void *) e_utf8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_SJIS, NULL, 0, 0, (void *) e_sjis, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_KoreanWansung, NULL, 0, 0, (void *) e_wansung, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_KoreanJohab, NULL, 0, 0, (void *) e_johab, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ChineseTrad, NULL, 0, 0, (void *) e_big5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

/* don't use mnemonics 'C' or 'O' (Cancel & OK) */
enum pref_types { pr_int, pr_real, pr_bool, pr_enum, pr_encoding, pr_string };
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
	{ "AutoWidthSync", pr_bool, &adjustwidth, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupAWS },
	{ "AutoLBearingSync", pr_bool, &adjustlbearing, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupALS },
	{ "AutoHint", pr_bool, &autohint_before_rasterize, NULL, NULL, 'A', NULL, 0, _STR_PrefsPopupAH },
	{ "LocalEncoding", pr_encoding, &prefs_encoding, NULL, NULL, 'L', NULL, 0, _STR_PrefsPopupLoc },
	{ "NewCharset", pr_encoding, &default_encoding, NULL, NULL, 'N', NULL, 0, _STR_PrefsPopupForNewFonts },
	{ "ShowRulers", pr_bool, &CVShows.showrulers, NULL, NULL, '\0', NULL, 1, _STR_PrefsPopupRulers },
	{ "ItalicConstrained", pr_bool, &ItalicConstrained, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupIC },
	{ "AccentOffsetPercent", pr_int, &accent_offset, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupAO },
	{ "AcuteCenterBottom", pr_bool, &GraveAcuteCenterBottom, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupGA },
	{ "GreekFixup", pr_bool, &greekfixup, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupGF },
	{ "ArrowMoveSize", pr_real, &arrowAmount, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupAA },
	{ "SnapDistance", pr_real, &snapdistance, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupSD },
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
	{ "FoundryName", pr_string, &BDFFoundry, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupFN },
	{ "TTFFoundry", pr_string, &TTFFoundry, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupTFN },
	{ "XUID-Base", pr_string, &xuid, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupXU },
	{ "AskBDFResolution", pr_bool, &ask_user_for_resolution, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupBR },
	{ "DumpGlyphMap", pr_bool, &glyph_2_name_map, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupG2N },
	{ NULL }
},
 hidden_list[] = {
	{ "AntiAlias", pr_bool, &default_fv_antialias, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVShowHmetrics", pr_int, &default_fv_showhmetrics, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVShowVmetrics", pr_int, &default_fv_showvmetrics, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultFVSize", pr_enum, &default_fv_font_size, NULL, NULL, 'S', fvsize_enums, 1 },
	{ "OnlyCopyDisplayed", pr_bool, &onlycopydisplayed, NULL, NULL, '\0', NULL, 1 },
	{ "CopyMetaData", pr_bool, &copymetadata, NULL, NULL, '\0', NULL, 1 },
	{ "PalettesDocked", pr_bool, &palettes_docked, NULL, NULL, '\0', NULL, 1 },
	{ "MarkExtrema", pr_int, &CVShows.markextrema, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultScreenDpiSystem", pr_int, &oldsystem, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultOutputFormat", pr_int, &oldformatstate, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultBitmapFormat", pr_int, &oldbitmapstate, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultPSNameLength", pr_int, &oldpsstate, NULL, NULL, '\0', NULL, 1 },
	{ "DefaultTTFHints", pr_int, &oldttfhintstate, NULL, NULL, '\0', NULL, 1 },
	{ "PageWidth", pr_int, &pagewidth, NULL, NULL, '\0', NULL, 1 },
	{ "PageHeight", pr_int, &pageheight, NULL, NULL, '\0', NULL, 1 },
	{ "PrintType", pr_int, &printtype, NULL, NULL, '\0', NULL, 1 },
	{ "PrintCommand", pr_string, &printcommand, NULL, NULL, '\0', NULL, 1 },
	{ "PageLazyPrinter", pr_string, &printlazyprinter, NULL, NULL, '\0', NULL, 1 },
	{ NULL }
},
 oldnames[] = {
	{ "LocalCharset", pr_encoding, &prefs_encoding, NULL, NULL, 'L', NULL, 0, _STR_PrefsPopupLoc },
	{ "DefaultTTFApple", pr_int, &pointless, NULL, NULL, '\0', NULL, 1 },
	{ NULL }
},
 *prefs_list[] = { general_list, args_list, generate_list, hidden_list, NULL },
 *load_prefs_list[] = { general_list, args_list, generate_list, hidden_list, oldnames, NULL };

struct visible_prefs_list { int tab_name; struct prefs_list *pl; } visible_prefs_list[] = {
    { _STR_Generic, general_list},
    { _STR_PrefsApps, args_list},
    { _STR_PrefsFontInfo, generate_list},
    { 0 }
 };

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

static void CheckLang(void) {
    /*const char *loc = setlocale(LC_MESSAGES,NULL);*/ /* This always returns "C" for me, even when it shouldn't be */
    const char *loc = getenv("LC_ALL");
    char buffer[100], full[1024];

    if ( loc==NULL ) loc = getenv("LC_MESSAGES");
    if ( loc==NULL ) loc = getenv("LANG");

    if ( loc==NULL )
return;

    strcpy(buffer,"pfaedit.");
    strcat(buffer,loc);
    strcat(buffer,".ui");
    if ( !CheckLangDir(full,sizeof(full),GResourceProgramDir,loc) &&
#ifdef SHAREDIR
	    !CheckLangDir(full,sizeof(full),SHAREDIR,loc) &&
#endif
	    !CheckLangDir(full,sizeof(full),getPfaEditShareDir(),loc) &&
	    !CheckLangDir(full,sizeof(full),"/usr/share/pfaedit",loc) )
return;

    GStringSetResourceFile(full);
}

static void GreekHack(void) {
    int i;

    if ( greekfixup ) {
	psunicodenames[0x2206] = NULL;		/* Increment */
	psunicodenames[0x2126] = NULL;		/* Ohm sign */
	psunicodenames[0x0394] = "Delta";	/* Delta */
	psunicodenames[0x03A9] = "Omega";	/* Omega */

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
    } else {
	psunicodenames[0x2206] = "Delta";	/* Increment */
	psunicodenames[0x2126] = "Omega";	/* Ohm sign */
	psunicodenames[0x0394] = NULL;		/* Delta */
	psunicodenames[0x03A9] = NULL;		/* Omega */
	for ( i=0xf500; i<=0xf51a; ++i )
	    psunicodenames[i] = NULL;
    }
    /* I'm leaving mu at 00b5 (rather than 03bc) */
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

void LoadPrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    char line[1100];
    int i, j, ri=0, mn=0, ms=0;
    char *pt;
    struct prefs_list *pl;

    PfaEditSetFallback();
    LoadPfaEditEncodings();
    CheckLang();
    GreekHack();

    if ( prefs==NULL )
return;
    if ( (p=fopen(prefs,"r"))==NULL )
return;
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
	  case pr_string:
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
    if ( prefs_encoding==e_unknown )
	local_encoding = DefaultEncoding();
    else
	local_encoding = prefs_encoding;
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
	  case pr_string:
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
	      case pr_string:
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
	p->done = true;
	SavePrefs();
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
    GGadgetCreateData *pgcd, gcd[5], sgcd[45];
    GTextInfo *plabel, **list, label[5], slabel[45], *plabels[10];
    GTabInfo aspects[8];
    struct pref_data p;
    int i, gc, sgc, j, k, line, line_max, llen, y, y2, ii;
    char buf[20];
    int gcnt[20];
    static unichar_t nullstr[] = { 0 };
    struct prefs_list *pl;
    char *tempstr;

    MfArgsInit();
    for ( k=line_max=0; visible_prefs_list[k].tab_name!=0; ++k ) {
	for ( i=line=gcnt[k]=0; visible_prefs_list[k].pl[i].name!=NULL; ++i ) {
	    if ( visible_prefs_list[k].pl[i].dontdisplay )
	continue;
	    gcnt[k] += 2;
	    if ( visible_prefs_list[k].pl[i].type==pr_bool ) ++gcnt[k];
	    ++line;
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
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,268));
    pos.height = GDrawPointsToPixels(NULL,line_max*26+69);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);

    memset(sgcd,0,sizeof(sgcd));
    memset(slabel,0,sizeof(slabel));

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
		pgcd[gc].gd.pos.width = 145;
		if ( pgcd[gc].gd.label==NULL ) pgcd[gc].gd.label = &encodingtypes[0];
		pgcd[gc++].creator = GListButtonCreate;
		y += 28;
	      break;
	      case pr_string:
		if ( pl->val!=NULL )
		    tempstr = *((char **) pl->val);
		else
		    tempstr = (char *) ((pl->get)());
		if ( tempstr!=NULL )
		    plabel[gc].text = /* def2u_*/ uc_copy( *((char **) pl->val));
		else if ( ((char **) pl->val)==&BDFFoundry )
		    plabel[gc].text = /* def2u_*/ uc_copy( "PfaEdit" );
		else
		    plabel[gc].text = /* def2u_*/ uc_copy( "" );
		plabel[gc].text_is_1byte = false;
		pgcd[gc++].creator = GTextFieldCreate;
		y += 26;
		if ( pl->val==NULL )
		    free(tempstr);
	      break;
	    }
	    ++line;
	}
	if ( y>y2 ) y2 = y;
    }

    aspects[k].text = (unichar_t *) _STR_ScriptMenu;
    aspects[k].text_in_resource = true;
    aspects[k++].gcd = sgcd;

    gc = 0;

    gcd[gc].gd.pos.x = gcd[gc].gd.pos.y = 2;
    gcd[gc].gd.pos.width = pos.width-4; gcd[gc].gd.pos.height = pos.height-2;
    gcd[gc].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[gc++].creator = GGroupCreate;

    gcd[gc].gd.pos.x = 4; gcd[gc].gd.pos.y = 6;
    gcd[gc].gd.pos.width = 260;
    gcd[gc].gd.pos.height = y2+20+4;
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
	  case pr_string: case pr_int: case pr_real:
	    free(plabels[k][gc+1].text);
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

