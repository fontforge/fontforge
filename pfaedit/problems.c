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
#include <gwidget.h>
#include <ustring.h>
#include <math.h>

struct problems {
    FontView *fv;
    CharView *cv;
    SplineChar *sc;
    unsigned int openpaths: 1;
    unsigned int pointstooclose: 1;
    /*unsigned int missingextrema: 1;*/
    unsigned int xnearval: 1;
    unsigned int ynearval: 1;
    unsigned int ynearstd: 1;		/* baseline, xheight, cap, ascent, descent, etc. */
    unsigned int linenearstd: 1;	/* horizontal, vertical, italicangle */
    unsigned int hintwithnopt: 1;
    unsigned int ptnearhint: 1;
    unsigned int hintwidthnearval: 1;
    unsigned int explain: 1;
    unsigned int done: 1;
    unsigned int doneexplain: 1;
    unsigned int finish: 1;
    double near, xval, yval, widthval;
    double xheight, caph, ascent, descent;
    GWindow explainw;
    GGadget *explaintext, *explainvals;
    SplineChar *lastcharopened;
};

static int openpaths=1, pointstooclose=1/*, missing=0*/, doxnear=0, doynear=0;
static int doynearstd=1, linestd=1, hintnopt=0, ptnearhint=0, hintwidth=0, explain=1;
static double near=3, xval=0, yval=0, widthval=50;

#define CID_Stop		2001
#define CID_Next		2002

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
#define CID_Explain		1015


static int explain_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct problems *p = GDrawGetUserData(gw);
	p->doneexplain = true;
    } else if ( event->type==et_controlevent &&
	    event->u.control.subtype == et_buttonactivate ) {
	struct problems *p = GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)==CID_Stop )
	    p->finish = true;
	p->doneexplain = true;
    }
return( event->type!=et_char );
}

static void ExplainIt(struct problems *p, SplineChar *sc, int explain,
	double found, double expected ) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7];
    GTextInfo label[7];
    unichar_t ubuf[100]; char buf[20];

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
	pos.width =GDrawPointsToPixels(NULL,400);
	pos.height = GDrawPointsToPixels(NULL,80);
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

	gcd[1].gd.pos.x = 15-3; gcd[1].gd.pos.y = gcd[0].gd.pos.y+27;
	gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
	gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[1].text = (unichar_t *) _STR_Next;
	label[1].text_in_resource = true;
	gcd[1].gd.mnemonic = 'N';
	gcd[1].gd.label = &label[1];
	gcd[1].gd.cid = CID_Next;
	gcd[1].creator = GButtonCreate;

	gcd[2].gd.pos.x = 400-GIntGetResource(_NUM_Buttonsize)-15; gcd[2].gd.pos.y = gcd[1].gd.pos.y+3;
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

	GGadgetsCreate(p->explainw,gcd);
	p->explaintext = gcd[0].ret;
	p->explainvals = gcd[4].ret;
    } else
	GGadgetSetTitle(p->explaintext,GStringGetResource(explain,NULL));

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
    GGadgetSetTitle(p->explainvals,ubuf);

    p->doneexplain = false;
    GDrawSetVisible(p->explainw,true);

    SCUpdateAll(sc);		/* We almost certainly just selected something */
    if ( sc!=p->lastcharopened ) {
	if ( sc->views!=NULL )
	    GDrawRaise(sc->views->gw);
	else
	    CharViewCreate(sc,p->fv);
	p->lastcharopened = sc;
    }

    while ( !p->doneexplain )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(p->explainw,false);
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
	    if ( test->first!=NULL && test->first->prev==NULL ) {
		sp = test->first;
		changed = true;
		while ( 1 ) {
		    sp->selected = true;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		}
		ExplainIt(p,sc,_STR_ProbOpenPath,0,0);
		if ( missing(p,test,NULL))
      goto restart;
	    }
	}
    }

    if ( p->pointstooclose && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->next==NULL )
	    break;
		nsp = sp->next->to;
		if ( (nsp->me.x-sp->me.x)*(nsp->me.x-sp->me.x) + (nsp->me.y-sp->me.y)*(nsp->me.y-sp->me.y) < 2*2 ) {
		    changed = true;
		    sp->selected = nsp->selected = true;
		    ExplainIt(p,sc,_STR_ProbPointsTooClose,0,0);
		    if ( missing(p,test,sp))
  goto restart;
		}
		sp = nsp;
	    } while ( sp!=test->first && !p->finish );
	}
    }

