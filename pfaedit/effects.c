/* Copyright (C) 2003 by George Williams */
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
#include <gkeysym.h>
#include <utype.h>
#include <math.h>
#include "edgelist.h"

void FVOutline(FontView *fv, real width) {
    StrokeInfo si;
    SplineSet *temp, *spl;
    int i, cnt=0, changed;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines )
	++cnt;
    GProgressStartIndicatorR(10,_STR_Outlining,_STR_Outlining,0,cnt,1);

    memset(&si,0,sizeof(si));
    si.removeexternal = true;
    si.radius = width;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines ) {
	SplineChar *sc = fv->sf->chars[i];
	SCPreserveState(sc,false);
	temp = SSStroke(sc->splines,&si,sc);
	for ( spl=sc->splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	SplineSetsCorrect(sc->splines,&changed);
	SCCharChangedUpdate(sc);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void CVOutline(CharView *cv, real width) {
    StrokeInfo si;
    SplineSet *temp, *spl;
    int changed;

    memset(&si,0,sizeof(si));
    si.removeexternal = true;
    si.radius = width;

    CVPreserveState(cv);
    temp = SSStroke(*cv->heads[cv->drawmode],&si,cv->sc);
    for ( spl=*cv->heads[cv->drawmode]; spl->next!=NULL; spl=spl->next );
    spl->next = temp;
    SplineSetsCorrect(*cv->heads[cv->drawmode],&changed);
    CVCharChangedUpdate(cv);
}

static void MVOutline(MetricsView *mv, real width) {
    StrokeInfo si;
    SplineSet *temp, *spl;
    int i, changed;

    memset(&si,0,sizeof(si));
    si.removeexternal = true;
    si.radius = width;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	SCPreserveState(sc,false);
	temp = SSStroke(sc->splines,&si,sc);
	for ( spl=sc->splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	SplineSetsCorrect(sc->splines,&changed);
	SCCharChangedUpdate(sc);
    }
}

void FVInline(FontView *fv, real width, real inset) {
    StrokeInfo si;
    SplineSet *temp, *spl, *temp2;
    int i, cnt=0, changed;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines )
	++cnt;
    GProgressStartIndicatorR(10,_STR_Inlining,_STR_Inlining,0,cnt,1);

    memset(&si,0,sizeof(si));
    si.removeexternal = true;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines ) {
	SplineChar *sc = fv->sf->chars[i];
	SCPreserveState(sc,false);
	si.radius = width;
	temp = SSStroke(sc->splines,&si,sc);
	si.radius = width+inset;
	temp2 = SSStroke(sc->splines,&si,sc);
	for ( spl=sc->splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	for ( ; spl->next!=NULL; spl=spl->next );
	spl->next = temp2;
	SplineSetsCorrect(sc->splines,&changed);
	SCCharChangedUpdate(sc);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void CVInline(CharView *cv, real width, real inset) {
    StrokeInfo si;
    SplineSet *temp, *spl, *temp2;
    int changed;

    memset(&si,0,sizeof(si));
    si.removeexternal = true;

    CVPreserveState(cv);
    si.radius = width;
    temp = SSStroke(*cv->heads[cv->drawmode],&si,cv->sc);
    si.radius = width+inset;
    temp2 = SSStroke(*cv->heads[cv->drawmode],&si,cv->sc);
    for ( spl=*cv->heads[cv->drawmode]; spl->next!=NULL; spl=spl->next );
    spl->next = temp;
    for ( ; spl->next!=NULL; spl=spl->next );
    spl->next = temp2;
    SplineSetsCorrect(*cv->heads[cv->drawmode],&changed);
    CVCharChangedUpdate(cv);
}

static void MVInline(MetricsView *mv, real width, real inset) {
    StrokeInfo si;
    SplineSet *temp, *spl, *temp2;
    int i, changed;

    memset(&si,0,sizeof(si));
    si.removeexternal = true;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	SCPreserveState(sc,false);
	si.radius = width;
	temp = SSStroke(sc->splines,&si,sc);
	si.radius = width+inset;
	temp2 = SSStroke(sc->splines,&si,sc);
	for ( spl=sc->splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	for ( ; spl->next!=NULL; spl=spl->next );
	spl->next = temp2;
	SplineSetsCorrect(sc->splines,&changed);
	SCCharChangedUpdate(sc);
    }
}

static real def_outline_width = 10, def_gap_width = 20;

#define CID_Width	1000
#define CID_Gap		1001

typedef struct outlinedata {
    unsigned int done: 1;
    FontView *fv;
    CharView *cv;
    MetricsView *mv;
    int isinline;
    int wireframe;
    GWindow gw;
} OutlineData;

static int OD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OutlineData *od = GDrawGetUserData(GGadgetGetWindow(g));
	real width, gap;
	int err = 0;

	width = GetRealR(od->gw,CID_Width,_STR_Width,&err);
	if ( od->isinline )
	    gap = GetRealR(od->gw,CID_Gap,_STR_Gap,&err);
	if ( err )
return(true);
	def_outline_width = width;
	if ( od->isinline ) {
	    def_gap_width = gap;
	    if ( od->fv!=NULL )
		FVInline(od->fv,width,gap);
	    else if ( od->cv!=NULL )
		CVInline(od->cv,width,gap);
	    else if ( od->mv!=NULL )
		MVInline(od->mv,width,gap);
	} else {
	    if ( od->fv!=NULL )
		FVOutline(od->fv,width);
	    else if ( od->cv!=NULL )
		CVOutline(od->cv,width);
	    else if ( od->mv!=NULL )
		MVOutline(od->mv,width);
	}
	od->done = true;
    }
return( true );
}

static int OD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OutlineData *od = GDrawGetUserData(GGadgetGetWindow(g));
	od->done = true;
    }
