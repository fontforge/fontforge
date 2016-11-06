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
#include "autohint.h"
#include "cvundoes.h"
#include "fontforgeui.h"
#include "ustring.h"
#include <math.h>

#define CID_Base	1001
#define CID_Width	1002
#define CID_Label	1003
#define CID_HStem	1004
#define CID_VStem	1005
#define CID_Next	1006
#define CID_Prev	1007
#define CID_Remove	1008
#define CID_Add		1009
#define CID_Overlap	1010
#define CID_Count	1011
#define CID_MovePoints	1012
#define CID_RegenHM	1013
#define CID_TopBox	1014

typedef struct reviewhintdata {
    unsigned int done: 1;
    unsigned int ishstem: 1;
    unsigned int oldmanual: 1;
    unsigned int undocreated: 1;
    unsigned int changed: 1;
    CharView *cv;
    GWindow gw;
    StemInfo *active;
    StemInfo *lastactive;
    StemInfo *oldh, *oldv;
} ReviewHintData;

static void RH_SetNextPrev(ReviewHintData *hd) {
    if ( hd->active==NULL ) {
	GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Next),false);
	GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Prev),false);
	GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Remove),false);
	GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Base),false);
	GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Width),false);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Remove),true);
	GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Base),true);
	GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Width),true);
	GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Next),hd->active->next!=NULL);
	if ( hd->ishstem )
	    GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Prev),hd->active!=hd->cv->b.sc->hstem);
	else
	    GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Prev),hd->active!=hd->cv->b.sc->vstem);
    }
    GDrawRequestExpose(hd->gw,NULL,false);
}

static void RH_SetupHint(ReviewHintData *hd) {
    char buffer[20]; unichar_t ubuf[20];
    static unichar_t nullstr[] = {'\0'};
    StemInfo *h;
    int pos,cnt;

    if ( hd->lastactive!=NULL )
	hd->lastactive->active = false;

    pos = cnt = 0;
    for ( h=hd->ishstem ? hd->cv->b.sc->hstem : hd->cv->b.sc->vstem; h!=NULL; h=h->next ) {
	++cnt;
	if ( h==hd->active ) pos=cnt;
    }
    sprintf( buffer,"%d/%d", pos, cnt );
    if ( cnt==3 ) {
	StemInfo *h2, *h3;
	h = hd->ishstem ? hd->cv->b.sc->hstem : hd->cv->b.sc->vstem;
	h2 = h->next; h3 = h2->next;
	if ( h->width == h2->width && h2->width==h3->width &&
		h2->start-h->start == h3->start-h2->start )
	    strcat( buffer, hd->ishstem ? " hstem3" : " vstem3" );
    }
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(hd->gw,CID_Count),ubuf);
    
    if ( hd->active==NULL ) {
	GGadgetSetTitle(GWidgetGetControl(hd->gw,CID_Base),nullstr);
	GGadgetSetTitle(GWidgetGetControl(hd->gw,CID_Width),nullstr);
	GGadgetSetVisible(GWidgetGetControl(hd->gw,CID_Overlap),false);
    } else {
	hd->active->active = true;
	sprintf( buffer,"%g", (double) (!hd->active->ghost ? hd->active->start : hd->active->start+hd->active->width) );
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(hd->gw,CID_Base),ubuf);
	GTextFieldShow(GWidgetGetControl(hd->gw,CID_Base),0);
	sprintf( buffer,"%g", (double) (!hd->active->ghost ? hd->active->width : -hd->active->width) );
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(hd->gw,CID_Width),ubuf);
	GTextFieldShow(GWidgetGetControl(hd->gw,CID_Width),0);
	GGadgetSetVisible(GWidgetGetControl(hd->gw,CID_Overlap),hd->active->hasconflicts);
    }
    if ( hd->lastactive!=hd->active ) {
	hd->lastactive = hd->active;
	SCOutOfDateBackground(hd->cv->b.sc);
	SCUpdateAll(hd->cv->b.sc);	/* Changing the active Hint means we should redraw everything */
    }
    RH_SetNextPrev(hd);
}

