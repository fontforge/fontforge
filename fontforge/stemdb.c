/* Copyright (C) 2005,2006 by George Williams */
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
#include "pfaedit.h"
#include "edgelist2.h"
#include "stemdb.h"
#include <math.h>

static const double slope_error = .05;
    /* If the dot vector of a slope and the normal of another slope is less */
    /*  than this error, then we say they are close enough to be colinear */
static const double dist_error_hv = 1;
static const double dist_error_diag = 5;
    /* It's easy to get horizontal/vertical lines aligned properly */
    /* it is more difficult to get diagonal ones done */

struct st {
    Spline *s;
    double st, lt;
};

static void PointInit(struct glyphdata *gd,SplinePoint *sp, SplineSet *ss) {
    struct pointdata *pd;
    double len;

    if ( sp->ttfindex==0xffff )
return;
    pd = &gd->points[sp->ttfindex];
    pd->sp = sp;

    if ( sp->next==NULL ) {
	pd->nextunit.x = ss->first->me.x - sp->me.x;
	pd->nextunit.y = ss->first->me.y - sp->me.y;
	pd->nextlinear = true;
    } else if ( sp->next->knownlinear ) {
	pd->nextunit.x = sp->next->to->me.x - sp->me.x;
	pd->nextunit.y = sp->next->to->me.y - sp->me.y;
	pd->nextlinear = true;
    } else if ( sp->nonextcp ) {
	pd->nextunit.x = sp->next->to->prevcp.x - sp->me.x;
	pd->nextunit.y = sp->next->to->prevcp.y - sp->me.y;
    } else {
	pd->nextunit.x = sp->nextcp.x - sp->me.x;
	pd->nextunit.y = sp->nextcp.y - sp->me.y;
    }
    len = sqrt(pd->nextunit.x*pd->nextunit.x + pd->nextunit.y*pd->nextunit.y);
    if ( len==0 )
	pd->nextzero = true;
    else {
	pd->nextlen = len;
	pd->nextunit.x /= len;
	pd->nextunit.y /= len;
	if ( pd->nextunit.y==0 ) pd->next_hor = true;
	else if ( pd->nextunit.x==0 ) pd->next_ver = true;
    }

    if ( sp->prev==NULL ) {
	pd->prevunit.x = sp->me.x - ss->last->me.x;
	pd->prevunit.y = sp->me.y - ss->last->me.y;
	pd->prevlinear = true;
    } else if ( sp->prev->knownlinear ) {
	pd->prevunit.x = sp->me.x - sp->prev->from->me.x;
	pd->prevunit.y = sp->me.y - sp->prev->from->me.y;
	pd->prevlinear = true;
    } else if ( sp->noprevcp ) {
	pd->prevunit.x = sp->me.x - sp->prev->from->nextcp.x;
	pd->prevunit.y = sp->me.y - sp->prev->from->nextcp.y;
    } else {
	pd->prevunit.x = sp->me.x - sp->prevcp.x;
	pd->prevunit.y = sp->me.y - sp->prevcp.y;
    }
    len = sqrt(pd->prevunit.x*pd->prevunit.x + pd->prevunit.y*pd->prevunit.y);
    if ( len==0 )
	pd->prevzero = true;
    else {
	pd->prevlen = len;
	pd->prevunit.x /= len;
	pd->prevunit.y /= len;
	if ( pd->prevunit.y==0 ) pd->prev_hor = true;
	else if ( pd->prevunit.x==0 ) pd->prev_ver = true;
    }
    if ( pd->next_hor ) {
	pd->colinear = pd->prev_hor;
    } else if ( pd->next_ver ) {
	pd->colinear = pd->prev_ver;
    } else if ( !pd->prev_hor && !pd->prev_ver ) {
	double same = pd->prevunit.x*pd->nextunit.x + pd->prevunit.y*pd->nextunit.y;
	if ( same<-.95 )
	    pd->colinear = true;
    }
}

static int BBoxIntersectsLine(Spline *s,Spline *line) {
    double t,x,y;
    DBounds b;

    b.minx = b.maxx = s->from->me.x;
    b.miny = b.maxy = s->from->me.y;
    if ( s->to->me.x<b.minx ) b.minx = s->to->me.x;
    else if ( s->to->me.x>b.maxx ) b.maxx = s->to->me.x;
    if ( s->to->me.y<b.miny ) b.miny = s->to->me.y;
    else if ( s->to->me.y>b.maxy ) b.maxy = s->to->me.y;
    if ( s->to->prevcp.x<b.minx ) b.minx = s->to->prevcp.x;
    else if ( s->to->prevcp.x>b.maxx ) b.maxx = s->to->prevcp.x;
    if ( s->to->prevcp.y<b.miny ) b.miny = s->to->prevcp.y;
    else if ( s->to->prevcp.y>b.maxy ) b.maxy = s->to->prevcp.y;
    if ( s->from->nextcp.x<b.minx ) b.minx = s->from->nextcp.x;
    else if ( s->from->nextcp.x>b.maxx ) b.maxx = s->from->nextcp.x;
    if ( s->from->nextcp.y<b.miny ) b.miny = s->from->nextcp.y;
    else if ( s->from->nextcp.y>b.maxy ) b.maxy = s->from->nextcp.y;

    if ( line->splines[0].c!=0 ) {
	t = (b.minx-line->splines[0].d)/line->splines[0].c;
	y = line->splines[1].c*t+line->splines[1].d;
	if ( y>=b.miny && y<=b.maxy )
return( true );
	t = (b.maxx-line->splines[0].d)/line->splines[0].c;
	y = line->splines[1].c*t+line->splines[1].d;
	if ( y>=b.miny && y<=b.maxy )
return( true );
    }
    if ( line->splines[1].c!=0 ) {
	t = (b.miny-line->splines[1].d)/line->splines[1].c;
	x = line->splines[0].c*t+line->splines[0].d;
	if ( x>=b.minx && x<=b.maxx )
return( true );
	t = (b.maxy-line->splines[1].d)/line->splines[1].c;
	x = line->splines[0].c*t+line->splines[0].d;
	if ( x>=b.minx && x<=b.maxx )
return( true );
    }
return( false );
}