return( true );
}

static int od_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	OutlineData *od = GDrawGetUserData(gw);
	od->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void OutlineDlg(FontView *fv, CharView *cv,MetricsView *mv,int isinline) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9];
    GTextInfo label[9];
    OutlineData od;
    char buffer[20], buffer2[20];
    int i;

    od.done = false;
    od.fv = fv;
    od.cv = cv;
    od.mv = mv;
    od.isinline = isinline;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(isinline?_STR_Inline:_STR_Outline,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
	pos.height = GDrawPointsToPixels(NULL,75);
	od.gw = gw = GDrawCreateTopWindow(NULL,&pos,od_e_h,&od,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	i = 0;
	label[i].text = (unichar_t *) _STR_WidthC;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 7; gcd[i].gd.pos.y = 7+3; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	sprintf( buffer, "%g", def_outline_width );
	label[i].text = (unichar_t *) buffer;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 40; gcd[i].gd.pos.y = 7; gcd[i].gd.pos.width = 40;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_Width;
	gcd[i++].creator = GTextFieldCreate;

	if ( isinline ) {
	    label[i].text = (unichar_t *) _STR_Gap;
	    label[i].text_in_resource = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 90; gcd[i].gd.pos.y = 7+3; 
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i++].creator = GLabelCreate;

	    sprintf( buffer2, "%g", def_gap_width );
	    label[i].text = (unichar_t *) buffer2;
	    label[i].text_is_1byte = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 120; gcd[i].gd.pos.y = 7;  gcd[i].gd.pos.width = 40;
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i].gd.cid = CID_Gap;
	    gcd[i++].creator = GTextFieldCreate;
	}

	gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = 7+32;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _STR_OK;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = OD_OK;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -20; gcd[i].gd.pos.y = 7+32+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _STR_Cancel;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = OD_Cancel;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
	gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
	gcd[i].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);

    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Width));
    GTextFieldSelect(GWidgetGetControl(gw,CID_Width),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !od.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
}

static SplineSet *SpMove(SplinePoint *sp,real offset,SplineSet *cur,SplineSet *lines) {
    SplinePoint *new;
    SplineSet *line;

    new = chunkalloc(sizeof(SplinePoint));
    *new = *sp;
    new->me.x += offset;
    new->nextcp.x += offset;
    new->prevcp.x += offset;
    new->prev = new->next = NULL;

    if ( cur->first==NULL )
	cur->first = new;
    else
	SplineMake(cur->last,new,sp->next->order2);
    cur->last = new;

    line = chunkalloc(sizeof(SplineSet));
    line->first = SplinePointCreate(sp->me.x,sp->me.y);
    line->last = SplinePointCreate(new->me.x,new->me.y);
    SplineMake(line->first,line->last,sp->next->order2);
    line->next = lines;

return( line );
}

