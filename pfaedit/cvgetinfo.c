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
#include <ustring.h>
#include <math.h>
#include <utype.h>
#include <gkeysym.h>

#define TCnt	3

typedef struct gidata {
    CharView *cv;
    SplineChar *sc;
    RefChar *rf;
    ImageList *img;
    AnchorPoint *ap;
    SplinePoint *cursp;
    SplinePointList *curspl;
    SplinePointList *oldstate;
    AnchorPoint *oldaps;
    GWindow gw;
    int done, first, changed;
} GIData;

#define CID_BaseX	2001
#define CID_BaseY	2002
#define CID_NextXOff	2003
#define CID_NextYOff	2004
#define CID_NextPos	2005
#define CID_PrevXOff	2006
#define CID_PrevYOff	2007
#define CID_PrevPos	2008
#define CID_NextDef	2009
#define CID_PrevDef	2010

#define CID_X		3001
#define CID_Y		3002
#define CID_NameList	3003
#define CID_Mark	3004
#define CID_BaseChar	3005
#define CID_BaseLig	3006
#define CID_BaseMark	3007
#define CID_CursEntry	3008
#define CID_CursExit	3009
#define CID_LigIndex	3010
#define CID_Next	3011
#define CID_Prev	3012
#define CID_Delete	3013
#define CID_New		3014

#define RI_Width	215
#define RI_Height	180

#define II_Width	130
#define II_Height	70

#define PI_Width	218
#define PI_Height	200

#define AI_Width	160
#define AI_Height	234

static int GI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
    }
return( true );
}

static int GI_TransChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->changed = true;
    }
return( true );
}

static int GI_ROK_Do(GIData *ci) {
    int errs=false,i;
    real trans[6];
    SplinePointList *spl, *new;
    RefChar *ref = ci->rf, *subref;

    for ( i=0; i<6; ++i ) {
	trans[i] = GetRealR(ci->gw,1000+i,_STR_TransformationMatrix,&errs);
	if ( !errs &&
		((i<4 && (trans[i]>30 || trans[i]<-30)) ||
		 (i>=4 && (trans[i]>16000 || trans[i]<-16000))) ) {
	    /* Don't want the user to insert an enormous scale factor or */
	    /*  it will move points outside the legal range. */
	    GTextFieldSelect(GWidgetGetControl(ci->gw,1000+i),0,-1);
	    GWidgetErrorR(_STR_OutOfRange,_STR_OutOfRange);
	    errs = true;
	}
	if ( errs )
return( false );
    }

    for ( i=0; i<6 && ref->transform[i]==trans[i]; ++i );
    if ( i==6 )		/* Didn't really change */
return( true );

    for ( i=0; i<6; ++i )
	ref->transform[i] = trans[i];
    SplinePointListFree(ref->splines);
    ref->splines = SplinePointListTransform(SplinePointListCopy(ref->sc->splines),trans,true);
    spl = NULL;
    if ( ref->splines!=NULL )
	for ( spl = ref->splines; spl->next!=NULL; spl = spl->next );
    for ( subref = ref->sc->refs; subref!=NULL; subref=subref->next ) {
	new = SplinePointListTransform(SplinePointListCopy(subref->splines),trans,true);
	if ( spl==NULL )
	    ref->splines = new;
	else
	    spl->next = new;
	if ( new!=NULL )
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
    }

    SplineSetFindBounds(ref->splines,&ref->bb);
    CVCharChangedUpdate(ci->cv);
return( true );
}

static int GI_ROK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GI_ROK_Do(ci))
	    ci->done = true;
    }
return( true );
}

static int GI_Show(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( ci->changed ) {
	    static int buts[] = { _STR_Change, _STR_Retain, _STR_Cancel, 0 };
	    int ans = GWidgetAskR(_STR_TransformChanged,buts,0,2,_STR_TransformChangedApply);
	    if ( ans==2 )
return( true );
	    else if ( ans==0 ) {
		if ( !GI_ROK_Do(ci))
return( true );
	    }
	}
	ci->done = true;
	CharViewCreate(ci->rf->sc,ci->cv->fv);
    }
return( true );
}

