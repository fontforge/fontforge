/* Copyright (C) 2000-2004 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <ustring.h>
#include <math.h>
#include <utype.h>
#include <gkeysym.h>

#define RAD2DEG	(180/3.1415926535897932)
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
    int prevchanged, nextchanged;
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
#define CID_Curve	2011
#define CID_Corner	2012
#define CID_Tangent	2013
#define CID_NextR	2014
#define CID_NextTheta	2015
#define CID_PrevR	2016
#define CID_PrevTheta	2017
#define CID_HintMask	2020
#define CID_ActiveHints	2030
#define CID_TabSet	2100

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

#define PI_Width	228
#define PI_Height	308

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
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_OutOfRange,_STR_OutOfRange);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Value out of range"),_("Value out of range"));
#endif
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
    SplinePointListFree(ref->layers[0].splines);
    ref->layers[0].splines = SplinePointListTransform(SplinePointListCopy(ref->sc->layers[ly_fore].splines),trans,true);
    spl = NULL;
    if ( ref->layers[0].splines!=NULL )
	for ( spl = ref->layers[0].splines; spl->next!=NULL; spl = spl->next );
    for ( subref = ref->sc->layers[ly_fore].refs; subref!=NULL; subref=subref->next ) {
	new = SplinePointListTransform(SplinePointListCopy(subref->layers[0].splines),trans,true);
	if ( spl==NULL )
	    ref->layers[0].splines = new;
	else
	    spl->next = new;
	if ( new!=NULL )
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
    }

    SplineSetFindBounds(ref->layers[0].splines,&ref->bb);
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
#if defined(FONTFORGE_CONFIG_GDRAW)
	    static int buts[] = { _STR_Change, _STR_Retain, _STR_Cancel, 0 };
	    int ans = GWidgetAskR(_STR_TransformChanged,buts,0,2,_STR_TransformChangedApply);
#elif defined(FONTFORGE_CONFIG_GTK)
	    char *buts[4];
	    buts[0] = _("C_hange");
	    buts[1] = _("_Retain");
	    buts[2] = GTK_STOCK_CANCEL;
	    buts[3] = NULL;
	    int ans = gwwv_ask(_("Transformation Matrix Changed"),buts,0,2,_("You have changed the transformation matrix, do you wish to use the new version?"));
#endif
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
#if defined(FONTFORGE_CONFIG_GDRAW)
	wattrs.window_title = GStringGetResource(_STR_ReferenceInfo,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	wattrs.window_title = _("Reference Info");
#endif
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
#if defined(FONTFORGE_CONFIG_GDRAW)
	gcd[j].gd.popup_msg = GStringGetResource(_STR_TransformPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	gcd[j].gd.popup_msg = _("The transformation matrix specifies how the points in\nthe source character should be transformed before\nthey are drawn in the current character.\n x(new) = tm[1,1]*x + tm[2,1]*y + tm[3,1]\n y(new) = tm[1,2]*x + tm[2,2]*y + tm[3,2]");
#endif
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
#if defined(FONTFORGE_CONFIG_GDRAW)
	wattrs.window_title = GStringGetResource(_STR_ImageInfo,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	wattrs.window_title = _("Image Info");
#endif
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
	    else if ( an->type==act_mkmk ) {
		if ( ap->type==at_basemark )
		    sawexit = true;
		else
		    sawentry = true;
	    } else if ( ap->type!=at_baselig )
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
    AnchorClass *an, *maybe;
    int val, maybelig;
    SplineFont *sf;
    /* Are there any anchors with this name? If so can't reuse it */
    /*  unless they are ligature anchores */
    /*  or 'curs' anchors, which allow exactly two points (entry, exit) */
    /*  or 'mkmk' anchors, which allow a mark to be both a base and an attach */

    *waslig = false; maybelig = false; maybe = NULL;
    sf = sc->parent;
    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    for ( an=sf->anchor; an!=NULL; an=an->next ) {
	val = IsAnchorClassUsed(sc,an);
	if ( val>=0 ) {
	    *waslig = val;
return( an );
	} else if ( val!=-1 && maybe==NULL ) {
	    maybe = an;
	    maybelig = val;
	}
    }
    *waslig = maybelig;
return( maybe );
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
    ap->type = an->type==act_mark ? at_basechar :
		an->type==act_mkmk ? at_basemark :
		at_centry;
    for ( pst = cv->sc->possub; pst!=NULL && pst->type!=pst_ligature; pst=pst->next );
    if ( waslig<-1 && an->type==act_mkmk ) {
	ap->type = waslig==-2 ? at_basemark : at_mark;
    } else if ( waslig==-2  && an->type==act_curs )
	ap->type = at_cexit;
    else if ( waslig==-3 || an->type==act_curs )
	ap->type = at_centry;
    else if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
	    iscombining(sc->unicodeenc))
	ap->type = at_mark;
    else if ( an->type==act_mkmk )
	ap->type = at_basemark;
    else if ( pst!=NULL || waslig )
	ap->type = at_baselig;
    if (( ap->type==at_basechar || ap->type==at_baselig ) && an->type==act_mkmk )
	ap->type = at_basemark;
    ap->next = sc->anchor;
    if ( waslig>=0 )
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
    SplineFont *sf = ci->sc->parent;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    for ( i=0, an=sf->anchor; an!=ap->anchor; ++i, an=an->next );
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_NameList),i);
}

static void AI_DisplayClass(GIData *ci,AnchorPoint *ap) {
    AnchorClass *ac = ap->anchor;
    AnchorPoint *aps;
    int saw[at_max];

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseChar),ac->type==act_mark);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseLig),ac->type==act_mark);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseMark),ac->type==act_mkmk);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursEntry),ac->type==act_curs);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursExit),ac->type==act_curs);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Mark),ac->type!=act_curs);

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LigIndex),ap->type==at_baselig);

    if ( ac->type==act_mkmk && (ap->type==at_basechar || ap->type==at_baselig)) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseMark),true);
	ap->type = at_basemark;
    } else if ( ac->type==act_mark && ap->type==at_basemark ) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseChar),true);
	ap->type = at_basechar;
    } else if ( ac->type==act_curs && ap->type!=at_centry && ap->type!=at_cexit ) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_CursEntry),true);
	ap->type = at_centry;
    }

    memset(saw,0,sizeof(saw));
    for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next ) if ( aps!=ap ) {
	if ( aps->anchor==ac ) saw[aps->type] = true;
    }
    if ( ac->type==act_curs ) {
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursEntry),!saw[at_centry]);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_CursExit),!saw[at_cexit]);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Mark),!saw[at_mark]);
	if ( saw[at_basechar]) GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseChar),false);
	if ( saw[at_basemark]) GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseMark),false);
    }
}

