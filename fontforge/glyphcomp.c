/* Copyright (C) 2006 by George Williams */
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
#include "scriptfuncs.h"
#include <math.h>

/* ************************************************************************** */
/* ********************* Code to compare outline glyphs ********************* */
/* ************************************************************************** */

static double FindNewT(double pos,const Spline1D *s,double old_t) {
    double ts[3];
    int i;
    double closest;

    SplineSolveFull(s, pos, ts);
    closest = -1;
    for ( i=0; i<3 && ts[i]!=-1; ++i ) {
	if ( ts[i]>old_t && ts[i]<=1 ) {
	    if ( closest==-1 || ts[i]<closest )
		closest = ts[i];
	}
    }
return( closest );
}

/* I have arranged so that either both splineset are clockwise or anti */
/*  Thus when we walk forward on one splineset we will walk forward on the */
/*  other. So no need to back track here */
static int NearSplineSet(BasePoint *here,const SplineSet *ss,
	const Spline **last_found,double *last_t,double err) {
    const Spline *first, *s, *best_s;
    double dx, dy, adx, ady, best, best_t, ts[3], t;
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
		    SplineSolveFull(&s->splines[0], here->x+err*offset[j], ts);
		else
		    SplineSolveFull(&s->splines[1], here->y+err*offset[j], ts);
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
		    SplineSolveFull(&s->splines[0], here->x+err*offset[j], ts);
		else
		    SplineSolveFull(&s->splines[1], here->y+err*offset[j], ts);
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
	forever {
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

static int AllPointsMatch(const SplinePoint *start1, const SplinePoint *start2, real err) {
    double dx, dy;
    const SplinePoint *sp1=start1, *sp2=start2;

    forever {
	if ( (dx = sp1->me.x-sp2->me.x)<=err && dx>=-err &&
		(dy = sp1->me.y-sp2->me.y)<=err && dy>=-err &&
		(dx = sp1->nextcp.x-sp2->nextcp.x)<=err && dx>=-err &&
		(dy = sp1->nextcp.y-sp2->nextcp.y)<=err && dy>=-err &&
		(dx = sp1->prevcp.x-sp2->prevcp.x)<=err && dx>=-err &&
		(dy = sp1->prevcp.y-sp2->prevcp.y)<=err && dy>=-err )
	    /* Good */;
	else
return( false );

	if ( sp2->next==NULL && sp1->next==NULL )
return( true );
	if ( sp2->next==NULL || sp1->next==NULL )
return( false );
	sp1 = sp1->next->to;
	sp2 = sp2->next->to;
	if ( sp1 == start1 && sp2 == start2 )
return( true );
	if ( sp1 == start1 || sp2 == start2 )
return( false );
    }
}

static int ContourPointsMatch(const SplineSet *ss1, const SplineSet *ss2, real err) {
    const SplinePoint *sp2;

    /* Does ANY point on the second contour match the start point of the first? */
    for ( sp2 = ss2->first; ; ) {
	if ( AllPointsMatch(ss1->first,sp2,err) )
return( sp2==ss2->first?1:2 );
	if ( sp2->next==NULL )
return( false );
	sp2 = sp2->next->to;
	if ( sp2 == ss2->first )
return( false );
    }
}
    
enum Compare_Ret SSsCompare(const SplineSet *ss1, const SplineSet *ss2, real pt_err, real spline_err) {
    int cnt1, cnt2, bestcnt;
    const SplineSet *ss, *s2s, *bestss;
    enum Compare_Ret info = 0;
    int allmatch;
    DBounds *b1, *b2;
    const SplineSet **match;
    double diff, delta, bestdiff;
    double dx, dy;

    for ( ss=ss1, cnt1=0; ss!=NULL; ss=ss->next, ++cnt1 );
    for ( ss=ss2, cnt2=0; ss!=NULL; ss=ss->next, ++cnt2 );
    if ( cnt1!=cnt2 )
return( SS_DiffContourCount|SS_NoMatch );

    b1 = galloc(cnt1*sizeof(DBounds));
    b2 = galloc(cnt1*sizeof(DBounds));
    match = galloc(cnt1*sizeof(SplineSet *));
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
		    (ss1->first==ss1->last) == (ss2->first==ss2->last)) {
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
	    int ret = ContourPointsMatch(ss,match[cnt1],pt_err);
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
	if ( allmatch )
	    info |= SS_ContourMatch;
    }

    free(match);
    if ( !allmatch )
return( SS_NoMatch );

return( info );
}

/* ************************************************************************** */
/* ********************* Code to compare bitmap glyphs ********************** */
/* ************************************************************************** */

enum Compare_Ret BitmapCompare(BDFChar *bc1, BDFChar *bc2, int err, int bb_err) {
    uint8 *pt1, *pt2;
    int i,j, d, xlen;
    int mask;
    int xmin, xmax, ymin, ymax, c1, c2;

    if ( bc1->byte_data!=bc2->byte_data )
return( BC_DepthMismatch|BC_NoMatch );

    if ( bc1->width!=bc2->width )
return( SS_WidthMismatch|BC_NoMatch );

    BCFlattenFloat(bc1);
    BCCompressBitmap(bc1);

    if ( bc1->byte_data ) {
	if ( (d=bc1->xmin-bc2->xmin)>bb_err || d<-bb_err ||
		(d=bc1->ymin-bc2->ymin)>bb_err || d<-bb_err ||
		(d=bc1->xmax-bc2->xmax)>bb_err || d<-bb_err ||
		(d=bc1->ymax-bc2->ymax)>bb_err || d<-bb_err )
return( BC_BoundingBoxMismatch|BC_NoMatch );
		
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
return( BC_NoMatch );
	    }
	}
    } else {
	/* Bitmap */
	if ( bc1->xmin!=bc2->xmin || bc1->xmax!=bc2->xmax ||
		bc1->ymin!=bc2->ymin || bc1->ymax!=bc2->ymax )
return( BC_BoundingBoxMismatch|BC_NoMatch );

	xlen = bc1->xmax-bc1->xmin;
	mask = 0xff00>>((xlen&7)+1);
	xlen>>=3;
	for ( j=0; j<=bc1->ymax-bc1->ymin; ++j ) {
	    pt1 = bc1->bitmap+j*bc1->bytes_per_line;
	    pt2 = bc2->bitmap+j*bc2->bytes_per_line;
	    for ( i=xlen-1; i>=0; --i )
		if ( pt1[i]!=pt2[i] )
return( BC_NoMatch );
	    if ( (pt1[xlen]&mask)!=(pt2[xlen]&mask) )
return( BC_NoMatch );
	}
    }

