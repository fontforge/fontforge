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

#include "splinestroke.h"

#include "cvundoes.h"
#include "fontforge.h"
#include "splinefont.h"
#include "splineorder2.h"
#include "splineoverlap.h"
#include <math.h>
#define PI      3.1415926535897932

typedef struct strokepoint {
    Spline *sp;
    bigreal t;
    BasePoint me;		/* This lies on the original curve */
    BasePoint slope;		/* Slope at that point */
    BasePoint left;
    BasePoint right;
    /* double radius_of_curvature; */
    unsigned int butt_bevel: 1;		/* the butt line cap and bevel line join always produce an edge which will be hidden by the pen. We don't want that to happen so must special case them. */
    unsigned int left_hidden: 1;
    unsigned int right_hidden: 1;
    unsigned int hide_left_if_on_edge: 1;
    unsigned int hide_right_if_on_edge: 1;
    unsigned int extemum: 1;
    unsigned int circle: 1;		/* On a cap or a join, point lies on a circular arc centered at me. So slope is the normal to (left-me) */
    unsigned int line: 1;		/* On a cap, join or place where slope paralel to poly edge, point lies on a line can find slope by vector (pos,pos-1) or (pos+1,pos) */
    unsigned int needs_point_left: 1;	/* Contour changes direction abruptly on the left side */
    unsigned int needs_point_right: 1;
    uint8 lt;
    uint8 rt;
} StrokePoint;

/* Square line caps and miter line joins cover more area than the circular pen*/
/*  So we need a list of the polygons which represent these guys so we can do */
/*  an additional hit test on them. */
/* I'm not sure that these will always be presented to us in a clockwise fashion*/
/*  We can test that by taking a point which is the average of the vertices */
/*  and seeing if the hit-test says it is inside (It will be inside because */
/*  these polies are convex). If the hit test fails then the poly must be */
/*  drawn counter clockwise */
struct extrapoly {
    BasePoint poly[4];
    int ptcnt;		/* 3 (for miters) or 4 (for square caps) */
};

enum pentype { pt_circle, pt_square, pt_poly };

typedef struct strokecontext {
    enum pentype pentype;
    int cur, max;
    StrokePoint *all;
    struct extrapoly *ep;
    int ecur, emax;
    TPoint *tpt;
    int tmax;
    bigreal resolution;	/* take samples roughly this many em-units */ /* radius/16.0? */
    bigreal radius;
    bigreal radius2;	/* Squared */
    enum linejoin join;	/* Only for circles */
    enum linecap cap;	/* Only for circles */
    bigreal miterlimit;	/* Only for circles if join==lj_miter */
	    /* PostScript uses 1/sin( theta/2 ) as their miterlimit */
	    /*  I use -cos(theta). (where theta is the angle between the slopes*/
	    /*  same idea, different implementation */
    int n;		/* For polygon pens */
    BasePoint *corners;	/* Expressed as an offset from the center of the poly */ /* (Where center could mean center of bounding box, or average or all verteces) */
    BasePoint *slopes;	/* slope[0] is unitvector corners[1]-corners[0] */
    bigreal largest_distance2;	/* The greatest distance from center of poly to a corner */ /* Used to speed up hit tests */
    bigreal longest_edge;
    unsigned int open: 1;	/* Original is an open contour */
    unsigned int remove_inner: 1;
    unsigned int remove_outer: 1;
    /* unsigned int rotate_relative_to_direction: 1; */	/* Rotate the polygon pen so it maintains the same orientation with respect to the contour's slope */
    /* Um, the above is essentially equivalent to a circular pen. No point in duplicating it */
    unsigned int leave_users_center: 1;			/* Don't move the pen so its center is at the origin */
    unsigned int scaled_or_rotated: 1;
    unsigned int transform_needed: 1;
    real transform[6];
    real inverse[6];
} StrokeContext;

static char *glyphname=NULL;

/* Basically the idea is we find the spline, and then at each point, project */
/*  out normal to the current slope and find a point that is radius units away*/
/*  Now that's all well and good but we've also got to handle linecaps and joins */
/*  So we also generate point data for them */
/* Then we check and see if any points are closer to any other baseline points*/
/*  than radius. Any that are get ignored (they will be internal to the       */
/*  resultant shape. (linecaps can also get hidden -- not sure about joints)  */
/* Then (I hope) the points surrounding any hidden points should be very close*/
/*  together. Put break points where there were spline breaks in the original */
/*  and where there is a significant hidden area, and at certain places within*/
/*  caps and joins */

/* Butt line caps and bevel line joins cause problems because they are going */
/*  be inside (hidden) by a circle centered on the line end or joint. So we */
/*  can't do the hit test on these edges using any point on the contour which */
/*  is within radius of the joint. */
/* Hmmm. That works for the edge itself, but if some other part of the contour*/
/*  approaches closely (within radius) to the non-existant area it will be */
/*  incorrectly hidden. BUG!!!!! */
/* Square line caps and miter line joins have the opposite problem: They should*/
/*  hide more points than a circular pen will cover. So we can build up a list*/
/*  of the polygons which represent these extrusions (rectangle for the cap, */
/*  and triangle for the miter) and do a polygon hit test with those. */

/* OK, that's a circular pen. Now by scaling we can turn a circle into an */
/*  elipse, and by rotating we can orient the main axis how we like */
/*  (we actually scale the splines by a transform that turns the ellipse into */
/*   a circle, do the stroking with a circle, and then do the inverse trans-  */
/*   form (on the splines) that turns the circle into the ellipse) */

/* We could also get away with the radius field, and scale the splines by     */
/*  1/radius, and then inverse scale by radius. That way we'd always stroke   */
/*  a unit circle. I think that is not a good idea as there are routines where*/
/*  we assume a 1em error is acceptable -- but not if scaled up to radius     */
/*  later */

/* Fine. But we also want to deal with the caligraphic pen, which is a rectangle */
/*  tilted at 45degrees (or something like that). Well we do the same rotation*/
/*  scaling process we can turn the rectangle into a square oriented with the */
/*  coordinate axes. So how do we stroke a square pen? */

/* Similar, but not quite the same. We don't generate points that are normal  */
/*  to the spline, instead the stroked edge will be the drawn by one of the   */
/*  corners of the square (actually two corners, one on the left, one on the  */
/*  right of the spline), and which corner will depend on the slope of the    */
/*  spline. We will change the corner every time the spline becomes parallel  */
/*  to the square's edges -- that is at the extrema of the spline. When that  */
/*  happens we will have to draw a set of points that represent the side of   */
/*  the square */
/* Same rigamarole for caps and joins (except that these are determined by a  */
/*  square, not a circle, so the shapes will be bumpy not rounded). */
/* Then we have to do the hit test thing to find which points get hidden. Well*/
/*  hit test on a square is pretty easy. If the square is centered at (cx,cy) */
/*  and the square's edges are 2*r, then a point (x,y) is within the square if*/
/*  (x>=cx-r && x<cx+r && y>=cy-r && y<=cy+r) */

/* Well that was easy enough. Can we generalize? A regular polygon: the       */
/*  stroked edges will be dragged out one of the polygon's corners. Which one */
/*  will be determined by the spline's slope. When the spline moves parallel  */
/*  to one of the edges then we will switch corners and have to throw in a set*/
/*  of points as long as that edge. */
/* Note for a regulare n-gon where n even then the left and right sides will  */
/*  change corners at the same time (because their edges are parallel when    */
/*  mirrored across the spline). This is not true of n-gons where n is odd.   */
/*  (So in a regular triangle with one edge flat at the bottom, then we will  */
/*   only switch corners when the spline achieves a minimum in y, not a max)  */
/* Caps and Joins, now we have a polygon so it's even bumpier */
/* Hit test, more work but doable. There are two circles of interest to us,   */
/*  one which is the biggest circle that can be drawn within the polygon, and */
/*  the other, the smallest circle that can be drawn outside the poly. If a   */
/*  point is within the inscribed circle then it is in the poly. If a point is*/
/*  outside the exscribed circle then it is out of the poly. Between those two*/
/*  we have to do some math */

/* Actually, I think any convex polygon could be used with the above algorithm*/
/* The difficulty would be figuring out a good way of inputting the shape of  */
/* the pen (and testing that it wasn't concave). */
/* For a concave poly, the concavities are only relevant at the joins and caps*/
/* (otherwise the concavity will be filled as the pen moves) so we could      */
/* simply use the smallest convex poly that contains the concave one, and just */
/* worry about the concavity at the caps and joins (and maybe in doing the hit*/
/* test) But I don't think there is much point in dealing with that, and there*/
/* are the same issues on describing the shape as before */

static int AdjustedSplineLength(Spline *s) {
    /* I find the spline length in hopes of subdividing the spline into chunks*/
    /*  roughly 1em unit apart. But if the spline bends a lot, then when      */
    /*  expanded out by a pen the distance between chunks will be amplified   */
    /*  So do something quick and dirty to adjust for that */
    /* What I really want to do is to split the spline into segments, find */
    /*  the radius of curvature, find the length, multiply length by       */
    /*  (radius-curvature+c->radius)/radius-curvature                      */
    /*  For each segment and them sum that. But that's too hard */
    bigreal len = SplineLength(s);
    bigreal xdiff=s->to->me.x-s->from->me.x, ydiff=s->to->me.y-s->from->me.y;
    bigreal distance = sqrt(xdiff*xdiff+ydiff*ydiff);

    if ( len<=distance )
return( len );			/* It's a straight line */
    len += 1.5*(len-distance);
return( len );
}

enum hittest { ht_Outside, ht_Inside, ht_OnEdge };

static enum hittest PolygonHitTest(BasePoint *poly,BasePoint *polyslopes, int n,BasePoint *test, bigreal *distance) {
    /* If the poly is drawn clockwise, then a point is inside the poly if it */
    /*  is on the right side of each edge */
    /* A point lies on an edge if the dot product of:		  */
    /*  1. the vector normal to the edge			  */
    /*  2. the vector from either vertex to the point in question */
    /* is 0 (and it is otherwise inside)			  */
    /* Now, if we choose the normal vector which points right, then */
    /*  the dot product must be positive.			    */
    /* Note we could modify this code to work with counter-clockwise*/
    /*  polies. Either all the dots must be positive, or all must be*/
    /*  negative */
    /* Sigh. Rounding errors. dot may be off by a very small amount */
    /*  (the polyslopes need to be unit vectors for this test to work) */
    int i, zero_cnt=0, outside=false;
    bigreal dx,dy, dot, bestd= 0;

    for ( i=0; i<n; ++i ) {

	dx = test->x    - poly[i].x;
	dy = test->y    - poly[i].y;
	dot = dx*polyslopes[i].y - dy*polyslopes[i].x;
	if ( dot>=-0.001 && dot<=.001 )
	    ++zero_cnt;
	else if ( dot<0 ) {	/* It's on the left, so it can't be inside */
	    if ( distance==NULL )
return( ht_Outside );
	    outside= true;
	    if ( bestd<-dot )
		bestd = -dot;
	}
    }

    if ( outside ) {
	*distance = bestd;
return( ht_Outside );
    }

    if ( distance!=NULL )
	*distance = 0;
    /* zero_cnt==1 => on edge, zero_cnt==2 => on a vertex (on edge), zero_cnt>2 is impossible on a nice poly */
    if ( zero_cnt>0 ) {
return( ht_OnEdge );
    }

return( ht_Inside );
}

/* Something is a valid polygonal pen if: */
/*  1. It contains something (not a line, or a single point, something with area) */
/*  2. It contains ONE contour */
/*	(Multiple contours can be dealt with as long as each contour follows */
/*	 requirements 3-n. If we have multiple contours: Find the offset from */
/*	 the center of each contour to the center of all the contours. Trace  */
/*	 each contour individually and then translate by this offset) */
/*  3. All edges must be lines (no control points) */
/*  4. No more than 255 corners (could extend this number, but why?) */
/*  5. It must be drawn clockwise (not really an error, if found just invert, but important for later checks). */
/*  6. It must be convex */
/*  7. No extranious points on the edges */

enum PolyType PolygonIsConvex(BasePoint *poly,int n, int *badpointindex) {
    /* For each vertex: */
    /*  Remove that vertex from the polygon, and then test if the vertex is */
    /*  inside the resultant poly. If it is inside, then the polygon is not */
    /*  convex */
    /* If all verteces are outside then we have a convex poly */
    /* If one vertex is on an edge, then we have a poly with an unneeded vertex */
    bigreal nx,ny;
    int i,j,ni;

    if ( badpointindex!=NULL )
	*badpointindex = -1;
    if ( n<3 )
return( Poly_TooFewPoints );
    /* All the points might lie on one line. That wouldn't be a polygon */
    nx = -(poly[1].y-poly[0].y);
    ny = (poly[1].x-poly[0].x);
    for ( i=2; i<n; ++i ) {
	if ( (poly[i].x-poly[0].x)*ny - (poly[i].y-poly[0].y)*nx != 0 )
    break;
    }
    if ( i==n )
return( Poly_Line );		/* Colinear */
    if ( n==3 ) {
	/* Triangles are always convex */
return( Poly_Convex );
    }

    for ( j=0; j<n; ++j ) {
	/* Test to see if poly[j] is inside the polygon poly[0..j-1,j+1..n] */
	/* Basically the hit test code above modified to ignore poly[j] */
	int outside = 0, zero_cnt=0, sign=0;
	bigreal sx, sy, dx,dy, dot;

	for ( i=0; ; ++i ) {
	    if ( i==j )
	continue;
	    ni = i+1;
	    if ( ni==n ) ni=0;
	    if ( ni==j ) {
		++ni;
		if ( ni==n )
		    ni=0;		/* Can't be j, because it already was */
	    }

	    sx = poly[ni].x - poly[i].x;
	    sy = poly[ni].y - poly[i].y;
	    dx = poly[j ].x - poly[i].x;
	    dy = poly[j ].y - poly[i].y;
	    /* Only care about signs, so I don't need unit vectors */
	    dot = dx*sy - dy*sx;
	    if ( dot==0 )
		++zero_cnt;
	    else if ( sign==0 )
		sign= dot;
	    else if ( (dot<0 && sign>0) || (dot>0 && sign<0)) {
		outside = true;
	break;
	    }
	    if ( ni==0 )
	break;
	}
	if ( !outside ) {
	    if ( badpointindex!=NULL )
		*badpointindex = j;
	    if ( zero_cnt>0 )
return( Poly_PointOnEdge );
	    else
return( Poly_Concave );
	}
    }
return( Poly_Convex );
}
/******************************************************************************/
/* ******************************* Circle Pen ******************************* */
/******************************************************************************/

static void LineCap(StrokeContext *c,int isend) {
    int cnt, i, start, end, incr;
    BasePoint base, base2, slope, slope2, halfleft, halfright;
    StrokePoint done;
    StrokePoint *p;
    bigreal factor, si, co;

    cnt = ceil(c->radius/c->resolution);
    if ( cnt<2 ) cnt = 2;
    if ( c->cur+2*cnt+10 >= c->max ) {
	int extras = 2*cnt+200;
	c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	c->max += extras;
    }
    done = c->all[c->cur-1];
    if ( !isend )
	--c->cur;		/* If not at the end then we want to insert */
				/* the line cap data before the last thing */
			        /* on the array. So save it (in "done") and */
			        /* remove from array. We'll put it back at the*/
			        /* end */

    base = done.me;
    slope = done.slope;

    cnt = ceil(c->radius/c->resolution);
    if ( cnt<3 )
	cnt = 3;
    if ( c->cap == lc_butt ) {
	/* Flat end at the point */
	if ( isend ) { start=cnt; end=0; incr=-1; } else { start=0; end=cnt; incr=1; }
	for ( i=start; ; i+=incr ) {
	    p = &c->all[c->cur++];
	    p->sp = done.sp;
	    p->t = isend;
	    p->line = true;
	    p->butt_bevel = true;
	    p->me = base;
	    p->slope = slope;
	    p->needs_point_left = p->needs_point_right = i==0 || i==cnt;
	    factor = c->radius*i/(cnt);
	    p->left.x = base.x - factor*p->slope.y;
	    p->left.y = base.y + factor*p->slope.x;
	    p->right.x = base.x + factor*p->slope.y;
	    p->right.y = base.y - factor*p->slope.x;
	    if ( i==end )
	break;
	}
    } else if ( c->cap == lc_square ) {
	/* Continue in the direction of motion for 1 radius, then a flat end*/
	if ( cnt<4 ) cnt=4;
	if ( cnt&1 )
	    ++cnt;
	if ( isend ) {
	    start=2*cnt; end=0; incr=-1;
	} else {
	    start=0; end=2*cnt; incr=1;
	    slope.x = -slope.x;
	    slope.y = -slope.y;
	}
	base2.x = base.x + (c->radius) * slope.x;
	base2.y = base.y + (c->radius) * slope.y;
	halfleft.x = base2.x - c->radius*done.slope.y;
	halfleft.y = base2.y + c->radius*done.slope.x;
	halfright.x = base2.x + c->radius*done.slope.y;
	halfright.y = base2.y - c->radius*done.slope.x;
	{
	    struct extrapoly *ep;
	    if ( c->ecur>=c->emax )
		c->ep = realloc(c->ep,(c->emax+=40)*sizeof(struct extrapoly));
	    ep = &c->ep[c->ecur++];
	    ep->poly[0] = done.left;
	    ep->poly[1].x = done.left.x + c->radius*slope.x;
	    ep->poly[1].y = done.left.y + c->radius*slope.y;
	    ep->poly[2].x = done.right.x + c->radius*slope.x;
	    ep->poly[2].y = done.right.y + c->radius*slope.y;
	    ep->poly[3] = done.right;
	    if ( !isend ) {
		BasePoint temp;
		ep->poly[0] = done.right;
		temp = ep->poly[1];
		ep->poly[1] = ep->poly[2];
		ep->poly[2] = temp;
		ep->poly[3] = done.left;
	    }
	    ep->ptcnt = 4;
	}
	for ( i=start; ; i+=incr ) {
	    p = &c->all[c->cur++];
	    p->sp = done.sp;
	    p->t = isend;
	    p->line = true;
	    p->me = base;
	    p->slope = slope;
	    p->needs_point_left = p->needs_point_right = (i==0 || i==cnt || i==cnt+cnt);
	    if ( i<=cnt ) {
		factor = c->radius*i/cnt;
		p->left.x = base2.x - factor*done.slope.y;
		p->left.y = base2.y + factor*done.slope.x;
		p->right.x = base2.x + factor*done.slope.y;
		p->right.y = base2.y - factor*done.slope.x;
	    } else {
		factor = c->radius*(i-cnt)/cnt;
		p->left.x = halfleft.x - factor*slope.x;
		p->left.y = halfleft.y - factor*slope.y;
		p->right.x = halfright.x - factor*slope.x;
		p->right.y = halfright.y - factor*slope.y;
	    }
	    if ( i==end )
	break;
	}
    } else {
	/* Semicircle */
	if ( cnt<8 )
	    cnt = 8;
	if ( isend ) {
	    start=cnt; end=0; incr=-1;
	} else {
	    start=0; end=cnt; incr=1;
	}
	for ( i=start; ; i+=incr ) {
	    p = &c->all[c->cur++];
	    *p = done;
	    p->circle = true;
	    p->needs_point_left = p->needs_point_right = i==0 || i==cnt;
	    si = sin((PI/2.0)*i/(bigreal) cnt);
	    co = sqrt(1-si*si);
	    if ( isend ) co=-co;
	    slope2.x = slope.x*co + slope.y*si;
	    slope2.y =-slope.x*si + slope.y*co;
	    p->left.x = base.x - c->radius*slope2.x;
	    p->left.y = base.y - c->radius*slope2.y;
	    slope2.x = slope.x*co - slope.y*si;
	    slope2.y = slope.x*si + slope.y*co;
	    p->right.x = base.x - c->radius*slope2.x;
	    p->right.y = base.y - c->radius*slope2.y;
	    if ( i==end )
	break;
	}
    }
    if ( !isend )
	c->all[c->cur++] = done;
}

