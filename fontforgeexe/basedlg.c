/* Copyright (C) 2008-2012 by George Williams */
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

#include <fontforge-config.h>

#include "chardata.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "splineutil.h"
#include "tottfgpos.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>

static uint32 TagFromString(char *pt) {
    char script[4];

    memset(script,' ',4);
    script[0] = *pt++;
    if ( *pt!='\0' ) {
	script[1] = *pt++;
	if ( *pt!='\0' ) {
	    script[2] = *pt++;
	    if ( *pt!='\0' )
		script[3] = *pt;
	}
    }
return( CHR(script[0],script[1],script[2],script[3]));
}

typedef struct basedlg {
    GWindow gw;
    SplineFont *sf;
    struct Base *old;
    int vertical;
    int done;
} BaseDlg;

typedef struct baselangdlg {
    GWindow gw;
    SplineFont *sf;
    struct baselangextent *old;
    int vertical;
    int islang;
    int done;
    uint32 script;
} BaseLangDlg;

extern GTextInfo scripts[], languages[];

static struct col_init baselangci[4] = {
    { me_stringchoicetag , NULL, languages, NULL, N_("Language") },
    { me_int , NULL, NULL, NULL, N_("Min") },
    { me_int , NULL, NULL, NULL, N_("Max") },
    { me_addr, NULL, NULL, NULL, "hidden" }	/* Used to hold per-language data */
    };
static struct col_init basefeatci[4] = {
    { me_string , NULL, NULL, NULL, N_("Feature") },
    { me_int , NULL, NULL, NULL, N_("Min") },
    { me_int , NULL, NULL, NULL, N_("Max") },
    { me_addr, NULL, NULL, NULL, "hidden" }	/* Not used */
    };

#define CID_Languages	1008
#define CID_Extents	1009

static void Base_EnableOtherButtons(GGadget *g, int r, int c) {
    GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Extents),r!=-1);
}

static void Base_DelClean(GGadget *g, int r) {
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *md = _GMatrixEditGet(g,&rows);

    BaseLangFree((struct baselangextent *) md[r*cols+cols-1].u.md_str);
    md[r*cols+cols-1].u.md_str = NULL;
}

static void BaseLang_FinishEdit(GGadget *g, int r, int c, int wasnew) {

    if ( wasnew && c==0 ) {
	BaseLangDlg *b = GDrawGetUserData(GGadgetGetWindow(g));
	int rows, cols = GMatrixEditGetColCnt(g);
	struct matrix_data *md = GMatrixEditGet(g,&rows);
	uint32 lang = TagFromString(md[r*cols+0].u.md_str);
	int gid;
	SplineChar *sc;
	DBounds bnd, scb;

	memset(&bnd,0,sizeof(bnd));
	for ( gid=0; gid<b->sf->glyphcnt; ++gid ) if ( (sc=b->sf->glyphs[gid])!=NULL ) {
	    if ( lang==CHR('E','N','G',' ') && (sc->unicodeenc<0 || sc->unicodeenc>127))
		/* English just uses ASCII */;
	    else if ( b->script==CHR('l','a','t','n') &&
		    (sc->unicodeenc<0 || sc->unicodeenc>255) &&
		    lang!=CHR('V','I','T',' ') )
		/* Most languages in latin script only use one accent per */
		/*  letter. So latin1 should provide a reasonable bounding */
		/*  box. Vietnamese is an exception. */;
	    else if ( SCScriptFromUnicode(sc)!=b->script )
		/* Not interesting */;
	    else {
		SplineCharFindBounds(sc,&scb);
		if ( bnd.minx==bnd.maxx )
		    bnd = scb;
		else {
		    if ( scb.minx<bnd.minx ) bnd.minx = scb.minx;
		    if ( scb.miny<bnd.miny ) bnd.miny = scb.miny;
		    if ( scb.maxx>bnd.maxx ) bnd.maxx = scb.maxx;
		    if ( scb.maxy>bnd.maxy ) bnd.maxy = scb.maxy;
		}
	    }
	}
	if ( b->vertical ) {
	    md[r*cols+1].u.md_ival = floor(bnd.minx);
	    md[r*cols+2].u.md_ival = ceil(bnd.maxx);
	} else {
	    md[r*cols+1].u.md_ival = floor(bnd.miny);
	    md[r*cols+2].u.md_ival = ceil(bnd.maxy);
	}
    }
}
	
