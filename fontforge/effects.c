/* Copyright (C) 2003-2012 by George Williams */
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

#include "effects.h"

#include "autohint.h"
#include "cvundoes.h"
#include "fontforgevw.h"
#include "splineoverlap.h"
#include "splinestroke.h"
#include "splineutil2.h"
#include <ustring.h>
#include <gkeysym.h>
#include <utype.h>
#include <math.h>
#include "edgelist.h"

void FVOutline(FontViewBase *fv, real width) {
    StrokeInfo si;
    SplineSet *temp, *spl;
    int i, cnt=0, changed, gid;
    SplineChar *sc;
    int layer = fv->active_layer;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( (gid=fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL &&
		fv->selected[i] && sc->layers[layer].splines )
	    ++cnt;
    ff_progress_start_indicator(10,_("Outlining glyphs"),_("Outlining glyphs"),0,cnt,1);

    memset(&si,0,sizeof(si));
    si.removeexternal = true;
    si.radius = width;
    /*si.removeoverlapifneeded = true;*/

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i )
	if ( (gid=fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL && fv->selected[i] &&
		sc->layers[layer].splines && !sc->ticked ) {
	    sc->ticked = true;
	    SCPreserveLayer(sc,layer,false);
	    temp = SplineSetStroke(sc->layers[layer].splines,&si,sc->layers[layer].order2);
	    for ( spl=sc->layers[layer].splines; spl->next!=NULL; spl=spl->next );
	    spl->next = temp;
	    SplineSetsCorrect(sc->layers[layer].splines,&changed);
	    SCCharChangedUpdate(sc,layer);
	    if ( !ff_progress_next())
    break;
	}
    ff_progress_end_indicator();
}

void FVInline(FontViewBase *fv, real width, real inset) {
    StrokeInfo si;
    SplineSet *temp, *spl, *temp2;
    int i, cnt=0, changed, gid;
    SplineChar *sc;
    int layer = fv->active_layer;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( (gid=fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL && fv->selected[i] &&
		sc->layers[layer].splines )
	    ++cnt;
    ff_progress_start_indicator(10,_("Inlining glyphs"),_("Inlining glyphs"),0,cnt,1);

    memset(&si,0,sizeof(si));
    si.removeexternal = true;
    /*si.removeoverlapifneeded = true;*/

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i )
	if ( (gid=fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL && fv->selected[i] &&
		sc->layers[layer].splines && !sc->ticked ) {
	    sc->ticked = true;
	    SCPreserveLayer(sc,layer,false);
	    si.radius = width;
	    temp = SplineSetStroke(sc->layers[layer].splines,&si,sc->layers[layer].order2);
	    si.radius = width+inset;
	    temp2 = SplineSetStroke(sc->layers[layer].splines,&si,sc->layers[layer].order2);
	    for ( spl=sc->layers[layer].splines; spl->next!=NULL; spl=spl->next );
	    spl->next = temp;
	    for ( ; spl->next!=NULL; spl=spl->next );
	    spl->next = temp2;
	    SplineSetsCorrect(sc->layers[layer].splines,&changed);
	    SCCharChangedUpdate(sc,layer);
	    if ( !ff_progress_next())
    break;
	}
    ff_progress_end_indicator();
}

/**************************************************************************** */
/* Shadow */
/**************************************************************************** */
static SplineSet *SpMove(SplinePoint *sp,real offset,
	SplineSet *cur,SplineSet *lines,
	SplineSet *spl) {
    SplinePoint *new;
    SplineSet *line;
    BasePoint test;

    new = chunkalloc(sizeof(SplinePoint));
    *new = *sp;
    new->hintmask = NULL;
    new->me.x += offset;
    new->nextcp.x += offset;
    new->prevcp.x += offset;
    new->prev = new->next = NULL;

    if ( cur->first==NULL ) {
	cur->first = new;
	cur->start_offset = 0;
    } else
	SplineMake(cur->last,new,sp->next->order2);
    cur->last = new;

    /* Does the line segment we want to create immediately move inside of the */
    /*  main contour? If so we aren't interested in it */
    test = sp->me;
    ++test.x;
    if ( !SSPointWithin(spl,&test)) {
	line = chunkalloc(sizeof(SplineSet));
	line->first = SplinePointCreate(sp->me.x,sp->me.y);
	line->last = SplinePointCreate(new->me.x,new->me.y);
	SplineMake(line->first,line->last,sp->next->order2);
	line->next = lines;
	lines = line;
    }

return( lines );
}

static void OrientEdges(SplineSet *base,SplineChar *sc) {
    SplineSet *spl;
    Spline *s, *first;
    EIList el;
    EI *active=NULL, *apt, *e;
    SplineChar dummy;
    Layer layers[2];
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
    memset(layers,'\0',sizeof(layers));
    el.layer = ly_fore;
    dummy.layers = layers;
    dummy.layers[ly_fore].splines = base;
    if ( sc!=NULL ) dummy.name = sc->name;
    ELFindEdges(&dummy,&el);
    if ( el.coordmax[1]-el.coordmin[1] > 1.e6 ) {
	fprintf( stderr, "Warning: Unreasonably big splines. They will be ignored.\n" );
return;
    }
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
	int wireframe,SplineChar *sc,SplineSet **_lines) {
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
		bigreal d = 4*s->splines[1].b*s->splines[1].b-4*3*s->splines[1].a*s->splines[1].c;
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
		    new->hintmask = NULL;
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
		    lines = SpMove(sp,shadow_length,cur,lines,base);
		} else if ( sp->next->rightedge ) {
		    cur = chunkalloc(sizeof(SplineSet));
		    if ( last==NULL )
			head = cur;
		    else
			last->next = cur;
		    last = cur;
		    lines = SpMove(sp,shadow_length,cur,lines,base);
		} else if ( sp->prev->rightedge ) {
		    lines = SpMove(sp,shadow_length,cur,lines,base);
		    cur = NULL;
		}
		sp = sp->next->to;
		if ( sp==found )
	    break;
	    }
	    *_lines = lines;
	}
    }
