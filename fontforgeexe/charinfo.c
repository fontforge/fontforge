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

#include "fontforgeui.h"
#include "cvundoes.h"
#include <ustring.h>
#include <math.h>
#include <utype.h>
#include <chardata.h>
#include "ttf.h"		/* For MAC_DELETED_GLYPH_NAME */
#include <gkeysym.h>
#include "gutils/unicodelibinfo.h"

extern int lookup_hideunused;

static int last_gi_aspect = 0;

typedef struct charinfo {
    CharView *cv;
    EncMap *map;
    SplineChar *sc, *cachedsc;
    int def_layer;
    SplineChar *oldsc;		/* oldsc->charinfo will point to us. Used to keep track of that pointer */
    int enc;
    GWindow gw;
    int done, first, changed;
    struct lookup_subtable *old_sub;
    int r,c;
    int lc_seen, lc_aspect, vert_aspect;
    Color last, real_last;
    struct splinecharlist *changes;
    int name_change, uni_change;
} CharInfo;

#define CI_Width	218
#define CI_Height	292

#define CID_UName	1001
#define CID_UValue	1002
#define CID_UChar	1003
#define CID_Cancel	1005
#define CID_ComponentMsg	1006
#define CID_Components	1007
#define CID_Comment	1008
#define CID_Color	1009
#define CID_GClass	1010
#define CID_Tabs	1011

#define CID_TeX_Height	1012
#define CID_TeX_Depth	1013
#define CID_TeX_Italic	1014
#define CID_HorAccent	1015
/* Room for one more here, if we need it */
#define CID_TeX_HeightD	1017
#define CID_TeX_DepthD	1018
#define CID_TeX_ItalicD	1019
#define CID_HorAccentD	1020

#define CID_ItalicDevTab	1022
#define CID_AccentDevTab	1023

#define CID_IsExtended	1024
#define CID_DefLCCount	1040
#define CID_LCCount	1041
#define CID_LCCountLab	1042

#define CID_UnlinkRmOverlap	1045
#define CID_AltUni	1046

/* Offsets for repeated fields. add 100*index (index<=6) */
#define CID_List	1220
#define CID_New		1221
#define CID_Delete	1222
#define CID_Edit	1223

#define CID_PST		1111
#define CID_Tag		1112
#define CID_Contents	1113
#define CID_SelectResults	1114
#define CID_MergeResults	1115
#define CID_RestrictSelection	1116

/* Offsets for repeated fields. add 100*index (index<2) */ /* 0=>Vert, 1=>Hor */
#define CID_VariantList		2000
#define CID_ExtItalicCor	2001
#define CID_ExtItalicDev	2002
#define CID_ExtensionList	2003

#define CID_IsTileMargin	3001
#define CID_TileMargin		3002
#define CID_IsTileBBox		3003
#define CID_TileBBoxMinX	3004
#define CID_TileBBoxMinY	3005
#define CID_TileBBoxMaxX	3006
#define CID_TileBBoxMaxY	3007

#define SIM_DX		1
#define SIM_DY		3
#define SIM_DX_ADV	5
#define SIM_DY_ADV	7
#define PAIR_DX1	2
#define PAIR_DY1	4
#define PAIR_DX_ADV1	6
#define PAIR_DY_ADV1	8
#define PAIR_DX2	10
#define PAIR_DY2	12
#define PAIR_DX_ADV2	14
#define PAIR_DY_ADV2	16

static GTextInfo glyphclasses[] = {
    { (unichar_t *) N_("Automatic"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("No Class"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Base Glyph"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Base Lig"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Mark"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Component"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

#define CUSTOM_COLOR	9
#define COLOR_CHOOSE	(-10)
static GTextInfo std_colors[] = {
    { (unichar_t *) N_("Color|Choose..."), NULL, 0, 0, (void *) COLOR_CHOOSE, NULL, 0, 1, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Color|Default"), &def_image, 0, 0, (void *) COLOR_DEFAULT, NULL, 0, 1, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { NULL, &white_image, 0, 0, (void *) 0xffffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &red_image, 0, 0, (void *) 0xff0000, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &green_image, 0, 0, (void *) 0x00ff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &blue_image, 0, 0, (void *) 0x0000ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &yellow_image, 0, 0, (void *) 0xffff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &cyan_image, 0, 0, (void *) 0x00ffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &magenta_image, 0, 0, (void *) 0xff00ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, NULL, 0, 0, (void *) 0x000000, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static char *newstrings[] = { N_("New Positioning"), N_("New Pair Position"),
	N_("New Substitution Variant"),
	N_("New Alternate List"), N_("New Multiple List"), N_("New Ligature"), NULL };

static unichar_t *CounterMaskLine(SplineChar *sc, HintMask *hm) {
    unichar_t *textmask = NULL;
    int j,k,len;
    StemInfo *h;
    char buffer[100];

    for ( j=0; j<2; ++j ) {
	len = 0;
	for ( h=sc->hstem, k=0; h!=NULL && k<HntMax; h=h->next, ++k ) {
	    if ( (*hm)[k>>3]& (0x80>>(k&7)) ) {
		sprintf( buffer, "H<%g,%g>, ",
			rint(h->start*100)/100, rint(h->width*100)/100 );
		if ( textmask!=NULL )
		    uc_strcpy(textmask+len,buffer);
		len += strlen(buffer);
	    }
	}
	for ( h=sc->vstem; h!=NULL && k<HntMax; h=h->next, ++k ) {
	    if ( (*hm)[k>>3]& (0x80>>(k&7)) ) {
		sprintf( buffer, "V<%g,%g>, ",
			rint(h->start*100)/100, rint(h->width*100)/100 );
		if ( textmask!=NULL )
		    uc_strcpy(textmask+len,buffer);
		len += strlen(buffer);
	    }
	}
	if ( textmask==NULL ) {
	    textmask = malloc((len+1)*sizeof(unichar_t));
	    *textmask = '\0';
	}
    }
    if ( len>1 && textmask[len-2]==',' )
	textmask[len-2] = '\0';
return( textmask );
}

#define CID_HintMask	2020
#define HI_Width	200
#define HI_Height	260

struct hi_data {
    int done, ok, empty;
    GWindow gw;
    HintMask *cur;
    SplineChar *sc;
};

static int HI_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct hi_data *hi = GDrawGetUserData(GGadgetGetWindow(g));
	int32 i, len;
	GTextInfo **ti = GGadgetGetList(GWidgetGetControl(hi->gw,CID_HintMask),&len);

	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected )
	break;

	memset(hi->cur,0,sizeof(HintMask));
	if ( i==len ) {
	    hi->empty = true;
	} else {
	    for ( i=0; i<len; ++i )
		if ( ti[i]->selected )
		    (*hi->cur)[i>>3] |= (0x80>>(i&7));
	}
	PI_ShowHints(hi->sc,GWidgetGetControl(hi->gw,CID_HintMask),false);

	hi->done = true;
	hi->ok = true;
    }
return( true );
}

static void HI_DoCancel(struct hi_data *hi) {
    hi->done = true;
    PI_ShowHints(hi->sc,GWidgetGetControl(hi->gw,CID_HintMask),false);
}

static int HI_HintSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct hi_data *hi = GDrawGetUserData(GGadgetGetWindow(g));

	PI_ShowHints(hi->sc,g,true);
	/* Do I need to check for overlap here? */
    }
return( true );
}

static int HI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	HI_DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int hi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	HI_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html#Counters");
return( true );
	}
return( false );
    }
return( true );
}

static void CI_AskCounters(CharInfo *ci,HintMask *old) {
    HintMask *cur = old != NULL ? old : chunkalloc(sizeof(HintMask));
    struct hi_data hi;
    GWindowAttrs wattrs;
    GGadgetCreateData hgcd[5], *varray[11], *harray[8], boxes[3];
    GTextInfo hlabel[5];
    GGadget *list = GWidgetGetControl(ci->gw,CID_List+600);
    int j,k;
    GRect pos;

    memset(&hi,0,sizeof(hi));
    hi.cur = cur;
    hi.sc = ci->sc;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = old==NULL?_("New Counter Mask"):_("Edit Counter Mask");
	wattrs.is_dlg = true;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,HI_Width));
	pos.height = GDrawPointsToPixels(NULL,HI_Height);
	hi.gw = GDrawCreateTopWindow(NULL,&pos,hi_e_h,&hi,&wattrs);


	memset(hgcd,0,sizeof(hgcd));
	memset(boxes,0,sizeof(boxes));
	memset(hlabel,0,sizeof(hlabel));

	j=k=0;

	hgcd[j].gd.pos.x = 20-3; hgcd[j].gd.pos.y = HI_Height-31-3;
	hgcd[j].gd.pos.width = -1; hgcd[j].gd.pos.height = 0;
	hgcd[j].gd.flags = gg_visible | gg_enabled;
	hlabel[j].text = (unichar_t *) _("Select hints between which counters are formed");
	hlabel[j].text_is_1byte = true;
	hlabel[j].text_in_resource = true;
	hgcd[j].gd.label = &hlabel[j];
	varray[k++] = &hgcd[j]; varray[k++] = NULL;
	hgcd[j++].creator = GLabelCreate;

	hgcd[j].gd.pos.x = 5; hgcd[j].gd.pos.y = 5;
	hgcd[j].gd.pos.width = HI_Width-10; hgcd[j].gd.pos.height = HI_Height-45;
	hgcd[j].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
	hgcd[j].gd.cid = CID_HintMask;
	hgcd[j].gd.u.list = SCHintList(ci->sc,old);
	hgcd[j].gd.handle_controlevent = HI_HintSel;
	varray[k++] = &hgcd[j]; varray[k++] = NULL;
	varray[k++] = GCD_Glue; varray[k++] = NULL;
	hgcd[j++].creator = GListCreate;

	hgcd[j].gd.pos.x = 20-3; hgcd[j].gd.pos.y = HI_Height-31-3;
	hgcd[j].gd.pos.width = -1; hgcd[j].gd.pos.height = 0;
	hgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_default;
	hlabel[j].text = (unichar_t *) _("_OK");
	hlabel[j].text_is_1byte = true;
	hlabel[j].text_in_resource = true;
	hgcd[j].gd.label = &hlabel[j];
	hgcd[j].gd.handle_controlevent = HI_Ok;
	harray[0] = GCD_Glue; harray[1] = &hgcd[j]; harray[2] = GCD_Glue; harray[3] = GCD_Glue;
	hgcd[j++].creator = GButtonCreate;

	hgcd[j].gd.pos.x = -20; hgcd[j].gd.pos.y = HI_Height-31;
	hgcd[j].gd.pos.width = -1; hgcd[j].gd.pos.height = 0;
	hgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	hlabel[j].text = (unichar_t *) _("_Cancel");
	hlabel[j].text_is_1byte = true;
	hlabel[j].text_in_resource = true;
	hgcd[j].gd.label = &hlabel[j];
	hgcd[j].gd.handle_controlevent = HI_Cancel;
	harray[4] = GCD_Glue; harray[5] = &hgcd[j]; harray[6] = GCD_Glue; harray[7] = NULL;
	hgcd[j++].creator = GButtonCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray;
	boxes[2].creator = GHBoxCreate;
	varray[k++] = &boxes[2]; varray[k++] = NULL; 
	varray[k++] = GCD_Glue; varray[k++] = NULL;
	varray[k] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(hi.gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,1);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);
	GTextInfoListFree(hgcd[0].gd.u.list);

	PI_ShowHints(hi.sc,hgcd[0].ret,true);

    GDrawSetVisible(hi.gw,true);
    while ( !hi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(hi.gw);

    if ( !hi.ok ) {
	if ( old==NULL ) chunkfree(cur,sizeof(HintMask));
return;		/* Cancelled */
    } else if ( old==NULL && hi.empty ) {
	if ( old==NULL ) chunkfree(cur,sizeof(HintMask));
return;		/* Didn't add anything new */
    } else if ( old==NULL ) {
	GListAddStr(list,CounterMaskLine(hi.sc,cur),cur);
return;
    } else if ( !hi.empty ) {
	GListReplaceStr(list,GGadgetGetFirstListSelectedItem(list),
		CounterMaskLine(hi.sc,cur),cur);
return;
    } else {
	GListDelSelected(list);
	chunkfree(cur,sizeof(HintMask));
    }
}

static int CI_NewCounter(GGadget *g, GEvent *e) {
    CharInfo *ci;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ci = GDrawGetUserData(GGadgetGetWindow(g));
	CI_AskCounters(ci,NULL);
    }
return( true );
}

static int CI_EditCounter(GGadget *g, GEvent *e) {
    GTextInfo *ti;
    GGadget *list;
    CharInfo *ci;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ci = GDrawGetUserData(GGadgetGetWindow(g));
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_List+6*100);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	CI_AskCounters(ci,ti->userdata);
    }
return( true );
}

static int CI_DeleteCounter(GGadget *g, GEvent *e) {
    int32 len; int i,j, offset;
    GTextInfo **old, **new_;
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	offset = GGadgetGetCid(g)-CID_Delete;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_List+offset);
	old = GGadgetGetList(list,&len);
	new_ = calloc(len+1,sizeof(GTextInfo *));
	for ( i=j=0; i<len; ++i )
	    if ( !old[i]->selected ) {
		new_[j] = (GTextInfo *) malloc(sizeof(GTextInfo));
		*new_[j] = *old[i];
		new_[j]->text = u_copy(new_[j]->text);
		++j;
	    }
	new_[j] = (GTextInfo *) calloc(1,sizeof(GTextInfo));
	if ( offset==600 ) {
	    for ( i=0; i<len; ++i ) if ( old[i]->selected )
		chunkfree(old[i]->userdata,sizeof(HintMask));
	}
	GGadgetSetList(list,new_,false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Delete+offset),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Edit+offset),false);
    }
return( true );
}

static int CI_CounterSelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	int offset = GGadgetGetCid(g)-CID_List;
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Delete+offset),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Edit+offset),sel!=-1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int offset = GGadgetGetCid(g)-CID_List;
	e->u.control.subtype = et_buttonactivate;
	e->u.control.g = GWidgetGetControl(ci->gw,CID_Edit+offset);
	CI_EditCounter(e->u.control.g,e);
    }
return( true );
}

static int ParseUValue(GWindow gw, int cid, int minusoneok) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    unichar_t *end;
    int val;

    if (( *ret=='U' || *ret=='u' ) && ret[1]=='+' )
	val = u_strtoul(ret+2,&end,16);
    else if ( *ret=='#' )
	val = u_strtoul(ret+1,&end,16);
    else
	val = u_strtoul(ret,&end,16);
    if ( val==-1 && minusoneok )
return( -1 );
    if ( *end || val<0 || val>0x10ffff ) {
	GGadgetProtest8( _("Unicode _Value:") );
return( -2 );
    }
return( val );
}

static void SetNameFromUnicode(GWindow gw,int cid,int val) {
    unichar_t *temp;
    char buf[100];
    CharInfo *ci = GDrawGetUserData(gw);

    temp = utf82u_copy(StdGlyphName(buf,val,ci->sc->parent->uni_interp,ci->sc->parent->for_new_glyphs));
    GGadgetSetTitle(GWidgetGetControl(gw,cid),temp);
    free(temp);
}

void SCInsertPST(SplineChar *sc,PST *new_) {
    new_->next = sc->possub;
    sc->possub = new_;
}

static int CI_NameCheck(const unichar_t *name) {
    int bad, questionable;
    extern int allow_utf8_glyphnames;
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_No"); buts[2] = NULL;

    if ( uc_strcmp(name,".notdef")==0 )		/* This name is a special case and doesn't follow conventions */
return( true );
    if ( u_strlen(name)>31 ) {
	ff_post_error(_("Bad Name"),_("Glyph names are limited to 31 characters"));
return( false );
    } else if ( *name=='\0' ) {
	ff_post_error(_("Bad Name"),_("Bad Name"));
return( false );
    } else if ( isdigit(*name) || *name=='.' ) {
	ff_post_error(_("Bad Name"),_("A glyph name may not start with a digit nor a full stop (period)"));
return( false );
    }
    bad = questionable = false;
    while ( *name ) {
	if ( *name<=' ' || (!allow_utf8_glyphnames && *name>=0x7f) ||
		*name=='(' || *name=='[' || *name=='{' || *name=='<' ||
		*name==')' || *name==']' || *name=='}' || *name=='>' ||
		*name=='%' || *name=='/' )
	    bad=true;
	else if ( !isalnum(*name) && *name!='.' && *name!='_' )
	    questionable = true;
	++name;
    }
    if ( bad ) {
	ff_post_error(_("Bad Name"),_("A glyph name must be ASCII, without spaces and may not contain the characters \"([{<>}])/%%\", and should contain only alphanumerics, periods and underscores"));
return( false );
    } else if ( questionable ) {
	if ( gwwv_ask(_("Bad Name"),(const char **) buts,0,1,_("A glyph name should contain only alphanumerics, periods and underscores\nDo you want to use this name in spite of that?"))==1 )
return(false);
    }
return( true );
}

static void CI_ParseCounters(CharInfo *ci) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(ci->gw,CID_List+600),&len);
    SplineChar *sc = ci->cachedsc;

    free(sc->countermasks);

    sc->countermask_cnt = len;
    if ( len==0 )
	sc->countermasks = NULL;
    else {
	sc->countermasks = malloc(len*sizeof(HintMask));
	for ( i=0; i<len; ++i ) {
	    memcpy(sc->countermasks[i],ti[i]->userdata,sizeof(HintMask));
	    chunkfree(ti[i]->userdata,sizeof(HintMask));
	    ti[i]->userdata = NULL;
	}
    }
}

int DeviceTableOK(char *dvstr, int *_low, int *_high) {
    char *pt, *end;
    int low, high, pixel, cor;

    low = high = -1;
    if ( dvstr!=NULL ) {
	while ( *dvstr==' ' ) ++dvstr;
	for ( pt=dvstr; *pt; ) {
	    pixel = strtol(pt,&end,10);
	    if ( pixel<=0 || pt==end)
	break;
	    pt = end;
	    if ( *pt==':' ) ++pt;
	    cor = strtol(pt,&end,10);
	    if ( pt==end || cor<-128 || cor>127 )
	break;
	    pt = end;
	    while ( *pt==' ' ) ++pt;
	    if ( *pt==',' ) ++pt;
	    while ( *pt==' ' ) ++pt;
	    if ( low==-1 ) low = high = pixel;
	    else if ( pixel<low ) low = pixel;
	    else if ( pixel>high ) high = pixel;
	}
	if ( *pt != '\0' )
return( false );
    }
    *_low = low; *_high = high;
return( true );
}

DeviceTable *DeviceTableParse(DeviceTable *dv,char *dvstr) {
    char *pt, *end;
    int low, high, pixel, cor;

    DeviceTableOK(dvstr,&low,&high);
    if ( low==-1 ) {
	if ( dv!=NULL ) {
	    free(dv->corrections);
	    memset(dv,0,sizeof(*dv));
	}
return( dv );
    }
    if ( dv==NULL )
	dv = chunkalloc(sizeof(DeviceTable));
    else
	free(dv->corrections);
    dv->first_pixel_size = low;
    dv->last_pixel_size = high;
    dv->corrections = calloc(high-low+1,1);

    for ( pt=dvstr; *pt; ) {
	pixel = strtol(pt,&end,10);
	if ( pixel<=0 || pt==end)
    break;
	pt = end;
	if ( *pt==':' ) ++pt;
	cor = strtol(pt,&end,10);
	if ( pt==end || cor<-128 || cor>127 )
    break;
	pt = end;
	while ( *pt==' ' ) ++pt;
	if ( *pt==',' ) ++pt;
	while ( *pt==' ' ) ++pt;
	dv->corrections[pixel-low] = cor;
    }
return( dv );
}

void VRDevTabParse(struct vr *vr,struct matrix_data *md) {
    ValDevTab temp, *adjust;
    int any = false;

    if ( (adjust = vr->adjust)==NULL ) {
	adjust = &temp;
	memset(&temp,0,sizeof(temp));
    }
    any |= (DeviceTableParse(&adjust->xadjust,md[0].u.md_str)!=NULL);
    any |= (DeviceTableParse(&adjust->yadjust,md[2].u.md_str)!=NULL);
    any |= (DeviceTableParse(&adjust->xadv,md[4].u.md_str)!=NULL);
    any |= (DeviceTableParse(&adjust->yadv,md[6].u.md_str)!=NULL);
    if ( any && adjust==&temp ) {
	vr->adjust = chunkalloc(sizeof(ValDevTab));
	*vr->adjust = temp;
    } else if ( !any && vr->adjust!=NULL ) {
	ValDevFree(vr->adjust);
	vr->adjust = NULL;
    }
}

void DevTabToString(char **str,DeviceTable *adjust) {
    char *pt;
    int i;

    if ( adjust==NULL || adjust->corrections==NULL ) {
	*str = NULL;
return;
    }
    *str = pt = malloc(11*(adjust->last_pixel_size-adjust->first_pixel_size+1)+1);
    for ( i=adjust->first_pixel_size; i<=adjust->last_pixel_size; ++i ) {
	if ( adjust->corrections[i-adjust->first_pixel_size]!=0 )
	    sprintf( pt, "%d:%d, ", i, adjust->corrections[i-adjust->first_pixel_size]);
	pt += strlen(pt);
    }
    if ( pt>*str && pt[-2] == ',' )
	pt[-2] = '\0';
}
    
void ValDevTabToStrings(struct matrix_data *mds,int first_offset,ValDevTab *adjust) {
    if ( adjust==NULL )
return;
    DevTabToString(&mds[first_offset].u.md_str,&adjust->xadjust);
    DevTabToString(&mds[first_offset+2].u.md_str,&adjust->yadjust);
    DevTabToString(&mds[first_offset+4].u.md_str,&adjust->xadv);
    DevTabToString(&mds[first_offset+6].u.md_str,&adjust->yadv);
}

