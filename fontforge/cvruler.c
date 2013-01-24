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
#include "fontforgeui.h"
#include <math.h>
#include <ustring.h>
#include <assert.h>

#include "cvruler.h"

int measuretoolshowhorizontolvertical = true;
Color measuretoollinecol = 0x000000;
Color measuretoolpointcol = 0xFF0000;
Color measuretoolpointsnappedcol = 0x00FF00;
Color measuretoolcanvasnumberscol = 0xFF0000;
Color measuretoolcanvasnumberssnappedcol = 0x00FF00;
Color measuretoolwindowforegroundcol = 0x000000;
Color measuretoolwindowbackgroundcol = 0xe0e0c0;

BasePoint last_ruler_offset[2] = { {0,0}, {0,0} };

static void SlopeToBuf(char *buf, int buflen, const char *label, double dx, double dy) {
    if ( dx==0 && dy==0 )
	snprintf( buf, buflen, _("%s No Slope"), label );
    else if ( dx==0 )
	snprintf( buf, buflen, "%s dy/dx= ∞, %4g°", label, atan2(dy,dx)*180/M_PI);
    else
	snprintf( buf, buflen, "%s dy/dx= %4g, %4g°", label, dy/dx, atan2(dy,dx)*180/M_PI);
}

static int GetCurvature(CharView *cv, Spline *s, double t,double *curvature,double *radius) {
    double kappa, emsize;

    kappa = SplineCurvature(s,t);
    if ( kappa==CURVATURE_ERROR )
	return false;
    else {
	emsize = cv->b.sc->parent->ascent + cv->b.sc->parent->descent;
	if ( kappa==0 )
	    *radius = INFINITY;
	else
	    *radius = 1.0/kappa;
	*curvature = kappa*emsize;
    }
    return true;
}

static int GetSplineLength(Spline *s1,SplinePoint *sp1,double t1,Spline *s2,SplinePoint *sp2,double t2,double *l) {
    double len;

    if ( sp1!=NULL && sp2!=NULL &&
    ((sp1->next!=NULL && sp1->next->to==sp2) ||
     (sp1->prev!=NULL && sp1->prev->from==sp2)) ) {
	if ( sp1->next!=NULL && sp1->next->to==sp2 )
	    len = SplineLength(sp1->next);
	else
	    len = SplineLength(sp1->prev);
    } else if ( s1 == s2 && s2!=NULL )
	len = SplineLengthRange(s2,t1,t2);
    else if ( sp1!=NULL && s2!=NULL &&
	    sp1->next == s2 )
	len = SplineLengthRange(s2,0,t2);
    else if ( sp1!=NULL && s2!=NULL &&
	    sp1->prev == s2 )
	len = SplineLengthRange(s2,t2,1);
    else if ( sp2!=NULL && s1!=NULL &&
	    sp2->next == s1 )
	len = SplineLengthRange(s1,0,t1);
    else if ( sp2!=NULL && s1!=NULL &&
	    sp2->prev == s1 )
	len = SplineLengthRange(s1,t1,1);
    else
	return false;

    *l = len;
    return true;
}

static void CheckFont(CharView *cv)
{
    static GFont *rvfont = NULL;

    // TBD, correct place to do font.
    if ( rvfont==NULL ) {
	FontRequest rq;

        memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = FIXED_UI_FAMILIES;
	rq.point_size = -12;
	rq.weight = 400;

	/*
	 * This assumes that font will not actually depend on the CharView,
	 * that is just used in set-up.
	 */
	// rvfont = GDrawInstanciateFont(cv->ruler_w,&rq);
	rvfont = GDrawInstanciateFont(cv->v,&rq);
	rvfont = GResourceFindFont("CharView.Measure.Font",rvfont);
    }

    if ( cv->rfont==NULL ) {
	int as, ds, ld;

	GDrawWindowFontMetrics(cv->v,rvfont,&as,&ds,&ld);
	cv->rfh = as+ds; cv->ras = as;
	cv->rfont = rvfont;
    }
}

/*
 * Comparison function for use with qsort.
 */
static int BasePointCompare(const void *_l, const void *_r) {
	const BasePoint *l = _l, *r = _r;
	if ( l->x>r->x)
return( 1 );
	if ( l->x<r->x)
return( -1 );
	if ( l->y>r->y)
return( 1 );
	if ( l->y<r->y)
return( -1 );
return( 0 );
}