static void OrientEdges(SplineSet *base,SplineChar *sc) {
    SplineSet *spl;
    Spline *s, *first;
    EIList el;
    EI *active=NULL, *apt, *e;
    SplineChar dummy;
    int i, waschange, winding, change;

    for ( spl=base; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( s = spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    s->leftedge = false;
	    s->rightedge = false;
	    if ( first==NULL ) first = s;
	}
    }

    memset(&el,'\0',sizeof(el));
    memset(&dummy,'\0',sizeof(dummy));
    dummy.splines = base;
    if ( sc!=NULL ) dummy.name = sc->name;
    ELFindEdges(&dummy,&el);
    el.major = 1;
    ELOrder(&el,el.major);

    waschange = false;
    for ( i=0; i<el.cnt ; ++i ) {
	active = EIActiveEdgesRefigure(&el,active,i,1,&change);
	if ( waschange || change || el.ends[i] || el.ordered[i]!=NULL ||
		(i!=el.cnt-1 && (el.ends[i+1] || el.ordered[i+1]!=NULL)) ) {
	    waschange = change;
    continue;
	}
	waschange = change;
	winding = 0;
	for ( apt=active; apt!=NULL ; apt = e) {
	    if ( EISkipExtremum(apt,i+el.low,1)) {
		e = apt->aenext->aenext;
	continue;
	    }
	    if ( winding==0 )
		apt->spline->leftedge = true;
	    winding += apt->up ? 1 : -1;
	    if ( winding==0 )
		apt->spline->rightedge = true;
	    e = apt->aenext;
	}
    }
    free(el.ordered);
    free(el.ends);
    ElFreeEI(&el);
}

