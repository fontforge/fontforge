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
#include "pfaeditui.h"
#include "gwidget.h"
#include "ustring.h"
#include <math.h>

struct problems {
    FontView *fv;
    CharView *cv;
    unsigned int openpaths: 1;
    unsigned int pointstooclose: 1;
    unsigned int missingextrema: 1;
    unsigned int done: 1;
};

static int openpaths=1, pointstooclose=1, missing=0;

#define CID_OpenPaths		1001
#define CID_PointsTooClose	1002
#define CID_MissingExtrema	1003

static int SCProblems(CharView *cv,SplineChar *sc,struct problems *p) {
    SplineSet *spl, *test;
    Spline *spline, *first;
    SplinePoint *sp, *nsp;
    int needsupdate=false, changed=false;

    if ( cv!=NULL ) {
	needsupdate = CVClearSel(cv);
	spl = *cv->heads[cv->drawmode];
	sc = cv->sc;
    } else {
	for ( spl = sc->splines; spl!=NULL; spl = spl->next ) {
	    if ( spl->first->selected ) { needsupdate = true; spl->first->selected = false; }
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( spline->to->selected )
		    { needsupdate = true; spline->to->selected = false; }
		if ( first==NULL ) first = spline;
	    }
	}
	spl = sc->splines;
    }

    if ( p->openpaths ) {
	for ( test=spl; test!=NULL; test=test->next ) {
	    if ( test->first!=NULL && test->first->prev==NULL ) {
		sp = test->first;
		changed = true;
		while ( 1 ) {
		    sp->selected = true;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		}
	    }
	}
    }

    if ( p->pointstooclose ) {
	for ( test=spl; test!=NULL; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->next==NULL )
	    break;
		nsp = sp->next->to;
		if ( (nsp->me.x-sp->me.x)*(nsp->me.x-sp->me.x) + (nsp->me.y-sp->me.y)*(nsp->me.y-sp->me.y) < 2*2 ) {
		    changed = true;
		    sp->selected = nsp->selected = true;
		}
		sp = nsp;
	    } while ( sp!=test->first );
	}
    }

    if ( p->missingextrema ) {
	for ( test=spl; test!=NULL; test = test->next ) {
	    first = NULL;
	    for ( spline = test->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( !spline->islinear ) {
		    double t1, t2, t3, t4;
		    SplineFindInflections(&spline->splines[0],&t1,&t2);
		    SplineFindInflections(&spline->splines[1],&t3,&t4);
		    if (( t1>0 && t1<1 ) || (t2>0 && t2<1 ) || (t3>0 && t3<1) ||
			    (t4>0 && t4<1)) {
			spline->from->selected = true;
			spline->to->selected = true;
			changed = true;
		    }
		}
		if ( first==NULL ) first = spline;
	    }
	}
    }

    if ( needsupdate || changed )
	SCUpdateAll(sc);
return( changed );
}

static void DoProbs(struct problems *p) {
    int i, ret;
    SplineChar *sc;

    if ( p->cv!=NULL )
	ret = SCProblems(p->cv,NULL,p);
    else {
	ret = false;
	for ( i=0; i<p->fv->sf->charcnt; ++i )
	    if ( p->fv->selected[i] && (sc = p->fv->sf->chars[i])!=NULL ) {
		if ( SCProblems(NULL,sc,p)) {
		    if ( sc->views!=NULL )
			GDrawRaise(sc->views->gw);
		    else
			CharViewCreate(sc,p->fv);
		    ret = true;
		}
	    }
    }
    if ( !ret )
	GDrawError( "No problems found");
}

static int Prob_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct problems *p = GDrawGetUserData(gw);
	openpaths = p->openpaths = GGadgetIsChecked(GWidgetGetControl(gw,CID_OpenPaths));
	pointstooclose = p->pointstooclose = GGadgetIsChecked(GWidgetGetControl(gw,CID_PointsTooClose));
	missing = p->missingextrema = GGadgetIsChecked(GWidgetGetControl(gw,CID_MissingExtrema));
	GDrawSetVisible(gw,false);
	if ( openpaths || pointstooclose || missing ) {
	    DoProbs(p);
	}
	p->done = true;
    }
return( true );
}

static int Prob_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct problems *p = GDrawGetUserData(GGadgetGetWindow(g));
	p->done = true;
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct problems *p = GDrawGetUserData(gw);
	p->done = true;
    }
return( event->type!=et_char );
}

