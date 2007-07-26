/* Copyright (C) 2000-2007 by George Williams & Michal Nowakowski */
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
#include <math.h>
#include <utype.h>

#include "ttf.h"
#include "splinefont.h"

/* define some often used instructions */
#define SVTCA_y                 (0x00)
#define SVTCA_x                 (0x01)
#define SRP0                    (0x10)
#define SRP1                    (0x11)
#define SRP2                    (0x12)
#define SZP0                    (0x13)
#define RTG                     (0x18)
#define MDAP                    (0x2e)
#define MDAP_rnd                (0x2f)
#define IUP_y                   (0x30)
#define IUP_x                   (0x31)
#define SHP_rp2                 (0x32)
#define SHP_rp1                 (0x33)
#define MIAP                    (0x3e)
#define MIAP_rnd                (0x3f)
#define RDTG                    (0x7d)
#define MDRP_rnd_grey           (0xc4)
#define MDRP_min_rnd_black      (0xcd)
#define MDRP_min_rnd_white      (0xce)
#define MDRP_rp0_rnd_white      (0xd6)
#define MDRP_rp0_min_rnd_black  (0xdd)
#define MDRP_rp0_min_rnd_white  (0xde)
#define MIRP_min_rnd_black      (0xed)
#define MIRP_rp0_min_rnd_black  (0xfd)

struct glyphinstrs {
    SplineFont *sf;
    BlueData *bd;
    int fudge;
};

typedef struct instrct {
    SplineChar *sc;
    SplineSet *ss;
    struct glyphinstrs *gi;  /* finally, I'll bring its members here */
    int ptcnt;
    int *contourends;
    BasePoint *bp;
    uint8 *touched;
    uint8 *instrs;        /* the beginning of the instructions */
    uint8 *pt;            /* the current position in the instructions */

  /* stuff for hinting edges of stems and blues: */
    int xdir;             /* direction flag: x=true, y=false */
    int cdir;             /* is current contour clockwise (outer)? */
    struct __edge {
	real base;        /* where the edge is */
	real end;         /* the second edge - currently only for blue snapping */
	int refpt;        /* best ref. point for this base, ttf index, -1 if none */
	int refscore;     /* quality of basept, for searching better one, 0 if none */
	int othercnt;     /* count of other points worth instructing for this edge */
	int *others;      /* ttf indexes of these points */
    } edge;
} InstrCt;

extern int autohint_before_generate;

#if 0		/* in getttfinstrs.c */
struct ttf_table *SFFindTable(SplineFont *sf,uint32 tag) {
    struct ttf_table *tab;

    for ( tab=sf->ttf_tables; tab!=NULL && tab->tag!=tag; tab=tab->next );
return( tab );
}
#endif

/* We'll need at least three stack levels and a twilight point (and thus also
 * a twilight zone). We must ensure this is indicated in 'maxp' table.
 * Note: we'll surely need more stack levels in the future. Twilight point
 * count may vary depending on hinting method; for now, one is enough.
 *
 * TODO! I don't know why, but FF sometimes won't set the stack depth.
 * Either I or FF is perhaps using wrong offset.
 */
static void init_maxp(InstrCt *ct) {
    struct ttf_table *tab = SFFindTable(ct->sc->parent,CHR('m','a','x','p'));

    if ( tab==NULL ) {
	tab = chunkalloc(sizeof(struct ttf_table));
	tab->next = ct->sc->parent->ttf_tables;
	ct->sc->parent->ttf_tables = tab;
	tab->tag = CHR('m','a','x','p');
    }

    if ( tab->len<32 ) {
	tab->data = grealloc(tab->data,32);
	memset(tab->data+tab->len,0,32-tab->len);
	tab->len = tab->maxlen = 32;
    }

    uint16 zones = memushort(tab->data, 32,  7*sizeof(uint16));
    uint16 twpts = memushort(tab->data, 32,  8*sizeof(uint16));
    uint16 stack = memushort(tab->data, 32, 12*sizeof(uint16));

    if (zones<2) zones=2;
    /* if (twpts<ct->gi->bd->bluecnt) twpts=ct->gi->bd->bluecnt; */
    if (twpts<1) twpts=1;
    if (stack<3) stack=3; /* we'll surely need more in future */

    memputshort(tab->data, 7*sizeof(uint16), zones);
    memputshort(tab->data, 8*sizeof(uint16), twpts);
    memputshort(tab->data,12*sizeof(uint16), stack);
}

/* Turning dropout control on will dramatically improve mono rendering, even
 * without further hinting, especcialy for light typefaces. And turning hinting
 * off at veeery small pixel sizes is required, because hints tend to visually
 * tear outlines apart when not having enough workspace.
 */
static void init_prep(InstrCt *ct) {
    struct ttf_table *tab = SFFindTable(ct->sc->parent,CHR('p','r','e','p'));

    if ( tab==NULL || (tab->len==0) ) {
	tab = chunkalloc(sizeof(struct ttf_table));
	tab->next = ct->sc->parent->ttf_tables;
	ct->sc->parent->ttf_tables = tab;
	tab->tag = CHR('p','r','e','p');

	tab->len = tab->maxlen = 17;
	tab->data = grealloc(tab->data,17);

	tab->data[0]  = 0x4b;
	tab->data[1]  = 0xb0;
	tab->data[2]  = 0x05; /* hinting threshold - should be configurable */
	tab->data[3]  = 0x50;
	tab->data[4]  = 0x58;
	tab->data[5]  = 0xb1;
	tab->data[6]  = 0x01;
	tab->data[7]  = 0x01;
	tab->data[8]  = 0x8e;
	tab->data[9]  = 0x59;
	tab->data[10] = 0xb8;
	tab->data[11] = 0x01;
	tab->data[12] = 0xff;
	tab->data[13] = 0x85;
	tab->data[14] = 0xb0;
	tab->data[15] = 0x01;
	tab->data[16] = 0x1d;
    }
}

