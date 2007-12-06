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
#include "pfaedit.h"
#include "groups.h"
#include "plugins.h"
#include <charset.h>
#include <gfile.h>
#include <ustring.h>

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

static char *othersubrsfile = NULL;

extern int adjustwidth;
extern int adjustlbearing;
extern Encoding *default_encoding;
extern int autohint_before_rasterize;
extern int autohint_before_generate;
extern int accent_offset;
extern int GraveAcuteCenterBottom;
extern int PreferSpacingAccents;
extern int CharCenterHighest;
extern int recognizePUA;
extern int snaptoint;
extern float joinsnap;
extern char *BDFFoundry;
extern char *TTFFoundry;
extern char *xuid;
extern char *SaveTablesPref;
extern int maxundoes;			/* in cvundoes */
extern int prefer_cjk_encodings;	/* in parsettf */
extern int onlycopydisplayed, copymetadata, copyttfinstr;
extern int oldformatstate;		/* in savefontdlg.c */
extern int oldbitmapstate;		/* in savefontdlg.c */
extern int old_ttf_flags;		/* in savefontdlg.c */
extern int old_ps_flags;		/* in savefontdlg.c */
extern int old_otf_flags;		/* in savefontdlg.c */
extern int old_validate;		/* in savefontdlg.c */
extern int preferpotrace;		/* in autotrace.c */
extern int autotrace_ask;		/* in autotrace.c */
extern int mf_ask;			/* in autotrace.c */
extern int mf_clearbackgrounds;		/* in autotrace.c */
extern int mf_showerrors;		/* in autotrace.c */
extern char *mf_args;			/* in autotrace.c */
static int glyph_2_name_map=0;		/* was in tottf.c, now a flag in savefont options dlg */
extern int coverageformatsallowed;	/* in tottfgpos.c */
extern int hint_diagonal_ends;		/* in stemdb.c */
extern int hint_diagonal_intersections;	/* in stemdb.c */
extern int hint_bounding_boxes;		/* in stemdb.c */
extern int detect_diagonal_stems;	/* in stemdb.c */
extern int new_em_size;				/* in splineutil2.c */
extern int new_fonts_are_order2;		/* in splineutil2.c */
extern int loaded_fonts_same_as_new;		/* in splineutil2.c */
extern int use_second_indic_scripts;		/* in tottfgpos.c */
extern MacFeat *default_mac_feature_map,	/* from macenc.c */
		*user_mac_feature_map;
extern int allow_utf8_glyphnames;		/* in charinfo.c */
extern int clear_tt_instructions_when_needed;	/* in charview.c */
extern int ask_user_for_cmap;			/* in parsettf.c */
extern NameList *force_names_when_opening;
extern NameList *force_names_when_saving;
extern NameList *namelist_for_new_fonts;

enum pref_types { pr_int, pr_real, pr_bool, pr_enum, pr_encoding, pr_string,
	pr_file, pr_namelist };

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
} core_list[] = {
	{ N_("OtherSubrsFile"), pr_file, &othersubrsfile, NULL, NULL, 'O', NULL, 0, N_("If you wish to replace Adobe's OtherSubrs array (for Type1 fonts)\nwith an array of your own, set this to point to a file containing\na list of up to 14 PostScript subroutines. Each subroutine must\nbe preceded by a line starting with '%%%%' (any text before the\nfirst '%%%%' line will be treated as an initial copyright notice).\nThe first three subroutines are for flex hints, the next for hint\nsubstitution (this MUST be present), the 14th (or 13 as the\nnumbering actually starts with 0) is for counter hints.\nThe subroutines should not be enclosed in a [ ] pair.") },
	NULL
},
   extras[] = {
	{ N_("NewCharset"), pr_encoding, &default_encoding, NULL, NULL, 'N', NULL, 0, N_("Default encoding for\nnew fonts") },
},
 *prefs_list[] = { core_list, extras, NULL };

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