static void AI_DisplayIndex(GIData *ci,AnchorPoint *ap) {
    char buffer[12];
    unichar_t ubuf[12];

    sprintf(buffer,"%d", ap->lig_index );
    uc_strcpy(ubuf,buffer);

    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_LigIndex),ubuf);
}

static void AI_DisplayRadio(GIData *ci,enum anchor_type type) {
    switch ( type ) {
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
    AI_DisplayRadio(ci,ap->type);

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
#if defined(FONTFORGE_CONFIG_GDRAW)
	    static int buts[] = { _STR_Yes, _STR_No, 0 };
	    if ( GWidgetAskR(_STR_LastAnchor,buts,0,1,_STR_RemoveLastAnchor)==1 ) {
#elif defined(FONTFORGE_CONFIG_GTK)
	    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_NO, NULL };
	    if ( gwwv_ask(_("Last Anchor Point"),buts,0,1,_("You are deleting the last anchor point in this character.\nDoing so will cause this dialog to close, is that what you want?"))==1 ) {
#endif
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
	SplineFont *sf = ci->sc->parent;

	if ( sf->cidmaster ) sf = sf->cidmaster;

	if ( AnchorClassUnused(ci->sc,&waslig)==NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetPostNoticeR(_STR_MakeNewClass,_STR_MakeNewAnchorClass);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_notice(_("Make a new anchor class"),_("I cannot find an unused anchor class\nto assign a new point to. If you\nwish a new anchor point you must\ndefine a new anchor class with\nElement->Font Info"));
#endif
	    FontInfo(sf,8,true);		/* Anchor Class */
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
#if defined(FONTFORGE_CONFIG_GDRAW)
		GWidgetErrorR(_STR_OutOfOrder,_STR_IndexOutOfOrder,aps->lig_index);
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Out Of Order"),_("Marks within a ligature should be ordered with the direction of writing.\nThis one and %d are out of order."),aps->lig_index);
#endif
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
#if defined(FONTFORGE_CONFIG_GDRAW)
		    GWidgetErrorR(_STR_IndexInUse,_STR_LigIndexInUse);
#elif defined(FONTFORGE_CONFIG_GTK)
		    gwwv_post_error(_("Index in use"),_("This ligature index is already in use"));
#endif
return( true );
		} else if ( aps->lig_index>max )
		    max = aps->lig_index;
	    }
	}
	if ( index>max+10 ) {
	    char buf[20]; unichar_t ubuf[20];
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_TooBig,_STR_IndexTooBig);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Too Big"),_("This index is much larger than the closest neighbor"));
#endif
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
	int change=0, max=0;
	int ntype;
	int sawentry, sawexit;

	if ( an==ci->ap->anchor )
return( true );			/* No op */

	ntype = ci->ap->type;
	if ( an->type==act_curs ) {
	    if ( ntype!=at_centry && ntype!=at_cexit )
		ntype = at_centry;
	} else if ( an->type==act_mkmk ) {
	    if ( ntype!=at_basemark && ntype!=at_mark )
		ntype = at_basemark;
	} else if ( ntype==at_centry || ntype==at_cexit || ntype==at_basemark ) {
	    PST *pst;
	    for ( pst = ci->sc->possub; pst!=NULL && pst->type!=pst_ligature; pst=pst->next );
	    if ( ci->sc->unicodeenc!=-1 && ci->sc->unicodeenc<0x10000 &&
		    iscombining(ci->sc->unicodeenc))
		ntype = at_mark;
	    else if ( pst!=NULL )
		ntype = at_baselig;
	    else
		ntype = at_basechar;
	}
	sawentry = sawexit = false;
	for ( ap=ci->sc->anchor; ap!=NULL; ap = ap->next ) {
	    if ( ap!=ci->ap && ap->anchor==an ) {
		if ( an->type==act_curs ) {
		    if ( ap->type == at_centry ) sawentry = true;
		    else if ( ap->type== at_cexit ) sawexit = true;
		} else if ( an->type==act_mkmk ) {
		    if ( ap->type == at_mark ) sawentry = true;
		    else if ( ap->type== at_basemark ) sawexit = true;
		} else {
		    if ( ap->type!=at_baselig )
	break;
		    if ( ap->lig_index==ci->ap->lig_index )
			change = true;
		    else if ( ap->lig_index>max )
			max = ap->lig_index;
		}
	    } else if ( ap!=ci->ap && ap->anchor->merge_with==an->merge_with &&
		    ap->type==at_mark )
	break;
	}
	if ( ap!=NULL || (sawentry && sawexit)) {
	    AI_SelectList(ci,ci->ap);
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_ClassUsed,_STR_AnchorClassUsed);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Class already used"),_("This anchor class already is associated with a point in this character"));
#endif
	} else {
	    ci->ap->anchor = an;
	    if ( an->type==act_curs ) {
		if ( sawentry ) ntype = at_cexit;
		else if ( sawexit ) ntype = at_centry;
	    } else if ( an->type==act_mkmk ) {
		if ( sawentry ) ntype = at_basemark;
		else if ( sawexit ) ntype = at_mark;
	    }
	    if ( ci->ap->type!=ntype ) {
		ci->ap->type = ntype;
		AI_DisplayRadio(ci,ntype);
	    }
	    if ( ci->ap->type!=at_baselig )
		ci->ap->lig_index = 0;
	    else if ( change ) {
		ci->ap->lig_index = max+1;
		AI_DisplayIndex(ci,ci->ap);
	    }
	    AI_DisplayClass(ci,ci->ap);
	    AI_TestOrdering(ci,ci->ap->me.x);
	}
	CVCharChangedUpdate(ci->cv);
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
    SplineFont *sf;

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
#if defined(FONTFORGE_CONFIG_GDRAW)
	wattrs.window_title = GStringGetResource(_STR_AnchorPointInfo,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	wattrs.window_title = _("Anchor Point Info");
#endif
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
	sf = cv->sc->parent;
	if ( sf->cidmaster ) sf = sf->cidmaster;
	gcd[j].gd.u.list = AnchorClassesList(sf);
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

void PI_ShowHints(SplineChar *sc, GGadget *list, int set) {
    StemInfo *h;
    int32 i, len;

    if ( !set ) {
	for ( h = sc->hstem; h!=NULL; h=h->next )
	    h->active = false;
	for ( h = sc->vstem; h!=NULL; h=h->next )
	    h->active = false;
    } else {
	GTextInfo **ti = GGadgetGetList(list,&len);
	for ( h = sc->hstem, i=0; h!=NULL && i<len; h=h->next, ++i )
	    h->active = ti[i]->selected;
	for ( h = sc->vstem; h!=NULL && i<len; h=h->next, ++i )
	    h->active = ti[i]->selected;
    }
    SCOutOfDateBackground(sc);
    SCUpdateAll(sc);
}

static void _PI_ShowHints(GIData *ci,int set) {
    PI_ShowHints(ci->cv->sc,GWidgetGetControl(ci->gw,CID_HintMask),set);
}

static void PI_DoCancel(GIData *ci) {
    CharView *cv = ci->cv;
    ci->done = true;
    if ( cv->drawmode==dm_fore )
	MDReplace(cv->sc->md,cv->sc->layers[ly_fore].splines,ci->oldstate);
    SplinePointListsFree(cv->layerheads[cv->drawmode]->splines);
    cv->layerheads[cv->drawmode]->splines = ci->oldstate;
    CVRemoveTopUndo(cv);
    SCClearSelPt(cv->sc);
    _PI_ShowHints(ci,false);
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

static void PIFillup(GIData *ci, int except_cid);

static void PI_FigureNext(GIData *ci) {
    if ( ci->prevchanged ) {
	SplinePoint *cursp = ci->cursp;
	if ( !ci->sc->parent->order2 && cursp->pointtype==pt_curve ) {
	    double dx, dy, len, len2;
	    dx = cursp->prevcp.x - cursp->me.x;
	    dy = cursp->prevcp.y - cursp->me.y;
	    len = sqrt(dx*dx+dy*dy);
	    if ( len!=0 ) {
		len2 = sqrt((cursp->nextcp.x-cursp->me.x)*(cursp->nextcp.x-cursp->me.x)+
			(cursp->nextcp.y-cursp->me.y)*(cursp->nextcp.y-cursp->me.y));
		cursp->nextcp.x=cursp->me.x-dx*len2/len;
		cursp->nextcp.y=cursp->me.y-dy*len2/len;
		if ( cursp->next!=NULL )
		    SplineRefigure(cursp->next);
		CVCharChangedUpdate(ci->cv);
		PIFillup(ci,-1);
	    }
	}
    }
    ci->prevchanged = false;
}

static void PI_FigurePrev(GIData *ci) {
    if ( ci->nextchanged ) {
	SplinePoint *cursp = ci->cursp;
	if ( !ci->sc->parent->order2 && cursp->pointtype==pt_curve ) {
	    double dx, dy, len, len2;
	    dx = cursp->nextcp.x - cursp->me.x;
	    dy = cursp->nextcp.y - cursp->me.y;
	    len = sqrt(dx*dx+dy*dy);
	    if ( len!=0 ) {
		len2 = sqrt((cursp->prevcp.x-cursp->me.x)*(cursp->prevcp.x-cursp->me.x)+
			(cursp->prevcp.y-cursp->me.y)*(cursp->prevcp.y-cursp->me.y));
		cursp->prevcp.x=cursp->me.x-dx*len2/len;
		cursp->prevcp.y=cursp->me.y-dy*len2/len;
		if ( cursp->prev!=NULL )
		    SplineRefigure(cursp->prev);
		CVCharChangedUpdate(ci->cv);
		PIFillup(ci,-1);
	    }
	}
    }
    ci->nextchanged = false;
}

static void PI_FigureHintMask(GIData *ci) {
    int32 i, len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(ci->gw,CID_HintMask),&len);

    for ( i=0; i<len; ++i )
	if ( ti[i]->selected )
    break;

    if ( i==len )
	chunkfree(ci->cursp->hintmask,sizeof(HintMask));
    else {
	if ( ci->cursp->hintmask==NULL )
	    ci->cursp->hintmask = chunkalloc(sizeof(HintMask));
	else
	    memset(ci->cursp->hintmask,0,sizeof(HintMask));
	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected )
		(*ci->cursp->hintmask)[i>>3] |= (0x80>>(i&7));
    }
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

	PI_FigureHintMask(ci);
	_PI_ShowHints(ci,false);

	PI_FigureNext(ci);
	PI_FigurePrev(ci);

	ci->done = true;
	/* All the work has been done as we've gone along */
    }
