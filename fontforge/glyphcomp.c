/* -*- coding: utf-8 -*- */
/* Copyright (C) 2006-2012 by George Williams */
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

#include "bvedit.h"
#include "fontforgevw.h"
#include "cvundoes.h"
#include "scriptfuncs.h"
#include <math.h>
#include <ustring.h>

/* ************************************************************************** */
/* *********************       Error dispatchers        ********************* */
/* ************************************************************************** */
#if !defined(_NO_PYTHON) || !defined(_NO_FFSCRIPT)
static void GCError(Context *c, const char *msg) {
#ifdef _NO_PYTHON
    ScriptError(c,msg);
#elif defined(_NO_FFSCRIPT)
    PyFF_ErrorString(msg,NULL);
#else
    if ( c==NULL )
	PyFF_ErrorString(msg,NULL);
    else
	ScriptError(c,msg);
#endif
}

static void GCErrorString(Context *c, const char *frmt, const char *str) {
#ifdef _NO_PYTHON
    ScriptErrorString(c,frmt,str);
#elif defined(_NO_FFSCRIPT)
    PyFF_ErrorString(frmt,str);
#else
    if ( c==NULL )
	PyFF_ErrorString(frmt,str);
    else
	ScriptErrorString(c,frmt,str);
#endif
}

static void GCError3(Context *c, const char *frmt, const char *str, int size, int depth) {
#ifdef _NO_PYTHON
    ScriptErrorF(c,frmt,str, size,depth);
#elif defined(_NO_FFSCRIPT)
    PyFF_ErrorF3(frmt,str, size,depth);
#else
    if ( c==NULL )
	PyFF_ErrorF3(frmt,str, size,depth);
    else
	ScriptErrorF(c,frmt,str, size,depth);
#endif
}
#endif
/* ************************************************************************** */
/* ********************* Code to compare outline glyphs ********************* */
/* ************************************************************************** */

static double FindNewT(double pos,const Spline1D *s,double old_t) {
    extended ts[3];
    int i;
    double closest;

    CubicSolve(s, pos, ts);
    closest = -1;
    for ( i=0; i<3 && ts[i]!=-1; ++i ) {
	if ( ts[i]>old_t && ts[i]<=1 ) {
	    if ( closest==-1 || ts[i]<closest )
		closest = ts[i];
	}
    }
return( closest );
}

/* I have arranged so that either both splinesets are clockwise or anti */
/*  Thus when we walk forward on one splineset we will walk forward on the */
/*  other. So no need to back track here */
static int NearSplineSet(BasePoint *here,const SplineSet *ss,
	const Spline **last_found,double *last_t,double err) {
    const Spline *first, *s, *best_s;
    extended ts[3];
    double dx, dy, adx, ady, best, best_t, t;
    BasePoint test;
    int i, j;
    static double offset[] = { 0, .5, -.5 };

    if ( *last_found==NULL ) {
	/* We are looking for the first point. Check our first point */
	/*  Usually that will be right */
	if ( (dx = here->x-ss->first->me.x)<=err && dx>=-err &&
		(dy = here->y-ss->first->me.y)<=err && dy>=-err ) {
	    *last_found = ss->first->next;
	    *last_t = 0;
return( true );
	}
	/* Ok, it wasn't. Check everywhere */
	first = NULL;
	best_s = NULL; best = 1e10; best_t = -1;
	for ( j=0 ; j<3; ++j ) {
	    for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
		if ( first==NULL ) first = s;
		if ( (dx=s->to->me.x-s->from->me.x)<0 ) dx = -dx;
		if ( (dy=s->to->me.y-s->from->me.y)<0 ) dy = -dy;
		if ( dx>dy )
		    CubicSolve(&s->splines[0], here->x+err*offset[j], ts);
		else
		    CubicSolve(&s->splines[1], here->y+err*offset[j], ts);
		for ( i=0; i<3 && ts[i]!=-1 ; ++i ) {
		    test.x = ((s->splines[0].a*ts[i]+s->splines[0].b)*ts[i]+s->splines[0].c)*ts[i]+s->splines[0].d;
		    test.y = ((s->splines[1].a*ts[i]+s->splines[1].b)*ts[i]+s->splines[1].c)*ts[i]+s->splines[1].d;
		    if ( (dx=test.x-here->x)<0 ) dx = -dx;
		    if ( (dy=test.y-here->y)<0 ) dy = -dy;
		    if ( dx<=err && dy<=err ) {
			if ( best_s==NULL || dx+dy<best ) {
			    if ( best==0 ) {
				*last_found = s;
				*last_t = ts[i];
return( true );
			    }
			    best_s = s;
			    best = dx+dy;
			    best_t = ts[i];
			}
		    }
		}
	    }
	}
	if ( best_s==NULL )
return( false );
	*last_found = best_s;
	*last_t = best_t;
return( true );
    } else {
	/* Ok, we've moved 1 em-unit on the other splineset. We can't have */
	/*  moved very far on this splineset (and we can't move backwards) */
	/* Well, we can move backwards if we got rounding errors earlier that */
	/*  pushed us too far forward. But not far backwards */
	s = *last_found;
	t = *last_t;
	if ( (adx = (3*s->splines[0].a* t + 2*s->splines[0].b)* t + s->splines[0].c)<0 ) adx = -adx;
	if ( (ady = (3*s->splines[1].a* t + 2*s->splines[1].b)* t + s->splines[1].c)<0 ) ady = -ady;
	for ( j=0; j<3; ++j ) {
	    while ( s!=NULL ) {
		if ( adx>ady )
		    CubicSolve(&s->splines[0], here->x+err*offset[j], ts);
		else
		    CubicSolve(&s->splines[1], here->y+err*offset[j], ts);
		for ( i=0; i<3 && ts[i]!=-1 ; ++i ) if ( ts[i]>=t ) {
		    test.x = ((s->splines[0].a*ts[i]+s->splines[0].b)*ts[i]+s->splines[0].c)*ts[i]+s->splines[0].d;
		    test.y = ((s->splines[1].a*ts[i]+s->splines[1].b)*ts[i]+s->splines[1].c)*ts[i]+s->splines[1].d;
		    if ( (dx=test.x-here->x)<0 ) dx = -dx;
		    if ( (dy=test.y-here->y)<0 ) dy = -dy;
		    if ( dx<=err && dy<=err ) {
			*last_found = s;
			*last_t = ts[i];
return( true );
		    }
		}
		/* If the end point of the current spline is further from the */
		/*  current point than the step size (either 1 or err) then */
		/*  looking in the following spline won't help. We didn't match */
		/* Let's be a little more generous than the step size. twice the step */
		if ( t>.9 ||
			(((dx=s->to->me.x-here->x)<=3 || dx<=3*err) && (dx>=-3 || dx>=-3*err) &&
			 ((dy=s->to->me.y-here->y)<=3 || dy<=3*err ) && (dy>=-3 || dy>=-3*err)) ) {
		    s = s->to->next;
		    t = 0;
		    if ( (adx = (3*s->splines[0].a* t + 2*s->splines[0].b)* t + s->splines[0].c)<0 ) adx = -adx;
		    if ( (ady = (3*s->splines[1].a* t + 2*s->splines[1].b)* t + s->splines[1].c)<0 ) ady = -ady;
		} else
	    break;
	    }
	    s = *last_found;
	    t = *last_t;
	}
return( false );
    }
}

/* Walk along this contour, moving by 1 em-unit (approximately) each step */
/*  and checking that the point obtained is close to one on the other contour */
static int ContourMatch(const SplineSet *ss1, const SplineSet *ss2, real err) {
    const Spline *s, *first;
    double t, newt;
    const Spline *last_found = NULL;
    double last_t;
    BasePoint here;
    double dx, dy, adx, ady, step;

    step = err>=1 ? err*1.001 : 1;
    first = NULL;
    for ( s = ss1->first->next; s!=NULL && s!=first; s=s->to->next ) {
	t = 0;
	if ( first==NULL ) first = s;
	if ( !NearSplineSet(&s->from->me,ss2,&last_found,&last_t,err))
return( false );
	here = s->from->me;
	for (;;) {
	    adx = dx = (3*s->splines[0].a*t + 2*s->splines[0].b)*t + s->splines[0].c;
	    ady = dy = (3*s->splines[1].a*t + 2*s->splines[1].b)*t + s->splines[1].c;
	    if ( adx<0 ) adx = -adx;
	    if ( ady<0 ) ady = -ady;
	    if ( ady>adx ) {
		if ( dy<0 )
		    here.y = here.y-step;
		else
		    here.y = here.y+step;
		newt = FindNewT(here.y,&s->splines[1],t);
		/* Now it is possible that it will turn an abrupt corner soon */
		/* and the instantanious slope here may not be useful */
		if ( newt==-1 ) {
		    double t_xp, t_xm;
		    t_xp = FindNewT(here.x+step,&s->splines[0],t);
		    t_xm = FindNewT(here.x-step,&s->splines[0],t);
		    if ( t_xp!=-1 && t_xm!=-1 )
			newt = t_xp<t_xm ? t_xp : t_xm;
		    else if ( t_xp!=-1 )
			newt = t_xp;
		    else
			newt = t_xm;
		}
	    } else {
		if ( dx<0 )
		    here.x = here.x-step;
		else
		    here.x = here.x+step;
		newt = FindNewT(here.x,&s->splines[0],t);
		if ( newt==-1 ) {
		    double t_yp, t_ym;
		    t_yp = FindNewT(here.y+step,&s->splines[1],t);
		    t_ym = FindNewT(here.y-step,&s->splines[1],t);
		    if ( t_yp!=-1 && t_ym!=-1 )
			newt = t_yp<t_ym ? t_yp : t_ym;
		    else if ( t_yp!=-1 )
			newt = t_yp;
		    else
			newt = t_ym;
		}
	    }
	    t = newt;
	    if ( t<0 || t>=1 )
	break;
	    here.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	    here.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
	    if ( !NearSplineSet(&here,ss2,&last_found,&last_t,err))
return( false );
	}
    }
return( true );
}

static int AllPointsMatch(const SplinePoint *start1, const SplinePoint *start2,
	real err, SplinePoint **_hmfail) {
    double dx, dy;
    const SplinePoint *sp1=start1, *sp2=start2;
    SplinePoint *hmfail=NULL;

    for (;;) {
	if ( (dx = sp1->me.x-sp2->me.x)<=err && dx>=-err &&
		(dy = sp1->me.y-sp2->me.y)<=err && dy>=-err &&
		(dx = sp1->nextcp.x-sp2->nextcp.x)<=err && dx>=-err &&
		(dy = sp1->nextcp.y-sp2->nextcp.y)<=err && dy>=-err &&
		(dx = sp1->prevcp.x-sp2->prevcp.x)<=err && dx>=-err &&
		(dy = sp1->prevcp.y-sp2->prevcp.y)<=err && dy>=-err )
	    /* Good */;
	else
return( false );

	if ( sp1->hintmask!=NULL && sp2->hintmask!=NULL &&
		memcmp(sp1->hintmask,sp2->hintmask,sizeof(HintMask))==0 )
	    /* hm continues to match */;
	else if ( sp1->hintmask!=NULL || sp2->hintmask!=NULL ) {
	    hmfail=(SplinePoint *) sp1;
	}

	if ( sp2->next==NULL && sp1->next==NULL ) {
	    if ( hmfail!=NULL ) *_hmfail = hmfail;
return( true );
	}
	if ( sp2->next==NULL || sp1->next==NULL )
return( false );
	sp1 = sp1->next->to;
	sp2 = sp2->next->to;
	if ( sp1 == start1 && sp2 == start2 ) {
	    if ( hmfail!=NULL ) *_hmfail = hmfail;
return( true );
	}
	if ( sp1 == start1 || sp2 == start2 )
return( false );
    }
}

static int ContourPointsMatch(const SplineSet *ss1, const SplineSet *ss2,
	real err, SplinePoint **_hmfail) {
    const SplinePoint *sp2;

    /* Does ANY point on the second contour match the start point of the first? */
    for ( sp2 = ss2->first; ; ) {
	if ( AllPointsMatch(ss1->first,sp2,err,_hmfail) )
return( sp2==ss2->first?1:2 );
	if ( sp2->next==NULL )
return( false );
	sp2 = sp2->next->to;
	if ( sp2 == ss2->first )
return( false );
    }
}

