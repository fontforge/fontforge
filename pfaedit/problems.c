/* Copyright (C) 2001-2002 by George Williams */
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
#include <gwidget.h>
#include <ustring.h>
#include <math.h>
#include <gkeysym.h>

struct problems {
    FontView *fv;
    CharView *cv;
    SplineChar *sc;
    SplineChar *msc;
    unsigned int openpaths: 1;
    unsigned int pointstooclose: 1;
    /*unsigned int missingextrema: 1;*/
    unsigned int xnearval: 1;
    unsigned int ynearval: 1;
    unsigned int ynearstd: 1;		/* baseline, xheight, cap, ascent, descent, etc. */
    unsigned int linenearstd: 1;	/* horizontal, vertical, italicangle */
    unsigned int cpnearstd: 1;		/* control points near: horizontal, vertical, italicangle */
    unsigned int cpodd: 1;		/* control points beyond points on spline */
    unsigned int hintwithnopt: 1;
    unsigned int ptnearhint: 1;
    unsigned int hintwidthnearval: 1;
    unsigned int direction: 1;
    unsigned int flippedrefs: 1;
    unsigned int cidmultiple: 1;
    unsigned int cidblank: 1;
    unsigned int explain: 1;
    unsigned int done: 1;
    unsigned int doneexplain: 1;
    unsigned int finish: 1;
    unsigned int ignorethis: 1;
    real near, xval, yval, widthval;
    int explaining;
    real found, expected;
    real xheight, caph, ascent, descent;
    GWindow explainw;
    GGadget *explaintext, *explainvals, *ignoregadg;
    SplineChar *lastcharopened;
    CharView *cvopened;
};

static int openpaths=1, pointstooclose=1/*, missing=0*/, doxnear=0, doynear=0;
static int doynearstd=1, linestd=1, cpstd=1, cpodd=1, hintnopt=0, ptnearhint=0;
static int hintwidth=0, direction=0, flippedrefs=1;
static int cidblank=0, cidmultiple=1;
static real near=3, xval=0, yval=0, widthval=50;

#define CID_Stop		2001
#define CID_Next		2002
#define CID_Fix			2003

#define CID_OpenPaths		1001
#define CID_PointsTooClose	1002
/*#define CID_MissingExtrema	1003*/
#define CID_XNear		1004
#define CID_YNear		1005
#define CID_YNearStd		1006
#define CID_HintNoPt		1007
#define CID_PtNearHint		1008
#define CID_HintWidthNear	1009
#define CID_HintWidth		1010
#define CID_Near		1011
#define CID_XNearVal		1012
#define CID_YNearVal		1013
#define CID_LineStd		1014
#define CID_Direction		1015
#define CID_CpStd		1016
#define CID_CpOdd		1017
#define CID_CIDMultiple		1018
#define CID_CIDBlank		1019
#define CID_FlippedRefs		1020


static void FixIt(struct problems *p) {
    SplinePointList *spl;
    SplinePoint *sp;
    /*StemInfo *h;*/
    RefChar *r;

#if 0	/* The ultimate cause (the thing we need to fix) for these two errors */
	/* is that the stem is wrong, it's too hard to fix that here, so best */
	/* not to attempt to fix the proximal cause */
    if ( p->explaining==_STR_ProbHintHWidth ) {
	for ( h=p->sc->hstem; h!=NULL && !h->active; h=h->next );
	if ( h!=NULL ) {
	    h->width = p->widthval;
	    SCOutOfDateBackground(p->sc);
	    SCUpdateAll(p->sc);
	} else
	    GDrawIError("Could not find hint");
return;
    }
    if ( p->explaining==_STR_ProbHintVWidth ) {
	for ( h=p->sc->vstem; h!=NULL && !h->active; h=h->next );
	if ( h!=NULL ) {
	    h->width = p->widthval;
	    SCOutOfDateBackground(p->sc);
	    SCUpdateAll(p->sc);
	} else
	    GDrawIError("Could not find hint");
return;
    }
#endif
    if ( p->explaining==_STR_ProbFlippedRef ) {
	for ( r=p->sc->refs; r!=NULL && !r->selected; r = r->next );
	if ( r!=NULL ) {
	    SCPreserveState(p->sc,false);
	    SCRefToSplines(p->sc,r);
	    p->sc->splines = SplineSetsCorrect(p->sc->splines);
	    SCCharChangedUpdate(p->sc);
	} else
	    GDrawIError("Could not find referenc");
return;
    }

    for ( spl=p->sc->splines; spl!=NULL; spl=spl->next ) {
	for ( sp = spl->first; ; ) {
	    if ( sp->selected )
	break;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( sp->selected )
    break;
    }
    if ( sp==NULL ) {
	GDrawIError("Nothing selected");
return;
    }

/* I do not handle:
    _STR_ProbOpenPath
    _STR_ProbPointsTooClose
    _STR_ProbLineItal, _STR_ProbAboveItal, _STR_ProbBelowItal, _STR_ProbRightItal, _STR_ProbLeftItal
    _STR_ProbAboveOdd, _STR_ProbBelowOdd, _STR_ProbRightOdd, _STR_ProbLeftOdd
    _STR_ProbHintControl
*/

    SCPreserveState(p->sc,false);
    if ( p->explaining==_STR_ProbXNear || p->explaining==_STR_ProbPtNearVHint) {
	sp->prevcp.x += p->expected-sp->me.x;
	sp->nextcp.x += p->expected-sp->me.x;
	sp->me.x = p->expected;
    } else if ( p->explaining==_STR_ProbYNear || p->explaining==_STR_ProbPtNearHHint ||
	    p->explaining==_STR_ProbYBase || p->explaining==_STR_ProbYXHeight ||
	    p->explaining==_STR_ProbYCapHeight || p->explaining==_STR_ProbYAs ||
	    p->explaining==_STR_ProbYDs ) {
	sp->prevcp.y += p->expected-sp->me.y;
	sp->nextcp.y += p->expected-sp->me.y;
	sp->me.y = p->expected;
    } else if ( p->explaining==_STR_ProbLineHor ) {
	if ( sp->me.y!=p->found ) {
	    sp=sp->next->to;
	    if ( !sp->selected || sp->me.y!=p->found ) {
		GDrawIError("Couldn't find line");
return;
	    }
	}
	sp->prevcp.y += p->expected-sp->me.y;
	sp->nextcp.y += p->expected-sp->me.y;
	sp->me.y = p->expected;
    } else if ( p->explaining==_STR_ProbAboveHor || p->explaining==_STR_ProbBelowHor ||
	    p->explaining==_STR_ProbRightHor || p->explaining==_STR_ProbLeftHor ) {
	BasePoint *tofix, *other;
	if ( sp->nextcp.y==p->found ) {
	    tofix = &sp->nextcp;
	    other = &sp->prevcp;
	} else {
	    tofix = &sp->prevcp;
	    other = &sp->nextcp;
	}
	if ( tofix->y!=p->found ) {
	    GDrawIError("Couldn't find control point");
return;
	}
	tofix->y = p->expected;
	if ( sp->pointtype==pt_curve )
	    other->y = p->expected;
	else
	    sp->pointtype = pt_corner;
    } else if ( p->explaining==_STR_ProbLineVert ) {
	if ( sp->me.x!=p->found ) {
	    sp=sp->next->to;
	    if ( !sp->selected || sp->me.x!=p->found ) {
		GDrawIError("Couldn't find line");
return;
	    }
	}
	sp->prevcp.x += p->expected-sp->me.x;
	sp->nextcp.x += p->expected-sp->me.x;
	sp->me.x = p->expected;
    } else if ( p->explaining==_STR_ProbAboveVert || p->explaining==_STR_ProbBelowVert ||
	    p->explaining==_STR_ProbRightVert || p->explaining==_STR_ProbLeftVert ) {
	BasePoint *tofix, *other;
	if ( sp->nextcp.x==p->found ) {
	    tofix = &sp->nextcp;
	    other = &sp->prevcp;
	} else {
	    tofix = &sp->prevcp;
	    other = &sp->nextcp;
	}
	if ( tofix->x!=p->found ) {
	    GDrawIError("Couldn't find control point");
return;
	}
	tofix->x = p->expected;
	if ( sp->pointtype==pt_curve )
	    other->x = p->expected;
	else
	    sp->pointtype = pt_corner;
    } else if ( p->explaining==_STR_ProbExpectedCounter || p->explaining==_STR_ProbExpectedClockwise ) {
	SplineSetReverse(spl);
    } else
	GDrawIError("Did not fix: %d", p->explaining );
    if ( sp->next!=NULL )
	SplineRefigure(sp->next);
    if ( sp->prev!=NULL )
	SplineRefigure(sp->prev);
    SCCharChangedUpdate(p->sc);
}

static int explain_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct problems *p = GDrawGetUserData(gw);
	p->doneexplain = true;
    } else if ( event->type==et_controlevent &&
	    event->u.control.subtype == et_buttonactivate ) {
	struct problems *p = GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)==CID_Stop )
	    p->finish = true;
	else if ( GGadgetGetCid(event->u.control.g)==CID_Fix )
	    FixIt(p);
	p->doneexplain = true;
    } else if ( event->type==et_controlevent &&
	    event->u.control.subtype == et_radiochanged ) {
	struct problems *p = GDrawGetUserData(gw);
	p->ignorethis = GGadgetIsChecked(event->u.control.g);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("problems.html");
return( true );
	}