static int ReverseBasePointCompare(const void *l, const void *r) {
return( -BasePointCompare(l,r) );
}

static int SpExists(CharView *cv, SplinePoint *sp) {
    SplineSet *spl;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	if ( SpExistsInSS(sp,spl) )
return( true );
    }
return( false );
}

/*
 * Fill buffer with intersects on a line (from,to).
 * return number found, buf fill the buffer only up to a max_intersections.
 *
 * The points from and to are also put in the buffer.
 *
 * Copied somewhat from CVMouseUpKnife(), perhaps they should be consolidated
 */
static int GetIntersections(CharView *cv,BasePoint from,BasePoint to,BasePoint *all_intersections,int max_intersections) {
    SplineSet *spl;
    Spline *s, *nexts;
    Spline dummy;
    SplinePoint dummyfrom, dummyto;
    BasePoint inters[9];	/* These bounds are hard coded in the SplinesIntersect function */
    extended t1s[10], t2s[10];
    int i;
    int total_intersections = 0;

    memset(&dummy,0,sizeof(dummy));
    memset(&dummyfrom,0,sizeof(dummyfrom));
    memset(&dummyto,0,sizeof(dummyto));
    dummyfrom.me.x = from.x; dummyfrom.me.y = from.y;
    dummyto.me.x = to.x; dummyto.me.y = to.y;
    dummyfrom.nextcp = dummyfrom.prevcp = dummyfrom.me;
    dummyto.nextcp = dummyto.prevcp = dummyto.me;
    dummyfrom.nonextcp = dummyfrom.noprevcp = dummyto.nonextcp = dummyto.noprevcp = true;
    dummy.splines[0].d = from.x; dummy.splines[0].c = to.x-from.x;
    dummy.splines[1].d = from.y; dummy.splines[1].c = to.y-from.y;
    dummy.from = &dummyfrom; dummy.to = &dummyto;
    dummy.islinear = dummy.knownlinear = true;
    dummyfrom.next = dummyto.prev = &dummy;

    all_intersections[total_intersections++] = from;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	for ( s = spl->first->next; s!=NULL ; ) {
	    nexts = NULL;
	    if ( s->to!=spl->first )
		nexts = s->to->next;

	    if ( SplinesIntersect(s,&dummy,inters,t1s,t2s)>0 ) {
		/* TBD, why is 4 used here (copied from CVMouseUpKnife()) */
		for ( i=0; i<4 && t1s[i]!=-1; ++i ) {
		    if ( (total_intersections+1)<max_intersections )
			all_intersections[total_intersections] = inters[i];
		    total_intersections++;
		}
	    }
	    s = nexts;
	}
    }

    if ( total_intersections<max_intersections )
	all_intersections[total_intersections++] = to;

    qsort(all_intersections,
	total_intersections > max_intersections ? max_intersections : total_intersections,
	sizeof(all_intersections[0]),
	BasePointCompare(&from,&to)<=0 ? BasePointCompare : ReverseBasePointCompare );

    /*
     * Filter out intersectsions that are too close.
     * This is for snapped points, but we get more than one extra per snap,
     * so do them all for now.
     */
    for ( i = 1 ; i<total_intersections && i<max_intersections ; ) {
	if ( (0.00001 > fabs(all_intersections[i].x-all_intersections[i-1].x)) &&
	     (0.00001 > fabs(all_intersections[i].y-all_intersections[i-1].y)) ) {
	    int j;

	    for( j = i+1 ; j<total_intersections &&  j<max_intersections ; j++ )
		all_intersections[j-1] = all_intersections[j];
	    if ( total_intersections < max_intersections )
		total_intersections--;
	} else {
	    i++;
	}
    }

return( total_intersections );	/* note that it could be greater than max */
}