return( true );
}

static void mysprintf( char *buffer, char *format, real v) {
    char *pt;

    sprintf( buffer, format, v );
    pt = strrchr(buffer,'.');
    if ( pt[1]=='0' && pt[2]=='0' )
	*pt='\0';
    else if ( pt[2]=='0' )
	pt[2] = '\0';
}

static void mysprintf2( char *buffer, real v1, real v2) {
    char *pt;

    mysprintf(buffer,"%.2f", v1);
    pt = buffer+strlen(buffer);
    *pt++ = ',';
    mysprintf(pt,"%.2f", v2);
}

static void PIFillup(GIData *ci, int except_cid) {
    char buffer[50];
    unichar_t ubuf[50];
    double dx, dy;

    mysprintf(buffer, "%.2f", ci->cursp->me.x );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_BaseX )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_BaseX),ubuf);

    mysprintf(buffer, "%.2f", ci->cursp->me.y );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_BaseY )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_BaseY),ubuf);

    dx = ci->cursp->nextcp.x-ci->cursp->me.x;
    dy = ci->cursp->nextcp.y-ci->cursp->me.y;
    mysprintf(buffer, "%.2f", dx );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_NextXOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextXOff),ubuf);

    mysprintf(buffer, "%.2f", dy );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_NextYOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextYOff),ubuf);

    if ( except_cid!=CID_NextR ) {
	mysprintf(buffer, "%.2f", sqrt( dx*dx+dy*dy ));
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextR),ubuf);
    }

    if ( except_cid!=CID_NextTheta ) {
	if ( ci->cursp->pointtype==pt_tangent && ci->cursp->prev!=NULL ) {
	    dx = ci->cursp->me.x-ci->cursp->prev->from->me.x;
	    dy = ci->cursp->me.y-ci->cursp->prev->from->me.y;
	}
	mysprintf(buffer, "%.1f", atan2(dy,dx)*RAD2DEG);
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextTheta),ubuf);
    }

    mysprintf2(buffer, ci->cursp->nextcp.x,ci->cursp->nextcp.y );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextPos),ubuf);

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_NextDef), ci->cursp->nextcpdef );

    dx = ci->cursp->prevcp.x-ci->cursp->me.x;
    dy = ci->cursp->prevcp.y-ci->cursp->me.y;
    mysprintf(buffer, "%.2f", dx );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_PrevXOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevXOff),ubuf);

    mysprintf(buffer, "%.2f", dy );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_PrevYOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevYOff),ubuf);

    if ( except_cid!=CID_PrevR ) {
	mysprintf(buffer, "%.2f", sqrt( dx*dx+dy*dy ));
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevR),ubuf);
    }

    if ( except_cid!=CID_PrevTheta ) {
	if ( ci->cursp->pointtype==pt_tangent && ci->cursp->next!=NULL ) {
	    dx = ci->cursp->me.x-ci->cursp->next->to->me.x;
	    dy = ci->cursp->me.y-ci->cursp->next->to->me.y;
	}
	mysprintf(buffer, "%.1f", atan2(dy,dx)*RAD2DEG);
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevTheta),ubuf);
    }

    mysprintf2(buffer, ci->cursp->prevcp.x,ci->cursp->prevcp.y );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevPos),ubuf);

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_PrevDef), ci->cursp->prevcpdef );

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Curve), ci->cursp->pointtype==pt_curve );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Corner), ci->cursp->pointtype==pt_corner );
    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Tangent), ci->cursp->pointtype==pt_tangent );

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_PrevTheta), ci->cursp->pointtype!=pt_tangent );
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_NextTheta), ci->cursp->pointtype!=pt_tangent );
}

