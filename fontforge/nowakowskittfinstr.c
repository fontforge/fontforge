/* Copyright (C) 2000-2008 by
   George Williams, Michal Nowakowski & Alexey Kryukov */

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
#include "fontforgevw.h"
#include <math.h>
#include <utype.h>

#include "ttf.h"
#include "splinefont.h"
#include "stemdb.h"

extern int autohint_before_generate;

/* non-optimized instructions will be using a stack of depth 6, allowing
 * for easy testing whether the code leaves trash on the stack or not.
 */
#define OPTIMIZE_TTF_INSTRS 1
#if OPTIMIZE_TTF_INSTRS
#define STACK_DEPTH 256
#else
#define STACK_DEPTH 6
#endif

/* Experimental feature: tries to maintain relative position of some important
 * points between stems' edges, so that glyph's shape is mostly preserved
 * when strongly gridfitted. This in terms of FreeType is called 'Strong Point
 * Interpolation'. In FF it now does more or else what it should, but generates
 * large and sometimes incomplete code - it needs optimization. Please note that
 * it has also impact on diagonal hints' positioning - usually positive.
 */
#define TESTIPSTRONG 1

/* define some often used instructions */
#define SVTCA_y                 (0x00)
#define SVTCA_x                 (0x01)
#define SRP0                    (0x10)
#define SRP1                    (0x11)
#define SRP2                    (0x12)
#define SZP0                    (0x13)
#define SLOOP			(0x17)
#define RTG                     (0x18)
#define SMD			(0x1a)
#define DUP                     (0x20)
#define DEPTH                   (0x24)
#define CALL                    (0x2b)
#define MDAP                    (0x2e)
#define MDAP_rnd                (0x2f)
#define IUP_y                   (0x30)
#define IUP_x                   (0x31)
#define SHP_rp2                 (0x32)
#define SHP_rp1                 (0x33)
#define SHPIX                   (0x38)
#define IP			(0x39)
#define ALIGNRP                 (0x3c)
#define MIAP_rnd                (0x3f)
#define ADD                     (0x60)
#define MUL                     (0x63)
#define NEG                     (0x65)
#define SROUND                  (0x76)
#define FLIPPT                  (0x80)
#define MDRP_grey               (0xc0)
#define MDRP_min_white          (0xca)
#define MDRP_min_rnd_black      (0xcd)
#define MDRP_rp0_rnd_white      (0xd6)
#define MDRP_rp0_min_rnd_black  (0xdd)
#define MDRP_rp0_min_rnd_white  (0xde)
#define MIRP_min_black          (0xe9)
#define MIRP_min_rnd_black      (0xed)
#define MIRP_rp0_min_black      (0xf9)
#define MIRP_rp0_min_rnd_black  (0xfd)


/******************************************************************************
 *
 * Low-lewel routines to add data to bytecode instruction stream.
 *
 ******************************************************************************/

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

#define pushnum(a, b) pushpoint(a, b)

static uint8 *pushpointstem(uint8 *instrs, int pt, int stem) {
    int isword = pt>255 || stem>255;
    instrs = pushheader(instrs,isword,2);
    instrs = addpoint(instrs,isword,pt);
return( addpoint(instrs,isword,stem));
}

#define push2points(a, b, c) pushpointstem(a, b, c)
#define push2nums(a, b, c) pushpointstem(a, b, c)

/* Push a bunch of point numbers (or other numbers) ont the stack.
 * TODO!
 * Possible strategies:
 *   - push point by point (poor space efficiency)
 *   - push all the stock at once (currently used, better, but has
 *     poor space efficiency in case of a word among several bytes).
 *   - push bytes and words separately
 */
static uint8 *pushpoints(uint8 *instrs, int ptcnt, const int *pts) {
    int i, isword = 0;
    for (i=0; i<ptcnt; i++) if (pts[i]>255) isword=1;

    /* It's an error to push more than STACK_DEPTH points. */
    if (ptcnt > STACK_DEPTH)
        IError("Truetype stack overflow will occur.");

    if (ptcnt > 255 && !isword) {
        instrs = pushpoints(instrs, 255, pts);
	ptcnt-=255;
	pts+=255;
    }

    instrs = pushheader(instrs,isword,ptcnt);
    for (i=0; i<ptcnt; i++) instrs = addpoint(instrs, isword, pts[i]);
return( instrs );
}

#define pushnums(a, b, c) pushpoints(a, b, c)

/* As we don't have "push F26dot6" command in truetype instructions,
   we need to do this by hand. As we can explicitly push only 16-bit
   quantities, we need to push a F26dot6 value in halves, shift left
   the more significant half and add halves.

   There are no checks for overflow!
 */
static uint8 *pushF26Dot6(uint8 *instrs, double num) {
    unsigned int a, elems[3];
    int negative=0;

    if (num < 0) {
	negative=1;
	num*=-1.0;
    }

    num *= 64;
    a = rint(num);
    elems[0] = a % 65536;
    elems[1] = (unsigned int)rint(a / 65536.0) % 65536;
    elems[2] = 16384;

    if (elems[1]) {
        instrs = pushpoints(instrs, 3, elems);
	*instrs++ = DUP;
        *instrs++ = MUL;
	*instrs++ = MUL;
        *instrs++ = ADD;
    }
    else instrs = pushpoint(instrs, elems[0]);

    if (negative) *instrs++ = NEG;

return( instrs );
}

/* Push a EF2Dot14. No checks for overflow!
 */
static uint8 *pushEF2Dot14(uint8 *instrs, double num) {
    unsigned int a;
    int negative=0;

    if (num < 0) {
	negative=1;
	num*=-1.0;
    }

    num *= 16384;
    a = rint(num);
    instrs = pushpoint(instrs, a);
    if (negative) *instrs++ = NEG;

return( instrs );
}

/* An apparatus for instructing sets of points with given truetype command.
 * The command must pop exactly 1 element from the stack and mustn't push any.
 * These points must be marked as 'touched' elsewhere! this function only
 * generates intructions.
 */
static uint8 *instructpoints(uint8 *instrs, int ptcnt, const int *pts, uint8 command) {
    int i, use_sloop;

    use_sloop = 0;
    use_sloop |= (command == SHP_rp1);
    use_sloop |= (command == SHP_rp2);
    use_sloop |= (command == SHPIX);
    use_sloop |= (command == IP);
    use_sloop |= (command == FLIPPT);
    use_sloop |= (command == ALIGNRP);
    use_sloop = use_sloop && (ptcnt > 3);

    instrs = pushpoints(instrs, ptcnt<=STACK_DEPTH?ptcnt:STACK_DEPTH, pts);

    if (use_sloop) {
	*instrs++ = DEPTH;
	*instrs++ = SLOOP;
	*instrs++ = command;
    }
    else for (i=0; i<(ptcnt<=STACK_DEPTH?ptcnt:STACK_DEPTH); i++)
	*instrs++ = command;

    if (ptcnt>STACK_DEPTH)
	instrs=instructpoints(instrs, ptcnt-STACK_DEPTH, pts+STACK_DEPTH, command);

return( instrs );
}

/******************************************************************************
 *
 * Low-level routines for getting a cvt index for a stem width, assuming there
 * are any numbers in cvt. Includes legacy code for importing PS Private into
 * CVT.
 *
 ******************************************************************************/

struct ttf_table *SFFindTable(SplineFont *sf,uint32 tag) {
    struct ttf_table *tab;

    for ( tab=sf->ttf_tables; tab!=NULL && tab->tag!=tag; tab=tab->next );
return( tab );
}

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

/* We are given a stem weight and try to find matching one in CVT.
 * If none found, we return -1.
 */
static StdStem *CVTSeekStem(int xdir, GlobalInstrCt *gic, double value, int can_fail) {
    StdStem *mainstem = xdir?&(gic->stdvw):&(gic->stdhw);
    StdStem *otherstems = xdir?gic->stemsnapv:gic->stemsnaph;
    StdStem *closest = NULL;
    int otherstemcnt = xdir?gic->stemsnapvcnt:gic->stemsnaphcnt;
    int i;
    double mindelta=1e20, delta, closestwidth=1e20;

    if (mainstem->width == -1)
return NULL;

    value = fabs(value);
    delta = fabs(mainstem->width - value);

    if (delta < mindelta) {
        mindelta = delta;
	closestwidth = rint(mainstem->width);
	closest = mainstem;
    }

    for (i=0; i<otherstemcnt; i++) {
        delta = fabs(otherstems[i].width - value);

        if (delta < mindelta) {
            mindelta = delta;
	    closestwidth = otherstems[i].width;
	    closest = otherstems+i;
	}
    }

    if (mindelta <= gic->fudge)
return closest;
    if (value/closestwidth < 1.11 && value/closestwidth > 0.9)
return closest;
    if (can_fail)
return NULL;
return closest;
}

/******************************************************************************
 ******************************************************************************
 **
 **  We need to initialize global instructing context before autoinstructing
 **  a glyph, because we want to be core that global hinting tables (cvt, prep,
 **  fpgm) were (or weren't) properly set up.
 **
 ******************************************************************************
 ******************************************************************************/

/* Helper routines: read PS private entry and return its contents.
 */
static int GetBlueFuzz(SplineFont *sf) {
    char *str, *end;

    if ( sf->private==NULL || (str=PSDictHasEntry(sf->private,"BlueFuzz"))==NULL || !isdigit(str[0]) )
return 1;
return strtod(str, &end);
}

/* Return BlueScale as PPEM at which we have to stop suppressing overshoots */
static int GetBlueScale(SplineFont *sf) {
    char *str, *end;
    double bs;
    int result;

    if ( sf->private==NULL || (str=PSDictHasEntry(sf->private,"BlueScale"))==NULL )
return 42;

    bs = strtod(str, &end);
    if (end==str || bs<=0.0) bs=0.039625;
    bs*=240;
    bs+=0.49;
    bs*=300.0/72.0;

    result = (int)rint(bs);
    if (result>255) result = 255; /* Who would need such blue scale??? */

return result;
}

static real *ParsePSArray(const char *str, int *rescnt) {
    char *end;
    real d, *results=NULL;

    if ((rescnt == NULL) || (str == NULL))
return NULL;

    *rescnt = 0;

    while (*str)
    {
        while (!isdigit(*str) && *str!='-' && *str!='+' && *str!='.' && *str!='\0')
	    ++str;

	if ( *str=='\0' )
    break;

        d = strtod(str, &end);

        if ( d>=-32768 && d<=32767 ) {
            if (*rescnt) {
                results = grealloc(results, sizeof(real)*(++(*rescnt)));
	        results[*rescnt-1] = d;
            }
            else (results = gcalloc(*rescnt=1, sizeof(real)))[0] = d;
	}

        str = end;
    }

return results;
}

static real *GetNParsePSArray(SplineFont *sf, char *name, int *rescnt) {
return ParsePSArray(PSDictHasEntry(sf->private, name), rescnt);
}

/* Tell if the two segments, [b1,o1] and [b2,o2] intersect.
 * This can be used to determine whether blues or stems overlap.
 */
static int SegmentsOverlap(real b1, real o1, real b2, real o2) {
    real t;

    if (b1 > o1) {
        t = o1;
	o1 = b1;
	b1 = t;
    }

    if (b2 > o2) {
        t = o2;
	o2 = b2;
	b2 = t;
    }

return !((b2 > o1) || (o2 < b1));
}

/* To be used with qsort() - sorts BlueZone array by base in ascending order.
 */
static int SortBlues(const void *a, const void *b) {
    return ((BlueZone *)a)->base > ((BlueZone *)b)->base;
}

/* Import blue data into global instructing context. Include family blues too.
 * We assume that blues are needed for family blues to make sense. If there are
 * only family blues, we treat them as normal blues. Otherwise, if a family blue
 * zone doesn't match any normal blue zone, or if they match perfectly,
 * it is ignored.
 */
static void GICImportBlues(GlobalInstrCt *gic) {
    int bluecnt = 0;
    int i, j, cnt;
    real *values;

    int HasPSBlues =
             (PSDictHasEntry(gic->sf->private, "BlueValues") != NULL) ||
             (PSDictHasEntry(gic->sf->private, "OtherBlues") != NULL);

    int HasPSFamilyBlues =
             (PSDictHasEntry(gic->sf->private, "FamilyBlues") != NULL) ||
             (PSDictHasEntry(gic->sf->private, "FamilyOtherBlues") != NULL);

    char *PrimaryBlues = HasPSBlues ? "BlueValues" : "FamilyBlues";
    char *OtherBlues = HasPSBlues ? "OtherBlues" : "FamilyOtherBlues";

    if (HasPSBlues || HasPSFamilyBlues){
        values = GetNParsePSArray(gic->sf, PrimaryBlues, &cnt);
	cnt /= 2;
	if (cnt > 7) cnt = 7;

	if (values != NULL) {
	    gic->bluecnt = bluecnt = cnt;

	    /* First pair is a bottom zone (see Type1 specification). */
	    gic->blues[0].base = values[1];
	    gic->blues[0].overshoot = values[0];
	    gic->blues[0].family_base = strtod("NAN", NULL);

	    /* Next pairs are top zones (see Type1 specification). */
	    for (i=1; i<bluecnt; i++) {
	        gic->blues[i].family_base = strtod("NAN", NULL);
		gic->blues[i].base = values[2*i];
		gic->blues[i].overshoot = values[2*i+1];
	    }

	    free(values);
	}

        values = GetNParsePSArray(gic->sf, OtherBlues, &cnt);
	cnt /= 2;
	if (cnt > 5) cnt = 5;

	if (values != NULL) {
	    gic->bluecnt += cnt;

	    /* All pairs are bottom zones (see Type1 specification). */
	    for (i=0; i<cnt; i++) {
	        gic->blues[i+bluecnt].family_base = strtod("NAN", NULL);
		gic->blues[i+bluecnt].base = values[2*i+1];
		gic->blues[i+bluecnt].overshoot = values[2*i];
	    }

	    free(values);
	    bluecnt += cnt;
	}

	/* Add family data to blues */
	if (HasPSBlues && HasPSFamilyBlues) {
            values = GetNParsePSArray(gic->sf, "FamilyBlues", &cnt);
	    cnt /= 2;
	    if (cnt > 7) cnt = 7;

	    if (values != NULL) {
	        /* First pair is a bottom zone (see Type1 specification). */
	        for (j=0; j<bluecnt; j++)
		    if (finite(gic->blues[j].family_base))
		        continue;
		    else if (values[1] != gic->blues[j].base &&
		             SegmentsOverlap(gic->blues[j].base,
		                       gic->blues[j].overshoot,
				       values[0], values[1]))
		        gic->blues[j].family_base = values[1];

		/* Next pairs are top zones (see Type1 specification). */
		for (i=1; i<cnt; i++) {
		    for (j=0; j<bluecnt; j++)
		        if (finite(gic->blues[j].family_base))
			    continue;
			else if (values[2*i] != gic->blues[j].base &&
			         SegmentsOverlap(gic->blues[j].base,
				           gic->blues[j].overshoot,
					   values[2*i], values[2*i+1]))
			    gic->blues[j].family_base = values[2*i];
		}

		free(values);
	    }

            values = GetNParsePSArray(gic->sf, "FamilyOtherBlues", &cnt);
	    cnt /= 2;
	    if (cnt > 5) cnt = 5;

	    if (values != NULL) {
		/* All pairs are bottom zones (see Type1 specification). */
		for (i=0; i<cnt; i++) {
		    for (j=0; j<bluecnt; j++)
		        if (finite(gic->blues[j].family_base))
			    continue;
			else if (values[2*i+1] != gic->blues[j].base &&
			         SegmentsOverlap(gic->blues[j].base,
				           gic->blues[j].overshoot,
					   values[2*i], values[2*i+1]))
			    gic->blues[j].family_base = values[2*i+1];
		}

		free(values);
	    }
	}
    }
    else if (gic->bd->bluecnt) {
        /* If there are no PS private entries, we have */
	/* to use FF's quickly guessed fallback blues. */
        gic->bluecnt = bluecnt = gic->bd->bluecnt;

        for (i=0; i<bluecnt; i++) {
	    gic->blues[i].family_base = strtod("NAN", NULL);
	    gic->blues[i].family_cvtindex = -1;

	    if (gic->bd->blues[i][1] <= 0) {
	        gic->blues[i].base = gic->bd->blues[i][1];
		gic->blues[i].overshoot = gic->bd->blues[i][0];
	    }
	    else {
	        gic->blues[i].base = gic->bd->blues[i][0];
		gic->blues[i].overshoot = gic->bd->blues[i][1];
	    }
        }
    }

    /* 'highest' and 'lowest' are not to be set yet. */
    for (i=0; i<gic->bluecnt; i++)
        gic->blues[i].highest = gic->blues[i].lowest = -1;

    /* I assume ascending order in snap_to_blues(). */
    qsort(gic->blues, gic->bluecnt, sizeof(BlueZone), SortBlues);
}