enum Compare_Ret SSsCompare(const SplineSet *ss1, const SplineSet *ss2,
	real pt_err, real spline_err, SplinePoint **_hmfail) {
    int cnt1, cnt2, bestcnt;
    const SplineSet *ss, *s2s, *bestss;
    enum Compare_Ret info = 0;
    int allmatch;
    DBounds *b1, *b2;
    const SplineSet **match;
    double diff, delta, bestdiff;
    double dx, dy;

    *_hmfail = NULL;

    for ( ss=ss1, cnt1=0; ss!=NULL; ss=ss->next, ++cnt1 );
    for ( ss=ss2, cnt2=0; ss!=NULL; ss=ss->next, ++cnt2 );
    if ( cnt1!=cnt2 )
return( SS_DiffContourCount|SS_NoMatch );

    b1 = malloc(cnt1*sizeof(DBounds));
    b2 = malloc(cnt1*sizeof(DBounds));
    match = malloc(cnt1*sizeof(SplineSet *));
    for ( ss=ss1, cnt1=0; ss!=NULL; ss=ss->next, ++cnt1 ) {
	SplineSet *next = ss->next; ((SplineSet *) ss)->next = NULL;
	SplineSetFindBounds(ss,&b1[cnt1]);
	((SplineSet *) ss)->next = next;
    }
    for ( ss=ss2, cnt1=0; ss!=NULL; ss=ss->next, ++cnt1 ) {
	SplineSet *next = ss->next; ((SplineSet *) ss)->next = NULL;
	SplineSetFindBounds(ss,&b2[cnt1]);
	((SplineSet *) ss)->next = next;
    }
    for ( ss=ss1, cnt1=0; ss!=NULL; ss=ss->next, ++cnt1 ) {
	bestdiff = -1;
	for ( s2s=ss2, cnt2=0; s2s!=NULL; s2s=s2s->next, ++cnt2 ) if ( b2[cnt2].minx<=b2[cnt2].maxx ) {
	    if ( (diff = b1[cnt1].minx - b2[cnt2].minx)<0 ) diff = -diff;
	    if ( (delta = b1[cnt1].maxx - b2[cnt2].maxx)<0 ) delta = -delta;
	    diff += delta;
	    if ( (delta = b1[cnt1].miny - b2[cnt2].miny)<0 ) delta = -delta;
	    diff += delta;
	    if ( (delta = b1[cnt1].maxy - b2[cnt2].maxy)<0 ) delta = -delta;
	    diff += delta;
	    if ( (diff<bestdiff || bestdiff == -1 ) &&
		    /* Only match closed contours with closed, open with open */
		    (ss1->first->prev==NULL) == (ss2->first->prev==NULL)) {
		bestdiff = diff;
		bestss = s2s;
		bestcnt = cnt2;
		if ( diff==0 )
	break;
	    }
	}
	if ( bestdiff==-1 ) {
	    free(b1); free(b2); free(match);
return( SS_MismatchOpenClosed|SS_NoMatch );
	}
	match[cnt1] = bestss;
	b2[bestcnt].maxx = b2[bestcnt].minx-1;	/* Mark as used */
	if ( bestcnt!=cnt1 )
	    info = SS_DisorderedContours;
    }
    free(b2);
    free(b1);

    if ( pt_err>=0 ) {
	allmatch = true;
	for ( ss=ss1, cnt1=0; ss!=NULL; ss=ss->next, ++cnt1 ) {
	    int ret = ContourPointsMatch(ss,match[cnt1],pt_err,_hmfail);
	    if ( !ret ) {
		allmatch = false;
	break;
	    } else if ( ret==2 )
		allmatch = 2;
	}
	if ( allmatch ) {
	    if ( allmatch==2 ) info |= SS_DisorderedStart;
	    info |= SS_PointsMatch;
	}
    } else
	allmatch = false;

    if ( !allmatch && spline_err>=0 ) {
	allmatch = true;
	for ( ss=ss1, cnt1=0; ss!=NULL; ss=ss->next, ++cnt1 ) {
	    int dir_mismatch = SplinePointListIsClockwise(ss)!=
				SplinePointListIsClockwise(match[cnt1]);
	    int good;
	    if ( dir_mismatch )
		SplineSetReverse((SplineSet *) match[cnt1]);
	    good = ContourMatch(ss,match[cnt1],spline_err) &&
		    ContourMatch(match[cnt1],ss,spline_err);
	    if ( dir_mismatch )
		SplineSetReverse((SplineSet *) match[cnt1]);
	    if ( !good ) {
		allmatch = false;
	break;
	    }
	    if ( dir_mismatch )
		info |= SS_DisorderedDirection;
	    if ( (dx = ss->first->me.x-match[cnt1]->first->me.x)>spline_err ||
		    dx < -spline_err ||
		    (dy = ss->first->me.y-match[cnt1]->first->me.y)>spline_err ||
		    dy < -spline_err )
		info |= SS_DisorderedStart;
	}
	if ( allmatch ) {
	    info |= SS_ContourMatch;
	    *_hmfail = NULL;
	}
    }

    free(match);
    if ( !allmatch )
return( SS_NoMatch|SS_ContourMismatch );

return( info );
}

static int SSRefCompare(const SplineSet *ss1,const SplineSet *ss2,
	const RefChar *refs1, const RefChar *refs2,
	real pt_err, real spline_err) {
    /* Convert all references to contours */
    /* Note: Hintmasks are trashed */
    SplineSet *head1;
    SplineSet *head2;
    SplineSet *temp, *tail;
    const RefChar *r;
    int ret, layer;
    SplinePoint *junk;

    head1 = SplinePointListCopy(ss1);
    if ( head1==NULL ) tail = NULL;
    else for ( tail=head1 ; tail->next!=NULL; tail = tail->next );
    for ( r=refs1; r!=NULL; r=r->next ) {
	for ( layer=0; layer<r->layer_cnt; ++layer ) {
	    temp = SplinePointListCopy(r->layers[layer].splines);
	    if ( head1==NULL )
		head1=tail = temp;
	    else
		tail->next = temp;
	    if ( tail!=NULL )
		for ( ; tail->next!=NULL; tail = tail->next );
	}
    }

    head2 = SplinePointListCopy(ss2);
    if ( head2==NULL ) tail = NULL;
    else for ( tail=head2 ; tail->next!=NULL; tail = tail->next );
    for ( r=refs2; r!=NULL; r=r->next ) {
	for ( layer=0; layer<r->layer_cnt; ++layer ) {
	    temp = SplinePointListCopy(r->layers[layer].splines);
	    if ( head2==NULL )
		head2 = tail = temp;
	    else
		tail->next = temp;
	    if ( tail!=NULL )
		for ( ; tail->next!=NULL; tail = tail->next );
	}
    }

    ret = SSsCompare(head1,head2,pt_err,spline_err,&junk);
    if ( !(ret&SS_NoMatch) )
	ret |= SS_UnlinkRefMatch;

    SplinePointListsFree(head1);
    SplinePointListsFree(head2);
return( ret );
}

/* ************************************************************************** */
/* ********************* Code to compare bitmap glyphs ********************** */
/* ************************************************************************** */

enum Compare_Ret BitmapCompare(BDFChar *bc1, BDFChar *bc2, int err, int bb_err) {
    uint8 *pt1, *pt2;
    int i,j, d, xlen;
    int mask;
    int xmin, xmax, ymin, ymax, c1, c2;
    int failed = 0;

    if ( bc1->byte_data!=bc2->byte_data )
return( BC_DepthMismatch|BC_NoMatch );

    if ( bc1->width!=bc2->width )
	failed = SS_WidthMismatch|BC_NoMatch;

    if ( bc1->vwidth!=bc2->vwidth )
	failed |= SS_VWidthMismatch|BC_NoMatch;

    BCFlattenFloat(bc1);
    BCCompressBitmap(bc1);

    if ( bc1->byte_data ) {
	if ( (d=bc1->xmin-bc2->xmin)>bb_err || d<-bb_err ||
		(d=bc1->ymin-bc2->ymin)>bb_err || d<-bb_err ||
		(d=bc1->xmax-bc2->xmax)>bb_err || d<-bb_err ||
		(d=bc1->ymax-bc2->ymax)>bb_err || d<-bb_err )
return( BC_BoundingBoxMismatch|BC_NoMatch|failed );
		
	xmin = bc1->xmin>bc2->xmin ? bc2->xmin : bc1->xmin;
	ymin = bc1->ymin>bc2->ymin ? bc2->ymin : bc1->ymin;
	xmax = bc1->xmax<bc2->xmax ? bc2->xmax : bc1->xmax;
	ymax = bc1->ymax<bc2->ymax ? bc2->ymax : bc1->ymax;
	for ( j=ymin; j<=ymax; ++j ) {
	    if ( j>=bc1->ymin && j<=bc1->ymax )
		pt1 = bc1->bitmap+(j-bc1->ymin)*bc1->bytes_per_line;
	    else
		pt1 = NULL;
	    if ( j>=bc2->ymin && j<=bc2->ymax )
		pt2 = bc2->bitmap+(j-bc2->ymin)*bc2->bytes_per_line;
	    else
		pt2 = NULL;
	    for ( i=xmin; i<=xmax; ++i ) {
		if ( pt1!=NULL && i>=bc1->xmin && i<=bc1->xmax )
		    c1 = pt1[i-bc1->xmin];
		else
		    c1 = 0;
		if ( pt2!=NULL && i>=bc2->xmin && i<=bc2->xmax )
		    c2 = pt2[i-bc2->xmin];
		else
		    c2 = 0;
		if ( (d = c1-c2)>err || d<-err )
return( BC_NoMatch|BC_BitmapMismatch|failed );
	    }
	}
    } else {
	/* Bitmap */
	if ( bc1->xmin!=bc2->xmin || bc1->xmax!=bc2->xmax ||
		bc1->ymin!=bc2->ymin || bc1->ymax!=bc2->ymax )
return( BC_BoundingBoxMismatch|BC_NoMatch|failed );

	xlen = bc1->xmax-bc1->xmin;
	mask = 0xff00>>((xlen&7)+1);
	xlen>>=3;
	for ( j=0; j<=bc1->ymax-bc1->ymin; ++j ) {
	    pt1 = bc1->bitmap+j*bc1->bytes_per_line;
	    pt2 = bc2->bitmap+j*bc2->bytes_per_line;
	    for ( i=xlen-1; i>=0; --i )
		if ( pt1[i]!=pt2[i] )
return( BC_NoMatch|BC_BitmapMismatch|failed );
	    if ( (pt1[xlen]&mask)!=(pt2[xlen]&mask) )
return( BC_NoMatch|BC_BitmapMismatch|failed );
	}
    }

return( failed == 0 ? BC_Match : failed );
}

/* ************************************************************************** */
/* **************** Code to selected glyphs against clipboard *************** */
/* ************************************************************************** */

#if !defined(_NO_PYTHON) || !defined(_NO_FFSCRIPT)
static int RefCheck(const RefChar *ref1,const RefChar *ref2 ) {
    const RefChar *r1, *r2;
    int i;
    int ptmatchdiff = 0;

    for ( r2 = ref2; r2!=NULL; r2=r2->next )
	((RefChar *) r2)->checked = false;

    for ( r1 = ref1; r1!=NULL; r1=r1->next ) {
	for ( r2 = ref2; r2!=NULL; r2=r2->next ) if ( !r2->checked ) {
	    /* BUG!!!! glyphs with no unicode encoding? */
	    if ( r2->unicode_enc == r1->unicode_enc ) {
		for ( i=0; i<6 &&
		    RealNear(r1->transform[i],r2->transform[i]); ++i );
		if ( i==6 )
	break;
	    }
	}
	if ( r2!=NULL )
	    ((RefChar *) r2)->checked = true;
	else
return( false );
	if ( r1->point_match != r2->point_match ||
		(r1->point_match &&
		    (r1->match_pt_base!=r2->match_pt_base && r1->match_pt_ref!=r2->match_pt_ref)))
	    ptmatchdiff = 1;
    }

    for ( r2 = ref2; r2!=NULL; r2=r2->next ) if ( !r2->checked ) {
return( false );
    }

return( true + ptmatchdiff );
}

int CompareLayer(Context *c, const SplineSet *ss1,const SplineSet *ss2,
	const RefChar *refs1, const RefChar *refs2,
	real pt_err, real spline_err, const char *name, int diffs_are_errors,
	SplinePoint **_hmfail) {
    int val, refc;

    if ( pt_err<0 && spline_err<0 )
return( SS_PointsMatch );
    val = SSsCompare(ss1,ss2, pt_err, spline_err,_hmfail);
    refc = RefCheck( refs1,refs2 );
    if ( !refc ) {
	if ( !(val&SS_NoMatch) )
	    val = SS_NoMatch|SS_RefMismatch;
	else
	    val |= SS_RefMismatch;
    } else if ( refc==2 )
	val |= SS_RefPtMismatch;
    if ( (val&SS_NoMatch) && diffs_are_errors ) {
	if ( val & SS_DiffContourCount )
	    GCErrorString(c,"Spline mismatch (different number of contours) in glyph", name);
	else if ( val & SS_MismatchOpenClosed )
	    GCErrorString(c,"Open/Closed contour mismatch in glyph", name);
	else if ( val & SS_RefMismatch )
	    GCErrorString(c,"Reference mismatch in glyph", name);
	else
	    GCErrorString(c,"Spline mismatch in glyph", name);
return( -1 );
    }
    if ( (val & SS_RefPtMismatch) && diffs_are_errors ) {
	GCErrorString(c,"References have different truetype point matching in glyph", name);
return( -1 );
    }
return( val );
}

