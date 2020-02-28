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

#include <fontforge-config.h>

#include "chardata.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "lookups.h"
#include "splineutil.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>

extern GTextInfo scripts[], languages[];

static char *JSTF_GlyphDlg(GGadget *, int r, int c);
static char *JSTF_LookupListDlg(GGadget *, int r, int c);
static char *JSTF_Langs(GGadget *, int r, int c);

static struct col_init glyph_ci[] = {
    { me_string , NULL, NULL, NULL, N_("Glyph Names") },
    COL_INIT_EMPTY
};

static struct col_init lookup_ci[] = {
    { me_enum , NULL, NULL, NULL, N_("Lookups") },
    COL_INIT_EMPTY
};

static struct col_init jstf_lang_ci[] = {
    { me_stringchoicetag , NULL, languages, NULL, N_("Language") },
    { me_funcedit, JSTF_LookupListDlg, NULL, NULL, N_("Extend Lookups On") },
    { me_funcedit, JSTF_LookupListDlg, NULL, NULL, N_("Extend Lookups Off") },
    { me_funcedit, JSTF_LookupListDlg, NULL, NULL, N_("Extend Max Lookups") },
    { me_funcedit, JSTF_LookupListDlg, NULL, NULL, N_("Shrink Lookups On") },
    { me_funcedit, JSTF_LookupListDlg, NULL, NULL, N_("Shrink Lookups Off") },
    { me_funcedit, JSTF_LookupListDlg, NULL, NULL, N_("Shrink Max Lookups") },
    COL_INIT_EMPTY
};

static struct col_init justify_ci[] = {
    { me_stringchoicetag , NULL, scripts, NULL, N_("writing system|Script") },
    { me_funcedit, JSTF_GlyphDlg, NULL, NULL, N_("Extenders") },
    { me_button, JSTF_Langs, NULL, NULL, N_("Language info") },
    { me_addr, NULL, NULL, NULL, N_("Hidden") },
    COL_INIT_EMPTY
};

struct glyph_list_dlg {
    int done;
    char *ret;
    GWindow gw;
    SplineFont *sf;
};

typedef struct jstf_dlg {
    int done, ldone;
    GWindow gw, lgw;
    SplineFont *sf;
    struct jstf_lang **here;
} Jstf_Dlg;

#define CID_OK		1001
#define CID_Cancel	1002

#define CID_Glyphs	2001
#define CID_Lookups	2002
#define CID_Languages	2003
#define CID_Scripts	2004

static void JustUIInit(void) {
    static int done = false;
    int i, j;
    static struct col_init *needswork[] = {
	justify_ci, jstf_lang_ci, lookup_ci, glyph_ci,
	NULL
    };

    if ( done )
return;
    done = true;
    for ( j=0; needswork[j]!=NULL; ++j ) {
	for ( i=0; needswork[j][i].title!=NULL; ++i )
	    if ( needswork[j][i].title!=NULL )
		needswork[j][i].title = S_(needswork[j][i].title);
    }
}

/* ************************************************************************** */
/* dlg which presents a list of glyph names                                   */
/* ************************************************************************** */

static unichar_t **JSTF_Glyph_Completion(GGadget *t,int from_tab) {
    struct glyph_list_dlg *gld = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = gld->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,false));
}

static void GlyphMatrixInit(struct matrixinit *mi,char *glyphstr,SplineFont *sf) {
    struct matrix_data *md;
    int k, cnt;
    char *start, *end;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 1;
    mi->col_init = glyph_ci;

    md = NULL;
    for ( k=0; k<2; ++k ) {
	cnt = 0;
	if ( glyphstr!=NULL ) for ( start = glyphstr; *start; ) {
	    for ( end=start; *end!='\0' && *end!=' ' && *end!=','; ++end );
	    if ( k ) {
		char *str = copyn(start,end-start);
		md[1*cnt+0].u.md_str = SFNameList2NameUni(sf,str);
		free(str);
	    }
	    ++cnt;
	    while ( *end==' ' || *end==',' ) ++end;
	    start = end;
	}
	if ( md==NULL )
	    md = calloc(1*(cnt+10),sizeof(struct matrix_data));
    }
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;
}