static void BaseLangMatrixInit(struct matrixinit *mi,struct baselangextent *old,
	int islang, int isvert) {
    struct matrix_data *md;
    int cnt;
    char lang[8];
    struct baselangextent *bl;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 4;
    mi->col_init = islang ? baselangci : basefeatci;

    mi->col_init[1].title = isvert ? _("Min") : _("Min (descent)");
    mi->col_init[2].title = isvert ? _("Max") : _("Max (ascent)");

    for ( cnt = 0, bl=old; bl!=NULL; bl=bl->next, ++cnt);
    mi->initial_row_cnt = cnt;
    mi->matrix_data     = md = calloc(mi->col_cnt*cnt,sizeof(struct matrix_data));

    for ( cnt = 0, bl=old; bl!=NULL; bl=bl->next, ++cnt) {
	lang[0] = bl->lang>>24;
	lang[1] = bl->lang>>16;
	lang[2] = bl->lang>>8;
	lang[3] = bl->lang;
	lang[4] = '\0';
	md[mi->col_cnt*cnt+0].u.md_str = copy(lang);
	md[mi->col_cnt*cnt+1].u.md_ival = bl->descent;
	md[mi->col_cnt*cnt+2].u.md_ival = bl->ascent;
	md[mi->col_cnt*cnt+mi->col_cnt-1].u.md_str = (char *) BaseLangCopy(bl->features);
    }

    mi->finishedit = BaseLang_FinishEdit;
    mi->candelete = NULL;
    mi->popupmenu = NULL;
    mi->handle_key = NULL;
    mi->bigedittitle = NULL;
}

static struct baselangextent *SFBaselang(SplineFont *sf,struct baselangextent *old,
	int is_vertical, int islang, uint32 script );

static int BaseLang_Extents(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	BaseLangDlg *b = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *gme = GWidgetGetControl(b->gw,CID_Languages);
	int rows, cols = GMatrixEditGetColCnt(gme);
	struct matrix_data *md = _GMatrixEditGet(gme,&rows);
	int r = GMatrixEditGetActiveRow(gme);

	if ( r<0 || r>=rows )
return( true );
	md[r*cols+cols-1].u.md_str = (char *)
		SFBaselang(b->sf,(struct baselangextent *)md[r*cols+cols-1].u.md_str,
			b->vertical,false, b->script);
    }
return( true );
}

static void BaseLang_DoCancel(BaseLangDlg *b) {
    GGadget *g = GWidgetGetControl(b->gw,CID_Languages);
    int r, rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *md = _GMatrixEditGet(g,&rows);

    for ( r=0; r<rows; ++r ) {
	BaseLangFree( (struct baselangextent *) md[r*cols+cols-1].u.md_str);
	md[r*cols+cols-1].u.md_str = NULL;
    }
    b->done = true;
}

static int BaseLang_Cancel(GGadget *g, GEvent *e) {
    BaseLangDlg *b;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	b = GDrawGetUserData(GGadgetGetWindow(g));
	BaseLang_DoCancel(b);
    }
return( true );
}

static int BaseLang_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	BaseLangDlg *b = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *g = GWidgetGetControl(b->gw,CID_Languages);
	int r, rows, cols = GMatrixEditGetColCnt(g);
	struct matrix_data *md = GMatrixEditGet(g,&rows);
	struct baselangextent *cur, *last;

	if ( md==NULL )
return( true );

	BaseLangFree(b->old);
	b->old = last = NULL;
	for ( r=0; r<rows; ++r ) {
	    cur = chunkalloc(sizeof(struct baselangextent));
	    cur->lang = TagFromString(md[r*cols+0].u.md_str);
	    cur->descent = md[r*cols+1].u.md_ival;
	    cur->ascent = md[r*cols+2].u.md_ival;
	    cur->features = (struct baselangextent *) md[r*cols+3].u.md_str;
	    md[r*cols+3].u.md_str = NULL;
	    if ( last==NULL )
		b->old = cur;
	    else
		last->next = cur;
	    last = cur;
	}
	b->done = true;
    }
return( true );
}

static int baselang_e_h(GWindow gw, GEvent *event) {
    BaseLangDlg *b = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	BaseLang_DoCancel(b);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/baseline.html", NULL);
return( true );
	}
