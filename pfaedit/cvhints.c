/* Copyright (C) 2000,2001 by George Williams */
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
#include "ustring.h"

int CVTwoForePointsSelected(CharView *cv, SplinePoint **sp1, SplinePoint **sp2) {
    SplineSet *spl;
    SplinePoint *test, *sps[2], *first;
    int cnt;

    if ( sp1!=NULL ) { *sp1 = NULL; *sp2 = NULL; }
    if ( cv->drawmode!=dm_fore )
return( false ) ;
    cnt = 0;
    for ( spl = cv->sc->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( test = spl->first; test!=first; test = test->next->to ) {
	    if ( test->selected ) {
		if ( cnt>=2 )
return( false );
		sps[cnt++] = test;
	    }
	    if ( first == NULL ) first = test;
	    if ( test->next==NULL )
	break;
	}
    }
    if ( cnt==2 ) {
	if ( sp1!=NULL ) { *sp1 = sps[0]; *sp2 = sps[1]; }
return( true );
    }
return( false );
}

#define CID_Base	1001
#define CID_Width	1002
#define CID_Label	1003
#define CID_HStem	1004
#define CID_VStem	1005
#define CID_Next	1006
#define CID_Prev	1007
#define CID_Remove	1008
#define CID_Add		1009

typedef struct reviewhintdata {
    unsigned int done: 1;
    unsigned int ishstem: 1;
    CharView *cv;
    GWindow gw;
    Hints *active;
    Hints *oldh, *oldv;
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
	    GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Prev),hd->active!=hd->cv->sc->hstem);
	else
	    GGadgetSetEnabled(GWidgetGetControl(hd->gw,CID_Prev),hd->active!=hd->cv->sc->vstem);
    }
    GDrawRequestExpose(hd->gw,NULL,false);
}

static void RH_SetupHint(ReviewHintData *hd) {
    char buffer[20]; unichar_t ubuf[20];
    static unichar_t nullstr[] = {'\0'};
    if ( hd->active==NULL ) {
	GGadgetSetTitle(GWidgetGetControl(hd->gw,CID_Base),nullstr);
	GGadgetSetTitle(GWidgetGetControl(hd->gw,CID_Width),nullstr);
    } else {
	sprintf( buffer,"%g", hd->active->base );
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(hd->gw,CID_Base),ubuf);
	sprintf( buffer,"%g", hd->active->width );
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(hd->gw,CID_Width),ubuf);
    }
    RH_SetNextPrev(hd);
}

static int RH_TextChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( hd->active!=NULL ) {
	    int cid = GGadgetGetCid(g);
	    int err=0;
	    int val = GetInt(hd->gw,cid,cid==CID_Base?"Base":"Width",&err);
	    if ( err )
return( true );
	    if ( cid==CID_Base )
		hd->active->base = val;
	    else
		hd->active->width = val;
	    SCOutOfDateBackground(hd->cv->sc);
	    SCUpdateAll(hd->cv->sc);
	}
    }
return( true );
}

static int RH_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	HintsFree(hd->oldh);
	HintsFree(hd->oldv);
	/* Everything else got done as we went along... */
	hd->done = true;
    }
return( true );
}

static void DoCancel(ReviewHintData *hd) {
    HintsFree(hd->cv->sc->hstem);
    HintsFree(hd->cv->sc->vstem);
    hd->cv->sc->hstem = hd->oldh;
    hd->cv->sc->vstem = hd->oldv;
    SCOutOfDateBackground(hd->cv->sc);
    SCUpdateAll(hd->cv->sc);
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
	Hints *prev;
	if ( hd->active==NULL )
return( true );			/* Eh? */
	if ( hd->active==hd->cv->sc->hstem ) {
	    hd->cv->sc->hstem = hd->active->next;
	    prev = hd->cv->sc->hstem;
	} else if ( hd->active==hd->cv->sc->vstem ) {
	    hd->cv->sc->vstem = hd->active->next;
	    prev = hd->cv->sc->vstem;
	} else {
	    prev = hd->ishstem ? hd->cv->sc->hstem : hd->cv->sc->vstem;
	    for ( ; prev->next!=hd->active && prev->next!=NULL; prev = prev->next );
	    prev->next = hd->active->next;
	}
	free( hd->active );
	hd->active = prev;
	SCOutOfDateBackground(hd->cv->sc);
	SCUpdateAll(hd->cv->sc);
	RH_SetupHint(hd);
    }