static int JSTF_Glyph_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct glyph_list_dlg *gld = GDrawGetUserData(GGadgetGetWindow(g));
	int rows, i, len;
	struct matrix_data *strings = GMatrixEditGet(GWidgetGetControl(gld->gw,CID_Glyphs), &rows);
	char *ret;

	if ( rows==0 )
	    gld->ret = NULL;
	else {
	    len = 0;
	    for ( i=0; i<rows; ++i )
		len += strlen(strings[1*i+0].u.md_str) +1;
	    ret = malloc(len+1);
	    for ( i=0; i<rows; ++i ) {
		strcpy(ret,strings[1*i+0].u.md_str);
		strcat(ret," ");
		ret += strlen(ret);
	    }
	    if ( ret>gld->ret && ret[-1] == ' ' )
		ret[-1] = '\0';
	    gld->ret = GlyphNameListDeUnicode(ret);
	    free(ret);
	}
	gld->done = true;
    }
return( true );
}

static int JSTF_Glyph_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct glyph_list_dlg *gld = GDrawGetUserData(GGadgetGetWindow(g));
	gld->done = true;
	gld->ret = NULL;
    }
return( true );
}

static int glyph_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	struct glyph_list_dlg *gld = GDrawGetUserData(gw);
	gld->done = true;
	gld->ret = NULL;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/justify.html", "#justify-glyphs-dlg");
return( true );
	}
return( false );
    }

return( true );
}

char *GlyphListDlg(SplineFont *sf, char *glyphstr) {
    int i, k, j;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[4], boxes[3];
    GGadgetCreateData *varray[6], *harray3[8];
    GTextInfo label[4];
    struct matrixinit mi;
    struct glyph_list_dlg gld;

    memset(&gld,0,sizeof(gld));
    gld.sf = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Extender Glyphs (kashidas, etc.)");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268));
    pos.height = GDrawPointsToPixels(NULL,375);
    gld.gw = gw = GDrawCreateTopWindow(NULL,&pos,glyph_e_h,&gld,&wattrs);

    GlyphMatrixInit(&mi,glyphstr,sf);

    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&label,0,sizeof(label));
    k=j=0;
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_Glyphs;
    gcd[k].gd.u.matrix = &mi;
    gcd[k].gd.popup_msg = _( "A list of glyph names");
    gcd[k].creator = GMatrixEditCreate;
    varray[j++] = &gcd[k++]; varray[j++] = NULL;

    gcd[k].gd.pos.x = 30-3; 
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = JSTF_Glyph_OK;
    gcd[k].gd.cid = CID_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -30;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = JSTF_Glyph_Cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray3[0] = harray3[2] = harray3[3] = harray3[4] = harray3[6] = GCD_Glue;
    harray3[7] = NULL;
    harray3[1] = &gcd[k-2]; harray3[5] = &gcd[k-1];

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = harray3;
    boxes[0].creator = GHBoxCreate;
    varray[j++] = &boxes[0]; varray[j++] = NULL; varray[j] = NULL;
    
    boxes[1].gd.pos.x = boxes[1].gd.pos.y = 2;
    boxes[1].gd.flags = gg_enabled|gg_visible;
    boxes[1].gd.u.boxelements = varray;
    boxes[1].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes+1);

    for ( i=0; i<mi.initial_row_cnt; ++i ) {
	free( mi.matrix_data[2*i+0].u.md_str );
    }
    free( mi.matrix_data );

    GMatrixEditSetNewText(gcd[0].ret,S_("GlyphName|New"));
    GMatrixEditSetColumnCompletion(gcd[0].ret, 0, JSTF_Glyph_Completion );
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);

    GHVBoxFitWindow(boxes[1].ret);

    GDrawSetVisible(gw,true);
    while ( !gld.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return( gld.ret );
}

static char *JSTF_GlyphDlg(GGadget *g, int r, int c) {
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    int cols = GMatrixEditGetColCnt(g);
    char *glyphstr = strings[cols*r+c].u.md_str;
    Jstf_Dlg *jd = GDrawGetUserData(GGadgetGetWindow(g));

return( GlyphListDlg(jd->sf,glyphstr));
}

/* ************************************************************************** */
/* dlg which presents a list of lookups                                       */
/* ************************************************************************** */