static int CompareBitmap(Context *c,SplineChar *sc,const Undoes *cur,
	real pixel_off_frac, int bb_err, int diffs_are_errors ) {
    int ret, err;
    BDFFont *bdf;
    BDFChar bc;

    for ( bdf=c->curfv->sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=cur->u.bmpstate.pixelsize || BDFDepth(bdf)!=cur->u.bmpstate.depth); bdf=bdf->next );
    if ( bdf==NULL || sc->orig_pos>=bdf->glyphcnt ||
	    bdf->glyphs[sc->orig_pos]==NULL ) {
	GCError(c,"Missing bitmap size" );
return( -1 );
    }
    memset(&bc,0,sizeof(bc));
    bc.xmin = cur->u.bmpstate.xmin;
    bc.xmax = cur->u.bmpstate.xmax;
    bc.ymin = cur->u.bmpstate.ymin;
    bc.ymax = cur->u.bmpstate.ymax;
    bc.bytes_per_line = cur->u.bmpstate.bytes_per_line;
    bc.bitmap = (uint8 *) cur->u.bmpstate.bitmap;
    bc.byte_data = cur->u.bmpstate.depth!=1;
    bc.width = cur->u.bmpstate.width;
    err = pixel_off_frac * (1<<BDFDepth(bdf));
    ret = BitmapCompare(bdf->glyphs[sc->orig_pos],&bc,err, bb_err);
    if ( (ret&BC_NoMatch) && diffs_are_errors ) {
	if ( ret&BC_BoundingBoxMismatch )
	    GCError3(c,"Bitmaps bounding boxes do not match in glyph %s pixelsize %d depth %d",
		    sc->name, bdf->pixelsize, BDFDepth(bdf));
	else if ( ret&SS_WidthMismatch )
	    GCError3(c,"Bitmaps advance widths do not match in glyph %s pixelsize %d depth %d",
		    sc->name, bdf->pixelsize, BDFDepth(bdf));
	else if ( ret&SS_VWidthMismatch )
	    GCError3(c,"Bitmaps vertical advance widths do not match in glyph %s pixelsize %d depth %d",
		    sc->name, bdf->pixelsize, BDFDepth(bdf));
	else
	    GCError3(c,"Bitmap mismatch in glyph %s pixelsize %d depth %d",
		    sc->name, bdf->pixelsize, BDFDepth(bdf));
return( -1 );
    }
return( ret );
}

static int CompareHints( SplineChar *sc, const void *_test ) {
    StemInfo *h = sc->hstem, *v = sc->vstem;
    const StemInfo *test = _test;

    if ( test!=NULL && test->hinttype == ht_h ) {
	while ( test!=NULL && (test->hinttype==ht_unspecified || test->hinttype==ht_h)) {
	    if ( h==NULL )
return( false );
	    if ( rint(h->start) != rint(test->start) || rint(h->width) != rint(test->width) )
return( false );
	    h = h->next;
	    test = test->next;
	}
    }
    if ( test!=NULL && test->hinttype == ht_v ) {
	while ( test!=NULL && (test->hinttype==ht_unspecified || test->hinttype==ht_v)) {
	    if ( v==NULL )
return( false );
	    if ( rint(v->start) != rint(test->start) || rint(v->width) != rint(test->width) )
return( false );
	    v = v->next;
	    test = test->next;
	}
    }

    if ( test!=NULL && test->hinttype==ht_d )
	test = NULL;		/* In case there are some old files (with dhints) */

    if ( h!=NULL || v!=NULL || test!=NULL )
return( false );

return( true );
}

static int CompareSplines(Context *c,SplineChar *sc,const Undoes *cur,
	real pt_err, real spline_err, int comp_hints, int diffs_are_errors ) {
    int ret=0, failed=0, temp, ly;
    const Undoes *layer;
    real err = pt_err>0 ? pt_err : spline_err;
    SplinePoint *hmfail;

    switch ( cur->undotype ) {
      case ut_state: case ut_statehint: case ut_statename:
	if ( err>=0 ) {
	    ret = CompareLayer(c,sc->layers[ly_fore].splines,cur->u.state.splines,
			sc->layers[ly_fore].refs,cur->u.state.refs,
			pt_err, spline_err,sc->name, diffs_are_errors, &hmfail);
	    if ( ret==-1 )
return( -1 );
	    if ( ret&SS_NoMatch )
		failed |= ret;
	    if ( sc->vwidth-cur->u.state.vwidth>err || sc->vwidth-cur->u.state.vwidth<-err )
		failed |= SS_NoMatch|SS_VWidthMismatch;
	    if ( sc->width-cur->u.state.width>err || sc->width-cur->u.state.width<-err )
		failed |= SS_NoMatch|SS_WidthMismatch;
	}
	if ( cur->undotype==ut_statehint && (comp_hints&1) &&
		!CompareHints( sc,cur->u.state.hints ))
	    failed |= SS_NoMatch|SS_HintMismatch;
	if ( cur->undotype==ut_statehint && (comp_hints&2) &&
		(sc->hconflicts || sc->vconflicts || !(comp_hints&4)) &&
		hmfail!=NULL )
	    failed |= SS_NoMatch|SS_HintMaskMismatch;
	if ( failed )
	    ret = failed;
      break;
      case ut_layers:
	if ( err>=0 ) {
	    for ( ly=ly_fore, layer = cur->u.multiple.mult;
		    ly<sc->layer_cnt && layer!=NULL;
		    ++ly, layer = cur->next ) {
		temp = CompareLayer(c,sc->layers[ly].splines,cur->u.state.splines,
			    sc->layers[ly].refs,cur->u.state.refs,
			    pt_err, spline_err,sc->name, diffs_are_errors, &hmfail);
		if ( temp==-1 )
return( -1 );
		if ( temp&SS_NoMatch )
		    failed |= temp;
		else
		    ret |= temp;
		/* No hints in type3 fonts */
	    }
	    if ( ly==ly_fore && (sc->vwidth-cur->u.state.vwidth>err || sc->vwidth-cur->u.state.vwidth<-err) )
		failed |= SS_NoMatch|SS_VWidthMismatch;
	    if ( ly==ly_fore && (sc->width-cur->u.state.width>err || sc->width-cur->u.state.width<-err) )
		failed |= SS_NoMatch|SS_WidthMismatch;
	}
	if ( ly!=sc->layer_cnt || layer!=NULL )
	    failed |= SS_NoMatch|SS_LayerCntMismatch;
	if ( failed )
	    ret = failed;
      break;
      default:
	GCError(c,"Unexpected clipboard contents");
return( -1 );
    }

    if ( (ret&SS_WidthMismatch) && diffs_are_errors ) {
	GCErrorString(c,"Advance width mismatch in glyph", sc->name);
return( -1 );
    }
    if ( (ret&SS_VWidthMismatch) && diffs_are_errors ) {
	GCErrorString(c,"Vertical advance width mismatch in glyph", sc->name);
return( -1 );
    }
    if ( (ret&SS_HintMismatch) && diffs_are_errors ) {
	GCErrorString(c,"Hinting mismatch in glyph", sc->name);
return( -1 );
    }
    if ( (ret&SS_HintMaskMismatch) && diffs_are_errors ) {
	if ( hmfail==NULL || c==NULL )
	    GCErrorString(c,"Hint mask mismatch in glyph", sc->name);
#if !defined(_NO_FFSCRIPT)
	else
	    ScriptErrorF(c,"Hint mask mismatch at (%g,%g) in glyph: %s",
		    hmfail->me.x, hmfail->me.y, sc->name);
#endif
return( -1 );
    }
    if ( (ret&SS_LayerCntMismatch) && diffs_are_errors ) {
	GCErrorString(c,"Layer difference in glyph", sc->name);
return( -1 );
    }
return( ret );
}

int CompareGlyphs(Context *c, real pt_err, real spline_err,
	real pixel_off_frac, int bb_err, int comp_hints, int diffs_are_errors ) {
    FontViewBase *fv = c->curfv;
    SplineFont *sf = fv->sf;
    int i, cnt=0;
    int ret=0;
    const Undoes *cur, *bmp;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] )
	++cnt;
    if ( cnt==0 ) {
	GCError(c,"Nothing selected");
return( -1 );
    }

    cur = CopyBufferGet();
    if ( cur->undotype==ut_noop || cur->undotype==ut_none ) {
	GCError(c,"Nothing in clipboard");
return( -1 );
    }

    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	SplineChar *sc = fv->map->map[i]==-1 ? NULL : sf->glyphs[ fv->map->map[i] ];

	if ( sc==NULL ) {
	    GCError(c,"Missing character");
return( -1 );
	}

	if ( cur==NULL ) {
	    GCError(c,"Too few glyphs in clipboard");
return( -1 );
	}

	switch ( cur->undotype ) {
	  case ut_state: case ut_statehint: case ut_statename:
	  case ut_layers:
	    if ( pt_err>=0 || spline_err>0 || comp_hints ) {
		ret |= CompareSplines(c,sc,cur,pt_err,spline_err,comp_hints,diffs_are_errors);
		if ( ret==-1 )
return( -1 );
	    }
	  break;
	  case ut_bitmapsel: case ut_bitmap:
	    if ( pixel_off_frac>=0 ) {
		ret |= CompareBitmap(c,sc,cur,pixel_off_frac,bb_err,diffs_are_errors);
		if ( ret==-1 )
return( -1 );
	    }
	  break;
	  case ut_composit:
	    if ( cur->u.composit.state!=NULL && ( pt_err>=0 || spline_err>0 || comp_hints ))
		ret |= CompareSplines(c,sc,cur->u.composit.state,pt_err,spline_err,comp_hints,diffs_are_errors);
	    if ( pixel_off_frac>=0 ) {
		for ( bmp=cur->u.composit.bitmaps; bmp!=NULL; bmp = bmp->next ) {
		    ret |= CompareBitmap(c,sc,bmp,pixel_off_frac,bb_err,diffs_are_errors);
		    if ( ret==-1 )
return( -1 );
		}
	    }
	  break;
	  default:
	    GCError(c,"Unexpected clipboard contents");
return( -1 );
	}
	if ( ret&(SS_NoMatch|SS_RefMismatch|SS_WidthMismatch|BC_NoMatch) ) {
	    ret &= ~(BC_Match|SS_PointsMatch|SS_ContourMatch);
return( ret );
	}
	cur = cur->next;
    }

    if ( cur!=NULL ) {
	GCError(c,"Too many glyphs in clipboard");
return( -1 );
    }
return( ret );
}
#endif /* !defined(_NO_PYTHON) || !defined(_NO_FFSCRIPT) */

/* ************************************************************************** */
/* *********************** Code to compare two fonts ************************ */
/* ************************************************************************** */

struct font_diff {
    SplineFont *sf1, *sf2;
    SplineFont *sf1_mst, *sf2_mst;
    EncMap *map1;
    int sf1_glyphcnt;		/* It may change if addmissing, but our arrays (like matches) won't */
    FILE *diffs;
    int flags;
    char *name1, *name2;
    int top_diff, middle_diff, local_diff, diff;
    SplineChar **matches;
    SplineChar *last_sc;
    char held[600];
    int lcnt1, lcnt2;
    OTLookup **l2match1, **l1match2;
    int scnt1, scnt2;
    struct lookup_subtable **s2match1, **s1match2;
    int is_gpos;
    struct lookup_subtable *cur_sub1, *cur_sub2;
};

static void GlyphDiffSCError(struct font_diff *fd, SplineChar *sc, char *format, ... ) {
    va_list ap;

    if ( !fd->top_diff ) {
	fprintf( fd->diffs, "%s", _("Outline Glyphs\n") );
	fd->top_diff = fd->diff = true;
    }
    if ( !fd->local_diff ) {
	putc(' ',fd->diffs);
	fprintf( fd->diffs, "%s", _("Glyph Differences\n") );
	fd->local_diff = fd->diff = true;
    }
    va_start(ap,format);
    if ( fd->last_sc==sc ) {
	if ( fd->held[0] ) {
	    fputs("  ",fd->diffs);
/* GT: FontForge needs to recognize the quotes used here(“”). If you change them */
/* GT: (in the translated strings) let me know. It currently also recognizes */
/* GT: guillemets and a couple of other quotes as well. */
/* GT:   pfaedit@users.sourceforge.net */
	    fprintf( fd->diffs, U_("Glyph “%s” differs\n"), sc->name );
	    fprintf( fd->diffs, "   %s", fd->held );
	    fd->held[0] = '\0';
	}
	fputs("   ",fd->diffs);
	vfprintf(fd->diffs,format,ap);
    } else {
	vsnprintf(fd->held,sizeof(fd->held),format,ap);
	fd->last_sc = sc;
    }
    va_end(ap);
}

static void GlyphDiffSCFinish(struct font_diff *fd) {
    if ( fd->held[0] ) {
	fputs("  ",fd->diffs);
	fprintf( fd->diffs, "%s", fd->held );
	fd->held[0] = '\0';
    }
    fd->last_sc = NULL;
}

static int SCCompareHints( SplineChar *sc1, SplineChar *sc2 ) {
    StemInfo *h1, *h2;

    for ( h1=sc1->hstem, h2=sc2->hstem; h1!=NULL && h2!=NULL; h1=h1->next, h2=h2->next )
	if ( h1->width!=h2->width || h1->start!=h2->start )
return( false );
    if ( h1!=NULL || h2!=NULL )
return( false );

    for ( h1=sc1->vstem, h2=sc2->vstem; h1!=NULL && h2!=NULL; h1=h1->next, h2=h2->next )
	if ( h1->width!=h2->width || h1->start!=h2->start )
return( false );
    if ( h1!=NULL || h2!=NULL )
return( false );

return( true );
}