#if 0		/* in getttfinstrs.c */
int TTF__getcvtval(SplineFont *sf,int val) {
    int i;
    struct ttf_table *cvt_tab = SFFindTable(sf,CHR('c','v','t',' '));

    if ( cvt_tab==NULL ) {
	cvt_tab = chunkalloc(sizeof(struct ttf_table));
	cvt_tab->tag = CHR('c','v','t',' ');
	cvt_tab->maxlen = 200;
	cvt_tab->data = galloc(100*sizeof(short));
	cvt_tab->next = sf->ttf_tables;
	sf->ttf_tables = cvt_tab;
    }
    for ( i=0; sizeof(uint16)*i<cvt_tab->len; ++i ) {
	int tval = (int16) memushort(cvt_tab->data,cvt_tab->len, sizeof(uint16)*i);
	if ( val>=tval-1 && val<=tval+1 )
return( i );
    }
    if ( sizeof(uint16)*i>=cvt_tab->maxlen ) {
	if ( cvt_tab->maxlen==0 ) cvt_tab->maxlen = cvt_tab->len;
	cvt_tab->maxlen += 200;
	cvt_tab->data = grealloc(cvt_tab->data,cvt_tab->maxlen);
    }
    memputshort(cvt_tab->data,sizeof(uint16)*i,val);
    cvt_tab->len += sizeof(uint16);
return( i );
}

int TTF_getcvtval(SplineFont *sf,int val) {

    /* by default sign is unimportant in the cvt */
    /* For some instructions anyway, but not for MIAP so this routine has */
    /*  been broken in two. */
    if ( val<0 ) val = -val;
return( TTF__getcvtval(sf,val));
}
#endif

static int _CVT_SeekInPrivateString(SplineFont *sf, char *str, double value, double fudge) {
    char *end;
    double d;

    if ( str==NULL )
return -1;
    while ( *str ) {
	while ( !isdigit(*str) && *str!='-' && *str!='+' && *str!='.' && *str!='\0' )
	    ++str;
	if ( *str=='\0' )
    break;
	d = strtod(str,&end);
	if ( d>=-32768 && d<=32767 ) {
	    int v = rint(d);

	    if (fabs(d - value) <= fudge)
		return TTF__getcvtval(sf,v);
	}
	str = end;
    }

    return -1;
}

#if 0		/* in getttfinstrs.c */
static void _CVT_ImportPrivateString(SplineFont *sf,char *str) {
    char *end;
    double d;

    if ( str==NULL )
return;
    while ( *str ) {
	while ( !isdigit(*str) && *str!='-' && *str!='+' && *str!='.' && *str!='\0' )
	    ++str;
	if ( *str=='\0' )
    break;
	d = strtod(str,&end);
	if ( d>=-32768 && d<=32767 ) {
	    int v = rint(d);
	    TTF__getcvtval(sf,v);
	}
	str = end;
    }
}

void CVT_ImportPrivate(SplineFont *sf) {
    if ( sf->private==NULL )
return;
    _CVT_ImportPrivateString(sf,PSDictHasEntry(sf->private,"StdHW"));
    _CVT_ImportPrivateString(sf,PSDictHasEntry(sf->private,"StdVW"));
    _CVT_ImportPrivateString(sf,PSDictHasEntry(sf->private,"StemSnapH"));
    _CVT_ImportPrivateString(sf,PSDictHasEntry(sf->private,"StemSnapV"));
    _CVT_ImportPrivateString(sf,PSDictHasEntry(sf->private,"BlueValues"));
    _CVT_ImportPrivateString(sf,PSDictHasEntry(sf->private,"OtherBlues"));
    _CVT_ImportPrivateString(sf,PSDictHasEntry(sf->private,"FamilyBlues"));
    _CVT_ImportPrivateString(sf,PSDictHasEntry(sf->private,"FamilyOtherBlues"));
}
#endif

static uint8 *pushheader(uint8 *instrs, int isword, int tot) {
    if ( isword ) {
	if ( tot>8 ) {
	    *instrs++ = 0x41;		/* N(next byte) Push words */
	    *instrs++ = tot;
	} else
	    *instrs++ = 0xb8+(tot-1);	/* Push Words */
    } else {
	if ( tot>8 ) {
	    *instrs++ = 0x40;		/* N(next byte) Push bytes */
	    *instrs++ = tot;
	} else
	    *instrs++ = 0xb0+(tot-1);	/* Push bytes */
    }
return( instrs );
}

static uint8 *addpoint(uint8 *instrs,int isword,int pt) {
    if ( !isword ) {
	*instrs++ = pt;
    } else {
	*instrs++ = pt>>8;
	*instrs++ = pt&0xff;
    }
return( instrs );
}

static uint8 *pushpoint(uint8 *instrs,int pt) {
    instrs = pushheader(instrs,pt>255,1);
return( addpoint(instrs,pt>255,pt));
}

static uint8 *pushpointstem(uint8 *instrs,int pt, int stem) {
    int isword = pt>255 || stem>255;
    instrs = pushheader(instrs,isword,2);
    instrs = addpoint(instrs,isword,pt);
return( addpoint(instrs,isword,stem));
}

/* Find previous point index on the contour. */
static int PrevOnContour(int *contourends, BasePoint *bp, int p) {
    int i;

    if (p == 0) return contourends[0];
    else {
	for (i=0; contourends[i+1]; i++)
	    if (contourends[i]+1 == p)
		return contourends[i+1];

	return p-1;
    }
}

/* Find next point index on the contour. */
static int NextOnContour(int *contourends, BasePoint *bp, int p) {
    int i;

    if (p == 0) return 1;
    else {
	for (i=0; contourends[i]; i++) {
	    if (contourends[i] == p) {
		if (i==0) return 0;
		else return contourends[i-1]+1;
	    }
	}
	return p+1;
    }
}