/* To be used with qsort() - sorts StdStem array by width in ascending order.
 */
static int SortStems(const void *a, const void *b) {
    return ((StdStem *)a)->width > ((StdStem *)b)->width;
}

/* Import stem data into global instructing context. We deal only with
 * horizontal or vertical stems (xdir decides) here. If Std*V is not specified,
 * but there exists StemSnap*, we'll make up a fake Std*V as a fallback.
 * Subtle manipulations with Std*W's value can result in massive change of
 * font appearance at some pixel sizes, because it's used as a base for
 * normalization of all other stems.
 */
static void GICImportStems(int xdir, GlobalInstrCt *gic) {
    int i, cnt, next;
    real *values;
    char *s_StdW = xdir?"StdVW":"StdHW";
    char *s_StemSnap = xdir?"StemSnapV":"StemSnapH";
    StdStem *stdw = xdir?&(gic->stdvw):&(gic->stdhw);
    StdStem **stemsnap = xdir?&(gic->stemsnapv):&(gic->stemsnaph);
    int *stemsnapcnt = xdir?&(gic->stemsnapvcnt):&(gic->stemsnaphcnt);

    if ((values = GetNParsePSArray(gic->sf, s_StdW, &cnt)) != NULL) {
        stdw->width = *values;
        free(values);
    }

    if ((values = GetNParsePSArray(gic->sf, s_StemSnap, &cnt)) != NULL) {
        *stemsnap = (StdStem *)gcalloc(cnt, sizeof(StdStem));

        for (next=i=0; i<cnt; i++)
	    if (values[i] != gic->stdhw.width)
	        (*stemsnap)[next++].width = values[i];

	if (!next) {
	    free(*stemsnap);
	    *stemsnap = NULL;
	}

	*stemsnapcnt = next;
        free(values);

        /* I assume ascending order here and in normalize_stems(). */
        qsort(*stemsnap, *stemsnapcnt, sizeof(StdStem), SortStems);
    }

    /* No StdW, but StemSnap exists? */
    if (stdw->width == -1 && *stemsnap != NULL) {
        cnt = *stemsnapcnt;
	i = cnt/2;
	stdw->width = (*stemsnap)[i].width;
	memmove((*stemsnap)+i, (*stemsnap)+i+1, cnt-i-1);

	if (--(*stemsnapcnt) == 0) {
	    free(*stemsnap);
	    *stemsnap = NULL;
	}
    }
}

/* Assign CVT indices to blues and stems in global instructing context. In case
 * we can't implant it because of already existent cvt table, reassign the cvt
 * indices, picking them from existing cvt table (thus a cvt value can't be
 * considered 'horizontal' or 'vertical', and reliable stem normalization is
 * thus impossible) and adding some for new values.
 */
static void init_cvt(GlobalInstrCt *gic) {
    int i, cvtindex, cvtsize;
    uint8 *cvt;

    cvtsize = 1;
    if (gic->stdhw.width != -1) cvtsize++;
    if (gic->stdvw.width != -1) cvtsize++;
    cvtsize += gic->stemsnaphcnt;
    cvtsize += gic->stemsnapvcnt;
    cvtsize += gic->bluecnt * 2; /* possible family blues */

    cvt = gcalloc(cvtsize, cvtsize * sizeof(int16));
    cvtindex = 0;

    /* Assign cvt indices */
    for (i=0; i<gic->bluecnt; i++) {
        gic->blues[i].cvtindex = cvtindex;
        memputshort(cvt, 2*cvtindex++, rint(gic->blues[i].base));

	if (finite(gic->blues[i].family_base)) {
	    gic->blues[i].family_cvtindex = cvtindex;
            memputshort(cvt, 2*cvtindex++, rint(gic->blues[i].family_base));
	}
    }

    if (gic->stdhw.width != -1) {
        gic->stdhw.cvtindex = cvtindex;
	memputshort(cvt, 2*cvtindex++, rint(gic->stdhw.width));
    }

    for (i=0; i<gic->stemsnaphcnt; i++) {
        gic->stemsnaph[i].cvtindex = cvtindex;
	memputshort(cvt, 2*cvtindex++, rint(gic->stemsnaph[i].width));
    }

    if (gic->stdvw.width != -1) {
        gic->stdvw.cvtindex = cvtindex;
	memputshort(cvt, 2*cvtindex++, rint(gic->stdvw.width));
    }

    for (i=0; i<gic->stemsnapvcnt; i++) {
        gic->stemsnapv[i].cvtindex = cvtindex;
	memputshort(cvt, 2*cvtindex++, rint(gic->stemsnapv[i].width));
    }

    cvtsize = cvtindex;
    cvt = grealloc(cvt, cvtsize * sizeof(int16));

    /* Try to implant the new cvt table */
    gic->cvt_done = 0;

    struct ttf_table *tab = SFFindTable(gic->sf, CHR('c','v','t',' '));

    if ( tab==NULL ) {
	tab = chunkalloc(sizeof(struct ttf_table));
	tab->next = gic->sf->ttf_tables;
	gic->sf->ttf_tables = tab;
	tab->tag = CHR('c','v','t',' ');

	tab->len = tab->maxlen = cvtsize * sizeof(int16);
	if (tab->maxlen >256) tab->maxlen = 256;
        tab->data = cvt;

        gic->cvt_done = 1;
    }
    else {
        if (tab->len >= cvtsize * sizeof(int16) &&
	    memcmp(cvt, tab->data, cvtsize * sizeof(int16)) == 0)
	        gic->cvt_done = 1;

        free(cvt);

	if (!gic->cvt_done) {
	    ff_post_error(_("Can't insert 'cvt'"),
		_("There already exists a 'cvt' table, perhaps legacy. "
		  "FontForge can use it, but can't make any assumptions on "
		  "values stored there, so generated instructions will be of "
		  "lower quality. If legacy hinting is to be scrapped, it is "
		  "suggested to clear the `cvt` and repeat autoinstructing. "
	    ));
	}
    }

    if (gic->cvt_done)
return;

    /* Fallback mode starts here. */

    for (i=0; i<gic->bluecnt; i++)
        gic->blues[i].cvtindex =
	    TTF_getcvtval(gic->sf, gic->blues[i].base);

    if (gic->stdhw.width != -1)
        gic->stdhw.cvtindex =
	    TTF_getcvtval(gic->sf, gic->stdhw.width);

    for (i=0; i<gic->stemsnaphcnt; i++)
        gic->stemsnaph[i].cvtindex =
	    TTF_getcvtval(gic->sf, gic->stemsnaph[i].width);

    if (gic->stdvw.width != -1)
        gic->stdvw.cvtindex =
	    TTF_getcvtval(gic->sf, gic->stdvw.width);

    for (i=0; i<gic->stemsnapvcnt; i++)
        gic->stemsnapv[i].cvtindex =
	    TTF_getcvtval(gic->sf, gic->stemsnapv[i].width);
}

/* We'll need at least STACK_DEPTH stack levels and a twilight point (and thus
 * also a twilight zone). We also currently define five functions in fpgm, and
 * we use two storage locations to keep some factors for normalizing widths of
 * stems that don't use 'cvt' values (in future). We must ensure this is
 * indicated in the 'maxp' table.
 */
static void init_maxp(GlobalInstrCt *gic) {
    struct ttf_table *tab = SFFindTable(gic->sf, CHR('m','a','x','p'));
    uint16 zones, twpts, fdefs, stack;

    if ( tab==NULL ) {
	tab = chunkalloc(sizeof(struct ttf_table));
	tab->next = gic->sf->ttf_tables;
	gic->sf->ttf_tables = tab;
	tab->tag = CHR('m','a','x','p');
    }

    if ( tab->len<32 ) {
	tab->data = grealloc(tab->data,32);
	memset(tab->data+tab->len,0,32-tab->len);
	tab->len = tab->maxlen = 32;
    }

    zones = memushort(tab->data, 32,  7*sizeof(uint16));
    twpts = memushort(tab->data, 32,  8*sizeof(uint16));
    fdefs = memushort(tab->data, 32, 10*sizeof(uint16));
    stack = memushort(tab->data, 32, 12*sizeof(uint16));

    if (gic->fpgm_done && zones<2) zones=2;
    if (gic->fpgm_done && twpts<1) twpts=1;
    if (gic->fpgm_done && fdefs<9) fdefs=9;
    if (stack<STACK_DEPTH) stack=STACK_DEPTH;

    memputshort(tab->data, 7*sizeof(uint16), zones);
    memputshort(tab->data, 8*sizeof(uint16), twpts);
    memputshort(tab->data,10*sizeof(uint16), fdefs);
    memputshort(tab->data,12*sizeof(uint16), stack);
}

/* Other hinting software puts certain actions in FPGM to ease developer's life
 * and compress the code. I feel that having a 'standard' library of functions
 * could also help FF users.
 *
 * Caution! This code is heavily relied by autohinting. Any other code should
 * be placed below it. It's good to first clear font's hinting tables, then
 * autohint it, and then insert user's own code and do the manual hinting of
 * glyphs that do need it.
 */
static void init_fpgm(GlobalInstrCt *gic) {
    uint8 new_fpgm[] =
    {
	/* Function 0: position a point within a blue zone (given via cvt).
	 * Note: in case of successful init of 'cvt' and 'prep' this function
	 * could be much simpler.
	 * Syntax: PUSHB_3 point cvt_of_blue 0 CALL
	 */
	0xb0, // PUSHB_1
	0x00, //   0
	0x2c, // FDEF
	0xb0, //   PUSHB_1
	0x00, //     0
	0x13, //   SZP0
	0x4b, //   MPPEM
	0xb0, //   PUSHB_1 - under this ppem blues will be specially rounded
	GetBlueScale(gic->sf),
	0x50, //   LT
	0x58, //   IF
	0xb0, //     PUSHB_0
	0x4a, //       74
	0x76, //     SROUND - round blues a bit up to grid
	0x59, //   EIF
	0xb0, //   PUSHB_1
	0x00, //     0
	0x23, //   SWAP
	0x3f, //   MIAP[rnd] - blue zone positioned here
	0x18, //   RTG - round state for overshoots in monochrome mode
	0xb0, //   PUSHB_1
	0x06, //     6
	0x2b, //   CALL
	0x58, //   IF
	0x3d, //     RTDG - round state for overshoots in antialiased mode
	0x59, //   EIF
	0x4b, //   MPPEM
	0xb0, //   PUSHB_1 - under following ppem overshoots will be suppressed
	GetBlueScale(gic->sf),
	0x50, //   LT
	0x58, //   IF
	0x7d, //   RDTG - suppress overshoots
	0x59, //   EIF
	0x20, //   DUP
	0xd4, //   MDRP[rp0,rnd,grey]
	0xb0, //   PUSHB_1
	0x01, //     1
	0x13, //   SZP0
	0x2e, //   MDAP[no-rnd]
	0x18, //   RTG
	0x2d, // ENDF

	/* Function 1: Place given point relatively to previous. Check if the
	 * gridfitted position of the point is too far from its original
	 * position, and shift it, if necessary. The function is used to place
	 * vertical stems, it assures almost linear advance width to PPEM
	 * scaling. Shift amount is capped to at most 1 px to prevent some
	 * weird artifacts at very small ppems. In cleartype mode, no shift
	 * is made at all.
	 * Syntax: PUSHB_2 point 1 CALL
	 */
	0xb0, // PUSHB_1
	0x01, //   1
	0x2c, // FDEF
        0x20, //   DUP
        0x20, //   DUP
	0xda, //   MDRP[rp0,min,white]
	0x2f, //   MDAP[rnd], this is needed for grayscale mode
	0xb0, //   PUSHB_1
	0x07, //     7
	0x2b, //   CALL
	0x5c, //   NOT
	0x58, //   IF
        0x20, //     DUP
        0x20, //     DUP
        0x47, //     GC[cur]
        0x23, //     SWAP
        0x46, //     GC[orig]
        0x61, //     SUB
        0x6a, //     ROUND[white]
        0x20, //     DUP
        0x58, //     IF
        0x20, //       DUP
	0x64, //       ABS
	0x62, //       DIV
        0x38, //       SHPIX
	0x1b, //     ELSE
	0x21, //       POP
	0x21, //       POP
        0x59, //     EIF
	0x1b, //   ELSE
	0x21, //     POP
        0x59, //   EIF
	0x2d, // ENDF

	/* Function 2: Below given ppem, substitute the width with cvt entry.
	 * Leave the resulting width on the stack. Used as the first step in
	 * normalizing cvt stems, see normalize_stem().
	 * Syntax: PUSHX_3 width cvt_index ppem 2 CALL
	 */
	0xb0, // PUSHB_1
	0x02, //   2
	0x2c, // FDEF
	0x4b, //   MPPEM
	0x52, //   GT
	0x58, //   IF
	0x45, //     RCVT
	0x23, //     SWAP
	0x59, //   EIF
	0x21, //   POP
	0x2d, // ENDF

	/* Function 3: round a stack element as a black distance, respecting
	 * minimum distance of 1px. This is used for rounding stems after width
	 * normalization. Often preceeded with SROUND, so finally sets RTG.
	 * Leaves the rounded width on the stack.
	 * Syntax: PUSHX_2 width_to_be_rounded 3 CALL
	 */
	0xb0, // PUSHB_1
	0x03, //   3
	0x2c, // FDEF
	0x69, //   ROUND[black]
	0x18, //   RTG
	0x20, //   DUP
	0xb0, //   PUSHB_1
	0x40, //     64, that's one pixel as F26Dot6
	0x50, //   LT
	0x58, //   IF
	0x21, //     POP
	0xb0, //     PUSHB_1
	0x40, //       64
	0x59, //   EIF
	0x2d, // ENDF

	/* Function 4: Position the second edge of a stem that is not normally
	 * regularized via cvt (but we snap it to cvt width below given ppem).
	 * Vertical stems need special round state when not snapped to cvt
	 * (basically, they are shortened by 0.25px before being rounded).
	 * Syntax: PUSHX_5 pt cvt_index chg_rp0 ppem 4 CALL
	 */
	0xb0, // PUSHB_1
	0x04, //   4
	0x2c, // FDEF
	0xb0, //   PUSHB_1
	0x06, //     6
	0x2b, //   CALL
	0x58, //   IF
	0x21, //     POP
	0x23, //     SWAP
	0x21, //     POP
	0x7a, //     ROFF
	0x58, //     IF
	0xdd, //       MDRP[rp0,min,rnd,black]
	0x1b, //     ELSE
	0xcd, //       MDRP[min,rnd,black]
	0x59, //     EIF
	0x1b, //   ELSE
	0x4b, //     MPPEM
	0x52, //     GT
	0x58, //     IF
	0x58, //       IF
	0xfd, //         MIRP[rp0,min,rnd,black]
	0x1b, //       ELSE
	0xed, //         MIRP[min,rnd,black]
	0x59, //       EIF
	0x1b, //     ELSE
	0x21, //       POP
	0xb0, //       PUSHB_1
	0x05, //         5
	0x2b, //       CALL
	0x58, //       IF
	0xb0, //         PUSHB_1
	0x46, //           70
	0x76, //         SROUND
	0x59, //       EIF
	0x58, //       IF
	0xdd, //         MDRP[rp0,min,rnd,black]
	0x1b, //       ELSE
	0xcd, //         MDRP[min,rnd,black]
	0x59, //       EIF
	0x59, //     EIF
	0x59, //   EIF
	0x18, //   RTG
	0x2d, // ENDF

	/* Function 5: determine if we are hinting vertically. The function
	 * is crude and it's use is limited to conditions set by SVTCA[].
	 * Syntax: PUSHB_1 5 CALL; leaves boolean on the stack.
	 */
	0xb0, // PUSHB_1
	0x05, //   5
	0x2c, // FDEF
	0x0d, //   GFV
	0x5c, //   NOT
	0x5a, //   AND
	0x2d, // ENDF

	/* Function 6: check if we are hinting in grayscale.
	 * CAUTION! Older FreeType versions lie if asked.
	 * Syntax: PUSHB_1 6 CALL; leaves boolean on the stack.
	 */
	0xb0, // PUSHB_1
	0x06, //   6
	0x2c, // FDEF
	0xb1, //   PUSHB_2
	0x22, //     34
	0x01, //     1
	0x88, //   GETINFO
	0x50, //   LT
	0x58, //   IF
	0xb0, //     PUSHB_1
	0x20, //       32
	0x88, //     GETINFO
	0x5c, //     NOT
	0x5c, //     NOT
	0x1b, //   ELSE
	0xb0, //     PUSHB_1
	0x00, //       0
	0x59, //   EIF
	0x2d, // ENDF

	/* Function 7: check if we are hinting in cleartype.
	 * CAUTION! Older FreeType versions lie if asked.
	 * Syntax: PUSHB_1 7 CALL; leaves boolean on the stack.
	 */
	0xb0, // PUSHB_1
	0x07, //   7
	0x2c, // FDEF
	0xb1, //   PUSHB_2
	0x24, //     36
	0x01, //     1
	0x88, //   GETINFO
	0x50, //   LT
	0x58, //   IF
	0xb0, //     PUSHB_1
	0x40, //       64
	0x88, //     GETINFO
	0x5c, //     NOT
	0x5c, //     NOT
	0x1b, //   ELSE
	0xb0, //     PUSHB_1
	0x00, //       0
	0x59, //   EIF
	0x2d, // ENDF

	/* Function 8: Interpolate a point between
	 * two other points and snap it to the grid.
	 * Syntax: PUSHX_4 pt_to_ip rp1 rp2 8 CALL;
	 */
	0xb0, // PUSHB_1
	0x08, //   8
	0x2c, // FDEF
	0x12, //   SRP2
	0x11, //   SRP1
	0x20, //   DUP
	0x39, //   IP
	0x2f, //   MDAP[rnd]
	0x2d  // ENDF
    };

    struct ttf_table *tab = SFFindTable(gic->sf, CHR('f','p','g','m'));

    if ( tab==NULL ) {
	/* We have to create such table. */
	tab = chunkalloc(sizeof(struct ttf_table));
	tab->next = gic->sf->ttf_tables;
	gic->sf->ttf_tables = tab;
	tab->tag = CHR('f','p','g','m');
	tab->len = 0;
    }

    if (tab->len==0 ||
        (tab->len < sizeof(new_fpgm) && !memcmp(tab->data, new_fpgm, tab->len)))
    {
	/* We can safely update font program. */
	tab->len = tab->maxlen = sizeof(new_fpgm);
	tab->data = grealloc(tab->data, sizeof(new_fpgm));
	memmove(tab->data, new_fpgm, sizeof(new_fpgm));
        gic->fpgm_done = 1;
    }
    else {
	/* there already is a font program. */
	gic->fpgm_done = 0;
	if (tab->len >= sizeof(new_fpgm))
	    if (!memcmp(tab->data, new_fpgm, sizeof(new_fpgm)))
		gic->fpgm_done = 1;  /* it's ours. */

	/* Log warning message. */
	if (!gic->fpgm_done)
	    ff_post_error(_("Can't insert 'fpgm'"),
		_("There exists a 'fpgm' code that seems incompatible with "
		  "FontForge's. Instructions generated will be of lower "
		  "quality. If legacy hinting is to be scrapped, it is "
		  "suggested to clear the `fpgm` and repeat autoinstructing. "
		  "It will be then possible to append user's code to "
		  "FontForge's 'fpgm', but due to possible future updates, "
		  "it is extremely advised to use high numbers for user's "
		  "functions."
	    ));
    }
}