static int gi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	GIData *ci = GDrawGetUserData(gw);
	ci->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("getinfo.html");
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static void RefGetInfo(CharView *cv, RefChar *ref) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[16];
    GTextInfo label[16];
    char namebuf[100], tbuf[6][40];
    char ubuf[40];
    int i,j;

    gi.cv = cv;
    gi.sc = cv->sc;
    gi.rf = ref;
    gi.changed = false;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_ReferenceInfo,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,RI_Width));
	pos.height = GDrawPointsToPixels(NULL,
		ref->sc->unicodeenc!=-1?RI_Height+12:RI_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,gi_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	sprintf( namebuf, "Reference to character %.20s at %d", ref->sc->name, ref->sc->enc);
	label[0].text = (unichar_t *) namebuf;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;
	j = 1;

	if ( ref->sc->unicodeenc!=-1 ) {
	    sprintf( ubuf, " Unicode: U+%04x", ref->sc->unicodeenc );
	    label[1].text = (unichar_t *) ubuf;
	    label[1].text_is_1byte = true;
	    gcd[1].gd.label = &label[1];
	    gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 17;
	    gcd[1].gd.flags = gg_enabled|gg_visible;
	    gcd[1].creator = GLabelCreate;
	    j=2;
	}

	label[j].text = (unichar_t *) _STR_TransformedBy;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+14;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.popup_msg = GStringGetResource(_STR_TransformPopup,NULL);
	gcd[j].creator = GLabelCreate;
	++j;

	for ( i=0; i<6; ++i ) {
	    sprintf(tbuf[i],"%g", ref->transform[i]);
	    label[i+j].text = (unichar_t *) tbuf[i];
	    label[i+j].text_is_1byte = true;
	    gcd[i+j].gd.label = &label[i+j];
	    gcd[i+j].gd.pos.x = 20+((i&1)?85:0); gcd[i+j].gd.pos.width=75;
	    gcd[i+j].gd.pos.y = gcd[j-1].gd.pos.y+14+(i/2)*26; 
	    gcd[i+j].gd.flags = gg_enabled|gg_visible;
	    gcd[i+j].gd.cid = i+1000;
	    gcd[i+j].gd.handle_controlevent = GI_TransChange;
	    gcd[i+j].creator = GTextFieldCreate;
	}

	gcd[6+j].gd.pos.x = (RI_Width-GIntGetResource(_NUM_Buttonsize))/2;
	gcd[6+j].gd.pos.y = RI_Height+(j==3?12:0)-64;
	gcd[6+j].gd.pos.width = -1; gcd[6+j].gd.pos.height = 0;
	gcd[6+j].gd.flags = gg_visible | gg_enabled ;
	label[6+j].text = (unichar_t *) _STR_Show;
	label[6+j].text_in_resource = true;
	gcd[6+j].gd.mnemonic = 'S';
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.handle_controlevent = GI_Show;
	gcd[6+j].creator = GButtonCreate;

	gcd[7+j].gd.pos.x = 30-3; gcd[7+j].gd.pos.y = RI_Height+(j==3?12:0)-32-3;
	gcd[7+j].gd.pos.width = -1; gcd[7+j].gd.pos.height = 0;
	gcd[7+j].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[7+j].text = (unichar_t *) _STR_OK;
	label[7+j].text_in_resource = true;
	gcd[7+j].gd.mnemonic = 'O';
	gcd[7+j].gd.label = &label[7+j];
	gcd[7+j].gd.handle_controlevent = GI_ROK;
	gcd[7+j].creator = GButtonCreate;

	gcd[8+j].gd.pos.x = -30; gcd[8+j].gd.pos.y = gcd[7+j].gd.pos.y+3;
	gcd[8+j].gd.pos.width = -1; gcd[8+j].gd.pos.height = 0;
	gcd[8+j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[8+j].text = (unichar_t *) _STR_Cancel;
	label[8+j].text_in_resource = true;
	gcd[8+j].gd.mnemonic = 'C';
	gcd[8+j].gd.label = &label[8+j];
	gcd[8+j].gd.handle_controlevent = GI_Cancel;
	gcd[8+j].creator = GButtonCreate;

	gcd[9+j].gd.pos.x = 5; gcd[9+j].gd.pos.y = gcd[6+j].gd.pos.y-6;
	gcd[9+j].gd.pos.width = RI_Width-10;
	gcd[9+j].gd.flags = gg_visible | gg_enabled;
	gcd[9+j].creator = GLineCreate;

	gcd[10+j] = gcd[9+j];
	gcd[9+j].gd.pos.y = gcd[7+j].gd.pos.y-3;

	GGadgetsCreate(gi.gw,gcd);
	GWidgetIndicateFocusGadget(gcd[j].ret);

    GWidgetHidePalettes();
    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
}

static void ImgGetInfo(CharView *cv, ImageList *img) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12];
    GTextInfo label[12];
    char posbuf[100], scalebuf[100];

    gi.cv = cv;
    gi.sc = cv->sc;
    gi.img = img;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_ImageInfo,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,II_Width));
	pos.height = GDrawPointsToPixels(NULL,II_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,gi_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	sprintf( posbuf, "Image at: (%.0f,%.0f)", img->xoff,
		img->yoff-GImageGetHeight(img->image)*img->yscale);
	label[0].text = (unichar_t *) posbuf;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	sprintf( scalebuf, "Scaled by: (%.2f,%.2f)", img->xscale, img->yscale );
	label[1].text = (unichar_t *) scalebuf;
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 19; 
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].creator = GLabelCreate;

	gcd[2].gd.pos.x = (II_Width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)-6)/2; gcd[2].gd.pos.y = II_Height-32-3;
	gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
	gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_but_cancel;
	label[2].text = (unichar_t *) _STR_OK;
	label[2].text_in_resource = true;
	gcd[2].gd.mnemonic = 'O';
	gcd[2].gd.label = &label[2];
	gcd[2].gd.handle_controlevent = GI_Cancel;
	gcd[2].creator = GButtonCreate;

	GGadgetsCreate(gi.gw,gcd);

    GWidgetHidePalettes();
    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
}

static int IsAnchorClassUsed(SplineChar *sc,AnchorClass *an) {
    AnchorPoint *ap;
    int waslig=0, sawentry=0, sawexit=0;

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->anchor==an ) {
	    if ( ap->type==at_centry )
		sawentry = true;
	    else if ( ap->type==at_cexit )
		sawexit = true;
	    else if ( ap->type!=at_baselig )
return( -1 );
	    else if ( waslig<ap->lig_index+1 )
		waslig = ap->lig_index+1;
	}
    }
    if ( sawentry && sawexit )
return( -1 );
    else if ( sawentry )
return( -2 );
    else if ( sawexit )
return( -3 );
return( waslig );
}

AnchorClass *AnchorClassUnused(SplineChar *sc,int *waslig) {
    AnchorClass *an;
    int val;
    /* Are there any anchors with this name? If so can't reuse it */
    /*  unless they are ligature anchores */
    /*  or 'curs' anchors, which allow exactly two points (entry, exit) */

    *waslig = false;
    for ( an=sc->parent->anchor; an!=NULL; an=an->next ) {
	val = IsAnchorClassUsed(sc,an);
	if ( val!=-1 ) {
	    *waslig = val;
return( an );
	}
    }
return( NULL );
}