static int stcmp(const void *_p1, const void *_p2) {
    const struct st *stpt1 = _p1, *stpt2 = _p2;
    if ( stpt1->lt>stpt2->lt )
return( 1 );
    else if ( stpt1->lt<stpt2->lt )
return( -1 );

return( 0 );
}

static int LineType(struct st *st,int i, int cnt,Spline *line) {
    SplinePoint *sp;
    BasePoint nextcp, prevcp;
    double dn, dp;

    if ( st[i].st>.01 && st[i].st<.99 )
return( 0 );		/* Not near an end-point, just a normal line */
    if ( i+1>=cnt )
return( 0 );		/* No following spline */
    if ( st[i+1].st>.01 && st[i+1].st<.99 )
return( 0 );		/* Following spline not near an end-point, can't */
			/*  match to this one, just a normal line */
    if ( st[i].st<.5 && st[i+1].st>.5 ) {
	if ( st[i+1].s->to->next!=st[i].s )
return( 0 );
	sp = st[i].s->from;
    } else if ( st[i].st>.5 && st[i+1].st<.5 ) {
	if ( st[i].s->to->next!=st[i+1].s )
return( 0 );
	sp = st[i].s->to;
    } else
return( 0 );

    if ( !sp->nonextcp )
	nextcp = sp->nextcp;
    else
	nextcp = sp->next->to->me;
    if ( !sp->noprevcp )
	prevcp = sp->prevcp;
    else
	prevcp = sp->prev->from->me;
    nextcp.x -= sp->me.x; nextcp.y -= sp->me.y;
    prevcp.x -= sp->me.x; prevcp.y -= sp->me.y;

    dn = nextcp.x*line->splines[0].c + nextcp.y*line->splines[1].c;
    dp = prevcp.x*line->splines[0].c + prevcp.y*line->splines[1].c;
    if ( (dn<0 && dp<0) || (dn>0 && dp>0))
return( 1 );		/* Treat this line and the next as one */
    else
return( 2 );		/* Ignore both this line and the next */
}

static Spline *MonotonicFindAlong(Spline **sspace,Spline *line,struct st *stspace,
	SplinePoint *sp, double *other_t) {
    Spline *s;
    int i,j,cnt;
    BasePoint pts[9];
    double lts[10], sts[10];
    int eo;		/* I do horizontal/vertical by winding number */
			/* But figuring winding number with respect to a */
			/* diagonal line is hard. So I use even-odd */
			/* instead. */

    for ( i=j=0; (s=sspace[j])!=NULL; ++j ) {
	if ( BBoxIntersectsLine(s,line) ) {
	    /* Lines parallel to the direction we are testing just get in the */
	    /*  way and don't add any useful info */
	    if ( s->islinear &&
		    RealNear(line->splines[0].c*s->splines[1].c,
			    line->splines[1].c*s->splines[0].c))
    continue;
	    if ( SplinesIntersect(line,s,pts,lts,sts)<=0 )
    continue;
	    for ( j=0; sts[j]!=-1; ++j ) {
		if ( sts[j]>=0 && sts[j]<=1 ) {
		    stspace[i].s    = s;
		    stspace[i].lt   = lts[j];
		    stspace[i++].st = sts[j];
		}
	    }
	}
    }
    stspace[i].s = NULL;
    cnt = i;
    qsort(stspace,cnt,sizeof(struct st),stcmp);

    eo = 0;
    for ( i=0; i<cnt; ++i ) {
	s = stspace[i].s;
	if ( (s->to==sp && stspace[i].st>.99) || (s->from==sp && stspace[i].st<.01 )) {
	    if ( eo&1 ) {
		*other_t = stspace[i-1].st;
return( stspace[i-1].s );
	    }
	    if ( LineType(stspace,i,cnt,line)==0 )
		j=i+1;
	    else
		j=i+2;
	    if ( j<cnt ) {
		*other_t = stspace[j].st;
return( stspace[j].s );
	    }
	    fprintf( stderr, "MonotonicFindAlong: Ran out of intersections.\n" );
return( NULL );
	}
	switch ( LineType(stspace,i,cnt,line) ) {
	  case 0:	/* Normal spline */
	    ++eo;
	  break;
	  case 1:	/* Intersects at end-point & next entry is other side */
	    ++eo;	/*  And the two sides continue in approximately the   */
	    ++i;	/*  same direction */
	  break;
	  case 2:	/* Intersects at end-point & next entry is other side */
	    ++i;	/*  And the two sides go in opposite directions */
	  break;
	}
    }
    fprintf( stderr, "MonotonicFindAlong: Never found our point.\n" );
return( NULL );
}


