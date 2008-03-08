/* Copyright (C) 2000-2008 by George Williams */
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
#ifndef _EDGELIST_H
#define _EDGELIST_H
#include "splinefont.h"

typedef struct hints {
    real base, width;
    real b1, b2, e1, e2;
    real ab, ae;
    unsigned int adjustb: 1;
    unsigned int adjuste: 1;
    struct hints *next;
} Hints;

/* Instead of y and x coordinates these are based on major and other */
/*  major maybe either x or y depending on what we're interested in */
/*  at the moment, and other will be the other one. Of course it's */
/*  consistant accross the datastructure at any given time */
typedef struct edge {
    real mmin, mmax;		/* relative to es->mmin */
    real t_mmin, t_mmax;
    real tmin, tmax;
    real o_mmin, o_mmax;
    real t_cur, o_cur, m_cur;
    unsigned int up: 1;		/* line is directed up in the spline list */
    unsigned int max_adjusted: 1;	/* by hstem hints */
    unsigned int min_adjusted: 1;
    Spline *spline;		/* spline which generated this segment */
    struct edge *esnext, *aenext;
    struct edge *before, *after;
    int last_opos, last_mpos;
    real oldt;		/* only used for FindIntersections of RemoveOverlap */
} Edge;

typedef struct edgelist {
    Edge **edges;
    int cnt;
    real mmin, mmax;
    real omin, omax;
    real scale;
    int bytes_per_line;
    uint8 *bitmap;
    Edge *last, *splinesetfirst;
    SplineChar *sc;
    int layer;
    char *interesting;
    int major, other;
    unsigned int genmajoredges: 1;	/* generate a list of edges parallel to the major axis */
    Edge *majors;		/* ordered so that lowest edge is first */
    Edge *majorhold;		/* to hold major edges as we pass them and they become useless */
    Hints *hhints, *vhints;
    int is_overlap;
    DBounds bbox;		/* Not always set. {m,o}{min,max} a provide scaled bbox, this is in glyph units */
} EdgeList;

extern void FreeEdges(EdgeList *es);
extern double TOfNextMajor(Edge *e, EdgeList *es, double sought_y );
extern void FindEdgesSplineSet(SplinePointList *spl, EdgeList *es, int ignore_clip);
extern Edge *ActiveEdgesInsertNew(EdgeList *es, Edge *active,int i);
extern Edge *ActiveEdgesRefigure(EdgeList *es, Edge *active,real i);
extern Edge *ActiveEdgesFindStem(Edge *apt, Edge **prev, real i);

/* Version which is better for everything other than rasterization */
/*  (I think) */
typedef struct edgeinfo {
    /* The spline is broken up at all extrema. So... */
    /*  The spline between tmin and tmax is monotonic in both coordinates */
    /*  If the spline becomes vert/horizontal that will be at one of the */
    /*   end points too */
    Spline *spline;
    real tmin, tmax;
    real coordmin[2];
    real coordmax[2];
    unsigned int up: 1;
    unsigned int hv: 1;
    unsigned int hvbottom: 1;
    unsigned int hvtop: 1;
    unsigned int hor: 1;
    unsigned int vert: 1;
    unsigned int almosthor: 1;
    unsigned int almostvert: 1;
    unsigned int horattmin: 1;
    unsigned int horattmax: 1;
    unsigned int vertattmin: 1;
    unsigned int vertattmax: 1;
    unsigned hup: 1;
    unsigned vup: 1;
    real tcur;		/* Value of t for current major coord */
    real ocur;		/* Value of the other coord for current major coord */
    struct edgeinfo *next;
    struct edgeinfo *ordered;
    struct edgeinfo *aenext;
    struct edgeinfo *splinenext;
    SplineChar *sc;
    int major;
} EI;

typedef struct eilist {
    EI *edges;
    real coordmin[2];
    real coordmax[2];
    int low, high, cnt;
    EI **ordered;
    char *ends;			/* flag to say an edge ends on this line */
    SplineChar *sc;
    int layer;
    int major;
    EI *splinelast, *splinefirst;
    EI **bottoms, **tops;	/* Used only be FindNeeded in RemoveOverlap */
    unsigned leavetiny: 1;
    enum overlap_type ot;
} EIList;

extern void ElFreeEI(EIList *el);
extern void ELFindEdges(SplineChar *sc, EIList *el);
extern void ELOrder(EIList *el, int major );
extern real EITOfNextMajor(EI *e, EIList *el, real sought_m );
extern int EISameLine(EI *e, EI *n, real i, int major);
extern int EISkipExtremum(EI *e, real i, int major);
extern EI *EIActiveEdgesFindStem(EI *apt, real i, int major);
extern EI *EIActiveListReorder(EI *active,int *change);
extern EI *EIActiveEdgesRefigure(EIList *el, EI *active,real i,int major,
	int *_change);
#endif