#if 0
    if ( p->missingextrema && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish; test = test->next ) {
	    first = NULL;
	    for ( spline = test->first->next; spline!=NULL && spline!=first && !p->finish; spline=spline->to->next ) {
		if ( !spline->knownlinear ) {
		    double t1, t2, t3, t4;
		    SplineFindInflections(&spline->splines[0],&t1,&t2);
		    SplineFindInflections(&spline->splines[1],&t3,&t4);
		    if (( t1>0 && t1<1 ) || (t2>0 && t2<1 ) || (t3>0 && t3<1) ||
			    (t4>0 && t4<1)) {
			spline->from->selected = true;
			spline->to->selected = true;
			changed = true;
			ExplainIt(p,sc,_STR_ProbMissingExtreme,0,0);
		    }
		}
		if ( first==NULL ) first = spline;
	    }
	}
    }
#endif

    if ( p->xnearval && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->me.x-p->xval<p->near && p->xval-sp->me.x<p->near &&
			sp->me.x!=p->xval ) {
		    changed = true;
		    sp->selected = true;
		    ExplainIt(p,sc,_STR_ProbXNear,sp->me.x,p->xval);
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	}
    }

    if ( p->ynearval && !p->finish ) {
	for ( test=spl; test!=NULL && !p->finish; test=test->next ) {
	    sp = test->first;
	    do {
		if ( sp->me.y-p->yval<p->near && p->yval-sp->me.y<p->near &&
			sp->me.y != p->yval ) {
		    changed = true;
		    sp->selected = true;
		    ExplainIt(p,sc,_STR_ProbYNear,sp->me.y,p->yval);
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	}
    }

    if ( p->ynearstd && !p->finish ) {
	double expected;
	int msg;
	for ( test=spl; test!=NULL && !p->finish; test=test->next ) {
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
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
	}
    }

    if ( p->linenearstd && !p->finish ) {
	double ia = (90-p->fv->sf->italicangle)*(3.1415926535897932/180);
	int hasia = p->fv->sf->italicangle!=0;
	for ( test=spl; test!=NULL && !p->finish; test = test->next ) {
	    first = NULL;
	    for ( spline = test->first->next; spline!=NULL && spline!=first && !p->finish; spline=spline->to->next ) {
		if ( spline->knownlinear ) {
		    double yoff, xoff, angle;
		    int ishor=false, isvert=false, isital=false;
		    yoff = spline->to->me.y-spline->from->me.y;
		    xoff = spline->to->me.x-spline->from->me.x;
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
			spline->from->selected = true;
			spline->to->selected = true;
			changed = true;
			ExplainIt(p,sc,ishor?_STR_ProbLineHor:isvert?_STR_ProbLineVert:_STR_ProbLineItal,0,0);
			if ( missingspline(p,test,spline))
  goto restart;
		    }
		}
		if ( first==NULL ) first = spline;
	    }
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
		h->active = false;
		if ( missinghint(sc->hstem,h))
      goto restarthhint;
	    }
	}
      restartvhint:
	for ( h=sc->vstem; h!=NULL ; h=h->next ) {
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
		if ( missinghint(sc->vstem,h))
      goto restartvhint;
		h->active = false;
	    }
	}
    }

    if ( p->ptnearhint && !p->finish ) {
	double found, expected;
	for ( test=spl; test!=NULL && !p->finish; test=test->next ) {
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
		    if ( missing(p,test,sp))
  goto restart;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=test->first && !p->finish );
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
	    if ( (hs!=NULL && missinghint(sc->hstem,hs)) &&
		    ( vs!=NULL && missinghint(sc->vstem,vs)))
      goto restart;
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
	for ( i=0; i<p->fv->sf->charcnt && !p->finish; ++i )
	    if ( p->fv->selected[i] && (sc = p->fv->sf->chars[i])!=NULL ) {
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
	hintnopt = p->hintwithnopt = GGadgetIsChecked(GWidgetGetControl(gw,CID_HintNoPt));
	ptnearhint = p->ptnearhint = GGadgetIsChecked(GWidgetGetControl(gw,CID_PtNearHint));
	hintwidth = p->hintwidthnearval = GGadgetIsChecked(GWidgetGetControl(gw,CID_HintWidthNear));
	explain = p->explain = GGadgetIsChecked(GWidgetGetControl(gw,CID_Explain));
	if ( doxnear )
	    p->xval = xval = GetDoubleR(gw,CID_XNearVal,_STR_XNear,&errs);
	if ( doynear )
	    p->yval = yval = GetDoubleR(gw,CID_YNearVal,_STR_YNear,&errs);
	if ( hintwidth )
	    widthval = p->widthval = GetDoubleR(gw,CID_HintWidth,_STR_HintWidth,&errs);
	near = p->near = GetDoubleR(gw,CID_Near,_STR_Near,&errs);
	if ( errs )
return( true );
	if ( doynearstd )
	    FigureStandardHeights(p);
	GDrawSetVisible(gw,false);
	if ( openpaths || pointstooclose /*|| missing*/ || doxnear || doynear ||
		doynearstd || linestd || hintnopt || ptnearhint || hintwidth ) {
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

void FindProblems(FontView *fv,CharView *cv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20];
    GTextInfo label[20];
    struct problems p;
    char xnbuf[20], ynbuf[20], widthbuf[20], nearbuf[20];

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Findprobs,NULL);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,281);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&p,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&p,0,sizeof(p));

    if ( fv==NULL ) fv = cv->fv;
    p.fv = fv; p.cv=cv;
    if ( cv!=NULL )
	p.lastcharopened = cv->sc;

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