static Spline *FindMatchingHVEdge(struct glyphdata *gd, struct pointdata *pd,
	int is_next, double *other_t ) {
    double test, t;
    int which;
    Spline *s;
    Monotonic *m;
    int winding, nw, i,j;
    struct monotonic **space;

    /* Things are difficult if we go exactly through the point. Move off */
    /*  to the side a tiny bit and hope that doesn't matter */
    if ( is_next ) {
	s = pd->sp->next;
	t = .001;
	which = pd->next_ver;
    } else {
	s = pd->sp->prev;
	t = .999;
	which = pd->prev_ver;
    }
    if ( s==NULL )		/* Somehow we got an open contour? */
return( NULL );
    test = ((s->splines[which].a*t+s->splines[which].b)*t+s->splines[which].c)*t+s->splines[which].d;
    MonotonicFindAt(gd->ms,which,test,space = gd->space);

    winding = 0;
    for ( i=0; space[i]!=NULL; ++i ) {
	m = space[i];
	nw = ((&m->xup)[which] ? 1 : -1 );
	if ( m->s == s && t>=m->tstart && t<=m->tend )
    break;
	winding += nw;
    }
    if ( space[i]==NULL ) {
	fprintf( stderr, "FindMatchinHVEdge didn't\n" );
return( NULL );
    }

    if (( nw<0 && winding>0 ) || (nw>0 && winding<0)) {
	winding = nw;
	for ( j=i-1; j>=0; --j ) {
	    m = space[j];
	    winding += ((&m->xup)[which] ? 1 : -1 );
	    if ( winding==0 ) {
		*other_t = space[j]->t;
return( space[j]->s );
	    }
	}
    } else {
	winding = nw;
	for ( j=i+1; space[j]!=NULL; ++j ) {
	    m = space[j];
	    winding += ((&m->xup)[which] ? 1 : -1 );
	    if ( winding==0 ) {
		*other_t = space[j]->t;
return( space[j]->s);
	    }
	}
    }
    fprintf( stderr, "FindMatchingHVEdge fell into an impossible position\n" );
return( NULL );
}

static Spline *FindMatchingEdge(struct glyphdata *gd, struct pointdata *pd,
	int is_next, int only_hv ) {
    BasePoint *dir, norm, absnorm;
    Spline myline;
    double *other_t = is_next ? &pd->next_e_t : &pd->prev_e_t;
    double t1,t2;

    if (( is_next && (pd->next_hor || pd->next_ver)) ||
	    ( !is_next && (pd->prev_hor || pd->prev_ver)))
return( FindMatchingHVEdge(gd,pd,is_next,other_t));
    else if ( only_hv )
return( NULL );

    if ( gd->stspace==NULL ) {
	int i, cnt;
	SplineSet *spl;
	Spline *s, *first;
	for ( i=0; i<2; ++i ) {
	    cnt = 0;
	    for ( spl=gd->sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
		first = NULL;
		if ( spl->first->prev!=NULL ) {
		    for ( s=spl->first->next; s!=first; s=s->to->next ) {
			if ( first==NULL ) first = s;
			if ( i )
			    gd->sspace[cnt] = s;
			++cnt;
		    }
		}
	    }
	    if ( !i ) {
		gd->scnt = cnt;
		gd->sspace = galloc((cnt+1)*sizeof(Spline *));
	    } else
		gd->sspace[cnt] = NULL;
	}
	gd->stspace = galloc((3*cnt+2)*sizeof(struct st));
	SplineCharFindBounds(gd->sc,&gd->size);
	gd->size.minx -= 10; gd->size.miny -= 10;
	gd->size.maxx += 10; gd->size.maxy += 10;
    }
    if ( is_next )
	dir = &pd->nextunit;
    else
	dir = &pd->prevunit;
    norm.x = -dir->y;
    norm.y = dir->x;
    absnorm = norm;
    if ( absnorm.x<0 ) absnorm.x = -absnorm.x;
    if ( absnorm.y<0 ) absnorm.y = -absnorm.y;
    memset(&myline,0,sizeof(myline));
    myline.knownlinear = myline.islinear = true;
    if ( absnorm.x > absnorm.y ) {
	/* Greater change in x than in y */
	t1 = (gd->size.minx-pd->sp->me.x)/norm.x;
	t2 = (gd->size.maxx-pd->sp->me.x)/norm.x;
	myline.splines[0].d = gd->size.minx;
	myline.splines[0].c = gd->size.maxx-gd->size.minx;
	myline.splines[1].d = pd->sp->me.y+t1*norm.y;
	myline.splines[1].c = (t2-t1)*norm.y;
    } else {
	t1 = (gd->size.miny-pd->sp->me.y)/norm.y;
	t2 = (gd->size.maxy-pd->sp->me.y)/norm.y;
	myline.splines[0].d = gd->size.miny;
	myline.splines[0].c = gd->size.maxy-gd->size.miny;
	myline.splines[1].d = pd->sp->me.x+t1*norm.x;
	myline.splines[1].c = (t2-t1)*norm.x;
    }
return( MonotonicFindAlong(gd->sspace,&myline,gd->stspace,pd->sp,other_t));
}

