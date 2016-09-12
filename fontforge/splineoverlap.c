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
#include "fontforge.h"
#include "splinefont.h"
#include "edgelist2.h"
#include <math.h>
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif
#include <stdarg.h>

#include <gwidget.h>		/* For PostNotice */

/* First thing we do is divide each spline into a set of sub-splines each of */
/*  which is monotonic in both x and y (always increasing or decreasing)     */
/* Then we compare each monotonic spline with every other one and see if they*/
/*  intersect.  If they do, split each up into sub-sub-segments and create an*/
/*  intersection point (note we need to be a little careful if an intersec-  */
/*  tion happens at an end point. We don't need to create a intersection for */
/*  two adjacent splines, there isn't a real intersection... but if a third  */
/*  spline crosses that point (or ends there) then all three (four) splines  */
/*  need to be joined into an intersection point)                            */
/* Nasty things happen if splines are coincident. They will almost never be  */
/*  perfectly coincident and will keep crossing and recrossing as rounding   */
/*  errors suggest one is before the other. Look for coincident splines and  */
/*  treat the places they start and stop being coincident as intersections   */
/*  then when we find needed splines below look for these guys and ignore    */
/*  recrossings of splines which are close together                          */
/* Figure out if each monotonic sub-spline is needed or not		     */
/*  (Note: It was tempting to split the bits up into real splines rather     */
/*   than keeping them as sub-sections of the original. Unfortunately this   */
/*   splitting introduced rounding errors which meant that we got more       */
/*   intersections, which meant that splines could be both needed and un.    */
/*   so I don't do that until later)					     */
/*  if the spline hasn't been tagged yet:				     */
/*   does the spline change greater in x or y?				     */
/*   draw a line parallel to the OTHER axis which hits our spline and doesn't*/
/*    hit any endpoints (or intersections, which are end points too now)     */
/*   count the winding number (as we do this we can mark other splines as    */
/*    needed or not) and figure out if our spline is needed		     */
/*  So run through the list of intersections				     */
/*	At an intersection there should be an even number of needed monos.   */
/*	Use this as the basis of a new splineset, trace it around until	     */
/*	  we get back to the start intersection (should happen)		     */
/*	  (Note: We may need to reverse a monotonic sub-spline or two)	     */
/*	  As we go, mark each monotonic as having been used		     */
/*  Keep doing this until all needed exits from all intersections have been  */
/*	used.								     */
/*  The free up our temporary data structures, merge in any open splinesets  */
/*	free the old closed splinesets					     */

// Frank recommends using the following macro whenever making changes
// to this code and capturing and diffing output in order to track changes
// in errors and reports.
// (The pointers tend to clutter the diff a bit.)
// #define FF_OVERLAP_VERBOSE

static char *glyphname=NULL;

static void SOError(const char *format,...) {
    va_list ap;
    va_start(ap,format);
    if ( glyphname==NULL )
	fprintf(stderr, "Internal Error (overlap): " );
    else
	fprintf(stderr, "Internal Error (overlap) in %s: ", glyphname );
    vfprintf(stderr,format,ap);
    va_end(ap);
}

static void SONotify(const char *format,...) {
    va_list ap;
    va_start(ap,format);
#ifdef FF_OVERLAP_VERBOSE
    if ( glyphname==NULL )
	fprintf(stderr, "Note (overlap): " );
    else
	fprintf(stderr, "Note (overlap) in %s: ", glyphname );
    vfprintf(stderr,format,ap);
#endif
    va_end(ap);
}

static void ValidateMListT(struct mlist * input) {
  if (input->isend && input->t == input->m->tstart) {
    SONotify("MList %p claims to be an end on %p but has t (%f) = tstart (%f).\n", input, input->m, input->t, input->m->tstart);
  } else if (input->isend && input->t != input->m->tend) {
    SONotify("MList %p claims to be an end on %p but has t (%f) != tend (%f).\n", input, input->m, input->t, input->m->tend);
  } else if (!input->isend && input->t == input->m->tend) {
    SONotify("MList %p claims to be a start on %p but has t (%f) = tend (%f).\n", input, input->m, input->t, input->m->tend);
  } else if (!input->isend && input->t != input->m->tstart) {
    SONotify("MList %p claims to be a start on %p but has t (%f) != tstart (%f).\n", input, input->m, input->t, input->m->tstart);
  }
}

static void ValidateMListTs(struct mlist * input) {
  struct mlist * current;
  for (current = input; current != NULL; current = current->next) ValidateMListT(current);
}

#ifdef FF_OVERLAP_VERBOSE
#define ValidateMListTs_IF_VERBOSE(input) ValidateMListTs(input);
#else
#define ValidateMListTs_IF_VERBOSE(input) 
#endif

extended evalSpline(Spline *s, extended t, int dim) {
  return ((s->splines[dim].a*t+s->splines[dim].b)*t+s->splines[dim].c)*t+s->splines[dim].d;
}

static void ValidateMonotonic(Monotonic *ms) {
  if (ms->start != NULL) {
    if (!RealWithin(ms->start->inter.x, evalSpline(ms->s, ms->tstart, 0), 0.00001) ||
        !RealWithin(ms->start->inter.y, evalSpline(ms->s, ms->tstart, 1), 0.00001))
      SOError("The start of the monotonic does not match the listed intersection.\n");
    ValidateMListTs(ms->start->monos);
  }
  if (ms->end != NULL) {
    if (!RealWithin(ms->end->inter.x, evalSpline(ms->s, ms->tend, 0), 0.00001) ||
        !RealWithin(ms->end->inter.y, evalSpline(ms->s, ms->tend, 1), 0.00001))
      SOError("The end of the monotonic does not match the listed intersection.\n");
    ValidateMListTs(ms->end->monos);
  }
  if (ms->tstart == 0) {
    if (!RealWithin(ms->s->from->me.x, evalSpline(ms->s, ms->tstart, 0), 0.00001) ||
        !RealWithin(ms->s->from->me.y, evalSpline(ms->s, ms->tstart, 1), 0.00001))
      SOError("The start of the monotonic does not match that of the containing spline.\n");
  }
  if (ms->tend == 1) {
    if (!RealWithin(ms->s->to->me.x, evalSpline(ms->s, ms->tend, 0), 0.00001) ||
        !RealWithin(ms->s->to->me.y, evalSpline(ms->s, ms->tend, 1), 0.00001))
      SOError("The end of the monotonic does not match that of the containing spline.\n");
  }
  return;
}

static void Validate(Monotonic *ms, Intersection *ilist) {
    MList *ml;
    int mcnt;

    while ( ilist!=NULL ) {
        // For each listed intersection, verify that each connected monotonic
        // starts or ends at the intersection (identified by pointer, not geography).
	for ( mcnt=0, ml=ilist->monos; ml!=NULL; ml=ml->next ) {
	    if ( ml->m->isneeded ) ++mcnt;
	    if ( ml->m->start!=ilist && ml->m->end!=ilist )
		SOError( "Intersection (%g,%g) not on a monotonic which should contain it.\n",
			(double) ilist->inter.x, (double) ilist->inter.y );
	}
	if ( mcnt&1 )
	    SOError( "Odd number of needed monotonic sections at intersection. (%g,%g)\n",
		    (double) ilist->inter.x,(double) ilist->inter.y );
	ilist = ilist->next;
    }

    while ( ms!=NULL ) {
        if ( ms->prev == NULL )
          SOError( "Open monotonic loop.\n" );
	else if ( ms->prev->end!=ms->start )
	    SOError( "Mismatched intersection.\n (%g,%g)->(%g,%g) ends at (%g,%g) while (%g,%g)->(%g,%g) starts at (%g,%g)\n",
		(double) ms->prev->s->from->me.x,(double) ms->prev->s->from->me.y,
		(double) ms->prev->s->to->me.x,(double) ms->prev->s->to->me.y,
		(double) (ms->prev->end!=NULL?ms->prev->end->inter.x:-999999), (double) (ms->prev->end!=NULL?ms->prev->end->inter.y:-999999), 
		(double) ms->s->from->me.x,(double) ms->s->from->me.y,
		(double) ms->s->to->me.x,(double) ms->s->to->me.y,
		(double) (ms->start!=NULL?ms->start->inter.x:-999999), (double) (ms->start!=NULL?ms->start->inter.y:-999999) );
	ms = ms->linked;
    }
}

static Monotonic *SplineToMonotonic(Spline *s,extended startt,extended endt,
	Monotonic *last,int exclude) {
    Monotonic *m;
    BasePoint start, end;

    if ( startt==0 )
	start = s->from->me;
    else {
	start.x = ((s->splines[0].a*startt+s->splines[0].b)*startt+s->splines[0].c)*startt
		    + s->splines[0].d;
	start.y = ((s->splines[1].a*startt+s->splines[1].b)*startt+s->splines[1].c)*startt
		    + s->splines[1].d;
    }
    if ( endt==1.0 )
	end = s->to->me;
    else {
	end.x = ((s->splines[0].a*endt+s->splines[0].b)*endt+s->splines[0].c)*endt
		    + s->splines[0].d;
	end.y = ((s->splines[1].a*endt+s->splines[1].b)*endt+s->splines[1].c)*endt
		    + s->splines[1].d;
    }
    if ( ( (real) (((start.x+end.x)/2)==start.x || (real) ((start.x+end.x)/2)==end.x) &&
	    (real) (((start.y+end.y)/2)==start.y || (real) ((start.y+end.y)/2)==end.y) ) ||
         (endt <= startt) || Within4RoundingErrors(startt, endt)) {
	/* The distance between the two extrema is so small */
	/*  as to be unobservable. In other words we'd end up with a zero*/
	/*  length spline */
	if ( endt==1.0 && last!=NULL && last->s==s )
	    last->tend = endt;
return( last );
    }

    m = chunkalloc(sizeof(Monotonic));
    m->s = s;
    m->tstart = startt;
    m->tend = endt;
#ifdef FF_RELATIONAL_GEOM
    m->otstart = startt;
    m->otend = endt;
#endif
    m->exclude = exclude;

    if ( end.x>start.x ) {
	m->xup = true;
	m->b.minx = start.x;
	m->b.maxx = end.x;
    } else {
	m->b.minx = end.x;
	m->b.maxx = start.x;
    }
    if ( end.y>start.y ) {
	m->yup = true;
	m->b.miny = start.y;
	m->b.maxy = end.y;
    } else {
	m->b.miny = end.y;
	m->b.maxy = start.y;
    }

    if ( last!=NULL ) {
    // Validate(last, NULL);
	last->next = m;
	last->linked = m;
	m->prev = last;
    // Validate(last, NULL);
    }
return( m );
}

static int SSIsSelected(SplineSet *spl) {
    SplinePoint *sp;

    for ( sp=spl->first; ; ) {
	if ( sp->selected )
return( true );
	if ( sp->next==NULL )
return( false );
	sp = sp->next->to;
	if ( sp==spl->first )
return( false );
    }
}

static int BpSame(BasePoint *bp1, BasePoint *bp2) {
    BasePoint mid;

    mid.x = (bp1->x+bp2->x)/2; mid.y = (bp1->y+bp2->y)/2;
    if ( (bp1->x==mid.x || bp2->x==mid.x) &&
	    (bp1->y==mid.y || bp2->y==mid.y))
return( true );

return( false );
}

static int SSRmNullSplines(SplineSet *spl) {
    Spline *s, *first, *next;

    first = NULL;
    for ( s=spl->first->next ; s!=first; s=next ) {
	next = s->to->next;
	if ( ((s->splines[0].a>-.01 && s->splines[0].a<.01 &&
		s->splines[0].b>-.01 && s->splines[0].b<.01 &&
		s->splines[1].a>-.01 && s->splines[1].a<.01 &&
		s->splines[1].b>-.01 && s->splines[1].b<.01) ||
		/* That describes a null spline (a line between the same end-point) */
	       RealNear((s->from->nextcp.x-s->from->me.x)*(s->to->me.y-s->to->prevcp.y)-
			(s->from->nextcp.y-s->from->me.y)*(s->to->me.x-s->to->prevcp.x),0)) &&
		/* And the above describes a point with a spline between it */
		/*  and itself where the spline covers no area (the two cps */
		/*  point in the same direction) */
		BpSame(&s->from->me,&s->to->me)) {
	    if ( next==s )
return( true );
	    if ( next->from->selected ) s->from->selected = true;
	    s->from->next = next;
	    s->from->nextcp = next->from->nextcp;
	    s->from->nonextcp = next->from->nonextcp;
	    s->from->nextcpdef = next->from->nextcpdef;
	    SplinePointFree(next->from);
	    if ( spl->first==next->from )
		spl->last = spl->first = s->from;
	    next->from = s->from;
	    SplineFree(s);
	} else {
	    if ( first==NULL )
		first = s;
	}
    }
return( false );
}

static Monotonic *SSToMContour(SplineSet *spl, Monotonic *start,
	Monotonic **end, enum overlap_type ot) {
    extended ts[4];
    Spline *first, *s;
    Monotonic *head=NULL, *last=NULL;
    int cnt, i, selected = false;
    extended lastt;

    if ( spl->first->prev==NULL )
return( start );		/* Open contours have no interior, ignore 'em */
    if ( spl->first->prev->from==spl->first &&
	    spl->first->noprevcp && spl->first->nonextcp )
return( start );		/* Let's just remove single points */

    if ( ot==over_rmselected || ot==over_intersel || ot==over_fisel || ot==over_exclude ) {
	selected = SSIsSelected(spl);
	if ( ot==over_rmselected || ot==over_intersel || ot==over_fisel ) {
	    if ( !selected )
return( start );
	    selected = false;
	}
    }

    /* We blow up on zero length splines. And a zero length contour is nasty */
    if ( SSRmNullSplines(spl))
return( start );

    first = NULL;
    for ( s=spl->first->next; s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	cnt = Spline2DFindExtrema(s,ts);
	lastt = 0;
	for ( i=0; i<cnt; ++i ) {
	    last = SplineToMonotonic(s,lastt,ts[i],last,selected);
	    if ( head==NULL ) head = last;
	    lastt=ts[i];
	}
	if ( lastt!=1.0 ) {
	    last = SplineToMonotonic(s,lastt,1.0,last,selected);
	    if ( head==NULL ) head = last;
	}
    }
    head->prev = last;
    last->next = head;
    if ( start==NULL )
	start = head;
    else
	(*end)->linked = head;
    *end = last;
    Validate(start, NULL);
return( start );
}

Monotonic *SSsToMContours(SplineSet *spl, enum overlap_type ot) {
    Monotonic *head=NULL, *last = NULL;

    while ( spl!=NULL ) {
	if ( spl->first->prev!=NULL )
	    head = SSToMContour(spl,head,&last,ot);
	spl = spl->next;
    }
return( head );
}

static void _AddSpline(Intersection *il,Monotonic *m,extended t,int isend) {
    // This adds a monotonic spline to the list of splines attached
    // to a given intersection, with the t-value at which it intersects.
    // It also updates the spline so that it starts or ends at the correct point.
    // This can cause inconsistencies if the point subsequently gets mapped
    // away again due to similar points that do not get consolidated.
    MList *ml;

ValidateMListTs_IF_VERBOSE(il->monos)

    for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	if ( ml->s==m->s && RealNear( ml->t,t ) && ml->isend==isend ) {
          if (ml->t == t) SONotify("Duplicate spline at %p (%f, %f).\n", il, il->inter.x, il->inter.y);
          else SONotify("Near-duplicate spline at %p (%f, %f).\n", il, il->inter.x, il->inter.y);
	  return;
	}
    }

    ml = chunkalloc(sizeof(MList)); // Create a new monotonic list item.
    // Add the new item to the monotonic list for the input intersection.
    ml->next = il->monos;
    il->monos = ml;
    
    ml->s = m->s; // Set the spline.
    ml->m = m;			/* This may change. We'll fix it up later */
    ml->t = t;
    ml->isend = isend;
    // If the start or end is not mapped to the correct intersection, adjust accordingly.
    if ( isend ) {
	if ( m->end!=NULL && m->end!=il )
	    SOError("Resetting _end. was: (%g,%g) now: (%g,%g)\n",
		    (double) m->end->inter.x, (double) m->end->inter.y, (double) il->inter.x, (double) il->inter.y);
	m->end = il;
    } else {
	if ( m->start!=NULL && m->start!=il )
	    SOError("Resetting _start. was: (%g,%g) now: (%g,%g)\n",
		    (double) m->start->inter.x, (double) m->start->inter.y, (double) il->inter.x, (double) il->inter.y);
	m->start = il;
    }
return;
}

static void MListReplaceMonotonic(struct mlist * input, struct monotonic * findm, struct monotonic * replacem, int isend) {
  // This replaces a reference to one monotonic with a reference to another.
  struct mlist * current;
  for (current = input; current != NULL; current = current->next)
    if (current->m == findm && current->isend == isend) { current->m = replacem; }
}

static void MListReplaceMonotonicT(struct mlist * input, struct monotonic * findm, int isend, extended t) {
  // This replaces a reference to one monotonic with a reference to another.
  struct mlist * current;
  for (current = input; current != NULL; current = current->next)
    if (current->m == findm && current->isend == isend) { current->t = t; }
}

static void MListCleanEmpty(struct mlist ** base_pointer) {
  // It is necessary to use double pointers so that we can set the previous reference.
  struct mlist ** current_pointer = base_pointer;
  struct mlist * tmp_pointer;
  while (*current_pointer) {
    if ((*current_pointer)->m == NULL) {
      tmp_pointer = (*current_pointer)->next;
      chunkfree(*current_pointer, sizeof(struct mlist));
      (*current_pointer) = tmp_pointer;
    }
    current_pointer = &((*current_pointer)->next);
  }
  return;
}

static void MListRemoveMonotonic(struct mlist ** base_pointer, struct monotonic * findm, int isend) {
  // It is necessary to use double pointers so that we can set the previous reference.
  struct mlist ** current_pointer = base_pointer;
  struct mlist * tmp_pointer;
  while (*current_pointer) {
    if (((*current_pointer)->m == findm) && ((*current_pointer)->isend == isend)) {
      tmp_pointer = (*current_pointer)->next;
      chunkfree(*current_pointer, sizeof(struct mlist));
      (*current_pointer) = tmp_pointer;
    }
    if (*current_pointer) current_pointer = &((*current_pointer)->next);
  }
  return;
}

static void MListReplaceMonotonicComplete(struct mlist ** input, struct monotonic * findm, struct monotonic * replacem, struct monotonic * replacement, int isend) {
  // This replaces a reference to one monotonic with a copied reference. I hope that it is not necessary.
  // It is necessary to use double pointers so that we can set the previous reference.
  struct mlist ** current_pointer = input;
  struct monotonic * tmp_pointer;
  while (*current_pointer) {
    if ((*current_pointer)->m == findm) {
      if ((tmp_pointer = chunkalloc(sizeof(struct monotonic))) &&
      (memcpy(tmp_pointer, replacement, sizeof(struct monotonic)) == 0)) {
        chunkfree((*current_pointer)->m, sizeof(struct monotonic));
        (*current_pointer)->m = tmp_pointer;
      } else SOError("Error copying segment.\n");
    }
    current_pointer = &((*current_pointer)->next);
  }
  return;
}

static extended FixMonotonicT(struct monotonic * input_mono, extended startt, extended x, extended y) {
  extended tmpt;
  if (input_mono->s->from->me.x == x && input_mono->s->from->me.y == y) {
    return 0;
  } else if (input_mono->s->to->me.x == x && input_mono->s->to->me.y == y) {
    return 1;
  } else if (input_mono->b.maxx-input_mono->b.minx > input_mono->b.maxy-input_mono->b.miny) {
    tmpt = SplineSolveFixup(&input_mono->s->splines[0], startt-0.0001, startt+0.0001, x);
  } else {
    tmpt = SplineSolveFixup(&input_mono->s->splines[1], startt-0.0001, startt+0.0001, y);
  }
  return tmpt;
}

static int MonotonicCheckZeroLength(struct monotonic * input1) {
  if (input1->start == input1->end) return 1;
  if (input1->tstart == input1->tend) return 1;
  if (input1->start != NULL && input1->end != NULL && 
    input1->start->inter.x == input1->end->inter.x && input1->start->inter.y == input1->end->inter.y) {
    SOError("Zero-length monotonic between unlike points.\n"); return 1;
  }
  return 0;
}

static void MonotonicElide(struct mlist ** base, struct monotonic * input1) {
  // This connects the adjacent segments, deletes monotonic connections to and from this segment,
  // and removes intersection records for this segment as well.
  // This is mostly useful before merging two coincident intersections.
  if (input1->next != NULL) input1->next->prev = input1->prev;
  if (input1->prev != NULL) input1->prev->next = input1->next;
  if (input1->start != NULL) MListRemoveMonotonic(base, input1, 0);
  if (input1->end != NULL) MListRemoveMonotonic(base, input1, 1);
  input1->next = NULL;
  input1->prev = NULL;
  return;
}

static void CleanMonotonics(struct monotonic ** base_pointer) {
  // It is necessary to use double pointers so that we can set the previous reference.
  struct monotonic ** current_pointer = base_pointer;
  struct monotonic * tmp_pointer;
  while (*current_pointer) {
    if (((*current_pointer)->next == NULL) || ((*current_pointer)->prev == NULL)) {
      if (((*current_pointer)->next != NULL) || ((*current_pointer)->prev != NULL)) {
        SOError("Partially stranded monotonic.\n");
      } else {
        tmp_pointer = (*current_pointer)->linked;
        chunkfree(*current_pointer, sizeof(struct monotonic));
        (*current_pointer) = tmp_pointer;
      }
    }
    current_pointer = &((*current_pointer)->linked);
  }
  return;
}