static int OnHint(StemInfo *hint,real major,real minor) {
    HintInstance *hi;

    if ( true /*hint->hasconflicts*/ ) {
	for ( hi=hint->where; hi!=NULL; hi=hi->next ) {
	    if ( minor>=hi->begin && minor<=hi->end )
	break;
	}
	if ( hi==NULL )
return( 0 );
	/* Ok, it's active */
    }
    if ( major==hint->start )
return( 1 );
    else if ( major==hint->start+hint->width )
return( 2 );

return( 0 );
}

static void RH_MovePoints(ReviewHintData *hd,StemInfo *active,int start,int width) {
    SplineChar *sc = hd->cv->b.sc;
    SplineSet *spl;
    SplinePoint *sp;
    int which;
    int layer = CVLayer( (CharViewBase *) (hd->cv));

    if ( !hd->undocreated ) {
	SCPreserveLayer(sc,layer,true);
	hd->undocreated = true;
    }

    for ( spl=sc->layers[layer].splines; spl!=NULL; spl=spl->next ) {
	for ( sp = spl->first ; ; ) {
	    if ( hd->ishstem ) {
		switch ((which = OnHint(active,sp->me.y,sp->me.x)) ) {
		  case 1:
		    sp->nextcp.y += start-sp->me.y;
		    sp->prevcp.y += start-sp->me.y;
		    sp->me.y = start;
		  break;
		  case 2:
		    sp->nextcp.y += start+width-sp->me.y;
		    sp->prevcp.y += start+width-sp->me.y;
		    sp->me.y = start+width;
		  break;
		  case 0:
		    /* Not on hint */;
		  break;
		}
	    } else {
		switch ( (which = OnHint(active,sp->me.x,sp->me.y)) ) {
		  case 1:
		    sp->nextcp.x += start-sp->me.x;
		    sp->prevcp.x += start-sp->me.x;
		    sp->me.x = start;
		  break;
		  case 2:
		    sp->nextcp.x += start+width-sp->me.x;
		    sp->prevcp.x += start+width-sp->me.x;
		    sp->me.x = start+width;
		  break;
		  case 0:
		    /* Not on hint */;
		  break;
		}
	    }
	    if ( which ) {
		if ( sp->prev )
		    SplineRefigure(sp->prev);
		if ( sp->next )
		    SplineRefigure(sp->next);
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp== spl->first )
	break;
	}
    }
}

static int RH_TextChanged(GGadget *g, GEvent *e) {
    int wasconflict;
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( hd->active!=NULL ) {
	    int cid = GGadgetGetCid(g);
	    int err=0;
	    real start = GetCalmReal8(hd->gw,CID_Base,_("Base:"),&err);
	    real width = GetCalmReal8(hd->gw,CID_Width,_("Size:"),&err);
	    if ( err )
return( true );
	    if ( GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_MovePoints)) ) {
		if ( width<0 )
		    RH_MovePoints(hd,hd->active,start+width,-width);
		else
		    RH_MovePoints(hd,hd->active,start,width);
	    }
	    if ( cid==CID_Base )
		hd->active->start = start;
	    else
		hd->active->width = width;
	    if ( width<0 ) {
		hd->active->ghost = true;
		hd->active->width = -width;
		hd->active->start = start+width;
	    } else
		hd->active->ghost = false;
	    wasconflict = hd->active->hasconflicts;
	    if ( hd->ishstem )
		hd->cv->b.sc->hconflicts = StemListAnyConflicts(hd->cv->b.sc->hstem);
	    else
		hd->cv->b.sc->vconflicts = StemListAnyConflicts(hd->cv->b.sc->vstem);
	    hd->cv->b.sc->manualhints = true;
	    hd->changed = true;
	    if ( wasconflict!=hd->active->hasconflicts ) {
		GGadgetSetVisible(GWidgetGetControl(hd->gw,CID_Overlap),hd->active->hasconflicts);
		if ( hd->active->hasconflicts )
		    GHVBoxFitWindow(GWidgetGetControl(hd->gw,CID_TopBox));
	    }
	    SCOutOfDateBackground(hd->cv->b.sc);
	    SCUpdateAll(hd->cv->b.sc);
	}
    }
return( true );
}