static void PIChangePoint(GIData *ci) {
    int aspect = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_TabSet));
    GGadget *list = GWidgetGetControl(ci->gw,CID_HintMask);
    int32 i, len;
    HintMask *hm;
    SplinePoint *sp;
    SplineSet *spl;

    GGadgetGetList(list,&len);

    PIFillup(ci,0);
    if ( ci->cursp->hintmask==NULL ) {
	for ( i=0; i<len; ++i )
	    GGadgetSelectListItem(list,i,false);
    } else {
	for ( i=0; i<len && i<HntMax; ++i )
	    GGadgetSelectListItem(list,i, (*ci->cursp->hintmask)[i>>3]&(0x80>>(i&7))?true:false );
    }
    _PI_ShowHints(ci,aspect==1);

    list = GWidgetGetControl(ci->gw,CID_ActiveHints);
    hm = NULL;
    /* Figure out what hintmask is active at the current point */
    /* Note: we must walk each ss backwards because we reverse the splineset */
    /*  when we output postscript */
    for ( spl = ci->sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( sp->hintmask )
		hm = sp->hintmask;
	    if ( sp==ci->cursp )
	break;
	    if ( sp->prev==NULL )
	break;
	    sp = sp->prev->from;
	    if ( sp==spl->first )
	break;
	}
    }
    if ( hm==NULL ) {
	for ( i=0; i<len; ++i )
	    GGadgetSelectListItem(list,i,false);
    } else {
	for ( i=0; i<len && i<HntMax; ++i )
	    GGadgetSelectListItem(list,i, (*hm)[i>>3]&(0x80>>(i&7))?true:false );
    }
}

static int PI_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CharView *cv = ci->cv;

	PI_FigureHintMask(ci);

	PI_FigureNext(ci);
	PI_FigurePrev(ci);
	
	ci->cursp->selected = false;
	if ( ci->cursp->next!=NULL && ci->cursp->next->to!=ci->curspl->first )
	    ci->cursp = ci->cursp->next->to;
	else {
	    if ( ci->curspl->next == NULL )
		ci->curspl = cv->layerheads[cv->drawmode]->splines;
	    else
		ci->curspl = ci->curspl->next;
	    ci->cursp = ci->curspl->first;
	}
	ci->cursp->selected = true;
	PIChangePoint(ci);
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

	PI_FigureHintMask(ci);

	PI_FigureNext(ci);
	PI_FigurePrev(ci);

	ci->cursp->selected = false;
	
	if ( ci->cursp!=ci->curspl->first ) {
	    ci->cursp = ci->cursp->prev->from;
	} else {
	    if ( ci->curspl==cv->layerheads[cv->drawmode]->splines ) {
		for ( spl = cv->layerheads[cv->drawmode]->splines; spl->next!=NULL; spl=spl->next );
	    } else {
		for ( spl = cv->layerheads[cv->drawmode]->splines; spl->next!=ci->curspl; spl=spl->next );
	    }
	    ci->curspl = spl;
	    ci->cursp = spl->last;
	    if ( spl->last==spl->first && spl->last->prev!=NULL )
		ci->cursp = ci->cursp->prev->from;
	}
	ci->cursp->selected = true;
	cv->p.nextcp = cv->p.prevcp = false;
	PIChangePoint(ci);
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
    } else if ( e->type==et_controlevent &&
	    e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_FigureNext(ci);
	PI_FigurePrev(ci);
    }
return( true );
}