static void LineJoin(StrokeContext *c,int atbreak) {
    BasePoint nslope, pslope, curslope, nextslope, base, final, inter,
	    center, vector, rot, temp, diff_angle, incr_angle;
    bigreal len, dot, normal_dot, factor;
    int bends_left;
    StrokePoint done;
    StrokePoint *p, *pp;
    int pindex;
    int cnt, cnt1, i, was_neg;
    int force_bevel;
    /* atbreak means that we are dealing with a closed contour, and we have*/
    /*  reached the "end" of the contour (which is only the end because we */
    /*  had to start somewhere), so the next point on the contour is the   */
    /*  start (index 0), as opposed to (index+1) */

    if ( atbreak ) {
	pindex = c->cur-1;
	done = c->all[0];
    } else {
	pindex = c->cur-2;
	done = c->all[c->cur-1];
	/* We decrement c->cur a little later, after we figure out if we need */
	/*  to add a line join */
    }
    if ( pindex<0 ) IError("LineJoin: pindex<0");
    pslope = c->all[pindex].slope;
    nslope = done.slope;
    len = sqrt(nslope.x*nslope.x + nslope.y*nslope.y);
    nslope.x /= len; nslope.y /= len;

    len = sqrt(pslope.x*pslope.x + pslope.y*pslope.y);
    pslope.x /= len; pslope.y /= len;

    dot = (nslope.x*pslope.x + nslope.y*pslope.y);
    if ( dot>=.999 )
return;		/* Essentially colinear */ /* Won't be perfect because control points lie on integers */
    /* miterlimit of 6, 18 degrees */
    force_bevel = ( c->join==lj_miter && dot<c->miterlimit );

    cnt = ceil(c->radius/c->resolution);
    if ( cnt<6 ) cnt = 6;
    if ( c->cur+2*cnt+10 >= c->max ) {
	int extras = 2*cnt+200;
	c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	c->max += extras;
    }
    if ( !atbreak )
	--c->cur;		/* If not at the break then we want to insert */
				/* the line join data before the last thing */
			        /* on the array. So save it (in "done") and */
			        /* remove from array. We'll put it back at the*/
			        /* end */

    normal_dot = -nslope.x*pslope.y + nslope.y*pslope.x;
    if ( normal_dot>0 )
	bends_left = true;
    else
	bends_left = false;	/* So it bends right */

    if ( c->join==lj_bevel || force_bevel ) {
	/* This is easy, a line between the two end points */
	if ( bends_left ) {
	    /* If it bends left, then the joint is on the right */
	    final = done.right; base = c->all[pindex].right;
	} else {
	    final = done.left; base = c->all[pindex].left;
	}
	curslope.x = final.x - base.x;
	curslope.y = final.y - base.y;
	len = sqrt(curslope.x*curslope.x + curslope.y*curslope.y);
	cnt = ceil(len/c->resolution);
	if ( cnt<3 )
	    cnt=3;
	if ( c->cur+cnt+10 >= c->max ) {
	    int extras = cnt+200;
	    c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	    memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	    c->max += extras;
	}
	for ( i=0; i<=cnt; ++i ) {
	    p = &c->all[c->cur++];
	    *p = c->all[pindex];
	    p->line = true;
	    p->butt_bevel = true;
	    p->needs_point_left = p->needs_point_right = i==0 || i==cnt;
	    p->left_hidden = bends_left;
	    p->right_hidden = !bends_left;
	    factor = ((bigreal) i)/cnt;
	    if ( i==cnt )
		p->left = final;
	    else {
		p->left.x = base.x + factor*curslope.x;
		p->left.y = base.y + factor*curslope.y;
	    }
	    if ( bends_left )
		p->right = p->left;
	}
    } else if ( c->join==lj_miter ) {
	/* Almost as easy. Extend from those same two points in the slope */
	/*  at the each until they intersect */
	if ( bends_left ) {
	    base = c->all[pindex].right; final = done.right;
	} else {
	    base = c->all[pindex].left; final = done.left;
	}
	if ( !IntersectLinesSlopes(&inter,&base,&c->all[pindex].slope,
				    &final,&done.slope))
	    inter = base;
	{
	    struct extrapoly *ep;
	    if ( c->ecur>=c->emax )
		c->ep = realloc(c->ep,(c->emax+=40)*sizeof(struct extrapoly));
	    ep = &c->ep[c->ecur++];
	    ep->poly[0] = base;
	    ep->poly[1] = inter;
	    ep->poly[2] = final;
	    if ( bends_left ) {
		ep->poly[0] = final;
		ep->poly[2] = base;
	    }
	    ep->ptcnt = 3;
	}
	curslope.x = inter.x - base.x;
	curslope.y = inter.y - base.y;
	len = sqrt(curslope.x*curslope.x + curslope.y*curslope.y);
	cnt = ceil(len/c->resolution);
	if ( cnt<3 )
	    cnt=3;
	nextslope.x = final.x - inter.x;
	nextslope.y = final.y - inter.y;
	len = sqrt(nextslope.x*nextslope.x + nextslope.y*nextslope.y);
	cnt1 = ceil(len/c->resolution);
	if ( cnt1<3 )
	    cnt1=3;
	if ( c->cur+cnt+cnt1+2 >= c->max ) {
	    int extras = cnt+cnt1+200;
	    c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	    memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	    c->max += extras;
	}
	for ( i=0; i<=cnt; ++i ) {
	    p = &c->all[c->cur++];
	    *p = c->all[pindex];
	    p->line = true;
	    p->needs_point_left = p->needs_point_right = i==0 || i==cnt;
	    p->left_hidden = bends_left;
	    p->right_hidden = !bends_left;
	    factor = ((bigreal) i)/cnt;
	    if ( i==cnt )
		p->left = inter;
	    else {
		p->left.x = base.x + factor*curslope.x;
		p->left.y = base.y + factor*curslope.y;
	    }
	    if ( bends_left )
		p->right = p->left;
	}
	for ( i=1; i<=cnt1; ++i ) {
	    p = &c->all[c->cur++];
	    *p = c->all[pindex];
	    p->line = true;
	    p->needs_point_left = p->needs_point_right = i==0 || i==cnt;
	    p->left_hidden = bends_left;
	    p->right_hidden = !bends_left;
	    factor = ((bigreal) i)/cnt1;
	    if ( i==cnt1 )
		p->left = final;
	    else {
		p->left.x = inter.x + factor*nextslope.x;
		p->left.y = inter.y + factor*nextslope.y;
	    }
	    if ( bends_left )
		p->right = p->left;
	}
    } else {
	/* Now the unit vectors can also be viewed as (sin,cos) pairs */
	/* so to get the separation between them we rotate nslope by -pslope */
	/* or multiple nslope by the rotation matrix: */
	/*  (pslope.x    pslope.y)  */
	/*  (-pslope.y   pslope.x)  */
	diff_angle.x = nslope.x*pslope.x + nslope.y*pslope.y;
	diff_angle.y =-nslope.x*pslope.y + nslope.y*pslope.x;
	/* But what we really want is a small fraction of that. Say 1/20th of */
	/*  the angle. Well that's hard to compute exactly, but don't need exact */
	if ( diff_angle.x<=0 ) {	/* more than 90 (or less than -90) */
	    incr_angle.y = .078459;	/* sin(PI/40) */
	    if ( diff_angle.y<0 )
		incr_angle.y = -incr_angle.y;
	} else
	    incr_angle.y = diff_angle.y/20;
	incr_angle.x = sqrt(1-incr_angle.y*incr_angle.y);
	
	pp = &c->all[pindex];
	center = pp->me;
	if ( bends_left ) {
	    vector.x = pp->right.x -center.x;
	    vector.y = pp->right.y -center.y;
	} else {
	    vector.x = pp->left.x -center.x;
	    vector.y = pp->left.y -center.y;
	}
	rot = incr_angle; was_neg = false;
	for (;;) {
	    if ( c->cur >= c->max ) {
		int extras = 400;
		int off = pp-c->all;
		c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
		memset(c->all+c->max,0,extras*sizeof(StrokePoint));
		c->max += extras;
		pp = &c->all[off];
	    }
	    p = &c->all[c->cur++];
	    *p = *pp;
	    p->circle = true;
	    p->needs_point_left = p->needs_point_right = false;
	    p->left_hidden = bends_left;
	    p->right_hidden = !bends_left;
	    if ( rot.x<=diff_angle.x || (diff_angle.x <= -1 && rot.x <= -0.999999) ) { /* close enough */
		p->right = done.right;
		p->left = done.left;
		p->needs_point_left = p->needs_point_right = true;
	break;
	    } else {
		temp.x = center.x + vector.x*rot.x - vector.y*rot.y;
		temp.y = center.y + vector.x*rot.y + vector.y*rot.x;
		if ( bends_left )
		    p->right = temp;
		else
		    p->left = temp;
		if ( rot.x<=0 && !was_neg ) {
		    was_neg = c->cur-1;
		    p->needs_point_left = p->needs_point_right = true;
		    if ( diff_angle.y>.1 ) {
			incr_angle.y = diff_angle.y/20;
			incr_angle.x = sqrt(1-incr_angle.y*incr_angle.y);
		    }
		}
		temp.x = rot.x*incr_angle.x - rot.y*incr_angle.y;
		temp.y = rot.x*incr_angle.y + rot.y*incr_angle.x;
		if ( temp.y>1 ) {	/* Rounding errors */
		    temp.y=1;
		    temp.x=0;
		} else if ( temp.y<-1 ) {
		    temp.y = -1;
		    temp.x = 0;
		}
		rot = temp;
	    }
	}
	if ( was_neg!=0 && c->cur-was_neg<5 )
	    c->all[was_neg].needs_point_left = c->all[was_neg].needs_point_right = false;
    }
    if (!atbreak) {
        if (c->cur >= c->max) {
            int extras = 10;
            c->all = realloc(c->all, (c->max+extras)*sizeof(StrokePoint));
            memset(c->all+c->max, 0, extras*sizeof(StrokePoint));
            c->max += extras;
        }
        c->all[c->cur++] = done;
    }
}

static void FindSlope(StrokeContext *c,Spline *s, bigreal t, bigreal tdiff) {
    StrokePoint *p = &c->all[c->cur-1];
    bigreal len;

    p->sp = s;
    p->t = t;
    p->me.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
    p->me.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
    p->slope.x = (3*s->splines[0].a*t+2*s->splines[0].b)*t+s->splines[0].c;
    p->slope.y = (3*s->splines[1].a*t+2*s->splines[1].b)*t+s->splines[1].c;
    p->needs_point_left = p->needs_point_right = (t==0.0 || t==1.0);

    if ( p->slope.x==0 && p->slope.y==0 ) {
	/* If the control point is at the endpoint then at the endpoints we */
	/*  have an undefined slope. Can't have that. */
	/* I suppose it could happen elsewhere */
	if ( t>0 )
	    p->slope = p[-1].slope;
	else {
	    bigreal nextt=t+tdiff;
	    p->slope.x = (3*s->splines[0].a*nextt+2*s->splines[0].b)*nextt+s->splines[0].c;
	    p->slope.y = (3*s->splines[1].a*nextt+2*s->splines[1].b)*nextt+s->splines[1].c;
	    if ( p->slope.x==0 && p->slope.y==0 ) {
		BasePoint next;
		next.x = ((s->splines[0].a*nextt+s->splines[0].b)*nextt+s->splines[0].c)*nextt+s->splines[0].d;
		next.y = ((s->splines[1].a*nextt+s->splines[1].b)*nextt+s->splines[1].c)*nextt+s->splines[1].d;
		p->slope.x = next.x - p->me.x;
		p->slope.y = next.y - p->me.y;
	    }
	}
	if ( p->slope.x==0 && p->slope.y==0 ) {
	    p->slope.x = s->to->me.x = s->from->me.x;
	    p->slope.y = s->to->me.y = s->from->me.y;
	}
	if ( p->slope.x==0 && p->slope.y==0 )
	    p->slope.x = 1;
    }
    len = p->slope.x*p->slope.x + p->slope.y*p->slope.y;
    if ( len!=0 ) {
	len = sqrt(len);
	p->slope.x/=len;
	p->slope.y/=len;
    }
}

static void HideStrokePointsCircle(StrokeContext *c) {
    int i,j;
    bigreal xdiff,ydiff, dist2sq, dist1sq;
    bigreal res2 = c->resolution*c->resolution, res_bound = 100*res2;

    for ( i=c->cur-1; i>=0 ; --i ) {
	StrokePoint *p = &c->all[i];
	Spline *myline = p->sp->knownlinear ? p->sp : NULL;
	if ( p->line || p->circle )
	    myline = NULL;
	for ( j=c->cur-1; j>=0; --j ) if ( j!=i ) {
	    StrokePoint *op = &c->all[j];
	    /* Something based at the same location as us cannot cover us */
	    /*  but rounding errors might make it look as though it did */
	    if ( op->me.x==p->me.x && op->me.y==p->me.y )
	continue;
	    /* Something on the same line as us cannot cover us */
	    if ( op->sp==myline )
	continue;
	    if ( p->butt_bevel ) {
		/* Butt caps and bevel line joins cause problems because the */
		/*  point on which they are centered will, in fact, cover them*/
		/*  so we wait until the base point has moved at least radius */
		/*  from our base */
		xdiff = (p->me.x-op->me.x);
		ydiff = (p->me.y-op->me.y);
		dist2sq = xdiff*xdiff + ydiff*ydiff;
		if ( dist2sq<c->radius2 )
	continue;
	    }
	    if ( !p->left_hidden ) {
		xdiff = (p->left.x-op->me.x);
		ydiff = (p->left.y-op->me.y);
		dist1sq = xdiff*xdiff + ydiff*ydiff;
		if ( dist1sq<c->radius2 ) {
		    p->left_hidden = true;
		    if ( p->right_hidden )
	break;
		}
	    } else
		dist1sq = 1e20;
	    if ( !p->right_hidden ) {
		xdiff = (p->right.x-op->me.x);
		ydiff = (p->right.y-op->me.y);
		dist2sq = xdiff*xdiff + ydiff*ydiff;
		if ( dist2sq<c->radius2 ) {
		    p->right_hidden = true;
		    if ( p->left_hidden )
	break;
		}
	    } else
		dist2sq = 1e20;
	    if ( dist1sq>dist2sq )
		dist1sq = dist2sq;
	    dist1sq -= c->radius2;
	    if ( dist1sq>res_bound ) {
		/* If we are far away from the desired point then we can */
		/*  skip ahead more quickly. Since things should be spaced */
		/*  about resolution units apart we know how far we can    */
		/*  skip. Since that measurement isn't perfect we leave some*/
		/*  slop */
		dist1sq /= res2;
		if ( dist1sq<400 )
		    j -= (6-1);
		else
		    j -= .5*sqrt(dist1sq)-1;
		/* Minus 1 because we are going to add 1 anyway */
	    }
	}
    }

    /* now see if any miter join polygons (or square cap polys) cover anything */
    for ( j=0; j<c->ecur; ++j ) {
	BasePoint slopes[4];
	bigreal len;
	struct extrapoly *ep = &c->ep[j];
	for ( i=0; i<ep->ptcnt; ++i ) {
	    int ni = i+1;
	    if ( ni==ep->ptcnt ) ni=0;
	    slopes[i].x = ep->poly[ni].x - ep->poly[i].x;
	    slopes[i].y = ep->poly[ni].y - ep->poly[i].y;
	    len = sqrt(slopes[i].x*slopes[i].x + slopes[i].y*slopes[i].y);
	    if ( len==0 )
return;
	    slopes[i].x /= len; slopes[i].y /= len;
	}
	for ( i=c->cur-1; i>=0 ; --i ) {
	    StrokePoint *p = &c->all[i];
	    if ( !p->left_hidden )
		if ( PolygonHitTest(ep->poly,slopes,ep->ptcnt,&p->left,NULL)==ht_Inside )
		    p->left_hidden = true;
	    if ( !p->right_hidden )
		if ( PolygonHitTest(ep->poly,slopes,ep->ptcnt,&p->right,NULL)==ht_Inside )
		    p->right_hidden = true;
	}
    }
}

static void FindStrokePointsCircle(SplineSet *ss, StrokeContext *c) {
    Spline *s, *first;
    bigreal length;
    int i, len;
    bigreal diff, t;
    int open = ss->first->prev == NULL;
    int gothere = false;

    first = NULL;
    for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	length = AdjustedSplineLength(s);
	if ( length==0 )		/* This can happen when we have a spline with the same first and last point and no control point */
    continue;		/* We can safely ignore it because it is of zero length */
	    /* We need to ignore it because it gives us 0/0 in places */

	len = ceil( length/c->resolution );
	if ( len<2 ) len=2;
	/* There will be len+1 sample points taken. Two of those points will be*/
	/*  the end points, and there will be at least one internal point */
	diff = 1.0/len;
	for ( i=0, t=0; i<=len; ++i, t+= diff ) {
	    StrokePoint *p;
	    if ( c->cur >= c->max ) {
		int extras = len+200;
		c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
		memset(c->all+c->max,0,extras*sizeof(StrokePoint));
		c->max += extras;
	    }
	    p = &c->all[c->cur++];
	    if ( i==len ) t = 1.0;	/* In case there were rounding errors */
	    FindSlope(c,s,t,diff);
	    p->left.x = p->me.x - c->radius*p->slope.y;
	    p->left.y = p->me.y + c->radius*p->slope.x;
	    p->right.x = p->me.x + c->radius*p->slope.y;
	    p->right.y = p->me.y - c->radius*p->slope.x;
	    if ( i==0 ) {
		/* OK, the join or cap should happen before this point */
		/*  But we will use the values we calculate here. So we'll */
		/*  move the stuff we just calculated until after the joint */
		if ( open && !gothere )
		    LineCap(c,0);
		else if ( gothere )		/* Do nothing if it's the join at the start */
		    LineJoin(c,false);
		gothere = true;
	    }
	}
	if ( s->to->next==NULL )
	    LineCap(c,1);
    }
    if ( !open && c->cur>0 )
	LineJoin(c,true);
    HideStrokePointsCircle(c);

    /* just in case... add an on curve point every time the sample's slopes */
    /*  move through 90 degrees. (We only do this for circles because for  */
    /*  squares and polygons this will happen automatically whenever we change */
    /*  polygon/square vertices, which is close enough to the same idea as no */
    /*  matter) */
    { int j,k, skip=10/c->resolution;
    for ( i=c->cur-1; i>=0; ) {
	while ( i>=0 && ( c->all[i].left_hidden || c->all[i].needs_point_left ))
	    --i;
	if ( i<c->cur-1 && !c->all[i+1].left_hidden )
	    ++i;
	while ( i>0 && c->all[i].left.x == c->all[i-1].left.x && c->all[i].left.y == c->all[i-1].left.y )
	    --i;
	for ( j=i-1; j>=0 && !c->all[j].left_hidden && !c->all[j].needs_point_left ; --j ) {
	    if ( c->all[i].slope.x*c->all[j].slope.x + c->all[i].slope.y*c->all[j].slope.y <= 0 ) {
		for ( k=j-1; k>=0 && k>j-skip && !c->all[k].left_hidden && !c->all[k].needs_point_left ; --k );
		if ( k<=j-skip ) {
		    c->all[j].needs_point_left = true;
		    i=j;
		    j=k;
		}
	    }
	}
	i=j;
    }
    for ( i=c->cur-1; i>=0; ) {
	while ( i>=0 && ( c->all[i].right_hidden || c->all[i].needs_point_right ))
	    --i;
	if ( i<c->cur-1 && !c->all[i+1].right_hidden )
	    ++i;
	while ( i>0 && c->all[i].right.x == c->all[i-1].right.x && c->all[i].right.y == c->all[i-1].right.y )
	    --i;
	for ( j=i-1; j>=0 && !c->all[j].right_hidden && !c->all[j].needs_point_right ; --j ) {
	    if ( c->all[i].slope.x*c->all[j].slope.x + c->all[i].slope.y*c->all[j].slope.y <= 0 ) {
		for ( k=j-1; k>=0 && k>j-skip && !c->all[k].right_hidden && !c->all[k].needs_point_right ; --k );
		if ( k<=j-skip ) {
		    c->all[j].needs_point_right = true;
		    i=j;
		    j=k;
		}
	    }
	}
	i=j;
    }
    }
		
}