/* For hinting stems, I found it needed to check if candidate point for
 * instructing is pararell with a hint to avoid snapping wrong points.
 * I splitted the routine into two, as sometimes it may be needed to check
 * the angle to be strictly almost the same, not just pararell.
 */
static int __same_angle(int *contourends, BasePoint *bp, int p, double angle) {
    int PrevPoint, NextPoint;
    double PrevTangent, NextTangent;

    PrevPoint = PrevOnContour(contourends, bp, p);
    NextPoint = NextOnContour(contourends, bp, p);

    PrevTangent = atan2(bp[p].y - bp[PrevPoint].y, bp[p].x - bp[PrevPoint].x);
    NextTangent = atan2(bp[NextPoint].y - bp[p].y, bp[NextPoint].x - bp[p].x);

    /* If at least one of the tangents is close to the given angle, return true. */
    /* 'Close' means about 10 deg, i.e. about 0.174 rad. */
    PrevTangent = fabs(PrevTangent-angle);
    NextTangent = fabs(NextTangent-angle);
    while (PrevTangent > M_PI) PrevTangent -= 2*M_PI;
    while (NextTangent > M_PI) NextTangent -= 2*M_PI;
return (fabs(PrevTangent) <= 0.174) || (fabs(NextTangent) <= 0.174);
}

static int same_angle(int *contourends, BasePoint *bp, int p, double angle) {
return __same_angle(contourends, bp, p, angle) || __same_angle(contourends, bp, p, angle+M_PI);
}

/* I found it needed to write some simple functions to determine whether
 * a spline point is a vertical/horizontal extremum. I looked at the code
 * responsible for displaying a charview and found that FF also treats an
 * inflection point of a curve as an extremum. I'm not sure if this is OK,
 * but I retain this for consistency.
 */
static int IsXExtremum(SplinePoint *sp) {
return (!sp->nonextcp && !sp->noprevcp && sp->nextcp.x==sp->me.x && sp->prevcp.x==sp->me.x);
}

static int IsYExtremum(SplinePoint *sp) {
return (!sp->nonextcp && !sp->noprevcp && sp->nextcp.y==sp->me.y && sp->prevcp.y==sp->me.y);
}

/* Other interesting points are corners. I'll name two types of them.
 * An acute corner is one with angle between its tangents less than 90 deg.
 * I won't distinct X and Y tight corners, as they usually need hinting in
 * both directions.
 * An obtuse corner is one with angle between its tangents close to 180 deg.
 * I'll distinct X and Y flat corners, as they usually need hinting only
 * in one direction.
 */
static int IsAcuteCorner(int *contourends, BasePoint *bp, int p) {
return 0;
}

static int IsXObtuseCorner(int *contourends, BasePoint *bp, int p) {
    int PrevPoint = PrevOnContour(contourends, bp, p);
    int NextPoint = NextOnContour(contourends, bp, p);

return ((bp[PrevPoint].x > bp[p].x) && (bp[NextPoint].x > bp[p].x)) ||
       ((bp[PrevPoint].x < bp[p].x) && (bp[NextPoint].x < bp[p].x));
}

static int IsYObtuseCorner(int *contourends, BasePoint *bp, int p) {
    int PrevPoint = PrevOnContour(contourends, bp, p);
    int NextPoint = NextOnContour(contourends, bp, p);

return ((bp[PrevPoint].y > bp[p].y) && (bp[NextPoint].y > bp[p].y)) ||
       ((bp[PrevPoint].y < bp[p].y) && (bp[NextPoint].y < bp[p].y));
}

/* I found it easier to write an iterator that calls given function for each
 * point worth instructing than repeating the same loops all the time. Each
 * function passed to the iterator may need different set of arguments. So I
 * decided to make a container for them to leave argument lists reasonably short.
 * The control points are normally skipped, as instructing them seems to cause
 * more damages than profits. An exception is made for interpolated extrema:
 * as they can't be directly instructed, their control points are used.
 * The iterator won't run a function twice on the same point in any case.
 */
static void RunOnPoints(InstrCt *ct, void (*runme)(int p, SplinePoint *sp, InstrCt *ct)) {
    SplineSet *ss = ct->ss;
    SplinePoint *sp;
    uint8 *done;
    int p;

    done = (uint8 *)gcalloc(ct->ptcnt, sizeof(uint8));

    for ( ; ss!=NULL; ss=ss->next ) {
	ct->cdir = SplinePointListIsClockwise(ss);
	for ( sp=ss->first; ; ) {
	    if (sp->ttfindex == 0xffff) {
		if (ct->xdir?IsXExtremum(sp):IsYExtremum(sp)) {
		    if (!done[p = PrevOnContour(ct->contourends, ct->bp, sp->nextcpindex)]) {
			runme(p, sp, ct);
			done[p] = true;
		    }
		    if (!done[p = sp->nextcpindex]) {
			runme(p, sp, ct);
			done[p] = true;
		    }
		}
	    }
	    else if (!done[p = sp->ttfindex]) {
		runme(p, sp, ct);
		done[p] = true;
	    }

	    if ( sp->next==NULL ) break;
	    sp = sp->next->to;
	    if ( sp==ss->first ) break;
	}
    }
    
    free(done);
}

/* I decided to do snapping to blues at the very beginning of the instructing.
 * For each blue, one of the edges is put into CVT: lower if is't > zero,
 * the upper otherwise. A twilight point 0 is established at this height. All
 * the glyph's points decided to be worth snapping are then moved relative to
 * this twilight point, being subject to rounding 'down-to-int'. Space taken
 * is at most 6+8bluecnt+4ptcnt bytes.

 * Important notes:

 * The zone count must be set to 2, the twilight point count must be nonzero.
 * This is done automagically, otherwise this method wouldn't work at all.
 * See init_maxp. Currently there is only one twilight point used, but there
 * may be needed one or even two points per blue zone if some advanced snapping
 * and counter managing is to be done.

 * The inner (leftwards) contours aren't snapped to the blue zone.
 * This could have created weird artifacts. Of course this will fail for
 * glyphs with wrong direction, but I won't handle it for now.
 
 * TODO! Remind the user to correct direction or do it for him.

 * TODO! Try to do this with a single push and looped MDRP.

 * If we didn't snapped any point to a blue zone, we shouldn't mark any HStem
 * edges done. This could made some important points on inner contours missed.

 * Note! We mark as 'done' all edges that are inside the blue zone.
 * If a HStem is entirely within a blue zone, its width WON'T BE USED.
 * So if HStem's edges are within different blue zones (I saw this used
 * for hinting serifs).
 */