return( BC_Match );
}

/* ************************************************************************** */
/* **************** Code to selected glyphs against clipboard *************** */
/* ************************************************************************** */
static int CompareLayer(Context *c, const SplineSet *ss1,const SplineSet *ss2,
	real pt_err, real spline_err, const char *name, int diffs_are_errors ) {
    int val;

    val = SSsCompare(ss1,ss2, pt_err, spline_err);
    if ( (val&SS_NoMatch) && diffs_are_errors ) {
	if ( val & SS_DiffContourCount )
	    ScriptErrorString(c,"Spline mismatch (different number of contours) in glyph", name);
	else if ( val & SS_MismatchOpenClosed )
	    ScriptErrorString(c,"Open/Closed contour mismatch in glyph", name);
	else
	    ScriptErrorString(c,"Spline mismatch in glyph", name);
    }
return( val );
}

static int RefCheck(Context *c, const RefChar *ref1,const RefChar *ref2,
	const char *name, int diffs_are_errors ) {
    const RefChar *r1, *r2;
    int i;

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
	else if ( diffs_are_errors )
	    ScriptErrorString(c,"Reference mismatch in glyph", name);
	else
return( false );
    }

    for ( r2 = ref2; r2!=NULL; r2=r2->next ) if ( !r2->checked ) {
	if ( diffs_are_errors )
	    ScriptErrorString(c,"Reference mismatch in glyph", name);
	else
return( false );
    }

return( true );
}

static int CompareBitmap(Context *c,SplineChar *sc,const Undoes *cur,
	real pixel_off_frac, int bb_err, int diffs_are_errors ) {
    int ret, err;
    BDFFont *bdf;
    BDFChar bc;

    for ( bdf=c->curfv->sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=cur->u.bmpstate.pixelsize || BDFDepth(bdf)!=cur->u.bmpstate.depth); bdf=bdf->next );
    if ( bdf==NULL || sc->orig_pos>=bdf->glyphcnt ||
	    bdf->glyphs[sc->orig_pos]==NULL )
	ScriptError(c,"Missing bitmap size" );
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
	    ScriptErrorF(c,"Bitmaps bounding boxes do not match in glyph %s pixelsize %d depth %d",
		    sc->name, bdf->pixelsize, BDFDepth(bdf));
	else if ( ret&SS_WidthMismatch )
	    ScriptErrorF(c,"Bitmaps advance widths do not match in glyph %s pixelsize %d depth %d",
		    sc->name, bdf->pixelsize, BDFDepth(bdf));
	else
	    ScriptErrorF(c,"Bitmap mismatch in glyph %s pixelsize %d depth %d",
		    sc->name, bdf->pixelsize, BDFDepth(bdf));
    }
return( ret );
}

static int CompareHints( Context *c,SplineChar *sc, const void *_test,
	real pt_err ) {
    StemInfo *h = sc->hstem, *v = sc->vstem;
    const StemInfo *test = _test;

    while ( test!=NULL ) {
	if ( test->hinttype == ht_h ) {
	    if ( h==NULL )
return( false );
	    if ( h->start!=test->start || h->width!=test->width )
return( false );
	    h = h->next;
	} else if ( test->hinttype == ht_v ) {
	    if ( v==NULL )
return( false );
	    if ( v->start!=test->start || v->width!=test->width )
return( false );
	    v = v->next;
	} else
return( false );
	test = test->next;
    }
    if ( h!=NULL || v!=NULL )
return( false );

return( true );
}