return( false );
    }
return( true );
}

static void ExplainIt(struct problems *p, SplineChar *sc, int explain,
	real found, real expected ) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9];
    GTextInfo label[9];
    unichar_t ubuf[100]; char buf[20];
    SplinePointList *spl; Spline *spline, *first;
    int fixable;

    if ( !p->explain || p->finish )
return;
    if ( p->explainw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_ProbExplain,NULL);
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,400));
	pos.height = GDrawPointsToPixels(NULL,86);
	p->explainw = GDrawCreateTopWindow(NULL,&pos,explain_e_h,p,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) explain;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 6; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = 400-12;
	gcd[0].gd.flags = gg_visible | gg_enabled;
	gcd[0].creator = GLabelCreate;

	label[4].text = (unichar_t *) "";
	label[4].text_is_1byte = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 6; gcd[4].gd.pos.y = gcd[0].gd.pos.y+12; gcd[4].gd.pos.width = 400-12;
	gcd[4].gd.flags = gg_visible | gg_enabled;
	gcd[4].creator = GLabelCreate;

	label[5].text = (unichar_t *) _STR_IgnoreProblemFuture;
	label[5].text_in_resource = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.pos.x = 6; gcd[5].gd.pos.y = gcd[4].gd.pos.y+12;
	gcd[5].gd.flags = gg_visible | gg_enabled;
	gcd[5].creator = GCheckBoxCreate;

	gcd[1].gd.pos.x = 15-3; gcd[1].gd.pos.y = gcd[5].gd.pos.y+20;
	gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
	gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[1].text = (unichar_t *) _STR_Next;
	label[1].text_in_resource = true;
	gcd[1].gd.mnemonic = 'N';
	gcd[1].gd.label = &label[1];
	gcd[1].gd.cid = CID_Next;
	gcd[1].creator = GButtonCreate;

	gcd[2].gd.pos.x = -15; gcd[2].gd.pos.y = gcd[1].gd.pos.y+3;
	gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
	gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[2].text = (unichar_t *) _STR_Stop;
	label[2].text_in_resource = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.mnemonic = 'S';
	gcd[2].gd.cid = CID_Stop;
	gcd[2].creator = GButtonCreate;

	gcd[3].gd.pos.x = 2; gcd[3].gd.pos.y = 2;
	gcd[3].gd.pos.width = pos.width-4; gcd[3].gd.pos.height = pos.height-2;
	gcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[3].creator = GGroupCreate;

	gcd[6].gd.pos.x = 200-30; gcd[6].gd.pos.y = gcd[2].gd.pos.y;
	gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
	gcd[6].gd.flags = /*gg_visible |*/ gg_enabled;
	label[6].text = (unichar_t *) _STR_Fix;
	label[6].text_in_resource = true;
	gcd[6].gd.mnemonic = 'N';
	gcd[6].gd.label = &label[6];
	gcd[6].gd.cid = CID_Fix;
	gcd[6].creator = GButtonCreate;

	GGadgetsCreate(p->explainw,gcd);
	p->explaintext = gcd[0].ret;
	p->explainvals = gcd[4].ret;
	p->ignoregadg = gcd[5].ret;
    } else
	GGadgetSetTitle(p->explaintext,GStringGetResource(explain,NULL));
    p->explaining = explain;
    fixable = explain==/*_STR_ProbHintHWidth || explain==_STR_ProbHintVWidth ||*/
	    explain==_STR_ProbFlippedRef ||
	    explain==_STR_ProbXNear || explain==_STR_ProbPtNearVHint ||
	    explain==_STR_ProbYNear || explain==_STR_ProbPtNearHHint ||
	    explain==_STR_ProbYBase || explain==_STR_ProbYXHeight ||
	    explain==_STR_ProbYCapHeight || explain==_STR_ProbYAs ||
	    explain==_STR_ProbYDs ||
	    explain==_STR_ProbLineHor || explain==_STR_ProbLineVert ||
	    explain==_STR_ProbAboveHor || explain==_STR_ProbBelowHor ||
	    explain==_STR_ProbRightHor || explain==_STR_ProbLeftHor ||
	    explain==_STR_ProbAboveVert || explain==_STR_ProbBelowVert ||
	    explain==_STR_ProbRightVert || explain==_STR_ProbLeftVert ||
	    explain==_STR_ProbExpectedCounter || explain==_STR_ProbExpectedClockwise;
    GGadgetSetVisible(GWidgetGetControl(p->explainw,CID_Fix),fixable);

    if ( found==expected )
	ubuf[0]='\0';
    else {
	u_strcpy(ubuf,GStringGetResource(_STR_Found,NULL));
	sprintf(buf,"%.4g", found );
	uc_strcat(ubuf,buf);
	u_strcat(ubuf,GStringGetResource(_STR_Expected,NULL));
	sprintf(buf,"%.4g", expected );
	uc_strcat(ubuf,buf);
    }
    p->found = found; p->expected = expected;
    GGadgetSetTitle(p->explainvals,ubuf);
    GGadgetSetChecked(p->ignoregadg,false);

    p->doneexplain = false;
    p->ignorethis = false;

    if ( sc!=p->lastcharopened || sc->views==NULL ) {
	if ( p->cvopened!=NULL && CVValid(p->fv->sf,p->lastcharopened,p->cvopened) )
	    GDrawDestroyWindow(p->cvopened->gw);
	p->cvopened = NULL;
	if ( sc->views!=NULL )
	    GDrawRaise(sc->views->gw);
	else
	    p->cvopened = CharViewCreate(sc,p->fv);
	GDrawProcessPendingEvents(NULL);
	GDrawProcessPendingEvents(NULL);
	p->lastcharopened = sc;
    }
    SCUpdateAll(sc);		/* We almost certainly just selected something */

    GDrawSetVisible(p->explainw,true);
    GDrawRaise(p->explainw);

    while ( !p->doneexplain )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(p->explainw,false);

    if ( p->cv!=NULL ) {
	CVClearSel(p->cv);
    } else {
	for ( spl = p->sc->splines; spl!=NULL; spl = spl->next ) {
	    spl->first->selected = false;
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		spline->to->selected = false;
		if ( first==NULL ) first = spline;
	    }
	}
    }
}