void KpMDParse(SplineChar *sc,struct lookup_subtable *sub,
	struct matrix_data *possub,int rows,int cols,int i) {
    SplineChar *other;
    PST *pst;
    KernPair *kp;
    int isv, iskpable, offset, newv;
    char *dvstr;
    char *pt, *start;
    int ch;

    for ( start=possub[cols*i+1].u.md_str; ; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt!='\0' && *pt!=' ' && *pt!='('; ++pt );
	ch = *pt; *pt = '\0';
	other = SFGetChar(sc->parent,-1,start);
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable == sub &&
		    strcmp(start,pst->u.pair.paired)==0 )
	break;
	}
	kp = NULL;
	if ( pst==NULL && other!=NULL ) {
	    for ( isv=0; isv<2; ++isv ) {
		for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next )
		    if ( kp->subtable==(void *) possub[cols*i+0].u.md_ival &&
			    kp->sc == other )
		break;
		if ( kp!=NULL )
	    break;
	    }
	}
	newv = false;
	if ( other==NULL )
	    iskpable = false;
	else if ( sub->vertical_kerning ) {
	    newv = true;
	    iskpable = possub[cols*i+PAIR_DX1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX_ADV1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY2].u.md_ival==0 &&
			possub[cols*i+PAIR_DX_ADV2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV2].u.md_ival==0;
	    offset = possub[cols*i+PAIR_DY1].u.md_ival;
	    iskpable &= (possub[cols*i+PAIR_DX1+1].u.md_str==NULL || *possub[cols*i+PAIR_DX1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY1+1].u.md_str==0 || *possub[cols*i+PAIR_DY1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX_ADV1+1].u.md_str==0 || *possub[cols*i+PAIR_DX_ADV1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX2+1].u.md_str==0 || *possub[cols*i+PAIR_DX2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY2+1].u.md_str==0 || *possub[cols*i+PAIR_DY2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DX_ADV2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 );
	    dvstr = possub[cols*i+PAIR_DY1+1].u.md_str;
	} else if ( sub->lookup->lookup_flags & pst_r2l ) {
	    iskpable = possub[cols*i+PAIR_DX1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX_ADV1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV2].u.md_ival==0;
	    offset = possub[cols*i+PAIR_DX_ADV2].u.md_ival;
	    iskpable &= (possub[cols*i+PAIR_DX1+1].u.md_str==NULL || *possub[cols*i+PAIR_DX1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY1+1].u.md_str==0 || *possub[cols*i+PAIR_DY1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX_ADV1+1].u.md_str==0 || *possub[cols*i+PAIR_DX_ADV1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV1+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX2+1].u.md_str==0 || *possub[cols*i+PAIR_DX2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY2+1].u.md_str==0 || *possub[cols*i+PAIR_DY2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 );
	    dvstr = possub[cols*i+PAIR_DX_ADV2+1].u.md_str;
	} else {
	    iskpable = possub[cols*i+PAIR_DX1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY2].u.md_ival==0 &&
			possub[cols*i+PAIR_DX_ADV2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV2].u.md_ival==0;
	    offset = possub[cols*i+PAIR_DX_ADV1].u.md_ival;
	    iskpable &= (possub[cols*i+PAIR_DX1+1].u.md_str==NULL || *possub[cols*i+PAIR_DX1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY1+1].u.md_str==0 || *possub[cols*i+PAIR_DY1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV1+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX2+1].u.md_str==0 || *possub[cols*i+PAIR_DX2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY2+1].u.md_str==0 || *possub[cols*i+PAIR_DY2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DX_ADV2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 );
	    dvstr = possub[cols*i+PAIR_DX_ADV1+1].u.md_str;
	}
	if ( iskpable ) {
	    if ( kp==NULL ) {
		/* If there's a pst, ignore it, it will not get ticked and will*/
		/*  be freed later */
		kp = chunkalloc(sizeof(KernPair));
		kp->subtable = sub;
		kp->sc = other;
		if ( newv ) {
		    kp->next = sc->vkerns;
		    sc->vkerns = kp;
		} else {
		    kp->next = sc->kerns;
		    sc->kerns = kp;
		}
	    }
	    DeviceTableFree(kp->adjust);
	    kp->adjust = DeviceTableParse(NULL,dvstr);
	    kp->off = offset;
	    kp->kcid = true;
	} else {
	    if ( pst == NULL ) {
		/* If there's a kp, ignore it, it will not get ticked and will*/
		/*  be freed later */
		pst = chunkalloc(sizeof(PST));
		pst->type = pst_pair;
		pst->subtable = sub;
		pst->next = sc->possub;
		sc->possub = pst;
		pst->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
		pst->u.pair.paired = copy(start);
	    }
	    VRDevTabParse(&pst->u.pair.vr[0],&possub[cols*i+PAIR_DX1+1]);
	    VRDevTabParse(&pst->u.pair.vr[1],&possub[cols*i+PAIR_DX2]+1);
	    pst->u.pair.vr[0].xoff = possub[cols*i+PAIR_DX1].u.md_ival;
	    pst->u.pair.vr[0].yoff = possub[cols*i+PAIR_DY1].u.md_ival;
	    pst->u.pair.vr[0].h_adv_off = possub[cols*i+PAIR_DX_ADV1].u.md_ival;
	    pst->u.pair.vr[0].v_adv_off = possub[cols*i+PAIR_DY_ADV1].u.md_ival;
	    pst->u.pair.vr[1].xoff = possub[cols*i+PAIR_DX2].u.md_ival;
	    pst->u.pair.vr[1].yoff = possub[cols*i+PAIR_DY2].u.md_ival;
	    pst->u.pair.vr[1].h_adv_off = possub[cols*i+PAIR_DX_ADV2].u.md_ival;
	    pst->u.pair.vr[1].v_adv_off = possub[cols*i+PAIR_DY_ADV2].u.md_ival;
	    pst->ticked = true;
	}
	*pt = ch;
	if ( ch=='(' ) {
	    while ( *pt!=')' && *pt!='\0' ) ++pt;
	    if ( *pt==')' ) ++pt;
	}
	start = pt;
    }
}

static int CI_ProcessPosSubs(CharInfo *ci) {
    /* Check for duplicate entries in kerning and ligatures. If we find any */
    /*  complain and return failure */
    /* Check for various other errors */
    /* Otherwise process */
    SplineChar *sc = ci->cachedsc, *found;
    int i,j, rows, cols, isv, pstt, ch;
    char *pt;
    struct matrix_data *possub;
    char *buts[3];
    KernPair *kp, *kpprev, *kpnext;
    PST *pst, *pstprev, *pstnext;

    possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pst_ligature-1)*100), &rows );
    cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pst_ligature-1)*100) );
    for ( i=0; i<rows; ++i ) {
	for ( j=i+1; j<rows; ++j ) {
	    if ( possub[cols*i+0].u.md_ival == possub[cols*j+0].u.md_ival &&
		    strcmp(possub[cols*i+1].u.md_str,possub[cols*j+1].u.md_str)==0 ) {
		ff_post_error( _("Duplicate Ligature"),_("There are two ligature entries with the same components (%.80s) in the same lookup subtable (%.30s)"),
			possub[cols*j+1].u.md_str,
			((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name );
return( false );
	    }
	}
    }
    possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100), &rows );
    cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100));
    for ( i=0; i<rows; ++i ) {
	for ( j=i+1; j<rows; ++j ) {
	    if ( possub[cols*i+0].u.md_ival == possub[cols*j+0].u.md_ival &&
		    strcmp(possub[cols*i+1].u.md_str,possub[cols*j+1].u.md_str)==0 ) {
		ff_post_error( _("Duplicate Kern data"),_("There are two kerning entries for the same glyph (%.80s) in the same lookup subtable (%.30s)"),
			possub[cols*j+1].u.md_str,
			((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name );
return( false );
	    }
	}
    }

    /* Check for badly specified device tables */
    for ( pstt = pst_position; pstt<=pst_pair; ++pstt ) {
	int startc = pstt==pst_position ? SIM_DX+1 : PAIR_DX1+1;
	int low, high, c, r;
	possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100), &rows );
	cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100));
	for ( r=0; r<rows; ++r ) {
	    for ( c=startc; c<cols; c+=2 ) {
		if ( !DeviceTableOK(possub[r*cols+c].u.md_str,&low,&high) ) {
		    ff_post_error( _("Bad Device Table Adjustment"),_("A device table adjustment specified for %.80s is invalid"),
			    possub[cols*r+0].u.md_str );
return( true );
		}
	    }
	}
    }

    for ( pstt = pst_pair; pstt<=pst_ligature; ++pstt ) {
	possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100), &rows );
	cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100));
	for ( i=0; i<rows; ++i ) {
	    char *start = possub[cols*i+1].u.md_str;
	    while ( *start== ' ' ) ++start;
	    if ( *start=='\0' ) {
		ff_post_error( _("Missing glyph name"),_("You must specify a glyph name for subtable %s"),
			((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name );
return( false );
	    }
	    while ( *start ) {
		for ( pt=start; *pt!='\0' && *pt!=' ' && *pt!='(' ; ++pt );
		ch = *pt; *pt='\0';
		found = SFGetChar(sc->parent,-1,start);
		if ( found==NULL ) {
		    buts[0] = _("_Yes");
		    buts[1] = _("_Cancel");
		    buts[2] = NULL;
		    if ( gwwv_ask(_("Missing glyph"),(const char **) buts,0,1,_("In lookup subtable %.30s you refer to a glyph named %.80s, which is not in the font yet. Was this intentional?"),
			    ((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name,
			    start)==1 ) {
			*pt = ch;
return( false );
		    }
		} else if ( found==ci->sc && pstt!=pst_pair ) {
		    buts[0] = _("_Yes");
		    buts[1] = _("_Cancel");
		    buts[2] = NULL;
		    if ( gwwv_ask(_("Substitution generates itself"),(const char **) buts,0,1,_("In lookup subtable %.30s you replace a glyph with itself. Was this intentional?"),
			    ((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name)==1 ) {
			*pt = ch;
return( false );
		    }
		}
		*pt = ch;
		if ( ch=='(' ) {
		    while ( *pt!=')' && *pt!='\0' ) ++pt;
		    if ( *pt==')' ) ++pt;
		}
		while ( *pt== ' ' ) ++pt;
		start = pt;
	    }
	}
    }

    /* If we get this far, then we didn't find any errors */
    for ( pst = sc->possub; pst!=NULL; pst=pst->next )
	pst->ticked = pst->type==pst_lcaret;
    for ( isv=0; isv<2; ++isv )
	for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next )
	    kp->kcid = 0;

    for ( pstt=pst_substitution; pstt<=pst_ligature; ++pstt ) {
	possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100), &rows );
	cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100) );
	for ( i=0; i<rows; ++i ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable == (void *) possub[cols*i+0].u.md_ival &&
			!pst->ticked )
	    break;
	    }
	    if ( pst==NULL ) {
		pst = chunkalloc(sizeof(PST));
		pst->type = pstt;
		pst->subtable = (void *) possub[cols*i+0].u.md_ival;
		pst->next = sc->possub;
		sc->possub = pst;
	    } else
		free( pst->u.subs.variant );
	    pst->ticked = true;
	    pst->u.subs.variant = GlyphNameListDeUnicode( possub[cols*i+1].u.md_str );
	    if ( pstt==pst_ligature )
		pst->u.lig.lig = sc;
	}
    }

    possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pst_position-1)*100), &rows );
    cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pst_position-1)*100));
    for ( i=0; i<rows; ++i ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable == (void *) possub[cols*i+0].u.md_ival &&
		    !pst->ticked )
	break;
	}
	if ( pst==NULL ) {
	    pst = chunkalloc(sizeof(PST));
	    pst->type = pst_position;
	    pst->subtable = (void *) possub[cols*i+0].u.md_ival;
	    pst->next = sc->possub;
	    sc->possub = pst;
	}
	VRDevTabParse(&pst->u.pos,&possub[cols*i+SIM_DX+1]);
	pst->u.pos.xoff = possub[cols*i+SIM_DX].u.md_ival;
	pst->u.pos.yoff = possub[cols*i+SIM_DY].u.md_ival;
	pst->u.pos.h_adv_off = possub[cols*i+SIM_DX_ADV].u.md_ival;
	pst->u.pos.v_adv_off = possub[cols*i+SIM_DY_ADV].u.md_ival;
	pst->ticked = true;
    }

    possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100), &rows );
    cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100));
    for ( i=0; i<rows; ++i ) {
	struct lookup_subtable *sub = ((struct lookup_subtable *) possub[cols*i+0].u.md_ival);
	KpMDParse(sc,sub,possub,rows,cols,i);
    }

    /* Now, free anything that did not get ticked */
    for ( pstprev=NULL, pst = sc->possub; pst!=NULL; pst=pstnext ) {
	pstnext = pst->next;
	if ( pst->ticked )
	    pstprev = pst;
	else {
	    if ( pstprev==NULL )
		sc->possub = pstnext;
	    else
		pstprev->next = pstnext;
	    pst->next = NULL;
	    PSTFree(pst);
	}
    }
    for ( isv=0; isv<2; ++isv ) {
	for ( kpprev=NULL, kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kpnext ) {
	    kpnext = kp->next;
	    if ( kp->kcid!=0 )
		kpprev = kp;
	    else {
		if ( kpprev!=NULL )
		    kpprev->next = kpnext;
		else if ( isv )
		    sc->vkerns = kpnext;
		else
		    sc->kerns = kpnext;
		kp->next = NULL;
		KernPairsFree(kp);
	    }
	}
    }
    for ( isv=0; isv<2; ++isv )
	for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next )
	    kp->kcid = 0;
return( true );
}

static int gettex(GWindow gw,int cid,char *msg,int *err) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,cid));

    if ( *ret=='\0' )
return( TEX_UNDEF );
return( GetInt8(gw,cid,msg,err));
}

struct glyphvariants *GV_ParseConstruction(struct glyphvariants *gv,
	struct matrix_data *stuff, int rows, int cols) {
    int i;

    if ( gv==NULL )
	gv = chunkalloc(sizeof(struct glyphvariants));

    gv->part_cnt = rows;
    gv->parts = calloc(rows,sizeof(struct gv_part));
    for ( i=0; i<rows; ++i ) {
	gv->parts[i].component = copy(stuff[i*cols+0].u.md_str);
	gv->parts[i].is_extender = stuff[i*cols+1].u.md_ival;
	gv->parts[i].startConnectorLength = stuff[i*cols+2].u.md_ival;
	gv->parts[i].endConnectorLength = stuff[i*cols+3].u.md_ival;
	gv->parts[i].fullAdvance = stuff[i*cols+4].u.md_ival;
    }
return( gv );
}

static struct glyphvariants *CI_ParseVariants(struct glyphvariants *gv,
	CharInfo *ci, int is_horiz,
	char *italic_correction_devtab, int italic_correction,
	int only_parts) {
    char *variants = GGadgetGetTitle8(GWidgetGetControl(ci->gw,CID_VariantList+is_horiz*100));
    GGadget *construct = GWidgetGetControl(ci->gw,CID_ExtensionList+is_horiz*100);
    int rows, cols = GMatrixEditGetColCnt(construct);
    struct matrix_data *stuff = GMatrixEditGet(construct,&rows);

    if ( (variants==NULL || variants[0]=='\0' || only_parts) && rows==0 ) {
	free(variants);
	GlyphVariantsFree(gv);
return( NULL );
    }
    if ( gv==NULL )
	gv = chunkalloc(sizeof(struct glyphvariants));
    free(gv->variants); gv->variants = NULL;
    if ( only_parts ) {
	free(variants); variants = NULL;
    } else if ( variants!=NULL && *variants!='\0' )
	gv->variants = variants;
    else {
	gv->variants = NULL;
	free( variants);
    }
    if ( !only_parts ) {
	gv->italic_correction = italic_correction;
	gv->italic_adjusts = DeviceTableParse(gv->italic_adjusts,italic_correction_devtab);
    }
    gv = GV_ParseConstruction(gv,stuff,rows,cols);
return( gv );
}

static int CI_ValidateAltUnis(CharInfo *ci) {
    GGadget *au = GWidgetGetControl(ci->gw,CID_AltUni);
    int rows, cols = GMatrixEditGetColCnt(au);
    struct matrix_data *stuff = GMatrixEditGet(au,&rows);
    int i, asked = false;

    for ( i=0; i<rows; ++i ) {
	int uni = stuff[i*cols+0].u.md_ival, vs = stuff[i*cols+1].u.md_ival;
	if ( uni<0 || uni>=unicode4_size ||
		vs<-1 || vs>=unicode4_size ) {
	    ff_post_error(_("Unicode out of range"), _("Bad unicode value for an alternate unicode / variation selector"));
return( false );
	}
	if ( (vs>=0x180B && vs<=0x180D) ||	/* Mongolian VS */
		 (vs>=0xfe00 && vs<=0xfe0f) ||	/* First VS block */
		 (vs>=0xE0100 && vs<=0xE01EF) ) {	/* Second VS block */
	    /* ok, that's a reasonable value */;
	} else if ( vs==0 || vs==-1 ) {
	    /* That's ok too (means no selector, just an alternate encoding) */;
	} else if ( !asked ) {
	    char *buts[3];
	    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
	    if ( gwwv_ask(_("Unexpected Variation Selector"),(const char **) buts,0,1,
		    _("Variation selectors are normally between\n"
		      "   U+180B and U+180D\n"
		      "   U+FE00 and U+FE0F\n"
		      "   U+E0100 and U+E01EF\n"
		      "did you really intend to use U+%04X?"), vs)==1 )
return( false );
	    asked = true;
	}
    }
return( true );
}

static void CI_ParseAltUnis(CharInfo *ci) {
    GGadget *au = GWidgetGetControl(ci->gw,CID_AltUni);
    int rows, cols = GMatrixEditGetColCnt(au);
    struct matrix_data *stuff = GMatrixEditGet(au,&rows);
    int i;
    struct altuni *altuni, *last = NULL;
    SplineChar *sc = ci->cachedsc;
    int deenc = false;
    FontView *fvs;
    int oldcnt, newcnt;

    oldcnt = 0;
    for ( altuni=sc->altuni ; altuni!=NULL; altuni = altuni->next )
	if ( altuni->vs==-1 && altuni->fid==0 )
	    ++oldcnt;

    newcnt = 0;
    for ( i=0; i<rows; ++i ) {
	int uni = stuff[i*cols+0].u.md_ival, vs = stuff[i*cols+1].u.md_ival;
	if ( vs!=0 )
    continue;
	++newcnt;
	if ( uni==sc->unicodeenc )
    continue;
	for ( altuni=sc->altuni ; altuni!=NULL; altuni = altuni->next )
	    if ( uni==altuni->unienc && altuni->vs==-1 && altuni->fid==0 )
	break;
	if ( altuni==NULL ) {
	    deenc = true;
    break;
	}
    }
    if ( oldcnt!=newcnt || deenc ) {
	for ( fvs=(FontView *) sc->parent->fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame ) {
	    fvs->b.map->enc = &custom;
	    FVSetTitle((FontViewBase *) fvs);
	}
    }
    AltUniFree(sc->altuni); sc->altuni = NULL;
    for ( i=0; i<rows; ++i ) {
	int uni = stuff[i*cols+0].u.md_ival, vs = stuff[i*cols+1].u.md_ival;
	altuni = chunkalloc(sizeof(struct altuni));
	altuni->unienc = uni;
	altuni->vs = vs==0 ? -1 : vs;
	altuni->fid = 0;
	if ( last == NULL )
	    sc->altuni = altuni;
	else
	    last->next = altuni;
	last = altuni;
    }
}

static KernPair *CI_KPCopy(KernPair *kp) {
    KernPair *head=NULL, *last=NULL, *newkp;

    while ( kp!=NULL ) {
	newkp = chunkalloc(sizeof(KernPair));
	*newkp = *kp;
	newkp->adjust = DeviceTableCopy(kp->adjust);
	newkp->next = NULL;
	if ( head==NULL )
	    head = newkp;
	else
	    last->next = newkp;
	last = newkp;
	kp = kp->next;
    }
return( head );
}

static PST *CI_PSTCopy(PST *pst) {
    PST *head=NULL, *last=NULL, *newpst;

    while ( pst!=NULL ) {
	newpst = chunkalloc(sizeof(KernPair));
	*newpst = *pst;
	if ( newpst->type==pst_ligature ) {
	    newpst->u.lig.components = copy(pst->u.lig.components);
	} else if ( newpst->type==pst_pair ) {
	    newpst->u.pair.paired = copy(pst->u.pair.paired);
	    newpst->u.pair.vr = chunkalloc(sizeof( struct vr [2]));
	    memcpy(newpst->u.pair.vr,pst->u.pair.vr,sizeof(struct vr [2]));
	    newpst->u.pair.vr[0].adjust = ValDevTabCopy(pst->u.pair.vr[0].adjust);
	    newpst->u.pair.vr[1].adjust = ValDevTabCopy(pst->u.pair.vr[1].adjust);
	} else if ( newpst->type==pst_lcaret ) {
	    newpst->u.lcaret.carets = malloc(pst->u.lcaret.cnt*sizeof(uint16));
	    memcpy(newpst->u.lcaret.carets,pst->u.lcaret.carets,pst->u.lcaret.cnt*sizeof(uint16));
	} else if ( newpst->type==pst_substitution || newpst->type==pst_multiple || newpst->type==pst_alternate )
	    newpst->u.subs.variant = copy(pst->u.subs.variant);
	newpst->next = NULL;
	if ( head==NULL )
	    head = newpst;
	else
	    last->next = newpst;
	last = newpst;
	pst = pst->next;
    }
return( head );
}

static SplineChar *CI_SCDuplicate(SplineChar *sc) {
    SplineChar *newsc;		/* copy everything we care about in this dlg */

    newsc = chunkalloc(sizeof(SplineChar));
    newsc->name = copy(sc->name);
    newsc->parent = sc->parent;
    newsc->unicodeenc = sc->unicodeenc;
    newsc->orig_pos = sc->orig_pos;
    newsc->comment = copy(sc->comment);
    newsc->unlink_rm_ovrlp_save_undo = sc->unlink_rm_ovrlp_save_undo;
    newsc->glyph_class = sc->glyph_class;
    newsc->color = sc->color;
    if ( sc->countermask_cnt!=0 ) {
	newsc->countermask_cnt = sc->countermask_cnt;
	newsc->countermasks = malloc(sc->countermask_cnt*sizeof(HintMask));
	memcpy(newsc->countermasks,sc->countermasks,sc->countermask_cnt*sizeof(HintMask));
    }
    newsc->tex_height = sc->tex_height;
    newsc->tex_depth = sc->tex_depth;
    newsc->italic_correction = sc->italic_correction;
    newsc->top_accent_horiz = sc->top_accent_horiz;
    newsc->is_extended_shape = sc->is_extended_shape;
    newsc->italic_adjusts = DeviceTableCopy(sc->italic_adjusts);
    newsc->top_accent_adjusts = DeviceTableCopy(sc->top_accent_adjusts);
    newsc->horiz_variants = GlyphVariantsCopy(sc->horiz_variants);
    newsc->vert_variants = GlyphVariantsCopy(sc->vert_variants);
    newsc->altuni = AltUniCopy(sc->altuni,NULL);
    newsc->lig_caret_cnt_fixed = sc->lig_caret_cnt_fixed;
    newsc->possub = CI_PSTCopy(sc->possub);
    newsc->kerns = CI_KPCopy(sc->kerns);
    newsc->vkerns = CI_KPCopy(sc->vkerns);
    newsc->tile_margin = sc->tile_margin;
    newsc->tile_bounds = sc->tile_bounds;
return( newsc );
}

static int CI_CheckMetaData(CharInfo *ci,SplineChar *oldsc,char *name,int unienc, char *comment) {
    SplineFont *sf = oldsc->parent;
    int i;
    int isnotdef, samename=false, sameuni=false;
    struct altuni *alt;
    SplineChar *newsc = ci->cachedsc;
    struct splinecharlist *scl, *baduniscl, *badnamescl;
    SplineChar *baduni, *badname;

    for ( alt=oldsc->altuni; alt!=NULL && (alt->unienc!=unienc || alt->vs!=-1 || alt->fid!=0); alt=alt->next );
    if ( unienc==oldsc->unicodeenc || alt!=NULL )
	sameuni=true;
    samename = ( oldsc->name!=NULL && strcmp(name,oldsc->name)==0 );
    
    isnotdef = strcmp(name,".notdef")==0;
    if (( !sameuni && unienc!=-1) || (!samename && !isnotdef) ) {
	baduniscl = badnamescl = NULL;
	baduni = badname = NULL;
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]!=ci->sc ) {
	    if ( unienc!=-1 && sf->glyphs[i]->unicodeenc==unienc ) {
		for ( scl=ci->changes; scl!=NULL && scl->sc->orig_pos!=i; scl = scl->next );
		if ( scl==NULL ) {
		    baduni = sf->glyphs[i];
		    baduniscl = NULL;
		} else if ( scl->sc->unicodeenc==unienc ) {
		    baduni = scl->sc;
		    baduniscl = scl;
		}
	    }
	    if ( !isnotdef && strcmp(name,sf->glyphs[i]->name )==0 ) {
		for ( scl=ci->changes; scl!=NULL && scl->sc->orig_pos!=i; scl = scl->next );
		if ( scl==NULL ) {
		    badname = sf->glyphs[i];
		    badnamescl = NULL;
		} else if ( strcmp(scl->sc->name,name)==0 ) {
		    badname = scl->sc;
		    badnamescl = scl;
		}
	    }
	}
	for ( scl=ci->changes; scl!=NULL ; scl = scl->next ) if ( scl->sc!=newsc ) {
	    if ( unienc!=-1 && scl->sc->unicodeenc==unienc ) {
		baduni = scl->sc;
		baduniscl = scl;
	    }
	    if ( !isnotdef && strcmp(scl->sc->name,name)==0 ) {
		badname = scl->sc;
		badnamescl = scl;
	    }
	}
	if ( baduni!=NULL || badname!=NULL ) {
	    char *buts[3];
	    buts[0] = _("_Yes"); buts[1]=_("_Cancel"); buts[2] = NULL;
	    if ( badname==baduni ) {
		if ( ff_ask(_("Multiple"),(const char **) buts,0,1,_("There is already a glyph with this name and encoding,\nboth must be unique within a font,\ndo you want to swap them?"))==1 )
return( false );
		/* If we're going to swap, then add the swapee to the list of */
		/*  things that need changing */
		if ( baduniscl==NULL ) {
		    baduni = CI_SCDuplicate(baduni);
		    baduniscl = chunkalloc(sizeof(struct splinecharlist));
		    baduniscl->sc = baduni;
		    baduniscl->next = ci->changes;
		    ci->changes = baduniscl;
		}
		baduni->unicodeenc = oldsc->unicodeenc;
		free(baduni->name); baduni->name = copy(oldsc->name);
	    } else {
		if ( baduni!=NULL ) {
		    if ( ff_ask(_("Multiple"),(const char **) buts,0,1,_("There is already a glyph with this encoding,\nwhich must be unique within a font,\ndo you want to swap the encodings of the two?"))==1 )
return( false );
		    if ( baduniscl==NULL ) {
			baduni = CI_SCDuplicate(baduni);
			baduniscl = chunkalloc(sizeof(struct splinecharlist));
			baduniscl->sc = baduni;
			baduniscl->next = ci->changes;
			ci->changes = baduniscl;
		    }
		    baduni->unicodeenc = oldsc->unicodeenc;
		}
		if ( badname!=NULL ) {
		    if ( ff_ask(_("Multiple"),(const char **) buts,0,1,_("There is already a glyph with this name,\nwhich must be unique within a font,\ndo you want to swap the names of the two?"))==1 )
return( false );
		    if ( badnamescl==NULL ) {
			badname = CI_SCDuplicate(badname);
			badnamescl = chunkalloc(sizeof(struct splinecharlist));
			badnamescl->sc = badname;
			badnamescl->next = ci->changes;
			ci->changes = badnamescl;
		    }
		    free(badname->name); badname->name = copy(oldsc->name);
		}
	    }
	}
    }
    if ( !samename )
	ci->name_change = true;
    if ( !sameuni )
	ci->uni_change = true;
    free( newsc->name ); free( newsc->comment );
    newsc->name = copy( name );
    newsc->unicodeenc = unienc;
    newsc->comment = copy( comment );
return( true );
}

static int _CI_OK(CharInfo *ci) {
    int val;
    int ret;
    char *name, *comment;
    const unichar_t *nm;
    int err = false;
    int tex_height, tex_depth, italic, topaccent;
    int hic, vic;
    int lc_cnt=-1;
    char *italicdevtab=NULL, *accentdevtab=NULL, *hicdt=NULL, *vicdt=NULL;
    int lig_caret_cnt_fixed=0;
    int low,high;
    real tile_margin=0;
    DBounds tileb;
    SplineChar *oldsc = ci->cachedsc==NULL ? ci->sc : ci->cachedsc;

    if ( !CI_ValidateAltUnis(ci))
return( false );

    val = ParseUValue(ci->gw,CID_UValue,true);
    if ( val==-2 )
return( false );
    tex_height = gettex(ci->gw,CID_TeX_Height,_("Height"),&err);
    tex_depth  = gettex(ci->gw,CID_TeX_Depth ,_("Depth") ,&err);
    italic     = gettex(ci->gw,CID_TeX_Italic,_("Italic Correction"),&err);
    topaccent  = gettex(ci->gw,CID_HorAccent,_("Top Accent Horizontal Pos"),&err);
    if ( err )
return( false );
    hic = GetInt8(ci->gw,CID_ExtItalicCor+1*100,_("Horizontal Extension Italic Correction"),&err);
    vic = GetInt8(ci->gw,CID_ExtItalicCor+0*100,_("Vertical Extension Italic Correction"),&err);
    if ( err )
return( false );

    memset(&tileb,0,sizeof(tileb));
    if ( ci->sc->parent->multilayer ) {
	if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_IsTileMargin)))
	    tile_margin = GetReal8(ci->gw,CID_TileMargin,_("Tile Margin"),&err);
	else {
	    tileb.minx = GetReal8(ci->gw,CID_TileBBoxMinX,_("Tile Min X"),&err);
	    tileb.miny = GetReal8(ci->gw,CID_TileBBoxMinY,_("Tile Min Y"),&err);
	    tileb.maxx = GetReal8(ci->gw,CID_TileBBoxMaxX,_("Tile Max X"),&err);
	    tileb.maxy = GetReal8(ci->gw,CID_TileBBoxMaxY,_("Tile Max Y"),&err);
	}
	if ( err )
return( false );
    }

    lig_caret_cnt_fixed = !GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_DefLCCount));
    if ( ci->lc_seen ) {
	lc_cnt = GetInt8(ci->gw,CID_LCCount,_("Ligature Caret Count"),&err);
	if ( err )
return( false );
	if ( lc_cnt<0 || lc_cnt>100 ) {
	    ff_post_error(_("Bad Lig. Caret Count"),_("Unreasonable ligature caret count"));
return( false );
	}
    }
    nm = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
    if ( !CI_NameCheck(nm) )