#if 0
    label[2].text = (unichar_t *) _STR_MissingExtrema;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'E';
    gcd[2].gd.pos.x = 6; gcd[2].gd.pos.y = gcd[1].gd.pos.y+17; 
    gcd[2].gd.flags = gg_visible | gg_enabled;
    if ( missing ) gcd[2].gd.flags |= gg_cb_on;
    gcd[2].gd.popup_msg = me;
    gcd[2].gd.cid = CID_MissingExtrema;
    gcd[2].creator = GCheckBoxCreate;
#endif

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

    label[8].text = (unichar_t *) _STR_HintNoPt;
    label[8].text_in_resource = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.mnemonic = 'H';
    gcd[8].gd.pos.x = 6; gcd[8].gd.pos.y = gcd[7].gd.pos.y+17; 
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

    label[12].text = (unichar_t *) _STR_PointsNear;
    label[12].text_in_resource = true;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.mnemonic = 'N';
    gcd[12].gd.pos.x = 6; gcd[12].gd.pos.y = gcd[11].gd.pos.y+40; 
    gcd[12].gd.flags = gg_visible | gg_enabled;
    gcd[12].creator = GLabelCreate;

    sprintf(nearbuf,"%g",near);
    label[13].text = (unichar_t *) nearbuf;
    label[13].text_is_1byte = true;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.pos.x = 130; gcd[13].gd.pos.y = gcd[12].gd.pos.y-6; gcd[13].gd.pos.width = 40;
    gcd[13].gd.flags = gg_visible | gg_enabled;
    gcd[13].gd.cid = CID_Near;
    gcd[13].creator = GTextFieldCreate;

    label[14].text = (unichar_t *) _STR_ExplainErr;
    label[14].text_in_resource = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.mnemonic = 'A';
    gcd[14].gd.pos.x = 6; gcd[14].gd.pos.y = gcd[12].gd.pos.y+24; 
    gcd[14].gd.flags = gg_visible | gg_enabled;
    if ( explain ) gcd[14].gd.flags |= gg_cb_on;
    gcd[14].gd.cid = CID_Explain;
    gcd[14].creator = GCheckBoxCreate;

    gcd[15].gd.pos.x = 15-3; gcd[15].gd.pos.y = gcd[14].gd.pos.y+24;
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

    gcd[18].gd.pos.x = 10; gcd[18].gd.pos.y = gcd[11].gd.pos.y+27;
    gcd[18].gd.pos.width = 200-20;
    gcd[18].gd.flags = gg_enabled | gg_visible;
    gcd[18].creator = GLineCreate;

    GGadgetsCreate(gw,gcd);

    GDrawSetVisible(gw,true);
    while ( !p.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    if ( p.explainw!=NULL )
	GDrawDestroyWindow(p.explainw);
}