return( false );
      break;
    }
return( true );
}

static struct baselangextent *SFBaselang(SplineFont *sf,struct baselangextent *old,
	int is_vertical, int islang, uint32 script ) {
    int i,j,k;
    GRect pos;
    GWindowAttrs wattrs;
    BaseLangDlg b;
    GWindow gw;
    char buffer[200];
    GGadgetCreateData gcd[13], box[4], buttongcd[2];
    GGadgetCreateData *buttonarray[8], *varray[12];
    GTextInfo label[13], buttonlabel[1];
    struct matrixinit mi;
    GGadget *g;

    memset(&b,0,sizeof(b));
    b.sf = sf;
    b.old = old;
    b.vertical = is_vertical;
    b.islang = islang;
    b.script = script;

    memset(&wattrs,0,sizeof(wattrs));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    snprintf(buffer,sizeof(buffer),is_vertical ? _("Vertical Extents for %c%c%c%c") : _("Horizontal Extents for %c%c%c%c"),
	    script>>24, script>>16, script>>8, script);
    wattrs.utf8_window_title = buffer;
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,300));
    pos.height = GDrawPointsToPixels(NULL,200);
    b.gw = gw = GDrawCreateTopWindow(NULL,&pos,baselang_e_h,&b,&wattrs);

    memset(&gcd,0,sizeof(gcd));
    memset(&buttongcd,0,sizeof(buttongcd));
    memset(&box,0,sizeof(box));
    memset(&label,0,sizeof(label));
    memset(&buttonlabel,0,sizeof(buttonlabel));

    i = j = k = 0;
    if ( islang )
	label[i].text = (unichar_t *) _(
		"Set the minimum and maximum values by which\n"
		"the glyphs in this script extend below and\n"
		"above the baseline. This may vary by language" );
    else
	label[i].text = (unichar_t *) _(
		"Set the minimum and maximum values by which\n"
		"the glyphs in this script extend below and\n"
		"above the baseline when modified by a feature." );
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5+4; 
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].creator = GLabelCreate;
    varray[j++] = &gcd[i++]; varray[j++] = NULL;

    BaseLangMatrixInit(&mi,old,islang,is_vertical);
    gcd[i].gd.pos.width = 300; gcd[i].gd.pos.height = 200;
    gcd[i].gd.flags = gg_enabled | gg_visible;
    gcd[i].gd.cid = CID_Languages;
    gcd[i].gd.u.matrix = &mi;
    gcd[i++].creator = GMatrixEditCreate;
    varray[j++] = &gcd[i-1]; varray[j++] = NULL;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = BaseLang_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = BaseLang_Cancel;
    gcd[i++].creator = GButtonCreate;

    buttonarray[0] = GCD_Glue; buttonarray[1] = &gcd[i-2]; buttonarray[2] = GCD_Glue;
    buttonarray[3] = GCD_Glue; buttonarray[4] = &gcd[i-1]; buttonarray[5] = GCD_Glue;
    buttonarray[6] = NULL;
    box[3].gd.flags = gg_enabled|gg_visible;
    box[3].gd.u.boxelements = buttonarray;
    box[3].creator = GHBoxCreate;
    varray[j++] = &box[3]; varray[j++] = NULL; varray[j++] = NULL;

    box[0].gd.pos.x = box[0].gd.pos.y = 2;
    box[0].gd.flags = gg_enabled|gg_visible;
    box[0].gd.u.boxelements = varray;
    box[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,box);
    GHVBoxSetExpandableRow(box[0].ret,1);
    GHVBoxSetExpandableCol(box[3].ret,gb_expandgluesame);

    buttongcd[0].gd.flags = gg_visible;
    buttonlabel[0].text = (unichar_t *) S_("Set Feature Extents");
    buttonlabel[0].text_is_1byte = true;
    buttonlabel[0].text_in_resource = true;
    buttongcd[0].gd.label = &buttonlabel[0];
    buttongcd[0].gd.cid = CID_Extents;
    buttongcd[0].gd.handle_controlevent = BaseLang_Extents;
    buttongcd[0].creator = GButtonCreate;

    g = GWidgetGetControl(gw,CID_Languages);
    if ( islang ) {
	GMatrixEditAddButtons(g,buttongcd);
	GMatrixEditSetOtherButtonEnable(g,Base_EnableOtherButtons);
    }
    GMatrixEditShowColumn(g,3,false);

    GMatrixEditSetBeforeDelete(GWidgetGetControl(gw,CID_Languages),Base_DelClean);
    GHVBoxFitWindow(box[0].ret);

    GDrawSetVisible(gw,true);
    while ( !b.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return( b.old );
}