return( false );

    italicdevtab = GGadgetGetTitle8(GWidgetGetControl(ci->gw,CID_ItalicDevTab));
    accentdevtab = GGadgetGetTitle8(GWidgetGetControl(ci->gw,CID_AccentDevTab));
    hicdt = GGadgetGetTitle8(GWidgetGetControl(ci->gw,CID_ExtItalicDev+1*100));
    vicdt = GGadgetGetTitle8(GWidgetGetControl(ci->gw,CID_ExtItalicDev+0*100));
    if ( !DeviceTableOK(italicdevtab,&low,&high) || !DeviceTableOK(accentdevtab,&low,&high) ||
	    !DeviceTableOK(hicdt,&low,&high) || !DeviceTableOK(vicdt,&low,&high)) {
	ff_post_error( _("Bad Device Table Adjustment"),_("A device table adjustment specified for the MATH table is invalid") );
	free( accentdevtab );
	free( italicdevtab );
	free(hicdt); free(vicdt);
return( false );
    }
    if ( ci->cachedsc==NULL ) {
	struct splinecharlist *scl;
	ci->cachedsc = chunkalloc(sizeof(SplineChar));
	ci->cachedsc->orig_pos = ci->sc->orig_pos;
	ci->cachedsc->parent = ci->sc->parent;
	scl = chunkalloc(sizeof(struct splinecharlist));
	scl->sc = ci->cachedsc;
	scl->next = ci->changes;
	ci->changes = scl;
    }
    /* CI_ProcessPosSubs is the first thing which might change anything real */
    if ( !CI_ProcessPosSubs(ci)) {
	free( accentdevtab );
	free( italicdevtab );
	free(hicdt); free(vicdt);
return( false );
    }
    name = u2utf8_copy( nm );
    comment = GGadgetGetTitle8(GWidgetGetControl(ci->gw,CID_Comment));
    if ( comment!=NULL && *comment=='\0' ) {
	free(comment);
	comment=NULL;
    }
    ret = CI_CheckMetaData(ci,oldsc,name,val,comment);
    free(name); free(comment);
    if ( !ret ) {
	free( accentdevtab );
	free( italicdevtab );
	free(hicdt); free(vicdt);
return( false );
    }
    ci->cachedsc->unlink_rm_ovrlp_save_undo = GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_UnlinkRmOverlap));
    ci->cachedsc->glyph_class = GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_GClass));
    val = GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_Color));
    if ( val!=-1 )
	ci->cachedsc->color = (intpt) (std_colors[val].userdata);
    CI_ParseCounters(ci);
    ci->cachedsc->tex_height = tex_height;
    ci->cachedsc->tex_depth  = tex_depth;
    ci->cachedsc->italic_correction = italic;
    ci->cachedsc->top_accent_horiz = topaccent;
    ci->cachedsc->is_extended_shape = GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_IsExtended));
    ci->cachedsc->italic_adjusts = DeviceTableParse(ci->cachedsc->italic_adjusts,italicdevtab);
    ci->cachedsc->top_accent_adjusts = DeviceTableParse(ci->cachedsc->top_accent_adjusts,accentdevtab);
    ci->cachedsc->horiz_variants = CI_ParseVariants(ci->cachedsc->horiz_variants,ci,1,hicdt,hic,false);
    ci->cachedsc->vert_variants  = CI_ParseVariants(ci->cachedsc->vert_variants ,ci,0,vicdt,vic,false);

    free( accentdevtab );
    free( italicdevtab );
    free(hicdt); free(vicdt);

    CI_ParseAltUnis(ci);

    if ( ci->lc_seen ) {
	PST *pst, *prev=NULL;
	int i;
	ci->cachedsc->lig_caret_cnt_fixed = lig_caret_cnt_fixed;
	for ( pst = ci->cachedsc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next )
	    prev = pst;
	if ( pst==NULL && lc_cnt==0 )
	    /* Nothing to do */;
	else if ( pst!=NULL && lc_cnt==0 ) {
	    if ( prev==NULL )
		ci->cachedsc->possub = pst->next;
	    else
		prev->next = pst->next;
	    pst->next = NULL;
	    PSTFree(pst);
	} else {
	    if ( pst==NULL ) {
		pst = chunkalloc(sizeof(PST));
		pst->type = pst_lcaret;
		pst->next = ci->sc->possub;
		ci->cachedsc->possub = pst;
	    }
	    if ( lc_cnt>pst->u.lcaret.cnt )
		pst->u.lcaret.carets = realloc(pst->u.lcaret.carets,lc_cnt*sizeof(int16));
	    for ( i=pst->u.lcaret.cnt; i<lc_cnt; ++i )
		pst->u.lcaret.carets[i] = 0;
	    pst->u.lcaret.cnt = lc_cnt;
	}
    }

    ci->cachedsc->tile_margin = tile_margin;
    ci->cachedsc->tile_bounds = tileb;

return( ret );
}

static void CI_ApplyAll(CharInfo *ci) {
    int refresh_fvdi = false;
    struct splinecharlist *scl;
    SplineChar *cached, *sc;
    SplineFont *sf = ci->sc->parent;
    FontView *fvs;

    for ( scl = ci->changes; scl!=NULL; scl=scl->next ) {
	cached = scl->sc;
	sc = sf->glyphs[cached->orig_pos];
	SCPreserveState(sc,2);
	if ( strcmp(cached->name,sc->name)!=0 || cached->unicodeenc!=sc->unicodeenc )
	    refresh_fvdi = 1;
	if ( sc->name==NULL || strcmp( sc->name,cached->name )!=0 ) {
	    if ( sc->name!=NULL )
		SFGlyphRenameFixup(sf,sc->name,cached->name,false);
	    free(sc->name); sc->name = copy(cached->name);
	    sc->namechanged = true;
	    GlyphHashFree(sf);
	}
	if ( sc->unicodeenc != cached->unicodeenc ) {
	    struct splinecharlist *scl;
	    int layer;
	    RefChar *ref;
	    struct altuni *alt;

	    /* All references need the new unicode value */
	    for ( scl=sc->dependents; scl!=NULL; scl=scl->next ) {
		for ( layer=ly_back; layer<scl->sc->layer_cnt; ++layer )
		    for ( ref = scl->sc->layers[layer].refs; ref!=NULL; ref=ref->next )
			if ( ref->sc==sc )
			    ref->unicode_enc = cached->unicodeenc;
	    }
	    /* If the current unicode enc were in the list of alt unis */
	    /*  the user might have forgotten to remove it. So if s/he did */
	    /*  forget, swap the altuni value with the old value */
	    for ( alt=cached->altuni; alt!=NULL && (alt->unienc!=cached->unicodeenc || alt->vs!=-1 || alt->fid!=0); alt=alt->next );
	    if ( alt!=NULL )	/* alt->unienc==new value */
		alt->unienc = sc->unicodeenc;
	    sc->unicodeenc = cached->unicodeenc;
	}
	free(sc->comment); sc->comment = copy(cached->comment);
	sc->unlink_rm_ovrlp_save_undo = cached->unlink_rm_ovrlp_save_undo;
	sc->glyph_class = cached->glyph_class;
	if ( sc->color != cached->color )
	    refresh_fvdi = true;
	sc->color = cached->color;
	free(sc->countermasks);
	sc->countermask_cnt = cached->countermask_cnt;
	sc->countermasks = cached->countermasks;
	cached->countermasks = NULL; cached->countermask_cnt = 0;
	sc->tex_height = cached->tex_height;
	sc->tex_depth  = cached->tex_depth;
	sc->italic_correction = cached->italic_correction;
	sc->top_accent_horiz = cached->top_accent_horiz;
	sc->is_extended_shape = cached->is_extended_shape;
	DeviceTableFree(sc->italic_adjusts);
	DeviceTableFree(sc->top_accent_adjusts);
	sc->italic_adjusts = cached->italic_adjusts;
	sc->top_accent_adjusts = cached->top_accent_adjusts;
	cached->italic_adjusts = cached->top_accent_adjusts = NULL;
	GlyphVariantsFree(sc->horiz_variants);
	GlyphVariantsFree(sc->vert_variants);
	sc->horiz_variants = cached->horiz_variants;
	sc->vert_variants = cached->vert_variants;
	cached->horiz_variants = cached->vert_variants = NULL;
	AltUniFree(sc->altuni);
	sc->altuni = cached->altuni;
	cached->altuni = NULL;
	sc->lig_caret_cnt_fixed = cached->lig_caret_cnt_fixed;
	PSTFree(sc->possub);
	sc->possub = cached->possub;
	cached->possub = NULL;
	KernPairsFree(sc->kerns); KernPairsFree(sc->vkerns);
	sc->kerns = cached->kerns; sc->vkerns = cached->vkerns;
	cached->kerns = cached->vkerns = NULL;
	sc->tile_margin = cached->tile_margin;
	sc->tile_bounds = cached->tile_bounds;
	if ( !sc->changed ) {
	    sc->changed = true;
	    refresh_fvdi = true;
	}
	SCRefreshTitles(sc);
    }
    if ( ci->name_change || ci->uni_change ) {
	for ( fvs=(FontView *) sf->fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame ) {
	    /* Postscript encodings are by name, others are by unicode */
	    /* Hence slight differences in when we update the encoding */
	    if ( (ci->name_change && fvs->b.map->enc->psnames!=NULL ) ||
		    (ci->uni_change && fvs->b.map->enc->psnames==NULL )) {
		fvs->b.map->enc = &custom;
		FVSetTitle((FontViewBase *) fvs);
		refresh_fvdi = true;
	    }
	}
    }
    if ( refresh_fvdi ) {
	for ( fvs=(FontView *) sf->fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame ) {
	    GDrawRequestExpose(fvs->gw,NULL,false);	/* Redraw info area just in case this char is selected */
	    GDrawRequestExpose(fvs->v,NULL,false);	/* Redraw character area in case this char is on screen */
	}
    }
    if ( ci->changes )
	sf->changed = true;
}

static void CI_Finish(CharInfo *ci) {
    struct splinecharlist *scl, *next;

    for ( scl=ci->changes; scl!=NULL; scl=next ) {
	next = scl->next;
	SplineCharFree(scl->sc);
	chunkfree(scl,sizeof(*scl));
    }
    GDrawDestroyWindow(ci->gw);
}

static int CI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( _CI_OK(ci) ) {
	    CI_ApplyAll(ci);
	    CI_Finish(ci);
	}
    }
return( true );
}

static void CI_BoundsToMargin(CharInfo *ci) {
    int err=false;
    real margin = GetCalmReal8(ci->gw,CID_TileMargin,NULL,&err);
    DBounds b;
    char buffer[40];

    if ( err )
return;
    SplineCharFindBounds(ci->sc,&b);
    sprintf( buffer, "%g", (double)(b.minx-margin) );
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileBBoxMinX),buffer);
    sprintf( buffer, "%g", (double)(b.miny-margin) );
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileBBoxMinY),buffer);
    sprintf( buffer, "%g", (double)(b.maxx+margin) );
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileBBoxMaxX),buffer);
    sprintf( buffer, "%g", (double)(b.maxy+margin) );
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileBBoxMaxY),buffer);
}

static int CI_TileMarginChange(GGadget *g, GEvent *e) {
    CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));

    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus )
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_IsTileMargin),true);
    else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged )
	CI_BoundsToMargin(ci);
return( true );
}

/* Generate default settings for the entries in ligature lookup
 * subtables. */
static char *LigDefaultStr(int uni, char *name, int alt_lig ) {
    const unichar_t *alt=NULL, *pt;
    char *components = NULL, *tmp;
    int len;
    unichar_t hack[30], *upt;
    char buffer[80];

    /* If it's not (bmp) unicode we have no info on it */
    /*  Unless it looks like one of adobe's special ligature names */
    if ( uni==-1 || uni>=0x10000 )
	/* Nope */;
    else if ( isdecompositionnormative(uni) &&
		unicode_alternates[uni>>8]!=NULL &&
		(alt = unicode_alternates[uni>>8][uni&0xff])!=NULL ) {
	if ( alt[1]=='\0' )
	    alt = NULL;		/* Single replacements aren't ligatures */
	else if ( iscombining(alt[1]) && ( alt[2]=='\0' || iscombining(alt[2]))) {
	    if ( alt_lig != -10 )	/* alt_lig = 10 => mac unicode decomp */
		alt = NULL;		/* Otherwise, don't treat accented letters as ligatures */
	} else if (! is_LIGATURE_or_VULGAR_FRACTION((uint32)(uni)) &&
		uni!=0x152 && uni!=0x153 &&	/* oe ligature should not be standard */
		uni!=0x132 && uni!=0x133 &&	/* nor ij */
		(uni<0xfb2a || uni>0xfb4f) &&	/* Allow hebrew precomposed chars */
		uni!=0x215f &&
		!((uni>=0x0958 && uni<=0x095f) || uni==0x929 || uni==0x931 || uni==0x934)) {
	    alt = NULL;
	} else if ( (tmp=unicode_name(65))==NULL ) { /* test for 'A' to see if library exists */
	    if ( (uni>=0xbc && uni<=0xbe ) ||		/* Latin1 fractions */
		    (uni>=0x2153 && uni<=0x215e ) ||	/* other fractions */
		    (uni>=0xfb00 && uni<=0xfb06 ) ||	/* latin ligatures */
		    (uni>=0xfb13 && uni<=0xfb17 ) ||	/* armenian ligatures */
		    uni==0xfb17 ||			/* hebrew ligature */
		    (uni>=0xfb2a && uni<=0xfb4f ) ||	/* hebrew precomposed chars */
		    (uni>=0xfbea && uni<=0xfdcf ) ||	/* arabic ligatures */
		    (uni>=0xfdf0 && uni<=0xfdfb ) ||	/* arabic ligatures */
		    (uni>=0xfef5 && uni<=0xfefc ))	/* arabic ligatures */
		;	/* These are good */
	    else
		alt = NULL;
	} else
	    free(tmp); /* found 'A' means there is a library, now cleanup */
    }
    if ( alt==NULL ) {
	if ( name==NULL || alt_lig )
return( NULL );
	else
return( AdobeLigatureFormat(name));
    }

    if ( uni==0xfb03 && alt_lig==1 )
	components = copy("ff i");
    else if ( uni==0xfb04 && alt_lig==1 )
	components = copy("ff l");
    else if ( alt!=NULL ) {
	if ( alt[1]==0x2044 && (alt[2]==0 || alt[3]==0) && alt_lig==1 ) {
	    u_strcpy(hack,alt);
	    hack[1] = '/';
	    alt = hack;
	} else if ( alt_lig>0 )
return( NULL );

	if ( isarabisolated(uni) || isarabinitial(uni) || isarabmedial(uni) || isarabfinal(uni) ) {
	    /* If it is arabic, then convert from the unformed version to the formed */
	    if ( u_strlen(alt)<sizeof(hack)/sizeof(hack[0])-1 ) {
		u_strcpy(hack,alt);
		for ( upt=hack ; *upt ; ++upt ) {
		    /* Make everything medial */
		    if ( *upt>=0x600 && *upt<=0x6ff )
			*upt = ArabicForms[*upt-0x600].medial;
		}
		if ( isarabisolated(uni) || isarabfinal(uni) ) {
		    int len = upt-hack-1;
		    if ( alt[len]>=0x600 && alt[len]<=0x6ff )
			hack[len] = ArabicForms[alt[len]-0x600].final;
		}
		if ( isarabisolated(uni) || isarabinitial(uni) ) {
		    if ( alt[0]>=0x600 && alt[0]<=0x6ff )
			hack[0] = ArabicForms[alt[0]-0x600].initial;
		}
		alt = hack;
	    }
	}

	components=NULL;
	while ( 1 ) {
	    len = 0;
	    for ( pt=alt; *pt; ++pt ) {
		if ( components==NULL ) {
		    len += strlen(StdGlyphName(buffer,*pt,ui_none,(NameList *)-1))+1;
		} else {
		    const char *temp = StdGlyphName(buffer,*pt,ui_none,(NameList *)-1);
		    strcpy(components+len,temp);
		    len += strlen( temp );
		    components[len++] = ' ';
		}
	    }
	    if ( components!=NULL )
	break;
	    components = malloc(len+1);
	}
	components[len-1] = '\0';
    }
return( components );
}

char *AdobeLigatureFormat(char *name) {
    /* There are two formats for ligs: <glyph-name>_<glyph-name>{...} or */
    /*  uni<code><code>{...} (only works for BMP) */
    /* I'm not checking to see if all the components are valid */
    char *components, *pt, buffer[12];
    const char *next;
    int len = strlen(name), uni;

    if ( strncmp(name,"uni",3)==0 && (len-3)%4==0 && len>7 ) {
	pt = name+3;
	components = malloc(1); *components = '\0';
	while ( *pt ) {
	    if ( sscanf(pt,"%4x", (unsigned *) &uni )==0 ) {
		free(components); components = NULL;
	break;
	    }
	    next = StdGlyphName(buffer,uni,ui_none,(NameList *)-1);
	    components = realloc(components,strlen(components) + strlen(next) + 2);
	    if ( *components!='\0' )
		strcat(components," ");
	    strcat(components,next);
	    pt += 4;
	}
	if ( components!=NULL )
return( components );
    }

    if ( strchr(name,'_')==NULL )
return( NULL );
    pt = components = copy(name);
    while ( (pt = strchr(pt,'_'))!=NULL )
	*pt = ' ';
return( components );
}

uint32 LigTagFromUnicode(int uni) {
    int tag = CHR('l','i','g','a');	/* standard */

    if (( uni>=0xbc && uni<=0xbe ) || (uni>=0x2153 && uni<=0x215f) )
	tag = CHR('f','r','a','c');	/* Fraction */
    /* hebrew precomposed characters */
    else if ( uni>=0xfb2a && uni<=0xfb4e )
	tag = CHR('c','c','m','p');
    else if ( uni==0xfb4f )
	tag = CHR('h','l','i','g');
    /* armenian */
    else if ( uni>=0xfb13 && uni<=0xfb17 )
	tag = CHR('l','i','g','a');
    /* devanagari ligatures */
    else if ( (uni>=0x0958 && uni<=0x095f) || uni==0x931 || uni==0x934 || uni==0x929 )
	tag = CHR('n','u','k','t');
    else switch ( uni ) {
      case 0xfb05:		/* long-s t */
	/* This should be 'liga' for long-s+t and 'hlig' for s+t */
	tag = CHR('l','i','g','a');
      break;
      case 0x00c6: case 0x00e6:		/* ae, AE */
      case 0x0152: case 0x0153:		/* oe, OE */
      case 0x0132: case 0x0133:		/* ij, IJ */
      case 0xfb06:			/* s t */
	tag = CHR('d','l','i','g');
      break;
      case 0xfefb: case 0xfefc:	/* Lam & Alef, required ligs */
	tag = CHR('r','l','i','g');
      break;
    }
return( tag );
}

SplineChar *SuffixCheck(SplineChar *sc,char *suffix) {
    SplineChar *alt = NULL;
    SplineFont *sf = sc->parent;
    char namebuf[200];

    if ( *suffix=='.' ) ++suffix;
    if ( sf->cidmaster!=NULL ) {
	sprintf( namebuf, "%.20s.%d.%.80s", sf->cidmaster->ordering, sc->orig_pos, suffix );
	alt = SFGetChar(sf,-1,namebuf);
	if ( alt==NULL ) {
	    sprintf( namebuf, "cid-%d.%.80s", sc->orig_pos, suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
    }
    if ( alt==NULL && sc->unicodeenc!=-1 ) {
	sprintf( namebuf, "uni%04X.%.80s", sc->unicodeenc, suffix );
	alt = SFGetChar(sf,-1,namebuf);
    }
    if ( alt==NULL ) {
	sprintf( namebuf, "glyph%d.%.80s", sc->orig_pos, suffix );
	alt = SFGetChar(sf,-1,namebuf);
    }
    if ( alt==NULL ) {
	sprintf( namebuf, "%.80s.%.80s", sc->name, suffix );
	alt = SFGetChar(sf,-1,namebuf);
    }
return( alt );
}

static SplineChar *SuffixCheckCase(SplineChar *sc,char *suffix, int cvt2lc ) {
    SplineChar *alt = NULL;
    SplineFont *sf = sc->parent;
    char namebuf[100];

    if ( *suffix=='.' ) ++suffix;
    if ( sf->cidmaster!=NULL )
return( NULL );

    /* Small cap characters are sometimes named "a.sc" */
    /*  and sometimes "A.small" */
    /* So if I want a 'smcp' feature I must convert "a" to "A.small" */
    /* And if I want a 'c2sc' feature I must convert "A" to "a.sc" */
    if ( cvt2lc ) {
	if ( alt==NULL && sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
		isupper(sc->unicodeenc)) {
	    sprintf( namebuf, "uni%04X.%s", tolower(sc->unicodeenc), suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
	if ( alt==NULL && isupper(*sc->name)) {
	    sprintf( namebuf, "%c%s.%s", tolower(*sc->name), sc->name+1, suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
    } else {
	if ( alt==NULL && sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
		islower(sc->unicodeenc)) {
	    sprintf( namebuf, "uni%04X.%s", toupper(sc->unicodeenc), suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
	if ( alt==NULL && islower(*sc->name)) {
	    sprintf( namebuf, "%c%s.%s", toupper(*sc->name), sc->name+1, suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
    }
return( alt );
}

void SCLigCaretCheck(SplineChar *sc,int clean) {
    PST *pst, *carets=NULL, *prev_carets=NULL, *prev;
    int lig_comp_max=0, lc, i;
    char *pt;
    /* Check to see if this is a ligature character, and if so, does it have */
    /*  a ligature caret structure. If a lig but no lig caret structure then */
    /*  create a lig caret struct */
    /* This is not entirely sufficient. If we have an old type1 font with afm */
    /*  file then there was no way of saying "ffi = f + f + i" instead you    */
    /*  said "ffi = ff + i" (only two component ligatures allowed). This means*/
    /*  we'd get the wrong number of lcaret positions */

    if ( sc->lig_caret_cnt_fixed )
return;

    for ( pst=sc->possub, prev=NULL; pst!=NULL; prev = pst, pst=pst->next ) {
	if ( pst->type == pst_lcaret ) {
	    if ( carets!=NULL )
		IError("Too many ligature caret structures" );
	    else {
		carets = pst;
		prev_carets = prev;
	    }
	} else if ( pst->type==pst_ligature ) {
	    for ( lc=0, pt=pst->u.lig.components; *pt; ++pt )
		if ( *pt==' ' ) ++lc;
	    if ( lc>lig_comp_max )
		lig_comp_max = lc;
	}
    }
    if ( lig_comp_max == 0 ) {
	if ( clean && carets!=NULL ) {
	    if ( prev_carets==NULL )
		sc->possub = carets->next;
	    else
		prev_carets->next = carets->next;
	    carets->next = NULL;
	    PSTFree(carets);
	}
return;
    }
    if ( carets==NULL ) {
	carets = chunkalloc(sizeof(PST));
	carets->type = pst_lcaret;
	carets->subtable = NULL;		/* Not relevant here */
	carets->next = sc->possub;
	sc->possub = carets;
    }
    if ( carets->u.lcaret.cnt>=lig_comp_max ) {
	carets->u.lcaret.cnt = lig_comp_max;
return;
    }
    if ( carets->u.lcaret.carets==NULL )
	carets->u.lcaret.carets = (int16 *) calloc(lig_comp_max,sizeof(int16));
    else {
	carets->u.lcaret.carets = (int16 *) realloc(carets->u.lcaret.carets,lig_comp_max*sizeof(int16));
	for ( i=carets->u.lcaret.cnt; i<lig_comp_max; ++i )
	    carets->u.lcaret.carets[i] = 0;
    }
    carets->u.lcaret.cnt = lig_comp_max;
}

static int CI_SName(GGadget *g, GEvent *e) {	/* Set From Name */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
	int i;
	char buf[40], *ctemp; unichar_t ubuf[2], *temp;
	ctemp = u2utf8_copy(ret);
	i = UniFromName(ctemp,ui_none,&custom);
	free(ctemp);
	if ( i==-1 ) {
	    /* Adobe says names like uni00410042 represent a ligature (A&B) */
	    /*  (that is "uni" followed by two (or more) 4-digit codes). */
	    /* But that names outside of BMP should be uXXXX or uXXXXX or uXXXXXX */
	    if ( ret[0]=='u' && ret[1]!='n' && u_strlen(ret)<=1+6 ) {
		unichar_t *end;
		i = u_strtol(ret+1,&end,16);
		if ( *end )
		    i = -1;
		else		/* Make sure it is properly capitalized */
		    SetNameFromUnicode(ci->gw,CID_UName,i);
	    }
	}

	sprintf(buf,"U+%04x", i);
	temp = uc_copy(i==-1?"-1":buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);

	ubuf[0] = i;
	if ( i==-1 || i>0xffff )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

static int CI_SValue(GGadget *g, GEvent *e) {	/* Set From Value */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t ubuf[2];
	int val;

	val = ParseUValue(ci->gw,CID_UValue,false);
	if ( val<0 )
return( true );

	SetNameFromUnicode(ci->gw,CID_UName,val);

	ubuf[0] = val;
	if ( val==-1 )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

GTextInfo *TIFromName(const char *name) {
    GTextInfo *ti = calloc(1,sizeof(GTextInfo));
    ti->text = utf82u_copy(name);
    ti->fg = COLOR_DEFAULT;
    ti->bg = COLOR_DEFAULT;
return( ti );
}

static void CI_SetNameList(CharInfo *ci,int val) {
    GGadget *g = GWidgetGetControl(ci->gw,CID_UName);
    int cnt;

    if ( GGadgetGetUserData(g)==(void *) (intpt) val )
return;		/* Didn't change */
    {
	GTextInfo **list = NULL;
	char **names = AllGlyphNames(val,ci->sc->parent->for_new_glyphs,ci->sc);

	for ( cnt=0; names[cnt]!=NULL; ++cnt );
	list = malloc((cnt+1)*sizeof(GTextInfo*)); 
	for ( cnt=0; names[cnt]!=NULL; ++cnt ) {
	    list[cnt] = TIFromName(names[cnt]);
	    free(names[cnt]);
	}
	free(names);
	list[cnt] = TIFromName(NULL);
	GGadgetSetList(g,list,true);
    }
    GGadgetSetUserData(g,(void *) (intpt) val);
}

static int CI_UValChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UValue));
	unichar_t *end;
	int val;

	if (( *ret=='U' || *ret=='u' ) && ret[1]=='+' )
	    ret += 2;
	val = u_strtol(ret,&end,16);
	if ( *end=='\0' )
	    CI_SetNameList(ci,val);
    }
return( true );
}

static int CI_CharChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UChar));
	int val = *ret;
	unichar_t *temp, ubuf[2]; char buf[10];

	if ( ret[0]=='\0' )
return( true );
	else if ( ret[1]!='\0' ) {
	    ff_post_notice(_("Only a single character allowed"),_("Only a single character allowed"));
	    ubuf[0] = ret[0];
	    ubuf[1] = '\0';
	    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
return( true );
	}

	SetNameFromUnicode(ci->gw,CID_UName,val);
	CI_SetNameList(ci,val);

	sprintf(buf,"U+%04x", val);
	temp = uc_copy(buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);
    }
return( true );
}

static int CI_CommentChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	/* Let's give things with comments a white color. This may not be a good idea */
	if ( ci->first && ci->sc->color==COLOR_DEFAULT &&
		0==GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_Color)) )
	    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),1);
	ci->first = false;
    }
return( true );
}

struct devtab_dlg {
    int done;
    GWindow gw;
    GGadget *gme;
    DeviceTable devtab;
};

static struct col_init devtabci[] = {
    { me_int, NULL, NULL, NULL, N_("Pixel Size") },
    { me_int, NULL, NULL, NULL, N_("Correction") },
    COL_INIT_EMPTY
};

static void DevTabMatrixInit(struct matrixinit *mi,char *dvstr) {
    struct matrix_data *md;
    int k, p, cnt;
    DeviceTable devtab;

    memset(&devtab,0,sizeof(devtab));
    DeviceTableParse(&devtab,dvstr);
    cnt = 0;
    if ( devtab.corrections!=NULL ) {
	for ( k=devtab.first_pixel_size; k<=devtab.last_pixel_size; ++k )
	    if ( devtab.corrections[k-devtab.first_pixel_size]!=0 )
		++cnt;
    }

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 2;
    mi->col_init = devtabci;

    md = NULL;
    for ( k=0; k<2; ++k ) {
	cnt = 0;
	if ( devtab.corrections==NULL )
	    /* Do Nothing */;
	else for ( p=devtab.first_pixel_size; p<=devtab.last_pixel_size; ++p ) {
	    if ( devtab.corrections[p-devtab.first_pixel_size]!=0 ) {
		if ( k ) {
		    md[2*cnt+0].u.md_ival = p;
		    md[2*cnt+1].u.md_ival = devtab.corrections[p-devtab.first_pixel_size];
		}
		++cnt;
	    }
	}
	if ( md==NULL )
	    md = calloc(2*(cnt+10),sizeof(struct matrix_data));
    }
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;

    mi->initrow = NULL;
    mi->finishedit = NULL;
    mi->candelete = NULL;
    mi->popupmenu = NULL;
    mi->handle_key = NULL;
    mi->bigedittitle = NULL;

    free( devtab.corrections );
}