/******************************************************************************/
/* ******************************* Square Pen ******************************* */
/******************************************************************************/

static BasePoint SquareCorners[] = {
    { -1,  1 },
    {  1,  1 },
    {  1, -1 },
    { -1, -1 },
};

static int WhichSquareCorner(BasePoint *slope, bigreal t, int *right_trace) {
    /* upper left==0, upper right==1, lower right==2, lower left==3 */
    /* Now if we've got a straight line that is horizontal or vertical then */
    /* We return the corner used by the left tracer. The right tracer will */
    /*  use that corner+2%4 */
    int left_trace, rt;

    if ( slope->x==0 ) {
	if ( slope->y>0 )
	    left_trace = 3;
	else
	    left_trace = 1;
    } else if ( slope->y==0 ) {
	if ( slope->x>0 )
	    left_trace = 0;
	else
	    left_trace = 2;
    } else if ( slope->x>0 && slope->y>0 )
	left_trace = 0;
    else if ( slope->x>0 )
	left_trace = 1;
    else if ( slope->y>0 )
	left_trace = 3;
    else
	left_trace = 2;

    rt = left_trace + 2;
    if ( rt>=4 )
	rt -= 4;
    *right_trace = rt;
return( left_trace );
}

static void SquareCap(StrokeContext *c,int isend) {
    int cnt, i, start, end, incr;
    int start_corner, end_corner, cc, nc;
    BasePoint slope1, slope2;
    StrokePoint done;
    StrokePoint *p;
    bigreal t;
    /* PostScript has all sorts of funky line endings for circular points */
    /* I shan't bother with them, except for the one which is "draw the */
    /* final half pen". In PostScript that means draw a semi-circle. Here */
    /* we draw half a square (which is easier to draw than a circle) */

    done = c->all[c->cur-1];
    if ( !isend ) {
	end_corner = done.lt;
	start_corner = done.rt;
    } else {
	end_corner = done.rt;
	start_corner = done.lt;
    }
    /* we draw the square edges in a clockwise direction. So we start at */
    /*  start_corner and add 1 until we get to end corner */
    /* start-end%4==2 */
    cc = end_corner-start_corner;
    if ( cc<0 )
	cc+= 4;
    if ( cc==0 || cc==3 )
	IError( "Unexpected value in SquareCap" );
    /* We want the seam to be half-way. That means if cnt==2 it will be ON the*/
    /*  corner point we add, and if cnt==1 it will be halfway along the line */
    /*  between the two new corners. */
    cnt = ceil(c->radius/c->resolution);
    if ( c->cur+2*cnt+10 >= c->max ) {
	int extras = 2*cnt+200;
	c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	c->max += extras;
    }
    if ( !isend )
	--c->cur;		/* If not at the end then we want to insert */
				/* the line cap data before the last thing */
			        /* on the array. So save it (in "done") and */
			        /* remove from array. We'll put it back at the*/
			        /* end */


    if ( cc==2 ) {
	nc = start_corner+1;
	if ( nc==4 ) nc=0;
	slope1.x = (SquareCorners[nc].x-SquareCorners[done.lt].x)*c->radius;
	slope1.y = (SquareCorners[nc].y-SquareCorners[done.lt].y)*c->radius;
	slope2.x = (SquareCorners[nc].x-SquareCorners[done.rt].x)*c->radius;
	slope2.y = (SquareCorners[nc].y-SquareCorners[done.rt].y)*c->radius;
	if ( !isend ) { start=cnt; end=1; incr=-1; } else { start=1; end=cnt; incr=1; }
	for ( i=start; ; i+=incr ) {
	    t = ((bigreal) i)/cnt;
	    p = &c->all[c->cur++];
	    *p = done;
	    p->line = true;
	    p->needs_point_left = p->needs_point_right = i==cnt;
	    p->left.x = done.left.x + t*slope1.x;
	    p->left.y = done.left.y + t*slope1.y;
	    p->right.x = done.right.x + t*slope2.x;
	    p->right.y = done.right.y + t*slope2.y;
	    if ( i==end )
	break;
	}
    } else {
	slope1.x = done.left.x - done.right.x;
	slope1.y = done.left.y - done.right.y;
	if ( isend ) { start=cnt; end=1; incr=-1; } else { start=1; end=cnt; incr=1; }
	for ( i=start; ; i+=incr ) {
	    t = ((bigreal) i)/(2*cnt);
	    p = &c->all[c->cur++];
	    *p = done;
	    p->line = true;
	    p->needs_point_left = p->needs_point_right = i==cnt;
	    p->left.x = done.left.x - t*slope1.x;
	    p->left.y = done.left.y - t*slope1.y;
	    p->right.x = done.right.x + t*slope1.x;
	    p->right.y = done.right.y + t*slope1.y;
	    if ( i==end )
	break;
	}
    }
    if ( !isend )
	c->all[c->cur++] = done;
}

static void SquareJoin(StrokeContext *c,int atbreak) {
    BasePoint nslope, pslope, base, slope, me;
    bigreal dot;
    int bends_left, start, end, cnt, lastc, nc;
    StrokePoint done;
    StrokePoint *p;
    int pindex, nindex, i;

    if ( atbreak ) {
	pindex = c->cur-1;
	nindex = 0;
    } else {
	pindex = c->cur-2;
	nindex = c->cur-1;
    }
    if ( pindex<0 ) IError("LineJoin: pindex<0");
    done = c->all[nindex];
    pslope = c->all[pindex].slope;
    nslope = done.slope;

    dot = nslope.y*pslope.x - nslope.x*pslope.y;
    if ( dot==0 ) {
	if ( nslope.x*pslope.x + nslope.y*pslope.y > 0 )
return;		/* Colinear */
	/* Otherwise we go in the reverse direction */
	/* We need to know whether we are bending left or right, and a dot of 0 */
	/*  won't tell us ... so half the time we get this wrong */
    }
    if ( done.rt==c->all[pindex].rt )
return;		/* Slope changes, but not enough for us to flip to a new corner */
    if ( dot>0 ) {
	bends_left = true;
	start = c->all[pindex].rt; end = done.rt;
	/* Either the left point at nindex or at pindex will fall exactly on */
	/*  the other edge. It should be hidden, but because it is exactly on*/
	/*  edge it will be retained. Mark as hidden explicitly */
	done.left_hidden = true;
	if ( atbreak )
	    c->all[0].left_hidden = true;
    } else {
	bends_left = false;
	start = c->all[pindex].lt; end = done.lt;
	c->all[pindex].right_hidden = true;
    }

    cnt = ceil(c->radius/c->resolution);
    if ( cnt<2 ) cnt = 2;
    if ( c->cur+3*cnt+10 >= c->max ) {
	int extras = 3*cnt+200;
	c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	c->max += extras;
    }
    if ( !atbreak )
	--c->cur;		/* If not at the break then we want to insert */
				/* the line join data before the last thing */
			        /* on the array. So save it (in "done") and */
			        /* remove from array. We'll put it back at the*/
			        /* end */

    lastc = start;
    for ( nc= bends_left ? start-1 : start+1; ; bends_left ? --nc : ++nc ) {
	if ( nc==4 ) nc=0;
	else if ( nc<0 ) nc = 3;
	base.x = done.me.x + SquareCorners[lastc].x*c->radius;
	base.y = done.me.y + SquareCorners[lastc].y*c->radius;
	slope.x = SquareCorners[nc].x - SquareCorners[lastc].x;
	slope.y = SquareCorners[nc].y - SquareCorners[lastc].y;
	for ( i=1; i<=cnt; ++i ) {
	    p = &c->all[c->cur++];
	    *p = c->all[pindex];
	    p->line = true;
	    p->needs_point_left = p->needs_point_right = i==cnt;
	    p->left_hidden = bends_left;
	    p->right_hidden = !bends_left;
	    me.x = base.x + c->radius*i*slope.x/cnt;
	    me.y = base.y + c->radius*i*slope.y/cnt;
	    if ( bends_left )
		p->right = me;
	    else
		p->left = me;
	}
	if ( nc==end )
    break;
	lastc = nc;
    }
    if ( !atbreak )
	c->all[c->cur++] = done;
}

static void HideStrokePointsSquare(StrokeContext *c) {
    int i,j;
    bigreal xdiff,ydiff, dist1, dist2;
    bigreal res_bound = 10*c->resolution;
    /* Similar to the case for a circular pen, except the hit test for a square */
    /*  is slightly different from the hit test for a circle */

    for ( i=c->cur-1; i>=0 ; --i ) {
	StrokePoint *p = &c->all[i];
	Spline *myline = p->sp->knownlinear ? p->sp : NULL;
	if ( p->line || p->circle )
	    myline = NULL;
	for ( j=c->cur-1; j>=0; --j ) if ( j!=i ) {
	    StrokePoint *op = &c->all[j];
	    /* Something based at the same location as us cannot cover us */
	    /*  but rounding errors might make it look as though it did */
	    if ( op->me.x==p->me.x && op->me.y==p->me.y )
	continue;
	    /* Something on the same line as us cannot cover us */
	    if ( op->sp==myline )
	continue;
	    dist1 = dist2 = 1e20;
	    if ( !p->left_hidden ) {
		if ( (xdiff = (p->left.x-op->me.x))<0 ) xdiff = -xdiff;
		if ( (ydiff = (p->left.y-op->me.y))<0 ) ydiff = -ydiff;
		if ( xdiff<c->radius && ydiff<c->radius ) {
		    p->left_hidden = true;
		    if ( p->right_hidden )
	break;
		} else
		    dist1 = xdiff<ydiff ? xdiff : ydiff;
	    }
	    if ( !p->right_hidden ) {
		if ( (xdiff = (p->right.x-op->me.x))<0 ) xdiff = -xdiff;
		if ( (ydiff = (p->right.y-op->me.y))<0 ) ydiff = -ydiff;
		if ( xdiff<c->radius && ydiff<c->radius ) {
		    p->right_hidden = true;
		    if ( p->left_hidden )
	break;
		} else
		    dist2 = xdiff<ydiff ? xdiff : ydiff;
	    }
	    if ( dist1>dist2 )
		dist1 = dist2;
	    dist1 -= c->radius;
	    if ( dist1>res_bound ) {
		/* If we are far away from the desired point then we can */
		/*  skip ahead more quickly. Since things should be spaced */
		/*  about resolution units apart we know how far we can    */
		/*  skip. Since that measurement isn't perfect we leave some*/
		/*  slop */
		j -= .66667*dist1/c->resolution-1;
		/* Minus 1 because we are going to add 1 anyway */
	    }
	}
    }
}

static int SquareWhichExtreme(StrokePoint *before,StrokePoint *after) {
    /* Just on the off chance that we hit exactly on the extremum */

    if ( RealWithin(before->slope.x*before->slope.y,0,.0001) )
return( -1 );		/* the extremum will have a 0 value for either x/y */
    if ( RealWithin(after->slope.x*after->slope.y,0,.0001) )
return( 1 );

    if ( before->slope.y*after->slope.y < 0 ) {
	/* The extremum is in y */
	if ( before->slope.y<0 )	/* Heading for a minimum */
return( before->me.y<after->me.y ? -1 : 1 );
	else				/* A maximum */
return( before->me.y>after->me.y ? -1 : 1 );
    } else {
	/* The extremum is in x */
	if ( before->slope.x<0 )
return( before->me.x<after->me.x ? -1 : 1 );
	else
return( before->me.x>after->me.x ? -1 : 1 );
    }
}

static void FindStrokePointsSquare(SplineSet *ss, StrokeContext *c) {
    Spline *s, *first;
    bigreal length;
    int i, len, cnt;
    bigreal diff, t;
    int open = ss->first->prev == NULL;
    int gothere = false;
    int lt, rt;
    bigreal factor;

    cnt = ceil(2*c->radius/c->resolution);

    first = NULL;
    for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	length = AdjustedSplineLength(s);
	if ( length==0 )		/* This can happen when we have a spline with the same first and last point and no control point */
    continue;		/* We can safely ignore it because it is of zero length */
	    /* We need to ignore it because it gives us 0/0 in places */

	len = ceil( length/c->resolution );
	if ( len<3 ) len=3;
	/* There will be len+1 sample points take. Two of those points will be*/
	/*  the end points, and there will be at least two internal points */
	if ( c->cur+len+1+8*(cnt+2) >= c->max ) {
	    int extras = len+8*cnt+200;
	    c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	    memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	    c->max += extras;
	}
	diff = 1.0/len;
	for ( i=0, t=0; i<=len; ++i, t+= diff ) {
	    StrokePoint *p;
	    if ( c->cur >= c->max ) {
		int extras = len+8*cnt+200;
		c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
		memset(c->all+c->max,0,extras*sizeof(StrokePoint));
		c->max += extras;
	    }
	    p = &c->all[c->cur++];
	    if ( i==len ) t = 1.0;	/* In case there were rounding errors */
	    FindSlope(c,s,t,diff);
	    lt = WhichSquareCorner(&p->slope,t,&rt);
	    if ( i==0 || p[-1].lt==lt ) {
		p->left.x = p->me.x + c->radius*SquareCorners[lt].x;
		p->left.y = p->me.y + c->radius*SquareCorners[lt].y;
		p->right.x = p->me.x + c->radius*SquareCorners[rt].x;
		p->right.y = p->me.y + c->radius*SquareCorners[rt].y;
		p->lt = lt; p->rt = rt;
	    } else {
		StrokePoint final, base, afters;
		BasePoint slopel, sloper;
		int k, needsafter=false;
		p->left.x = p->me.x + c->radius*SquareCorners[p[-1].lt].x;
		p->left.y = p->me.y + c->radius*SquareCorners[p[-1].lt].y;
		p->right.x = p->me.x + c->radius*SquareCorners[p[-1].rt].x;
		p->right.y = p->me.y + c->radius*SquareCorners[p[-1].rt].y;
		p->lt = p[-1].lt; p->rt = p[-1].rt;

		/* The side change should happen at the extremum */
		/* but we aren't there exactly we've got a point before it */
		/* and a point after it. Either might be closer (and it matters, */
		/* if we get it wrong we can hide the inserted side by mistake) */
		/* so figure out which is appropriate */
		if ( SquareWhichExtreme(p-1,p)==-1 ) {
		    final = base = p[-1];
		    final.left.x = final.me.x + c->radius*SquareCorners[lt].x;
		    final.left.y = final.me.y + c->radius*SquareCorners[lt].y;
		    final.right.x = final.me.x + c->radius*SquareCorners[rt].x;
		    final.right.y = final.me.y + c->radius*SquareCorners[rt].y;
		    final.lt = lt; final.rt = rt;
		    final.needs_point_left = final.needs_point_right = true;
		    p[-1].needs_point_left = p[-1].needs_point_right = true;

		    afters = *p;
		    afters.left.x = afters.me.x + c->radius*SquareCorners[lt].x;
		    afters.left.y = afters.me.y + c->radius*SquareCorners[lt].y;
		    afters.right.x = afters.me.x + c->radius*SquareCorners[rt].x;
		    afters.right.y = afters.me.y + c->radius*SquareCorners[rt].y;
		    --c->cur;
		    needsafter = true;
		} else {
		    final = base = *p;
		    final.left.x = final.me.x + c->radius*SquareCorners[lt].x;
		    final.left.y = final.me.y + c->radius*SquareCorners[lt].y;
		    final.right.x = final.me.x + c->radius*SquareCorners[rt].x;
		    final.right.y = final.me.y + c->radius*SquareCorners[rt].y;
		    final.lt = lt; final.rt = rt;
		    final.needs_point_left = final.needs_point_right = true;
		    p->needs_point_left = p->needs_point_right = true;
		}

		slopel.x = final.left.x - base.left.x;
		slopel.y = final.left.y - base.left.y;
		sloper.x = final.right.x - base.right.x;
		sloper.y = final.right.y - base.right.y;
		for ( k=1; k<cnt; ++k ) {
		    p = &c->all[c->cur++];
		    *p = base;
		    p->needs_point_left = p->needs_point_right = false;
		    p->line = true;
		    factor = ((bigreal) k)/cnt;
		    p->left.x = base.left.x + factor*slopel.x;
		    p->left.y = base.left.y + factor*slopel.y;
		    p->right.x = base.right.x + factor*sloper.x;
		    p->right.y = base.right.y + factor*sloper.y;
		}
		p = &c->all[c->cur++];
		*p = final;
		if ( needsafter ) {
		    p = &c->all[c->cur++];
		    *p = afters;
		}
	    }
	    if ( i==0 ) {
		/* OK, the join or cap should happen before this point */
		/*  But we will use the values we calculate here. So we'll */
		/*  move the stuff we just calculated until after the joint */
		if ( open && !gothere )
		    SquareCap(c,0);
		else if ( gothere )
		    SquareJoin(c,false);
		gothere = true;
	    }
	}
	if ( s->to->next==NULL )
	    SquareCap(c,1);
    }
    if ( !c->open && c->cur>0 )
	SquareJoin(c,true);
    HideStrokePointsSquare(c);
}

/******************************************************************************/
/* ****************************** Polygon Pen ******************************* */
/******************************************************************************/

static int WhichPolyCorner(StrokeContext *c, BasePoint *slope, int *right_trace) {
    /* The corner we are interested in is the corner whose normal distance */
    /*  from the slope is the greatest. */
    /* Normally there will be one such corner. If the slope is parallel to */
    /*  an edge there will be two. There will never be three because this  */
    /*  is a convex poly and there are no unnecessary points */
    int bestl, bestl2, bestr, bestr2;
    bigreal bestl_off, off, bestr_off;
    int i;

    bestl_off = bestr_off = -1;
    bestl = bestl2 = bestr = bestr2 = -1;
    for ( i=0; i<c->n; ++i ) {
	off = -(c->corners[i].x*slope->y) + (c->corners[i].y*slope->x);
	if ( off>0 && off>=bestl_off ) {
	    if ( off>bestl_off ) {
		bestl_off = off;
		bestl = i;
		bestl2 = -1;
	    } else {
		bestl2 = i;
	    }
	}

	off = (c->corners[i].x*slope->y) - (c->corners[i].y*slope->x);
	if ( off>0 && off>=bestr_off ) {
	    if ( off>bestr_off ) {
		bestr_off = off;
		bestr = i;
		bestr2 = -1;
	    } else {
		bestr2 = i;
	    }
	}
    }
    if ( bestl==-1 || bestr==-1 )
	IError( "Failed to find corner in WhichPolyCorner" );
    if ( bestl2!=-1 ) {
	if ( bestl+1!=bestl2 && !(bestl2==c->n-1 && bestl==0) )
	    IError("Unexpected multiple left corners in WhichPolyCorner");
	if ( bestl==0 && bestl2==c->n-1 )
	    bestl = c->n-1;
    }
    if ( bestr2!=-1 ) {
	if ( bestr+1!=bestr2 && !(bestr2==c->n-1 && bestr==0) )
	    IError("Unexpected multiple right corners in WhichPolyCorner");
	if ( bestr==0 && bestr2==c->n-1 )
	    bestr = c->n-1;
    }
    *right_trace = bestr;
return( bestl );
}