static int fdRefCheck(struct font_diff *fd, SplineChar *sc1,
	RefChar *ref1, RefChar *ref2, int complain ) {
    RefChar *r1, *r2;
    int i;
    int ret = true;

    for ( r2 = ref2; r2!=NULL; r2=r2->next )
	r2->checked = false;

    for ( r1 = ref1; r1!=NULL; r1=r1->next ) {
	for ( r2 = ref2; r2!=NULL; r2=r2->next ) if ( !r2->checked ) {
	    if ( r1->sc->unicodeenc==r2->sc->unicodeenc &&
		    (r1->sc->unicodeenc!=-1 || strcmp(r1->sc->name,r2->sc->name)==0)) {
		for ( i=0; i<6 &&
		    RealNear(r1->transform[i],r2->transform[i]); ++i );
		if ( i==6 )
	break;
	    }
	}
	if ( r2!=NULL ) {
	    r2->checked = true;
	    if ( r1->point_match != r2->point_match ||
		    (r1->point_match &&
			(r1->match_pt_base!=r2->match_pt_base && r1->match_pt_ref!=r2->match_pt_ref))) {
		if ( complain )
		    GlyphDiffSCError(fd,sc1,U_("Glyph “%s” refers to %s with a different truetype point matching scheme\n"),
			    sc1->name, r1->sc->name );
		ret = 2;
	    }
	} else {
	    for ( r2 = ref2; r2!=NULL; r2=r2->next ) if ( !r2->checked ) {
		if ( r1->sc->unicodeenc==r2->sc->unicodeenc &&
			(r1->sc->unicodeenc!=-1 || strcmp(r1->sc->name,r2->sc->name)==0))
	    break;
	    }
	    if ( r2==NULL ) {
		if ( complain )
		    GlyphDiffSCError(fd,sc1,U_("Glyph “%s” contains a reference to %s in %s\n"),
			    sc1->name, r1->sc->name, fd->name1 );
		ret = false;
	    } else {
		if ( complain )
		    GlyphDiffSCError(fd,sc1,U_("Glyph “%s” refers to %s with a different transformation matrix\n"),
			    sc1->name, r1->sc->name, fd->name1 );
		ret = false;
		r2->checked = true;
	    }
	}
    }

    for ( r2 = ref2; r2!=NULL; r2=r2->next ) if ( !r2->checked ) {
	if ( complain )
	    GlyphDiffSCError(fd,sc1,U_("Glyph “%s” contains a reference to %s in %s\n"),
		    sc1->name, r2->sc->name, fd->name2 );
	ret = false;
    }
return( ret );
}

static void SCAddBackgrounds(SplineChar *sc1,SplineChar *sc2) {
    SplineSet *last;
    RefChar *ref;

    SCOutOfDateBackground(sc1);
    SplinePointListsFree(sc1->layers[ly_back].splines);
    sc1->layers[ly_back].splines = SplinePointListCopy(sc2->layers[ly_fore].splines);
    if ( sc1->layers[ly_back].splines!=NULL )
	for ( last = sc1->layers[ly_back].splines; last->next!=NULL; last=last->next );
    else
	last = NULL;
    for ( ref = sc2->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( last!=NULL ) {
	    last->next = SplinePointListCopy(ref->layers[0].splines);
	    while ( last->next!=NULL ) last=last->next;
	} else {
	    sc1->layers[ly_back].splines = SplinePointListCopy(ref->layers[0].splines);
	    if ( sc1->layers[ly_back].splines!=NULL )
		for ( last = sc1->layers[ly_back].splines; last->next!=NULL; last=last->next );
	}
    }
    if ( sc1->layers[ly_back].order2!=sc2->layers[ly_fore].order2 )
	sc1->layers[ly_back].splines =
		SplineSetsConvertOrder(sc1->layers[ly_back].splines,
			sc1->layers[ly_back].order2);
    SCCharChangedUpdate(sc1,ly_back);
}

static void SCCompare(SplineChar *sc1,SplineChar *sc2,struct font_diff *fd) {
    int layer, last;
    int val;
    SplinePoint *hmfail;

    if ( sc1->parent->multilayer && sc1->layer_cnt!=sc2->layer_cnt )
	GlyphDiffSCError(fd,sc1,U_("Glyph “%s” has a different number of layers\n"),
		sc1->name );
    else {
	last = ly_fore;
	if ( sc1->parent->multilayer )
	    last = sc1->layer_cnt-1;
	for ( layer=ly_fore; layer<=last; ++layer ) {
	    if ( sc1->layers[layer].dofill != sc2->layers[layer].dofill )
		GlyphDiffSCError(fd,sc1,U_("Glyph “%s” has a different fill in layer %d\n"),
			sc1->name, layer );
	    if ( sc1->layers[layer].dostroke != sc2->layers[layer].dostroke )
		GlyphDiffSCError(fd,sc1,U_("Glyph “%s” has a different stroke in layer %d\n"),
			sc1->name, layer );
	    if ( !(fd->flags&fcf_exact) ) {
		int tdiff, rd;
		val = SS_NoMatch;
		rd = fdRefCheck(fd, sc1, sc1->layers[layer].refs, sc2->layers[layer].refs, false );
		if ( !rd ) {
		    val = SSRefCompare(sc1->layers[layer].splines, sc2->layers[layer].splines,
			    sc1->layers[layer].refs, sc2->layers[layer].refs,
			    0,1.5 );
		} 
		if ( val&SS_NoMatch ) {
		    fdRefCheck(fd, sc1, sc1->layers[layer].refs, sc2->layers[layer].refs, true );
		    val = SSsCompare(sc1->layers[layer].splines, sc2->layers[layer].splines,
			    0,1.5, &hmfail );
		}
		tdiff = fd->diff;
		if ( rd==2 )
		    GlyphDiffSCError(fd,sc1,U_("Glyph “%s” contains a reference which has different truetype point match specifications\n"),
			    sc1->name );
		if ( (val&SS_ContourMatch) && (fd->flags&fcf_warn_not_exact) )
		    GlyphDiffSCError(fd,sc1,U_("Glyph “%s” does not have splines which match exactly, but they are close\n"),
			    sc1->name );
		if ( (val&SS_UnlinkRefMatch) && (fd->flags&fcf_warn_not_ref_exact) )
		    GlyphDiffSCError(fd,sc1,U_("A match was found after unlinking references in glyph “%s”\n"),
			    sc1->name );
		fd->diff = tdiff;	/* those are warnings, not errors */
	    } else {
		fdRefCheck(fd, sc1, sc1->layers[layer].refs, sc2->layers[layer].refs, true );
		val = SSsCompare(sc1->layers[layer].splines, sc2->layers[layer].splines,
			0,-1, &hmfail );
	    }
	    if ( val&SS_NoMatch ) {
		if ( val & SS_DiffContourCount )
		    GlyphDiffSCError(fd,sc1,U_("Different number of contours in glyph “%s”\n"), sc1->name);
		else if ( val & SS_MismatchOpenClosed )
		    GlyphDiffSCError(fd,sc1,U_("Open/Closed contour mismatch in glyph “%s”\n"), sc1->name);
		else
		    GlyphDiffSCError(fd,sc1,U_("Spline mismatch in glyph “%s”\n"), sc1->name);
	    }
	}
    }
    if ( fd->last_sc==sc1 && (fd->flags&fcf_adddiff2sf1))
	SCAddBackgrounds(sc1,sc2);

    if ( sc1->width!=sc2->width )
	GlyphDiffSCError(fd,sc1,U_("Glyph “%s” has advance width %d in %s but %d in %s\n"),
		sc1->name, sc1->width, fd->name1, sc2->width, fd->name2 );
    if ( sc1->vwidth!=sc2->vwidth )
	GlyphDiffSCError(fd,sc1,U_("Glyph “%s” has vertical advance width %d in %s but %d in %s\n"),
		sc1->name, sc1->vwidth, fd->name1, sc2->vwidth, fd->name2 );

    if ( ( fd->flags&fcf_hintmasks ) && !(val&SS_NoMatch) &&
	    (sc1->hconflicts || sc1->vconflicts || !(fd->flags&fcf_hmonlywithconflicts)) &&
	    hmfail!=NULL )
	GlyphDiffSCError(fd,sc1,U_("Hint masks differ in glyph “%s” at (%g,%g)\n"),
		sc1->name, hmfail->me.x, hmfail->me.y );
    if ( ( fd->flags&fcf_hinting ) && !SCCompareHints( sc1,sc2 ))
	GlyphDiffSCError(fd,sc1,U_("Hints differ in glyph “%s”\n"), sc1->name);
    if (( fd->flags&fcf_hinting ) && (sc1->ttf_instrs_len!=0 || sc2->ttf_instrs_len!=0)) {
	if ( sc1->ttf_instrs_len==0 )
	    GlyphDiffSCError(fd,sc1,U_("Glyph “%s” in %s has no truetype instructions\n"),
		    sc1->name, fd->name1 );
	else if ( sc2->ttf_instrs_len==0 )
	    GlyphDiffSCError(fd,sc1,U_("Glyph “%s” in %s has no truetype instructions\n"),
		    sc1->name, fd->name2 );
	else if ( sc1->ttf_instrs_len!=sc2->ttf_instrs_len ||
		memcmp(sc1->ttf_instrs,sc2->ttf_instrs,sc1->ttf_instrs_len)!=0 )
	    GlyphDiffSCError(fd,sc1,U_("Glyph “%s” has different truetype instructions\n"),
		    sc1->name );
    }
    GlyphDiffSCFinish(fd);
}

static void FDAddMissingGlyph(struct font_diff *fd,SplineChar *sc2) {
    SplineChar *sc;
    int enc;

    enc = SFFindSlot(fd->sf1,fd->map1,sc2->unicodeenc,sc2->name);
    if ( enc==-1 )
	enc = fd->map1->enccount;
    sc = SFMakeChar(fd->sf1,fd->map1,enc);
    sc->width = sc2->width;
    sc->vwidth = sc2->vwidth;
    sc->widthset = sc2->widthset;
    free(sc->name);
    sc->name = copy(sc2->name);
    sc->unicodeenc = sc2->unicodeenc;
    SCAddBackgrounds(sc,sc2);
}

static void comparefontglyphs(struct font_diff *fd) {
    int gid1, gid2;
    SplineChar *sc, *sc2;
    SplineFont *sf1 = fd->sf1, *sf2=fd->sf2;

    fd->top_diff = fd->local_diff = false;
    for ( gid1=0; gid1<fd->sf1_glyphcnt; ++gid1 ) {
	if ( (sc=sf1->glyphs[gid1])!=NULL && !sc->ticked ) {
	    if ( !fd->top_diff )
		fprintf( fd->diffs, "%s", _("Outline Glyphs\n") );
	    if ( !fd->local_diff ) {
		putc(' ',fd->diffs);
		fprintf( fd->diffs, _("Glyphs in %s but not in %s\n"), fd->name1, fd->name2 );
	    }
	    fd->local_diff = fd->top_diff = fd->diff = true;
	    fputs("  ",fd->diffs);
	    fprintf( fd->diffs, U_("Glyph “%s” missing from %s\n"), sc->name, fd->name2 );
	}
    }

    fd->local_diff = false;
    for ( gid2=0; gid2<sf2->glyphcnt; ++gid2 )
	if ( (sc=sf2->glyphs[gid2])!=NULL && !sc->ticked ) {
	    if ( !fd->top_diff )
		fprintf( fd->diffs, "%s", _("Outline Glyphs\n") );
	    if ( !fd->local_diff ) {
		putc(' ',fd->diffs);
		fprintf( fd->diffs, _("Glyphs in %s but not in %s\n"), fd->name2, fd->name1 );
	    }
	    fd->local_diff = fd->top_diff = fd->diff = true;
	    fputs("  ",fd->diffs);
	    fprintf( fd->diffs, U_("Glyph “%s” missing from %s\n"), sc->name, fd->name1 );
	    if ( fd->flags&fcf_addmissing )
		FDAddMissingGlyph(fd,sc);
	}

    if ( sf1->ascent+sf1->descent != sf2->ascent+sf2->descent ) {
	if ( !fd->top_diff )
	    fprintf( fd->diffs, "%s", _("Outline Glyphs\n") );
	putc(' ',fd->diffs);
	fprintf( fd->diffs, "%s", _("Glyph Differences\n") );
	fputs("  ",fd->diffs);
	fprintf( fd->diffs, "%s", _("ppem is different in the two fonts, cowardly refusing to compare glyphs\n") );
	fd->diff = true;
return;
    }

    fd->local_diff = false;
    for ( gid1=0; gid1<fd->sf1_glyphcnt; ++gid1 ) {
	if ( (sc=sf1->glyphs[gid1])!=NULL && (sc2=fd->matches[gid1])!=NULL )
	    SCCompare(sc,sc2,fd);
    }
}