static int DevTabDlg_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct devtab_dlg *dvd = GDrawGetUserData(GGadgetGetWindow(g));
	int rows, i, low, high;
	struct matrix_data *corrections = GMatrixEditGet(dvd->gme, &rows);

	low = high = -1;
	for ( i=0; i<rows; ++i ) {
	    if ( corrections[2*i+1].u.md_ival<-128 || corrections[2*i+1].u.md_ival>127 ) {
		ff_post_error(_("Bad correction"),_("The correction on line %d is too big.  It must be between -128 and 127"),
			i+1 );
return(true);
	    } else if ( corrections[2*i+0].u.md_ival<0 || corrections[2*i+0].u.md_ival>32767 ) {
		gwwv_post_error(_("Bad pixel size"),_("The pixel size on line %d is out of bounds."),
			i+1 );
return(true);
	    }
	    if ( corrections[2*i+1].u.md_ival!=0 ) {
		if ( low==-1 )
		    low = high = corrections[2*i+0].u.md_ival;
		else if ( corrections[2*i+0].u.md_ival<low )
		    low = corrections[2*i+0].u.md_ival;
		else if ( corrections[2*i+0].u.md_ival>high )
		    high = corrections[2*i+0].u.md_ival;
	    }
	}
	memset(&dvd->devtab,0,sizeof(DeviceTable));
	if ( low!=-1 ) {
	    dvd->devtab.first_pixel_size = low;
	    dvd->devtab.last_pixel_size = high;
	    dvd->devtab.corrections = calloc(high-low+1,1);
	    for ( i=0; i<rows; ++i ) {
		if ( corrections[2*i+1].u.md_ival!=0 ) {
		    dvd->devtab.corrections[ corrections[2*i+0].u.md_ival-low ] =
			    corrections[2*i+1].u.md_ival;
		}
	    }
	}
	dvd->done = 2;
    }
return( true );
}

static int DevTabDlg_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct devtab_dlg *dvd = GDrawGetUserData(GGadgetGetWindow(g));
	dvd->done = true;
    }
return( true );
}

static int devtabdlg_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	struct devtab_dlg *dvd = GDrawGetUserData(gw);
	dvd->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }

return( true );
}

char *DevTab_Dlg(GGadget *g, int r, int c) {
    int rows, k, j, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    char *dvstr = strings[cols*r+c].u.md_str;
    struct devtab_dlg dvd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[4], boxes[3];
    GGadgetCreateData *varray[6], *harray3[8];
    GTextInfo label[4];
    struct matrixinit mi;

    memset(&dvd,0,sizeof(dvd));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Device Table Adjustments");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268));
    pos.height = GDrawPointsToPixels(NULL,375);
    dvd.gw = gw = GDrawCreateTopWindow(NULL,&pos,devtabdlg_e_h,&dvd,&wattrs);

    DevTabMatrixInit(&mi,dvstr);

    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&label,0,sizeof(label));
    k=j=0;
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.u.matrix = &mi;
    gcd[k].gd.popup_msg = (unichar_t *) _(
	"At small pixel sizes (screen font sizes)\n"
	"the rounding errors that occur may be\n"
	"extremely ugly. A device table allows\n"
	"you to specify adjustments to the rounded\n"
	"Every pixel size my have its own adjustment.");
    gcd[k].creator = GMatrixEditCreate;
    varray[j++] = &gcd[k++]; varray[j++] = NULL;

    gcd[k].gd.pos.x = 30-3; 
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = DevTabDlg_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -30;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = DevTabDlg_Cancel;
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

    dvd.gme = gcd[0].ret;
    GMatrixEditSetNewText(gcd[0].ret,S_("PixelSize|New"));
    GHVBoxSetExpandableRow(boxes[1].ret,1);
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);

    GHVBoxFitWindow(boxes[1].ret);

    GDrawSetVisible(gw,true);
    while ( !dvd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    if ( dvd.done==2 ) {
	char *ret;
	DevTabToString(&ret,&dvd.devtab);
	free(dvd.devtab.corrections);
return( ret );
    } else
return( copy(dvstr));
}

static void finishedit(GGadget *g, int r, int c, int wasnew);
static void kernfinishedit(GGadget *g, int r, int c, int wasnew);
static void kerninit(GGadget *g, int r);
static void enable_enum(GGadget *g, GMenuItem *mi, int r, int c);

static struct col_init simplesubsci[] = {
    { me_enum , NULL, NULL, enable_enum, N_("Subtable") },
    { me_string, NULL, NULL, NULL, N_("Replacement Glyph Name") },
    COL_INIT_EMPTY
};
static struct col_init ligatureci[] = {
    { me_enum , NULL, NULL, NULL, N_("Subtable") },	/* There can be multiple ligatures for a glyph */
    { me_string, NULL, NULL, NULL, N_("Source Glyph Names") },
    COL_INIT_EMPTY
};
static struct col_init altsubsci[] = {
    { me_enum , NULL, NULL, enable_enum, N_("Subtable") },
    { me_string, NULL, NULL, NULL, N_("Replacement Glyph Names") },
    COL_INIT_EMPTY
};
static struct col_init multsubsci[] = {
    { me_enum , NULL, NULL, enable_enum, N_("Subtable") },
    { me_string, NULL, NULL, NULL, N_("Replacement Glyph Names") },
    COL_INIT_EMPTY
};
static struct col_init simpleposci[] = {
    { me_enum , NULL, NULL, enable_enum, N_("Subtable") },
    { me_int, NULL, NULL, NULL, NU_("∆x") },	/* delta-x */
/* GT: "Adjust" here means Device Table based pixel adjustments, an OpenType */
/* GT: concept which allows small corrections for small pixel sizes where */
/* GT: rounding errors (in kerning for example) may smush too glyphs together */
/* GT: or space them too far apart. Generally not a problem for big pixelsizes*/
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y") },		/* delta-y */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆x_adv") },	/* delta-x-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y_adv") },	/* delta-y-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    COL_INIT_EMPTY
};
static struct col_init pairposci[] = {
    { me_enum , NULL, NULL, NULL, N_("Subtable") },	/* There can be multiple kern-pairs for a glyph */
    { me_string , DevTab_Dlg, NULL, NULL, N_("Second Glyph Name") },
    { me_int, NULL, NULL, NULL, NU_("∆x #1") },		/* delta-x */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y #1") },		/* delta-y */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆x_adv #1") },	/* delta-x-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y_adv #1") },	/* delta-y-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆x #2") },		/* delta-x */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y #2") },		/* delta-y */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆x_adv #2") },	/* delta-x-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y_adv #2") },	/* delta-y-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    COL_INIT_EMPTY
};
static int pst2lookuptype[] = { ot_undef, gpos_single, gpos_pair, gsub_single,
     gsub_alternate, gsub_multiple, gsub_ligature, 0 };
struct matrixinit mi[] = {
    { sizeof(simpleposci)/sizeof(struct col_init)-1, simpleposci, 0, NULL, NULL, NULL, finishedit, NULL, NULL, NULL },
    { sizeof(pairposci)/sizeof(struct col_init)-1, pairposci, 0, NULL, kerninit, NULL, kernfinishedit, NULL, NULL, NULL },
    { sizeof(simplesubsci)/sizeof(struct col_init)-1, simplesubsci, 0, NULL, NULL, NULL, finishedit, NULL, NULL, NULL },
    { sizeof(altsubsci)/sizeof(struct col_init)-1, altsubsci, 0, NULL, NULL, NULL, finishedit, NULL, NULL, NULL },
    { sizeof(multsubsci)/sizeof(struct col_init)-1, multsubsci, 0, NULL, NULL, NULL, finishedit, NULL, NULL, NULL },
    { sizeof(ligatureci)/sizeof(struct col_init)-1, ligatureci, 0, NULL, NULL, NULL, finishedit, NULL, NULL, NULL },
    MATRIXINIT_EMPTY
};

static void enable_enum(GGadget *g, GMenuItem *mi, int r, int c) {
    int i,rows,j;
    struct matrix_data *possub;
    CharInfo *ci;
    int sel,cols;

    if ( c!=0 )
return;
    ci = GDrawGetUserData(GGadgetGetWindow(g));
    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    possub = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);

    ci->old_sub = (void *) possub[r*cols+0].u.md_ival;

    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].ti.line )	/* Lines, and the new entry always enabled */
	    mi[i].ti.disabled = false;
	else if ( mi[i].ti.userdata == NULL )
	    /* One of the lookup (rather than subtable) entries. leave disabled */;
	else if ( mi[i].ti.userdata == (void *) possub[r*cols+0].u.md_ival ) {
	    mi[i].ti.selected = true;		/* Current thing, they can keep on using it */
	    mi[i].ti.disabled = false;
	} else {
	    for ( j=0; j<rows; ++j )
		if ( mi[i].ti.userdata == (void *) possub[j*cols+0].u.md_ival ) {
		    mi[i].ti.selected = false;
		    mi[i].ti.disabled = true;
	    break;
		}
	    if ( j==rows ) {	/* This subtable hasn't been used yet */
		mi[i].ti.disabled = false;
	    }
	}
    }
}

void SCSubtableDefaultSubsCheck(SplineChar *sc, struct lookup_subtable *sub,
	struct matrix_data *possub, int col_cnt, int r, int layer) {
    FeatureScriptLangList *fl;
    int lookup_type = sub->lookup->lookup_type;
    SplineChar *alt;
    char buffer[8];
    int i;
    static uint32 form_tags[] = { CHR('i','n','i','t'), CHR('m','e','d','i'), CHR('f','i','n','a'), CHR('i','s','o','l'), 0 };
    real loff, roff;

    if ( lookup_type == gsub_single && sub->suffix != NULL ) {
	alt = SuffixCheck(sc,sub->suffix);
	if ( alt!=NULL ) {
	    possub[r*col_cnt+1].u.md_str = copy( alt->name );
return;
	}
    }

    for ( fl = sub->lookup->features; fl!=NULL; fl=fl->next ) {
	if ( lookup_type == gpos_single ) {
	    /* These too features are designed to crop off the left and right */
	    /*  side bearings respectively */
	    if ( fl->featuretag == CHR('l','f','b','d') ) {
		GuessOpticalOffset(sc,layer,&loff,&roff,0);
		/* Adjust horixontal positioning and horizontal advance by */
		/*  the left side bearing */
		possub[r*col_cnt+SIM_DX].u.md_ival = -loff;
		possub[r*col_cnt+SIM_DX_ADV].u.md_ival = -loff;
return;
	    } else if ( fl->featuretag == CHR('r','t','b','d') ) {
		GuessOpticalOffset(sc,layer,&loff,&roff,0);
		/* Adjust horizontal advance by right side bearing */
		possub[r*col_cnt+SIM_DX_ADV].u.md_ival = -roff;
return;
	    }
	} else if ( lookup_type == gsub_single ) {
	    alt = NULL;
	    if ( fl->featuretag == CHR('s','m','c','p') ) {
		alt = SuffixCheck(sc,"sc");
		if ( alt==NULL )
		    alt = SuffixCheckCase(sc,"small",false);
	    } else if ( fl->featuretag == CHR('c','2','s','c') ) {
		alt = SuffixCheck(sc,"small");
		if ( alt==NULL )
		    alt = SuffixCheckCase(sc,"sc",true);
	    } else if ( fl->featuretag == CHR('r','t','l','a') ) {
		if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && tomirror(sc->unicodeenc)!=0 )
		    alt = SFGetChar(sc->parent,tomirror(sc->unicodeenc),NULL);
	    } else if ( sc->unicodeenc==0x3c3 && fl->featuretag==CHR('f','i','n','a') ) {
		/* Greek final sigma */
		alt = SFGetChar(sc->parent,0x3c2,NULL);
	    }
	    if ( alt==NULL ) {
		buffer[0] = fl->featuretag>>24;
		buffer[1] = fl->featuretag>>16;
		buffer[2] = fl->featuretag>>8;
		buffer[3] = fl->featuretag&0xff;
		buffer[4] = 0;
		alt = SuffixCheck(sc,buffer);
	    }
	    if ( alt==NULL && sc->unicodeenc>=0x600 && sc->unicodeenc<0x700 ) {
		/* Arabic forms */
		for ( i=0; form_tags[i]!=0; ++i ) if ( form_tags[i]==fl->featuretag ) {
		    if ( (&(ArabicForms[sc->unicodeenc-0x600].initial))[i]!=0 &&
			    (&(ArabicForms[sc->unicodeenc-0x600].initial))[i]!=sc->unicodeenc &&
			    (alt = SFGetChar(sc->parent,(&(ArabicForms[sc->unicodeenc-0x600].initial))[i],NULL))!=NULL )
		break;
		}
	    }
	    if ( alt!=NULL ) {
		possub[r*col_cnt+1].u.md_str = copy( alt->name );
return;
	    }
	} else if ( lookup_type == gsub_ligature ) {
	    if ( fl->featuretag == LigTagFromUnicode(sc->unicodeenc) ) {
		int alt_index;
		for ( alt_index = 0; ; ++alt_index ) {
		    char *components = LigDefaultStr(sc->unicodeenc,sc->name,alt_index);
		    if ( components==NULL )
		break;
		    for ( i=0; i<r; ++i ) {
			if ( possub[i*col_cnt+0].u.md_ival == (intpt) sub &&
				strcmp(possub[i*col_cnt+1].u.md_str,components)==0 )
		    break;
		    }
		    if ( i==r ) {
			possub[r*col_cnt+1].u.md_str = components;
return;
		    }
		    free( components );
		}
	    }
	}
    }
}

static void finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *possub;
    CharInfo *ci;
    int sel,cols;
    struct lookup_subtable *sub;
    struct subtable_data sd;
    GTextInfo *ti;

    if ( c!=0 )
return;
    ci = GDrawGetUserData(GGadgetGetWindow(g));
    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    possub = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( possub[r*cols+0].u.md_ival!=0 ) {
	if ( wasnew )
	    SCSubtableDefaultSubsCheck(ci->sc,(struct lookup_subtable *) possub[r*cols+0].u.md_ival, possub,
		    cols, r, ci->def_layer );
return;
    }
    /* They asked to create a new subtable */

    memset(&sd,0,sizeof(sd));
    sd.flags = sdf_dontedit;
    sub = SFNewLookupSubtableOfType(ci->sc->parent,pst2lookuptype[sel+1],&sd,ci->def_layer);
    if ( sub!=NULL ) {
	possub[r*cols+0].u.md_ival = (intpt) sub;
	ti = SFSubtableListOfType(ci->sc->parent, pst2lookuptype[sel+1], false, false);
	GMatrixEditSetColumnChoices(g,0,ti);
	GTextInfoListFree(ti);
	if ( wasnew && ci->cv!=NULL )
	    SCSubtableDefaultSubsCheck(ci->sc,sub, possub, cols, r, CVLayer((CharViewBase *) (ci->cv)));
    } else if ( ci->old_sub!=NULL ) {
	/* Restore old value */
	possub[r*cols+0].u.md_ival = (intpt) ci->old_sub;
    } else {
	GMatrixEditDeleteRow(g,r);
    }
    ci->old_sub = NULL;
    GGadgetRedraw(g);
}

static void kern_AddKP(void *data,SplineChar *left, SplineChar *right, int off) {
    int *kp_offset = data;
    *kp_offset = off;
}

static void kernfinishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *possub;
    CharInfo *ci;
    int cols;
    struct lookup_subtable *sub;
    SplineChar *lefts[2], *rights[2];
    int touch, separation, kp_offset=0;
    SplineChar *osc;

    if ( c==1 ) {
	ci = GDrawGetUserData(GGadgetGetWindow(g));
	possub = GMatrixEditGet(g, &rows);
	cols = GMatrixEditGetColCnt(g);
	sub = (struct lookup_subtable *) possub[r*cols+0].u.md_ival;
	if ( possub[r*cols+PAIR_DX_ADV1].u.md_ival==0 &&
		possub[r*cols+1].u.md_str!=NULL &&
		(osc = SFGetChar(ci->sc->parent,-1,possub[r*cols+1].u.md_str))!=NULL ) {
	    lefts[1] = rights[1] = NULL;
	    if ( sub->lookup->lookup_flags & pst_r2l ) {
		lefts[0] = osc;
		rights[0] = ci->sc;
	    } else {
		lefts[0] = ci->sc;
		rights[0] = osc;
	    }
	    touch = sub->kerning_by_touch;
	    separation = sub->separation;
	    if ( separation==0 && !touch )
		separation = 15*(osc->parent->ascent+osc->parent->descent)/100;
	    AutoKern2(osc->parent,ci->def_layer,lefts,rights,sub,
		    separation,0,touch,0,0,	/* Don't bother with minkern or onlyCloser, they asked for this, they get it, whatever it may be */
		    kern_AddKP,&kp_offset);
	    possub[r*cols+PAIR_DX_ADV1].u.md_ival=kp_offset;
	}
    } else
	finishedit(g,r,c,wasnew);
}

static int SubHasScript(uint32 script,struct lookup_subtable *sub) {
    FeatureScriptLangList *f;
    struct scriptlanglist *s;

    if ( sub==NULL )
return(false);
    for ( f = sub->lookup->features; f!=NULL; f=f->next ) {
	for ( s=f->scripts; s!=NULL; s=s->next ) {
	    if ( s->script == script )
return( true );
	}
    }
return( false );
}

static void kerninit(GGadget *g, int r) {
    CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
    GMenuItem *mi = GMatrixEditGetColumnChoices(g,0);
    int i,cols,rows;
    struct matrix_data *possub;
    uint32 script;

    possub = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);

    if ( r!=0 )
	possub[r*cols+0].u.md_ival = possub[(r-1)*cols+0].u.md_ival;
    else {
	script = SCScriptFromUnicode(ci->sc);
	for ( i=0; mi[i].ti.line || mi[i].ti.text!=NULL; ++i ) {
	    if ( SubHasScript(script,(struct lookup_subtable *) mi[i].ti.userdata ) )
	break;
	}
	if ( mi[i].ti.line || mi[i].ti.text!=NULL )
	    possub[r*cols+0].u.md_ival = (intpt) mi[i].ti.userdata;
    }
}

static void CI_DoHideUnusedSingle(CharInfo *ci) {
    GGadget *pstk = GWidgetGetControl(ci->gw,CID_List+(pst_position-1)*100);
    int rows, cols = GMatrixEditGetColCnt(pstk);
    struct matrix_data *old = GMatrixEditGet(pstk,&rows);
    uint8 cols_used[20];
    int r, col, tot;

    if ( lookup_hideunused ) {
	memset(cols_used,0,sizeof(cols_used));
	for ( r=0; r<rows; ++r ) {
	    for ( col=1; col<cols; col+=2 ) {
		if ( old[cols*r+col].u.md_ival!=0 )
		    cols_used[col] = true;
		if ( old[cols*r+col+1].u.md_str!=NULL && *old[cols*r+col+1].u.md_str!='\0' )
		    cols_used[col+1] = true;
	    }
	}
	for ( col=1, tot=0; col<cols; ++col )
	    tot += cols_used[col];
	/* If no columns used (no info yet, all info is to preempt a kernclass and sets to 0) */
	/*  then show what we expect to be the default column for this kerning mode*/
	if ( tot==0 ) {
	    if ( strstr(ci->sc->name,".vert")!=NULL || strstr(ci->sc->name,".vrt2")!=NULL )
		cols_used[SIM_DY] = true;
	    else
		cols_used[SIM_DX] = true;
	}
	for ( col=1; col<cols; ++col )
	    GMatrixEditShowColumn(pstk,col,cols_used[col]);
    } else {
	for ( col=1; col<cols; ++col )
	    GMatrixEditShowColumn(pstk,col,true);
    }
    GWidgetToDesiredSize(ci->gw);

    GGadgetRedraw(pstk);
}

static void CI_DoHideUnusedPair(CharInfo *ci) {
    GGadget *pstk = GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100);
    int rows, cols = GMatrixEditGetColCnt(pstk);
    struct matrix_data *old = GMatrixEditGet(pstk,&rows);
    uint8 cols_used[20];
    int r, col, tot;

    if ( lookup_hideunused ) {
	memset(cols_used,0,sizeof(cols_used));
	for ( r=0; r<rows; ++r ) {
	    for ( col=2; col<cols; col+=2 ) {
		if ( old[cols*r+col].u.md_ival!=0 )
		    cols_used[col] = true;
		if ( old[cols*r+col+1].u.md_str!=NULL && *old[cols*r+col+1].u.md_str!='\0' )
		    cols_used[col+1] = true;
	    }
	}
	for ( col=2, tot=0; col<cols; ++col )
	    tot += cols_used[col];
	/* If no columns used (no info yet, all info is to preempt a kernclass and sets to 0) */
	/*  then show what we expect to be the default column for this kerning mode*/
	if ( tot==0 ) {
	    if ( strstr(ci->sc->name,".vert")!=NULL || strstr(ci->sc->name,".vrt2")!=NULL )
		cols_used[PAIR_DY_ADV1] = true;
	    else if ( SCRightToLeft(ci->sc))
		cols_used[PAIR_DX_ADV2] = true;
	    else
		cols_used[PAIR_DX_ADV1] = true;
	}
	for ( col=2; col<cols; ++col )
	    GMatrixEditShowColumn(pstk,col,cols_used[col]);
    } else {
	for ( col=2; col<cols; ++col )
	    GMatrixEditShowColumn(pstk,col,true);
    }
    GWidgetToDesiredSize(ci->gw);

    GGadgetRedraw(pstk);
}

static int CI_HideUnusedPair(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	lookup_hideunused = GGadgetIsChecked(g);
	CI_DoHideUnusedPair(ci);
	GGadgetRedraw(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100));
    }
return( true );
}

static int CI_HideUnusedSingle(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	lookup_hideunused = GGadgetIsChecked(g);
	CI_DoHideUnusedSingle(ci);
	GGadgetRedraw(GWidgetGetControl(ci->gw,CID_List+(pst_position-1)*100));
    }
return( true );
}

static void CI_FreeKernedImage(const void *_ci, GImage *img) {
    GImageDestroy(img);
}

static const int kern_popup_size = 100;

static BDFChar *Rasterize(SplineChar *sc,int def_layer) {
    void *freetypecontext=NULL;
    BDFChar *ret;

    freetypecontext = FreeTypeFontContext(sc->parent,sc,sc->parent->fv,def_layer);
    if ( freetypecontext!=NULL ) {
	ret = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,kern_popup_size,72,8);
	FreeTypeFreeContext(freetypecontext);
    } else
	ret = SplineCharAntiAlias(sc,def_layer,kern_popup_size,4);
return( ret );
}

static GImage *CI_GetKernedImage(const void *_ci) {
    CharInfo *ci = (CharInfo *) _ci;
    GGadget *pstk = GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100);
    int rows, cols = GMatrixEditGetColCnt(pstk);
    struct matrix_data *old = GMatrixEditGet(pstk,&rows);
    SplineChar *othersc = SFGetChar(ci->sc->parent,-1, old[cols*ci->r+1].u.md_str);
    BDFChar *me, *other;
    double scale = kern_popup_size/(double) (ci->sc->parent->ascent+ci->sc->parent->descent);
    int kern;
    int width, height, miny, maxy, minx, maxx;
    GImage *img;
    struct _GImage *base;
    Color fg, bg;
    int l,clut_scale;
    int x,y, xoffset, yoffset, coff1, coff2;
    struct lookup_subtable *sub = (struct lookup_subtable *) (old[cols*ci->r+0].u.md_ival);

    if ( othersc==NULL )
return( NULL );
    me = Rasterize(ci->sc,ci->def_layer);
    other = Rasterize(othersc,ci->def_layer);
    if ( sub->vertical_kerning ) {
	int vwidth = rint(ci->sc->vwidth*scale);
	kern = rint( old[cols*ci->r+PAIR_DY_ADV1].u.md_ival*scale );
	miny = me->ymin + rint(old[cols*ci->r+PAIR_DY1].u.md_ival*scale);
	maxy = me->ymax + rint(old[cols*ci->r+PAIR_DY1].u.md_ival*scale);
	if ( miny > vwidth + kern + rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale) + other->ymin )
	    miny = vwidth + kern + rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale) + other->ymin;
	if ( maxy < vwidth + kern + rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale) + other->ymax )
	    maxy = vwidth + kern + rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale) + other->ymax;
	height = maxy - miny + 2;
	minx = me->xmin + rint(old[cols*ci->r+PAIR_DX1].u.md_ival*scale); maxx = me->xmax + rint(old[cols*ci->r+PAIR_DX1].u.md_ival*scale);
	if ( minx>other->xmin + rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale) ) minx = other->xmin+ rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale) ;
	if ( maxx<other->xmax + rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale) ) maxx = other->xmax+ rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale) ;

	img = GImageCreate(it_index,maxx-minx+2,height);
	base = img->u.image;
	memset(base->data,'\0',base->bytes_per_line*base->height);

	yoffset = 1 + maxy - vwidth - kern - rint(old[cols*ci->r+PAIR_DY1].u.md_ival*scale);
	xoffset = 1 - minx + rint(old[cols*ci->r+PAIR_DX1].u.md_ival*scale);
	for ( y=me->ymin; y<=me->ymax; ++y ) {
	    for ( x=me->xmin; x<=me->xmax; ++x ) {
		base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] =
			me->bitmap[(me->ymax-y)*me->bytes_per_line + (x-me->xmin)];
	    }
	}
	yoffset = 1 + maxy - rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale);
	xoffset = 1 - minx + rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale);
	for ( y=other->ymin; y<=other->ymax; ++y ) {
	    for ( x=other->xmin; x<=other->xmax; ++x ) {
		int n = other->bitmap[(other->ymax-y)*other->bytes_per_line + (x-other->xmin)];
		if ( n>base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] )
		    base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] = n;
	    }
	}
    } else {
	coff1 = coff2 = 0;
	if ( sub->lookup->lookup_flags & pst_r2l ) {
	    BDFChar *temp = me;
	    me = other;
	    other = temp;
	    coff1 = 8; coff2 = -8;
	}
	kern = rint( old[cols*ci->r+PAIR_DX_ADV1+coff1].u.md_ival*scale );
	minx = me->xmin + rint(old[cols*ci->r+PAIR_DX1+coff1].u.md_ival*scale);
	maxx = me->xmax + rint(old[cols*ci->r+PAIR_DX1+coff1].u.md_ival*scale);
	if ( minx > me->width + kern + rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale) + other->xmin )
	    minx = me->width + kern + rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale) + other->xmin;
	if ( maxx < me->width + kern + rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale) + other->xmax )
	    maxx = me->width + kern + rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale) + other->xmax;
	width = maxx - minx + 2;
	miny = me->ymin + rint(old[cols*ci->r+PAIR_DY1+coff1].u.md_ival*scale); maxy = me->ymax + rint(old[cols*ci->r+PAIR_DY1+coff1].u.md_ival*scale);
	if ( miny>other->ymin + rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale) ) miny = other->ymin+ rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale) ;
	if ( maxy<other->ymax + rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale) ) maxy = other->ymax+ rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale) ;

	img = GImageCreate(it_index,width,maxy-miny+2);
	base = img->u.image;
	memset(base->data,'\0',base->bytes_per_line*base->height);

	xoffset = rint(old[cols*ci->r+PAIR_DX1+coff1].u.md_ival*scale) + 1 - minx;
	yoffset = 1 + maxy - rint(old[cols*ci->r+PAIR_DY1+coff1].u.md_ival*scale);
	for ( y=me->ymin; y<=me->ymax; ++y ) {
	    for ( x=me->xmin; x<=me->xmax; ++x ) {
		base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] =
			me->bitmap[(me->ymax-y)*me->bytes_per_line + (x-me->xmin)];
	    }
	}
	xoffset = 1 - minx + me->width + kern - rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale);
	yoffset = 1 + maxy - rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale);
	for ( y=other->ymin; y<=other->ymax; ++y ) {
	    for ( x=other->xmin; x<=other->xmax; ++x ) {
		int n = other->bitmap[(other->ymax-y)*other->bytes_per_line + (x-other->xmin)];
		if ( n>base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] )
		    base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] = n;
	    }
	}
    }
    memset(base->clut,'\0',sizeof(*base->clut));
    bg = GDrawGetDefaultBackground(NULL);
    fg = GDrawGetDefaultForeground(NULL);
    clut_scale = me->depth == 8 ? 8 : 4;
    base->clut->clut_len = 1<<clut_scale;
    for ( l=0; l<(1<<clut_scale); ++l )
	base->clut->clut[l] =
	    COLOR_CREATE(
	     COLOR_RED(bg) + (l*(COLOR_RED(fg)-COLOR_RED(bg)))/((1<<clut_scale)-1),
	     COLOR_GREEN(bg) + (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg)))/((1<<clut_scale)-1),
	     COLOR_BLUE(bg) + (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg)))/((1<<clut_scale)-1) );
    BDFCharFree(me);
    BDFCharFree(other);