/* When initializing global instructing context, we want to set up the 'prep'
 * table in order to apply family blues and normalize stem widths for monochrome
 * display.
 *
 * The stem normalizer is heavily based on simple concept from FreeType2.
 *
 * First round the StdW. Then for each StemSnap (going outwards from StdW) check
 * if it's within 1px from its already rounded neighbor, and if so, snap it
 * before rounding. From all vertical stems (but not StdHW itself), 0.25px is
 * subtracted before rounding. Similar method is used for non-cvt stems, they're
 * snapped to the closest standard width if possible.
 *
 * NOTE: because of tiny scaling issues, we have to compute ppem at which each
 * stem stops being snapped to its already-rounded neighbor here instead of
 * relegating this to the truetype bytecide interpreter. We can't simply rely
 * on cvt cut-in.
 */

static int compute_blue_height(real val, int EM, int bluescale, int ppem) {
    int scaled_val = rint((rint(fabs(val)) * ppem * 64)/EM);
    if (ppem < bluescale) scaled_val += 16;
return (scaled_val + 32) / 64 * (val / fabs(val));
}

static uint8 *use_family_blues(uint8 *prep_head, GlobalInstrCt *gic) {
    int i, h1, h2, stopat;
    int bs = GetBlueScale(gic->sf);
    int EM = gic->sf->ascent + gic->sf->descent;
    int callargs[3];

    for (i=0; i<gic->bluecnt; i++) {
	if (finite(gic->blues[i].family_base))
	{
	    for (stopat=0; stopat<32768; stopat++) {
		h1 = compute_blue_height(gic->blues[i].base, EM, bs, stopat);
		h2 = compute_blue_height(gic->blues[i].family_base, EM, bs, stopat);
		if (abs(h1 - h2) > 1) break;
	    }

	    callargs[0] = gic->blues[i].family_cvtindex;
	    callargs[1] = stopat;
	    callargs[2] = 2;

	    prep_head = pushnum(prep_head, gic->blues[i].cvtindex);
	    *prep_head++ = DUP;
	    *prep_head++ = 0x45; //RCVT
	    prep_head = pushnums(prep_head, 3, callargs);
	    *prep_head++ = CALL;
	    *prep_head++ = 0x44; //WCVTP
	}
    }

    return prep_head;
}

/* Return width (in pixels) of given stem, taking snaps into account.
 */
#define SNAP_THRESHOLD (64)
static int compute_stem_width(int xdir, StdStem *stem, int EM, int ppem) {
    int scaled_width; /* in 1/64th pixels */
    int snapto_width; /* in 1/64th pixels */

    scaled_width = (int)rint((rint(fabs(stem->width)) * ppem * 64.0)/EM);
    if (scaled_width < 64) scaled_width = 64;

    if (stem->snapto != NULL)
    {
        if (stem->stopat > ppem) {
	    snapto_width = 64*compute_stem_width(xdir, stem->snapto, EM, ppem);

	    if (abs(snapto_width - scaled_width) < SNAP_THRESHOLD)
	        scaled_width = snapto_width;
	}

	if (xdir) scaled_width -= 16;
    }

return (scaled_width + 32) / 64;
}

/* Normalize a single stem. The code generated assumes there is a scaled stem
 * width on bytecode interpreter's stack, and leaves normalized width there.
 */
static uint8 *normalize_stem(uint8 *prep_head, int xdir, StdStem *stem, GlobalInstrCt *gic) {
    int callargs[3];
    int i;

    stem->stopat = 32767;

    if (stem->snapto != NULL)
    {
        /* compute ppem at which to stop snapping stem to stem->snapto */
        int EM = gic->sf->ascent + gic->sf->descent;

        for (i=7; i<32768; i++) {
	    int width_parent = compute_stem_width(xdir, stem->snapto, EM, i);
	    int width_me = compute_stem_width(xdir, stem, EM, i);

	    if (width_parent != width_me) {
	        stem->stopat = i;
		break;
	    }
	}

	/* snap if below given ppem */
	callargs[0] = stem->snapto->cvtindex;
	callargs[1] = stem->stopat;
	callargs[2] = 2;
	prep_head = pushnums(prep_head, 3, callargs);
	*prep_head++ = CALL;

        /* Round[black], respecting minimum distance of 1 px */
	/* Vertical stems (but not StdVW) use special rounding threshold. */
	/* The rounding function restores default round state at the end. */
        if (xdir) {
            prep_head = push2nums(prep_head, 3, 70);
	    *prep_head++ = SROUND;
        }
        else prep_head = pushnum(prep_head, 3);

        *prep_head++ = CALL;
    }
    else {
        /* simply round[black] respecting minimum distance of 1 px */
        prep_head = pushnum(prep_head, 3);
	*prep_head++ = CALL;
    }

return prep_head;
}

/* Append the code for normalizing standard stems' widths to 'prep'.
 */
static uint8 *normalize_stems(uint8 *prep_head, int xdir, GlobalInstrCt *gic) {
    int i, t;
    StdStem *mainstem = xdir?&(gic->stdvw):&(gic->stdhw);
    StdStem *otherstems = xdir?gic->stemsnapv:gic->stemsnaph;
    int otherstemcnt = xdir?gic->stemsnapvcnt:gic->stemsnaphcnt;

    if (mainstem->width == -1)
return prep_head;

    /* set up the standard width */
    mainstem->snapto = NULL;
    *prep_head++ = xdir?SVTCA_x:SVTCA_y;
    prep_head = pushnum(prep_head, mainstem->cvtindex);
    *prep_head++ = DUP;
    *prep_head++ = 0x45; //RCVT
    prep_head = normalize_stem(prep_head, xdir, mainstem, gic);
    *prep_head++ = 0x44; //WCVTP

    /* set up other standard widths */
    for (i=0; i<otherstemcnt && otherstems[i].width < mainstem->width; i++);
    t = i-1;

    for (i=t; i>=0; i--) {
        otherstems[i].snapto = i==t?mainstem:otherstems+i+1;
        prep_head = pushnum(prep_head, otherstems[i].cvtindex);
	*prep_head++ = DUP;
        *prep_head++ = 0x45; //RCVT
	prep_head = normalize_stem(prep_head, xdir, otherstems+i, gic);
        *prep_head++ = 0x44; //WCVTP
    }

    for (i=t+1; i<otherstemcnt; i++) {
        otherstems[i].snapto = i==t+1?mainstem:otherstems+i-1;
	prep_head = pushnum(prep_head, otherstems[i].cvtindex);
	*prep_head++ = DUP;
        *prep_head++ = 0x45; //RCVT
	prep_head = normalize_stem(prep_head, xdir, otherstems+i, gic);
        *prep_head++ = 0x44; //WCVTP
    }

return prep_head;
}

/* Turning dropout control on will dramatically improve mono rendering, even
 * without further hinting, especcialy for light typefaces. And turning hinting
 * off at veeery small pixel sizes is required, because hints tend to visually
 * tear outlines apart when not having enough workspace.
 *
 * We also normalize stem widths here, this usually massively improves overall
 * consistency. We currently do this only for monochrome rendering (this
 * includes WinXP's cleartype).
 *
 * TODO! We should take 'gasp' table into account and set up blues here.
 */
static void init_prep(GlobalInstrCt *gic) {
    uint8 new_prep_preamble[] =
    {
	/* Enable dropout control */
	0xb8, // PUSHW_1
	0x01, //   511
	0xff, //   ...still that 511
	0x85, // SCANCTRL

	/* Measurements are taken along Y axis */
	0x00, // SVTCA[y-axis]

        /* Turn hinting off at very small pixel sizes */
	0x4b, // MPPEM
	0xb0, // PUSHB_1
	0x08, //   8 - hinting threshold - should be configurable
	0x50, // LT
	0x58, // IF
	0xb1, //   PUSHB_2
	0x01, //     1
	0x01, //     1
	0x8e, //   INSTCTRL
	0x59, // EIF

	/* Determine the cvt cut-in used */
	0xb1, // PUSHB_2
	0x46, //   70/64 = about 1.094 pixel (that's our default setting)
	0x06, //   6
	0x2b, // CALL
	0x58, // IF
	0x21, //   POP
	0xb0, //   PUSHB_1
	0x10, //     16/64 = 0.25 pixel (very low cut-in for grayscale mode)
	0x59, // EIF
	0x4b, // MPPEM
	0xb0, // PUSHB_1
	0x14, //   20 PPEM - a threshold below which we'll use larger CVT cut-in
	0x52, // GT
	0x58, // IF
	0x21, //   POP
	0xb0, //   PUSHB_1
	0x80, //     128/64 = 2 pixels (extreme regularization for small ppems)
	0x59, // EIF
	0x1d  // SCVTCI
    };

    int preplen = sizeof(new_prep_preamble);
    int prepmaxlen = preplen;
    uint8 *new_prep, *prep_head;
    struct ttf_table *tab;

    if (gic->cvt_done) {
        prepmaxlen += 48 + 38*(gic->stemsnaphcnt + gic->stemsnapvcnt);
	prepmaxlen += 14*(gic->bluecnt);
    }

    new_prep = gcalloc(prepmaxlen, sizeof(uint8));
    memmove(new_prep, new_prep_preamble, preplen*sizeof(uint8));
    prep_head = new_prep + preplen;

    if (gic->cvt_done && gic->fpgm_done) {
	/* Apply family blues. */
	prep_head = use_family_blues(prep_head, gic);

        /* Normalize stems (only in monochrome mode) */
        prep_head = pushnum(prep_head, 6);
	*prep_head++ = CALL;
	*prep_head++ = 0x5c; // NOT
	*prep_head++ = 0x58; // IF
        prep_head = normalize_stems(prep_head, 0, gic);
        prep_head = normalize_stems(prep_head, 1, gic);
	*prep_head++ = 0x59; // EIF
        preplen = prep_head - new_prep;
    }

    tab = SFFindTable(gic->sf, CHR('p','r','e','p'));

    if ( tab==NULL ) {
	/* We have to create such table. */
	tab = chunkalloc(sizeof(struct ttf_table));
	tab->next = gic->sf->ttf_tables;
	gic->sf->ttf_tables = tab;
	tab->tag = CHR('p','r','e','p');
	tab->len = 0;
    }

    if (tab->len==0 ||
        (tab->len < preplen && !memcmp(tab->data, new_prep, tab->len)))
    {
	/* We can safely update cvt program. */
	tab->len = tab->maxlen = preplen;
	tab->data = grealloc(tab->data, preplen);
	memmove(tab->data, new_prep, preplen);
        gic->prep_done = 1;
    }
    else {
	/* there already is a font program. */
	gic->prep_done = 0;
	if (tab->len >= preplen)
	    if (!memcmp(tab->data, new_prep, preplen))
		gic->prep_done = 1;  /* it's ours */

	/* Log warning message. */
	if (!gic->prep_done)
	    ff_post_error(_("Can't insert 'prep'"),
		_("There exists a 'prep' code incompatible with FontForge's. "
		  "It can't be guaranteed it will work well. It is suggested "
		  "to allow FontForge to insert its code and then append user"
		  "'s own."
	    ));
    }

    free(new_prep);
}

/*
 * Initialize Global Instructing Context
 */
#define EDGE_FUZZ (500.0)
void InitGlobalInstrCt(GlobalInstrCt *gic, SplineFont *sf, BlueData *bd) {
    BlueData _bd;

    if (bd == NULL) {
	QuickBlues(sf,&_bd);
	bd = &_bd;
    }

    gic->sf = sf;
    gic->bd = bd;
    gic->fudge = (sf->ascent+sf->descent)/EDGE_FUZZ;

    gic->cvt_done = false;
    gic->fpgm_done = false;
    gic->prep_done = false;

    gic->bluecnt = 0;
    gic->stdhw.width = -1;
    gic->stemsnaph = NULL;
    gic->stemsnaphcnt = 0;
    gic->stdvw.width = -1;
    gic->stemsnapv = NULL;
    gic->stemsnapvcnt = 0;

    GICImportBlues(gic);
    GICImportStems(0, gic); /* horizontal stems */
    GICImportStems(1, gic); /* vertical stems */

    init_cvt(gic);
    init_fpgm(gic);
    init_prep(gic);
    init_maxp(gic);
}