static void PolyCap(StrokeContext *c,int isend) {
    int cnt, i, start, end, incr;
    int start_corner, end_corner, cc, nc, bc;
    BasePoint slope1, slope2, base1, base2;
    StrokePoint done;
    StrokePoint *p;
    bigreal t;
    /* Again, we don't worry about funny line endings, we just draw the bit of*/
    /*  polygon that sticks out */

    done = c->all[c->cur-1];
    if ( !isend ) {
	end_corner = done.lt;
	start_corner = done.rt;
    } else {
	end_corner = done.rt;
	start_corner = done.lt;
    }
    cc = end_corner-start_corner;
    if ( cc<0 )
	cc += c->n;
    if ( !isend ) {
	int temp = start_corner + cc/2;
	start_corner = end_corner - cc/2;
	end_corner = temp;
	if ( end_corner  >= c->n ) end_corner   -= c->n;
	if ( start_corner < 0    ) start_corner += c->n;
    }

    cnt = ceil(c->radius/c->resolution);
    if ( c->cur+cc*cnt+10 >= c->max ) {
	int extras = cc*cnt+200;
	c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	c->max += extras;
    }
    if ( !isend )
	--c->cur;		/* If not at the end then we want to insert */
				/* the line cap data before the last thing */
			        /* on the array. So save it (in "done") and */
			        /* remove from array. We'll put it back at the*/
			        /* end */
    /* we draw the poly edges in a clockwise direction. So if isend, we start*/
    /*  at start_corner and add 1 until we get to middle */
    /* But if !isend we start in the middle and work out */
    /* We do things slightly differently if the number of edges we need to */
    /*  draw is even or odd. */
    for ( ; cc>0 ; ) {
	base1.x = done.me.x + c->corners[start_corner].x;
	base1.y = done.me.y + c->corners[start_corner].y;
	base2.x = done.me.x + c->corners[end_corner].x;
	base2.y = done.me.y + c->corners[end_corner].y;
	if (( !isend && cc&1) || ( isend && cc==1 ) ) {
	    slope1.x = c->corners[end_corner].x - c->corners[start_corner].x;
	    slope1.y = c->corners[end_corner].y - c->corners[start_corner].y;
	    if ( !isend ) { start=cnt; end=1; incr=-1; } else { start=1; end=cnt; incr=1; }
	    for ( i=start; ; i+=incr ) {
		t = ((bigreal) i)/(2*cnt);
		p = &c->all[c->cur++];
		*p = done;
		p->line = true;
		p->left_hidden = p->right_hidden = false;
		p->needs_point_left = p->needs_point_right = i==cnt;
		p->left.x = base1.x + t*slope1.x;
		p->left.y = base1.y + t*slope1.y;
		p->right.x = base2.x - t*slope1.x;
		p->right.y = base2.y - t*slope1.y;
		if ( i==end )
	    break;
	    }
	    cc -= 1;
	} else {
	    nc = start_corner+1;
	    if ( nc==c->n )
		nc=0;
	    bc = end_corner-1;
	    if ( bc==-1 )
		bc = c->n-1;
	    slope1.x = (c->corners[nc].x-c->corners[start_corner].x);
	    slope1.y = (c->corners[nc].y-c->corners[start_corner].y);
	    slope2.x = (c->corners[bc].x-c->corners[end_corner].x);
	    slope2.y = (c->corners[bc].y-c->corners[end_corner].y);
	    for ( i=0; i<=cnt; ++i ) {
		t = ((bigreal) i)/cnt;
		p = &c->all[c->cur++];
		*p = done;
		p->left_hidden = p->right_hidden = false;
		p->line = true;
		p->needs_point_left = p->needs_point_right = i==cnt||i==0;
		p->left.x = base1.x + t*slope1.x;
		p->left.y = base1.y + t*slope1.y;
		p->right.x = base2.x + t*slope2.x;
		p->right.y = base2.y + t*slope2.y;
	    }
	    if ( ++start_corner>=c->n ) start_corner=0;
	    if ( --end_corner<0 ) end_corner = c->n-1;
	    
	    cc -= 2;
	}
    }
    /* OK if there were an even number of edges to add, then we are done */
    /* Otherwise we need add one more edge, and meet in the middle */

    if ( cc&1 ) {
    }
    if ( !isend )
	c->all[c->cur++] = done;
}

static void PolyJoin(StrokeContext *c,int atbreak) {
    BasePoint nslope, pslope, base, slope, me;
    bigreal dot;
    int bends_left, dir;
    StrokePoint done;
    StrokePoint *p;
    int pindex, nindex, start, end, cnt, lastc, nc, i;
    /* atbreak means that we are dealing with a close contour, and we have */
    /*  reached the "end" of the contour (which is only the end because we */
    /*  had to start somewhere), so the next point on the contour is the   */
    /*  start (index 0), as opposed to (index+1) */

    if ( atbreak ) {
	pindex = c->cur-1;
	nindex = 0;
    } else {
	pindex = c->cur-2;
	nindex = c->cur-1;
    }
    if ( pindex<0 ) IError("LineJoin: pindex<0");
    done = c->all[nindex];
    pslope = c->all[pindex].slope;
    nslope = done.slope;

    dot = nslope.y*pslope.x - nslope.x*pslope.y;
    if ( dot==0 ) {
	if ( nslope.x*pslope.x + nslope.y*pslope.y > 0 )
return;		/* Colinear */
	/* Otherwise we go in the reverse direction */
	/* We need to know whether we are bending left or right, and a dot of 0 */
	/*  won't tell us ... so half the time we get this wrong */
    }
    if ( dot>0 ) {
	/* Slope changes, but not enough for us to flip to a new corner */
	if ( done.rt==c->all[pindex].rt )
return;
	bends_left = true; dir = -1;
	start = c->all[pindex].rt; end = done.rt;
	done.hide_left_if_on_edge = true;
	if ( atbreak )
	    c->all[0].hide_left_if_on_edge = true;
	c->all[pindex].hide_left_if_on_edge = true;
    } else {
	if ( done.lt==c->all[pindex].lt )
return;
	bends_left = false; dir = 1;
	start = c->all[pindex].lt; end = done.lt;
	done.hide_right_if_on_edge = true;
	if ( atbreak )
	    c->all[0].hide_right_if_on_edge = true;
	c->all[pindex].hide_right_if_on_edge = true;
    }

    cnt = ceil(c->radius/c->resolution);
    if ( cnt<2 ) cnt = 2;

    if ( !atbreak )
	--c->cur;		/* If not at the break then we want to insert */
				/* the line join data before the last thing */
			        /* on the array. So save it (in "done") and */
			        /* remove from array. We'll put it back at the*/
			        /* end */

    lastc = start;
    for ( nc=start+dir; ; nc+=dir ) {
	if ( c->cur+cnt+10 >= c->max ) {
	    int extras = 3*cnt+200;
	    c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	    memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	    c->max += extras;
	}
	if ( nc==c->n ) nc=0;
	else if ( nc<0 ) nc = c->n-1;
	base.x = done.me.x + c->corners[lastc].x;
	base.y = done.me.y + c->corners[lastc].y;
	/* Can't use c->slopes because they are unit vectors */
	slope.x = c->corners[nc].x - c->corners[lastc].x;
	slope.y = c->corners[nc].y - c->corners[lastc].y;
	for ( i=1; i<=cnt; ++i ) {
	    p = &c->all[c->cur++];
	    *p = c->all[pindex];
	    p->line = true;
	    p->needs_point_left = p->needs_point_right = i==cnt;
	    p->left_hidden = bends_left;
	    p->right_hidden = !bends_left;
	    me.x = base.x + i*slope.x/cnt;
	    me.y = base.y + i*slope.y/cnt;
	    if ( bends_left )
		p->right = me;
	    else
		p->left = me;
	}
	if ( nc==end )
    break;
	lastc=nc;
    }
    if ( !atbreak )
	c->all[c->cur++] = done;
}

static void HideStrokePointsPoly(StrokeContext *c) {
    int i,j;
    bigreal xdiff,ydiff, dist1sq, dist2sq;
    BasePoint rel;
    bigreal res2 = c->resolution*c->resolution, res_bound = 100*res2;
    enum hittest hit;
    /* Similar to the case for a circular pen, except the hit test for a poly */
    /*  is slightly different from the hit test for a circle */


    for ( i=c->cur-1; i>=0 ; --i ) {
	StrokePoint *p = &c->all[i];
	Spline *myline = p->sp->knownlinear ? p->sp : NULL;
	if ( p->line || p->circle )
	    myline = NULL;
	for ( j=c->cur-1; j>=0; --j ) if ( j!=i ) {
	    StrokePoint *op = &c->all[j];
	    /* Something based at the same location as us cannot cover us */
	    /*  but rounding errors might make it look as though it did */
	    if ( op->me.x==p->me.x && op->me.y==p->me.y )
	continue;
	    /* Something on the same line as us cannot cover us */
	    if ( op->sp==myline )
	continue;
	    dist1sq = dist2sq = 1e20;
	    if ( !p->left_hidden ) {
		xdiff = (p->left.x-op->me.x);
		ydiff = (p->left.y-op->me.y);
		dist1sq = xdiff*xdiff + ydiff*ydiff;
		if ( dist1sq<=c->largest_distance2 ) {
		    rel.x = xdiff;
		    rel.y = ydiff;
		    if ( (hit = PolygonHitTest(c->corners,c->slopes,c->n,&rel,NULL))==ht_Inside ||
			    (hit==ht_OnEdge && p->hide_left_if_on_edge)) {
			p->left_hidden = true;
			if ( p->right_hidden )
	break;
		    }
		}
	    }
	    if ( !p->right_hidden ) {
		xdiff = (p->right.x-op->me.x);
		ydiff = (p->right.y-op->me.y);
		dist2sq = xdiff*xdiff + ydiff*ydiff;
		if ( dist2sq<=c->largest_distance2 ) {
		    rel.x = xdiff;
		    rel.y = ydiff;
		    if ( (hit = PolygonHitTest(c->corners,c->slopes,c->n,&rel,NULL))==ht_Inside ||
			    (hit==ht_OnEdge && p->hide_right_if_on_edge)) {
			p->right_hidden = true;
			if ( p->left_hidden )
	break;
		    }
		}
	    }
	    if ( dist1sq>dist2sq )
		dist1sq = dist2sq;
	    dist1sq -= c->largest_distance2;
	    if ( dist1sq>res_bound ) {
		/* If we are far away from the desired point then we can */
		/*  skip ahead more quickly. Since things should be spaced */
		/*  about resolution units apart we know how far we can    */
		/*  skip. Since that measurement isn't perfect we leave some*/
		/*  slop */
		dist1sq /= res2;
		if ( dist1sq<400 )
		    j -= (6-1);
		else
		    j -= .7*sqrt(dist1sq)-1;
		/* Minus 1 because we are going to add 1 anyway */
	    }
	}
    }
}

static int PolyWhichExtreme(StrokeContext *c,int corner,int bends_left,
	StrokePoint *before,StrokePoint *after) {
    bigreal by, ay, sy;
    int ret;

    if ( bends_left ) {
	if ( --corner<0 )
	    corner = c->n-1;
    }

    /* Rotate before/after by the slope of the old side of the polygon */
    /*  and find which is further from the origin */
    /* The rotation means that our min/max will be in y */
    /* (so we only need do the calculations for y */
    sy = -before->slope.y*c->slopes[corner].x + before->slope.x*c->slopes[corner].y;
    if ( RealWithin(sy,0,.0001) )		/* Extremum is at the point, not between */
return( -1 );
    if ( RealWithin(-after->slope.y*c->slopes[corner].x + after->slope.x*c->slopes[corner].y, 0,.0001) )
return( 1 );

    by = -before->me.y*c->slopes[corner].x + before->me.x*c->slopes[corner].y;
    ay = -after->me.y*c->slopes[corner].x + after->me.x*c->slopes[corner].y;

    if ( sy < 0 ) {
	/* Heading for a minimum */
	ret = ( by<ay ? -1 : 1 );
    } else {
	/* A maximum */
	ret = ( by>ay ? -1 : 1 );
    }
return( ret );
}

static void FindStrokePointsPoly(SplineSet *ss, StrokeContext *c) {
    Spline *s, *first;
    bigreal length;
    int i, len, bends_left, k;
    bigreal diff, t, factor;
    int open = ss->first->prev == NULL;
    int gothere = false, stuff_happened;
    int lt, rt, oldlt, oldrt, nlt, nrt;
    BasePoint basel, baser, slopel, sloper;
    int cnt, ldiff, rdiff, dir;
    StrokePoint final, base, saveend;

    cnt = c->longest_edge/c->resolution;
    if ( cnt<3 ) cnt = 3;

    first = NULL;
    for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	length = AdjustedSplineLength(s);
	if ( length==0 )		/* This can happen when we have a spline with the same first and last point and no control point */
    continue;		/* We can safely ignore it because it is of zero length */
	    /* We need to ignore it because it gives us 0/0 in places */

	len = ceil( length/c->resolution );
	if ( len<2 ) len=2;
	/* There will be len+1 sample points take. Two of those points will be*/
	/*  the end points, and there will be at least one internal point */
	/* there may be as many as c->n corner changes. Actually there can be */
	/*  c->n on each side, and sides can change independently so 2*c->n */
	if ( c->cur+len+1+(cnt+2)*2*c->n >= c->max ) {
	    int extras = len+(cnt+2)*2*c->n+200;
	    c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
	    memset(c->all+c->max,0,extras*sizeof(StrokePoint));
	    c->max += extras;
	}
	diff = 1.0/len;
	oldlt = oldrt = -1;
	memset(&basel,0,sizeof(basel)); memset(&baser,0,sizeof(baser));
	memset(&slopel,0,sizeof(slopel)); memset(&sloper,0,sizeof(sloper));
	for ( i=0, t=0; i<=len; ++i, t+= diff ) {
	    StrokePoint *p;
	    if ( c->cur+2 >= c->max ) {
		int extras = 200+len;
		c->all = realloc(c->all,(c->max+extras)*sizeof(StrokePoint));
		memset(c->all+c->max,0,extras*sizeof(StrokePoint));
		c->max += extras;
		p = &c->all[c->cur-1];
	    }
	    p = &c->all[c->cur++];
	    if ( i==len ) t = 1.0;	/* In case there were rounding errors */
	    FindSlope(c,s,t,diff);
	    lt = WhichPolyCorner(c,&p->slope,&rt);
	    stuff_happened = false;
	    if ( p!=c->all && (lt!=oldlt || rt!=oldrt )) {
		bigreal dot = p->slope.y*p[-1].slope.x - p->slope.x*p[-1].slope.y;
		bends_left = dot>0;
		dir = bends_left? -1 : 1;
		saveend = *p;
		--c->cur;
		stuff_happened = true;
	    }
	    while ( oldlt!=-1 && (lt!=oldlt || rt!=oldrt )) {
		ldiff=0; nlt = oldlt;
		if ( lt!=oldlt ) {
		    ldiff = PolyWhichExtreme(c,oldlt,bends_left,p-1,p);
		    if ( (nlt = oldlt+dir)<0 ) nlt += c->n;
		    else if ( nlt>=c->n ) nlt = 0;
		    p[-1].needs_point_left = true;
		}
		rdiff = 0; nrt = oldrt;
		if ( rt!=oldrt ) {
		    rdiff = PolyWhichExtreme(c,oldrt,bends_left,p-1,p);
		    if ( (nrt = oldrt+dir)<0 ) nrt += c->n;
		    else if ( nrt>=c->n ) nrt = 0;
		    p[-1].needs_point_right = true;
		}
		if ( ldiff<0 || rdiff<0 ) {
		    int local_nlt = ldiff<0 ? nlt : oldlt;
		    int local_nrt = rdiff<0 ? nrt : oldrt;
		    final = base = p[-1];
		    final.left.x = final.me.x + c->corners[local_nlt].x;
		    final.left.y = final.me.y + c->corners[local_nlt].y;
		    final.right.x = final.me.x + c->corners[local_nrt].x;
		    final.right.y = final.me.y + c->corners[local_nrt].y;
		    final.lt = local_nlt; final.rt = local_nrt;

		    slopel.x = final.left.x - base.left.x;
		    slopel.y = final.left.y - base.left.y;
		    sloper.x = final.right.x - base.right.x;
		    sloper.y = final.right.y - base.right.y;
		    for ( k=1; k<=cnt; ++k ) {
			p = &c->all[c->cur++];
			*p = base;
			p->needs_point_left = p->needs_point_right = false;
			p->line = true;
			factor = ((bigreal) k)/cnt;
			p->left.x = base.left.x + factor*slopel.x;
			p->left.y = base.left.y + factor*slopel.y;
			p->right.x = base.right.x + factor*sloper.x;
			p->right.y = base.right.y + factor*sloper.y;
		    }
		    p->needs_point_left = oldlt != local_nlt;
		    p->needs_point_right = oldrt != local_nrt;
		    oldlt = local_nlt; oldrt = local_nrt;
		}
		if ( ldiff>0 || rdiff>0 ) {
		    int local_nlt = ldiff>0 ? nlt : oldlt;
		    int local_nrt = rdiff>0 ? nrt : oldrt;
		    final = base = saveend;
		    base.left.x = base.me.x + c->corners[p[-1].lt].x;
		    base.left.y = base.me.y + c->corners[p[-1].lt].y;
		    base.right.x = base.me.x + c->corners[p[-1].rt].x;
		    base.right.y = base.me.y + c->corners[p[-1].rt].y;
		    final.left.x = final.me.x + c->corners[local_nlt].x;
		    final.left.y = final.me.y + c->corners[local_nlt].y;
		    final.right.x = final.me.x + c->corners[local_nrt].x;
		    final.right.y = final.me.y + c->corners[local_nrt].y;
		    final.lt = lt; final.rt = rt;

		    slopel.x = final.left.x - base.left.x;
		    slopel.y = final.left.y - base.left.y;
		    sloper.x = final.right.x - base.right.x;
		    sloper.y = final.right.y - base.right.y;
		    for ( k=0; k<=cnt; ++k ) {
			p = &c->all[c->cur++];
			*p = base;
			p->needs_point_left = p->needs_point_right = false;
			if ( k==0 || k==cnt ) {
			    p->needs_point_left = oldlt != local_nlt;
			    p->needs_point_right = oldrt != local_nrt;
			}
			p->line = true;
			factor = ((bigreal) k)/cnt;
			p->left.x = base.left.x + factor*slopel.x;
			p->left.y = base.left.y + factor*slopel.y;
			p->right.x = base.right.x + factor*sloper.x;
			p->right.y = base.right.y + factor*sloper.y;
		    }
		    oldlt = local_nlt; oldrt = local_nrt;
		}
	    }
	    if ( stuff_happened ) {
		p = &c->all[c->cur++];
		*p = saveend;
	    }
	    p->left.x = p->me.x + c->corners[lt].x;
	    p->left.y = p->me.y + c->corners[lt].y;
	    p->right.x = p->me.x + c->corners[rt].x;
	    p->right.y = p->me.y + c->corners[rt].y;
	    p->lt = lt; p->rt = rt;
	    oldlt = lt; oldrt = rt;

	    if ( i==0 ) {
		/* OK, the join or cap should happen before this point */
		/*  But we will use the values we calculate here. So we'll */
		/*  move the stuff we just calculated until after the joint */
		if ( open && !gothere )
		    PolyCap(c,0);
		else if ( gothere )
		    PolyJoin(c,false);
		gothere = true;
	    }
	    if ( c->cur>c->max )
		IError("Memory corrupted" );
	}
	if ( s->to->next==NULL )
	    PolyCap(c,1);
    }
    if ( !c->open && c->cur>0 )
	PolyJoin(c,true);
    HideStrokePointsPoly(c);
}