static AnchorPoint *AnchorPointNew(CharView *cv) {
    AnchorClass *an;
    AnchorPoint *ap;
    int waslig;
    SplineChar *sc = cv->sc;
    PST *pst;

    an = AnchorClassUnused(sc,&waslig);
    if ( an==NULL )
return(NULL);
    ap = chunkalloc(sizeof(AnchorPoint));
    ap->anchor = an;
    ap->me.x = cv->p.cx; /* cv->p.cx = 0; */
    ap->me.y = cv->p.cy; /* cv->p.cy = 0; */
    ap->type = at_basechar;
    for ( pst = cv->sc->possub; pst!=NULL && pst->type!=pst_ligature; pst=pst->next );
    if ( waslig==-2 )
	ap->type = at_cexit;
    else if ( waslig==-3 || an->feature_tag==CHR('c','u','r','s'))
	ap->type = at_centry;
    else if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
	    iscombining(sc->unicodeenc))
	ap->type = at_mark;
    else if ( an->feature_tag==CHR('m','k','m','k') )
	ap->type = at_basemark;
    else if ( pst!=NULL || waslig )
	ap->type = at_baselig;
    if (( ap->type==at_basechar || ap->type==at_baselig ) && an->feature_tag==CHR('m','k','m','k') )
	ap->type = at_basemark;
    ap->next = sc->anchor;
    ap->lig_index = waslig;
    sc->anchor = ap;
return( ap );
}

void SCOrderAP(SplineChar *sc) {
    int lc=0, cnt=0, out=false, i,j;
    AnchorPoint *ap, **array;
    /* Order so that first ligature index comes first */

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->lig_index<lc ) out = true;
	if ( ap->lig_index>lc ) lc = ap->lig_index;
	++cnt;
    }
    if ( !out )
return;

    array = galloc(cnt*sizeof(AnchorPoint *));
    for ( i=0, ap=sc->anchor; ap!=NULL; ++i, ap=ap->next )
	array[i] = ap;
    for ( i=0; i<cnt-1; ++i ) {
	for ( j=i+1; j<cnt; ++j ) {
	    if ( array[i]->lig_index>array[j]->lig_index ) {
		ap = array[i];
		array[i] = array[j];
		array[j] = ap;
	    }
	}
    }
    sc->anchor = array[0];
    for ( i=0; i<cnt-1; ++i )
	array[i]->next = array[i+1];
    array[cnt-1]->next = NULL;
    free( array );
}

static void AI_SelectList(GIData *ci,AnchorPoint *ap) {
    int i;
    AnchorClass *an;

    for ( i=0, an=ci->sc->parent->anchor; an!=ap->anchor; ++i, an=an->next );
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_NameList),i);
}

static void AI_DisplayClass(GIData *ci,AnchorPoint *ap) {
    AnchorClass *ac = ap->anchor;
    int curs = ac->feature_tag==CHR('c','u','r','s');
    int enable_mkmk = ac->feature_tag!=CHR('m','a','r','k') && ac->feature_tag!=CHR('a','b','v','m') &&
	    ac->feature_tag!=CHR('b','l','w','m') && !curs;
    int enable_mark = ac->feature_tag!=CHR('m','k','m','k');
    AnchorPoint *aps;
    int saw[at_max];

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseChar),enable_mark && !curs);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseLig),enable_mark && !curs);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseMark),enable_mkmk && !curs);

    if ( !enable_mark && !curs && (ap->type==at_basechar || ap->type==at_baselig)) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseMark),true);
	ap->type = at_basemark;
    } else if ( !enable_mkmk && !curs && ap->type==at_basemark ) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseChar),true);
	ap->type = at_basechar;
    }

    memset(saw,0,sizeof(saw));
    for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next ) if ( aps!=ap ) {
	if ( aps->anchor==ac ) saw[aps->type] = true;
    }
    if ( curs ) {
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursEntry),!saw[at_centry]);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursExit),!saw[at_cexit]);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Mark),false);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursEntry),false);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursExit),false);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Mark),!saw[at_mark]);
	if ( saw[at_basechar]) GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseChar),false);
	if ( saw[at_basemark]) GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseMark),false);
    }
}

static void AI_Display(GIData *ci,AnchorPoint *ap) {
    char val[40];
    unichar_t uval[40];
    AnchorPoint *aps;

    ci->ap = ap;
    for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next )
	aps->selected = false;
    ap->selected = true;
    sprintf(val,"%g",ap->me.x);
    uc_strcpy(uval,val);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_X),uval);
    sprintf(val,"%g",ap->me.y);
    uc_strcpy(uval,val);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Y),uval);
    sprintf(val,"%d",ap->type==at_baselig?ap->lig_index:0);
    uc_strcpy(uval,val);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_LigIndex),uval);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LigIndex),ap->type==at_baselig);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Next),ap->next!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Prev),ci->sc->anchor!=ap);

    AI_DisplayClass(ci,ap);

    switch ( ap->type ) {
      case at_mark:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Mark),true);
      break;
      case at_basechar:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseChar),true);
      break;
      case at_baselig:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseLig),true);
      break;
      case at_basemark:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseMark),true);
      break;
      case at_centry:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_CursEntry),true);
      break;
      case at_cexit:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_CursExit),true);
      break;
    }

    AI_SelectList(ci,ap);
    SCUpdateAll(ci->sc);
}

static int AI_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));

	if ( ci->ap->next==NULL )
return( true );
	AI_Display(ci,ci->ap->next);
    }
return( true );
}

static int AI_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap, *prev;

	prev = NULL;
	for ( ap=ci->sc->anchor; ap!=ci->ap; ap = ap->next )
	    prev = ap;
	if ( prev==NULL )
return( true );
	AI_Display(ci,prev);
    }
return( true );
}

static int AI_Ok(GGadget *g, GEvent *e);
static int AI_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap, *prev;

	prev=NULL;
	for ( ap=ci->sc->anchor; ap!=ci->ap; ap=ap->next )
	    prev = ap;
	if ( prev==NULL && ci->ap->next==NULL ) {
	    static int buts[] = { _STR_Yes, _STR_No, 0 };
	    if ( GWidgetAskR(_STR_LastAnchor,buts,0,1,_STR_RemoveLastAnchor)==1 ) {
		AI_Ok(g,e);
return( true );
	    }
	}
	ap = ci->ap->next;
	if ( prev==NULL )
	    ci->sc->anchor = ap;
	else
	    prev->next = ap;
	chunkfree(ci->ap,sizeof(AnchorPoint));
	AI_Display(ci,ap);
    }
return( true );
}