/* In TrueType I want to make sure that everything on a diagonal line remains */
/*  on the same line. Hence we compute the line. Also we are interested in */
/*  points that are on the intersection of two lines */
static struct linedata *BuildLine(struct glyphdata *gd,struct pointdata *pd,int is_next ) {
    int i;
    BasePoint *dir, *base;
    struct pointdata **pspace = gd->pspace;
    int pcnt=0;
    double dist_error;
    struct linedata *line;
    double n_s_err, p_s_err, off;

    dir = is_next ? &pd->nextunit : &pd->prevunit;
    dist_error = ( dir->x==0 || dir->y==0 ) ? dist_error_hv : dist_error_diag ;	/* Diagonals are harder to align */
    if ( dir->x==0 && dir->y==0 )
return( NULL );
    base = &pd->sp->me;

    for ( i= (pd - gd->points)+1; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd2 = &gd->points[i];
	off = (pd2->sp->me.x-base->x)*dir->y - (pd2->sp->me.y-base->y)*dir->x;
	if ( off>dist_error || off<-dist_error )
    continue;
	n_s_err = dir->x*pd2->nextunit.y - dir->y*pd2->nextunit.x;
	p_s_err = dir->x*pd2->prevunit.y - dir->y*pd2->prevunit.x;
	if ( (n_s_err<slope_error && n_s_err>-slope_error && pd2->nextline==NULL ) ||
		(p_s_err<slope_error && p_s_err>-slope_error && pd2->prevline==NULL ))
	    pspace[pcnt++] = pd2;
    }

    if ( pcnt==0 )
return( NULL );
    if ( pcnt==1 ) {
	/* if the line consists of just these two points, only count it as */
	/*  a true line if the two form a line */
	if ( (pd->sp->next->to!=pspace[0]->sp || !pd->sp->next->knownlinear) &&
		( pd->sp->prev->from!=pspace[0]->sp || !pd->sp->prev->knownlinear ))
return( NULL );
    }

    line = &gd->lines[gd->linecnt++];
    line->pcnt = pcnt+1;
    line->points = galloc((pcnt+1)*sizeof(struct pointdata *));
    line->points[0] = pd;
    line->unitvector = *dir;
    line->online = *base;
    for ( i=0; i<pcnt; ++i ) {
	n_s_err = dir->x*pspace[i]->nextunit.y - dir->y*pspace[i]->nextunit.x;
	p_s_err = dir->x*pspace[i]->prevunit.y - dir->y*pspace[i]->prevunit.x;
	if ( n_s_err<slope_error && n_s_err>-slope_error && pspace[i]->nextline==NULL ) {
	    pspace[i]->nextline = line;
	    if ( pspace[i]->colinear )
		pspace[i]->prevline = line;
	}
	if ( p_s_err<slope_error && p_s_err>-slope_error && pspace[i]->prevline==NULL ) {
	    pspace[i]->prevline = line;
	    if ( pspace[i]->colinear )
		pspace[i]->nextline = line;
	}
	line->points[i+1] = pspace[i];
    }
return( line );
}

static void AddToStem(struct stemdata *stem,struct pointdata *pd1,struct pointdata *pd2,
	int is_next1, int is_next2, struct pointdata *pd2potential ) {
    struct stem_chunk *chunk;
    BasePoint *dir = &stem->unit;
    BasePoint *test = &pd1->sp->me;
    double off, dist_error;
    int j;
    struct pointdata *pd1potential = pd1;

    dist_error = ( dir->x==0 || dir->y==0 ) ? dist_error_hv : dist_error_diag;	/* Diagonals are harder to align */

    off = (test->x-stem->left.x)*dir->y - (test->y-stem->left.y)*dir->x;
    if ( off>dist_error || off<-dist_error ) {
	struct pointdata *pd = pd1;
	int in = is_next1;
	pd1 = pd2; is_next1 = is_next2;
	pd2 = pd; is_next2 = in;
	pd1potential = pd2potential; pd2potential = NULL;
    }

    for ( j=stem->chunk_cnt-1; j>=0; --j ) {
	chunk = &stem->chunks[j];
	if ( chunk->l==pd1 && chunk->r==pd2 )
    break;
    }

    if ( j<0 ) {
	stem->chunks = grealloc(stem->chunks,(stem->chunk_cnt+1)*sizeof(struct stem_chunk));
	chunk = &stem->chunks[stem->chunk_cnt++];
	chunk->l = pd1;
	chunk->r = pd2;
	chunk->lpotential = pd1potential;
	chunk->rpotential = pd2potential;
	chunk->lnext = is_next1;
	chunk->rnext = is_next2;
    }
    if ( pd1!=NULL ) {
	if ( is_next1 ) {
	    if ( pd1->nextstem == NULL ) {
		pd1->nextstem = stem;
#if 0	/* They can be different, if so we need to figure which is better */
		if ( pd1->colinear )
		    pd1->prevstem = stem;
#endif
	    }
	} else {
	    if ( pd1->prevstem == NULL ) {
		pd1->prevstem = stem;
	    }
	}
    }
    if ( pd2!=NULL ) {
	if ( is_next2 ) {
	    if ( pd2->nextstem == NULL ) {
		pd2->nextstem = stem;
	    }
	} else {
	    if ( pd2->prevstem == NULL ) {
		pd2->prevstem = stem;
	    }
	}
    }
}