/******************************************************************************/
/* ******************************* To Splines ******************************* */
/******************************************************************************/

static void SSRemoveColinearPoints(SplineSet *ss) {
    SplinePoint *sp, *nsp, *nnsp;
    BasePoint dir, ndir;
    bigreal len;
    int removed;

    sp = ss->first;
    if ( sp->next==NULL )
return;
    nsp = sp->next->to;
    if ( nsp==sp )
return;
    dir.x = nsp->me.x - sp->me.x; dir.y = nsp->me.y - sp->me.y;
    len = dir.x*dir.x + dir.y*dir.y;
    if ( len!=0 ) {
	len = sqrt(len);
	dir.x /= len; dir.y /= len;
	if ( sp->next->knownlinear && ( !sp->nonextcp || !nsp->noprevcp )) {
	    sp->nonextcp = true;
	    nsp->noprevcp = true;
	    SplineRefigure(sp->next);
	}
    }
    if ( nsp->next==NULL )
return;
    nnsp = nsp->next->to;
    memset(&ndir,0,sizeof(ndir));
    removed = false;
    for (;;) {
	if ( sp==nsp )
    break;
	if ( nsp->next->knownlinear ) {
	    ndir.x = nnsp->me.x - nsp->me.x; ndir.y = nnsp->me.y - nsp->me.y;
	    len = ndir.x*ndir.x + ndir.y*ndir.y;
	    if ( len!=0 ) {
		len = sqrt(len);
		ndir.x /= len; ndir.y /= len;
		if ( nsp->next->knownlinear && ( !nsp->nonextcp || !nnsp->noprevcp )) {
		    nsp->nonextcp = true;
		    nnsp->noprevcp = true;
		    SplineRefigure(nsp->next);
		}
	    }
	}
	if ( sp->next->knownlinear && nsp->next->knownlinear ) {
	    bigreal dot =dir.x*ndir.y - dir.y*ndir.x;
	    if ( dot<.001 && dot>-.001 ) {
		sp->next->to = nnsp;
		nnsp->prev = sp->next;
		SplineRefigure(sp->next);
		SplineFree(nsp->next);
		SplinePointFree(nsp);
		if ( ss->first==nsp ) { ss->first = sp; ss->start_offset = 0; }
		if ( ss->last ==nsp ) ss->last  = sp;
		removed = true;
	    } else
		sp = nsp;
	} else
	    sp = nsp;
	dir = ndir;
	nsp = nnsp;
	if ( nsp->next==NULL )
    break;
	nnsp = nsp->next->to;
	if ( sp==ss->first ) {
	    if ( !removed )
    break;
	    removed = false;
	}
    }
}

static struct shapedescrip {
    BasePoint me, prevcp, nextcp;
}
unitcircle[] = {
    { { -1, 0 }, { -1, -0.552 }, { -1, 0.552 } },
    { { 0 , 1 }, { -0.552, 1 }, { 0.552, 1 } },
    { { 1, 0 }, { 1, 0.552 }, { 1, -0.552 } },
    { { 0, -1 }, { 0.552, -1 }, { -0.552, -1 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 } }
};

static SplinePoint *SpOnCircle(int i,bigreal radius,BasePoint *center) {
    SplinePoint *sp = SplinePointCreate(unitcircle[i].me.x*radius + center->x,
					unitcircle[i].me.y*radius + center->y);
    sp->pointtype = pt_curve;
    sp->prevcp.x = unitcircle[i].prevcp.x*radius + center->x;
    sp->prevcp.y = unitcircle[i].prevcp.y*radius + center->y;
    sp->nextcp.x = unitcircle[i].nextcp.x*radius + center->x;
    sp->nextcp.y = unitcircle[i].nextcp.y*radius + center->y;
    sp->nonextcp = sp->noprevcp = false;
return( sp );
}

SplineSet *UnitShape(int n) {
    SplineSet *ret;
    SplinePoint *sp1, *sp2;
    int i;
    BasePoint origin;

    ret = chunkalloc(sizeof(SplineSet));
    if ( n>=3 || n<=-3 ) {
	/* Regular n-gon with n sides */
	/* Inscribed in a unit circle, if n<0 then circumscribed around */
	bigreal angle = 2*PI/(2*n);
	bigreal factor=1;
	if ( n<0 ) {
	    angle = -angle;
	    n = -n;
	    factor = 1/cos(angle);
	}
	angle -= PI/2;
	ret->first = sp1 = SplinePointCreate(factor*cos(angle), factor*sin(angle));
	sp1->pointtype = pt_corner;
	for ( i=1; i<n; ++i ) {
	    angle = 2*PI/(2*n) + i*2*PI/n - PI/2;
	    sp2 = SplinePointCreate(factor*cos(angle),factor*sin(angle));
	    sp2->pointtype = pt_corner;
	    SplineMake3(sp1,sp2);
	    sp1 = sp2;
	}
	SplineMake3(sp1,ret->first);
	ret->last = ret->first;
	SplineSetReverse(ret);		/* Drat, just drew it counter-clockwise */
    } else if ( n ) {
	ret->first = sp1 = SplinePointCreate(SquareCorners[0].x,
			    SquareCorners[0].y);
	sp1->pointtype = pt_corner;
	for ( i=1; i<4; ++i ) {
	    sp2 = SplinePointCreate(SquareCorners[i].x,
				SquareCorners[i].y);
	    sp2->pointtype = pt_corner;
	    SplineMake3(sp1,sp2);
	    sp1 = sp2;
	}
	SplineMake3(sp1,ret->first);
	ret->last = ret->first;
    } else {
	/* Turn into a circle */
	origin.x = origin.y = 0;
	ret->first = sp1 = SpOnCircle(0,1,&origin);
	for ( i=1; i<4; ++i ) {
	    sp2 = SpOnCircle(i,1,&origin);
	    SplineMake3(sp1,sp2);
	    sp1 = sp2;
	}
	SplineMake3(sp1,ret->first);
	ret->last = ret->first;
    }
return( ret );
}

static SplinePointList *SinglePointStroke(SplinePoint *sp,struct strokecontext *c) {
    SplineSet *ret;
    SplinePoint *sp1, *sp2;
    int i;

    ret = chunkalloc(sizeof(SplineSet));

    if ( c->pentype==pt_circle && c->cap==lc_butt ) {
	/* Leave as a single point */
	ret->first = ret->last = SplinePointCreate(sp->me.x,sp->me.y);
	ret->first->pointtype = pt_corner;
    } else if ( c->pentype==pt_circle && c->cap==lc_round ) {
	/* Turn into a circle */
	ret->first = sp1 = SpOnCircle(0,c->radius,&sp->me);
	for ( i=1; i<4; ++i ) {
	    sp2 = SpOnCircle(i,c->radius,&sp->me);
	    SplineMake3(sp1,sp2);
	    sp1 = sp2;
	}
	SplineMake3(sp1,ret->first);
	ret->last = ret->first;
    } else if ( c->pentype==pt_circle || c->pentype==pt_square ) {
	ret->first = sp1 = SplinePointCreate(sp->me.x+c->radius*SquareCorners[0].x,
			    sp->me.y+c->radius*SquareCorners[0].y);
	sp1->pointtype = pt_corner;
	for ( i=1; i<4; ++i ) {
	    sp2 = SplinePointCreate(sp->me.x+c->radius*SquareCorners[i].x,
				sp->me.y+c->radius*SquareCorners[i].y);
	    sp2->pointtype = pt_corner;
	    SplineMake3(sp1,sp2);
	    sp1 = sp2;
	}
	SplineMake3(sp1,ret->first);
	ret->last = ret->first;
    } else {
	ret->first = sp1 = SplinePointCreate(sp->me.x+c->corners[0].x,
			    sp->me.y+c->corners[0].y);
	sp1->pointtype = pt_corner;
	for ( i=1; i<c->n; ++i ) {
	    sp2 = SplinePointCreate(sp->me.x+c->corners[i].x,
				sp->me.y+c->corners[i].y);
	    sp2->pointtype = pt_corner;
	    SplineMake3(sp1,sp2);
	    sp1 = sp2;
	}
	SplineMake3(sp1,ret->first);
	ret->last = ret->first;
    }
return( ret );
}

static int AllHiddenLeft(struct strokecontext *c) {
    int i;

    for ( i=c->cur-1; i>=0 && c->all[i].left_hidden; --i );
return( i<0 );
}

static int AllHiddenRight(struct strokecontext *c) {
    int i;

    for ( i=c->cur-1; i>=0 && c->all[i].right_hidden; --i );
return( i<0 );
}

static void LeftSlopeAtPos(struct strokecontext *c,int pos,int prev,BasePoint *slope) {
    /* Normally the slope is just the slope stored in the StrokePoint, but */
    /*  that's not the case for line caps and joins (will be for internal  */
    /*  poly edges */
    bigreal len;
    int i;

    if (( prev && pos==0) || (!prev && pos==c->cur-1)) {
	slope->x = slope->y = 0;	/* Won't know the slope till we join it to something else */
return;			/* Can't normalize */
    }
    if ( ( prev && c->all[pos-1].circle) ||
	     (!prev && c->all[pos+1].circle ) ) {
	/* circle cap/join. Slope is normal to the line from left to me */
	slope->x =  (c->all[pos].left.y-c->all[pos].me.y);
	slope->y = -(c->all[pos].left.x-c->all[pos].me.x);
    } else if ( ( prev && c->all[pos-1].line) ||
	     (!prev && c->all[pos+1].line ) ) {
	slope->x = slope->y = 0;
	/* Sometimes two adjacent points will be coincident (hence the loop) */
	for ( i=1;
		slope->x==0 && slope->y==0 && ((prev && pos>=i) || (!prev && pos+i<c->cur));
		++i ) {
	    if ( prev ) {
		slope->x = c->all[pos].left.x - c->all[pos-i].left.x;
		slope->y = c->all[pos].left.y - c->all[pos-i].left.y;
	    } else {
		slope->x = c->all[pos+i].left.x - c->all[pos].left.x;
		slope->y = c->all[pos+i].left.y - c->all[pos].left.y;
	    }
	}
    } else {
	*slope = c->all[pos].slope;
return;		/* Already a unit vector */
    }
    len = slope->x*slope->x + slope->y*slope->y;
    if ( len!=0 ) {
	len = sqrt(len);
	slope->x /= len;
	slope->y /= len;
    }
}

static SplinePoint *LeftPointFromContext(struct strokecontext *c, int *_pos,
	int *newcontour) {
    /* pos should be pointing to an area where we should create a new splinepoint */
    /* this can happen for several reasons: */
    /*  1) There was one in the original contour */
    /*     (actually there will be two: t==1 on one spline, t==0 on the other*/
    /*  2) The line cap/join requires one here */
    /*  3) A square or poly pen switched corners */
    /*  4) a bunch of StrokePoints were hidden. In which case we must create */
    /*     a point by taking the first and last unhidden points (around pos) */
    /*     and their slopes and finding where those intersect. If we've done */
    /*     everything right, the intersection will be near the two unhidden  */
    /*     points. This can eat up arbetrary amounts of strokepoints */
    /* When pos==0, life is more complex. In a closed contour we may have to */
    /*  walk backward on the contour which means starting at end and going   */
    /*  back from there. On an open contour, we have to go forwards along the*/
    /*  right edge. If there's an original splinepoint here, we just have to */
    /*  walk back one to find the slope, but if things are hidden we can walk*/
    /*  arbetrarily far */
    int pos = *_pos, start_pos=pos, orig_pos = pos;
    BasePoint posslope, startslope,inter;
    SplinePoint *ret;
    bigreal normal_dot, len1, len2;
    bigreal res_bound = 16*c->resolution*c->resolution;

    *newcontour = false;
    if ( pos==0 && !c->open ) {
	if ( c->all[0].left_hidden ||
		( c->all[0].needs_point_left && c->all[c->cur-1].needs_point_left ))
	    for ( start_pos=c->cur-1; start_pos>=0 && c->all[start_pos].left_hidden; --start_pos);
	/* Might be some hidden points if the other side has a joint !!!!!*/
    } else if ( c->all[start_pos].left_hidden && start_pos!=0 )
	--start_pos;
    if ( pos+1<c->cur && c->all[pos].t==1 && c->all[pos+1].t==0 )
	++pos;
    while ( pos<c->cur && c->all[pos].left_hidden )
	++pos;
    while ( pos+1<c->cur &&
	    c->all[pos].left.x==c->all[pos+1].left.x &&
	    c->all[pos].left.y==c->all[pos+1].left.y )
	++pos;
    if ( pos>=c->cur ) {
	if ( c->open )
	    pos = c->cur-1;
	else {
	    pos = 0;
	    while ( pos<c->cur && c->all[pos].left_hidden )
		++pos;
	    if ( pos>=c->cur )
		pos = c->cur-1;		/* Should never happen */
	}
    }
    LeftSlopeAtPos(c,pos,false,&posslope);
    LeftSlopeAtPos(c,start_pos,true,&startslope);
    normal_dot = startslope.x*posslope.y - startslope.y*posslope.x;
    if ( normal_dot<0 ) normal_dot = -normal_dot;
    len2 = (c->all[start_pos].left.x-c->all[pos].left.x)*(c->all[start_pos].left.x-c->all[pos].left.x) + (c->all[start_pos].left.y-c->all[pos].left.y)*(c->all[start_pos].left.y-c->all[pos].left.y);
    if ( len2>=res_bound && orig_pos==0 ) {
	/* If we're going to start a new contour at the beginnin, don't bother */
	len2 = 0;
	start_pos = pos;
	startslope = posslope;
	normal_dot = 0;
    }
/* I used to think that if there were hidden points between start and pos then*/
/* those two would be near each other. But there are cases where the hidden */
/* section is at the ends of a long rectangle. In that case we must draw a  */
/* line between the two (actually we should start a new contour, but we aren't*/
    if ( normal_dot < .01 && len2<.001 ) {
	ret = SplinePointCreate(c->all[pos].left.x, c->all[pos].left.y);
	ret->pointtype = pt_curve;
    } else if ( len2<16*c->resolution*c->resolution ) {
	if ( !IntersectLinesSlopes(&inter,
		&c->all[start_pos].left,&startslope,
		&c->all[pos].left,&posslope))
	    inter = c->all[pos].left;
	len1 = (inter.x-c->all[pos].left.x)*(inter.x-c->all[pos].left.x) + (inter.y-c->all[pos].left.y)*(inter.y-c->all[pos].left.y);
	if ( (len1>4 && len1>10*len2) || len1>100 ) {
	    /* Intersection is too far (slopes are very accute? */
	    inter.x = (c->all[pos].left.x + c->all[start_pos].left.x)/2;
	    inter.y = (c->all[pos].left.y + c->all[start_pos].left.y)/2;
	}
	ret = SplinePointCreate(inter.x, inter.y);
	ret->pointtype = pt_corner;
    } else {
	ret = SplinePointCreate(c->all[start_pos].left.x,c->all[start_pos].left.y);
	*newcontour=true;		/* indicates a break in the contour */
    }
    /* CP distance is irrelevent here, all we care about is direction */
    ret->prevcp.x -= startslope.x;
    ret->prevcp.y -= startslope.y;
    if ( startslope.x!=0 || startslope.y!=0 )
	ret->noprevcp = false;
    if ( !*newcontour ) {
	ret->nextcp.x += posslope.x;
	ret->nextcp.y += posslope.y;
	if ( posslope.x!=0 || posslope.y!=0 )
	    ret->nonextcp = false;
	/* the same point will often appear at the end of one spline and the */
	/*  start of the next */
	/* Or a joint may add a lot of points to the other side which show up */
	/*  hidden on this side */
	while ( pos+1<c->cur &&
		((ret->me.x==c->all[pos+1].left.x && ret->me.y==c->all[pos+1].left.y ) ||
		 c->all[pos+1].left_hidden))
	    ++pos;
	len2 = (ret->me.x-c->all[pos].left.x)*(ret->me.x-c->all[pos].left.x) + (ret->me.y-c->all[pos].left.y)*(ret->me.y-c->all[pos].left.y);
	if ( len2>=16*c->resolution*c->resolution )
	    *newcontour = true;
    }
    *_pos = pos;
return( ret );
}

static void RightSlopeAtPos(struct strokecontext *c,int pos,int prev,BasePoint *slope) {
    /* Normally the slope is just the slope stored in the StrokePoint, but */
    /*  that's not the case for line caps and joins (will be for internal  */
    /*  poly edges */
    bigreal len;
    int i;

    if (( prev && pos==0) || (!prev && pos==c->cur-1)) {
	slope->x = slope->y = 0;	/* Won't know the slope till we join it to something else */
return;			/* Can't normalize */
    }
    if ( ( prev && c->all[pos-1].circle) ||
	     (!prev && c->all[pos+1].circle ) ) {
	/* circle cap/join. Slope is normal to the line from right to me */
	slope->x = -(c->all[pos].right.y-c->all[pos].me.y);
	slope->y =  (c->all[pos].right.x-c->all[pos].me.x);
    } else if ( ( prev && c->all[pos-1].line) ||
	     (!prev && c->all[pos+1].line ) ) {
	slope->x = slope->y = 0;
	for ( i=1;
		slope->x==0 && slope->y==0 && ((prev && pos>=i) || (!prev && pos+i<c->cur));
		++i ) {
	    if ( prev ) {
		slope->x = c->all[pos].right.x - c->all[pos-i].right.x;
		slope->y = c->all[pos].right.y - c->all[pos-i].right.y;
	    } else {
		slope->x = c->all[pos+i].right.x - c->all[pos].right.x;
		slope->y = c->all[pos+i].right.y - c->all[pos].right.y;
	    }
	}
    } else {
	*slope = c->all[pos].slope;
return;		/* Already a unit vector */
    }
    len = slope->x*slope->x + slope->y*slope->y;
    if ( len!=0 ) {
	len = sqrt(len);
	slope->x /= len;
	slope->y /= len;
    }
}