static int AI_New(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int waslig;
	AnchorPoint *ap;

	if ( AnchorClassUnused(ci->sc,&waslig)==NULL ) {
	    GWidgetPostNoticeR(_STR_MakeNewClass,_STR_MakeNewAnchorClass);
	    FontInfo(ci->sc->parent,8,true);		/* Anchor Class */
	    if ( AnchorClassUnused(ci->sc,&waslig)==NULL )
return(true);
	    GGadgetSetList(GWidgetGetControl(ci->gw,CID_NameList),
		    AnchorClassesLList(ci->sc->parent),false);
	}
	ap = AnchorPointNew(ci->cv);
	if ( ap==NULL )
return( true );
	AI_Display(ci,ap);
    }
return( true );
}

static int AI_TypeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap = ci->ap;

	if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_Mark)) )
	    ap->type = at_mark;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_BaseChar)) )
	    ap->type = at_basechar;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_BaseLig)) )
	    ap->type = at_baselig;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_BaseMark)) )
	    ap->type = at_basemark;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_CursEntry)) )
	    ap->type = at_centry;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_CursExit)) )
	    ap->type = at_cexit;
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LigIndex),ap->type==at_baselig);
    }
return( true );
}

static void AI_TestOrdering(GIData *ci,real x) {
    AnchorPoint *aps, *ap=ci->ap;
    AnchorClass *ac = ap->anchor;
    int isr2l;

    isr2l = SCRightToLeft(ci->sc);
    for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next ) {
	if ( aps->anchor==ac && aps!=ci->ap ) {
	    if (( aps->lig_index<ap->lig_index &&
		    ((!isr2l && aps->me.x>x) ||
		     ( isr2l && aps->me.x<x))) ||
		( aps->lig_index>ap->lig_index &&
		    (( isr2l && aps->me.x>x) ||
		     (!isr2l && aps->me.x<x))) ) {
		GWidgetErrorR(_STR_OutOfOrder,_STR_IndexOutOfOrder,aps->lig_index);
return;
	    }
	}
    }
return;
}

static int AI_LigIndexChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int index, max;
	int err=false;
	AnchorPoint *ap = ci->ap, *aps;

	index = GetCalmRealR(ci->gw,CID_LigIndex,_STR_LigIndex,&err);
	if ( index<0 || err )
return( true );
	if ( *_GGadgetGetTitle(g)=='\0' )
return( true );
	max = 0;
	AI_TestOrdering(ci,ap->me.x);
	for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next ) {
	    if ( aps->anchor==ap->anchor && aps!=ap ) {
		if ( aps->lig_index==index ) {
		    GWidgetErrorR(_STR_IndexInUse,_STR_LigIndexInUse);
return( true );
		} else if ( aps->lig_index>max )
		    max = aps->lig_index;
	    }
	}
	if ( index>max+10 ) {
	    char buf[20]; unichar_t ubuf[20];
	    GWidgetErrorR(_STR_TooBig,_STR_IndexTooBig);
	    sprintf(buf,"%d", max+1);
	    uc_strcpy(ubuf,buf);
	    GGadgetSetTitle(g,ubuf);
	    index = max+1;
	}
	ap->lig_index = index;
    }
return( true );
}

static int AI_ANameChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap;
	GTextInfo *ti = GGadgetGetListItemSelected(g);
	AnchorClass *an = ti->userdata;
	int bad=0, max=0;

	if ( an==ci->ap->anchor )
return( true );			/* No op */
	if ( an->feature_tag==CHR('c','u','r','s') || ci->ap->anchor->feature_tag==CHR('c','u','r','s')) {
	    GWidgetErrorR(_STR_BadClass,_STR_AnchorClassCurs);
return( true );
	}
	for ( ap=ci->sc->anchor; ap!=NULL; ap = ap->next ) {
	    if ( ap!=ci->ap && ap->anchor==an ) {
		if ( ap->type!=at_baselig || ci->ap->type!=at_baselig )
	break;
		if ( ap->lig_index==ci->ap->lig_index )
		    bad = true;
		else if ( ap->lig_index>max )
		    max = ap->lig_index;
	    }
	}
	if ( ap!=NULL ) {
	    AI_SelectList(ci,ap);
	    GWidgetErrorR(_STR_ClassUsed,_STR_AnchorClassUsed);
	} else {
	    ci->ap->anchor = an;
	    if ( ci->ap->type!=at_baselig )
		ci->ap->lig_index = 0;
	    else if ( bad )
		ci->ap->lig_index = max+1;
	    AI_DisplayClass(ci,ci->ap);
	    AI_TestOrdering(ci,ci->ap->me.x);
	}
    }
return( true );
}

static int AI_PosChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	AnchorPoint *ap = ci->ap;

	if ( GGadgetGetCid(g)==CID_X ) {
	    dx = GetCalmRealR(ci->gw,CID_X,_STR_X,&err)-ap->me.x;
	    AI_TestOrdering(ci,ap->me.x+dx);
	} else
	    dy = GetCalmRealR(ci->gw,CID_Y,_STR_Y,&err)-ap->me.y;
	if ( (dx==0 && dy==0) || err )
return( true );
	ap->me.x += dx;
	ap->me.y += dy;
	CVCharChangedUpdate(ci->cv);
    }
return( true );
}

static void AI_DoCancel(GIData *ci) {
    CharView *cv = ci->cv;
    ci->done = true;
    AnchorPointsFree(cv->sc->anchor);
    cv->sc->anchor = ci->oldaps;
    ci->oldaps = NULL;
    CVRemoveTopUndo(cv);
    SCUpdateAll(cv->sc);
}

static int ai_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	AI_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("getinfo.html");
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static int AI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	AI_DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int AI_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
	/* All the work has been done as we've gone along */
	/* Well, we should reorder the list... */
	SCOrderAP(ci->cv->sc);
    }
return( true );
}