static int OnStem(struct stemdata *stem,struct pointdata *pd) {
    double dist_error, off;
    BasePoint *dir = &stem->unit;
    BasePoint *test = &pd->sp->me;

    dist_error = ( dir->x==0 || dir->y==0 ) ? dist_error_hv : dist_error_diag;	/* Diagonals are harder to align */

    off = (test->x-stem->left.x)*dir->y - (test->y-stem->left.y)*dir->x;
    if ( off<dist_error && off>-dist_error )
return( true );

    off = (test->x-stem->right.x)*dir->y - (test->y-stem->right.y)*dir->x;
    if ( off<dist_error && off>-dist_error )
return( true );

return( false );
}

static struct stemdata *FindStem(struct glyphdata *gd,struct pointdata *pd,
	BasePoint *dir,SplinePoint *match, int is_next2) {
    int j;
    BasePoint *mdir;
    struct pointdata *pd2;
    struct stemdata *stem;
    double err;

    if ( match->ttfindex==0xffff )
return( NULL );			/* Point doesn't really exist */

    pd2 = &gd->points[match->ttfindex];

    mdir = is_next2 ? &pd2->nextunit : &pd2->prevunit;
    if ( mdir->x==0 && mdir->y==0 )
return( NULL );
    err = mdir->x*dir->y - mdir->y*dir->x;
    if (( err<slope_error && err>-slope_error ) ||
	    (( dir->x==0 || dir->y==0 ) &&
	       (pd->sp->next->to==match || pd->sp->prev->from==match) &&
	       err<2*slope_error && err>-2*slope_error )) {
	    /* Special check for cm serifs */
	stem = is_next2 ? pd2->nextstem : pd2->prevstem;
	if ( stem!=NULL && OnStem(stem,pd))
return( stem );

	for ( j=0; j<gd->stemcnt; ++j ) {
	    err = gd->stems[j].unit.x*dir->y - gd->stems[j].unit.y*dir->x;
	    if ( err<slope_error && err>-slope_error &&
		    OnStem(&gd->stems[j],pd) && OnStem(&gd->stems[j],pd2)) {
return( &gd->stems[j] );
	    }
	}
return( (struct stemdata *) (-1) );
    }
return( NULL );
}

static int OnLine(BasePoint *base,BasePoint *test,BasePoint *dir) {
    double err = dir->x*(base->y-test->y) - dir->y*(base->x-test->x);
return( err<slope_error && err>-slope_error );
}

static struct stemdata *TestStem(struct glyphdata *gd,struct pointdata *pd,
	BasePoint *dir,SplinePoint *match, int is_next, int is_next2) {
    BasePoint *mdir;
    struct pointdata *pd2, *pd2potential;
    struct stemdata *stem, *stem2 = (struct stemdata *) (-2);
    double err, width;

    if ( match->ttfindex==0xffff )
return( NULL );
    width = (match->me.x-pd->sp->me.x)*dir->y - (match->me.y-pd->sp->me.y)*dir->x;
    if ( width<.5 && width>-.5 )
return( NULL );		/* Zero width stems aren't interesting */
    if ( pd->sp->next->to==match || pd->sp->prev->from==match )
return( NULL );		/* Don't want a stem between two splines that intersect */

    stem = FindStem(gd,pd,dir,match,is_next2);
    if ( stem==NULL )
return( NULL );

    pd2 = &gd->points[match->ttfindex];
    stem2 = NULL;
    /* OK, we've got a valid stem between us and pd2. Now see if we get the */
    /*  same stem when we look at pd2. If we do, then all is happy and nice */
    /*  and we can add both points to the stem. If not, then we're sort of */
    /*  happy, but we can only add pd to the stem */
    if ( pd2->nextstem==stem )
	stem2 = stem;
    else if ( pd2->prevstem==stem )
	stem2 = stem;
    else if ( pd2->nextstem==NULL || pd2->prevstem==NULL ) {
	mdir = is_next2 ? &pd2->nextunit : &pd2->prevunit;
	if ( mdir->x!=0 || mdir->y!=0 ) {
	    err = mdir->x*dir->y - mdir->y*dir->x;
	    if ( err<slope_error && err>-slope_error &&
		    (is_next2 ? pd2->nextstem : pd2->prevstem)==NULL ) {
		Spline *other;
		double t;
		SplinePoint *test;
		if ( !is_next2 ) {
		    other = pd2->prevedge;
		    t = pd2->prev_e_t;
		} else {
		    other = pd2->nextedge;
		    t = pd2->next_e_t;
		}
		if ( other!=NULL ) {
		    stem2 = FindStem(gd,pd2,dir,test = t>.5 ? other->to : other->from,t<=.5);
		    if ( stem2==NULL )
			stem2 = FindStem(gd,pd2,dir,test = t>.5 ? other->from : other->to,t>.5);
		    if ( stem2==(struct stemdata *) (-1) ) {
			if ( !OnLine(&pd->sp->me,&test->me,dir))
			    stem2 = NULL;
		    } else if ( stem2==stem )
			/* Good */;
		    else if ( OnLine( &pd->sp->me,&other->to->me,dir ) ||
			    OnLine( &pd->sp->me,&other->from->me,dir )) {
			stem2 = stem;
		    } else
			stem2 = NULL;
		}
	    }
	}
    }

    pd2potential = pd2;
    if ( stem2==NULL )
	pd2 = NULL;

    if ( stem==(struct stemdata *) (-1) ) {
	stem = &gd->stems[gd->stemcnt++];
	stem->unit = *dir;
	if ( dir->x+dir->y<0 ) {
	    stem->unit.x = -stem->unit.x;
	    stem->unit.y = -stem->unit.y;
	}
	if (( stem->unit.x>stem->unit.y && pd->sp->me.y<match->me.y ) ||
		(stem->unit.y>=stem->unit.x && pd->sp->me.x<match->me.x )) {
	    stem->left = pd->sp->me;
	    stem->right = match->me;
	} else {
	    stem->left = match->me;
	    stem->right = pd->sp->me;
	}
	stem->chunks = NULL;
	stem->chunk_cnt = 0;
    }

    AddToStem(stem,pd,pd2,is_next,is_next2,pd2potential);
return( stem );
}
	