static int PI_NextChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_NextXOff ) {
	    dx = GetCalmRealR(ci->gw,CID_NextXOff,_STR_NextCPX,&err)-(cursp->nextcp.x-cursp->me.x);
	    if ( cursp->pointtype==pt_tangent && cursp->prev!=NULL ) {
		if ( cursp->prev->from->me.x==cursp->me.x ) {
		    dy = dx; dx = 0;	/* They should be constrained not to change in the x direction */
		} else
		    dy = dx*(cursp->prev->from->me.y-cursp->me.y)/(cursp->prev->from->me.x-cursp->me.x);
	    }
	} else if ( GGadgetGetCid(g)==CID_NextYOff ) {
	    dy = GetCalmRealR(ci->gw,CID_NextYOff,_STR_NextCPY,&err)-(cursp->nextcp.y-cursp->me.y);
	    if ( cursp->pointtype==pt_tangent && cursp->prev!=NULL ) {
		if ( cursp->prev->from->me.y==cursp->me.y ) {
		    dx = dy; dy = 0;	/* They should be constrained not to change in the y direction */
		} else
		    dx = dy*(cursp->prev->from->me.x-cursp->me.x)/(cursp->prev->from->me.y-cursp->me.y);
	    }
	} else {
	    double len, theta;
	    len = GetCalmRealR(ci->gw,CID_NextR,_STR_NextCPDist,&err);
	    theta = GetCalmRealR(ci->gw,CID_NextTheta,_STR_NextCPAngle,&err)/RAD2DEG;
	    dx = len*cos(theta) - (cursp->nextcp.x-cursp->me.x);
	    dy = len*sin(theta) - (cursp->nextcp.y-cursp->me.y);
	}
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->nextcp.x += dx;
	cursp->nextcp.y += dy;
	cursp->nonextcp = false;
	ci->nextchanged = true;
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
    } else if ( e->type==et_controlevent &&
	    e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_FigureNext(ci);
    }
return( true );
}

static int PI_PrevChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_PrevXOff ) {
	    dx = GetCalmRealR(ci->gw,CID_PrevXOff,_STR_PrevCPX,&err)-(cursp->prevcp.x-cursp->me.x);
	    if ( cursp->pointtype==pt_tangent && cursp->next!=NULL ) {
		if ( cursp->next->to->me.x==cursp->me.x ) {
		    dy = dx; dx = 0;	/* They should be constrained not to change in the x direction */
		} else
		    dy = dx*(cursp->next->to->me.y-cursp->me.y)/(cursp->next->to->me.x-cursp->me.x);
	    }
	} else if ( GGadgetGetCid(g)==CID_PrevYOff ) {
	    dy = GetCalmRealR(ci->gw,CID_PrevYOff,_STR_PrevCPY,&err)-(cursp->prevcp.y-cursp->me.y);
	    if ( cursp->pointtype==pt_tangent && cursp->next!=NULL ) {
		if ( cursp->next->to->me.y==cursp->me.y ) {
		    dx = dy; dy = 0;	/* They should be constrained not to change in the y direction */
		} else
		    dx = dy*(cursp->next->to->me.x-cursp->me.x)/(cursp->next->to->me.y-cursp->me.y);
	    }
	} else {
	    double len, theta;
	    len = GetCalmRealR(ci->gw,CID_PrevR,_STR_PrevCPDist,&err);
	    theta = GetCalmRealR(ci->gw,CID_PrevTheta,_STR_PrevCPAngle,&err)/RAD2DEG;
	    dx = len*cos(theta) - (cursp->prevcp.x-cursp->me.x);
	    dy = len*sin(theta) - (cursp->prevcp.y-cursp->me.y);
	}
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->prevcp.x += dx;
	cursp->prevcp.y += dy;
	cursp->noprevcp = false;
	ci->prevchanged = true;
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
    } else if ( e->type==et_controlevent &&
	    e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	PI_FigurePrev(ci);
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

static int PI_PTypeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	SplinePoint *cursp = ci->cursp;
	enum pointtype pt = GGadgetGetCid(g)-CID_Curve + pt_curve;

	if ( pt==cursp->pointtype ) {
	    /* Can't happen */
	} else if ( pt==pt_corner ) {
	    cursp->pointtype = pt_corner;
	    CVCharChangedUpdate(ci->cv);
	} else {
	    SPChangePointType(cursp,pt);
	    CVCharChangedUpdate(ci->cv);
	    PIFillup(ci,GGadgetGetCid(g));
	}
    }
return( true );
}

static int PI_AspectChange(GGadget *g, GEvent *e) {
    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int aspect = GTabSetGetSel(g);

	_PI_ShowHints(ci,aspect==1);
    }
return( true );
}

static int PI_HintSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));

	_PI_ShowHints(ci,true);

	if ( GGadgetIsListItemSelected(g,e->u.control.u.list.changed_index)) {
	    /* If we just selected something, check to make sure it doesn't */
	    /*  conflict with any other hints. */
	    /* Adobe says this is an error, but in "three" in AdobeSansMM */
	    /*  (in black,extended) we have a hintmask which contains two */
	    /*  overlapping hints */
	    /* So just make it a warning */
	    int i,j;
	    StemInfo *h, *h2=NULL;
	    for ( i=0, h=ci->cv->sc->hstem; h!=NULL && i!=e->u.control.u.list.changed_index; h=h->next, ++i );
	    if ( h!=NULL ) {
		for ( h2 = ci->cv->sc->hstem, i=0 ; h2!=NULL; h2=h2->next, ++i ) {
		    if ( h2!=h && GGadgetIsListItemSelected(g,i)) {
			if (( h2->start<h->start && h2->start+h2->width>h->start ) ||
			    ( h2->start>=h->start && h->start+h->width>h2->start ))
		break;
		    }
		}
	    } else {
		j = i;
		for ( h=ci->cv->sc->vstem; h!=NULL && i!=e->u.control.u.list.changed_index; h=h->next, ++i );
		if ( h==NULL )
		    GDrawIError("Failed to find hint");
		else {
		    for ( h2 = ci->cv->sc->hstem, i=j ; h2!=NULL; h2=h2->next, ++i ) {
			if ( h2!=h && GGadgetIsListItemSelected(g,i)) {
			    if (( h2->start<h->start && h2->start+h2->width>h->start ) ||
				( h2->start>=h->start && h->start+h->width>h2->start ))
		    break;
			}
		    }
		}
	    }
	    if ( h2!=NULL )
#if defined(FONTFORGE_CONFIG_GDRAW)
		GWidgetErrorR(_STR_OverlappedHints,_STR_OverlappedHintsLong,
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Overlapped Hints"),_("The hint you have just selected overlaps with <%.2f,%.2f>. You should deselect one of the two."),
#endif
			h2->start,h2->width);
	}
    }