static void comparebitmapglyphs(struct font_diff *fd, BDFFont *bdf1, BDFFont *bdf2) {
    BDFChar *bdfc, *bdfc2;
    int gid1, gid2;

    for ( gid1=0; gid1<bdf1->glyphcnt; ++gid1 ) if ( (bdfc=bdf1->glyphs[gid1])!=NULL )
	bdfc->ticked = false;
    for ( gid2=0; gid2<bdf2->glyphcnt; ++gid2 ) if ( (bdfc=bdf2->glyphs[gid2])!=NULL )
	bdfc->ticked = false;

    fd->middle_diff = fd->local_diff = false;
    for ( gid1=0; gid1<bdf1->glyphcnt; ++gid1 ) {
	if ( (bdfc=bdf1->glyphs[gid1])!=NULL ) {
	    bdfc2 = NULL;
	    if ( fd->matches[gid1]!=NULL ) {
		gid2 = fd->matches[gid1]->orig_pos;
		if ( gid2<bdf2->glyphcnt ) {
		    bdfc2 = bdf2->glyphs[gid2];
		    bdfc2->ticked = true;
		    bdfc->ticked = true;
		}
	    }
	    if ( bdfc2==NULL ) {
		if ( !fd->top_diff )
		    fprintf( fd->diffs, "%s", _("Bitmap Strikes\n") );
		if ( !fd->middle_diff ) {
		    putc(' ',fd->diffs);
		    fprintf( fd->diffs, _("Strike %d@%d\n"),
			    bdf1->pixelsize, BDFDepth(bdf1) );
		}
		if ( !fd->local_diff ) {
		    fputs("  ",fd->diffs);
		    fprintf( fd->diffs, _("Glyphs in %s but not in %s at %d@%d\n"),
			    fd->name1, fd->name2, bdf1->pixelsize, BDFDepth(bdf1) );
		}
		fd->local_diff = fd->middle_diff = fd->top_diff = fd->diff = true;
		fputs("   ",fd->diffs);
		fprintf( fd->diffs, U_("Glyph “%s” missing from %s at %d@%d\n"),
			bdfc->sc->name, fd->name2, bdf1->pixelsize, BDFDepth(bdf1) );
	    }
	}
    }

    fd->local_diff = false;
    for ( gid2=0; gid2<bdf2->glyphcnt; ++gid2 )
	if ( (bdfc=bdf2->glyphs[gid2])!=NULL && !bdfc->ticked ) {
	    if ( !fd->top_diff )
		fprintf( fd->diffs, "%s", _("Bitmap Strikes\n") );
	    if ( !fd->middle_diff ) {
		putc(' ',fd->diffs);
		fprintf( fd->diffs, _("Strike %d@%d\n"),
			bdf1->pixelsize, BDFDepth(bdf1) );
	    }
	    if ( !fd->local_diff ) {
		fputs("  ",fd->diffs);
		fprintf( fd->diffs, _("Glyphs in %s but not in %s at %d@%d\n"),
			fd->name2, fd->name2, bdf1->pixelsize, BDFDepth(bdf1) );
	    }
	    fd->local_diff = fd->middle_diff = fd->top_diff = fd->diff = true;
	    fputs("   ",fd->diffs);
	    fprintf( fd->diffs, U_("Glyph “%s” missing from %s at %d@%d\n"),
		    bdfc->sc->name, fd->name1, bdf1->pixelsize, BDFDepth(bdf1) );
	}

    fd->local_diff = false;
    for ( gid1=0; gid1<bdf1->glyphcnt; ++gid1 ) {
	if ( (bdfc=bdf1->glyphs[gid1])!=NULL ) {
	    bdfc2 = NULL;
	    if ( fd->matches[gid1]!=NULL ) {
		gid2 = fd->matches[gid1]->orig_pos;
		if ( gid2<bdf2->glyphcnt )
		    bdfc2 = bdf2->glyphs[gid2];
	    }
	    if ( bdfc2!=NULL ) {
		int val = BitmapCompare(bdfc,bdfc2,0,0);
		const char *leader = "   ";
		if ( !fd->top_diff )
		    fprintf( fd->diffs, "%s", _("Bitmap Strikes\n") );
		if ( !fd->middle_diff ) {
		    putc(' ',fd->diffs);
		    fprintf( fd->diffs, _("Strike %d@%d\n"),
			    bdf1->pixelsize, BDFDepth(bdf1) );
		}
		if ( !fd->local_diff ) {
		    fputs("  ",fd->diffs);
		    fprintf( fd->diffs, _("Glyphs Differences at %d@%d\n"),
			    bdf1->pixelsize, BDFDepth(bdf1) );
		}
		if ( ((val&SS_WidthMismatch)!=0) + ((val&SS_VWidthMismatch)!=0) +
			((val&(BC_BoundingBoxMismatch|BC_BitmapMismatch))!=0)>1 ) {
		    fputs(leader,fd->diffs);
		    fprintf( fd->diffs, U_("Glyph “%s” differs at %d@%d\n"),
			    bdfc->sc->name,bdf1->pixelsize, BDFDepth(bdf1) );
		    leader = "    ";
		}
		if ( val&SS_WidthMismatch ) {
		    fputs(leader,fd->diffs);
		    fprintf(fd->diffs,U_("Glyph “%s” has advance width %d in %s but %d in %s at %d@%d\n"),
			    bdfc->sc->name, bdfc->width, fd->name1, bdfc2->width, fd->name2,
			    bdf1->pixelsize, BDFDepth(bdf1));
		}
		if ( val&SS_VWidthMismatch ) {
		    fputs(leader,fd->diffs);
		    fprintf(fd->diffs,U_("Glyph “%s” has vertical advance width %d in %s but %d in %s at %d@%d\n"),
			    bdfc->sc->name, bdfc->vwidth, fd->name1, bdfc2->vwidth, fd->name2,
			    bdf1->pixelsize, BDFDepth(bdf1));
		}
		if ( val&(BC_BoundingBoxMismatch|BC_BitmapMismatch) ) {
		    fputs(leader,fd->diffs);
		    fprintf(fd->diffs,U_("Glyph “%s” has a different bitmap at %d@%d\n"),
			    bdfc->sc->name, bdf1->pixelsize, BDFDepth(bdf1));
		}
		fd->local_diff = fd->middle_diff = fd->top_diff = fd->diff = true;
	    }
	}
    }
}

static void comparebitmapstrikes(struct font_diff *fd) {
    SplineFont *sf1 = fd->sf1, *sf2=fd->sf2;
    BDFFont *bdf1, *bdf2;

    fd->top_diff = fd->middle_diff = fd->local_diff = false;
    for ( bdf1=sf1->bitmaps; bdf1!=NULL; bdf1=bdf1->next ) {
	for ( bdf2=sf2->bitmaps;
		bdf2!=NULL && (bdf1->pixelsize!=bdf2->pixelsize || BDFDepth(bdf1)!=BDFDepth(bdf2));
		bdf2=bdf2->next );
	if ( bdf2==NULL ) {
	    if ( !fd->top_diff )
		fprintf( fd->diffs, "%s", _("Bitmap Strikes\n") );
	    if ( !fd->middle_diff ) {
		putc(' ',fd->diffs);
		fprintf( fd->diffs, _("Strikes in %s but not in %s\n"), fd->name1, fd->name2 );
	    }
	    fd->top_diff = fd->middle_diff = fd->diff = true;
	    fputs("  ",fd->diffs);
	    fprintf( fd->diffs, _("Strike %d@%d missing from %s\n"),
		    bdf1->pixelsize, BDFDepth(bdf1), fd->name2 );
	}
    }

    fd->middle_diff = false;
    for ( bdf2=sf2->bitmaps; bdf2!=NULL; bdf2=bdf2->next ) {
	for ( bdf1=sf1->bitmaps;
		bdf1!=NULL && (bdf2->pixelsize!=bdf1->pixelsize || BDFDepth(bdf2)!=BDFDepth(bdf1));
		bdf1=bdf1->next );
	if ( bdf1==NULL ) {
	    if ( !fd->top_diff )
		fprintf( fd->diffs, "%s", _("Bitmap Strikes\n") );
	    if ( !fd->middle_diff ) {
		putc(' ',fd->diffs);
		fprintf( fd->diffs, _("Strikes in %s but not in %s\n"), fd->name2, fd->name1 );
	    }
	    fd->top_diff = fd->middle_diff = fd->diff = true;
	    fputs("  ",fd->diffs);
	    fprintf( fd->diffs, _("Strike %d@%d missing from %s\n"),
		    bdf2->pixelsize, BDFDepth(bdf2), fd->name1 );
	}
    }

    fd->middle_diff = false;
    for ( bdf1=sf1->bitmaps; bdf1!=NULL; bdf1=bdf1->next ) {
	for ( bdf2=sf2->bitmaps;
		bdf2!=NULL && (bdf1->pixelsize!=bdf2->pixelsize || BDFDepth(bdf1)!=BDFDepth(bdf2));
		bdf2=bdf2->next );
	if ( bdf2!=NULL )
	    comparebitmapglyphs(fd, bdf1, bdf2);
    }
}

static void NameCompare(struct font_diff *fd,const char *name1, const char *name2, char *id) {

    if (!name1) name1=""; if (!name2) name2="";
    if ( strcmp(name1,name2)!=0 ) {
	if ( !fd->top_diff )
	    fprintf( fd->diffs, "Names\n" );
	fd->top_diff = fd->diff = true;
	putc(' ',fd->diffs);
	fprintf( fd->diffs, _("The %s differs. In %s it is ("), id, fd->name1 );
	while ( *name1!='\0' ) {
	    putc(*name1,fd->diffs);
	    if ( *name1=='\n' )
		fputs("  ",fd->diffs);
	    ++name1;
	}
	fprintf( fd->diffs, _(") while in %s it is ("), fd->name2 );
	while ( *name2!='\0' ) {
	    putc(*name2,fd->diffs);
	    if ( *name2=='\n' )
		fputs("  ",fd->diffs);
	    ++name2;
	}
	fputs(")\n",fd->diffs);
    }
}

static void TtfNameCompare(struct font_diff *fd,char *name1,char *name2,
	int lang,int strid) {
    char strnamebuf[200];

    if (!name1) name1=""; if (!name2) name2="";
    if ( strcmp(name1,name2)==0 )
return;
    sprintf( strnamebuf, "%.90s %.90s", TTFNameIds(strid), MSLangString(lang));
    NameCompare(fd,name1, name2, strnamebuf);
}

static void TtfMissingName(struct font_diff *fd,char *fontname_present,
	char *fontname_missing, char *name, int lang,int strid) {
    char strnamebuf[200];

    sprintf( strnamebuf, "%.90s %.90s", TTFNameIds(strid), MSLangString(lang));
    if ( !fd->top_diff )
	fprintf( fd->diffs, "Names\n" );
    fd->top_diff = fd->diff = true;
    putc(' ',fd->diffs);
    fprintf( fd->diffs, _("The %s is missing in %s. Whilst in %s it is ("),
	    strnamebuf, fontname_missing, fontname_present );
    while ( *name!='\0' ) {
	putc(*name,fd->diffs);
	if ( *name=='\n' )
	    fputs("  ",fd->diffs);
	++name;
    }
    fputs(")\n",fd->diffs);
}

static void comparefontnames(struct font_diff *fd) {
    SplineFont *sf1 = fd->sf1, *sf2=fd->sf2;
    struct ttflangname *names1, *names2;
    int id;

    fd->top_diff = fd->middle_diff = fd->local_diff = false;

    NameCompare(fd,sf1->fontname,sf2->fontname,_("font name"));
    NameCompare(fd,sf1->familyname,sf2->familyname,_("family name"));
    NameCompare(fd,sf1->fullname,sf2->fullname,_("full name"));
    NameCompare(fd,sf1->weight,sf2->weight,_("weight"));
    NameCompare(fd,sf1->copyright,sf2->copyright,_("copyright notice"));
    NameCompare(fd,sf1->version,sf2->version,_("version"));
    for ( names1=sf1->names; names1!=NULL; names1=names1->next ) {
	for ( names2=sf2->names; names2!=NULL && names2->lang!=names1->lang; names2=names2->next );
	if ( names2!=NULL ) {
	    for ( id=0; id<ttf_namemax; ++id )
		if ( names1->names[id]!=NULL && names2->names[id]!=NULL )
		    TtfNameCompare(fd,names1->names[id],names2->names[id],names1->lang,id);
	}
    }
    for ( names1=sf1->names; names1!=NULL; names1=names1->next ) {
	for ( names2=sf2->names; names2!=NULL && names2->lang!=names1->lang; names2=names2->next );
	if ( names2!=NULL ) {
	    for ( id=0; id<ttf_namemax; ++id )
		if ( names1->names[id]!=NULL && names2->names[id]==NULL )
		    TtfMissingName(fd,fd->name1,fd->name2,names1->names[id],names1->lang,id);
	} else {
	    for ( id=0; id<ttf_namemax; ++id )
		if ( names1->names[id]!=NULL )
		    TtfMissingName(fd,fd->name1,fd->name2,names1->names[id],names1->lang,id);
	}
    }
    for ( names2=sf2->names; names2!=NULL; names2=names2->next ) {
	for ( names1=sf1->names; names1!=NULL && names1->lang!=names2->lang; names1=names1->next );
	if ( names1!=NULL ) {
	    for ( id=0; id<ttf_namemax; ++id )
		if ( names2->names[id]!=NULL && names1->names[id]==NULL )
		    TtfMissingName(fd,fd->name2,fd->name1,names2->names[id],names2->lang,id);
	} else {
	    for ( id=0; id<ttf_namemax; ++id )
		if ( names2->names[id]!=NULL )
		    TtfMissingName(fd,fd->name2,fd->name1,names2->names[id],names2->lang,id);
	}
    }
}

/* ************************************************************************** */
/* ************************   Compare font lookups   ************************ */
/* ************************************************************************** */

static int ScriptMatch(struct scriptlanglist *sl1, struct scriptlanglist *sl2,
	int exactness) {
    struct scriptlanglist *s1, *s2;

    if ( exactness ) {
	/* Features & scripts should be ordered */
	for ( s1=sl1, s2=sl2; s1!=NULL && s2!=NULL; s1=s1->next, s2=s2->next ) {
	    if ( s1->script != s2->script )
return( false );
	}
return( true );
    } else {
	for ( s1=sl1; s1!=NULL; s1=s1->next ) {
	    /* Someone from Adobe said (on the OpenType list) that it should */
	    /*  be possible to activate almost all lookups from the default */
	    /*  script (for shaping engines which can't handle the script in */
	    /*  question, or if glyphs are used in a context in which no */
	    /*  script can be determined, or glyphs from font A are used */
	    /*  surounded by those of font B, and the font B glyphs establish */
	    /*  a script font A does not support) so we do not view the presence*/
	    /*  of the default script as a reliable indicator -- only if it */
	    /*  is the sole script */
	    if ( s1->script==DEFAULT_SCRIPT && (s1->next!=NULL || s1!=sl1))
	continue;
	    for ( s2=sl2; s2!=NULL; s2=s2->next ) {
		if ( s2->script==DEFAULT_SCRIPT && (s2->next!=NULL || s2!=sl2))
	    continue;
		if ( s1->script == s2->script )
return( true );
	    }
	}
return( false );
    }
}