return( true );
}

static int RH_Add(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	CVCreateHint(hd->cv,hd->ishstem);
	hd->active = hd->ishstem ? hd->cv->sc->hstem : hd->cv->sc->vstem;
	RH_SetupHint(hd);
    }
return( true );
}

static int RH_NextPrev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ReviewHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	Hints *prev;
	if ( GGadgetGetCid(g)==CID_Next ) {
	    if ( hd->active->next !=NULL )
		hd->active = hd->active->next;
	} else {
	    prev = hd->ishstem ? hd->cv->sc->hstem : hd->cv->sc->vstem;
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
	hd->active =  hd->ishstem ? hd->cv->sc->hstem : hd->cv->sc->vstem;
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
    GGadgetCreateData gcd[15];
    GTextInfo label[15];
    static ReviewHintData hd;
    static unichar_t title[] = { 'R','e','v','i','e','w',' ','H','i','n','t','s','.','.','.',  '\0' };

    hd.done = false;
    hd.cv = cv;

    if ( hd.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = title;
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,170);
	pos.height = GDrawPointsToPixels(NULL,150);
	hd.gw = gw = GDrawCreateTopWindow(NULL,&pos,rh_e_h,&hd,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) "Base:";
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 17+5+6; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 37; gcd[1].gd.pos.y = 17+5;  gcd[1].gd.pos.width = 40;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_Base;
	gcd[1].gd.handle_controlevent = RH_TextChanged;
	gcd[1].creator = GTextFieldCreate;

	label[2].text = (unichar_t *) "Size:";
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 90; gcd[2].gd.pos.y = 17+5+6; 
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].creator = GLabelCreate;

	gcd[3].gd.pos.x = 120; gcd[3].gd.pos.y = 17+5;  gcd[3].gd.pos.width = 40;
	gcd[3].gd.flags = gg_enabled|gg_visible;
	gcd[3].gd.cid = CID_Width;
	gcd[3].gd.handle_controlevent = RH_TextChanged;
	gcd[3].creator = GTextFieldCreate;

	gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 17+37+60;
	gcd[4].gd.pos.width = 55+6; gcd[4].gd.pos.height = 0;
	gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[4].text = (unichar_t *) "OK";
	label[4].text_is_1byte = true;
	gcd[4].gd.mnemonic = 'O';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.handle_controlevent = RH_OK;
	gcd[4].creator = GButtonCreate;

	gcd[5].gd.pos.x = 170-55-20; gcd[5].gd.pos.y = 17+37+3+60;
	gcd[5].gd.pos.width = 55; gcd[5].gd.pos.height = 0;
	gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[5].text = (unichar_t *) "Cancel";
	label[5].text_is_1byte = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.mnemonic = 'C';
	gcd[5].gd.handle_controlevent = RH_Cancel;
	gcd[5].creator = GButtonCreate;

	label[6].text = (unichar_t *) "HStem";
	label[6].text_is_1byte = true;
	gcd[6].gd.label = &label[6];
	gcd[6].gd.pos.x = 3; gcd[6].gd.pos.y = 2; 
	gcd[6].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[6].gd.cid = CID_HStem;
	gcd[6].gd.handle_controlevent = RH_HVStem;
	gcd[6].creator = GRadioCreate;

	label[7].text = (unichar_t *) "VStem";
	label[7].text_is_1byte = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.pos.x = 60; gcd[7].gd.pos.y = 2; 
	gcd[7].gd.flags = gg_enabled|gg_visible;
	gcd[7].gd.cid = CID_VStem;
	gcd[7].gd.handle_controlevent = RH_HVStem;
	gcd[7].creator = GRadioCreate;

	gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = 17+31+30;
	gcd[8].gd.pos.width = 170-10;
	gcd[8].gd.flags = gg_enabled|gg_visible;
	gcd[8].creator = GLineCreate;

	gcd[9].gd.pos.x = 20; gcd[9].gd.pos.y = 17+33;
	gcd[9].gd.pos.width = 55; gcd[9].gd.pos.height = 0;
	gcd[9].gd.flags = gg_visible | gg_enabled;
	label[9].text = (unichar_t *) "Create";
	label[9].text_is_1byte = true;
	gcd[9].gd.mnemonic = 'e';
	gcd[9].gd.label = &label[9];
	gcd[9].gd.cid = CID_Add;
	gcd[9].gd.handle_controlevent = RH_Add;
	gcd[9].creator = GButtonCreate;

	gcd[10].gd.pos.x = 170-55-20; gcd[10].gd.pos.y = 17+33;
	gcd[10].gd.pos.width = 55; gcd[10].gd.pos.height = 0;
	gcd[10].gd.flags = gg_visible | gg_enabled;
	label[10].text = (unichar_t *) "Remove";
	label[10].text_is_1byte = true;
	gcd[10].gd.label = &label[10];
	gcd[10].gd.mnemonic = 'R';
	gcd[10].gd.cid = CID_Remove;
	gcd[10].gd.handle_controlevent = RH_Remove;
	gcd[10].creator = GButtonCreate;

	gcd[11].gd.pos.x = 20; gcd[11].gd.pos.y = 17+37+30;
	gcd[11].gd.pos.width = 55; gcd[11].gd.pos.height = 0;
	gcd[11].gd.flags = gg_visible | gg_enabled;
	label[11].text = (unichar_t *) "< Prev";
	label[11].text_is_1byte = true;
	gcd[11].gd.mnemonic = 'P';
	gcd[11].gd.label = &label[11];
	gcd[11].gd.cid = CID_Prev;
	gcd[11].gd.handle_controlevent = RH_NextPrev;
	gcd[11].creator = GButtonCreate;

	gcd[12].gd.pos.x = 170-55-20; gcd[12].gd.pos.y = 17+37+30;
	gcd[12].gd.pos.width = 55; gcd[12].gd.pos.height = 0;
	gcd[12].gd.flags = gg_visible | gg_enabled;
	label[12].text = (unichar_t *) "Next >";
	label[12].text_is_1byte = true;
	gcd[12].gd.label = &label[12];
	gcd[12].gd.mnemonic = 'N';
	gcd[12].gd.cid = CID_Next;
	gcd[12].gd.handle_controlevent = RH_NextPrev;
	gcd[12].creator = GButtonCreate;

	GGadgetsCreate(gw,gcd);
    } else
	gw = hd.gw;
    if ( cv->sc->hstem==NULL && cv->sc->vstem==NULL )
	hd.active = NULL;
    else if ( cv->sc->hstem!=NULL && cv->sc->vstem!=NULL ) {
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_HStem)))
	    hd.active = cv->sc->hstem;
	else
	    hd.active = cv->sc->vstem;
    } else if ( cv->sc->hstem!=NULL ) {
	GGadgetSetChecked(GWidgetGetControl(gw,CID_HStem),true);
	hd.active = cv->sc->hstem;
    } else {
	GGadgetSetChecked(GWidgetGetControl(gw,CID_VStem),true);
	hd.active = cv->sc->vstem;
    }
    hd.ishstem = (hd.active==cv->sc->hstem);
    hd.oldh = HintsCopy(cv->sc->hstem);
    hd.oldv = HintsCopy(cv->sc->vstem);
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
    CharView *cv;
    GWindow gw;
} CreateHintData;