static int CompareSplines(Context *c,SplineChar *sc,const Undoes *cur,
	real pt_err, real spline_err, int diffs_are_errors ) {
    int ret=0, ly;
    const Undoes *layer;
    real err = pt_err>0 ? pt_err : spline_err;

    switch ( cur->undotype ) {
      case ut_state: case ut_statehint: case ut_statename:
	ret |= CompareLayer(c,sc->layers[ly_fore].splines,cur->u.state.splines,
		    pt_err, spline_err,sc->name, diffs_are_errors);
	if ( sc->width-cur->u.state.width>err || sc->width-cur->u.state.width<-err )
	    ret = SS_NoMatch|SS_WidthMismatch;
	if ( !RefCheck( c,sc->layers[ly_fore].refs,cur->u.state.refs, sc->name, diffs_are_errors ))
	    ret = SS_NoMatch|SS_RefMismatch;
	if ( cur->undotype==ut_statehint && pt_err>0 &&
		!CompareHints( c,sc,cur->u.state.u.hints,pt_err ))
	    ret = SS_NoMatch|SS_HintMismatch;
      break;
      case ut_layers:
	for ( ly=ly_fore, layer = cur->u.multiple.mult;
		ly<sc->layer_cnt && layer!=NULL;
		++ly, layer = cur->next ) {
	    ret |= CompareLayer(c,sc->layers[ly].splines,cur->u.state.splines,
			pt_err, spline_err,sc->name, diffs_are_errors);
	    if ( ly==ly_fore && (sc->width-cur->u.state.width>err || sc->width-cur->u.state.width<-err) )
		ret = SS_NoMatch|SS_WidthMismatch;
	    if ( !RefCheck( c,sc->layers[ly].refs,cur->u.state.refs, sc->name, diffs_are_errors ))
		ret = SS_NoMatch|SS_RefMismatch;
	    /* No hints in type3 fonts */
	}
	if ( ly!=sc->layer_cnt || layer!=NULL )
	    ScriptErrorString(c,"Layer difference in glyph", sc->name);
      break;
      default:
	ScriptError(c,"Unexpected clipboard contents");
    }
    if ( (ret&SS_WidthMismatch) && diffs_are_errors )
	ScriptErrorString(c,"Advance width mismatch in glyph", sc->name);
return( ret );
}

int CompareGlyphs(Context *c, real pt_err, real spline_err,
	real pixel_off_frac, int bb_err, int diffs_are_errors ) {
    FontView *fv = c->curfv;
    SplineFont *sf = fv->sf;
    int i, cnt=0;
    int ret=0;
    const Undoes *cur, *bmp;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] )
	++cnt;
    if ( cnt==0 )
	ScriptError(c,"Nothing selected");

    cur = CopyBufferGet();
    if ( cur->undotype==ut_noop || cur->undotype==ut_none )
	ScriptError(c,"Nothing in clipboard");

    if ( cur->undotype==ut_multiple )
	cur = cur->u.multiple.mult;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	SplineChar *sc = fv->map->map[i]==-1 ? NULL : sf->glyphs[ fv->map->map[i] ];

	if ( sc==NULL )
	    ScriptError(c,"Missing character");

	if ( cur==NULL )
	    ScriptError(c,"Too few glyphs in clipboard");

	switch ( cur->undotype ) {
	  case ut_state: case ut_statehint: case ut_statename:
	  case ut_layers:
	    if ( pt_err>=0 || spline_err>0 )
		ret |= CompareSplines(c,sc,cur,pt_err,spline_err,diffs_are_errors);
	  break;
	  case ut_bitmapsel: case ut_bitmap:
	    if ( pixel_off_frac>=0 )
		ret |= CompareBitmap(c,sc,cur,pixel_off_frac,bb_err,diffs_are_errors);
	  break;
	  case ut_composit:
	    if ( cur->u.composit.state!=NULL && ( pt_err>=0 || spline_err>0 ))
		ret |= CompareSplines(c,sc,cur->u.composit.state,pt_err,spline_err,diffs_are_errors);
	    if ( pixel_off_frac>=0 ) {
		for ( bmp=cur->u.composit.bitmaps; bmp!=NULL; bmp = bmp->next )
		    ret |= CompareBitmap(c,sc,bmp,pixel_off_frac,bb_err,diffs_are_errors);
	    }
	  break;
	  default:
	    ScriptError(c,"Unexpected clipboard contents");
	}
	if ( ret&(SS_NoMatch|SS_RefMismatch|SS_WidthMismatch|BC_NoMatch) ) {
	    ret &= ~(BC_Match|SS_PointsMatch|SS_ContourMatch);
return( ret );
	}
	cur = cur->next;
    }

    if ( cur!=NULL )
	ScriptError(c,"Too many glyphs in clipboard");
return( ret );
}