GTextInfo baselinetags[] = {
    { (unichar_t *) ("hang"), NULL, 0, 0, (void *) CHR('h','a','n','g'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) ("icfb"), NULL, 0, 0, (void *) CHR('i','c','f','b'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) ("icft"), NULL, 0, 0, (void *) CHR('i','c','f','t'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) ("ideo"), NULL, 0, 0, (void *) CHR('i','d','e','o'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) ("idtp"), NULL, 0, 0, (void *) CHR('i','d','t','p'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) ("math"), NULL, 0, 0, (void *) CHR('m','a','t','h'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) ("romn"), NULL, 0, 0, (void *) CHR('r','o','m','n'), NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static struct col_init baselinesci[10] = {
    { me_stringchoicetag , NULL, scripts, NULL, N_("writing system|Script") },
    { me_enum , NULL, baselinetags, NULL, N_("Default Baseline") },
    { me_int, NULL, NULL, NULL, "hang" },
    { me_int, NULL, NULL, NULL, "icfb" },
    { me_int, NULL, NULL, NULL, "icft" },
    { me_int, NULL, NULL, NULL, "ideo" },
    { me_int, NULL, NULL, NULL, "idtp" },
    { me_int, NULL, NULL, NULL, "math" },
    { me_int, NULL, NULL, NULL, "romn" },
    { me_int, NULL, NULL, NULL, "hidden" }	/* Used to hold per-script data */
    };
static uint32 stdtags[] = {
    CHR('h','a','n','g'), CHR('i','c','f','b'), CHR('i','c','f','t'), CHR('i','d','e','o'),
    CHR('i','d','t','p'), CHR('m','a','t','h'), CHR('r','o','m','n'), 0
};

static void Base_FinishEdit(GGadget *g, int r, int c, int wasnew) {

    if ( wasnew && c==0 ) {
	int rows, cols = GMatrixEditGetColCnt(g);
	struct matrix_data *md = GMatrixEditGet(g,&rows);
	uint32 script = TagFromString(md[r*cols+0].u.md_str);
	uint32 bsln;
	int i=0,j,k;

/* This if is duplicated (almost) in tottfaat.c: PerGlyphDefBaseline */
	if ( script==CHR('k','a','n','a') || script==CHR('h','a','n','g') ||
		script==CHR('h','a','n','i') || script==CHR('b','o','p','o') ||
		script==CHR('j','a','m','o') || script==CHR('y','i',' ',' '))
	    bsln = CHR('i','d','e','o');
	else if ( script==CHR('t','i','b','t' ) ||
		script == CHR('b','e','n','g' ) || script == CHR('b','n','g','2') ||
		script == CHR('d','e','v','a' ) || script == CHR('d','e','v','2') ||
		script == CHR('g','u','j','r' ) || script == CHR('g','j','r','2') ||
		script == CHR('g','u','r','u' ) || script == CHR('g','u','r','2') ||
		script == CHR('k','n','d','a' ) || script == CHR('k','n','d','2') ||
		script == CHR('m','l','y','m' ) || script == CHR('m','l','m','2') ||
		script == CHR('o','r','y','a' ) || script == CHR('o','r','y','2') ||
		script == CHR('t','a','m','l' ) || script == CHR('t','m','l','2') ||
		script == CHR('t','e','l','u' ) || script == CHR('t','e','l','2'))
	    bsln = CHR('h','a','n','g');
	else if ( script==CHR('m','a','t','h') )
	    bsln = CHR('m','a','t','h');
	else
	    bsln = CHR('r','o','m','n');
	md[r*cols+1].u.md_ival = bsln;
	for ( j=0; j<rows; ++j ) if ( j!=r ) {
	    if ( md[j*cols+1].u.md_ival == i ) {
		for ( k=1; k<cols-1; ++k )
		    md[r*cols+k].u.md_ival = md[j*cols+k].u.md_ival;
	break;
	    }
	}
    }
}
	
static void BaselineMatrixInit(struct matrixinit *mi,struct Base *old) {
    struct matrix_data *md;
    int k, i, cnt, mustfreemem;
    int _maps[20], *mapping;
    char script[8];
    struct basescript *bs;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 10;
    mi->col_init = baselinesci;

    cnt = 0;
    mustfreemem = 0;
    if ( old!=NULL )
	for ( cnt=0, bs=old->scripts; bs!=NULL; bs=bs->next, ++cnt );
    mi->initial_row_cnt = cnt;
    mi->matrix_data     = md = calloc(mi->col_cnt*cnt,sizeof(struct matrix_data));

    if ( old!=NULL ) {
	if ( old->baseline_cnt<sizeof(_maps)/sizeof(_maps[0]) )
	    mapping = _maps;
	else  {
            mustfreemem = 1;
	    mapping = malloc(old->baseline_cnt*sizeof(int));
        }
	for ( k=0; k<old->baseline_cnt; ++k ) {
	    mapping[k] = -1;
	    for ( i=0; stdtags[i]!=0; ++i )
		if ( old->baseline_tags[k]==stdtags[i]) mapping[k] = i+2;
	}
	for ( cnt=0, bs=old->scripts; bs!=NULL; bs=bs->next, ++cnt ) {
	    script[0] = bs->script>>24;
	    script[1] = bs->script>>16;
	    script[2] = bs->script>>8;
	    script[3] = bs->script;
	    script[4] = '\0';
	    md[mi->col_cnt*cnt+0].u.md_str = copy(script);
	    if ( old->baseline_cnt!=0 ) {
		md[mi->col_cnt*cnt+1].u.md_ival = old->baseline_tags[bs->def_baseline];
		for ( k=0; k<old->baseline_cnt; ++k ) if ( mapping[k]!=-1 )
		    md[mi->col_cnt*cnt+mapping[k]].u.md_ival = bs->baseline_pos[k];
	    }
	    md[mi->col_cnt*cnt+mi->col_cnt-1].u.md_str = (char *) BaseLangCopy(bs->langs);
	}
        if (mustfreemem != 0) 
            free(mapping);
    }

    mi->finishedit = Base_FinishEdit;
    mi->candelete = NULL;
    mi->popupmenu = NULL;
    mi->handle_key = NULL;
    mi->bigedittitle = NULL;
}


#define CID_BaseHang	1001
#define CID_BaseIcfb	1002
#define CID_BaseIcft	1003
#define CID_BaseIdeo	1004
#define CID_BaseIdtp	1005
#define CID_BaseMath	1006
#define CID_BaseRomn	1007
#define CID_Baselines	1008

static int Base_Extents(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	BaseDlg *b = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *gme = GWidgetGetControl(b->gw,CID_Baselines);
	int rows, cols = GMatrixEditGetColCnt(gme);
	struct matrix_data *md = _GMatrixEditGet(gme,&rows);
	int r = GMatrixEditGetActiveRow(gme);

	if ( r<0 || r>=rows )
return( true );
	md[r*cols+cols-1].u.md_str = (char *)
		SFBaselang(b->sf,(struct baselangextent *)md[r*cols+cols-1].u.md_str,
			b->vertical,true, TagFromString(md[r*cols+0].u.md_str));
    }
return( true );
}

static int Base_ChangeBase(GGadget *g, GEvent *e) {
    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	int k, any = false;
	GWindow gw = GGadgetGetWindow(g);
	GGadget *bases = GWidgetGetControl(gw,CID_Baselines);
	for ( k=CID_BaseHang; k<=CID_BaseRomn; ++k ) {
	    int checked = GGadgetIsChecked(GWidgetGetControl(gw,k));
	    GMatrixEditShowColumn(bases,k-CID_BaseHang+2, checked);
	    any |= checked;
	}
	
	GMatrixEditShowColumn(bases,1,any);
	GMatrixEditShowColumn(bases,9,false);
    }
return( true );
}

