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

#include <sys/types.h>
#include <dirent.h>
#include <locale.h>

int adjustwidth = true;
int adjustlbearing = true;
int default_encoding = em_iso8859_1;
int autohint_before_rasterize = 1;
int ItalicConstrained=true;
int accent_offset = 4;
char *BDFFoundry=NULL;
char *xuid=NULL;
char *RecentFiles[RECENT_MAX] = { NULL };
/*struct cvshows CVShows = { 1, 1, 1, 1, 1, 0, 1 };*/ /* in charview */
/* int default_fv_font_size = 24; */	/* in fontview */
/* int default_fv_antialias = false */	/* in fontview */
/* int local_encoding; */		/* in gresource.c *//* not a charset */
int greekfixup = true;
extern int onlycopydisplayed, copymetadata;

static GTextInfo localencodingtypes[] = {
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
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_SJIS, NULL, 0, 0, (void *) e_sjis, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_KoreanWansung, NULL, 0, 0, (void *) e_wansung, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_KoreanJohab, NULL, 0, 0, (void *) e_johab, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ChineseTrad, NULL, 0, 0, (void *) e_big5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

/* don't use mnemonics 'C' or 'O' (Cancel & OK) */
enum pref_types { pr_int, pr_bool, pr_enum, pr_encoding, pr_string };
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
} prefs_list[] = {
	{ "AutoWidthSync", pr_bool, &adjustwidth, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupAWS },
	{ "AutoLBearingSync", pr_bool, &adjustlbearing, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupALS },
	{ "DefaultFVSize", pr_enum, &default_fv_font_size, NULL, NULL, 'S', fvsize_enums, 1 },
	{ "AntiAlias", pr_bool, &default_fv_antialias, NULL, NULL, '\0', NULL, 1 },
	{ "AutoHint", pr_bool, &autohint_before_rasterize, NULL, NULL, 'A', NULL, 0, _STR_PrefsPopupAH },
	{ "LocalEncoding", pr_encoding, &local_encoding, NULL, NULL, 'L', NULL, 0, _STR_PrefsPopupLoc },
	{ "NewCharset", pr_encoding, &default_encoding, NULL, NULL, 'N', NULL, 0, _STR_PrefsPopupForNewFonts },
	{ "ShowRulers", pr_bool, &CVShows.showrulers, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupRulers },
	{ "FoundryName", pr_string, &BDFFoundry, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupFN },
	{ "XUID-Base", pr_string, &xuid, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupXU },
	{ "PageWidth", pr_int, &pagewidth, NULL, NULL, '\0', NULL, 1 },
	{ "PageHeight", pr_int, &pageheight, NULL, NULL, '\0', NULL, 1 },
	{ "PrintType", pr_int, &printtype, NULL, NULL, '\0', NULL, 1 },
	{ "ItalicConstrained", pr_bool, &ItalicConstrained, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupIC },
	{ "AccentOffsetPercent", pr_int, &accent_offset, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupAO },
	{ "AutotraceArgs", pr_string, NULL, GetAutoTraceArgs, SetAutoTraceArgs, '\0', NULL, 1 },
	{ "GreekFixup", pr_bool, &greekfixup, NULL, NULL, '\0', NULL, 0, _STR_PrefsPopupGF },
	{ "OnlyCopyDisplayed", pr_bool, &onlycopydisplayed, NULL, NULL, '\0', NULL, 1 },
	{ "CopyMetaData", pr_bool, &copymetadata, NULL, NULL, '\0', NULL, 1 },
	{ NULL }
},
 oldnames[] = {
	{ "LocalCharset", pr_encoding, &local_encoding, NULL, NULL, 'L', NULL, 0, _STR_PrefsPopupLoc },
	{ NULL }
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

    strcpy(buffer,"pfaedit.");
    strcat(buffer,loc);
    strcat(buffer,".ui");

    /*first look for full locale string (pfaedit.en_us.iso8859-1.ui) */
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

void LoadPrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    char line[1100];
    int i, ri=0;
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
	for ( i=0; prefs_list[i].name!=NULL; ++i )
	    if ( strncmp(line,prefs_list[i].name,pt-line)==0 )
	break;
	if ( prefs_list[i].name!=NULL )
	    pl = &prefs_list[i];
	else {
	    for ( i=0; oldnames[i].name!=NULL; ++i )
		if ( strncmp(line,oldnames[i].name,pt-line)==0 )
	    break;
	    pl = NULL;
	    if ( oldnames[i].name!=NULL )
		pl = &oldnames[i];
	}
	for ( ++pt; *pt=='\t'; ++pt );
	if ( line[strlen(line)-1]=='\n' )
	    line[strlen(line)-1] = '\0';
	if ( line[strlen(line)-1]=='\r' )
	    line[strlen(line)-1] = '\0';
	if ( pl==NULL ) {
	    if ( strncmp(line,"Recent:",strlen("Recent:"))==0 && ri<RECENT_MAX )
		RecentFiles[ri++] = copy(pt);
    continue;
	}
	switch ( pl->type ) {
	  case pr_encoding:
	    if ( sscanf( pt, "%d", pl->val )!=1 ) {
		Encoding *item;
		for ( item = enclist; item!=NULL && strcmp(item->enc_name,pt)!=0; item = item->next );
		if ( item==NULL )
		    *((int *) (pl->val)) = em_iso8859_1;
		else
		    *((int *) (pl->val)) = item->enc_num;
	    }
	  break;
	  case pr_bool: case pr_int:
	    sscanf( pt, "%d", pl->val );
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
}

void SavePrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    int i, val;
    char *temp;

    if ( prefs==NULL )
return;
    if ( (p=fopen(prefs,"w"))==NULL )
return;

    for ( i=0; prefs_list[i].name!=NULL; ++i ) {
	switch ( prefs_list[i].type ) {
	  case pr_encoding:
	    val = *(int *) (prefs_list[i].val);
	    if ( val<em_base )
		fprintf( p, "%s:\t%d\n", prefs_list[i].name, val );
	    else {
		Encoding *item;
		for ( item = enclist; item!=NULL && item->enc_num!=val; item=item->next );
		fprintf( p, "%s:\t%s\n", prefs_list[i].name, item==NULL?"-1":item->enc_name );
	    }
	  break;
	  case pr_bool: case pr_int:
	    fprintf( p, "%s:\t%d\n", prefs_list[i].name, *(int *) (prefs_list[i].val) );
	  break;
	  case pr_string:
	    if ( (prefs_list[i].val)!=NULL )
		temp = *(char **) (prefs_list[i].val);
	    else
		temp = (char *) (prefs_list[i].get());
	    if ( temp!=NULL )
		fprintf( p, "%s:\t%s\n", prefs_list[i].name, temp );
	    if ( (prefs_list[i].val)==NULL )
		free(temp);
	  break;
	}
    }

    for ( i=0; i<RECENT_MAX && RecentFiles[i]!=NULL; ++i )
	fprintf( p, "Recent:\t%s\n", RecentFiles[i]);

    fclose(p);
}