static void UpdateIntersections(CharView *cv,BasePoint from,BasePoint to)
{
    if ( !cv->ruler_intersections ) {
	cv->allocated_ruler_intersections = 32;
	cv->ruler_intersections = galloc(cv->allocated_ruler_intersections * sizeof(cv->ruler_intersections[0]));
    }
    for(;;) {
	cv->num_ruler_intersections = GetIntersections(cv,from,to,cv->ruler_intersections,cv->allocated_ruler_intersections);
	if ( cv->num_ruler_intersections>cv->allocated_ruler_intersections ) {
	    cv->allocated_ruler_intersections = cv->num_ruler_intersections * 2;
	    cv->ruler_intersections = grealloc(cv->ruler_intersections,cv->allocated_ruler_intersections * sizeof(cv->ruler_intersections[0]));
	} else
	    break;
    }
}

static void RulerPlace(CharView *cv, GEvent *event) {
    // TBD, correct place to do font.
    CheckFont(cv);

    if ( cv->p.pressed ) {
	BasePoint from;

	from.x = cv->p.cx;
	from.y = cv->p.cy;

	if ( cv->p.sp )
	    cv->start_intersection_snapped = cv->p.sp;
	else
	    cv->start_intersection_snapped = 0;

	if ( cv->info_sp )
	    cv->end_intersection_snapped = cv->info_sp;
	else
	    cv->end_intersection_snapped = 0;

	UpdateIntersections(cv,from,cv->info);
    }
}

void CVMouseDownRuler(CharView *cv, GEvent *event) {
    cv->autonomous_ruler_w = false;
    RulerPlace(cv,event);
    cv->p.rubberlining = true;
}

void CVMouseMoveRuler(CharView *cv, GEvent *event) {

    if ( cv->autonomous_ruler_w )
return;

    if ( !cv->p.pressed && (event->u.mouse.state&ksm_alt) ) {
return;
    }
    if ( !cv->p.pressed )
	CVMouseAtSpline(cv,event);
    RulerPlace(cv,event);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);		/* The resize needs to happen before the expose */
    if ( !cv->p.pressed && (event->u.mouse.state&ksm_alt) ) /* but a mouse up might sneak in... */
return;
    GDrawRequestExpose(cv->v,NULL,false);
}

void CVMouseUpRuler(CharView *cv, GEvent *event) {
    // TBD, what is this really for?
    last_ruler_offset[1] = last_ruler_offset[0];
    last_ruler_offset[0].x = cv->info.x-cv->p.cx;
    last_ruler_offset[0].y = cv->info.y-cv->p.cy;
}

void CPStartInfo(CharView *cv, GEvent *event) {
    if ( !cv->showcpinfo )
return;

    cv->autonomous_ruler_w = false;
}

void CPUpdateInfo(CharView *cv, GEvent *event) {
}

void CPEndInfo(CharView *cv) {
}

