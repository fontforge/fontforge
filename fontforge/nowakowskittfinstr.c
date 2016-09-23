/* Copyright (C) 2000-2012 by
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

int instruct_diagonal_stems = 1,
    instruct_serif_stems = 1,
    instruct_ball_terminals = 1,
    interpolate_strong = 1,
    interpolate_more_strong = 1, /* not applicable if interpolate_strong==0 */
    control_counters = 0;

/* non-optimized instructions will be using a stack of depth 6, allowing
 * for easy testing whether the code leaves trash on the stack or not.
 */
#define OPTIMIZE_TTF_INSTRS 1
#if OPTIMIZE_TTF_INSTRS
#define STACK_DEPTH 256
#else
#define STACK_DEPTH 6
#endif

/* define some often used instructions */
#define SVTCA_y                 (0x00)
#define SVTCA_x                 (0x01)
#define SRP0                    (0x10)
#define SRP1                    (0x11)
#define SRP2                    (0x12)
#define SZP0                    (0x13)
#define SLOOP                   (0x17)
#define RTG                     (0x18)
#define SMD                     (0x1a)
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
#define IP                      (0x39)
#define ALIGNRP                 (0x3c)
#define MIAP_rnd                (0x3f)
#define ADD                     (0x60)
#define MUL                     (0x63)
#define NEG                     (0x65)
#define SROUND                  (0x76)
#define FLIPPT                  (0x80)
#define MDRP_grey               (0xc0)
#define MDRP_min_black          (0xc9)
#define MDRP_min_white          (0xca)
#define MDRP_min_rnd_black      (0xcd)
#define MDRP_rp0_rnd_white      (0xd6)
#define MDRP_rp0_min_rnd_grey   (0xdc)
#define MDRP_rp0_min_rnd_black  (0xdd)
#define MIRP_min_black          (0xe9)
#define MIRP_min_rnd_black      (0xed)
#define MIRP_rp0_min_black      (0xf9)
#define MIRP_rp0_min_rnd_black  (0xfd)


/******************************************************************************
 *
 * Low-level routines to add data for PUSHes to bytecode instruction stream.
 * pushheader() adds PUSH preamble, then repeating addpoint() adds items.
 *
 * Numbers larger than 65535 are not supported (according to TrueType spec,
 * there can't be more points in a glyph, simple or compound). Negative
 * numbers aren't supported, either. So don't use these functions as they
 * are - there are higher-level ones further below, that handle things nicely.
 *
 ******************************************************************************/