static void LookupMatrixInit(struct matrixinit *mi,char *lookupstr,
	SplineFont *sf, int col) {
    struct matrix_data *md;
    int j, k, cnt;
    char *temp;
    char *start, *end;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 1;
    mi->col_init = lookup_ci;

    lookup_ci[0].enum_vals = SFLookupArrayFromMask(sf,col==3 || col==6 ? (gpos_single_mask|gpos_pair_mask) : 0 );

    md = NULL;
    for ( k=0; k<2; ++k ) {
	cnt = 0;
	if ( lookupstr!=NULL ) for ( start = lookupstr; *start; ) {
	    for ( end=start; *end!='\0' && *end!=','; ++end );
	    if ( k ) {
		for ( j=0; (temp=(char *) (lookup_ci[0].enum_vals[j].text))!=NULL; ++j )
		    if ( strlen(temp)==end-start && strncmp(temp,start,end-start)==0 ) {
			md[1*cnt++ + 0].u.md_ival = (intpt) lookup_ci[0].enum_vals[j].userdata;
		break;
		    }
	    } else
		++cnt;
	    while ( *end==' ' || *end==',') ++end;
	    start = end;
	}
	if ( md==NULL )
	    md = calloc(1*(cnt+10),sizeof(struct matrix_data));
    }
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;
}

static int JSTF_Lookup_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct glyph_list_dlg *gld = GDrawGetUserData(GGadgetGetWindow(g));
	int rows, i, len;
	struct matrix_data *strings = GMatrixEditGet(GWidgetGetControl(gld->gw,CID_Lookups), &rows);
	char *ret;

	if ( rows==0 )
	    gld->ret = NULL;
	else {
	    len = 0;
	    for ( i=0; i<rows; ++i ) {
		OTLookup *otl = (OTLookup *) strings[1*i+0].u.md_ival;
		len += strlen(otl->lookup_name) +2;
	    }
	    gld->ret = ret = malloc(len+1);
	    for ( i=0; i<rows; ++i ) {
		OTLookup *otl = (OTLookup *) strings[1*i+0].u.md_ival;
		strcpy(ret,otl->lookup_name);
		strcat(ret,", ");
		ret += strlen(ret);
	    }
	    if ( ret>gld->ret && ret[-1] == ' ' )
		*--ret = '\0';
	    if ( ret>gld->ret && ret[-1] == ',' )
		*--ret = '\0';
	}
	gld->done = true;
    }
return( true );
}

static int lookup_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	struct glyph_list_dlg *gld = GDrawGetUserData(gw);
	gld->done = true;
	gld->ret = NULL;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/justify.html", "#justify-lookup-dlg");
return( true );
	}
return( false );
    }

return( true );
}

static char *JSTF_LookupListDlg(GGadget *g, int r, int c) {
    int rows, k, j;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    int cols = GMatrixEditGetColCnt(g);
    char *lookupstr = strings[cols*r+c].u.md_str;
    Jstf_Dlg *jd = GDrawGetUserData(GGadgetGetWindow(g));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[4], boxes[3];
    GGadgetCreateData *varray[6], *harray3[8];
    GTextInfo label[4];
    struct matrixinit mi;
    struct glyph_list_dlg gld;

    memset(&gld,0,sizeof(gld));
    gld.sf = jd->sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title =
	    c==1 ? _("Lookups turned ON to extend a line") :
	    c==2 ? _("Lookups turned OFF to extend a line") :
	    c==3 ? _("Lookups which specify the maximum size by which a glyph may grow") :
	    c==4 ? _("Lookups turned ON to shrink a line") :
	    c==5 ? _("Lookups turned OFF to shrink a line") :
	           _("Lookups which specify the maximum size by which a glyph may shrink");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268));
    pos.height = GDrawPointsToPixels(NULL,375);
    gld.gw = gw = GDrawCreateTopWindow(NULL,&pos,lookup_e_h,&gld,&wattrs);

    LookupMatrixInit(&mi,lookupstr,gld.sf,c);

    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&label,0,sizeof(label));
    k=j=0;
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_Lookups;
    gcd[k].gd.u.matrix = &mi;
    gcd[k].gd.popup_msg = _( "A list of lookup names");
    gcd[k].creator = GMatrixEditCreate;
    varray[j++] = &gcd[k++]; varray[j++] = NULL;

    gcd[k].gd.pos.x = 30-3; 
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = JSTF_Lookup_OK;
    gcd[k].gd.cid = CID_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -30;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = JSTF_Glyph_Cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray3[0] = harray3[2] = harray3[3] = harray3[4] = harray3[6] = GCD_Glue;
    harray3[7] = NULL;
    harray3[1] = &gcd[k-2]; harray3[5] = &gcd[k-1];

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = harray3;
    boxes[0].creator = GHBoxCreate;
    varray[j++] = &boxes[0]; varray[j++] = NULL; varray[j] = NULL;
    
    boxes[1].gd.pos.x = boxes[1].gd.pos.y = 2;
    boxes[1].gd.flags = gg_enabled|gg_visible;
    boxes[1].gd.u.boxelements = varray;
    boxes[1].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes+1);

    free( mi.matrix_data );

    GMatrixEditSetNewText(gcd[0].ret,S_("LookupName|New"));
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);

    GHVBoxFitWindow(boxes[1].ret);

    GDrawSetVisible(gw,true);
    while ( !gld.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GTextInfoListFree(lookup_ci[0].enum_vals);
    lookup_ci[0].enum_vals = NULL;
return( gld.ret );
}