struct pref_data {
    int done;
};

static int Prefs_Ok(GGadget *g, GEvent *e) {
    int i;
    int err=0, enc;
    struct pref_data *p;
    GWindow gw;
    const unichar_t *ret;
    int lc=-1;
    GTextInfo *ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	p = GDrawGetUserData(gw);
	for ( i=0; prefs_list[i].name!=NULL; ++i ) {
	    /* before assigning values, check for any errors */
	    /* if any errors, then NO values should be assigned, in case they cancel */
	    if ( prefs_list[i].dontdisplay )
	continue;
	    if ( prefs_list[i].type==pr_int ) {
		GetInt(gw,1000+i,prefs_list[i].name,&err);
	    } else if ( prefs_list[i].val == &local_encoding ) {
		enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,1000+i));
		lc = (int) (localencodingtypes[enc].userdata);
	    }
	}
	if ( err )
return( true );

	if ( lc!=-1 ) {
	    local_encoding = lc;		/* must be done early, else strings don't convert */
	}

	for ( i=0; prefs_list[i].name!=NULL; ++i ) {
	    if ( prefs_list[i].dontdisplay )
	continue;
	    switch( prefs_list[i].type ) {
	      case pr_int:
	        *((int *) (prefs_list[i].val)) = GetInt(gw,1000+i,prefs_list[i].name,&err);
	      break;
	      case pr_bool:
	        *((int *) (prefs_list[i].val)) = GGadgetIsChecked(GWidgetGetControl(gw,1000+i));
	      break;
	      case pr_encoding:
		if ( prefs_list[i].val==&local_encoding )
	continue;
		enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,1000+i));
		ti = GGadgetGetListItem(GWidgetGetControl(gw,1000+i),enc);
		*((int *) (prefs_list[i].val)) = (int) (ti->userdata);
	      break;
	      case pr_string:
	        ret = _GGadgetGetTitle(GWidgetGetControl(gw,1000+i));
		free( *((char **) (prefs_list[i].val)) );
		*((char **) (prefs_list[i].val)) = NULL;
		if ( ret!=NULL && *ret!='\0' )
		    *((char **) (prefs_list[i].val)) = /* u2def_*/ cu_copy(ret);
	      break;
	    }
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
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void DoPrefs(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData *gcd;
    GTextInfo *label, **list;
    struct pref_data p;
    int i, gc, j, line, llen, y;
    char buf[20];

    for ( i=line=gc=0; prefs_list[i].name!=NULL; ++i ) {
	if ( prefs_list[i].dontdisplay )
    continue;
	gc += 2;
	if ( prefs_list[i].type==pr_bool ) ++gc;
	++line;
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
    pos.width =GDrawPointsToPixels(NULL,260);
    pos.height = GDrawPointsToPixels(NULL,line*26+45);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);

    gcd = gcalloc(gc+4,sizeof(GGadgetCreateData));
    label = gcalloc(gc+4,sizeof(GTextInfo));

    gc = 0;
    gcd[gc].gd.pos.x = gcd[gc].gd.pos.y = 2;
    gcd[gc].gd.pos.width = pos.width-4; gcd[gc].gd.pos.height = pos.height-2;
    gcd[gc].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[gc++].creator = GGroupCreate;

    for ( i=line=0, y=8; prefs_list[i].name!=NULL; ++i ) {
	if ( prefs_list[i].dontdisplay )
    continue;
	label[gc].text = (unichar_t *) prefs_list[i].name;
	label[gc].text_is_1byte = true;
	gcd[gc].gd.label = &label[gc];
	gcd[gc].gd.mnemonic = prefs_list[i].mn;
	gcd[gc].gd.popup_msg = GStringGetResource(prefs_list[i].popup,NULL);
	gcd[gc].gd.pos.x = 8;
	gcd[gc].gd.pos.y = y + 6;
	gcd[gc].gd.flags = gg_visible | gg_enabled;
	gcd[gc++].creator = GLabelCreate;

	label[gc].text_is_1byte = true;
	gcd[gc].gd.label = &label[gc];
	gcd[gc].gd.mnemonic = prefs_list[i].mn;
	gcd[gc].gd.popup_msg = GStringGetResource(prefs_list[i].popup,NULL);
	gcd[gc].gd.pos.x = 110;
	gcd[gc].gd.pos.y = y;
	gcd[gc].gd.flags = gg_visible | gg_enabled;
	gcd[gc].gd.cid = 1000+i;
	switch ( prefs_list[i].type ) {
	  case pr_bool:
	    label[gc].text = (unichar_t *) "On";
	    gcd[gc++].creator = GRadioCreate;
	    gcd[gc] = gcd[gc-1];
	    gcd[gc].gd.pos.x += 50;
	    gcd[gc].gd.cid = 0;
	    gcd[gc].gd.label = &label[gc];
	    label[gc].text = (unichar_t *) "Off";
	    label[gc].text_is_1byte = true;
	    if ( *((int *) prefs_list[i].val))
		gcd[gc-1].gd.flags |= gg_cb_on;
	    else
		gcd[gc].gd.flags |= gg_cb_on;
	    ++gc;
	    y += 22;
	  break;
	  case pr_int:
	    sprintf(buf,"%d", *((int *) prefs_list[i].val));
	    label[gc].text = (unichar_t *) copy( buf );
	    gcd[gc++].creator = GTextFieldCreate;
	    y += 26;
	  break;
	  case pr_encoding:
	    if ( prefs_list[i].val==&local_encoding ) {
		gcd[gc].gd.u.list = localencodingtypes;
		gcd[gc].gd.label = EncodingTypesFindEnc(localencodingtypes,
			*(int *) prefs_list[i].val);
	    } else {
		gcd[gc].gd.u.list = GetEncodingTypes();
		gcd[gc].gd.label = EncodingTypesFindEnc(gcd[gc].gd.u.list,
			*(int *) prefs_list[i].val);
	    }
	    gcd[gc].gd.pos.width = 145;
	    if ( gcd[gc].gd.label==NULL ) gcd[gc].gd.label = &encodingtypes[0];
	    gcd[gc++].creator = GListButtonCreate;
	    y += 28;
	  break;
	  case pr_string:
	    if ( *((char **) prefs_list[i].val)!=NULL )
		label[gc].text = /* def2u_*/ uc_copy( *((char **) prefs_list[i].val));
	    else if ( ((char **) prefs_list[i].val)==&BDFFoundry )
		label[gc].text = /* def2u_*/ uc_copy( "PfaEdit" );
	    else
		label[gc].text = /* def2u_*/ uc_copy( "" );
	    label[gc].text_is_1byte = false;
	    gcd[gc++].creator = GTextFieldCreate;
	    y += 26;
	  break;
	}
	++line;
    }

    gcd[gc].gd.pos.x = 30-3; gcd[gc].gd.pos.y = y+5-3;
    gcd[gc].gd.pos.width = -1; gcd[gc].gd.pos.height = 0;
    gcd[gc].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[gc].text = (unichar_t *) _STR_OK;
    label[gc].text_in_resource = true;
    gcd[gc].gd.mnemonic = 'O';
    gcd[gc].gd.label = &label[gc];
    gcd[gc].gd.handle_controlevent = Prefs_Ok;
    gcd[gc++].creator = GButtonCreate;

    gcd[gc].gd.pos.x = 250-GIntGetResource(_NUM_Buttonsize)-30; gcd[gc].gd.pos.y = gcd[gc-1].gd.pos.y+3;
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

    for ( gc=1,i=0; prefs_list[i].name!=NULL; ++i ) {
	if ( prefs_list[i].dontdisplay )
    continue;
	switch ( prefs_list[i].type ) {
	  case pr_bool:
	    ++gc;
	  break;
	  case pr_encoding: {
	    GGadget *g = gcd[gc+1].ret;
	    list = GGadgetGetList(g,&llen);
	    for ( j=0; j<llen ; ++j ) {
		if ( list[j]->text!=NULL &&
			(void *) ( *((int *) prefs_list[i].val)) == list[j]->userdata )
		    list[j]->selected = true;
		else
		    list[j]->selected = false;
	    }
	    if ( gcd[gc+1].gd.u.list!=encodingtypes && gcd[gc+1].gd.u.list!=localencodingtypes )
		GTextInfoListFree(gcd[gc+1].gd.u.list);
	  } break;
	  case pr_string: case pr_int:
	    free(label[gc+1].text);
	  break;
	}
	gc += 2;
    }

    free(gcd);
    free(label);

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