static void snap_to_bluezone(int p, SplinePoint *sp, InstrCt *ct) {
    real coord = ct->bp[p].y;
    real fudge = 0; /* This should be BlueFuzz from PS Private Dictionary */

    if ((ct->cdir) &&
        (coord >= ct->edge.base-fudge) && (coord <= ct->edge.end+fudge) &&
	( same_angle(ct->contourends, ct->bp, p, 0.0) || IsYExtremum(sp) ||
	  IsYObtuseCorner(ct->contourends, ct->bp, p))
       )
    {
	ct->pt = pushpoint(ct->pt, p);
	*(ct->pt)++ = MDRP_rnd_grey;
	ct->touched[p] |= tf_y;

#if 0 
	/* a visual signal of snapping, for debugging purposes only */
	sp->selected = true;
#endif
    }
}

static void snap_to_blues(InstrCt *ct) {
    int i, cvt;
    StemInfo *hint;
    uint8 *backup, *current;

    /* I will use twilight zone */
    ct->pt = pushpoint(ct->pt, 0); /* misnomer, it's a zone number (twilight) */
    *(ct->pt)++ = SZP0;

    for (i=0; i<(ct->gi->bd->bluecnt); i++) {
	/* do a backup in case there are no points in this blue zone */
	backup = ct->pt;

	/* decide the cvt index */
	if (ct->gi->bd->blues[i][0] > 0) cvt = rint(ct->gi->bd->blues[i][0]);
	else cvt = rint(ct->gi->bd->blues[i][1]);
	cvt = TTF__getcvtval(ct->gi->sf,cvt);

	/* place the twilight point and set rounding down to int */
	ct->pt = pushpointstem(ct->pt, 0, cvt);
	*(ct->pt)++ = MIAP_rnd; //create and round it
	*(ct->pt)++ = RDTG;

	/* instruct all the desired points in this blue zone */
	current = ct->pt;
	ct->edge.base = ct->gi->bd->blues[i][0];
	ct->edge.end = ct->gi->bd->blues[i][1];
	RunOnPoints(ct, &snap_to_bluezone);

	/* Any points instructed? */
	if (ct->pt == current) ct->pt = backup;
	else {
	    *(ct->pt)++ = RTG; /* restore default round state */

	    /* Signalize that some HStem edges are now done */
	    for ( hint=ct->sc->hstem; hint!=NULL; hint=hint->next ) {
		if ((hint->start + ct->gi->fudge > ct->gi->bd->blues[i][0]) &&
		    (hint->start - ct->gi->fudge < ct->gi->bd->blues[i][1]))
		    hint->startdone = true;

		if ((hint->start + hint->width + ct->gi->fudge > ct->gi->bd->blues[i][0]) &&
		    (hint->start + hint->width - ct->gi->fudge < ct->gi->bd->blues[i][1]))
		    hint->enddone = true;
	    }
	}
    }

    /* Now get back to the glyph zone */
    ct->pt = pushpoint(ct->pt, 1); /* misnomer, it's a zone number (glyph) */
    *(ct->pt)++ = SZP0;
}

/* Each stem hint consists of two edges. Hinting each edge is broken in two
 * steps. First: init_edge() seeks for points to snap and chooses one as a
 * reference point - see search_edge(). It should be then instructed elsewhere
 * (a general method of edge positioning). Finally, finish_edge() instructs the
 * rest of points found with given command: SHP, SHPIX, IP, FLIPPT or ALIGNRP
 * (in future the function may use SLOOP to minimize the size of generated ttf
 * code, so other opcodes mustn't be used).
 */

/* search for points to be snapped to an edge - to be used in RunOnPoints() */
static void search_edge(int p, SplinePoint *sp, InstrCt *ct) {
    int tmp, score = 1;
    real coord = ct->xdir?ct->bp[p].x:ct->bp[p].y;
    real fudge = ct->gi->fudge;
    uint8 touchflag = ct->xdir?tf_x:tf_y;

    if ((fabs(coord - ct->edge.base) <= fudge) &&
	same_angle(ct->contourends, ct->bp, p, ct->xdir?0.5*M_PI:0.0)) {

	if (ct->touched[p]) score += 4;
	if (ct->xdir?IsXExtremum(sp):IsYExtremum(sp)) {
	    score+=2;
	    if (sp->ttfindex==p) score++;
	}
	else if (IsAcuteCorner(ct->contourends, ct->bp, p) || 
	  ct->xdir?IsXObtuseCorner(ct->contourends, ct->bp, p):
	  IsYObtuseCorner(ct->contourends, ct->bp, p)
	) score++;

	if (score > ct->edge.refscore) {
	    tmp = ct->edge.refpt;
	    ct->edge.refpt = p;
	    ct->edge.refscore = score;
	    p = tmp;
	}

	if ((p!=-1) && !(ct->touched[p] & touchflag)) {
	    ct->edge.othercnt++;

	    if (ct->edge.othercnt==1) ct->edge.others=(int *)gcalloc(1, sizeof(int));
	    else ct->edge.others=(int *)grealloc(ct->edge.others, ct->edge.othercnt*sizeof(int));

	    ct->edge.others[ct->edge.othercnt-1] = p;
	}
    }
}