return( img );
}

/* Draws an image of a glyph with a vertical bar down the middle. */
/*  used to show italic correction position (we also dot in the width line) */
/*  and top accent horizontal position for the MATH table */
GImage *SC_GetLinedImage(SplineChar *sc, int def_layer, int pos, int is_italic_cor) {
    BDFChar *me;
    double scale = kern_popup_size/(double) (sc->parent->ascent+sc->parent->descent);
    int miny, maxy, minx, maxx;
    GImage *img;
    struct _GImage *base;
    Color fg, bg;
    int l,clut_scale;
    int x,y, xoffset, yoffset;
    int pixel;

    if ( is_italic_cor )
	pos += sc->width;
    pos = rint( pos*scale );
    if ( pos<-100 || pos>100 )
return( NULL );
    me = Rasterize(sc,def_layer);
    if ( pos<me->xmin-10 || pos>me->xmax+30 ) {
	BDFCharFree(me);
return( NULL );
    }
    if ( (minx=me->xmin)>0 ) minx = 0;
    if ( (maxx=me->xmax)<me->width ) maxx = me->width;
    if ( pos<minx ) minx = pos-2;
    if ( pos>maxx ) maxx = pos+2;
    miny = me->ymin - 4;
    maxy = me->ymax + 4;

    pixel = me->depth == 8 ? 0xff : 0xf;

    img = GImageCreate(it_index,maxx-minx+2,maxy-miny+2);
    base = img->u.image;
    memset(base->data,'\0',base->bytes_per_line*base->height);

    xoffset = 1 - minx;
    yoffset = 1 + maxy;
    for ( y=me->ymin; y<=me->ymax; ++y ) {
	for ( x=me->xmin; x<=me->xmax; ++x ) {
	    base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] =
		    me->bitmap[(me->ymax-y)*me->bytes_per_line + (x-me->xmin)];
	}
    }
    for ( y=miny; y<=maxy; ++y ) {
	base->data[(yoffset-y)*base->bytes_per_line + (pos+xoffset)] = pixel;
	if ( is_italic_cor && (y&1 ))
	    base->data[(yoffset-y)*base->bytes_per_line + (me->width+xoffset)] = pixel;
    }
    
    memset(base->clut,'\0',sizeof(*base->clut));
    bg = GDrawGetDefaultBackground(NULL);
    fg = GDrawGetDefaultForeground(NULL);
    clut_scale = me->depth == 8 ? 8 : 4;
    base->clut->clut_len = 1<<clut_scale;
    for ( l=0; l<(1<<clut_scale); ++l )
	base->clut->clut[l] =
	    COLOR_CREATE(
	     COLOR_RED(bg) + (l*(COLOR_RED(fg)-COLOR_RED(bg)))/((1<<clut_scale)-1),
	     COLOR_GREEN(bg) + (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg)))/((1<<clut_scale)-1),
	     COLOR_BLUE(bg) + (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg)))/((1<<clut_scale)-1) );
    BDFCharFree(me);
return( img );
}

#define ICON_WIDTH 15

GImage *GV_GetConstructedImage(SplineChar *sc,int def_layer,struct glyphvariants *gv, int is_horiz) {
    SplineFont *sf = sc->parent;
    BDFChar *me, **others;
    double scale = kern_popup_size/(double) (sf->ascent+sf->descent);
    GImage *img;
    struct _GImage *base;
    Color fg, bg;
    int l,clut_scale;
    int x,y;
    int i,j;

    if ( gv==NULL || gv->part_cnt==0 )
return( NULL );
    me = Rasterize(sc,def_layer);
    others = malloc(gv->part_cnt*sizeof(BDFChar *));
    for ( i=0; i<gv->part_cnt; ++i ) {
	SplineChar *othersc = SFGetChar(sf,-1,gv->parts[i].component);
	if ( othersc==NULL ) {
	    for ( j=0; j<i; ++j )
		BDFCharFree(others[j]);
	    free(others);
return( NULL );
	}
	others[i] = Rasterize(othersc,def_layer);
    }
    if ( is_horiz ) {
	int ymin, ymax;
	int width, xoff;

	for ( i=1; i<gv->part_cnt; ++i ) {	/* Normalize all but first. Makes construction easier */
	    others[i]->xmax -= others[i]->xmin;
	    others[i]->xmin = 0;
	}
	xoff = me->xmin<0 ? -me->xmin : 0;
	width = xoff + me->width + ICON_WIDTH;
	ymin = me->ymin; ymax = me->ymax;
	for ( i=0; i<gv->part_cnt; ++i ) {
	    int overlap;
	    if ( i==gv->part_cnt-1 )
		overlap=0;
	    else {
		overlap = gv->parts[i].endConnectorLength>gv->parts[i+1].startConnectorLength ?
			gv->parts[i+1].startConnectorLength :
			gv->parts[i].endConnectorLength;
		overlap = rint( scale*overlap );
	    }
	    width += rint(gv->parts[i].fullAdvance*scale) - overlap + others[i]->xmin/* Only does anything if i==0, then it normalizes the rest to the same baseline */;
	    if ( others[i]->ymin<ymin ) ymin = others[i]->ymin;
	    if ( others[i]->ymax>ymax ) ymax = others[i]->ymax;
	}
	if ( ymax<=ICON_WIDTH ) ymax = ICON_WIDTH;
	if ( ymin>0 ) ymin = 0;
	img = GImageCreate(it_index,width+10,ymax-ymin+2);
	base = img->u.image;
	memset(base->data,'\0',base->bytes_per_line*base->height);

	++xoff;		/* One pixel margin */

	for ( y=me->ymin; y<=me->ymax; ++y ) {
	    for ( x=me->xmin; x<=me->xmax; ++x ) {
		base->data[(1+ymax-y)*base->bytes_per_line + (x+xoff)] =
			me->bitmap[(me->ymax-y)*me->bytes_per_line + (x-me->xmin)];
	    }
	}
	xoff += me->width;
	{
	    int pixel = me->depth == 8 ? 0xff : 0xf;
	    for ( j = -1; j<2; j+=2 ) {
		if ( me->ymax<-me->ymin )
		    y = (me->ymax+me->ymin)/2;
		else
		    y = 1+me->ymax/2;
		y = ymax-y + j*2;
		for ( x=1; x<ICON_WIDTH-5; ++x )
		    base->data[y*base->bytes_per_line + (x+xoff)] = pixel;
		for ( x=ICON_WIDTH-8; x<ICON_WIDTH-1; ++x )
		    base->data[(y+j*(ICON_WIDTH-4-x))*base->bytes_per_line + (x+xoff)] = pixel;
	    }
	    xoff += ICON_WIDTH;
	}
	for ( i=0; i<gv->part_cnt; ++i ) {
	    int overlap;
	    if ( i==gv->part_cnt-1 )
		overlap=0;
	    else {
		overlap = gv->parts[i].endConnectorLength>gv->parts[i+1].startConnectorLength ?
			gv->parts[i+1].startConnectorLength :
			gv->parts[i].endConnectorLength;
		overlap = rint( scale*overlap );
	    }
	    for ( y=others[i]->ymin; y<=others[i]->ymax; ++y ) {
		for ( x=others[i]->xmin; x<=others[i]->xmax; ++x ) {
		    int n = others[i]->bitmap[(others[i]->ymax-y)*others[i]->bytes_per_line + (x-others[i]->xmin)];
		    if ( n>base->data[(ymax-y)*base->bytes_per_line + (x+xoff)] )
			base->data[(ymax-y)*base->bytes_per_line + (x+xoff)] = n;
		}
	    }
	    xoff += rint(gv->parts[i].fullAdvance*scale) - overlap + others[i]->xmin/* Only does anything if i==0, then it normalizes the rest to the same baseline */;
	}
    } else {
	int xmin, xmax, ymin, ymax;
	int yoff, xoff, width;

	for ( i=1; i<gv->part_cnt; ++i ) {	/* Normalize all but first. Makes construction easier */
	    others[i]->ymax -= others[i]->ymin;
	    others[i]->ymin = 0;
	}

	xoff = me->xmin<0 ? -me->xmin : 0;
	width = xoff + me->width + ICON_WIDTH;
	ymin = me->ymin; ymax = me->ymax;
	xmin = xmax = 0;
	yoff = 0;
	for ( i=0; i<gv->part_cnt; ++i ) {
	    int overlap;
	    if ( i==gv->part_cnt-1 )
		overlap=0;
	    else {
		overlap = gv->parts[i].endConnectorLength>gv->parts[i+1].startConnectorLength ?
			gv->parts[i+1].startConnectorLength :
			gv->parts[i].endConnectorLength;
		overlap = rint( scale*overlap );
	    }
	    if ( ymin>others[i]->ymin+yoff ) ymin = others[i]->ymin+yoff;
	    if ( ymax<others[i]->ymax+yoff ) ymax = others[i]->ymax+yoff;
	    yoff += rint(gv->parts[i].fullAdvance*scale) - overlap + others[i]->ymin/* Only does anything if i==0, then it normalizes the rest to the same baseline */;
	    if ( others[i]->xmin<xmin ) xmin = others[i]->xmin;
	    if ( others[i]->xmax>xmax ) xmax = others[i]->xmax;
	}
	if ( xmin<-width ) {
	    xoff = -xmin-width;
	    width = -xmin;
	}
	if ( xmax>0 )
	    width += xmax;
	if ( ymax<=ICON_WIDTH ) ymax = ICON_WIDTH;
	if ( ymin>0 ) ymin = 0;
	img = GImageCreate(it_index,width+2,ymax-ymin+2);
	base = img->u.image;
	memset(base->data,'\0',base->bytes_per_line*base->height);

	++xoff;		/* One pixel margin */

	for ( y=me->ymin; y<=me->ymax; ++y ) {
	    for ( x=me->xmin; x<=me->xmax; ++x ) {
		base->data[(1+ymax-y)*base->bytes_per_line + (x+xoff)] =
			me->bitmap[(me->ymax-y)*me->bytes_per_line + (x-me->xmin)];
	    }
	}
	xoff += me->width;
	{
	    int pixel = me->depth == 8 ? 0xff : 0xf;
	    for ( j = -1; j<2; j+=2 ) {
		if ( me->ymax<-me->ymin )
		    y = (me->ymax+me->ymin)/2;
		else
		    y = 1+me->ymax/2;
		y = ymax-y + j*2;
		for ( x=1; x<ICON_WIDTH-5; ++x )
		    base->data[y*base->bytes_per_line + (x+xoff)] = pixel;
		for ( x=ICON_WIDTH-8; x<ICON_WIDTH-1; ++x )
		    base->data[(y+j*(ICON_WIDTH-4-x))*base->bytes_per_line + (x+xoff)] = pixel;
	    }
	    xoff += ICON_WIDTH;
	}
	yoff=0;
	for ( i=0; i<gv->part_cnt; ++i ) {
	    int overlap;
	    if ( i==gv->part_cnt-1 )
		overlap=0;
	    else {
		overlap = gv->parts[i].endConnectorLength>gv->parts[i+1].startConnectorLength ?
			gv->parts[i+1].startConnectorLength :
			gv->parts[i].endConnectorLength;
		overlap = rint( scale*overlap );
	    }
	    for ( y=others[i]->ymin; y<=others[i]->ymax; ++y ) {
		for ( x=others[i]->xmin; x<=others[i]->xmax; ++x ) {
		    int n = others[i]->bitmap[(others[i]->ymax-y)*others[i]->bytes_per_line + (x-others[i]->xmin)];
		    if ( n>base->data[(ymax-y-yoff)*base->bytes_per_line + (x+xoff)] )
			base->data[(ymax-y-yoff)*base->bytes_per_line + (x+xoff)] = n;
		}
	    }
	    yoff += rint(gv->parts[i].fullAdvance*scale) - overlap + others[i]->ymin/* Only does anything if i==0, then it normalizes the rest to the same baseline */;
	}
    }
    for ( i=0; i<gv->part_cnt; ++i )
	BDFCharFree(others[i]);
    free(others);

    memset(base->clut,'\0',sizeof(*base->clut));
    bg = GDrawGetDefaultBackground(NULL);
    fg = GDrawGetDefaultForeground(NULL);
    clut_scale = me->depth == 8 ? 8 : 4;
    BDFCharFree(me);
    base->clut->clut_len = 1<<clut_scale;
    for ( l=0; l<(1<<clut_scale); ++l )
	base->clut->clut[l] =
	    COLOR_CREATE(
	     COLOR_RED(bg) + (l*(COLOR_RED(fg)-COLOR_RED(bg)))/((1<<clut_scale)-1),
	     COLOR_GREEN(bg) + (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg)))/((1<<clut_scale)-1),
	     COLOR_BLUE(bg) + (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg)))/((1<<clut_scale)-1) );
return( img );
}

static GImage *CI_GetConstructedImage(const void *_ci) {
    CharInfo *ci = (CharInfo *) _ci;
    GImage *ret;
    int is_horiz = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-ci->vert_aspect;
    struct glyphvariants *gv;

    gv = CI_ParseVariants(NULL,ci,is_horiz,NULL,0,true);

    ret = GV_GetConstructedImage(ci->sc,ci->def_layer,gv,is_horiz);
    GlyphVariantsFree(gv);
return( ret );
}

GImage *NameList_GetImage(SplineFont *sf,SplineChar *sc,int def_layer,
	char *namelist, int isliga ) {
    BDFChar *me, **extras;
    int width, xmin, xmax, ymin, ymax;
    GImage *img;
    struct _GImage *base;
    Color fg, bg;
    int l,clut_scale;
    int x,y;
    SplineChar *other;
    int extracnt;
    int i,j;
    char *subs = namelist, *pt, *start;
    int ch;

    if ( sc==NULL || sf==NULL || namelist==NULL )
return( NULL );
    me = Rasterize(sc,def_layer);
    ymin = me->ymin; ymax = me->ymax;
    xmin = me->xmin; xmax = me->xmax; width = me->width;
    extracnt = 0; extras = NULL;

    for ( pt=subs; *pt ; ++extracnt ) {
	while ( *pt!=' ' && *pt!='\0' ) ++pt;
	if ( *pt==' ' )
	    while ( *pt==' ' ) ++pt;
    }
    extras = malloc(extracnt*sizeof(BDFChar *));
    extracnt = 0;
    for ( pt=subs; *pt ; ) {
	start = pt;
	while ( *pt!=' ' && *pt!='\0' && *pt!='(' ) ++pt;
	ch = *pt; *pt = '\0';
	other = SFGetChar(sf,-1, start);
	*pt = ch;
	if ( ch=='(' ) {
	    while ( *pt!=')' && *pt!='\0' ) ++pt;
	    if ( *pt==')' ) ++pt;
	}
	if ( other!=NULL ) {
	    if ( extracnt==0 ) width += ICON_WIDTH;
	    extras[extracnt] = Rasterize(other,def_layer);
	    if ( width+extras[extracnt]->xmin < xmin ) xmin = width+extras[extracnt]->xmin;
	    if ( width+extras[extracnt]->xmax > xmax ) xmax = width+extras[extracnt]->xmax;
	    if ( extras[extracnt]->ymin < ymin ) ymin = extras[extracnt]->ymin;
	    if ( extras[extracnt]->ymax > ymax ) ymax = extras[extracnt]->ymax;
	    width += extras[extracnt++]->width;
	}
	if ( *pt==' ' )
	    while ( *pt==' ' ) ++pt;
    }

    if ( ymax<=ICON_WIDTH ) ymax = ICON_WIDTH;
    if ( ymin>0 ) ymin = 0;
    if ( xmax<xmin ) {
        for ( i=0; i<extracnt; ++i )
            BDFCharFree(extras[i]);
        free(extras);
        return( NULL );
    }

    if ( xmin>0 ) xmin = 0;

    img = GImageCreate(it_index,xmax - xmin + 2,ymax-ymin+2);
    base = img->u.image;
    memset(base->data,'\0',base->bytes_per_line*base->height);

    width = -xmin;
    ++width;

    for ( y=me->ymin; y<=me->ymax; ++y ) {
	for ( x=me->xmin; x<=me->xmax; ++x ) {
	    base->data[(1+ymax-y)*base->bytes_per_line + (x+width)] =
		    me->bitmap[(me->ymax-y)*me->bytes_per_line + (x-me->xmin)];
	}
    }
    width += me->width;
    if ( extracnt!=0 ) {
	int pixel = me->depth == 8 ? 0xff : 0xf;
	if ( !isliga ) {
	    for ( j = -1; j<2; j+=2 ) {
		if ( me->ymax<-me->ymin )
		    y = (me->ymax+me->ymin)/2;
		else
		    y = 1+me->ymax/2;
		y = ymax-y + j*2;
		for ( x=1; x<ICON_WIDTH-5; ++x )
		    base->data[y*base->bytes_per_line + (x+width)] = pixel;
		for ( x=ICON_WIDTH-8; x<ICON_WIDTH-1; ++x )
		    base->data[(y+j*(ICON_WIDTH-4-x))*base->bytes_per_line + (x+width)] = pixel;
	    }
	} else if ( isliga>0 ) {
	    for ( j = -1; j<2; j+=2 ) {
		y = 1+ymax/2 + j*2;
		for ( x=5; x<ICON_WIDTH-1; ++x )
		    base->data[y*base->bytes_per_line + (x+width)] = pixel;
		for ( x=8; x>1 ; --x )
		    base->data[(y+j*(x-3))*base->bytes_per_line + (x+width)] = pixel;
	    }
	}
	width += ICON_WIDTH;
	for ( i=0; i<extracnt; ++i ) {
	    BDFChar *other = extras[i];
	    for ( y=other->ymin; y<=other->ymax; ++y ) {
		for ( x=other->xmin; x<=other->xmax; ++x ) {
		    if ( other->bitmap[(other->ymax-y)*other->bytes_per_line + (x-other->xmin)] != 0 )
			base->data[(1+ymax-y)*base->bytes_per_line + (x+width)] =
				other->bitmap[(other->ymax-y)*other->bytes_per_line + (x-other->xmin)];
		}
	    }
	    width += other->width;
	}
    }
    memset(base->clut,'\0',sizeof(*base->clut));
    bg = GDrawGetDefaultBackground(NULL);
    fg = GDrawGetDefaultForeground(NULL);
    clut_scale = me->depth == 8 ? 8 : 4;
    base->clut->clut_len = 1<<clut_scale;
    for ( l=0; l<(1<<clut_scale); ++l )
	base->clut->clut[l] =
	    COLOR_CREATE(
	     COLOR_RED(bg) + (l*(COLOR_RED(fg)-COLOR_RED(bg)))/((1<<clut_scale)-1),
	     COLOR_GREEN(bg) + (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg)))/((1<<clut_scale)-1),
	     COLOR_BLUE(bg) + (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg)))/((1<<clut_scale)-1) );
     BDFCharFree(me);
     for ( i=0; i<extracnt; ++i )
	 BDFCharFree(extras[i]);
     free(extras);
return( img );
}

GImage *PST_GetImage(GGadget *pstk,SplineFont *sf,int def_layer,
	struct lookup_subtable *sub,int popup_r, SplineChar *sc ) {
    int rows, cols = GMatrixEditGetColCnt(pstk);
    struct matrix_data *old = GMatrixEditGet(pstk,&rows);

    if ( sc==NULL || sub==NULL )
return( NULL );
    if ( sub->lookup->lookup_type<gsub_single || sub->lookup->lookup_type>gsub_ligature )
return( NULL );

return( NameList_GetImage(sf,sc,def_layer,old[cols*popup_r+1].u.md_str,
	sub->lookup->lookup_type==gsub_ligature));
}

static GImage *_CI_GetImage(const void *_ci) {
    CharInfo *ci = (CharInfo *) _ci;
    int offset = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    GGadget *pstk = GWidgetGetControl(ci->gw,CID_List+offset*100);
    int rows, cols = GMatrixEditGetColCnt(pstk);
    struct matrix_data *old = GMatrixEditGet(pstk,&rows);
    struct lookup_subtable *sub = (struct lookup_subtable *) (old[cols*ci->r+0].u.md_ival);

    if ( ci->r>=rows )
return( NULL );

return( PST_GetImage(pstk,ci->sc->parent,ci->def_layer,sub,ci->r,ci->sc) );
}

static void CI_KerningPopupPrepare(GGadget *g, int r, int c) {
    CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);

    if ( c<0 || c>=cols || r<0 || r>=rows || old[cols*r+1].u.md_str==NULL ||
	SFGetChar(ci->sc->parent,-1, old[cols*r+1].u.md_str)==NULL )
return;
    ci->r = r; ci->c = c;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,ci,CI_GetKernedImage,CI_FreeKernedImage);
}

static void CI_SubsPopupPrepare(GGadget *g, int r, int c) {
    CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);

    (void) GMatrixEditGet(g,&rows);
    if ( c<0 || c>=cols || r<0 || r>=rows )
return;
    ci->r = r; ci->c = c;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,ci,_CI_GetImage,CI_FreeKernedImage);
}

static void CI_ConstructionPopupPrepare(GGadget *g, int r, int c) {
    CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
    int rows/*, cols = GMatrixEditGetColCnt(g)*/;

    (void) GMatrixEditGet(g,&rows);
    if ( rows==0 )
return;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,ci,CI_GetConstructedImage,CI_FreeKernedImage);
}

static unichar_t **CI_GlyphNameCompletion(GGadget *t,int from_tab) {
    CharInfo *ci = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = ci->sc->parent;

return( SFGlyphNameCompletion(sf,t,from_tab,false));
}

static unichar_t **CI_GlyphListCompletion(GGadget *t,int from_tab) {
    CharInfo *ci = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = ci->sc->parent;

return( SFGlyphNameCompletion(sf,t,from_tab,true));
}


static void extpart_finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *possub;
    CharInfo *ci;
    int is_horiz,cols;
    DBounds b;
    double full_advance;
    SplineChar *sc;

    if ( c!=0 )
return;
    if ( !wasnew )
return;
    /* If they added a new glyph to the sequence then set some defaults for it. */
    /*  only the full advance has any likelyhood of being correct */
    ci = GDrawGetUserData(GGadgetGetWindow(g));
    is_horiz = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-ci->vert_aspect;
    possub = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( possub[r*cols+0].u.md_str==NULL )
return;
    sc = SFGetChar(ci->sc->parent,-1,possub[r*cols+0].u.md_str);
    if ( sc==NULL )
return;
    SplineCharFindBounds(sc,&b);
    if ( is_horiz )
	full_advance = b.maxx - b.minx;
    else
	full_advance = b.maxy - b.miny;
    possub[r*cols+2].u.md_ival = possub[r*cols+3].u.md_ival = rint(full_advance/3);
    possub[r*cols+4].u.md_ival = rint(full_advance);
    GGadgetRedraw(g);
}

static GTextInfo truefalse[] = {
    { (unichar_t *) N_("false"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("true"),  NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};

static struct col_init extensionpart[] = {
    { me_string , NULL, NULL, NULL, N_("Glyph") },
    { me_enum, NULL, truefalse, NULL, N_("Extender") },
/* GT: "Len" is an abreviation for "Length" */
    { me_int, NULL, NULL, NULL, N_("StartLen") },
    { me_int, NULL, NULL, NULL, N_("EndLen") },
    { me_int, NULL, NULL, NULL, N_("FullLen") },
    COL_INIT_EMPTY
};
static struct matrixinit mi_extensionpart =
    { sizeof(extensionpart)/sizeof(struct col_init)-1, extensionpart, 0, NULL, NULL, NULL, extpart_finishedit, NULL, NULL, NULL };

static int isxheight(int uni) {
    if ( uni>=0x10000 || !islower(uni))
return( false );

    if ( uni=='a' || uni=='c' || uni=='e' || uni=='i' || uni=='j' ||
	    (uni>='m' && uni<='z') ||
	    uni==0x131 || uni==0x237 || /* Ignore accented letters except the dotlessi/j */
	    (uni>=0x250 && uni<0x253) || uni==0x254 || uni==0x255 ||
	    (uni>=0x258 && uni<0x265) || uni==0x269 || uni==0x26A ||
	    (uni>=0x26f && uni<=0x277) || uni==0x279 ||
	    uni==0x3b1 || uni==0x3b3 || uni==0x3b5 || uni==0x3b7 ||
	    uni==0x3b9 || uni==0x3ba || uni==0x3bc || uni==0x3bd ||
	    (uni>=0x3bf && uni<=0x3c7) || uni==0x3c9 ||
	    (uni>=0x400 && uni<=0x45f))
return( true );

return( false );
}

static int isbaseline(int uni) {
    /* Treat rounded letters as being on the base line */
    /* But don't bother including guys that normally are on the baseline */

    if ( (uni>='a' && uni<'g') || uni=='h' || uni=='i' ||
	    (uni>='k' && uni<='o') || (uni>='r' && uni<'y') || uni=='z' ||
	    (uni>='A' && uni<='Z' ) ||
	    uni==0x3b0 || uni==0x3b4 || uni==0x3b5 || (uni>=0x3b8 && uni<0x3bb) ||
	    uni==0x3bd || uni==0x3bf || uni==0x3c0 || (uni>=0x3c2 && uni<0x3c6) ||
	    uni==0x3c8 || uni==0x3c7 || uni==0x3c9 ||
	    (uni>=0x391 && uni<0x3aa) ||
	    (uni>=0x400 && uni<0x40f) || (uni>=0x410 && uni<=0x413) ||
	    (uni>=0x415 && uni<0x425) || uni==0x427 || uni==0x428 ||
	    (uni>=0x42a && uni<0x433) || (uni>=0x435 && uni<0x45e) )
return( true );

return( false );
}

static int TeX_Default(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = GGadgetGetCid(g);
	DBounds b;
	int value;
	SplineChar *basesc = NULL;
	SplineFont *sf = ci->sc->parent;
	int style;
	char buf[12];

	basesc = ci->sc;
	/* Try to align the top of lowercase (xheight) letters all at the */
	/*  same height. Ditto for uppercase & ascender letters */
	if ( cid==CID_TeX_HeightD && ci->sc->unicodeenc<0x10000 &&
		isxheight(ci->sc->unicodeenc) &&
		(basesc = SFGetChar(sf,'x',NULL))!=NULL )
	    /* Done */;
	else if ( cid==CID_TeX_HeightD && ci->sc->unicodeenc<0x10000 &&
		islower(ci->sc->unicodeenc) &&
		(basesc = SFGetChar(sf,'l',NULL))!=NULL )
	    /* Done */;
	else if ( cid==CID_TeX_HeightD && ci->sc->unicodeenc<0x10000 &&
		isupper(ci->sc->unicodeenc) &&
		(basesc = SFGetChar(sf,'I',NULL))!=NULL )
	    /* Done */;
	else if ( cid==CID_TeX_DepthD && ci->sc->unicodeenc<0x10000 &&
		isbaseline(ci->sc->unicodeenc) &&
		(basesc = SFGetChar(sf,'I',NULL))!=NULL )
	    /* Done */;
	else
	    basesc = ci->sc;

	SplineCharFindBounds(basesc,&b);
	style = MacStyleCode(sf,NULL);

	if ( cid == CID_TeX_HeightD ) {
	    if ( basesc!=ci->sc && basesc->tex_height!=TEX_UNDEF )
		value = basesc->tex_height;
	    else
		value = rint(b.maxy);
	    if ( value<0 ) value = 0;
	} else if ( cid == CID_TeX_DepthD ) {
	    if ( basesc!=ci->sc && basesc->tex_depth!=TEX_UNDEF )
		value = basesc->tex_depth;
	    else {
		value = -rint(b.miny);
		if ( value<5 ) value = 0;
	    }
	} else if ( cid == CID_HorAccentD ) {
	    double italic_off = (b.maxy-b.miny)*tan(-sf->italicangle);
	    if ( b.maxx-b.minx-italic_off < 0 )
		value = rint(b.minx + (b.maxx-b.minx)/2);
	    else
		value = rint(b.minx + italic_off + (b.maxx - b.minx - italic_off)/2);
	} else if ( (style&sf_italic) || sf->italicangle!=0 ) {
	    value = rint((b.maxx-ci->sc->width) +
			    (sf->ascent+sf->descent)/16.0);
	} else
	    value = 0;
	sprintf( buf, "%d", value );
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,cid-5),buf);
    }