return( head );
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

static bigreal IntersectLine(Spline *spline1,Spline *spline2) {
    extended t1s[10], t2s[10];
    BasePoint pts[9];
    bigreal mint=1;
    int i;

    if ( !SplinesIntersect(spline1,spline2,pts,t1s,t2s))
return( -1 );
    for ( i=0; i<10 && t1s[i]!=-1; ++i ) {
	if ( t1s[i]<.001 || t1s[i]>.999 )
	    /* Too close to end point, ignore it */;
	else if ( t1s[i]<mint )
	    mint = t1s[i];
    }
    if ( mint == 1 )
return( -1 );

return( mint );
}

static int ClipLineTo3D(Spline *line,SplineSet *spl) {
    bigreal t= -1, cur;
    Spline *s, *first;

    while ( spl!=NULL ) {
	first = NULL;
	for ( s = spl->first->next; s!=NULL && s!=first ; s = s->to->next ) {
	    cur = IntersectLine(line,s);
	    if ( cur>.001 && (t==-1 || cur<t))
		t = cur;
	    if ( first==NULL ) first = s;
	}
	spl = spl->next;
    }
    if ( t!=-1 ) {
	SplinePoint *from = line->from;
	SplineBisect(line,t);
	line = from->next;
	SplinePointFree(line->to->next->to);
	SplineFree(line->to->next);
	line->to->next = NULL;
return( true );
    }
return( false );
}