void CVRulerExpose(GWindow pixmap,CharView *cv) {

    // TBD, correct place to do font.
    CheckFont(cv);

    if ( cv->num_ruler_intersections >= 2 ) {
	BasePoint to,from;
	from = cv->ruler_intersections[0];
	to = cv->ruler_intersections[cv->num_ruler_intersections-1];

	// In case things have moved. TBD: handles removal of points, but what about re-use?
	// Snapped points are followed, but no new snapping occurs, that is left just for user ruler placement.
	if ( cv->start_intersection_snapped ) {
	    if ( SpExists( cv, cv->start_intersection_snapped ) )
		from = cv->start_intersection_snapped->me;
	    else
		cv->start_intersection_snapped = 0;
	}
	if ( cv->end_intersection_snapped ) {
	    if ( SpExists( cv, cv->end_intersection_snapped ) )
		to = cv->end_intersection_snapped->me;
	    else
		cv->end_intersection_snapped = 0;
	}
	UpdateIntersections(cv,from,to);
    }

    if ( cv->num_ruler_intersections >= 2 ) {
	int x =  cv->xoff + rint(cv->ruler_intersections[0].x*cv->scale);
	int y = -cv->yoff + cv->height - rint(cv->ruler_intersections[0].y*cv->scale);
	int xend =  cv->xoff + rint(cv->ruler_intersections[cv->num_ruler_intersections-1].x*cv->scale);
	int yend = -cv->yoff + cv->height - rint(cv->ruler_intersections[cv->num_ruler_intersections-1].y*cv->scale);
	real xdist = fabs(cv->ruler_intersections[0].x - cv->ruler_intersections[cv->num_ruler_intersections-1].x);
	real ydist = fabs(cv->ruler_intersections[0].y - cv->ruler_intersections[cv->num_ruler_intersections-1].y);
	int i;
	int len;
	int charwidth = 6; /* TBD */
	Color textcolor = (cv->start_intersection_snapped && cv->end_intersection_snapped) ? measuretoolcanvasnumberssnappedcol : measuretoolcanvasnumberscol;

	if ( measuretoolshowhorizontolvertical ) {
	    char buf[40];
	    unichar_t ubuf[40];

	    if ( xdist*cv->scale>10.0 && ydist*cv->scale>10.0 ) {

		assert(cv->rfont);

		GDrawSetFont(pixmap,cv->rfont);
		len = snprintf(buf,sizeof buf,"%g",xdist);
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(pixmap,(x+xend)/2 - len*charwidth/2,y + (y > yend ? 12 : -5),ubuf,-1,textcolor);
		GDrawDrawLine(pixmap,x,y,xend,y,measuretoollinecol);

		len = snprintf(buf,sizeof buf,"%g",ydist);
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(pixmap,xend + (x < xend ? charwidth/2 : -(len * charwidth + charwidth/2)),(y+yend)/2,ubuf,-1,textcolor);
		GDrawDrawLine(pixmap,xend,y,xend,yend,measuretoollinecol);
	    }
	}

	if ( !cv->p.rubberlining ) {
	    GDrawDrawLine(pixmap,x,y,xend,yend,measuretoollinecol);
	}

	GDrawSetFont(pixmap,cv->rfont);
	for ( i=0 ; i<cv->num_ruler_intersections; ++i ) {
	    GRect rect,prev_rect;

	    rect.x = cv->xoff + rint(cv->ruler_intersections[i].x*cv->scale) - 1;
	    rect.y = -cv->yoff + cv->height - rint(cv->ruler_intersections[i].y*cv->scale) - 1;
	    rect.width = 3;
	    rect.height = 3;

	    GDrawFillElipse(pixmap,&rect,((i==(cv->num_ruler_intersections-1) && cv->end_intersection_snapped) || (i==0 && cv->start_intersection_snapped)) ? measuretoolpointsnappedcol : measuretoolpointcol);
	    if ( i>0 && (cv->num_ruler_intersections<6 || (prev_rect.x + 10)<rect.x || (prev_rect.y + 10)<rect.y || (prev_rect.y - 10)>rect.y) ) {
		real xoff = cv->ruler_intersections[i].x - cv->ruler_intersections[i-1].x;
		real yoff = cv->ruler_intersections[i].y - cv->ruler_intersections[i-1].y;
		real len = sqrt(xoff*xoff+yoff*yoff);
		char buf[40];
		unichar_t ubuf[40];
		int x,y;

		x = (prev_rect.x + rect.x)/2;
		y = (prev_rect.y + rect.y)/2;

		len = snprintf(buf,sizeof buf,"%g",len);
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(pixmap,x + (x < xend ? -(len*charwidth) : charwidth/2 ),y + (y < yend ? 12 : -5),ubuf,-1,textcolor);
	    }
	    prev_rect = rect;
	}
    }

    if ( cv->rv && GDrawIsVisible(cv->rv->gw)) {
	GDrawRequestExpose(cv->rv->gw,NULL,false);
    }
}