void ApGetInfo(CharView *cv, AnchorPoint *ap) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[22];
    GTextInfo label[22];
    int j;

    memset(&gi,0,sizeof(gi));
    gi.cv = cv;
    gi.sc = cv->sc;
    gi.oldaps = AnchorPointsCopy(cv->sc->anchor);
    CVPreserveState(cv);
    if ( ap==NULL ) {
	ap = AnchorPointNew(cv);
	if ( ap==NULL )
return;
    }
	
    gi.ap = ap;
    gi.changed = false;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_AnchorPointInfo,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,AI_Width));
	pos.height = GDrawPointsToPixels(NULL,AI_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,ai_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	j=0;
	label[j].text = ap->anchor->name;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = 5; 
	gcd[j].gd.pos.width = AI_Width-10; 
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NameList;
	gcd[j].gd.u.list = AnchorClassesList(cv->sc->parent);
	gcd[j].gd.handle_controlevent = AI_ANameChanged;
	gcd[j].creator = GListButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_XC;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+34;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 23; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_X;
	gcd[j].gd.handle_controlevent = AI_PosChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_YC;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 85; gcd[j].gd.pos.y = gcd[j-2].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 103; gcd[j].gd.pos.y = gcd[j-2].gd.pos.y;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Y;
	gcd[j].gd.handle_controlevent = AI_PosChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Mark;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Mark;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_BaseChar;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 70; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseChar;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_BaseLig;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+14;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseLig;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_BaseMark;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseMark;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_CursiveEntry;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+14;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_CursEntry;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_CursiveExit;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_CursExit;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_LigIndex;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+26;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 65; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_LigIndex;
	gcd[j].gd.handle_controlevent = AI_LigIndexChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_New;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+30;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_New;
	gcd[j].gd.handle_controlevent = AI_New;
	gcd[j].creator = GButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Delete;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = -5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Delete;
	gcd[j].gd.handle_controlevent = AI_Delete;
	gcd[j].creator = GButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_PrevArrow;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Prev;
	gcd[j].gd.handle_controlevent = AI_Prev;
	gcd[j].creator = GButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_NextArrow;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = -15; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Next;
	gcd[j].gd.handle_controlevent = AI_Next;
	gcd[j].creator = GButtonCreate;
	++j;

	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+26;
	gcd[j].gd.pos.width = AI_Width-10;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLineCreate;
	++j;

	label[j].text = (unichar_t *) _STR_OK;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+8;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.handle_controlevent = AI_Ok;
	gcd[j].creator = GButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Cancel;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = -5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+3;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.handle_controlevent = AI_Cancel;
	gcd[j].creator = GButtonCreate;
	++j;

	gcd[j].gd.pos.x = 2; gcd[j].gd.pos.y = 2;
	gcd[j].gd.pos.width = pos.width-4; gcd[j].gd.pos.height = pos.height-4;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
	gcd[j].creator = GLineCreate;
	++j;

	GGadgetsCreate(gi.gw,gcd);
	AI_Display(&gi,ap);
	GWidgetIndicateFocusGadget(GWidgetGetControl(gi.gw,CID_X));

    GWidgetHidePalettes();
    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
    AnchorPointsFree(gi.oldaps);
}

void MDReplace(MinimumDistance *md,SplineSet *old,SplineSet *rpl) {
    /* Replace all the old points with the ones in rpl in the minimu distance hints */
    SplinePoint *osp, *rsp;
    MinimumDistance *test;

    while ( old!=NULL && rpl!=NULL ) {
	osp = old->first; rsp = rpl->first;
	while ( 1 ) {
	    for ( test=md; test!=NULL ; test=test->next ) {
		if ( test->sp1==osp )
		    test->sp1 = rsp;
		if ( test->sp2==osp )
		    test->sp2 = rsp;
	    }
	    if ( osp->next==NULL )
	break;
	    osp = osp->next->to;
	    rsp = rsp->next->to;
	    if ( osp==old->first )
	break;
	}
	old = old->next;
	rpl = rpl->next;
    }
}

static void PI_DoCancel(GIData *ci) {
    CharView *cv = ci->cv;
    ci->done = true;
    if ( cv->drawmode==dm_fore )
	MDReplace(cv->sc->md,cv->sc->splines,ci->oldstate);
    SplinePointListsFree(*cv->heads[cv->drawmode]);
    *cv->heads[cv->drawmode] = ci->oldstate;
    CVRemoveTopUndo(cv);
    SCClearSelPt(cv->sc);
    SCUpdateAll(cv->sc);
}

static int pi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	PI_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("getinfo.html");
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static int PI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PI_DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int PI_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
	/* All the work has been done as we've gone along */
    }
return( true );
}

static void mysprintf( char *buffer, real v) {
    char *pt;

    sprintf( buffer, "%.2f", v );
    pt = strrchr(buffer,'.');
    if ( pt[1]=='0' && pt[2]=='0' )
	*pt='\0';
    else if ( pt[2]=='0' )
	pt[2] = '\0';
}

static void mysprintf2( char *buffer, real v1, real v2) {
    char *pt;

    mysprintf(buffer,v1);
    pt = buffer+strlen(buffer);
    *pt++ = ',';
    mysprintf(pt,v2);
}

static void PIFillup(GIData *ci, int except_cid) {
    char buffer[50];
    unichar_t ubuf[50];

    mysprintf(buffer, ci->cursp->me.x );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_BaseX )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_BaseX),ubuf);

    mysprintf(buffer, ci->cursp->me.y );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_BaseY )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_BaseY),ubuf);

    mysprintf(buffer, ci->cursp->nextcp.x-ci->cursp->me.x );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_NextXOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextXOff),ubuf);

    mysprintf(buffer, ci->cursp->nextcp.y-ci->cursp->me.y );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_NextYOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextYOff),ubuf);

    mysprintf2(buffer, ci->cursp->nextcp.x,ci->cursp->nextcp.y );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextPos),ubuf);

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_NextDef), ci->cursp->nextcpdef );

    mysprintf(buffer, ci->cursp->prevcp.x-ci->cursp->me.x );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_PrevXOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevXOff),ubuf);

    mysprintf(buffer, ci->cursp->prevcp.y-ci->cursp->me.y );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_PrevYOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevYOff),ubuf);

    mysprintf2(buffer, ci->cursp->prevcp.x,ci->cursp->prevcp.y );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevPos),ubuf);

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_PrevDef), ci->cursp->prevcpdef );
}