static void Base_DoCancel(BaseDlg *b) {
    GGadget *g = GWidgetGetControl(b->gw,CID_Baselines);
    int r, rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *md = _GMatrixEditGet(g,&rows);

    for ( r=0; r<rows; ++r ) {
	BaseLangFree( (struct baselangextent *) md[r*cols+cols-1].u.md_str);
	md[r*cols+cols-1].u.md_str = NULL;
    }
    b->done = true;
}

static int Base_Cancel(GGadget *g, GEvent *e) {
    BaseDlg *b;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	b = GDrawGetUserData(GGadgetGetWindow(g));
	Base_DoCancel(b);
    }
return( true );
}

static int Base_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	BaseDlg *b = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *g = GWidgetGetControl(b->gw,CID_Baselines);
	int r, rows, cols = GMatrixEditGetColCnt(g);
	struct matrix_data *md = GMatrixEditGet(g,&rows);
	int mapping[20], backmap[20];
	int i, j, cnt;
	struct basescript *bs, *last;

	if ( md==NULL )
return( true );

	i=0;
	mapping[0] = GGadgetIsChecked(GWidgetGetControl(b->gw,CID_BaseHang))? i++ : -1;
	mapping[1] = GGadgetIsChecked(GWidgetGetControl(b->gw,CID_BaseIcfb))? i++ : -1;
	mapping[2] = GGadgetIsChecked(GWidgetGetControl(b->gw,CID_BaseIcft))? i++ : -1;
	mapping[3] = GGadgetIsChecked(GWidgetGetControl(b->gw,CID_BaseIdeo))? i++ : -1;
	mapping[4] = GGadgetIsChecked(GWidgetGetControl(b->gw,CID_BaseIdtp))? i++ : -1;
	mapping[5] = GGadgetIsChecked(GWidgetGetControl(b->gw,CID_BaseMath))? i++ : -1;
	mapping[6] = GGadgetIsChecked(GWidgetGetControl(b->gw,CID_BaseRomn))? i++ : -1;
	cnt = i;
	memset(backmap,-1,sizeof(backmap));
	for ( j=0; j<7; ++j ) if ( mapping[j]!=-1 )
	    backmap[mapping[j]] = j;

	if ( cnt!=0 ) {
	    for ( r=0; r<rows; ++r ) {
		int tag = md[cols*r+1].u.md_ival;
		for ( j=0; stdtags[j]!=0 && stdtags[j]!=tag; ++j );
		if ( stdtags[j]==0 || mapping[ j ]==-1 ) {
		    ff_post_error(_("Bad default baseline"),_("Script '%c%c%c%c' claims baseline '%c%c%c%c' as its default, but that baseline is not currently active."),
			    md[cols*r+0].u.md_ival>>24, md[cols*r+0].u.md_ival>>16,
			    md[cols*r+0].u.md_ival>>8, md[cols*r+0].u.md_ival,
			    tag>>24, tag>>16, tag>>8, tag );
return( true );
		}
	    }
	}

	BaseFree(b->old);
	b->old = chunkalloc(sizeof(struct Base));

	b->old->baseline_cnt = cnt;
	if ( i!=0 ) {
	    b->old->baseline_tags = malloc(cnt*sizeof(uint32));
	    for ( i=0; stdtags[i]!=0 ; ++i ) if ( mapping[i]!=-1 )
		b->old->baseline_tags[mapping[i]] = stdtags[i];
	}
	last = NULL;
	for ( r=0; r<rows; ++r ) {
	    if ( cnt==0 && md[r*cols+cols-1].u.md_str==NULL )
	continue;
	    bs = chunkalloc(sizeof(struct basescript));
	    bs->script = TagFromString(md[r*cols+0].u.md_str);
	    if ( cnt!=0 ) {
		int tag = md[cols*r+1].u.md_ival;
		for ( j=0; stdtags[j]!=0 && stdtags[j]!=tag; ++j );
		bs->def_baseline = mapping[j];
		bs->baseline_pos = malloc(cnt*sizeof(int16));
		for ( i=0; stdtags[i]!=0 ; ++i ) if ( mapping[i]!=-1 )
		    bs->baseline_pos[mapping[i]] = md[r*cols+i+2].u.md_ival;
	    }
	    bs->langs = (struct baselangextent *) md[r*cols+cols-1].u.md_str;
	    md[r*cols+cols-1].u.md_str = NULL;
	    if ( last==NULL )
		b->old->scripts = bs;
	    else
		last->next = bs;
	    last = bs;
	}
	b->done = true;
    }