static void Do_OKRegen(ReviewHintData *hd) {
    SplineChar *sc = hd->cv->b.sc;
    StemInfo *curh = sc->hstem, *curv = sc->vstem;
    int do_regen = GGadgetIsChecked(GWidgetGetControl(hd->gw,CID_RegenHM));

    /* We go backwards here, but not for long. The point is to go back to */
    /*  the original hint state so we can preserve it, now that we know we*/
    /*  are going to modify it */
    sc->hstem = hd->oldh; sc->vstem = hd->oldv;
    SCPreserveHints(sc,CVLayer((CharViewBase *) hd->cv));
    sc->hstem = curh; sc->vstem = curv;

    StemInfosFree(hd->oldh);
    StemInfosFree(hd->oldv);
    if ( hd->lastactive!=NULL )
	hd->lastactive->active = false;
    if ( hd->changed ) {
	SCClearHintMasks(hd->cv->b.sc,CVLayer((CharViewBase *) (hd->cv)),true);
	if ( do_regen )
	    SCFigureHintMasks(hd->cv->b.sc,CVLayer((CharViewBase *) (hd->cv)));
    }
    /* Everything else got done as we went along... */
    SCOutOfDateBackground(hd->cv->b.sc);
    SCUpdateAll(hd->cv->b.sc);
    SCHintsChanged(hd->cv->b.sc);
    hd->done = true;
}

static int RH_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	Do_OKRegen(hd);
    }
return( true );
}

static void DoCancel(ReviewHintData *hd) {
    StemInfosFree(hd->cv->b.sc->hstem);
    StemInfosFree(hd->cv->b.sc->vstem);
    hd->cv->b.sc->hstem = hd->oldh;
    hd->cv->b.sc->vstem = hd->oldv;
    hd->cv->b.sc->hconflicts = StemListAnyConflicts(hd->cv->b.sc->hstem);
    hd->cv->b.sc->vconflicts = StemListAnyConflicts(hd->cv->b.sc->vstem);
    hd->cv->b.sc->manualhints = hd->oldmanual;
    if ( hd->undocreated )
	SCDoUndo(hd->cv->b.sc,ly_fore);
    SCOutOfDateBackground(hd->cv->b.sc);
    SCUpdateAll(hd->cv->b.sc);
    hd->done = true;
}

static int RH_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int RH_Remove(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	StemInfo *prev;
	if ( hd->active==NULL )
return( true );			/* Eh? */
	if ( hd->active==hd->cv->b.sc->hstem ) {
	    hd->cv->b.sc->hstem = hd->active->next;
	    prev = hd->cv->b.sc->hstem;
	} else if ( hd->active==hd->cv->b.sc->vstem ) {
	    hd->cv->b.sc->vstem = hd->active->next;
	    prev = hd->cv->b.sc->vstem;
	} else {
	    prev = hd->ishstem ? hd->cv->b.sc->hstem : hd->cv->b.sc->vstem;
	    for ( ; prev->next!=hd->active && prev->next!=NULL; prev = prev->next );
	    prev->next = hd->active->next;
	}
	if ( hd->ishstem )
	    hd->cv->b.sc->hconflicts = StemListAnyConflicts(hd->cv->b.sc->hstem);
	else
	    hd->cv->b.sc->vconflicts = StemListAnyConflicts(hd->cv->b.sc->vstem);
	hd->cv->b.sc->manualhints = true;
	hd->changed = true;
	StemInfoFree( hd->active );
	hd->active = prev;
	SCOutOfDateBackground(hd->cv->b.sc);
	RH_SetupHint(hd);
	/*SCUpdateAll(hd->cv->b.sc);*/	/* Done in RH_SetupHint now */
    }
return( true );
}

static int RH_Add(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	CVCreateHint(hd->cv,hd->ishstem,false);
	hd->active = hd->ishstem ? hd->cv->b.sc->hstem : hd->cv->b.sc->vstem;
	RH_SetupHint(hd);
    }
return( true );
}

static int RH_NextPrev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	StemInfo *prev;
	if ( GGadgetGetCid(g)==CID_Next ) {
	    if ( hd->active->next !=NULL )
		hd->active = hd->active->next;
	} else {
	    prev = hd->ishstem ? hd->cv->b.sc->hstem : hd->cv->b.sc->vstem;
	    for ( ; prev->next!=hd->active && prev->next!=NULL; prev = prev->next );
	    hd->active = prev;
	}
	RH_SetupHint(hd);
    }
return( true );
}

static int RH_HVStem(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	hd->ishstem = GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_HStem));
	hd->active =  hd->ishstem ? hd->cv->b.sc->hstem : hd->cv->b.sc->vstem;
	RH_SetupHint(hd);
    }