void FindProblems(FontView *fv,CharView *cv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7];
    GTextInfo label[7];
    struct problems p;
    static unichar_t title[] = { 'P','r','o','b','l','e','m','s',  '\0' };
    static unichar_t op[] = { 'A','l','l',' ','p','a','t','h','s',' ','s','h','o','u','l','d',' ','b','e',' ','c','l','o','s','e','d',',',' ','t','h','e','r','e',' ','s','h','o','u','l','d',' ','b','e',' ','n','o',' ','e','x','p','o','s','e','d',' ','e','n','d','p','o','i','n','t','s','.',  '\0' };
    static unichar_t pc[] = { 'I','f',' ','t','w','o',' ','a','d','j','a','c','e','n','t',' ','p','o','i','n','t','s',' ','o','n',' ','t','h','e',' ','s','a','m','e',' ','p','a','t','h',' ','a','r','e',' ','l','e','s','s',' ','t','h','a','n',' ','a',' ','f','e','w','\n','e','m','-','u','n','i','t','s',' ','a','p','a','r','t',' ','t','h','e','y',' ','w','i','l','l',' ','c','a','u','s','e',' ','p','r','o','b','l','e','m','s',' ','f','o','r',' ','s','o','m','e',' ','o','f',' ','p','f','a','e','d','i','t','\'','s','\n','c','o','m','m','a','n','d','s','.',' ','P','o','s','t','S','c','r','i','p','t',' ','s','h','o','u','l','d','n','\'','t',' ','c','a','r','e',' ','t','h','o','u','g','h','.',  '\0' };
    static unichar_t me[] = { 'G','h','o','s','t','v','i','e','w',' ','(','p','e','r','h','a','p','s',' ','o','t','h','e','r',' ','i','n','t','e','r','p','r','e','t','e','r','s','?',')',' ','h','a','s',' ','a',' ','p','r','o','b','l','e','m',' ','w','h','e','n',' ','a','\n','h','i','n','t',' ','e','x','i','s','t','s',' ','w','i','t','h','o','u','t',' ','a','n','y',' ','p','o','i','n','t','s',' ','t','h','a','t',' ','l','i','e',' ','o','n',' ','i','t','.',' ','U','s','u','a','l','l','y',' ','t','h','i','s',' ','i','s',' ','b','e','c','a','u','s','e','\n','t','h','e','r','e',' ','a','r','e',' ','n','o',' ','p','o','i','n','t','s',' ','a','t',' ','t','h','e',' ','e','x','t','r','e','m','a','.',  '\0' };

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,150);
    pos.height = GDrawPointsToPixels(NULL,104);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&p,0,sizeof(p));

    p.fv = fv; p.cv=cv;

    label[0].text = (unichar_t *) "Open Paths";
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.mnemonic = 'P';
    gcd[0].gd.pos.x = 6; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.flags = gg_visible | gg_enabled;
    if ( openpaths ) gcd[0].gd.flags |= gg_cb_on;
    gcd[0].gd.popup_msg = op;
    gcd[0].gd.cid = CID_OpenPaths;
    gcd[0].creator = GCheckBoxCreate;

    label[1].text = (unichar_t *) "Points too close";
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.mnemonic = 't';
    gcd[1].gd.pos.x = 6; gcd[1].gd.pos.y = gcd[0].gd.pos.y+18; 
    gcd[1].gd.flags = gg_visible | gg_enabled;
    if ( pointstooclose ) gcd[1].gd.flags |= gg_cb_on;
    gcd[1].gd.popup_msg = pc;
    gcd[1].gd.cid = CID_PointsTooClose;
    gcd[1].creator = GCheckBoxCreate;

    label[2].text = (unichar_t *) "Missing extrema";
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'E';
    gcd[2].gd.pos.x = 6; gcd[2].gd.pos.y = gcd[1].gd.pos.y+18; 
    gcd[2].gd.flags = gg_visible | gg_enabled;
    if ( missing ) gcd[2].gd.flags |= gg_cb_on;
    gcd[2].gd.popup_msg = me;
    gcd[2].gd.cid = CID_MissingExtrema;
    gcd[2].creator = GCheckBoxCreate;

    gcd[3].gd.pos.x = 15-3; gcd[3].gd.pos.y = gcd[2].gd.pos.y+27;
    gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[3].text = (unichar_t *) _STR_OK;
    label[3].text_in_resource = true;
    gcd[3].gd.mnemonic = 'O';
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = Prob_OK;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 150-GIntGetResource(_NUM_Buttonsize)-15; gcd[4].gd.pos.y = gcd[3].gd.pos.y+3;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[4].text = (unichar_t *) _STR_Cancel;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.mnemonic = 'C';
    gcd[4].gd.handle_controlevent = Prob_Cancel;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = 2; gcd[5].gd.pos.y = 2;
    gcd[5].gd.pos.width = pos.width-4; gcd[5].gd.pos.height = pos.height-2;
    gcd[5].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[5].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);

    GDrawSetVisible(gw,true);
    while ( !p.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