static int PI_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CharView *cv = ci->cv;
	ci->cursp->selected = false;
	if ( ci->cursp->next!=NULL && ci->cursp->next->to!=ci->curspl->first )
	    ci->cursp = ci->cursp->next->to;
	else {
	    if ( ci->curspl->next == NULL )
		ci->curspl = *cv->heads[cv->drawmode];
	    else
		ci->curspl = ci->curspl->next;
	    ci->cursp = ci->curspl->first;
	}
	ci->cursp->selected = true;
	PIFillup(ci,0);
	CVShowPoint(cv,ci->cursp);
	SCUpdateAll(cv->sc);
    }
return( true );
}

static int PI_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CharView *cv = ci->cv;
	SplinePointList *spl;
	ci->cursp->selected = false;
	
	if ( ci->cursp!=ci->curspl->first ) {
	    ci->cursp = ci->cursp->prev->from;
	} else {
	    if ( ci->curspl==*cv->heads[cv->drawmode] ) {
		for ( spl = *cv->heads[cv->drawmode]; spl->next!=NULL; spl=spl->next );
	    } else {
		for ( spl = *cv->heads[cv->drawmode]; spl->next!=ci->curspl; spl=spl->next );
	    }
	    ci->curspl = spl;
	    ci->cursp = spl->last;
	    if ( spl->last==spl->first && spl->last->prev!=NULL )
		ci->cursp = ci->cursp->prev->from;
	}
	ci->cursp->selected = true;
	cv->p.nextcp = cv->p.prevcp = false;
	PIFillup(ci,0);
	CVShowPoint(cv,ci->cursp);
	SCUpdateAll(cv->sc);
    }
return( true );
}

static int PI_BaseChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_BaseX )
	    dx = GetCalmRealR(ci->gw,CID_BaseX,_STR_BaseX,&err)-cursp->me.x;
	else
	    dy = GetCalmRealR(ci->gw,CID_BaseY,_STR_BaseY,&err)-cursp->me.y;
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->me.x += dx;
	cursp->nextcp.x += dx;
	cursp->prevcp.x += dx;
	cursp->me.y += dy;
	cursp->nextcp.y += dy;
	cursp->prevcp.y += dy;
	if ( cursp->next!=NULL )
	    SplineRefigure(cursp->next);
	if ( cursp->prev!=NULL )
	    SplineRefigure(cursp->prev);
	CVCharChangedUpdate(ci->cv);
	PIFillup(ci,GGadgetGetCid(g));
    }
return( true );
}

static int PI_NextChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_NextXOff )
	    dx = GetCalmRealR(ci->gw,CID_NextXOff,_STR_NextCPX,&err)-(cursp->nextcp.x-cursp->me.x);
	else
	    dy = GetCalmRealR(ci->gw,CID_NextYOff,_STR_NextCPY,&err)-(cursp->nextcp.y-cursp->me.y);
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->nextcp.x += dx;
	cursp->nextcp.y += dy;
	cursp->nonextcp = false;
	if (( dx>.1 || dx<-.1 || dy>.1 || dy<-.1 ) && cursp->nextcpdef ) {
	    cursp->nextcpdef = false;
	    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_NextDef), false );
	}
	if ( ci->sc->parent->order2 )
	    SplinePointNextCPChanged2(cursp,false);
	else if ( cursp->next!=NULL )
	    SplineRefigure3(cursp->next);
	CVCharChangedUpdate(ci->cv);
	PIFillup(ci,GGadgetGetCid(g));
    }
return( true );
}

static int PI_PrevChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_PrevXOff )
	    dx = GetCalmRealR(ci->gw,CID_PrevXOff,_STR_PrevCPX,&err)-(cursp->prevcp.x-cursp->me.x);
	else
	    dy = GetCalmRealR(ci->gw,CID_PrevYOff,_STR_PrevCPY,&err)-(cursp->prevcp.y-cursp->me.y);
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->prevcp.x += dx;
	cursp->prevcp.y += dy;
	cursp->noprevcp = false;
	if (( dx>.1 || dx<-.1 || dy>.1 || dy<-.1 ) && cursp->prevcpdef ) {
	    cursp->prevcpdef = false;
	    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_PrevDef), false );
	}
	if ( ci->sc->parent->order2 )
	    SplinePointPrevCPChanged2(cursp,false);
	else if ( cursp->prev!=NULL )
	    SplineRefigure(cursp->prev);
	CVCharChangedUpdate(ci->cv);
	PIFillup(ci,GGadgetGetCid(g));
    }
return( true );
}

static int PI_NextDefChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	SplinePoint *cursp = ci->cursp;

	cursp->nextcpdef = GGadgetIsChecked(g);
	/* If they turned def off, that's a noop, but if they turned it on... */
	/*  then set things to the default */
	if ( cursp->nextcpdef ) {
	    BasePoint temp = cursp->prevcp;
	    SplineCharDefaultNextCP(cursp);
	    if ( !cursp->prevcpdef )
		cursp->prevcp = temp;
	    CVCharChangedUpdate(ci->cv);
	    PIFillup(ci,GGadgetGetCid(g));
	}
    }
return( true );
}

static int PI_PrevDefChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	SplinePoint *cursp = ci->cursp;

	cursp->prevcpdef = GGadgetIsChecked(g);
	/* If they turned def off, that's a noop, but if they turned it on... */
	/*  then set things to the default */
	if ( cursp->prevcpdef ) {
	    BasePoint temp = cursp->nextcp;
	    SplineCharDefaultPrevCP(cursp);
	    if ( !cursp->nextcpdef )
		cursp->nextcp = temp;
	    CVCharChangedUpdate(ci->cv);
	    PIFillup(ci,GGadgetGetCid(g));
	}
    }
return( true );
}