/*
 * Finalize Global Instructing Context
 */
void FreeGlobalInstrCt(GlobalInstrCt *gic) {
    gic->sf = NULL;
    gic->bd = NULL;
    gic->fudge = 0;

    gic->cvt_done = false;
    gic->fpgm_done = false;
    gic->prep_done = false;

    gic->bluecnt = 0;
    gic->stdhw.width = -1;
    if (gic->stemsnaphcnt != 0) free(gic->stemsnaph);
    gic->stemsnaphcnt = 0;
    gic->stemsnaph = NULL;
    gic->stdvw.width = -1;
    if (gic->stemsnapvcnt != 0) free(gic->stemsnapv);
    gic->stemsnapvcnt = 0;
    gic->stemsnapv = NULL;
}

/******************************************************************************
 ******************************************************************************
 **
 **  Stuff for managing global instructing context ends here. Now we'll deal
 **  with single glyphs.
 **
 **  Many functions here need large or similar sets of arguments. I decided to
 **  define an '(local) instructing context' to have them in one place and keep
 **  functions' argument lists reasonably short. I first need to define some
 **  internal sub-structures for instructing diagonal stems. Similar structures
 **  for CVT management (based on PS Private) are defined in splinefont.h, and
 **  were initialized handled above.
 **
 ******************************************************************************
 ******************************************************************************/

/* This structure is used to keep a point number together with
   its coordinates */
typedef struct numberedpoint {
    int num;
    struct basepoint *pt;
} NumberedPoint;

/* A line, described by two points */
typedef struct pointvector {
    struct numberedpoint *pt1, *pt2;
    int done;
} PointVector;

/* In this structure we store information about diagonales,
   relatively to which the given point should be positioned */
typedef struct diagpointinfo {
    struct pointvector *line[2];
    int count;
} DiagPointInfo;

/* Diagonal stem hints. This structure is a bit similar to DStemInfo FF
   uses in other cases, but additionally stores point numbers and hint width */
typedef struct dstem {
    struct dstem *next;
    struct numberedpoint pts[4];
    int done;
    real width;
} DStem;

typedef struct instrct {
    /* Things that are global for font and should be
       initialized before instructing particular glyph. */
    GlobalInstrCt *gic;

    SplineChar *sc;
    SplineSet *ss;
    int ptcnt;            /* number of points in this glyph */
    int *contourends;     /* points ending their contours. null-terminated. */

    /* instructions */
    uint8 *instrs;        /* the beginning of the instructions */
    uint8 *pt;            /* the current position in the instructions */

    /* properties, indexed by ttf point index. Some could be compressed. */
    BasePoint *bp;        /* point coordinates */
    uint8 *touched;       /* touchflags; points explicitly instructed */
    uint8 *affected;      /* touchflags; almost touched, but optimized out */
    uint8 *oncurve;       /* boolean; these points are on-curve */

    /* stuff for hinting diagonals */
    DStem *diagstems;
    DiagPointInfo *diagpts; /* indexed by ttf point index */

    /* stuff for hinting edges (for stems and blues): */
    int xdir;             /* direction flag: x=true, y=false */
    int cdir;             /* is current contour outer? - blues need this */
    struct __edge {
	real base;        /* where the edge is */
	int refpt;        /* best ref. point for an edge, ttf index, -1 if none */
	int refscore;     /* its quality, for searching better one, 0 if none */
	int othercnt;     /* count of other points to instruct for this edge */
	int *others;      /* their ttf indices, optimize_edge() is advised */
    } edge;

    /* Some variables for tracking graphics state */
    int rp0;
} InstrCt;

/******************************************************************************
 *
 * Low-level routines for manipulting and classifying splinepoints
 *
 ******************************************************************************/