static struct stemdata *BuildStem(struct glyphdata *gd,struct pointdata *pd,int is_next ) {
    BasePoint *dir;
    Spline *other;
    double t;
    struct stemdata *stem;

    if ( is_next ) {
	dir = &pd->nextunit;
	other = pd->nextedge;
	t = pd->next_e_t;
    } else {
	dir = &pd->prevunit;
	other = pd->prevedge;
	t = pd->prev_e_t;
    }
    if ( other==NULL )
return( NULL );

    stem = TestStem(gd,pd,dir,t>.5 ? other->to : other->from,is_next,t<=.5);
    if ( stem==NULL )
	stem = TestStem(gd,pd,dir,t>.5 ? other->from : other->to,is_next,t>.5);
return( stem );
}

static int swidth_cmp(const void *_p1, const void *_p2) {
    const struct stemdata * const *stpt1 = _p1, * const *stpt2 = _p2;
    const struct stemdata *s1 = *stpt1, *s2 = *stpt2;
    if ( s1->width>s2->width )
return( 1 );
    else if ( s1->width<s2->width )
return( -1 );

return( 0 );
}

static struct pointdata *PotentialAdd(struct glyphdata *gd,struct stemdata *stem,
	struct pointdata *pd, int is_next ) {

    if ( is_next && (pd->nextstem==NULL || pd->nextstem->toobig )) {
	pd->nextstem = stem;
return( pd );
    } else if ( !is_next && (pd->prevstem==NULL || pd->prevstem->toobig )) {
	pd->prevstem = stem;
return( pd );
    }
return( NULL );
}

static int IsClosestPotential(struct stemdata **sspace,int cur,int goodcnt,
	int is_l,struct stem_chunk *chunk) {
    struct pointdata *search;
    double mydist, dist;
    SplinePoint *sp1, *sp2;
    int i,j;

    if ( is_l ) {
	if ( chunk->r==NULL )
return( false );
	search = chunk->lpotential;
	sp1 = search->sp; sp2 = chunk->r->sp;
	mydist = (sp1->me.x-sp2->me.x)*(sp1->me.x-sp2->me.x) +
		(sp1->me.y-sp2->me.y)*(sp1->me.y-sp2->me.y);
    } else {
	if ( chunk->l==NULL )
return( false );
	search = chunk->rpotential;
	sp1 = search->sp; sp2 = chunk->l->sp;
	mydist = (sp1->me.x-sp2->me.x)*(sp1->me.x-sp2->me.x) +
		(sp1->me.y-sp2->me.y)*(sp1->me.y-sp2->me.y);
    }
    for ( i=cur+1; i<goodcnt; ++i ) {
	struct stemdata *stem = sspace[i];
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    struct stem_chunk *chunk = &stem->chunks[j];
	    if ( is_l && chunk->r!=NULL && chunk->l==NULL && chunk->lpotential==search ) {
		sp2 = chunk->r->sp;
		dist = (sp1->me.x-sp2->me.x)*(sp1->me.x-sp2->me.x) +
			(sp1->me.y-sp2->me.y)*(sp1->me.y-sp2->me.y);
		if ( dist<mydist )
return( false );
	    } else if ( !is_l && chunk->l!=NULL && chunk->r==NULL && chunk->rpotential==search ) {
		sp2 = chunk->l->sp;
		dist = (sp1->me.x-sp2->me.x)*(sp1->me.x-sp2->me.x) +
			(sp1->me.y-sp2->me.y)*(sp1->me.y-sp2->me.y);
		if ( dist<mydist )
return( false );
	    }
	}
    }