static void _ExplainIt(struct problems *p, int enc, int explain,
	real found, real expected ) {
    ExplainIt(p,p->sc=SFMakeChar(p->fv->sf,enc),explain,found,expected);
}

/* if they deleted a point or a splineset while we were explaining then we */
/*  need to do some fix-ups. This routine detects a deletion and lets us know */
/*  that more processing is needed */
static int missing(struct problems *p,SplineSet *test, SplinePoint *sp) {
    SplinePointList *spl, *check;
    SplinePoint *tsp;

    if ( !p->explain )
return( false );

    if ( p->cv!=NULL )
	spl = *p->cv->heads[p->cv->drawmode];
    else
	spl = p->sc->splines;
    for ( check = spl; check!=test && check!=NULL; check = check->next );
    if ( check==NULL )
return( true );		/* Deleted splineset */

    if ( sp!=NULL ) {
	for ( tsp=test->first; tsp!=sp ; ) {
	    if ( tsp->next==NULL )
return( true );
	    tsp = tsp->next->to;
	    if ( tsp==test->first )
return( true );
	}
    }
return( false );
}

static int missingspline(struct problems *p,SplineSet *test, Spline *spline) {
    SplinePointList *spl, *check;
    Spline *t, *first=NULL;

    if ( !p->explain )
return( false );

    if ( p->cv!=NULL )
	spl = *p->cv->heads[p->cv->drawmode];
    else
	spl = p->sc->splines;
    for ( check = spl; check!=test && check!=NULL; check = check->next );
    if ( check==NULL )
return( true );		/* Deleted splineset */

    for ( t=test->first->next; t!=NULL && t!=first && t!=spline; t = t->to->next )
	if ( first==NULL ) first = t;
return( t!=spline );
}

static int missinghint(StemInfo *base, StemInfo *findme) {

    while ( base!=NULL && base!=findme )
	base = base->next;
return( base==NULL );
}

static int HVITest(struct problems *p,BasePoint *to, BasePoint *from,
	Spline *spline, int hasia, real ia) {
    real yoff, xoff, angle;
    int ishor=false, isvert=false, isital=false;
    int isto;
    int type;
    BasePoint *base, *other;
    static int hmsgs[5] = { _STR_ProbLineHor, _STR_ProbAboveHor, _STR_ProbBelowHor, _STR_ProbRightHor, _STR_ProbLeftHor };
    static int vmsgs[5] = { _STR_ProbLineVert, _STR_ProbAboveVert, _STR_ProbBelowVert, _STR_ProbRightVert, _STR_ProbLeftVert };
    static int imsgs[5] = { _STR_ProbLineItal, _STR_ProbAboveItal, _STR_ProbBelowItal, _STR_ProbRightItal, _STR_ProbLeftItal };

    yoff = to->y-from->y;
    xoff = to->x-from->x;
    angle = atan2(yoff,xoff);
    if ( angle<0 )
	angle += 3.1415926535897932;
    if ( angle<.1 || angle>3.1415926535897932-.1 ) {
	if ( yoff!=0 )
	    ishor = true;
    } else if ( angle>1.5707963-.1 && angle<1.5707963+.1 ) {
	if ( xoff!=0 )
	    isvert = true;
    } else if ( hasia && angle>ia-.1 && angle<ia+.1 ) {
	if ( angle<ia-.03 || angle>ia+.03 )
	    isital = true;
    }
    if ( ishor || isvert || isital ) {
	isto = false;
	if ( &spline->from->me==from || &spline->from->me==to )
	    spline->from->selected = true;
	if ( &spline->to->me==from || &spline->to->me==to )
	    spline->to->selected = isto = true;
	if ( from==&spline->from->me || from == &spline->to->me ) {
	    base = from; other = to;
	} else {
	    base = to; other = from;
	}
	if ( &spline->from->me==from && &spline->to->me==to ) {
	    type = 0;	/* Line */
	    if ( (ishor && xoff<0) || (isvert && yoff<0)) {
		base = from;
		other = to;
	    } else {
		base = to;
		other = from;
	    }
	} else if ( abs(yoff)>abs(xoff) )
	    type = ((yoff>0) ^ isto)?1:2;
	else
	    type = ((xoff>0) ^ isto)?3:4;
	if ( ishor )
	    ExplainIt(p,p->sc,hmsgs[type], other->y,base->y);
	else if ( isvert )
	    ExplainIt(p,p->sc,vmsgs[type], other->x,base->x);
	else
	    ExplainIt(p,p->sc,imsgs[type],0,0);
return( true );
    }
return( false );
}

/* Is the control point outside of the spline segment when projected onto the */
/*  vector between the end points of the spline segment? */
static int OddCPCheck(BasePoint *cp,BasePoint *base,BasePoint *v,
	SplinePoint *sp, struct problems *p) {
    real len = (cp->x-base->x)*v->x+ (cp->y-base->y)*v->y;
    real xoff, yoff;
    int msg=0;

    if ( len<0 || len>1 || (len==0 && &sp->me!=base) || (len==1 && &sp->me==base)) {
	xoff = cp->x-sp->me.x; yoff = cp->y-sp->me.y;
	if ( fabs(yoff)>fabs(xoff) )
	    msg = yoff>0?_STR_ProbAboveOdd:_STR_ProbBelowOdd;
	else
	    msg = xoff>0?_STR_ProbRightOdd:_STR_ProbLeftOdd;
	sp->selected = true;
	ExplainIt(p,p->sc,msg, 0,0);
return( true );
    }
return( false );
}