static SplineSet *AddVerticalExtremaAndMove(SplineSet *base,real shadow_length,
	int wireframe,SplineChar *sc) {
    SplineSet *spl, *head=NULL, *last=NULL, *cur, *lines=NULL;
    Spline *s, *first;
    SplinePoint *sp, *found, *new;
    real t[2];
    int p;

    if ( shadow_length==0 )
return(NULL);

    t[0] = t[1] = 0;
    for ( spl=base; spl!=NULL; spl=spl->next ) if ( spl->first->prev!=NULL && spl->first->prev->from!=spl->first ) {
	/* Add any extrema which aren't already splinepoints */
	first = NULL;
	for ( s=spl->first->next; s!=first; s=s->to->next ) {
	    if ( first==NULL ) first = s;
	    p=0;
	    if ( s->splines[1].a!=0 ) {
		double d = 4*s->splines[1].b*s->splines[1].b-4*3*s->splines[1].a*s->splines[1].c;
		if ( d>0 ) {
		    d = sqrt(d);
		    t[p++] = (-2*s->splines[1].b+d)/(2*3*s->splines[1].a);
		    t[p++] = (-2*s->splines[1].b-d)/(2*3*s->splines[1].a);
		}
	    } else if ( s->splines[1].b!=0 )
		t[p++] = -s->splines[1].c/(2*s->splines[1].b);
	    if ( p==2 && (t[1]<=0.0001 || t[1]>=.9999 ))
		--p;
	    if ( p>=1 && (t[0]<=0.0001 || t[0]>=.9999 )) {
		t[0] = t[1];
		--p;
	    }
	    if ( p==2 && t[0]>t[1] )
		t[0] = t[1];
	    if ( p>0 ) {
		sp = SplineBisect(s,t[0]);
		s = sp->prev;
		/* If there were any other t values, ignore them here, we'll */
		/*  find them when we process the next half of the spline */
	    }
	}
    }

    OrientEdges(base,sc);
    for ( spl=base; spl!=NULL; spl=spl->next ) if ( spl->first->prev!=NULL && spl->first->prev->from!=spl->first ) {
	if ( !wireframe ) {
	    /* Make duplicates of any ticked points and move them over */
	    found = spl->first;
	    for ( sp = found; ; ) {
		if ( sp->next->rightedge && sp->prev->rightedge ) {
		    sp->me.x += shadow_length;
		    sp->nextcp.x += shadow_length;
		    sp->prevcp.x += shadow_length;
		    SplineRefigure(sp->prev);
		} else if ( sp->next->rightedge || sp->prev->rightedge ) {
		    new = chunkalloc(sizeof(SplinePoint));
		    *new = *sp;
		    new->ticked = false; sp->ticked = false;
		    if ( sp->next->rightedge ) {
			sp->next->from = new;
			sp->nonextcp = true;
			sp->nextcp = sp->me;
			new->me.x += shadow_length;
			new->nextcp.x += shadow_length;
			new->noprevcp = true;
			new->prevcp = new->me;
			SplineMake(sp,new,sp->prev->order2);
			sp = new;
		    } else {
			sp->prev->to = new;
			sp->noprevcp = true;
			sp->prevcp = sp->me;
			new->me.x += shadow_length;
			new->prevcp.x += shadow_length;
			new->nonextcp = true;
			new->nextcp = new->me;
			SplineMake(new,sp,sp->next->order2);
			SplineRefigure(new->prev);
			if ( sp==found ) found = new;
		    }
		}
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	    SplineRefigure(spl->first->prev);		/* Just in case... */
	} else {
	    cur = NULL;
	    /* Wire frame... do hidden line removal by only copying the */
	    /*  points which would be moved by the above code */
	    for ( sp=spl->first; sp->prev->rightedge || !sp->next->rightedge ; sp = sp->next->to );
	    for ( found = sp; ; ) {
		if ( sp->next->rightedge && sp->prev->rightedge ) {
		    lines = SpMove(sp,shadow_length,cur,lines);
		} else if ( sp->next->rightedge ) {
		    cur = chunkalloc(sizeof(SplineSet));
		    if ( last==NULL )
			head = cur;
		    else
			last->next = cur;
		    last = cur;
		    lines = SpMove(sp,shadow_length,cur,lines);
		} else if ( sp->prev->rightedge ) {
		    lines = SpMove(sp,shadow_length,cur,lines);
		    cur = NULL;
		}
		sp = sp->next->to;
		if ( sp==found )
	    break;
	    }
	    last->next = lines;
	}
    }
return( head );
}

static void SSSelectAll(SplineSet *spl,int select) {
    SplinePoint *sp;

    while ( spl!=NULL ) {
	for ( sp=spl->first; ; ) {
	    sp->selected = select;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	spl = spl->next;
    }
}

static void SSCleanup(SplineSet *spl) {
    SplinePoint *sp;
    Spline *s, *first;
    /* look for likely rounding errors (caused by the two rotations) and */
    /*  get rid of them */

    while ( spl!=NULL ) {
	for ( sp=spl->first; ; ) {
	    sp->me.x = rint(sp->me.x*64)/64.;
	    sp->me.y = rint(sp->me.y*64)/64.;
	    sp->nextcp.x = rint(sp->nextcp.x*64)/64.;
	    sp->nextcp.y = rint(sp->nextcp.y*64)/64.;
	    sp->prevcp.x = rint(sp->prevcp.x*64)/64.;
	    sp->prevcp.y = rint(sp->prevcp.y*64)/64.;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	first = NULL;
	/* look for things which should be horizontal or vertical lines and make them so */
	for ( s=spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    real xdiff = s->to->me.x-s->from->me.x, ydiff = s->to->me.y-s->from->me.y;
	    real x,y;
	    if (( xdiff<.01 && xdiff>-.01 ) && (ydiff<-10 || ydiff>10)) {
		xdiff /= 2;
		s->to->me.x = (s->from->me.x += xdiff);
		s->from->prevcp.x += xdiff;
		s->from->nextcp.x += xdiff;
		s->to->prevcp.x -= xdiff;
		s->to->nextcp.x -= xdiff;
		if ( s->to->nonextcp ) s->to->nextcp.x = s->to->me.x;
		if ( s->to->noprevcp ) s->to->prevcp.x = s->to->me.x;
	    } else if (( ydiff<.01 && ydiff>-.01 ) && (xdiff<-10 || xdiff>10)) {
		ydiff /= 2;
		s->to->me.y = (s->from->me.y += ydiff);
		s->from->prevcp.y += ydiff;
		s->from->nextcp.y += ydiff;
		s->to->prevcp.y -= ydiff;
		s->to->nextcp.y -= ydiff;
		if ( s->to->nonextcp ) s->to->nextcp.y = s->to->me.y;
		if ( s->to->noprevcp ) s->to->prevcp.y = s->to->me.y;
	    }
	    xdiff = s->from->nextcp.x-s->from->me.x; ydiff = s->from->nextcp.y-s->from->me.y;
	    if (( xdiff<.01 && xdiff>-.01 ) && (ydiff<-10 || ydiff>10))
		s->from->nextcp.x = s->from->me.x;
	    if (( ydiff<.01 && ydiff>-.01 ) && (xdiff<-10 || xdiff>10))
		s->from->nextcp.y = s->from->me.y;
	    xdiff = s->to->prevcp.x-s->to->me.x; ydiff = s->to->prevcp.y-s->to->me.y;
	    if (( xdiff<.01 && xdiff>-.01 ) && (ydiff<-10 || ydiff>10))
		s->to->prevcp.x = s->to->me.x;
	    if (( ydiff<.01 && ydiff>-.01 ) && (xdiff<-10 || xdiff>10))
		s->to->prevcp.y = s->to->me.y;
	    x = s->from->me.x; y = s->from->me.y;
	    if ( x==s->from->nextcp.x && x==s->to->prevcp.x && x==s->to->me.x &&
		    ((y<s->to->me.y && s->from->nextcp.y>=y && s->from->nextcp.y<=s->to->prevcp.y && s->to->prevcp.y<=s->to->me.y) ||
		     (y>=s->to->me.y && s->from->nextcp.y<=y && s->from->nextcp.y>=s->to->prevcp.y && s->to->prevcp.y>=s->to->me.y))) {
		s->from->nonextcp = true; s->to->noprevcp = true;
		s->from->nextcp = s->from->me;
		s->to->prevcp = s->to->me;
	    }
	    if ( y==s->from->nextcp.y && y==s->to->prevcp.y && y==s->to->me.y &&
		    ((x<s->to->me.x && s->from->nextcp.x>=x && s->from->nextcp.x<=s->to->prevcp.x && s->to->prevcp.x<=s->to->me.x) ||
		     (x>=s->to->me.x && s->from->nextcp.x<=x && s->from->nextcp.x>=s->to->prevcp.x && s->to->prevcp.x>=s->to->me.x))) {
		s->from->nonextcp = true; s->to->noprevcp = true;
		s->from->nextcp = s->from->me;
		s->to->prevcp = s->to->me;
	    }
	    SplineRefigure(s);
	    if ( first==NULL ) first = s;
	}
	spl = spl->next;
    }
}

static SplineSet *SSShadow(SplineSet *spl,real angle, real outline_width,
	real shadow_length,SplineChar *sc, int wireframe) {
    real trans[6];
    StrokeInfo si;
    SplineSet *internal, *temp, *frame, *fatframe, *outline, *mask;
    int isfore = spl==sc->splines;

    if ( spl==NULL )
return( NULL );

    trans[0] = trans[3] = cos(angle);
    trans[2] = sin(angle);
    trans[1] = -trans[2];
    trans[4] = trans[5] = 0;
    spl = SplinePointListTransform(spl,trans,true);
    SSCleanup(spl);

    internal = NULL;
    if ( outline_width!=0 && !wireframe ) {
	memset(&si,0,sizeof(si));
	si.removeexternal = true;
	si.removeoverlapifneeded = true;
	si.radius = outline_width;
	temp = SplinePointListCopy(spl);	/* SSStroke confuses the direction I think */
	internal = SSStroke(temp,&si,sc);
	SplinePointListsFree(temp);
	SplineSetsAntiCorrect(internal);
    }

    frame = AddVerticalExtremaAndMove(spl,shadow_length,wireframe,sc);
    if ( !wireframe ) 
	spl = SplineSetRemoveOverlap(spl,over_remove);	/* yes, spl, NOT frame. frame is always NULL if !wireframe */
    else {
	if ( outline_width!=0 ) {
	    memset(&si,0,sizeof(si));
	    si.radius = outline_width/2;
	    fatframe = SSStroke(frame,&si,sc);
	    SplinePointListsFree(frame); frame = fatframe;
	    outline = SSStroke(spl,&si,sc);
	    SSSelectAll(frame,false);
	    si.radius = outline_width/3;
	    si.removeinternal = true;
	    mask = SSStroke(spl,&si,sc);
	    SSSelectAll(mask,true);
	    for ( temp=mask; temp->next!=NULL; temp=temp->next);
	    temp->next = frame;
return( mask );
	    frame = SplineSetRemoveOverlap(mask,over_exclude);
#if 0
	    SplinePointListsFree(spl);
	    for ( temp=outline; temp->next!=NULL; temp=temp->next);
	    temp->next = frame;
	    spl = SplineSetRemoveOverlap(outline,over_remove);
#else
	    SplinePointListsFree(spl);
	    for ( temp=outline; temp->next!=NULL; temp=temp->next);
	    temp->next = frame;
	    spl = outline;
#endif
	} else {
	    for ( temp=spl; temp->next!=NULL; temp=temp->next);
	    temp->next = frame;
	}
    }

    if ( internal!=NULL ) {
	for ( temp = spl; temp->next!=NULL; temp=temp->next );
	temp->next = internal;
    }

    trans[1] = -trans[1]; trans[2] = -trans[2];
    spl = SplinePointListTransform(spl,trans,true);	/* rotate back */
    SSCleanup(spl);
    if ( isfore ) {
	sc->width += fabs(trans[0]*shadow_length);
	if ( trans[0]<0 ) {
	    trans[4] = -trans[0]*shadow_length;
	    trans[0] = trans[3] = 1;
	    trans[1] = trans[2] = 0;
	    spl = SplinePointListTransform(spl,trans,true);
	}
    }

return( spl );
}
    
void FVShadow(FontView *fv,real angle, real outline_width,
	real shadow_length, int wireframe) {
    int i, cnt=0;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines )
	++cnt;
    GProgressStartIndicatorR(10,_STR_Shadowing,_STR_Shadowing,0,cnt,1);

    for ( i=0; i<fv->sf->charcnt; ++i )
	    if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines ) {
	SplineChar *sc = fv->sf->chars[i];
	SCPreserveState(sc,false);
	sc->splines = SSShadow(sc->splines,angle,outline_width,shadow_length,sc,wireframe);
	SCCharChangedUpdate(sc);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void CVShadow(CharView *cv,real angle, real outline_width,
	real shadow_length,int wireframe) {
    CVPreserveState(cv);
    *cv->heads[cv->drawmode] = SSShadow(*cv->heads[cv->drawmode],angle,outline_width,shadow_length,cv->sc,wireframe);
    CVCharChangedUpdate(cv);
}

static void MVShadow(MetricsView *mv,real angle, real outline_width,
	real shadow_length,int wireframe) {
    int i;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	SCPreserveState(sc,false);
	sc->splines = SSShadow(sc->splines,angle,outline_width,shadow_length,sc,wireframe);
	SCCharChangedUpdate(sc);
    }
}

static real def_shadow_len=100, def_sun_angle= -45;

#define CID_ShadowLen		1001
#define CID_LightAngle		1002

static int SD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OutlineData *od = GDrawGetUserData(GGadgetGetWindow(g));
	real width, angle, len;
	int err = 0;

	width = GetRealR(od->gw,CID_Width,_STR_Width,&err);
	len = GetRealR(od->gw,CID_ShadowLen,_STR_ShadowLen,&err);
	angle = GetRealR(od->gw,CID_LightAngle,_STR_LightAngle,&err);
	if ( err )
return(true);
	def_outline_width = width;
	def_shadow_len = len;
	def_sun_angle = angle;
	angle *= -3.1415926535897932/180;
	angle -= 3.1415926535897932/2;
	if ( od->fv!=NULL )
	    FVShadow(od->fv,angle,width,len,od->wireframe);
	else if ( od->cv!=NULL )
	    CVShadow(od->cv,angle,width,len,od->wireframe);
	else if ( od->mv!=NULL )
	    MVShadow(od->mv,angle,width,len,od->wireframe);
	od->done = true;
    }
return( true );
}