return( true );
}

static int CI_SubSuperPositionings(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	MathKernDialog(ci->sc,ci->def_layer);
    }
return( true );
}

static struct col_init altuniinfo[] = {
    { me_uhex , NULL, NULL, NULL, N_("Unicode") },
    { me_uhex, NULL, truefalse, NULL, N_("Variation Selector (or 0)") },
    COL_INIT_EMPTY
};
static struct matrixinit mi_altuniinfo =
    { sizeof(altuniinfo)/sizeof(struct col_init)-1, altuniinfo, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static void CI_NoteAspect(CharInfo *ci) {
    int new_aspect = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs));
    PST *pst;
    int cnt;
    char buf[20];

    last_gi_aspect = new_aspect;
    if ( new_aspect == ci->lc_aspect && (!ci->lc_seen || GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_DefLCCount)))) {
	ci->lc_seen = true;
	for ( pst=ci->sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
	if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_DefLCCount)) &&
		pst==NULL ) {
	    int rows, cols, i;
	    struct matrix_data *possub;
	    /* Normally we look for ligatures in the possub list, but here*/
	    /*  we will examine the ligature pane itself to get the most */
	    /*  up to date info */
	    possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pst_ligature-1)*100), &rows );
	    cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pst_ligature-1)*100) );
	    cnt = 0;
	    for ( i=0; i<rows; ++i ) {
		char *pt = possub[cols*i+1].u.md_str;
		int comp = 0;
		while ( *pt!='\0' ) {
		    while ( *pt==' ' ) ++pt;
		    if ( *pt=='\0' )
		break;
		    while ( *pt!=' ' && *pt!='\0' ) ++pt;
		    ++comp;
		}
		if ( comp>cnt ) cnt = comp;
	    }
	    --cnt;		/* We want one fewer caret than there are components -- carets go BETWEEN components */
	} else if ( pst!=NULL )
	    cnt = pst->u.lcaret.cnt;
	else
	    cnt = 0;
	if ( cnt<0 ) cnt = 0;
	sprintf( buf, "%d", cnt );
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_LCCount),buf);
    }
}

static int CI_AspectChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CI_NoteAspect(ci);
    }
return( true );
}

static int CI_DefLCChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int show = !GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LCCount),show);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LCCountLab),show);
    }
return( true );
}

void GV_ToMD(GGadget *g, struct glyphvariants *gv) {
    int cols = GMatrixEditGetColCnt(g), j;
    struct matrix_data *mds;
    if ( gv==NULL ) {
	GMatrixEditSet(g, NULL,0,false);
return;
    }
    mds = calloc(gv->part_cnt*cols,sizeof(struct matrix_data));
    for ( j=0; j<gv->part_cnt; ++j ) {
	mds[j*cols+0].u.md_str = copy(gv->parts[j].component);
	mds[j*cols+1].u.md_ival = gv->parts[j].is_extender;
	mds[j*cols+2].u.md_ival = gv->parts[j].startConnectorLength;
	mds[j*cols+3].u.md_ival = gv->parts[j].endConnectorLength;
	mds[j*cols+4].u.md_ival = gv->parts[j].fullAdvance;
    }
    GMatrixEditSet(g, mds,gv->part_cnt,false);
}

static void GA_ToMD(GGadget *g, SplineChar *sc) {
    struct altuni *alt;
    int cols = GMatrixEditGetColCnt(g), cnt;
    struct matrix_data *mds;
    if ( sc->altuni==NULL ) {
	GMatrixEditSet(g, NULL,0,false);
return;
    }
    for ( cnt=0, alt=sc->altuni; alt!=NULL; ++cnt, alt=alt->next );
    mds = calloc(cnt*cols,sizeof(struct matrix_data));
    for ( cnt=0, alt=sc->altuni; alt!=NULL; ++cnt, alt=alt->next ) {
	mds[cnt*cols+0].u.md_ival = alt->unienc;
	mds[cnt*cols+1].u.md_ival = alt->vs==-1? 0 : alt->vs;
    }
    GMatrixEditSet(g, mds,cnt,false);
}

static void CI_SetColorList(CharInfo *ci,Color color) {
    int i;
    uint16 junk;

    std_colors[CUSTOM_COLOR].image = NULL;
    for ( i=0; std_colors[i].image!=NULL; ++i ) {
	if ( std_colors[i].userdata == (void *) (intpt) color )
    break;
    }
    if ( std_colors[i].image==NULL ) {
	std_colors[i].image = &customcolor_image;
	customcolor_image.u.image->clut->clut[1] = color;
	std_colors[i].userdata = (void *) (intpt) color;
    }
    GGadgetSetList(GWidgetGetControl(ci->gw,CID_Color), GTextInfoArrayFromList(std_colors,&junk), false);
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),i);
    if ( color!=COLOR_DEFAULT )
	ci->last = color;
    ci->real_last = color;
}

struct colcount { Color color; int cnt; };
static int colcountorder(const void *op1, const void *op2) {
    const struct colcount *c1 = op1, *c2 = op2;

return( c2->cnt - c1->cnt );		/* Biggest first */
}

struct hslrgb *SFFontCols(SplineFont *sf,struct hslrgb fontcols[6]) {
    int i, gid, cnt;
    struct colcount *colcount, stds[7];
    SplineChar *sc;

    memset(stds,0,sizeof(stds));
    stds[0].color = 0xffffff;
    stds[1].color = 0xff0000;
    stds[2].color = 0x00ff00;
    stds[3].color = 0x0000ff;
    stds[4].color = 0xffff00;
    stds[5].color = 0x00ffff;
    stds[6].color = 0xff00ff;
    colcount = calloc(sf->glyphcnt,sizeof(struct colcount));

    cnt = 0;
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc= sf->glyphs[gid])!=NULL ) {
	if ( sc->color==COLOR_DEFAULT )
    continue;
	for ( i=0; i<7 && sc->color!=stds[i].color; ++i );
	if ( i<7 ) {
	    ++stds[i].cnt;
    continue;
	}
	for ( i=0; i<cnt && sc->color!=colcount[i].color; ++i );
	if ( i==cnt )
	    colcount[cnt++].color = sc->color;
	++colcount[i].cnt;
    }

    if ( cnt<6 ) {
	for ( i=0; i<cnt; ++i )
	    ++colcount[i].cnt;
	for ( i=0; i<7; ++i ) if ( stds[i].cnt!=0 ) {
	    colcount[cnt].color = stds[i].color;
	    colcount[cnt++].cnt = 1;
	}
    }
    qsort(colcount,cnt,sizeof(struct colcount),colcountorder);

    memset(fontcols,0,6*sizeof(struct hslrgb));
    for ( i=0; i<6 && i<cnt; ++i ) {
	fontcols[i].rgb = true;
	fontcols[i].r = ((colcount[i].color>>16)&0xff)/255.0;
	fontcols[i].g = ((colcount[i].color>>8 )&0xff)/255.0;
	fontcols[i].b = ((colcount[i].color    )&0xff)/255.0;
    }
    free(colcount);
    if ( cnt==0 )
return( NULL );

return(fontcols);
}
    
static int CI_PickColor(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	GTextInfo *ti = GGadgetGetListItemSelected(g);
	if ( ti==NULL )
	    /* Can't happen */;
	else if ( ti->userdata == (void *) COLOR_CHOOSE ) {
	    struct hslrgb col, font_cols[6];
	    memset(&col,0,sizeof(col));
	    col.rgb = true;
	    col.r = ((ci->last>>16)&0xff)/255.;
	    col.g = ((ci->last>>8 )&0xff)/255.;
	    col.b = ((ci->last    )&0xff)/255.;
	    col = GWidgetColor(_("Pick a color"),&col,SFFontCols(ci->sc->parent,font_cols));
	    if ( col.rgb ) {
		ci->last = (((int) rint(255.*col.r))<<16 ) |
			    (((int) rint(255.*col.g))<<8 ) |
			    (((int) rint(255.*col.b)) );
		CI_SetColorList(ci,ci->last);
	    } else /* Cancelled */
		CI_SetColorList(ci,ci->real_last);
	} else {
	    if ( (intpt) ti->userdata!=COLOR_DEFAULT )
		ci->last = (intpt) ti->userdata;
	    ci->real_last = (intpt) ti->userdata;
	}
    }
return( true );
}

static void CIFillup(CharInfo *ci) {
    SplineChar *sc = ci->cachedsc!=NULL ? ci->cachedsc : ci->sc;
    SplineFont *sf = sc->parent;
    unichar_t *temp;
    char buffer[400];
    char buf[200];
    const unichar_t *bits;
    int i,j,gid, isv;
    struct matrix_data *mds[pst_max];
    int cnts[pst_max];
    PST *pst;
    KernPair *kp;
    unichar_t ubuf[4];
    GTextInfo **ti;
    char *devtabstr;

    sprintf(buf,_("Glyph Info for %.40s"),sc->name);
    GDrawSetWindowTitles8(ci->gw, buf, _("Glyph Info..."));

    if ( ci->oldsc!=NULL && ci->oldsc->charinfo==ci )
	ci->oldsc->charinfo = NULL;
    ci->sc->charinfo = ci;
    ci->oldsc = ci->sc;

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,-1), ci->enc>0 &&
	    ((gid=ci->map->map[ci->enc-1])==-1 ||
	     sf->glyphs[gid]==NULL || sf->glyphs[gid]->charinfo==NULL ||
	     gid==sc->orig_pos));
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,1), ci->enc<ci->map->enccount-1 &&
	    ((gid=ci->map->map[ci->enc+1])==-1 ||
	     sf->glyphs[gid]==NULL || sf->glyphs[gid]->charinfo==NULL ||
	     gid==sc->orig_pos));

    temp = utf82u_copy(sc->name);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UName),temp);
    free(temp);
    CI_SetNameList(ci,sc->unicodeenc);

    sprintf(buffer,"U+%04x", sc->unicodeenc);
    temp = utf82u_copy(sc->unicodeenc==-1?"-1":buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
    free(temp);

    ubuf[0] = sc->unicodeenc;
    if ( sc->unicodeenc==-1 )
	ubuf[0] = '\0';
    ubuf[1] = '\0';
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);

    memset(cnts,0,sizeof(cnts));
    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) if ( pst->type!=pst_lcaret )
	++cnts[pst->type];
    for ( isv=0; isv<2; ++isv ) {
	for ( kp=isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next )
	    ++cnts[pst_pair];
    }
    for ( i=pst_null+1; i<pst_max && i<pst_lcaret ; ++i )
	mds[i] = calloc((cnts[i]+1)*mi[i-1].col_cnt,sizeof(struct matrix_data));
    memset(cnts,0,sizeof(cnts));
    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) if ( pst->type!=pst_lcaret ) {
	j = (cnts[pst->type]++ * mi[pst->type-1].col_cnt);
	mds[pst->type][j+0].u.md_ival = (intpt) pst->subtable;
	if ( pst->type==pst_position ) {
	    mds[pst->type][j+SIM_DX].u.md_ival = pst->u.pos.xoff;
	    mds[pst->type][j+SIM_DY].u.md_ival = pst->u.pos.yoff;
	    mds[pst->type][j+SIM_DX_ADV].u.md_ival = pst->u.pos.h_adv_off;
	    mds[pst->type][j+SIM_DY_ADV].u.md_ival = pst->u.pos.v_adv_off;
	    ValDevTabToStrings(mds[pst_position],j+SIM_DX+1,pst->u.pos.adjust);
	} else if ( pst->type==pst_pair ) {
	    mds[pst->type][j+1].u.md_str = copy(pst->u.pair.paired);
	    mds[pst->type][j+PAIR_DX1].u.md_ival = pst->u.pair.vr[0].xoff;
	    mds[pst->type][j+PAIR_DY1].u.md_ival = pst->u.pair.vr[0].yoff;
	    mds[pst->type][j+PAIR_DX_ADV1].u.md_ival = pst->u.pair.vr[0].h_adv_off;
	    mds[pst->type][j+PAIR_DY_ADV1].u.md_ival = pst->u.pair.vr[0].v_adv_off;
	    mds[pst->type][j+PAIR_DX2].u.md_ival = pst->u.pair.vr[1].xoff;
	    mds[pst->type][j+PAIR_DY2].u.md_ival = pst->u.pair.vr[1].yoff;
	    mds[pst->type][j+PAIR_DX_ADV2].u.md_ival = pst->u.pair.vr[1].h_adv_off;
	    mds[pst->type][j+PAIR_DY_ADV2].u.md_ival = pst->u.pair.vr[1].v_adv_off;
	    ValDevTabToStrings(mds[pst_pair],j+PAIR_DX1+1,pst->u.pair.vr[0].adjust);
	    ValDevTabToStrings(mds[pst_pair],j+PAIR_DX2+1,pst->u.pair.vr[1].adjust);
	} else {
	    mds[pst->type][j+1].u.md_str = SFNameList2NameUni(sf,pst->u.subs.variant);
	}
    }
    for ( isv=0; isv<2; ++isv ) {
	for ( kp=isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) {
	    j = (cnts[pst_pair]++ * mi[pst_pair-1].col_cnt);
	    mds[pst_pair][j+0].u.md_ival = (intpt) kp->subtable;
	    mds[pst_pair][j+1].u.md_str = SCNameUniStr(kp->sc);
	    if ( isv ) {
		mds[pst_pair][j+PAIR_DY_ADV1].u.md_ival = kp->off;
		DevTabToString(&mds[pst_pair][j+PAIR_DY_ADV1+1].u.md_str,kp->adjust);
	    } else if ( kp->subtable->lookup->lookup_flags&pst_r2l ) {
		mds[pst_pair][j+PAIR_DX_ADV2].u.md_ival = kp->off;
		DevTabToString(&mds[pst_pair][j+PAIR_DX_ADV2+1].u.md_str,kp->adjust);
	    } else {
		mds[pst_pair][j+PAIR_DX_ADV1].u.md_ival = kp->off;
		DevTabToString(&mds[pst_pair][j+PAIR_DX_ADV1+1].u.md_str,kp->adjust);
	    }
	}
    }
    for ( i=pst_null+1; i<pst_lcaret /* == pst_max-1 */; ++i ) {
	GMatrixEditSet(GWidgetGetControl(ci->gw,CID_List+(i-1)*100),
		mds[i],cnts[i],false);
    }
    /* There's always a pane showing kerning data */
    CI_DoHideUnusedPair(ci);
    CI_DoHideUnusedSingle(ci);

    bits = SFGetAlternate(sc->parent,sc->unicodeenc,sc,true);
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_ComponentMsg),
	bits==NULL ? _("No components") :
	hascomposing(sc->parent,sc->unicodeenc,sc) ? _("Accented glyph composed of:") :
	    _("Glyph composed of:"));
    if ( bits==NULL ) {
	ubuf[0] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Components),ubuf);
    } else {
	unichar_t *temp = malloc(11*u_strlen(bits)*sizeof(unichar_t));
	unichar_t *upt=temp;
	while ( *bits!='\0' ) {
	    sprintf(buffer, "U+%04x ", *bits );
	    uc_strcpy(upt,buffer);
	    upt += u_strlen(upt);
	    ++bits;
	}
	upt[-1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Components),temp);
	free(temp);
    }

    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),0);

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_UnlinkRmOverlap),sc->unlink_rm_ovrlp_save_undo);

    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_Comment),
	    sc->comment?sc->comment:"");
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_GClass),sc->glyph_class);
    CI_SetColorList(ci,sc->color);
    ci->first = sc->comment==NULL;

    ti = malloc((sc->countermask_cnt+1)*sizeof(GTextInfo *));
    ti[sc->countermask_cnt] = calloc(1,sizeof(GTextInfo));
    for ( i=0; i<sc->countermask_cnt; ++i ) {
	ti[i] = calloc(1,sizeof(GTextInfo));
	ti[i]->text = CounterMaskLine(sc,&sc->countermasks[i]);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	ti[i]->userdata = chunkalloc(sizeof(HintMask));
	memcpy(ti[i]->userdata,sc->countermasks[i],sizeof(HintMask));
    }
    GGadgetSetList(GWidgetGetControl(ci->gw,CID_List+600),ti,false);

    if ( sc->tex_height!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_height);
    else
	buffer[0] = '\0';
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TeX_Height),buffer);

    if ( sc->tex_depth!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_depth);
    else
	buffer[0] = '\0';
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TeX_Depth),buffer);

    if ( sc->italic_correction!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->italic_correction);
    else
	buffer[0] = '\0';
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TeX_Italic),buffer);
    DevTabToString(&devtabstr,sc->italic_adjusts);
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_ItalicDevTab),devtabstr==NULL?"":devtabstr);
    free(devtabstr);

    if ( sc->top_accent_horiz!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->top_accent_horiz);
    else
	buffer[0] = '\0';
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_HorAccent),buffer);
    DevTabToString(&devtabstr,sc->top_accent_adjusts);
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_AccentDevTab),devtabstr==NULL?"":devtabstr);
    free(devtabstr);

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_IsExtended),sc->is_extended_shape);

    {
	GGadget *g = GWidgetGetControl(ci->gw,CID_VariantList+0*100);
	if ( sc->vert_variants==NULL || sc->vert_variants->variants==NULL )
	    GGadgetSetTitle8(g,"");
	else
	    GGadgetSetTitle8(g,sc->vert_variants->variants);
	sprintf(buffer,"%d",sc->vert_variants!=NULL?sc->vert_variants->italic_correction:0);
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_ExtItalicCor+0*100),buffer);
	DevTabToString(&devtabstr,sc->vert_variants!=NULL?
		sc->vert_variants->italic_adjusts:
		NULL);
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_ExtItalicDev+0*100),devtabstr==NULL?"":devtabstr);
	free(devtabstr);

	g = GWidgetGetControl(ci->gw,CID_VariantList+1*100);
	if ( sc->horiz_variants==NULL || sc->horiz_variants->variants==NULL )
	    GGadgetSetTitle8(g,"");
	else
	    GGadgetSetTitle8(g,sc->horiz_variants->variants);
	sprintf(buffer,"%d",sc->horiz_variants!=NULL?sc->horiz_variants->italic_correction:0);
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_ExtItalicCor+1*100),buffer);
	DevTabToString(&devtabstr,sc->horiz_variants!=NULL?
		sc->horiz_variants->italic_adjusts:
		NULL);
	GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_ExtItalicDev+1*100),devtabstr==NULL?"":devtabstr);
	free(devtabstr);
    }
    for ( i=0; i<2; ++i ) {
	struct glyphvariants *gv = i ? sc->horiz_variants : sc->vert_variants ;
	GGadget *g = GWidgetGetControl(ci->gw,CID_ExtensionList+i*100);
	GV_ToMD(g, gv);
    }
    GA_ToMD(GWidgetGetControl(ci->gw,CID_AltUni), sc);

    if ( ci->sc->parent->multilayer ) {
	int margined = sc->tile_margin!=0 || (sc->tile_bounds.minx==0 && sc->tile_bounds.maxx==0);
	char buffer[40];

	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_IsTileMargin),margined);
	if ( margined ) {
	    sprintf( buffer, "%g", (double) sc->tile_margin );
	    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileMargin),buffer);
	    CI_BoundsToMargin(ci);
	} else {
	    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileMargin),"0");
	    sprintf( buffer, "%g", (double) sc->tile_bounds.minx );
	    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileBBoxMinX),buffer);
	    sprintf( buffer, "%g", (double) sc->tile_bounds.miny );
	    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileBBoxMinY),buffer);
	    sprintf( buffer, "%g", (double) sc->tile_bounds.maxx );
	    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileBBoxMaxX),buffer);
	    sprintf( buffer, "%g", (double) sc->tile_bounds.maxy );
	    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_TileBBoxMaxY),buffer);
	}
    }

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_DefLCCount), !sc->lig_caret_cnt_fixed );
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LCCountLab), sc->lig_caret_cnt_fixed );
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LCCount), sc->lig_caret_cnt_fixed );
    ci->lc_seen = false;
    CI_NoteAspect(ci);
}

static int CI_NextPrev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int enc = ci->enc + GGadgetGetCid(g);	/* cid is 1 for next, -1 for prev */
	SplineChar *new_;
	struct splinecharlist *scl;

	if ( enc<0 || enc>=ci->map->enccount ) {
	    GGadgetSetEnabled(g,false);
return( true );
	}
	if ( !_CI_OK(ci))
return( true );
	new_ = SFMakeChar(ci->sc->parent,ci->map,enc);
	if ( new_->charinfo!=NULL && new_->charinfo!=ci ) {
	    GGadgetSetEnabled(g,false);
return( true );
	}
	ci->sc = new_;
	ci->enc = enc;
	for ( scl=ci->changes; scl!=NULL && scl->sc->orig_pos!=new_->orig_pos;
		scl = scl->next );
	ci->cachedsc = scl==NULL ? NULL : scl->sc;
	CIFillup(ci);
    }
return( true );
}

static void CI_DoCancel(CharInfo *ci) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(ci->gw,CID_List+600),&len);

    for ( i=0; i<len; ++i )
	chunkfree(ti[i]->userdata,sizeof(HintMask));
    CI_Finish(ci);
}

static int CI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CI_DoCancel(ci);
    }
return( true );
}

static int ci_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	CharInfo *ci = GDrawGetUserData(gw);
	CI_DoCancel(ci);
    } else if ( event->type==et_char ) {
	CharInfo *ci = GDrawGetUserData(gw);
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html");
return( true );
	} else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	    MenuExit(NULL,NULL,NULL);
	} else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
	    CI_DoCancel(ci);
	}