static int SCProblems(CharView *cv,SplineChar *sc,struct problems *p) {
    SplineSet *spl, *test;
    Spline *spline, *first;
    SplinePoint *sp, *nsp;
    int needsupdate=false, changed=false;
    StemInfo *h;

  restart:
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
    p->sc = sc;
    if (( p->ptnearhint || p->hintwidthnearval || p->hintwithnopt ) &&
	    sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,true);

    if ( p->openpaths ) {
	for ( test=spl; test!=NULL && !p->finish; test=test->next ) {
	    /* I'm also including in "open paths" the special case of a */
	    /*  singleton point with connects to itself */
	    if ( test->first!=NULL && ( test->first->prev==NULL ||
		    ( test->first->prev == test->first->next &&
			test->first->noprevcp && test->first->nonextcp))) {
		changed = true;
		test->first->selected = test->last->selected = true;
		ExplainIt(p,sc,_STR_ProbOpenPath,0,0);
		if ( p->ignorethis ) {
		    p->openpaths = false;
	break;
		}
		if ( missing(p,test,NULL))
      goto restart;
	    }
	}
    }

    if ( p->pointstooclose && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->pointstooclose; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->next==NULL )
	    break;
		nsp = sp->next->to;
		if ( (nsp->me.x-sp->me.x)*(nsp->me.x-sp->me.x) + (nsp->me.y-sp->me.y)*(nsp->me.y-sp->me.y) < 2*2 ) {
		    changed = true;
		    sp->selected = nsp->selected = true;
		    ExplainIt(p,sc,_STR_ProbPointsTooClose,0,0);
		    if ( p->ignorethis ) {
			p->pointstooclose = false;
	    break;
		    }
		    if ( missing(p,test,nsp))
  goto restart;
		}
		sp = nsp;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->pointstooclose )
	break;
	}
    }

    if ( p->xnearval && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->xnearval; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->me.x-p->xval<p->near && p->xval-sp->me.x<p->near &&
			sp->me.x!=p->xval ) {
		    changed = true;
		    sp->selected = true;
		    ExplainIt(p,sc,_STR_ProbXNear,sp->me.x,p->xval);
		    if ( p->ignorethis ) {
			p->xnearval = false;
	    break;
		    }
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->xnearval )
	break;
	}
    }

    if ( p->ynearval && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->ynearval; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->me.y-p->yval<p->near && p->yval-sp->me.y<p->near &&
			sp->me.y != p->yval ) {
		    changed = true;
		    sp->selected = true;
		    ExplainIt(p,sc,_STR_ProbYNear,sp->me.y,p->yval);
		    if ( p->ignorethis ) {
			p->ynearval = false;
	    break;
		    }
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->ynearval )
	break;
	}
    }

    if ( p->ynearstd && !p->finish ) {
	real expected;
	int msg;
	for ( test=spl; test!=NULL && !p->finish && p->ynearstd; test=test->next ) {
	    sp = test->first;
	    do {
		if (( sp->me.y-p->xheight<p->near && p->xheight-sp->me.y<p->near && sp->me.y!=p->xheight ) ||
			( sp->me.y-p->caph<p->near && p->caph-sp->me.y<p->near && sp->me.y!=p->caph && sp->me.y!=p->ascent ) ||
			( sp->me.y-p->ascent<p->near && p->ascent-sp->me.y<p->near && sp->me.y!=p->caph && sp->me.y!=p->ascent ) ||
			( sp->me.y-p->descent<p->near && p->descent-sp->me.y<p->near && sp->me.y!=p->descent ) ||
			( sp->me.y<p->near && -sp->me.y<p->near && sp->me.y!=0 ) ) {
		    changed = true;
		    sp->selected = true;
		    if ( sp->me.y<p->near && -sp->me.y<p->near ) {
			msg = _STR_ProbYBase;
			expected = 0;
		    } else if ( sp->me.y-p->xheight<p->near && p->xheight-sp->me.y<p->near ) {
			msg = _STR_ProbYXHeight;
			expected = p->xheight;
		    } else if ( sp->me.y-p->caph<p->near && p->caph-sp->me.y<p->near ) {
			msg = _STR_ProbYCapHeight;
			expected = p->caph;
		    } else if ( sp->me.y-p->ascent<p->near && p->ascent-sp->me.y<p->near ) {
			msg = _STR_ProbYAs;
			expected = p->ascent;
		    } else {
			msg = _STR_ProbYDs;
			expected = p->descent;
		    }
		    ExplainIt(p,sc,msg,sp->me.y,expected);
		    if ( p->ignorethis ) {
			p->ynearstd = false;
	    break;
		    }
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->ynearstd )
	break;
	}
    }

    if ( p->linenearstd && !p->finish ) {
	real ia = (90-p->fv->sf->italicangle)*(3.1415926535897932/180);
	int hasia = p->fv->sf->italicangle!=0;
	for ( test=spl; test!=NULL && !p->finish && p->linenearstd; test = test->next ) {
	    first = NULL;
	    for ( spline = test->first->next; spline!=NULL && spline!=first && !p->finish; spline=spline->to->next ) {
		if ( spline->knownlinear ) {
		    if ( HVITest(p,&spline->to->me,&spline->from->me,spline,
			    hasia, ia)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->linenearstd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		}
		if ( first==NULL ) first = spline;
	    }
	    if ( !p->linenearstd )
	break;
	}
    }

    if ( p->cpnearstd && !p->finish ) {
	real ia = (90-p->fv->sf->italicangle)*(3.1415926535897932/180);
	int hasia = p->fv->sf->italicangle!=0;
	for ( test=spl; test!=NULL && !p->finish && p->linenearstd; test = test->next ) {
	    first = NULL;
	    for ( spline = test->first->next; spline!=NULL && spline!=first && !p->finish; spline=spline->to->next ) {
		if ( !spline->knownlinear ) {
		    if ( !spline->from->nonextcp &&
			    HVITest(p,&spline->from->nextcp,&spline->from->me,spline,
				hasia, ia)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->cpnearstd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		    if ( !spline->to->noprevcp &&
			    HVITest(p,&spline->to->me,&spline->to->prevcp,spline,
				hasia, ia)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->cpnearstd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		}
		if ( first==NULL ) first = spline;
	    }
	    if ( !p->cpnearstd )
	break;
	}
    }

    if ( p->cpodd && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish && p->linenearstd; test = test->next ) {
	    first = NULL;
	    for ( spline = test->first->next; spline!=NULL && spline!=first && !p->finish; spline=spline->to->next ) {
		if ( !spline->knownlinear ) {
		    BasePoint v; real len;
		    v.x = spline->to->me.x-spline->from->me.x;
		    v.y = spline->to->me.y-spline->from->me.y;
		    len = /*sqrt*/(v.x*v.x+v.y*v.y);
		    v.x /= len; v.y /= len;
		    if ( !spline->from->nonextcp &&
			    OddCPCheck(&spline->from->nextcp,&spline->from->me,&v,
			     spline->from,p)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->cpodd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		    if ( !spline->to->noprevcp &&
			    OddCPCheck(&spline->to->prevcp,&spline->from->me,&v,
			     spline->to,p)) {
			changed = true;
			if ( p->ignorethis ) {
			    p->cpodd = false;
	    break;
			}
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		}
		if ( first==NULL ) first = spline;
	    }
	    if ( !p->cpodd )
	break;
	}
    }

    if ( p->hintwithnopt && !p->finish ) {
	int anys, anye;
      restarthhint:
	for ( h=sc->hstem; h!=NULL ; h=h->next ) {
	    anys = anye = false;
	    for ( test=spl; test!=NULL && !p->finish && (!anys || !anye); test=test->next ) {
		sp = test->first;
		do {
		    if (sp->me.y==h->start )
			anys = true;
		    if (sp->me.y==h->start+h->width )
			anye = true;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		} while ( sp!=test->first && !p->finish );
	    }
	    if ( h->ghost && ( anys || anye ))
		/* Ghost hints only define one edge */;
	    else if ( !anys || !anye ) {
		h->active = true;
		changed = true;
		ExplainIt(p,sc,_STR_ProbHintControl,0,0);
		if ( p->ignorethis ) {
		    p->hintwithnopt = false;
	break;
		}
		h->active = false;
		if ( missinghint(sc->hstem,h))
      goto restarthhint;
	    }
	}
      restartvhint:
	for ( h=sc->vstem; h!=NULL && p->hintwithnopt && !p->finish; h=h->next ) {
	    anys = anye = false;
	    for ( test=spl; test!=NULL && !p->finish && (!anys || !anye); test=test->next ) {
		sp = test->first;
		do {
		    if (sp->me.x==h->start )
			anys = true;
		    if (sp->me.x==h->start+h->width )
			anye = true;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		} while ( sp!=test->first && !p->finish );
	    }
	    if ( !anys || !anye ) {
		h->active = true;
		changed = true;
		ExplainIt(p,sc,_STR_ProbHintControl,0,0);
		if ( p->ignorethis ) {
		    p->hintwithnopt = false;
	break;
		}
		if ( missinghint(sc->vstem,h))
      goto restartvhint;
		h->active = false;
	    }
	}
    }

    if ( p->ptnearhint && !p->finish ) {
	real found, expected;
	for ( test=spl; test!=NULL && !p->finish && p->ptnearhint; test=test->next ) {
	    sp = test->first;
	    do {
		int hs = false, vs = false;
		for ( h=sc->hstem; h!=NULL; h=h->next ) {
		    if (( sp->me.y-h->start<p->near && h->start-sp->me.y<p->near &&
				sp->me.y!=h->start ) ||
			    ( sp->me.y-h->start+h->width<p->near && h->start+h->width-sp->me.y<p->near &&
				sp->me.y!=h->start+h->width )) {
			found = sp->me.y;
			if ( sp->me.y-h->start<p->near && h->start-sp->me.y<p->near )
			    expected = h->start;
			else
			    expected = h->start+h->width;
			h->active = true;
			hs = true;
		break;
		    }
		}
		if ( !hs ) {
		    for ( h=sc->vstem; h!=NULL; h=h->next ) {
			if (( sp->me.x-h->start<p->near && h->start-sp->me.x<p->near &&
				    sp->me.x!=h->start ) ||
				( sp->me.x-h->start+h->width<p->near && h->start+h->width-sp->me.x<p->near &&
				    sp->me.x!=h->start+h->width )) {
			    found = sp->me.x;
			    if ( sp->me.x-h->start<p->near && h->start-sp->me.x<p->near )
				expected = h->start;
			    else
				expected = h->start+h->width;
			    h->active = true;
			    vs = true;
		    break;
			}
		    }
		}
		if ( hs || vs ) {
		    changed = true;
		    sp->selected = true;
		    ExplainIt(p,sc,hs?_STR_ProbPtNearHHint:_STR_ProbPtNearVHint,found,expected);
		    if ( p->ignorethis ) {
			p->ptnearhint = false;
	    break;
		    }
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	    if ( !p->ptnearhint )
	break;
	}
    }

    if ( p->hintwidthnearval && !p->finish ) {
	StemInfo *hs = NULL, *vs = NULL;
	for ( h=sc->hstem; h!=NULL; h=h->next ) {
	    if ( h->width-p->widthval<p->near && p->widthval-h->width<p->near &&
		    h->width!=p->widthval ) {
		h->active = true;
		hs = h;
	break;
	    }
	}
	for ( h=sc->vstem; h!=NULL; h=h->next ) {
	    if ( h->width-p->widthval<p->near && p->widthval-h->width<p->near &&
		    h->width!=p->widthval ) {
		h->active = true;
		vs = h;
	break;
	    }
	}
	if ( hs || vs ) {
	    changed = true;
	    ExplainIt(p,sc,hs?_STR_ProbHintHWidth:_STR_ProbHintVWidth,
		    hs?hs->width:vs->width,p->widthval);
	    if ( hs!=NULL && !missinghint(sc->hstem,hs)) hs->active = false;
	    if ( vs!=NULL && !missinghint(sc->vstem,vs)) vs->active = false;
	    if ( p->ignorethis )
		p->hintwidthnearval = false;
	    else if ( (hs!=NULL && missinghint(sc->hstem,hs)) &&
		    ( vs!=NULL && missinghint(sc->vstem,vs)))
      goto restart;
	}
    }

    if ( p->direction && !p->finish ) {
	SplineSet **base, *ret;
	int lastscan= -1;
	if ( cv!=NULL )
	    base = cv->heads[cv->drawmode];
	else
	    base = &sc->splines;
	while ( !p->finish && (ret=SplineSetsDetectDir(base,&lastscan))!=NULL ) {
	    sp = ret->first;
	    changed = true;
	    while ( 1 ) {
		sp->selected = true;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ret->first )
	    break;
	    }
	    if ( SplinePointListIsClockwise(ret))
		ExplainIt(p,sc,_STR_ProbExpectedCounter,0,0);
	    else
		ExplainIt(p,sc,_STR_ProbExpectedClockwise,0,0);
	    if ( p->ignorethis ) {
		p->direction = false;
	break;
	    }
	}
    }

    if ( p->flippedrefs && !p->finish && ( cv==NULL || cv->drawmode==dm_fore )) {
	RefChar *ref;
	for ( ref = sc->refs; ref!=NULL ; ref = ref->next )
	    ref->selected = false;
	for ( ref = sc->refs; !p->finish && ref!=NULL ; ref = ref->next ) {
	    if ( ref->transform[0]*ref->transform[3]<0 ||
		    (ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
		changed = true;
		ref->selected = true;
		ExplainIt(p,sc,_STR_ProbFlippedRef,0,0);
		ref->selected = false;
		if ( p->ignorethis ) {
		    p->flippedrefs = false;
	break;
		}
	    }
	}
    }

    if ( needsupdate || changed )
	SCUpdateAll(sc);
return( changed );
}

static int CIDCheck(struct problems *p,int cid) {
    int found = false;

    if ( (p->cidmultiple || p->cidblank) && !p->finish ) {
	SplineFont *csf = p->fv->cidmaster;
	int i, cnt;
	for ( i=cnt=0; i<csf->subfontcnt; ++i )
	    if ( cid<csf->subfonts[i]->charcnt &&
		    SCWorthOutputting(csf->subfonts[i]->chars[cid]) )
		++cnt;
	if ( cnt>1 && p->cidmultiple ) {
	    _ExplainIt(p,cid,_STR_ProbCIDMult,cnt,1);
	    if ( p->ignorethis )
		p->cidmultiple = false;
	    found = true;
	} else if ( cnt==0 && p->cidblank ) {
	    _ExplainIt(p,cid,_STR_ProbCIDBlank,0,0);
	    if ( p->ignorethis )
		p->cidblank = false;
	    found = true;
	}
    }
return( found );
}

static void DoProbs(struct problems *p) {
    int i, ret;
    SplineChar *sc;

    if ( p->cv!=NULL ) {
	ret = SCProblems(p->cv,NULL,p);
	ret |= CIDCheck(p,p->cv->sc->enc);
    } else if ( p->msc!=NULL ) {
	ret = SCProblems(NULL,p->msc,p);
	ret |= CIDCheck(p,p->msc->enc);
    } else {
	ret = false;
	for ( i=0; i<p->fv->sf->charcnt && !p->finish; ++i )
	    if ( p->fv->selected[i] ) {
		if ( (sc = p->fv->sf->chars[i])!=NULL ) {
		    if ( SCProblems(NULL,sc,p)) {
			if ( sc!=p->lastcharopened ) {
			    if ( sc->views!=NULL )
				GDrawRaise(sc->views->gw);
			    else
				CharViewCreate(sc,p->fv);
			    p->lastcharopened = sc;
			}
			ret = true;
		    }
		}
		ret |= CIDCheck(p,i);
	    }
    }
    if ( !ret )
	GDrawError( "No problems found");
}

static void FigureStandardHeights(struct problems *p) {
    BlueData bd;

    QuickBlues(p->fv->sf,&bd);
    p->xheight = bd.xheight;
    p->caph = bd.caph;
    p->ascent = bd.ascent;
    p->descent = bd.descent;
}

static int Prob_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct problems *p = GDrawGetUserData(gw);
	int errs = false;

	openpaths = p->openpaths = GGadgetIsChecked(GWidgetGetControl(gw,CID_OpenPaths));
	pointstooclose = p->pointstooclose = GGadgetIsChecked(GWidgetGetControl(gw,CID_PointsTooClose));
	/*missing = p->missingextrema = GGadgetIsChecked(GWidgetGetControl(gw,CID_MissingExtrema))*/;
	doxnear = p->xnearval = GGadgetIsChecked(GWidgetGetControl(gw,CID_XNear));
	doynear = p->ynearval = GGadgetIsChecked(GWidgetGetControl(gw,CID_YNear));
	doynearstd = p->ynearstd = GGadgetIsChecked(GWidgetGetControl(gw,CID_YNearStd));
	linestd = p->linenearstd = GGadgetIsChecked(GWidgetGetControl(gw,CID_LineStd));
	cpstd = p->cpnearstd = GGadgetIsChecked(GWidgetGetControl(gw,CID_CpStd));
	cpodd = p->cpodd = GGadgetIsChecked(GWidgetGetControl(gw,CID_CpOdd));
	hintnopt = p->hintwithnopt = GGadgetIsChecked(GWidgetGetControl(gw,CID_HintNoPt));
	ptnearhint = p->ptnearhint = GGadgetIsChecked(GWidgetGetControl(gw,CID_PtNearHint));
	hintwidth = p->hintwidthnearval = GGadgetIsChecked(GWidgetGetControl(gw,CID_HintWidthNear));
	direction = p->direction = GGadgetIsChecked(GWidgetGetControl(gw,CID_Direction));
	flippedrefs = p->flippedrefs = GGadgetIsChecked(GWidgetGetControl(gw,CID_FlippedRefs));
	if ( p->fv->cidmaster!=NULL ) {
	    cidmultiple = p->cidmultiple = GGadgetIsChecked(GWidgetGetControl(gw,CID_CIDMultiple));
	    cidblank = p->cidblank = GGadgetIsChecked(GWidgetGetControl(gw,CID_CIDBlank));
	}
	p->explain = true;
	if ( doxnear )
	    p->xval = xval = GetRealR(gw,CID_XNearVal,_STR_XNear,&errs);
	if ( doynear )
	    p->yval = yval = GetRealR(gw,CID_YNearVal,_STR_YNear,&errs);
	if ( hintwidth )
	    widthval = p->widthval = GetRealR(gw,CID_HintWidth,_STR_HintWidth,&errs);
	near = p->near = GetRealR(gw,CID_Near,_STR_Near,&errs);
	if ( errs )
return( true );
	if ( doynearstd )
	    FigureStandardHeights(p);
	GDrawSetVisible(gw,false);
	if ( openpaths || pointstooclose /*|| missing*/ || doxnear || doynear ||
		doynearstd || linestd || hintnopt || ptnearhint || hintwidth ||
		direction || p->cidmultiple || p->cidblank || p->flippedrefs ) {
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

static int Prob_TextChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GGadgetSetChecked(GWidgetGetControl(GGadgetGetWindow(g),(int) GGadgetGetUserData(g)),true);
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

void FindProblems(FontView *fv,CharView *cv, SplineChar *sc) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[26];
    GTextInfo label[25];
    struct problems p;
    char xnbuf[20], ynbuf[20], widthbuf[20], nearbuf[20];
    int ypos;

    memset(&p,0,sizeof(p));
    if ( fv==NULL ) fv = cv->fv;
    p.fv = fv; p.cv=cv; p.msc = sc;
    if ( cv!=NULL )
	p.lastcharopened = cv->sc;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Findprobs,NULL);
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,fv->cidmaster==NULL?335:372);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_OpenPaths;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.mnemonic = 'P';
    gcd[0].gd.pos.x = 6; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.flags = gg_visible | gg_enabled;
    if ( openpaths ) gcd[0].gd.flags |= gg_cb_on;
    gcd[0].gd.popup_msg = GStringGetResource(_STR_OpenPathsPopup,NULL);
    gcd[0].gd.cid = CID_OpenPaths;
    gcd[0].creator = GCheckBoxCreate;

    label[1].text = (unichar_t *) _STR_Points2Close;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.mnemonic = 't';
    gcd[1].gd.pos.x = 6; gcd[1].gd.pos.y = gcd[0].gd.pos.y+17; 
    gcd[1].gd.flags = gg_visible | gg_enabled;
    if ( pointstooclose ) gcd[1].gd.flags |= gg_cb_on;
    gcd[1].gd.popup_msg = GStringGetResource(_STR_Points2ClosePopup,NULL);
    gcd[1].gd.cid = CID_PointsTooClose;
    gcd[1].creator = GCheckBoxCreate;

    label[2].text = (unichar_t *) _STR_XNear;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'X';
    gcd[2].gd.pos.x = 6; gcd[2].gd.pos.y = gcd[1].gd.pos.y+20; 
    gcd[2].gd.flags = gg_visible | gg_enabled;
    if ( doxnear ) gcd[2].gd.flags |= gg_cb_on;
    gcd[2].gd.popup_msg = GStringGetResource(_STR_XNearPopup,NULL);
    gcd[2].gd.cid = CID_XNear;
    gcd[2].creator = GCheckBoxCreate;

    sprintf(xnbuf,"%g",xval);
    label[3].text = (unichar_t *) xnbuf;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 60; gcd[3].gd.pos.y = gcd[2].gd.pos.y-1; gcd[3].gd.pos.width = 40;
    gcd[3].gd.flags = gg_visible | gg_enabled;
    gcd[3].gd.cid = CID_XNearVal;
    gcd[5].gd.handle_controlevent = Prob_TextChanged;
    gcd[5].data = (void *) CID_XNear;
    gcd[3].creator = GTextFieldCreate;

    label[4].text = (unichar_t *) _STR_YNear;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.mnemonic = 'Y';
    gcd[4].gd.pos.x = 6; gcd[4].gd.pos.y = gcd[2].gd.pos.y+26; 
    gcd[4].gd.flags = gg_visible | gg_enabled;
    if ( doynear ) gcd[4].gd.flags |= gg_cb_on;
    gcd[4].gd.popup_msg = GStringGetResource(_STR_YNearPopup,NULL);
    gcd[4].gd.cid = CID_YNear;
    gcd[4].creator = GCheckBoxCreate;

    sprintf(ynbuf,"%g",yval);
    label[5].text = (unichar_t *) ynbuf;
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = 60; gcd[5].gd.pos.y = gcd[4].gd.pos.y-1; gcd[5].gd.pos.width = 40;
    gcd[5].gd.flags = gg_visible | gg_enabled;
    gcd[5].gd.cid = CID_YNearVal;
    gcd[5].gd.handle_controlevent = Prob_TextChanged;
    gcd[5].data = (void *) CID_YNear;
    gcd[5].creator = GTextFieldCreate;

    label[6].text = (unichar_t *) _STR_YNearStd;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.mnemonic = 'S';
    gcd[6].gd.pos.x = 6; gcd[6].gd.pos.y = gcd[4].gd.pos.y+20; 
    gcd[6].gd.flags = gg_visible | gg_enabled;
    if ( doynearstd ) gcd[6].gd.flags |= gg_cb_on;
    gcd[6].gd.popup_msg = GStringGetResource(_STR_YNearStdPopup,NULL);
    gcd[6].gd.cid = CID_YNearStd;
    gcd[6].creator = GCheckBoxCreate;

    label[7].text = (unichar_t *) (fv->sf->italicangle==0?_STR_LineStd:_STR_LineStd2);
    label[7].text_in_resource = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.mnemonic = 'E';
    gcd[7].gd.pos.x = 6; gcd[7].gd.pos.y = gcd[6].gd.pos.y+17; 
    gcd[7].gd.flags = gg_visible | gg_enabled;
    if ( linestd ) gcd[7].gd.flags |= gg_cb_on;
    gcd[7].gd.popup_msg = GStringGetResource(_STR_LineStdPopup,NULL);
    gcd[7].gd.cid = CID_LineStd;
    gcd[7].creator = GCheckBoxCreate;

    label[19].text = (unichar_t *) (fv->sf->italicangle==0?_STR_CpStd:_STR_CpStd2);
    label[19].text_in_resource = true;
    gcd[19].gd.label = &label[19];
    gcd[19].gd.mnemonic = 'C';
    gcd[19].gd.pos.x = 6; gcd[19].gd.pos.y = gcd[7].gd.pos.y+17; 
    gcd[19].gd.flags = gg_visible | gg_enabled;
    if ( cpstd ) gcd[19].gd.flags |= gg_cb_on;
    gcd[19].gd.popup_msg = GStringGetResource(_STR_CpStdPopup,NULL);
    gcd[19].gd.cid = CID_CpStd;
    gcd[19].creator = GCheckBoxCreate;

    label[20].text = (unichar_t *) _STR_CpOdd;
    label[20].text_in_resource = true;
    gcd[20].gd.label = &label[20];
    gcd[20].gd.mnemonic = 'b';
    gcd[20].gd.pos.x = 6; gcd[20].gd.pos.y = gcd[19].gd.pos.y+17; 
    gcd[20].gd.flags = gg_visible | gg_enabled;
    if ( cpodd ) gcd[20].gd.flags |= gg_cb_on;
    gcd[20].gd.popup_msg = GStringGetResource(_STR_CpOddPopup,NULL);
    gcd[20].gd.cid = CID_CpOdd;
    gcd[20].creator = GCheckBoxCreate;

    label[8].text = (unichar_t *) _STR_HintNoPt;
    label[8].text_in_resource = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.mnemonic = 'H';
    gcd[8].gd.pos.x = 6; gcd[8].gd.pos.y = gcd[20].gd.pos.y+17; 
    gcd[8].gd.flags = gg_visible | gg_enabled;
    if ( hintnopt ) gcd[8].gd.flags |= gg_cb_on;
    gcd[8].gd.popup_msg = GStringGetResource(_STR_HintNoPtPopup,NULL);
    gcd[8].gd.cid = CID_HintNoPt;
    gcd[8].creator = GCheckBoxCreate;

    label[9].text = (unichar_t *) _STR_PtNearHint;
    label[9].text_in_resource = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.mnemonic = 'H';
    gcd[9].gd.pos.x = 6; gcd[9].gd.pos.y = gcd[8].gd.pos.y+17; 
    gcd[9].gd.flags = gg_visible | gg_enabled;
    if ( ptnearhint ) gcd[9].gd.flags |= gg_cb_on;
    gcd[9].gd.popup_msg = GStringGetResource(_STR_PtNearHintPopup,NULL);
    gcd[9].gd.cid = CID_PtNearHint;
    gcd[9].creator = GCheckBoxCreate;

    label[10].text = (unichar_t *) _STR_HintWidth;
    label[10].text_in_resource = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.mnemonic = 'W';
    gcd[10].gd.pos.x = 6; gcd[10].gd.pos.y = gcd[9].gd.pos.y+21;
    gcd[10].gd.flags = gg_visible | gg_enabled;
    if ( hintwidth ) gcd[10].gd.flags |= gg_cb_on;
    gcd[10].gd.popup_msg = GStringGetResource(_STR_HintWidthPopup,NULL);
    gcd[10].gd.cid = CID_HintWidthNear;
    gcd[10].creator = GCheckBoxCreate;

    sprintf(widthbuf,"%g",widthval);
    label[11].text = (unichar_t *) widthbuf;
    label[11].text_is_1byte = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.pos.x = 100+5; gcd[11].gd.pos.y = gcd[10].gd.pos.y-1; gcd[11].gd.pos.width = 40;
    gcd[11].gd.flags = gg_visible | gg_enabled;
    gcd[11].gd.cid = CID_HintWidth;
    gcd[11].gd.handle_controlevent = Prob_TextChanged;
    gcd[11].data = (void *) CID_HintWidthNear;
    gcd[11].creator = GTextFieldCreate;

    label[12].text = (unichar_t *) _STR_CheckDirection;
    label[12].text_in_resource = true;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.mnemonic = 'S';
    gcd[12].gd.pos.x = 6; gcd[12].gd.pos.y = gcd[10].gd.pos.y+20; 
    gcd[12].gd.flags = gg_visible | gg_enabled;
    if ( direction ) gcd[12].gd.flags |= gg_cb_on;
    gcd[12].gd.popup_msg = GStringGetResource(_STR_CheckDirectionPopup,NULL);
    gcd[12].gd.cid = CID_Direction;
    gcd[12].creator = GCheckBoxCreate;

    label[21].text = (unichar_t *) _STR_CheckFlippedRefs;
    label[21].text_in_resource = true;
    gcd[21].gd.label = &label[21];
    gcd[21].gd.mnemonic = 'r';
    gcd[21].gd.pos.x = 6; gcd[21].gd.pos.y = gcd[12].gd.pos.y+20; 
    gcd[21].gd.flags = gg_visible | gg_enabled;
    if ( flippedrefs ) gcd[21].gd.flags |= gg_cb_on;
    gcd[21].gd.popup_msg = GStringGetResource(_STR_CheckFlippedRefsPopup,NULL);
    gcd[21].gd.cid = CID_FlippedRefs;
    gcd[21].creator = GCheckBoxCreate;

    gcd[18].gd.pos.x = 10; gcd[18].gd.pos.y = gcd[21].gd.pos.y+22;
    gcd[18].gd.pos.width = 200-20;
    gcd[18].gd.flags = gg_enabled | gg_visible;
    gcd[18].creator = GLineCreate;

    if ( fv->cidmaster!=NULL ) {
	label[22].text = (unichar_t *) _STR_CIDMultiple;
	label[22].text_in_resource = true;
	gcd[22].gd.label = &label[22];
	gcd[22].gd.mnemonic = 'S';
	gcd[22].gd.pos.x = 6; gcd[22].gd.pos.y = gcd[21].gd.pos.y+24;
	gcd[22].gd.flags = gg_visible | gg_enabled;
	if ( cidmultiple ) gcd[22].gd.flags |= gg_cb_on;
	gcd[22].gd.popup_msg = GStringGetResource(_STR_CIDMultiplePopup,NULL);
	gcd[22].gd.cid = CID_CIDMultiple;
	gcd[22].creator = GCheckBoxCreate;

	label[23].text = (unichar_t *) _STR_CIDBlank;
	label[23].text_in_resource = true;
	gcd[23].gd.label = &label[23];
	gcd[23].gd.mnemonic = 'S';
	gcd[23].gd.pos.x = 6; gcd[23].gd.pos.y = gcd[22].gd.pos.y+17; 
	gcd[23].gd.flags = gg_visible | gg_enabled;
	if ( cidblank ) gcd[23].gd.flags |= gg_cb_on;
	gcd[23].gd.popup_msg = GStringGetResource(_STR_CIDBlankPopup,NULL);
	gcd[23].gd.cid = CID_CIDBlank;
	gcd[23].creator = GCheckBoxCreate;

	ypos = gcd[23].gd.pos.y-2;

	gcd[24].gd.pos.x = 10; gcd[24].gd.pos.y = gcd[23].gd.pos.y+20;
	gcd[24].gd.pos.width = 200-20;
	gcd[24].gd.flags = gg_enabled | gg_visible;
	gcd[24].creator = GLineCreate;
    } else
	ypos = gcd[21].gd.pos.y;

    label[13].text = (unichar_t *) _STR_PointsNear;
    label[13].text_in_resource = true;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.mnemonic = 'N';
    gcd[13].gd.pos.x = 6; gcd[13].gd.pos.y = ypos+37; 
    gcd[13].gd.flags = gg_visible | gg_enabled;
    gcd[13].creator = GLabelCreate;

    sprintf(nearbuf,"%g",near);
    label[14].text = (unichar_t *) nearbuf;
    label[14].text_is_1byte = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.pos.x = 130; gcd[14].gd.pos.y = gcd[13].gd.pos.y-6; gcd[14].gd.pos.width = 40;
    gcd[14].gd.flags = gg_visible | gg_enabled;
    gcd[14].gd.cid = CID_Near;
    gcd[14].creator = GTextFieldCreate;

    gcd[15].gd.pos.x = 15-3; gcd[15].gd.pos.y = gcd[14].gd.pos.y+30;
    gcd[15].gd.pos.width = -1; gcd[15].gd.pos.height = 0;
    gcd[15].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[15].text = (unichar_t *) _STR_OK;
    label[15].text_in_resource = true;
    gcd[15].gd.mnemonic = 'O';
    gcd[15].gd.label = &label[15];
    gcd[15].gd.handle_controlevent = Prob_OK;
    gcd[15].creator = GButtonCreate;

    gcd[16].gd.pos.x = 200-GIntGetResource(_NUM_Buttonsize)-15; gcd[16].gd.pos.y = gcd[15].gd.pos.y+3;
    gcd[16].gd.pos.width = -1; gcd[16].gd.pos.height = 0;
    gcd[16].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[16].text = (unichar_t *) _STR_Cancel;
    label[16].text_in_resource = true;
    gcd[16].gd.label = &label[16];
    gcd[16].gd.mnemonic = 'C';
    gcd[16].gd.handle_controlevent = Prob_Cancel;
    gcd[16].creator = GButtonCreate;

    gcd[17].gd.pos.x = 2; gcd[17].gd.pos.y = 2;
    gcd[17].gd.pos.width = pos.width-4; gcd[17].gd.pos.height = pos.height-2;
    gcd[17].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[17].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);

    GDrawSetVisible(gw,true);
    while ( !p.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    if ( p.explainw!=NULL )
	GDrawDestroyWindow(p.explainw);
}