static int CH_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CreateHintData *hd = GDrawGetUserData(GGadgetGetWindow(g));
	int base, width;
	int err = 0;
	Hints *h;

	base = GetInt(hd->gw,CID_Base,"Base",&err);
	width = GetInt(hd->gw,CID_Width,"Size",&err);
	if ( err )
return(true);
	h = calloc(1,sizeof(Hints));
	h->base = base;
	h->width = width;
	if ( hd->ishstem ) {
	    h->next = hd->cv->sc->hstem;
	    hd->cv->sc->hstem = h;
	} else {
	    h->next = hd->cv->sc->vstem;
	    hd->cv->sc->vstem = h;
	}
	SCOutOfDateBackground(hd->cv->sc);
	SCUpdateAll(hd->cv->sc);
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

void CVCreateHint(CharView *cv,int ishstem) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9];
    GTextInfo label[9];
    static CreateHintData chd;
    static unichar_t title[] = { 'C','r','e','a','t','e',' ','H','i','n','t','.','.','.',  '\0' };
    static unichar_t hstem[] = { 'C','r','e','a','t','e',' ', 'H', 'o', 'r', 'i', 'z', 'o', 'n', 't', 'a', 'l', ' ', 'S', 't', 'e', 'm', ' ','H','i','n','t',  '\0' };
    static unichar_t vstem[] = { 'C','r','e','a','t','e',' ', 'V', 'e', 'r', 't', 'i', 'c', 'a', 'l', ' ', 'S', 't', 'e', 'm', ' ','H','i','n','t',  '\0' };
    char buffer[20]; unichar_t ubuf[20];

    chd.done = false;
    chd.ishstem = ishstem;
    chd.cv = cv;

    if ( chd.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = title;
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,170);
	pos.height = GDrawPointsToPixels(NULL,90);
	chd.gw = gw = GDrawCreateTopWindow(NULL,&pos,chd_e_h,&chd,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) "Base:";
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 17+5+6; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	sprintf( buffer, "%g", ishstem ? cv->p.cy : cv->p.cx );
	label[1].text = (unichar_t *) buffer;
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 37; gcd[1].gd.pos.y = 17+5;  gcd[1].gd.pos.width = 40;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_Base;
	gcd[1].creator = GTextFieldCreate;

	label[2].text = (unichar_t *) "Size:";
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 90; gcd[2].gd.pos.y = 17+5+6; 
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].creator = GLabelCreate;

	label[3].text = (unichar_t *) "60";
	label[3].text_is_1byte = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 120; gcd[3].gd.pos.y = 17+5;  gcd[3].gd.pos.width = 40;
	gcd[3].gd.flags = gg_enabled|gg_visible;
	gcd[3].gd.cid = CID_Width;
	gcd[3].creator = GTextFieldCreate;

	gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 17+37;
	gcd[4].gd.pos.width = 55; gcd[4].gd.pos.height = 0;
	gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[4].text = (unichar_t *) "OK";
	label[4].text_is_1byte = true;
	gcd[4].gd.mnemonic = 'O';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.handle_controlevent = CH_OK;
	gcd[4].creator = GButtonCreate;

	gcd[5].gd.pos.x = 170-55-20; gcd[5].gd.pos.y = 17+37+3;
	gcd[5].gd.pos.width = 55; gcd[5].gd.pos.height = 0;
	gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[5].text = (unichar_t *) "Cancel";
	label[5].text_is_1byte = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.mnemonic = 'C';
	gcd[5].gd.handle_controlevent = CH_Cancel;
	gcd[5].creator = GButtonCreate;

	label[6].text = hstem /*ishstem ? hstem : vstem*/;	/* Initialize to bigger size */
	gcd[6].gd.label = &label[6];
	gcd[6].gd.pos.x = 17; gcd[6].gd.pos.y = 5; 
	gcd[6].gd.flags = gg_enabled|gg_visible;
	gcd[6].gd.cid = CID_Label;
	gcd[6].creator = GLabelCreate;

	gcd[7].gd.pos.x = 5; gcd[7].gd.pos.y = 17+31;
	gcd[7].gd.pos.width = 170-10;
	gcd[7].gd.flags = gg_enabled|gg_visible;
	gcd[7].creator = GLineCreate;

	GGadgetsCreate(gw,gcd);
    } else {
	gw = chd.gw;
	sprintf( buffer, "%g", ishstem ? cv->p.cy : cv->p.cx );
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(gw,CID_Base),ubuf);
    }
    GGadgetSetTitle(GWidgetGetControl(gw,CID_Label),ishstem ? hstem : vstem);
    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Base));
    GTextFieldSelect(GWidgetGetControl(gw,CID_Base),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !chd.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
}