static int rv_e_h(GWindow gw, GEvent *event) {
    MeasureToolView *rv = (MeasureToolView *) GDrawGetUserData(gw);
    int i,line;
    unichar_t ubuf[128];

    // TBD, some real formating
    int point_start_line = 0;
    int point_start_x = 2;
    int intersection_start_line = 12;
    int intersection_start_x = 2;

    switch ( event->type ) {
      case et_expose:
      case et_visibility:

	assert(rv->cv);
	assert(rv->cv->rfont);

	GDrawSetFont(gw,rv->cv->rfont);
	GDrawClear(gw,NULL);

	if (1) {
	    char buf[180];
	    Spline *s = 0;
	    SplinePoint *sp = 0;
	    double t = 0.0;

	    line = point_start_line;

	    /* Give current location accurately */
	    snprintf( buf, sizeof buf, "%s: %f %f",
		_("Position"),
		(double) rv->cv->info.x, (double) rv->cv->info.y
		);
	    utf82u_strncpy(ubuf,buf,sizeof ubuf);
	    GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
	    line++;

	    /* If the button is pressed, "info" tracks the position after the press */
	    /* if a conptrol point is selected, we are interesting in the spline point */
	    if (rv->cv->p.prevcp || rv->cv->p.nextcp) {
		s = 0;
		t = 0;
		sp = rv->cv->p.sp;
	    } else if (rv->cv->p.pressed) {
		s = rv->cv->info_spline;
		t = rv->cv->info_t;
		sp = rv->cv->info_sp;
	    } else {
		s = rv->cv->p.spline;
		t = rv->cv->p.t;
		sp = rv->cv->p.sp;
	    }

	    if (s) {
		snprintf( buf, sizeof buf, "%s: %f %f",
		    _("Near"),
		    (double) (((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d),
		    (double) (((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d) );
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;
		line++;
		line++;
	    } else if (sp) {
		char cpn[64];
		char cpp[64];

		if (!sp->nonextcp) {
		    snprintf(cpn,sizeof cpn,"  %s: %f %f ∆ (%g,%g)",
			_("Next CP"),
			sp->nextcp.x, sp->nextcp.y,
			sp->nextcp.x-sp->me.x, sp->nextcp.y-sp->me.y
			);
		} else {
		    snprintf(cpn,sizeof cpn,"  %s: none", _("Next CP") );
		}
		if (!sp->noprevcp) {
		    snprintf(cpp,sizeof cpp,"  %s: %f %f ∆ (%g,%g)",
			_("Prev CP"),
			sp->prevcp.x, sp->prevcp.y,
			sp->prevcp.x-sp->me.x, sp->prevcp.y-sp->me.y
			);
		} else {
		    snprintf(cpp,sizeof cpp,"  %s: none", _("Prev CP") );
		}

		snprintf( buf, sizeof buf, "%s: %f %f",
		    (rv->cv->p.prevcp || rv->cv->p.nextcp) ? _("Controlling") : _("Near"),
		    (double) sp->me.x,
		    (double) sp->me.y
		    );

		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		utf82u_strncpy(ubuf,cpp,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		utf82u_strncpy(ubuf,cpn,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

	    } else {
		snprintf( buf, sizeof buf, "%s: %s",
		    _("Near"),
		    _("none") );
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;
		line++;
		line++;
	    }

	    if (s) {
		double dx = (3*s->splines[0].a*t+2*s->splines[0].b)*t+s->splines[0].c;
		double dy = (3*s->splines[1].a*t+2*s->splines[1].b)*t+s->splines[1].c;
		SlopeToBuf(buf,sizeof buf,_("Slope:"),dx,dy);
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;
		line++;
		line++;
	    } else if (sp) {
		// TBD
		char buf_n[64];
		char buf_p[64];

	        // TBD for continous curves there is really only one slope

		if (sp->next) {
		    double dx = sp->next->splines[0].c;
		    double dy = sp->next->splines[1].c;
		    SlopeToBuf(buf_n,sizeof buf_n,"",dx,dy);
		} else {
		    snprintf(buf_n,sizeof buf_n,"");
		}
		if (sp->prev) {
		    double dx = (3*sp->prev->splines[0].a*1+2*sp->prev->splines[0].b)*1+sp->prev->splines[0].c;
		    double dy = (3*sp->prev->splines[1].a*1+2*sp->prev->splines[1].b)*1+sp->prev->splines[1].c;
		    SlopeToBuf(buf_p,sizeof buf_p,"",dx,dy);
		} else {
		    snprintf(buf_p,sizeof buf_p,"");
		}

		snprintf( buf, sizeof buf, "%s:",_("Slope"));
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		snprintf( buf, sizeof buf, "  %s:%s",_("Prev"),buf_p);
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		snprintf( buf, sizeof buf, "  %s:%s",_("Next"),buf_n);
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

	    } else {
		snprintf( buf, sizeof buf, _("%s: %s"),
		    _("Slope"),
		    _("none")
		    );
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;
		line++;
		line++;
	    }

	    if (s) {
		double curvature;
		double radius;

		if (GetCurvature(rv->cv,s,t,&curvature,&radius)) {
		    snprintf(buf,sizeof buf,"%s: %g %s: %g",
			_("Curvature"),
			curvature,
			_("Radius"),
			radius);
		} else {
		    snprintf(buf,sizeof buf,"%s: %s",
			_("Curvature"),
			_("bad"));
		}
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;
		line++;
		line++;
	    } else if (sp) {
		double ncurvature;
		double nradius;
		double pcurvature;
		double pradius;
		int gotn = 0;
		int gotp = 0;

		if (sp->next) {
		    gotn = GetCurvature(rv->cv,sp->next,0,&ncurvature,&nradius);
		}
		if (sp->prev) {
		    gotp = GetCurvature(rv->cv,sp->prev,1,&pcurvature,&pradius);
		}

		// TBD, we really should be able to get a single curvature at continous spline points, and to get angles discontinous ones

		snprintf(buf,sizeof buf,"%s:", _("Curvature") );
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		if (gotp) {
		    snprintf(buf,sizeof buf,"  %s: %g %s: %g",
			_("Prev"),
			pcurvature,
			_("Radius"),
			pradius);
		} else {
		    snprintf(buf,sizeof buf,"  %s: %s",_("Prev"),_("none"));
		}
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		if (gotn) {
		    snprintf(buf,sizeof buf,"  %s: %g %s: %g",
			_("Next"),
			ncurvature,
			_("Radius"),
			nradius);
		} else {
		    snprintf(buf,sizeof buf,"  %s: %s",_("Next"),_("none"));
		}
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		if (gotn && gotp) {
		    snprintf(buf,sizeof buf,"  %s: %g",
			U_("∆Curvature"),
			ncurvature - pcurvature);
		    utf82u_strncpy(ubuf,buf,sizeof ubuf);
		    GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		    line++;
		}
	    } else {
		snprintf(buf,sizeof buf,"%s: %s",
		    _("Curvature"),
		    _("none"));
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,point_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;
	    }
	    line++;
	}

	line = intersection_start_line;
	if (1) {

	    if (rv->cv->num_ruler_intersections > 1) {
		char buf[80];
		real xoff = 0.0;
		real yoff = 0.0;
		real len = 0.0;
		double slen;
		char sbuf[80];

		// TBD, length along spline, fill in zerowed arguments
		if (GetSplineLength(0,rv->cv->start_intersection_snapped,0.0,0,rv->cv->end_intersection_snapped,0.0,&slen)) {
		    snprintf( sbuf, sizeof sbuf, "%s: %f",
			_("Spline"),
			slen);
		} else {
		    snprintf( sbuf, sizeof sbuf, "");
		}

		xoff = rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-1].x - rv->cv->ruler_intersections[0].x;
		yoff = rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-1].y - rv->cv->ruler_intersections[0].y;
		len = sqrt(xoff*xoff+yoff*yoff);

		snprintf( buf, sizeof buf, "%s: %f %.0f°  X %f Y %f %s",
		    _("Ruler length"), (double) len,
		    atan2(yoff,xoff)*180/M_PI,
		    (double) xoff,(double) yoff,
		    sbuf
		    );
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,intersection_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		utf82u_strncpy(ubuf,_("Ruler Intersections/Segments:"),sizeof ubuf);
		GDrawDrawText(gw,intersection_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		snprintf(buf,sizeof buf,"%5s %8s %8s %8s %8s %8s",
			"#",
			_("End-X"),
			_("End-Y"),
			_("Length-X"),
			_("Length-Y"),
			_("Length")
			);
		utf82u_strncpy(ubuf,buf,sizeof ubuf);
		GDrawDrawText(gw,intersection_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		line++;

		for ( i=0; i < rv->cv->num_ruler_intersections ; ++i,++line ) {
		    const char *state = "";
		    char buf2[20];

		    if (i > 0) {
			xoff = rv->cv->ruler_intersections[i].x - rv->cv->ruler_intersections[i-1].x;
			yoff = rv->cv->ruler_intersections[i].y - rv->cv->ruler_intersections[i-1].y;
			len = sqrt(xoff*xoff+yoff*yoff);
		    } else {
			xoff = 0.0;
			yoff = 0.0;
			len = 0.0;
		    }

		    if (i == 0 &&  rv->cv->start_intersection_snapped) state = "snapped";
		    else if (i == (rv->cv->num_ruler_intersections-1) &&  rv->cv->end_intersection_snapped) state = "snapped";

		    if (i) snprintf(buf2,sizeof buf2,"%d",i);
		    else snprintf(buf2,sizeof buf2,_("start"));
		    snprintf(buf,sizeof buf,"%5s %8g %8g %8g %8g %8g %s",
			buf2,
			rv->cv->ruler_intersections[i].x,
			rv->cv->ruler_intersections[i].y,
			fabs(xoff),
			fabs(yoff),
			len,
			state);

		    utf82u_strncpy(ubuf,buf,sizeof ubuf);

		    GDrawDrawText(gw,intersection_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
		}

		if (rv->cv->num_ruler_intersections > 4) {
		    line++;

		    // TBD, the meaning is problematic.
		    // We currently on start with crossing intersections, but touching, including snapping could be more valid start and ends.
		    real xoff = rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-2].x - rv->cv->ruler_intersections[1].x;
		    real yoff = rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-2].y - rv->cv->ruler_intersections[1].y;
		    real len = sqrt(xoff*xoff+yoff*yoff);
		    char buf2[20];
		    snprintf(buf2,sizeof buf2,"1-%d",rv->cv->num_ruler_intersections-2);
		    snprintf(buf,sizeof buf,"%5s %8g %8g %8g %8g %8g %s",
			buf2,
			rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-2].x,
			rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-2].y,
			fabs(xoff),
			fabs(yoff),
			len,
			""
			);
		    utf82u_strncpy(ubuf,buf,sizeof ubuf);
	            GDrawDrawText(gw,intersection_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
	            line++;
		}

		if (rv->cv->num_ruler_intersections > 2) {
	            line++;

		    real xoff = rv->cv->ruler_intersections[0].x - rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-1].x;
		    real yoff = rv->cv->ruler_intersections[0].y - rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-1].y;
		    real len = sqrt(xoff*xoff+yoff*yoff);

		    snprintf(buf,sizeof buf,"%5s %8g %8g %8g %8g %8g %s",
		        _("Total"),
		        rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-1].x,
		        rv->cv->ruler_intersections[rv->cv->num_ruler_intersections-1].y,
		        fabs(xoff),
		        fabs(yoff),
		        len,
		        (rv->cv->start_intersection_snapped && rv->cv->end_intersection_snapped) ? _("snapped") : ""
		        );

	            utf82u_strncpy(ubuf,buf,sizeof ubuf);
	            GDrawDrawText(gw,intersection_start_x,line*rv->cv->rfh+rv->cv->ras+1,ubuf,-1,measuretoolwindowforegroundcol);
	            line++;
	        }
	    }
	}

      break;
      case et_resize:
      break;
      case et_close:
	GDrawDestroyWindow(rv->gw);
      break;
      case et_destroy:
	rv->gw = 0;
      break;
      default:
	// printf("rv_e_h Unhandled Event: %d\n",event->type);
      break;
    }
return( true );
}

MeasureToolView *MeasureToolViewCreate(CharView *cv) {
    GRect pos;
    GWindowAttrs wattrs;
    char buf[120];

    if (cv->rv && cv->rv->gw) {
	GDrawRaise(cv->rv->gw);
return( cv->rv );
    }

    if (!cv->rv) {
	cv->rv = gcalloc(1,sizeof(MeasureToolView));
	cv->rv->cv = cv;
    }

    pos.x = 0;
    pos.y = 0;
    pos.width = 600;
    pos.height = 400;

    snprintf(buf,sizeof buf,_("Measure Tool Metrics for %.80s from %.90s"),
	cv->b.sc->name, cv->b.sc->parent->fontname);

    memset(&wattrs,0,sizeof(wattrs));
    // wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_icon;
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle;
    wattrs.event_masks = ~0;
    wattrs.utf8_window_title = buf;

    cv->rv->gw = GDrawCreateTopWindow(NULL,&pos,rv_e_h,cv->rv,&wattrs);

    GDrawSetWindowTypeName(cv->rv->gw, "RulerMetricsView");

    GDrawSetVisible(cv->rv->gw,true);

return( cv->rv );
}

void MeasureToolViewFree(MeasureToolView *rv) {

    assert(rv);

    if (rv->gw) {
	GDrawDestroyWindow(rv->gw);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
    }
    rv->cv->rv = 0;
    free(rv);
}