/* Initialize the InstrCt for instructing given edge.
 * We try to hint only those points on edge that are necessary.
 * IUP will do the rest.
 */

static int sortbynum(const void *a, const void *b) { return *(int *)a > *(int *)b; }

static void init_edge(InstrCt *ct, real base) {
    ct->edge.base = base;
    ct->edge.refpt = -1;
    ct->edge.refscore = 0;
    ct->edge.othercnt = 0;
    ct->edge.others = NULL;

    RunOnPoints(ct, &search_edge);

    if (ct->edge.othercnt == 0)
return;

    int i, next, leading_unneeded=0, final_unneeded=0;
    int *others = ct->edge.others;
    int othercnt = ct->edge.othercnt;
    int refpt = ct->edge.refpt;

    /* now remove unneeded points */

#define NextP(_p) NextOnContour(ct->contourends, ct->bp, _p)

    qsort(others, othercnt, sizeof(int), sortbynum);

    if (((others[0] < refpt) && (NextP(others[0]) == others[othercnt-1])) ||
        (NextP(refpt) == others[0]))
	leading_unneeded = 1;

    if (NextP(others[othercnt-1]) == refpt) final_unneeded = 1;

    for (i=0; (i < othercnt-1) && (others[i] < refpt); i++) {
	next = NextP(others[i]);
	if ((next == others[i+1]) || (next == refpt)) others[i] = -1;
    }

    for (i=othercnt-1; (i > 0) && (others[i] > refpt); i--)
	if ((NextP(others[i-1]) == others[i]) || (NextP(refpt) == others[i]))
	    others[i] = -1;

    if (leading_unneeded) {
	for(i=0; (i < othercnt) && (others[i] == -1); i++);
	if (i < othercnt) others[i] = -1;
    }

    if (final_unneeded) {
	for(i=othercnt-1; (i >= 0) && (others[i] == -1); i--);
	if (i >= 0) others[i] = -1; 
    }

    for (i=next=0; i<othercnt; i++)
	if (others[i] != -1)
	    others[next++] = others[i];
    
    ct->edge.othercnt = next;

    if (ct->edge.othercnt == 0) {
	free(ct->edge.others);
	ct->edge.others = NULL;
    }

#undef NextP
}

/* Finish instructing the edge. The 'command' must be
 * either of: SHP, SHPIX, IP, FLIPPT or ALIGNRP.

 * TODO! 
 * Possible strategies:
 *   - do point by point (currently used, poor space efficiency)
 *   - push all the stock at once (or bytes and words separately)
 *   - when the stuff is pushed, try to use SLOOP or relative jumps.
 */

static void finish_edge(InstrCt *ct, uint8 command) {

    if (ct->edge.othercnt==0)
return;

    int i;
    int *others = ct->edge.others;

    for(i=0; i<(ct->edge.othercnt); i++) {
	ct->pt = pushpoint(ct->pt, others[i]);
	*(ct->pt)++ = command;
	ct->touched[others[i]] |= (ct->xdir?tf_x:tf_y);
    }

    free(ct->edge.others);
    ct->edge.others=NULL;
    ct->edge.othercnt = 0;
}

/*
 Find points that should be snapped to this hint's edges.
 The searching routine will return two types of points per edge:
 a 'chosen one' that should be used as a reference for this hint, and
 'others' to position after him with SHP[rp2].

 If the hint's width is in the PS dictionary, or at least it is close within
 fudge margin to one of the values, place the value in the cvt and use MIRP.
 Otherwise don't pollute the cvt and use MDRP. In both cases, the distance
 should be black, rounded and kept not under reasonable minimum.

 If one of the edges is already positioned, set a RP0 there, then MIRP or
 MDRP a reference point at the second edge, setting it as rp0 if this is
 the end vertical edge, and do its others.

 If none of the edges is positioned:
   If this hint is the first, previously overlapped, or simply horizontal,
   position the reference point at the base where it is using MDAP; otherwise
   position the hint's base rp0 relatively to the previous hint's end using
   MDRP with white minimum distance. Then do its others; MIRP or MDRP rp0
   at the hint's end and do its others.

 Mark startdones and enddones.
*/

