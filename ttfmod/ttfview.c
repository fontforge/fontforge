/* Copyright (C) 2001 by George Williams */
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
#include "ttfmodui.h"
#include <ustring.h>
#include <gkeysym.h>
#include <utype.h>
#include <gfile.h>
#include <math.h>

extern int _GScrollBar_Width;

#define ttf_width 16
#define ttf_height 16
static unsigned char ttf_bits[] = {
   0xff, 0x07, 0x21, 0x04, 0x20, 0x00, 0xfc, 0x7f, 0x24, 0x40, 0xf4, 0x5e,
   0xbc, 0x72, 0xa0, 0x02, 0xa0, 0x02, 0xa0, 0x02, 0xf0, 0x02, 0x80, 0x02,
   0xc0, 0x06, 0x40, 0x04, 0xc0, 0x07, 0x00, 0x00};
GWindow ttf_icon = NULL;

TtfView *tfv_list=NULL;

void FontNew(void) {}

#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_SelAll	2106

static struct tableinfo {
    int32 name;
    int description;
    void (*createviewer)(Table *,TtfView *);
    char *helppage;
} tableinfo[] = {
/* "Required" TrueType tables (except sometimes (ie. OpenType, bitmap only, etc.)) */
    { CHR('c','m','a','p'), _STR_Tbl_cmap, NULL, "http://partners.adobe.com/asn/developer/opentype/cmap.html" },
    { CHR('g','l','y','f'), _STR_Tbl_glyf, NULL, "http://partners.adobe.com/asn/developer/opentype/glyf.html" },
    { CHR('h','e','a','d'), _STR_Tbl_head, headCreateEditor, "http://partners.adobe.com/asn/developer/opentype/head.html" },
    { CHR('h','h','e','a'), _STR_Tbl_hhea, _heaCreateEditor, "http://partners.adobe.com/asn/developer/opentype/hhea.html" },
    { CHR('h','m','t','x'), _STR_Tbl_hmtx, metricsCreateEditor, "http://partners.adobe.com/asn/developer/opentype/hmtx.html" },
    { CHR('m','a','x','p'), _STR_Tbl_maxp, maxpCreateEditor, "http://partners.adobe.com/asn/developer/opentype/maxp.html" },
    { CHR('n','a','m','e'), _STR_Tbl_name, NULL, "http://partners.adobe.com/asn/developer/opentype/name.html" },
    { CHR('O','S','/','2'), _STR_Tbl_OS2 , OS2CreateEditor, "http://partners.adobe.com/asn/developer/opentype/os2.html" },
    { CHR('p','o','s','t'), _STR_Tbl_post, postCreateEditor, "http://partners.adobe.com/asn/developer/opentype/post.html" },
/* Other MS/Adobe/Apple tables */
    { CHR('c','v','t',' '), _STR_Tbl_cvt , shortCreateEditor, "http://partners.adobe.com/asn/developer/opentype/cvt.html" },
    { CHR('E','B','D','T'), _STR_Tbl_EBDT, NULL, "http://partners.adobe.com/asn/developer/opentype/ebdt.html" },
    { CHR('E','B','L','C'), _STR_Tbl_EBLC, NULL, "http://partners.adobe.com/asn/developer/opentype/eblc.html" },
    { CHR('E','B','S','C'), _STR_Tbl_EBSC, NULL, "http://partners.adobe.com/asn/developer/opentype/ebsc.html" },
    { CHR('f','p','g','m'), _STR_Tbl_fpgm, instrCreateEditor, "http://partners.adobe.com/asn/developer/opentype/fpgm.html" },
    { CHR('g','a','s','p'), _STR_Tbl_gasp, NULL, "http://partners.adobe.com/asn/developer/opentype/gasp.html" },
    { CHR('h','d','m','x'), _STR_Tbl_hdmx, NULL, "http://partners.adobe.com/asn/developer/opentype/hdmx.html" },
    { CHR('k','e','r','n'), _STR_Tbl_kern, NULL, "http://partners.adobe.com/asn/developer/opentype/kern.html" },
    { CHR('L','T','S','H'), _STR_Tbl_LTSH, NULL, "http://partners.adobe.com/asn/developer/opentype/ltsh.html" },
    { CHR('p','r','e','p'), _STR_Tbl_prep, instrCreateEditor, "http://partners.adobe.com/asn/developer/opentype/prep.html" },
    { CHR('P','C','L','T'), _STR_Tbl_PCLT, NULL, "http://partners.adobe.com/asn/developer/opentype/pclt.html" },
    { CHR('V','D','M','X'), _STR_Tbl_VDMX, NULL, "http://partners.adobe.com/asn/developer/opentype/vdmx.html" },
    { CHR('v','h','e','a'), _STR_Tbl_vhea, _heaCreateEditor, "http://partners.adobe.com/asn/developer/opentype/vhea.html" },
    { CHR('v','m','t','x'), _STR_Tbl_vmtx, metricsCreateEditor, "http://partners.adobe.com/asn/developer/opentype/vmtx.html" },
    { CHR('C','F','F',' '), _STR_Tbl_CFF , NULL, "http://partners.adobe.com/asn/developer/opentype/cff.html" },
/*    { CHR('f','v','a','r'), _STR_Tbl_fvar, NULL, "http://partners.adobe.com/asn/developer/opentype/tablist.html" }, */
    { CHR('M','M','S','D'), _STR_Tbl_MMSD, NULL, "http://partners.adobe.com/asn/developer/opentype/tablist.html" },
    { CHR('M','M','F','X'), _STR_Tbl_MMFX, NULL, "http://partners.adobe.com/asn/developer/opentype/tablist.html" },
    { CHR('B','A','S','E'), _STR_Tbl_BASE, NULL, "http://partners.adobe.com/asn/developer/opentype/base.html" },
    { CHR('G','D','E','F'), _STR_Tbl_GDEF, NULL, "http://partners.adobe.com/asn/developer/opentype/gdef.html" },
    { CHR('G','P','O','S'), _STR_Tbl_GPOS, NULL, "http://partners.adobe.com/asn/developer/opentype/gpos.html" },
    { CHR('G','S','U','B'), _STR_Tbl_GSUB, NULL, "http://partners.adobe.com/asn/developer/opentype/gsub.html" },
    { CHR('J','S','T','F'), _STR_Tbl_JSTF, NULL, "http://partners.adobe.com/asn/developer/opentype/jstf.html" },
    { CHR('D','S','I','G'), _STR_Tbl_DSIG, NULL, "http://partners.adobe.com/asn/developer/opentype/dsig.html" },
    { CHR('V','O','R','G'), _STR_Tbl_VORG, NULL, "http://partners.adobe.com/asn/developer/opentype/vorg.html" },
/* Apple only (GX?) tables */
    { CHR('a','c','n','t'), _STR_Tbl_acnt, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6acnt.html" },
    { CHR('a','v','a','r'), _STR_Tbl_avar, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6avar.html" },
    { CHR('b','d','a','t'), _STR_Tbl_bdat, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6bdat.html" },
    { CHR('b','h','e','d'), _STR_Tbl_bhed, headCreateEditor, "http://fonts.apple.com/TTRefMan/RM06/Chap6bhed.html" },
    { CHR('b','l','o','c'), _STR_Tbl_bloc, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6bloc.html" },
    { CHR('b','s','l','n'), _STR_Tbl_bsln, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6bsln.html" },
    { CHR('c','v','a','r'), _STR_Tbl_cvar, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6cvar.html" },
    { CHR('f','d','s','c'), _STR_Tbl_fdsc, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6fdsc.html" },
    { CHR('f','e','a','t'), _STR_Tbl_feat, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6feat.html" },
    { CHR('f','m','t','x'), _STR_Tbl_fmtx, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6fmtx.html" },
    { CHR('f','v','a','r'), _STR_Tbl_fvar, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6fvar.html" },
    { CHR('g','v','a','r'), _STR_Tbl_gvar, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6gvar.html" },
    { CHR('h','s','t','y'), _STR_Tbl_hsty, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6hsty.html" },
    { CHR('j','u','s','t'), _STR_Tbl_just, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6just.html" },
    { CHR('l','c','a','r'), _STR_Tbl_lcar, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6lcar.html" },
    { CHR('m','o','r','t'), _STR_Tbl_mort, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6mort.html" },
    { CHR('m','o','r','x'), _STR_Tbl_morx, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6morx.html" },
    { CHR('o','p','b','d'), _STR_Tbl_opbd, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6opbd.html" },
    { CHR('p','r','o','p'), _STR_Tbl_prop, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6prop.html" },
    { CHR('t','r','a','k'), _STR_Tbl_trak, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6trak.html" },
    { CHR('Z','a','p','f'), _STR_Tbl_Zapf, NULL, "http://fonts.apple.com/TTRefMan/RM06/Chap6Zapf.html" },
    { 0 }
};

TtfFile *LoadTtfFont(char *filename) {
    TtfView *tfv;
    TtfFile *ttf;
    char *pt, *ept;
    static char *extens[] = { ".ttf", ".otf", ".ttc", ".TTF", ".OTF", ".TTC", NULL };
    int i;

    if ( filename==NULL )
return( NULL );

    if (( pt = strrchr(filename,'/'))==NULL ) pt = filename;
    if ( strchr(pt,'.')==NULL ) {
	/* They didn't give an extension. If there's a file with no extension */
	/*  see if it's a valid ttf file (and if so use the extensionless */
	/*  filename), otherwise guess at an extension */
	int ok = false;
	FILE *test = fopen(filename,"r");
	if ( test!=NULL ) {
	    int magic = getlong(test);
	    if ( magic==0x00010000 || magic == CHR('O','T','T','O') || magic == CHR('t','t','c','f'))
		ok = true;
	    fclose(test);
	}
	if ( !ok ) {
	    pt = galloc(strlen(filename)+8);
	    strcpy(pt,filename);
	    ept = pt+strlen(pt);
	    for ( i=0; extens[i]!=NULL; ++i ) {
		strcpy(ept,extens[i]);
		if ( GFileExists(pt))
	    break;
	    }
	    if ( extens[i]!=NULL )
		filename = pt;
	    else {
		free(pt);
		pt = NULL;
	    }
	}
    } else
	pt = NULL;

    ttf = NULL;
    /* Only one view per font */
    for ( tfv=tfv_list; tfv!=NULL && ttf==NULL; tfv=tfv->next ) {
	if ( tfv->ttf->filename!=NULL && strcmp(tfv->ttf->filename,filename)==0 )
	    ttf = tfv->ttf;
    }

    if ( ttf==NULL )
	ttf = ReadTtfFont(filename);

    if ( pt==filename )
	free(filename);
return( ttf );
}

TtfView *ViewTtfFont(char *filename) {
    TtfFile *ttf = LoadTtfFont(filename);
    if ( ttf==NULL )
return( NULL );
    if ( ttf->tfv!=NULL ) {
	GDrawSetVisible(ttf->tfv->gw,true);
	GDrawRaise(ttf->tfv->gw);
return( ttf->tfv );
    }
return( TtfViewCreate(ttf));
}

static char *GetTtfFontName(int mult) {
    /* Some people use pf3 as an extension for postscript type3 fonts */
    static unichar_t wild[] = { '*', '.', '{', 't','t','f',',', 'o','t','f',',', 't','t','c','}', 
	     '{','.','g','z',',','.','Z',',','.','b','z','2',',','}',  '\0' };
    unichar_t *ret = FVOpenFont(GStringGetResource(_STR_OpenTtf,NULL),
	    NULL,wild,NULL,mult,false);
    char *temp = cu_copy(ret);

    free(ret);
return( temp );
}

void MenuOpen(GWindow base,struct gmenuitem *mi,GEvent *e) {
    char *temp = GetTtfFontName(true);
    char *eod, *fpt, *file, *full;

    if ( temp==NULL )
return;
    eod = strrchr(temp,'/');
    *eod = '\0';
    file = eod+1;
    do {
	fpt = strstr(file,"; ");
	if ( fpt!=NULL ) *fpt = '\0';
	full = galloc(strlen(temp)+1+strlen(file)+1);
	strcpy(full,temp); strcat(full,"/"); strcat(full,file);
	ViewTtfFont(full);
	file = fpt+2;
    } while ( fpt!=NULL );
    free(temp);
}

int _TFVMenuSaveAs(TtfView *tfv) {
return( 0 );
}

int _TFVMenuSave(TtfView *tfv) {
return( 0 );
}

int _TFVMenuRevert(TtfView *tfv) {
return( 0 );
}

static void TFVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = (TtfView *) GDrawGetUserData(gw);

    _TFVMenuSaveAs(tfv);
}

static void TFVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = (TtfView *) GDrawGetUserData(gw);
    _TFVMenuSave(tfv);
}

static void TFVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = (TtfView *) GDrawGetUserData(gw);
    _TFVMenuRevert(tfv);
}

void MenuHelp(GWindow base,struct gmenuitem *mi,GEvent *e) {
    system("netscape http://pfaedit.sf.net/ttfmod/ &");
}

void TableHelp(int table_name) {
    int k;
    char buffer[1000];

    for ( k=0; tableinfo[k].name!=0; ++k )
	if ( tableinfo[k].name == table_name )
    break;
    if ( tableinfo[k].name==0 )
	MenuHelp(NULL,NULL,NULL);
    else {
	sprintf(buffer,"netscape %s &" );
	system(buffer);
    }
}

static int _TFVMenuClose(TtfView *tfv) {
    int i,j;
    /* Check for changes !!!! */
    for ( i=0; i<tfv->ttf->font_cnt; ++i ) {
	for ( j=0; j<tfv->ttf->fonts[i]->tbl_cnt; ++j )
	    if ( !tfv->ttf->fonts[i]->tbls[j]->destroyed &&
		    tfv->ttf->fonts[i]->tbls[j]->tv!=NULL ) {
		/* same table may be referenced by several fonts */
		/* or in the case of EBDT, and bdat by the same font */
		tfv->ttf->fonts[i]->tbls[j]->destroyed = true;
		GDrawDestroyWindow(tfv->ttf->fonts[i]->tbls[j]->tv->gw);
	    }
    }
    GDrawDestroyWindow(tfv->gw);
return( 1 );
}

static void TFVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = (TtfView *) GDrawGetUserData(gw);

    DelayEvent((void (*)(void *)) _TFVMenuClose, tfv);
}

void MenuPrefs(GWindow base,struct gmenuitem *mi,GEvent *e) {
    DoPrefs();
}

static void _MenuExit(void *junk) {
    TtfView *tfv, *next;

    for ( tfv = tfv_list; tfv!=NULL; tfv = next ) {
	next = tfv->next;
	if ( !_TFVMenuClose(tfv))
return;
    }
    exit(0);
}

void MenuExit(GWindow base,struct gmenuitem *mi,GEvent *e) {
    DelayEvent((void (*)(void *)) _MenuExit, NULL);
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static GMenuItem dummyitem[] = { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) _STR_Open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) _STR_Close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, TFVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, TFVMenuSave },
    { { (unichar_t *) _STR_Saveas, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, TFVMenuSaveAs },
    { { (unichar_t *) _STR_Revertfile, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, TFVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { (unichar_t *) _STR_Undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL },
    { { (unichar_t *) _STR_Redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, NULL, MID_Cut },
    { { (unichar_t *) _STR_Copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, NULL, MID_Copy },
    { { (unichar_t *) _STR_Paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, NULL, MID_Paste },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_SelectAll, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, NULL },
    { NULL }
};

GMenuItem helplist[] = {
    { { (unichar_t *) _STR_Help, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, GK_F1, 0, NULL, NULL, MenuHelp },
    { NULL }
};

static GMenuItem mblist[] = {
    { { (unichar_t *) _STR_File, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) _STR_Edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'E' }, 0, 0, edlist },
    { { (unichar_t *) _STR_Window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 0, 0, NULL, WindowMenuBuild, NULL },
    { { (unichar_t *) _STR_Help, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};

static void TFVMouse(TtfView *tfv,GEvent *e) {
    TtfFile *ttf = tfv->ttf;
    TtfFont *font;
    int seek = (e->u.mouse.y-2) / tfv->fh + tfv->lpos;
    int i,j, l, k;
    Table *tab;

    GGadgetEndPopup();

    j = -2;
    for ( i=l=0; i< ttf->font_cnt; ++i ) {
	j = -1;
	if ( l==seek )
    break;
	font = ttf->fonts[i];
	++l;
	for ( j=0; j<font->tbl_cnt; ++j, ++l ) {
	    if ( l==seek )
	break;
	}
	if ( j==font->tbl_cnt )
	    j= -2;
	if ( l==seek )
    break;
    }
    if ( j==-2 )		/* Beyond end of file */
return;

    tab = NULL; k = -1;
    if ( j!=-1 ) {
	tab = font->tbls[j];
	for ( k=0; tableinfo[k].name!=0; ++k )
	    if ( tableinfo[k].name == tab->name )
	break;
	if ( tableinfo[k].name==0 ) k = -1;
    }

    if ( e->type == et_mousedown ) {
	tfv->pressed = true;
	if ( i!=tfv->selectedfont ) {
	    tfv->selectedfont = i;
	    GDrawRequestExpose(tfv->v,NULL,false);
	}
	if ( tab!=NULL && e->u.mouse.clicks==2 ) {
	    if ( tab->tv!=NULL ) {
		GDrawSetVisible(tab->tv->gw,true);
		GDrawRaise(tab->tv->gw);
	    } else if ( tableinfo[k].createviewer==NULL ||
		    (e->u.mouse.state&(ksm_control|ksm_meta)) )
		binaryCreateEditor(tab,tfv);
	    else
		(tableinfo[k].createviewer)(tab,tfv);
	}
    } else if ( e->type == et_mouseup ) {
	tfv->pressed = false;
    } else if ( e->type == et_mousemove && !tfv->pressed ) {
	if ( k!=-1 )
	    GGadgetPreparePopup(tfv->gw,
		    GStringGetResource(tableinfo[k].description,NULL));
    }
}

static void TFVExpose(TtfView *tfv,GWindow pixmap,GRect *rect) {
    int low, high;
    int i,j, l;
    TtfFile *ttf = tfv->ttf;
    TtfFont *font;
    unichar_t buffer[60];
    static unichar_t spec[] = { ' ', '%', '8', 'd', ' ', '%', '8', 'd',  '\0' };

    low = rect->y/tfv->fh + tfv->lpos;
    high = (rect->y+rect->height+tfv->fh-1)/tfv->fh + tfv->lpos;
    for ( i=l=0; i< ttf->font_cnt; ++i ) {
	if ( l>high )
    break;
	font = ttf->fonts[i];
	if ( l>=low ) {
	    GDrawDrawLine(pixmap,0,(l-tfv->lpos)*tfv->fh,tfv->vwidth,(l-tfv->lpos)*tfv->fh,
		i==tfv->selectedfont || i==tfv->selectedfont+1 ? 0 : 0x808080 );
	    GDrawSetFont(pixmap,tfv->bold);
	    GDrawDrawText(pixmap,10,(l-tfv->lpos)*tfv->fh+tfv->as+2,font->fontname,-1,NULL,0);
	}
	++l;
	for ( j=0; j<font->tbl_cnt; ++j, ++l ) {
	    if ( l>high )
	break;
	    if ( l>=low ) {
		buffer[0] = (font->tbls[j]->name>>24)&0xff;
		buffer[1] = (font->tbls[j]->name>>16)&0xff;
		buffer[2] = (font->tbls[j]->name>>8)&0xff;
		buffer[3] = (font->tbls[j]->name   )&0xff;
		u_sprintf(buffer+4, spec,
			font->tbls[j]->start, font->tbls[j]->len );
		GDrawSetFont(pixmap, font->tbls[j]->changed ? tfv->bold : tfv->font);
		GDrawDrawText(pixmap,3,(l-tfv->lpos)*tfv->fh+tfv->as+2,buffer,-1,NULL,0);
	    }
	}
    }
}

static void TFVScroll(TtfView *tfv,struct sbevent *sb) {
    int newpos = tfv->lpos;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= tfv->vheight/tfv->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += tfv->vheight/tfv->fh;
      break;
      case et_sb_bottom:
        newpos = tfv->lheight-tfv->vheight/tfv->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>tfv->lheight-tfv->vheight/tfv->fh )
        newpos = tfv->lheight-tfv->vheight/tfv->fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=tfv->lpos ) {
	int diff = newpos-tfv->lpos;
	tfv->lpos = newpos;
	GScrollBarSetPos(tfv->vsb,tfv->lpos);
	GDrawScroll(tfv->v,NULL,0,diff*tfv->fh);
    }
}

static void TFVResize(TtfView *tfv,GEvent *event) {
    GRect pos;
    int lh, i;

    /* Multiple of the number of lines we've got */
    if ( (event->u.resize.size.height-tfv->mbh-4)%tfv->fh!=0 ) {
	int lc = (event->u.resize.size.height-tfv->mbh-4+tfv->fh/2)/tfv->fh;
	if ( lc<=0 ) lc = 1;
	GDrawResize(tfv->gw, event->u.resize.size.width,lc*tfv->fh+tfv->mbh+4);
return;
    }

    pos.width = GDrawPointsToPixels(tfv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-tfv->mbh;
    pos.x = event->u.resize.size.width-pos.width; pos.y = tfv->mbh;
    GGadgetResize(tfv->vsb,pos.width,pos.height);
    GGadgetMove(tfv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(tfv->v,pos.width,pos.height);

    tfv->vheight = pos.height; tfv->vwidth = pos.width;

    for ( i=0, lh=0; i<tfv->ttf->font_cnt; ++i )
	lh += 1 + tfv->ttf->fonts[i]->tbl_cnt;
    tfv->lheight = lh;
    GScrollBarSetBounds(tfv->vsb,0,lh,tfv->vheight/tfv->fh);
    if ( tfv->lpos + tfv->vheight/tfv->fh > lh )
	tfv->lpos = lh-tfv->vheight/tfv->fh;
    GScrollBarSetPos(tfv->vsb,tfv->lpos);
}

static void TtfViewFree(TtfView *tfv) {
}

static int v_e_h(GWindow gw, GEvent *event) {
    TtfView *tfv = (TtfView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	TFVExpose(tfv,gw,&event->u.expose.rect);
      break;
      case et_char:
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	TFVMouse(tfv,event);
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int tfv_e_h(GWindow gw, GEvent *event) {
    TtfView *tfv = (TtfView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
      break;
      case et_resize:
	TFVResize(tfv,event);
      break;
      case et_char:
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    TFVScroll(tfv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	TFVMenuClose(gw,NULL,NULL);
      break;
      case et_create:
	tfv->next = tfv_list;
	tfv_list = tfv;
      break;
      case et_destroy:
	if ( tfv_list==tfv )
	    tfv_list = tfv->next;
	else {
	    TtfView *n;
	    for ( n=tfv_list; n->next!=tfv; n=n->next );
	    n->next = tfv->next;
	}
	TtfViewFree(tfv);
      break;
    }
return( true );
}

TtfView *TtfViewCreate(TtfFile *tf) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    GGadget *sb;
    GRect gsize;
    TtfView *tfv = gcalloc(1,sizeof(TtfView));
    FontRequest rq;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    int as,ds,ld, i,lh;

    if ( ttf_icon==NULL )
	ttf_icon = GDrawCreateBitmap(NULL,ttf_width,ttf_height,ttf_bits);

    tf->tfv = tfv;
    tfv->ttf = tf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_pointer;
    wattrs.window_title = tf->is_ttc?uc_copy(GFileNameTail(tf->filename)):
			    u_copy(tf->fonts[0]->fontname);
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width = 180;
    pos.height = 100;
    tfv->gw = gw = GDrawCreateTopWindow(NULL,&pos,tfv_e_h,tfv,&wattrs);
    free((unichar_t *) wattrs.window_title);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    tfv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(tfv->mb,&gsize);
    tfv->mbh = gsize.height;

    gd.pos.y = tfv->mbh; gd.pos.height = pos.height-tfv->mbh;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    tfv->vsb = sb = GScrollBarCreate(gw,&gd,tfv);

    wattrs.mask = wam_events|wam_cursor;
    pos.x = 0; pos.y = tfv->mbh;
    pos.width = gd.pos.x; pos.height -= tfv->mbh;
    tfv->v = GWidgetCreateSubWindow(gw,&pos,v_e_h,tfv,&wattrs);
    GDrawSetVisible(tfv->v,true);

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -12;
    rq.weight = 700;
    tfv->bold = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    rq.weight = 400;
    tfv->font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(tfv->v,tfv->font);
    GDrawFontMetrics(tfv->font,&as,&ds,&ld);
    tfv->as = as+1;
    tfv->fh = tfv->as+ds;

    for ( i=0, lh=0; i<tfv->ttf->font_cnt; ++i )
	lh += 1 + tfv->ttf->fonts[i]->tbl_cnt;
    if ( lh>40 ) lh = 40;
    if ( lh<4 ) lh = 4;
    GDrawResize(tfv->gw,pos.width+gd.pos.width,tfv->mbh+lh*tfv->fh);

    GDrawSetVisible(gw,true);
return( tfv );
}