static int FeatureMatch(FeatureScriptLangList *fl1, FeatureScriptLangList *fl2,
	int exactness) {
    FeatureScriptLangList *f1, *f2;

    if ( exactness ) {
	/* Features & scripts should be ordered */
	for ( f1=fl1, f2=fl2; f1!=NULL && f2!=NULL; f1=f1->next, f2=f2->next ) {
	    if ( f1->featuretag!=f2->featuretag || !ScriptMatch(f1->scripts,f2->scripts,exactness))
return( false );
	}
return( true );
    } else {
	for ( f1=fl1; f1!=NULL; f1=f1->next ) {
	    for ( f2=fl2; f2!=NULL; f2=f2->next )
		if ( f1->featuretag==f2->featuretag && ScriptMatch(f1->scripts,f2->scripts,exactness))
return( true );
	}
return( false );
    }
}

static int comparepst(struct font_diff *fd,PST *pst1,PST *pst2) {
    if ( pst1->type!=pst2->type )
return( false );
    if ( pst1->type==pst_position ) {
	if ( pst1->u.pos.xoff!=pst2->u.pos.xoff ||
		pst1->u.pos.yoff!=pst2->u.pos.yoff ||
		pst1->u.pos.h_adv_off!=pst2->u.pos.h_adv_off ||
		pst1->u.pos.v_adv_off!=pst2->u.pos.v_adv_off )
return( false );
    } else if ( pst1->type==pst_pair ) {
	if ( pst1->u.pair.vr[0].xoff!=pst2->u.pair.vr[0].xoff ||
		pst1->u.pair.vr[0].yoff!=pst2->u.pair.vr[0].yoff ||
		pst1->u.pair.vr[0].h_adv_off!=pst2->u.pair.vr[0].h_adv_off ||
		pst1->u.pair.vr[0].v_adv_off!=pst2->u.pair.vr[0].v_adv_off ||
		pst1->u.pair.vr[1].xoff!=pst2->u.pair.vr[1].xoff ||
		pst1->u.pair.vr[1].yoff!=pst2->u.pair.vr[1].yoff ||
		pst1->u.pair.vr[1].h_adv_off!=pst2->u.pair.vr[1].h_adv_off ||
		pst1->u.pair.vr[1].v_adv_off!=pst2->u.pair.vr[1].v_adv_off ||
		strcmp(pst1->u.pair.paired,pst2->u.pair.paired)!=0 )
return( false );
    } else if ( pst1->type==pst_substitution || pst1->type==pst_alternate ||
	    pst1->type==pst_multiple || pst1->type==pst_ligature ) {
	if ( strcmp(pst1->u.subs.variant,pst2->u.subs.variant)!=0 )
return( false );
    }
return( true );
}

static int compareap(struct font_diff *fd,AnchorPoint *ap1,AnchorPoint *ap2) {
    if ( ap1->type!=ap2->type )
return( false );
    if ( ap1->me.x!=ap2->me.x || ap1->me.y!=ap2->me.y )
return( false );
    if ( ap1->has_ttf_pt!=ap2->has_ttf_pt ||
	    (ap1->has_ttf_pt && ap1->ttf_pt_index!=ap2->ttf_pt_index))
return( 2 );

return( true );
}

static int classcmp(char *str1, char *str2) {
    int cnt1, cnt2, ch1, ch2;
    char *pt1, *pt2, *start1, *start2;

    /* Sometimes classes are in the same order and all is easy */
    if ( strcmp(str1,str2)==0 )
return( 0 );

    for ( pt1=str1, cnt1=1; *pt1!='\0' ; ++pt1 )
	if ( *pt1==' ' ) {
	    ++cnt1;
	    while ( pt1[1]==' ' ) ++pt1;
	}
    for ( pt2=str2, cnt2=1; *pt2!='\0' ; ++pt2 )
	if ( *pt2==' ' ) {
	    ++cnt2;
	    while ( pt2[1]==' ' ) ++pt2;
	}
    if ( cnt1!=cnt2 )
return( -1 );

    for ( start1=str1; *start1!='\0' ; ) {
	for ( pt1 = start1; *pt1!=' ' && *pt1!='\0'; ++pt1 );
	ch1 = *pt1; *pt1 = '\0';
	for ( start2=str2; *start2!='\0' ; ) {
	    for ( pt2 = start2; *pt2!=' ' && *pt2!='\0'; ++pt2 );
	    ch2 = *pt2; *pt2 = '\0';
	    if ( strcmp( start1, start2 )==0 ) {
		*pt2 = ch2;
	break;
	    }
	    *pt2 = ch2;
	    while ( *pt2==' ' ) ++pt2;
	    start2 = pt2;
	}
	*pt1 = ch1;
	if ( *start2=='\0' )
return( -1 );
	while ( *pt1==' ' ) ++pt1;
	start1 = pt1;
    }
return( 0 );
}

static int comparekc(struct font_diff *fd,KernClass *kc1, KernClass *kc2) {
    int i;

    if ( kc1->first_cnt!=kc2->first_cnt || kc1->second_cnt!=kc2->second_cnt )
return( false );
    if ( memcmp(kc1->offsets,kc2->offsets,kc1->first_cnt*kc2->second_cnt*sizeof(int16))!=0 )
return( false );

    if ( kc1->firsts[0]==NULL && kc2->firsts[0]==NULL )
	/* That's ok */;
    else if ( kc1->firsts[0]!=NULL || kc2->firsts[0]!=NULL )
return( false );
    else if ( classcmp(kc1->firsts[0],kc2->firsts[0])!=0 )
return( false );

    for ( i=1; i<kc1->first_cnt; ++i )
	if ( classcmp(kc1->firsts[i],kc2->firsts[i])!=0 )
return( false );
    for ( i=1; i<kc1->second_cnt; ++i )
	if ( classcmp(kc1->seconds[i],kc2->seconds[i])!=0 )
return( false );

return( true );
}

static int NestedLookupsMatch(struct font_diff *fd,OTLookup *otl1, OTLookup *otl2) {

    if ( fd->l2match1[otl1->lookup_index]==NULL ||
	    fd->l2match1[otl1->lookup_index]!=otl2 )
return( false );

return( true );
}

static int comparefpst(struct font_diff *fd,FPST *fpst1, FPST *fpst2) {
    int i,j;

    if ( fpst1->type!=fpst2->type || fpst1->format!=fpst2->format )
return( false );
    if ( fpst1->rule_cnt != fpst2->rule_cnt )
return( false );

    if ( fpst1->nccnt!=fpst2->nccnt || fpst1->bccnt!=fpst2->bccnt || fpst1->fccnt!=fpst2->fccnt )
return( false );
    for ( i=0; i<fpst1->nccnt; ++i )
	if ( !(fpst1->nclass[i]==NULL && fpst2->nclass[i]==NULL ) &&
		((fpst1->nclass[i]==NULL || fpst2->nclass[i]==NULL ) ||
	    classcmp(fpst1->nclass[i],fpst2->nclass[i])!=0 ))
return( false );
    for ( i=0; i<fpst1->bccnt; ++i )
	if ( !(fpst1->bclass[i]==NULL && fpst2->bclass[i]==NULL ) &&
		((fpst1->bclass[i]==NULL || fpst2->bclass[i]==NULL ) ||
	    classcmp(fpst1->bclass[i],fpst2->bclass[i])!=0 ))
return( false );
    for ( i=0; i<fpst1->fccnt; ++i )
	if ( !(fpst1->fclass[i]==NULL && fpst2->fclass[i]==NULL ) &&
		((fpst1->fclass[i]==NULL || fpst2->fclass[i]==NULL ) ||
	    classcmp(fpst1->fclass[i],fpst2->fclass[i])!=0 ))
return( false );

    for ( i=0; i<fpst1->rule_cnt; ++i ) {
	if ( fpst1->format==pst_glyphs ) {
	    if ( strcmp(fpst1->rules[i].u.glyph.names,fpst2->rules[i].u.glyph.names)!=0 )
return( false );
	    if ( fpst1->rules[i].u.glyph.back!=NULL && fpst2->rules[i].u.glyph.back!=NULL &&
		    strcmp(fpst1->rules[i].u.glyph.back,fpst2->rules[i].u.glyph.back)!=0 )
return( false );
	    if ( fpst1->rules[i].u.glyph.fore!=NULL && fpst2->rules[i].u.glyph.fore!=NULL &&
		    strcmp(fpst1->rules[i].u.glyph.fore,fpst2->rules[i].u.glyph.fore)!=0 )
return( false );
	} else if ( fpst1->format == pst_class ) {
	    if ( fpst1->rules[i].u.class.ncnt!=fpst2->rules[i].u.class.ncnt ||
		    fpst1->rules[i].u.class.bcnt!=fpst2->rules[i].u.class.bcnt ||
		    fpst1->rules[i].u.class.fcnt!=fpst2->rules[i].u.class.fcnt )
return( false );
	    if ( memcmp(fpst1->rules[i].u.class.nclasses,fpst2->rules[i].u.class.nclasses,
		    fpst1->rules[i].u.class.ncnt*sizeof(uint16))!=0 )
return( false );
	    if ( fpst1->rules[i].u.class.bcnt!=0 &&
		    memcmp(fpst1->rules[i].u.class.bclasses,fpst2->rules[i].u.class.bclasses,
			fpst1->rules[i].u.class.bcnt*sizeof(uint16))!=0 )
return( false );
	    if ( fpst1->rules[i].u.class.fcnt!=0 &&
		    memcmp(fpst1->rules[i].u.class.fclasses,fpst2->rules[i].u.class.fclasses,
			fpst1->rules[i].u.class.fcnt*sizeof(uint16))!=0 )
return( false );
	} else if ( fpst1->format == pst_coverage || fpst1->format == pst_reversecoverage ) {
	    if ( fpst1->rules[i].u.coverage.ncnt!=fpst2->rules[i].u.class.ncnt ||
		    fpst1->rules[i].u.coverage.bcnt!=fpst2->rules[i].u.class.bcnt ||
		    fpst1->rules[i].u.coverage.fcnt!=fpst2->rules[i].u.class.fcnt )
return( false );
	    for ( j=0; j<fpst1->rules[i].u.coverage.ncnt; ++j )
		if ( classcmp(fpst1->rules[i].u.coverage.ncovers[j],fpst2->rules[i].u.coverage.ncovers[j])!=0 )
return( false );
	    for ( j=0; j<fpst1->rules[i].u.coverage.bcnt; ++j )
		if ( classcmp(fpst1->rules[i].u.coverage.bcovers[j],fpst2->rules[i].u.coverage.bcovers[j])!=0 )
return( false );
	    for ( j=0; j<fpst1->rules[i].u.coverage.fcnt; ++j )
		if ( classcmp(fpst1->rules[i].u.coverage.fcovers[j],fpst2->rules[i].u.coverage.fcovers[j])!=0 )
return( false );
	    if ( fpst1->format == pst_reversecoverage )
		if ( strcmp(fpst1->rules[i].u.rcoverage.replacements,fpst2->rules[i].u.rcoverage.replacements)!=0 )
return( false );
	} else
return( false);

	if ( fpst1->rules[i].lookup_cnt!=fpst2->rules[i].lookup_cnt )
return( false );
	for ( j=0; j<fpst1->rules[i].lookup_cnt; ++j )
	    if ( fpst1->rules[i].lookups[j].seq!=fpst2->rules[i].lookups[j].seq )
return( false );
	for ( j=0; j<fpst1->rules[i].lookup_cnt; ++j )
	    if ( !NestedLookupsMatch(fd,
		    fpst1->rules[i].lookups[j].lookup,
		    fpst2->rules[i].lookups[j].lookup))
return( false );
    }
    
return( true );
}