return( true );
}

static int rh_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	DoCancel( GDrawGetUserData(gw));
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void CVReviewHints(CharView *cv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[18], *harray1[6], *harray2[6], *harray3[6], *harray4[6],
	*varray[7][2], boxes[8], *barray[6][6];
    GTextInfo label[18];
    static ReviewHintData hd;
    int k;

    hd.done = false;
    hd.cv = cv;

    if ( hd.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Review Hints");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
	pos.height = GDrawPointsToPixels(NULL,178);
	hd.gw = gw = GDrawCreateTopWindow(NULL,&pos,rh_e_h,&hd,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	k=0;

	label[k].text = (unichar_t *) _("_HStem");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[k].gd.cid = CID_HStem;
	gcd[k].gd.handle_controlevent = RH_HVStem;
	gcd[k].creator = GRadioCreate;
	harray2[0] = &gcd[k++];

	label[k].text = (unichar_t *) _("_VStem");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_VStem;
	gcd[k].gd.handle_controlevent = RH_HVStem;
	gcd[k].creator = GRadioCreate;
	harray2[1] = &gcd[k++];

	label[k].text = (unichar_t *) "999/999 hstem3";
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Count;
	gcd[k].creator = GLabelCreate;
	harray2[2] = GCD_HPad10; harray2[3] = &gcd[k++]; harray2[4] = GCD_Glue; harray2[5] = NULL;

	label[k].text = (unichar_t *) _("_Move Points");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[k].gd.cid = CID_MovePoints;
	gcd[k].gd.popup_msg = (unichar_t *) _("When the hint's position is changed\nadjust the postion of any points\nwhich lie on that hint");
	gcd[k].creator = GCheckBoxCreate;
	harray3[0] = &gcd[k++]; harray3[1] = GCD_Glue; harray3[2] = NULL;

	label[k].text = (unichar_t *) _("_Base:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 14+17+5+3; 
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].creator = GLabelCreate;
	harray1[0] = &gcd[k++];

	gcd[k].gd.pos.width = 40;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Base;
	gcd[k].gd.handle_controlevent = RH_TextChanged;
	gcd[k].creator = GTextFieldCreate;
	harray1[1] = &gcd[k++];

	label[k].text = (unichar_t *) _("_Size:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].creator = GLabelCreate;
	harray1[2] = &gcd[k++];

	gcd[k].gd.pos.width = 40;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Width;
	gcd[k].gd.handle_controlevent = RH_TextChanged;
	gcd[k].creator = GTextFieldCreate;
	harray1[3] = &gcd[k++]; harray1[4] = GCD_Glue; harray1[5] = NULL;

	gcd[k].gd.flags = gg_visible | gg_enabled;
	label[k].text = (unichar_t *) "Overlaps another hint";
	label[k].text_is_1byte = true;
	label[k].fg = 0xff0000; label[k].bg = COLOR_DEFAULT;	/* Doesn't work, needs to be in box */
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Overlap;
	gcd[k].creator = GLabelCreate;
	harray4[0] = GCD_Glue; harray4[1] = &gcd[k++]; harray4[2] = GCD_Glue; harray4[3] = NULL;


	gcd[k].gd.flags = gg_visible | gg_enabled;
	label[k].text = (unichar_t *) _("Cr_eate");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Add;
	gcd[k].gd.handle_controlevent = RH_Add;
	gcd[k].creator = GButtonCreate;
	barray[0][0] = GCD_Glue; barray[0][1] = &gcd[k++]; barray[0][2] = GCD_Glue;

	gcd[k].gd.flags = gg_visible | gg_enabled;
	label[k].text = (unichar_t *) _("Re_move");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Remove;
	gcd[k].gd.handle_controlevent = RH_Remove;
	gcd[k].creator = GButtonCreate;
	barray[0][3] = &gcd[k++]; barray[0][4] = GCD_Glue; barray[0][5] = NULL;

	gcd[k].gd.pos.width = 170-10;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].creator = GLineCreate;
	barray[1][0] = GCD_Glue; barray[1][1] = &gcd[k++]; barray[1][2] = barray[1][3] = GCD_ColSpan; barray[1][4] = GCD_Glue; barray[1][5] = NULL;

	gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	label[k].text = (unichar_t *) _("< _Prev");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Prev;
	gcd[k].gd.popup_msg = (unichar_t *) _("Previous Hint.");
	gcd[k].gd.handle_controlevent = RH_NextPrev;
	gcd[k].creator = GButtonCreate;
	barray[2][0] = GCD_Glue; barray[2][1] = &gcd[k++]; barray[2][2] = GCD_Glue;

	gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	label[k].text = (unichar_t *) _("_Next >");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_Next;
	gcd[k].gd.popup_msg = (unichar_t *) _("Next Hint.");
	gcd[k].gd.handle_controlevent = RH_NextPrev;
	gcd[k].creator = GButtonCreate;
	barray[2][3] = &gcd[k++]; barray[2][4] = GCD_Glue; barray[2][5] = NULL;

	label[k].text = (unichar_t *) _("Regenerate Hint Substitution Points");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_cb_on|gg_utf8_popup;
	gcd[k].gd.cid = CID_RegenHM;
	gcd[k].gd.popup_msg = (unichar_t *) _("If you have made any changes to the hints,\nthen in addition to changing the glyph's hints\nrefigure it's hint masks and substitution points.");
	gcd[k].creator = GCheckBoxCreate;
	barray[3][0] = &gcd[k++]; barray[3][1] = barray[3][2] = barray[3][3] = barray[3][4] = GCD_ColSpan; barray[3][5] = NULL;

	gcd[k].gd.flags = gg_visible | gg_enabled;
	label[k].text = (unichar_t *) _("_OK");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = RH_OK;
	gcd[k].creator = GButtonCreate;
	barray[4][0] = GCD_Glue; barray[4][1] = &gcd[k++]; barray[4][2] = GCD_Glue;

	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[k].text = (unichar_t *) _("_Cancel");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = RH_Cancel;
	gcd[k].creator = GButtonCreate;
	barray[4][3] = &gcd[k++]; barray[4][4] = GCD_Glue; barray[4][5] = NULL;
	barray[5][0] = NULL;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray2;
	boxes[2].creator = GHBoxCreate;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = harray3;
	boxes[3].creator = GHBoxCreate;

	boxes[4].gd.flags = gg_enabled|gg_visible;
	boxes[4].gd.u.boxelements = harray1;
	boxes[4].creator = GHBoxCreate;

	boxes[5].gd.flags = gg_enabled|gg_visible;
	boxes[5].gd.u.boxelements = harray4;
	boxes[5].creator = GHBoxCreate;

	boxes[6].gd.flags = gg_enabled|gg_visible;
	boxes[6].gd.u.boxelements = barray[0];
	boxes[6].creator = GHVBoxCreate;

	varray[0][0] = &boxes[2]; varray[0][1] = NULL;
	varray[1][0] = &boxes[3]; varray[1][1] = NULL;
	varray[2][0] = &boxes[4]; varray[2][1] = NULL;
	varray[3][0] = &boxes[5]; varray[3][1] = NULL;
	varray[4][0] = &boxes[6]; varray[4][1] = NULL;
	varray[5][0] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray[0];
	boxes[0].gd.cid = CID_TopBox;
	boxes[0].creator = GHVGroupCreate;


	GGadgetsCreate(gw,boxes);

	GHVBoxSetExpandableRow(boxes[0].ret,3);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[6].ret,gb_expandglue);
	GHVBoxFitWindow(boxes[0].ret);
    } else {
	gw = hd.gw;
	GDrawSetTransientFor(gw,(GWindow) -1);
    }
    if ( cv->b.sc->hstem==NULL && cv->b.sc->vstem==NULL )
	hd.active = NULL;
    else if ( cv->b.sc->hstem!=NULL && cv->b.sc->vstem!=NULL ) {
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_HStem)))
	    hd.active = cv->b.sc->hstem;
	else
	    hd.active = cv->b.sc->vstem;
    } else if ( cv->b.sc->hstem!=NULL ) {
	GGadgetSetChecked(GWidgetGetControl(gw,CID_HStem),true);
	hd.active = cv->b.sc->hstem;
    } else {
	GGadgetSetChecked(GWidgetGetControl(gw,CID_VStem),true);
	hd.active = cv->b.sc->vstem;
    }
    hd.ishstem = (hd.active==cv->b.sc->hstem);
    hd.oldh = StemInfoCopy(cv->b.sc->hstem);
    hd.oldv = StemInfoCopy(cv->b.sc->vstem);
    hd.oldmanual = cv->b.sc->manualhints;
    hd.changed = false;
    RH_SetupHint(&hd);
    if ( hd.active!=NULL ) {
	GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Base));
	GTextFieldSelect(GWidgetGetControl(gw,CID_Base),0,-1);
    }

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !hd.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
}