static SplinePoint *RightPointFromContext(struct strokecontext *c, int *_pos,
	int *newcontour) {
    int pos = *_pos, start_pos=pos, orig_pos=pos;
    BasePoint posslope, startslope,inter;
    SplinePoint *ret;
    bigreal normal_dot, len1, len2;
    bigreal res_bound = 16*c->resolution*c->resolution;

    *newcontour = false;
    if ( pos==0 && !c->open ) {
	if ( c->all[0].right_hidden ||
		( c->all[0].needs_point_right && c->all[c->cur-1].needs_point_right ))
	    for ( start_pos=c->cur-1; start_pos>=0 && c->all[start_pos].right_hidden; --start_pos);
	/* Might be some hidden points if the other side has a joint !!!!!*/
    } else if ( c->all[start_pos].right_hidden && start_pos!=0 )
	--start_pos;
    if ( pos+1<c->cur && c->all[pos].t==1 && c->all[pos+1].t==0 )
	++pos;
    while ( pos<c->cur && c->all[pos].right_hidden )
	++pos;
    while ( pos+1<c->cur &&
	    c->all[pos].right.x==c->all[pos+1].right.x &&
	    c->all[pos].right.y==c->all[pos+1].right.y )
	++pos;
    if ( pos>=c->cur ) {
	if ( c->open )
	    pos = c->cur-1;
	else {
	    pos = 0;
	    while ( pos<c->cur && c->all[pos].right_hidden )
		++pos;
	    if ( pos>=c->cur )
		pos = c->cur-1;		/* Should never happen */
	}
    }
    RightSlopeAtPos(c,pos,false,&posslope);
    RightSlopeAtPos(c,start_pos,true,&startslope);
    normal_dot = startslope.x*posslope.y - startslope.y*posslope.x;
    if ( normal_dot<0 ) normal_dot = -normal_dot;
    len2 = (c->all[start_pos].right.x-c->all[pos].right.x)*(c->all[start_pos].right.x-c->all[pos].right.x) + (c->all[start_pos].right.y-c->all[pos].right.y)*(c->all[start_pos].right.y-c->all[pos].right.y);
    if ( len2>=res_bound && orig_pos==0 ) {
	/* If we're going to start a new contour at the beginnin, don't bother */
	len2 = 0;
	start_pos = pos;
	startslope = posslope;
	normal_dot = 0;
    }
/* I used to think that if there were hidden points between start and pos then*/
/* those two would be near each other. But there are cases where the hidden */
/* section is at the ends of a long rectangle. In that case we should start a */
/* new contour */
    if ( normal_dot < .01 && len2 < .01 ) {
	ret = SplinePointCreate(c->all[pos].right.x, c->all[pos].right.y);
	ret->pointtype = pt_curve;
    } else if ( len2<res_bound ) {
	if ( !IntersectLinesSlopes(&inter,
		&c->all[start_pos].right,&startslope,
		&c->all[pos].right,&posslope))
	    inter = c->all[pos].right;
	len1 = (inter.x-c->all[pos].right.x)*(inter.x-c->all[pos].right.x) + (inter.y-c->all[pos].right.y)*(inter.y-c->all[pos].right.y);
	if ( len1>4 && len1>10*len2 ) {
	    /* Intersection is too far (slopes are very accute? */
	    inter.x = (c->all[pos].right.x + c->all[start_pos].right.x)/2;
	    inter.y = (c->all[pos].right.y + c->all[start_pos].right.y)/2;
	}
	ret = SplinePointCreate(inter.x, inter.y);
	ret->pointtype = pt_corner;
    } else {
	ret = SplinePointCreate(c->all[start_pos].right.x,c->all[start_pos].right.y);
	*newcontour = true;
    }
    /* CP distance is irrelevent here, all we care about is direction */
    ret->prevcp.x -= startslope.x;
    ret->prevcp.y -= startslope.y;
    if ( startslope.x!=0 || startslope.y!=0 )
	ret->noprevcp = false;
    if ( !*newcontour ) {
	/* the same point will often appear at the end of one spline and the */
	/*  start of the next */
	/* Or a joint may add a lot of points to the other side which show up */
	/*  hidden on this side */
	int opos = pos;
	while ( pos+1<c->cur &&
		((ret->me.x==c->all[pos+1].right.x && ret->me.y==c->all[pos+1].right.y ) ||
		 c->all[pos+1].right_hidden))
	    ++pos;
	if ( opos!=pos ) {
	    RightSlopeAtPos(c,pos,false,&posslope);
	    ret->pointtype = pt_corner;
	}
	ret->nextcp.x += posslope.x;
	ret->nextcp.y += posslope.y;
	if ( posslope.x!=0 || posslope.y!=0 )
	    ret->nonextcp = false;
	len2 = (ret->me.x-c->all[pos].right.x)*(ret->me.x-c->all[pos].right.x) + (ret->me.y-c->all[pos].right.y)*(ret->me.y-c->all[pos].right.y);
	if ( len2>=res_bound )
	    *newcontour = true;
    }
    *_pos = pos;
return( ret );
}

static void HideSomeMorePoints(StrokeContext *c) {
    /* Fix up a couple of problems that hit tests miss */
    /* 1. At the extreme end of a spline when it comes to a joint, then  */
    /*    the last point generated (t==1/t==0) will lie on the >edge< of */
    /*    the covering polygon. So the hit test won't find it (not interior)*/
    /*    But it should none the less be removed */
    /* 2. We don't have a continuous view of the generated contour, we have */
    /*    a set of samples. Similarly, we don't use a continuous view of the*/
    /*    original contour, we use the same set of samples for our hit tests*/
    /*    it can happen that the region of the original which will hide a   */
    /*    given generated sample, is not in our sample set. */
    /* So let's say that any singleton unhidden sample between two hidden   */
    /*  samples should be hidden */
    /* But that isn't good enough. I had two adjacent unhidden points once  */

    /* Similarly there are cases (if the poly edges aren't horizontal/vert) */
    /*  where, due to rounding errors, something which SHOULD be on the edge*/
    /*  will appear to be inside */
    int i, last_h, next_h;

    if ( c->cur==0 )
return;

    last_h = c->open ? false : c->all[c->cur-1].left_hidden;
    for ( i=0; i<c->cur; ++i ) {
	StrokePoint *p = &c->all[i];
	next_h = (i<c->cur-1) ? p[1].left_hidden : c->open ? false : c->all[0].left_hidden;
	if ( last_h && !p->left_hidden ) {
	    if ( next_h )
		p->left_hidden = true;
	    else if ( i<c->cur-2 && !next_h && p[2].left_hidden &&
		    (p->t==0 || p->t==1 || p[1].t==0 || p[1].t==1)) {
		p->left_hidden = true;
		p[1].left_hidden = true;
	    }
	} else if ( !last_h && p->left_hidden && !next_h )
	    p->left_hidden = false;
	if ( !p->left_hidden ) {
	    /* If two points are at the same location and one is hidden */
	    /*  and the other isn't, then hide both. Note, must be careful */
	    /*  because location data isn't always meaningful for hidden pts */
	    /*  hence the check on "hide_?_if_on_edge" */
	    if ( i>0 && p[-1].left_hidden && p[-1].hide_left_if_on_edge &&
		    p[-1].left.x == p->left.x && p[-1].left.y==p->left.y )
		p->left_hidden = true;
	    else if ( i<c->cur-1 && p[1].left_hidden && p[1].hide_left_if_on_edge &&
		    p[1].left.x == p->left.x && p[1].left.y==p->left.y )
		p->left_hidden = true;
	}
	    
	last_h = p->left_hidden;
    }

    last_h = c->open ? false : c->all[c->cur-1].right_hidden;
    for ( i=0; i<c->cur; ++i ) {
	StrokePoint *p = &c->all[i];
	next_h = (i<c->cur-1) ? p[1].right_hidden : c->open ? false : c->all[0].right_hidden;
	if ( last_h && !p->right_hidden ) {
	    if ( next_h )
		p->right_hidden = true;
	    else if ( i<c->cur-2 && !next_h && p[2].right_hidden &&
		    (p->t==0 || p->t==1 || p[1].t==0 || p[1].t==1)) {
		p->right_hidden = true;
		p[1].right_hidden = true;
	    }
	} else if ( !last_h && p->right_hidden && !next_h )
	    p->right_hidden = false;
	last_h = p->right_hidden;
    }
}

static void AddUnhiddenPoints(StrokeContext *c) {
    /* Now when we have some hidden points on one side, (and it isn't at a */
    /*  known joint) then it's probably a good idea to put a point on the  */
    /*  other side (if one isn't there already) to stablize it, because our*/
    /*  approximation routines can get local maxima -- at the right place  */
    /*  but too much or too little */
    int start, end, mid;
    int i, any;
    bigreal xdiff, ydiff;

    if ( c==NULL || c->all==NULL )
return;
    start=0;
    if ( c->all[0].left_hidden )
	for ( start=0; start<c->cur && c->all[start].left_hidden; ++start );
    /* Not interested in a hidden section around 0 because there will be an on- */
    /*  curve point there already */
    while ( start<c->cur ) {
	while ( start<c->cur && !c->all[start].left_hidden )
	    ++start;
	if ( start>=c->cur )
    break;
	for ( end=start; end<c->cur && c->all[end].left_hidden; ++end );
	if ( end>=c->cur )
    break;			/* again, this would wrap around to 0 so not interesting */
	--start;
	mid = (start+end)/2;
	/* See if already at a joint */
	any = false;
	for ( i=1; ; ++i ) {
	    int back, fore;
	    if ( mid-i>0 ) {
		if ( c->all[mid-i].line || c->all[mid-i].circle || c->all[mid-i].needs_point_right ) {
		    any = true;
	break;
		}
		xdiff = c->all[mid-i].me.x-c->all[mid].me.x;
		ydiff = c->all[mid-i].me.y-c->all[mid].me.y;
		back = (xdiff*xdiff + ydiff*ydiff > 100 );
	    } else
		back = true;
	    if ( mid+i<c->cur ) {
		if ( c->all[mid+i].line || c->all[mid+i].circle || c->all[mid+i].needs_point_right ) {
		    any = true;
	break;
		}
		xdiff = c->all[mid+i].me.x-c->all[mid].me.x;
		ydiff = c->all[mid+i].me.y-c->all[mid].me.y;
		fore = (xdiff*xdiff + ydiff*ydiff > 100 );
	    } else
		fore = true;
	    if ( back && fore )
	break;
	}
	if ( !any )
	    c->all[mid].needs_point_right = true;
	start = end;
    }

    /* Same thing for the right side */
    start=0;
    if ( c->all[0].right_hidden )
	for ( start=0; start<c->cur && c->all[start].right_hidden; ++start );
    while ( start<c->cur ) {
	while ( start<c->cur && !c->all[start].right_hidden )
	    ++start;
	if ( start>=c->cur )
    break;
	for ( end=start; end<c->cur && c->all[end].right_hidden; ++end );
	if ( end>=c->cur )
    break;			/* again, this would wrap around to 0 so not interesting */
	--start;
	mid = (start+end)/2;
	/* See if already at a joint */
	any = false;
	for ( i=1; ; ++i ) {
	    int back, fore;
	    if ( mid-i>0 ) {
		if ( c->all[mid-i].line || c->all[mid-i].circle || c->all[mid-i].needs_point_left ) {
		    any = true;
	break;
		}
		xdiff = c->all[mid-i].me.x-c->all[mid].me.x;
		ydiff = c->all[mid-i].me.y-c->all[mid].me.y;
		back = (xdiff*xdiff + ydiff*ydiff > 100 );
	    } else
		back = true;
	    if ( mid+i<c->cur ) {
		if ( c->all[mid+i].line || c->all[mid+i].circle || c->all[mid+i].needs_point_left ) {
		    any = true;
	break;
		}
		xdiff = c->all[mid+i].me.x-c->all[mid].me.x;
		ydiff = c->all[mid+i].me.y-c->all[mid].me.y;
		fore = (xdiff*xdiff + ydiff*ydiff > 100 );
	    } else
		fore = true;
	    if ( back && fore )
	break;
	}
	if ( !any )
	    c->all[mid].needs_point_left = true;
	start = end;
    }
}

static void PointJoint(SplinePoint *base, SplinePoint *other, bigreal resolution) {
    BasePoint inter, off;
    SplinePoint *hasnext, *hasprev;
    bigreal xdiff, ydiff, len, len2;
    int bad = false;

    if ( other->next==NULL && other->prev==NULL ) {
	/* Fragments consisting of a single point do happen */ /* Unfortunately*/
	SplinePointFree(other);
return;
    }

    if ( base->next==NULL ) {
	hasnext = other;
	hasprev = base;
	base->next = other->next;
	base->next->from = base;
	base->nextcp = other->nextcp;
	base->nonextcp = other->nonextcp;
    } else {
	hasnext = base;
	hasprev = other;
	base->prev = other->prev;
	base->prevcp = other->prevcp;
	base->noprevcp = other->noprevcp;
	base->prev->to = base;
    }
    /* We probably won't have exactly the same location on each side to be joined */
    /* No matter. Extend in the same direction and find the point of intersection */
    /*  of the two slopes */
    if ( !IntersectLines(&inter,
	    &hasnext->me,hasnext->nonextcp ? &hasnext->next->to->me:&hasnext->nextcp,
	    &hasprev->me,hasprev->noprevcp ? &hasprev->prev->from->me:&hasprev->prevcp)) {
	bad = true;
    } else {
	xdiff = inter.x-base->me.x; ydiff=inter.y-base->me.y;
	len = xdiff*xdiff + ydiff*ydiff;
	if ( len > 9*resolution*resolution )
	    bad = true;
    }
    /* Well, if the intersection is extremely far away, then just average the */
    /*  two points */
    if ( bad ) {
	inter.x = (hasnext->me.x+hasprev->me.x)/2;
	inter.y = (hasnext->me.y+hasprev->me.y)/2;
    }

    /* Adjust control points for the new position */
    off.x = hasnext->nextcp.x - hasnext->me.x;
    off.y = hasnext->nextcp.y - hasnext->me.y;
    xdiff = hasnext->next->to->me.x - hasnext->me.x;
    ydiff = hasnext->next->to->me.y - hasnext->me.y;
    len = xdiff*xdiff + ydiff*ydiff;
    xdiff = hasnext->next->to->me.x - inter.x;
    ydiff = hasnext->next->to->me.y - inter.y;
    len2 = xdiff*xdiff + ydiff*ydiff;
    if ( len!=0 && len2>len ) {
	len = sqrt(len2/len);
	off.x *= len;
	off.y *= len;
    }
    base->nextcp.x = inter.x + off.x;
    base->nextcp.y = inter.y + off.y;

    /* And the prevcp */
    off.x = hasprev->prevcp.x - hasprev->me.x;
    off.y = hasprev->prevcp.y - hasprev->me.y;
    xdiff = hasprev->prev->from->me.x - hasprev->me.x;
    ydiff = hasprev->prev->from->me.y - hasprev->me.y;
    len = xdiff*xdiff + ydiff*ydiff;
    xdiff = hasprev->prev->from->me.x - inter.x;
    ydiff = hasprev->prev->from->me.y - inter.y;
    len2 = xdiff*xdiff + ydiff*ydiff;
    if ( len!=0 && len2>len ) {
	len = sqrt(len2/len);
	off.x *= len;
	off.y *= len;
    }
    base->prevcp.x = inter.x + off.x;
    base->prevcp.y = inter.y + off.y;

    base->me = inter;
    SplineRefigure(base->next);
    SplineRefigure(base->prev);
    SplinePointCategorize(base);
    SplinePointFree(other);
}

static SplineSet *JoinFragments(SplineSet *fragments,SplineSet **contours,
	bigreal resolution) {
    bigreal res2 = resolution*resolution;
    SplineSet *prev, *cur, *test, *test2, *next, *prev2;
    bigreal xdiff, ydiff;

    /* Remove any single point fragments */
    prev=NULL;
    for ( cur=fragments; cur!=NULL; cur=next ) {
	next = cur->next;
	if ( cur->first==cur->last && cur->first->prev==NULL ) {
	    SplinePointListFree(cur);
	    if ( prev==NULL )
		fragments = next;
	    else
		prev->next = next;
	} else
	    prev = cur;
    }
    prev=NULL;
    for ( cur=fragments; cur!=NULL; cur=next ) {
	next = cur->next;
	prev2 = prev; test2 = NULL;
	for ( test=cur; test!=NULL; prev2=test, test=test->next ) {
	    xdiff = cur->last->me.x - test->first->me.x;
	    ydiff = cur->last->me.y - test->first->me.y;
	    if ( xdiff*xdiff + ydiff*ydiff <= res2 )
	break;
	}
	if ( test==cur &&
		( test->first->next==NULL || test->first->next->to == test->last )) {
	    /* The fragment is smaller than the resolution, so just get rid of it */
	    if ( prev==NULL )
		fragments = next;
	    else
		prev->next = next;
	    cur->next = NULL;
	    SplinePointListFree(cur);
    continue;
	}
	if ( test==NULL ) {
	    prev2 = prev;
	    for ( test2=cur; test2!=NULL; prev2=test2, test2=test2->next ) {
		xdiff = cur->first->me.x - test2->last->me.x;
		ydiff = cur->first->me.y - test2->last->me.y;
		if ( xdiff*xdiff + ydiff*ydiff <= res2 )
	    break;
	    }
	}
	if ( test!=NULL || test2!=NULL ) {
	    if ( test!=NULL ) {
		PointJoint(cur->last,test->first,resolution);
		if ( cur==test ) {
		    cur->first = cur->last;
		    cur->start_offset = 0;
		} else
		    cur->last = test->last;
	    } else {
		PointJoint(cur->first,test2->last,resolution);
		if ( cur==test2 )
		    cur->last = cur->first;
		else {
		    cur->first = test2->first;
		    cur->start_offset = 0;
		}
		test = test2;
	    }
	    if ( cur!=test ) {
		test->first = test->last = NULL;
		cur->start_offset = 0;
		prev2->next = test->next;
		if ( next==test )
		    next = test->next;
		SplinePointListFree(test);

		xdiff = cur->last->me.x - cur->first->me.x;
		ydiff = cur->last->me.y - cur->first->me.y;
		if ( xdiff*xdiff + ydiff*ydiff <= res2 ) {
		    PointJoint(cur->first,cur->last,resolution);
		    cur->last= cur->first;
		}
	    }
	    if ( cur->last==cur->first ) {
		if ( prev==NULL )
		    fragments = next;
		else
		    prev->next = next;
		cur->next = *contours;
		*contours = cur;
	    } else
		prev = cur;
	} else
	    prev = cur;
    }
return( fragments );
}

static int Linelike(SplineSet *ss,bigreal resolution) {
    BasePoint slope;
    bigreal len, dot;
    Spline *s;

    if ( ss->first->prev!=NULL )
return( false );

    if ( ss->first->next == ss->last->prev )
return( true );

    slope.x = ss->last->me.x-ss->first->me.x;
    slope.y = ss->last->me.y-ss->first->me.y;
    len = slope.x*slope.x + slope.y*slope.y;
    if ( len==0 )
return( false );
    len = sqrt(len);
    slope.x/=len; slope.y/=len;
    for ( s=ss->first->next; s!=NULL; s=s->to->next ) {
	dot = slope.y*(s->from->nextcp.x-ss->first->me.x) -
		slope.x*(s->from->nextcp.y-ss->first->me.y);
	if ( dot>resolution || dot<-resolution )
return( false );
	dot = slope.y*(s->to->prevcp.x-ss->first->me.x) -
		slope.x*(s->to->prevcp.y-ss->first->me.y);
	if ( dot>resolution || dot<-resolution )
return( false );
	dot = slope.y*(s->to->me.x-ss->first->me.x) -
		slope.x*(s->to->me.y-ss->first->me.y);
	if ( dot>resolution || dot<-resolution )
return( false );
    }
return( true );
}