return( false );
    } else if ( event->type == et_destroy ) {
	CharInfo *ci = GDrawGetUserData(gw);
	ci->sc->charinfo = NULL;
	free(ci);
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void SCCharInfo(SplineChar *sc,int deflayer, EncMap *map,int enc) {
    CharInfo *ci;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData ugcd[14], cgcd[6], psgcd[7][7], cogcd[3], mgcd[9], tgcd[16];
    GGadgetCreateData lcgcd[4], vargcd[2][7];
    GTextInfo ulabel[14], clabel[6], pslabel[7][6], colabel[3], mlabel[9], tlabel[16];
    GTextInfo lclabel[4], varlabel[2][6];
    GGadgetCreateData mbox[4], *mvarray[7], *mharray1[7], *mharray2[8];
    GGadgetCreateData ubox[3], *uhvarray[29], *uharray[6];
    GGadgetCreateData cbox[3], *cvarray[5], *charray[4];
    GGadgetCreateData pstbox[7][4], *pstvarray[7][5], *pstharray1[7][8];
    GGadgetCreateData cobox[2], *covarray[4];
    GGadgetCreateData tbox[3], *thvarray[36], *tbarray[4];
    GGadgetCreateData lcbox[2], *lchvarray[4][4];
    GGadgetCreateData varbox[2][2], *varhvarray[2][5][4];
    GGadgetCreateData tilegcd[16], tilebox[4];
    GTextInfo tilelabel[16];
    GGadgetCreateData *tlvarray[6], *tlharray[4], *tlhvarray[4][5];
    int i;
    GTabInfo aspects[17];
    static GBox smallbox = { bt_raised, bs_rect, 2, 1, 0, 0, 0, 0, 0, 0, COLOR_DEFAULT, COLOR_DEFAULT, 0, 0, 0, 0, 0, 0, 0 };
    static int boxset=0;
    FontRequest rq;
    static GFont *font=NULL;

    CharInfoInit();

    if ( sc->charinfo!=NULL ) {
	GDrawSetVisible(sc->charinfo->gw,true);
	GDrawRaise(sc->charinfo->gw);
return;
    }

    ci = calloc(1,sizeof(CharInfo));
    ci->sc = sc;
    ci->def_layer = deflayer;
    ci->done = false;
    ci->map = map;
    ci->last = 0xffffff;
    if ( enc==-1 )
	enc = map->backmap[sc->orig_pos];
    ci->enc = enc;

    if ( !boxset ) {
	extern GBox _ggadget_Default_Box;
	GGadgetInit();
	smallbox = _ggadget_Default_Box;
	smallbox.padding = 1;
	boxset = 1;
    }

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = false;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Glyph Info");
	wattrs.is_dlg = false;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,CI_Width+65));
	pos.height = GDrawPointsToPixels(NULL,CI_Height);
	ci->gw = GDrawCreateTopWindow(NULL,&pos,ci_e_h,ci,&wattrs);

	memset(&ugcd,0,sizeof(ugcd));
	memset(&ubox,0,sizeof(ubox));
	memset(&ulabel,0,sizeof(ulabel));

	ulabel[0].text = (unichar_t *) _("Gl_yph Name:");
	ulabel[0].text_is_1byte = true;
	ulabel[0].text_in_resource = true;
	ugcd[0].gd.label = &ulabel[0];
	ugcd[0].gd.pos.x = 5; ugcd[0].gd.pos.y = 5+4; 
	ugcd[0].gd.flags = gg_enabled|gg_visible;
	ugcd[0].gd.mnemonic = 'N';
	ugcd[0].creator = GLabelCreate;
	uhvarray[0] = &ugcd[0];

	ugcd[1].gd.pos.x = 85; ugcd[1].gd.pos.y = 5;
	ugcd[1].gd.flags = gg_enabled|gg_visible;
	ugcd[1].gd.mnemonic = 'N';
	ugcd[1].gd.cid = CID_UName;
	ugcd[1].creator = GListFieldCreate;
	ugcd[1].data = (void *) (-2);
	uhvarray[1] = &ugcd[1]; uhvarray[2] = NULL;

	ulabel[2].text = (unichar_t *) _("Unicode _Value:");
	ulabel[2].text_in_resource = true;
	ulabel[2].text_is_1byte = true;
	ugcd[2].gd.label = &ulabel[2];
	ugcd[2].gd.pos.x = 5; ugcd[2].gd.pos.y = 31+4; 
	ugcd[2].gd.flags = gg_enabled|gg_visible;
	ugcd[2].gd.mnemonic = 'V';
	ugcd[2].creator = GLabelCreate;
	uhvarray[3] = &ugcd[2];

	ugcd[3].gd.pos.x = 85; ugcd[3].gd.pos.y = 31;
	ugcd[3].gd.flags = gg_enabled|gg_visible;
	ugcd[3].gd.mnemonic = 'V';
	ugcd[3].gd.cid = CID_UValue;
	ugcd[3].gd.handle_controlevent = CI_UValChanged;
	ugcd[3].creator = GTextFieldCreate;
	uhvarray[4] = &ugcd[3]; uhvarray[5] = NULL;

	ulabel[4].text = (unichar_t *) _("Unicode C_har:");
	ulabel[4].text_in_resource = true;
	ulabel[4].text_is_1byte = true;
	ugcd[4].gd.label = &ulabel[4];
	ugcd[4].gd.pos.x = 5; ugcd[4].gd.pos.y = 57+4; 
	ugcd[4].gd.flags = gg_enabled|gg_visible;
	ugcd[4].gd.mnemonic = 'h';
	ugcd[4].creator = GLabelCreate;
	uhvarray[6] = &ugcd[4];

	ugcd[5].gd.pos.x = 85; ugcd[5].gd.pos.y = 57;
	ugcd[5].gd.flags = gg_enabled|gg_visible|gg_text_xim;
	ugcd[5].gd.mnemonic = 'h';
	ugcd[5].gd.cid = CID_UChar;
	ugcd[5].gd.handle_controlevent = CI_CharChanged;
	ugcd[5].creator = GTextFieldCreate;
	uhvarray[7] = &ugcd[5]; uhvarray[8] = NULL;

	ugcd[6].gd.pos.x = 12; ugcd[6].gd.pos.y = 117;
	ugcd[6].gd.flags = gg_visible | gg_enabled;
	ulabel[6].text = (unichar_t *) _("Set From N_ame");
	ulabel[6].text_is_1byte = true;
	ulabel[6].text_in_resource = true;
	ugcd[6].gd.mnemonic = 'a';
	ugcd[6].gd.label = &ulabel[6];
	ugcd[6].gd.handle_controlevent = CI_SName;
	ugcd[6].creator = GButtonCreate;
	uharray[0] = GCD_Glue; uharray[1] = &ugcd[6];

	ugcd[7].gd.pos.x = 107; ugcd[7].gd.pos.y = 117;
	ugcd[7].gd.flags = gg_visible | gg_enabled;
	ulabel[7].text = (unichar_t *) _("Set From Val_ue");
	ulabel[7].text_is_1byte = true;
	ulabel[7].text_in_resource = true;
	ugcd[7].gd.mnemonic = 'l';
	ugcd[7].gd.label = &ulabel[7];
	ugcd[7].gd.handle_controlevent = CI_SValue;
	ugcd[7].creator = GButtonCreate;
	uharray[2] = GCD_Glue; uharray[3] = &ugcd[7]; uharray[4] = GCD_Glue; uharray[5] = NULL;

	ubox[2].gd.flags = gg_enabled|gg_visible;
	ubox[2].gd.u.boxelements = uharray;
	ubox[2].creator = GHBoxCreate;
	uhvarray[9] = &ubox[2]; uhvarray[10] = GCD_ColSpan; uhvarray[11] = NULL;

	ugcd[8].gd.flags = gg_visible | gg_enabled|gg_utf8_popup;
	ulabel[8].text = (unichar_t *) _("Alternate Unicode Encodings / Variation Selectors");
	ulabel[8].text_is_1byte = true;
	ugcd[8].gd.label = &ulabel[8];
	ugcd[8].gd.popup_msg = (unichar_t *) _(
	    "Some glyphs may be used for more than one\n"
	    "unicode code point -- I don't recommend\n"
	    "doing this, better to use a reference --\n"
	    "but it is possible.\n"
	    "The latin \"A\", the greek \"Alpha\" and the\n"
	    "cyrillic \"A\" look very much the same.\n\n"
	    "On the other hand certain Mongolian and CJK\n"
	    "characters have multiple glyphs depending\n"
	    "on a unicode Variation Selector.\n\n"
	    "In the first case use a variation selector\n"
	    "of 0, in the second use the appropriate\n"
	    "codepoint.");
	ugcd[8].creator = GLabelCreate;
	uhvarray[12] = &ugcd[8]; uhvarray[13] = GCD_ColSpan; uhvarray[14] = NULL;

	ugcd[9].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	ugcd[9].gd.u.matrix = &mi_altuniinfo;
	ugcd[9].gd.cid = CID_AltUni;
	ugcd[9].gd.popup_msg = ugcd[8].gd.popup_msg;
	ugcd[9].creator = GMatrixEditCreate;
	uhvarray[15] = &ugcd[9]; uhvarray[16] = GCD_ColSpan; uhvarray[17] = NULL;

	ugcd[10].gd.pos.x = 5; ugcd[10].gd.pos.y = 83+4;
	ugcd[10].gd.flags = gg_visible | gg_enabled;
	ulabel[10].text = (unichar_t *) _("OT _Glyph Class:");
	ulabel[10].text_is_1byte = true;
	ulabel[10].text_in_resource = true;
	ugcd[10].gd.label = &ulabel[10];
	ugcd[10].creator = GLabelCreate;
	uhvarray[18] = &ugcd[10];

	ugcd[11].gd.pos.x = 85; ugcd[11].gd.pos.y = 83;
	ugcd[11].gd.flags = gg_visible | gg_enabled;
	ugcd[11].gd.cid = CID_GClass;
	ugcd[11].gd.u.list = glyphclasses;
	ugcd[11].creator = GListButtonCreate;
	uhvarray[19] = &ugcd[11]; uhvarray[20] = NULL;

	ugcd[12].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	ulabel[12].text = (unichar_t *) _("Mark for Unlink, Remove Overlap before Generating");
	ulabel[12].text_is_1byte = true;
	ulabel[12].text_in_resource = true;
	ugcd[12].gd.label = &ulabel[12];
	ugcd[12].gd.cid = CID_UnlinkRmOverlap;
	ugcd[12].gd.popup_msg = (unichar_t *) _("A few glyphs, like Aring, Ccedilla, Eogonek\nare composed of two overlapping references.\nOften it is desirable to retain the references\n(so that changes made to the base glyph are\nreflected in the composed glyph), but that\nmeans you are stuck with overlapping contours.\nThis flag means that just before generating\nthe font, FontForge will unlink the references\nand run remove overlap on them, while\n retaining the references in the SFD.");
	ugcd[12].creator = GCheckBoxCreate;
	uhvarray[21] = &ugcd[12]; uhvarray[22] = GCD_ColSpan; uhvarray[23] = NULL;
	uhvarray[24] = GCD_Glue; uhvarray[25] = GCD_Glue; uhvarray[26] = NULL;
	uhvarray[27] = NULL;

	ubox[0].gd.flags = gg_enabled|gg_visible;
	ubox[0].gd.u.boxelements = uhvarray;
	ubox[0].creator = GHVBoxCreate;


	memset(&cgcd,0,sizeof(cgcd));
	memset(&cbox,0,sizeof(cbox));
	memset(&clabel,0,sizeof(clabel));

	clabel[0].text = (unichar_t *) _("Comment");
	clabel[0].text_is_1byte = true;
	cgcd[0].gd.label = &clabel[0];
	cgcd[0].gd.pos.x = 5; cgcd[0].gd.pos.y = 5; 
	cgcd[0].gd.flags = gg_enabled|gg_visible;
	cgcd[0].creator = GLabelCreate;
	cvarray[0] = &cgcd[0];

	cgcd[1].gd.pos.x = 5; cgcd[1].gd.pos.y = cgcd[0].gd.pos.y+13;
	cgcd[1].gd.pos.height = 7*12+6;
	cgcd[1].gd.flags = gg_enabled|gg_visible|gg_textarea_wrap|gg_text_xim;
	cgcd[1].gd.cid = CID_Comment;
	cgcd[1].gd.handle_controlevent = CI_CommentChanged;
	cgcd[1].creator = GTextAreaCreate;
	cvarray[1] = &cgcd[1]; cvarray[2] = GCD_Glue;

	clabel[2].text = (unichar_t *) _("Color:");
	clabel[2].text_is_1byte = true;
	cgcd[2].gd.label = &clabel[2];
	cgcd[2].gd.pos.x = 5; cgcd[2].gd.pos.y = cgcd[1].gd.pos.y+cgcd[1].gd.pos.height+5+6; 
	cgcd[2].gd.flags = gg_enabled|gg_visible;
	cgcd[2].creator = GLabelCreate;
	charray[0] = &cgcd[2];

	cgcd[3].gd.pos.x = cgcd[3].gd.pos.x; cgcd[3].gd.pos.y = cgcd[2].gd.pos.y-6;
	cgcd[3].gd.flags = gg_enabled|gg_visible;
	cgcd[3].gd.cid = CID_Color;
	cgcd[3].gd.handle_controlevent = CI_PickColor;
	std_colors[0].image = GGadgetImageCache("colorwheel.png");
	cgcd[3].gd.u.list = std_colors;
	cgcd[3].creator = GListButtonCreate;
	charray[1] = &cgcd[3]; charray[2] = GCD_Glue; charray[3] = NULL;

	cbox[2].gd.flags = gg_enabled|gg_visible;
	cbox[2].gd.u.boxelements = charray;
	cbox[2].creator = GHBoxCreate;
	cvarray[3] = &cbox[2]; cvarray[4] = NULL;

	cbox[0].gd.flags = gg_enabled|gg_visible;
	cbox[0].gd.u.boxelements = cvarray;
	cbox[0].creator = GVBoxCreate;

	memset(&psgcd,0,sizeof(psgcd));
	memset(&pstbox,0,sizeof(pstbox));
	memset(&pslabel,0,sizeof(pslabel));

	for ( i=0; i<6; ++i ) {
	    psgcd[i][0].gd.pos.x = 5; psgcd[i][0].gd.pos.y = 5;
	    psgcd[i][0].gd.flags = gg_visible | gg_enabled;
	    psgcd[i][0].gd.cid = CID_List+i*100;
	    psgcd[i][0].gd.u.matrix = &mi[i];
	    mi[i].col_init[0].enum_vals = SFSubtableListOfType(sc->parent, pst2lookuptype[i+1], false, false);
	    psgcd[i][0].creator = GMatrixEditCreate;
	}
	for ( i=pst_position; i<=pst_pair; ++i ) {
	    pslabel[i-1][1].text = (unichar_t *) _("_Hide Unused Columns");
	    pslabel[i-1][1].text_is_1byte = true;
	    pslabel[i-1][1].text_in_resource = true;
	    psgcd[i-1][1].gd.label = &pslabel[i-1][1];
	    psgcd[i-1][1].gd.pos.x = 5; psgcd[i-1][1].gd.pos.y = 5+4; 
	    psgcd[i-1][1].gd.flags = lookup_hideunused ? (gg_enabled|gg_visible|gg_cb_on|gg_utf8_popup) : (gg_enabled|gg_visible|gg_utf8_popup);
	    psgcd[i-1][1].gd.popup_msg = (unichar_t *) _("Don't display columns of 0s.\nThe OpenType lookup allows for up to 8 kinds\nof data, but almost all kerning lookups will use just one.\nOmitting the others makes the behavior clearer.");
	    psgcd[i-1][1].gd.handle_controlevent = i==pst_position ? CI_HideUnusedSingle : CI_HideUnusedPair;
	    psgcd[i-1][1].creator = GCheckBoxCreate;
	    pstvarray[i-1][0] = &psgcd[i-1][0];
	    pstvarray[i-1][1] = &psgcd[i-1][1];
	    pstvarray[i-1][2] = NULL;

	    pstbox[i-1][0].gd.flags = gg_enabled|gg_visible;
	    pstbox[i-1][0].gd.u.boxelements = pstvarray[i-1];
	    pstbox[i-1][0].creator = GVBoxCreate;
	}

	    psgcd[6][0].gd.pos.x = 5; psgcd[6][0].gd.pos.y = 5;
	    psgcd[6][0].gd.flags = gg_visible | gg_enabled;
	    psgcd[6][0].gd.cid = CID_List+6*100;
	    psgcd[6][0].gd.handle_controlevent = CI_CounterSelChanged;
	    psgcd[6][0].gd.box = &smallbox;
	    psgcd[6][0].creator = GListCreate;
	    pstvarray[6][0] = &psgcd[6][0];

	    psgcd[6][1].gd.pos.x = 10; psgcd[6][1].gd.pos.y = psgcd[6][0].gd.pos.y+psgcd[6][0].gd.pos.height+4;
	    psgcd[6][1].gd.flags = gg_visible | gg_enabled;
	    pslabel[6][1].text = (unichar_t *) S_("CounterHint|_New...");
	    pslabel[6][1].text_is_1byte = true;
	    pslabel[6][1].text_in_resource = true;
	    psgcd[6][1].gd.label = &pslabel[6][1];
	    psgcd[6][1].gd.cid = CID_New+6*100;
	    psgcd[6][1].gd.handle_controlevent = CI_NewCounter;
	    psgcd[6][1].gd.box = &smallbox;
	    psgcd[6][1].creator = GButtonCreate;
	    pstharray1[6][0] = GCD_Glue; pstharray1[6][1] = &psgcd[6][1];

	    psgcd[6][2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); psgcd[6][2].gd.pos.y = psgcd[6][1].gd.pos.y;
	    psgcd[6][2].gd.flags = gg_visible;
	    pslabel[6][2].text = (unichar_t *) _("_Delete");
	    pslabel[6][2].text_is_1byte = true;
	    pslabel[6][2].text_in_resource = true;
	    psgcd[6][2].gd.label = &pslabel[6][2];
	    psgcd[6][2].gd.cid = CID_Delete+6*100;
	    psgcd[6][2].gd.handle_controlevent = CI_DeleteCounter;
	    psgcd[6][2].gd.box = &smallbox;
	    psgcd[6][2].creator = GButtonCreate;
	    pstharray1[6][2] = GCD_Glue; pstharray1[6][3] = &psgcd[6][2];

	    psgcd[6][3].gd.pos.x = -10; psgcd[6][3].gd.pos.y = psgcd[6][1].gd.pos.y;
	    psgcd[6][3].gd.flags = gg_visible;
	    pslabel[6][3].text = (unichar_t *) _("_Edit...");
	    pslabel[6][3].text_is_1byte = true;
	    pslabel[6][3].text_in_resource = true;
	    psgcd[6][3].gd.label = &pslabel[6][3];
	    psgcd[6][3].gd.cid = CID_Edit+6*100;
	    psgcd[6][3].gd.handle_controlevent = CI_EditCounter;
	    psgcd[6][3].gd.box = &smallbox;
	    psgcd[6][3].creator = GButtonCreate;
	    pstharray1[6][4] = GCD_Glue; pstharray1[6][5] = &psgcd[6][3]; pstharray1[6][6] = GCD_Glue; pstharray1[6][7] = NULL;

	    pstbox[6][2].gd.flags = gg_enabled|gg_visible;
	    pstbox[6][2].gd.u.boxelements = pstharray1[6];
	    pstbox[6][2].creator = GHBoxCreate;
	    pstvarray[6][1] = &pstbox[6][2]; pstvarray[6][2] = NULL;

	    pstbox[6][0].gd.flags = gg_enabled|gg_visible;
	    pstbox[6][0].gd.u.boxelements = pstvarray[6];
	    pstbox[6][0].creator = GVBoxCreate;
	psgcd[6][4].gd.flags = psgcd[6][5].gd.flags = 0;	/* No copy, paste for hint masks */

	memset(&cogcd,0,sizeof(cogcd));
	memset(&cobox,0,sizeof(cobox));
	memset(&colabel,0,sizeof(colabel));

	colabel[0].text = (unichar_t *) _("Accented glyph composed of:");
	colabel[0].text_is_1byte = true;
	cogcd[0].gd.label = &colabel[0];
	cogcd[0].gd.pos.x = 5; cogcd[0].gd.pos.y = 5; 
	cogcd[0].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	cogcd[0].gd.cid = CID_ComponentMsg;
	cogcd[0].creator = GLabelCreate;

	cogcd[1].gd.pos.x = 5; cogcd[1].gd.pos.y = cogcd[0].gd.pos.y+12;
	cogcd[1].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	cogcd[1].gd.cid = CID_Components;
	cogcd[1].creator = GLabelCreate;

	covarray[0] = &cogcd[0]; covarray[1] = &cogcd[1]; covarray[2] = GCD_Glue; covarray[3] = NULL;
	cobox[0].gd.flags = gg_enabled|gg_visible;
	cobox[0].gd.u.boxelements = covarray;
	cobox[0].creator = GVBoxCreate;
	


	memset(&tgcd,0,sizeof(tgcd));
	memset(&tbox,0,sizeof(tbox));
	memset(&tlabel,0,sizeof(tlabel));

	tlabel[0].text = (unichar_t *) _("Height:");
	tlabel[0].text_is_1byte = true;
	tgcd[0].gd.label = &tlabel[0];
	tgcd[0].gd.pos.x = 5; tgcd[0].gd.pos.y = 5+4; 
	tgcd[0].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[0].gd.popup_msg = (unichar_t *) _("The height and depth fields are the metrics fields used\nby TeX, they are corrected for optical distortion.\nSo 'x' and 'o' probably have the same height.");
	tgcd[0].creator = GLabelCreate;
	thvarray[0] = &tgcd[0];

	tgcd[1].gd.pos.x = 85; tgcd[1].gd.pos.y = 5;
	tgcd[1].gd.pos.width = 60;
	tgcd[1].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[1].gd.cid = CID_TeX_Height;
	tgcd[1].creator = GTextFieldCreate;
	tgcd[1].gd.popup_msg = tgcd[0].gd.popup_msg;
	thvarray[1] = &tgcd[1];

	tlabel[2].text = (unichar_t *) _("Guess");
	tlabel[2].text_is_1byte = true;
	tgcd[2].gd.label = &tlabel[2];
	tgcd[2].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[2].gd.cid = CID_TeX_HeightD;
	tgcd[2].gd.handle_controlevent = TeX_Default;
	tgcd[2].creator = GButtonCreate;
	tgcd[2].gd.popup_msg = tgcd[0].gd.popup_msg;
	thvarray[2] = &tgcd[2]; thvarray[3] = GCD_Glue; thvarray[4] = NULL;

	tlabel[3].text = (unichar_t *) _("Depth:");
	tlabel[3].text_is_1byte = true;
	tgcd[3].gd.label = &tlabel[3];
	tgcd[3].gd.pos.x = 5; tgcd[3].gd.pos.y = 31+4; 
	tgcd[3].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[3].gd.popup_msg = tgcd[0].gd.popup_msg;
	tgcd[3].creator = GLabelCreate;
	thvarray[5] = &tgcd[3];

	tgcd[4].gd.pos.x = 85; tgcd[4].gd.pos.y = 31;
	tgcd[4].gd.pos.width = 60;
	tgcd[4].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[4].gd.cid = CID_TeX_Depth;
	tgcd[4].creator = GTextFieldCreate;
	tgcd[4].gd.popup_msg = tgcd[0].gd.popup_msg;
	thvarray[6] = &tgcd[4];

	tlabel[5].text = (unichar_t *) _("Guess");
	tlabel[5].text_is_1byte = true;
	tgcd[5].gd.label = &tlabel[5];
	tgcd[5].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[5].gd.cid = CID_TeX_DepthD;
	tgcd[5].gd.handle_controlevent = TeX_Default;
	tgcd[5].creator = GButtonCreate;
	tgcd[5].gd.popup_msg = tgcd[0].gd.popup_msg;
	thvarray[7] = &tgcd[5];  thvarray[8] = GCD_Glue; thvarray[9] = NULL;

	tlabel[6].text = (unichar_t *) _("Italic Correction:");
	tlabel[6].text_is_1byte = true;
	tgcd[6].gd.label = &tlabel[6];
	tgcd[6].gd.pos.x = 5; tgcd[6].gd.pos.y = 57+4; 
	tgcd[6].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[6].creator = GLabelCreate;
	tgcd[6].gd.popup_msg = (unichar_t *) _("The Italic correction field is used by both TeX and the MS 'MATH'\ntable. It is used when joining slanted text (italic) to upright.\nIt is the amount of extra white space needed so the slanted text\nwill not run into the upright text.");
	thvarray[10] = &tgcd[6];

	tgcd[7].gd.pos.x = 85; tgcd[7].gd.pos.y = 57;
	tgcd[7].gd.pos.width = 60;
	tgcd[7].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[7].gd.cid = CID_TeX_Italic;
	tgcd[7].creator = GTextFieldCreate;
	tgcd[7].gd.popup_msg = tgcd[6].gd.popup_msg;
	thvarray[11] = &tgcd[7];

	tlabel[8].text = (unichar_t *) _("Guess");
	tlabel[8].text_is_1byte = true;
	tgcd[8].gd.label = &tlabel[8];
	tgcd[8].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[8].gd.cid = CID_TeX_ItalicD;
	tgcd[8].gd.handle_controlevent = TeX_Default;
	tgcd[8].creator = GButtonCreate;
	tgcd[8].gd.popup_msg = tgcd[6].gd.popup_msg;
	thvarray[12] = &tgcd[8];

	tgcd[9].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[9].gd.cid = CID_ItalicDevTab;
	tgcd[9].creator = GTextFieldCreate;
	tgcd[9].gd.popup_msg = (unichar_t *) _("A device table for italic correction.\nExpects a comma separated list of <pixelsize>\":\"<adjustment>\nAs \"9:-1,12:1,13:1\"");
	thvarray[13] = &tgcd[9]; thvarray[14] = NULL;

	tlabel[10].text = (unichar_t *) _("Top Accent Pos:");
	tlabel[10].text_is_1byte = true;
	tgcd[10].gd.label = &tlabel[10];
	tgcd[10].gd.pos.x = 5; tgcd[10].gd.pos.y = 57+4; 
	tgcd[10].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[10].gd.popup_msg = tgcd[9].gd.popup_msg;
	tgcd[10].creator = GLabelCreate;
	tgcd[10].gd.popup_msg = (unichar_t *) _("In the MS 'MATH' table this value specifies where (horizontally)\nan accent should be placed above the glyph. Vertical placement\nis handled by other means");
	thvarray[15] = &tgcd[10];

	tgcd[11].gd.pos.x = 85; tgcd[11].gd.pos.y = 57;
	tgcd[11].gd.pos.width = 60;
	tgcd[11].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[11].gd.cid = CID_HorAccent;
	tgcd[11].creator = GTextFieldCreate;
	tgcd[11].gd.popup_msg = tgcd[10].gd.popup_msg;
	thvarray[16] = &tgcd[11];

	tlabel[12].text = (unichar_t *) _("Guess");
	tlabel[12].text_is_1byte = true;
	tgcd[12].gd.label = &tlabel[12];
	tgcd[12].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[12].gd.cid = CID_HorAccentD;
	tgcd[12].gd.handle_controlevent = TeX_Default;
	tgcd[12].creator = GButtonCreate;
	tgcd[12].gd.popup_msg = tgcd[10].gd.popup_msg;
	thvarray[17] = &tgcd[12];

	tgcd[13].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[13].gd.cid = CID_AccentDevTab;
	tgcd[13].creator = GTextFieldCreate;
	tgcd[13].gd.popup_msg = (unichar_t *) _("A device table for horizontal accent positioning.\nExpects a comma separated list of <pixelsize>\":\"<adjustment>\nAs \"9:-1,12:1,13:1\"");
	thvarray[18] = &tgcd[13]; thvarray[19] = NULL;

	tlabel[14].text = (unichar_t *) _("Is Extended Shape");
	tlabel[14].text_is_1byte = true;
	tgcd[14].gd.label = &tlabel[14];
	tgcd[14].gd.pos.x = 5; tgcd[14].gd.pos.y = 57+4; 
	tgcd[14].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[14].gd.cid = CID_IsExtended;
	tgcd[14].creator = GCheckBoxCreate;
	tgcd[14].gd.popup_msg = (unichar_t *) _("Is this an extended shape (like a tall parenthesis)?\nExtended shapes need special attention for vertical\nsuperscript placement.");
	thvarray[20] = &tgcd[14];
	thvarray[21] = thvarray[22] = GCD_ColSpan; thvarray[23] = GCD_Glue; thvarray[24] = NULL;

	tlabel[15].text = (unichar_t *) _("Math Kerning");	/* Graphical */
	tlabel[15].text_is_1byte = true;
	tgcd[15].gd.label = &tlabel[15];
	tgcd[15].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[15].gd.handle_controlevent = CI_SubSuperPositionings;
	tgcd[15].creator = GButtonCreate;
	tgcd[15].gd.popup_msg = (unichar_t *) _("Brings up a dialog which gives fine control over\nhorizontal positioning of subscripts and superscripts\ndepending on their vertical positioning.");
	tbarray[0] = GCD_Glue; tbarray[1] = &tgcd[15]; tbarray[2] = GCD_Glue; tbarray[3] = NULL;

	tbox[2].gd.flags = gg_enabled|gg_visible;
	tbox[2].gd.u.boxelements = tbarray;
	tbox[2].creator = GHBoxCreate;

	thvarray[25] = &tbox[2];
	thvarray[26] = thvarray[27] = thvarray[28] = GCD_ColSpan; thvarray[29] = NULL;

	thvarray[30] = thvarray[31] = thvarray[32] = thvarray[33] = GCD_Glue; thvarray[34] = NULL;
	thvarray[35] = NULL;

	tbox[0].gd.flags = gg_enabled|gg_visible;
	tbox[0].gd.u.boxelements = thvarray;
	tbox[0].creator = GHVBoxCreate;

	memset(&lcgcd,0,sizeof(lcgcd));
	memset(&lcbox,0,sizeof(lcbox));
	memset(&lclabel,0,sizeof(lclabel));

	lclabel[0].text = (unichar_t *) _("Default Ligature Caret Count");
	lclabel[0].text_is_1byte = true;
	lclabel[0].text_in_resource = true;
	lcgcd[0].gd.cid = CID_DefLCCount;
	lcgcd[0].gd.label = &lclabel[0];
	lcgcd[0].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	lcgcd[0].gd.popup_msg = (unichar_t *) _("Ligature caret locations are used by a text editor\nwhen it needs to draw a text edit caret inside a\nligature. This means there should be a caret between\neach ligature component so if there are n components\nthere should be n-1 caret locations.\n  You may adjust the caret locations themselves in the\noutline glyph view (drag them from to origin to the\nappropriate place)." );
	lcgcd[0].gd.handle_controlevent = CI_DefLCChange;
	lcgcd[0].creator = GCheckBoxCreate;
	lchvarray[0][0] = &lcgcd[0];
	lchvarray[0][1] = lchvarray[0][2] = GCD_Glue; lchvarray[0][3] = NULL;

	lclabel[1].text = (unichar_t *) _("Ligature Caret Count:");
	lclabel[1].text_is_1byte = true;
	lclabel[1].text_in_resource = true;
	lcgcd[1].gd.label = &lclabel[1];
	lcgcd[1].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	lcgcd[1].gd.cid = CID_LCCountLab;
	lcgcd[1].gd.popup_msg = (unichar_t *) _("Ligature caret locations are used by a text editor\nwhen it needs to draw a text edit caret inside a\nligature. This means there should be a caret between\neach ligature component so if there are n components\nthere should be n-1 caret locations.\n  You may adjust the caret locations themselves in the\noutline glyph view (drag them from to origin to the\nappropriate place)." );
	lcgcd[1].creator = GLabelCreate;
	lchvarray[1][0] = &lcgcd[1];

	lcgcd[2].gd.pos.width = 50;
	lcgcd[2].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	lcgcd[2].gd.cid = CID_LCCount;
	lcgcd[2].gd.popup_msg = (unichar_t *) _("Ligature caret locations are used by a text editor\nwhen it needs to draw a text edit caret inside a\nligature. This means there should be a caret between\neach ligature component so if there are n components\nthere should be n-1 caret locations.\n  You may adjust the caret locations themselves in the\noutline glyph view (drag them from to origin to the\nappropriate place)." );
	lcgcd[2].creator = GNumericFieldCreate;
	lchvarray[1][1] = &lcgcd[2]; lchvarray[1][2] = GCD_Glue; lchvarray[1][3] = NULL;

	lchvarray[2][0] = lchvarray[2][1] = lchvarray[2][2] = GCD_Glue;
	lchvarray[2][3] = lchvarray[3][0] = NULL;

	lcbox[0].gd.flags = gg_enabled|gg_visible;
	lcbox[0].gd.u.boxelements = lchvarray[0];
	lcbox[0].creator = GHVBoxCreate;

	memset(&vargcd,0,sizeof(vargcd));
	memset(&varbox,0,sizeof(varbox));
	memset(&varlabel,0,sizeof(varlabel));

	for ( i=0; i<2; ++i ) {
	    varlabel[i][0].text = (unichar_t *) _("Variant Glyphs:");
	    varlabel[i][0].text_is_1byte = true;
	    vargcd[i][0].gd.label = &varlabel[i][0];
	    vargcd[i][0].gd.pos.x = 5; vargcd[i][0].gd.pos.y = 57+4; 
	    vargcd[i][0].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    vargcd[i][0].creator = GLabelCreate;
	    vargcd[i][0].gd.popup_msg = (unichar_t *) _("A list of the names of pre defined glyphs which represent\nbigger versions of the current glyph.");
	    varhvarray[i][0][0] = &vargcd[i][0];

	    vargcd[i][1].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    vargcd[i][1].gd.cid = CID_VariantList+i*100;
	    vargcd[i][1].creator = GTextCompletionCreate;
	    vargcd[i][1].gd.popup_msg = vargcd[i][0].gd.popup_msg;
	    varhvarray[i][0][1] = &vargcd[i][1]; varhvarray[i][0][2] = GCD_ColSpan; varhvarray[i][0][3] = NULL;

	    varlabel[i][2].text = (unichar_t *) _("Glyph Extension Components");
	    varlabel[i][2].text_is_1byte = true;
	    vargcd[i][2].gd.label = &varlabel[i][2];
	    vargcd[i][2].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    vargcd[i][2].creator = GLabelCreate;
	    vargcd[i][2].gd.popup_msg = (unichar_t *) _("A really big version of this glyph may be made up of the\nfollowing component glyphs. They will be stacked either\nhorizontally or vertically. Glyphs marked as Extenders may\nbe removed or repeated (to make shorter or longer versions).\nThe StartLength is the length of the flat section at the\nstart of the glyph which may be overlapped with the previous\nglyph, while the EndLength is the similar region at the end\nof the glyph. The FullLength is the full length of the glyph." );
	    varhvarray[i][1][0] = &vargcd[i][2];
	    varhvarray[i][1][1] = varhvarray[i][1][2] = GCD_ColSpan; varhvarray[i][1][3] = NULL;

/* GT: "Cor" is an abbreviation for correction */
	    varlabel[i][3].text = (unichar_t *) _("Italic Cor:");
	    varlabel[i][3].text_is_1byte = true;
	    vargcd[i][3].gd.label = &varlabel[i][3];
	    vargcd[i][3].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    vargcd[i][3].creator = GLabelCreate;
	    vargcd[i][3].gd.popup_msg = (unichar_t *) _("The italic correction of the composed glyph. Should be independent of glyph size");
	    varhvarray[i][2][0] = &vargcd[i][3];

	    vargcd[i][4].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    vargcd[i][4].gd.pos.width = 60;
	    vargcd[i][4].gd.cid = CID_ExtItalicCor+i*100;
	    vargcd[i][4].creator = GTextFieldCreate;
	    vargcd[i][4].gd.popup_msg = vargcd[i][3].gd.popup_msg;
	    varhvarray[i][2][1] = &vargcd[i][4];

	    vargcd[i][5].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    vargcd[i][5].gd.pos.width = 60;
	    vargcd[i][5].gd.cid = CID_ExtItalicDev+i*100;
	    vargcd[i][5].creator = GTextFieldCreate;
	    vargcd[i][5].gd.popup_msg = vargcd[i][3].gd.popup_msg;
	    varhvarray[i][2][2] = &vargcd[i][5];
	    varhvarray[i][2][3] = NULL;

	    vargcd[i][6].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    vargcd[i][6].gd.u.matrix = &mi_extensionpart;
	    vargcd[i][6].gd.cid = CID_ExtensionList+i*100;
	    vargcd[i][6].creator = GMatrixEditCreate;
	    varhvarray[i][3][0] = &vargcd[i][6];
	    varhvarray[i][3][1] = varhvarray[i][3][2] = GCD_ColSpan; varhvarray[i][3][3] = NULL;

	    varhvarray[i][4][0] = NULL;

	    varbox[i][0].gd.flags = gg_enabled|gg_visible;
	    varbox[i][0].gd.u.boxelements = varhvarray[i][0];
	    varbox[i][0].creator = GHVBoxCreate;
	}

	memset(&tilegcd,0,sizeof(tilegcd));
	memset(&tilebox,0,sizeof(tilebox));
	memset(&tilelabel,0,sizeof(tilelabel));

	i=0;
	tilelabel[i].text = (unichar_t *) _(
	    "If this glyph is used as a pattern to tile\n"
	    "some other glyph then it is useful to specify\n"
	    "the amount of whitespace surrounding the tile.\n"
	    "Either specify a margin to extend the bounding\n"
	    "box of the contents, or specify the bounds\n"
	    "explicitly.");
	tilelabel[i].text_is_1byte = true;
	tilelabel[i].text_in_resource = true;
	tilegcd[i].gd.label = &tilelabel[i];
	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i++].creator = GLabelCreate;
	tlvarray[0] = &tilegcd[i-1];

	tilelabel[i].text = (unichar_t *) _("Tile Margin:");
	tilelabel[i].text_is_1byte = true;
	tilelabel[i].text_in_resource = true;
	tilegcd[i].gd.label = &tilelabel[i];
	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i].gd.cid = CID_IsTileMargin;
	tilegcd[i++].creator = GRadioCreate;
	tlharray[0] = &tilegcd[i-1];

	tilegcd[i].gd.pos.width = 60;
	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i].gd.cid = CID_TileMargin;
	tilegcd[i].gd.handle_controlevent = CI_TileMarginChange;
	tilegcd[i++].creator = GTextFieldCreate;
	tlharray[1] = &tilegcd[i-1]; tlharray[2] = GCD_Glue; tlharray[3] = NULL;

	tilebox[2].gd.flags = gg_enabled|gg_visible;
	tilebox[2].gd.u.boxelements = tlharray;
	tilebox[2].creator = GHBoxCreate;
	tlvarray[1] = &tilebox[2];

	tilelabel[i].text = (unichar_t *) _("Tile Bounding Box:");
	tilelabel[i].text_is_1byte = true;
	tilelabel[i].text_in_resource = true;
	tilegcd[i].gd.label = &tilelabel[i];
	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i].gd.cid = CID_IsTileBBox;
	tilegcd[i++].creator = GRadioCreate;
	tlvarray[2] = &tilegcd[i-1];

	tlhvarray[0][0] = GCD_Glue;

	tilelabel[i].text = (unichar_t *) _("  X");
	tilelabel[i].text_is_1byte = true;
	tilegcd[i].gd.label = &tilelabel[i];
	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i++].creator = GLabelCreate;
	tlhvarray[0][1] = &tilegcd[i-1];

	tilelabel[i].text = (unichar_t *) _("  Y");
	tilelabel[i].text_is_1byte = true;
	tilegcd[i].gd.label = &tilelabel[i];
	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i++].creator = GLabelCreate;
	tlhvarray[0][2] = &tilegcd[i-1]; tlhvarray[0][3] = GCD_Glue; tlhvarray[0][4] = NULL;

	tilelabel[i].text = (unichar_t *) _("Min");
	tilelabel[i].text_is_1byte = true;
	tilegcd[i].gd.label = &tilelabel[i];
	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i++].creator = GLabelCreate;
	tlhvarray[1][0] = &tilegcd[i-1];

	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i].gd.cid = CID_TileBBoxMinX;
	tilegcd[i++].creator = GTextFieldCreate;
	tlhvarray[1][1] = &tilegcd[i-1];

	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i].gd.cid = CID_TileBBoxMinY;
	tilegcd[i++].creator = GTextFieldCreate;
	tlhvarray[1][2] = &tilegcd[i-1]; tlhvarray[1][3] = GCD_Glue; tlhvarray[1][4] = NULL;

	tilelabel[i].text = (unichar_t *) _("Max");
	tilelabel[i].text_is_1byte = true;
	tilegcd[i].gd.label = &tilelabel[i];
	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i++].creator = GLabelCreate;
	tlhvarray[2][0] = &tilegcd[i-1];

	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i].gd.cid = CID_TileBBoxMaxX;
	tilegcd[i++].creator = GTextFieldCreate;
	tlhvarray[2][1] = &tilegcd[i-1];

	tilegcd[i].gd.flags = gg_enabled|gg_visible;
	tilegcd[i].gd.cid = CID_TileBBoxMaxY;
	tilegcd[i++].creator = GTextFieldCreate;
	tlhvarray[2][2] = &tilegcd[i-1]; tlhvarray[2][3] = GCD_Glue; tlhvarray[2][4] = NULL;
	tlhvarray[3][0] = NULL;

	tilebox[3].gd.flags = gg_enabled|gg_visible;
	tilebox[3].gd.u.boxelements = tlhvarray[0];
	tilebox[3].creator = GHVBoxCreate;
	tlvarray[3] = &tilebox[3]; tlvarray[4] = GCD_Glue; tlvarray[5] = NULL;

	tilebox[0].gd.flags = gg_enabled|gg_visible;
	tilebox[0].gd.u.boxelements = tlvarray;
	tilebox[0].creator = GVBoxCreate;

	memset(&mgcd,0,sizeof(mgcd));
	memset(&mbox,0,sizeof(mbox));
	memset(&mlabel,0,sizeof(mlabel));
	memset(&aspects,'\0',sizeof(aspects));

	i = 0;
	aspects[i].text = (unichar_t *) _("Unicode");
	aspects[i].text_is_1byte = true;
	aspects[i].selected = true;
	aspects[i++].gcd = ubox;

	aspects[i].text = (unichar_t *) _("Comment");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = cbox;

	aspects[i].text = (unichar_t *) _("Positionings");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[pst_position-1];

	aspects[i].text = (unichar_t *) _("Pairwise Pos");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[pst_pair-1];

	aspects[i].text = (unichar_t *) _("Substitutions");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = psgcd[2];

	aspects[i].text = (unichar_t *) _("Alt Subs");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = psgcd[3];

	aspects[i].text = (unichar_t *) _("Mult Subs");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = psgcd[4];

	aspects[i].text = (unichar_t *) _("Ligatures");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = psgcd[5];

	aspects[i].text = (unichar_t *) _("Components");
	aspects[i].text_is_1byte = true;
	aspects[i].nesting = 1;
	aspects[i++].gcd = cobox;

	ci->lc_aspect = i;
	aspects[i].text = (unichar_t *) _("Lig. Carets");
	aspects[i].text_is_1byte = true;
	aspects[i].nesting = 1;
	aspects[i++].gcd = lcbox;

	aspects[i].text = (unichar_t *) _("Counters");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[6];

	aspects[i].text = (unichar_t *) U_("ΤεΧ & Math");	/* TeX */
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = tbox;

	ci->vert_aspect = i;