static void geninstrs(InstrCt *ct, StemInfo *hint) {
    real hbase, base, width, hend;
    StemInfo *h;
    real fudge = ct->gi->fudge;
    StemInfo *firsthint, *lasthint=NULL, *testhint;
    int first;
    int cvtindex=-1;

    /* if this hint has conflicts don't try to establish a minimum distance */
    /* between it and the last stem, there might not be one */        
    if (ct->xdir) firsthint = ct->sc->vstem;
    else firsthint = ct->sc->hstem;
    for ( testhint = firsthint; testhint!=NULL && testhint!=hint; testhint = testhint->next ) {
	if ( HIoverlap(testhint->where,hint->where)!=0 )
	    lasthint = testhint;
    }
    first = lasthint==NULL;
    if ( hint->hasconflicts ) first = true;		

    /* Check whether to use CVT value or shift the stuff directly */
    hbase = base = rint(hint->start);
    width = rint(hint->width);
    hend = hbase + width;

    if (ct->xdir) cvtindex = _CVT_SeekInPrivateString(ct->gi->sf, PSDictHasEntry(ct->gi->sf->private, "StemSnapV"), width, fudge);
    else cvtindex = _CVT_SeekInPrivateString(ct->gi->sf, PSDictHasEntry(ct->gi->sf->private, "StemSnapH"), width, fudge);

    /* flip the hint if needed */
    if (hint->enddone) {
	hbase = (base + width);
	width = -width;
	hend = base;
    }

    if (hint->startdone || hint->enddone) {
	/* Set a reference point on the base edge.
	 * Ghost hints can get skipped, that's a bug.
	 */
	init_edge(ct, hbase);
	if (ct->edge.refpt == -1) goto done;
	ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	*ct->pt++ = MDAP; /* sets rp0 and rp1 */

	/* Still unhinted points on that edge? */
	if (ct->edge.othercnt != 0) finish_edge(ct, SHP_rp1);

	/* set a reference point on the other edge */
	init_edge(ct, hend);
	if (ct->edge.refpt == -1) goto done;

	if (cvtindex == -1) {
	    ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	    *ct->pt++ = MDRP_min_rnd_black;
	}
	else {
	    ct->pt = pushpointstem(ct->pt, ct->edge.refpt, cvtindex);
	    *ct->pt++ = MIRP_min_rnd_black;
	}

	ct->touched[ct->edge.refpt] |= (ct->xdir?tf_x:tf_y);

	/* finish the edge and prepare for the next stem hint */
	finish_edge(ct, SHP_rp2);

	if (hint->startdone) {
	    ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	    *ct->pt++ = SRP0;
	}
    }
    else {
	init_edge(ct, hbase);
	if (ct->edge.refpt == -1) goto done;

        /* Now I must place the stem's origin in respect to others... */
	/* The steps here are extremely simple and often do not work. */
	/* What's really needed here is an iterative procedure that would */
	/* preserve counters and widths, like in freetype2. */
	/* For horizontal stems, interpolating between blues MUST be done. */
	/* rp0 and rp1 are set to refpt after its positioning */
	if (!ct->xdir || first) {
	    ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	    *ct->pt++ = MDAP_rnd;
	}
	else {
	  ct->pt = pushpointstem(ct->pt, ct->edge.refpt, ct->edge.refpt);
	  *ct->pt++ = MDRP_rp0_min_rnd_white;
	  *ct->pt++ = MDAP;
	}

	ct->touched[ct->edge.refpt] |= (ct->xdir?tf_x:tf_y);
	finish_edge(ct, SHP_rp1);

	/* Start the second edge */
	init_edge(ct, hend);
	if (ct->edge.refpt == -1) goto done;

	if (cvtindex == -1) {
	  ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	  *ct->pt++ = MDRP_rp0_min_rnd_black;
	}
	else {
	    ct->pt = pushpointstem(ct->pt, ct->edge.refpt, cvtindex);
	    *ct->pt++ = MIRP_rp0_min_rnd_black;
	}

	ct->touched[ct->edge.refpt] |= (ct->xdir?tf_x:tf_y);
	finish_edge(ct, SHP_rp2);
    }

    done:

    for ( h=hint->next; h!=NULL && h->start<=hint->start+hint->width; h=h->next ) {
	if ( (h->start>=hint->start-ct->gi->fudge && h->start<=hint->start+ct->gi->fudge) ||
		(h->start>=hint->start+hint->width-ct->gi->fudge && h->start<=hint->start+hint->width+ct->gi->fudge) )
	    h->startdone = true;
	if ( (h->start+h->width>=hint->start-ct->gi->fudge && h->start+h->width<=hint->start+ct->gi->fudge) ||
		(h->start+h->width>=hint->start+hint->width-ct->gi->fudge && h->start+h->width<=hint->start+hint->width+ct->gi->fudge) )
	    h->enddone = true;
    }
}

/* diagonal stem hints */
/* Several DStemInfo hints may actually be colinear. This structure contains all on the line */
typedef struct dstem {
    struct dstem *next;
    BasePoint leftedgetop, leftedgebottom, rightedgetop, rightedgebottom;
    int pnum[4];
    /*struct dsteminfolist { struct dsteminfolist *next; DStemInfo *d; int pnum[4];} *dl;*/
    uint8 *used;	/* Points lying near this diagonal 1=>left edge, 2=> right, 0=> off */
    struct dstemlist { struct dstemlist *next; struct dstem *ds; BasePoint *is[4]; int pnum[4]; int done;} *intersects;
    struct dstemlist *top, *bottom;
    unsigned int done: 1;
} DStem;
enum intersect { in_ll, in_lr, in_rl, in_rr };	/* intersection of: two left edges, left/right, right/left, right/right */