typedef struct createhintdata {
    unsigned int done: 1;
    unsigned int ishstem: 1;
    unsigned int preservehints: 1;
    CharView *cv;
    GWindow gw;
} CreateHintData;

static int CH_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CreateHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	real base, width;
	int err = 0;
	StemInfo *h;
	int layer = CVLayer((CharViewBase *) hd->cv);

	base = GetReal8(hd->gw,CID_Base,_("Base:"),&err);
	width = GetReal8(hd->gw,CID_Width,_("Size:"),&err);
	if ( err )
return(true);
	if ( hd->preservehints ) {
	    SCPreserveHints(hd->cv->b.sc,layer);
	    SCHintsChanged(hd->cv->b.sc);
	}
	h = chunkalloc(sizeof(StemInfo));
	if ( width==-21 || width==-20 ) {
	    base += width;
	    width = -width;
	    h->ghost = true;
	}
	h->start = base;
	h->width = width;
	if ( hd->ishstem ) {
	    SCGuessHHintInstancesAndAdd(hd->cv->b.sc,layer,h,0x80000000,0x80000000);
	    hd->cv->b.sc->hconflicts = StemListAnyConflicts(hd->cv->b.sc->hstem);
	} else {
	    SCGuessVHintInstancesAndAdd(hd->cv->b.sc,layer,h,0x80000000,0x80000000);
	    hd->cv->b.sc->vconflicts = StemListAnyConflicts(hd->cv->b.sc->vstem);
	}
	hd->cv->b.sc->manualhints = true;
	if ( h!=NULL && hd->cv->b.sc->parent->mm==NULL )
	    SCModifyHintMasksAdd(hd->cv->b.sc,layer,h);
	else
	    SCClearHintMasks(hd->cv->b.sc,layer,true);
	SCOutOfDateBackground(hd->cv->b.sc);
	SCUpdateAll(hd->cv->b.sc);
	hd->done = true;
    }