void ShadowDlg(FontView *fv, CharView *cv,MetricsView *mv,int wireframe) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[10];
    OutlineData od;
    char buffer[20], buffer2[20], buffer3[20];
    int i;

    od.done = false;
    od.fv = fv;
    od.cv = cv;
    od.mv = mv;
    od.wireframe = wireframe;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_Shadow,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,160));
	pos.height = GDrawPointsToPixels(NULL,125);
	od.gw = gw = GDrawCreateTopWindow(NULL,&pos,od_e_h,&od,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	i = 0;
	label[i].text = (unichar_t *) _STR_OutlineWidth;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 7; gcd[i].gd.pos.y = 7+3; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	sprintf( buffer, "%g", def_outline_width );
	label[i].text = (unichar_t *) buffer;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 90; gcd[i].gd.pos.y = 7; gcd[i].gd.pos.width = 50;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_Width;
	gcd[i++].creator = GTextFieldCreate;

	label[i].text = (unichar_t *) _STR_ShadowLen;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	sprintf( buffer2, "%g", def_shadow_len );
	label[i].text = (unichar_t *) buffer2;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-3;  gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_ShadowLen;
	gcd[i++].creator = GTextFieldCreate;

	label[i].text = (unichar_t *) _STR_LightAngle;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	sprintf( buffer3, "%g", def_sun_angle );
	label[i].text = (unichar_t *) buffer3;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-3;  gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_LightAngle;
	gcd[i++].creator = GTextFieldCreate;

	gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+30;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _STR_OK;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = SD_OK;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -20; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _STR_Cancel;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = OD_Cancel;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
	gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
	gcd[i].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);

    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_ShadowLen));
    GTextFieldSelect(GWidgetGetControl(gw,CID_ShadowLen),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !od.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
}