return( true );
}

GTextInfo *SCHintList(SplineChar *sc,HintMask *hm) {
    StemInfo *h;
    int i;
    GTextInfo *ti;
    char buffer[100];

    for ( h=sc->hstem, i=0; h!=NULL; h=h->next, ++i );
    for ( h=sc->vstem     ; h!=NULL; h=h->next, ++i );
    ti = gcalloc(i+1,sizeof(GTextInfo));

    for ( h=sc->hstem, i=0; h!=NULL; h=h->next, ++i ) {
	ti[i].fg = ti[i].bg = COLOR_DEFAULT;
	ti[i].userdata = h;
	if ( h->ghost && h->width>0 )
	    sprintf( buffer, "H<%g,%g>",
		    rint(h->start*100)/100+rint(h->width*100)/100, -rint(h->width*100)/100 );
	else
	    sprintf( buffer, "H<%g,%g>",
		    rint(h->start*100)/100, rint(h->width*100)/100 );
	ti[i].text = uc_copy(buffer);
	if ( hm!=NULL && ((*hm)[i>>3]&(0x80>>(i&7))))
	    ti[i].selected = true;
    }

    for ( h=sc->vstem    ; h!=NULL; h=h->next, ++i ) {
	ti[i].fg = ti[i].bg = COLOR_DEFAULT;
	ti[i].userdata = h;
	if ( h->ghost && h->width>0 )
	    sprintf( buffer, "V<%g,%g>",
		    rint(h->start*100)/100+rint(h->width*100)/100, -rint(h->width*100)/100 );
	else
	    sprintf( buffer, "V<%g,%g>",
		    rint(h->start*100)/100, rint(h->width*100)/100 );
	ti[i].text = uc_copy(buffer);
	if ( hm!=NULL && ((*hm)[i>>3]&(0x80>>(i&7))))
	    ti[i].selected = true;
    }
return( ti );
}