/* finds all intersections between this spline and all the other splines in the */
/*  character */
static extended *BottomFindIntersections(Spline *bottom,SplineSet *lines,SplineSet *spl) {
    extended *ts;
    int tcnt, tmax;
    extended t1s[26], t2s[26];
    BasePoint pts[25];
    Spline *first, *s;
    int i,j;

    tmax = 100;
    ts = malloc(tmax*sizeof(extended));
    tcnt = 0;

    while ( spl!=NULL ) {
	first = NULL;
	for ( s = spl->first->next; s!=NULL && s!=first ; s = s->to->next ) {
	    if ( SplinesIntersect(bottom,s,pts,t1s,t2s)) {
		for ( i=0; i<25 && t1s[i]!=-1; ++i ) if ( t2s[i]>.001 && t2s[i]<.999 ) {
		    if ( tcnt>=tmax ) {
			tmax += 100;
			ts = realloc(ts,tmax*sizeof(extended));
		    }
		    ts[tcnt++] = t1s[i];
		}
	    }
	    if ( first==NULL ) first = s;
	}
	spl = spl->next;
    }
    while ( lines!=NULL ) {
	first = NULL;
	for ( s = lines->first->next; s!=NULL && s!=first ; s = s->to->next ) {
	    if ( SplinesIntersect(bottom,s,pts,t1s,t2s)) {
		for ( i=0; i<25 && t1s[i]!=-1; ++i ) if ( t2s[i]>.001 && t2s[i]<.999 ) {
		    if ( tcnt>=tmax ) {
			tmax += 100;
			ts = realloc(ts,tmax*sizeof(extended));
		    }
		    ts[tcnt++] = t1s[i];
		}
	    }
	    if ( first==NULL ) first = s;
	}
	lines = lines->next;
    }
    if ( tcnt==0 ) {
	free(ts);
return( NULL );
    }
    for ( i=0; i<tcnt; ++i ) for ( j=i+1; j<tcnt; ++j ) {
	if ( ts[i]>ts[j] ) {
	    extended temp = ts[i];
	    ts[i] = ts[j];
	    ts[j] = temp;
	}
    }
    for ( i=j=1; i<tcnt; ++i )
	if ( ts[i]!=ts[i-1] )
	    ts[j++] = ts[i];
    ts[j] = -1;
return( ts );
}

static int LineAtPointCompletes(SplineSet *lines,BasePoint *pt) {
    while ( lines!=NULL ) {
	if ( lines->last->me.x==pt->x && lines->last->me.y==pt->y )
return( true );
	lines = lines->next;
    }
return( false );
}