/* This routine is a set of hacks to handle things I can't figure out how to */
/*  fix the right way */
static SplineSet *EdgeEffects(SplineSet *fragments,StrokeContext *c) {
    SplineSet *cur, *test, *prev, *next;
    bigreal r2;
    Spline *s;

/* If we have a very thin horizontal stem, so thin that the inside stroke is */
/*  completely hidden, and if at the end of the stroke (say the main vertical*/
/*  stem of E) the two edges end at the same point, then we are left with two*/
/*  disconnected lines which overlap. The points don't get hidden because they*/
/*  are on the edge of the square pen, rather than inside it */

/* This has problems when resolution<1 */
    for ( cur=fragments; cur!=NULL; cur=cur->next ) {
	SSRemoveColinearPoints(cur);
	if ( !cur->last->prev->knownlinear )
    continue;
	for ( test=fragments; test!=NULL; test=test->next ) {
	    if ( !test->first->next->knownlinear )
	continue;
	    if ( BpColinear(&cur->last->me,&test->first->me,&cur->last->prev->from->me) &&
		    BpColinear(&test->first->me,&cur->last->me,&test->first->next->to->me) ) {
		test->first->me = cur->last->me;
		test->first->nextcp = cur->last->me;
		test->first->nonextcp = true;
		test->first->next->to->prevcp = test->first->next->to->me;
		test->first->next->to->noprevcp = true;
		SplineRefigure(test->first->next);
	break;
	    }
	}
    }

    /* We can also get single small segments where the other side gets hidden */
    /* If we get to this point we know there isn't a good match, so just toss 'em */
    prev = NULL;
    for ( cur=fragments; cur!=NULL; cur=next ) {
	next = cur->next;
	if ( !Linelike(cur,1.0) )
	    prev = cur;
	else {
	    if ( prev==NULL )
		fragments = next;
	    else
		prev->next = next;
	    cur->next = NULL;
	    SplinePointListFree(cur);
	}
    }

    /* And sometimes we are just missing a chunk */
    r2 = ( c->pentype==pt_poly ) ? c->longest_edge : 2*c->radius;
    r2 += 2*c->resolution;		/* Add a little fudge */
    r2 *= r2;
    for ( cur=fragments; cur!=NULL; cur=cur->next ) {
	bigreal xdiff = cur->last->me.x - cur->first->me.x;
	bigreal ydiff = cur->last->me.y - cur->first->me.y;
	bigreal len = xdiff*xdiff + ydiff*ydiff;
	if ( len!=0 && len <= r2 ) {
	    SplinePoint *sp = SplinePointCreate(cur->first->me.x,cur->first->me.y);
	    SplineMake3(cur->last,sp);
	    cur->last = sp;
	} else if ( len!=0 ) {
	    bigreal t = -1;
	    for ( s= cur->last->prev; s!=NULL && s!=cur->first->next; s=s->from->prev ) {
		t = SplineNearPoint(s,&cur->first->me,c->resolution);
		if ( t!=-1 ) {
		    SplinePoint *sp = SplineBisect(s,t), *sp2;
		    sp2 = chunkalloc(sizeof(SplinePoint));
		    *sp2 = *sp;
		    sp->next = NULL;
		    sp2->prev = NULL;
		    sp2->next->from = sp2;
		    next = chunkalloc(sizeof(SplineSet));
		    *next = *cur;
		    cur->last = sp;
		    next->first = sp2;
		    cur->next = next;
	    break;
		}
	    }
	    if ( t==-1 ) {
		for ( s= cur->first->next; s!=NULL && s!=cur->last->prev; s=s->to->next ) {
		    t = SplineNearPoint(s,&cur->last->me,c->resolution);
		    if ( t!=-1 ) {
			SplinePoint *sp = SplineBisect(s,t), *sp2;
			sp2 = chunkalloc(sizeof(SplinePoint));
			*sp2 = *sp;
			sp->prev = NULL;
			sp2->next = NULL;
			sp2->prev->to = sp2;
			next = chunkalloc(sizeof(SplineSet));
			*next = *cur;
			cur->first = sp; cur->start_offset = 0;
			next->last = sp2;
			cur->next = next;
		break;
		    }
		}
	    }
	}
    }
return( fragments );
}

static int ReversedLines(Spline *line1,Spline *line2, SplinePoint **start, SplinePoint **end) {
    BasePoint slope1, slope2;
    bigreal len1, len2, normal_off;
    int f1,f2,t1,t2;

    slope1.x = line1->to->me.x-line1->from->me.x;
    slope1.y = line1->to->me.y-line1->from->me.y;
    slope2.x = line2->to->me.x-line2->from->me.x;
    slope2.y = line2->to->me.y-line2->from->me.y;
    if ( slope1.x*slope2.x + slope1.y*slope2.y >= 0 )
return( false );		/* Lines go vaguely in the same direction, not reversed */
    len1 = sqrt(slope1.x*slope1.x + slope1.y*slope1.y);
    len2 = sqrt(slope2.x*slope2.x + slope2.y*slope2.y);
    if ( len1==0 || len2==0 )
return( false );		/* zero length lines are just points. Ignore */

    normal_off = slope1.x*slope2.y/len2 - slope1.y*slope2.x/len2;
    if ( normal_off<-.1 || normal_off>.1 )
return( false );		/* Not parallel */
    normal_off = slope2.x*slope1.y/len1 - slope2.y*slope1.x/len1;
    if ( normal_off<-.1 || normal_off>.1 )
return( false );		/* Not parallel */

    /* OK, the lines are parallel, and point in the right direction, but do they */
    /*  intersect? */
    f2 = BpWithin(&line1->from->me,&line2->from->me,&line1->to->me);
    t2 = BpWithin(&line1->from->me,&line2->to->me,&line1->to->me);
    if ( f2 && t2 ) {
	*start = line2->to;
	*end = line2->from;
return( true );
    }
    f1 = BpWithin(&line2->from->me,&line1->from->me,&line2->to->me);
    if ( f1 && f2 ) {
	*start = line1->from;
	*end = line2->from;
return( true );
    } else if ( f1 && t2 ) {
	*start = line1->from;
	*end = line2->to;
return( true );
    }
    t1 = BpWithin(&line2->from->me,&line1->to->me,&line2->to->me);
    if ( f1 && t1 ) {
	*start = line1->from;
	*end = line1->to;
return( true );
    } else if ( t1 && f2 ) {
	*start = line2->from;
	*end = line1->to;
return( true );
    } else if ( t1 && t2 ) {
	*start = line2->to;
	*end = line1->to;
return( true );
    }

return( false );
}

static SplinePoint *SplineInsertPoint(Spline *s,SplinePoint *sp) {
    SplinePoint *to = s->to;

    s->from->nonextcp = true; to->noprevcp = true;
    if ( sp->me.x==s->from->me.x && sp->me.y==s->from->me.y )
return( s->from );
    if ( sp->me.x==to->me.x && sp->me.y==to->me.y )
return( to );

    sp = SplinePointCreate(sp->me.x,sp->me.y);
    s->to = sp;
    sp->prev = s;
    SplineMake3(sp,to);
return( sp );
}

static void BreakLine(Spline *s,SplinePoint *begin,SplinePoint *end,
	SplinePoint **newbegin,SplinePoint **newend) {
    if ( s->from->me.x==begin->me.x && s->from->me.y==begin->me.y )
	*newbegin = s->from;
    else {
	*newbegin = SplineInsertPoint(s,begin);
	s = (*newbegin)->next;
    }
    if ( s->to->me.x==end->me.x && s->to->me.y==end->me.y )
	*newend = s->to;
    else {
	*newend = SplineInsertPoint(s,end);
    }
}

static SplineSet *RemoveBackForthLine(SplineSet *ss) {
    SplinePoint *sp, *start, *end/*, *nsp, *psp*/;
    SplineSet *other, *test;
    SplinePoint *sp2, *first1, *first2, *second1, *second2;
    /*int removed = true;*/

    ss->next = NULL;

    SSRemoveColinearPoints(ss);

    if ( ss->first->prev==NULL )
return( ss );
    if ( ss->first->next->to == ss->first && ss->first->next->knownlinear ) {
	/* Entire splineset is a single point */ /* Remove it all */
	SplinePointListFree(ss);
return( NULL );
    }

    /* OK, here we've gotten rid of all the places where the line doubles back */
    /*  mirrored by one of its end-points. But there could be line segments */
    /*  which double back on another segment somewhere else in the contour. */
    /* Removing them means splitting the contour in two */
    for ( sp=ss->first; ; ) {
	if ( sp->next->knownlinear ) for ( sp2=sp->next->to ;; ) {
	    if (sp2->next->knownlinear &&
		    ReversedLines(sp->next,sp2->next, &start, &end)) {
		int isnext=sp->next->to==sp2, isprev=sp->prev->from==sp2;
		BreakLine(sp->next,start,end,&first1,&second1);
		BreakLine(sp2->next,end,start,&first2,&second2);
		if ( first1->me.x!=second2->me.x || first1->me.y!=second2->me.y ) {
		    IError("Confusion reighns!");
return( ss );
		}
		if ( second1->me.x!=first2->me.x || second1->me.y!=first2->me.y ) {
		    IError("Confusion regnas!");
return( ss );
		}
		if ( isnext || isprev ) {
		    /* SSRemoveColinearPoints should have caught this but */
		    /*  it has slightly different rounding conditions and sometimes doesn't */
		    if ( !isnext ) {
			SplinePoint *t = sp2;
			sp2 = sp;
			sp = t;
			second2 = second1;
			first1 = first2;
		    }
		    if ( second1 !=sp2 || first2!=sp2 ) {
			IError("Confusion wiggles!\n");
return(ss);
		    }
		    SplineFree(sp2->prev);
		    SplineFree(sp2->next);
		    SplinePointFree(sp2);
		    first1->next = second2->next;
		    first1->nextcp = second2->nextcp;
		    first1->nonextcp = second2->nonextcp;
		    first1->next->from = first1;
		    SplinePointFree(second2);
	break;
		}
		SplineFree(first1->next);
		SplineFree(first2->next);
		other = chunkalloc(sizeof(SplineSet));
		other->first = other->last = second1;
		second1->prev = first2->prev;
		second1->prevcp = first2->prevcp;
		second1->noprevcp = first2->noprevcp;
		second1->prev->to = second1;
		SplinePointFree(first2);
		first1->next = second2->next;
		first1->nextcp = second2->nextcp;
		first1->nonextcp = second2->nonextcp;
		first1->next->from = first1;
		SplinePointFree(second2);
		ss->first = ss->last = first1;
		ss->start_offset = 0;
		ss = RemoveBackForthLine(ss);
		other = RemoveBackForthLine(other);
		if ( ss==NULL )
		    ss = other;
		else {
		    for ( test=ss; test->next!=NULL; test=test->next );
		    test->next = other;
		}
return( ss );
	    }
	    sp2 = sp2->next->to;
	    if ( sp2==ss->first )
	break;
	}
	sp = sp->next->to;
	if ( sp==ss->first )
    break;
    }

return( ss );
}

#if 0
// If there are odd crashes, double-frees, or bad accesses dealing with strokes, try looking for duplicate points and contours.

struct pointer_list_item;
struct pointer_list_item {
	void *item;
	struct pointer_list_item *next;
};
void pointer_list_item_free_chain(struct pointer_list_item *start) {
	struct pointer_list_item *curr = start;
	struct pointer_list_item *next;
	while (curr != NULL) {
		next = curr->next;
		free(curr);
		curr = next;
	}
	return;
}
struct pointer_list_item *pointer_list_item_get(struct pointer_list_item *start, void *item) {
	struct pointer_list_item *curr = start;
	struct pointer_list_item *next;
	while (curr != NULL) {
		next = curr->next;
		if (item == curr->item) return curr;
		curr = next;
	}
	return NULL;
}
struct pointer_list_item *pointer_list_item_add(struct pointer_list_item *start, void *item) {
	struct pointer_list_item *curr = calloc(1, sizeof(struct pointer_list_item));
	curr->item = item;
	curr->next = start;
	return curr;
}
static int SplineSetFindDupes(SplineSet *contours) {
	struct pointer_list_item *points = NULL;
	struct pointer_list_item *paths = NULL;
	SplineSet *contour_curr = contours;
	int err = 0;
	int path_cnt = 0;
	int point_cnt = 0;
	while (contour_curr != NULL) {
		if (pointer_list_item_get(paths, contour_curr)) {
			fprintf(stderr, "Duplicate path!\n");
			err |= 1;
		} else {
			paths = pointer_list_item_add(paths, contour_curr);
		}
		SplinePoint *point_curr = contour_curr->first;
		int point_local_cnt = 0;
		while (point_curr != NULL) {
			if (pointer_list_item_get(points, point_curr)) {
				fprintf(stderr, "Duplicate point!\n");
				err |= 1;
			} else {
				points = pointer_list_item_add(points, point_curr);
			}
			if (point_curr->next == NULL || point_curr->next->to == contour_curr->last) break;
			point_curr = point_curr->next->to;
		}
		contour_curr = contour_curr->next;
	}
	pointer_list_item_free_chain(points);
	pointer_list_item_free_chain(paths);
	return err;
}
#endif // 0

static SplineSet *SSRemoveBackForthLine(SplineSet *contours) {
    /* Similar to the above. If we have a stem which is exactly 2*radius wide */
    /*  then we will have a line running down the middle of the stem which */
    /*  encloses no area. Get rid of it. More complicated cases can occur (a */
    /*  plus sign where each stem is 2*radius, ... */
    SplineSet *prev, *next, *cur, *ret;
		SplineSet *cur_tmp;

    prev = NULL;
    for ( cur=contours; cur!=NULL; cur=next ) {
	next = cur->next;
	// ret = RemoveBackForthLine(cur);
	// In order to use SplineSetRemoveOverlap, we need to break the SplineSet down into paths.
	// Otherwise, all of them get merged.
	cur_tmp = calloc(1, sizeof(SplineSet));
	cur_tmp->next = NULL;
	cur_tmp->first = cur->first;
	cur_tmp->last = cur->last;
	ret = SplineSetRemoveOverlap(NULL, cur_tmp, over_remove);
	if ( ret==NULL ) {
	    if ( prev==NULL )
		contours = next;
	    else
		prev->next = next;
	} else if ( cur!=ret ) {
	    if ( prev==NULL )
		contours = ret;
	    else
		prev->next = ret;
	}
	if ( ret!=NULL ) {
	    while ( ret->next!=NULL )
		ret = ret->next;
	    prev = ret;
	    ret->next = next;
	}
    }
return( contours );
}

#define MAX_TPOINTS	40

static int InterpolateTPoints(StrokeContext *c,int start_pos,int end_pos,
	int isleft) {
    /* We have a short segment of the contour here. So short that there are */
    /*  very few points StrokePoints on it. If we've only got 1 StrokePoint */
    /*  we tend to get singular matrices. 2 StrokePoints just gives a bad */
    /*  approximation (or might), etc. */
    /* But we have the original spline. We can add as many points between */
    /*  start and end as we like */
    bigreal start_t, end_t, t, diff_t;
    bigreal start_x, end_x, x, diff_x, start_y, end_y, y, diff_y, r2;
    BasePoint slope, me;
    Spline *s;
    int lt,rt;
    int i,j;
    bigreal len;

    if ( start_pos==0 )
return( end_pos-start_pos );

    if ( 20 >= c->tmax )
	c->tpt = realloc(c->tpt,(c->tmax = 20+MAX_TPOINTS)*sizeof(TPoint));

    if ( c->all[start_pos].line ) {
	me = c->all[start_pos-1].me;
	if ( isleft ) {
	    slope.x = (c->all[end_pos].left.x - me.x)/6;
	    slope.y = (c->all[end_pos].left.y - me.y)/6;
	} else {
	    slope.x = (c->all[end_pos].right.x - me.x)/6;
	    slope.y = (c->all[end_pos].right.y - me.y)/6;
	}
	for ( i=0; i<5; ++i ) {
	    c->tpt[i].x = me.x + slope.x*(i+1);
	    c->tpt[i].y = me.y + slope.y*(i+1);
	    c->tpt[i].t = (i+1)/6.0;
	}
return( 5 );
    } else if ( c->all[start_pos].circle ) {
	me = c->all[start_pos].me;		/* Center */
	if ( isleft ) {
	    start_x = c->all[start_pos-1].left.x - me.x;
	    end_x = c->all[end_pos].left.x - me.x;
	    start_y = c->all[start_pos-1].left.y - me.y;
	    end_y = c->all[end_pos].left.y - me.y;
	} else {
	    start_x = c->all[start_pos-1].right.x - me.x;
	    end_x = c->all[end_pos].right.x - me.x;
	    start_y = c->all[start_pos-1].right.y - me.y;
	    end_y = c->all[end_pos].right.y - me.y;
	}
	if ( (diff_x = end_x-start_x)<0 ) diff_x = -diff_x;
	if ( (diff_y = end_y-start_y)<0 ) diff_y = -diff_y;
	/* By choosing the bigger difference, I insure the other coord will */
	/*  not cross over from negative to positive. Actually this is only */
	/*  true for small segments of the circle. But that's what we've got*/
	r2 = c->radius*c->radius;
	if ( diff_y>diff_x ) {
	    diff_y = (end_y-start_y)/11.0;
	    for ( y=start_y+diff_y, i=0; i<10; ++i, y+=diff_y ) {
		x = sqrt( r2-y*y );
		if ( start_x<0 ) x=-x;
		c->tpt[i].x = me.x + x;
		c->tpt[i].y = me.y + y;
		c->tpt[i].t = (i+1)/11.0;
	    }
	} else {
	    diff_x = (end_x-start_x)/11.0;
	    for ( x=start_x+diff_x, i=0; i<10; ++i, x+=diff_x ) {
		y = sqrt( r2-x*x );
		if ( start_y<0 ) y=-y;
		c->tpt[i].x = me.x + x;
		c->tpt[i].y = me.y + y;
		c->tpt[i].t = (i+1)/11.0;
	    }
	}
return( 10 );
    }

    if ( c->all[start_pos-1].t == c->all[end_pos].t ||
	    c->all[start_pos-1].sp!=c->all[end_pos].sp ||
	    ( isleft && c->all[start_pos-1].lt!=c->all[end_pos].lt) ||
	    (!isleft && c->all[start_pos-1].rt!=c->all[end_pos].rt))
return( end_pos-start_pos );		/* Well, nothing we can do here */

    start_t = c->all[start_pos-1].t; end_t = c->all[end_pos].t;
    diff_t = (end_t-start_t)/11;
    s = c->all[start_pos].sp;
    lt = c->all[start_pos].lt; rt = c->all[start_pos].rt;
    for ( t=start_t+diff_t, j=i=0; i<10; ++i, t+=diff_t ) {
	me.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	me.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
	slope.x = (3*s->splines[0].a*t+2*s->splines[0].b)*t+s->splines[0].c;
	slope.y = (3*s->splines[1].a*t+2*s->splines[1].b)*t+s->splines[1].c;
	len = slope.x*slope.x + slope.y*slope.y;
	if ( len==0 && c->pentype==pt_circle )
    continue;
	len = sqrt(len);
	slope.x /= len; slope.y /= len;
	if ( isleft ) {
	    if ( c->pentype==pt_circle ) {
		c->tpt[j].x = me.x - c->radius*slope.y;
		c->tpt[j].y = me.y + c->radius*slope.x;
	    } else if ( c->pentype==pt_square ) {
		c->tpt[j].x = me.x + c->radius*SquareCorners[lt].x;
		c->tpt[j].y = me.y + c->radius*SquareCorners[lt].y;
	    } else {
		c->tpt[j].x = me.x + c->corners[lt].x;
		c->tpt[j].y = me.y + c->corners[lt].y;
	    }
	    c->tpt[j++].t = (i+1)/11.0;
	} else {
	    if ( c->pentype==pt_circle ) {
		c->tpt[j].x = me.x + c->radius*slope.y;
		c->tpt[j].y = me.y - c->radius*slope.x;
	    } else if ( c->pentype==pt_square ) {
		c->tpt[j].x = me.x + c->radius*SquareCorners[rt].x;
		c->tpt[j].y = me.y + c->radius*SquareCorners[rt].y;
	    } else {
		c->tpt[j].x = me.x + c->corners[rt].x;
		c->tpt[j].y = me.y + c->corners[rt].y;
	    }
	    c->tpt[j++].t = (i+1)/11.0;
	}
    }
return( j );
}