static void MoveIntersection(Intersection *input2, real newx, real newy) {
  extended tmpt;
ValidateMListTs_IF_VERBOSE(input2->monos)
  // For each element in input2->monos, we want to remap intersections from input2 to input1.
  struct mlist * spline_mod;
  struct preintersection * tmppreinter;
  for (spline_mod = input2->monos; spline_mod != NULL; spline_mod = spline_mod->next) {
    if (spline_mod->isend) {
        // Adjust the t-value.
        tmpt = FixMonotonicT(spline_mod->m, spline_mod->m->tend, newx, newy);
        if (tmpt == -1 ) SOError("Fixup error 1 in MoveIntersection.\n");
        else {
          // We adjust the pre-intersections first.
          for (tmppreinter = spline_mod->m->pending; tmppreinter != NULL; tmppreinter = tmppreinter->next) {
            if (tmppreinter->m1 == spline_mod->m && tmppreinter->t1 == spline_mod->m->tend) tmppreinter->t1 = tmpt;
            else if (tmppreinter->m2 == spline_mod->m && tmppreinter->t2 == spline_mod->m->tend) tmppreinter->t2 = tmpt;
          }
          spline_mod->m->tend = tmpt; // Set the t-value.
          spline_mod->t = tmpt;
        }
        SONotify("Move 1 end.\n");
        // We also adjust the adjacent segment if necessary.
        if (spline_mod->m->next != NULL) {
          // Adjust the t-value.
          tmpt = FixMonotonicT(spline_mod->m->next, spline_mod->m->next->tstart, newx, newy);
          if (tmpt == -1 ) SOError("Fixup error 2 in MoveIntersection.\n");
          else {
            // We adjust the pre-intersections first.
            for (tmppreinter = spline_mod->m->pending; tmppreinter != NULL; tmppreinter = tmppreinter->next) {
              if (tmppreinter->m1 == spline_mod->m->next && tmppreinter->t1 == spline_mod->m->next->tstart) tmppreinter->t1 = tmpt;
              else if (tmppreinter->m2 == spline_mod->m->next && tmppreinter->t2 == spline_mod->m->next->tstart) tmppreinter->t2 = tmpt;
            }
            spline_mod->m->next->tstart = tmpt; // Set the t-value.
          }
          SONotify("Move 1 adjacent start.\n");
        }
    } else {
        // Adjust the t-value.
        tmpt = FixMonotonicT(spline_mod->m, spline_mod->m->tstart, newx, newy);
        if (tmpt == -1 ) SOError("Fixup error 3 in MoveIntersection.\n");
        else {
          // We adjust the pre-intersections first.
          for (tmppreinter = spline_mod->m->pending; tmppreinter != NULL; tmppreinter = tmppreinter->next) {
            if (tmppreinter->m1 == spline_mod->m && tmppreinter->t1 == spline_mod->m->tstart) tmppreinter->t1 = tmpt;
            else if (tmppreinter->m2 == spline_mod->m && tmppreinter->t2 == spline_mod->m->tstart) tmppreinter->t2 = tmpt;
          }
          spline_mod->m->tstart = tmpt; // Set the t-value.
          spline_mod->t = tmpt;
        }
        SONotify("Move 1 start.\n");
        // We also adjust the adjacent segment if necessary.
        if (spline_mod->m->prev != NULL) {
          // Adjust the t-value.
          tmpt = FixMonotonicT(spline_mod->m->prev, spline_mod->m->prev->tend, newx, newy);
          if (tmpt == -1 ) SOError("Fixup error 4 in MoveIntersection.\n");
          else {
            // We adjust the pre-intersections first.
            for (tmppreinter = spline_mod->m->pending; tmppreinter != NULL; tmppreinter = tmppreinter->next) {
              if (tmppreinter->m1 == spline_mod->m->prev && tmppreinter->t1 == spline_mod->m->prev->tend) tmppreinter->t1 = tmpt;
              else if (tmppreinter->m2 == spline_mod->m->prev && tmppreinter->t2 == spline_mod->m->prev->tend) tmppreinter->t2 = tmpt;
            }
            spline_mod->m->prev->tend = tmpt; // Set the t-value.
          }
          SONotify("Move 1 adjacent end.\n");
      }
    }
  }
  input2->inter.x = newx;
  input2->inter.y = newy;
  // Note that we do not fix up segment mappings after adjusting the t-values.
ValidateMListTs_IF_VERBOSE(input2->monos)
  return;
}

static void MergeIntersections(Intersection *input1, Intersection *input2) {
  extended tmpt;
ValidateMListTs_IF_VERBOSE(input1->monos)
ValidateMListTs_IF_VERBOSE(input2->monos)
  // For each element in input2->monos, we want to remap intersections from input2 to input1.
  struct mlist * spline_mod;
  struct preintersection * tmppreinter;
  SONotify("Merge %p (%f, %f) into %p (%f, %f).\n", input2, input2->inter.x, input2->inter.y,
    input1, input1->inter.x, input1->inter.y);
  struct mlist * previousmlist = NULL;
  for (spline_mod = input2->monos; spline_mod != NULL; previousmlist = spline_mod, spline_mod = spline_mod->next) {
    if (spline_mod->isend) {
      if (spline_mod->m->end == input2) {
        // Set the end of this spline to the right intersection only if the current value matches.
        spline_mod->m->end = input1;
        // Adjust the t-value.
        tmpt = FixMonotonicT(spline_mod->m, spline_mod->m->tend, input1->inter.x, input1->inter.y);
        if (tmpt == -1 ) SOError("Fixup error 1 in MergeIntersections.\n");
        else {
          // We adjust the pre-intersections first.
          for (tmppreinter = spline_mod->m->pending; tmppreinter != NULL; tmppreinter = tmppreinter->next) {
            if (tmppreinter->m1 == spline_mod->m->next && tmppreinter->t1 == spline_mod->m->next->tstart) tmppreinter->t1 = tmpt;
            else if (tmppreinter->m2 == spline_mod->m->next && tmppreinter->t2 == spline_mod->m->next->tstart) tmppreinter->t2 = tmpt;
          }
          spline_mod->m->tend = tmpt; // Set the t-value.
          spline_mod->t = tmpt;
        }
        SONotify("Remap 1 end.\n");
        // We also adjust the adjacent segment if necessary.
        if ((spline_mod->m->next != NULL) && (spline_mod->m->next->start == input2)) {
          // Set the start of this spline to the right intersection only if the current value matches.
          spline_mod->m->next->start = input1;
          // Adjust the t-value.
          tmpt = FixMonotonicT(spline_mod->m->next, spline_mod->m->next->tstart, input1->inter.x, input1->inter.y);
          if (tmpt == -1 ) SOError("Fixup error 2 in MergeIntersections.\n");
          else {
            // We adjust the pre-intersections first.
            for (tmppreinter = spline_mod->m->pending; tmppreinter != NULL; tmppreinter = tmppreinter->next) {
              if (tmppreinter->m1 == spline_mod->m->next && tmppreinter->t1 == spline_mod->m->next->tstart) tmppreinter->t1 = tmpt;
              else if (tmppreinter->m2 == spline_mod->m->next && tmppreinter->t2 == spline_mod->m->next->tstart) tmppreinter->t2 = tmpt;
            }
            spline_mod->m->next->tstart = tmpt; // Set the t-value.
            MListReplaceMonotonicT(input2->monos, spline_mod->m->next, 0, tmpt); // Reset the t-value for the mlist entry for that monotonic.
          }
          SONotify("Remap 1 adjacent start.\n");
          // Check for zero-length segments. (We cache the decision variables so that we don't dereference nulled pointers.)
          int zflag1 = 0, zflag2 = 0;
          if (MonotonicCheckZeroLength(spline_mod->m)) { zflag1 = 1; }
          if (MonotonicCheckZeroLength(spline_mod->m->next)) { zflag2 = 1; }
          if (zflag2) { MonotonicElide(&(input2->monos), spline_mod->m->next); SONotify("Remove zero-length segment.\n"); }
          if (zflag1) {
            MonotonicElide(&(input2->monos), spline_mod->m); SONotify("Remove zero-length segment.\n");
            if (previousmlist != NULL) spline_mod = previousmlist; else spline_mod = input2->monos;
          }
        }
      }
    } else {
      if (spline_mod->m->start == input2) {
        // Set the start of this spline to the right intersection only if the current value matches.
        spline_mod->m->start = input1;
        // Adjust the t-value.
        tmpt = FixMonotonicT(spline_mod->m, spline_mod->m->tstart, input1->inter.x, input1->inter.y);
        if (tmpt == -1 ) SOError("Fixup error 3 in MergeIntersections.\n");
        else {
          // We adjust the pre-intersections first.
          for (tmppreinter = spline_mod->m->pending; tmppreinter != NULL; tmppreinter = tmppreinter->next) {
            if (tmppreinter->m1 == spline_mod->m && tmppreinter->t1 == spline_mod->m->tstart) tmppreinter->t1 = tmpt;
            else if (tmppreinter->m2 == spline_mod->m && tmppreinter->t2 == spline_mod->m->tstart) tmppreinter->t2 = tmpt;
          }
          spline_mod->m->tstart = tmpt; // Set the t-value.
          spline_mod->t = tmpt;
        }
        SONotify("Remap 1 start.\n");
        // We also adjust the adjacent segment if necessary.
        if ((spline_mod->m->prev != NULL) && (spline_mod->m->prev->end == input2)) {
          // Set the end of this spline to the right intersection only if the current value matches.
          spline_mod->m->prev->end = input1;
          // Adjust the t-value.
          tmpt = FixMonotonicT(spline_mod->m->prev, spline_mod->m->prev->tend, input1->inter.x, input1->inter.y);
          if (tmpt == -1 ) SOError("Fixup error 4 in MergeIntersections.\n");
          else {
            // We adjust the pre-intersections first.
            for (tmppreinter = spline_mod->m->pending; tmppreinter != NULL; tmppreinter = tmppreinter->next) {
              if (tmppreinter->m1 == spline_mod->m->prev && tmppreinter->t1 == spline_mod->m->prev->tend) tmppreinter->t1 = tmpt;
              else if (tmppreinter->m2 == spline_mod->m->prev && tmppreinter->t2 == spline_mod->m->prev->tend) tmppreinter->t2 = tmpt;
            }
            spline_mod->m->prev->tend = tmpt;
            MListReplaceMonotonicT(input2->monos, spline_mod->m->prev, 1, tmpt); // Reset the t-value for the mlist entry for that monotonic.
          }
          SONotify("Remap 1 adjacent end.\n");
          // Check for zero-length segments. (We cache the decision variables so that we don't dereference nulled pointers.)
          int zflag1 = 0, zflag2 = 0;
          if (MonotonicCheckZeroLength(spline_mod->m)) { zflag1 = 1; }
          if (MonotonicCheckZeroLength(spline_mod->m->prev)) { zflag2 = 1; }
          if (zflag2) { MonotonicElide(&(input2->monos), spline_mod->m->prev); SONotify("Remove zero-length segment.\n"); }
          if (zflag1) {
            MonotonicElide(&(input2->monos), spline_mod->m); SONotify("Remove zero-length segment.\n");
            if (previousmlist != NULL) spline_mod = previousmlist; else spline_mod = input2->monos;
          }
        }
      }
    }
  }
  if (input1->monos == NULL) {
    input1->monos = input2->monos;
    input2->monos = NULL;
  } else {
    // Find the end of input1->monos.
    struct mlist * output_list_pos = input1->monos;
    while (output_list_pos != NULL && output_list_pos->next != NULL) output_list_pos = output_list_pos->next;
    // So output_list_pos->next is null.
    output_list_pos->next = input2->monos;
    input2->monos = NULL;
  }
  // Remove references to invalid segments.
  MListCleanEmpty(&input1->monos);
  // Note that we do not fix up segment mappings after adjusting the t-values.
ValidateMListTs_IF_VERBOSE(input1->monos)
  return;
}