static void PointGetInfo(CharView *cv, SplinePoint *sp, SplinePointList *spl) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[37], hgcd[2], h2gcd[2], mgcd[8];
    GTextInfo label[37], mlabel[8];
    GTabInfo aspects[4];
    static GBox cur, nextcp, prevcp;
    extern Color nextcpcol, prevcpcol;
    GWindow root;
    GRect screensize;
    GPoint pt;
    int j, defxpos, nextstarty;

    cur.main_background = nextcp.main_background = prevcp.main_background = COLOR_DEFAULT;
    cur.main_foreground = 0xff0000;
    nextcp.main_foreground = nextcpcol;
    prevcp.main_foreground = prevcpcol;
    gi.cv = cv;
    gi.sc = cv->sc;
    gi.cursp = sp;
    gi.curspl = spl;
    gi.oldstate = SplinePointListCopy(cv->layerheads[cv->drawmode]->splines);
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
#if defined(FONTFORGE_CONFIG_GDRAW)
	wattrs.window_title = GStringGetResource(_STR_PointInfo,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	wattrs.window_title = _("Point Info");
#endif
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
	memset(&hgcd,0,sizeof(hgcd));
	memset(&h2gcd,0,sizeof(h2gcd));
	memset(&mgcd,0,sizeof(mgcd));
	memset(&mlabel,0,sizeof(mlabel));
	memset(&aspects,0,sizeof(aspects));

	j=0;
	label[j].text = (unichar_t *) _STR_Base;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = 5+6; 
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[j].gd.box = &cur;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = 5; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseX;
	gcd[j].gd.handle_controlevent = PI_BaseChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = 5; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseY;
	gcd[j].gd.handle_controlevent = PI_BaseChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_PrevCP;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 9; gcd[j].gd.pos.y = 36; 
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[j].gd.box = &prevcp;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 65;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevPos;
	gcd[j].creator = GLabelCreate;
	++j;

	defxpos = 130;
	label[j].text = (unichar_t *) _STR_Default;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = defxpos; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-3;
	gcd[j].gd.flags = (gg_enabled|gg_visible);
	gcd[j].gd.cid = CID_PrevDef;
	gcd[j].gd.handle_controlevent = PI_PrevDefChanged;
	gcd[j].creator = GCheckBoxCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Offset;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[3].gd.pos.x+10; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+18+4; 
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevXOff;
	gcd[j].gd.handle_controlevent = PI_PrevChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevYOff;
	gcd[j].gd.handle_controlevent = PI_PrevChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Dist;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[3].gd.pos.x+10; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28; 
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevR;
	gcd[j].gd.handle_controlevent = PI_PrevChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 60;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_PrevTheta;
	gcd[j].gd.handle_controlevent = PI_PrevChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Degree;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-1].gd.pos.x+gcd[j-1].gd.pos.width+2; gcd[j].gd.pos.y = gcd[j-3].gd.pos.y; 
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[3].gd.pos.y-5;
	gcd[j].gd.pos.width = PI_Width-10; gcd[j].gd.pos.height = 70;
	gcd[j].gd.flags = gg_enabled | gg_visible;
	gcd[j].creator = GGroupCreate;
	++j;

	nextstarty = gcd[j-3].gd.pos.y+34;
	label[j].text = (unichar_t *) _STR_NextCP;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[3].gd.pos.x; gcd[j].gd.pos.y = nextstarty; 
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[j].gd.box = &nextcp;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = nextstarty;  gcd[j].gd.pos.width = 65;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextPos;
	gcd[j].creator = GLabelCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Default;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = defxpos; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-3;
	gcd[j].gd.flags = (gg_enabled|gg_visible);
	gcd[j].gd.cid = CID_NextDef;
	gcd[j].gd.handle_controlevent = PI_NextDefChanged;
	gcd[j].creator = GCheckBoxCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Offset;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[3].gd.pos.x+10; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+18+4; 
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextXOff;
	gcd[j].gd.handle_controlevent = PI_NextChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextYOff;
	gcd[j].gd.handle_controlevent = PI_NextChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Dist;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[3].gd.pos.x+10; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28; 
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4; gcd[j].gd.pos.width = 70;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextR;
	gcd[j].gd.handle_controlevent = PI_NextChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	gcd[j].gd.pos.x = 137; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y; gcd[j].gd.pos.width = 60;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NextTheta;
	gcd[j].gd.handle_controlevent = PI_NextChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Degree;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-1].gd.pos.x+gcd[j-1].gd.pos.width+2; gcd[j].gd.pos.y = gcd[j-3].gd.pos.y; 
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = nextstarty-5;
	gcd[j].gd.pos.width = PI_Width-10; gcd[j].gd.pos.height = 70;
	gcd[j].gd.flags = gg_enabled | gg_visible;
	gcd[j].creator = GGroupCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Type;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[0].gd.pos.x; gcd[j].gd.pos.y = gcd[j-3].gd.pos.y+32;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	label[j].image = &GIcon_midcurve;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 60; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-2;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Curve;
	gcd[j].gd.handle_controlevent = PI_PTypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].image = &GIcon_midcorner;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 100; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Corner;
	gcd[j].gd.handle_controlevent = PI_PTypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].image = &GIcon_midtangent;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 140; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Tangent;
	gcd[j].gd.handle_controlevent = PI_PTypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	hgcd[0].gd.pos.x = 5; hgcd[0].gd.pos.y = 5;
	hgcd[0].gd.pos.width = PI_Width-20; hgcd[0].gd.pos.height = gcd[j-1].gd.pos.y+10;
	hgcd[0].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
	hgcd[0].gd.cid = CID_HintMask;
	hgcd[0].gd.u.list = SCHintList(cv->sc,NULL);
	hgcd[0].gd.handle_controlevent = PI_HintSel;
	hgcd[0].creator = GListCreate;

	h2gcd[0].gd.pos.x = 5; h2gcd[0].gd.pos.y = 5;
	h2gcd[0].gd.pos.width = PI_Width-20; h2gcd[0].gd.pos.height = gcd[j-1].gd.pos.y+10;
	h2gcd[0].gd.flags = gg_visible | gg_list_multiplesel;
	h2gcd[0].gd.cid = CID_ActiveHints;
	h2gcd[0].gd.u.list = SCHintList(cv->sc,NULL);
	h2gcd[0].creator = GListCreate;

	j = 0;

	aspects[j].text = (unichar_t *) _STR_Location;
	aspects[j].text_in_resource = true;
	aspects[j++].gcd = gcd;

	aspects[j].text = (unichar_t *) _STR_HintMask;
	aspects[j].text_in_resource = true;
	aspects[j++].gcd = hgcd;

	aspects[j].text = (unichar_t *) _STR_ActiveHints;
	aspects[j].text_in_resource = true;
	aspects[j++].gcd = h2gcd;

	j = 0;

	mgcd[j].gd.pos.x = 4; mgcd[j].gd.pos.y = 6;
	mgcd[j].gd.pos.width = PI_Width-8;
	mgcd[j].gd.pos.height = hgcd[0].gd.pos.height+10+24;
	mgcd[j].gd.u.tabs = aspects;
	mgcd[j].gd.flags = gg_visible | gg_enabled;
	mgcd[j].gd.handle_controlevent = PI_AspectChange;
	mgcd[j].gd.cid = CID_TabSet;
	mgcd[j++].creator = GTabSetCreate;

	mgcd[j].gd.pos.x = (PI_Width-2*50-10)/2; mgcd[j].gd.pos.y = mgcd[j-1].gd.pos.y+mgcd[j-1].gd.pos.height+5;
	mgcd[j].gd.pos.width = 53; mgcd[j].gd.pos.height = 0;
	mgcd[j].gd.flags = gg_visible | gg_enabled;
	mlabel[j].text = (unichar_t *) _STR_PrevArrow;
	mlabel[j].text_in_resource = true;
	mgcd[j].gd.mnemonic = 'P';
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.handle_controlevent = PI_Prev;
	mgcd[j].creator = GButtonCreate;
	++j;

	mgcd[j].gd.pos.x = PI_Width-50-(PI_Width-2*50-10)/2; mgcd[j].gd.pos.y = mgcd[j-1].gd.pos.y;
	mgcd[j].gd.pos.width = 53; mgcd[j].gd.pos.height = 0;
	mgcd[j].gd.flags = gg_visible | gg_enabled;
	mlabel[j].text = (unichar_t *) _STR_NextArrow;
	mlabel[j].text_in_resource = true;
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.mnemonic = 'N';
	mgcd[j].gd.handle_controlevent = PI_Next;
	mgcd[j].creator = GButtonCreate;
	++j;

	mgcd[j].gd.pos.x = 5; mgcd[j].gd.pos.y = mgcd[j-1].gd.pos.y+30;
	mgcd[j].gd.pos.width = PI_Width-10;
	mgcd[j].gd.flags = gg_enabled|gg_visible;
	mgcd[j].creator = GLineCreate;
	++j;

	mgcd[j].gd.pos.x = 20-3; mgcd[j].gd.pos.y = PI_Height-35-3;
	mgcd[j].gd.pos.width = -1; mgcd[j].gd.pos.height = 0;
	mgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_default;
	mlabel[j].text = (unichar_t *) _STR_OK;
	mlabel[j].text_in_resource = true;
	mgcd[j].gd.mnemonic = 'O';
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.handle_controlevent = PI_Ok;
	mgcd[j].creator = GButtonCreate;
	++j;

	mgcd[j].gd.pos.x = -20; mgcd[j].gd.pos.y = PI_Height-35;
	mgcd[j].gd.pos.width = -1; mgcd[j].gd.pos.height = 0;
	mgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	mlabel[j].text = (unichar_t *) _STR_Cancel;
	mlabel[j].text_in_resource = true;
	mgcd[j].gd.label = &mlabel[j];
	mgcd[j].gd.mnemonic = 'C';
	mgcd[j].gd.handle_controlevent = PI_Cancel;
	mgcd[j].creator = GButtonCreate;
	++j;

	mgcd[j].gd.pos.x = 2; mgcd[j].gd.pos.y = 2;
	mgcd[j].gd.pos.width = pos.width-4; mgcd[j].gd.pos.height = pos.height-4;
	mgcd[j].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	mgcd[j].creator = GGroupCreate;
	++j;

	GGadgetsCreate(gi.gw,mgcd);
	GTextInfoListFree(hgcd[0].gd.u.list);
	GTextInfoListFree(h2gcd[0].gd.u.list);

	PIChangePoint(&gi);

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