static SplineSet *ApproximateStrokeContours(StrokeContext *c) {
    int end_pos, i, start_pos=0, pos, ipos;
    SplinePoint *first=NULL, *last=NULL, *cur;
    SplineSet *ret=NULL;
    SplineSet *lfragments, *rfragments, *contours;
    int tot, jump, extras, skip, skip_cnt, newcontour;

    /* Normally there will be one contour along the left side, and one contour*/
    /*  on the right side. If the original were a closed contour, then each of*/
    /*  these new contours should also be closed. If an open contour then we  */
    /*  will probably want to join the two sides into one contour. BUT if the */
    /*  endpoints of an open contour are < radius apart, then the linecaps may*/
    /*  merge and we end up with the closed contour case again. */
    /* More serious, if a contour double backs on itself it may squeeze itself*/
    /*  out of existance so we might end up with several contour fragments. */
    /* So first collect fragments, then try to join them. */

    lfragments = NULL;

    /* handle the left side */
    if (( !c->remove_outer || c->open ) && !AllHiddenLeft(c) ) {
	pos = 0;
	while ( pos<c->cur-1 ) {
	    last = first = LeftPointFromContext(c,&pos, &newcontour);
	    while ( newcontour && pos<c->cur ) {
		SplinePointFree(first);
		start_pos = pos;
		if ( pos<c->cur )
		    last = first = LeftPointFromContext(c,&pos, &newcontour);
		else {
		    last = first = NULL;
	    break;
		}
		if ( pos<start_pos || (pos+start_pos==0) ) {
		    /* Wrapped around */
		    SplinePointFree(first);
		    first=last=NULL;
		    pos = c->cur;
	    break;
		}
	    }
	    for ( ; pos<c->cur-1 && !newcontour; ) {
		start_pos = pos+1;
		for ( i=start_pos; i<c->cur && !c->all[i].left_hidden && !c->all[i].needs_point_left; ++i);
		end_pos = pos = i;
		cur = LeftPointFromContext(c,&pos, &newcontour);
		if ( end_pos>start_pos && c->all[end_pos-1].left.x==cur->me.x && c->all[end_pos-1].left.y==cur->me.y )
		    --end_pos;
		tot = end_pos-start_pos;
		jump = 1;
		extras = 0; skip = MAX_TPOINTS;
		if ( tot > MAX_TPOINTS ) {
		    jump = (tot/MAX_TPOINTS);
		    extras = tot%MAX_TPOINTS;
		    skip = MAX_TPOINTS/(extras+1);
		    tot = MAX_TPOINTS;
		}
		if ( tot >= c->tmax )
		    c->tpt = realloc(c->tpt,(c->tmax = tot+MAX_TPOINTS)*sizeof(TPoint));
		/* There is really no point in having a huge number of data points */
		/*  I don't need 1000 points to approximate the curve, 10 will probably */
		/*  do. The extra points just slow us down (we need them for the */
		/*  Hide checks, but no longer) */
		ipos = start_pos; skip_cnt=0;
		for ( i=0; i<=tot; ++i ) {
		    TPoint *tpt = c->tpt + i;
		    StrokePoint *spt = c->all+ipos;
		    tpt->x = spt->left.x;
		    tpt->y = spt->left.y;
		    tpt->t = (ipos-start_pos+1)/ (bigreal) (end_pos-start_pos+1);
		    ipos += jump;
		    if ( ++skip_cnt>skip && extras>0 ) {
			++ipos;
			--extras;
			skip_cnt = 0;
		    }
		    if ( ipos>end_pos )
			ipos = end_pos;
		}
		if ( end_pos<start_pos+3 )
		    tot = InterpolateTPoints(c,start_pos,end_pos,true);
		if ( end_pos!=start_pos ) {
		    ApproximateSplineFromPointsSlopes(last,cur,c->tpt,tot,false);
		} else
		    SplineMake3(last,cur);
		last = cur;
		if ( pos<end_pos ) {
		    pos = c->cur+1;
	    break;
		}
	    }
	    if ( first!=NULL ) {
		ret = chunkalloc(sizeof(SplineSet));
		ret->first = first; ret->last = last;
		ret->next = lfragments;
		lfragments = ret;
	    }
	}
    }

    /* handle the right side */
    rfragments = NULL;
    if (( !c->remove_inner || c->open ) && !AllHiddenRight(c)) {
	pos = 0;
	while ( pos<c->cur-1 ) {
	    last = first = RightPointFromContext(c,&pos,&newcontour);
	    while ( newcontour && pos<c->cur ) {
		SplinePointFree(first);
		start_pos = pos;
		if ( pos<c->cur )
		    last = first = RightPointFromContext(c,&pos,&newcontour);
		else {
		    last = first = NULL;
	    break;
		}
		if ( pos<start_pos || (pos+start_pos==0) ) {
		    /* Wrapped around */
		    SplinePointFree(first);
		    first=last=NULL;
		    pos = c->cur;
	    break;
		}
	    }
	    for ( ; pos<c->cur-1 && !newcontour; ) {
		start_pos = pos+1;
		for ( i=start_pos; i<c->cur && !c->all[i].right_hidden && !c->all[i].needs_point_right; ++i);
		end_pos = pos = i;
		cur = RightPointFromContext(c,&pos,&newcontour);
		if ( c->all[end_pos-1].right.x==cur->me.x && c->all[end_pos-1].right.y==cur->me.y )
		    --end_pos;
		tot = end_pos-start_pos;
		jump = 1;
		extras = 0; skip = MAX_TPOINTS;
		if ( tot > MAX_TPOINTS ) {
		    jump = (tot/MAX_TPOINTS);
		    extras = tot%MAX_TPOINTS;
		    skip = MAX_TPOINTS/(extras+1);
		    tot = MAX_TPOINTS;
		}
		if ( tot >= c->tmax )
		    c->tpt = realloc(c->tpt,(c->tmax = tot+MAX_TPOINTS)*sizeof(TPoint));
		ipos = start_pos; skip_cnt=0;
		for ( i=0; i<=tot; ++i ) {
		    TPoint *tpt = c->tpt + i;
		    StrokePoint *spt = c->all+ipos;
		    tpt->x = spt->right.x;
		    tpt->y = spt->right.y;
		    tpt->t = (ipos-start_pos+1)/ (bigreal) (end_pos-start_pos+1);
		    ipos += jump;
		    if ( ++skip_cnt>skip && extras>0 ) {
			++ipos;
			--extras;
			skip_cnt = 0;
		    }
		    if ( ipos>end_pos )
			ipos = end_pos;
		}
		if ( end_pos<start_pos+3 )
		    tot = InterpolateTPoints(c,start_pos,end_pos,false);
		if ( end_pos!=start_pos ) {
		    ApproximateSplineFromPointsSlopes(last,cur,c->tpt,tot,false);
		} else
		    SplineMake3(last,cur);
		last = cur;
		if ( last->next!=NULL ) last=last->next->to;
		if ( pos<end_pos ) {
		    pos = c->cur+1;
	    break;
		}
	    }
	    if ( first!=NULL ) {
		ret = chunkalloc(sizeof(SplineSet));
		ret->first = first; ret->last = last;
		ret->next = rfragments;
		rfragments = ret;
	    }
	}
    }

    if ( c->open || !c->remove_outer ) {
	for ( ret=rfragments; ret!=NULL; ret=ret->next )
	    SplineSetReverse(ret);
    }

    contours = NULL;
    lfragments=JoinFragments(lfragments,&contours,0);
    rfragments=JoinFragments(rfragments,&contours,0);
    lfragments=JoinFragments(lfragments,&contours,c->resolution);
    rfragments=JoinFragments(rfragments,&contours,c->resolution);
    /* I handle lfragments and rfragments separately up to this point */
    /*  as an optimization. An lfragment should not join an rfragment */
    /*  (except in the case of an open contour which does not join itself)*/
    /*  so there are fewer things to check (n^2 check) if we do it separately*/
    /* But everything should join on the first pass, except for an open */
    /*  contour. So now forget about optimizations and just deal with any */
    /*  remainders in one big lump. Also catch the open contour case */
    if ( lfragments!=NULL ) {
	for ( ret=lfragments; ret->next!=NULL; ret=ret->next );
	ret->next = rfragments;
	lfragments=JoinFragments(lfragments,&contours,0);
    } else
	lfragments = rfragments;
    for ( i=2; lfragments!=NULL && i<c->radius/2; ++i )
	lfragments=JoinFragments(lfragments,&contours,i*c->resolution);
    lfragments = EdgeEffects(lfragments,c);
    lfragments=JoinFragments(lfragments,&contours,c->resolution);
    lfragments = EdgeEffects(lfragments,c);
    lfragments=JoinFragments(lfragments,&contours,c->resolution);
    if ( lfragments!=NULL ) {
	IError(glyphname==NULL?_("Some fragments did not join"):_("Some fragments did not join in %s"),glyphname);
	for ( ret=lfragments; ret->next!=NULL; ret=ret->next );
	ret->next = contours;
	contours = lfragments;
    }
    contours = SSRemoveBackForthLine(contours);

return( contours );
}

static SplineSet *SplineSet_Stroke(SplineSet *ss,struct strokecontext *c,
	int order2) {
    SplineSet *base;
    SplineSet *ret;

    base = SplinePointListCopy(ss);
    base = SSRemoveZeroLengthSplines(base);		/* Hard to get a slope for a zero length spline, that causes problems */
    if ( base==NULL )
return(NULL);
    if ( c->transform_needed )
	base = SplinePointListTransform(base,c->transform,tpt_AllPoints);
    if ( base->first->next==NULL )
	ret = SinglePointStroke(base->first,c);
    else {
	if ( c->cur!=0 )
	    memset(c->all,0,c->cur*sizeof(StrokePoint));
	c->cur = 0;
	c->ecur = 0;
	c->open = base->first->prev==NULL;
	switch ( c->pentype ) {
	  case pt_circle:
	    FindStrokePointsCircle(base, c);
	  break;
	  case pt_square:
	    FindStrokePointsSquare(base, c);
	  break;
	  case pt_poly:
	    FindStrokePointsPoly(base, c);
	  break;
	}
	HideSomeMorePoints(c);
	AddUnhiddenPoints(c);
	ret = ApproximateStrokeContours(c);
    }
    if ( c->transform_needed )
	ret = SplinePointListTransform(ret,c->inverse,tpt_AllPoints);
    if ( order2 )
	ret = SplineSetsConvertOrder(ret,order2 );
    SplinePointListFree(base);
return(ret);
}

static SplineSet *SplineSets_Stroke(SplineSet *ss,struct strokecontext *c,
	int order2) {
    SplineSet *first=NULL, *last=NULL, *cur;

    while ( ss!=NULL ) {
	cur = SplineSet_Stroke(ss,c,order2);
	if ( first==NULL )
	    first = last = cur;
	else
	    last->next = cur;
	if ( last!=NULL )
	    while ( last->next!=NULL )
		last = last->next;
	ss = ss->next;
    }
return( first );
}

SplineSet *SplineSetStroke(SplineSet *ss,StrokeInfo *si, int order2) {
    StrokeContext c;
    SplineSet *first, *last, *cur, *ret, *active = NULL, *anext;
    SplinePoint *sp, *nsp;
    int n, max;
    bigreal d2, maxd2, len, maxlen;
    DBounds b;
    BasePoint center;
    real trans[6];

    if ( si->stroke_type==si_centerline )
	IError("centerline not handled");

    memset(&c,0,sizeof(c));
    c.resolution = si->resolution;
    if ( si->resolution==0 )
	c.resolution = 1;
    c.pentype = si->stroke_type==si_std ? pt_circle :
		si->stroke_type==si_caligraphic ? pt_square :
		   pt_poly;
    c.join = si->join;
    c.cap  = si->cap;
    c.miterlimit = /* -cos(theta) */ -.98 /* theta=~11 degrees, PS miterlimit=10 */;
    c.radius = si->radius;
    c.radius2 = si->radius*si->radius;
    c.remove_inner = si->removeinternal;
    c.remove_outer = si->removeexternal;
    c.leave_users_center = si->leave_users_center;
    c.scaled_or_rotated = si->factor!=NULL;
    if ( c.pentype==pt_circle || c.pentype==pt_square ) {
	if ( si->minorradius==0 )
	    si->minorradius = si->radius;
	if ( si->minorradius!=si->radius ||
		(si->penangle!=0 && si->stroke_type!=si_std) ) {	/* rotating a circle is irrelevant (rotating an elipse means something) */
	    bigreal sn,co,factor;
	    c.transform_needed = true;
	    sn = sin(si->penangle);
	    co = cos(si->penangle);
	    factor = si->radius/si->minorradius;
	    c.transform[0] = c.transform[3] = co;
	    c.transform[1] = -sn;
	    c.transform[2] = sn;
	    c.transform[1] *= factor; c.transform[3] *= factor;
	    c.inverse[0] = c.inverse[3] = co;
	    c.inverse[1] = sn;
	    c.inverse[2] = -sn;
	    c.inverse[3] /= factor; c.inverse[2] /= factor;
	}
	if ( si->resolution==0 && c.resolution>c.radius/3 )
	    c.resolution = c.radius/3;
    if (c.resolution == 0) {
        ff_post_notice(_("Invalid stroke parameters"), _("Stroke resolution is zero"));
        return SplinePointListCopy(ss);
    }
	ret = SplineSets_Stroke(ss,&c,order2);
    } else {
	first = last = NULL;
	max = 0;
	memset(&center,0,sizeof(center));
	for ( active=si->poly; active!=NULL; active=active->next ) {
	    for ( sp=active->first, n=0; ; ) {
		++n;
		if ( sp->next==NULL )
return( NULL );				/* That's an error, must be closed */
		sp=sp->next->to;
		if ( sp==active->first )
	    break;
	    }
	    if ( n>max )
		max = n;
	}
	c.corners = malloc(max*sizeof(BasePoint));
	c.slopes  = malloc(max*sizeof(BasePoint));
	memset(trans,0,sizeof(trans));
	trans[0] = trans[3] = 1;
	if ( !c.leave_users_center ) {
	    SplineSetQuickBounds(si->poly,&b);
	    trans[4] = -(b.minx+b.maxx)/2;
	    trans[5] = -(b.miny+b.maxy)/2;
	    SplinePointListTransform(si->poly,trans,tpt_AllPoints);
	}
	for ( active=si->poly; active!=NULL; active=anext ) {
	    int reversed = false;
	    if ( !SplinePointListIsClockwise(active) ) {
		reversed = true;
		SplineSetReverse(active);
	    }
	    if ( !c.scaled_or_rotated ) {
		anext = active->next; active->next = NULL;
		SplineSetQuickBounds(active,&b);
		trans[4] = -(b.minx+b.maxx)/2;
		trans[5] = -(b.miny+b.maxy)/2;
		SplinePointListTransform(active,trans,tpt_AllPoints);	/* Only works if pen is fixed and does not rotate or get scaled */
		active->next = anext;
	    }
	    maxd2 = 0; maxlen = 0;
	    for ( sp=active->first, n=0; ; ) {
		nsp = sp->next->to;
		c.corners[n] = sp->me;
		c.slopes[n].x = nsp->me.x - sp->me.x;
		c.slopes[n].y = nsp->me.y - sp->me.y;
		len = c.slopes[n].x*c.slopes[n].x + c.slopes[n].y*c.slopes[n].y;
		len = sqrt(len);
		if ( len>maxlen ) maxlen = len;
		if ( len!=0 ) {
		    c.slopes[n].x/=len; c.slopes[n].y/=len;
		}
		d2 = sp->me.x*sp->me.x + sp->me.y*sp->me.y;
		if ( d2>maxd2 )
		    maxd2 = d2;
		++n;
		sp=nsp;
		if ( sp==active->first )
	    break;
	    }
	    c.n = n;
	    c.largest_distance2 = maxd2;
	    c.longest_edge = maxlen;
	    c.radius = sqrt(maxd2);
	    c.radius2 = maxd2;
	    if ( si->resolution==0 && c.resolution>c.radius/3 )
		c.resolution = c.radius/3;
        if (c.resolution == 0) {
            ff_post_notice(_("Invalid stroke parameters"), _("Stroke resolution is zero"));
            return SplinePointListCopy(ss);
        }
	    cur = SplineSets_Stroke(ss,&c,order2);
	    if ( !c.scaled_or_rotated ) {
		trans[4] = -trans[4]; trans[5] = -trans[5];
		SplinePointListTransform(cur,trans,tpt_AllPoints);
		anext = active->next; active->next = NULL;
		SplinePointListTransform(active,trans,tpt_AllPoints);
		active->next = anext;
	    }
	    if ( reversed ) {
		SplineSet *ss;
		for ( ss=cur; ss!=NULL; ss=ss->next )
		    SplineSetReverse(ss);
		SplineSetReverse(active);
	    }
	    if ( first==NULL )
		first = last = cur;
	    else
		last->next = cur;
	    if ( last!=NULL )
		while ( last->next!=NULL )
		    last = last->next;
	}
	free(c.corners);
	free(c.slopes);
	ret = first;
    }
    free(c.all);
    free(c.tpt);
return( ret );
}

#include "baseviews.h"

void FVStrokeItScript(void *_fv, StrokeInfo *si,int pointless_argument) {
    FontViewBase *fv = _fv;
    int layer = fv->active_layer;
    SplineSet *temp;
    int i, cnt=0, gid;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i ) if ( (gid=fv->map->map[i])!=-1 && fv->sf->glyphs[gid]!=NULL && fv->selected[i] )
	++cnt;
    ff_progress_start_indicator(10,_("Stroking..."),_("Stroking..."),0,cnt,1);

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( (gid=fv->map->map[i])!=-1 && (sc = fv->sf->glyphs[gid])!=NULL &&
		!sc->ticked && fv->selected[i] ) {
	    sc->ticked = true;
	    glyphname = sc->name;
	    if ( sc->parent->multilayer ) {
		SCPreserveState(sc,false);
		for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
		    temp = SplineSetStroke(sc->layers[layer].splines,si,sc->layers[layer].order2);
		    SplinePointListsFree( sc->layers[layer].splines );
		    sc->layers[layer].splines = temp;
		}
		SCCharChangedUpdate(sc,ly_all);
	    } else {
		SCPreserveLayer(sc,layer,false);
		temp = SplineSetStroke(sc->layers[layer].splines,si,sc->layers[layer].order2);
		SplinePointListsFree( sc->layers[layer].splines );
		sc->layers[layer].splines = temp;
		SCCharChangedUpdate(sc,layer);
	    }
	    if ( !ff_progress_next())
    break;
	}
    }
 glyphname = NULL;
    ff_progress_end_indicator();
}