/* See if we have an exact match on the subtable */
static int comparelookupsubtable(struct font_diff *fd,struct lookup_subtable *sub1,struct lookup_subtable *sub2) {
    int gid1;
    SplineChar *sc1, *sc2;
    PST *pst1, *pst2;
    AnchorPoint *ap1, *ap2;
    int test_anchors, test_psts, test_kerns;
    int lookup_type;
    int isv;
    KernPair *kp1, *kp2;

    /* These are complex to check and involve testing nested lookups which we */
    /*  might not have worked out yet */
    if ( sub1->fpst!=NULL || sub1->sm!=NULL )
return( false );
    if ( sub1->kc!=NULL || sub2->kc!=NULL ) {
	if ( sub1->kc!=NULL && sub2->kc!=NULL && comparekc(fd,sub1->kc,sub2->kc))
return( true );

return( false );
    }

    lookup_type = sub1->lookup->lookup_type;
    test_anchors = lookup_type>=gpos_cursive && lookup_type<=gpos_mark2mark;
    test_psts = (lookup_type>=gsub_single && lookup_type<=gsub_ligature) ||
	    lookup_type == gpos_single || lookup_type==gpos_pair;
    test_kerns = lookup_type==gpos_pair;
    if ( !test_anchors && !test_kerns && !test_psts )
return( false );

    for ( gid1=0; gid1<fd->sf1_glyphcnt; ++gid1 ) if ( (sc2=fd->matches[gid1])!=NULL && (sc1=fd->sf1->glyphs[gid1])!=NULL ) {
	if ( test_psts ) {
	    for ( pst1=sc1->possub; pst1!=NULL; pst1=pst1->next ) if ( pst1->subtable==sub1 ) {
		for ( pst2=sc2->possub; pst2!=NULL; pst2=pst2->next ) if ( pst2->subtable==sub2 ) {
		    if ( comparepst(fd,pst1,pst2))
		break;
		}
		if ( pst2==NULL )
return( false );
	    }
	    for ( pst2=sc2->possub; pst2!=NULL; pst2=pst2->next ) if ( pst2->subtable==sub2 ) {
		for ( pst1=sc1->possub; pst1!=NULL; pst1=pst1->next ) if ( pst1->subtable==sub1 ) {
		    if ( comparepst(fd,pst1,pst2))
		break;
		}
		if ( pst1==NULL )
return( false );
	    }
	}
	if ( test_kerns ) {
	    for ( isv=0; isv<2; ++isv ) {
		for ( kp1= isv ? sc1->kerns : sc1->vkerns; kp1!=NULL; kp1=kp1->next ) if ( kp1->subtable==sub1 ) {
		    for ( kp2= isv ? sc2->kerns : sc2->vkerns; kp2!=NULL; kp2=kp2->next ) if ( kp2->subtable==sub2 ) {
			if ( kp1->off == kp2->off && kp2->sc==fd->matches[kp1->sc->orig_pos])
		    break;
		    }
		    if ( kp2==NULL )
return( false );
		}
		for ( kp2= isv ? sc2->kerns : sc2->vkerns; kp2!=NULL; kp2=kp2->next ) if ( kp2->subtable==sub2 ) {
		    for ( kp1= isv ? sc1->kerns : sc1->vkerns; kp1!=NULL; kp1=kp1->next ) if ( kp1->subtable==sub1 ) {
			if ( kp1->off == kp2->off && kp2->sc==fd->matches[kp1->sc->orig_pos])
		    break;
		    }
		    if ( kp1==NULL )
return( false );
		}
	    }
	}
	if ( test_anchors ) {
	    for ( ap1=sc1->anchor; ap1!=NULL; ap1=ap1->next ) if ( ap1->anchor->subtable == sub1 ) {
		for ( ap2=sc2->anchor; ap2!=NULL; ap2=ap2->next ) if ( ap2->anchor->subtable == sub2 ) {
		    if ( compareap(fd,ap1,ap2)!=true )
		break;
		}
		if ( ap2==NULL )
return( false );
	    }
	    for ( ap2=sc2->anchor; ap2!=NULL; ap2=ap2->next ) if ( ap2->anchor->subtable == sub2 ) {
		for ( ap1=sc1->anchor; ap1!=NULL; ap1=ap1->next ) if ( ap1->anchor->subtable == sub1 ) {
		    if ( compareap(fd,ap1,ap2)!=true )
		break;
		}
		if ( ap1==NULL )
return( false );
	    }
	}
    }
return( true );
}

static void MatchLookups(struct font_diff *fd) {
    int lcnt, scnt, scnt2;
    OTLookup *otl, *otl2;
    SplineFont *sf1 = fd->sf1, *sf2 = fd->sf2;
    int exactness;
    struct lookup_subtable *sub, *sub2;

    if ( sf1->cidmaster ) sf1 = sf1->cidmaster;
    if ( sf2->cidmaster ) sf2 = sf2->cidmaster;
    fd->sf1_mst = sf1; fd->sf2_mst = sf2;

    for ( scnt = lcnt=0, otl=fd->is_gpos ? sf1->gpos_lookups : sf1->gsub_lookups; otl!=NULL; otl=otl->next, ++lcnt ) {
	otl->lookup_index = lcnt;
	otl->ticked = false;
	for ( sub=otl->subtables; sub!=NULL; sub=sub->next, ++scnt )
	    sub->subtable_offset = scnt;
    }
    fd->lcnt1 = lcnt;
    fd->l2match1 = calloc(lcnt,sizeof(OTLookup *));
    fd->scnt1 = scnt;
    fd->s2match1 = calloc(scnt,sizeof(OTLookup *));

    for ( scnt = lcnt=0, otl=fd->is_gpos ? sf2->gpos_lookups : sf2->gsub_lookups; otl!=NULL; otl=otl->next, ++lcnt ) {
	otl->lookup_index = lcnt;
	otl->ticked = false;
	for ( sub=otl->subtables; sub!=NULL; sub=sub->next, ++scnt )
	    sub->subtable_offset = scnt;
    }
    fd->lcnt2 = lcnt;
    fd->l1match2 = calloc(lcnt,sizeof(OTLookup *));
    fd->scnt1 = scnt;
    fd->s1match2 = calloc(scnt,sizeof(OTLookup *));

    for ( otl=fd->is_gpos ? sf1->gpos_lookups : sf1->gsub_lookups; otl!=NULL; otl=otl->next ) {
	for ( otl2=fd->is_gpos ? sf2->gpos_lookups : sf2->gsub_lookups; otl2!=NULL; otl2=otl2->next ) {
	    if ( otl->lookup_type != otl2->lookup_type || otl2->ticked )
	continue;
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		for ( sub2=otl2->subtables; sub2!=NULL; sub2=sub2->next ) {
		    if ( sub2->ticked )
		continue;
		    if ( comparelookupsubtable(fd,sub,sub2)) {
			sub->ticked = sub2->ticked = true;
			otl->ticked = otl2->ticked = true;
			fd->s2match1[sub->subtable_offset] = sub2;
			fd->s1match2[sub2->subtable_offset] = sub;
			fd->l2match1[otl->lookup_index] = otl2;
			fd->l1match2[otl2->lookup_index] = otl;
      goto break_3_loops;
		    }
		}
	    }
	}
      break_3_loops: ;
    }
	    
    for ( exactness=3; exactness>=0; --exactness ) {
	for ( lcnt=0, otl=fd->is_gpos ? sf1->gpos_lookups : sf1->gsub_lookups; otl!=NULL; otl=otl->next, ++lcnt ) {
	    if ( otl->ticked )
	continue;
	    for ( otl2=fd->is_gpos ? sf2->gpos_lookups : sf2->gsub_lookups; otl2!=NULL; otl2=otl2->next ) {
		if ( otl2->ticked )
	    continue;
		if ( otl->lookup_type==otl2->lookup_type &&
			(!(exactness&2) || strcmp(otl->lookup_name,otl2->lookup_name)==0) &&
			FeatureMatch(otl->features,otl2->features,exactness&1)) {
		    otl->ticked = otl2->ticked = true;
		    fd->l2match1[lcnt] = otl2;
		    fd->l1match2[otl2->lookup_index] = otl;
	    break;
		}
	    }
	}
    }

    for ( lcnt=0, otl=fd->is_gpos ? sf1->gpos_lookups : sf1->gsub_lookups; otl!=NULL; otl=otl->next, ++lcnt ) {
	if ( (otl2 = fd->l2match1[lcnt])==NULL )
    break;
	for ( scnt=0, sub=otl->subtables; sub!=NULL; ++scnt, sub=sub->next );
	for ( scnt2=0, sub=otl2->subtables; sub!=NULL; ++scnt2, sub=sub->next );
	if ( scnt==scnt2 ) {
	    /* Try assigning all unassigned subtables in order */
	    for ( sub=otl->subtables, sub2=otl2->subtables; sub!=NULL && sub2!=NULL; ) {
		if ( fd->s2match1[sub->subtable_offset]==NULL &&
			fd->s1match2[sub2->subtable_offset]==NULL ) {
		    fd->s2match1[sub->subtable_offset]=sub2;
		    fd->s1match2[sub2->subtable_offset]=sub;
		    sub=sub->next;
		    sub2=sub2->next;
		} else {
		    if ( fd->s2match1[sub->subtable_offset]!=NULL )
			sub=sub->next;
		    if ( fd->s1match2[sub2->subtable_offset]!=NULL )
			sub2=sub2->next;
		}
	    }
	}
    }
}

static void featureheader(struct font_diff *fd) {

    if ( !fd->top_diff )
	fprintf( fd->diffs, "%s", fd->is_gpos ? _("Glyph Positioning\n") : _("Glyph Substitution\n"));
    if ( !fd->middle_diff ) {
	putc( ' ', fd->diffs);
	fprintf( fd->diffs, "%s", _("Lookup Differences\n") );
    }
    if ( !fd->local_diff ) {
	fputs("  ",fd->diffs);
	fprintf( fd->diffs, _("Lookup subtable %s (matched with %s)\n"),
		fd->cur_sub1->subtable_name,
		fd->cur_sub2==NULL ? _("<Nothing>") : fd->cur_sub2->subtable_name );
    }
    fd->top_diff = fd->middle_diff = fd->diff = fd->local_diff = true;
}

static void complainscfeature(struct font_diff *fd, SplineChar *sc, char *format, ... ) {
    va_list ap;

    featureheader(fd);

    va_start(ap,format);
    if ( fd->last_sc==sc ) {
	if ( fd->held[0] ) {
	    fputs("   ",fd->diffs);
	    fprintf( fd->diffs, U_("Glyph “%s” differs\n"), sc->name );
	    fprintf( fd->diffs, "    %s", fd->held );
	    if ( fd->held[strlen(fd->held)-1]!='\n' )
		putc('\n',fd->diffs);
	    fd->held[0] = '\0';
	}
	fputs("    ",fd->diffs);
	vfprintf(fd->diffs,format,ap);
    } else {
	vsnprintf(fd->held,sizeof(fd->held),format,ap);
	fd->last_sc = sc;
    }
    va_end(ap);
}

static void complainapfeature(struct font_diff *fd,SplineChar *sc,
	AnchorPoint *ap,char *missingname) {
    complainscfeature(fd, sc, U_("“%s” in %s did not contain an anchor point (%g,%g) class %s\n"),
	    sc->name, missingname, ap->me.x, ap->me.y, ap->anchor->name);
}

static void complainapfeature2(struct font_diff *fd,SplineChar *sc,
	AnchorPoint *ap,char *missingname) {
    complainscfeature(fd, sc, U_("“%s” in %s contains an anchor point (%g,%g) class %s which differs from its counterpart by point matching\n"),
	    sc->name, missingname, ap->me.x, ap->me.y, ap->anchor->name);
}

static void complainpstfeature(struct font_diff *fd,SplineChar *sc,
	PST *pst,char *missingname) {
    if ( pst->type==pst_position ) {
	complainscfeature(fd, sc, U_("“%s” in %s did not contain a positioning lookup ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d\n"),
		sc->name, missingname, pst->u.pos.xoff, pst->u.pos.yoff, pst->u.pos.h_adv_off, pst->u.pos.v_adv_off );
    } else if ( pst->type==pst_pair ) {
	complainscfeature(fd, sc, U_("“%s” in %s did not contain a pairwise positioning lookup ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d to %s ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d\n"),
		sc->name, missingname, 
		pst->u.pair.vr[0].xoff, pst->u.pair.vr[0].yoff, pst->u.pair.vr[0].h_adv_off, pst->u.pair.vr[0].v_adv_off,
		pst->u.pair.paired,
	    pst->u.pair.vr[1].xoff, pst->u.pair.vr[1].yoff, pst->u.pair.vr[1].h_adv_off, pst->u.pair.vr[1].v_adv_off );
    } else if ( pst->type==pst_substitution || pst->type==pst_alternate || pst->type==pst_multiple || pst->type==pst_ligature ) {
	complainscfeature(fd, sc, U_("“%s” in %s did not contain a substitution lookup to %s\n"),
		sc->name, missingname, pst->u.subs.variant );
    }
}

static void finishscfeature(struct font_diff *fd ) {
    if ( fd->held[0] ) {
	fputs("   ",fd->diffs);
	fputs(fd->held,fd->diffs);
	fd->held[0] = '\0';
    }
}