static void AddSpline(Intersection *il,Monotonic *m,extended t) {
    MList *ml;

ValidateMListTs_IF_VERBOSE(il->monos)
// Validate(m, NULL);
    if ( m->start==il || m->end==il )
return;

    for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	if ( ml->s==m->s && RealWithin( ml->t,t,.0001 )) {
          SONotify("No spline duplicate added due to small t difference.\n");
          return;
        }
    }

    if (( t-m->tstart < m->tend-t ) && ((m->tstart == t) || (Within4RoundingErrors(m->tstart,t) && ( m->start==NULL || (
           Within16RoundingErrors(m->start->inter.x,il->inter.x) &&
           Within16RoundingErrors(m->start->inter.y,il->inter.y)))))) {
      // If the intersection is closer to the reported start of the segment than to the end, we examine the start point.
      // We trigger if the t-value is close and either there isn't an existing intersection or they are geometrically close.
      // Or alternately if the t-value matches exactly.
      // That indicates that something elsewhere is not sufficiently precise, but what can we do?
        // If the intersection is very close to the start point, set the segment start to the intersection.
	if ( m->start!=NULL && m->start!=il ) {
	    SONotify("Resetting start. was: (%g,%g) now: (%g,%g)\n",
		    (double) m->start->inter.x, (double) m->start->inter.y, (double) il->inter.x, (double) il->inter.y);
	    MergeIntersections(m->start, il); il = m->start;
        }
	m->start = il;
	_AddSpline(il,m,m->tstart,false);
	if (m->prev != NULL) _AddSpline(il,m->prev,m->prev->tend,true);
    } else if ((t-m->tstart > m->tend-t) && ((m->tend == t) || 
           (Within4RoundingErrors(m->tend,t) && ( m->end==NULL || (
           Within16RoundingErrors(m->end->inter.x,il->inter.x) &&
           Within16RoundingErrors(m->end->inter.y,il->inter.y)))))) {
        // If the intersection is very close to the end point, set the segment end to the intersection.
	if ( m->end!=NULL && m->end!=il ) {
	    SONotify("Resetting end. was: (%g,%g) now: (%g,%g)\n",
		    (double) m->end->inter.x, (double) m->end->inter.y, (double) il->inter.x, (double) il->inter.y);
	    MergeIntersections(m->end, il); il = m->end;
        }
	m->end = il;
	_AddSpline(il,m,m->tend,true);
	if (m->next != NULL) _AddSpline(il,m->next,m->next->tstart,false);
    } else if ((m->s != NULL) && Within4RoundingErrors(t, 0) &&
	       Within4RoundingErrors(il->inter.x, m->s->from->me.x) && Within4RoundingErrors(il->inter.y, m->s->from->me.y)) {
	        SONotify("Move the intersection to the beginning of the spline.\n");
	        MoveIntersection(il, m->s->from->me.x, m->s->from->me.y);
	        m->start = il;
                _AddSpline(il,m,0,false);
                if (m->prev != NULL) _AddSpline(il,m->prev, m->prev->tend, true);
    } else if ((m->s != NULL) && Within4RoundingErrors(t, 1) &&
	       Within4RoundingErrors(il->inter.x, m->s->to->me.x) && Within4RoundingErrors(il->inter.y, m->s->to->me.y)) {
	        SONotify("Move the intersection to the end of the spline.\n");
	        MoveIntersection(il, m->s->to->me.x, m->s->to->me.y);
	        m->end = il;
                _AddSpline(il,m,1,true);
                if (m->next != NULL) _AddSpline(il,m->next, m->next->tstart, false);
    } else if ((m->start != NULL) && (m->start->inter.x == il->inter.x) && (m->start->inter.y == il->inter.y)) {
	      if (m->start != il) {
                SONotify("It's an exact match, so we merge the two intersections.\n");
                MergeIntersections(m->start, il); il = m->start;
	        _AddSpline(il,m,m->tstart,false);
	        if (m->prev != NULL) _AddSpline(il,m->prev,m->prev->tend,true);
	      } else SOError("Duplicate monotonic on this intersection.\n");

    } else if ((m->end != NULL) && (m->end->inter.x == il->inter.x) && (m->end->inter.y == il->inter.y)) {
	      if (m->end != il) {
	        SONotify("It's an exact match, so we merge the two intersections.\n");
	        MergeIntersections(m->end, il); il = m->end;
	        _AddSpline(il,m,m->tend,true);
	        if (m->next != NULL) _AddSpline(il,m->next,m->next->tstart,false);
	      } else SOError("Duplicate monotonic on this intersection.\n");
    } else {
	/* Ok, if we've got a new intersection on this spline then break up */
	/*  the monotonic into two bits which end and start at this inter */
	if ( t<=m->tstart || t>=m->tend )
	    SOError( "Attempt to subset monotonic rejoin inappropriately: t = %g should be in (%g,%g)\n",
		    t, m->tstart, m->tend );
	else if (m->tstart == m->tend)
	    SOError( "Attempt to subset monotonic rejoin inappropriately: m->tstart and m->tend are equal (%f = %f, t = %f)\n",
		    m->tstart, m->tend, t );
	else if (Within16RoundingErrors(m->tstart, m->tend))
	    SOError( "Attempt to subset monotonic rejoin inappropriately: m->tstart and m->tend are very close (%f = %f, t = %f)\n",
		    m->tstart, m->tend, t );
        else if (Within16RoundingErrors(m->tstart, m->tend))
	    SOError( "Attempt to subset monotonic rejoin inappropriately: m->tstart and m->tend are very close (%f = %f, t = %f)\n",
		    m->tstart, m->tend, t );
	else if (Within4RoundingErrors(m->s->from->me.x,m->s->to->me.x) && Within4RoundingErrors(m->s->from->me.y,m->s->to->me.y))
	    SOError( "The curve is too short.\n");
        else {
	    /* It is monotonic, so a subset of it must also be */
	    Monotonic *m2 = chunkalloc(sizeof(Monotonic));
	    BasePoint pt, inter;
            BasePoint oldend;
            if (m->end != NULL) oldend = m->end->inter;
            else {
              oldend.x = 0.0;
              oldend.y = 0.0;
            }
            extended oldtend = m->tend;
	    *m2 = *m;
	    m2->pending = NULL;
	    m->next = m2;
	    m2->prev = m;
	    m2->next->prev = m2;
	    m2->end = m->end;
	    m2->tend = m->tend;
	    m->linked = m2;
	    m->tend = t;
	    m->end = il;
	    m2->start = il;
	    m2->tstart = t;
#ifdef FF_RELATIONAL_GEOM
	    m2->otend = m->otend;
	    m->otend = t;
	    m2->otstart = t;
#endif
	    if ( m->start!=NULL )
		pt = m->start->inter;
	    else {
		pt.x = ((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+
			m->s->splines[0].c)*m->tstart+m->s->splines[0].d;
		pt.y = ((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+
			m->s->splines[1].c)*m->tstart+m->s->splines[1].d;
	    }
	/* t may not be perfectly correct (because the correct value isn't expressable) */
	/* so evalutating the spline at t may produce a slight variation */
	/* now if t is a double and inter.x/y are floats that doesn't matter */
	/* but if both are doubles then it does */
	/*  Similar behavior seems needed above and below where we test against m->start/m->end */
	    inter = il->inter;
	    if ( pt.x>inter.x ) {
		m->b.minx = inter.x;
		m->b.maxx = pt.x;
	    } else {
		m->b.minx = pt.x;
		m->b.maxx = inter.x;
	    }
	    if ( pt.y>inter.y ) {
		m->b.miny = inter.y;
		m->b.maxy = pt.y;
	    } else {
		m->b.miny = pt.y;
		m->b.maxy = inter.y;
	    }
	    if ( m2->end!=NULL )
		pt = m2->end->inter;
	    else {
		pt.x = ((m2->s->splines[0].a*m2->tend+m2->s->splines[0].b)*m2->tend+
			m2->s->splines[0].c)*m2->tend+m2->s->splines[0].d;
		pt.y = ((m2->s->splines[1].a*m2->tend+m2->s->splines[1].b)*m2->tend+
			m2->s->splines[1].c)*m2->tend+m2->s->splines[1].d;
	    }
	    if ( pt.x>inter.x ) {
		m2->b.minx = inter.x;
		m2->b.maxx = pt.x;
	    } else {
		m2->b.minx = pt.x;
		m2->b.maxx = inter.x;
	    }
	    if ( pt.y>inter.y ) {
		m2->b.miny = inter.y;
		m2->b.maxy = pt.y;
	    } else {
		m2->b.miny = pt.y;
		m2->b.maxy = inter.y;
	    }
	    SONotify("Segment on t = %f between %f and %f ((%f, %f) between (%f, %f) and (%f, %f)).\n", t, m->tstart, oldtend,
              il->inter.x, il->inter.y, m->start ? m->start->inter.x : 0.0, m->start ? m->start->inter.y : 0.0,
              oldend.x, oldend.y);
	    SONotify("Or, rather, between (%f, %f) and (%f, %f)).\n",
              m->s ? m->s->from->me.x : 0.0, m->s ? m->s->from->me.y : 0.0,
              m->s ? m->s->to->me.x : 0.0, m->s ? m->s->to->me.y : 0);
	      _AddSpline(il,m,t,true);
	      _AddSpline(il,m2,t,false);
	      // If the end of m before break-up has a reference to m, we must replace that reference with one to m2.
	      if (m2->end != NULL) MListReplaceMonotonic(m2->end->monos, m, m2, true);
// ValidateMonotonic(m);
// ValidateMonotonic(m2);
	}
    }
ValidateMListTs_IF_VERBOSE(il->monos)
// ValidateMonotonic(m);
// Validate(m, NULL);
}

static void SetStartPoint(BasePoint *pt,Monotonic *m) {
    if ( m->start!=NULL )
	*pt = m->start->inter;
    else if ( m->tstart==0 )
	*pt = m->s->from->me;
    else {
	pt->x = ((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart +
		m->s->splines[0].c)*m->tstart + m->s->splines[0].d;
	pt->y = ((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart +
		m->s->splines[1].c)*m->tstart + m->s->splines[1].d;
    }
}

static void SetEndPoint(BasePoint *pt,Monotonic *m) {
    if ( m->end!=NULL )
	*pt = m->end->inter;
    else if ( m->tend==1.0 )
	*pt = m->s->to->me;
    else {
	pt->x = ((m->s->splines[0].a*m->tend+m->s->splines[0].b)*m->tend +
		m->s->splines[0].c)*m->tend + m->s->splines[0].d;
	pt->y = ((m->s->splines[1].a*m->tend+m->s->splines[1].b)*m->tend +
		m->s->splines[1].c)*m->tend + m->s->splines[1].d;
    }
}

static int CloserT(Spline *s1,bigreal test,bigreal current,Spline *s2,bigreal t2 ) {
    bigreal basex=((s2->splines[0].a*t2+s2->splines[0].b)*t2+s2->splines[0].c)*t2+s2->splines[0].d;
    bigreal basey=((s2->splines[1].a*t2+s2->splines[1].b)*t2+s2->splines[1].c)*t2+s2->splines[1].d;
    bigreal testx=((s1->splines[0].a*test+s1->splines[0].b)*test+s1->splines[0].c)*test+s1->splines[0].d;
    bigreal testy=((s1->splines[1].a*test+s1->splines[1].b)*test+s1->splines[1].c)*test+s1->splines[1].d;
    bigreal curx=((s1->splines[0].a*current+s1->splines[0].b)*current+s1->splines[0].c)*current+s1->splines[0].d;
    bigreal cury=((s1->splines[1].a*current+s1->splines[1].b)*current+s1->splines[1].c)*current+s1->splines[1].d;

return( (testx-basex)*(testx-basex) + (testy-basey)*(testy-basey) <=
	(curx-basex)*(curx-basex) + (cury-basey)*(cury-basey) );
}

static void ILReplaceMono(Intersection *il,Monotonic *m,Monotonic *otherm) {
    MList *ml;

    for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	if ( ml->m==m ) {
	    ml->m = otherm;
    break;
	}
    }
}

struct inter_data {
    Monotonic *m, *otherm;
    bigreal t, othert;
    BasePoint inter;
    int new;
};

static void SplitMonotonicAtT(Monotonic *m,int which,bigreal t,bigreal coord,
	struct inter_data *id) {
// Validate(m, NULL);
    Monotonic *otherm = NULL;
    bigreal othert;
    real cx,cy;
    Spline1D *sx, *sy;

    sx = &m->s->splines[0]; sy = &m->s->splines[1];
    cx = ((sx->a*t+sx->b)*t+sx->c)*t+sx->d;
    cy = ((sy->a*t+sy->b)*t+sy->c)*t+sy->d;
    /* t might not be tstart/tend, but it could still produce a point which */
    /*  (after rounding errors) is at the start/end point of the monotonic */
    if ( t<=m->tstart || t>=m->tend ||
	    ((cx<=m->b.minx || cx>=m->b.maxx) && (cy<=m->b.miny || cy>=m->b.maxy))) {
	struct intersection *pt=NULL;
	if ( t-m->tstart<m->tend-t ) {
	    t = m->tstart;
	    otherm = m->prev;
	    othert = m->prev->tend;
	    pt = m->start;
	} else {
	    t = m->tend;
	    otherm = m->next;
	    othert = m->next->tstart;
	    pt = m->end;
	}
	sx = &m->s->splines[0]; sy = &m->s->splines[1];
	cx = ((sx->a*t+sx->b)*t+sx->c)*t+sx->d;
	cy = ((sy->a*t+sy->b)*t+sy->c)*t+sy->d;
	if ( which==1 ) cy = coord; else if ( which==0 ) cx = coord;	/* Correct for rounding errors */
	if ( pt!=NULL ) { cx = pt->inter.x; cy = pt->inter.y; }
	id->new = false;
    } else {
	SONotify("Break monotonic from t = %f to t = %f at t = %f.\n", m->tstart, m->tend, t);
	othert = t;
	otherm = chunkalloc(sizeof(Monotonic));
	*otherm = *m;
	otherm->pending = NULL;
	m->next = otherm;
	m->linked = otherm;
	otherm->prev = m;
	otherm->next->prev = otherm;
	m->tend = t;
	if ( otherm->end!=NULL ) {
	    m->end = NULL;
	    ILReplaceMono(otherm->end,m,otherm);
	}
	otherm->tstart = t; otherm->start = NULL;
        otherm->tend = m->tend; otherm->end = m->end; // Frank added this.
	m->end = NULL;
#ifdef FF_RELATIONAL_GEOM
        otherm->otend = m->otend;
	m->otend = t;
	otherm->otstart = t;
#endif
	if ( which==1 ) cy = coord; else if ( which==0 ) cx = coord;	/* Correct for rounding errors */
	if ( m->xup ) {
	    m->b.maxx = otherm->b.minx = cx;
	} else {
	    m->b.minx = otherm->b.maxx = cx;
	}
	if ( m->yup ) {
	    m->b.maxy = otherm->b.miny = cy;
	} else {
	    m->b.miny = otherm->b.maxy = cy;
	}
	id->new = true;
    }
    id->m = m; id->otherm = otherm;
    id->t = t; id->othert = othert;
    id->inter.x = cx; id->inter.y = cy;
// Validate(m, NULL);
}

static extended RealDistance(extended v1, extended v2) {
  if (v2 > v1) return v2 - v1;
  else if (v2 < v1) return v1 - v2;
  return 0.0;
}

static int RealCloser(extended ref0, extended ref1, extended queryval) {
  if (RealDistance(ref1, queryval) < RealDistance(ref0, queryval)) return 1;
  return 0;
}

static void SplitMonotonicAtFlex(Monotonic *m,int which,bigreal coord,
	struct inter_data *id, int doit) {
    bigreal t=0;
    int low=0, high=0;
    extended startx, starty, endx, endy;
    {
      // We set our fallback values.
      if (m->tstart == 0) {
        startx = m->s->from->me.x;
        starty = m->s->from->me.y;
      } else if (m->start != NULL) {
        startx = m->start->inter.x;
        starty = m->start->inter.y;
      } else {
        startx = evalSpline(m->s, m->tstart, 0);
        starty = evalSpline(m->s, m->tstart, 1);
      }
      if (m->tend == 1) {
        endx = m->s->to->me.x;
        endy = m->s->to->me.y;
      } else if (m->end != NULL) {
        endx = m->end->inter.x;
        endy = m->end->inter.y;
      } else {
        endx = evalSpline(m->s, m->tend, 0);
        endy = evalSpline(m->s, m->tend, 1);
      }
    }

    if (( which==0 && coord<=m->b.minx ) || (which==1 && coord<=m->b.miny)) {
	low = true;
	if (( which==0 && coord<m->b.minx ) || (which==1 && coord<m->b.miny))
	  SOError("Coordinate out of range.\n");
    } else if ( (which==0 && coord==m->b.maxx) || (which==1 && coord==m->b.maxy) ) {
	high = true;
	if (( which==0 && coord>m->b.maxx ) || (which==1 && coord>m->b.maxy))
	  SOError("Coordinate out of range.\n");
    }

    if ( low || high ) {
	if ( (low && (&m->xup)[which]) || (high && !(&m->xup)[which]) ) {
	    t = m->tstart;
	} else if ( (low && !(&m->xup)[which]) || (high && (&m->xup)[which]) ) {
	    t = m->tend;
	}
    } else {
	t = IterateSplineSolveFixup(&m->s->splines[which],m->tstart,m->tend,coord);
	// Generally, this fails not because the value is far out of bounds but because it's very near to one of the ends
        // (in which case the solver may not be able to find a t-value that produces the desired coordinate)
        // or because it's just out bounds by a little bit due to rounding errors and nudging and such.
        // In the second case, we could navigate into the adjacent monotonic and try to put a new intersection there,
        // but it's more likely that the desired/expected result is putting the point at the end of the segment.
	if ( t==-1 ) {
          // If the solver fails, we try to match an end if feasible.
	  if (which) {
            if (RealCloser(starty, endy, coord)) {
              if (Within16RoundingErrors(coord, endy)) t = m->tend;
            } else {
              if (Within16RoundingErrors(coord, starty)) t = m->tstart;
            }
          } else {
            if (RealCloser(startx, endx, coord)) {
              if (Within16RoundingErrors(coord, endx)) t = m->tend;
            } else {
              if (Within16RoundingErrors(coord, startx)) t = m->tstart;
            }
          }
          if (t != -1) SONotify("Spline solver failed to find a value; falling back to approximate monotonic end.\n");
	}
	if ( t==-1 ) {
          // If that matching fails, we accept some extra fuzziness.
	  if (which) {
            if (RealCloser(starty, endy, coord)) {
              if (RealNear(coord, endy)) t = m->tend;
            } else {
              if (RealNear(coord, starty)) t = m->tstart;
            }
          } else {
            if (RealCloser(startx, endx, coord)) {
              if (RealNear(coord, endx)) t = m->tend;
            } else {
              if (RealNear(coord, startx)) t = m->tstart;
            }
          }
          if (t != -1) SONotify("Spline solver failed to find a value; falling back to roughly approximate monotonic end.\n");
	}
        if (t == -1)
	    SOError("Intersection failed!\n");
    }
    if ((t == m->tend)
#ifdef FF_RELATIONAL_GEOM
       || (t > m->tend && t <= m->otend)
#endif // FF_RELATIONAL_GEOM
      ) {
      SONotify("We do not split at the end.\n");
      id->m = m; id->t;
      id->otherm = NULL; id->othert = 0; // TODO
      if (t == 1) {
        id->inter.x = m->s->to->me.x;
        id->inter.y = m->s->to->me.y;
      } else if (m->end != NULL) {
        id->inter.x = m->end->inter.x;
        id->inter.y = m->end->inter.y;
      } else {
        SOError("There is neither a spline end nor an intersection at the end of this monotonic.\n");
        id->inter.x = evalSpline(m->s, t, 0);
        id->inter.y = evalSpline(m->s, t, 0);
      }
    } else if ((t == m->tstart)
#ifdef FF_RELATIONAL_GEOM
       || (t < m->tstart && t >= m->otstart)
#endif // FF_RELATIONAL_GEOM
      ) {
      SONotify("We do not split at the start.\n");
      id->m = m; id->t;
      id->otherm = NULL; id->othert = 0;
      if (t == 0) {
        id->inter.x = m->s->from->me.x;
        id->inter.y = m->s->from->me.y;
      } else if (m->start != NULL) {
        id->inter.x = m->start->inter.x;
        id->inter.y = m->start->inter.y;
      } else {
        SOError("There is neither a spline end nor an intersection at the start of this monotonic.\n");
        id->inter.x = evalSpline(m->s, t, 0);
        id->inter.y = evalSpline(m->s, t, 0);
      }
    } else if (t != -1) {
      if (Within16RoundingErrors(t,m->tstart) || Within16RoundingErrors(t,m->tend)) {
        SOError("We're about to create a spline with a very small t-value.\n");
      }
      if (doit) SplitMonotonicAtT(m,which,t,coord,id);
      else {
        id->new = 1;
        id->t = t;
        id->inter.x = evalSpline(m->s, t, 0);
        id->inter.y = evalSpline(m->s, t, 1);
      }
    } else {
        id->t = t;
        id->inter.x = 0;
        id->inter.y = 0;
    }
}

static void SplitMonotonicAt(Monotonic *m,int which,bigreal coord,
	struct inter_data *id) {
  SplitMonotonicAtFlex(m, which, coord, id, 1);
}

static void SplitMonotonicAtFake(Monotonic *m,int which,bigreal coord,
	struct inter_data *id) {
  SplitMonotonicAtFlex(m, which, coord, id, 0);
}

/* An IEEE double has 52 bits of precision. So one unit of rounding error will be */
/*  the number divided by 2^51 */
# define BR_RE_Factor	(1024.0*1024.0*1024.0*1024.0*1024.0*2.0)
/* But that's not going to work near 0, so, since the t values we care about */
/*  are [0,1], let's use 1.0/D_RE_Factor */

static int ImproveInter(Monotonic *m1, Monotonic *m2,
	extended *_t1,extended *_t2,BasePoint *inter) {
    Spline *s1 = m1->s, *s2 = m2->s;
    extended x1, x2, y1, y2;
    extended t1p, t1m, t2p, t2m;
    extended x1p, x1m, x2p, x2m, y1p, y1m, y2p, y2m;
    extended error, errors[9], beste;
    int i, besti;
    extended factor;
    extended t1,t2;
    int cnt=1, clamp;
    /* We want to find (t1,t2) so that (m1(t1)-m2(t2))^2==0 */
    /* Make slight adjustments to the t?s in all directions and see if that */
    /*  improves things */
    /* We know that the current values of (t1,t2) are close to an intersection*/

    t1=*_t1, t2=*_t2;

    x1 = ((s1->splines[0].a*t1 + s1->splines[0].b)*t1 + s1->splines[0].c)*t1 + s1->splines[0].d;
    x2 = ((s2->splines[0].a*t2 + s2->splines[0].b)*t2 + s2->splines[0].c)*t2 + s2->splines[0].d;
    y1 = ((s1->splines[1].a*t1 + s1->splines[1].b)*t1 + s1->splines[1].c)*t1 + s1->splines[1].d;
    y2 = ((s2->splines[1].a*t2 + s2->splines[1].b)*t2 + s2->splines[1].c)*t2 + s2->splines[1].d;
    error = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
    if ( error==0 )
return( true );
    for ( clamp = 1; clamp>=0; --clamp ) {
	factor = sqrt(error); /* 32*1024.0*1024.0*1024.0/BR_RE_Factor;*/
	for ( cnt=0; cnt<51; ++cnt ) {
	    extended off1 = factor*t1;
	    extended off2 = factor*t2;
	    if ( t1<.0001 ) off1 = factor;
	    if ( t2<.0001 ) off2 = factor;
	    if ( clamp ) {
		if (( t1p = t1+off1 )>m1->tend ) t1p = m1->tend;
		if (( t1m = t1-off1 )<m1->tstart ) t1m = m1->tstart;
		if (( t2p = t2+off2 )>m2->tend ) t2p = m2->tend;
		if (( t2m = t2-off2 )<m2->tstart ) t2m = m2->tstart;
	    } else {
		t1p = t1+off1;
		t1m = t1-off1;
		t2p = t2+off2;
		t2m = t2-off2;
	    }
	    if ( t1p==t1 && t2p==t2 )
	break;
    
	    x1p = ((s1->splines[0].a*t1p + s1->splines[0].b)*t1p + s1->splines[0].c)*t1p + s1->splines[0].d;
	    x2p = ((s2->splines[0].a*t2p + s2->splines[0].b)*t2p + s2->splines[0].c)*t2p + s2->splines[0].d;
	    y1p = ((s1->splines[1].a*t1p + s1->splines[1].b)*t1p + s1->splines[1].c)*t1p + s1->splines[1].d;
	    y2p = ((s2->splines[1].a*t2p + s2->splines[1].b)*t2p + s2->splines[1].c)*t2p + s2->splines[1].d;
	    x1m = ((s1->splines[0].a*t1m + s1->splines[0].b)*t1m + s1->splines[0].c)*t1m + s1->splines[0].d;
	    x2m = ((s2->splines[0].a*t2m + s2->splines[0].b)*t2m + s2->splines[0].c)*t2m + s2->splines[0].d;
	    y1m = ((s1->splines[1].a*t1m + s1->splines[1].b)*t1m + s1->splines[1].c)*t1m + s1->splines[1].d;
	    y2m = ((s2->splines[1].a*t2m + s2->splines[1].b)*t2m + s2->splines[1].c)*t2m + s2->splines[1].d;
	    errors[0] = (x1m-x2m)*(x1m-x2m) + (y1m-y2m)*(y1m-y2m);
	    errors[1] = (x1m-x2)*(x1m-x2) + (y1m-y2)*(y1m-y2);
	    errors[2] = (x1m-x2p)*(x1m-x2p) + (y1m-y2p)*(y1m-y2p);
	    errors[3] = (x1-x2m)*(x1-x2m) + (y1-y2m)*(y1-y2m);
	    errors[4] = error;
	    errors[5] = (x1-x2p)*(x1-x2p) + (y1-y2p)*(y1-y2p);
	    errors[6] = (x1p-x2m)*(x1p-x2m) + (y1p-y2m)*(y1p-y2m);
	    errors[7] = (x1p-x2)*(x1p-x2) + (y1p-y2)*(y1p-y2);
	    errors[8] = (x1p-x2p)*(x1p-x2p) + (y1p-y2p)*(y1p-y2p);
	    besti = -1; beste = error;
	    for ( i=0; i<9; ++i ) {
		if ( errors[i]<beste ) {
		    besti = i;
		    beste = errors[i];
		}
	    }
	    if ( besti!=-1 ) {
		if ( besti<3 ) { t1 = t1m; x1=x1m; y1=y1m; }
		else if ( besti>5 ) { t1 = t1p; x1=x1p; y1=y1p; }
		if ( besti%3==0 ) { t2 = t2m; x2 = x2m; y2=y2m; }
		else if ( besti%3==2 ) { t2 = t2p; x2=x2p; y2=y2p; }
		if ( t1<m1->tstart || t1>m1->tend || t2<m2->tstart || t2>m2->tend )
return( false );
		error = beste;
		if ( beste==0 )
	break;
	    }
	    factor/=2;
	    if ( factor<1.0/BR_RE_Factor )
	break;
	}
	if ( Within4RoundingErrors(x1,x2) && Within4RoundingErrors(y1,y2))
    break;
    }
    if ( !RealWithin(x1,x2,.005) || !RealWithin(y1,y2,.005))
return( false );
    inter->x = (x1+x2)/2; inter->y = (y1+y2)/2;
    *_t1 = t1; *_t2 = t2;
return( true );
}

static Intersection *_AddIntersection(Intersection *ilist,Monotonic *m1,
	Monotonic *m2,extended t1,extended t2,BasePoint *inter) {
    Intersection *il, *closest=NULL;
    bigreal dist, dx, dy, bestd=9e10;

    // We first search for an existing intersection.
    /* I tried changing from Within16 to Within64 here, and below, and the */
    /*  result was that I cause more new errors (about 6) than I fixed old(1) */
    for ( il = ilist; il!=NULL; il=il->next ) {
ValidateMListTs_IF_VERBOSE(il->monos)
	SONotify("Compare (%f, %f) to (%f, %f): x %s, y %s...", inter->x, inter->y, il->inter.x, il->inter.y,
          Within4RoundingErrors(il->inter.x,inter->x) ? "yes" : "no",
          Within4RoundingErrors(il->inter.y,inter->y) ? "yes" : "no");
	if ( Within16RoundingErrors(il->inter.x,inter->x) && Within16RoundingErrors(il->inter.y,inter->y)) {
            SONotify(" maybe.\n");
	    if ( (dx = il->inter.x-inter->x)<0 ) dx = -dx; // We want absolute values.
	    if ( (dy = il->inter.y-inter->y)<0 ) dy = -dy;
	    dist = dx+dy; // Calculate rough distance and check whether this is the closest existing intersection.
	    if ( dist<bestd ) {
		bestd = dist;
		closest = il;
		if ( dist==0 )
    break;
	    }
	} else {
          SONotify(" off by (%.12f, %.12f).\n", il->inter.x-inter->x, il->inter.y-inter->y);
        }
    }

    if ( m1->tstart==0 && m1->start==NULL &&
	    Within16RoundingErrors(m1->s->from->me.x,inter->x) && Within16RoundingErrors(m1->s->from->me.y,inter->y)) {
        // If the spline starts close to the intersection, move the intersection to the beginning of the spline.
	t1=0;
        if (m1->s->from->me.x != inter->x && m1->s->from->me.y != inter->y) {
	  *inter = m1->s->from->me;
          SONotify("Nudge intersection from (%f, %f) to spline point (%f, %f).\n", inter->x, inter->y, m1->s->from->me.x, m1->s->from->me.y);
        }
    } else if ( m1->tend==1.0 && m1->end==NULL &&
	    Within16RoundingErrors(m1->s->to->me.x,inter->x) && Within16RoundingErrors(m1->s->to->me.y,inter->y)) {
        // If the spline ends close to the intersection, move the intersection to the end of the spline.
	t1=1.0;
        if (m1->s->to->me.x != inter->x && m1->s->to->me.y != inter->y) {
	  *inter = m1->s->to->me;
          SONotify("Nudge intersection from (%f, %f) to spline point (%f, %f).\n", inter->x, inter->y, m1->s->to->me.x, m1->s->to->me.y);
        }
    } else if ( m2->tstart==0 && m2->start==NULL &&
	    Within16RoundingErrors(m2->s->from->me.x,inter->x) && Within16RoundingErrors(m2->s->from->me.y,inter->y)) {
        // If the spline starts close to the intersection, move the intersection to the beginning of the spline.
	t2=0;
        if (m2->s->from->me.x != inter->x && m2->s->from->me.y != inter->y) {
	  *inter = m2->s->from->me;
          SONotify("Nudge intersection from (%f, %f) to spline point (%f, %f).\n", inter->x, inter->y, m2->s->from->me.x, m2->s->from->me.y);
        }
    } else if ( m2->tend==1.0 && m2->end==NULL &&
	    Within16RoundingErrors(m2->s->to->me.x,inter->x) && Within16RoundingErrors(m2->s->to->me.y,inter->y)) {
        // If the spline ends close to the intersection, move the intersection to the end of the spline.
	t2=1.0;
        if (m2->s->to->me.x != inter->x && m2->s->to->me.y != inter->y) {
	  *inter = m2->s->to->me;
          SONotify("Nudge intersection from (%f, %f) to spline point (%f, %f).\n", inter->x, inter->y, m2->s->to->me.x, m2->s->to->me.y);
        }
    }


    // If the provided (and now adjusted) intersection matches a spline start or end perfectly
    // and the closest intersection does not, we void the closest intersection.
    if ( closest!=NULL && (closest->inter.x!=inter->x || closest->inter.y!=inter->y ) &&
	    ((t1==0 && m1->s->from->me.x==inter->x && m1->s->from->me.y==inter->y) ||
	     (t1==1 && m1->s->to->me.x==inter->x && m1->s->to->me.y==inter->y) ||
	     (t2==0 && m2->s->from->me.x==inter->x && m2->s->from->me.y==inter->y) ||
	     (t2==1 && m2->s->to->me.x==inter->x && m2->s->to->me.y==inter->y)))
	closest = NULL;

    // If we are not reusing a point, make one.
    if ( closest==NULL ) {
        SONotify("New inter at (%f, %f).\n", inter->x, inter->y);
	closest = chunkalloc(sizeof(Intersection));
	closest->inter = *inter;
	closest->next = ilist;
	ilist = closest;
        // Add the splines to the list in the intersection.
        AddSpline(closest,m1,t1);
        AddSpline(closest,m2,t2);
ValidateMListTs_IF_VERBOSE(closest->monos)
        if (closest->monos == NULL) {
          SONotify("Never mind that new point.\n");
          ilist = closest->next;
          chunkfree(closest, sizeof(Intersection)); closest = NULL;
        }
    } else {
        SONotify("Old inter at (%f, %f).\n", closest->inter.x, closest->inter.y);
        // Add the splines to the list in the intersection.
        AddSpline(closest,m1,t1);
        AddSpline(closest,m2,t2);
ValidateMListTs_IF_VERBOSE(closest->monos)
    }
          for ( il = ilist; il!=NULL; il=il->next ) {
ValidateMListTs_IF_VERBOSE(il->monos)
          }
return( ilist );
}

static Intersection *AddIntersection(Intersection *ilist,Monotonic *m1,
	Monotonic *m2,extended t1,extended t2,BasePoint *inter) {
    Intersection *il;
    extended ot1 = t1, ot2 = t2;
// ValidateMonotonic(m1);
// ValidateMonotonic(m2);
    for ( il = ilist; il!=NULL; il=il->next ) {
ValidateMListTs_IF_VERBOSE(il->monos)
    }
    /* This is just a join between two adjacent monotonics. There might already*/
    /*  be an intersection there, but if there be, we've already found it */
    /* Do this now, because no point wasting the time it takes to ImproveInter*/
    if (( m1->next==m2 && (t1==t2 || (t1==1.0 && t2==0.0))) ||
	( m2->next==m1 && (t2==t1 || (t2==1.0 && t1==0.0))) )
return( ilist );

    /* Fixup some rounding errors */
    if ( !ImproveInter(m1,m2,&t1,&t2,inter))
return( ilist );

    /* Yeah, I know we just did this, but ImproveInter might have smoothed out*/
    /*  some rounding errors */
    if (( m1->next==m2 && (t1==t2 || (t1==1.0 && t2==0.0))) ||
	( m2->next==m1 && (t2==t1 || (t2==1.0 && t1==0.0))) )
return( ilist );

    if (( inter->x<=m1->b.minx || inter->x>=m1->b.maxx ) &&
	    (inter->y<=m1->b.miny || inter->y>=m1->b.maxy) &&
	    t1!=m1->tstart && t1!=m1->tend ) {
        // If the intersection is not on the body of the second curve,
        // evaluate the beginning of the curve and the end of the curve and check for a match.
        // And then reset the t values corresponding to the intersection.
	/* rounding errors. Multiple t values may lead to the same inter position */
	/*  Things can get confused if we should be at the endpoints */
	float xs = ((m1->s->splines[0].a*m1->tstart+m1->s->splines[0].b)*m1->tstart+m1->s->splines[0].c)*m1->tstart+m1->s->splines[0].d;
	float ys = ((m1->s->splines[1].a*m1->tstart+m1->s->splines[1].b)*m1->tstart+m1->s->splines[1].c)*m1->tstart+m1->s->splines[1].d;
	if ( xs==inter->x && ys==inter->y )
	    t1 = m1->tstart;
	else {
	    float xe = ((m1->s->splines[0].a*m1->tend+m1->s->splines[0].b)*m1->tend+m1->s->splines[0].c)*m1->tend+m1->s->splines[0].d;
	    float ye = ((m1->s->splines[1].a*m1->tend+m1->s->splines[1].b)*m1->tend+m1->s->splines[1].c)*m1->tend+m1->s->splines[1].d;
	    if ( xe==inter->x && ye==inter->y )
		t1 = m1->tend;
	}
    }
    if (( inter->x<=m2->b.minx || inter->x>=m2->b.maxx ) &&
	    (inter->y<=m2->b.miny || inter->y>=m2->b.maxy) &&
	    t2!=m2->tstart && t2!=m2->tend ) {
        // If the intersection is not on the body of the second curve,
        // evaluate the beginning of the curve and the end of the curve and check for a match.
        // And then reset the t values corresponding to the intersection.
	float xs = ((m2->s->splines[0].a*m2->tstart+m2->s->splines[0].b)*m2->tstart+m2->s->splines[0].c)*m2->tstart+m2->s->splines[0].d;
	float ys = ((m2->s->splines[1].a*m2->tstart+m2->s->splines[1].b)*m2->tstart+m2->s->splines[1].c)*m2->tstart+m2->s->splines[1].d;
	if ( xs==inter->x && ys==inter->y )
	    t2 = m2->tstart;
	else {
	    float xe = ((m2->s->splines[0].a*m2->tend+m2->s->splines[0].b)*m2->tend+m2->s->splines[0].c)*m2->tend+m2->s->splines[0].d;
	    float ye = ((m2->s->splines[1].a*m2->tend+m2->s->splines[1].b)*m2->tend+m2->s->splines[1].c)*m2->tend+m2->s->splines[1].d;
	    if ( xe==inter->x && ye==inter->y )
		t2 = m2->tend;
	}
    }

    // We perform the adjacency check again.
    if (( m1->next==m2 && (t1==t2 || (t1==1.0 && t2==0.0))) ||
	( m2->next==m1 && (t2==t1 || (t2==1.0 && t1==0.0))) )
return( ilist );

    // We perform a very loose adjacency check.
    if (( m1->s->to == m2->s->from && RealWithin(t1,1.0,.01) && RealWithin(t2,0,.01)) ||
	    ( m1->s->from == m2->s->to && RealWithin(t1,0,.01) && RealWithin(t2,1.0,.01))) {
      SONotify("Discarding intersection at (%f, %f) due to proximity to a segment join.\n", m1->s->to->me.x, m1->s->to->me.y);
      return( ilist );
    }

    // If there was already a starting or ending intersection different from the provided intersection
    // and if the provided intersection was to be at the start or end of the monotonic
    // we restore the original t value (for each monotonic separately).
    if (( t1==m1->tstart && m1->start!=NULL &&
	    (inter->x!=m1->start->inter.x || inter->y!=m1->start->inter.y)) ||
	( t1==m1->tend && m1->end!=NULL &&
	    (inter->x!=m1->end->inter.x || inter->y!=m1->end->inter.y)))
	t1 = ot1;
    if (( t2==m2->tstart && m2->start!=NULL &&
	    (inter->x!=m2->start->inter.x || inter->y!=m2->start->inter.y)) ||
	( t2==m2->tend && m2->end!=NULL &&
	    (inter->x!=m2->end->inter.x || inter->y!=m2->end->inter.y)))
	t2 = ot2;

    // We perform a very loose adjacency check.
    /* The ordinary join of one spline to the next doesn't really count */
    /*  Or one monotonic sub-spline to the next either */
    if (( m1->next==m2 && RealNear(t1,m1->tend) && RealNear(t2,m2->tstart)) ||
	    (m2->next==m1 && RealNear(t1,m1->tstart) && RealNear(t2,m2->tend)) ) {
      SONotify("Discarding intersection at (%f, %f) due to proximity to a segment join.\n", inter->x, inter->y);
      return( ilist );
    }

    if ( RealWithin(m1->tstart,t1,.01) )
	il = m1->start;
    else if ( RealWithin(m1->tend,t1,.01) )
	il = m1->end;
    else
	il = NULL;
    if ( il!=NULL &&
	    ((RealWithin(m2->tstart,t2,.01) && m2->start==il) ||
	     (RealWithin(m2->tend,t2,.01) && m2->end==il)) )
return( ilist );
    for ( il = ilist; il!=NULL; il=il->next ) {
ValidateMListTs_IF_VERBOSE(il->monos)
    }
// ValidateMonotonic(m1);
// ValidateMonotonic(m2);
// If all else fails, we try to add an intersection.
return( _AddIntersection(ilist,m1,m2,t1,t2,inter));
}

static Intersection *SplitMonotonicsAt(Monotonic *m1,Monotonic *m2,
	int which,bigreal coord,Intersection *ilist) {
    struct inter_data id1, id2;
    memset(&id1, 0, sizeof(id1));
    memset(&id2, 0, sizeof(id2));
    Intersection *check;
    /* Intersections (even pseudo intersections) too close together are nasty things! */
    if ( Within64RoundingErrors(coord,((m1->s->splines[which].a*m1->tstart+m1->s->splines[which].b)*m1->tstart+m1->s->splines[which].c)*m1->tstart+m1->s->splines[which].d) ||
	 Within64RoundingErrors(coord,((m1->s->splines[which].a*m1->tend+m1->s->splines[which].b)*m1->tend+m1->s->splines[which].c)*m1->tend+m1->s->splines[which].d ) ||
	 Within64RoundingErrors(coord,((m2->s->splines[which].a*m2->tstart+m2->s->splines[which].b)*m2->tstart+m2->s->splines[which].c)*m2->tstart+m2->s->splines[which].d) ||
	 Within64RoundingErrors(coord,((m2->s->splines[which].a*m2->tend+m2->s->splines[which].b)*m2->tend+m2->s->splines[which].c)*m2->tend+m2->s->splines[which].d ) )
return( ilist );
    for ( Intersection * il = ilist; il!=NULL; il=il->next ) {
ValidateMListTs_IF_VERBOSE(il->monos)
    }
    SplitMonotonicAtFake(m1,which,coord,&id1);
    SplitMonotonicAtFake(m2,which,coord,&id2);
    for ( Intersection * il = ilist; il!=NULL; il=il->next ) {
ValidateMListTs_IF_VERBOSE(il->monos)
    }
    if ( !id1.new && !id2.new ) {
    for ( Intersection * il = ilist; il!=NULL; il=il->next ) {
ValidateMListTs_IF_VERBOSE(il->monos)
    }
return( ilist );
    }
    if ( !id1.new )
	id2.inter = id1.inter; // Use the senior intersection if possible.
    /* else if ( !id2.new ) */		/* We only use id2.inter */
	/* id1.inter = id2.inter;*/
    // ilist = check = _AddIntersection(ilist,id1.m,id1.otherm,id1.t,id1.othert,&id2.inter);
    // ilist = _AddIntersection(ilist,id2.m,id2.otherm,id2.t,id2.othert,&id2.inter);	/* Use id1.inter to avoid rounding errors */
    ilist = _AddIntersection(ilist,m1,m2,id1.t,id2.t,&id2.inter);
    // if ( check!=ilist )
	// IError("Added too many intersections.");
// ValidateMonotonic(m1);
// ValidateMonotonic(m2);
return( ilist );
}

static Intersection *AddCloseIntersection(Intersection *ilist,Monotonic *m1,
	Monotonic *m2,extended t1,extended t2,BasePoint *inter) {
    struct inter_data id1, id2;
    Intersection *check;

    if ( t1<m1->tstart+.01 && CloserT(m1->s,m1->tstart,t1,m2->s,t2) ) {
	if ( m1->start!=NULL )	/* Since we use the m2 inter value, life gets confused if we've already got a different intersection here */
return( ilist );
	t1 = m1->tstart;
    } else if ( t1>m1->tend-.01 && CloserT(m1->s,m1->tend,t1,m2->s,t2) ) {
	if ( m1->end!=NULL )
return( ilist );
	t1 = m1->tend;
    }
    if ( t2<m2->tstart+.01 && CloserT(m2->s,m2->tstart,t2,m1->s,t1) ) {
	if ( m2->start!=NULL )
return( ilist );
	t2 = m2->tstart;
    } else if ( t2>m2->tend-.01 && CloserT(m2->s,m2->tend,t2,m1->s,t1) ) {
	if ( m2->end!=NULL )
return( ilist );
	t2 = m2->tend;
    }
#if 0
    SplitMonotonicAtT(m1,-1,t1,0,&id1);
    SplitMonotonicAtT(m2,-1,t2,0,&id2);
    ilist = check = _AddIntersection(ilist,id1.m,id1.otherm,id1.t,id1.othert,&id2.inter);
    ilist = _AddIntersection(ilist,id2.m,id2.otherm,id2.t,id2.othert,&id2.inter);	/* Use id1.inter to avoid rounding errors */
    if ( check!=ilist )
	IError("Added too many intersections.");
#endif // 0
    ilist = _AddIntersection(ilist,m1,m2,t1,t2,inter);
return( ilist );
}

static void AddPreIntersection(Monotonic *m1, Monotonic *m2,
	extended t1,extended t2,BasePoint *inter, int isclose) {
    PreIntersection *p;

    /* This is just a join between two adjacent monotonics. There might already*/
    /*  be an intersection there, but if there be, we've already found it */
    /* Do this now, because no point wasting the time it takes to ImproveInter*/
    if (( m1->next==m2 && (t1==t2 || (t1==1.0 && t2==0.0))) ||
	( m2->next==m1 && (t2==t1 || (t2==1.0 && t1==0.0))) )
return;

    p = chunkalloc(sizeof(PreIntersection));
    p->next = m1->pending;
    m1->pending = p;
    p->m1 = m1;
    p->t1 = t1;
    p->m2 = m2;
    p->t2 = t2;
    p->inter = *inter;
    p->is_close = isclose;
}

static void FindMonotonicIntersection(Monotonic *m1,Monotonic *m2) {
    /* Note that two monotonic cubics can still intersect in multiple points */
    /*  so we can't just check if the splines are on opposite sides of each */
    /*  other at top and bottom */
    DBounds b;
    const bigreal error = .00001;
    BasePoint pt;
    extended t1,t2;
    int pick;
    int oncebefore=false;

    // ValidateMonotonic(m1); ValidateMonotonic(m2);
    // We bound the common area of the two splines since any intersection must be there.
    b.minx = m1->b.minx>m2->b.minx ? m1->b.minx : m2->b.minx;
    b.maxx = m1->b.maxx<m2->b.maxx ? m1->b.maxx : m2->b.maxx;
    b.miny = m1->b.miny>m2->b.miny ? m1->b.miny : m2->b.miny;
    b.maxy = m1->b.maxy<m2->b.maxy ? m1->b.maxy : m2->b.maxy;

    if ( b.maxy==b.miny && b.minx==b.maxx ) {
        // This essentially means that we know exactly where the intersection is.
	extended x1,y1, x2,y2, t1,t2;
	if ( m1->next==m2 || m2->next==m1 )
return;		/* Not interesting. Only intersection is at a shared endpoint */
	if ( ((m1->start==m2->start || m1->end==m2->start) && m2->start!=NULL) ||
		((m1->start==m2->end || m1->end==m2->end ) && m2->end!=NULL ))
return;
	pt.x = b.minx; pt.y = b.miny;
        // We want as much precision as possible, so we iterate on the longer dimension of each spline.
	if ( m1->b.maxx-m1->b.minx > m1->b.maxy-m1->b.miny )
	    t1 = IterateSplineSolveFixup(&m1->s->splines[0],m1->tstart,m1->tend,b.minx);
	else
	    t1 = IterateSplineSolveFixup(&m1->s->splines[1],m1->tstart,m1->tend,b.miny);
	if ( m2->b.maxx-m2->b.minx > m2->b.maxy-m2->b.miny )
	    t2 = IterateSplineSolveFixup(&m2->s->splines[0],m2->tstart,m2->tend,b.minx);
	else
	    t2 = IterateSplineSolveFixup(&m2->s->splines[1],m2->tstart,m2->tend,b.miny);
	if ( t1!=-1 && t2!=-1 ) {
	    ImproveInter(m1,m2,&t1,&t2,&pt);
	    x1 = ((m1->s->splines[0].a*t1+m1->s->splines[0].b)*t1+m1->s->splines[0].c)*t1+m1->s->splines[0].d;
	    y1 = ((m1->s->splines[1].a*t1+m1->s->splines[1].b)*t1+m1->s->splines[1].c)*t1+m1->s->splines[1].d;
	    x2 = ((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d;
	    y2 = ((m2->s->splines[1].a*t2+m2->s->splines[1].b)*t2+m2->s->splines[1].c)*t2+m2->s->splines[1].d;
	    if ( Within16RoundingErrors(x1,x2) && Within16RoundingErrors(y1,y2) )
		AddPreIntersection(m1,m2,t1,t2,&pt,false);
	}
    } else if ( b.maxy==b.miny ) {
        // We know the y-dimension of the intersection.
	extended x1,x2;
	if ( m1->next==m2 || m2->next==m1 )
return;		/* Not interesting. Only intersection is at a shared endpoint */
	if (( b.maxy==m1->b.maxy && m1->yup ) || ( b.maxy==m1->b.miny && !m1->yup ))
	    t1 = m1->tend; // If the spline ends at maxy (with yup confirming direction), set the t-value.
	else if (( b.maxy==m1->b.miny && m1->yup ) || ( b.maxy==m1->b.maxy && !m1->yup ))
	    t1 = m1->tstart; // If the spline starts at maxy (with yup confirming direction), set the t-value.
	else
	    t1 = IterateSplineSolveFixup(&m1->s->splines[1],m1->tstart,m1->tend,b.miny); // Find t for that y.
	if (( b.maxy==m2->b.maxy && m2->yup ) || ( b.maxy==m2->b.miny && !m2->yup ))
	    t2 = m2->tend; // If the spline ends at maxy (with yup confirming direction), set the t-value.
	else if (( b.maxy==m2->b.miny && m2->yup ) || ( b.maxy==m2->b.maxy && !m2->yup ))
	    t2 = m2->tstart; // If the spline starts at maxy (with yup confirming direction), set the t-value.
	else
	    t2 = IterateSplineSolveFixup(&m2->s->splines[1],m2->tstart,m2->tend,b.miny); // Find t for that y.
	if ( t1!=-1 && t2!=-1 ) {
	    x1 = ((m1->s->splines[0].a*t1+m1->s->splines[0].b)*t1+m1->s->splines[0].c)*t1+m1->s->splines[0].d;
	    x2 = ((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d;
	    if ( x1-x2>-.01 && x1-x2<.01 ) {
		pt.x = (x1+x2)/2; pt.y = b.miny;
		AddPreIntersection(m1,m2,t1,t2,&pt,false);
	    }
	}
    } else if ( b.maxx==b.minx ) {
	// We know the x-dimension of the intersection.
	extended y1,y2;
	if ( m1->next==m2 || m2->next==m1 )
return;		/* Not interesting. Only intersection is at an endpoint */
	if (( b.maxx==m1->b.maxx && m1->xup ) || ( b.maxx==m1->b.minx && !m1->xup ))
	    t1 = m1->tend;
	else if (( b.maxx==m1->b.minx && m1->xup ) || ( b.maxx==m1->b.maxx && !m1->xup ))
	    t1 = m1->tstart;
	else
	    t1 = IterateSplineSolveFixup(&m1->s->splines[0],m1->tstart,m1->tend,b.minx);
	if (( b.maxx==m2->b.maxx && m2->xup ) || ( b.maxx==m2->b.minx && !m2->xup ))
	    t2 = m2->tend;
	else if (( b.maxx==m2->b.minx && m2->xup ) || ( b.maxx==m2->b.maxx && !m2->xup ))
	    t2 = m2->tstart;
	else
	    t2 = IterateSplineSolveFixup(&m2->s->splines[0],m2->tstart,m2->tend,b.minx);
	if ( t1!=-1 && t2!=-1 ) {
	    y1 = ((m1->s->splines[1].a*t1+m1->s->splines[1].b)*t1+m1->s->splines[1].c)*t1+m1->s->splines[1].d;
	    y2 = ((m2->s->splines[1].a*t2+m2->s->splines[1].b)*t2+m2->s->splines[1].c)*t2+m2->s->splines[1].d;
	    if ( y1-y2>-.01 && y1-y2<.01 ) {
		pt.x = b.minx; pt.y = (y1+y2)/2;
		AddPreIntersection(m1,m2,t1,t2,&pt,false);
	    }
	}
    } else {
        // We know not the x-coordinate or the y-coordinate.
	for ( pick=0; pick<2; ++pick ) {
            // We work on the bigger dimension second.
	    int doy = (( b.maxy-b.miny > b.maxx-b.minx ) && pick ) ||
			    (( b.maxy-b.miny <= b.maxx-b.minx ) && !pick );
	    int any = false;
	    if ( doy ) {
                // We work on y.
		extended diff, y, x1,x2, x1o,x2o;
		extended t1,t2, t1o,t2o/*, t1t,t2t */;
		volatile extended bkp_y;

		diff = (b.maxy-b.miny)/32; // We slice the region into 32nds.
		y = b.miny;
		x1o = x2o = 0;
		while ( y<b.maxy ) {
		    t1o = IterateSplineSolveFixup(&m1->s->splines[1],m1->tstart,m1->tend,y);
		    if ( t1o==-1 ) // If there is no match, try slightly out-of-bounds.
			t1o = IterateSplineSolveFixup(&m1->s->splines[1],m1->tstart-m1->tstart/32,m1->tend+m1->tend/32,y);
		    t2o = IterateSplineSolveFixup(&m2->s->splines[1],m2->tstart,m2->tend,y);
		    if ( t2o==-1 ) // If there is no match, try slightly out-of-bounds.
			t2o = IterateSplineSolveFixup(&m2->s->splines[1],m2->tstart-m2->tstart/32,m2->tend+m2->tend/32,y);
		    if ( t1o!=-1 && t2o!=-1 )
		break; // If there is an in-bounds t-value for each curve that puts it at this y value, move to the next step.
		    y += diff;
		}
                // Evaluate the x values of the two splines at the shared y-point.
		x1o = ((m1->s->splines[0].a*t1o+m1->s->splines[0].b)*t1o+m1->s->splines[0].c)*t1o+m1->s->splines[0].d;
		x2o = ((m2->s->splines[0].a*t2o+m2->s->splines[0].b)*t2o+m2->s->splines[0].c)*t2o+m2->s->splines[0].d;
		if ( x1o==x2o ) {	/* Unlikely... but just in case */
		    pt.x = x1o; pt.y = y;
		    AddPreIntersection(m1,m2,t1o,t2o,&pt,false);
		    any = true;
		}
		oncebefore = false;
		for ( y+=diff; ; y += diff ) {
		    /* I used to say y<=b.maxy in the above for statement. */
		    /*  that seemed to get rounding errors on the mac, so we do it */
		    /*  like this now: */
		    if ( y>b.maxy ) {
			if ( oncebefore )
		break;
			if ( y<b.maxy+diff )
			    y = b.maxy;
			else
		break;
			oncebefore = true;
		    }

		    /* This is a volatile code! */
		    /* "diff" may become so small in comparison with "y", */
		    /* that "y+=diff" might actually not change the value of "y". */
		    // So we double diff until it is significant.
		    bkp_y=y+diff;
		    while (bkp_y==y) { diff *= 2; bkp_y = y+diff; }
		    /* Someone complained here that ff was depending on "exact" */
		    /*  arithmetic here. They failed to understand what was going */
		    /*  on, or even to read the comment above which should explain*/

		    // We want t-values that put our two splines at y.
		    t1 = IterateSplineSolveFixup(&m1->s->splines[1],m1->tstart,m1->tend,y);
		    if ( t1==-1 )
			t1 = IterateSplineSolveFixup(&m1->s->splines[1],m1->tstart-m1->tstart/32,m1->tend+m1->tend/32,y);
		    t2 = IterateSplineSolveFixup(&m2->s->splines[1],m2->tstart,m2->tend,y);
		    if ( t2==-1 )
			t2 = IterateSplineSolveFixup(&m2->s->splines[1],m2->tstart-m2->tstart/32,m2->tend+m2->tend/32,y);
		    if ( t1==-1 || t2==-1 )
		continue; // No luck at this y-value; let's try again.
		    // Evaluate x at the t-values for this y-value.
		    x1 = ((m1->s->splines[0].a*t1+m1->s->splines[0].b)*t1+m1->s->splines[0].c)*t1+m1->s->splines[0].d;
		    x2 = ((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d;
		    if ( x1==x2 && x1o!=x2o ) {
		        // If there is a match here and not at the previous y-value, we add a PreIntersection.
			pt.x = x1; pt.y = y;
			AddPreIntersection(m1,m2,t1,t2,&pt,false);
			any = true;
			x1o = x1; x2o = x2;
		    } else if ( x1o!=x2o && (x1o>x2o) != ( x1>x2 ) ) {
			/* A cross over has occured. (assume we have a small enough */
			/*  region that three cross-overs can't have occurred) */
			/* Use a binary search to track it down */
			extended ytop, ybot, ytest, oldy;
			extended oldx1 = x1, oldx2=x2;
			oldy = ytop = y;
			ybot = y-diff;
			if ( ybot<b.miny )
			    ybot = b.miny;
			x1o = x1; x2o = x2;
			while ( ytop!=ybot ) {
			    extended t1t, t2t;
			    ytest = (ytop+ybot)/2;
			    t1t = IterateSplineSolveFixup(&m1->s->splines[1],m1->tstart,m1->tend,ytest);
			    t2t = IterateSplineSolveFixup(&m2->s->splines[1],m2->tstart,m2->tend,ytest);
			    x1 = ((m1->s->splines[0].a*t1t+m1->s->splines[0].b)*t1t+m1->s->splines[0].c)*t1t+m1->s->splines[0].d;
			    x2 = ((m2->s->splines[0].a*t2t+m2->s->splines[0].b)*t2t+m2->s->splines[0].c)*t2t+m2->s->splines[0].d;
			    if ( t1t==-1 || t2t==-1 ) {
				if ( t1t==-1 && (RealNear(ytest,m1->b.miny) || RealNear(ytest,m1->b.maxy)))
				    /* OK */;
				else if ( t2t==-1 && (RealNear(ytest,m2->b.miny) || RealNear(ytest,m2->b.maxy)))
				    /* OK */;
				else
				    SOError( "Can't find something in range. y=%g\n", (double) ytest );
			break;
			    } else if (( x1-x2<error && x1-x2>-error ) || ytop==ytest || ybot==ytest ) {
				pt.y = ytest; pt.x = (x1+x2)/2;
				AddPreIntersection(m1,m2,t1t,t2t,&pt,false);
				any = true;
			break;
			    } else if ( (x1o>x2o) != ( x1>x2 ) ) {
				ybot = ytest;
			    } else {
				ytop = ytest;
			    }
			}
			y = oldy;		/* Might be more than one intersection, keep going */
			x1 = oldx1; x2 = oldx2;
		    }
		    x1o = x1; x2o = x2;
		    if ( y==b.maxy )
		break;
		}
	    } else {
                // We work on x.
		volatile extended bkp_x, x;
		extended diff, y1,y2, y1o,y2o;
		extended t1,t2, t1o,t2o/*, t1t,t2t*/ ;

		diff = (b.maxx-b.minx)/32; // We slice the region into 32nds.
		x = b.minx;
		y1o = y2o = 0;
		while ( x<b.maxx ) {
		    t1o = IterateSplineSolveFixup(&m1->s->splines[0],m1->tstart,m1->tend,x);
		    if ( t1o==-1 ) // If there is no match, try slightly out-of-bounds.
			t1o = IterateSplineSolveFixup(&m1->s->splines[0],m1->tstart-m1->tstart/32,m1->tend+m1->tend/32,x);
		    t2o = IterateSplineSolveFixup(&m2->s->splines[0],m2->tstart,m2->tend,x);
		    if ( t2o==-1 ) // If there is no match, try slightly out-of-bounds.
			t2o = IterateSplineSolveFixup(&m2->s->splines[0],m2->tstart-m2->tstart/32,m2->tend+m2->tend/32,x);
		    if ( t1o!=-1 && t2o!=-1 )
		break; // If there is an in-bounds t-value for each curve that puts it at this x value, move to the next step.
		    x += diff;
		}
                // Evaluate the x values of the two splines at the shared y-point.
		y1o = ((m1->s->splines[1].a*t1o+m1->s->splines[1].b)*t1o+m1->s->splines[1].c)*t1o+m1->s->splines[1].d;
		y2o = ((m2->s->splines[1].a*t2o+m2->s->splines[1].b)*t2o+m2->s->splines[1].c)*t2o+m2->s->splines[1].d;
		if ( y1o==y2o ) {
		    pt.y = y1o; pt.x = x;
		    AddPreIntersection(m1,m2,t1o,t2o,&pt,false);
		    any = true;
		}
		y1 = y2 = 0;
		oncebefore = false;
		for ( x+=diff; ; x += diff ) {
		    if ( x>b.maxx ) {
			if ( oncebefore )
		break;
			if ( x<b.maxx+diff )
			    x = b.maxx;
			else
		break;
			oncebefore= true;
		    }

		    /* This is a volatile code! */
		    /* "diff" may become so small in comparison with "y", */
		    /* that "y+=diff" might actually not change the value of "y". */
		    // So we double diff until it is significant.
		    bkp_x=x+diff;
		    while (bkp_x==x) { diff *= 2; bkp_x = x+diff; }

		    // We want t-values that put our two splines at x.
		    t1 = IterateSplineSolveFixup(&m1->s->splines[0],m1->tstart,m1->tend,x);
		    if ( t1==-1 )
			t1 = IterateSplineSolveFixup(&m1->s->splines[0],m1->tstart-m1->tstart/32,m1->tend+m1->tend/32,x);
		    t2 = IterateSplineSolveFixup(&m2->s->splines[0],m2->tstart,m2->tend,x);
		    if ( t2==-1 )
			t2 = IterateSplineSolveFixup(&m2->s->splines[0],m2->tstart-m2->tstart/32,m2->tend+m2->tend/32,x);
		    if ( t1==-1 || t2==-1 )
		continue; // No luck at this x-value; let's try again.
		    // Evaluate y at the t-values for this x-value.
		    y1 = ((m1->s->splines[1].a*t1+m1->s->splines[1].b)*t1+m1->s->splines[1].c)*t1+m1->s->splines[1].d;
		    y2 = ((m2->s->splines[1].a*t2+m2->s->splines[1].b)*t2+m2->s->splines[1].c)*t2+m2->s->splines[1].d;
		    if ( y1==y2 && y1o!=y2o ) {
		        // If there is a match here and not at the previous y-value, we add a PreIntersection.
			pt.y = y1; pt.x = x;
			AddPreIntersection(m1,m2,t1,t2,&pt,false);
			any = true;
			y1o = y1; y2o = y2;
		    } else if ( y1o!=y2o && (y1o>y2o) != ( y1>y2 ) ) {
			/* A cross over has occured. (assume we have a small enough */
			/*  region that three cross-overs can't have occurred) */
			/* Use a binary search to track it down */
			extended xtop, xbot, xtest, oldx;
			extended oldy1 = y1, oldy2=y2;
			oldx = xtop = x;
			xbot = x-diff;
			if ( xbot<b.minx ) xbot = b.minx;
			y1o = y1; y2o = y2;
			while ( xtop!=xbot ) {
			    extended t1t, t2t;
			    xtest = (xtop+xbot)/2;
			    t1t = IterateSplineSolveFixup(&m1->s->splines[0],m1->tstart,m1->tend,xtest);
			    t2t = IterateSplineSolveFixup(&m2->s->splines[0],m2->tstart,m2->tend,xtest);
			    y1 = ((m1->s->splines[1].a*t1t+m1->s->splines[1].b)*t1t+m1->s->splines[1].c)*t1t+m1->s->splines[1].d;
			    y2 = ((m2->s->splines[1].a*t2t+m2->s->splines[1].b)*t2t+m2->s->splines[1].c)*t2t+m2->s->splines[1].d;
			    if ( t1t==-1 || t2t==-1 ) {
				if ( t1t==-1 && (RealNear(xtest,m1->b.minx) || RealNear(xtest,m1->b.maxx)))
				    /* OK */;
				else if ( t2t==-1 && (RealNear(xtest,m2->b.minx) || RealNear(xtest,m2->b.maxx)))
				    /* OK */;
				else
				    SOError( "Can't find something in range. x=%g\n", (double) xtest );
			break;
			    } else if (( y1-y2<error && y1-y2>-error ) || xtop==xtest || xbot==xtest ) {
				pt.x = xtest; pt.y = (y1+y2)/2;
				AddPreIntersection(m1,m2,t1t,t2t,&pt,false);
				any = true;
			break;
			    } else if ( (y1o>y2o) != ( y1>y2 ) ) {
				xbot = xtest;
			    } else {
				xtop = xtest;
			    }
			}
			x = oldx;
			y1 = oldy1; y2 = oldy2;
		    }
		    y1o = y1; y2o = y2;
		    if ( x==b.maxx )
		break;
		}
	    }
	    if ( any )
	break;
	}
    }
    // ValidateMonotonic(m1); ValidateMonotonic(m2);
return;
}

static extended SplineContainsPoint(Monotonic *m,BasePoint *pt) {
    int which, nw;
    extended t;

    which = ( m->b.maxx-m->b.minx > m->b.maxy-m->b.miny )? 0 : 1;
    nw = !which;
    t = IterateSplineSolveFixup(&m->s->splines[which],m->tstart,m->tend,(&pt->x)[which]);
    if ( t!=-1 && Within16RoundingErrors((&pt->x)[nw],
	   ((m->s->splines[nw].a*t+m->s->splines[nw].b)*t +
		m->s->splines[nw].c)*t + m->s->splines[nw].d ))
return( t );

    which = nw;
    nw = !which;
    t = IterateSplineSolveFixup(&m->s->splines[which],m->tstart,m->tend,(&pt->x)[which]);
    if ( t!=-1 && Within16RoundingErrors((&pt->x)[nw],
	   ((m->s->splines[nw].a*t+m->s->splines[nw].b)*t +
		m->s->splines[nw].c)*t + m->s->splines[nw].d ))
return( t );

return( -1 );
}

/* If two splines are coincident, then pretend they intersect at both */
/*  end-points and nowhere else */
static int CoincidentIntersect(Monotonic *m1,Monotonic *m2,BasePoint *pts,
	extended *t1s,extended *t2s) {
    int cnt=0;
    extended t, t2, diff;

    if ( m1==m2 )
return( false );		/* Stupid question */
    /* Adjacent splines can double back on themselves */
    if ( m1->next==m2 || m1->prev==m2 ) {
	/* But normally they'll only intersect in one point, where they join */
	/* and that doesn't count */
	if ( (m1->b.minx>m2->b.minx ? m1->b.minx : m2->b.minx) == (m1->b.maxx<m2->b.maxx ? m1->b.maxx : m2->b.maxx) &&
		(m1->b.miny>m2->b.miny ? m1->b.miny : m2->b.miny) == (m1->b.maxy<m2->b.maxy ? m1->b.maxy : m2->b.maxy) )
return( false );
    }

    SetStartPoint(&pts[cnt],m1);
    t1s[cnt] = m1->tstart;
    if ( (t2s[cnt] = SplineContainsPoint(m2,&pts[cnt]))!=-1 )
	++cnt;

    SetEndPoint(&pts[cnt],m1);
    t1s[cnt] = m1->tend;
    if ( (t2s[cnt] = SplineContainsPoint(m2,&pts[cnt]))!=-1 )
	++cnt;

    if ( cnt!=2 ) {
	SetStartPoint(&pts[cnt],m2);
	t2s[cnt] = m2->tstart;
	if ( cnt==1 && pts[0].x==pts[1].x && pts[0].y==pts[1].y )
	    /* This happened once, when working with two splines with a common*/
	    /*  start point, and it lead to errors. So it's not a silly check*/;
	else if ( (t1s[cnt] = SplineContainsPoint(m1,&pts[cnt]))!=-1 )
	    ++cnt;
    }

    if ( cnt!=2 ) {
	SetEndPoint(&pts[cnt],m2);
	t2s[cnt] = m2->tend;
	if ( (t1s[cnt] = SplineContainsPoint(m1,&pts[cnt]))!=-1 )
	    ++cnt;
    }

    if ( cnt!=2 )
return( false );

    if (( Within16RoundingErrors(t1s[0],m1->tstart) && Within16RoundingErrors(t1s[1],m1->tend)) ||
	( Within16RoundingErrors(t2s[0],m2->tstart) && Within16RoundingErrors(t2s[1],m2->tend)) )
	/* It covers one of the monotonics entirely */;
    else if ( RealWithin(t1s[0],t1s[1],.01) )
return( false );	/* But otherwise, don't create a new tiny spline */

    /* Ok, if we've gotten this far we know that two of the end points are  */
    /*  on both splines.                                                    */
    t1s[2] = t2s[2] = -1;
    if ( !m1->s->knownlinear || !m2->s->knownlinear ) {
	if ( t1s[1]<t1s[0] ) {
	    extended temp = t1s[1]; t1s[1] = t1s[0]; t1s[0] = temp;
	    temp = t2s[1]; t2s[1] = t2s[0]; t2s[0] = temp;
	}
	diff = (t1s[1]-t1s[0])/16;
	for ( t=t1s[0]+diff; t<t1s[1]-diff/4; t += diff ) {
	    BasePoint here, slope;
	    here.x = ((m1->s->splines[0].a*t+m1->s->splines[0].b)*t+m1->s->splines[0].c)*t+m1->s->splines[0].d;
	    here.y = ((m1->s->splines[1].a*t+m1->s->splines[1].b)*t+m1->s->splines[1].c)*t+m1->s->splines[1].d;
	    if ( (slope.x = (3*m1->s->splines[0].a*t+2*m1->s->splines[0].b)*t+m1->s->splines[0].c)<0 )
		slope.x = -slope.x;
	    if ( (slope.y = (3*m1->s->splines[1].a*t+2*m1->s->splines[1].b)*t+m1->s->splines[1].c)<0 )
		slope.y = -slope.y;
	    if ( slope.y>slope.x ) {
		t2 = IterateSplineSolveFixup(&m2->s->splines[1],t2s[0],t2s[1],here.y);
		if ( t2==-1 || !RealWithin(here.x,((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d,.1))
return( false );
	    } else {
		t2 = IterateSplineSolveFixup(&m2->s->splines[0],t2s[0],t2s[1],here.x);
		if ( t2==-1 || !RealWithin(here.y,((m2->s->splines[1].a*t2+m2->s->splines[1].b)*t2+m2->s->splines[1].c)*t2+m2->s->splines[1].d,.1))
return( false );
	    }
	}
    }

return( true );
}

static void DumpMonotonic(Monotonic *input) {
  fprintf(stderr, "Monotonic: %p\n", input);
  fprintf(stderr, "  spline: %p; tstart: %f; tstop: %f; next: %p; prev: %p; start: %p; end: %p;\n", 
    input->s, input->tstart, input->tend, input->next, input->prev, input->start, input->end);
  fprintf(stderr, "  ");
  if (input->start != NULL) fprintf(stderr, "start: (%f, %f) ", input->start->inter.x, input->start->inter.y);
  if (input->end != NULL) fprintf(stderr, "end: (%f, %f) ", input->end->inter.x, input->end->inter.y);
  fprintf(stderr, "rstart: (%f, %f) ", evalSpline(input->s, input->tstart, 0), evalSpline(input->s, input->tstart, 1));
  fprintf(stderr, "rend: (%f, %f) ", evalSpline(input->s, input->tend, 0), evalSpline(input->s, input->tend, 1));
  fprintf(stderr, "\n");
}

#ifdef FF_OVERLAP_VERBOSE
#define FF_DUMP_MONOTONIC_IF_VERBOSE(m) DumpMonotonic(m);
#else
#define FF_DUMP_MONOTONIC_IF_VERBOSE(m) 
#endif

static Monotonic *FindMonoContaining(Monotonic *base, bigreal t) {
    Monotonic *m;

    for ( m=base; m->s == base->s; m=m->next ) {
	FF_DUMP_MONOTONIC_IF_VERBOSE(m)
	if ( t >= m->tstart && t <= m->tend )
return( m );
	if ( m->next == base ) /* don't search forever! */
	    break;
    }
#ifdef FF_RELATIONAL_GEOM
    for ( m=base; m->s == base->s; m=m->next ) {
	FF_DUMP_MONOTONIC_IF_VERBOSE(m)
	if ( t >= m->otstart && t <= m->otend )
return( m );
	if ( m->next == base ) /* don't search forever! */
	    break;
    }
#endif
    SOError("Failed to find monotonic containing %g\n", (double) t );
    for ( m=base; m->s == base->s; m=m->prev ) {
	if ( t >= m->tstart && t <= m->tend )
return( m );
	if ( m->prev == base ) /* don't search forever! */
	    break;
    }
#ifdef FF_RELATIONAL_GEOM
    for ( m=base; m->s == base->s; m=m->prev ) {
	FF_DUMP_MONOTONIC_IF_VERBOSE(m)
	if ( t >= m->otstart && t <= m->otend )
return( m );
	if ( m->prev == base ) /* don't search forever! */
	    break;
    }
#endif
    SOError("Failed to find monotonic containing %g twice\n", (double) t );
return( NULL );
}

static Intersection *TurnPreInter2Inter(Monotonic *ms) {
    PreIntersection *p, *pnext;
    Intersection *ilist = NULL;
    Monotonic *m1, *m2;

    for ( ; ms!=NULL; ms=ms->linked ) {
	for ( p = ms->pending; p!=NULL; p=pnext ) {
	    pnext = p->next;
	    m1 = FindMonoContaining(p->m1,p->t1);
	    if ( m1 == NULL ) {
		m1 = FindMonoContaining(p->m1,p->t1-1e-06);
		if ( m1 != NULL )
		    p->t1 = m1->tend;
	    }
	    if ( m1 == NULL ) {
		m1 = FindMonoContaining(p->m1,p->t1+1e-06);
		if ( m1 != NULL )
		    p->t1 = m1->tstart;
	    }
	    m2 = FindMonoContaining(p->m2,p->t2);
	    if ( m2 == NULL ) {
		m2 = FindMonoContaining(p->m2,p->t2-1e-06);
		if ( m2 != NULL )
		    p->t2 = m2->tend;
	    }
	    if ( m2 == NULL ) {
		m2 = FindMonoContaining(p->m2,p->t2+1e-06);
		if ( m2 != NULL )
		    p->t2 = m2->tstart;
	    }
	    if ( p->is_close )
		ilist = AddCloseIntersection(ilist,m1,m2,p->t1,p->t2,&p->inter);
	    else
		ilist = AddIntersection(ilist,m1,m2,p->t1,p->t2,&p->inter);
	    chunkfree(p,sizeof(PreIntersection));
	}
	ms->pending = NULL;
    }
return( ilist );
}

static void FigureProperMonotonicsAtIntersections(Intersection *ilist) {
    MList *ml, *ml2, *mlnext, *prev, *p2;

    while ( ilist!=NULL ) {
	// We examine each intersection.
	for ( ml=ilist->monos; ml!=NULL; ml=ml->next ) {
	    // We examine each monotonic connected to this intersection.
	    if ( (ml->t==ml->m->tstart && !ml->isend) ||
		    (ml->t==ml->m->tend && ml->isend)) {
		// If the recorded t-value of the intersection-spline record corresponds
		// to the starting or ending t-value of the spline (depending upon isend)
		// it's right.
	    } else if ( ml->t>ml->m->tstart ) {
		// If the intersection occurs at a t-value past the start of the current monotonic,
		// keep crawling forwards on the spline (across monotonics) until we find
		// a monotonic containing the desired t-value (success) or until
		// the spline pointers differ between the intersection-spline record
		// and the monotonic record (error).
		while ( ml->t>ml->m->tend ) {
#ifdef FF_RELATIONAL_GEOM
		    // If we have relational geometry and if we have gone off the end of the segment or found a gap,
		    // we use the virtual (pre-adjustment) t-values.
		    if ((ml->m->prev != NULL) && (ml->m->next->s == ml->s) && (ml->m->tend != ml->m->next->tstart))
		      SOError("Segment gap: %f != %f; (%f == %f).\n", ml->m->tend , ml->m->next->tstart, ml->m->otend, ml->m->next->otstart);
		    if ((ml->m->next == NULL) || (ml->m->next->s !=ml->s) || (ml->t < ml->m->prev->tstart))
		      if (ml->t<=ml->m->otend) { ml->t = ml->m->tend; break; }
#endif
		    ml->m = ml->m->next;
		    if ( ml->m->s!=ml->s ) {
		      // We've gone off the spline on which the t-value is valid.
			SOError("we could not find a matching monotonic\n" );
		break;
		    }
		}
	    } else {
		// If the intersection occurs at a t-value prior to the start of the current monotonic,
		// keep crawling back on the spline (across monotonics) until we find
		// a monotonic containing the desired t-value (success) or until
		// the spline pointers differ between the intersection-spline record
		// and the monotonic record (error).
		while ( ml->t<ml->m->tstart ) {
#ifdef FF_RELATIONAL_GEOM
		    // If we have relational geometry and if we have gone off the end of the segment or found a gap,
		    // we use the virtual (pre-adjustment) t-values.
		    if ((ml->m->prev != NULL) && (ml->m->prev->s == ml->s) && (ml->m->tstart != ml->m->prev->tend))
		      SOError("Segment gap: %f != %f; (%f == %f).\n", ml->m->tstart , ml->m->prev->tend, ml->m->otstart, ml->m->prev->otend);
		    if ((ml->m->prev == NULL) || (ml->m->prev->s != ml->s) || (ml->t > ml->m->prev->tend))
		      if (ml->t>=ml->m->otstart) { ml->t = ml->m->tstart; break; }
#endif
		    ml->m = ml->m->prev;
		    if ( ml->m->s!=ml->s ) {
		      // We've gone off the spline on which the t-value is valid.
			SOError( "we could not find a matching monotonic\n" );
		break;
		    }
		}
	    }
	    if ( ml->t<=ml->m->tstart && ml->isend && ml->m->prev && ml->t<=ml->m->prev->tend) {
	        SONotify("Step back.\n");
		ml->m = ml->m->prev; // Step back one if we want an end but are at a start.
		if (ml->m->s!=ml->s) SOError("We've gone off-spline!\n");
		else ml->t = ml->m->tend;
	    } else if ( ml->t>=ml->m->tend && !ml->isend && ml->m->next && ml->t>=ml->m->next->tstart) {
	        SONotify("Step ahead.\n");
		ml->m = ml->m->next; // Step ahead one if we want a start but are at an end.
		if (ml->m->s!=ml->s) SOError("We've gone off-spline!\n");
		else ml->t = ml->m->tstart;
	    }
	    if ( ml->t!=ml->m->tstart && ml->t!=ml->m->tend ) {
		SOError( "we could not find a matching monotonic time\n" );
		SOError("  ml->t (%f) equals neither ml->m->tstart (%f) nor ml->m->tend (%f).\n", ml->t, ml->m->tstart, ml->m->tend);
#ifdef FF_RELATIONAL_GEOM
		SOError("  What about ml->m->otstart (%f) nor ml->m->otend (%f).\n", ml->m->otstart, ml->m->otend);
#endif
	    }
	}
	for ( prev=NULL, ml=ilist->monos; ml!=NULL; ml = mlnext ) {
	    mlnext = ml->next;
	    if ( ml->m->start==ml->m->end ) {
		for ( p2 = ml, ml2=ml->next; ml2!=NULL; p2=ml2, ml2 = ml2->next ) {
		    if ( ml2->m==ml->m )
		break;
		}
		if ( ml2!=NULL ) {
		    if ( ml2==mlnext ) mlnext = ml2->next;
		    p2->next = ml2->next;
		    chunkfree(ml2,sizeof(*ml2));
		}
		if ( prev==NULL )
		    ilist->monos = mlnext;
		else
		    prev->next = mlnext;
		chunkfree(ml,sizeof(*ml));
	    }
	}
	ilist = ilist->next;
    }
}

static Intersection *FindIntersections(Monotonic *ms, enum overlap_type ot) {
    Monotonic *m1, *m2;
    BasePoint pts[9];
    extended t1s[10], t2s[10];
    Intersection *ilist=NULL;
    int i;
    // For each monotonic, check against each other monotonic for an intersection.
    for ( m1=ms; m1!=NULL; m1=m1->linked ) {
	for ( m2=m1->linked; m2!=NULL; m2=m2->linked ) {
	    if ( m2->b.minx > m1->b.maxx ||
		    m2->b.maxx < m1->b.minx ||
		    m2->b.miny > m1->b.maxy ||
		    m2->b.maxy < m1->b.miny )
	continue;		/* Can't intersect since they don't have overlapping bounding boxes */
	    // ValidateMonotonic(m1); ValidateMonotonic(m2);
	    if ( CoincidentIntersect(m1,m2,pts,t1s,t2s) ) {
		// If the splines are nearly coincident , we add up to 4 preintersections with the close flag.
		for ( i=0; i<4 && t1s[i]!=-1; ++i ) {
		    if ( t1s[i]>=m1->tstart && t1s[i]<=m1->tend &&
			    t2s[i]>=m2->tstart && t2s[i]<=m2->tend ) {
			AddPreIntersection(m1,m2,t1s[i],t2s[i],&pts[i],true);
		    }
		}
	    } else if ( m1->s->knownlinear || m2->s->knownlinear ) {
		// We add the intersections for non-coincident splines.
		// Why we limit to 4 intersections is beyond me.
		if ( SplinesIntersect(m1->s,m2->s,pts,t1s,t2s)>0 )
		    for ( i=0; i<4 && t1s[i]!=-1; ++i ) {
			if ( t1s[i]>=m1->tstart && t1s[i]<=m1->tend &&
				t2s[i]>=m2->tstart && t2s[i]<=m2->tend ) {
			    AddPreIntersection(m1,m2,t1s[i],t2s[i],&pts[i],false);
			}
		    }
	    } else {
		FindMonotonicIntersection(m1,m2);
	    }
	}
    }

    ilist = TurnPreInter2Inter(ms);
    FigureProperMonotonicsAtIntersections(ilist);
    // Remove invalid segments.
    CleanMonotonics(&ms);

    /* Now suppose we have a contour which intersects nothing? */
    /* with no intersections we lose track of it and it will vanish */
    /* That's not a good idea. Make sure each contour has at least one inter */
    if ( ot!=over_findinter && ot!=over_fisel ) {
	for ( m1=ms; m1!=NULL; m1=m2->linked ) {
	    if ( m1->start==NULL && m1->end==NULL ) {
		Intersection *il;
		il = chunkalloc(sizeof(Intersection));
		il->inter = m1->s->from->me;
		il->next = ilist;
		AddSpline(il,m1,0);
		AddSpline(il,m1->prev,1.0);
		ilist = il;
	    }
	    /* skip to next contour */
	    for ( m2=m1; m2->linked==m2->next; m2=m2->linked );
	}
    }

return( ilist );
}

static int dcmp(const void *_p1, const void *_p2) {
    const extended *dpt1 = _p1, *dpt2 = _p2;
    if ( *dpt1>*dpt2 )
return( 1 );
    else if ( *dpt1<*dpt2 )
return( -1 );

return( 0 );
}

static extended *FindOrderedEndpoints(Monotonic *ms,int which) {
    int cnt;
    Monotonic *m;
    extended *ends;
    int i,j,k;

    for ( m=ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
    ends = malloc((2*cnt+1)*sizeof(extended));
    for ( m=ms, cnt=0; m!=NULL; m=m->linked, cnt+=2 ) {
	if ( m->start!=NULL )
	    ends[cnt] = (&m->start->inter.x)[which];
	else if ( m->tstart==0 )
	    ends[cnt] = (&m->s->from->me.x)[which];
	else
	    ends[cnt] = ((m->s->splines[which].a*m->tstart+m->s->splines[which].b)*m->tstart+
		    m->s->splines[which].c)*m->tstart+m->s->splines[which].d;
	if ( m->end!=NULL )
	    ends[cnt+1] = (&m->end->inter.x)[which];
	else if ( m->tend==1.0 )
	    ends[cnt+1] = (&m->s->to->me.x)[which];
	else
	    ends[cnt+1] = ((m->s->splines[which].a*m->tend+m->s->splines[which].b)*m->tend+
		    m->s->splines[which].c)*m->tend+m->s->splines[which].d;
    }

    qsort(ends,cnt,sizeof(extended),dcmp);
    for ( i=0; i<cnt; ++i ) {
	for ( j=i; j<cnt && ends[i]==ends[j]; ++j );
	if ( j>i+1 ) {
	    for ( k=i+1; j<cnt; ends[k++] = ends[j++]);
	    cnt-= j-k;
	}
    }
    ends[cnt] = 1e10;
return( ends );
}

static int mcmp(const void *_p1, const void *_p2) {
    const Monotonic * const *mpt1 = _p1, * const *mpt2 = _p2;
    if ( (*mpt1)->other>(*mpt2)->other )
return( 1 );
    else if ( (*mpt1)->other<(*mpt2)->other )
return( -1 );

return( 0 );
}

int CheckMonotonicClosed(struct monotonic *ms) {
  struct monotonic * current;
  if (ms == NULL) return 0;
  current = ms->next;
  while (current != ms && current != NULL) {
    current = current->next;    
  }
  if (current == NULL) return 0;
  return 1;
}

int MonotonicFindAt(Monotonic *ms,int which, extended test, Monotonic **space ) {
    /* Find all monotonic sections which intersect the line (x,y)[which] == test */
    /*  find the value of the other coord on that line */
    /*  Order them (by the other coord) */
    /*  then run along that line figuring out which monotonics are needed */
    extended t;
    Monotonic *m, *mm;
    int i, j, k, cnt;
    int nw = !which;

    for ( m=ms, i=0; m!=NULL; m=m->linked ) {
        if (CheckMonotonicClosed(m) == 0) continue; // Open monotonics break things.
	if (( which==0 && test >= m->b.minx && test <= m->b.maxx ) ||
		( which==1 && test >= m->b.miny && test <= m->b.maxy )) {
	    /* Lines parallel to the direction we are testing just get in the */
	    /*  way and don't add any useful info */
	    if ( m->s->knownlinear &&
		    (( which==1 && m->s->from->me.y==m->s->to->me.y ) ||
			(which==0 && m->s->from->me.x==m->s->to->me.x)))
    continue;
	    t = IterateSplineSolveFixup(&m->s->splines[which],m->tstart,m->tend,test);
	    if ( t==-1 ) {
		if ( which==0 ) {
		    if (( test-m->b.minx > m->b.maxx-test && m->xup ) ||
			    ( test-m->b.minx < m->b.maxx-test && !m->xup ))
			t = m->tstart;
		    else
			t = m->tend;
		} else {
		    if (( test-m->b.miny > m->b.maxy-test && m->yup ) ||
			    ( test-m->b.miny < m->b.maxy-test && !m->yup ))
			t = m->tstart;
		    else
			t = m->tend;
		}
	    }
	    m->t = t;
	    if ( t==m->tend ) t -= (m->tend-m->tstart)/100;
	    else if ( t==m->tstart ) t += (m->tend-m->tstart)/100;
	    m->other = ((m->s->splines[nw].a*t+m->s->splines[nw].b)*t+
		    m->s->splines[nw].c)*t+m->s->splines[nw].d;
	    space[i++] = m;
	}
    }
    cnt = i;

    /* Things get a little tricky at end-points */
    for ( i=0; i<cnt; ++i ) {
	m = space[i];
	if ( m->t==m->tend ) {
	    /* Ignore horizontal/vertical lines (as appropriate) */
	    for ( mm=m->next; mm!=m && mm !=NULL; mm=mm->next ) {
		if ( !mm->s->knownlinear )
	    break;
		if (( which==1 && mm->s->from->me.y!=m->s->to->me.y ) ||
			(which==0 && mm->s->from->me.x!=m->s->to->me.x))
	    break;
	    }
	} else if ( m->t==m->tstart ) {
	    for ( mm=m->prev; mm!=m && mm !=NULL; mm=mm->prev ) {
		if ( !mm->s->knownlinear )
	    break;
		if (( which==1 && mm->s->from->me.y!=m->s->to->me.y ) ||
			(which==0 && mm->s->from->me.x!=m->s->to->me.x))
	    break;
	    }
	} else
    break;
	/* If the next monotonic continues in the same direction, and we found*/
	/*  it too, then don't count both. They represent the same intersect */
	/* If they are in oposite directions then they cancel each other out */
	/*  and that is correct */
	if ( mm!=m &&	/* Should always be true */
		(&mm->xup)[which]==(&m->xup)[which] ) {
	    for ( j=cnt-1; j>=0; --j )
		if ( space[j]==mm )
	    break;
	    if ( j!=-1 ) {
		/* remove mm */
		for ( k=j+1; k<cnt; ++k )
		    space[k-1] = space[k];
		--cnt;
		if ( i>j ) --i;
	    }
	}
    }

    space[cnt] = NULL; space[cnt+1] = NULL;
    qsort(space,cnt,sizeof(Monotonic *),mcmp);
return(cnt);
}

static Intersection *TryHarderWhenClose(int which, bigreal tried_value, Monotonic **space,int cnt,
	Intersection *ilist) {
    /* If splines are very close together at a certain point then we can't */
    /*  tell the proper ordering due to rounding errors. */
    int i, j;
    bigreal low, high, test, t1, t2, c1, c2, incr;
    int neg_cnt, pos_cnt, pc, nc;
    int other = !which;

    for ( i=cnt-2; i>=0; --i ) {
	Monotonic *m1 = space[i], *m2 = space[i+1];
	if ( Within4RoundingErrors( m1->other,m2->other )) {
	    /* Now, we know that these two monotonics do not intersect */
	    /*  (except possibly at the end points, because we found all */
	    /*  intersections earlier) so we can compare them anywhere */
	    /*  along their mutual span, and any ordering (not encumbered */
	    /*  by rounding errors) should be valid */
	    if ( which==0 ) {
		low = m1->b.minx>m2->b.minx ? m1->b.minx : m2->b.minx;
		high = m1->b.maxx<m2->b.maxx ? m1->b.maxx : m2->b.maxx;
	    } else {
		low = m1->b.miny>m2->b.miny ? m1->b.miny : m2->b.miny;
		high = m1->b.maxy<m2->b.maxy ? m1->b.maxy : m2->b.maxy;
	    }
	    if ( low==high )
    continue;			/* One ends, the other begins. No overlap really */
	    if ( RealNear(low,high))
    continue;			/* Too close together to be very useful */
#define DECIMATE	32
	    incr = (high-low)/DECIMATE;
	    neg_cnt = pos_cnt=0;
	    pc = nc = 0;
	    for ( j=0, test=low+incr; j<=DECIMATE; ++j, test += incr ) {
		if ( test>high ) test=high;
#undef DECIMATE
		t1 = IterateSplineSolveFixup(&m1->s->splines[which],m1->tstart,m1->tend,test);
		t2 = IterateSplineSolveFixup(&m2->s->splines[which],m2->tstart,m2->tend,test);
		if ( t1==-1 || t2==-1 )
	    continue;
		c1 = ((m1->s->splines[other].a*t1+m1->s->splines[other].b)*t1+m1->s->splines[other].c)*t1+m1->s->splines[other].d;
		c2 = ((m2->s->splines[other].a*t2+m2->s->splines[other].b)*t2+m2->s->splines[other].c)*t2+m2->s->splines[other].d;
		if ( !Within16RoundingErrors(c1,c2)) {
		    if ( c1>c2 ) { pos_cnt=1; neg_cnt=0; }
		    else { pos_cnt=0; neg_cnt=1; }
	    break;
		} else if ( !Within4RoundingErrors(c1,c2) ) {
		    if ( c1>c2 )
			++pos_cnt;
		    else
			++neg_cnt;
		} else {
		    /* Here the diff might be 0, which doesn't count as either +/- */
		    /*  earlier diff was bigger than error so that couldn't happen */
		    if ( c1>c2 )
			++pc;
		    else if ( c1!=c2 )
			++nc;
		}
	    }
	    if ( pos_cnt>neg_cnt ) {
		/* Out of order */
		space[i+1] = m1;
		space[i] = m2;
	    } else if ( pos_cnt==0 && neg_cnt==0 ) {
		/* the two monotonics are never far from one another over */
		/*  this range. So let's add intersections at the end of */
		/*  the range so we don't get confused */
		if ( ilist!=NULL ) {
		    real rh = (real) high;
		    if ( (which==0 && (m1->b.minx!=m2->b.minx || m1->b.maxx!=m2->b.maxx)) ||
			    (which==1 && (m1->b.miny!=m2->b.miny || m1->b.maxy!=m2->b.maxy)) ) {
			ilist = SplitMonotonicsAt(m1,m2,which,low,ilist);
			if ( (which==0 && rh>m1->b.maxx && rh<=m1->next->b.maxx) ||
				(which==1 && rh>m1->b.maxy && rh<=m1->next->b.maxy))
			    m1 = m1->next;
			if ( (which==0 && rh>m2->b.maxx && rh<=m2->next->b.maxx) ||
				(which==1 && rh>m2->b.maxy && rh<=m2->next->b.maxy))
			    m2 = m2->next;
			ilist = SplitMonotonicsAt(m1,m2,which,high,ilist);
		    }
		    if ( (&m1->xup)[which]!=(&m2->xup)[which] ) {
			/* the two monotonics cancel each other out */
			/* (they are close together, and go in opposite directions) */
			m1->mutual_collapse = m1->isunneeded = true;
			m2->mutual_collapse = m2->isunneeded = true;
		    }
		}
		if ( pc>nc ) {
		    space[i+1] = m1;
		    space[i] = m2;
		}
	    }
	}
    }
return( ilist );
}

static void MonosMarkConnected(Monotonic *startm,int needed,bigreal test,int which) {
    Monotonic *m;
    /* If a monotonic is needed, then all monotonics connected to it should */
    /*  also be needed -- until we hit an intersection */

    for ( m=startm; ; ) {
	if ( m->isneeded || m->isunneeded ) {
	    if ( m->isneeded!=needed )
		SOError( "monotonic is both needed and unneeded (%g,%g)->(%g,%g). %s=%g (prev=%g)\n",
		    (double) (((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d),
		    (double) (((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d),
		    which ? "y" : "x", (double) test, (double) m->when_set );
	} else {
	    m->isneeded = needed;
	    m->isunneeded = !needed;
	    m->when_set = test;
	}
	if ( m->end!=NULL )
    break;
	m=m->next;
	if ( m==startm )
    break;
    }

    for ( m=startm; ; ) {
	if ( m->start!=NULL )
    break;
	m=m->prev;
	if ( m==startm )
    break;
	if ( m->isneeded || m->isunneeded ) {
	    if ( m->isneeded!=needed )
		SOError( "monotonic is both needed and unneeded (%g,%g)->(%g,%g). %s=%g (prev=%g)\n",
		    (double) (((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d),
		    (double) (((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d),
		    which ? "y" : "x", (double) test, (double) m->when_set );
	} else {
	    m->isneeded = needed;
	    m->isunneeded = !needed;
	    m->when_set = test;
	}
    }
}

static int IsNeeded(enum overlap_type ot,int winding, int nwinding, int ew, int new) {
    if ( ot==over_remove || ot==over_rmselected ) {
return( winding==0 || nwinding==0 );
    } else if ( ot==over_intersect || ot==over_intersel ) {
return( !( (winding>-2 && winding<2 && nwinding>-2 && nwinding<2) ||
		    ((winding<=-2 || winding>=2) && (nwinding<=-2 || nwinding>=2))));
    } else if ( ot == over_exclude ) {
return( !( (( winding==0 || nwinding==0 ) && ew==0 && new==0 ) ||
		    (winding!=0 && (( ew!=0 && new==0 ) || ( ew==0 && new!=0))) ));
    }
return( false );
}

static void FigureNeeds(Monotonic *ms,int which, extended test, Monotonic **space,
	enum overlap_type ot, bigreal close_level) {
    /* Find all monotonic sections which intersect the line (x,y)[which] == test */
    /*  find the value of the other coord on that line */
    /*  Order them (by the other coord) */
    /*  then run along that line figuring out which monotonics are needed */
    int i, winding, ew, close, n;

    TryHarderWhenClose(which,test,space,MonotonicFindAt(ms,which,test,space),NULL);

    winding = 0; ew = 0;
    for ( i=0; space[i]!=NULL; ++i ) {
	int needed;
	Monotonic *m, *nm;
	int new;
	int nwinding, nnwinding, nneeded, nnew, niwinding, niew, nineeded, inneeded, inwinding, inew;
      /* retry: */
	needed = false;
	nwinding=winding;
	new=ew;
	m = space[i];
	if ( m->mutual_collapse )
    continue;
	n=0;
	do {
	    ++n;
	    nm = space[i+n];
	} while ( nm!=NULL && nm->mutual_collapse );
	if ( m->exclude )
	    new += ( (&m->xup)[which] ? 1 : -1 );
	else
	    nwinding += ( (&m->xup)[which] ? 1 : -1 );
	/* We do some look ahead and figure out the neededness of the next */
	/*  monotonic on the list. This is because we may need to reorder them*/
	/*  (if the two are close together we might get rounding errors). */
	/*  So not only do we figure out the neededness of both this and the */
	/*  next mono using the current ordering, but we also do it as things*/
	/*  would appear after reversing the two. So... */
	/* needed -- means the current mono is needed with the current order */
	/* nneeded -- next mono is needed with the current order */
	/* nineeded -- next mono is needed with reveresed order */
	/* inneeded -- cur mono is needed with reversed order */
	niwinding = winding; niew = ew;
	nnwinding = nwinding; nnew = new;
	if ( nm!=NULL ) {
	    if ( nm->exclude ) {
		nnew += ( (&nm->xup)[which] ? 1 : -1 );
		niew += ( (&nm->xup)[which] ? 1 : -1 );
	    } else {
		nnwinding += ( (&nm->xup)[which] ? 1 : -1 );
		niwinding += ( (&nm->xup)[which] ? 1 : -1 );
	    }
	}
	inwinding = niwinding; inew = niew;
	if ( m->exclude )
	    inew += ( (&m->xup)[which] ? 1 : -1 );
	else
	    inwinding += ( (&m->xup)[which] ? 1 : -1 );
	needed = IsNeeded(ot,winding,nwinding,ew,new);
	nneeded = IsNeeded(ot,nwinding,nnwinding,new,nnew);
	nineeded = IsNeeded(ot,winding,niwinding,ew,niew);
	inneeded = IsNeeded(ot,niwinding,inwinding,niew,inew);
	if ( nm!=NULL )
	    close = nm->other-m->other < close_level &&
		    nm->other-m->other > -close_level;
	else
	    close = false;
	if ( i>0 && m->other-space[i-1]->other < close_level &&
		m->other-space[i-1]->other > -close_level )	/* In case we reversed things */
	    close = true;
	/* On our first pass through the list, don't set needed/unneeded */
	/*  when two monotonics are close together. (We get rounding errors */
	/*  when things are too close and get confused about the order */
	if ( !close ) {
	    if ( nm!=NULL && nm->other-m->other < .01 ) {
		if ((( m->isneeded || m->isunneeded ) && m->isneeded!=needed &&
			(nm->isneeded==nineeded ||
			 (!nm->isneeded && !nm->isunneeded)) ) ||
		      ( (nm->isneeded || nm->isunneeded) && nm->isneeded!=nneeded &&
			(m->isneeded == inneeded ||
			 (!m->isneeded && !m->isunneeded)) )) {
		    space[i] = nm;
		    space[i+1] = m;
		    needed = nineeded;
		    m = nm;
		}
	    }
	    if ( !m->isneeded && !m->isunneeded ) {
		MonosMarkConnected(m,needed,test,which);
		/* m->isneeded = needed; m->isunneeded = !needed; */
		/* m->when_set = test;		*//* Debugging */
	    } else if ( m->isneeded!=needed || m->isunneeded!=!needed ) {
		SOError( "monotonic is both needed and unneeded (%g,%g)->(%g,%g). %s=%g (prev=%g)\n",
		    (double) (((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d),
		    (double) (((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d),
		    which ? "y" : "x", (double) test, (double) m->when_set );
	    }
	}
	winding = nwinding;
	ew = new;
    }
    if ( winding!=0 )
	SOError( "Winding number did not return to 0 when %s=%g\n",
		which ? "y" : "x", (double) test );
}

struct gaps { extended test, len; int which; };

static int gcmp(const void *_p1, const void *_p2) {
    const struct gaps *gpt1 = _p1, *gpt2 = _p2;
    if ( gpt1->len > gpt2->len )
return( 1 );
    else if ( gpt1->len < gpt2->len )
return( -1 );

return( 0 );
}

static Intersection *FindNeeded(Monotonic *ms,enum overlap_type ot,Intersection *ilist) {
    extended *ends[2];
    Monotonic *m, **space;
    extended top, bottom, test, last, gap_len;
    int i,j,k,l, cnt,which;
    struct gaps *gaps;
    extended min_gap;
    static const bigreal closeness_level[] = { .1, .01, 0, -1 };

    if ( ms==NULL )
return(ilist);

    for ( m=ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
    space = malloc(4*(cnt+2)*sizeof(Monotonic*));	/* We need at most cnt, but we will be adding more monotonics... */

    /* Check (again) for coincident spline segments */
    for ( m=ms; m!=NULL; m=m->linked ) {
	if ( m->b.maxx-m->b.minx > m->b.maxy-m->b.miny ) {
	    top = m->b.maxx;
	    bottom = m->b.minx;
	    which = 0;
	} else {
	    top = m->b.maxy;
	    bottom = m->b.miny;
	    which = 1;
	}
	test=(top+bottom)/2;
	ilist = TryHarderWhenClose(which,test,space,MonotonicFindAt(ms,which,test,space),ilist);
    }

    ends[0] = FindOrderedEndpoints(ms,0);
    ends[1] = FindOrderedEndpoints(ms,1);

    for ( m=ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
    gaps = malloc(2*cnt*sizeof(struct gaps));

    /* Look for the longest splines without interruptions first. These are */
    /* least likely to cause problems and will give us a good basis from which*/
    /* to make guesses should rounding errors occur later */
    for ( j=k=0; j<2; ++j )
	for ( i=0; ends[j][i+1]!=1e10; ++i ) {
	    gaps[k].which = j;
	    gaps[k].len = (ends[j][i+1]-ends[j][i]);
	    gaps[k++].test = (ends[j][i+1]+ends[j][i])/2;
	}
    qsort(gaps,k,sizeof(struct gaps),gcmp);
    min_gap = 1e10;
    for ( m=ms; m!=NULL; m=m->linked ) {
	if ( m->b.maxx-m->b.minx > m->b.maxy-m->b.miny ) {
	    if ( min_gap > m->b.maxx-m->b.minx ) min_gap = m->b.maxx-m->b.minx;
	} else {
	    if ( m->b.maxy-m->b.miny==0 )
		SOError("Zero y clearance.\n");
	    if ( min_gap > m->b.maxy-m->b.miny ) min_gap = m->b.maxy-m->b.miny;
	}
    }
    if ( min_gap<.5 ) min_gap = .5;
    for ( i=0; i<k && gaps[i].len>=min_gap; ++i )
	FigureNeeds(ms,gaps[i].which,gaps[i].test,space,ot,1.0);

    for ( l=0; closeness_level[l]>=0; ++l ) {
	for ( m=ms; m!=NULL; m=m->linked ) if ( !m->isneeded && !m->isunneeded ) {
	    if ( m->b.maxx-m->b.minx > m->b.maxy-m->b.miny ) {
		top = m->b.maxx;
		bottom = m->b.minx;
		which = 0;
	    } else {
		top = m->b.maxy;
		bottom = m->b.miny;
		which = 1;
	    }
	    /* If we try a test which is at one of the endpoints of any monotonic */
	    /*  then bad things happen (because two monotonics will appear to be  */
	    /*  active when only one is. This can be corrected for, but it can    */
	    /*  become very complex and it is easiest just to insure that we never*/
	    /*  test at such a point. So look for the biggest gap along this mono */
	    /*  and test in the middle of that */
	    last = bottom; gap_len = 0; test=-1;
	    for ( i=0; ends[which][i]<top; ++i ) {
		if ( ends[which][i]>bottom ) {
		    if ( ends[which][i]-last > gap_len ) {
			gap_len = ends[which][i]-last;
			test = last + gap_len/2;
		    }
		    last = ends[which][i];
		}
	    }
	    if ( top-last > gap_len ) {
		gap_len = top-last;
		test = last + gap_len/2;
	    }
	    if ( test!=last && test!=top )
		FigureNeeds(ms,which,test,space,ot,closeness_level[l]);
	}
    }
    for ( m=ms; m!=NULL; m=m->linked ) if ( !m->isneeded && !m->isunneeded ) {
	SOError( "Neither needed nor unneeded (%g,%g)->(%g,%g)\n",
		    (double) (((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d),
		    (double) (((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d));
    }
    free(ends[0]);
    free(ends[1]);
    free(space);
    free(gaps);
return( ilist );
}

static void FindUnitVectors(Intersection *ilist) {
    MList *ml;
    Intersection *il;
    BasePoint u;
    bigreal len;

    for ( il=ilist; il!=NULL; il=il->next ) {
	for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	    if ( ml->m->isneeded ) {
		Spline *s = ml->m->s;
		bigreal t1, t2;
		t1 = ml->t;
		if ( ml->isend )
		    t2 = ml->t - (ml->t-ml->m->tstart)/20.0;
		else
		    t2 = ml->t + (ml->m->tend-ml->t)/20.0;
		u.x = ((s->splines[0].a*t1 + s->splines[0].b)*t1 + s->splines[0].c)*t1 -
			((s->splines[0].a*t2 + s->splines[0].b)*t2 + s->splines[0].c)*t2;
		u.y = ((s->splines[1].a*t1 + s->splines[1].b)*t1 + s->splines[1].c)*t1 -
			((s->splines[1].a*t2 + s->splines[1].b)*t2 + s->splines[1].c)*t2;
		len = u.x*u.x + u.y*u.y;
		if ( len!=0 ) {
		    len = sqrt(len);
		    u.x /= len;
		    u.y /= len;
		}
		ml->unit = u;
	    }
	}
    }
}

static void TestForBadDirections(Intersection *ilist) {
    /* If we have a glyph with at least two contours one drawn clockwise, */
    /*  one counter, and these two intersect, then our algorithm will */
    /*  not remove what appears to the user to be an overlap. Warn about */
    /*  this. */
    /* I think it happens if all exits from an intersection are needed */
    MList *ml, *ml2;
    int cnt, ncnt;
    Intersection *il;

    /* If we have two splines one going from a->b and the other from b->a */
    /*  tracing exactly the same route, then they should cancel each other */
    /*  out. But depending on the order we hit them they may both be marked */
    /*  needed */  /* OverlapBugs.sfd: asciicircumflex */
    for ( il=ilist; il!=NULL; il=il->next ) {
	for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	    if ( ml->m->isneeded && ml->m->s->knownlinear &&
		    ml->m->start!=NULL && ml->m->end!=NULL ) {
		for ( ml2 = ml->next; ml2!=NULL; ml2=ml2->next ) {
		    if ( ml2->m->isneeded && ml2->m->s->knownlinear &&
			    ml2->m->start == ml->m->end &&
			    ml2->m->end == ml->m->start ) {
			ml2->m->isneeded = false;
			ml->m->isneeded = false;
			ml2->m->isunneeded = true;
			ml->m->isunneeded = true;
		break;
		    }
		}
	    }
	}
    }

    while ( ilist!=NULL ) {
	cnt = ncnt = 0;
	for ( ml = ilist->monos; ml!=NULL; ml=ml->next ) {
	    ++cnt;
	    if ( ml->m->isneeded ) ++ncnt;
	}
	ilist = ilist->next;
    }
}

static void MonoFigure(Spline *s,extended firstt,extended endt, SplinePoint *first,
	SplinePoint *end) {
    extended f;
    Spline1D temp;

    f = endt - firstt;
    first->nonextcp = false; end->noprevcp = false;
    if ( s->order2 ) {
	/*temp.d = first->me.x;*/
	/*temp.a = 0;*/
	/* temp.b = s->splines[0].b *f*f; */
	temp.c = (s->splines[0].c + 2*s->splines[0].b*firstt) * f;
	end->prevcp.x = first->nextcp.x = first->me.x + temp.c/2;
	if ( temp.c>-.003 && temp.c<.003 ) end->prevcp.x = first->nextcp.x = first->me.x;

	temp.c = (s->splines[1].c + 2*s->splines[1].b*firstt) * f;
	end->prevcp.y = first->nextcp.y = first->me.y + temp.c/2;
	if ( temp.c>-.003 && temp.c<.003 ) end->prevcp.y = first->nextcp.y = first->me.y;
	SplineMake2(first,end);
    } else {
	/*temp.d = first->me.x;*/
	/*temp.a = s->splines[0].a*f*f*f;*/
	temp.b = (s->splines[0].b + 3*s->splines[0].a*firstt) *f*f;
	temp.c = (s->splines[0].c + 2*s->splines[0].b*firstt + 3*s->splines[0].a*firstt*firstt) * f;
	first->nextcp.x = first->me.x + temp.c/3;
	end->prevcp.x = first->nextcp.x + (temp.b+temp.c)/3;
	if ( temp.c>-.01 && temp.c<.01 ) first->nextcp.x = first->me.x;
	if ( (temp.b+temp.c)>-.01 && (temp.b+temp.c)<.01 ) end->prevcp.x = end->me.x;

	temp.b = (s->splines[1].b + 3*s->splines[1].a*firstt) *f*f;
	temp.c = (s->splines[1].c + 2*s->splines[1].b*firstt + 3*s->splines[1].a*firstt*firstt) * f;
	first->nextcp.y = first->me.y + temp.c/3;
	end->prevcp.y = first->nextcp.y + (temp.b+temp.c)/3;
	if ( temp.c>-.01 && temp.c<.01 ) first->nextcp.y = first->me.y;
	if ( (temp.b+temp.c)>-.01 && (temp.b+temp.c)<.01 ) end->prevcp.y = end->me.y;
	SplineMake3(first,end);
    }
    if ( SplineIsLinear(first->next)) {
	first->nextcp = first->me;
	end->prevcp = end->me;
	first->nonextcp = end->noprevcp = true;
	SplineRefigure(first->next);
    }
}

static Intersection *MonoFollow(Intersection *curil, Monotonic *m) {
    Monotonic *mstart=m;

    if ( m->start==curil ) {
	while ( m!=NULL && m->end==NULL ) {
	    m=m->next;
	    if ( m==mstart )
	break;
	}
	if ( m==NULL )
return( NULL );

return( m->end );
    } else {
	while ( m!=NULL && m->start==NULL ) {
	    m=m->prev;
	    if ( m==mstart )
	break;
	}
	if ( m==NULL )
return( NULL );

return( m->start );
    }
}

static int MonoGoesSomewhereUseful(Intersection *curil, Monotonic *m) {
    Intersection *nextil = MonoFollow(curil,m);
    MList *ml;
    int cnt;

    if ( nextil==NULL )
return( false );
    cnt = 0;
    for ( ml=nextil->monos; ml!=NULL ; ml=ml->next )
	if ( ml->m->isneeded )
	    ++cnt;
    if ( cnt>=2 )	/* One for the mono that one in, one for another going out... */
return( true );

return( false );
}

static MList *FindMLOfM(Intersection *curil,Monotonic *finalm) {
    MList *ml;

    for ( ml=curil->monos; ml!=NULL; ml=ml->next ) {
	if ( ml->m==finalm )
return( ml );
    }
return( NULL );
}

static SplinePoint *MonoFollowForward(Intersection **curil, MList *ml,
	SplinePoint *last, Monotonic **finalm) {
    SplinePoint *mid;
    Monotonic *m = ml->m, *mstart;

    for (;;) {
	for ( mstart = m; m->s==mstart->s; m=m->next) {
	    if ( !m->isneeded )
		SOError( "Expected needed monotonic @(%g,%g) (%g,%g)->(%g,%g).\n", (*curil)->inter.x, (*curil)->inter.y,
		    (double) (((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d),
		    (double) (((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d) );
	    m->isneeded = false;		/* Mark as used */
	    if ( m->end!=NULL )
	break;
	}
	if ( m->s==mstart->s ) {
	    if ( m->end==NULL ) SOError( "Invariant condition does not hold.\n" );
	    mid = SplinePointCreate(m->end->inter.x,m->end->inter.y);
	} else {
	    m = m->prev;
	    mid = SplinePointCreate(m->s->to->me.x,m->s->to->me.y);
	}
	if ( mstart->tstart==0 && m->tend==1.0 ) {
	    /* I check for this special case to avoid rounding errors */
	    last->nextcp = m->s->from->nextcp;
	    last->nonextcp = m->s->from->nonextcp;
	    mid->prevcp = m->s->to->prevcp;
	    mid->noprevcp = m->s->to->noprevcp;
	    SplineMake(last,mid,m->s->order2);
	} else {
	    MonoFigure(m->s,mstart->tstart,m->tend,last,mid);
	}
	last = mid;
	if ( m->end!=NULL ) {
	    *curil = m->end;
	    *finalm = m;
return( last );
	}
	m = m->next;
    }
}

static SplinePoint *MonoFollowBackward(Intersection **curil, MList *ml,
	SplinePoint *last, Monotonic **finalm) {
    SplinePoint *mid;
    Monotonic *m = ml->m, *mstart;

    for (;;) {
	for ( mstart=m; m->s==mstart->s; m=m->prev) {
	    if ( !m->isneeded )
		SOError( "Expected needed monotonic (back) @(%g,%g) (%g,%g)->(%g,%g).\n", (double) (*curil)->inter.x, (double) (*curil)->inter.y,
		    (double) (((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d),
		    (double) (((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d) );
	    m->isneeded = false;		/* Mark as used */
	    if ( m->start!=NULL )
	break;
	}
	if ( m->s==mstart->s ) {
	    if ( m->start==NULL ) SOError( "Invariant condition does not hold.\n" );
	    mid = SplinePointCreate(m->start->inter.x,m->start->inter.y);
	} else {
	    m = m->next;
	    mid = SplinePointCreate(m->s->from->me.x,m->s->from->me.y);
	}
	if ( m->s->knownlinear ) mid->pointtype = pt_corner;
	if ( mstart->tend==1.0 && m->tstart==0 ) {
	    /* I check for this special case to avoid rounding errors */
	    last->nextcp = m->s->to->prevcp;
	    last->nonextcp = m->s->to->noprevcp;
	    mid->prevcp = m->s->from->nextcp;
	    mid->noprevcp = m->s->from->nonextcp;
	    SplineMake(last,mid,m->s->order2);
	} else {
	    MonoFigure(m->s,mstart->tend,m->tstart,last,mid);
	}
	last = mid;
	if ( m->start!=NULL ) {
	    *curil = m->start;
	    *finalm = m;
return( last );
	}
	m = m->prev;
    }
}

static SplineSet *JoinAContour(Intersection *startil,MList *ml) {
    SplineSet *ss = chunkalloc(sizeof(SplineSet));
    SplinePoint *last;
    Intersection *curil;
    int allexclude = ml->m->exclude;
    Monotonic *finalm;
    MList *lastml;

    // Start building a new spline.
    ss->first = last = SplinePointCreate(startil->inter.x,startil->inter.y);
    curil = startil;
    for (;;) {
	if ( allexclude && !ml->m->exclude ) allexclude = false;
	finalm = NULL;
	// Create a spline on the attached monotonic if it is in fact connected here.
	if ( ml->m->start==curil ) {
	    last = MonoFollowForward(&curil,ml,last,&finalm);
	} else if ( ml->m->end==curil ) {
	    SONotify("Building contour backwards.\n");
	    last = MonoFollowBackward(&curil,ml,last,&finalm);
	} else {
	    SOError( "Couldn't find endpoint (%g,%g).\n",
		    (double) curil->inter.x, (double) curil->inter.y );
	    ml->m->isneeded = false;		/* Prevent infinite loops */
	    ss->last = last;
    break;
	}
	if ( curil==startil ) {
	    // Close the curve if we're back at the beginning.
	    // Connect the first point to the last segment.
	    ss->first->prev = last->prev;
	    ss->first->prevcp = last->prevcp;
	    ss->first->noprevcp = last->noprevcp;
	    last->prev->to = ss->first;
	    // And then delete the last point since it's been replaced by the first.
	    SplinePointFree(last);
	    ss->last = ss->first;
    break;
	}
	// Find the record for the current monotonic at the newly traversed intersection.
	lastml = FindMLOfM(curil,finalm);
	if ( lastml==NULL ) {
	    SOError("Could not find finalm");
	    /* Try to preserve direction */
	    for ( ml=curil->monos; ml!=NULL && (!ml->m->isneeded || ml->m->end==curil); ml=ml->next );
	    if ( ml==NULL )
		for ( ml=curil->monos; ml!=NULL && !ml->m->isneeded; ml=ml->next );
	} else {
	    
	    int k; MList *bestml; bigreal bestdot;
	    for ( k=0; k<2; ++k ) {
		bestml = NULL; bestdot = -2;
		for ( ml=curil->monos; ml!=NULL ; ml=ml->next ) {
		    if ( ml->m->isneeded && (ml->m->start==curil || k) ) {
			bigreal dot = lastml->unit.x*ml->unit.x + lastml->unit.y*ml->unit.y;
			if ( dot>bestdot ) {
			    bestml = ml;
			    bestdot = dot;
			}
		    }
		}
		if ( bestml!=NULL )
	    break;
	    }
	    ml = bestml;
	}
	if ( ml==NULL ) {
	    for ( ml=curil->monos; ml!=NULL ; ml=ml->next )
		if ( ml->m->isunneeded && ml->m->start==curil &&
			MonoFollow(curil,ml->m)==startil )
	    break;
	    if ( ml==NULL )
		for ( ml=curil->monos; ml!=NULL ; ml=ml->next )
		    if ( ml->m->isunneeded && ml->m->end==curil &&
			    MonoFollow(curil,ml->m)==startil )
		break;
	    if ( ml!=NULL ) {
		SOError("Closing contour with unneeded path\n" );
		ml->m->isneeded = true;
	    }
	}
	if ( ml==NULL ) {
	    SOError( "couldn't find a needed exit from an intersection\n" );
	    ss->last = last;
    break;
	}
    }
    SPLCategorizePoints(ss);
    if ( allexclude && SplinePointListIsClockwise(ss)==1 )
	SplineSetReverse(ss);
return( ss );
}

static SplineSet *FindMatchingContour(SplineSet *head,SplineSet *cur) {
    SplineSet *test;

    for ( test=head; test!=NULL; test=test->next ) {
	if ( test->first->prev==NULL &&
		test->first->me.x==cur->last->me.x && test->first->me.y==cur->last->me.y &&
		test->last->me.x==cur->first->me.x && test->last->me.y==cur->first->me.y )
    break;
    }
    if ( test==NULL ) {
	for ( test=head; test!=NULL; test=test->next ) {
	    if ( test->first->prev==NULL &&
		    test->last->me.x==cur->last->me.x && test->last->me.y==cur->last->me.y &&
		    test->first->me.x==cur->first->me.x && test->first->me.y==cur->first->me.y ) {
		SplineSetReverse(cur);
	break;
	    }
	}
    }
    if ( test==NULL ) {
	for ( test=head; test!=NULL; test=test->next ) {
	    if ( test->first->prev==NULL &&
		    ((test->first->me.x==cur->last->me.x && test->first->me.y==cur->last->me.y) ||
		     (test->last->me.x==cur->first->me.x && test->last->me.y==cur->first->me.y )))
	break;
	}
    }
    if ( test==NULL ) {
	for ( test=head; test!=NULL; test=test->next ) {
	    if ( test->first->prev==NULL &&
		    ((test->last->me.x==cur->last->me.x && test->last->me.y==cur->last->me.y) ||
		     (test->first->me.x==cur->first->me.x && test->first->me.y==cur->first->me.y ))) {
		SplineSetReverse(cur);
	break;
	    }
	}
    }
return( test );
}

static SplineSet *JoinAllNeeded(Intersection *ilist) {
    Intersection *il;
    SplineSet *head=NULL, *last=NULL, *cur, *test;
    MList *ml;
    int reverse_flag = 0; // We set this if we're following a path backwards so that we know to fix it when done.

    for ( il=ilist; il!=NULL; il=il->next ) {
	/* Try to preserve direction */
	for (;;) {
	    reverse_flag = 0;
	    // We loop until there are no more monotonics connected to this intersection.
	    // First we iterate through the connected monotonics until we find one that is needed (and not already handled) and starts at this intersection.
	    for ( ml=il->monos; ml!=NULL && (!ml->m->isneeded || ml->m->end==il); ml=ml->next );
	    // If we do not find such a monotonic, we allow monotonics that end at this intersection.
	    if ( ml==NULL ) {
		for ( ml=il->monos; ml!=NULL && !ml->m->isneeded; ml=ml->next );
		if (ml != NULL) {
		  // Unfortunately, this probably means that something is wrong since we ought to have only closed curves at this point.
		  // The problem is most likely in the needed/unneeded logic.
		  SONotify("An intersection has a terminating monotonic but not a starting monotonic.\n");
		  reverse_flag = 1; // We'll need to reverse this later.
		}
	    }
	    if ( ml==NULL )
	break;
	    if ( !MonoGoesSomewhereUseful(il,ml->m)) {
		Monotonic *m = ml->m;
		SOError("Humph. This monotonic leads nowhere (%g,%g)->(%g,%g).\n",
		    (double) (((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d),
		    (double) (((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d),
		    (double) (((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d) );
	/* break; */
	    }
	    cur = JoinAContour(il,ml);
	    if (reverse_flag == 1) SplineSetReverse(cur);
	    if ( head==NULL )
		head = cur;
	    else {
		if ( cur->first->prev==NULL ) {
		    /* Open contours are errors. See if we had an earlier error */
		    /*  to which we can join this */
		    test = FindMatchingContour(head,cur);
		    if ( test!=NULL ) {
			if ( test->first->me.x==cur->last->me.x && test->first->me.y==cur->last->me.y ) {
			    test->first->prev = cur->last->prev;
			    cur->last->prev->to = test->first;
			    SplinePointFree(cur->last);
			    if ( test->last->me.x==cur->first->me.x && test->last->me.y==cur->first->me.y ) {
				test->last->next = cur->first->next;
			        cur->first->next->from = test->last;
			        SplinePointFree(cur->first);
			        test->last = test->first;
			    } else
				test->first = cur->first;
			} else {
			    if ( test->last->me.x!=cur->first->me.x || test->last->me.y!=cur->first->me.y )
				SOError( "Join failed");
			    else {
				test->last->next = cur->first->next;
			        cur->first->next->from = test->last;
			        SplinePointFree(cur->first);
			        test->last = test->first;
			    }
			}
			cur->first = cur->last = NULL;
			SplinePointListFree(cur);
			cur=NULL;
		    }
		}
		if ( cur!=NULL )
		    last->next = cur;
	    }
	    if ( cur!=NULL )
		last = cur;
	}
    }
return( head );
}

static SplineSet *MergeOpenAndFreeClosed(SplineSet *new,SplineSet *old,
	enum overlap_type ot) {
    SplineSet *next;

    while ( old!=NULL ) {
	next = old->next;
	if ( old->first->prev==NULL ||
		(( ot==over_rmselected || ot==over_intersel || ot==over_fisel) &&
		  !SSIsSelected(old)) ) {
	    old->next = new;
	    new = old;
	} else {
	    old->next = NULL;
	    SplinePointListFree(old);
	}
	old = next;
    }
return(new);
}

void FreeMonotonics(Monotonic *m) {
    Monotonic *next;

    while ( m!=NULL ) {
	next = m->linked;
	chunkfree(m,sizeof(*m));
	m = next;
    }
}

static void FreeMList(MList *ml) {
    MList *next;

    while ( ml!=NULL ) {
	next = ml->next;
	chunkfree(ml,sizeof(*ml));
	ml = next;
    }
}

static void FreeIntersections(Intersection *ilist) {
    Intersection *next;

    while ( ilist!=NULL ) {
	next = ilist->next;
	FreeMList(ilist->monos);
	chunkfree(ilist,sizeof(*ilist));
	ilist = next;
    }
}

static void MonoSplit(Monotonic *m) {
    Spline *s = m->s;
    SplinePoint *last = s->from;
    SplinePoint *final = s->to;
    extended lastt = 0;

    last->next = NULL;
    final->prev = NULL;
    while ( m!=NULL && m->s==s && m->tend<1 ) {
	if ( m->end!=NULL ) {
	    SplinePoint *mid = SplinePointCreate(m->end->inter.x,m->end->inter.y);
	    if ( m->s->knownlinear ) mid->pointtype = pt_corner;
	    MonoFigure(s,lastt,m->tend,last,mid);
	    lastt = m->tend;
	    last = mid;
	}
	m = m->linked;
    }
    MonoFigure(s,lastt,1.0,last,final);
    SplineFree(s);
}

static void FixupIntersectedSplines(Monotonic *ms) {
    /* If all we want is intersections, then the contours are already correct */
    /*  all we need to do is run through the Monotonic list and when we find */
    /*  an intersection, make sure it has real splines around it */
    Monotonic *m;
    int needs_split;

    while ( ms!=NULL ) {
	needs_split = false;
	for ( m=ms; m!=NULL && m->s==ms->s; m=m->linked ) {
	    if ( (m->tstart!=0 && m->start!=NULL) || (m->tend!=1 && m->end!=NULL))
		needs_split = true;
	}
	if ( needs_split )
	    MonoSplit(ms);
	ms = m;
    }
}

static int BpClose(BasePoint *here, BasePoint *there, bigreal error) {
    extended dx, dy;

    if ( (dx = here->x-there->x)<0 ) dx= -dx;
    if ( (dy = here->y-there->y)<0 ) dy= -dy;
    if ( dx<error && dy<error )
return( true );

return( false );
}

static SplineSet *SSRemoveTiny(SplineSet *base) {
    DBounds b;
    bigreal error;
    extended test, dx, dy;
    SplineSet *prev = NULL, *head = base, *ssnext;
    SplinePoint *sp, *nsp;

    SplineSetQuickBounds(base,&b);
    error = b.maxy-b.miny;
    test = b.maxx-b.minx;
    if ( test>error ) error = test;
    if ( (test = b.maxy)<0 ) test = -test;
    if ( test>error ) error = test;
    if ( (test = b.maxx)<0 ) test = -test;
    if ( test>error ) error = test;
    error /= 30000;

    while ( base!=NULL ) {
	ssnext = base->next;
	for ( sp=base->first; ; ) {
	    if ( sp->next==NULL )
	break;
	    nsp = sp->next->to;
	    if ( BpClose(&sp->me,&nsp->me,error) ) {
		// A spline with ends this close is likely to cause problems.
		// So we want to remove it, or, if it is significant, to consolidate the end points.
		if ( BpClose(&sp->me,&sp->nextcp,2*error) &&
			BpClose(&nsp->me,&nsp->prevcp,2*error)) {
		    /* Remove the spline if the control points are also extremely close */
		    if ( nsp==sp ) {
			/* Only this spline in the contour, so remove the contour */
			base->next = NULL;
			SplinePointListFree(base);
			if ( prev==NULL )
			    head = ssnext;
			else
			    prev->next = ssnext;
			base = NULL;
	break;
		    }
		    // We want to remove the spline following sp.
		    // This requires that we rewrite the following spline so that it starts at sp,
		    // that we point the control point reference in sp to the next control point,
		    // and that we refigure the spline.
		    // So, first we free the next spline.
		    SplineFree(sp->next);
		    // If the next point has a next control point, we copy it to the next control point for this point.
		    if ( nsp->nonextcp ) {
			sp->nextcp = sp->me;
			sp->nonextcp = true;
		    } else {
			sp->nextcp = nsp->nextcp;
			sp->nonextcp = false;
		    }
		    sp->nextcpdef = nsp->nextcpdef;
		    sp->next = nsp->next; // Change the spline reference.
		    if ( nsp->next!=NULL ) {
			// Make the next spline refer to sp and refigure it.
			nsp->next->from = sp;
			SplineRefigure(sp->next);
		    }
		    if ( nsp==base->last )
			base->last = sp;
		    if ( nsp==base->first )
			base->first = sp;
		    SplinePointFree(nsp);
		    if ( sp->next==NULL )
	break;
		    nsp = sp->next->to;
		} else {
		    /* Leave the spline, since it goes places, but move the two points together */
		    BasePoint new;
		    new.x = (sp->me.x+nsp->me.x)/2;
		    new.y = (sp->me.y+nsp->me.y)/2;
		    dx = new.x-sp->me.x; dy = new.y-sp->me.y;
		    sp->me = new;
		    sp->nextcp.x += dx; sp->nextcp.y += dy;
		    sp->prevcp.x += dx; sp->prevcp.y += dy;
		    dx = new.x-nsp->me.x; dy = new.y-nsp->me.y;
		    nsp->me = new;
		    nsp->nextcp.x += dx; nsp->nextcp.y += dy;
		    nsp->prevcp.x += dx; nsp->prevcp.y += dy;
		    if (sp->next->order2) {
		      // The control points must be identical if the curve is quadratic.
		      BasePoint new2;
		      new2.x = (sp->nextcp.x+nsp->prevcp.x)/2;
		      new2.y = (sp->nextcp.y+nsp->prevcp.y)/2;
		      sp->nextcp = nsp->prevcp = new2;
		    }
		    SplineRefigure(sp->next);
		    if ( sp->prev ) {
		      if (sp->prev->order2) {
		        // The control points must be identical if the curve is quadratic.
		        BasePoint new2;
		        new2.x = (sp->prev->from->nextcp.x+sp->prevcp.x)/2;
		        new2.y = (sp->prev->from->nextcp.y+sp->prevcp.y)/2;
		        sp->prev->from->nextcp = sp->prevcp = new2;
		      }
		      SplineRefigure(sp->prev);
		    }
		    if ( nsp->next ) {
		      if (nsp->next->order2) {
		        // The control points must be identical if the curve is quadratic.
		        BasePoint new2;
		        new2.x = (nsp->nextcp.x+nsp->next->to->prevcp.x)/2;
		        new2.y = (nsp->nextcp.y+nsp->next->to->prevcp.y)/2;
		        nsp->nextcp = nsp->next->to->prevcp = new2;
		      }
		      SplineRefigure(nsp->next);
		    }
		}
	    }
	    sp = nsp;
	    if ( sp==base->first )
	break;
	}
	if ( sp->prev!=NULL && !sp->noprevcp ) {
	    int refigure = false;
	    if ( sp->me.x-sp->prevcp.x>-error && sp->me.x-sp->prevcp.x<error ) {
		// We round the x-value of the previous control point to the on-curve point value if it is close.
		sp->prevcp.x = sp->me.x;
		// If the curve is quadratic, we need to update the corresponding values in the previous point.
		if ((sp->prev) && (sp->prev->order2) && (sp->prev->from)) sp->prev->from->nextcp.x = sp->me.x;
		refigure = true;
	    }
	    if ( sp->me.y-sp->prevcp.y>-error && sp->me.y-sp->prevcp.y<error ) {
		// We round the y-value of the previous control point to the on-curve point value if it is close.
		sp->prevcp.y = sp->me.y;
		// If the curve is quadratic, we need to update the corresponding values in the previous point.
		if ((sp->prev) && (sp->prev->order2) && (sp->prev->from)) sp->prev->from->nextcp.y = sp->me.y;
		refigure = true;
	    }
	    if ( sp->me.x==sp->prevcp.x && sp->me.y==sp->prevcp.y ) {
		// We disable the control point if necessary.
		sp->noprevcp = true;
		if ((sp->prev) && (sp->prev->order2) && (sp->prev->from)) sp->prev->from->nonextcp = true;
	    }
	    if ( refigure )
		SplineRefigure(sp->prev);
	}
	if ( sp->next!=NULL && !sp->nonextcp ) {
	    int refigure = false;
	    if ( sp->me.x-sp->nextcp.x>-error && sp->me.x-sp->nextcp.x<error ) {
		// We round the x-value of the next control point to the on-curve point value if it is close.
		sp->nextcp.x = sp->me.x;
		// If the curve is quadratic, we need to update the corresponding values in the next point.
		if ((sp->next) && (sp->next->order2) && (sp->next->to)) sp->next->to->prevcp.x = sp->me.x;
		refigure = true;
	    }
	    if ( sp->me.y-sp->nextcp.y>-error && sp->me.y-sp->nextcp.y<error ) {
		// We round the x-value of the next control point to the on-curve point value if it is close.
		sp->nextcp.y = sp->me.y;
		// If the curve is quadratic, we need to update the corresponding values in the next point.
		if ((sp->next) && (sp->next->order2) && (sp->next->to)) sp->next->to->prevcp.y = sp->me.y;
		refigure = true;
	    }
	    if ( sp->me.x==sp->nextcp.x && sp->me.y==sp->nextcp.y ) {
		// We disable the control point if necessary.
		sp->nonextcp = true;
		if ((sp->next) && (sp->next->order2) && (sp->next->to)) sp->next->to->noprevcp = true;
	    }
	    if ( refigure )
		SplineRefigure(sp->next);
	}
	if ( base!=NULL )
	    prev = base;
	base = ssnext;
    }

return( head );
}

static void RemoveNextSP(SplinePoint *psp,SplinePoint *sp,SplinePoint *nsp,
	SplineSet *base) {
    if ( psp==nsp ) {
	SplineFree(psp->next);
	psp->next = psp->prev;
	psp->next->from = psp;
	SplinePointFree(sp);
	SplineRefigure(psp->prev);
    } else {
	psp->next = nsp->next;
	psp->next->from = psp;
	psp->nextcp = nsp->nextcp;
	psp->nonextcp = nsp->nonextcp;
	psp->nextcpdef = nsp->nextcpdef;
	SplineFree(sp->prev);
	SplineFree(sp->next);
	SplinePointFree(sp);
	SplinePointFree(nsp);
	SplineRefigure(psp->next);
    }
    if ( base->first==sp || base->first==nsp )
	base->first = psp;
    if ( base->last==sp || base->last==nsp )
	base->last = psp;
}

static void RemovePrevSP(SplinePoint *psp,SplinePoint *sp,SplinePoint *nsp,
	SplineSet *base) {
    if ( psp==nsp ) {
	SplineFree(nsp->prev);
	nsp->prev = nsp->next;
	nsp->prev->to = nsp;
	SplinePointFree(sp);
	SplineRefigure(nsp->next);
    } else {
	nsp->prev = psp->prev;
	nsp->prev->to = nsp;
	nsp->prevcp = nsp->me;
	nsp->noprevcp = true;
	nsp->prevcpdef = psp->prevcpdef;
	SplineFree(sp->prev);
	SplineFree(sp->next);
	SplinePointFree(sp);
	SplinePointFree(psp);
	SplineRefigure(nsp->prev);
    }
    if ( base->first==sp || base->first==psp )
	base->first = nsp;
    if ( base->last==sp || base->last==psp )
	base->last = nsp;
}

static SplinePoint *SameLine(SplinePoint *psp, SplinePoint *sp, SplinePoint *nsp) {
    BasePoint noff, poff;
    real nlen, plen, normal;

    noff.x = nsp->me.x-sp->me.x; noff.y = nsp->me.y-sp->me.y;
    poff.x = psp->me.x-sp->me.x; poff.y = psp->me.y-sp->me.y;
    nlen = sqrt(noff.x*noff.x + noff.y*noff.y);
    plen = sqrt(poff.x*poff.x + poff.y*poff.y);
    if ( nlen==0 )
return( nsp );
    if ( plen==0 )
return( psp );
    normal = (noff.x*poff.y - noff.y*poff.x)/nlen/plen;
    if ( normal<-.0001 || normal>.0001 )
return( NULL );

    if ( noff.x*poff.x < 0 || noff.y*poff.y < 0 )
return( NULL );		/* Same line, but different directions */
    if ( (noff.x>0 && noff.x>poff.x) ||
	    (noff.x<0 && noff.x<poff.x) ||
	    (noff.y>0 && noff.y>poff.y) ||
	    (noff.y<0 && noff.y<poff.y))
return( nsp );

return( psp );
}

static bigreal AdjacentSplinesMatch(Spline *s1,Spline *s2,int s2forward) {
    /* Is every point on s2 close to a point on s1 */
    bigreal t, tdiff, t1 = -1;
    bigreal xoff, yoff;
    bigreal t1start, t1end;
    extended ts[2];
    int i;

    if ( (xoff = s2->to->me.x-s2->from->me.x)<0 ) xoff = -xoff;
    if ( (yoff = s2->to->me.y-s2->from->me.y)<0 ) yoff = -yoff;
    if ( xoff>yoff )
	SplineFindExtrema(&s1->splines[0],&ts[0],&ts[1]);
    else
	SplineFindExtrema(&s1->splines[1],&ts[0],&ts[1]);
    if ( s2forward ) {
	t = 0;
	tdiff = 1/16.0;
	t1end = 1;
	for ( i=1; i>=0 && ts[i]==-1; --i );
	t1start = i<0 ? 0 : ts[i];
    } else {
	t = 1;
	tdiff = -1/16.0;
	t1start = 0;
	t1end = ( ts[0]==-1 ) ? 1.0 : ts[0];
    }

    for ( ; (s2forward && t<=1) || (!s2forward && t>=0 ); t += tdiff ) {
	bigreal x1, y1, xo, yo;
	bigreal x = ((s2->splines[0].a*t+s2->splines[0].b)*t+s2->splines[0].c)*t+s2->splines[0].d;
	bigreal y = ((s2->splines[1].a*t+s2->splines[1].b)*t+s2->splines[1].c)*t+s2->splines[1].d;
	if ( xoff>yoff )
	    t1 = IterateSplineSolveFixup(&s1->splines[0],t1start,t1end,x);
	else
	    t1 = IterateSplineSolveFixup(&s1->splines[1],t1start,t1end,y);
	if ( t1<0 || t1>1 )
return( -1 );
	x1 = ((s1->splines[0].a*t1+s1->splines[0].b)*t1+s1->splines[0].c)*t1+s1->splines[0].d;
	y1 = ((s1->splines[1].a*t1+s1->splines[1].b)*t1+s1->splines[1].c)*t1+s1->splines[1].d;
	if ( (xo = (x-x1))<0 ) xo = -xo;
	if ( (yo = (y-y1))<0 ) yo = -yo;
	if ( xo+yo>.5 )
return( -1 );
    }
return( t1 );
}
	
void SSRemoveBacktracks(SplineSet *ss) {
    SplinePoint *sp;

    if ( ss==NULL )
return;
    for ( sp=ss->first; ; ) {
	if ( sp->next!=NULL && sp->prev!=NULL ) {
	    SplinePoint *nsp = sp->next->to, *psp = sp->prev->from, *isp;
	    BasePoint ndir, pdir;
	    bigreal dot, pdot, nlen, plen, t = -1;

	    ndir.x = (nsp->me.x - sp->me.x); ndir.y = (nsp->me.y - sp->me.y);
	    pdir.x = (psp->me.x - sp->me.x); pdir.y = (psp->me.y - sp->me.y);
	    nlen = ndir.x*ndir.x + ndir.y*ndir.y; plen = pdir.x*pdir.x + pdir.y*pdir.y;
	    dot = ndir.x*pdir.x + ndir.y*pdir.y;
	    if ( (pdot = ndir.x*pdir.y - ndir.y*pdir.x)<0 ) pdot = -pdot;
	    if ( dot>0 && dot>pdot ) {
		if ( nlen>plen && (t=AdjacentSplinesMatch(sp->next,sp->prev,false))!=-1 ) {
		    isp = SplineBisect(sp->next,t);
		    psp->nextcp.x = psp->me.x + (isp->nextcp.x-isp->me.x);
		    psp->nextcp.y = psp->me.y + (isp->nextcp.y-isp->me.y);
		    psp->nonextcp = isp->nonextcp;
		    psp->next = isp->next;
		    isp->next->from = psp;
		    SplineFree(isp->prev);
		    SplineFree(sp->prev);
		    SplinePointFree(isp);
		    SplinePointFree(sp);
		    if ( psp->next->order2 ) {
			psp->nextcp.x = nsp->prevcp.x = (psp->nextcp.x+nsp->prevcp.x)/2;
			psp->nextcp.y = nsp->prevcp.y = (psp->nextcp.y+nsp->prevcp.y)/2;
			if ( psp->nonextcp || nsp->noprevcp )
			    psp->nonextcp = nsp->noprevcp = true;
		    }
		    SplineRefigure(psp->next);
		    if ( ss->first==sp )
			ss->first = psp;
		    if ( ss->last==sp )
			ss->last = psp;
		    sp=psp;
		} else if ( nlen<plen && (t=AdjacentSplinesMatch(sp->prev,sp->next,true))!=-1 ) {
		    isp = SplineBisect(sp->prev,t);
		    nsp->prevcp.x = nsp->me.x + (isp->prevcp.x-isp->me.x);
		    nsp->prevcp.y = nsp->me.y + (isp->prevcp.y-isp->me.y);
		    nsp->noprevcp = isp->noprevcp;
		    nsp->prev = isp->prev;
		    isp->prev->to = nsp;
		    SplineFree(isp->next);
		    SplineFree(sp->next);
		    SplinePointFree(isp);
		    SplinePointFree(sp);
		    if ( psp->next->order2 ) {
			psp->nextcp.x = nsp->prevcp.x = (psp->nextcp.x+nsp->prevcp.x)/2;
			psp->nextcp.y = nsp->prevcp.y = (psp->nextcp.y+nsp->prevcp.y)/2;
			if ( psp->nonextcp || nsp->noprevcp )
			    psp->nonextcp = nsp->noprevcp = true;
		    }
		    SplineRefigure(nsp->prev);
		    if ( ss->first==sp )
			ss->first = psp;
		    if ( ss->last==sp )
			ss->last = psp;
		    sp=psp;
		}
	    }
	}
	if ( sp->next==NULL )
    break;
	sp=sp->next->to;
	if ( sp==ss->first )
    break;
    }
}


static int BetweenForCollinearPoints( SplinePoint* a, SplinePoint* middle, SplinePoint* b )
{
    int ret = 0;

    if( a->me.x <= middle->me.x && middle->me.x <= b->me.x )
	if( a->me.y <= middle->me.y && middle->me.y <= b->me.y )
	    return 1;
    
    return ret;
}

/* If we have a contour with no width, say a line from A to B and then from B */
/*  to A, then it will be ambiguous, depending on how we hit the contour, as  */
/*  to whether it is needed or not. Which will cause us to complain. Since    */
/*  they contain no area, they achieve nothing, so we might as well say they  */
/*  overlap themselves and remove them here */
static SplineSet *SSRemoveReversals(SplineSet *base) {
    SplineSet *head = base, *prev=NULL, *next;
    SplinePoint *sp;
    int changed;

    while ( base!=NULL ) {
	next = base->next;
	changed = true;
	while ( changed ) {
	    changed = false;
	    if ( base->first->next==NULL ||
		    (base->first->next->to==base->first &&
		     base->first->nextcp.x==base->first->prevcp.x &&
		     base->first->nextcp.y==base->first->prevcp.y)) {
		/* remove single points */
		if ( prev==NULL )
		    head = next;
		else
		    prev->next = next;
		base->next = NULL;
		SplinePointListFree(base);
		base = prev;
	break;
	    }
	    for ( sp=base->first; ; )
	    {
		if ( sp->next!=NULL && sp->prev!=NULL &&
		     sp->nextcp.x==sp->prevcp.x && sp->nextcp.y==sp->prevcp.y )
		{
		    SplinePoint *nsp = sp->next->to;
		    SplinePoint *psp = sp->prev->from;
		    SplinePoint *isp = 0;
		    
		    if ( psp->me.x==nsp->me.x && psp->me.y==nsp->me.y &&
			    psp->nextcp.x==nsp->prevcp.x && psp->nextcp.y==nsp->prevcp.y )
		    {
			/* We wish to remove sp, sp->next, sp->prev & one of nsp/psp */
			RemoveNextSP(psp,sp,nsp,base);
			changed = true;
			break;
		    }
		    else if ( sp->nonextcp /* which implies sp->noprevcp */ &&
			      psp->nonextcp && nsp->noprevcp &&
			      (isp = SameLine(psp,sp,nsp))!=NULL )
		    {
			/* printf(" sp x:%f y:%f\n",  sp->me.x,  sp->me.y ); */
			/* printf("psp x:%f y:%f\n", psp->me.x, psp->me.y ); */
			/* printf("nsp x:%f y:%f\n", nsp->me.x, nsp->me.y ); */
			/* printf("between1:%d\n", BetweenForCollinearPoints( nsp, psp, sp )); */
			/* printf("between2:%d\n", BetweenForCollinearPoints( psp, nsp, sp )); */

			if( BetweenForCollinearPoints( nsp, psp, sp )
			    || BetweenForCollinearPoints( psp, nsp, sp ) )
			{
			    /* leave these as they are */
			}
			else
			{
			    /* We have a line that backtracks, but doesn't cover */
			    /*  the entire spline, so we intersect */
			    /* We want to remove sp, the shorter of sp->next, sp->prev */
			    /*  and a bit of the other one. Also reomve one of nsp,psp */
			    if ( isp==psp )
			    {
				RemoveNextSP(psp,sp,nsp,base);
				psp->nextcp = psp->me;
				psp->nonextcp = true;
			    }
			    else
			    {
				RemovePrevSP(psp,sp,nsp,base);
			    }
			    changed = true;
			    break;
			}
		    }
		}
		if ( sp->next==NULL )
		    break;
		sp = sp->next->to;
		if ( sp==base->first )
		    break;
	    }
	}
	SSRemoveBacktracks(base);
	prev = base;
	base = next;
    }
return( head );
}

SplineSet *SplineSetRemoveOverlap(SplineChar *sc, SplineSet *base,enum overlap_type ot) {
    Monotonic *ms;
    Intersection *ilist;
    SplineSet *ret;

    if ( sc!=NULL )
	glyphname = sc->name;

    base = SSRemoveTiny(base);
    SSRemoveStupidControlPoints(base);
    SplineSetsRemoveAnnoyingExtrema(base,.3);
    /*SSOverlapClusterCpAngles(base,.01);*/
    // base = SSRemoveReversals(base);
    // Frank suspects that improvements to FindIntersections have made SSRemoveReverals unnecessary.
    // And it breaks certain glyphs such as the only glyph in rmo-triangle2.sfd from debugfonts.
    ms = SSsToMContours(base,ot);
    {
      Monotonic * tmpm = ms;
      while (tmpm != NULL) {
        ValidateMonotonic(tmpm);
        tmpm = tmpm->linked;
        if (tmpm == ms) break;
      }
    }
    ilist = FindIntersections(ms,ot);
    Validate(ms,ilist);
    if ( ot==over_findinter || ot==over_fisel ) {
	FixupIntersectedSplines(ms);
	ret = base;
    } else {
	ilist = FindNeeded(ms,ot,ilist);
	FindUnitVectors(ilist);
	if ( ot==over_remove || ot == over_rmselected )
	    TestForBadDirections(ilist);
	SplineSet *tmpret = JoinAllNeeded(ilist);
	ret = MergeOpenAndFreeClosed(tmpret,base,ot);
    }
    FreeMonotonics(ms);
    FreeIntersections(ilist);
    glyphname = NULL;
return( ret );
}