return( true );
}

static int CH_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CreateHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	hd->done = true;
    }
return( true );
}

static int chd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	CreateHintData *hd = GDrawGetUserData(gw);
	hd->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void CVCreateHint(CharView *cv,int ishstem,int preservehints) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9], *harray1[4], *harray2[9], *barray[7], *varray[5][2], boxes[5];
    GTextInfo label[9];
    static CreateHintData chd;
    char buffer[20]; unichar_t ubuf[20];

    chd.done = false;
    chd.ishstem = ishstem;
    chd.preservehints = preservehints;
    chd.cv = cv;

    if ( chd.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Create Hint");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
	pos.height = GDrawPointsToPixels(NULL,90);
	chd.gw = gw = GDrawCreateTopWindow(NULL,&pos,chd_e_h,&chd,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	label[0].text = (unichar_t *) _("_Base:");
	label[0].text_is_1byte = true;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 17+5+6; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;
	harray2[0] = GCD_Glue; harray2[1] = &gcd[0];

	sprintf( buffer, "%g", (double) (ishstem ? cv->p.cy : cv->p.cx) );
	label[1].text = (unichar_t *) buffer;
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 37; gcd[1].gd.pos.y = 17+5;  gcd[1].gd.pos.width = 40;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_Base;
	gcd[1].creator = GTextFieldCreate;
	harray2[2] = &gcd[1];

	label[2].text = (unichar_t *) _("_Size:");
	label[2].text_is_1byte = true;
	label[2].text_in_resource = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 90; gcd[2].gd.pos.y = 17+5+6; 
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].creator = GLabelCreate;
	harray2[3] = GCD_Glue; harray2[4] = &gcd[2];

	label[3].text = (unichar_t *) "60";
	label[3].text_is_1byte = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 120; gcd[3].gd.pos.y = 17+5;  gcd[3].gd.pos.width = 40;
	gcd[3].gd.flags = gg_enabled|gg_visible;
	gcd[3].gd.cid = CID_Width;
	gcd[3].creator = GTextFieldCreate;
	harray2[5] = &gcd[3]; harray2[6] = GCD_Glue; harray2[7] = NULL;

	gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 17+37;
	gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
	gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[4].text = (unichar_t *) _("_OK");
	label[4].text_is_1byte = true;
	label[4].text_in_resource = true;
	gcd[4].gd.mnemonic = 'O';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.handle_controlevent = CH_OK;
	gcd[4].creator = GButtonCreate;
	barray[0] = GCD_Glue; barray[1] = &gcd[4]; barray[2] = GCD_Glue;

	gcd[5].gd.pos.x = -20; gcd[5].gd.pos.y = 17+37+3;
	gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
	gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[5].text = (unichar_t *) _("_Cancel");
	label[5].text_is_1byte = true;
	label[5].text_in_resource = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.mnemonic = 'C';
	gcd[5].gd.handle_controlevent = CH_Cancel;
	gcd[5].creator = GButtonCreate;
	barray[3] = GCD_Glue; barray[4] = &gcd[5]; barray[5] = GCD_Glue; barray[6] = NULL;

	label[6].text = (unichar_t *) _("Create Horizontal Stem Hint");	/* Initialize to bigger size */
	label[6].text_is_1byte = true;
	gcd[6].gd.label = &label[6];
	gcd[6].gd.pos.x = 17; gcd[6].gd.pos.y = 5; 
	gcd[6].gd.flags = gg_enabled|gg_visible;
	gcd[6].gd.cid = CID_Label;
	gcd[6].creator = GLabelCreate;
	harray1[0] = GCD_Glue; harray1[1] = &gcd[6]; harray1[2] = GCD_Glue; harray1[3] = NULL;

	gcd[7].gd.pos.x = 5; gcd[7].gd.pos.y = 17+31;
	gcd[7].gd.pos.width = 170-10;
	gcd[7].gd.flags = gg_enabled|gg_visible;
	gcd[7].creator = GLineCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray1;
	boxes[2].creator = GHBoxCreate;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = harray2;
	boxes[3].creator = GHBoxCreate;

	boxes[4].gd.flags = gg_enabled|gg_visible;
	boxes[4].gd.u.boxelements = barray;
	boxes[4].creator = GHBoxCreate;

	varray[0][0] = &boxes[2]; varray[0][1] = NULL;
	varray[1][0] = &boxes[3]; varray[1][1] = NULL;
	varray[2][0] = &gcd[7];   varray[2][1] = NULL;
	varray[3][0] = &boxes[4]; varray[3][1] = NULL;
	varray[4][0] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray[0];
	boxes[0].creator = GHVGroupCreate;
	

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);
    } else {
	gw = chd.gw;
	sprintf( buffer, "%g", (double) (ishstem ? cv->p.cy : cv->p.cx) );
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(gw,CID_Base),ubuf);
	GDrawSetTransientFor(gw,(GWindow) -1);
    }
    GGadgetSetTitle8(GWidgetGetControl(gw,CID_Label),
	    ishstem ? _("Create Horizontal Stem Hint") :
		    _("Create Vertical Stem Hint"));
    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Base));
    GTextFieldSelect(GWidgetGetControl(gw,CID_Base),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !chd.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
}

void SCRemoveSelectedMinimumDistances(SplineChar *sc,int inx) {
    /* Remove any minimum distance where at least one of the two points is */
    /*  selected */
    MinimumDistance *md, *prev, *next;
    SplineSet *ss;
    SplinePoint *sp;

    prev = NULL;
    for ( md = sc->md; md!=NULL; md = next ) {
	next = md->next;
	if ( (inx==2 || inx==md->x) &&
		((md->sp1!=NULL && md->sp1->selected) ||
		 (md->sp2!=NULL && md->sp2->selected))) {
	    if ( prev==NULL )
		sc->md = next;
	    else
		prev->next = next;
	    chunkfree(md,sizeof(MinimumDistance));
	} else
	    prev = md;
    }

    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->selected ) {
		if ( inx==2 ) sp->roundx = sp->roundy = false;
		else if ( inx==1 ) sp->roundx = false;
		else sp->roundy = false;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
}