static void comparesubtable(struct font_diff *fd) {
    int gid1;
    SplineChar *sc1, *sc2;
    PST *pst1, *pst2;
    AnchorPoint *ap1, *ap2;
    int isv;
    KernPair *kp1, *kp2;
    int test_anchors, test_psts, test_kerns;
    int lookup_type;

    fd->last_sc = NULL;

    lookup_type = fd->cur_sub1->lookup->lookup_type;
    test_anchors = lookup_type>=gpos_cursive && lookup_type<=gpos_mark2mark;
    test_psts = (lookup_type>=gsub_single && lookup_type<=gsub_ligature) ||
	    lookup_type == gpos_single || lookup_type==gpos_pair;
    test_kerns = lookup_type==gpos_pair;
    if ( !test_anchors && !test_kerns && !test_psts ) {
	if ( fd->cur_sub1->kc && fd->cur_sub2->kc ) {
	    if ( comparekc(fd,fd->cur_sub1->kc,fd->cur_sub2->kc)) {
		featureheader(fd);
		fprintf( fd->diffs,_("The kerning class subtable %s in %s fails to match %s in %s\n"),
			fd->cur_sub1->subtable_name, fd->name1,
			fd->cur_sub2->subtable_name, fd->name2 );
	    }
	} else if ( fd->cur_sub1->fpst && fd->cur_sub2->fpst ) {
	    if ( !comparefpst(fd,fd->cur_sub1->fpst,fd->cur_sub2->fpst)) {
		featureheader(fd);
		fprintf( fd->diffs,_("The context/chaining subtable %s in %s fails to match %s in %s\n"),
			fd->cur_sub1->subtable_name, fd->name1,
			fd->cur_sub2->subtable_name, fd->name2 );
	    }
	} else {
	    featureheader(fd);
	    fputs("   ",fd->diffs);
	    fprintf( fd->diffs,_("I can't figure out how to compare the subtable, %s, in %s to %s in %s\n"),
		    fd->cur_sub1->subtable_name, fd->name1, fd->cur_sub2->subtable_name, fd->name2 );
	}
return;
    }

    for ( gid1=0; gid1<fd->sf1_glyphcnt; ++gid1 ) if ( (sc2=fd->matches[gid1])!=NULL && (sc1=fd->sf1->glyphs[gid1])!=NULL ) {
	if ( test_psts ) {
	    for ( pst1=sc1->possub; pst1!=NULL; pst1=pst1->next ) if ( pst1->subtable==fd->cur_sub1 ) {
		for ( pst2=sc2->possub; pst2!=NULL; pst2=pst2->next ) if ( pst2->subtable==fd->cur_sub2 ) {
		    if ( comparepst(fd,pst1,pst2))
		break;
		}
		if ( pst2==NULL )
		    complainpstfeature(fd,sc1,pst1,fd->name2);
	    }
	    for ( pst2=sc2->possub; pst2!=NULL; pst2=pst2->next ) if ( pst2->subtable==fd->cur_sub2 ) {
		for ( pst1=sc1->possub; pst1!=NULL; pst1=pst1->next ) if ( pst1->subtable==fd->cur_sub1 ) {
		    if ( comparepst(fd,pst1,pst2))
		break;
		}
		if ( pst1==NULL )
		    complainpstfeature(fd,sc1,pst2,fd->name1);
	    }
	}
	if ( test_anchors ) {
	    for ( ap1=sc1->anchor; ap1!=NULL; ap1=ap1->next ) if ( ap1->anchor->subtable==fd->cur_sub1 ) {
		for ( ap2=sc2->anchor; ap2!=NULL; ap2=ap2->next ) if ( ap2->anchor->subtable==fd->cur_sub2 ) {
		    int apret= compareap(fd,ap1,ap2);
		    if ( apret==2 )
			complainapfeature2(fd,sc1,ap1,fd->name2);
		    if ( apret )
		break;
		}
		if ( ap2==NULL )
		    complainapfeature(fd,sc1,ap1,fd->name2);
	    }
	    for ( ap2=sc2->anchor; ap2!=NULL; ap2=ap2->next ) if ( ap2->anchor->subtable==fd->cur_sub2 ) {
		for ( ap1=sc1->anchor; ap1!=NULL; ap1=ap1->next ) if ( ap1->anchor->subtable==fd->cur_sub1 ) {
		    /* don't need to check for point matching here, already done above */
		    if ( compareap(fd,ap1,ap2))
		break;
		}
		if ( ap1==NULL )
		    complainapfeature(fd,sc1,ap2,fd->name1);
	    }
	}
	if ( test_kerns ) {
	    for ( isv=0; isv<2; ++isv ) {
		for ( kp2= isv ? sc2->vkerns : sc2->kerns ; kp2!=NULL; kp2=kp2->next )
		    kp2->kcid = 0;
		for ( kp1= isv ? sc1->vkerns : sc1->kerns; kp1!=NULL; kp1=kp1->next ) {
		    if ( kp1->subtable!=fd->cur_sub1 )
		continue;
		    for ( kp2= isv ? sc2->vkerns : sc2->kerns ; kp2!=NULL; kp2=kp2->next ) if ( !kp2->kcid ) {
			if ( kp2->subtable!=fd->cur_sub2 )
		    continue;
			if ( strcmp(kp1->sc->name,kp2->sc->name)==0 )
		    break;
		    }
		    if ( kp2!=NULL ) {
			if ( kp2->off!=kp1->off )
			    complainscfeature(fd, sc1, U_("Kerning between “%s” and %s is %d in %s and %d in %s\n"),
				    sc1->name, kp1->sc->name, kp1->off, fd->name1, kp2->off, fd->name2 );
			kp2->kcid = 1;
		    } else {
			complainscfeature(fd, sc1, U_("No kerning between “%s” and %s in %s whilst it is %d in %s\n"),
				sc1->name, kp1->sc->name, fd->name2, kp1->off, fd->name1 );
		    }
		}
		for ( kp2= isv ? sc2->vkerns : sc2->kerns ; kp2!=NULL; kp2=kp2->next )
		    if ( !kp2->kcid && kp2->subtable == fd->cur_sub2 )
			complainscfeature(fd, sc1, U_("No kerning between “%s” and %s in %s whilst it is %d in %s\n"),
				sc2->name, kp2->sc->name, fd->name1, kp2->off, fd->name2 );
		}
	}
	finishscfeature(fd);
    }
}

static void compareg___(struct font_diff *fd) {
    OTLookup *otl;
    struct lookup_subtable *sub;

    fd->top_diff = fd->middle_diff = fd->local_diff = false;

    MatchLookups(fd);

    fd->middle_diff = false;
    for ( otl = fd->is_gpos ? fd->sf1_mst->gpos_lookups : fd->sf1_mst->gsub_lookups; otl!=NULL ; otl=otl->next ) {
	if ( fd->l2match1[otl->lookup_index]==NULL ) {
	    if ( !fd->top_diff )
		fprintf( fd->diffs, "%s", fd->is_gpos ? _("Glyph Positioning\n") : _("Glyph Substitution\n"));
	    if ( !fd->middle_diff ) {
		putc( ' ', fd->diffs);
		fprintf( fd->diffs, _("Lookups in %s but not in %s\n"), fd->name1, fd->name2 );
	    }
	    fd->top_diff = fd->middle_diff = fd->diff = true;
	    fputs("  ",fd->diffs);
	    fprintf( fd->diffs, _("Lookup %s is not in %s\n"),
		    otl->lookup_name, fd->name2 );
	}
    }
    fd->middle_diff = false;
    for ( otl = fd->is_gpos ? fd->sf1_mst->gpos_lookups : fd->sf1_mst->gsub_lookups; otl!=NULL ; otl=otl->next ) {
	if ( fd->l2match1[otl->lookup_index]!=NULL ) {
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		if ( fd->s2match1[sub->subtable_offset]==NULL ) {
		    if ( !fd->top_diff )
			fprintf( fd->diffs, "%s", fd->is_gpos ? _("Glyph Positioning\n") : _("Glyph Substitution\n"));
		    if ( !fd->middle_diff ) {
			putc( ' ', fd->diffs);
			fprintf( fd->diffs, _("Lookups subtables in %s but not in %s\n"), fd->name1, fd->name2 );
		    }
		    fd->top_diff = fd->middle_diff = fd->diff = true;
		    fputs("  ",fd->diffs);
		    fprintf( fd->diffs, _("Lookup subtable %s is not in %s\n"),
			    sub->subtable_name, fd->name2 );
		}
	    }
	}
    }

    fd->middle_diff = false;
    for ( otl = fd->is_gpos ? fd->sf2_mst->gpos_lookups : fd->sf2_mst->gsub_lookups; otl!=NULL ; otl=otl->next ) {
	if ( fd->l1match2[otl->lookup_index]==NULL ) {
	    if ( !fd->top_diff )
		fprintf( fd->diffs, "%s", fd->is_gpos ? _("Glyph Positioning\n") : _("Glyph Substitution\n"));
	    if ( !fd->middle_diff ) {
		putc( ' ', fd->diffs);
		fprintf( fd->diffs, _("Lookups in %s but not in %s\n"), fd->name2, fd->name1 );
	    }
	    fd->top_diff = fd->middle_diff = fd->diff = true;
	    fputs("  ",fd->diffs);
	    fprintf( fd->diffs, _("Lookup %s is not in %s\n"),
		    otl->lookup_name, fd->name1 );
	}
    }
    fd->middle_diff = false;
    for ( otl = fd->is_gpos ? fd->sf2_mst->gpos_lookups : fd->sf2_mst->gsub_lookups; otl!=NULL ; otl=otl->next ) {
	if ( fd->l1match2[otl->lookup_index]!=NULL ) {
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		if ( fd->s1match2[sub->subtable_offset]==NULL ) {
		    if ( !fd->top_diff )
			fprintf( fd->diffs, "%s", fd->is_gpos ? _("Glyph Positioning\n") : _("Glyph Substitution\n"));
		    if ( !fd->middle_diff ) {
			putc( ' ', fd->diffs);
			fprintf( fd->diffs, _("Lookups subtables in %s but not in %s\n"), fd->name2, fd->name1 );
		    }
		    fd->top_diff = fd->middle_diff = fd->diff = true;
		    fputs("  ",fd->diffs);
		    fprintf( fd->diffs, _("Lookup subtable %s is not in %s\n"),
			    sub->subtable_name, fd->name1 );
		}
	    }
	}
    }

    fd->middle_diff = false;
    for ( otl = fd->is_gpos ? fd->sf1_mst->gpos_lookups : fd->sf1_mst->gsub_lookups; otl!=NULL ; otl=otl->next ) {
	if ( fd->l1match2[otl->lookup_index]!=NULL ) {
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		if ( fd->s2match1[sub->subtable_offset]!=NULL ) {
		    fd->cur_sub1 = sub; fd->cur_sub2 = fd->s2match1[sub->subtable_offset];
		    fd->local_diff = false;
		    comparesubtable(fd);
		}
	    }
	}
    }

    free( fd->l2match1 ); free( fd->l1match2 );
    free( fd->s2match1 ); free( fd->s1match2 );
}

static void comparegpos(struct font_diff *fd) {
    fd->is_gpos = true;
    compareg___(fd);
}

static void comparegsub(struct font_diff *fd) {
    fd->is_gpos = false;
    compareg___(fd);
}

#include "ttf.h"

int CompareFonts(SplineFont *sf1, EncMap *map1, SplineFont *sf2, FILE *diffs,
	int flags) {
    int gid1, gid2;
    SplineChar *sc, *sc2;
    struct font_diff fd;

    memset(&fd,0,sizeof(fd));

    if (( sf1->cidmaster || sf1->subfontcnt!=0 ) &&
	    (sf2->cidmaster || sf2->subfontcnt!=0 )) {
	if ( sf1->cidmaster ) sf1 = sf1->cidmaster;
	if ( sf2->cidmaster ) sf2 = sf2->cidmaster;
	SFDummyUpCIDs(NULL,sf1);
	SFDummyUpCIDs(NULL,sf2);
    } else if ( sf1->subfontcnt!=0 )
	sf1 = sf1->subfonts[0];
    else if ( sf2->subfontcnt!=0 )
	sf2 = sf2->subfonts[0];
    fd.sf1 = sf1; fd.sf2 = sf2; fd.diffs = diffs; fd.flags = flags;
    fd.map1 = map1;
    fd.sf1_glyphcnt = sf1->glyphcnt;

    if ( strcmp( sf1->fontname,sf2->fontname )!=0 ) {
	fd.name1 = sf1->fontname; fd.name2 = sf2->fontname;
    } else if ( sf1->fullname!=NULL && sf2->fullname!=NULL &&
	    strcmp( sf1->fullname,sf2->fullname )!=0 ) {
	fd.name1 = sf1->fullname; fd.name2 = sf2->fullname;
    } else if ( sf1->version!=NULL && sf2->version!=NULL &&
	    strcmp( sf1->version,sf2->version )!=0 ) {
	fd.name1 = sf1->version; fd.name2 = sf2->version;
    } else {
	if ( sf1->filename==NULL )
	    fd.name1 = sf1->origname;
	else
	    fd.name1 = sf1->filename;
	if ( sf2->filename==NULL )
	    fd.name2 = sf2->origname;
	else
	    fd.name2 = sf2->filename;
    }

    for ( gid2=0; gid2<sf2->glyphcnt; ++gid2 ) if ( (sc=sf2->glyphs[gid2])!=NULL )
	sc->ticked = false;
    for ( gid1=0; gid1<sf1->glyphcnt; ++gid1 ) if ( (sc=sf1->glyphs[gid1])!=NULL )
	sc->ticked = false;
    fd.matches = calloc(sf1->glyphcnt,sizeof(SplineChar *));

    for ( gid1=0; gid1<sf1->glyphcnt; ++gid1 ) if ( (sc=sf1->glyphs[gid1])!=NULL ) {
	sc2 = SFGetChar(sf2,sc->unicodeenc,sc->name);
	fd.matches[gid1] = sc2;
	if ( sc2!=NULL ) {
	    sc2->ticked = true;
	    sc->ticked = true;
	}
    }

    if ( flags&fcf_names )
	comparefontnames(&fd);
    if ( flags&fcf_outlines )
	comparefontglyphs(&fd);
    if ( flags&fcf_bitmaps )
	comparebitmapstrikes(&fd);
    if ( flags&fcf_gpos )
	comparegpos(&fd);
    if ( flags&fcf_gsub )
	comparegsub(&fd);

    free(fd.matches);

    if ( sf1->subfontcnt!=0 && sf2->subfontcnt!=0 ) {
	free(sf1->glyphs); sf1->glyphs = NULL;
	sf1->glyphcnt = sf1->glyphmax = 0;
	free(sf2->glyphs); sf2->glyphs = NULL;
	sf2->glyphcnt = sf2->glyphmax = 0;
    }

    if ( fd.sf1_glyphcnt!=sf1->glyphcnt )	/* If we added glyphs, they didn't get hashed properly */
	GlyphHashFree(sf1);

return( fd.diff );
}

int LayersSimilar(Layer *ly1, Layer *ly2, double spline_err) {
    SplinePoint *hmfail;
    int ret;

    if ( !fdRefCheck(NULL,NULL,ly1->refs,ly2->refs,false) )
return( false );
    ret = SSsCompare(ly1->splines,ly2->splines,spline_err,spline_err,&hmfail);
    if ( ret&SS_NoMatch )
return( false );

return( true );
}