/* ************************************************************************** */
/* dlg which presents a list of languages and associated lookups              */
/* ************************************************************************** */
static char *Tag2Str(uint32 tag) {
    static char foo[8];
    foo[0] = tag>>24;
    foo[1] = tag>>16;
    foo[2] = tag>>8;
    foo[3] = tag;
    foo[4] = 0;
return( foo );
}

static uint32 Str2Tag(char *str) {
    unsigned char foo[4];

    memset(foo,' ',4);
    if ( str!=NULL && *str!='\0' ) {
	foo[0] = *str;
	if ( str[1]!='\0' ) {
	    foo[1] = str[1];
	    if ( str[2]!='\0' ) {
		foo[2] = str[2];
		if ( str[3]!='\0' )
		    foo[3] = str[3];
	    }
	}
    }
return( (foo[0]<<24) | (foo[1]<<16) | (foo[2]<<8) | foo[3] );
}

static char *OTList2Str(OTLookup **otll) {
    char *ret, *pt;
    int len, i;

    if ( otll==NULL )
return( NULL );
    len = 0;
    for ( i=0; otll[i]!=NULL; ++i )
	len += strlen(otll[i]->lookup_name) +2;
    if ( len==0 )
return( copy( "" ));

    ret = pt = malloc(len+1);
    for ( i=0; otll[i]!=NULL; ++i ) {
	strcpy(pt,otll[i]->lookup_name);
	strcat(pt,", ");
	pt += strlen(pt);
    }
    pt[-2] = '\0';
return( ret );
}