static void PointGetInfo(CharView *cv, SplinePoint *sp, SplinePointList *spl) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[24];
    GTextInfo label[24];
    static GBox cur, nextcp, prevcp;
    extern Color nextcpcol, prevcpcol;
    GWindow root;
    GRect screensize;
    GPoint pt;

    cur.main_background = nextcp.main_background = prevcp.main_background = COLOR_DEFAULT;
    cur.main_foreground = 0xff0000;
    nextcp.main_foreground = nextcpcol;
    prevcp.main_foreground = prevcpcol;
    gi.cv = cv;
    gi.sc = cv->sc;
    gi.cursp = sp;
    gi.curspl = spl;
    gi.oldstate = SplinePointListCopy(*cv->heads[cv->drawmode]);
    gi.done = false;
    CVPreserveState(cv);

    root = GDrawGetRoot(NULL);
    GDrawGetSize(root,&screensize);

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.positioned = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_PointInfo,NULL);
	wattrs.is_dlg = true;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,PI_Width));
	pos.height = GDrawPointsToPixels(NULL,PI_Height);
	pt.x = cv->xoff + rint(sp->me.x*cv->scale);
	pt.y = -cv->yoff + cv->height - rint(sp->me.y*cv->scale);
	GDrawTranslateCoordinates(cv->v,root,&pt);
	if ( pt.x+20+pos.width<=screensize.width )
	    pos.x = pt.x+20;
	else if ( (pos.x = pt.x-10-screensize.width)<0 )
	    pos.x = 0;
	pos.y = pt.y;
	if ( pos.y+pos.height+20 > screensize.height )
	    pos.y = screensize.height - pos.height - 20;
	if ( pos.y<0 ) pos.y = 0;
	gi.gw = GDrawCreateTopWindow(NULL,&pos,pi_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	label[0].text = (unichar_t *) _STR_Base;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+6; 
	gcd[0].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[0].gd.box = &cur;
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 60; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 70;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_BaseX;
	gcd[1].gd.handle_controlevent = PI_BaseChanged;
	gcd[1].creator = GTextFieldCreate;

	gcd[2].gd.pos.x = 137; gcd[2].gd.pos.y = 5; gcd[2].gd.pos.width = 70;
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].gd.cid = CID_BaseY;
	gcd[2].gd.handle_controlevent = PI_BaseChanged;
	gcd[2].creator = GTextFieldCreate;

	label[3].text = (unichar_t *) _STR_PrevCP;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 9; gcd[3].gd.pos.y = 35; 
	gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[3].gd.box = &prevcp;
	gcd[3].creator = GLabelCreate;

	gcd[4].gd.pos.x = 60; gcd[4].gd.pos.y = 35; gcd[4].gd.pos.width = 60;
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.cid = CID_PrevPos;
	gcd[4].creator = GLabelCreate;

	label[21].text = (unichar_t *) _STR_Default;
	label[21].text_in_resource = true;
	gcd[21].gd.label = &label[21];
	gcd[21].gd.pos.x = 125; gcd[21].gd.pos.y = gcd[4].gd.pos.y-3;
	gcd[21].gd.flags = (gg_enabled|gg_visible);
	gcd[21].gd.cid = CID_PrevDef;
	gcd[21].gd.handle_controlevent = PI_PrevDefChanged;
	gcd[21].creator = GCheckBoxCreate;

	gcd[5].gd.pos.x = 60; gcd[5].gd.pos.y = 49; gcd[5].gd.pos.width = 70;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.cid = CID_PrevXOff;
	gcd[5].gd.handle_controlevent = PI_PrevChanged;
	gcd[5].creator = GTextFieldCreate;

	gcd[6].gd.pos.x = 137; gcd[6].gd.pos.y = 49; gcd[6].gd.pos.width = 70;
	gcd[6].gd.flags = gg_enabled|gg_visible;
	gcd[6].gd.cid = CID_PrevYOff;
	gcd[6].gd.handle_controlevent = PI_PrevChanged;
	gcd[6].creator = GTextFieldCreate;

	label[7].text = (unichar_t *) _STR_NextCP;
	label[7].text_in_resource = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.pos.x = gcd[3].gd.pos.x; gcd[7].gd.pos.y = 82; 
	gcd[7].gd.flags = gg_enabled|gg_visible;
	gcd[7].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[7].gd.box = &nextcp;
	gcd[7].creator = GLabelCreate;

	gcd[8].gd.pos.x = 60; gcd[8].gd.pos.y = 82;  gcd[8].gd.pos.width = 60;
	gcd[8].gd.flags = gg_enabled|gg_visible;
	gcd[8].gd.cid = CID_NextPos;
	gcd[8].creator = GLabelCreate;

	label[22].text = (unichar_t *) _STR_Default;
	label[22].text_in_resource = true;
	gcd[22].gd.label = &label[22];
	gcd[22].gd.pos.x = gcd[21].gd.pos.x; gcd[22].gd.pos.y = gcd[8].gd.pos.y-3;
	gcd[22].gd.flags = (gg_enabled|gg_visible);
	gcd[22].gd.cid = CID_NextDef;
	gcd[22].gd.handle_controlevent = PI_NextDefChanged;
	gcd[22].creator = GCheckBoxCreate;

	gcd[9].gd.pos.x = 60; gcd[9].gd.pos.y = 96; gcd[9].gd.pos.width = 70;
	gcd[9].gd.flags = gg_enabled|gg_visible;
	gcd[9].gd.cid = CID_NextXOff;
	gcd[9].gd.handle_controlevent = PI_NextChanged;
	gcd[9].creator = GTextFieldCreate;

	gcd[10].gd.pos.x = 137; gcd[10].gd.pos.y = 96; gcd[10].gd.pos.width = 70;
	gcd[10].gd.flags = gg_enabled|gg_visible;
	gcd[10].gd.cid = CID_NextYOff;
	gcd[10].gd.handle_controlevent = PI_NextChanged;
	gcd[10].creator = GTextFieldCreate;

	gcd[11].gd.pos.x = (PI_Width-2*50-10)/2; gcd[11].gd.pos.y = 127;
	gcd[11].gd.pos.width = 53; gcd[11].gd.pos.height = 0;
	gcd[11].gd.flags = gg_visible | gg_enabled;
	label[11].text = (unichar_t *) _STR_PrevArrow;
	label[11].text_in_resource = true;
	gcd[11].gd.mnemonic = 'P';
	gcd[11].gd.label = &label[11];
	gcd[11].gd.handle_controlevent = PI_Prev;
	gcd[11].creator = GButtonCreate;

	gcd[12].gd.pos.x = PI_Width-50-(PI_Width-2*50-10)/2; gcd[12].gd.pos.y = gcd[11].gd.pos.y;
	gcd[12].gd.pos.width = 53; gcd[12].gd.pos.height = 0;
	gcd[12].gd.flags = gg_visible | gg_enabled;
	label[12].text = (unichar_t *) _STR_NextArrow;
	label[12].text_in_resource = true;
	gcd[12].gd.label = &label[12];
	gcd[12].gd.mnemonic = 'N';
	gcd[12].gd.handle_controlevent = PI_Next;
	gcd[12].creator = GButtonCreate;

	gcd[13].gd.pos.x = 5; gcd[13].gd.pos.y = 157;
	gcd[13].gd.pos.width = PI_Width-10;
	gcd[13].gd.flags = gg_enabled|gg_visible;
	gcd[13].creator = GLineCreate;

	gcd[14].gd.pos.x = 20-3; gcd[14].gd.pos.y = PI_Height-35-3;
	gcd[14].gd.pos.width = -1; gcd[14].gd.pos.height = 0;
	gcd[14].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[14].text = (unichar_t *) _STR_OK;
	label[14].text_in_resource = true;
	gcd[14].gd.mnemonic = 'O';
	gcd[14].gd.label = &label[14];
	gcd[14].gd.handle_controlevent = PI_Ok;
	gcd[14].creator = GButtonCreate;

	gcd[15].gd.pos.x = -20; gcd[15].gd.pos.y = PI_Height-35;
	gcd[15].gd.pos.width = -1; gcd[15].gd.pos.height = 0;
	gcd[15].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[15].text = (unichar_t *) _STR_Cancel;
	label[15].text_in_resource = true;
	gcd[15].gd.label = &label[15];
	gcd[15].gd.mnemonic = 'C';
	gcd[15].gd.handle_controlevent = PI_Cancel;
	gcd[15].creator = GButtonCreate;

	gcd[16].gd.pos.x = 2; gcd[16].gd.pos.y = 2;
	gcd[16].gd.pos.width = pos.width-4; gcd[16].gd.pos.height = pos.height-4;
	gcd[16].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[16].creator = GGroupCreate;

	gcd[17].gd.pos.x = 5; gcd[17].gd.pos.y = gcd[3].gd.pos.y-3;
	gcd[17].gd.pos.width = PI_Width-10; gcd[17].gd.pos.height = 43;
	gcd[17].gd.flags = gg_enabled | gg_visible;
	gcd[17].creator = GGroupCreate;

	gcd[18].gd.pos.x = 5; gcd[18].gd.pos.y = gcd[7].gd.pos.y-3;
	gcd[18].gd.pos.width = PI_Width-10; gcd[18].gd.pos.height = 43;
	gcd[18].gd.flags = gg_enabled | gg_visible;
	gcd[18].creator = GGroupCreate;

	label[19].text = (unichar_t *) _STR_Offset;
	label[19].text_in_resource = true;
	gcd[19].gd.label = &label[19];
	gcd[19].gd.pos.x = gcd[3].gd.pos.x+10; gcd[19].gd.pos.y = gcd[5].gd.pos.y+6; 
	gcd[19].gd.flags = gg_enabled|gg_visible;
	gcd[19].creator = GLabelCreate;

	label[20].text = (unichar_t *) _STR_Offset;
	label[20].text_in_resource = true;
	gcd[20].gd.label = &label[20];
	gcd[20].gd.pos.x = gcd[3].gd.pos.x+10; gcd[20].gd.pos.y = gcd[9].gd.pos.y+6; 
	gcd[20].gd.flags = gg_enabled|gg_visible;
	gcd[20].creator = GLabelCreate;

	GGadgetsCreate(gi.gw,gcd);

	PIFillup(&gi,0);

    GWidgetHidePalettes();
    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
}

