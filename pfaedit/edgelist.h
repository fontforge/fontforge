/* Copyright (C) 2000,2001 by George Williams */
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
    double base, width;
    double b1, b2, e1, e2;
    double ab, ae;
    unsigned int adjustb: 1;
    unsigned int adjuste: 1;
    struct hints *next;
} Hints;

/* Instead of y and x coordinates these are based on major and other */
/*  major maybe either x or y depending on what we're interested in */
/*  at the moment, and other will be the other one. Of course it's */
/*  consistant accross the datastructure at any given time */
typedef struct edge {
    double mmin, mmax;		/* relative to es->mmin */
    double t_mmin, t_mmax;
    double tmin, tmax;
    double o_mmin, o_mmax;
    double t_cur, o_cur, m_cur;
    unsigned int up: 1;		/* line is directed up in the spline list */
    unsigned int max_adjusted: 1;	/* by hstem hints */
    unsigned int min_adjusted: 1;
    Spline *spline;		/* spline which generated this segment */
    struct edge *esnext, *aenext;
    struct edge *before, *after;
    int last_opos, last_mpos;
    double oldt;		/* only used for FindIntersections of RemoveOverlap */
} Edge;

typedef struct edgelist {
    Edge **edges;
    int cnt;
    double mmin, mmax;
    double omin, omax;
    double scale;
    int bytes_per_line;
    uint8 *bitmap;
    Edge *last, *splinesetfirst;
    SplineChar *sc;
    char *interesting;
    int major, other;
    unsigned int genmajoredges: 1;	/* generate a list of edges parrallel to the major axis */
    Edge *majors;		/* ordered so that lowest edge is first */
    Edge *majorhold;		/* to hold major edges as we pass them and they become useless */
    Hints *hhints, *vhints;
} EdgeList;

extern void FreeEdges(EdgeList *es);
extern double TOfNextMajor(Edge *e, EdgeList *es, double sought_y );
extern void FindEdgesSplineSet(SplinePointList *spl, EdgeList *es);
extern Edge *ActiveEdgesInsertNew(EdgeList *es, Edge *active,int i);
extern Edge *ActiveEdgesRefigure(EdgeList *es, Edge *active,double i);
extern Edge *ActiveEdgesFindStem(Edge *apt, Edge **prev, double i);
#endif