static int MapSP2Index(SplineSet *ttfss,SplinePoint *csp, int ptcnt) {
    SplineSet *ss;
    SplinePoint *sp;

    if ( csp==NULL )
return( ptcnt+1 );		/* ptcnt+1 is the phantom point for the width */
    for ( ss=ttfss; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first;; ) {
	    if ( sp->me.x==csp->me.x && sp->me.y==csp->me.y )
return( sp->ttfindex );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
return( -1 );
}

/* Order of the md hints is important hence the double loop. */
/* We only do a hint if one edge has been fixed (or if we have no choice) */
static uint8 *gen_md_instrs(struct glyphinstrs *gi, uint8 *instrs,MinimumDistance *_md,
	SplineSet *ttfss, BasePoint *bp, int ptcnt, int xdir, uint8 *touched) {
    int mask = xdir ? 1 : 2;
    int pt1, pt2;
    int any, graspatstraws=false, undone;
    MinimumDistance *md;

    do {
	any = false; undone = false;
	for ( md=_md; md!=NULL; md=md->next ) {
	    if ( md->x==xdir && !md->done ) {
		pt1 = MapSP2Index(ttfss,md->sp1,ptcnt);
		pt2 = MapSP2Index(ttfss,md->sp2,ptcnt);
		if ( pt1==ptcnt+1 ) {
		    pt1 = pt2;
		    pt2 = ptcnt+1;
		}
		if ( pt1==0xffff || pt2==0xffff )
		    fprintf(stderr, "Internal Error: Failed to find point in minimum distance check\n" );
		else if ( pt1!=ptcnt+1 && (touched[pt1]&mask) &&
			pt2!=ptcnt+1 && (touched[pt2]&mask) )
		    md->done = true;	/* somebody else did it, might not be right for us, but... */
		else if ( !graspatstraws &&
			!(touched[pt1]&mask) &&
			 (pt2==ptcnt+1 || !(touched[pt2]&mask)) )
		     /* If neither edge has been touched, then don't process */
		     /*  it now. hope that by filling in some other mds we will*/
		     /*  establish one edge or the other, and then come back to*/
		     /*  it */
		    undone = true;
		else if ( pt2==ptcnt+1 || !(touched[pt2]&mask)) {
		    md->done = true;
		    instrs = pushpointstem(instrs,pt2,pt1);	/* Misnomer, I'm pushing two points */
		    if ( !(touched[pt1]&mask))
			*instrs++ = 0x2f;			/* MDAP[rnd] */
		    else
			*instrs++ = 0x10;			/* SRP0 */
		    *instrs++ = 0xcc;			/* MDRP[01100] min, rnd, grey */
		    touched[pt1] |= mask;
		    if ( pt2!=ptcnt+1 )
			touched[pt2]|= mask;
		    any = true;
		} else {
		    md->done = true;
		    instrs = pushpointstem(instrs,pt1,pt2);	/* Misnomer, I'm pushing two points */
		    *instrs++ = 0x10;			/* SRP0 */
		    *instrs++ = 0xcc;			/* MDRP[01100] min, rnd, grey */
		    touched[pt1] |= mask;
		    any = true;
		}
	    }
	}
	graspatstraws = undone && !any;
    } while ( undone );
return(instrs);
}


/* Rounding extremae to grid is generally a good idea, so I do this by default.
 * Yet it is possible that rounding an extremum is wrong, and that some other
 * points might also be rounded. There once was a 'round to x/y grid' flag for
 * each point with an UI to set it. It's gone now, but could probably be helpful
 * in future.
 */
 
/* This is supposed to be called within RunOnPoints().
 */
static void do_extrema(int ttfindex, SplinePoint *sp, InstrCt *ct) {
    if (!(ct->touched[ttfindex] & (ct->xdir?tf_x:tf_y)) &&
	 (ct->xdir?IsXExtremum(sp):IsYExtremum(sp)))
    {
	ct->pt = pushpoint(ct->pt,ttfindex);
	*(ct->pt)++ = MDAP_rnd;
	ct->touched[ttfindex] |= ct->xdir?tf_x:tf_y;
    }
}

/* And this is supposed to be ported so it can be called RunOnPoints().
 */
static uint8 *gen_rnd_instrs(struct glyphinstrs *gi, uint8 *instrs,SplineSet *ttfss,
	BasePoint *bp, int ptcnt, int xdir, uint8 *touched) {
    int mask = xdir ? 1 : 2;
    SplineSet *ss;
    SplinePoint *sp;

    for ( ss=ttfss; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ((( sp->roundx && xdir ) || ( sp->roundy && !xdir )) &&
		    !(touched[sp->ttfindex]&mask)) {
		instrs = pushpoint(instrs,sp->ttfindex);
		*instrs++ = 0x2f;		/* MDAP[rnd] */
		touched[sp->ttfindex] |= mask;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp == ss->first )
	break;
	}
    }
return( instrs );
}

static uint8 *dogeninstructions(InstrCt *ct) {
    StemInfo *hint;
    int max;
    uint8 *backup;
    DStemInfo *dstem;

    /* very basic preparations for default hinting */
    /* TODO! Move this to the end so that we have knowledge how deep stack do we need. */
    init_maxp(ct);
    init_prep(ct);

    /* Maximum instruction length is 6 bytes for each point in each dimension */
    /*  2 extra bytes to finish up. And one byte to switch from x to y axis */
    /* Diagonal take more space because we need to set the orientation on */
    /*  each stem, and worry about intersections, etc. */
    /*  That should be an over-estimate */
    max=2;
    if ( ct->sc->vstem!=NULL ) max += ct->ptcnt*6;
    if ( ct->sc->hstem!=NULL ) max += ct->ptcnt*6+1;
    for ( dstem=ct->sc->dstem; dstem!=NULL; max+=7+4*6+100, dstem=dstem->next );
    if ( ct->sc->md!=NULL ) max += ct->ptcnt*12;
    max += ct->ptcnt*6;			/* in case there are any rounds */
    max += ct->ptcnt*6;			/* paranoia */
    ct->instrs = ct->pt = galloc(max);

    /* Initially no stem hints are done */
    for ( hint=ct->sc->vstem; hint!=NULL; hint=hint->next )
	hint->enddone = hint->startdone = false;
    for ( hint=ct->sc->hstem; hint!=NULL; hint=hint->next )
	hint->enddone = hint->startdone = false;

    /* We start from instructing horizontal features (=> movement in y) */
    /*  Do this first so that the diagonal hinter will have everything moved */
    /*  properly when it sets the projection vector */
    /*  Even if we aren't doing the diagonals, we do the blues. */
    ct->xdir = false;
    *(ct->pt)++ = SVTCA_y;

    /* First of all, snap key points to the blue zones. This gives consistent */
    /* glyph heights and usually improves the text look very much. The blues */
    /* will be placed in CVT automagically, but the user may want to tweak'em */
    /* in PREP on his own. */
    snap_to_blues(ct);

    /* instruct horizontal stems (=> movement in y) */
    ct->pt = pushpoint(ct->pt, ct->ptcnt);
    *(ct->pt)++ = SRP0;
    for ( hint=ct->sc->hstem; hint!=NULL; hint=hint->next )
	if ( !hint->startdone || !hint->enddone )
	    geninstrs(ct,hint);

    /* TODO! Instruct top and bottom bearings for fonts which have them. */

    /* Extremae and others should take shifts by stems into account. */
    *(ct->pt)++ = IUP_y;
    backup = ct->pt;
    RunOnPoints(ct, &do_extrema);
    if (backup != ct->pt) *(ct->pt)++ = IUP_y;
//    ct->pt = gen_rnd_instrs(ct->gi,ct->pt,ct->ss,ct->bp,ct->ptcnt,false,ct->touched);
//    ct->pt = gen_md_instrs(ct->gi,ct->pt,ct->sc->md,ct->ss,ct->bp,ct->ptcnt,false,ct->touched);

    /* next instruct vertical features (=> movement in x) */
    ct->xdir = true;
    if ( ct->pt != ct->instrs ) *(ct->pt)++ = SVTCA_x;

    /* instruct vertical stems (=> movement in x) */
    ct->pt = pushpoint(ct->pt, ct->ptcnt);
    *(ct->pt)++ = SRP0;
    for ( hint=ct->sc->vstem; hint!=NULL; hint=hint->next )
	if ( !hint->startdone || !hint->enddone )
	    geninstrs(ct,hint);

    /* instruct right sidebearing */
    /* TODO! Solve the case of transformed references */
    if (ct->sc->width != 0) {
	ct->pt = pushpoint(ct->pt, ct->ptcnt+1);
	*(ct->pt)++ = MDRP_min_rnd_white;
    }

    /* Extremae and others should take shifts by stems into account. */
    *(ct->pt)++ = IUP_x;
    backup = ct->pt;
    RunOnPoints(ct, &do_extrema);
    if (backup != ct->pt) *(ct->pt)++ = IUP_x;
//    ct->pt = gen_rnd_instrs(ct->gi,ct->pt,ct->ss,ct->bp,ct->ptcnt,true,ct->touched);

    /* finally instruct diagonal stems (=> movement in x) */
    /*  This is done after vertical stems because it involves */
    /*  moving some points out-of their vertical stems. */
//    ct->pt = DStemInfoGeninst(ct->gi,ct->pt,ct->sc->dstem,ct->touched,ct->bp,ct->ptcnt);
//    ct->pt = gen_md_instrs(ct->gi,ct->pt,ct->sc->md,ct->ss,ct->bp,ct->ptcnt,true,ct->touched);

    /* Interpolate untouched points */
//    *(ct->pt)++ = IUP_y;
//    *(ct->pt)++ = IUP_x;

    if ((ct->pt)-(ct->instrs) > max) IError(
	"We're about to crash.\n"
	"We miscalculated the glyph's instruction set length\n"
	"When processing TTF instructions (hinting) of %s", ct->sc->name
    );

    ct->sc->ttf_instrs_len = (ct->pt)-(ct->instrs);
return ct->sc->ttf_instrs = grealloc(ct->instrs,(ct->pt)-(ct->instrs));
}

