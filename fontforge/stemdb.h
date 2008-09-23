/* Copyright (C) 2005-2008 by George Williams */
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
#ifndef _STEMDB_H_
# define _STEMDB_H_

# include "pfaedit.h"

typedef struct glyphdata {
    SplineFont *sf;
    BlueData bd;
    int fuzz;
    SplineChar *sc;
    int layer;
    int emsize;
    int order2;
    int has_slant;
    BasePoint slant_unit;
    int ccnt;
    int *contourends;
    int realcnt;			/* Includes control points, excludes implied points */
    int norefpcnt;			/* Does not include points in glyphs with references */
    int pcnt;				/* Includes control points, includes implied points */
    struct pointdata *points;		/* Entries corresponding to control points are empty */
    int stemcnt;
    struct stemdata *stems;
    int linecnt;
    struct linedata *lines;
    struct stembundle *hbundle;
    struct stembundle *vbundle;
    struct stembundle *ibundle;

    /* Temporary values, quickly freed */
    int mcnt;
    struct monotonic *ms;
    struct monotonic **space;
    int scnt;
    Spline **sspace;
    struct st *stspace;
    DBounds size;
    struct pointdata **pspace;
    struct segment { 
        double start, end, sbase, ebase; 
        int curved, scurved, ecurved; 
    } *lspace, *rspace, *bothspace, *activespace;

    int only_hv;
} GlyphData;

typedef struct pointdata {
    SplinePoint *sp;
    SplineSet *ss;
    int ttfindex;                               /* normally same as sp->ttfindex, but needed for offcurve points */
    BasePoint base;                             /* normally same as sp->me, but needed for offcurve points */
    BasePoint nextunit, prevunit;		/* unit vectors pointing in the next/prev directions */
    struct linedata *nextline, *prevline;	/* any other points lying on approximately the same line */
    Spline *nextedges[2], *prevedges[2];	/* There should always be a matching spline, which may end up as part of a stem, and may not */
    Spline *bothedge;
    double next_e_t[2], prev_e_t[2];	        /* Location on other edge where our normal hits it */
    double both_e_t;
    int next_e_cnt, prev_e_cnt;
    double next_dist[2], prev_dist[2];		/* Distance from the point to the matching edge */
    struct stemdata **nextstems, **prevstems;
    int *next_is_l, *prev_is_l;
    int nextcnt, prevcnt;
    double nextlen, prevlen;
    int value;                          /* Temporary value, used to compare points assigned to the same edge and determine which one can be used as a reference point*/
    unsigned int nextlinear: 1;
    unsigned int nextzero: 1;
    unsigned int prevlinear: 1;
    unsigned int prevzero: 1;
    unsigned int colinear: 1;
    unsigned int symetrical_h: 1;	/* Are next & prev symetrical? */
    unsigned int symetrical_v: 1;	/* Are next & prev symetrical? */
    unsigned int next_hor: 1;
    unsigned int next_ver: 1;
    unsigned int prev_hor: 1;
    unsigned int prev_ver: 1;
    unsigned int ticked: 1;
    uint8 touched, affected;
    uint8 x_extr, y_extr;
    uint8 x_corner, y_corner;
    BasePoint newpos;
    BasePoint newnext, newprev;
    BasePoint posdir;		/* If point has been positioned in 1 direction, this is that direction */
    double projection;		/* temporary value */
} PointData;

typedef struct linedata {
    BasePoint unit;
    BasePoint online;
    uint8 is_left;
    int pcnt;
    double length;
    struct pointdata **points;
} LineData;

typedef struct stemdata {
    BasePoint unit;		/* Unit vector pointing in direction of stem */
    BasePoint l_to_r;		/* Unit vector pointing from left to right (across stem) */
    BasePoint left;		/* a point on one side of the stem (not necissarily left, even for vertical stems) */
    BasePoint right;		/* and one on the other */
    BasePoint newunit;          /* Unit vector after repositioning (e. g. in Metafont routines) */
    BasePoint newleft, newright;/* Left and right edges after repositioning */
    int leftidx, rightidx;      /* TTF indices of the left and right key points */
    struct pointdata *keypts[4];/* Uppest and lowest points on left and right edges. Used for positioning diagonal stems */
    double lmin, lmax, rmin, rmax;
    double width;
    int chunk_cnt;		/* number of separate point-pairs on this stem */
    struct stem_chunk {
	struct stemdata *parent;
	struct pointdata *l;
	struct pointdata *r;
	uint8 lpotential, rpotential;
	uint8 lnext, rnext;	/* are we using the next/prev side of the left/right points */
	uint8 ltick, rtick;
	uint8 stub;
	uint8 stemcheat;	/* It's not a real stem, but it's something we'd like PostScript to hint for us */
        uint8 is_ball;          /* Specifies if this chunk marks the opposite sides of a ball terminal (useful for TTF instructions) */
        int l_e_idx, r_e_idx;   /* Which of the opposed edges assigned to the left and right points corresponds to this chunk */
    } *chunks;
    int activecnt;
    struct segment *active;
    uint8 toobig;		/* Stem is fatter than tall, unlikely to be a real stem */
    uint8 positioned;
    uint8 ticked;
    uint8 ghost;
    uint8 bbox;
    uint8 ldone, rdone;
    uint8 italic;
    int blue;			/* Blue zone a ghost hint is attached to */
    double len, clen;		/* Length of linear segments. clen adds "length" of curved bits */
    struct stembundle *bundle;
    int lpcnt, rpcnt;           /* Count of points assigned to left and right edges of this stem */
    struct linedata *leftline, *rightline;
    struct stemdata *master, *next_c_m, *prev_c_m;
    int confl_cnt;
    int dep_cnt;
    int serif_cnt;
    struct dependent_stem {
        struct stemdata *stem;
        uint8 lbase;
        char dep_type;          /* can be 'a' (align), 'i' (interpolate), 'm' (move) or 's' (serif) */
    } *dependent;               /* Lists other stems dependent from the given stem */
    struct dependent_serif {
        struct stemdata *stem;
        double width;           /* The distance from an edge of the main stem to the opposite edge of the serif stem */
        uint8 lbase;
        uint8 is_ball;
    } *serifs;                  /* Lists serifs and other elements protruding from the base stem */
} StemData;

typedef struct vchunk {
    struct stem_chunk *chunk;
    double dist;
    int parallel;
    int value;
} VChunk;

struct stembounds {
    struct stembounds *next;
    struct stemdata *stem;
    double tstart, tend;
    uint8 isr;
};

struct splinesteminfo {
    Spline *s;
    struct stembounds *sb;
};

typedef struct stembundle {
    BasePoint unit;		/* All these stems are parallel, pointing in unit direction */
    BasePoint l_to_r;		/* Axis along which these stems are ordered (normal to unit) */
    BasePoint bp;		/* Base point for measuring by l_to_r (stem->lpos,rpos) */
    int cnt;			/* Number of stems in the bundle */
    struct stemdata **stemlist;
} StemBundle;
    
extern struct glyphdata *GlyphDataBuild(SplineChar *sc, int layer, BlueData *bd, int use_existing);
extern struct glyphdata *GlyphDataInit(SplineChar *sc, int layer, double em_size, int only_hv);
extern struct glyphdata *StemInfoToStemData( struct glyphdata *gd,StemInfo *si,int is_v );
extern struct glyphdata *DStemInfoToStemData( struct glyphdata *gd,DStemInfo *dsi );
extern int IsStemAssignedToPoint( struct pointdata *pd,struct stemdata *stem,int is_next );
extern void GlyphDataFree(struct glyphdata *gd);

#endif		/* _STEMDB_H_ */