void CVPGetInfo(CharView *cv) {

    if ( cv->p.ref!=NULL )
	RefGetInfo(cv,cv->p.ref);
    else if ( cv->p.img!=NULL )
	ImgGetInfo(cv,cv->p.img);
    else if ( cv->p.ap!=NULL )
	ApGetInfo(cv,cv->p.ap);
    else if ( cv->p.sp!=NULL )
	PointGetInfo(cv,cv->p.sp,cv->p.spl);
}

void SCRefBy(SplineChar *sc) {
    int cnt,i,tot=0;
    unichar_t **deps = NULL;
    struct splinecharlist *d;
#if defined(FONTFORGE_CONFIG_GDRAW)
    static int buts[] = { _STR_Show, _STR_Cancel };
#elif defined(FONTFORGE_CONFIG_GTK)
    int buts[3];
    buts[0] = _("Show");
    buts[1] = GTK_STOCK_CANCEL;
    buts[2] = NULL;
#endif

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

#if defined(FONTFORGE_CONFIG_GDRAW)
    i = GWidgetChoicesBR(_STR_Dependents,(const unichar_t **) deps, cnt, 0, buts, _STR_Dependents );
#elif defined(FONTFORGE_CONFIG_GTK)
    i = gwwv_choose_with_buttons(_("Dependents"),(const char **) deps, cnt, 0, buts, _("Dependents") );
#endif
    if ( i!=-1 ) {
	i = tot-i;
	for ( d = sc->dependents, cnt=0; d!=NULL && cnt<i; d=d->next, ++cnt );
	CharViewCreate(d->sc,sc->parent->fv);
    }
    for ( i=0; i<=tot; ++i )
	free( deps[i] );
    free(deps);
}

static int UsedIn(char *name, char *subs) {
    int nlen = strlen( name );
    while ( *subs!='\0' ) {
	if ( strncmp(subs,name,nlen)==0 && (subs[nlen]==' ' || subs[nlen]=='\0'))
return( true );
	while ( *subs!=' ' && *subs!='\0' ) ++subs;
	while ( *subs==' ' ) ++subs;
    }
return( false );
}

int SCUsedBySubs(SplineChar *sc) {
    int k, i;
    SplineFont *_sf, *sf;
    PST *pst;

    if ( sc==NULL )
return( false );

    _sf = sc->parent;
    if ( _sf->cidmaster!=NULL ) _sf=_sf->cidmaster;
    k=0;
    do {
	sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    for ( pst=sf->chars[i]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type==pst_substitution || pst->type==pst_alternate ||
			pst->type==pst_multiple || pst->type==pst_ligature )
		    if ( UsedIn(sc->name,pst->u.mult.components))
return( true );
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
return( false );
}

void SCSubBy(SplineChar *sc) {
    int i,j,k,tot;
    unichar_t **deps = NULL;
    SplineChar **depsc;
    unichar_t ubuf[100];
    SplineFont *sf, *_sf;
    PST *pst;
#if defined(FONTFORGE_CONFIG_GDRAW)
    static int buts[] = { _STR_Show, _STR_Cancel };
#elif defined(FONTFORGE_CONFIG_GTK)
    int buts[3];
    buts[0] = _("Show");
    buts[1] = GTK_STOCK_CANCEL;
    buts[2] = NULL;
#endif

    if ( sc==NULL )
return;

    _sf = sc->parent;
    if ( _sf->cidmaster!=NULL ) _sf=_sf->cidmaster;
    for ( j=0; j<2; ++j ) {
	tot = 0;
	k=0;
	do {
	    sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
		for ( pst=sf->chars[i]->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->type==pst_substitution || pst->type==pst_alternate ||
			    pst->type==pst_multiple || pst->type==pst_ligature )
			if ( UsedIn(sc->name,pst->u.mult.components)) {
			    if ( deps!=NULL ) {
				u_snprintf(ubuf,sizeof(ubuf)/sizeof(ubuf[0]),
#if defined(FONTFORGE_CONFIG_GDRAW)
					GStringGetResource(_STR_SubsInGlyph,NULL),
#elif defined(FONTFORGE_CONFIG_GTK)
					_("'%c%c%c%c' in glyph %.40s"),
#endif
			                pst->tag>>24, (pst->tag>>16)&0xff,
			                (pst->tag>>8)&0xff, pst->tag&0xff,
			                sf->chars[i]->name);
				deps[tot] = u_copy(ubuf);
			        depsc[tot] = sf->chars[i];
			    }
			    ++tot;
			}
		}
	    }
	    ++k;
	} while ( k<_sf->subfontcnt );
	if ( tot==0 )
return;
	if ( j==0 ) {
	    deps = gcalloc(tot+1,sizeof(unichar_t *));
	    depsc = galloc(tot*sizeof(SplineChar *));
	}
    }

#if defined(FONTFORGE_CONFIG_GDRAW)
    i = GWidgetChoicesBR(_STR_DependentSubstitutions,(const unichar_t **) deps, tot, 0, buts, _STR_DependentSubstitutions );
#elif defined(FONTFORGE_CONFIG_GTK)
    i = gwwv_choose_with_buttons(_("Dependent Substitutions",(const char **) deps, tot, 0, buts, _("Dependent Substitutions") );
#endif
    if ( i>-1 ) {
	CharViewCreate(depsc[i],sc->parent->fv);
    }
    for ( i=0; i<=tot; ++i )
	free( deps[i] );
    free(deps);
    free(depsc);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