void CVGetInfo(CharView *cv) {
    SplinePoint *sp;
    SplinePointList *spl;
    RefChar *ref;
    ImageList *img;
    AnchorPoint *ap;

    if ( !CVOneThingSel(cv,&sp,&spl,&ref,&img,&ap)) {
#if 0
	if ( cv->fv->cidmaster==NULL )
	    SCCharInfo(cv->sc);
#endif
    } else if ( ref!=NULL )
	RefGetInfo(cv,ref);
    else if ( img!=NULL )
	ImgGetInfo(cv,img);
    else if ( ap!=NULL )
	ApGetInfo(cv,ap);
    else
	PointGetInfo(cv,sp,spl);
}

void SCRefBy(SplineChar *sc) {
    static int buts[] = { _STR_Show, _STR_Cancel };
    int cnt,i,tot=0;
    unichar_t **deps = NULL;
    struct splinecharlist *d;

    for ( i=0; i<2; ++i ) {
	cnt = 0;
	for ( d = sc->dependents; d!=NULL; d=d->next ) {
	    if ( deps!=NULL )
		deps[tot-cnt] = uc_copy(d->sc->name);
	    ++cnt;
	}
	if ( cnt==0 )
return;
	if ( i==0 )
	    deps = gcalloc(cnt+1,sizeof(unichar_t *));
	tot = cnt-1;
    }

    i = GWidgetChoicesBR(_STR_Dependents,(const unichar_t **) deps, cnt, 0, buts, _STR_Dependents );
    if ( i!=-1 ) {
	i = tot-i;
	for ( d = sc->dependents, cnt=0; d!=NULL && cnt<i; d=d->next, ++cnt );
	CharViewCreate(d->sc,sc->parent->fv);
    }
    for ( i=0; i<=tot; ++i )
	free( deps[i] );
    free(deps);
}