void NowakowskiSCAutoInstr(SplineChar *sc, BlueData *bd) {
    BlueData _bd;
    struct glyphinstrs gi;
    int cnt, contourcnt;
    BasePoint *bp;
    int *contourends;
    uint8 *touched;
    SplineSet *ss;
    RefChar *ref;
    InstrCt ct;

    if ( !sc->parent->order2 )
return;

    if ( sc->layers[ly_fore].refs!=NULL && sc->layers[ly_fore].splines!=NULL ) {
	gwwv_post_error(_("Can't instruct this glyph"),
		_("TrueType does not support mixed references and contours.\nIf you want instructions for %.30s you should either:\n * Unlink the reference(s)\n * Copy the inline contours into their own (unencoded\n    glyph) and make a reference to that."),
		sc->name );
return;
    }
    for ( ref = sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[0]>=2 || ref->transform[0]<-2 ||
		ref->transform[1]>=2 || ref->transform[1]<-2 ||
		ref->transform[2]>=2 || ref->transform[2]<-2 ||
		ref->transform[3]>=2 || ref->transform[3]<-2 )
    break;
    }
    if ( ref!=NULL ) {
	gwwv_post_error(_("Can't instruct this glyph"),
		_("TrueType does not support references which\nare scaled by more than 200%%.  But %1$.30s\nhas been in %2$.30s. Any instructions\nadded would be meaningless."),
		ref->sc->name, sc->name );
return;
    }

    if ( sc->ttf_instrs ) {
	free(sc->ttf_instrs);
	sc->ttf_instrs = NULL;
	sc->ttf_instrs_len = 0;
    }
    SCNumberPoints(sc);
    if ( autohint_before_generate && sc->changedsincelasthinted &&
	    !sc->manualhints )
	SplineCharAutoHint(sc,NULL);

    if ( sc->vstem==NULL && sc->hstem==NULL && sc->dstem==NULL && sc->md==NULL )
return;

    /* TODO!
     *
     * I'm having problems with references that are rotated or flipped
     * horizontally. Basically, such glyphs can get negative width. Such widths
     * are treated very differently under Freetype (OK) and Windows (terribly
     * shifted), and I suppose other rasterizers can also complain.
     */
    if ( sc->layers[ly_fore].splines==NULL )
return;

    if ( bd==NULL ) {
	QuickBlues(sc->parent,&_bd);
	bd = &_bd;
    }
    gi.sf = sc->parent;
    gi.bd = bd;
    gi.fudge = (sc->parent->ascent+sc->parent->descent)/500;

    contourcnt = 0;
    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next, ++contourcnt );
    cnt = SSTtfNumberPoints(sc->layers[ly_fore].splines);

    contourends = galloc((contourcnt+1)*sizeof(int));
    bp = galloc(cnt*sizeof(BasePoint));
    touched = gcalloc(cnt,1);
    contourcnt = cnt = 0;
    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	touched[cnt] |= tf_startcontour;
	cnt = SSAddPoints(ss,cnt,bp,NULL);
	touched[cnt-1] |= tf_endcontour;
	contourends[contourcnt++] = cnt-1;
    }
    contourends[contourcnt] = 0;

    ct.sc = sc;
    ct.ss = sc->layers[ly_fore].splines;
    ct.gi = &gi;
    ct.instrs = NULL;
    ct.pt = NULL;
    ct.ptcnt = cnt;
    ct.contourends = contourends;
    ct.bp = bp;
    ct.touched = touched;
    dogeninstructions(&ct);

    free(touched);
    free(bp);
    free(contourends);

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    SCMarkInstrDlgAsChanged(sc);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}