return( true );
}

static int base_e_h(GWindow gw, GEvent *event) {
    BaseDlg *b = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	Base_DoCancel(b);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/baseline.html", NULL);
return( true );
	}
return( false );
      break;
    }
return( true );
}

static void BaseUIInit(void) {
    static int inited = false;

    if ( inited )
return;

    baselinesci[0].title = S_(baselinesci[0].title);
    baselinesci[1].title = S_(baselinesci[1].title);
    baselangci[0].title = S_(baselangci[0].title);
    baselangci[1].title = S_(baselangci[1].title);
    baselangci[2].title = S_(baselangci[2].title);
    basefeatci[0].title = S_(basefeatci[0].title);
    basefeatci[1].title = S_(basefeatci[1].title);
    basefeatci[2].title = S_(basefeatci[2].title);
    inited = true;
}

struct Base *SFBaselines(SplineFont *sf,struct Base *old,int is_vertical) {
    int i,j,k;
    GRect pos;
    GWindowAttrs wattrs;
    BaseDlg b;
    GWindow gw;
    char buffer[200];
    GGadgetCreateData gcd[13], box[4], buttongcd[2];
    GGadgetCreateData *buttonarray[8], *varray[12], *hvarray[12];
    GTextInfo label[13], buttonlabel[1];
    struct matrixinit mi;
    GGadget *g;