/* GT: "Vert." is an abbreviation for Vertical */
	aspects[i].text = (unichar_t *) U_("Vert. Variants");
	aspects[i].text_is_1byte = true;
	aspects[i].nesting = 1;
	aspects[i++].gcd = varbox[0];

/* GT: "Horiz." is an abbreviation for Horizontal */
	aspects[i].text = (unichar_t *) U_("Horiz. Variants");
	aspects[i].text_is_1byte = true;
	aspects[i].nesting = 1;
	aspects[i++].gcd = varbox[1];

	if ( sc->parent->multilayer ) {
	    aspects[i].text = (unichar_t *) U_("Tile Size");
	    aspects[i].text_is_1byte = true;
	    aspects[i++].gcd = tilebox;
	}

	if ( last_gi_aspect<i )
	    aspects[last_gi_aspect].selected = true;

	mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
	mgcd[0].gd.u.tabs = aspects;
	mgcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
	mgcd[0].gd.cid = CID_Tabs;
	mgcd[0].gd.handle_controlevent = CI_AspectChange;
	mgcd[0].creator = GTabSetCreate;
	mvarray[0] = &mgcd[0]; mvarray[1] = NULL;

	mgcd[1].gd.pos.x = 40; mgcd[1].gd.pos.y = mgcd[0].gd.pos.y+mgcd[0].gd.pos.height+3;
	mgcd[1].gd.flags = gg_visible | gg_enabled ;
	mlabel[1].text = (unichar_t *) _("< _Prev");
	mlabel[1].text_is_1byte = true;
	mlabel[1].text_in_resource = true;
	mgcd[1].gd.mnemonic = 'P';
	mgcd[1].gd.label = &mlabel[1];
	mgcd[1].gd.handle_controlevent = CI_NextPrev;
	mgcd[1].gd.cid = -1;
	mharray1[0] = GCD_Glue; mharray1[1] = &mgcd[1]; mharray1[2] = GCD_Glue;
	mgcd[1].creator = GButtonCreate;

	mgcd[2].gd.pos.x = -40; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y;
	mgcd[2].gd.flags = gg_visible | gg_enabled ;
	mlabel[2].text = (unichar_t *) _("_Next >");
	mlabel[2].text_is_1byte = true;
	mlabel[2].text_in_resource = true;
	mgcd[2].gd.label = &mlabel[2];
	mgcd[2].gd.mnemonic = 'N';
	mgcd[2].gd.handle_controlevent = CI_NextPrev;
	mgcd[2].gd.cid = 1;
	mharray1[3] = GCD_Glue; mharray1[4] = &mgcd[2]; mharray1[5] = GCD_Glue; mharray1[6] = NULL;
	mgcd[2].creator = GButtonCreate;

	mbox[2].gd.flags = gg_enabled|gg_visible;
	mbox[2].gd.u.boxelements = mharray1;
	mbox[2].creator = GHBoxCreate;
	mvarray[2] = &mbox[2]; mvarray[3] = NULL;

	mgcd[3].gd.pos.x = 25-3; mgcd[3].gd.pos.y = CI_Height-31-3;
	mgcd[3].gd.flags = gg_visible | gg_enabled | gg_but_default;
	mlabel[3].text = (unichar_t *) _("_OK");
	mlabel[3].text_is_1byte = true;
	mlabel[3].text_in_resource = true;
	mgcd[3].gd.mnemonic = 'O';
	mgcd[3].gd.label = &mlabel[3];
	mgcd[3].gd.handle_controlevent = CI_OK;
	mharray2[0] = GCD_Glue; mharray2[1] = &mgcd[3]; mharray2[2] = GCD_Glue; mharray2[3] = GCD_Glue;
	mgcd[3].creator = GButtonCreate;

	mgcd[4].gd.pos.x = -25; mgcd[4].gd.pos.y = mgcd[3].gd.pos.y+3;
	mgcd[4].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	mlabel[4].text = (unichar_t *) _("_Cancel");
	mlabel[4].text_is_1byte = true;
	mlabel[4].text_in_resource = true;
	mgcd[4].gd.label = &mlabel[4];
	mgcd[4].gd.handle_controlevent = CI_Cancel;
	mgcd[4].gd.cid = CID_Cancel;
	mharray2[4] = GCD_Glue; mharray2[5] = &mgcd[4]; mharray2[6] = GCD_Glue; mharray2[7] = NULL;
	mgcd[4].creator = GButtonCreate;

	mbox[3].gd.flags = gg_enabled|gg_visible;
	mbox[3].gd.u.boxelements = mharray2;
	mbox[3].creator = GHBoxCreate;
	mvarray[4] = &mbox[3]; mvarray[5] = NULL;
	mvarray[6] = NULL;

	mbox[0].gd.pos.x = mbox[0].gd.pos.y = 2;
	mbox[0].gd.flags = gg_enabled|gg_visible;
	mbox[0].gd.u.boxelements = mvarray;
	mbox[0].creator = GHVGroupCreate;

	GGadgetsCreate(ci->gw,mbox);

	GHVBoxSetExpandableRow(mbox[0].ret,0);
	GHVBoxSetExpandableCol(mbox[2].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(mbox[3].ret,gb_expandgluesame);

	GHVBoxSetExpandableRow(ubox[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(ubox[0].ret,1);
	GHVBoxSetExpandableCol(ubox[2].ret,gb_expandgluesame);

	GHVBoxSetExpandableRow(cbox[0].ret,1);
	GHVBoxSetExpandableCol(cbox[2].ret,gb_expandglue);

	for ( i=0; i<6; ++i ) {
	    GGadget *g = GWidgetGetControl(ci->gw,CID_List+i*100);
	    GMatrixEditSetNewText(g, newstrings[i]);
	    if ( i==pst_substitution-1 || i==pst_pair-1 )
		GMatrixEditSetColumnCompletion(g,1,CI_GlyphNameCompletion);
	    else if ( i==pst_alternate-1 || i==pst_multiple-1 ||
		    i==pst_ligature-1)
		GMatrixEditSetColumnCompletion(g,1,CI_GlyphListCompletion);
	}
	GHVBoxSetExpandableRow(pstbox[pst_pair-1][0].ret,0);
	for ( i=0; i<6; ++i )
	    GMatrixEditSetMouseMoveReporter(psgcd[i][0].ret,CI_SubsPopupPrepare);
	GMatrixEditSetMouseMoveReporter(psgcd[pst_pair-1][0].ret,CI_KerningPopupPrepare);
	for ( i=6; i<7; ++i ) {
	    GHVBoxSetExpandableRow(pstbox[i][0].ret,0);
	    GHVBoxSetExpandableCol(pstbox[i][2].ret,gb_expandgluesame);
	}

	GHVBoxSetExpandableRow(cobox[0].ret,gb_expandglue);
	GHVBoxSetExpandableRow(tbox[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(tbox[0].ret,gb_expandglue);
	GHVBoxSetPadding(tbox[0].ret,6,4);
	GHVBoxSetExpandableCol(tbox[2].ret,gb_expandglue);

	GHVBoxSetExpandableRow(lcbox[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(lcbox[0].ret,gb_expandglue);

	GHVBoxSetExpandableRow(varbox[0][0].ret,3);
	GHVBoxSetExpandableRow(varbox[1][0].ret,3);

	if ( sc->parent->multilayer ) {
	    GHVBoxSetExpandableRow(tilebox[0].ret,gb_expandglue);
	    GHVBoxSetExpandableCol(tilebox[2].ret,gb_expandglue);
	    GHVBoxSetExpandableCol(tilebox[3].ret,gb_expandglue);
	}

	GHVBoxFitWindow(mbox[0].ret);

	if ( font==NULL ) {
	    memset(&rq,0,sizeof(rq));
	    rq.utf8_family_name = MONO_UI_FAMILIES;
	    rq.point_size = 12;
	    rq.weight = 400;
	    font = GDrawInstanciateFont(ci->gw,&rq);
	    font = GResourceFindFont("GlyphInfo.Font",font);
	}
	for ( i=0; i<5; ++i )
	    GGadgetSetFont(psgcd[i][0].ret,font);
	for ( i=0; i<2; ++i ) {
	    GCompletionFieldSetCompletion(vargcd[i][1].ret,CI_GlyphNameCompletion);
	    GCompletionFieldSetCompletionMode(vargcd[i][1].ret,true);
	    GMatrixEditSetColumnCompletion(vargcd[i][6].ret,0,CI_GlyphNameCompletion);
	    GMatrixEditSetMouseMoveReporter(vargcd[i][6].ret,CI_ConstructionPopupPrepare);
	}

    CIFillup(ci);

    GWidgetHidePalettes();
    GDrawSetVisible(ci->gw,true);
}

void CharInfoDestroy(CharInfo *ci) {
    GDrawDestroyWindow(ci->gw);
}

struct sel_dlg {
    int done;
    int ok;
    FontView *fv;
};

int FVParseSelectByPST(FontView *fv,struct lookup_subtable *sub,
	int search_type) {
    int first;

    first = FVBParseSelectByPST((FontViewBase *) fv,sub,search_type);

    if ( first!=-1 )
	FVScrollToChar(fv,first);
    else if ( !no_windowing_ui )
	ff_post_notice(_("Select By ATT..."),_("No glyphs matched"));
    if (  !no_windowing_ui )
	GDrawRequestExpose(fv->v,NULL,false);
return( true );
}
    
static int SelectStuff(struct sel_dlg *sld,GWindow gw) {
    struct lookup_subtable *sub = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PST))->userdata;
    int search_type = GGadgetIsChecked(GWidgetGetControl(gw,CID_SelectResults)) ? 1 :
	    GGadgetIsChecked(GWidgetGetControl(gw,CID_MergeResults)) ? 2 :
		3;
return( FVParseSelectByPST(sld->fv, sub, search_type));
}

static int selpst_e_h(GWindow gw, GEvent *event) {
    struct sel_dlg *sld = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	sld->done = true;
	sld->ok = false;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("selectbyatt.html");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	sld->ok = GGadgetGetCid(event->u.control.g);
	if ( !sld->ok || SelectStuff(sld,gw))
	    sld->done = true;
    }
return( true );
}

void FVSelectByPST(FontView *fv) {
    struct sel_dlg sld;
    GWindow gw;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14];
    GTextInfo label[14];
    GGadgetCreateData *varray[20], *harray[8];
    int i,j,isgpos, cnt;
    OTLookup *otl;
    struct lookup_subtable *sub;
    GTextInfo *ti;
    SplineFont *sf = fv->b.sf;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    ti = NULL;
    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( isgpos=0; isgpos<2; ++isgpos ) {
	    for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups ; otl!=NULL; otl=otl->next ) {
		if ( otl->lookup_type== gsub_single ||
			otl->lookup_type== gsub_multiple ||
			otl->lookup_type== gsub_alternate ||
			otl->lookup_type== gsub_ligature ||
			otl->lookup_type== gpos_single ||
			otl->lookup_type== gpos_pair ||
			otl->lookup_type== gpos_cursive ||
			otl->lookup_type== gpos_mark2base ||
			otl->lookup_type== gpos_mark2ligature ||
			otl->lookup_type== gpos_mark2mark )
		    for ( sub=otl->subtables; sub!=NULL; sub=sub->next )
			if ( sub->kc==NULL ) {
			    if ( ti!=NULL ) {
				ti[cnt].text = (unichar_t *) copy(sub->subtable_name);
			        ti[cnt].text_is_1byte = true;
			        ti[cnt].userdata = sub;
			        ti[cnt].selected = cnt==0;
			    }
			    ++cnt;
			}
	    }
	}
	if ( cnt==0 ) {
	    ff_post_notice(_("No Lookups"), _("No applicable lookup subtables"));
return;
	}
	if ( ti==NULL )
	    ti = calloc(cnt+1,sizeof(GTextInfo));
    }

    CharInfoInit();

    memset(&sld,0,sizeof(sld));
    sld.fv = fv;
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Select By Lookup Subtable");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,160));
	pos.height = GDrawPointsToPixels(NULL,204);
	gw = GDrawCreateTopWindow(NULL,&pos,selpst_e_h,&sld,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	i=j=0;

	label[i].text = (unichar_t *) _("Select Glyphs in lookup subtable");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;

	gcd[i].gd.label = &ti[0];
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 5+4;
	gcd[i].gd.flags = gg_enabled|gg_visible/*|gg_list_exactlyone*/;
	gcd[i].gd.u.list = ti;
	gcd[i].gd.cid = CID_PST;
	gcd[i++].creator = GListButtonCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;
	varray[j++] = GCD_Glue; varray[j++] = NULL;

	label[i].text = (unichar_t *) _("Select Results");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26; 
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_cb_on|gg_utf8_popup;
	gcd[i].gd.popup_msg = (unichar_t *) _("Set the selection of the font view to the glyphs\nfound by this search");
	gcd[i].gd.cid = CID_SelectResults;
	gcd[i++].creator = GRadioCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;

	label[i].text = (unichar_t *) _("Merge Results");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15; 
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[i].gd.popup_msg = (unichar_t *) _("Expand the selection of the font view to include\nall the glyphs found by this search");
	gcd[i].gd.cid = CID_MergeResults;
	gcd[i++].creator = GRadioCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;

	label[i].text = (unichar_t *) _("Restrict Selection");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15; 
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[i].gd.popup_msg = (unichar_t *) _("Only search the selected glyphs, and unselect\nany characters which do not match this search");
	gcd[i].gd.cid = CID_RestrictSelection;
	gcd[i++].creator = GRadioCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;
	varray[j++] = GCD_Glue; varray[j++] = NULL;

	gcd[i].gd.pos.x = 15-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+22;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.mnemonic = 'O';
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = true;
	gcd[i++].creator = GButtonCreate;
	harray[0] = GCD_Glue; harray[1] = &gcd[i-1]; harray[2] = GCD_Glue;

	gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.mnemonic = 'C';
	gcd[i].gd.cid = false;
	gcd[i++].creator = GButtonCreate;
	harray[3] = GCD_Glue; harray[4] = &gcd[i-1]; harray[5] = GCD_Glue;
	harray[6] = NULL;

	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.boxelements = harray;
	gcd[i].creator = GHBoxCreate;
	varray[j++] = &gcd[i++]; varray[j++] = NULL; varray[j++] = NULL;

	gcd[i].gd.pos.x = gcd[i].gd.pos.y = 2;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.boxelements = varray;
	gcd[i].creator = GHVGroupCreate;

	GGadgetsCreate(gw,gcd+i);
	GTextInfoListFree(ti);
	GHVBoxSetExpandableRow(gcd[i].ret,gb_expandglue);
	GHVBoxSetExpandableCol(gcd[i-1].ret,gb_expandgluesame);
	GHVBoxFitWindow(gcd[i].ret);

    GDrawSetVisible(gw,true);
    while ( !sld.done )
	GDrawProcessOneEvent(NULL);
    if ( sld.ok ) {
    }
    GDrawDestroyWindow(gw);
}

void CharInfoInit(void) {
    static GTextInfo *lists[] = { glyphclasses, std_colors, truefalse, NULL };
    static int done = 0;
    int i, j;
    static char **cnames[] = { newstrings, NULL };
    static struct col_init *col_inits[] = { extensionpart, altuniinfo,
	devtabci,
	simplesubsci, ligatureci, altsubsci, multsubsci, simpleposci,
	pairposci, NULL };

    if ( done )
return;
    done = true;
    for ( i=0; lists[i]!=NULL; ++i )
	for ( j=0; lists[i][j].text!=NULL; ++j )
	    lists[i][j].text = (unichar_t *) S_((char *) lists[i][j].text);
    for ( i=0; cnames[i]!=NULL; ++i )
	for ( j=0; cnames[i][j]!=NULL; ++j )
	    cnames[i][j] = _(cnames[i][j]);
    for ( i=0; col_inits[i]!=NULL; ++i )
	for ( j=0; col_inits[i][j].title!=NULL; ++j )
	    col_inits[i][j].title=_(col_inits[i][j].title);
}