return( true );
}

static void MinMaxStem(BasePoint *here,struct stemdata *stem, double *maxm, double *minm) {
    double m;

    m = (here->x-stem->left.x)*stem->unit.x +
	(here->y-stem->left.y)*stem->unit.y;
    if ( m>*maxm ) *maxm = m;
    if ( m<*minm ) *minm = m;
}

static int InChunk(SplinePoint *sp,struct stemdata *stem,int j,int is_left ) {
    int i;

    for ( i=j+1; i<stem->chunk_cnt; ++i ) {
	struct stem_chunk *chunk = &stem->chunks[i];
	if ( is_left && chunk->l!=NULL && chunk->l->sp==sp ) {
	    chunk->ltick = true;
return( true );
	} else if ( !is_left && chunk->r!=NULL && chunk->r->sp==sp ) {
	    chunk->rtick = true;
return( true );
	}
    }
return( false );
}

static void GDFindUnlikelyStems(struct glyphdata *gd) {
    double width, maxm, minm, len;
    int i,j;
    double em = (gd->sc->parent->ascent+gd->sc->parent->descent);
    SplinePoint *sp;

    /* If a vertical stem has straight edges, and it is wider than tall */
    /*  then it is unlikely to be a real stem */
    for ( i=0; i<gd->stemcnt; ++i ) {
	struct stemdata *stem = &gd->stems[i];

	width = (stem->left.x-stem->right.x)*stem->unit.y -
		(stem->left.y-stem->right.y)*stem->unit.x;
	if ( width<0 ) width = -width;
	stem->width = width;
	len = 0;
	for ( j=0; j<gd->pcnt; ++j ) if ( gd->points[j].sp!=NULL )
	    gd->points[j].sp->ticked = false;
	for ( j=0; j<stem->chunk_cnt; ++j )
	    stem->chunks[j].ltick = stem->chunks[j].rtick = false;
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    struct stem_chunk *chunk = &stem->chunks[j];
	    minm = 1e10; maxm = -1e10;
	    if ( chunk->l!=NULL && !chunk->ltick && !(sp=chunk->l->sp)->ticked ) {
		sp->ticked = true;
		MinMaxStem(&sp->me,stem,&maxm,&minm);
		if ( stem==chunk->l->nextstem ) {
		    MinMaxStem(&sp->nextcp,stem,&maxm,&minm);
		    if ( sp->next->knownlinear &&
			    InChunk(sp->next->to,stem,j,true))
			MinMaxStem(&sp->next->to->me,stem,&maxm,&minm);
		}
		if ( stem==chunk->l->prevstem ) {
		    MinMaxStem(&sp->prevcp,stem,&maxm,&minm);
		    if ( sp->prev->knownlinear &&
			    InChunk(sp->prev->from,stem,j,true))
			MinMaxStem(&sp->prev->from->me,stem,&maxm,&minm);
		}
	    }
	    if ( chunk->r!=NULL && !chunk->rtick && !(sp=chunk->r->sp)->ticked ) {
		/* I still use stem->left as the base to make measurements */
		/*  comensurate. Any normal distance becomes 0 in the dot product */
		sp->ticked = true;
		MinMaxStem(&sp->me,stem,&maxm,&minm);
		if ( stem==chunk->r->nextstem ) {
		    MinMaxStem(&sp->nextcp,stem,&maxm,&minm);
		    if ( sp->next->knownlinear &&
			    InChunk(sp->next->to,stem,j,false))
			MinMaxStem(&sp->next->to->me,stem,&maxm,&minm);
		}
		if ( stem==chunk->r->prevstem ) {
		    MinMaxStem(&sp->prevcp,stem,&maxm,&minm);
		    if ( sp->prev->knownlinear &&
			    InChunk(sp->prev->from,stem,j,false))
			MinMaxStem(&sp->prev->from->me,stem,&maxm,&minm);
		}
	    }
	    if ( maxm>minm )
		len += maxm-minm;
	}
	stem->toobig = ( width > len ) &&
		(width>em/10) &&
		(width>em/5 || 1.5*width>len);
    }
}

struct glyphdata *GlyphDataBuild(SplineChar *sc, int only_hv) {
    struct glyphdata *gd;
    int i,j;
    SplineSet *ss;
    SplinePoint *sp;
    Monotonic *m;
    int cnt, goodcnt;
    struct stemdata **sspace;

    /* We can't hint type3 fonts, so the only layer we care about is ly_fore */
    /* We shan't try to hint references yet */
    if ( sc->layers[ly_fore].splines==NULL )
return( NULL );

    gd = gcalloc(1,sizeof(struct glyphdata));

    /* SSToMContours can clean up the splinesets (remove 0 length splines) */
    /*  so it must be called BEFORE everything else (even though logically */
    /*  that doesn't make much sense). Otherwise we might have a pointer */
    /*  to something since freed */
    gd->ms = SSsToMContours(sc->layers[ly_fore].splines,over_remove);	/* second argument is meaningless here */

