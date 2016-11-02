/* Copyright (C) 2004-2012 by George Williams */
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
#ifndef _EDGELIST2_H
#define _EDGELIST2_H
#include "splinefont.h"

struct monotonic;

typedef struct mlist {
    Spline *s;
    struct monotonic *m;			/* May get slightly munched but will */
			/* always have right spline. we fix when we need it */
    extended t;
    int isend;
    BasePoint unit;
    struct mlist *next;
} MList;

typedef struct intersection {
    MList *monos;
    BasePoint inter;
    struct intersection *next;
} Intersection;
 
typedef struct preintersection {
    BasePoint inter;
    struct monotonic *m1; bigreal t1;
    struct monotonic *m2; bigreal t2;
    unsigned int is_close: 1;
    struct preintersection *next;
} PreIntersection;    

#define FF_RELATIONAL_GEOM

typedef struct monotonic {
    Spline *s;
    extended tstart, tend;
#ifdef FF_RELATIONAL_GEOM
    extended otstart, otend;
#endif
    struct monotonic *next, *prev;	/* along original contour */
    uint8 xup;				/* increasing t => increasing x */
    uint8 yup;
    unsigned int isneeded : 1;
    unsigned int isunneeded : 1;
    unsigned int mutual_collapse : 1;
    unsigned int exclude : 1;
    struct intersection *start;
    struct intersection *end;
    DBounds b;
    extended other, t;
    struct monotonic *linked;		/* singly linked list of all monotonic*/
    					/*  segments, no contour indication */
    double when_set;			/* Debugging */
    struct preintersection *pending;
} Monotonic;

#endif /* _EDGELIST2_H */