static OTLookup **Str2OTLList(SplineFont *sf,char *str ) {
    int cnt;
    char *pt, *ept;
    OTLookup **ret;

    if ( str==NULL )
return( NULL );
    while ( *str==' ' )
	++str;
    if ( *str=='\0' )
return( NULL );

    pt = str;
    cnt = 0;
    while ( (pt = strchr(pt+1,','))!=NULL )
	++cnt;

    ret = malloc( (cnt+2)*sizeof(OTLookup *));
    pt = str;
    cnt = 0;
    while ( (ept = strchr(pt,','))!=NULL ) {
	*ept = '\0';
	ret[cnt] = SFFindLookup(sf,pt);
	if ( ret[cnt]==NULL ) {
	    ff_post_error(_("Unknown lookup"), _("Unknown lookup name: %60.60s"),
		pt );
	    *ept = ',';
	    free(ret);
return( (OTLookup **) -1 );
	}
	*ept = ',';
	++cnt;
	pt = ept;
	while ( *pt==',' || *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
    }
    if ( *pt!='\0' ) {
	ret[cnt] = SFFindLookup(sf,pt);
	if ( ret[cnt]==NULL ) {
	    ff_post_error(_("Unknown lookup"), _("Unknown lookup name: %60.60s"),
		pt );
	    free(ret);
return( (OTLookup **) -1 );
	}
	++cnt;
    }
    ret[cnt] = NULL;
return( ret );
}
    
static void JLanguageMatrixInit(struct matrixinit *mi, struct jstf_lang *jl) {
    struct matrix_data *md;
    int i, cnt;
    struct jstf_lang *jlang;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 7;
    mi->col_init = jstf_lang_ci;

    md = NULL;
    cnt = 0;
    for ( jlang = jl; jlang!=NULL; jlang=jlang->next )
	cnt += jlang->cnt;
    md = calloc(mi->col_cnt*(cnt+10),sizeof(struct matrix_data));
    cnt = 0;
    for ( jlang = jl; jlang!=NULL; jlang=jlang->next ) {
	for ( i=0; i<jlang->cnt; ++i ) {
	    md[7*cnt+0].u.md_str = copy(Tag2Str(jl->lang));
	    md[7*cnt+1].u.md_str  = OTList2Str(jl->prios[i].enableExtend);
	    md[7*cnt+2].u.md_str  = OTList2Str(jl->prios[i].disableExtend);
	    md[7*cnt+3].u.md_str  = OTList2Str(jl->prios[i].maxExtend);
	    md[7*cnt+4].u.md_str  = OTList2Str(jl->prios[i].enableShrink);
	    md[7*cnt+5].u.md_str  = OTList2Str(jl->prios[i].disableShrink);
	    md[7*cnt+6].u.md_str  = OTList2Str(jl->prios[i].maxShrink);
	    ++cnt;
	}
    }
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;
}

static int JSTF_Language_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	Jstf_Dlg *jd = GDrawGetUserData(GGadgetGetWindow(g));
	int rows, i, j, k, cnt;
	struct matrix_data *strings = GMatrixEditGet(GWidgetGetControl(jd->lgw,CID_Languages), &rows);
	int cols = GMatrixEditGetColCnt(GWidgetGetControl(jd->lgw,CID_Languages));
	struct jstf_lang *head=NULL, *last=NULL, *cur;

	for ( i=0; i<rows; ++i ) if ( !(strings[i*cols+0].u.md_str[0]&0x80) ) {
	    for ( j=i, cnt=0; j<rows; ++j )
		if ( strcmp(strings[j*cols+0].u.md_str, strings[i*cols+0].u.md_str)==0 )
		    ++cnt;
	    cur = chunkalloc(sizeof(struct jstf_lang));
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	    cur->lang = Str2Tag(strings[i*cols+0].u.md_str);
	    cur->cnt  = cnt;
	    cur->prios=calloc(cnt,sizeof(struct jstf_prio));
	    for ( j=i, cnt=0; j<rows; ++j ) {
		if ( strcmp(strings[j*cols+0].u.md_str, strings[i*cols+0].u.md_str)==0 ) {
		    cur->prios[cnt].enableExtend = Str2OTLList(jd->sf,strings[j*cols+1].u.md_str );
		    cur->prios[cnt].disableExtend = Str2OTLList(jd->sf,strings[j*cols+2].u.md_str );
		    cur->prios[cnt].maxExtend = Str2OTLList(jd->sf,strings[j*cols+3].u.md_str );
		    cur->prios[cnt].enableShrink = Str2OTLList(jd->sf,strings[j*cols+4].u.md_str );
		    cur->prios[cnt].disableShrink = Str2OTLList(jd->sf,strings[j*cols+5].u.md_str );
		    cur->prios[cnt].maxShrink = Str2OTLList(jd->sf,strings[j*cols+6].u.md_str );
		    if ( cur->prios[cnt].enableExtend == (OTLookup **) -1 ||
			    cur->prios[cnt].disableExtend == (OTLookup **) -1 ||
			    cur->prios[cnt].maxExtend == (OTLookup **) -1 ||
			    cur->prios[cnt].enableShrink == (OTLookup **) -1 ||
			    cur->prios[cnt].disableShrink == (OTLookup **) -1 ||
			    cur->prios[cnt].maxShrink == (OTLookup **) -1 ) {
			JstfLangFree(head);
			for ( k=0; k<rows; ++k )
			    strings[k*cols+0].u.md_str[0] &= ~0x80;
return( true );
		    }
		    ++cnt;
		}
	    }
	    for ( j=rows-1; j>=i; --j )
		if ( strcmp(strings[j*cols+0].u.md_str, strings[i*cols+0].u.md_str)==0 )
		    strings[j*cols+0].u.md_str[0] |= 0x80;
	}
	JstfLangFree( *jd->here );
	*jd->here = head;
	jd->ldone = true;
    }