static uint8 *pushheader(uint8 *instrs, int isword, int tot) {
    if ( isword ) {
	if ( tot>8 ) {
	    *instrs++ = 0x41;		/* N(next word) Push words */
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

/* Exemplary high-level routines to add PUSH-es to bytecode instruction
 * stream. They handle negative numbers correctly. As they are used
 * in various roles here, some aliases are defined, so that the name
 * speaks for itself in the code.
 */

static uint8 *pushpoint(uint8 *instrs,int pt) {
    instrs = pushheader(instrs,(pt>255)||(pt<0),1);
return( addpoint(instrs,(pt>255)||(pt<0),pt));
}

#define pushnum(a, b) pushpoint(a, b)

static uint8 *pushpointstem(uint8 *instrs, int pt, int stem) {
    int isword = pt>255 || stem>255 || pt<0 || stem<0;
    instrs = pushheader(instrs,isword,2);
    instrs = addpoint(instrs,isword,pt);
return( addpoint(instrs,isword,stem));
}

#define push2points(a, b, c) pushpointstem(a, b, c)
#define push2nums(a, b, c) pushpointstem(a, b, c)

/* Push a bunch of point numbers (or other numbers) onto the stack.
 * TODO!
 * Possible strategies:
 *   - push point by point (poor space efficiency)
 *   - push all the stock at once (currently used, better, but has
 *     poor space efficiency in case of a word among several bytes).
 *   - push bytes and words separately
 */
static uint8 *pushpoints(uint8 *instrs, int ptcnt, const int *pts) {
    int i, isword = 0;
    for (i=0; i<ptcnt; i++) if (pts[i]>255 || pts[i]<0) isword=1;

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
 * we need to do this by hand. As we can explicitly push only 16-bit
 * quantities, we need to push a F26dot6 value in halves, shift left
 * the more significant half and add halves.
 *
 * There are no checks for overflow!
 */
static uint8 *pushF26Dot6(uint8 *instrs, double num) {
    int a, elems[3];
    int negative=0;

    if (num < 0) {
        negative=1;
        num*=-1.0;
    }

    num *= 64;
    a = rint(num);
    elems[0] = a % 65536;
    elems[1] = (int)rint(a / 65536.0) % 65536;
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

/* Compute an EF2Dot14 representation of a floating point number.
 * The number must be in range [-2.0 ... 1.0+(2^14-1)/(2^14) = 1.99993896...]
 *
 * There are no checks for overflow!
 */
static int EF2Dot14(double num) {
return( rint(num*16384) );
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

    instrs = pushpoints(instrs, ptcnt<STACK_DEPTH?ptcnt:STACK_DEPTH-1, pts);

    if (use_sloop) {
        *instrs++ = DEPTH;
        *instrs++ = SLOOP;
        *instrs++ = command;
    }
    else for (i=0; i<(ptcnt<STACK_DEPTH?ptcnt:STACK_DEPTH-1); i++)
        *instrs++ = command;

    if (ptcnt>=STACK_DEPTH)
        instrs=instructpoints(instrs, ptcnt-(STACK_DEPTH-1), pts+(STACK_DEPTH-1), command);

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
        cvt_tab->data = malloc(100*sizeof(short));
        cvt_tab->next = sf->ttf_tables;
        sf->ttf_tables = cvt_tab;
    }
    for ( i=0; (int)sizeof(uint16)*i<cvt_tab->len; ++i ) {
        int tval = (int16) memushort(cvt_tab->data,cvt_tab->len, sizeof(uint16)*i);
        if ( val>=tval-1 && val<=tval+1 )
return( i );
    }
    if ( (int)sizeof(uint16)*i>=cvt_tab->maxlen ) {
        if ( cvt_tab->maxlen==0 ) cvt_tab->maxlen = cvt_tab->len;
        cvt_tab->maxlen += 200;
        cvt_tab->data = realloc(cvt_tab->data,cvt_tab->maxlen);
    }
    memputshort(cvt_tab->data,sizeof(uint16)*i,val);
    cvt_tab->len += sizeof(uint16);
return( i );
}

/* by default sign is unimportant in the cvt
 * For some instructions anyway, but not for MIAP so this routine has
 *  been broken in two.
 */
int TTF_getcvtval(SplineFont *sf,int val) {
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
 **  a glyph, because we want to be sure that global hinting tables (cvt, prep,
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
                results = realloc(results, sizeof(real)*(++(*rescnt)));
                results[*rescnt-1] = d;
            }
            else (results = calloc(*rescnt=1, sizeof(real)))[0] = d;
        }

        str = end;
    }

return results;
}

static real *GetNParsePSArray(SplineFont *sf, const char *name, int *rescnt) {
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

    const char *PrimaryBlues = HasPSBlues ? "BlueValues" : "FamilyBlues";
    const char *OtherBlues = HasPSBlues ? "OtherBlues" : "FamilyOtherBlues";

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
		    if (isfinite(gic->blues[j].family_base))
		        continue;
		    else if (values[1] != gic->blues[j].base &&
		             SegmentsOverlap(gic->blues[j].base,
		                       gic->blues[j].overshoot,
				       values[0], values[1]))
		        gic->blues[j].family_base = values[1];

		/* Next pairs are top zones (see Type1 specification). */
		for (i=1; i<cnt; i++) {
		    for (j=0; j<bluecnt; j++)
		        if (isfinite(gic->blues[j].family_base))
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
		        if (isfinite(gic->blues[j].family_base))
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
 * horizontal or vertical stems (xdir decides) here. If Std*W is not specified,
 * but there exists StemSnap*, we'll make up a fake Std*V as a fallback.
 * Subtle manipulations with Std*W's value can result in massive change of
 * font appearance at some pixel sizes, because it's used as a base for
 * normalization of all other stems.
 */
static void GICImportStems(int xdir, GlobalInstrCt *gic) {
    int i, cnt, next;
    real *values;
    const char *s_StdW = xdir?"StdVW":"StdHW";
    const char *s_StemSnap = xdir?"StemSnapV":"StemSnapH";
    StdStem *stdw = xdir?&(gic->stdvw):&(gic->stdhw);
    StdStem **stemsnap = xdir?&(gic->stemsnapv):&(gic->stemsnaph);
    int *stemsnapcnt = xdir?&(gic->stemsnapvcnt):&(gic->stemsnaphcnt);

    if ((values = GetNParsePSArray(gic->sf, s_StdW, &cnt)) != NULL) {
        stdw->width = *values;
        free(values);
    }

    if ((values = GetNParsePSArray(gic->sf, s_StemSnap, &cnt)) != NULL) {
        *stemsnap = (StdStem *)calloc(cnt, sizeof(StdStem));

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
    struct ttf_table *tab;
    uint8 *cvt;

    cvtsize = 1;
    if (gic->stdhw.width != -1) cvtsize++;
    if (gic->stdvw.width != -1) cvtsize++;
    cvtsize += gic->stemsnaphcnt;
    cvtsize += gic->stemsnapvcnt;
    cvtsize += gic->bluecnt * 2; /* possible family blues */

    cvt = calloc(cvtsize, cvtsize * sizeof(int16));
    cvtindex = 0;

    /* Assign cvt indices */
    for (i=0; i<gic->bluecnt; i++) {
        gic->blues[i].cvtindex = cvtindex;
        memputshort(cvt, 2*cvtindex++, rint(gic->blues[i].base));

	if (isfinite(gic->blues[i].family_base)) {
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
    cvt = realloc(cvt, cvtsize * sizeof(int16));

    /* Try to implant the new cvt table */
    gic->cvt_done = 0;

    tab = SFFindTable(gic->sf, CHR('c','v','t',' '));

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
        if (tab->len >= cvtsize * (int)sizeof(int16) &&
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
 * also a twilight zone). We also currently define some functions in fpgm.
 * We must ensure this is indicated in the 'maxp' table.
 *
 * We also need two storage cells. As we now use SPVFS to set projection
 * vector for diagonal hinting, we have to adjust values taken by SPVFS,
 * so that diagonals look cleanly in all aspect ratios. Adjustments are
 * not trivial to compute, so we do this once (in prep) and store them
 * in storage[0] (for X direction) and storage[1] (for Y direction).
 */
static void init_maxp(GlobalInstrCt *gic) {
    struct ttf_table *tab = SFFindTable(gic->sf, CHR('m','a','x','p'));
    uint16 zones, twpts, store, fdefs, stack;

    if ( tab==NULL ) {
        tab = chunkalloc(sizeof(struct ttf_table));
        tab->next = gic->sf->ttf_tables;
        gic->sf->ttf_tables = tab;
        tab->tag = CHR('m','a','x','p');
    }

    if ( tab->len<32 ) {
        tab->data = realloc(tab->data,32);
        memset(tab->data+tab->len,0,32-tab->len);
        tab->len = tab->maxlen = 32;
    }

    zones = memushort(tab->data, 32,  7*sizeof(uint16));
    twpts = memushort(tab->data, 32,  8*sizeof(uint16));
    store = memushort(tab->data, 32,  9*sizeof(uint16));
    fdefs = memushort(tab->data, 32, 10*sizeof(uint16));
    stack = memushort(tab->data, 32, 12*sizeof(uint16));

    if (gic->fpgm_done && zones<2) zones=2;
    if (gic->fpgm_done && twpts<1) twpts=1;
    if (gic->fpgm_done && gic->prep_done && store<2) store=2;
    if (gic->fpgm_done && fdefs<22) fdefs=22;
    if (stack<STACK_DEPTH) stack=STACK_DEPTH;

    memputshort(tab->data, 7*sizeof(uint16), zones);
    memputshort(tab->data, 8*sizeof(uint16), twpts);
    memputshort(tab->data, 9*sizeof(uint16), store);
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

        /* Function 1: Place given point relatively to previous, maintaining the
         * minimum distance. Then call FPGM 12 to check if the point's gridfitted 
         * position is too far from its original position, and correct it, if necessary.
         * Syntax: PUSB_2 point 1 CALL
         */
        0xb0, // PUSHB_1
        0x01, //   1
        0x2c, // FDEF
        0x20, //   DUP
        0xda, //   MDRP[rp0,min,white]
        0xb0, //   PUSHB_1
        0x0c, //     12
        0x2b, //   CALL
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
        0x23, //       SWAP
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
         * CAUTION! FreeType doesn't support that, as subpixel
         * filtering is usually done by higher level library.
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
        0x2d, // ENDF

        /* Function 9: Link a serif-like element edge to the opposite
         * edge of the base stem when rounding down to grid, but ensure
         * that its distance from the reference point is larger than
         * the base stem width at least to a specified amount of pixels.
         * Syntax: PUSHX_3 min_dist inner_pt outer_pt CALL;
         */
        0xb0, // PUSHB_1
        0x09, //   9
        0x2c, // FDEF
        0x20, //   DUP
        0x7d, //   RDTG
        0xb0, //   PUSHB_1
        0x06, //     6
        0x2b, //   CALL
        0x58, //   IF
        0xc4, //     MDRP[min,grey]
        0x1b, //   ELSE
        0xcd, //     MDRP[min,rnd,black]
        0x59, //   EIF
        0x20, //   DUP
        0xb0, //   PUSHB_1
        0x03, //     3
        0x25, //   CINDEX
        0x49, //   MD[grid]
        0x23, //   SWAP
        0x20, //   DUP
        0xb0, //   PUSHB_1
        0x04, //     4
        0x26, //   MINDEX
        0x4a, //   MD[orig]
        0xb0, //   PUSHB_1
        0x00, //     0
        0x50, //   LT
        0x58, //   IF
        0x8a, //     ROLL
        0x65, //     NEG
        0x8a, //     ROLL
        0x61, //     SUB
        0x20, //     DUP
        0xb0, //     PUSHB_1
        0x00, //       0
        0x50, //     LT
        0x58, //     IF
        0x38, //       SHPIX
        0x1b, //     ELSE
        0x21, //       POP
        0x21, //       POP
        0x59, //     EIF
        0x1b, //   ELSE
        0x8a, //     ROLL
        0x8a, //     ROLL
        0x61, //     SUB
        0x20, //     DUP
        0xb0, //     PUSHB_1
        0x00, //       0
        0x52, //     GT
        0x58, //     IF
        0x38, //       SHPIX
        0x1b, //     ELSE
        0x21, //       POP
        0x21, //       POP
        0x59, //     EIF
        0x59, //   EIF
        0x18, //   RTG
        0x2d, // ENDF

        /* Function 10: depending from the hinting mode (grayscale or mono) set
         * rp0 either to pt1 or to pt2. This is used to link serif-like elements
         * either to the opposite side of the base stem or to the same side (i. e.
         * left-to-left and right-to-right).
         * Syntax: PUSHX_3 pt2 pt1 10 CALL 
         */
        0xb0, // PUSHB_1
        0x0a, //   10
        0x2c, // FDEF
        0xb0, //   PUSHB_1
        0x06, //     6
        0x2b, //   CALL
        0x58, //   IF
        0x21, //     POP
        0x10, //     SRP0
        0x1b, //   ELSE
        0x10, //     SRP0
        0x21, //     POP
        0x59, //   EIF
        0x2d, // ENDF

        /* Function 11: similar to FPGM 1, but places a point without
         * maintaining the minimum distance.
         * Syntax: PUSHX_2 point 11 CALL 
         */
        0xb0, // PUSHB_1
        0x0b, //   11
        0x2c, // FDEF
        0x20, //   DUP
        0xd2, //   MDRP[rp0,white]
        0xb0, //   PUSHB_1
        0x0c, //     12
        0x2b, //   CALL
        0x2d, // ENDF

        /* Function 12: Check if the gridfitted position of the point is too far 
         * from its original position, and shift it, if necessary. The function is 
         * used to place vertical stems, it assures almost linear advance width 
         * to PPEM scaling. Shift amount is capped to at most 1 px to prevent some
         * weird artifacts at very small ppems. In cleartype mode, no shift
         * is made at all.
         * Syntax: PUSHX_2 point 12 CALL 
         */
        0xb0, // PUSHB_1
        0x0c, //   12
        0x2c, // FDEF
        0x20, //   DUP
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

        /* Function 13: Interpolate a HStem edge's reference point between two other points 
         * and snap it to the grid. Then compare its new position with the ungridfitted
         * position of the second edge. If the gridfitted point belongs to the bottom edge
         * and now it is positioned above the top edge's original coordinate, then shift it
         * one pixel down; similarly, if the interpolation resulted in positioning the top
         * edge below the original coordinate of the bottom edge, shift it one pixel up.
         * Syntax: PUSHX_6 other_edge_refpt pt_to_ip rp1 rp2 13 CALL 
         */
        0xb0, // PUSHB_1
        0x0d, //   13
        0x2c, // FDEF
        0x12, //   SRP2
        0x11, //   SRP1
        0x20, //   DUP
        0x20, //   DUP
        0x39, //   IP
        0x2f, //   MDAP[rnd]
        0x20, //   DUP
        0x8a, //   ROLL
        0x20, //   DUP
        0x47, //   GC[orig]
        0x8a, //   ROLL
        0x46, //   GC[cur]
        0x61, //   SUB
        0x23, //   SWAP
        0x8a, //   ROLL
        0x20, //   DUP
        0x8a, //   ROLL
        0x23, //   SWAP
        0x4A, //   MD[orig]
        0xb0, //   PUSHB_1
        0x00, //     0
        0x50, //   LT
        0x58, //   IF
        0x23, //     SWAP
        0xb0, //     PUSHB_1
        0x00, //       0
        0x52, //     GT
        0x58, //     IF
        0xb0, //       PUSHB_1
        0x40, //         64
        0x38, //       SHPIX
        0x1b, //     ELSE
        0x21, //       POP
        0x59, //     EIF
        0x1b, //   ELSE
        0x23, //     SWAP
        0xb0, //     PUSHB_1
        0x00, //       0
        0x50, //     LT
        0x58, //     IF
        0xb0, //       PUSHB_1
        0x40, //         64
        0x65, //       NEG
        0x38, //       SHPIX
        0x1b, //     ELSE
        0x21, //       POP
        0x59, //     EIF
        0x59, //   EIF
        0x2d, // ENDF

        /* Function 14: Link two points using MDRP without maintaining
         * the minimum distance. In antialiased mode use rounding to
         * double grid for this operation, otherwise ensure there is no
         * distance between those two points below the given PPEM (i. e.
         * points are aligned). The function is used for linking nested
         * stems to each other, and guarantees their relative positioning
         * is preserved in the gridfitted outline.
         * Syntax: PUSHX_4 ppem ref_pt base_pt 14 CALL;
         */
        0xb0, // PUSHB_1
        0x0e, //   14
        0x2c, // FDEF
        0xb0, //   PUSHB_1
        0x06, //     6
        0x2b, //   CALL
        0x58, //   IF
        0x3d, //     RTDG
        0xd6, //     MDRP[rp0,rnd,white]
        0x18, //     RTG
        0x21, //     POP
        0x21, //     POP
        0x1b, //   ELSE
        0x20, //     DUP
        0xd6, //     MDRP[rp0,rnd,white]
        0x8a, //     ROLL
        0x4b, //     MPPEM
        0x52, //     GT
        0x58, //     IF
        0x20, //       DUP
        0x8a, //       ROLL
        0x23, //       SWAP
        0x49, //       MD[grid]
        0x20, //       DUP
        0xb0, //       PUSHB_1
        0x00, //         0
        0x55, //       NEQ
        0x58, //       IF
        0x38, //         SHPIX
        0x1b, //       ELSE
        0x21, //         POP
        0x21, //         POP
        0x59, //       EIF
        0x1b, //     ELSE
        0x21, //       POP
        0x21, //       POP
        0x59, //     EIF
        0x59, //   EIF
        0x2d,  // ENDF

        /* Function 15: similar to FPGM 1, but used to position a stem
         * relatively to the previous stem preserving the counter width
         * equal to the distance between another pair of previously positioned 
         * stems. Thus it serves nearly the same purpose as PS counter hints.
         * Syntax: PUSHX_6 master_counter_start_pt master_counter_end_pt
         *         current_counter_start_pt current_counter_end_pt ppem 15 CALL;
         */
        0xb0, // PUSHB_1
        0x0f, //   15
        0x2c, // FDEF
        0x23, //   SWAP
        0x20, //   DUP
        0xd6, //   MDRP[rp0,rnd,white]
        0x20, //   DUP
        0x2f, //   MDAP[rnd], this is needed for grayscale mode
        0xb0, //   PUSHB_1
        0x07, //     7
        0x2b, //   CALL
        0x5c, //   NOT
        0x58, //   IF
        0x23, //     SWAP
        0x20, //     DUP
        0x58, //     IF
        0x4b, //       MPPEM
        0x53, //       GTEQ
        0x1b, //     ELSE
        0x21, //       POP
        0xb0, //       PUSHB_1
        0x01, //         1
        0x59, //     EIF
        0x58, //     IF
        0x8a, //       ROLL
        0xb0, //       PUSHB_1
        0x04, //         4
        0x26, //       MINDEX
        0x49, //       MD[grid]
        0x23, //       SWAP
        0x8a, //       ROLL
        0x23, //       SWAP
        0x20, //       DUP
        0x8a, //       ROLL
        0x49, //       MD[grid]
        0x8a, //       ROLL
        0x23, //       SWAP
        0x61, //       SUB
        0x38, //       SHPIX
        0x1b, //     ELSE
        0x21, //       POP
        0x21, //       POP
        0x21, //       POP
        0x21, //       POP
        0x59, //     EIF
        0x1b, //   ELSE
        0x21, //     POP
        0x21, //     POP
        0x21, //     POP
        0x21, //     POP
        0x21, //     POP
        0x59, //   EIF
        0x2d, // ENDF

        /* Function 16: Same as FPGM 1, but calls FPGM 18 rather than FPGM 12
         * and thus takes 3 arguments.
         * Syntax: PUSHX_3 ref_point point 16 CALL 
         */
        0xb0, // PUSHB_1
        0x10, //   16
        0x2c, // FDEF
        0x20, //   DUP
        0xda, //   MDRP[rp0,min,white]
        0xb0, //   PUSHB_1
        0x12, //     18
        0x2b, //   CALL
        0x2d, // ENDF

        /* Function 17: Same as FPGM 11, but calls FPGM 18 rather than FPGM 12
         * and thus takes 3 arguments.
         * Syntax: PUSHX_3 ref_point point 17 CALL 
         */
        0xb0, // PUSHB_1
        0x11, //   17
        0x2c, // FDEF
        0x20, //   DUP
        0xd2, //   MDRP[rp0,white]
        0xb0, //   PUSHB_1
        0x12, //     18
        0x2b, //   CALL
        0x2d, // ENDF

        /* Function 18: this is a special version of FPGM 12, used when the counter
         * control is enabled but doesn't directly affect the stem which is going to
         * be positioned. Unlike FPGM 12, it doesn't just attempt to position a point
         * closely enough to its original coordinate, but also checks if the previous
         * stem has already been shifted relatively to its "ideal" position FPGM 12 would
         * determine. If so, then the desired point position is corrected relatively to
         * the current placement of the previous stem. 
         * Syntax: PUSHX_3 ref_point point 18 CALL 
         */
        0xb0, // PUSHB_1
        0x12, //   18
        0x2c, // FDEF
        0x20, //   DUP
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
        0x8a, //     ROLL
        0x20, //     DUP
        0x47, //     GC[cur]
        0x23, //     SWAP
        0x46, //     GC[orig]
        0x23, //     SWAP
        0x61, //     SUB
        0x6a, //     ROUND[white]
        0x60, //     ADD
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
        0x21, //     POP
        0x59, //   EIF
        0x2d, // ENDF

        /* Function 19: used to align a point relatively to a diagonal line,
         * specified by two other points. First we check if the point going
         * to be positioned doesn't deviate too far from the line in the original
         * outline. If the deviation is small enough to neglect it, we use ALIGNRP
         * to position the point, otherwise MDRP is used instead. We can't just
         * always use MDRP, because this command may produce wrong results at
         * small PPEMs, if the original and gridfitted coordinates of the line end
         * points specify slightly different unit vectors.
         * Syntax: point diag_start_point diag_end_point 19 CALL
         */
        0xb0, // PUSHB_1
        0x13, //   19
        0x2c, // FDEF
        0x20, //   DUP
        0x8a, //   ROLL
        0x20, //   DUP
        0x8a, //   ROLL
        0x87, //   SDPVTL[orthogonal]
        0x20, //   DUP
        0xb0, //   PUSHB_1
        0x03, //     4
        0x25, //   CINDEX
        0x4a, //   MD[orig]
        0x64, //   ABS 
        0x23, //   SWAP
        0x8a, //   ROLL
        0x07, //   SPVTL[orthogonal]
        0xb0, //   PUSHB_1
        0x20, //     32
        0x50, //   LT
        0x58, //   IF
        0x3c, //     ALIGNRP
        0x1b, //   ELSE
        0xc0, //     MDRP[grey]
        0x59, //   EIF
        0x2d, // ENDF

        /* Function 20: compute adjustments for X and Y components of projection
         * vector, for aspect ratios different than 1:1, and store them
         * in storage[0] and storage[1] respectively.
         * Syntax: 20 CALL (use it only ONCE, from PREP table).
         */
        0xb0, // PUSHB_1
        0x14, //   20
        0x2c, // FDEF
        0xb3, //   PUSHB_4 (we normally need no adjustments)
        0x00, //     0
        0x40, //     1.0 (F26Dot6)
        0x01, //     1
        0x40, //     1.0 (F26Dot6)
        0x42, //   WS
        0x42, //   WS 
        0x01, //   SVTCA[x-axis]
        0x4b, //   MPPEM
        0xb8, //   PUSHW_1
        0x10, //     4096
        0x00, //     ...still that 4096
        0x63, //   MUL (so we have PPEM along X casted to F26Dot6)
        0x00, //   SVTCA[y-axis]
        0x4b, //   MPPEM
        0xb8, //   PUSHW_1
        0x10, //     4096
        0x00, //     ...still that 4096
        0x63, //   MUL (so we have PPEM along Y casted to F26Dot6)
        0x20, //   DUP
        0x8a, //   ROLL
        0x20, //   DUP
        0x8a, //   ROLL
        0x55, //   NEQ
        0x58, //   IF (if PPEM along X != PPEM along Y)
        0x20, //     DUP
        0x8a, //     ROLL
        0x20, //     DUP
        0x8a, //     ROLL
        0x52, //     GT
        0x58, //     IF (if PPEM along X < PPEM along Y)
        0x23, //       SWAP
        0x62, //       DIV
        0x20, //       DUP
        0xb0, //       PUSHB_1
        0x00, //         0
        0x23, //       SWAP
        0x42, //       WS
        0x1b, //     ELSE (if PPEM along X > PPEM along Y)
        0x62, //       DIV
        0x20, //       DUP
        0xb0, //       PUSHB_1
        0x01, //         1
        0x23, //       SWAP
        0x42, //       WS
        0x59, //     EIF
        0x20, //     DUP [A LOOP STARTS HERE]
        0xb0, //     PUSHB_1
        0x40, //       1.0 (F26Dot6)
        0x52, //     GT
        0x58, //     IF (bigger adjustment is greater than 1.0 => needs fixing)
        0xb2, //       PUSHB_3
        0x00, //         0
        0x20, //         0.5 (F26Dot6)
        0x00, //         0
        0x43, //       RS
        0x63, //       MUL
        0x42, //       WS (we halved adjustment for X)
        0xb2, //       PUSHB_3
        0x01, //         1
        0x20, //         0.5 (F26Dot6)
        0x01, //         1
        0x43, //       RS
        0x63, //       MUL
        0x42, //       WS (we halved adjustment for Y)
        0xb0, //       PUSHB_1
        0x20, //         0.5 (F26Dot6)
        0x63, //       MUL (we halved the bigger adjustment)
        0xb0, //       PUSHB_1
        0x19, //         25
        0x65, //       NEG
        0x1c, //       JMPR (go back to the start of the loop)
        0x21, //       POP
        0x59, //     EIF
        0x1b, //   ELSE (if PPEM along X == PPEM along Y)
        0x21, //     POP
        0x21, //     POP
        0x59, //   EIF
        0x2d, // ENDF

        /* Function 21: call it before SFVFS or SPVFS, so that the vector
         * passed is aspect-ratio corrected.
         * Syntax: x y 21 CALL
         */
        0xb0, // PUSHB_1
        0x15, //   21
        0x2c, // FDEF
        0xb0, //   PUSHB_1
        0x01, //     1
        0x43, //   RS
        0x63, //   MUL
        0x23, //   SWAP
        0xb0, //   PUSHB_1
        0x00, //     0
        0x43, //   RS
        0x63, //   MUL
        0x23, //   SWAP
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
        (tab->len < (int)sizeof(new_fpgm) &&
        !memcmp(tab->data, new_fpgm, tab->len)))
    {
        /* We can safely update font program. */
        tab->len = tab->maxlen = sizeof(new_fpgm);
        tab->data = realloc(tab->data, sizeof(new_fpgm));
        memmove(tab->data, new_fpgm, sizeof(new_fpgm));
        gic->fpgm_done = 1;
    }
    else {
        /* there already is a font program. */
        gic->fpgm_done = 0;
        if (tab->len >= (int)sizeof(new_fpgm))
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
        if (isfinite(gic->blues[i].family_base))
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
        /* Enable dropout control. FreeType 2.3.7 need explicit SCANTYPE. */
        0xb8, // PUSHW_1
        0x01, //   511
        0xff, //   ...still that 511
        0x85, // SCANCTRL
        0xb0, // PUSHB_1
        0x01, //   1
        0x8d, // SCANTYPE

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

    if (gic->fpgm_done)
        prepmaxlen += 3;

    new_prep = calloc(prepmaxlen, sizeof(uint8));
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
    }

    /* compute adjustments for projection vector */
    if (gic->fpgm_done) {
        prep_head = pushnum(prep_head, 20);
        *prep_head++ = CALL;
    }

    preplen = prep_head - new_prep;

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
        tab->data = realloc(tab->data, preplen);
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
void InitGlobalInstrCt(GlobalInstrCt *gic, SplineFont *sf, int layer,
        BlueData *bd) {
    BlueData _bd;

    if (bd == NULL) {
        QuickBlues(sf,layer,&_bd);
        bd = &_bd;
    }

    gic->sf = sf;
    gic->bd = bd;
    gic->layer = layer;
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

/* A line, described by two points */
typedef struct pointvector {
    PointData *pd1, *pd2;
    int done;
} PointVector;

/* In this structure we store information about diagonales,
   relatively to which the given point should be positioned */
typedef struct diagpointinfo {
    struct pointvector line[2];
    int count;
} DiagPointInfo;

typedef struct instrct {
    /* Things that are global for font and should be
       initialized before instructing particular glyph. */
    GlobalInstrCt *gic;

    /* Here things for this particular glyph start. */
    SplineChar *sc;
    SplineSet *ss;

    /* instructions */
    uint8 *instrs;        /* the beginning of the instructions */
    uint8 *pt;            /* the current position in the instructions */

    /* properties indexed by contour number */
    int *contourends;     /* points ending their contours. Null-terminated. */
    uint8 *clockwise;     /* is given contour clockwise? */

    /* properties, indexed by ttf point index. Some could be compressed. */
    int ptcnt;            /* number of points in this glyph */
    BasePoint *bp;        /* point coordinates */
    uint8 *touched;       /* touchflags; points explicitly instructed */
    uint8 *affected;      /* touchflags; almost touched, but optimized out */

    /* data from stem detector */
    GlyphData *gd;

    /* stuff for hinting diagonals */
    int diagcnt;
    StemData **diagstems;
    DiagPointInfo *diagpts; /* indexed by ttf point index */

    /* stuff for hinting edges (stems, blues, strong point interpolation). */
    int xdir;             /* direction flag: x=true, y=false */
    int cdir;             /* is current contour outer? - blues need this */
    struct __edge {
        real base;        /* where the edge is */
        int refpt;        /* best ref. point for an edge, ttf index, -1 if none */
        int refscore;     /* its quality, for searching better one; 0 if none */
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
 * point for an edge.
 */
static int _IsExtremum(int xdir, SplinePoint *sp) {
return xdir?
    (!sp->nonextcp && !sp->noprevcp && sp->nextcp.x==sp->me.x && sp->prevcp.x==sp->me.x):
    (!sp->nonextcp && !sp->noprevcp && sp->nextcp.y==sp->me.y && sp->prevcp.y==sp->me.y);
}

static int IsExtremum(int xdir, int p, SplinePoint *sp) {
    int ret = _IsExtremum(xdir, sp);

    if ((sp->nextcpindex == p) && (sp->next != NULL) && (sp->next->to != NULL))
        ret = ret || _IsExtremum(xdir, sp->next->to);
    else if ((sp->ttfindex != p) && (sp->prev != NULL) && (sp->prev->from != NULL))
        ret = ret || _IsExtremum(xdir, sp->prev->from);

return ret;
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

    if ((sp->pointtype != pt_corner) || (p == 0xffff))
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
        if ((prev != NULL && IsAnglePoint(contourends, bp, prev->to)) || (prev == sp->prev))
    break;
    }

    next = sp->next;
    out = 0;
    while (next != NULL && fabs(out) < CURVATURE_THRESHOLD) {
        out = SplineCurvature(next, 0);
        if (fabs(out) < CURVATURE_THRESHOLD) out = SplineCurvature(next, 1);
        if (fabs(out) < CURVATURE_THRESHOLD) next = next->to->next;
        if ((next != NULL && IsAnglePoint(contourends, bp, next->from)) || (next == sp->next))
    break;
    }

    if (in==0 || out==0 || (prev != sp->prev && next != sp->next))
return 0;

    in/=fabs(in);
    out/=fabs(out);

return (in*out < 0);
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
    int c, p;

    done = (uint8 *)calloc(ct->ptcnt, sizeof(uint8));

    for ( c=0; ss!=NULL; ss=ss->next, ++c ) {
        ct->cdir = ct->clockwise[c];

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

/******************************************************************************
 *
 * Hinting is mostly aligning 'edges' (in FreeType's sense). Each stem hint
 * consists of two edges (or one, for ghost hints). And each blue zone can be
 * represented as an edge with extended fudge (overshoot).
 *
 * Hinting a stem edge is broken in two steps. First: init_stem_edge() seeks for
 * points to snap and chooses one that will be used as a reference point - it
 * should be then instructed elsewhere (a general method of edge positioning).
 * Old init_edge() is still used instead for blue zones and strong points.
 * Finally, finish_edge() instructs the rest of points found with given command,
 * using instructpoints(). It normally optimizes an edge before instructing,
 * but not in presence of diagonal hints.
 *
 * The contour_direction option of init_edge() is for hinting blues - snapping
 * internal contour to a bluezone seems just plainly wrong.
 *
 ******************************************************************************/

/* The following operations have been separated from search_edge(),  */
/* because sometimes it is important to be able to determine, if the */
/* given point is about to be gridfitted or interpolated             */
static int value_point(InstrCt *ct, int p, SplinePoint *sp, real fudge) {
    int score = 0;
    int EM = ct->gic->sf->ascent + ct->gic->sf->descent;
    uint8 touchflag = ct->xdir?tf_x:tf_y;

    if (IsCornerExtremum(ct->xdir, ct->contourends, ct->bp, p) ||
        IsExtremum(ct->xdir, p, sp))
            score+=4;

    if (same_angle(ct->contourends, ct->bp, p, ct->xdir?0.5*M_PI:0.0))
        score++;

    if (p == sp->ttfindex && IsAnglePoint(ct->contourends, ct->bp, sp))
        score++;

    if (interpolate_more_strong && (fudge > (EM/EDGE_FUZZ+0.0001)))
        if (IsExtremum(!ct->xdir, p, sp))
            score++;

    if (IsInflectionPoint(ct->contourends, ct->bp, sp))
        score++;

    if (score && ct->gd->points[p].sp != NULL) /* oncurve */
        score+=2;

    if (!score)
return( 0 );

    if (ct->diagstems != NULL && ct->diagpts[p].count) score+=9;
    if (ct->touched[p] & touchflag) score+=26;
return( score );
}

/* search for points to be snapped to an edge - to be used in RunOnPoints() */
static void search_edge(int p, SplinePoint *sp, InstrCt *ct) {
    int tmp, score;
    real fudge = ct->gic->fudge;
    uint8 touchflag = ct->xdir?tf_x:tf_y;
    real refcoord, coord = ct->xdir?ct->bp[p].x:ct->bp[p].y;

    if (fabs(coord - ct->edge.base) <= fudge)
    {
        score = value_point(ct, p, sp, ct->gic->fudge);
        if (!score)
            return;
        else if (ct->edge.refpt == -1) {
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

            if (ct->edge.othercnt==1) ct->edge.others=(int *)calloc(1, sizeof(int));
            else ct->edge.others=(int *)realloc(ct->edge.others, ct->edge.othercnt*sizeof(int));

            ct->edge.others[ct->edge.othercnt-1] = p;
        }
    }
}

static int StemPreferredForPoint(PointData *pd, StemData *stem,int is_next ) {
    StemData **stems;
    BasePoint bp;
    real off, bestoff;
    int i, is_l, best=0, *stemcnt;

    stems = ( is_next ) ? pd->nextstems : pd->prevstems;
    stemcnt = ( is_next) ? &pd->nextcnt : &pd->prevcnt;

    bestoff = 1e4;
    for ( i=0; i<*stemcnt; i++ ) {
        /* Ghost hints are always assigned to both sides of a point, no matter
         * what the next/previous spline direction is. So we need an additional
         * check for stem unit parallelity */
        if (stems[i]->toobig > stem->toobig || 
            stems[i]->unit.x != stem->unit.x || stems[i]->unit.y != stem->unit.y)
            continue;
        is_l = is_next ? pd->next_is_l[i] : pd->prev_is_l[i];
        bp = is_l ? stems[i]->left : stems[i]->right;
        off =  fabs(( pd->base.x - bp.x )*stem->l_to_r.x +
                    ( pd->base.y - bp.y )*stem->l_to_r.y );
        if (off < bestoff || (RealNear(off, bestoff) && stems[i] == stem)) {
            best = i;
            bestoff = off;
        }
    }
    if (best < *stemcnt && stem == stems[best])
        return( best );

    return( -1 );
}

static int has_valid_dstem( PointData *pd,int next ) {
    int i, cnt;
    StemData *test;

    cnt = next ? pd->nextcnt : pd->prevcnt;
    for ( i=0; i<cnt; i++ ) {
        test = next ? pd->nextstems[i] : pd->prevstems[i];
        if ( !test->toobig && test->lpcnt > 1 && test->rpcnt > 1 &&
            fabs( test->unit.x ) > .05 && fabs( test->unit.y ) > .05 )
            return( i );
    }
    return( -1 );
}

/* init_stem_edge(): Initialize the InstrCt for instructing given edge.
 *
 * Finds points that should be snapped to this hint's given edge.
 * It will return two types of points: a 'chosen one' ct->edge.refpt, that
 * should be used as a reference for this hint, and ct->edge.others that should
 * be positioned after ct.refpt with, for example, SHP.
 *
 * assign_points_to_edge() is a helper function, only to use from init_stem_edge().
 */
static void assign_points_to_edge(InstrCt *ct, StemData *stem, int is_l, int *refidx) {
    int i, previdx, nextidx, test_l, dint_inner = false, flag;
    PointData *pd;

    flag = RealNear( stem->unit.y,1 ) ? tf_x : tf_y;

    for ( i=0; i<ct->gd->realcnt; i++ ) {
        pd = &ct->gd->points[i];
        previdx = StemPreferredForPoint( pd,stem,false );
        nextidx = StemPreferredForPoint( pd,stem,true );
        if (!pd->ticked && (previdx != -1 || nextidx != -1)) {
            pd->ticked = true;
            /* Don't attempt to position inner points at diagonal intersections:
             * our diagonal stem hinter will handle them better */
            if ( ct->diagcnt > 0 && (
                ( stem->unit.y == 1 && pd->x_corner == 2 ) || 
                ( stem->unit.x == 1 && pd->y_corner == 2 ))) {

                dint_inner= has_valid_dstem( pd,true ) != -1 &&
                            has_valid_dstem( pd,false ) != -1;
            }
            test_l = (nextidx != -1) ? 
                pd->next_is_l[nextidx] : pd->prev_is_l[previdx];
            if (test_l == is_l && !dint_inner &&
                !(ct->touched[pd->ttfindex] & flag) && !(ct->affected[pd->ttfindex] & flag)) {
                ct->edge.others = (int *)realloc(
                    ct->edge.others, (ct->edge.othercnt+1)*sizeof(int));
                ct->edge.others[ct->edge.othercnt++] = pd->ttfindex;
                if ( *refidx == -1 ) *refidx = pd->ttfindex;
            }
        }
    }
}

static void init_stem_edge(InstrCt *ct, StemData *stem, int is_l) {
    real left, right, base;
    struct dependent_stem *slave;
    PointData *rpd = NULL;
    int i, *refidx = NULL;

    left = ( stem->unit.x == 0 ) ? stem->left.x : stem->left.y;
    right = ( stem->unit.x == 0 ) ? stem->right.x : stem->right.y;
    base = ( is_l ) ? left : right;

    ct->edge.base = base;
    ct->edge.refpt = -1;
    ct->edge.refscore = 0;
    ct->edge.othercnt = 0;
    ct->edge.others = NULL;

    refidx = ( is_l ) ? &stem->leftidx : &stem->rightidx;
    if ( *refidx != -1 )
        rpd = &ct->gd->points[*refidx];

    /* Don't attempt to position inner points at diagonal intersections:
     * our diagonal stem hinter will handle them better */
    if ( rpd != NULL && ct->diagcnt > 0 && (
        ( stem->unit.y == 1 && rpd->x_corner == 2 ) || 
        ( stem->unit.x == 1 && rpd->y_corner == 2 )) &&
        has_valid_dstem( rpd,true ) != -1 && has_valid_dstem( rpd,false ) != -1 )
        *refidx = -1;

    for ( i=0; i<ct->gd->realcnt; i++ )
        ct->gd->points[i].ticked = false;
    assign_points_to_edge(ct, stem, is_l, refidx);

    for ( i=0; i<stem->dep_cnt; i++ ) {
        slave = &stem->dependent[i];
        if (slave->dep_type == 'a' && 
            ((is_l && slave->lbase) || (!is_l && !slave->lbase))) {

            if ( is_l ) slave->stem->leftidx = *refidx;
            else slave->stem->rightidx = *refidx;
            assign_points_to_edge(ct, slave->stem, is_l, refidx);
        }
    }
    ct->edge.refpt = *refidx;
}

/* Initialize the InstrCt for instructing given edge. */
static void init_edge(InstrCt *ct, real base, int contour_direction) {
    ct->edge.base = base;
    ct->edge.refpt = -1;
    ct->edge.refscore = 0;
    ct->edge.othercnt = 0;
    ct->edge.others = NULL;

    RunOnPoints(ct, contour_direction, &search_edge);
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
 * Optimizers keep points used by diagonal hinter.
 *
 * optimize_strongpts() is used instead of two routines above when hinting
 * inter-stem zones (see interpolate_strong option). It's invoked after
 * instructing diagonal stems.
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
 * parallel to one of coordinate axes. If there are no diagonal hints, we can
 * instruct just one point of a segment, preferring refpt if included, and
 * preferring on-curve points ovef off-curve. Otherwise we must instruct all
 * points used by diagonal hinter along with refpt if included. We mark points
 * that are not to be instructed as 'affected'.
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
	for (i=segstart; i<=segend && ct->gd->points[others[i]].sp == NULL; i++);
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
    ct->edge.others = others = (int *)realloc(others, othercnt*sizeof(int));
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

	if (!(affected[others[i]] & touchflag)) tosnap[others[i]] = 1;
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

/* For any strong point, check whether it's position can rely on other
 * points (if so, we don't have to instruct it explicitly).
 * This optimization is two-pass. 'Obvious' Off-curve points are sweeped
 * first. Some remaining unneeded points (off- and on-curve) may then be
 * optimized out in second pass.
 *
 * TODO! This optimizer could be even more aggressive - it currently
 * skips some features too small or unexposed to benefit from hinting.
 */
static void optimize_strongpts_step1(InstrCt *ct);
static void optimize_strongpts_step2(InstrCt *ct);

static void optimize_strongpts(InstrCt *ct) {
    optimize_strongpts_step1(ct);
    optimize_strongpts_step2(ct);
}

static void optimize_strongpts_step1(InstrCt *ct) {
    int i, j;
    int *others = ct->edge.others;
    int othercnt = ct->edge.othercnt;
    int *contourends = ct->contourends;
    uint8 *tocull, *tocheck;

    if (othercnt == 0)
return;

    tocull = (uint8 *)calloc(ct->ptcnt, sizeof(uint8));
    tocheck = (uint8 *)calloc(ct->ptcnt, sizeof(uint8));
    for(i=0; i<ct->edge.othercnt; i++) tocheck[ct->edge.others[i]] = 1;

    /* for each point of "edge" (would be better called "zone") */
    for(i=0; i<ct->edge.othercnt; i++)
    {
	int pt = others[i];
	double pt_x = ct->bp[pt].x;
	double pt_y = ct->bp[pt].y;

	int pt_next = NextOnContour(contourends, pt);
	double pt_next_x = ct->bp[pt_next].x;
	double pt_next_y = ct->bp[pt_next].y;

	int pt_prev = PrevOnContour(contourends, pt);
	double pt_prev_x = ct->bp[pt_prev].x;
	double pt_prev_y = ct->bp[pt_prev].y;

	/* We sweep only off-curve points here */
	if (ct->gd->points[pt].sp != NULL)
    continue;

	if (IsCornerExtremum(ct->xdir, ct->contourends, ct->bp, pt))
    continue;

	/* Some off-curve points may 'belong' to extrema from other zone. */

	if (/*tocheck[pt_next] &&*/ (ct->gd->points[pt_next].sp != NULL) &&
	    (pt_x == pt_next_x || pt_y == pt_next_y))
		tocull[pt] = 1;

	if (/*tocheck[pt_prev] &&*/ (ct->gd->points[pt_prev].sp != NULL) &&
	    (pt_x == pt_prev_x || pt_y == pt_prev_y))
		tocull[pt] = 1;
    }

    /* remove optimized-out points from list to be instructed. */
    for(i=0; i<ct->edge.othercnt; i++)
	if (tocull[others[i]]) {
	    ct->edge.othercnt--;
	    for(j=i; j<ct->edge.othercnt; j++) others[j] = others[j+1];
	    i--;
	}

    free(tocull);
    free(tocheck);
}

static void optimize_strongpts_step2(InstrCt *ct) {
    int pass, i, j, forward;
    int next_closed, prev_closed;
    int next_pt_max, next_pt_min, prev_pt_max, prev_pt_min;
    int next_coord_max, next_coord_min, prev_coord_max, prev_coord_min;
    int *others = ct->edge.others;
    int othercnt = ct->edge.othercnt;
    int touchflag = (ct->xdir)?tf_x:tf_y;
    int *contourends = ct->contourends;
    uint8 *touched = ct->touched;
    uint8 *affected = ct->affected;
    uint8 *toinstr, *tocull, *tocheck;

    if (othercnt == 0)
return;

    toinstr = (uint8 *)calloc(ct->ptcnt, sizeof(uint8));
    tocull = (uint8 *)calloc(ct->ptcnt, sizeof(uint8));
    tocheck = (uint8 *)calloc(ct->ptcnt, sizeof(uint8));
    for(i=0; i<ct->edge.othercnt; i++) tocheck[ct->edge.others[i]] = 1;

    /* two passes... */
    for(pass=0; pass<2; pass++)
    {
	/* ...for each point of "edge" (would be better called "zone" here) */
	for(i=0; i<ct->edge.othercnt; i++)
	{
	    int pt = others[i];
	    double pt_coord = (ct->xdir) ? ct->bp[pt].x : ct->bp[pt].y;

	    /* In first pass, we sweep only off-curve points */
	    if ((pass==0) && (ct->gd->points[pt].sp != NULL))
	continue;

	    if (tocull[pt] || toinstr[pt])
	continue;

	    /* check path backward and forward */
	    for (forward=0; forward<2; forward++)
	    {
		int closed = 0;
		int pt_max = pt, pt_min = pt;
		double coord_max = pt_coord, coord_min = pt_coord;
		int curr = forward ? NextOnContour(contourends, pt):
				    PrevOnContour(contourends, pt);

		while(curr!=pt)
		{
		    double coord = (ct->xdir) ? ct->bp[curr].x : ct->bp[curr].y;

		    if (fabs(ct->edge.base - coord) > ct->gic->fudge)
		break;

		    if ((touched[curr] | affected[curr]) & touchflag || tocheck[curr])
		    {
			if (coord > coord_max) { coord_max = coord; pt_max = curr; }
			else if ((coord == coord_max) && (curr < pt_max)) pt_max = curr;
			
			if (coord < coord_min) { coord_min = coord; pt_min = curr; }
			else if ((coord == coord_min) && (curr < pt_min)) pt_min = curr;

			closed = 1;
		    }

		    if ((touched[curr] | affected[curr]) & touchflag || toinstr[curr])
		break;

		    curr = forward ? NextOnContour(contourends, curr):
				    PrevOnContour(contourends, curr);
		}

		if (forward) {
		    next_closed = closed;
		    next_pt_max = pt_max;
		    next_pt_min = pt_min;
		    next_coord_max = coord_max;
		    next_coord_min = coord_min;
		}
		else {
		    prev_closed = closed;
		    prev_pt_max = pt_max;
		    prev_pt_min = pt_min;
		    prev_coord_max = coord_max;
		    prev_coord_min = coord_min;
		}
	    }

	    if (prev_closed && next_closed && (
		(prev_coord_max >= pt_coord && pt != prev_pt_max && 
		 next_coord_min <= pt_coord && pt != next_pt_min) ||
		(prev_coord_min <= pt_coord && pt != prev_pt_min && 
		 next_coord_max >= pt_coord && pt != next_pt_max)))
		    tocull[pt] = 1;
	    else
		toinstr[pt] = 1;
	}
    }

    /* remove optimized-out points from list to be instructed. */
    for(i=0; i<ct->edge.othercnt; i++)
	if (tocull[others[i]]) {
	    ct->edge.othercnt--;
	    for(j=i; j<ct->edge.othercnt; j++) others[j] = others[j+1];
	    i--;
	}

    free(tocheck);
    free(toinstr);
    free(tocull);
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

/* Each stem hint has 'ldone' and 'rdone' flag, indicating whether 'left'
 * or 'right' edge is hinted or not. This functions marks as done all edges at
 * specified coordinate, starting from given hint (hints sometimes share edges).
 */
static void mark_startenddones(StemData *stem, int is_l ) {
    struct dependent_stem *slave;
    int i;
    uint8 *done;

    done = is_l ? &stem->ldone : &stem->rdone;
    *done = true;
    for (i=0; i<stem->dep_cnt; i++) {
        slave = &stem->dependent[i];
        if ( slave->dep_type == 'a' && slave->lbase == is_l ) {
            done = is_l ? &slave->stem->ldone : &slave->stem->rdone;
            *done = true;
        }
    }
}

static void build_cvt_stem(InstrCt *ct, real width, StdStem *cvt_stem) {
    int i, width_parent, width_me;
    int EM = ct->gic->sf->ascent + ct->gic->sf->descent;
    
    cvt_stem->width = (int)rint(fabs(width));
    cvt_stem->stopat = 32767;
    cvt_stem->snapto =
	CVTSeekStem(ct->xdir, ct->gic, width, false);

    for (i=7; i<32768; i++) {
	width_parent = compute_stem_width(ct->xdir, cvt_stem->snapto, EM, i);
	width_me = compute_stem_width(ct->xdir, cvt_stem, EM, i);

	if (width_parent != width_me) {
	    cvt_stem->stopat = i;
	    break;
	}
    }
}

/* This function has been separated from finish_stem(), because sometimes
 * it is necessary to maintain the distance between two points (usually on
 * opposite stem edges) without instructing the whole stem. Currently we use this
 * to achieve proper positioning of the left edge of a vertical stem in antialiased
 * mode, if instructing this stem has to be started from the right edge 
 */
static void maintain_black_dist(InstrCt *ct, real width, int refpt, int chg_rp0) {
    int callargs[5];
    StdStem *StdW = ct->xdir?&(ct->gic->stdvw):&(ct->gic->stdhw);
    StdStem *ClosestStem;
    StdStem cvt_stem;

    ClosestStem = CVTSeekStem(ct->xdir, ct->gic, width, true);

    if (ClosestStem != NULL) {
	ct->pt = push2nums(ct->pt, refpt, ClosestStem->cvtindex);

	if (ct->gic->cvt_done && ct->gic->fpgm_done && ct->gic->prep_done)
	    *(ct->pt)++ = chg_rp0?MIRP_rp0_min_black:MIRP_min_black;
	else *(ct->pt)++ = chg_rp0?MIRP_min_rnd_black:MIRP_rp0_min_rnd_black;
    }
    else {
	if (ct->gic->cvt_done && ct->gic->fpgm_done && ct->gic->prep_done &&
	    StdW->width!=-1)
	{
	    build_cvt_stem(ct, width, &cvt_stem);

	    callargs[0] = ct->edge.refpt;
	    callargs[1] = cvt_stem.snapto->cvtindex;
	    callargs[2] = chg_rp0?1:0;
	    callargs[3] = cvt_stem.stopat;
	    callargs[4] = 4;
	    ct->pt = pushnums(ct->pt, 5, callargs);
	    *(ct->pt)++ = CALL;
	}
	else {
	    ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	    *(ct->pt)++ = chg_rp0?MDRP_rp0_min_rnd_black:MDRP_min_rnd_black;
	}
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
static void finish_stem(StemData *stem, int shp_rp1, int chg_rp0, InstrCt *ct)
{
    int is_l, basedone, oppdone, reverse;
    real hleft, hright, width;

    if (stem == NULL)
        return;
    hleft = ((real *) &stem->left.x)[!ct->xdir];
    hright= ((real *) &stem->right.x)[!ct->xdir];

    is_l = (fabs(hleft - ct->edge.base) < fabs(hright - ct->edge.base));
    basedone = ( is_l && stem->ldone ) || ( !is_l && stem->rdone );
    oppdone = ( is_l && stem->rdone ) || ( !is_l && stem->ldone );
    reverse = ( ct->xdir && !is_l && !stem->ldone && !stem->ghost );
    width = stem->width;

    if ( !reverse && !basedone ) {
        ct->touched[ct->edge.refpt] |= ct->xdir?tf_x:tf_y;
        finish_edge(ct, shp_rp1?SHP_rp1:SHP_rp2);
        mark_startenddones(stem, is_l );
    }

    if (oppdone || (stem->ghost && ((stem->width==20) || (stem->width==21)))) {
        stem->ldone = stem->rdone = 1;
        return;
    }

    init_stem_edge(ct, stem, !is_l);
    if (ct->edge.refpt == -1) {
        /* We have skipped the right edge to start instructing this stem from
         * left. But its left edge appears to have no points to be instructed.
         * So return to the right edge and instruct it before exiting */
        if ( reverse && !basedone ) {
            init_stem_edge(ct, stem, is_l);
            ct->touched[ct->edge.refpt] |= ct->xdir?tf_x:tf_y;
            finish_edge(ct, shp_rp1?SHP_rp1:SHP_rp2);
            mark_startenddones(stem, is_l );
        }
        return;
    }
    maintain_black_dist(ct, width, ct->edge.refpt, chg_rp0);

    if ( reverse ) {
        is_l = !is_l;
        ct->rp0 = ct->edge.refpt;
        ct->pt = pushpoint(ct->pt, ct->rp0);
        *(ct->pt)++ = MDAP_rnd;
        ct->touched[ct->edge.refpt] |= ct->xdir?tf_x:tf_y;
        finish_edge(ct, SHP_rp1);
        mark_startenddones( stem, is_l );
        if ( !stem->rdone ) {
            init_stem_edge(ct, stem, false);
            if (ct->edge.refpt == -1)
                return;
            maintain_black_dist(ct, width, ct->edge.refpt, chg_rp0);
        }
    }

    if (chg_rp0) ct->rp0 = ct->edge.refpt;
    ct->touched[ct->edge.refpt] |= ct->xdir?tf_x:tf_y;
    finish_edge(ct, SHP_rp2);
    mark_startenddones( stem, !is_l );
}

static void mark_points_affected(InstrCt *ct,StemData *target,PointData *opd,int next) {
    Spline *s;
    PointData *pd, *cpd;
    int cpidx;

    s  = next ? opd->sp->next : opd->sp->prev;
    pd = next ? &ct->gd->points[s->to->ptindex] : &ct->gd->points[s->from->ptindex];
    while (IsStemAssignedToPoint(pd, target, !next) == -1) {
        if (pd->ttfindex < ct->gd->realcnt &&
            value_point(ct, pd->ttfindex, pd->sp, ct->gd->emsize))
            ct->affected[pd->ttfindex] |= ct->xdir?tf_x:tf_y;

        if (!pd->sp->noprevcp) {
            cpidx = pd->sp->prev->from->nextcpindex;
            cpd = &ct->gd->points[cpidx];
            if (value_point(ct, cpd->ttfindex, pd->sp, ct->gd->emsize))
                ct->affected[cpd->ttfindex] |= ct->xdir?tf_x:tf_y;
        }
        if (!pd->sp->nonextcp) {
            cpidx = pd->sp->nextcpindex;
            cpd = &ct->gd->points[cpidx];
            if (value_point(ct, cpd->ttfindex, pd->sp, ct->gd->emsize))
                ct->affected[cpd->ttfindex] |= ct->xdir?tf_x:tf_y;
        }
        s =  next ? pd->sp->next : pd->sp->prev;
        pd = next ? &ct->gd->points[s->to->ptindex] : &ct->gd->points[s->from->ptindex];
        if ( pd == opd ) {
            IError( "The ball terminal with a key point at %.3f,%.3f\n"
                    "appears to be incorrectly linked to the %s stem\n"
                    "<%.3f, %.3f>",
                    pd->base.x,pd->base.y,
                    ct->xdir?"vertical":"horizontal",
                    ct->xdir?target->left.x:target->right.y,target->width );
            break;
        }
    }
}

static void finish_serif(StemData *slave, StemData *master, int lbase, int is_ball, InstrCt *ct)
{
    int inner_pt, callargs[4];
    struct stem_chunk *chunk;
    PointData *opd;
    int i;

    if (slave == NULL || master == NULL)
return;
    inner_pt = ( lbase ) ? master->rightidx : master->leftidx;

    init_stem_edge(ct, slave, !lbase);
    if (ct->edge.refpt == -1)
return;

    if (ct->gic->fpgm_done) {
        callargs[0] = is_ball ? 0 : 64;
        callargs[1] = inner_pt;
        callargs[2] = ct->edge.refpt;
        callargs[3] = 9;
        ct->pt = pushnums(ct->pt, 4, callargs);
        *(ct->pt)++ = CALL;
    }
    else {
	*(ct->pt)++ = 0x7D; /* RDTG */
	ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	*(ct->pt)++ = MDRP_min_rnd_black;
	*(ct->pt)++ = 0x18; /* RTG */
    }

    ct->touched[ct->edge.refpt] |= ct->xdir?tf_x:tf_y;
    finish_edge(ct, SHP_rp2);
    mark_startenddones( slave, !lbase );

    if ( !interpolate_strong || !instruct_ball_terminals )
return;

    /* Preserve points on ball terminals from being interpolated
     * between edges by marking them as affected */
    for ( i=0; i<slave->chunk_cnt; i++ ) {
        chunk = &slave->chunks[i];
        opd = lbase ? chunk->r : chunk->l;

        if (chunk->is_ball && opd != NULL) {
            mark_points_affected(ct, chunk->ball_m, opd, true);
            mark_points_affected(ct, chunk->ball_m, opd, false);
        }
    }
}

static void link_serifs_to_edge(InstrCt *ct, StemData *stem, int is_l) {
    int i, callargs[3];
    struct dependent_serif *serif;

    /* We use an FPGM function to set rp0, and thus the exact value
     * is not known at the compilation time. So it is safer to reset
     * ct->rp0 to -1
     */
    if ( ct->gic->fpgm_done ) {
        ct->rp0 = -1;
        callargs[0] = is_l ? stem->rightidx : stem->leftidx;
        callargs[1] = is_l ? stem->leftidx : stem->rightidx;
        callargs[2] = 10;
        ct->pt = pushnums(ct->pt, 3, callargs);
        *(ct->pt)++ = CALL;
    } else {
        init_stem_edge(ct, stem, !is_l);
        if ( ct->rp0 != ct->edge.refpt ) {
            ct->pt = pushpoint(ct->pt, ct->edge.refpt);
            *(ct->pt)++ = SRP0;
            ct->rp0 = ct->edge.refpt;
        }
    }
    for (i=0; i<stem->serif_cnt; i++) {
        serif = &stem->serifs[i];
        if (serif->lbase == is_l && 
            ((serif->is_ball && instruct_ball_terminals) ||
            (!serif->is_ball && instruct_serif_stems)))
            finish_serif( serif->stem,stem,is_l,serif->is_ball,ct );
    }
}

static void instruct_serifs(InstrCt *ct, StemData *stem) {
    int i, lcnt, rcnt;
    struct dependent_serif *serif;

    if ( stem->leftidx == -1 || stem->rightidx == -1 )
        return;
    lcnt = rcnt = 0;
    for (i=0; i<stem->serif_cnt; i++) {
        serif = &stem->serifs[i];
        if ((serif->is_ball && !instruct_ball_terminals) ||
            (!serif->is_ball && !instruct_serif_stems))
                continue;
        if ( serif->lbase )
            lcnt++;
        else if ( !serif->lbase )
            rcnt++;
    }

    if (stem->ldone && lcnt > 0)
        link_serifs_to_edge(ct, stem, true);
    if (stem->rdone && rcnt > 0)
        link_serifs_to_edge(ct, stem, false);
}

static void instruct_dependent(InstrCt *ct, StemData *stem) {
    int i, j, rp, rp1, rp2, stopat, callargs[4];
    struct dependent_stem *slave;
    int w_master, w_slave;
    StdStem *std_master, *std_slave, norm_master, norm_slave;
    StdStem *StdW = ct->xdir?&(ct->gic->stdvw):&(ct->gic->stdhw);

    for (i=0; i<stem->dep_cnt; i++) {
        slave = &stem->dependent[i];
        if (slave->stem->master == NULL)
            continue;

        init_stem_edge(ct, slave->stem, slave->lbase);
        if (ct->edge.refpt == -1) continue;

        if (slave->dep_type == 'i' && stem->ldone && stem->rdone) {
            rp1 = ct->xdir ? stem->leftidx : stem->rightidx;
            rp2 = ct->xdir ? stem->rightidx : stem->leftidx;
            callargs[0] = ct->edge.refpt;
            callargs[1] = rp2;
            callargs[2] = rp1;
            if (ct->gic->fpgm_done) {
                callargs[3] = 8;
	        ct->pt = pushpoints(ct->pt, 4, callargs);
	        *(ct->pt)++ = CALL;
            } else {
	        ct->pt = pushpoints(ct->pt, 3, callargs);
	        *(ct->pt)++ = SRP1;
	        *(ct->pt)++ = SRP2;
	        *(ct->pt)++ = DUP;
	        *(ct->pt)++ = IP;
	        *(ct->pt)++ = MDAP_rnd;
            }
        }
        else if (slave->dep_type == 'm' &&
            ((slave->lbase && stem->ldone) || (!slave->lbase && stem->rdone))) {

            rp = slave->lbase ? stem->leftidx : stem->rightidx;
            if ( rp != ct->rp0 ) {
                ct->pt = pushpoint(ct->pt, rp);
	        *(ct->pt)++ = SRP0;
                ct->rp0 = rp;
            }

            /* It is possible that at certain PPEMs both the master and slave stems are
             * regularized, say, to 1 pixel, but the difference between their positions
             * is rounded to 1 pixel too. Thus one stem is shifted relatively to another,
             * so that the overlap disappears. This looks especially odd for nesting/nested
             * stems. We use a special FPGM function to prevent this.
             */
	    if ( ct->gic->cvt_done && ct->gic->fpgm_done && ct->gic->prep_done && StdW->width!=-1 && (
                ((&stem->left.x)[!ct->xdir] <= (&slave->stem->left.x)[!ct->xdir] &&
                ( &stem->right.x)[!ct->xdir] >= (&slave->stem->right.x)[!ct->xdir] ) ||
                ((&stem->left.x)[!ct->xdir] >= (&slave->stem->left.x)[!ct->xdir] &&
                ( &stem->right.x)[!ct->xdir] <= (&slave->stem->right.x)[!ct->xdir] ))) {

                std_master = CVTSeekStem(ct->xdir, ct->gic, stem->width, true);
                std_slave  = CVTSeekStem(ct->xdir, ct->gic, slave->stem->width, true);
                if ( std_master == NULL ) {
                    build_cvt_stem(ct, stem->width, &norm_master);
                    std_master = &norm_master;
                }
                if ( std_slave == NULL ) {
                    build_cvt_stem(ct, slave->stem->width, &norm_slave);
                    std_slave = &norm_slave;
                }

                stopat = 32768;
                for (j=7; j<=stopat; j++) {
	            w_master = compute_stem_width(ct->xdir, std_master, ct->gd->emsize, j);
		    w_slave  = compute_stem_width(ct->xdir, std_slave , ct->gd->emsize, j);

		    if (w_master != w_slave)
		        stopat = j;
	        }
                callargs[0] = stopat;
                callargs[1] = ct->rp0;
                callargs[2] = ct->edge.refpt;
                callargs[3] = 14;
	        ct->pt = pushpoints(ct->pt, 4, callargs);
	        *(ct->pt)++ = CALL;
            }
            else {
                ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	        *(ct->pt)++ = DUP;
	        *(ct->pt)++ = MDRP_rp0_rnd_white;
	        *(ct->pt)++ = SRP1;
            }
        }
        else if (slave->dep_type == 'a' &&
            ((slave->lbase && stem->ldone) || (!slave->lbase && stem->rdone))) {
            if ( ct->edge.refpt != ct->rp0 ) {
                ct->pt = pushpoint(ct->pt, ct->edge.refpt);
	        *(ct->pt)++ = SRP0;
            }
        }
        else
            continue;

        ct->rp0 = ct->edge.refpt;
        finish_stem(slave->stem, use_rp1, keep_old_rp0, ct);
        if ( instruct_serif_stems || instruct_ball_terminals )
            instruct_serifs(ct, slave->stem);

        instruct_dependent(ct, slave->stem);
    }
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
 * TODO! We currently instruct hints dependent on those controlled by blues.
 * This may be not always corrrect (e.g. if a dependent hint is itself
 * controlled by blue zone - possibly even different). Research needed.
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
 * any glyph's point at needed height, but we're not certain we'll find any.
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

static void check_blue_pts(InstrCt *ct) {
    BasePoint *bp = ct->bp;
    BlueZone *blues = ct->gic->blues;
    int i, j, bluecnt = ct->gic->bluecnt;

    for (i=0; i<bluecnt; i++)
        if (blues[i].lowest != -1)
            for (j=0; j<bluecnt; j++)
                if (i != j && blues[j].lowest != -1 && SegmentsOverlap(
                        bp[blues[i].lowest].y, bp[blues[i].highest].y,
                        bp[blues[j].lowest].y, bp[blues[j].highest].y))
                    fixup_blue_pts(blues+i, blues+j);
}

static int snap_stem_to_blue(InstrCt *ct,StemData *stem, BlueZone *blue, int idx) {
    int i, is_l, ret = 0;
    int callargs[3] = { 0/*pt*/, 0/*cvt*/, 0 };
    real base, advance, tmp;
    real fuzz = GetBlueFuzz(ct->gic->sf);
    StemData *slave;

    /* Which edge to start at? */
    /* Starting at the other would usually be wrong. */
    if (blue->overshoot < blue->base && ( !stem->ghost || stem->width == 21 ))
    {
        is_l = false;
        base = stem->right.y;
        advance = stem->left.y;
    }
    else {
        is_l = true;
        base = stem->left.y;
        advance = stem->right.y;
    }

    /* This is intended as a fallback if the base edge wasn't within
     * this bluezone, and advance edge was.
     */
    if (!stem->ghost &&
        !SegmentsOverlap(base+fuzz, base-fuzz, blue->base, blue->overshoot) &&
        SegmentsOverlap(advance+fuzz, advance-fuzz, blue->base, blue->overshoot))
    {
        tmp = base;
        base = advance;
        advance = tmp;
        is_l = !is_l;
    }

    /* instruct the stem */
    init_stem_edge(ct, stem, is_l);
    if (ct->edge.refpt == -1) {
        for ( i=0; i<stem->dep_cnt; i++ ) {
            slave = stem->dependent[i].stem;
            /* A hack which allows single-edge hints to tie features
             * to remote blue zones. */
            if ( stem->ghost ) slave->blue = idx;
            if ( slave->blue == idx )
                ret += snap_stem_to_blue(ct, slave, blue, idx);
        }
        return( ret );
    }
    update_blue_pts(idx, ct);
    callargs[0] = ct->rp0 = ct->edge.refpt;
    callargs[1] = blue->cvtindex;

    if (ct->gic->fpgm_done) {
        ct->pt = pushpoints(ct->pt, 3, callargs);
        *(ct->pt)++ = CALL;
    }
    else {
        ct->pt = pushpoints(ct->pt, 2, callargs);
        *(ct->pt)++ = MIAP_rnd;
    }

    finish_stem(stem, use_rp1, keep_old_rp0, ct);
    for ( i=0; i<stem->dep_cnt; i++ ) {
        slave = stem->dependent[i].stem;
        if ( slave->blue == idx ) {
            ret += snap_stem_to_blue(ct, slave, blue, idx);
            slave->master = NULL;
        }
    }

    if( instruct_serif_stems || instruct_ball_terminals )
        instruct_serifs(ct, stem);
    instruct_dependent(ct, stem);
    update_blue_pts(idx, ct); /* this uses only refpt: who cares? */
    return( ret + 1 );
}

/* Snap stems and perhaps also some other points to given bluezone and set up
 * its 'highest' and 'lowest' point indices.
 */
static void snap_to_blues(InstrCt *ct) {
    int i, j;
    int therewerestems;      /* were there any HStems snapped to this blue? */
    StemData *stem;          /* for HStems affected by blues */
    real base; /* for the hint */
    int callargs[3] = { 0/*pt*/, 0/*cvt*/, 0 };
    real fudge;
    int bluecnt=ct->gic->bluecnt;
    int queue[12];           /* Blue zones' indices in processing order */
    BlueZone *blues = ct->gic->blues;
    real fuzz = GetBlueFuzz(ct->gic->sf);

    if (bluecnt == 0)
return;

    /* Fill the processing queue - baseline goes first, then bottom zones */
    /* sorted by base in ascending order, then top zones sorted in descending */
    /* order. I assume the blues are sorted in ascending order first. */
    for (i=0; (i < bluecnt) && (blues[i].base < 0); i++);
    queue[0] = i;
    for (i=0; i<queue[0]; i++) queue[i+1] = i;
    for (i=queue[0]+1; i<bluecnt; i++) queue[i] = bluecnt - i + queue[0];

    /* Process the blues. */
    for (i=0; i<bluecnt; i++) {
	therewerestems = 0;

	/* Process all hints with edges within current blue zone. */
	for ( j=0; j<ct->gd->hbundle->cnt; j++ ) {
            stem = ct->gd->hbundle->stemlist[j];
	    if (stem->master != NULL || stem->blue != queue[i] || stem->ldone || stem->rdone)
                continue;

	    therewerestems += snap_stem_to_blue(ct, stem, &blues[queue[i]], queue[i]);
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

	    if (ct->edge.others != NULL) {
		free(ct->edge.others);
		ct->edge.others = NULL;
		ct->edge.othercnt = 0;
	    }

	    ct->gic->fudge = fudge;
	}
    }

    check_blue_pts(ct);
}

static int get_counters_cut_in(InstrCt *ct,  int m1, int m2, int c1, int c2) {
    real s1, e1, s2, e2, width1, width2;
    int i, swidth1, swidth2;
    int EM = ct->gic->sf->ascent + ct->gic->sf->descent;
    
    s1 = (&ct->gd->points[m1].base.x)[!ct->xdir];
    e1 = (&ct->gd->points[m2].base.x)[!ct->xdir];
    s2 = (&ct->gd->points[c1].base.x)[!ct->xdir];
    e2 = (&ct->gd->points[c2].base.x)[!ct->xdir];
    width1 = e1 - s1; width2 = e2 - s2;
    
    if ( RealNear( width1, width2 ))
        return( 0 );
    
    for (i=7; i<32768; i++) {
        swidth1 = (int)rint((rint(fabs(width1)) * i * 64.0)/EM);
        swidth2 = (int)rint((rint(fabs(width2)) * i * 64.0)/EM);
        if ( fabs(swidth1 - swidth2) >= SNAP_THRESHOLD )
            break;
    }
    return( i );
}

/******************************************************************************
 *
 * High-level functions for instructing horizontal and vertical stems.
 * Both use 'geninstrs' for positioning single, elementary stems.
 *
 ******************************************************************************/

/* geninstrs's main burden is to choose the better of two reference points
 * found by init_stem_edge() - one for each edge - and position it relatively
 * to other stems (if not already done).
 *
 * If none of the edges is positioned:
 *   If this hint is the first, previously overlapped, or simply horizontal,
 *   position the reference point at the base where it is using MDAP; otherwise
 *   position the hint's base rp0 relatively to the previous hint's end using
 *   MDRP with white minimum distance (fpgm function 1).
 *
 * Calling finish_stem() will deal with the rest of points needing explicit
 * positioning. Then we instruct serifs and dependent stems, if wanted.
 */
static void geninstrs(InstrCt *ct, StemData *stem, StemData *prev, int lbase) {
    int shp_rp1, chg_rp0, c_m_pt1 = -1, c_m_pt2 = -1;
    int callargs[6];
    real prev_pos = 0, cur_pos;

    if (stem->ldone && stem->rdone)
        return;
    if ((lbase && stem->rdone) || (!lbase && stem->ldone))
        lbase = !lbase;
    init_stem_edge(ct, stem, lbase);
    if (ct->edge.refpt == -1) {
        lbase = !lbase;
        init_stem_edge(ct, stem, lbase);
    }
    if (ct->edge.refpt == -1)
        return;

    if (ct->rp0 < ct->gd->realcnt && ct->rp0 >= 0) 
        prev_pos = (&ct->gd->points[ct->rp0].base.x)[!ct->xdir];
    cur_pos = (&ct->gd->points[ct->edge.refpt].base.x)[!ct->xdir];

    if (prev != NULL && stem->prev_c_m != NULL && prev->next_c_m != NULL ) {
        c_m_pt1 = ct->xdir ? prev->next_c_m->rightidx : prev->next_c_m->leftidx;
        c_m_pt2 = ct->xdir ? stem->prev_c_m->leftidx  : stem->prev_c_m->rightidx;
    }

    /* Now the stem's origin must be placed in respect to others... */
    /* TODO! What's really needed here is an iterative procedure that */
    /* would preserve counters and widths, like in freetype2. */
    /* For horizontal stems, interpolating between blues is being be done. */

    if (stem->ldone || stem->rdone ) {
        ct->pt = pushpoint(ct->pt, ct->edge.refpt);
        *(ct->pt)++ = MDAP; /* sets rp0 and rp1 */
        shp_rp1 = use_rp1;
        chg_rp0 = (ct->xdir && !lbase) || (!ct->xdir && lbase);
    }
    else if (!ct->xdir) { /* horizontal stem */
        ct->pt = pushpoint(ct->pt, ct->edge.refpt);
        *(ct->pt)++ = MDAP_rnd;
        shp_rp1 = use_rp1;
        chg_rp0 = keep_old_rp0;
    }
    else if (prev == NULL) { /* first vertical stem */
        ct->pt = pushpoint(ct->pt, ct->edge.refpt);
        *(ct->pt)++ = MDRP_rp0_rnd_white;
        shp_rp1 = use_rp2;
        chg_rp0 = keep_old_rp0;
    }
    else {
        if (ct->gic->fpgm_done) {
            if ( control_counters && c_m_pt1 != -1 && c_m_pt2 != -1 ) {
                callargs[0] = c_m_pt1;
                callargs[1] = c_m_pt2;
                callargs[2] = ct->rp0;
                callargs[3] = ct->edge.refpt;
                callargs[4] = get_counters_cut_in(ct,  c_m_pt1, c_m_pt2, ct->rp0, ct->edge.refpt);
                callargs[5] = 15;
                ct->pt = pushpoints(ct->pt, 6, callargs);

            } else if ( control_counters && prev != NULL && prev->leftidx != -1 && prev->rightidx != -1 ) {
                callargs[0] = ct->xdir ? prev->leftidx : prev->rightidx;
                callargs[1] = ct->edge.refpt;
                callargs[2] = ( cur_pos - prev_pos ) > ct->gic->fudge ? 16 : 17;
                ct->pt = pushpoints(ct->pt, 3, callargs);

            } else if ( fabs( cur_pos - prev_pos ) > ct->gic->fudge ) {
                ct->pt = push2nums(ct->pt, ct->edge.refpt, 1);
            } else {
                ct->pt = push2nums(ct->pt, ct->edge.refpt, 11);
            }
            *(ct->pt)++ = CALL;
        }
        else {
            ct->pt = pushpoint(ct->pt, ct->edge.refpt);
            if ( fabs( cur_pos - prev_pos ) > ct->gic->fudge )
                *(ct->pt)++ = MDRP_rp0_min_rnd_grey;
            else
                *(ct->pt)++ = MDRP_rp0_rnd_white;
        }
        shp_rp1 = use_rp2;

        /* Don't switch rp0 to the second edge. Thus, relative distance
         * to the next stem is be larger, and errors are hopefully lesser.
         * TODO! This is disputable.
         * TODO! For the last vstem, we probably want to switch rp0 anyway.
         */
        chg_rp0 = keep_old_rp0;
    }
    ct->rp0 = ct->edge.refpt;
    finish_stem(stem, shp_rp1, chg_rp0, ct);
    if ( instruct_serif_stems || instruct_ball_terminals )
        instruct_serifs(ct, stem);

    instruct_dependent(ct, stem);
}

/* High-level function for instructing horizontal stems.
 *
 * It is assumed that blues (and hstems associated with them) are already
 * done so that remaining stems can be interpolated between them.
 *
 * TODO! CJK hinting will probably need different function (HStemGeninstCJK?)
 * TODO! Instruct top and bottom bearings for fonts which have them.
 */
static void HStemGeninst(InstrCt *ct) {
    BlueZone *blues = ct->gic->blues;
    int bluecnt = ct->gic->bluecnt;
    BasePoint *bp = ct->bp;
    StemData *stem;
    int i, j, rp1, rp2, opp, bpt, ept;
    double hbase, hend;
    int mdrp_end, mdrp_base, ip_base, *rpts1, *rpts2;
    int callargs[5];

    if ( ct->gd->hbundle == NULL )
        return;
    rpts1 = calloc(ct->gd->hbundle->cnt, sizeof(int));
    rpts2 = calloc(ct->gd->hbundle->cnt, sizeof(int));

    /* Interpolating between blues is splitted to two stages: first
     * we determine which stems can be interpolated and which cannot
     * and store the numbers of reference points, and then (in the
     * second cycle) proceed to generating actual instructions. The reason is
     * that we need a special handling for dependent stems: if they
     * can be interpolated, we process them separately, but otherwise 
     * the normal algorithm for positioning dependent stems relatively
     * to their "masters" is used. It is necessary to know which method
     * to prefer for each stem at the time instructions are generated.
     */
    for ( i=0; i<ct->gd->hbundle->cnt; i++ )
    {
        stem = ct->gd->hbundle->stemlist[i];
	if (!stem->ldone && !stem->rdone)
	{
	    /* Set up upper edge (hend) and lower edge (hbase). */
	    hbase = stem->right.y;
	    hend = stem->left.y;

	    /* Find two points to interpolate the HStem between.
	       rp1 = lower, rp2 = upper. */
	    rp1 = -1;
	    rp2 = -1;

	    for (j=0; j<bluecnt; j++) {
	        if (blues[j].lowest == -1) // implies blues[j].highest==-1 too
	            continue;

	        if (bp[blues[j].lowest].y < hbase)
		    if (rp1==-1 || bp[rp1].y < bp[blues[j].lowest].y)
		        rp1=blues[j].lowest;

	        if (bp[blues[j].highest].y > hend)
		    if (rp2==-1 || bp[rp2].y > bp[blues[j].highest].y)
		        rp2=blues[j].highest;
	    }
            rpts1[i] = rp1; rpts2[i] = rp2;

            /* If a dependent stem has to be positioned by interpolating
             * one of its edges between the edges of the master stem and
             * we have found reference points to interpolate it between
             * blues, then we prefer to interpolate it between blues. However
             * we keep the standard handling for other types of dependent
             * stems, since usually positioning relatively to the "master"
             * stem is more important than positioning relatively to blues
             * in such cases.
             * Exception: nested stems marked for interpolation should be
             * positioned by interpolating between the edges of the nesting
             * stem.
             */
            if (rp1!=-1 && rp2!=-1 && stem->master != NULL)
                for (j=0; j<stem->master->dep_cnt; j++) {
                    if (stem->master->dependent[j].stem == stem &&
                        stem->master->dependent[j].dep_type == 'i' &&
                        (stem->master->left.y <= stem->left.y ||
                        stem->master->right.y >= stem->right.y)) {
                        stem->master = NULL;
                        break;
                    }
                }
        }
    }

    for ( i=0; i<ct->gd->hbundle->cnt; i++ )
    {
        stem = ct->gd->hbundle->stemlist[i];
        if ( stem->master != NULL )
            continue;
	if (!stem->ldone && !stem->rdone)
	{
	    hbase = stem->right.y;
	    hend = stem->left.y;
            
	    rp1 = rpts1[i]; rp2 = rpts2[i];
            /* Reference points not found? Fall back to old method. */
	    if (rp1==-1 || rp2==-1) {
		geninstrs(ct, stem, NULL, false);
		continue;
            }

	    bpt = ept = -1;
	    if ( !stem->ghost || stem->width == 21 ) {
	        init_stem_edge(ct, stem, false);
	        bpt = ct->edge.refpt;
	    }
	    if ( !stem->ghost || stem->width == 20 ) {
	        init_stem_edge(ct, stem, true);
	        ept = ct->edge.refpt;
	    }
	    if ( bpt == -1 && ept == -1 )
	        continue;

	    /* Align the stem relatively to rp0 and rp1. */
	    mdrp_end = ept != -1 &&
	        fabs(bp[rp2].y - hbase) < 0.2*fabs(bp[rp2].y - bp[rp1].y);
	    mdrp_base = bpt != -1 && 
	        fabs(bp[rp1].y - hend) < 0.2*fabs(bp[rp2].y - bp[rp1].y);

	    if (mdrp_end || mdrp_base) {
		if (mdrp_end) init_stem_edge(ct, stem, true);
		else init_stem_edge(ct, stem, false);

		if (ct->edge.refpt == -1) continue;

		if (mdrp_end) ct->pt = push2points(ct->pt, ct->edge.refpt, rp2);
		else ct->pt = push2points(ct->pt, ct->edge.refpt, rp1);

		*(ct->pt)++ = SRP0;
		*(ct->pt)++ = DUP;
		*(ct->pt)++ = MDRP_grey;
		*(ct->pt)++ = MDAP_rnd;
	    }
	    else if ( bpt == -1 || ept == -1 ) {
	        ip_base = ( ept == -1 );
	        init_stem_edge(ct, stem, !ip_base);
		if ( ct->gic->fpgm_done ) {
                    callargs[0] = ct->edge.refpt;
		    callargs[1] = rp1;
		    callargs[2] = rp2;
		    callargs[3] = 8;
		    ct->pt = pushnums(ct->pt, 4, callargs);
		    *(ct->pt)++ = CALL;
		}
		else {
                    callargs[0] = ct->edge.refpt;
		    callargs[1] = rp1;
		    callargs[2] = rp2;
		    ct->pt = pushnums(ct->pt, 3, callargs);
                    *(ct->pt)++ = SRP2;
                    *(ct->pt)++ = SRP1;
                    *(ct->pt)++ = DUP;
                    *(ct->pt)++ = IP;
                    *(ct->pt)++ = MDAP_rnd;
		}
	    }
	    else {
		ip_base = fabs(bp[rp2].y - hend) < fabs(bp[rp1].y - hbase);
                opp = ip_base ? ept : bpt;
                init_stem_edge(ct, stem, !ip_base);

		if (ct->edge.refpt == -1) continue;

		if ( ct->gic->fpgm_done ) {
                    callargs[0] = opp;
                    callargs[1] = ct->edge.refpt;
		    callargs[2] = rp1;
		    callargs[3] = rp2;
		    callargs[4] = 13;
		    ct->pt = pushnums(ct->pt, 5, callargs);
		    *(ct->pt)++ = CALL;
                } else {
                    callargs[0] = ct->edge.refpt;
		    callargs[1] = rp1;
		    callargs[2] = rp2;
		    ct->pt = pushnums(ct->pt, 3, callargs);
                    *(ct->pt)++ = SRP2;
                    *(ct->pt)++ = SRP1;
                    *(ct->pt)++ = DUP;
                    *(ct->pt)++ = IP;
                    *(ct->pt)++ = MDAP_rnd;
                }
	    }

	    ct->rp0 = ct->edge.refpt;
	    finish_stem(stem, use_rp1, keep_old_rp0, ct);
            if ( instruct_serif_stems || instruct_ball_terminals )
                instruct_serifs(ct, stem);

            instruct_dependent(ct, stem);
	}
    }
    free(rpts1);
    free(rpts2);
}

/*
 * High-level function for instructing vertical stems.
 *
 * TODO! CJK hinting may need different function (VStemGeninstCJK?)
 */
static void VStemGeninst(InstrCt *ct) {
    StemData *stem, *prev=NULL;
    int i;

    if (ct->rp0 != ct->ptcnt) {
        ct->pt = pushpoint(ct->pt, ct->ptcnt);
        *(ct->pt)++ = MDAP_rnd;
        ct->rp0 = ct->ptcnt;
    }

    if ( ct->gd->vbundle != NULL ) {
        for ( i=0; i<ct->gd->vbundle->cnt; i++ ) {
            stem = ct->gd->vbundle->stemlist[i];
            if ((!stem->ldone || !stem->rdone) && stem->master == NULL) {

                if (prev != NULL && prev->rightidx != -1 && ct->rp0 != prev->rightidx) {
                    ct->pt = pushpoint(ct->pt, prev->rightidx);
                    *(ct->pt)++ = SRP0;
                    ct->rp0 = prev->rightidx;
                }
                geninstrs(ct, stem, prev, true);
                prev = stem;
            }
        }
    }

    /* instruct right sidebearing */
    if (ct->sc->width != 0) {
        if ( ct->gic->fpgm_done && !control_counters ) {
            ct->pt = push2nums(ct->pt, ct->ptcnt+1, 1);
            *(ct->pt)++ = CALL;
        } else {
            /* select rp0 at the right edge of last stem - geninstrs() didn't. */
            /* TODO! after some time, move this to geninstrs(), to save space. */
            if (prev != NULL && prev->rightidx != -1 && ct->rp0 != prev->rightidx) {
                ct->pt = pushpoint(ct->pt, prev->rightidx);
                *(ct->pt)++ = SRP0;
                ct->rp0 = prev->rightidx;
            }
            ct->pt = pushpoint(ct->pt, ct->ptcnt+1);
            *(ct->pt)++ = MDRP_rp0_rnd_white;
        }
        ct->rp0 = ct->ptcnt+1;
    }
}

/******************************************************************************
 *
 * Everything related with diagonal hinting goes here
 *
 ******************************************************************************/

#define DIAG_MIN_DISTANCE   (0.84375)

static int ds_cmp( const void *_s1, const void *_s2 ) {
    StemData * const *s1 = _s1, * const *s2 = _s2;

    BasePoint *bp1, *bp2;
    bp1 = (*s1)->unit.y > 0 ? &(*s1)->keypts[0]->base : &(*s1)->keypts[2]->base;
    bp2 = (*s2)->unit.y > 0 ? &(*s2)->keypts[0]->base : &(*s2)->keypts[2]->base;
    if ( bp1->x < bp2->x || ( bp1->x == bp2->x && bp1->y < bp2->y ))
return( -1 );
    else if ( bp2->x < bp1->x || ( bp2->x == bp1->x && bp2->y < bp1->y ))
return( 1 );

return( 0 );
}

/* Takes a line defined by two points and returns a vector decribed as a
 * pair of x and y values, such that the value (x2 + y2) is equal to 1.
 * Note that the BasePoint structure is used to store the vector, although
 * it is not a point itself. This is just because that structure has "x"
 * and "y" fields which can be used for our purpose.
 */
static BasePoint GetVector ( BasePoint *top,BasePoint *bottom,int orth ) {
    real catx, caty, hyp, temp;
    BasePoint ret;

    catx = top->x - bottom->x; caty = top->y - bottom->y;
    hyp = sqrt(( catx*catx ) + ( caty*caty ));
    ret.y = caty/hyp; ret.x = catx/hyp;

    if( orth ) {
        temp = ret.x; ret.x = -ret.y; ret.y = temp;
    }
return( ret );
}

static int SetDStemKeyPoint( InstrCt *ct,StemData *stem,PointData *pd,int aindex ) {

    int nextidx, previdx, cpidx, prev_outer, next_outer, is_start;
    int nsidx, psidx, sidx;
    uint8 flag;
    PointData *ncpd, *pcpd, *cpd, *best = NULL;
    real prevdot, nextdot, cpdist;

    if ( pd == NULL )
return( false );

    flag = fabs( stem->unit.y ) > fabs( stem->unit.x ) ? tf_y : tf_x;
    is_start =  ( aindex == 0 || aindex == 2 );
    prevdot  =  ( pd->prevunit.x * stem->unit.x ) +
                ( pd->prevunit.y * stem->unit.y );
    nextdot  =  ( pd->nextunit.x * stem->unit.x ) +
                ( pd->nextunit.y * stem->unit.y );
    prev_outer = IsStemAssignedToPoint( pd,stem,false ) != -1 && 
                (( is_start && prevdot < 0 ) || ( !is_start && prevdot > 0 ));
    next_outer = IsStemAssignedToPoint( pd,stem,true  ) != -1 && 
                (( is_start && nextdot < 0 ) || ( !is_start && nextdot > 0 ));

    if ( pd->ttfindex >= ct->gd->realcnt ) {
        nextidx = pd->sp->nextcpindex;
        previdx = pd->sp->prev->from->nextcpindex;
        ncpd = &ct->gd->points[nextidx];
        pcpd = &ct->gd->points[previdx];
        psidx = IsStemAssignedToPoint( pcpd,stem,true );
        nsidx = IsStemAssignedToPoint( ncpd,stem,false );

        if ( psidx == -1 && nsidx == -1 )
return( false );

        if ( psidx > -1 && nsidx > -1 )
            best = ( prev_outer ) ? pcpd : ncpd;
        else
            best = ( psidx > -1 ) ? pcpd : ncpd;

    } else if (( !pd->sp->nonextcp && next_outer ) || ( !pd->sp->noprevcp && prev_outer )) {
        cpidx = ( prev_outer ) ? pd->sp->prev->from->nextcpindex : pd->sp->nextcpindex;
        cpd = &ct->gd->points[cpidx];
        sidx = IsStemAssignedToPoint( cpd,stem,prev_outer );

        if ( sidx != -1 ) {
            cpdist = fabs(( pd->base.x - cpd->base.x ) * stem->unit.x +
            	          ( pd->base.y - cpd->base.y ) * stem->unit.y );
            if (( cpdist > stem->clen/2 ) ||
                (!(ct->touched[pd->ttfindex] & flag) && !(ct->affected[pd->ttfindex] & flag) &&
                ( ct->touched[cpd->ttfindex] & flag || ct->affected[cpd->ttfindex] & flag )))
                best = cpd;
        }
        if ( best == NULL ) best = pd;
    } else
        best = pd;

    stem->keypts[aindex] = best;
return( true );
}

static void AssignLineToPoint( DiagPointInfo *diagpts,StemData *stem,int idx,int is_l ) {
    int num, base, i;
    PointData *pd1, *pd2;

    num = diagpts[idx].count;
    base = ( is_l ) ? 0 : 2;
    pd1 = stem->keypts[base];
    pd2 = stem->keypts[base+1];
    for ( i=0; i<num; i++ ) {
        if ( diagpts[idx].line[i].pd1 == pd1 && diagpts[idx].line[i].pd2 == pd2 )
return;
    }

    diagpts[idx].line[num].pd1 = stem->keypts[base];
    diagpts[idx].line[num].pd2 = stem->keypts[base+1];
    diagpts[idx].line[num].done = false;
    diagpts[idx].count++;
return;
}

/* Convert the existing diagonal stem layout to glyph data, containing
 * information about points assigned to each stem. Then run on stem chunks
 * and associate with each point the line it should be aligned by. Note that
 * we have to do this on a relatively early stage, as it may be important
 * to know, if the given point is subject to the subsequent diagonale hinting,
 * before any actual processing of diagonal stems is started.
 */
static void InitDStemData( InstrCt *ct ) {
    DiagPointInfo *diagpts = ct->diagpts;
    int i, j, idx, previdx, nextidx, num1, num2, psidx, nsidx, is_l, cnt=0;
    real prevlsp, prevrsp, prevlep, prevrep, lpos, rpos;
    GlyphData *gd;
    StemData *stem;
    PointData *ls, *rs, *le, *re, *tpd, *ppd, *npd;
    struct stem_chunk *chunk;

    gd = ct->gd;

    for ( i=0; i<gd->stemcnt; i++ ) {
        stem = &gd->stems[i];
	if ( stem->toobig )
    continue;
        if (( stem->unit.y > -.05 && stem->unit.y < .05 ) ||
            ( stem->unit.x > -.05 && stem->unit.x < .05 ))
    continue;
	if ( stem->lpcnt < 2 || stem->rpcnt < 2 )
    continue;

        prevlsp = prevrsp = 1e4;
        prevlep = prevrep = -1e4;
        ls = rs = le = re = NULL;
        for ( j=0; j<stem->chunk_cnt; j++ ) {
            chunk = &stem->chunks[j];
            if ( chunk->l != NULL ) {
                lpos =  ( chunk->l->base.x - stem->left.x )*stem->unit.x +
                        ( chunk->l->base.y - stem->left.y )*stem->unit.y;
                if ( lpos < prevlsp ) {
                    ls = chunk->l; prevlsp = lpos;
                } 
                if ( lpos > prevlep ) {
                    le = chunk->l; prevlep = lpos;
                }
            }
            if ( chunk->r != NULL ) {
                rpos =  ( chunk->r->base.x - stem->right.x )*stem->unit.x +
                        ( chunk->r->base.y - stem->right.y )*stem->unit.y;
                if ( rpos < prevrsp ) {
                    rs = chunk->r; prevrsp = rpos;
                } 
                if ( rpos > prevrep ) {
                    re = chunk->r; prevrep = rpos;
                }
           }
        }

        /* Swap "left" and "right" sides for vectors pointing north-east,
         * so that the "left" side is always determined along the x axis
         * rather than relatively to the vector direction */
        num1 = ( stem->unit.y > 0 ) ? 0 : 2;
        num2 = ( stem->unit.y > 0 ) ? 2 : 0;
        if (!SetDStemKeyPoint( ct,stem,ls,num1 ) || !SetDStemKeyPoint( ct,stem,rs,num2 ))
    continue;

        num1 = ( stem->unit.y > 0 ) ? 1 : 3;
        num2 = ( stem->unit.y > 0 ) ? 3 : 1;
        if (!SetDStemKeyPoint( ct,stem,le,num1 ) || !SetDStemKeyPoint( ct,stem,re,num2 ))
    continue;

        for ( j=0; j<gd->pcnt; j++ )
            gd->points[j].ticked = false;
        for ( j=0; j<gd->pcnt; j++ ) if ( gd->points[j].sp != NULL ) {
            tpd = &gd->points[j];
            idx = tpd->ttfindex;
            psidx = nsidx = -1;
            if ( idx < gd->realcnt ) {
                if ( !tpd->ticked && diagpts[idx].count < 2 && ( 
                    ( psidx = IsStemAssignedToPoint( tpd,stem,false )) > -1 ||
                    ( nsidx = IsStemAssignedToPoint( tpd,stem,true )) > -1)) {

                    is_l = ( nsidx > -1 ) ? tpd->next_is_l[nsidx] : tpd->prev_is_l[psidx];
                    if ( stem->unit.y < 0 ) is_l = !is_l;
                    AssignLineToPoint( diagpts,stem,idx,is_l );
                    tpd->ticked = true;
                }
            } else {
                previdx = tpd->sp->prev->from->nextcpindex;
                nextidx = tpd->sp->nextcpindex;
                ppd = &gd->points[previdx];
                npd = &gd->points[nextidx];
                if (!ppd->ticked && diagpts[previdx].count < 2 &&
                    ( nsidx = IsStemAssignedToPoint( ppd,stem,true )) > -1 ) {

                    is_l = ppd->next_is_l[nsidx];
                    if ( stem->unit.y < 0 ) is_l = !is_l;
                    AssignLineToPoint( diagpts,stem,previdx,is_l );
                    ppd->ticked = true;
                }
                if (!npd->ticked && diagpts[nextidx].count < 2 &&
                    ( psidx = IsStemAssignedToPoint( npd,stem,false )) > -1 ) {

                    is_l = npd->prev_is_l[psidx];
                    if ( stem->unit.y < 0 ) is_l = !is_l;
                    AssignLineToPoint( diagpts,stem,nextidx,is_l );
                    npd->ticked = true;
                }
            }
        }
        ct->diagstems[cnt++] = stem;
    }
    qsort( ct->diagstems,cnt,sizeof( StemData *),ds_cmp );
    ct->diagcnt = cnt;
}

/* Usually we have to start doing each diagonal stem from the point which
 * is most touched in any directions.
 */
static int FindDiagStartPoint( StemData *stem, uint8 *touched ) {
    int i;

    for ( i=0; i<4; ++i ) {
        if (( touched[stem->keypts[i]->ttfindex] & tf_x ) && 
            ( touched[stem->keypts[i]->ttfindex] & tf_y ))
return( i );
    }

    for ( i=0; i<4; ++i ) {
        if (( stem->unit.x > stem->unit.y &&
                touched[stem->keypts[i]->ttfindex] & tf_y ) ||
            ( stem->unit.y > stem->unit.x &&
                touched[stem->keypts[i]->ttfindex] & tf_x ))
return( i );
    }

    for ( i=0; i<4; ++i ) {
        if ( touched[stem->keypts[i]->ttfindex] & ( tf_x | tf_y ))
return( i );
    }
return( 0 );
}

/* Check the directions at which the given point still can be moved
 * (i. e. has not yet been touched) and set freedom vector to that
 * direction in case it has not already been set.
 */
static int SetFreedomVector( uint8 **instrs,int pnum,
    uint8 *touched,DiagPointInfo *diagpts,BasePoint *norm,BasePoint *fv,int pvset,int fpgm_ok ) {

    int i, pushpts[3];
    PointData *start=NULL, *end=NULL;
    BasePoint newfv;

    if (( touched[pnum] & tf_d ) && !( touched[pnum] & tf_x ) && !( touched[pnum] & tf_y )) {
        for ( i=0 ; i<diagpts[pnum].count ; i++) {
            if ( diagpts[pnum].line[i].done ) {
                start = diagpts[pnum].line[i].pd1;
                end = diagpts[pnum].line[i].pd2;
            }
        }

        /* This should never happen */
        if ( start == NULL || end == NULL )
return( false );

        newfv = GetVector( &start->base,&end->base,false );
        if ( !UnitsParallel( fv,&newfv,true )) {
            fv->x = newfv.x; fv->y = newfv.y;

            pushpts[0] = start->ttfindex; pushpts[1] = end->ttfindex;
            *instrs = pushpoints( *instrs,2,pushpts );
            *(*instrs)++ = 0x08;       /*SFVTL[parallel]*/
        }

return( true );

    } else if ( touched[pnum] & tf_x && !(touched[pnum] & tf_d) && !(touched[pnum] & tf_y)) {
        if (!( RealNear( fv->x,0 ) && RealNear( fv->y,1 ))) {
            *(*instrs)++ = 0x04;       /*SFVTCA[y-axis]*/
            fv->x = 0; fv->y = 1;
        }
return( true );

    } else if ( touched[pnum] & tf_y && !(touched[pnum] & tf_d) && !(touched[pnum] & tf_x)) {
        if (!( RealNear( fv->x,1 ) && RealNear( fv->y,0 ))) {
            *(*instrs)++ = 0x05;       /*SFVTCA[x-axis]*/
            fv->x = 1; fv->y = 0;
        }
return( true );
    } else if ( !(touched[pnum] & (tf_x|tf_y|tf_d))) {
        if ( !UnitsParallel( fv,norm,true )) {
            fv->x = norm->x; fv->y = norm->y;

            if ( pvset )
                *(*instrs)++ = 0x0E;   /*SFVTPV*/
            else {
                pushpts[0] = EF2Dot14(norm->x);
                pushpts[1] = EF2Dot14(norm->y);
                if ( fpgm_ok ) {
                    pushpts[2] = 21;
                    *instrs = pushpoints( *instrs,3,pushpts );
                    *(*instrs)++ = CALL; /* aspect-ratio correction */
                } else
                    *instrs = pushpoints( *instrs,2,pushpts );

                *(*instrs)++ = 0x0b; /* SFVFS */
            }
        }
return( true );
    }
return( false );
}

static int MarkLineFinished( int pnum,int startnum,int endnum,DiagPointInfo *diagpts ) {
    int i;

    for ( i=0; i<diagpts[pnum].count; i++ ) {
        if (( diagpts[pnum].line[i].pd1->ttfindex == startnum ) &&
            ( diagpts[pnum].line[i].pd2->ttfindex == endnum )) {

            diagpts[pnum].line[i].done = 2;
return( true );
        }
    }
return( false );
}

static uint8 *FixDStemPoint ( InstrCt *ct,StemData *stem,
    int pt,int refpt,int firstedge,int cvt,BasePoint *fv ) {
    uint8 *instrs, *touched;
    int ptcnt;
    DiagPointInfo *diagpts;

    diagpts = ct->diagpts;
    ptcnt = ct->gd->realcnt;
    touched = ct->touched;
    instrs = ct->pt;

    if ( SetFreedomVector( &instrs,pt,touched,diagpts,&stem->l_to_r,fv,true,
            ct->gic->fpgm_done && ct->gic->prep_done )) {
        if ( refpt == -1 ) {
            if (( fv->x == 1 && !( touched[pt] & tf_x )) || 
                ( fv->y == 1 && !( touched[pt] & tf_y ))) {

                instrs = pushpoint( instrs,pt );
                *instrs++ = MDAP;
            } else {
                instrs = pushpoint( instrs,pt );
                *instrs++ = SRP0;
            }
            ct->rp0 = pt;
        } else {
            if ( refpt != ct->rp0 ) {
                instrs = pushpoint( instrs,refpt );
                *instrs++ = SRP0;
                ct->rp0 = refpt;
            }

            if ( cvt < 0 ) {
                instrs = pushpoint( instrs,pt );
                *instrs++ = MDRP_grey;
            } else {
                instrs = pushpointstem( instrs,pt,cvt );
                *instrs++ = MIRP_rp0_min_black;
                ct->rp0 = pt;
            }
        }
        touched[pt] |= tf_d;

        if (!MarkLineFinished( pt,stem->keypts[0]->ttfindex,stem->keypts[1]->ttfindex,diagpts ))
            MarkLineFinished( pt,stem->keypts[2]->ttfindex,stem->keypts[3]->ttfindex,diagpts );
    }
return( instrs );
}

static int DStemHasSnappableCorners ( InstrCt *ct,StemData *stem,PointData *pd1,PointData *pd2 ) {
    uint8 *touched = ct->touched;

    /* We should be dealing with oncurve points */
    if ( pd1->sp == NULL || pd2->sp == NULL )
return( false );

    /* points should not be lined up vertically or horizontally */
    if (fabs( pd1->base.x - pd2->base.x ) <= ct->gic->fudge ||
        fabs( pd1->base.y - pd2->base.y ) <= ct->gic->fudge )
return( false );

    if ((   pd1->x_corner == 1 && !( touched[pd1->ttfindex] & tf_y ) &&
            pd2->y_corner == 1 && !( touched[pd2->ttfindex] & tf_x )) ||
        (   pd1->y_corner == 1 && !( touched[pd1->ttfindex] & tf_x ) &&
            pd2->x_corner == 1 && !( touched[pd2->ttfindex] & tf_y )))
return( true );

return( false );
}

static uint8 *SnapDStemCorners ( InstrCt *ct,StemData *stem,PointData *pd1,PointData *pd2,BasePoint *fv ) {
    uint8 *instrs, *touched;
    int xbase, ybase;

    instrs = ct->pt;
    touched = ct->touched;

    if ( pd1->x_corner && pd2->y_corner ) {
        xbase = pd1->ttfindex; ybase = pd2->ttfindex;
    } else {
        xbase = pd2->ttfindex; ybase = pd1->ttfindex;
    }

    *(ct->pt)++ = SVTCA_x;
    ct->pt = push2points( ct->pt,ybase,xbase );
    *(ct->pt)++ = touched[xbase] & tf_x ? MDAP : MDAP_rnd;
    *(ct->pt)++ = MDRP_min_black;
    *(ct->pt)++ = SVTCA_y;
    ct->pt = push2points( ct->pt,xbase,ybase );
    *(ct->pt)++ = touched[ybase] & tf_y ? MDAP : MDAP_rnd;
    *(ct->pt)++ = MDRP_min_black;

    touched[xbase] |= ( tf_x | tf_y );
    touched[ybase] |= ( tf_x | tf_y );
    fv->x = 0; fv->y = 1;

return( instrs );
}

/* A basic algorithm for hinting diagonal stems:
 * -- iterate through diagonal stems, ordered from left to right;
 * -- for each stem, find the most touched point, to start from,
 *    and fix that point. TODO: the positioning should be done
 *    relatively to points already touched by x or y;
 * -- position the second point on the same edge, using dual projection
 *    vector;
 * -- link to the second edge and repeat the same operation.
 *
 * For each point we first determine a direction at which it still can
 * be moved. If a point has already been positioned relatively to another
 * diagonal line, then we move it along that diagonale. Thus this algorithm
 * can handle things like "V" where one line's ending point is another
 * line's starting point without special exceptions.
 */
static uint8 *FixDstem( InstrCt *ct, StemData *ds, BasePoint *fv ) {
    int startnum, a1, a2, b1, b2, ptcnt, firstedge, cvt;
    int x_ldup, y_ldup, x_edup, y_edup, dsc1, dsc2;
    PointData *v1, *v2;
    uint8 *touched;
    int pushpts[3];

    if ( ds->ldone && ds->rdone )
return( ct->pt );

    ptcnt = ct->ptcnt;
    touched = ct->touched;

    dsc1 = DStemHasSnappableCorners( ct,ds,ds->keypts[0],ds->keypts[2] );
    dsc2 = DStemHasSnappableCorners( ct,ds,ds->keypts[1],ds->keypts[3] );

    if ( dsc1 || dsc2 ) {
        ct->pt = pushF26Dot6( ct->pt,.59662 );
        *(ct->pt)++ = SMD;

        if ( dsc1 )
            SnapDStemCorners( ct,ds,ds->keypts[0],ds->keypts[2],fv );
        if ( dsc2 )
            SnapDStemCorners( ct,ds,ds->keypts[1],ds->keypts[3],fv );

        ct->pt = pushF26Dot6( ct->pt,DIAG_MIN_DISTANCE );
        *(ct->pt)++ = SMD;
    }

    if ( !dsc1 || !dsc2 ) {
        startnum = FindDiagStartPoint( ds,touched );
        a1 = ds->keypts[startnum]->ttfindex;
        if (( startnum == 0 ) || ( startnum == 1 )) {
            firstedge = true;
            v1 = ds->keypts[0]; v2 = ds->keypts[1];
            a2 = ( startnum == 1 ) ? ds->keypts[0]->ttfindex : ds->keypts[1]->ttfindex;
            b1 = ( startnum == 1 ) ? ds->keypts[3]->ttfindex : ds->keypts[2]->ttfindex; 
            b2 = ( startnum == 1 ) ? ds->keypts[2]->ttfindex : ds->keypts[3]->ttfindex;
        } else {
            firstedge = false;
            v1 = ds->keypts[2]; v2 = ds->keypts[3];
            a2 = ( startnum == 3 ) ? ds->keypts[2]->ttfindex : ds->keypts[3]->ttfindex;
            b1 = ( startnum == 3 ) ? ds->keypts[1]->ttfindex : ds->keypts[0]->ttfindex; 
            b2 = ( startnum == 3 ) ? ds->keypts[0]->ttfindex : ds->keypts[1]->ttfindex;
        }

        /* Always put the calculated stem width into the CVT table, unless it is
         * already there. This approach would be wrong for vertical or horizontal
         * stems, but for diagonales it is just unlikely that we can find an
         * acceptable predefined value in StemSnapH or StemSnapV
         */
        cvt = TTF_getcvtval( ct->gic->sf,ds->width );

        pushpts[0] = EF2Dot14(ds->l_to_r.x);
        pushpts[1] = EF2Dot14(ds->l_to_r.y);
        if ( ct->gic->fpgm_done && ct->gic->prep_done ) {
            pushpts[2] = 21;
            ct->pt = pushnums( ct->pt, 3, pushpts );
            *(ct->pt)++ = CALL;    /* Aspect ratio correction */
        } else
            ct->pt = pushnums( ct->pt, 2, pushpts );
        *(ct->pt)++ = 0x0A;    /* SPVFS */

        pushpts[0] = v1->ttfindex; pushpts[1] = v2->ttfindex;

        x_ldup =( touched[a1] & tf_x && touched[a2] & tf_x ) ||
                ( touched[b1] & tf_x && touched[b2] & tf_x );
        y_ldup =( touched[a1] & tf_y && touched[a2] & tf_y ) ||
                ( touched[b1] & tf_y && touched[b2] & tf_y );
        x_edup =( touched[a1] & tf_x && touched[b1] & tf_x ) ||
                ( touched[a2] & tf_x && touched[b2] & tf_x );
        y_edup =( touched[a1] & tf_y && touched[b1] & tf_y ) ||
                ( touched[a2] & tf_y && touched[b2] & tf_y );

        if (( x_ldup && !y_edup ) || ( y_ldup && !x_edup)) {

            ct->pt = FixDStemPoint ( ct,ds,a1,-1,firstedge,-1,fv );
            ct->pt = FixDStemPoint ( ct,ds,b2,-1,firstedge,-1,fv );
            ct->pt = FixDStemPoint ( ct,ds,b1,a1,firstedge,cvt,fv );
            ct->pt = FixDStemPoint ( ct,ds,a2,b2,firstedge,cvt,fv );
        } else {
            ct->pt = FixDStemPoint ( ct,ds,a1,-1,firstedge,-1,fv );
            ct->pt = FixDStemPoint ( ct,ds,a2,a1,firstedge,-1,fv );
            ct->pt = FixDStemPoint ( ct,ds,b1,a1,firstedge,cvt,fv );
            ct->pt = FixDStemPoint ( ct,ds,b2,b1,firstedge,-1,fv );
        }
    }

    ds->ldone = ds->rdone = true;
return( ct->pt );
}

static uint8 *FixPointOnLine ( DiagPointInfo *diagpts,PointVector *line,
    PointData *pd,InstrCt *ct,BasePoint *fv,BasePoint *pv,int *rp1,int *rp2 ) {

    uint8 *instrs, *touched;
    BasePoint newpv;
    int ptcnt;
    int pushpts[4];

    touched = ct->touched;
    instrs = ct->pt;
    ptcnt = ct->ptcnt;

    newpv = GetVector( &line->pd1->base,&line->pd2->base,true );

    if ( SetFreedomVector( &instrs,pd->ttfindex,touched,diagpts,&newpv,fv,false,
            ct->gic->fpgm_done && ct->gic->prep_done )) {
        if ( ct->rp0 != line->pd1->ttfindex ) {
            instrs = pushpoint( instrs,line->pd1->ttfindex );
            *instrs++ = SRP0;
            ct->rp0 = line->pd1->ttfindex;
        }
        if ( ct->gic->fpgm_done ) {
            pv->x = newpv.x; pv->y = newpv.y;

            pushpts[0] = pd->ttfindex;
            pushpts[1] = line->pd1->ttfindex;
            pushpts[2] = line->pd2->ttfindex;
            pushpts[3] = 19;
            instrs = pushpoints( instrs,4,pushpts );
            *instrs++ = CALL;
        } else {
            if ( !UnitsParallel( pv,&newpv,true )) {
                pv->x = newpv.x; pv->y = newpv.y;

                pushpts[0] = line->pd1->ttfindex; pushpts[1] = line->pd2->ttfindex;
                instrs = pushpoints( instrs,2,pushpts );
                *instrs++ = 0x07;         /*SPVTL[orthogonal]*/
            }

            instrs = pushpoint( instrs,pd->ttfindex );
            *instrs++ = MDRP_grey;
        }
    }
return( instrs );
}

/* If a point has to be positioned just relatively to the diagonal
 * line (no intersections, no need to maintain other directions),
 * then we can interpolate it along that line. This usually produces
 * better results for things like a Danish slashed "O".
 */
static uint8 *InterpolateAlongDiag ( DiagPointInfo *diagpts,PointVector *line,
    PointData *pd,InstrCt *ct,BasePoint *fv,BasePoint *pv,int *rp1,int *rp2 ) {

    uint8 *instrs, *touched;
    BasePoint newpv;
    int ptcnt;
    int pushpts[3];

    touched = ct->touched;
    instrs = ct->pt;
    ptcnt = ct->ptcnt;

    if (diagpts[pd->ttfindex].count != 1 || touched[pd->ttfindex] & ( tf_x|tf_y ) ||
        diagpts[pd->ttfindex].line[0].done > 1 )
return( instrs );

    newpv = GetVector( &line->pd1->base,&line->pd2->base,false );

    if ( !UnitsParallel( pv,&newpv,false ) || 
        *rp1 != line->pd1->ttfindex || *rp2 != line->pd1->ttfindex ) {

        pushpts[0] = pd->ttfindex;
        pushpts[1] = line->pd1->ttfindex;
        pushpts[2] = line->pd2->ttfindex;
        instrs = pushpoints( instrs,3,pushpts );
    } else
        instrs = pushpoint ( instrs,pd->ttfindex );

    if ( !UnitsParallel( pv,&newpv,true )) {
        pv->x = newpv.x; pv->y = newpv.y;

        if ( *rp1 != line->pd1->ttfindex || *rp2 != line->pd1->ttfindex ) {
            *instrs++ = DUP;
            *instrs++ = 0x8a; /* ROLL */
            *instrs++ = DUP;
            *instrs++ = 0x8a; /* ROLL */
            *instrs++ = 0x23; /* SWAP */
        }
        *instrs++ = 0x06; /* SPVTL[parallel] */
    }

    if ( !UnitsParallel( fv,&newpv,true )) {
        *instrs++ = 0x0E; /* SFVTPV */
        fv->x = newpv.x; fv->y = newpv.y;
    }
    if ( *rp1 != line->pd1->ttfindex || *rp2 != line->pd1->ttfindex ) {
        *rp1 = line->pd1->ttfindex;
        *rp2 = line->pd1->ttfindex;

        *instrs++ = SRP1;
        *instrs++ = SRP2;
    }
    *instrs++ = IP;
    touched[pd->ttfindex] |= tf_d;
    diagpts[pd->ttfindex].line[0].done = 2;
return( instrs );
}

/* When all stem edges have already been positioned, run through other
 * points which are known to be related with some diagonales and position
 * them too. This may include both intersections and points which just
 * lie on a diagonal line. This function does not care about starting/ending
 * points of stems, unless they should be additionally positioned relatively
 * to another stem. Thus is can handle things like "X" or "K".
 */
static uint8 *MovePointsToIntersections( InstrCt *ct,BasePoint *fv ) {

    int i, j, ptcnt, rp1=-1, rp2=-1;
    uint8 *touched;
    BasePoint pv;
    PointData *curpd, *npd, *ppd;
    DiagPointInfo *diagpts;
    StemData *ds;

    touched = ct->touched;
    ptcnt = ct->gd->realcnt;
    diagpts = ct->diagpts;
    pv.x = 1; pv.y = 0;

    for ( i=0; i<ptcnt; i++ ) {
        if ( diagpts[i].count > 0 ) {
            for ( j=0; j<diagpts[i].count; j++ ) {
                if ( !diagpts[i].line[j].done ) {
                    curpd = &ct->gd->points[i];

                    ct->pt = FixPointOnLine( diagpts,&diagpts[i].line[j],
                        curpd,ct,fv,&pv,&rp1,&rp2 );

                    diagpts[i].line[j].done = true;
                    touched[i] |= tf_d;
                }
            }
        }
    }

    /* Second pass to interpolate points lying on diagonal lines (but not
     * starting/ending stem points) along those lines. This operation, unlike
     * moving points to diagonals, requires vectors to be set parallel to lines,
     * and this is the reason for which it is done in a separate cycle
     */
    for ( i=0; i<ct->diagcnt; i++ ) {
        ds = ct->diagstems[i];
        if ( ds->ldone ) {
            for ( j=0; j<ds->chunk_cnt; j++ ) if (( curpd = ds->chunks[j].l ) != NULL ) {
                if ( curpd->ttfindex < ct->ptcnt ) {
                    ct->pt = InterpolateAlongDiag ( diagpts,&diagpts[curpd->ttfindex].line[0],
                                curpd,ct,fv,&pv,&rp1,&rp2 );
                } else {
                    ppd = &ct->gd->points[curpd->sp->prev->from->nextcpindex];
                    npd = &ct->gd->points[curpd->sp->nextcpindex];
                    if ( IsStemAssignedToPoint(ppd, ds, true) != -1 )
                        ct->pt = InterpolateAlongDiag ( diagpts,&diagpts[ppd->ttfindex].line[0],
                            ppd,ct,fv,&pv,&rp1,&rp2 );
                    if ( IsStemAssignedToPoint(npd, ds, false) != -1 )
                        ct->pt = InterpolateAlongDiag ( diagpts,&diagpts[npd->ttfindex].line[0],
                            npd,ct,fv,&pv,&rp1,&rp2 );
                }
            }
        }
        if ( ds->rdone ) {
            for ( j=0; j<ds->chunk_cnt; j++ ) if (( curpd = ds->chunks[j].r ) != NULL ) {
                if ( curpd->ttfindex < ct->ptcnt ) {
                    ct->pt = InterpolateAlongDiag ( diagpts,&diagpts[curpd->ttfindex].line[0],
                                curpd,ct,fv,&pv,&rp1,&rp2 );
                } else {
                    ppd = &ct->gd->points[curpd->sp->prev->from->nextcpindex];
                    npd = &ct->gd->points[curpd->sp->nextcpindex];
                    if ( IsStemAssignedToPoint(ppd, ds, true) != -1 )
                        ct->pt = InterpolateAlongDiag ( diagpts,&diagpts[ppd->ttfindex].line[0],
                            ppd,ct,fv,&pv,&rp1,&rp2 );
                    if ( IsStemAssignedToPoint(npd, ds, false) != -1 )
                        ct->pt = InterpolateAlongDiag ( diagpts,&diagpts[npd->ttfindex].line[0],
                            npd,ct,fv,&pv,&rp1,&rp2 );
                }
            }
        }
    }
return( ct->pt );
}

static void TouchControlPoint( InstrCt *ct,PointData *pd, 
    int next,int *tobefixedy,int *tobefixedx,int *numx,int *numy ) {

    int idx, cpidx;
    PointData *cpd;
    uint8 *touched = ct->touched;

    idx = pd->ttfindex;
    cpidx = next ? pd->sp->nextcpindex : pd->sp->prev->from->nextcpindex;
    cpd = &ct->gd->points[cpidx];

    if ( has_valid_dstem( cpd, !next ) != -1 ) {
        /* if this control point is used to describe an implied spline
         * point, then it is instructed as if it was an oncurve point */
        if ( idx == 0xffff && touched[cpidx] & tf_d ) {
            if (!( touched[cpidx] & tf_y )) {
                tobefixedy[(*numy)++] = cpidx;
                touched[cpidx] |= tf_y;
            }

            if (!( touched[cpidx] & tf_x )) {
                tobefixedx[(*numx)++] = cpidx;
                touched[cpidx] |= tf_x;
            }
        /* otherwise we just mark it as affected to prevent undesired
         * interpolations */
        } else if ( idx < ct->gd->realcnt && touched[idx] & tf_d ) {
            ct->affected[cpidx] |= tf_x;
            ct->affected[cpidx] |= tf_y;
        }
    }
}

/* Finally explicitly touch all affected points by X and Y (unless they
 * have already been), so that subsequent IUP's can't distort our stems.
 */
static uint8 *TouchDStemPoints( InstrCt *ct,BasePoint *fv ) {

    int i, ptcnt, numx=0, numy=0, idx;
    int *tobefixedy, *tobefixedx;
    uint8 *instrs, *touched;
    DiagPointInfo *diagpts;
    PointData *pd;

    touched = ct->touched;
    instrs = ct->pt;
    ptcnt = ct->gd->pcnt;
    diagpts = ct->diagpts;

    tobefixedy = calloc( ptcnt,sizeof( int ));
    tobefixedx = calloc( ptcnt,sizeof( int ));

    /* Ensure that the projection vector is no longer set to a diagonal line */
    if ( fv->x == 1 && fv->y == 0 )
        *instrs++ = 0x03;       /* SPVTCA[x] */
    else if  ( fv->x == 0 && fv->y == 1 )
        *instrs++ = 0x02;       /* SPVTCA[y] */

    for ( i=0; i<ptcnt; i++ ) if ( ct->gd->points[i].sp != NULL ) {
        pd = &ct->gd->points[i];
        if (( has_valid_dstem( pd,false )) != -1 ||
            ( has_valid_dstem( pd,true )) != -1 ) {

            idx = pd->ttfindex;
            if ( idx < ct->gd->realcnt && touched[idx] & tf_d ) {
                if (!( touched[idx] & tf_y )) {
                    tobefixedy[numy++] = idx;
                    touched[idx] |= tf_y;
                }

                if (!( touched[idx] & tf_x )) {
                    tobefixedx[numx++] = idx;
                    touched[idx] |= tf_x;
                }
            }
            if ( !pd->sp->noprevcp )
                TouchControlPoint( ct,pd,false,tobefixedy,tobefixedx,&numx,&numy );
            if ( !pd->sp->nonextcp )
                TouchControlPoint( ct,pd,true,tobefixedy,tobefixedx,&numx,&numy );
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

    if ( numx == 0 && numy == 0 ) *instrs++ = SVTCA_x;

    free( tobefixedy );
    free( tobefixedx );
return( instrs );
}

static void DStemInfoGeninst( InstrCt *ct ) {
    BasePoint fv;
    int i;

    if (ct->diagcnt == 0)
return;

    fv.x = 1; fv.y = 0;

    ct->pt = pushF26Dot6( ct->pt,DIAG_MIN_DISTANCE );
    *(ct->pt)++ = SMD; /* Set Minimum Distance */

    for ( i=0; i<ct->diagcnt; i++ )
        ct->pt = FixDstem ( ct,ct->diagstems[i],&fv );

    ct->pt = MovePointsToIntersections( ct,&fv );
    ct->pt = TouchDStemPoints ( ct,&fv);

    ct->pt = pushF26Dot6( ct->pt,1.0 );
    *(ct->pt)++ = SMD; /* Set Minimum Distance */

    ct->xdir = fv.x;
}

/******************************************************************************
 *
 * Strong point interpolation
 *
 * TODO! Better optimization, if possible.
 * TODO! leftmost and righmost bounds, if not already controlled by stems.
 *
 ******************************************************************************/

/* To be used with qsort() - sorts edge array in ascending order. */
struct stemedge {
    int refpt;
    double pos;
};

/* To be used with qsort() - sorts edge array in ascending order. */
static int sortedges(const void *_e1, const void *_e2) {
    const struct stemedge *e1 = _e1, *e2 = _e2;
    return ( e1->pos > e2->pos );
}

static int AddEdge(InstrCt *ct, StemData *stem, int is_l, struct stemedge *edgelist, int cnt) {
    real coord;
    int i, skip, refidx;

    if (!stem->ghost || 
        (is_l && stem->width == 20) || (!is_l && stem->width == 21)) {

        coord  = is_l ?
            ((real *) &stem->left.x)[!ct->xdir] : ((real *) &stem->right.x)[!ct->xdir];
        refidx = is_l ? stem->leftidx : stem->rightidx;
        for (i=skip=0; i<cnt; i++)
            if (abs(coord - edgelist[i].pos) <= ct->gic->fudge ||
                edgelist[i].refpt == refidx) {
                skip=1;
                break;
            }
        if (!skip && refidx != -1) {
            edgelist[cnt  ].refpt = refidx;
            edgelist[cnt++].pos = coord;
        }
    }
    return( cnt );
}

/* Optional feature: tries to maintain relative position of some important
 * points between stems' edges, so that glyph's shape is mostly preserved
 * when strongly gridfitted. This in terms of FreeType is called 'Strong Point
 * Interpolation'. It now does more or else what it should, but generates large
 * and sometimes incomplete code - see 'todos' above, and optimize_strongpts().
 * Note: it would affect diagonals if done before instructing them.
 *
 * TODO: it now intrpolates strong points only between hints' edges.
 * What about between leftmost/rightmost edge and leftmost/rightmost
 * glyph extents, if they protrude beyond the edges?
 */
static void InterpolateStrongPoints(InstrCt *ct) {
    StemBundle *bundle;
    StemData *stem;
    uint8 touchflag = ct->xdir?tf_x:tf_y;
    real fudge;
    struct stemedge edgelist[192];
    int edgecnt=0, i, j;
    int lpoint = -1, ledge=0;
    int rpoint = -1;
    int nowrp1 = 1;
    int ldone = 0;

    bundle = ( ct->xdir ) ? ct->gd->vbundle : ct->gd->hbundle;
    if (bundle == NULL || bundle->cnt == 0)
        return;

    /* List all stem edges. List only active edges for ghost hints. */
    for(i=0; i<bundle->cnt; i++) {
        stem = bundle->stemlist[i];

        edgecnt = AddEdge(ct, stem, ct->xdir, edgelist, edgecnt);
        edgecnt = AddEdge(ct, stem, !ct->xdir, edgelist, edgecnt);
    }

    if (edgecnt < 2)
return;

    qsort(edgelist, edgecnt, sizeof(struct stemedge), sortedges);

    /* Interpolate important points between subsequent edges */
    for (i=0; i<edgecnt; i++) {
        rpoint = edgelist[i].refpt;
        if (rpoint == -1 || !(ct->touched[rpoint] & touchflag)) continue;

        if (lpoint==-1) {
            /* first edge */
            lpoint = rpoint;
            ledge = i;
        }
        else {
            fudge = ct->gic->fudge;
            ct->gic->fudge = (edgelist[i].pos-edgelist[ledge].pos)/2;
            init_edge(ct, (edgelist[i].pos+edgelist[ledge].pos)/2, ALL_CONTOURS);
            optimize_strongpts(ct); /* Special way is needed here. */
            ct->gic->fudge = fudge;

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
                for (j=0; j<ct->edge.othercnt; j++)
                    ct->touched[ct->edge.others[j]] |= touchflag;
            }

            if (ct->edge.othercnt) {
                free(ct->edge.others);
                ct->edge.othercnt = 0;
            }
        }
    }
}

/******************************************************************************
 *
 * Generate instructions for a glyph.
 *
 ******************************************************************************/

static uint8 *dogeninstructions(InstrCt *ct) {
    StemData *stem;
    int max, i;
    DStemInfo *dstem;
    BlueData nbd;

    /* Fill a temporary BlueData structure basing on the data stored in the global
     * instruction context. This is needed for GlyphDataBuild(), as it accepts
     * blue data only in this format
     */
    for ( i=0; i<ct->gic->bluecnt; i++ ) {
        if ( ct->gic->blues[i].base < ct->gic->blues[i].overshoot ) {
            nbd.blues[i][0] = ct->gic->blues[i].base;
            nbd.blues[i][1] = ct->gic->blues[i].overshoot;
        } else {
            nbd.blues[i][0] = ct->gic->blues[i].overshoot;
            nbd.blues[i][1] = ct->gic->blues[i].base;
        }
    }
    nbd.bluecnt = ct->gic->bluecnt;
    ct->gd = GlyphDataBuild( ct->sc,ct->gic->layer,&nbd,instruct_diagonal_stems );

    /* Maximum instruction length is 6 bytes for each point in each dimension
     *  2 extra bytes to finish up. And one byte to switch from x to y axis
     * Diagonal take more space because we need to set the orientation on
     *  each stem, and worry about intersections, etc.
     *  That should be an over-estimate
     */
    max=2;
    if ( ct->gd->hbundle!=NULL ) max += ct->ptcnt*8;
    if ( ct->gd->vbundle!=NULL ) max += ct->ptcnt*8+4;
    for ( dstem=ct->sc->dstem; dstem!=NULL; max+=7+4*6+100, dstem=dstem->next );
    if ( ct->sc->md!=NULL ) max += ct->ptcnt*12;
    max += ct->ptcnt*6;			/* in case there are any rounds */
    max += ct->ptcnt*6;			/* paranoia */
    ct->instrs = ct->pt = malloc(max);

    /* Initially no stem hints are done */
    if ( ct->gd->hbundle!=NULL ) {
        for ( i=0; i<ct->gd->hbundle->cnt; i++ ) {
            stem = ct->gd->hbundle->stemlist[i];
	    stem->ldone = stem->rdone = false;
        }
    }
    if ( ct->gd->vbundle!=NULL ) {
        for ( i=0; i<ct->gd->vbundle->cnt; i++ ) {
            stem = ct->gd->vbundle->stemlist[i];
	    stem->ldone = stem->rdone = false;
        }
    }

    if ( instruct_diagonal_stems ) {
        /* Prepare info about diagonal stems to be used during edge optimization. */
        /* These contents need to be explicitly freed after hinting diagonals. */
        ct->diagstems = calloc(ct->gd->stemcnt, sizeof(StemData *));
        ct->diagpts = calloc(ct->ptcnt, sizeof(struct diagpointinfo));
        InitDStemData(ct);
    }

    /* We start from instructing horizontal features (=> movement in y)
     * Do this first so that the diagonal hinter will have everything moved
     * properly when it sets the projection vector
     * Even if we aren't doing the diagonals, we do the blues.
     */
    ct->xdir = false;
    *(ct->pt)++ = SVTCA_y;
    snap_to_blues(ct);
    HStemGeninst(ct);

    /* Next instruct vertical features (=> movement in x). */
    ct->xdir = true;
    *(ct->pt)++ = SVTCA_x;
    VStemGeninst(ct);

    /* Then instruct diagonal stems (=> movement in x)
     * This is done after vertical stems because it involves
     * moving some points out-of their vertical stems.
     */
    if (instruct_diagonal_stems && ct->diagcnt > 0) DStemInfoGeninst(ct);

    if ( interpolate_strong ) {
        /* Adjust important points between hint edges. */
        if (ct->xdir == false) *(ct->pt)++ = SVTCA_x;
        ct->xdir = true;
        InterpolateStrongPoints(ct);
        ct->xdir = false;
        *(ct->pt)++ = SVTCA_y;
        InterpolateStrongPoints(ct);
    }

    /* Interpolate untouched points */
    *(ct->pt)++ = IUP_y;
    *(ct->pt)++ = IUP_x;

    if ((ct->pt)-(ct->instrs) > max) IError(
	"We're about to crash.\n"
	"We miscalculated the glyph's instruction set length\n"
	"When processing TTF instructions (hinting) of %s", ct->sc->name
    );

    if ( instruct_diagonal_stems ) {
        free(ct->diagstems);
        free(ct->diagpts);
    }
    GlyphDataFree( ct->gd );

    ct->sc->ttf_instrs_len = (ct->pt)-(ct->instrs);
    ct->sc->instructions_out_of_date = false;
return ct->sc->ttf_instrs = realloc(ct->instrs,(ct->pt)-(ct->instrs));
}

void NowakowskiSCAutoInstr(GlobalInstrCt *gic, SplineChar *sc) {
    int cnt, contourcnt;
    BasePoint *bp;
    int *contourends;
    uint8 *clockwise;
    uint8 *touched;
    uint8 *affected;
    SplineSet *ss;
    RefChar *ref;
    InstrCt ct;
    int i;

    if ( !sc->layers[gic->layer].order2 )
return;

    if ( sc->layers[gic->layer].refs!=NULL && sc->layers[gic->layer].splines!=NULL ) {
	ff_post_error(_("Can't instruct this glyph"),
		_("TrueType does not support mixed references and contours.\nIf you want instructions for %.30s you should either:\n * Unlink the reference(s)\n * Copy the inline contours into their own (unencoded\n    glyph) and make a reference to that."),
		sc->name );
return;
    }
    for ( ref = sc->layers[gic->layer].refs; ref!=NULL; ref=ref->next ) {
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
    SCNumberPoints(sc,gic->layer);
    if ( autohint_before_generate && sc->changedsincelasthinted &&
	    !sc->manualhints )
	SplineCharAutoHint(sc,gic->layer,NULL);

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

    if ( sc->layers[gic->layer].splines==NULL )
return;

    /* Start dealing with the glyph */
    contourcnt = 0;
    for ( ss=sc->layers[gic->layer].splines; ss!=NULL; ss=ss->next, ++contourcnt );
    cnt = SSTtfNumberPoints(sc->layers[gic->layer].splines);

    contourends = malloc((contourcnt+1)*sizeof(int));
    clockwise = calloc(contourcnt,1);
    bp = malloc(cnt*sizeof(BasePoint));
    touched = calloc(cnt,1);
    affected = calloc(cnt,1);

    contourcnt = cnt = 0;
    for ( ss=sc->layers[gic->layer].splines; ss!=NULL; ss=ss->next ) {
        touched[cnt] |= tf_startcontour;
        cnt = SSAddPoints(ss,cnt,bp,NULL);
        touched[cnt-1] |= tf_endcontour;
        contourends[contourcnt] = cnt-1;
        clockwise[contourcnt++] = SplinePointListIsClockwise(ss);
    }
    contourends[contourcnt] = 0;

    for (i=0; i<gic->bluecnt; i++)
        gic->blues[i].highest = gic->blues[i].lowest = -1;

    ct.gic = gic;

    ct.sc = sc;
    ct.ss = sc->layers[gic->layer].splines;
    ct.instrs = NULL;
    ct.pt = NULL;
    ct.ptcnt = cnt;
    ct.contourends = contourends;
    ct.clockwise = clockwise;
    ct.bp = bp;
    ct.touched = touched;
    ct.affected = affected;
    ct.diagstems = NULL;
    ct.diagpts = NULL;

    ct.rp0 = 0;

    dogeninstructions(&ct);

    free(touched);
    free(affected);
    free(bp);
    free(contourends);
    free(clockwise);

    SCMarkInstrDlgAsChanged(sc);
    SCHintsChanged(sc);
}