    gd->pcnt = SCNumberPoints(sc);
    for ( i=0, ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next, ++i );
    gd->ccnt = i;
    gd->contourends = galloc((i+1)*sizeof(int));
    for ( i=0, ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next, ++i ) {
	SplinePoint *last;
	if ( ss->first->prev!=NULL )
	    last = ss->first->prev->from;
	else
	    last = ss->last;
	if ( last->ttfindex==0xffff )
	    gd->contourends[i] = last->nextcpindex;
	else
	    gd->contourends[i] = last->ttfindex;
    }
    gd->contourends[i] = -1;
    gd->sc = sc;
    gd->sf = sc->parent;
    gd->fudge = (sc->parent->ascent+sc->parent->descent)/500;

    gd->points = gcalloc(gd->pcnt,sizeof(struct pointdata));
    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) if ( ss->first->prev!=NULL ) {
	for ( sp=ss->first; ; ) {
	    PointInit(gd,sp,ss);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }

    /*gd->ms = SSsToMContours(sc->layers[ly_fore].splines,over_remove);*/	/* second argument is meaningless here */
    for ( m=gd->ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
    gd->space = galloc((cnt+2)*sizeof(Monotonic*));
    gd->mcnt = cnt;

    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if ( !pd->nextzero )
	    pd->nextedge = FindMatchingEdge(gd,pd,true,only_hv);
	if ( !pd->prevzero )
	    pd->prevedge = FindMatchingEdge(gd,pd,false,only_hv);
    }

    /* There will never be more lines than there are points (counting next/prev as separate) */
    gd->lines = galloc(2*gd->pcnt*sizeof(struct linedata));
    gd->linecnt = 0;
    gd->pspace = galloc(gd->pcnt*sizeof(struct pointdata *));
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if (( !only_hv || pd->next_hor || pd->next_ver ) && pd->nextline==NULL ) {
	    pd->nextline = BuildLine(gd,pd,true);
	    if ( pd->colinear )
		pd->prevline = pd->nextline;
	}
	if (( !only_hv || pd->prev_hor || pd->prev_ver ) && pd->prevline==NULL ) {
	    pd->prevline = BuildLine(gd,pd,false);
	    if ( pd->colinear && pd->nextline == NULL )
		pd->nextline = pd->prevline;
	}
    }
    free(gd->pspace);

    /* There will never be more stems than there are points (counting next/prev as separate) */
    gd->stems = galloc(2*gd->pcnt*sizeof(struct stemdata));
    gd->stemcnt = 0;			/* None used so far */
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if ( pd->nextedge!=NULL && pd->nextstem==NULL )
	    BuildStem(gd,pd,true);
	if ( pd->prevedge!=NULL && pd->prevstem==NULL )
	    BuildStem(gd,pd,false);
    }
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if ( pd->nextstem==NULL && pd->colinear )
	    pd->nextstem = pd->prevstem;
	else if ( pd->prevstem==NULL && pd->colinear )
	    pd->prevstem = pd->nextstem;
    }

    FreeMonotonics(gd->ms);	gd->ms = NULL;
    free(gd->space);		gd->space = NULL;
    free(gd->sspace);		gd->sspace = NULL;
    free(gd->stspace);		gd->stspace = NULL;

    GDFindUnlikelyStems(gd);

    sspace = galloc(gd->stemcnt*sizeof(struct stemdata *));
    for ( i=j=0; i<gd->stemcnt; ++i ) if ( !gd->stems[i].toobig )
	sspace[j++] = &gd->stems[i];
    goodcnt = j;
    qsort(sspace,goodcnt,sizeof(struct stemdata *),swidth_cmp);

    /* we were cautious about assigning points to stems, go back now and see */
    /*  if there are any low-quality matches which remain unassigned, and if */
    /*  so then assign them to the stem they almost fit on. */
    /* If there are multiple stems, find the one which is closest to this point */
    for ( i=0; i<goodcnt; ++i ) {
	struct stemdata *stem = sspace[i];
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    struct stem_chunk *chunk = &stem->chunks[j];
	    if ( chunk->l==NULL && chunk->lpotential!=NULL &&
		    IsClosestPotential(sspace,i,goodcnt,true,chunk))
		chunk->l = PotentialAdd(gd,stem,chunk->lpotential,chunk->lnext);
	    if ( chunk->r==NULL && chunk->rpotential!=NULL &&
		    IsClosestPotential(sspace,i,goodcnt,false,chunk))
		chunk->r = PotentialAdd(gd,stem,chunk->rpotential,chunk->rnext);
	}
    }
    free(sspace);

return( gd );
}

void GlyphDataFree(struct glyphdata *gd) {
    int i;
    for ( i=0; i<gd->linecnt; ++i )
	free(gd->lines[i].points);
    for ( i=0; i<gd->stemcnt; ++i )
	free(gd->stems[i].chunks);
    free(gd->lines);
    free(gd->stems);
    free(gd->contourends);
    free(gd->points);
    free(gd);
}