return( true );
}

static int JSTF_Language_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	Jstf_Dlg *jd = GDrawGetUserData(GGadgetGetWindow(g));
	jd->ldone = true;
    }
return( true );
}

static int langs_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	Jstf_Dlg *jd = GDrawGetUserData(gw);
	jd->ldone = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/justify.html", "#justify-language-dlg");
return( true );
	}
return( false );
    }

return( true );
}

static char *JSTF_Langs(GGadget *g, int r, int c) {
    int rows, i, k, j;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    int cols = GMatrixEditGetColCnt(g);
    Jstf_Dlg *jd = GDrawGetUserData(GGadgetGetWindow(g));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[4], boxes[3];
    GGadgetCreateData *varray[6], *harray3[8];
    GTextInfo label[4];
    struct matrixinit mi;

    jd->ldone = false;
    jd->here = (struct jstf_lang **) &strings[cols*r+3].u.md_addr;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Justified Languages");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268));
    pos.height = GDrawPointsToPixels(NULL,375);
    jd->lgw = gw = GDrawCreateTopWindow(NULL,&pos,langs_e_h,jd,&wattrs);

    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&label,0,sizeof(label));
    k=j=0;
    JLanguageMatrixInit(&mi,*jd->here);
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[1].gd.pos.y+14;
    gcd[k].gd.pos.width = 900;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_Languages;
    gcd[k].gd.u.matrix = &mi;
    gcd[k].gd.popup_msg = _(
	"A list of languages and the lookups turned on and off\n"
	"for each to accomplish justification.  A language may\n"
	"appear more than once, in which case second (or third,\n"
	"etc.) will be tried if the first fails.");
    gcd[k].creator = GMatrixEditCreate;
    varray[j++] = &gcd[k++]; varray[j++] = NULL;

    gcd[k].gd.pos.x = 30-3; 
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = JSTF_Language_OK;
    gcd[k].gd.cid = CID_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -30;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = JSTF_Language_Cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray3[0] = harray3[2] = harray3[3] = harray3[4] = harray3[6] = GCD_Glue;
    harray3[7] = NULL;
    harray3[1] = &gcd[k-2]; harray3[5] = &gcd[k-1];

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = harray3;
    boxes[0].creator = GHBoxCreate;
    varray[j++] = &boxes[0]; varray[j++] = NULL; varray[j] = NULL;
    
    boxes[1].gd.pos.x = boxes[1].gd.pos.y = 2;
    boxes[1].gd.flags = gg_enabled|gg_visible;
    boxes[1].gd.u.boxelements = varray;
    boxes[1].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes+1);

    for ( i=0; i<mi.initial_row_cnt; ++i ) {
	free( mi.matrix_data[2*i+0].u.md_str );
    }
    free( mi.matrix_data );

    GMatrixEditSetNewText(gcd[0].ret,S_("Language|New"));
    GMatrixEditSetUpDownVisible(gcd[0].ret,true);
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);

    GHVBoxFitWindow(boxes[1].ret);

    GDrawSetVisible(gw,true);
    while ( !jd->ldone )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return( NULL );
}

/* ************************************************************************** */
/* dlg which presents a list of scripts and associated information            */
/* ************************************************************************** */

static void JScriptMatrixInit(struct matrixinit *mi,Justify *js) {
    struct matrix_data *md;
    int k, cnt;
    Justify *jscript;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 4;
    mi->col_init = justify_ci;

    md = NULL;
    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( jscript=js; jscript!=NULL; jscript = jscript->next ) {
	    if ( k ) {
		md[4*cnt+0].u.md_str = copy(Tag2Str(jscript->script));
		md[4*cnt+1].u.md_str  = copy(jscript->extenders);
		/* Nothing in 4*cnt+2 */
		md[4*cnt+3].u.md_addr = JstfLangsCopy(jscript->langs);
	    }
	    ++cnt;
	}
	if ( md==NULL )
	    md = calloc(4*(cnt+10),sizeof(struct matrix_data));
    }
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;
}