static int NOUI_GetPrefs(char *name,Val *val) {
    int i,j;
    
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

static int NOUI_SetPrefs(char *name,Val *val1, Val *val2) {
    int i,j;

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
    if ( getPfaEditDir(buffer)==NULL )
return( NULL );
    sprintf(buffer,"%s/prefs", getPfaEditDir(buffer));
    prefs = copy(buffer);
return( prefs );
}

static char *NOUI_getFontForgeShareDir(void) {
#if defined(SHAREDIR)
return( SHAREDIR "/fontforge" );
#elif defined(PREFIX)
return( PREFIX "/share/fontforge" );
#else
return( NULL );
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
#if 0
	{ "eucJP", e_euc },
	{ "EUC-JP", e_euc },
	{ "ujis", ??? },
	{ "EUC-KR", e_euckorean },
#endif
	{ NULL }};
    int i;
    static char *last_complaint;
    char buffer[80];

#if HAVE_ICONV_H
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

#if HAVE_ICONV_H
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
    srandom(tv.tv_usec+1);
    r2 = random();
    sprintf( buffer, "1021 %d %d", r1, r2 );
    free(xuid);
    xuid = copy(buffer);
}

static void NOUI_SetDefaults(void) {

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

static void NOUI_LoadPrefs(void) {
    char *prefs = getPfaEditPrefs();
    FILE *p;
    char line[1100];
    int i, j/*, ri=0, mn=0, ms=0, fn=0, ff=0, filt_max=0*/;
    int msp=0, msc=0;
    char *pt;
    struct prefs_list *pl;

#if !defined(NOPLUGIN)
    LoadPluginDir(NULL);
#endif
    LoadPfaEditEncodings();
#if 0
    LoadGroupList();
#endif

    if ( prefs!=NULL && (p=fopen(prefs,"r"))!=NULL ) {
	while ( fgets(line,sizeof(line),p)!=NULL ) {
	    if ( *line=='#' )
	continue;
	    pt = strchr(line,':');
	    if ( pt==NULL )
	continue;
	    for ( j=0; prefs_list[j]!=NULL; ++j ) {
		for ( i=0; prefs_list[j][i].name!=NULL; ++i )
		    if ( strncmp(line,prefs_list[j][i].name,pt-line)==0 )
		break;
		if ( prefs_list[j][i].name!=NULL )
	    break;
	    }
	    pl = NULL;
	    if ( prefs_list[j]!=NULL )
		pl = &prefs_list[j][i];
	    for ( ++pt; *pt=='\t'; ++pt );
	    if ( line[strlen(line)-1]=='\n' )
		line[strlen(line)-1] = '\0';
	    if ( line[strlen(line)-1]=='\r' )
		line[strlen(line)-1] = '\0';
	    if ( pl==NULL ) {
#if 0
		if ( strncmp(line,"Recent:",strlen("Recent:"))==0 && ri<RECENT_MAX )
		    RecentFiles[ri++] = copy(pt);
		else if ( strncmp(line,"MenuScript:",strlen("MenuScript:"))==0 && ms<SCRIPT_MENU_MAX )
		    script_filenames[ms++] = copy(pt);
		else if ( strncmp(line,"MenuName:",strlen("MenuName:"))==0 && mn<SCRIPT_MENU_MAX )
		    script_menu_names[mn++] = utf82u_copy(pt);
		else if ( strncmp(line,"FontFilterName:",strlen("FontFilterName:"))==0 ) {
		    if ( fn>=filt_max )
			user_font_filters = grealloc(user_font_filters,((filt_max+=10)+1)*sizeof( struct openfilefilters));
		    user_font_filters[fn].filter = NULL;
		    user_font_filters[fn++].name = copy(pt);
		    user_font_filters[fn].name = NULL;
		} else if ( strncmp(line,"FontFilter:",strlen("FontFilter:"))==0 ) {
		    if ( ff<filt_max )
			user_font_filters[ff++].filter = copy(pt);
		} else
#endif
		if ( strncmp(line,"MacMapCnt:",strlen("MacSetCnt:"))==0 ) {
		    sscanf( pt, "%d", &msc );
		    msp = 0;
		    user_macfeat_otftag = gcalloc(msc+1,sizeof(struct macsettingname));
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
	old_ttf_flags |= ttf_flag_glyphmap;
	old_otf_flags |= ttf_flag_glyphmap;
    }
    LoadNamelistDir(NULL);
}

static void NOUI_SavePrefs(int not_if_script) {
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

#if 0
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
#endif
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

static struct prefs_interface prefsnoui = {
    NOUI_SavePrefs,
    NOUI_LoadPrefs,
    NOUI_GetPrefs,
    NOUI_SetPrefs,
    NOUI_getFontForgeShareDir,
    NOUI_SetDefaults
};

struct prefs_interface *prefs_interface = &prefsnoui;

void FF_SetPrefsInterface(struct prefs_interface *prefsi) {
    prefs_interface = prefsi;
}