    memset(&b,0,sizeof(b));
    b.sf = sf;
    b.sf = sf;
    b.old = old;
    b.vertical = is_vertical;

    memset(&wattrs,0,sizeof(wattrs));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    snprintf(buffer,sizeof(buffer),"%s", is_vertical ? _("Vertical Baselines") : _("Horizontal Baselines") );
    wattrs.utf8_window_title = buffer;
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,300));
    pos.height = GDrawPointsToPixels(NULL,200);
    b.gw = gw = GDrawCreateTopWindow(NULL,&pos,base_e_h,&b,&wattrs);

    LookupUIInit();
    BaseUIInit();

    memset(&gcd,0,sizeof(gcd));
    memset(&buttongcd,0,sizeof(buttongcd));
    memset(&box,0,sizeof(box));
    memset(&label,0,sizeof(label));
    memset(&buttonlabel,0,sizeof(buttonlabel));

    i = j = k = 0;
    label[i].text = (unichar_t *) _("From the list below, select the baselines for which you\nwill provide data.");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5+4; 
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].creator = GLabelCreate;
    varray[j++] = &gcd[i++]; varray[j++] = NULL;

    label[i].text = (unichar_t *) _("hang");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.popup_msg = _("Indic (& Tibetan) hanging baseline");
    gcd[i].gd.cid = CID_BaseHang;
    gcd[i].gd.handle_controlevent = Base_ChangeBase;
    gcd[i].creator = GCheckBoxCreate;
    hvarray[k++] = &gcd[i++];

    label[i].text = (unichar_t *) _("icfb");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.popup_msg = _("Ideographic character face bottom edge baseline");
    gcd[i].gd.cid = CID_BaseIcfb;
    gcd[i].gd.handle_controlevent = Base_ChangeBase;
    gcd[i].creator = GCheckBoxCreate;
    hvarray[k++] = &gcd[i++];

    label[i].text = (unichar_t *) _("icft");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.popup_msg = _("Ideographic character face top edge baseline");
    gcd[i].gd.cid = CID_BaseIcft;
    gcd[i].gd.handle_controlevent = Base_ChangeBase;
    gcd[i].creator = GCheckBoxCreate;
    hvarray[k++] = &gcd[i++];

    label[i].text = (unichar_t *) _("ideo");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.popup_msg = _("Ideographic em-box bottom edge baseline");
    gcd[i].gd.cid = CID_BaseIdeo;
    gcd[i].gd.handle_controlevent = Base_ChangeBase;
    gcd[i].creator = GCheckBoxCreate;
    hvarray[k++] = &gcd[i++]; hvarray[k++] = NULL;

    label[i].text = (unichar_t *) _("idtp");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.popup_msg = _("Ideographic em-box top edge baseline");
    gcd[i].gd.cid = CID_BaseIdtp;
    gcd[i].gd.handle_controlevent = Base_ChangeBase;
    gcd[i].creator = GCheckBoxCreate;
    hvarray[k++] = &gcd[i++];

    label[i].text = (unichar_t *) _("math");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.popup_msg = _("Mathematical centerline");
    gcd[i].gd.cid = CID_BaseMath;
    gcd[i].gd.handle_controlevent = Base_ChangeBase;
    gcd[i].creator = GCheckBoxCreate;
    hvarray[k++] = &gcd[i++];

    label[i].text = (unichar_t *) _("romn");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.popup_msg = _("Baseline used for Latin, Greek, Cyrillic text.");
    gcd[i].gd.cid = CID_BaseRomn;
    gcd[i].gd.handle_controlevent = Base_ChangeBase;
    gcd[i].creator = GCheckBoxCreate;
    hvarray[k++] = &gcd[i++];
    hvarray[k++] = GCD_Glue; hvarray[k++] = NULL; hvarray[k++] = NULL;

    box[2].gd.flags = gg_enabled|gg_visible;
    box[2].gd.u.boxelements = hvarray;
    box[2].creator = GHVBoxCreate;
    varray[j++] = &box[2]; varray[j++] = NULL;

    label[i].text = (unichar_t *) _(
	    "If any of the above baselines are active then you should\n"
	    "specify which one is the default baseline for each script\n"
	    "in the font, and specify how to position glyphs in this\n"
	    "script relative to all active baselines");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5+4; 
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].creator = GLabelCreate;
    varray[j++] = &gcd[i++]; varray[j++] = NULL;

    BaselineMatrixInit(&mi,old);
    gcd[i].gd.pos.width = 300; gcd[i].gd.pos.height = 200;
    gcd[i].gd.flags = gg_enabled | gg_visible;
    gcd[i].gd.cid = CID_Baselines;
    gcd[i].gd.u.matrix = &mi;
    gcd[i++].creator = GMatrixEditCreate;
    varray[j++] = &gcd[i-1]; varray[j++] = NULL;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = Base_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = Base_Cancel;
    gcd[i++].creator = GButtonCreate;

    buttonarray[0] = GCD_Glue; buttonarray[1] = &gcd[i-2]; buttonarray[2] = GCD_Glue;
    buttonarray[3] = GCD_Glue; buttonarray[4] = &gcd[i-1]; buttonarray[5] = GCD_Glue;
    buttonarray[6] = NULL;
    box[3].gd.flags = gg_enabled|gg_visible;
    box[3].gd.u.boxelements = buttonarray;
    box[3].creator = GHBoxCreate;
    varray[j++] = &box[3]; varray[j++] = NULL; varray[j++] = NULL;

    box[0].gd.pos.x = box[0].gd.pos.y = 2;
    box[0].gd.flags = gg_enabled|gg_visible;
    box[0].gd.u.boxelements = varray;
    box[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,box);
    GHVBoxSetExpandableRow(box[0].ret,3);
    GHVBoxSetExpandableCol(box[3].ret,gb_expandgluesame);

    buttongcd[0].gd.flags = gg_visible;
    buttonlabel[0].text = (unichar_t *) S_("Set Extents");
    buttonlabel[0].text_is_1byte = true;
    buttonlabel[0].text_in_resource = true;
    buttongcd[0].gd.label = &buttonlabel[0];
    buttongcd[0].gd.cid = CID_Extents;
    buttongcd[0].gd.handle_controlevent = Base_Extents;
    buttongcd[0].creator = GButtonCreate;

    g = GWidgetGetControl(gw,CID_Baselines);
    GMatrixEditAddButtons(g,buttongcd);
    GMatrixEditSetOtherButtonEnable(g,Base_EnableOtherButtons);

    GMatrixEditSetBeforeDelete(GWidgetGetControl(gw,CID_Baselines),Base_DelClean);
    if ( old!=NULL ) {
	for ( k=0; k<old->baseline_cnt; ++k ) {
	    for ( i=0; stdtags[i]!=0; ++i )
		if ( old->baseline_tags[k] == stdtags[i] ) {
		    GGadgetSetChecked(GWidgetGetControl(gw,CID_BaseHang+i),true);
	    break;
		}
	}
    }
    Base_ChangeBase(GWidgetGetControl(gw,CID_BaseHang), NULL);
    GHVBoxFitWindow(box[0].ret);

    GDrawSetVisible(gw,true);
    while ( !b.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return( b.old );
}