static SplinePoint *SplinePointMidCreate(Spline *s,bigreal t) {
return( SplinePointCreate(
	((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d,
	((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d
    ));
}

static int MidLineCompetes(Spline *s,bigreal t,bigreal shadow_length,SplineSet *spl) {
    SplinePoint *to = SplinePointMidCreate(s,t);
    SplinePoint *from = SplinePointCreate(to->me.x-shadow_length,to->me.y);
    Spline *line = SplineMake(from,to,s->order2);
    int ret;

    ret = ClipLineTo3D(line,spl);
    if ( ret == false ) {
        SplinePointFree(line->to);		/* This might not be the same as to */
        SplinePointFree(line->from);	/* This will be the same as from */
        SplineFree(line);
    }
return( !ret );
}

static void SplineComplete(SplineSet *cur,Spline *s,bigreal t_of_from,bigreal t_of_to) {
    SplinePoint *to;
    bigreal dt = t_of_to-t_of_from;
    Spline1D x,y;
    /* Very similar to SplineBisect */

    to = SplinePointMidCreate(s,t_of_to);

    x.d = cur->last->me.x;
    x.c = dt*(s->splines[0].c + t_of_from*(2*s->splines[0].b + 3*s->splines[0].a*t_of_from));
    x.b = dt*dt*(s->splines[0].b + 3*s->splines[0].a*t_of_from);
    x.a = dt*dt*dt*s->splines[0].a;
    cur->last->nextcp.x = x.c/3 + x.d;
    to->prevcp.x = cur->last->nextcp.x + (x.b+x.c)/3;

    y.d = cur->last->me.y;
    y.c = dt*(s->splines[1].c + t_of_from*(2*s->splines[1].b + 3*s->splines[1].a*t_of_from));
    y.b = dt*dt*(s->splines[1].b + 3*s->splines[1].a*t_of_from);
    y.a = dt*dt*dt*s->splines[1].a;
    cur->last->nextcp.y = y.c/3 + y.d;
    to->prevcp.y = cur->last->nextcp.y + (y.b+y.c)/3;
    to->noprevcp = cur->last->nonextcp = false;

    SplineMake(cur->last,to,s->order2);
    cur->last = to;
}

/* I wish I did not need this routine, but unfortunately my remove overlap */
/*  gets very confused by two splines which are parrallel, and without this */
/*  fixup we get a lot of those at the edges */
static SplineSet *MergeLinesToBottoms(SplineSet *bottoms,SplineSet *lines) {
    SplineSet *prev, *l;

    while ( bottoms!=NULL ) {
	for ( prev=NULL, l=lines;
		l!=NULL && (l->last->me.x!=bottoms->first->me.x || l->last->me.y!=bottoms->first->me.y);
		prev=l, l=l->next );
	if ( l!=NULL ) {
	    if ( prev==NULL )
		lines = l->next;
	    else
		prev->next = l->next;
	    SplineMake(l->first,bottoms->first,l->first->next->order2);
	    bottoms->first = l->first;
	    bottoms->start_offset = 0;
	    SplineFree(l->last->prev);
	    SplinePointFree(l->last);
	    chunkfree(l,sizeof(*l));
	}
	for ( prev=NULL, l=lines;
		l!=NULL && (l->last->me.x!=bottoms->last->me.x || l->last->me.y!=bottoms->last->me.y);
		prev=l, l=l->next );
	if ( l!=NULL ) {
	    if ( prev==NULL )
		lines = l->next;
	    else
		prev->next = l->next;
	    SplineMake(bottoms->last,l->first,l->first->next->order2);
	    bottoms->last = l->first;
	    l->first->next = NULL;
	    SplineFree(l->last->prev);
	    SplinePointFree(l->last);
	    chunkfree(l,sizeof(*l));
	}
	bottoms = bottoms->next;
    }
return( lines );
}

static SplineSet *ClipBottomTo3D(SplineSet *bottom,SplineSet *lines,SplineSet *spl,
	bigreal shadow_length) {
    SplineSet *head=NULL, *last=NULL, *cur, *next;
    Spline *s;
    extended *ts;
    SplinePoint *sp;
    int i;

    while ( bottom!=NULL ) {
	next = bottom->next;
	cur = NULL;
	for ( s=bottom->first->next; s!=NULL ; s = s->to->next ) {
	    if ( LineAtPointCompletes(lines,&s->from->me) && cur==NULL ) {
		cur = chunkalloc(sizeof(SplineSet));
		cur->first = cur->last = SplinePointCreate(s->from->me.x,s->from->me.y);
		if ( head==NULL )
		    head = cur;
		else
		    last->next = cur;
		last = cur;
	    }
	    ts = BottomFindIntersections(s,lines,spl);
	    if ( ts==NULL || ts[0]==-1 ) {
		if ( cur!=NULL ) {
		    cur->last->nextcp = s->from->nextcp;
		    cur->last->nextcpdef = s->from->nextcpdef;
		    cur->last->nonextcp = s->from->nonextcp;
		    sp = SplinePointCreate(s->to->me.x,s->to->me.y);
		    sp->prevcp = s->to->prevcp;
		    sp->prevcpdef = s->to->prevcpdef;
		    sp->noprevcp = s->to->prevcpdef;
		    SplineMake(cur->last,sp,s->order2);
		    cur->last = sp;
		}
	    } else {
		i = 0;
		if ( cur!=NULL ) {
		    SplineComplete(cur,s,0,ts[0]);
		    cur = NULL;
		    i=1;
		}
		while ( ts[i]!=-1 ) {
		    bigreal tend = ts[i+1]==-1 ? 1 : ts[i+1];
		    if ( MidLineCompetes(s,(ts[i]+tend)/2,shadow_length,spl)) {
			cur = chunkalloc(sizeof(SplineSet));
			cur->first = cur->last = SplinePointMidCreate(s,ts[i]);
			if ( head==NULL )
			    head = cur;
			else
			    last->next = cur;
			last = cur;
			SplineComplete(cur,s,ts[i],tend);
			if ( ts[i+1]==-1 )
		break;
			cur = NULL;
			i += 2;
		    } else
			++i;
		}
		free(ts);
	    }
	}
	SplinePointListFree(bottom);
	bottom = next;
    }
return( head );
}
    
static SplineSet *ClipTo3D(SplineSet *bottoms,SplineSet *lines,SplineSet *spl,
	bigreal shadow_length) {
    SplineSet *temp;
    SplineSet *head;

    for ( temp=lines; temp!=NULL; temp=temp->next ) {
	ClipLineTo3D(temp->first->next,spl);
	temp->last = temp->first->next->to;
    }
    head = ClipBottomTo3D(bottoms,lines,spl,shadow_length);
    lines = MergeLinesToBottoms(head,lines);
    if ( lines!=NULL ) {
	for ( temp=lines; temp->next!=NULL; temp=temp->next);
	temp->next = head;
return( lines );
    } else
return( head );
}

SplineSet *SSShadow(SplineSet *spl,real angle, real outline_width,
	real shadow_length,SplineChar *sc, int wireframe) {
    real trans[6];
    StrokeInfo si;
    SplineSet *internal, *temp, *bottom, *fatframe, *lines;
    int isfore = spl==sc->layers[ly_fore].splines;
    int order2=false;

    if ( spl==NULL )
return( NULL );
    for ( temp=spl; temp!=NULL; temp=temp->next ) {
	if ( temp->first->next != NULL ) {
	    order2 = temp->first->next->order2;
    break;
	}
    }

    trans[0] = trans[3] = cos(angle);
    trans[2] = sin(angle);
    trans[1] = -trans[2];
    trans[4] = trans[5] = 0;
    spl = SplinePointListTransform(spl,trans,tpt_AllPoints);
    SSCleanup(spl);

    internal = NULL;
    if ( outline_width!=0 && !wireframe ) {
	memset(&si,0,sizeof(si));
	si.removeexternal = true;
	/*si.removeoverlapifneeded = true;*/
	si.radius = outline_width;
	temp = SplinePointListCopy(spl);	/* SplineSetStroke confuses the direction I think */
	internal = SplineSetStroke(temp,&si,order2);
	SplinePointListsFree(temp);
	SplineSetsAntiCorrect(internal);
    }

    lines = NULL;
    bottom = AddVerticalExtremaAndMove(spl,shadow_length,wireframe,sc,&lines);
    if ( !wireframe ) 
	spl = SplineSetRemoveOverlap(sc,spl,over_remove);	/* yes, spl, NOT frame. frame is always NULL if !wireframe */
    else {
	lines = ClipTo3D(bottom,lines,spl,shadow_length);
	for ( temp=spl; temp->next!=NULL; temp=temp->next);
	temp->next = lines;
	if ( outline_width!=0 ) {
	    memset(&si,0,sizeof(si));
	    si.radius = outline_width/2;
	    /*si.removeoverlapifneeded = true;*/
	    fatframe = SplineSetStroke(spl,&si,order2);
	    SplinePointListsFree(spl);
	    spl = fatframe; /* Don't try SplineSetRemoveOverlap: too likely to cause remove overlap problems. */
	}
    }

    if ( internal!=NULL ) {
	for ( temp = spl; temp->next!=NULL; temp=temp->next );
	temp->next = internal;
    }

    trans[1] = -trans[1]; trans[2] = -trans[2];
    spl = SplinePointListTransform(spl,trans,tpt_AllPoints);	/* rotate back */
    SSCleanup(spl);
    if ( isfore ) {
	sc->width += fabs(trans[0]*shadow_length);
	if ( trans[0]<0 ) {
	    trans[4] = -trans[0]*shadow_length;
	    trans[0] = trans[3] = 1;
	    trans[1] = trans[2] = 0;
	    spl = SplinePointListTransform(spl,trans,tpt_AllPoints);
	}
    }

return( spl );
}

void FVShadow(FontViewBase *fv,real angle, real outline_width,
	real shadow_length, int wireframe) {
    int i, cnt=0, gid;
    SplineChar *sc;
    int layer = fv->active_layer;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( (gid=fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL && fv->selected[i] &&
		sc->layers[layer].splines )
	    ++cnt;
    ff_progress_start_indicator(10,_("Shadowing glyphs"),_("Shadowing glyphs"),0,cnt,1);

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i )
	if ( (gid=fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL && fv->selected[i] &&
		sc->layers[layer].splines && !sc->ticked ) {
	    sc->ticked = true;
	    SCPreserveLayer(sc,layer,false);
	    sc->layers[layer].splines = SSShadow(sc->layers[layer].splines,angle,outline_width,shadow_length,sc,wireframe);
	    SCCharChangedUpdate(sc,layer);
	    if ( !ff_progress_next())
    break;
	}
    ff_progress_end_indicator();
}