/* Find previous point index on the contour. */
static int PrevOnContour(int *contourends, int p) {
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
static int NextOnContour(int *contourends, int p) {
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
 * instructing is pararell to hint's direction to avoid snapping wrong points.
 * I splitted the routine into two, as sometimes it may be needed to check
 * the angle to be strictly almost the same, not just pararell.
 */
static int __same_angle(int *contourends, BasePoint *bp, int p, double angle) {
    int PrevPoint, NextPoint;
    double PrevTangent, NextTangent;

    PrevPoint = PrevOnContour(contourends, p);
    NextPoint = NextOnContour(contourends, p);
    PrevTangent = atan2(bp[p].y - bp[PrevPoint].y, bp[p].x - bp[PrevPoint].x);
    NextTangent = atan2(bp[NextPoint].y - bp[p].y, bp[NextPoint].x - bp[p].x);

    /* If at least one of the tangents is close to the given angle, return */
    /* true. 'Close' means about 5 deg, i.e. about 0.087 rad. */
    PrevTangent = fabs(PrevTangent-angle);
    NextTangent = fabs(NextTangent-angle);
    while (PrevTangent > M_PI) PrevTangent -= 2*M_PI;
    while (NextTangent > M_PI) NextTangent -= 2*M_PI;
return (fabs(PrevTangent) <= 0.087) || (fabs(NextTangent) <= 0.087);
}

static int same_angle(int *contourends, BasePoint *bp, int p, double angle) {
return __same_angle(contourends, bp, p, angle) || __same_angle(contourends, bp, p, angle+M_PI);
}

/* I found it needed to write some simple functions to classify points snapped
 * to hint's edges. Classification helps to establish the most accurate leading
 * point for an edge. IsSnappable is a fallback routine.
 */
static int IsExtremum(int xdir, SplinePoint *sp) {
return xdir?
    (!sp->nonextcp && !sp->noprevcp && sp->nextcp.x==sp->me.x && sp->prevcp.x==sp->me.x):
    (!sp->nonextcp && !sp->noprevcp && sp->nextcp.y==sp->me.y && sp->prevcp.y==sp->me.y);
}

static int IsCornerExtremum(int xdir, int *contourends, BasePoint *bp, int p) {
    int PrevPoint = PrevOnContour(contourends, p);
    int NextPoint = NextOnContour(contourends, p);

return xdir?
    ((bp[PrevPoint].x > bp[p].x && bp[NextPoint].x > bp[p].x) ||
     (bp[PrevPoint].x < bp[p].x && bp[NextPoint].x < bp[p].x)):
    ((bp[PrevPoint].y > bp[p].y && bp[NextPoint].y > bp[p].y) ||
     (bp[PrevPoint].y < bp[p].y && bp[NextPoint].y < bp[p].y));
}

static int IsAnglePoint(int *contourends, BasePoint *bp, SplinePoint *sp) {
    int PrevPoint, NextPoint, p=sp->ttfindex;
    double PrevTangent, NextTangent;

    if ((sp->pointtype != pt_corner) || (p == 0x80000000))
return 0;

    PrevPoint = PrevOnContour(contourends, p);
    NextPoint = NextOnContour(contourends, p);
    PrevTangent = atan2(bp[p].y - bp[PrevPoint].y, bp[p].x - bp[PrevPoint].x);
    NextTangent = atan2(bp[NextPoint].y - bp[p].y, bp[NextPoint].x - bp[p].x);

return fabs(PrevTangent - NextTangent) > 0.261;
}

static int IsInflectionPoint(int *contourends, BasePoint *bp, SplinePoint *sp) {
    double CURVATURE_THRESHOLD = 1e-9;
    struct spline *prev, *next;
    double in, out;

    if (IsAnglePoint(contourends, bp, sp))
return 0;

    /* point of a single-point contour can't be an inflection point. */
    if (sp->prev != NULL && sp->prev->from != NULL && sp->prev->from == sp)
return 0;

    prev = sp->prev;
    in = 0;
    while (prev != NULL && fabs(in) < CURVATURE_THRESHOLD) {
        in = SplineCurvature(prev, 1);
	if (fabs(in) < CURVATURE_THRESHOLD) in = SplineCurvature(prev, 0);
	if (fabs(in) < CURVATURE_THRESHOLD) prev = prev->from->prev;
	if (IsAnglePoint(contourends, bp, prev->to))
    break;
    }

    next = sp->next;
    out = 0;
    while (next != NULL && fabs(out) < CURVATURE_THRESHOLD) {
        out = SplineCurvature(next, 0);
	if (fabs(out) < CURVATURE_THRESHOLD) out = SplineCurvature(next, 1);
	if (fabs(out) < CURVATURE_THRESHOLD) next = next->to->next;
	if (IsAnglePoint(contourends, bp, next->from))
    break;
    }

    if (in==0 || out==0 || (prev != sp->prev && next != sp->next))
return 0;

    in/=fabs(in);
    out/=fabs(out);

return (in*out < 0);
}

static int IsSnappable(InstrCt *ct, int p) {
    BasePoint *bp = ct->bp;
    int coord = ct->xdir ? bp[p].x : bp[p].y;
    int Prev = PrevOnContour(ct->contourends, p);
    int Next = NextOnContour(ct->contourends, p);

    if ((coord != ct->edge.base) || (!ct->oncurve[p]))
return 0;

return ct->xdir?
    (abs(bp[Prev].x - bp[p].x) <= abs(bp[Prev].y - bp[p].y) ||
     abs(bp[Next].x - bp[p].x) <= abs(bp[Next].y - bp[p].y)):
    (abs(bp[Prev].x - bp[p].x) >= abs(bp[Prev].y - bp[p].y) ||
     abs(bp[Next].x - bp[p].x) >= abs(bp[Next].y - bp[p].y));
}

/******************************************************************************
 *
 * I found it easier to write an iterator that calls given function for each
 * point worth instructing than repeating the same loops all the time.
 *
 * The control points are not skipped, but runmes often eliminate them as
 * instructing them seems to cause more damages than profits. They are included
 * here because edge optimizer cam be simpler and work more reliably then.
 *
 * The contour_direction option is for blues - snapping internal contour to a
 * blue zone is plain wrong, unless there is a stem hint tat don't fit to any
 * other blue zone.
 *
 ******************************************************************************/
#define EXTERNAL_CONTOURS 0
#define ALL_CONTOURS 1
#define INTERNAL_CONTOURS 2
static void RunOnPoints(InstrCt *ct, int contour_direction,
    void (*runme)(int p, SplinePoint *sp, InstrCt *ct))
{
    SplineSet *ss = ct->ss;
    SplinePoint *sp;
    uint8 *done;
    int p;

    done = (uint8 *)gcalloc(ct->ptcnt, sizeof(uint8));

    for ( ; ss!=NULL; ss=ss->next ) {
	ct->cdir = SplinePointListIsClockwise(ss);

	if (((contour_direction == EXTERNAL_CONTOURS) && !ct->cdir) ||
	    ((contour_direction == INTERNAL_CONTOURS) && ct->cdir)) continue;

	for ( sp=ss->first; ; ) {
	    if (sp->ttfindex != 0xffff) {
		if (!sp->noprevcp &&
		    !done[p = PrevOnContour(ct->contourends, sp->ttfindex)])
		{
		    runme(p, sp, ct);
		    done[p] = true;
		}

		if (!done[p = sp->ttfindex]) {
		    runme(p, sp, ct);
		    done[p] = true;
		}

		if (!sp->nonextcp && !done[p = sp->nextcpindex])
		{
		    runme(p, sp, ct);
		    done[p] = true;
		}
	    }
	    else if (!sp->nonextcp) {
		if (!done[p = PrevOnContour(ct->contourends, sp->nextcpindex)]) {
		    runme(p, sp, ct);
		    done[p] = true;
		}

		if (!done[p = sp->nextcpindex]) {
	    	    runme(p, sp, ct);
	    	    done[p] = true;
		}
	    }

	    if ( sp->next==NULL ) break;
	    sp = sp->next->to;
	    if ( sp==ss->first ) break;
	}
    }

    free(done);
}

/* One of first functions to run: determine which points lie on their curves. */
static void find_control_pts(int p, SplinePoint *sp, InstrCt *ct) {
    if (p == sp->ttfindex) ct->oncurve[p] |= 1;
}

/******************************************************************************
 *
 * Hinting is mostly aligning 'edges' (in FreeType's sense). Each stem hint
 * consists of two edges (or one, for ghost hints). And each blue zone can be
 * represented as an edge with extended fudge (overshoot).
 *
 * Hinting a stem edge is broken in two steps. First: init_edge() seeks for
 * points to snap and chooses one that will be used as a reference point - it
 * should be then instructed elsewhere (a general method of edge positioning).
 * Finally, finish_edge() instructs the rest of points found with given command,
 * using instructpoints(). It normally optimizes an edge before instructing,
 * but not in presence of diagonal hints.
 *
 * The contour_direction option of init_edge() is for hinting blues - snapping
 * internal contour to a bluezone seems just plainly wrong.
 *
 ******************************************************************************/

/* search for points to be snapped to an edge - to be used in RunOnPoints() */
static void search_edge(int p, SplinePoint *sp, InstrCt *ct) {
    int tmp, score = 0;
    real fudge = ct->gic->fudge;
    uint8 touchflag = ct->xdir?tf_x:tf_y;
    int EM = ct->gic->sf->ascent + ct->gic->sf->descent;
    real refcoord, coord = ct->xdir?ct->bp[p].x:ct->bp[p].y;

    if (fabs(coord - ct->edge.base) <= fudge)
    {
	if (IsCornerExtremum(ct->xdir, ct->contourends, ct->bp, p) ||
	    IsExtremum(ct->xdir, sp))
		score+=4;

	if (same_angle(ct->contourends, ct->bp, p, ct->xdir?0.5*M_PI:0.0))
	    score++;

	if (p == sp->ttfindex && IsAnglePoint(ct->contourends, ct->bp, sp))
	    score++;

	/* a crude way to distinguisg stem edge from zone */
	if (fudge > (EM/EDGE_FUZZ+0.0001) && IsExtremum(!ct->xdir, sp))
	    score++;

	if (IsInflectionPoint(ct->contourends, ct->bp, sp))
	    score++;

	if (score && ct->oncurve[p])
	    score+=2;

	if (!score)
return;

	if (ct->diagstems && ct->diagpts[p].count) score+=9;
	if (ct->touched[p] & touchflag) score+=26;

	if (ct->edge.refpt == -1) {
	    ct->edge.refpt = p;
	    ct->edge.refscore = score;
return;
	}

	refcoord = ct->xdir?ct->bp[ct->edge.refpt].x:ct->bp[ct->edge.refpt].y;

	if ((score > ct->edge.refscore) ||
	    (score == ct->edge.refscore &&
	    fabs(coord - ct->edge.base) < fabs(refcoord - ct->edge.base)))
	{
	    tmp = ct->edge.refpt;
	    ct->edge.refpt = p;
	    ct->edge.refscore = score;
	    p = tmp;
	}

	if ((p!=-1) && !((ct->touched[p] | ct->affected[p]) & touchflag)) {
	    ct->edge.othercnt++;

	    if (ct->edge.othercnt==1) ct->edge.others=(int *)gcalloc(1, sizeof(int));
	    else ct->edge.others=(int *)grealloc(ct->edge.others, ct->edge.othercnt*sizeof(int));

	    ct->edge.others[ct->edge.othercnt-1] = p;
	}
    }
}

/* If we failed to find good snappable points, then let's try to find any. */
/* TODO! Perhaps we shouldn't? */
static void search_edge_desperately(int p, SplinePoint *sp, InstrCt *ct) {
    uint8 touchflag = ct->xdir?tf_x:tf_y;

    if (IsSnappable(ct, p) && !((ct->touched[p] | ct->affected[p]) & touchflag)) {
	if (ct->edge.refpt == -1) ct->edge.refpt = p;
	else {
	    ct->edge.othercnt++;

	    if (ct->edge.othercnt==1) ct->edge.others=(int *)gcalloc(1, sizeof(int));
	    else ct->edge.others=(int *)grealloc(ct->edge.others, ct->edge.othercnt*sizeof(int));

	    ct->edge.others[ct->edge.othercnt-1] = p;
	}
    }
}

/* Initialize the InstrCt for instructing given edge. */
static void init_edge(InstrCt *ct, real base, int contour_direction) {
    ct->edge.base = base;
    ct->edge.refpt = -1;
    ct->edge.refscore = 0;
    ct->edge.othercnt = 0;
    ct->edge.others = NULL;

    RunOnPoints(ct, contour_direction, &search_edge);

    if (ct->edge.refpt == -1)
        RunOnPoints(ct, contour_direction, &search_edge_desperately);
}

/* Apparatus for edge hinting optimization. For given 'others' in ct,
 * it detects 'segments' (in FreeType's sense) and leaves only one point per
 * segment. A segment to which refpt belong is completely removed (refpt is
 * enough).
 *
 * optimize_edge() is the right high-level function to call with instructing
 * context (an edge must be previously initialized with init_edge). It calls
 * optimize_segment() internally - a function that is otherwise unsafe.
 *
 * optimize_blue() is even higher-level function to call before optimize_edge
 * if init_edge() was used to collect points in a blue zone (or other narrow
 * zone).
 *
 * Optimizer keeps points used by diagonal hinter.
 */

/* To be used with qsort() - sorts integer array in ascending order. */
static int sortbynum(const void *a, const void *b) {
    return *(int *)a > *(int *)b;
}

/* Find element's index within an array - return -1 if element not found. */
static int findoffs(const int *elems, int elemcnt, int val) {
    int i;
    for (i=0; i<elemcnt; i++) if (elems[i]==val) return i;
    return -1;
}

/* In given ct, others[segstart...segend] form a continuous segment on an edge
   parallel to one of coordinate axes. If there are no diagonal hints, we can
   instruct just one point of a segment, preferring refpt if included, and
   preferring on-curve points ovef off-curve. Otherwise we must instruct all
   points used by diagonal hinter along with refpt if included. We mark points
   that are not to be instructed as 'affected'.
 */
static void optimize_segment(int segstart, int segend, InstrCt *ct) {
    int i, local_refpt=-1;
    int *others = ct->edge.others;
    int touchflag = (ct->xdir)?tf_x:tf_y;
    int ondiags = 0;

    if (segstart==segend)
return;

    /* purely for aesthetic reasons - can be safely removed. */
    qsort(others+segstart, segend+1-segstart, sizeof(int), sortbynum);

    /* are there any to be used with dstems? */
    if (ct->diagstems)
	for (i=segstart; !ondiags && i<=segend; i++)
	    ondiags = ct->diagpts[others[i]].count;

    if (ondiags) {
	for (i=segstart; i<=segend; i++)
	    ct->affected[others[i]] |= ct->diagpts[others[i]].count?0:touchflag;
    }
    else {
	for (i=segstart; i<=segend && !ct->oncurve[others[i]]; i++);
	if (i<=segend) local_refpt = others[i];

	if (findoffs(others+segstart, segend+1-segstart, ct->edge.refpt) != -1)
	    local_refpt = ct->edge.refpt;

	if (local_refpt==-1) local_refpt = others[segstart];

	for (i=segstart; i<=segend; i++)
	    ct->affected[others[i]] |= local_refpt==others[i]?0:touchflag;
    }
}

/* Subdivide an edge into segments and optimize segments separately.
 * A segment consists oh a point, his neighbours, their neighbours...
 */
static void optimize_edge(InstrCt *ct) {
    int i, p, segstart, next;
    int refpt = ct->edge.refpt;
    int *others = ct->edge.others;
    int othercnt = ct->edge.othercnt;
    int touchflag = (ct->xdir)?tf_x:tf_y;

    if (othercnt == 0)
return;

    /* add edge.refpt to edge.others */
    ct->edge.othercnt = ++othercnt;
    ct->edge.others = others = (int *)grealloc(others, othercnt*sizeof(int));
    others[othercnt-1]=refpt;

    next = 0;
    while (next < othercnt) {
	p = others[segstart = next++];

	while((next < othercnt) && (i = findoffs(others+next, othercnt-next,
				    NextOnContour(ct->contourends, p))) != -1) {
	    p = others[i+=next];
	    others[i] = others[next];
	    others[next++] = p;
	}

	p=others[segstart];

	while((next < othercnt) && (i = findoffs(others+next, othercnt-next,
				    PrevOnContour(ct->contourends, p))) != -1) {
	    p = others[i+=next];
	    others[i] = others[next];
	    others[next++] = p;
	}

	optimize_segment(segstart, next-1, ct);
    }

    for (i=next=0; i<othercnt; i++)
	if (!(ct->affected[others[i]] & touchflag) && (others[i] != refpt))
	    others[next++] = others[i];

    if ((ct->edge.othercnt = next) == 0) {
	free(others);
	ct->edge.others = NULL;
    }
    else /* purely for aesthetic reasons - could be safely removed. */
	qsort(others, ct->edge.othercnt, sizeof(int), sortbynum);
}

/* For any given point on edge, if there exists a path to other point snapped
 * or to-be-snapped in that zone, such that any points on this path are within
 * that zone, then this given point may be optimized out.
 */
static void optimize_blue(InstrCt *ct) {
    int i, j, curr;
    int *others = ct->edge.others;
    int othercnt = ct->edge.othercnt;
    int touchflag = (ct->xdir)?tf_x:tf_y;
    int *contourends = ct->contourends;
    uint8 *touched = ct->touched;
    uint8 *affected = ct->affected;
    uint8 *tosnap;

    if (othercnt == 0)
return;

    tosnap = (uint8 *)calloc(ct->ptcnt, sizeof(uint8));

    for(i=0; i<ct->edge.othercnt; i++)
    {
	if (ct->diagpts && ct->diagpts[others[i]].count) continue;

	/* check path forward */
	curr=NextOnContour(contourends, others[i]);
	while(curr!=others[i]) {
	    double coord = (ct->xdir) ? ct->bp[curr].x : ct->bp[curr].y;
	    if (fabs(ct->edge.base - coord) > ct->gic->fudge) break;
	    if ((touched[curr] | affected[curr]) & touchflag || tosnap[curr]) {
		affected[others[i]] |= touchflag;
		break;
	    }
	    curr=NextOnContour(contourends, curr);
	}

	if (affected[others[i]] & touchflag) continue;

	/* check path backward */
	curr=PrevOnContour(contourends, others[i]);
	while(curr!=others[i]) {
	    double coord = (ct->xdir) ? ct->bp[curr].x : ct->bp[curr].y;
	    if (fabs(ct->edge.base - coord) > ct->gic->fudge) break;
	    if ((touched[curr] | affected[curr]) & touchflag || tosnap[curr]) {
		affected[others[i]] |= touchflag;
		break;
	    }
	    curr=PrevOnContour(contourends, curr);
	}

	if (!affected[others[i]] & touchflag) tosnap[others[i]] = 1;
    }

    free(tosnap);

    /* remove optimized-out points from list to be instructed. */
    for(i=0; i<ct->edge.othercnt; i++)
	if (affected[others[i]]) {
	    ct->edge.othercnt--;
	    for(j=i; j<ct->edge.othercnt; j++) others[j] = others[j+1];
	    i--;
	}
}

/* Finish instructing the edge. Try to hint only those points on edge that are
 * necessary - IUP should do the rest.
 */
static void finish_edge(InstrCt *ct, uint8 command) {
    int i;

    optimize_edge(ct);
    if (ct->edge.othercnt==0)
return;

    ct->pt=instructpoints(ct->pt, ct->edge.othercnt, ct->edge.others, command);
    for(i=0; i<ct->edge.othercnt; i++)
	ct->touched[ct->edge.others[i]] |= (ct->xdir?tf_x:tf_y);

    free(ct->edge.others);
    ct->edge.others=NULL;
    ct->edge.othercnt = 0;
}

/******************************************************************************
 *
 * Routines for hinting single stems.
 *
 ******************************************************************************/

/* Each stem hint has 'startdone' and 'enddone' flag, indicating whether 'start'
 * or 'end' edge is hinted or not. This functions marks as done all edges at
 * specified coordinate, starting from given hint (hints sometimes share edges).
 */
static void mark_startenddones(StemInfo *hint, double value, double fudge) {
    StemInfo *h;

    for (h=hint; h!=NULL; h=h->next) {
        if (fabs(h->start - value) <= fudge) h->startdone = true;
        if (fabs(h->start+h->width - value) <= fudge) h->enddone = true;
    }
}

/* Given the refpt for one of this hint's edges is already positioned, this
 * function aligns 'others' (SHP with given shp_rp) for this edge and positions
 * the second edge, optionally setting its refpt as rp0. It frees edge.others
 * and sets edge.othercnt to zero, but it leaves edge.refpt set to last
 * instructed edge.
 */
#define use_rp1 (true)
#define use_rp2 (false)
#define set_new_rp0 (true)
#define keep_old_rp0 (false)
static void finish_stem(StemInfo *hint, int shp_rp1, int chg_rp0, InstrCt *ct)
{
    int i, callargs[5];
    int EM = ct->gic->sf->ascent + ct->gic->sf->descent;
    real coord = ct->edge.base;
    StdStem *StdW = ct->xdir?&(ct->gic->stdvw):&(ct->gic->stdhw);
    StdStem *ClosestStem;
    StdStem stem;

    if (hint == NULL)
return;

    ClosestStem = CVTSeekStem(ct->xdir, ct->gic, hint->width, true);
    ct->touched[ct->edge.refpt] |= ct->xdir?tf_x:tf_y;
    finish_edge(ct, shp_rp1?SHP_rp1:SHP_rp2);
    mark_startenddones(ct->xdir?ct->sc->vstem:ct->sc->hstem, coord, ct->gic->fudge);

    if (!ct->xdir && hint->ghost && ((hint->width==20) || (hint->width==21))) {
        hint->startdone = hint->enddone = 1;
return;
    }

    if (fabs(hint->start - coord) < hint->width) {
        if (hint->ghost) coord = hint->start - hint->width;
	else coord = hint->start + hint->width;
    }
    else coord = hint->start;

    init_edge(ct, coord, ALL_CONTOURS);
    if (ct->edge.refpt == -1)
return;

    if (ClosestStem != NULL) {
	ct->pt = push2nums(ct->pt, ct->edge.refpt, ClosestStem->cvtindex);

	if (ct->gic->cvt_done && ct->gic->fpgm_done && ct->gic->prep_done)
	    *(ct->pt)++ = chg_rp0?MIRP_rp0_min_black:MIRP_min_black;
	else *(ct->pt)++ = chg_rp0?MIRP_min_rnd_black:MIRP_rp0_min_rnd_black;
    }
    else {
	if (ct->gic->cvt_done && ct->gic->fpgm_done && ct->gic->prep_done &&
	    StdW->width!=-1)
	{
	    stem.width = (int)rint(fabs(hint->width));
	    stem.stopat = 32767;
	    stem.snapto =
	        CVTSeekStem(ct->xdir, ct->gic, hint->width, false);

            for (i=7; i<32768; i++) {
	        int width_parent = compute_stem_width(ct->xdir, stem.snapto, EM, i);
		int width_me = compute_stem_width(ct->xdir, &stem, EM, i);

		if (width_parent != width_me) {
		    stem.stopat = i;
		    break;
		}
	    }

	    callargs[0] = ct->edge.refpt;
	    callargs[1] = stem.snapto->cvtindex;
	    callargs[2] = chg_rp0?1:0;
	    callargs[3] = stem.stopat;
	    callargs[4] = 4;
	    ct->pt = pushnums(ct->pt, 5, callargs);
	    *(ct->pt)++ = CALL;
	}
	else {
	    ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	    *(ct->pt)++ = chg_rp0?MDRP_rp0_min_rnd_black:MDRP_min_rnd_black;
	}
    }

    if (chg_rp0) ct->rp0 = ct->edge.refpt;
    ct->touched[ct->edge.refpt] |= ct->xdir?tf_x:tf_y;
    finish_edge(ct, SHP_rp2);
    mark_startenddones(ct->xdir?ct->sc->vstem:ct->sc->hstem, coord, ct->gic->fudge);
}

/******************************************************************************
 *
 * I decided to do snapping to blues at the very beginning of the instructing.
 *
 * Blues are processed in certain (important) order: baseline, descenders
 * (from deeper to shorter), ascenders (from taller to shorter).
 *
 * For each blue, one of the edges is put into CVT: lower if is't > zero,
 * the upper otherwise. A twilight point 0 is established at this height. All
 * the glyph's points decided to be worth snapping are then moved relative to
 * this twilight point, being subject to rounding 'down-to-int'. Space taken
 * is at most 8*ptcnt.
 *
 * For each blue, all yet unprocessed HStems affected are instructed. Ghost
 * hints are reckognised. If there is at least one stem hint in given blue zone,
 * autoinstructor will seek for other interesting features, so there is no need
 * to hint them explicitly.
 *
 * TODO! We have to check whether we have to instruct hints that overlaps with
 * those affected by blues.
 *
 * Important notes:
 *
 * The zone count must be set to 2, the twilight point count must be nonzero.
 * This is done automagically in init_maxp(), otherwise this method wouldn't
 * work at all. Currently there is only one twilight point used, but there
 * may be needed one or even two points per blue zone if some advanced snapping
 * and counter managing is to be done.
 *
 * Snapping relies on function 0 in FPGM, see init_fpgm().
 *
 * Using MIAP (single cvt, relying on cut-in) instead of twilight points
 * causes overshoots to appear/disappear inconsistently at small pixel sizes.
 * This flickering is disastrous to soft, wavy horizontal lines. We could use
 * any glyph's point at needed height, but we'r not certain we'll find any.
 *
 * The inner (leftwards) contours aren't snapped to the blue zone.
 * This could have created weird artifacts. Of course this will fail for
 * glyphs with wrong direction, but I won't handle it for now.
 *
 * TODO! Remind the user to correct direction or do it for him.
 * TODO! Try to instruct 'free points' with single push and LOOPCALL.
 *
 * If we didn't snapped any point to a blue zone, we shouldn't mark any HStem
 * edges done. This could made some important points on inner contours missed.
 *
 ******************************************************************************/

/* Each blue zone has two TTF point indices associated with it: 'highest' and
 * 'lowest'. These have to be points with highest and lowest Y coordinates that
 * are snapped to that blue zone (directly or by horizontal stem). Currently
 * we register only edge.refpt. These points are later to be used for horizontal
 * stems' positioning.
 */
static void update_blue_pts(int blueindex, InstrCt *ct)
{
    BasePoint *bp = ct->bp;
    BlueZone *blues = ct->gic->blues;

    if (ct->edge.refpt == -1)
return;

    if (blues[blueindex].highest == -1 ||
        bp[ct->edge.refpt].y > bp[blues[blueindex].highest].y)
            blues[blueindex].highest = ct->edge.refpt;

    if (blues[blueindex].lowest == -1 ||
        bp[ct->edge.refpt].y < bp[blues[blueindex].lowest].y)
            blues[blueindex].lowest = ct->edge.refpt;
}

/* It is theoretically possible that 'highest' and 'lowest' points of neighbour
 * blue zones overlap, and thus may spoil horizontal stems' positioning.
 * Here we fix this up.
 */
static void fixup_blue_pts(BlueZone *b1, BlueZone *b2) {
    if (b1->lowest > b2->lowest) b1->lowest = b2->lowest;
    if (b1->highest < b2->highest) b1->highest = b2->highest;
}

static void check_blue_pts(BlueZone *blues, int bluecnt) {
    int i, j, l;

    for (i=0; i<bluecnt-1; i++) {
        if (blues[i].lowest == -1) // this implies highest==-1, no pts attached
    continue;
        else if (SegmentsOverlap(blues[i].lowest, blues[i].highest,
	                         blues[i+1].lowest, blues[i+1].highest))
	{
	    fixup_blue_pts(blues+i, blues+i+1);

	    for (j=i+2; j<bluecnt; j++) {
	        if (blues[j].lowest==-1)
	    continue;
	        else if (SegmentsOverlap(blues[i].lowest, blues[i].highest,
	            blues[j].lowest, blues[j].highest))
		        fixup_blue_pts(blues+i, blues+j);
		else
	    break;
	    }

            l=j;

	    for(j=i+1; j<l; j++) {
	        if (blues[j].lowest == -1)
	    continue;
	        blues[j].lowest = blues[i].lowest;
		blues[j].highest = blues[i].highest;
	    }
	}
    }
}

/* Snap stems and perhaps also some other points to given bluezone and set up
 * its 'highest' and 'lowest' point indices.
 */
static void snap_to_blues(InstrCt *ct) {
    int i, j, cvt;
    int therewerestems;      /* were there any HStems snapped to this blue? */
    StemInfo *hint;          /* for HStems affected by blues */
    real base, advance, tmp; /* for the hint */
    int callargs[3] = { 0/*pt*/, 0/*cvt*/, 0 };
    real fudge;
    int bluecnt=ct->gic->bluecnt;
    int queue[12];           /* Blue zones' indices in processing order */
    BlueZone *blues = ct->gic->blues;
    real fuzz = GetBlueFuzz(ct->gic->sf);

    if (bluecnt == 0)
return;

    /* Fill the processing queue - baseline goes first, then botton zones */
    /* sorted by base in ascending order, then top zones sorted in descending */
    /* order. I assume the blues are sorted in ascending order first. */
    for (i=0; (i < bluecnt) && (blues[i].base < 0); i++);
    queue[0] = i;
    for (i=0; i<queue[0]; i++) queue[i+1] = i;
    for (i=queue[0]+1; i<bluecnt; i++) queue[i] = bluecnt - i + queue[0];

    /* Process the blues. */
    for (i=0; i<bluecnt; i++) {
	therewerestems = 0;
	cvt = callargs[1] = blues[queue[i]].cvtindex;

	/* Process all hints with edges within ccurrent blue zone. */
	for ( hint=ct->sc->hstem; hint!=NULL; hint=hint->next ) {
	    if (hint->startdone || hint->enddone) continue;

	    /* Which edge to start at? */
	    /*Starting at the other would usually be wrong. */
	    if (blues[queue[i]].overshoot < blues[queue[i]].base ||
	        (hint->ghost && hint->width == 21))
	    {
		base = hint->start;
		advance = hint->start + hint->width;
	    }
	    else {
		base = hint->start + hint->width;
		advance = hint->start;
	    }

	    /* This is intended as a fallback if the base edge wasn't within
	     * this bluezone, and advance was. This seems a bit controversial.
	     * For now, I keep this turned on.
	     */
	    if (!SegmentsOverlap(base+fuzz, base-fuzz,
		blues[queue[i]].base, blues[queue[i]].overshoot))
	    {
		tmp = base;
		base = advance;
		advance = tmp;

		if (!SegmentsOverlap(base+fuzz, base-fuzz,
		    blues[queue[i]].base, blues[queue[i]].overshoot)) continue;

		/* ghost hints need the right edge to be snapped */
		if (hint->ghost && ((hint->width == 20) || (hint->width == 21))) {
		    tmp = base;
		    base = advance;
		    advance = tmp;
		}
	    }

	    /* instruct the stem */
	    init_edge(ct, base, ALL_CONTOURS); /* stems don't care */
	    if (ct->edge.refpt == -1) continue;
	    update_blue_pts(queue[i], ct);
	    callargs[0] = ct->rp0 = ct->edge.refpt;

	    if (ct->gic->fpgm_done) {
	        ct->pt = pushpoints(ct->pt, 3, callargs);
	        *(ct->pt)++ = CALL;
	    }
	    else {
	        ct->pt = pushpoints(ct->pt, 2, callargs);
	        *(ct->pt)++ = MIAP_rnd;
	    }

	    finish_stem(hint, use_rp1, keep_old_rp0, ct);
	    update_blue_pts(queue[i], ct); /* this uses only refpt: who cares?*/
	    therewerestems = 1;

	    /* TODO! It might be worth to instruct at least one edge of each */
	    /* hint overlapping with currently processed. This would preserve */
	    /* relative hint placement in some difficult areas. */
	}

	/* Now I'll try to find points not snapped by any previous stem hint. */
	if (therewerestems) {
	    base = (blues[queue[i]].base + blues[queue[i]].overshoot) / 2.0;
	    fudge = ct->gic->fudge;
	    ct->gic->fudge = fabs(base - blues[queue[i]].base) + fuzz;
	    init_edge(ct, base, EXTERNAL_CONTOURS);
	    optimize_blue(ct);
	    optimize_edge(ct);

	    if (ct->edge.refpt == -1) {
		ct->gic->fudge = fudge;
		continue;
	    }

	    if (!(ct->touched[ct->edge.refpt]&tf_y || ct->affected[ct->edge.refpt]&tf_y)) {
		callargs[0] = ct->rp0 = ct->edge.refpt;

		if (ct->gic->fpgm_done) {
		  ct->pt = pushpoints(ct->pt, 3, callargs);
		  *(ct->pt)++ = CALL;
		}
		else {
		  ct->pt = pushpoints(ct->pt, 2, callargs);
		  *(ct->pt)++ = MIAP_rnd;
		}

		ct->touched[ct->edge.refpt] |= tf_y;
	    }

	    for (j=0; j<ct->edge.othercnt; j++) {
		callargs[0] = ct->rp0 = ct->edge.others[j];

		if (ct->gic->fpgm_done) {
		  ct->pt = pushpoints(ct->pt, 3, callargs);
		  *(ct->pt)++ = CALL;
		}
		else {
		  ct->pt = pushpoints(ct->pt, 2, callargs);
		  *(ct->pt)++ = MIAP_rnd;
		}

		ct->touched[ct->edge.others[j]] |= tf_y;
	    }

	    update_blue_pts(queue[i], ct);

	    if (ct->edge.othercnt > 0) {
		free(ct->edge.others);
		ct->edge.others = NULL;
		ct->edge.othercnt = 0;
	    }

	    ct->gic->fudge = fudge;
	}
    }

    check_blue_pts(blues, bluecnt);
}

/******************************************************************************
 *
 * High-level functions for instructing horizontal and vertical stems.
 * Both will use 'geninstrs' for positioning single stems.
 *
 ******************************************************************************/

/* Is this the first stem in a group of overlapping stems? */
static int first_in_group(StemInfo *firsthint, StemInfo *hint)
{
    StemInfo *lasthint, *testhint = firsthint;

    for (; testhint!=NULL && testhint!=hint; testhint = testhint->next)
    {
	if (SegmentsOverlap(hint->start, hint->start+testhint->width,
	                  testhint->start, testhint->start+hint->width))
	{
	    lasthint = testhint;
	    break;
	}
    }

    return !hint->hasconflicts && lasthint==NULL;
}

/* Find points that should be snapped to this hint's edges with init_edge().
 * It will return two types of points per edge: a 'chosen one' that should be
 * used as a reference for this hint, and 'others' to position after it with
 * SHP[].
 *
 * In fact, it is enough to position the reference point for the first edge
 * (if not already done) and then call finish_stem().
 *
 * If none of the edges is positioned:
 *   If this hint is the first, previously overlapped, or simply horizontal,
 *   position the reference point at the base where it is using MDAP; otherwise
 *   position the hint's base rp0 relatively to the previous hint's end using
 *   MDRP with white minimum distance (fpgm function 1).
 */
static void geninstrs(InstrCt *ct, StemInfo *hint) {
    real hbase, base, width, hend, stdwidth;
    int first; /* is this the first stem of overlapping stems' cluster? */

    /* if this hint has conflicts don't try to establish a minimum distance */
    /* between it and the last stem, there might not be one */
    StemInfo *firsthint = ct->xdir ? ct->sc->vstem : ct->sc->hstem;
    first = first_in_group(firsthint, hint);

    /* Check whether to use CVT value or shift the stuff directly */
    stdwidth = ct->xdir?ct->gic->stdvw.width:ct->gic->stdhw.width;
    hbase = base = hint->start;
    width = hint->width;
    hend = hbase + width;

    /* flip the hint if needed */
    if (hint->enddone) {
	hbase = (base + width);
	width = -width;
	hend = base;
    }

    if (hint->startdone || hint->enddone) {
	/* Set a reference point on the base edge. */
	init_edge(ct, hbase, ALL_CONTOURS);
	if (ct->edge.refpt == -1) return;

	if (ct->rp0 != ct->edge.refpt) {
	    ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	    *(ct->pt)++ = MDAP; /* sets rp0 and rp1 */
	    ct->rp0 = ct->edge.refpt;
	}

	finish_stem(hint, use_rp1, !hint->hasconflicts, ct);

	if (hint->startdone) {
	    ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	    *(ct->pt)++ = SRP0;
	    ct->rp0 = ct->edge.refpt;
	}
    }
    else {
	if (!ct->xdir) { /* will be simply put in place, just rounded */
	    /* a primitive way to minimize rounding errors */
	    if (fabs(hend) > fabs(hbase)) {
		hbase = (base + width);
		width = -width;
		hend = base;
	    }
	}

        /* Now I must place the stem's origin in respect to others... */
	/* TODO! What's really needed here is an iterative procedure that */
	/* would preserve counters and widths, like in freetype2. */
	/* For horizontal stems, interpolating between blues MUST be done. */

	init_edge(ct, hbase, ALL_CONTOURS);
	if (ct->edge.refpt == -1) return;
	else ct->rp0 = ct->edge.refpt; /* not yet rp0. */

	if (!first && hint->hasconflicts) {
	    ct->pt = pushpoint(ct->pt, ct->rp0);
	    *(ct->pt)++ = MDRP_rp0_rnd_white;
	    finish_stem(hint, use_rp2, !hint->hasconflicts, ct);
	}
	else if (!ct->xdir) {
	    ct->pt = pushpoint(ct->pt, ct->rp0);
	    *(ct->pt)++ = MDAP_rnd;
	    finish_stem(hint, use_rp1, !hint->hasconflicts, ct);
	}
	else if (hint == firsthint) {
	    ct->pt = pushpoint(ct->pt, ct->rp0);
	    *(ct->pt)++ = MDRP_rp0_rnd_white;
	    finish_stem(hint, use_rp2, !hint->hasconflicts, ct);
	}
	else {
            if (ct->gic->fpgm_done) {
	        ct->pt = push2nums(ct->pt, ct->rp0, 1);
	        *(ct->pt)++ = CALL;
	    }
	    else {
	        ct->pt = pushpoint(ct->pt, ct->rp0);
	        *(ct->pt)++ = MDRP_rp0_min_rnd_white;
	    }

	    finish_stem(hint, use_rp2, !hint->hasconflicts, ct);
	}
    }
}

/* High-level function for instructing horizontal stems.
 *
 * It is assumed that blues (and hstems associated with them) are already
 * done so that remaining stems can be interpolated between them.
 *
 * TODO! CJK hinting will probably need different function (HStemGeninstCJK?)
 * TODO! Instruct top and bottom bearings for fonts which have them.
 * TODO! Support for ghost hints not tied to any alignment zone.
 */
static void HStemGeninst(InstrCt *ct) {
    BlueZone *blues = ct->gic->blues;
    int bluecnt = ct->gic->bluecnt;
    BasePoint *bp = ct->bp;
    StemInfo *hint;
    int i, rp1, rp2;
    double hbase, hend;
    int mdrp_end, mdrp_base, ip_base;
    int callargs[4];

    for ( hint=ct->sc->hstem; hint!=NULL; hint=hint->next )
    {
	if (!hint->startdone && !hint->enddone &&
	    (!hint->hasconflicts || first_in_group(ct->sc->hstem, hint)))
	{
	    /* Set up upper edge (hend) and lower edge (hbase). */
	    if (hint->ghost)
	    {
	        if (hint->width == 21 || hint->width == 20)
		    continue; //should be in a blue zone & done earlier (?)

		hend = hint->start;
		hbase = hend - hint->width;
	    }
	    else {
	        hbase = hint->start;
		hend = hbase + hint->width;
	    }

	    /* Find two points to interpolate the HStem between.
	       rp1 = lower, rp2 = upper. */
	    rp1 = -1;
	    rp2 = -1;

	    for (i=0; i<bluecnt; i++) {
	        if (blues[i].lowest == -1) // implies blues[i].highest==-1 too
	            continue;

		if (bp[blues[i].lowest].y < hbase)
		    if (rp1==-1 || bp[rp1].y < bp[blues[i].lowest].y)
		        rp1=blues[i].lowest;

		if (bp[blues[i].highest].y > hend)
		    if (rp2==-1 || bp[rp2].y > bp[blues[i].highest].y)
		        rp2=blues[i].highest;
	    }

	    /* Reference points not found? Fall back to old method. */
	    if (rp1==-1 || rp2==-1) {
		geninstrs(ct,hint);
		continue;
	    }

	    /* Align the stem relatively to rp0 and rp1. */
	    mdrp_end = fabs(bp[rp2].y - hbase) < 0.2*fabs(bp[rp2].y - bp[rp1].y);
	    mdrp_base = fabs(bp[rp1].y - hend) < 0.2*fabs(bp[rp2].y - bp[rp1].y);

	    if (mdrp_end || mdrp_base) {
		if (mdrp_end) init_edge(ct, hend, ALL_CONTOURS);
		else init_edge(ct, hbase, ALL_CONTOURS);

		if (ct->edge.refpt == -1) continue;

		if (mdrp_end) ct->pt = push2points(ct->pt, ct->edge.refpt, rp2);
		else ct->pt = push2points(ct->pt, ct->edge.refpt, rp1);

		*(ct->pt)++ = SRP0;
		*(ct->pt)++ = DUP;
		*(ct->pt)++ = MDRP_grey;
		*(ct->pt)++ = MDAP_rnd;
	    }
	    else {
		ip_base = fabs(bp[rp2].y - hend) < fabs(bp[rp1].y - hbase);

		if (ip_base) init_edge(ct, hbase, ALL_CONTOURS);
		else init_edge(ct, hend, ALL_CONTOURS);

		if (ct->edge.refpt == -1) continue;

		callargs[0] = ct->edge.refpt;
		callargs[1] = rp1;
		callargs[2] = rp2;
		callargs[3] = 8;
		ct->pt = pushnums(ct->pt, 4, callargs);
		*(ct->pt)++ = CALL;
	    }

	    ct->rp0 = ct->edge.refpt;
	    finish_stem(hint, use_rp1, keep_old_rp0, ct);
	}
	else if (!hint->startdone || !hint->enddone)
	    geninstrs(ct,hint);
    }
}

/*
 * High-level function for instructing vertical stems.
 *
 * TODO! CJK hinting may need different function (VStemGeninstCJK?)
 * TODO! Support for vertical ghost hints.
 */
static void VStemGeninst(InstrCt *ct) {
    StemInfo *hint;

    if (ct->rp0 != ct->ptcnt) {
	ct->pt = pushpoint(ct->pt, ct->ptcnt);
	*(ct->pt)++ = SRP0;
	ct->rp0 = ct->ptcnt;
    }

    for ( hint=ct->sc->vstem; hint!=NULL; hint=hint->next )
	if ( !hint->startdone || !hint->enddone )
	    geninstrs(ct,hint);

    /* instruct right sidebearing */
    if (ct->sc->width != 0) {
	ct->pt = pushpoint(ct->pt, ct->ptcnt+1);
	*(ct->pt)++ = DUP;
	*(ct->pt)++ = MDRP_min_white;
	*(ct->pt)++ = MDAP_rnd;
	ct->rp0 = ct->ptcnt+1;
    }
}

/******************************************************************************
 *
 * Everything related with diagonal hinting goes here
 *
 ******************************************************************************/

#define DIAG_MIN_DISTANCE   (0.84375)

/* Takes a line defined by two points and returns a vector decribed as a
/* pair of x and y values, such that the value (x2 + y2) is equal to 1.
/* Note that the BasePoint structure is used to store the vector, although
/* it is not a point itself. This is just because that structure has "x"
/* and "y" fields which can be used for our purpose. */
static BasePoint GetVector ( BasePoint *top,BasePoint *bottom,int orth ) {
    real catx, caty, hip, temp;
    BasePoint ret;

    catx = top->x - bottom->x; caty = top->y - bottom->y;
    hip = sqrt(( catx*catx ) + ( caty*caty ));
    ret.y = caty/hip; ret.x = catx/hip;

    if( orth ) {
        temp = ret.x; ret.x = -ret.y; ret.y = temp;
    }
return( ret );
}

static void SetDStemKeyPoint( InstrCt *ct,struct glyphdata *gd,BasePoint *stemunit,
    DStem *ds,struct pointdata *pd,int aindex ) {
    int ptindex, nextindex, prev, prevdot, is_start;

    ptindex = pd->sp->ttfindex;
    nextindex = pd->sp->nextcpindex;
    prev = false;
    is_start = ( aindex == 0 || aindex == 2 );
    if ( ptindex >= gd->realcnt ) {
        prevdot =   ( pd->prevunit.x * stemunit->x ) +
                    ( pd->prevunit.y * stemunit->y );
        prev = (( is_start && prevdot < 0 ) || ( !is_start && prevdot > 0 ));
        ptindex = ( prev ) ? nextindex - 1 : nextindex;
    }
    ds->pts[aindex].num = ptindex;
    ds->pts[aindex].pt = &ct->bp[ptindex];
}

static DiagPointInfo *AssignLineToPoint( DiagPointInfo *diagpts,DStem *ds,int idx,int is_l ) {
    int num, base;

    num = diagpts[idx].count;
    base = ( is_l ) ? 0 : 2;

    diagpts[idx].line[num] = chunkalloc( sizeof( struct pointvector ));
    diagpts[idx].line[num]->pt1 = &ds->pts[base];
    diagpts[idx].line[num]->pt2 = &ds->pts[base+1];
    diagpts[idx].line[num]->done = false;
    diagpts[idx].count++;
return( diagpts );
}

/* Convert the existing diagonal stem layout to glyph data, containing
/* information about points assigned to each stem. Then run on stem chunks
/* and associate with each point the line it should be aligned by. Note that
/* we have to do this on a relatively early stage, as it may be important
/* to know, if the given point is subject to the subsequent diagonale hinting,
/* before any actual processing of diagonal stems is started.*/
static DiagPointInfo *InitDStemData( InstrCt *ct ) {
    DStem *head, *newhead;
    DiagPointInfo *diagpts;
    int i, j, idx, num1, num2, is_l;
    struct glyphdata *gd;
    struct stemdata *sd;
    struct stem_chunk *chunk;
    double em_size = ct->sc->parent->ascent + ct->sc->parent->descent;

    diagpts = gcalloc( ct->ptcnt, sizeof( struct diagpointinfo ));
    gd = GlyphDataInit( ct->sc,em_size,false );
    if ( ct->sc->dstem == NULL || gd == NULL )
return( diagpts );
    DStemInfoToStemData( gd,ct->sc->dstem );

    head = newhead = NULL;
    for ( i = gd->stemcnt-1; i >= 0; i-- ) {
        sd = &gd->stems[i];
	if ( sd->toobig || sd->len==0 || sd->activecnt >= sd->chunk_cnt )
    continue;
        if (( sd->unit.y > -.05 && sd->unit.y < .05 ) ||
            ( sd->unit.x > -.05 && sd->unit.x < .05 ))
    continue;
	if ( sd->lpcnt < 2 || sd->rpcnt < 2 )
    continue;

        newhead = chunkalloc( sizeof( DStem ));
        newhead->width = sd->width;
        newhead->done = false;
        for ( j=0; j<sd->chunk_cnt; j++ ) {
            chunk = &sd->chunks[j];
            /* Swap "left" and "right" sides for vectors pointing north-east,
            /* so that the "left" side is always determined along the x axis
            /* rather than relatively to the vector direction */
            num1 = ( sd->unit.y > 0 ) ? 0 : 2;
            num2 = ( sd->unit.y > 0 ) ? 2 : 0;
            if ( chunk->l != NULL && newhead->pts[num1].pt == NULL )
                SetDStemKeyPoint( ct,gd,&sd->unit,newhead,chunk->l,num1 );
            if ( chunk->r != NULL && newhead->pts[num2].pt == NULL )
                SetDStemKeyPoint( ct,gd,&sd->unit,newhead,chunk->r,num2 );
            if ( newhead->pts[num1].pt != NULL && newhead->pts[num2].pt != NULL )
        break;
        }
        for ( j=sd->chunk_cnt-1; j>=0; j-- ) {
            chunk = &sd->chunks[j];
            num1 = ( sd->unit.y > 0 ) ? 1 : 3;
            num2 = ( sd->unit.y > 0 ) ? 3 : 1;
            if ( chunk->l != NULL && newhead->pts[num1].pt == NULL )
                SetDStemKeyPoint( ct,gd,&sd->unit,newhead,chunk->l,num1 );
            if ( chunk->r != NULL && newhead->pts[num2].pt == NULL )
                SetDStemKeyPoint( ct,gd,&sd->unit,newhead,chunk->r,num2 );
            if ( newhead->pts[num1].pt != NULL && newhead->pts[num2].pt != NULL )
        break;
        }

        for ( j=0; j<gd->pcnt; ++j ) if ( gd->points[j].sp!=NULL )
	    gd->points[j].sp->ticked = false;
        for ( j=0; j<sd->chunk_cnt; j++ ) {
            chunk = &sd->chunks[j];
            is_l = ( sd->unit.y > 0 );
            if ( chunk->l != NULL && !chunk->l->sp->ticked ) {
                idx = chunk->l->sp->ttfindex;
                if ( idx < gd->realcnt ) {
                    if ( diagpts[idx].count < 2 )
                        diagpts = AssignLineToPoint( diagpts,newhead,idx,is_l );
                } else {
                    idx = chunk->l->sp->nextcpindex;
                    if ( diagpts[idx-1].count < 2 )
                        diagpts = AssignLineToPoint( diagpts,newhead,idx-1,is_l );
                    if ( diagpts[idx+1].count < 2 )
                        diagpts = AssignLineToPoint( diagpts,newhead,idx,is_l );
                }
                chunk->l->sp->ticked = true;
            }
            is_l = ( sd->unit.y < 0 );
            if ( chunk->r != NULL && !chunk->r->sp->ticked ) {
                idx = chunk->r->sp->ttfindex;
                if ( idx < gd->realcnt ) {
                    if ( diagpts[idx].count < 2 )
                        diagpts = AssignLineToPoint( diagpts,newhead,idx,is_l );
                } else {
                    idx = chunk->r->sp->nextcpindex;
                    if ( diagpts[idx-1].count < 2 )
                        diagpts = AssignLineToPoint( diagpts,newhead,idx-1,is_l );
                    if ( diagpts[idx].count < 2 )
                        diagpts = AssignLineToPoint( diagpts,newhead,idx,is_l );
                }
                chunk->r->sp->ticked = true;
            }
        }
        newhead->next = head;
        head = newhead;
    }
    GlyphDataFree( gd );
    ct->diagstems = head;
return( diagpts );
}

/* Usually we have to start doing each diagonal stem from the point which
/* is most touched in any directions. */
static int FindDiagStartPoint( DStem *ds, uint8 *touched ) {
    int i;

    for ( i=0; i<4; ++i ) {
        if (( touched[ds->pts[i].num] & tf_x ) && ( touched[ds->pts[i].num] & tf_y ))
return( i );
    }

    for ( i=0; i<4; ++i ) {
        if ( touched[ds->pts[i].num] & tf_y )
return( i );
    }

    for ( i=0; i<4; ++i ) {
        if ( touched[ds->pts[i].num] & tf_x )
return( i );
    }
return( 0 );
}

/* Check the directions at which the given point still can be moved
/* (i. e. has not yet been touched) and set freedom vector to that
/* direction in case it has not already been set */
static int SetFreedomVector( uint8 **instrs,int pnum,int ptcnt,
    uint8 *touched,DiagPointInfo *diagpts,
    NumberedPoint *lp1,NumberedPoint *lp2,BasePoint *fv ) {

    int i, pushpts[2];
    NumberedPoint *start=NULL, *end=NULL;
    BasePoint newfv;

    if (( touched[pnum] & tf_d ) && !( touched[pnum] & tf_x ) && !( touched[pnum] & tf_y )) {
        for( i=0 ; i<diagpts[pnum].count ; i++) {
            if ( diagpts[pnum].line[i]->done ) {
                start = diagpts[pnum].line[i]->pt1;
                end = diagpts[pnum].line[i]->pt2;
            }
        }

        /* This should never happen */
        if ( start == NULL || end == NULL )
return( false );

        newfv = GetVector( start->pt,end->pt,false );
        if ( !UnitsParallel( fv,&newfv,true )) {
            fv->x = newfv.x; fv->y = newfv.y;

            pushpts[0] = start->num; pushpts[1] = end->num;
            *instrs = pushpoints( *instrs,2,pushpts );
            *(*instrs)++ = 0x08;       /*SFVTL[parallel]*/
        }

return( true );

    } else if ( (touched[pnum] & tf_x) && !(touched[pnum] & tf_d) && !(touched[pnum] & tf_y)) {
        if (!( fv->x == 0 && fv->y == 1 )) {
            fv->x = 0; fv->y = 1;
            *(*instrs)++ = 0x04;   /*SFVTCA[y]*/
        }
return( true );

    } else if ( (touched[pnum] & tf_y) && !(touched[pnum] & tf_d) && !(touched[pnum] & tf_x)) {
        if (!( fv->x == 1 && fv->y == 0 )) {
            *(*instrs)++ = 0x05;   /*SFVTCA[x]*/
            fv->x = 1; fv->y = 0;
        }
return ( true );
    } else if ( !(touched[pnum] & (tf_x|tf_y|tf_d))) {
        newfv = GetVector( lp1->pt,lp2->pt,true );
        if ( !UnitsParallel( fv,&newfv,true )) {
            fv->x = newfv.x; fv->y = newfv.y;

            *(*instrs)++ = 0x0E;       /*SFVTPV*/
        }
return( true );
    }
return( false );
}

static int MarkLineFinished( int pnum,int startnum,int endnum,DiagPointInfo *diagpts ) {
    int i;

    for ( i=0; i<diagpts[pnum].count; i++ ) {
        if (( diagpts[pnum].line[i]->pt1->num == startnum ) &&
            ( diagpts[pnum].line[i]->pt2->num == endnum )) {

            diagpts[pnum].line[i]->done = true;
return( true );
        }
    }
return( false );
}

/* A basic algorith for hinting diagonal stems:
/* -- iterate through diagonal stems, ordered from left to right;
/* -- for each stem, find the most touched point, to start from,
/*    and fix that point. TODO: the positioning should be done
/*    relatively to points already touched by x or y;
/* -- position the second point on the same edge, using dual projection
/*    vector;
/* -- link to the second edge and repeat the same operation.
/*
/* For each point we first determine a direction at which it still can
/* be moved. If a point has already been positioned relatively to another
/* diagonal line, then we move it along that diagonale. Thus this algorithm
/* can handle things like "V" where one line's ending point is another
/* line's starting point without special exceptions. */
static uint8 *FixDstem( InstrCt *ct, DStem *ds, DiagPointInfo *diagpts, BasePoint *fv) {
    int startnum, a1, a2, b1, b2, ptcnt;
    NumberedPoint *v1, *v2;
    real distance;
    uint8 *instrs, *touched;
    int pushpts[2];
    int lfixed=false, rfixed=false;

    if ( ds->done )
return( ct->pt );

    ptcnt = ct->ptcnt;
    touched = ct->touched;
    instrs = ct->pt;

    startnum = FindDiagStartPoint( ds,touched );
    a1 = ds->pts[startnum].num;
    if (( startnum == 0 ) || ( startnum == 1 )) {
        v1 = &ds->pts[0]; v2 = &ds->pts[1];
        a2 = ( startnum == 1 ) ? ds->pts[0].num : ds->pts[1].num;
        b1 = ds->pts[2].num; b2 = ds->pts[3].num;
    } else {
        v1 = &ds->pts[2]; v2 = &ds->pts[3];
        a2 = ( startnum == 3 ) ? ds->pts[2].num : ds->pts[3].num;
        b1 = ds->pts[0].num; b2 = ds->pts[1].num;
    }
    pushpts[0] = v1->num; pushpts[1] = v2->num;
    instrs = pushpoints( instrs,2,pushpts );
    *instrs++ = 0x87;       /*SDPVTL [orthogonal] */

    if ( SetFreedomVector( &instrs,a1,ptcnt,touched,diagpts,v1,v2,fv )) {
        instrs = pushpoint( instrs,a1 );
        if (( fv->x == 1 && touched[a1] & tf_x ) || ( fv->y == 1 && touched[a1] & tf_y ))
            *instrs++ = SRP0;
        else
            *instrs++ = MDAP;
        touched[a1] |= tf_d;
        lfixed = true;

        /* Mark the point as already positioned relatively to the given
        /* diagonale. As the point may be associated either with the current
        /* vector, or with another edge of the stem (which, of course, both point
        /* in the same direction), we have to check it agains both vectors. */
        if (!MarkLineFinished( a1,ds->pts[0].num,ds->pts[1].num,diagpts ))
            MarkLineFinished( a1,ds->pts[2].num,ds->pts[3].num,diagpts );
    }

    if ( SetFreedomVector( &instrs,a2,ptcnt,touched,diagpts,v1,v2,fv )) {
        if ( !lfixed ) {
            instrs = pushpoint( instrs,a1 );
            *instrs++ = 0x10;	    /* Set RP0, SRP0 */
        }

        instrs = pushpoint( instrs,a2 );
        *instrs++ = 0xc0;	    /* MDRP */
        touched[a2] |= tf_d;

        if (!MarkLineFinished( a2,ds->pts[0].num,ds->pts[1].num,diagpts ))
            MarkLineFinished( a2,ds->pts[2].num,ds->pts[3].num,diagpts );
    }

    /* Always put the calculated stem width into the CVT table, unless it is
    /* already there. This approach would be wrong for vertical or horizontal
    /* stems, but for diagonales it is just unlikely that we can find an
    /* acceptable predefined value in StemSnapH or StemSnapV */
    distance = TTF_getcvtval( ct->gic->sf,ds->width );

    if ( SetFreedomVector( &instrs,b1,ptcnt,touched,diagpts,v1,v2,fv )) {
        instrs = pushpointstem( instrs,b1,distance );
        *instrs++ = 0xe0+0x19;  /* MIRP, srp0, minimum, black */
        touched[b1] |= tf_d;
        rfixed = true;
        if (!MarkLineFinished( b1,ds->pts[0].num,ds->pts[1].num,diagpts ))
            MarkLineFinished( b1,ds->pts[2].num,ds->pts[3].num,diagpts );
    }

    if ( SetFreedomVector( &instrs,b2,ptcnt,touched,diagpts,v1,v2,fv )) {
        if ( !rfixed ) {
            instrs = pushpoint( instrs,b1 );
            *instrs++ = 0x10;	    /* Set RP0, SRP0 */
        }
        instrs = pushpoint( instrs,b2 );
        *instrs++ = 0xc0;	    /* MDRP */
        touched[b2] |= tf_d;
        if (!MarkLineFinished( b2,ds->pts[0].num,ds->pts[1].num,diagpts ))
            MarkLineFinished( b2,ds->pts[2].num,ds->pts[3].num,diagpts );
    }

    ds->done = true;
return( instrs );
}

static uint8 *FixPointOnLine ( DiagPointInfo *diagpts,PointVector *line,
    NumberedPoint *pt,InstrCt *ct,BasePoint *fv,BasePoint *pv,
    int *rp0,int *rp1,int *rp2 ) {

    uint8 *instrs, *touched;
    BasePoint newpv;
    int ptcnt;
    int pushpts[3];

    touched = ct->touched;
    instrs = ct->pt;
    ptcnt = ct->ptcnt;

    newpv = GetVector( line->pt1->pt,line->pt2->pt,true );

    if ( !UnitsParallel( pv,&newpv,true )) {
        pv->x = newpv.x; pv->y = newpv.y;

        pushpts[0] = line->pt1->num; pushpts[1] = line->pt2->num;
        instrs = pushpoints( instrs,2,pushpts );
        *instrs++ = 0x07;         /*SPVTL[orthogonal]*/
    }

    if ( SetFreedomVector( &instrs,pt->num,ptcnt,touched,diagpts,line->pt1,line->pt2,fv )) {
        if ( *rp0 != line->pt1->num ) {
            *rp0 = line->pt1->num;
            pushpts[0] = pt->num; pushpts[1] = line->pt1->num;
            instrs = pushpoints( instrs,2,pushpts );
            *instrs++ = 0x10;	    /* Set RP0, SRP0 */
        } else {
            instrs = pushpoint( instrs,pt->num );
        }
        *instrs++ = 0xc0;	    /* MDRP */

        /* If a point has to be positioned just relatively to the diagonal
           line (no intersections, no need to maintain other directions),
           then we can interpolate it along that line. This usually produces
           better results for things like Danish slashed "O". */
        if ( !( touched[pt->num] & (tf_x|tf_y|tf_d )) &&
            !( diagpts[pt->num].count > 1 )) {
            if ( *rp1!=line->pt1->num || *rp2!=line->pt2->num) {
                *rp1=line->pt1->num;
                *rp2=line->pt2->num;

                pushpts[0]=pt->num;
                pushpts[1]=line->pt2->num;
                pushpts[2]=line->pt1->num;
                instrs = pushpoints( instrs,3,pushpts );

                *instrs++ = 0x11;	    /* Set RP1, SRP1 */
                *instrs++ = 0x12;	    /* Set RP2, SRP2 */
            } else {
                instrs = pushpoint( instrs,pt->num );
            }
            *instrs++ = 0x39;	    /* Interpolate points, IP */
        }
    }

return( instrs );
}

/* When all stem edges have already been positioned, run through other
/* points which are known to be related with some diagonales and position
/* them too. This may include both intersections and points which just
/* lie on a diagonal line. This function does not care about starting/ending
/* points of stems, unless they should be additinally positioned relatively
/* to another stem. Thus is can handle things like "X" or "K". */
static uint8 *MovePointsToIntersections( InstrCt *ct,DiagPointInfo *diagpts,
    BasePoint *fv ) {

    int i, j, ptcnt, rp0=-1, rp1=-1, rp2=-1;
    uint8 *touched;
    BasePoint pv;
    NumberedPoint *curpt;

    touched = ct->touched;
    ptcnt = ct->ptcnt;
    pv.x = 1; pv.y = 0;

    for ( i=0; i<ptcnt; i++ ) {
        if ( diagpts[i].count > 0 ) {
            for ( j=0; j<diagpts[i].count; j++ ) {
                if ( !diagpts[i].line[j]->done ) {
                    curpt = chunkalloc(sizeof(struct numberedpoint));
                    curpt->num = i;
                    curpt->pt = &(ct->bp[i]);

                    ct->pt = FixPointOnLine( diagpts,diagpts[i].line[j],
                        curpt,ct,fv,&pv,&rp0,&rp1,&rp2 );

                    touched[i] |= tf_d;
                    diagpts[i].line[j]->done = ( true );
                    chunkfree( curpt,sizeof(struct numberedpoint) );
                }
            }
        }
    }
return( ct->pt );
}

/* Finally explicitly touch all affected points by X and Y (unless they
/* have already been), so that subsequent IUP's can't distort our
/* stems. */
static uint8 *TouchDStemPoints( InstrCt *ct,DiagPointInfo *diagpts,
    BasePoint *fv ) {

    int i, ptcnt, numx=0, numy=0;
    int *tobefixedy, *tobefixedx;
    uint8 *instrs, *touched;

    touched = ct->touched;
    instrs = ct->pt;
    ptcnt = ct->ptcnt;

    tobefixedy = gcalloc( ptcnt,sizeof( int ) );
    tobefixedx = gcalloc( ptcnt,sizeof( int ) );

    for ( i=0; i<ptcnt; i++ ) {
        if ( diagpts[i].count > 0 ) {
            if (!(touched[i] & tf_y)) {
                tobefixedy[numy++]=i;
                touched[i] |= tf_y;
            }

            if (!(touched[i] & tf_x)) {
                tobefixedx[numx++]=i;
                touched[i] |= tf_x;
            }
        }
    }

    if ( numy>0 ) {
        if ( !(fv->x == 0 && fv->y == 1) ) *instrs++ = SVTCA_y;
        instrs = instructpoints ( instrs,numy,tobefixedy,MDAP );
    }

    if ( numx>0 ) {
        if ( !(fv->x == 1 && fv->y == 0) || numy > 0 ) *instrs++ = SVTCA_x;
        instrs = instructpoints ( instrs,numx,tobefixedx,MDAP );
    }

    free( tobefixedy );
    free( tobefixedx );
return( instrs );
}

static void DStemFree( DStem *ds,DiagPointInfo *diagpts,int cnt ) {
    DStem *next;
    int i,j;

    while ( ds!=NULL ) {
	next = ds->next;
        for ( i=0; i<4; i++ ) {
            /*chunkfree( &ds->pts[i],sizeof( struct numberedpoint ))*/;
        }
	chunkfree( ds,sizeof( struct dstem ));
	ds = next;
    }

    for ( i=0; i<cnt ; i++ ) {
        if ( diagpts[i].count > 0 ) {
            for ( j=0 ; j<diagpts[i].count ; j++ ) {
                chunkfree( diagpts[i].line[j],sizeof( struct pointvector ));
            }
        }
    }
}

static void DStemInfoGeninst( InstrCt *ct ) {
    DStem *ds, *curds;
    BasePoint fv;
    DiagPointInfo *diagpts;

    if (ct->diagstems == NULL)
return;

    ds = ct->diagstems;
    diagpts = ct->diagpts;

    fv.x = 1; fv.y = 0;

    ct->pt = pushF26Dot6( ct->pt,DIAG_MIN_DISTANCE );
    *(ct->pt)++ = SMD; /* Set Minimum Distance */

    for ( curds=ds; curds!=NULL; curds=curds->next )
        ct->pt = FixDstem ( ct,curds,diagpts,&fv );

    ct->pt = MovePointsToIntersections( ct,diagpts,&fv );
    ct->pt = TouchDStemPoints ( ct,diagpts,&fv);

#if 0
    ct->pt = pushF26Dot6( ct->pt,1.0 );
    *(ct->pt)++ = SMD; /* Set Minimum Distance */
#endif
}

#if TESTIPSTRONG
/******************************************************************************
 *
 * Strong point interpolation
 *
 * TODO! Optimization.
 * TODO! It could be faster.
 *
 ******************************************************************************/

/* To be used with qsort() - sorts real array in ascending order. */
static int sortreals(const void *a, const void *b) {
    return *(real *)a > *(real *)b;
}

static void InterpolateStrongPoints(InstrCt *ct) {
    uint8 *isdstemend=NULL;
    DStem *dhint;
    StemInfo *hint, *firsthint = ct->xdir?ct->sc->vstem:ct->sc->hstem;
    int p, touchflag = ct->xdir?tf_x:tf_y;
    real tmp, edgelist[192];
    int edgecnt=0, i, j, k, skip;
    int lpoint = -1, ledge=0;
    int rpoint = -1;
    int nowrp1 = 1;
    int ldone = 0;

    if (firsthint==NULL)
return;

    /* List all stem edges. List only active edges for ghost hints. */
    for (hint=firsthint; hint!=NULL; hint=hint->next) {
        tmp = hint->start;

	if (hint->ghost && hint->width == 20)
	    tmp += hint->width;

	for (i=skip=0; i<edgecnt; i++)
	    if (abs(tmp - edgelist[i]) <= ct->gic->fudge) {
	        skip=1;
		break;
	    }

	if (!skip) edgelist[edgecnt++] = tmp;

	if (hint->ghost)
    continue;

	tmp+=hint->width;

	for (i=skip=0; i<edgecnt; i++)
	    if (abs(tmp - edgelist[i]) <= ct->gic->fudge) {
	        skip=1;
		break;
	    }

	if (!skip) edgelist[edgecnt++] = tmp;
    }

    if (edgecnt < 2)
return;

    qsort(edgelist, edgecnt, sizeof(real), sortreals);

    /* If there are diagonal stems, list their endpoints */
    if (ct->diagstems) {
	isdstemend = gcalloc(ct->ptcnt, sizeof(uint8));

	for (dhint=ct->diagstems; dhint!=NULL; dhint=dhint->next)
	    for (k=0; k<4; k++) isdstemend[dhint->pts[k].num] = 1;
    }

    /* Interpolate important points between subsequent edges */
    for (i=0; i<edgecnt; i++) {
        init_edge(ct, edgelist[i], ALL_CONTOURS);
	rpoint = ct->edge.refpt;
	if (ct->edge.othercnt) free(ct->edge.others);
	if (rpoint == -1 || !(ct->touched[rpoint] & touchflag)) continue;

	if (lpoint==-1) {
	    /* first edge */
	    lpoint = rpoint;
	    ledge = i;
	}
	else {
	    real fudge = ct->gic->fudge;
	    ct->gic->fudge = (edgelist[i]-edgelist[ledge])/2;
	    init_edge(ct, (edgelist[i]+edgelist[ledge])/2, ALL_CONTOURS);
	    ct->gic->fudge = fudge;

	    /* Optimize out all points on DStems, but not DStems' ends. */
	    for (j=0; j<ct->edge.othercnt; j++) {
		p = ct->edge.others[j];

		if (ct->diagstems && ct->diagpts[p].count && !isdstemend[p])
		{
		    for (k=j+1; k<ct->edge.othercnt; k++)
			ct->edge.others[k-1] = ct->edge.others[k];

		    ct->edge.othercnt--;
		    j--;
		}
	    }

	    /* A correct method of optimization is needed here. */
	    /* optimize_blue(ct); */
	    /* optimize_edge(ct); */

	    if (!ct->edge.othercnt) {
		nowrp1 = 1;
		lpoint = rpoint;
		ledge = i;
		ldone = 0;
	    }
	    else if (ct->edge.refscore) {
		if (!ldone) {
		    ct->pt = push2points(ct->pt, rpoint, lpoint);
		    *ct->pt++ = SRP1;
		    *ct->pt++ = SRP2;
		}
		else {
		    ct->pt = pushpoint(ct->pt, rpoint);
		    if (nowrp1) *ct->pt++ = SRP1;
		    else *ct->pt++ = SRP2;
		    nowrp1 = !nowrp1;
		}

		lpoint = rpoint;
		ledge = i;
		ldone = 1;

		/* instruct points */
		ct->pt = instructpoints(ct->pt, ct->edge.othercnt,
						    ct->edge.others, IP);

		for (j=0; j<ct->edge.othercnt; j++) {
		    p = ct->edge.others[j];

		    if (!ct->diagstems || !ct->diagpts[p].count)
			ct->touched[p] |= touchflag;
		}
	    }

	    if (ct->edge.othercnt) free(ct->edge.others);
	}
    }

    if (ct->diagstems) free(isdstemend);
}
#endif /* TESTIPSTRONG */

/******************************************************************************
 *
 * Generate instructions for a glyph.
 *
 ******************************************************************************/

static uint8 *dogeninstructions(InstrCt *ct) {
    StemInfo *hint;
    int max;
    DStemInfo *dstem;

    /* Maximum instruction length is 6 bytes for each point in each dimension */
    /*  2 extra bytes to finish up. And one byte to switch from x to y axis */
    /* Diagonal take more space because we need to set the orientation on */
    /*  each stem, and worry about intersections, etc. */
    /*  That should be an over-estimate */
    max=2;
    if ( ct->sc->vstem!=NULL ) max += ct->ptcnt*8;
    if ( ct->sc->hstem!=NULL ) max += ct->ptcnt*8+4;
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

    /* classify points prior to usage */
    RunOnPoints(ct, ALL_CONTOURS, find_control_pts);

    /* Prepare info about diagonal stems to be used during edge optimization. */
    /* These contents need to be explicitly freed after hinting diagonals. */
    if ( ct->sc->dstem )
	ct->diagpts = InitDStemData(ct);

    /* We start from instructing horizontal features (=> movement in y) */
    /* Do this first so that the diagonal hinter will have everything moved */
    /* properly when it sets the projection vector */
    /* Even if we aren't doing the diagonals, we do the blues. */
    ct->xdir = false;
    *(ct->pt)++ = SVTCA_y;

    snap_to_blues(ct);
    HStemGeninst(ct);
#if TESTIPSTRONG
    InterpolateStrongPoints(ct);
#endif

    /* next instruct vertical features (=> movement in x) */
    ct->xdir = true;
    if ( ct->pt != ct->instrs ) *(ct->pt)++ = SVTCA_x;

    VStemGeninst(ct);
#if TESTIPSTRONG
    InterpolateStrongPoints(ct);
#endif

    /* finally instruct diagonal stems (=> movement in x) */
    /* This is done after vertical stems because it involves */
    /* moving some points out-of their vertical stems. */
    if ( ct->diagstems ) {
        DStemInfoGeninst(ct);
	DStemFree(ct->diagstems, ct->diagpts, ct->ptcnt);
	free(ct->diagpts);
    }

    /* Interpolate untouched points */
    *(ct->pt)++ = IUP_y;
    *(ct->pt)++ = IUP_x;

    if ((ct->pt)-(ct->instrs) > max) IError(
	"We're about to crash.\n"
	"We miscalculated the glyph's instruction set length\n"
	"When processing TTF instructions (hinting) of %s", ct->sc->name
    );

    ct->sc->ttf_instrs_len = (ct->pt)-(ct->instrs);
    ct->sc->instructions_out_of_date = false;
return ct->sc->ttf_instrs = grealloc(ct->instrs,(ct->pt)-(ct->instrs));
}

void NowakowskiSCAutoInstr(GlobalInstrCt *gic, SplineChar *sc) {
    int cnt, contourcnt;
    BasePoint *bp;
    int *contourends;
    uint8 *oncurve;
    uint8 *touched;
    uint8 *affected;
    SplineSet *ss;
    RefChar *ref;
    InstrCt ct;
    int i;

    if ( !sc->layers[ly_fore].order2 )
return;

    if ( sc->layers[ly_fore].refs!=NULL && sc->layers[ly_fore].splines!=NULL ) {
	ff_post_error(_("Can't instruct this glyph"),
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
	ff_post_error(_("Can't instruct this glyph"),
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

    if ( sc->vstem==NULL && sc->hstem==NULL && sc->dstem==NULL && sc->md==NULL)
return;

    /* TODO!
     *
     * We're having problems with references utilizing 'use my metrics' that are
     * rotated or flipped horizontally. Basically, such glyphs can get negative
     * width and behave strangely when the glyph referred is instructed. Such
     * widths are treated very differently under Freetype (OK) and Windows
     * (terribly shifted), and I suppose other rasterizers can also complain.
     * Perhaps we should advise turning 'use my metrics' off.
     */

    if ( sc->layers[ly_fore].splines==NULL )
return;

    /* Start dealing with the glyph */
    contourcnt = 0;
    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next, ++contourcnt );
    cnt = SSTtfNumberPoints(sc->layers[ly_fore].splines);

    contourends = galloc((contourcnt+1)*sizeof(int));
    bp = galloc(cnt*sizeof(BasePoint));
    oncurve = gcalloc(cnt,1);
    touched = gcalloc(cnt,1);
    affected = gcalloc(cnt,1);
    contourcnt = cnt = 0;
    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	touched[cnt] |= tf_startcontour;
	cnt = SSAddPoints(ss,cnt,bp,NULL);
	touched[cnt-1] |= tf_endcontour;
	contourends[contourcnt++] = cnt-1;
    }
    contourends[contourcnt] = 0;

    for (i=0; i<gic->bluecnt; i++)
        gic->blues[i].highest = gic->blues[i].lowest = -1;

    ct.gic = gic;

    ct.sc = sc;
    ct.ss = sc->layers[ly_fore].splines;
    ct.instrs = NULL;
    ct.pt = NULL;
    ct.ptcnt = cnt;
    ct.contourends = contourends;
    ct.bp = bp;
    ct.oncurve = oncurve;
    ct.touched = touched;
    ct.affected = affected;
    ct.diagstems = NULL;
    ct.diagpts = NULL;

    ct.rp0 = 0;

    dogeninstructions(&ct);

    free(oncurve);
    free(touched);
    free(affected);
    free(bp);
    free(contourends);

    SCMarkInstrDlgAsChanged(sc);
    SCHintsChanged(sc);
}