static int JSTF_Script_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	Jstf_Dlg *jd = GDrawGetUserData(GGadgetGetWindow(g));
	SplineFont *sf = jd->sf;
	Justify *head=NULL, *last=NULL, *cur;
	int rows, i;
	int cols = GMatrixEditGetColCnt(GWidgetGetControl(jd->gw,CID_Scripts));
	struct matrix_data *strings = GMatrixEditGet(GWidgetGetControl(jd->gw,CID_Scripts), &rows);

	for ( i=0; i<rows; ++i ) {
	    cur = chunkalloc(sizeof(Justify));
	    cur->script = Str2Tag(strings[cols*i+0].u.md_str);
	    cur->extenders = copy(strings[cols*i+1].u.md_str);
	    cur->langs = strings[cols*i+3].u.md_addr;
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
	JustifyFree(sf->justify);
	sf->justify = head;
	jd->done = true;
    }
return( true );
}

static void Jstf_Cancel(Jstf_Dlg *jd) {
    int rows, i;
    int cols = GMatrixEditGetColCnt(GWidgetGetControl(jd->gw,CID_Scripts));
    struct matrix_data *strings = GMatrixEditGet(GWidgetGetControl(jd->gw,CID_Scripts), &rows);

    for ( i=0; i<rows; ++i ) {
	JstfLangFree(strings[cols*i+3].u.md_addr);
    }
    jd->done = true;
}

static int Justify_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	Jstf_Dlg *jd = GDrawGetUserData(GGadgetGetWindow(g));
	Jstf_Cancel(jd);
    }
return( true );
}

static int jscripts_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	Jstf_Dlg *jd = GDrawGetUserData(gw);
	Jstf_Cancel(jd);
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/justify.html", NULL);
return( true );
	}
return( false );
    }

return( true );
}

void JustifyDlg(SplineFont *sf) {
    Jstf_Dlg jd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[4], boxes[3];
    GGadgetCreateData *varray[6], *harray3[8];
    GTextInfo label[4];
    struct matrixinit mi;
    int i,j,k;

    LookupUIInit();
    JustUIInit();

    memset(&jd,0,sizeof(jd));
    jd.sf = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Justified Scripts");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268));
    pos.height = GDrawPointsToPixels(NULL,375);
    jd.gw = gw = GDrawCreateTopWindow(NULL,&pos,jscripts_e_h,&jd,&wattrs);

    JScriptMatrixInit(&mi,sf->justify);

    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&label,0,sizeof(label));
    k=j=0;
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_Scripts;
    gcd[k].gd.u.matrix = &mi;
    gcd[k].gd.popup_msg = _( "A list of scripts with special justification needs");
    gcd[k].creator = GMatrixEditCreate;
    varray[j++] = &gcd[k++]; varray[j++] = NULL;

    gcd[k].gd.pos.x = 30-3; 
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = JSTF_Script_OK;
    gcd[k].gd.cid = CID_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -30;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Justify_Cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray3[0] = harray3[2] = harray3[3] = harray3[4] = harray3[6] = GCD_Glue;
    harray3[7] = NULL;
    harray3[1] = &gcd[k-2]; harray3[5] = &gcd[k-1];

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = harray3;
    boxes[0].creator = GHBoxCreate;
    varray[j++] = &boxes[0]; varray[j++] = NULL; varray[j] = NULL;
    
    boxes[1].gd.pos.x = boxes[1].gd.pos.y = 2;
    boxes[1].gd.flags = gg_enabled|gg_visible;
    boxes[1].gd.u.boxelements = varray;
    boxes[1].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes+1);

    for ( i=0; i<mi.initial_row_cnt; ++i ) {
	free( mi.matrix_data[2*i+0].u.md_str );
    }
    free( mi.matrix_data );

    GMatrixEditSetNewText(gcd[0].ret,S_("Script|New"));
    GMatrixEditSetUpDownVisible(gcd[0].ret,true);
    GMatrixEditShowColumn(gcd[0].ret,3,false);
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);

    GHVBoxFitWindow(boxes[1].ret);

    GDrawSetVisible(gw,true);
    while ( !jd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
