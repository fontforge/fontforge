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

#include "splinesave.h"

#include "autohint.h"
#include "dumppfa.h"
#include "fontforge.h"
#include "fvfonts.h"
#include "parsepfa.h"
#include <stdio.h>
#include <math.h>
#include "splinefont.h"
#include "splineorder2.h"
#include "splinesaveafm.h"
#include "psfont.h"
#include <ustring.h>
#include <string.h>
#include <utype.h>
#include <gwidget.h>

float GenerateHintWidthEqualityTolerance = 0.0;
int autohint_before_generate = 1;

/* Let's talk about references. */
/* If we are doing Type1 output, then the obvious way of doing them is seac */
/*  but that's so limitting. It only works for exactly two characters both */
/*  of which are in Adobe's Standard Enc. Only translations allowed. Only */
/*  one reference may be translated and the width of the char must match */
/*  that of the non-translated reference */
/*   The first extension we can make is to allow a single character reference */
/*  by making the other character be a space */
/*   But if we want to do more than that we must use subrs. If we have two */
/*  refs in subrs then we can do translations by preceding the subr calls by */
/*  appropriate rmovetos. Actually the specs say that only one rmoveto should */
/*  precede a path, so that means we can't allow the subroutines to position */
/*  themselves, they must just assume that they are called with the current */
/*  position correct for the first point. But then we need to know where the */
/*  first point should be placed, so we allocate a BasePoint to hold that info*/
/*  and store it into the "keys" array (which the subrs don't use). Similarly */
/*  we need to know where the subr will leave us, so we actually allocate 2 */
/*  BasePoints, one containing the start point, one the end point */
/*   But that's still not good enough, hints are defined in such a way that */
/*  they are not relocateable. So our subrs can't include any hint definitions*/
/*  (or if they do then that subr can't be translated at all). So hints must */
/*  be set outside the subrs, and the subrs can't be for chars that need hint */
/*  substitution. Unless... The subr will never be relocated. */
/*   So we generate two types of reference subrs, one containing no hints, the*/
/*  other containing all the hints, stems and flexes. The first type may be */
/*  translated, the second cannot */
/* Type2 doesn't allow any seacs */
/*  So everything must go in subrs. We have a slightly different problem here:*/
/*  hintmasks need to know exactly how many stem hints there are in the char */
/*  so we can't include any hintmask operators inside a subr (unless we */
/*  guarantee that all invocations of that subr are done with the same number */
/*  of hints in the character). This again means that no char with hint subs- */
/*  titutions may be put in a subr. UNLESS all the other references in a */
/*  refering character contain no hints */

/* That's very complex. And it doesn't do a very good job. */
/* Instead let's take all strings bounded by either moveto or hintmask operators */
/*  store these as potential subroutines. So a glyph becomes a sequence of    */
/*  potential subroutine calls preceded by the glyph header (width, hint decl,*/
/*  counter declarations, etc.) and intersperced by hintmask/moveto operators */
/* Each time we get a potential subr we hash it and see if we've used that    */
/*  string before. If we have then we merge the two. Otherwise it's a new one.*/
/* Then at the end we see what strings get used often enough to go into subrs */
/*  we create the subrs array from that.				      */
/* Then each glyph. We insert the preamble. We check of the potential subroutine */
/*  became a real subroutine. If so we call it, else we insert the data inline*/
/*  Do the same for the next hintmask/moveto and potential subroutine...      */

/* Then, on top of that I tried generating some full glyph subroutines, and   */
/*  to my surprise, it just made things worse.                                */

struct potentialsubrs {
    uint8 *data;		/* the charstring of the subr */
    int len;			/* the length of the charstring */
    int idx;			/* initially index into psubrs array */
    				/*  then index into subrs array or -1 if none */
    int cnt;			/* the usage count */
    int fd;			/* Which sub font is it in */
				/* -1 => used in more than one */
    int next;
    int full_glyph_index;	/* Into the glyphbits array */
				/* for full references */
    BasePoint *startstop;	/* Again for full references */
};

struct bits {
    uint8 *data;
    int dlen;
    int psub_index;
};

struct glyphbits {
    SplineChar *sc;
    int fd;			/* Which subfont is it in */
    int bcnt;
    struct bits *bits;
    uint8 wasseac;
};

#define HSH_SIZE	511
/* In type2 charstrings we divide every character into bits where a bit is */
/* bounded by a hintmask/moveto. Each of these is a potential subroutine and */
/* is stored here */
typedef struct glyphinfo {
    struct potentialsubrs *psubrs;
    int pcnt, pmax;
    int hashed[HSH_SIZE];
    struct glyphbits *gb, *active;
    SplineFont *sf;
    int layer;
    int glyphcnt;
    int subfontcnt;
    int bcnt, bmax;
    struct bits *bits;		/* For current glyph */
    const int *bygid;
    int justbroken;
    int instance_count;
} GlyphInfo;

struct mhlist {
    uint8 mask[HntMax/8];
    int subr;
    struct mhlist *next;
};

struct hintdb {
    uint8 mask[HntMax/8];
    int cnt;				/* number of hints */
    struct mhlist *sublist;
    struct pschars *subrs;
    /*SplineChar *sc;*/
    SplineChar **scs;
    int instance_count;
    unsigned int iscjk: 1;		/* If cjk then don't do stem3 hints */
					/* Will be done with counters instead */
	/* actually, most of the time we can't use stem3s, only if those three*/
	/* stems are always active and there are no other stems !(h/v)hasoverlap*/
    unsigned int noconflicts: 1;
    unsigned int startset: 1;
    unsigned int skiphm: 1;		/* Set when coming back to the start point of a contour. hintmask should be set the first time, not the second */
    unsigned int donefirsthm: 1;
    int cursub;				/* Current subr number */
    DBasePoint current;
    GlyphInfo *gi;
};

static void GIContentsFree(GlyphInfo *gi,SplineChar *dummynotdef) {
    int i,j;

    if ( gi->glyphcnt>0 && gi->gb[0].sc == dummynotdef ) {
	if ( dummynotdef->layers!=NULL ) {
	    SplinePointListsFree(dummynotdef->layers[gi->layer].splines);
	    dummynotdef->layers[gi->layer].splines = NULL;
	}
	StemInfosFree(dummynotdef->hstem);
	StemInfosFree(dummynotdef->vstem);
	dummynotdef->vstem = dummynotdef->hstem = NULL;
	free(dummynotdef->layers);
	dummynotdef->layers = NULL;
    }

    for ( i=0; i<gi->pcnt; ++i ) {
	free(gi->psubrs[i].data);
	free(gi->psubrs[i].startstop);
	gi->psubrs[i].data = NULL;
	gi->psubrs[i].startstop = NULL;
    }
    for ( i=0; i<gi->glyphcnt; ++i ) {
	for ( j=0; j<gi->gb[i].bcnt; ++j )
	    free(gi->gb[i].bits[j].data);
	free(gi->gb[i].bits);
	gi->gb[i].bits = NULL;
	gi->gb[i].bcnt = 0;
    }

    gi->pcnt = 0;
    gi->bcnt = 0;
    gi->justbroken = 0;
}

static void GIFree(GlyphInfo *gi,SplineChar *dummynotdef) {

    GIContentsFree(gi,dummynotdef);

    free(gi->gb);
    free(gi->psubrs);
    free(gi->bits);
}

static void StartNextSubroutine(GrowBuf *gb,struct hintdb *hdb) {
    GlyphInfo *gi;

    if ( hdb==NULL )
return;
    gi = hdb->gi;
    if ( gi==NULL )
return;
    /* Store everything in the grow buf into the data/dlen of the next bit */
    if ( gi->bcnt==-1 ) gi->bcnt = 0;
    if ( gi->bcnt>=gi->bmax )
	gi->bits = realloc(gi->bits,(gi->bmax+=20)*sizeof(struct bits));
    gi->bits[gi->bcnt].dlen = gb->pt-gb->base;
    gi->bits[gi->bcnt].data = malloc(gi->bits[gi->bcnt].dlen);
    gi->bits[gi->bcnt].psub_index = -1;
    memcpy(gi->bits[gi->bcnt].data,gb->base,gi->bits[gi->bcnt].dlen);
    gb->pt = gb->base;
    gi->justbroken = false;
}

static int hashfunc(uint8 *data, int len) {
    uint8 *end = data+len;
    unsigned int hash = 0, r;

    while ( data<end ) {
	r = (hash>>30)&3;
	hash <<= 2;
	hash = (hash|r)&0xffffffff;
	hash ^= *data++;
    }
return( hash%HSH_SIZE );
}

static void BreakSubroutine(GrowBuf *gb,struct hintdb *hdb) {
    GlyphInfo *gi;
    struct potentialsubrs *ps;
    int hash;
    int pi;

    if ( hdb==NULL )
return;
    gi = hdb->gi;
    if ( gi==NULL )
return;
    /* The stuff before the first moveto in a glyph (the header that sets */
    /*  the width, sets up the hints, counters, etc.) can't go into a subr */
    if ( gi->bcnt==-1 ) {
	gi->bcnt=0;
	gi->justbroken = true;
return;
    } else if ( gi->justbroken )
return;
    /* Otherwise stuff everything in the growbuffer into a subr */
    hash = hashfunc(gb->base,gb->pt-gb->base);
    ps = NULL;
    for ( pi=gi->hashed[hash]; pi!=-1; pi=gi->psubrs[pi].next ) {
	ps = &gi->psubrs[pi];
	if ( ps->len==gb->pt-gb->base && memcmp(ps->data,gb->base,gb->pt-gb->base)==0 )
    break;
    }
    if ( pi==-1 ) {
	if ( gi->pcnt>=gi->pmax )
	    gi->psubrs = realloc(gi->psubrs,(gi->pmax+=gi->glyphcnt)*sizeof(struct potentialsubrs));
	ps = &gi->psubrs[gi->pcnt];
	memset(ps,0,sizeof(*ps));	/* set cnt to 0 */
	ps->idx = gi->pcnt++;
	ps->len = gb->pt-gb->base;
	ps->data = malloc(ps->len);
	memcpy(ps->data,gb->base,ps->len);
	ps->next = gi->hashed[hash];
	gi->hashed[hash] = ps->idx;
	ps->fd = gi->active->fd;
	ps->full_glyph_index = -1;
    }
    if ( ps->fd!=gi->active->fd )
	ps->fd = -1;			/* used in multiple cid sub-fonts */
    gi->bits[gi->bcnt].psub_index = ps->idx;
    ++ps->cnt;
    gb->pt = gb->base;
    ++gi->bcnt;
    gi->justbroken = true;
}

static void MoveSubrsToChar(GlyphInfo *gi) {
    struct glyphbits *active;

    if ( gi==NULL )
return;
    active = gi->active;
    active->bcnt = gi->bcnt;
    active->bits = malloc(active->bcnt*sizeof(struct bits));
    memcpy(active->bits,gi->bits,active->bcnt*sizeof(struct bits));
    gi->bcnt = 0;
}

static int NumberHints(SplineChar *scs[MmMax], int instance_count) {
    int i,j, cnt=-1;
    StemInfo *s;

    for ( j=0; j<instance_count; ++j ) {
	for ( s=scs[j]->hstem, i=0; s!=NULL; s=s->next ) {
	    if ( i<HntMax )
		s->hintnumber = i++;
	    else
		s->hintnumber = -1;
	}
	for ( s=scs[j]->vstem; s!=NULL; s=s->next ) {
	    if ( i<HntMax )
		s->hintnumber = i++;
	    else
		s->hintnumber = -1;
	}
	if ( cnt==-1 )
	    cnt = i;
	else if ( cnt!=i )
	    IError("MM font with different hint counts");
    }
return( cnt );
}

void RefCharsFreeRef(RefChar *ref) {
    RefChar *rnext;

    while ( ref!=NULL ) {
	rnext = ref->next;
	/* don't free the splines */
	free(ref->layers);
	chunkfree(ref,sizeof(RefChar));
	ref = rnext;
    }
}

static void MarkTranslationRefs(SplineFont *sf,int layer) {
    int i;
    SplineChar *sc;
    RefChar *r;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	for ( r = sc->layers[layer].refs; r!=NULL; r=r->next )
	    r->justtranslated = (r->transform[0]==1 && r->transform[3]==1 &&
		    r->transform[1]==0 && r->transform[2]==0);
    }
}

/* ************************************************************************** */
/* ********************** Type1 PostScript CharStrings ********************** */
/* ************************************************************************** */

static bigreal myround( bigreal pos, int round ) {
    if ( round )
return( rint( pos ));
    else
return( rint( pos*1024. )/1024. );
}

static void AddNumber(GrowBuf *gb, real pos, int round) {
    int dodiv = 0;
    int val;
    unsigned char *str;

    if ( gb->pt+8>=gb->end )
	GrowBuffer(gb);

    if ( !round && pos!=floor(pos) ) {
	{
	    if ( rint(pos*64)/64 == pos ) {
		pos *= 64;
		dodiv = 64;
	    } else {
		pos *= 1024;
		dodiv = 1024;
	    }
	}
    }
    pos = rint(pos);
    if ( dodiv>0 && floor(pos)/dodiv == floor(pos/dodiv) ) {
	pos = rint(pos/dodiv);
	dodiv = 0;
    }
    val = pos;
    str = gb->pt;

    if ( pos>=-107 && pos<=107 )
	*str++ = val+139;
    else if ( pos>=108 && pos<=1131 ) {
	val -= 108;
	*str++ = (val>>8)+247;
	*str++ = val&0xff;
    } else if ( pos>=-1131 && pos<=-108 ) {
	val = -val;
	val -= 108;
	*str++ = (val>>8)+251;
	*str++ = val&0xff;
    } else {
	*str++ = '\377';
	*str++ = (val>>24)&0xff;
	*str++ = (val>>16)&0xff;
	*str++ = (val>>8)&0xff;
	*str++ = val&0xff;
    }
    if ( dodiv ) {
	if ( dodiv<107 )
	    *str++ = dodiv+139;
	else {
	    dodiv -= 108;
	    *str++ = (dodiv>>8)+247;
	    *str++ = dodiv&0xff;
	}
	*str++ = 12;		/* div (byte1) */
	*str++ = 12;		/* div (byte2) */
    }
    gb->pt = str;
}

/* When doing a multiple master font we have multiple instances of the same data */
/*  which must all be added, and then a call made to the appropriate blend routine */
/* This is complicated because all the data may not fit on the stack so we */
/*  may need to make multiple calls */
static void AddData(GrowBuf *gb, bigreal data[MmMax][6], int instances, int num_coords,
	int round) {
    int allsame = true, alls[6];
    int i,j, chunk,min,max,subr;

    for ( j=0; j<num_coords; ++j ) {
	alls[j] = true;
	for ( i=1; i<instances; ++i ) {
	    if ( data[i][j]!=data[0][j] ) {
		alls[j] = false;
		allsame = false;
	    break;
	    }
	}
    }

    if ( allsame ) {		/* No need for blending */
				/*  Probably a normal font, but possible in an mm */
	for ( j=0; j<num_coords; ++j )
	    AddNumber(gb,data[0][j],round);
return;
    }

    chunk = 22/instances;
    if ( chunk == 5 ) chunk = 4;	/* No subroutine for 5 items */
    min = 0;
    while ( min<num_coords ) {
	while ( min<num_coords && alls[min] ) {
	    AddNumber(gb,data[0][min],round);
	    ++min;
	}
	max = min+chunk;
	if ( max>num_coords ) max = num_coords;
	while ( max-1>min && alls[max-1] )
	    --max;
	if ( max-min==5 ) max=min+4;
	if ( min<max ) {
	    for ( j=min; j<max; ++j )
		AddNumber(gb,data[0][j],round);
	    for ( j=min; j<max; ++j )
		for ( i=1; i<instances; ++i )
		    AddNumber(gb,data[i][j]-data[0][j],round);
	    subr = (j-min) + 4;
	    if ( j-min==6 ) subr = 9;
	    AddNumber(gb,subr,round);
	    if ( gb->pt+1>=gb->end )
		GrowBuffer(gb);
	    *gb->pt++ = 10;			/* callsubr */
	    min = j;
	}
    }
}

int CvtPsStem3(GrowBuf *gb, SplineChar *scs[MmMax], int instance_count,
	int ishstem, int round) {
    StemInfo *h1, *h2, *h3;
    StemInfo _h1, _h2, _h3;
    bigreal data[MmMax][6];
    int i;
    real off;

    for ( i=0; i<instance_count; ++i ) {
	if ( (ishstem && scs[i]->hconflicts) || (!ishstem && scs[i]->vconflicts))
return( false );
	h1 = ishstem ? scs[i]->hstem : scs[i]->vstem;
	if ( h1==NULL || (h2 = h1->next)==NULL || (h3=h2->next)==NULL )
return( false );
	if ( h3->next!=NULL )
return( false );
	off = ishstem ? 0 : scs[i]->lsidebearing;
	if ( h1->width<0 ) {
	    _h1 = *h1;
	    _h1.start += _h1.width;
	    _h1.width = -_h1.width;
	    h1 = &_h1;
	}
	if ( h2->width<0 ) {
	    _h2 = *h2;
	    _h2.start += _h2.width;
	    _h2.width = -_h2.width;
	    h2 = &_h2;
	}
	if ( h3->width<0 ) {
	    _h3 = *h3;
	    _h3.start += _h3.width;
	    _h3.width = -_h3.width;
	    h3 = &_h3;
	}

	if ( h1->start>h2->start ) {
	    StemInfo *ht = h1; h1 = h2; h2 = ht;
	}
	if ( h1->start>h3->start ) {
	    StemInfo *ht = h1; h1 = h3; h3 = ht;
	}
	if ( h2->start>h3->start ) {
	    StemInfo *ht = h2; h2 = h3; h3 = ht;
	}
	if ( h1->width != h3->width )
return( false );
	if ( (h2->start+h2->width/2) - (h1->start+h1->width/2) !=
		(h3->start+h3->width/2) - (h2->start+h2->width/2) )
return( false );
	data[i][0] = h1->start-off;
	data[i][1] = h1->width;
	data[i][2] = h2->start-off;
	data[i][3] = h2->width;
	data[i][4] = h3->start-off;
	data[i][5] = h3->width;
    }
    if ( gb==NULL )
return( true );
    AddData(gb,data,instance_count,6,round);
    if ( gb->pt+3>=gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 12;
    *(gb->pt)++ = ishstem?2:1;				/* h/v stem3 */
return( true );
}

static void CvtPsHints(GrowBuf *gb, SplineChar *scs[MmMax], int instance_count,
	int ishstem, int round, int iscjk, real *offsets ) {
    StemInfo *hs[MmMax];
    bigreal data[MmMax][6];
    int i;
    real off;

    for ( i=0; i<instance_count; ++i )
	hs[i] = ishstem ? scs[i]->hstem : scs[i]->vstem;

    if ( hs[0]!=NULL && hs[0]->next!=NULL && hs[0]->next->next!=NULL &&
	    hs[0]->next->next->next==NULL )
	if ( !iscjk && CvtPsStem3(gb, scs, instance_count, ishstem, round))
return;

    while ( hs[0]!=NULL ) {
	for ( i=0; i<instance_count; ++i ) {
	    off = offsets!=NULL ? offsets[i] :
		    ishstem ? 0 : scs[i]->lsidebearing;
	    if ( hs[i]->ghost ) {
		data[i][0] = hs[i]->start-off+hs[i]->width;
		data[i][1] = -hs[i]->width;
	    } else {
		data[i][0] = hs[i]->start-off;
		data[i][1] = hs[i]->width;
	    }
	}
	AddData(gb,data,instance_count,2,round);
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*(gb->pt)++ = ishstem?1:3;			/* h/v stem */
	for ( i=0; i<instance_count; ++i )
	    hs[i] = hs[i]->next;
    }
}

static void CvtPsMasked(GrowBuf *gb,SplineChar *scs[MmMax], int instance_count,
	int ishstem, int round, uint8 mask[12] ) {
    StemInfo *hs[MmMax];
    bigreal data[MmMax][6], off;
    int i;

    for ( i=0; i<instance_count; ++i )
	hs[i] = ishstem ? scs[i]->hstem : scs[i]->vstem;

    while ( hs[0]!=NULL ) {
	if ( hs[0]->hintnumber!=-1 &&
		(mask[hs[0]->hintnumber>>3]&(0x80>>(hs[0]->hintnumber&7))) ) {
	    for ( i=0; i<instance_count; ++i ) {
		off = ishstem ? 0 : scs[i]->lsidebearing;
		if ( hs[i]->ghost ) {
		    data[i][0] = hs[i]->start-off+hs[i]->width;
		    data[i][1] = -hs[i]->width;
		} else {
		    data[i][0] = hs[i]->start-off;
		    data[i][1] = hs[i]->width;
		}
	    }
	    AddData(gb,data,instance_count,2,round);
	    if ( gb->pt+1>=gb->end )
		GrowBuffer(gb);
	    *(gb->pt)++ = ishstem?1:3;			/* h/v stem */
	}
	for ( i=0; i<instance_count; ++i )
	    hs[i] = hs[i]->next;
    }
}

static int FigureCounters(StemInfo *stems,real *hints,int base,real offset,
	int countermask_cnt, HintMask *counters) {
    StemInfo *h;
    int pos = base+1, subbase, cnt=0;
    real last = offset;
    int i;

    for ( i=0; i<countermask_cnt; ++i ) {
	subbase = pos;
	for ( h=stems; h!=NULL ; h=h->next ) {
	    if ( h->hintnumber!=-1 && (counters[i][h->hintnumber>>3]&(0x80>>(h->hintnumber&7))) ) {
		hints[pos++] = h->start-last;
		hints[pos++] = h->width;
		last = h->start+h->width;
	    }
	}
	if ( pos!=subbase ) {
	    hints[pos-2] += hints[pos-1];
	    hints[pos-1] = -hints[pos-1];		/* Mark end of group */
	    last = offset;				/* Each new group starts at 0 or lbearing */
	    ++cnt;
	}
    }
    hints[base] = cnt;
return( pos );
}

static void CounterHints1(GrowBuf *gb, SplineChar *sc, int round) {
    real hints[HntMax*2+2];		/* At most 96 hints, no hint used more than once */
    int pos, i, j;

    if ( sc->countermask_cnt==0 )
return;

    pos = FigureCounters(sc->hstem,hints,0,0,sc->countermask_cnt,
	    sc->countermasks);
    /* Adobe's docs (T1_Supp.pdf, section 2.4) say these should be offset from*/
    /*  the left side bearing. The example (T1_Supp.pdf, 2.6) shows them offset*/
    /*  from 0. I've no idea which is correct, so I'll follow the words, think-*/
    /*  that the lbearing might have been set to 0 even though it shouldn't */
    /*  have been. */
    pos = FigureCounters(sc->vstem,hints,pos,sc->lsidebearing,sc->countermask_cnt,
	    sc->countermasks);
    if ( pos==2 )	/* => no counters, one byte to say 0 h counters, one byte for 0 v counters */
return;
    for ( i=pos; i>22; i-=22 ) {
	for ( j=i-22; j<i; ++j )
	    AddNumber(gb,hints[j],round);
	AddNumber(gb,22,round);
	AddNumber(gb,12,round);
	if ( gb->pt+2>=gb->end )
	    GrowBuffer(gb);
	*(gb->pt)++ = 12;
	*(gb->pt)++ = 16;		/* CallOtherSubr */
    }
    for ( j=0; j<i; ++j )
	AddNumber(gb,hints[j],round);
    AddNumber(gb,i,round);
    AddNumber(gb,13,round);
    if ( gb->pt+2>=gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 12;
    *(gb->pt)++ = 16;		/* CallOtherSubr */
}

static void SubrsCheck(struct pschars *subrs) {
	/* <subr number presumed to be on stack> 1 3 callother pop callsubr */

    if ( subrs->next>=subrs->cnt ) {
	subrs->cnt += 100;
	subrs->values = realloc(subrs->values,subrs->cnt*sizeof(uint8 *));
	subrs->lens = realloc(subrs->lens,subrs->cnt*sizeof(int));
	if ( subrs->keys!=NULL ) {
	    int i;
	    subrs->keys = realloc(subrs->keys,subrs->cnt*sizeof(char *));
	    for ( i=subrs->cnt-100; i<subrs->cnt; ++i )
		subrs->keys[i] = NULL;
	}
    }
}

/* Does the hintmask we need already exist in a subroutine? if so return that */
/*  subr. Else build a new subr with the hints we need. Note we can only use */
/*  *stem3 commands if there are no conflicts in that coordinate, it isn't cjk*/
/*  and all the other conditions are met */
static int FindOrBuildHintSubr(struct hintdb *hdb, uint8 mask[12], int round) {
    struct mhlist *mh;
    GrowBuf gb;

    for ( mh=hdb->sublist; mh!=NULL; mh=mh->next ) {
	if ( memcmp(mask,mh->mask,sizeof(uint8)*12)==0 )
return( mh->subr );
    }
    SubrsCheck(hdb->subrs);

    memset(&gb,0,sizeof(gb));
    if ( !hdb->scs[0]->hconflicts )
	CvtPsHints(&gb,hdb->scs,hdb->instance_count,true,round,hdb->iscjk,NULL);
    else
	CvtPsMasked(&gb,hdb->scs,hdb->instance_count,true,round,mask);
    if ( !hdb->scs[0]->vconflicts )
	CvtPsHints(&gb,hdb->scs,hdb->instance_count,false,round,hdb->iscjk,NULL);
    else
	CvtPsMasked(&gb,hdb->scs,hdb->instance_count,false,round,mask);
    if ( gb.pt+1 >= gb.end )
	GrowBuffer(&gb);
    *gb.pt++ = 11;			/* return */

    /* Replace an old subroutine */
    if ( mh!=NULL ) {
	free( hdb->subrs->values[mh->subr]);
	hdb->subrs->values[mh->subr] = (uint8 *) copyn((char *) gb.base,gb.pt-gb.base);
	hdb->subrs->lens[mh->subr] = gb.pt-gb.base;
	memcpy(mh->mask,mask,sizeof(mh->mask));
    } else {
	hdb->subrs->values[hdb->subrs->next] = (uint8 *) copyn((char *) gb.base,gb.pt-gb.base);
	hdb->subrs->lens[hdb->subrs->next] = gb.pt-gb.base;

	mh = calloc(1,sizeof(struct mhlist));
	memcpy(mh->mask,mask,sizeof(mh->mask));
	mh->subr = hdb->subrs->next++;
	mh->next = hdb->sublist;
	hdb->sublist = mh;
    }
    free(gb.base);

return( mh->subr );
}

static void CallTransformedHintSubr(GrowBuf *gb,struct hintdb *hdb,
	SplineChar *scs[MmMax], RefChar *refs[MmMax], BasePoint trans[MmMax],
	int instance_count, int round) {
    HintMask hm;
    int s;

    if ( HintMaskFromTransformedRef(refs[0],&trans[0],scs[0],&hm)==NULL )
return;
    s = FindOrBuildHintSubr(hdb, hm, round);
    AddNumber(gb,s,round);
    AddNumber(gb,4,round);		/* subr 4 is (my) magic subr that does the hint subs call */
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *gb->pt++ = 10;			/* callsubr */
}

static void HintSetup(GrowBuf *gb,struct hintdb *hdb, SplinePoint *to,
	int round, int break_subr ) {
    int s;
    int i;

    if ( to->hintmask==NULL || hdb->noconflicts )
return;
    if ( hdb->scs[0]->hstem==NULL && hdb->scs[0]->vstem==NULL )		/* Hints are turned off. Hint mask still remains though */
return;
    for ( i=0; i<HntMax/8; ++i )
	if ( to->hintmask[i]!=0 )
    break;
    if ( i==HntMax/8 )		/* Empty mask */
return;

    s = FindOrBuildHintSubr(hdb,*to->hintmask,round);
    memcpy(hdb->mask,*to->hintmask,sizeof(HintMask));
    if ( hdb->cursub == s ) {			/* If we were able to redefine */
return;						/* the subroutine currently */
    }						/* active then we are done */

    if ( break_subr )
	BreakSubroutine(gb,hdb);

    AddNumber(gb,s,round);
    AddNumber(gb,4,round);		/* subr 4 is (my) magic subr that does the hint subs call */
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *gb->pt++ = 10;			/* callsubr */
    hdb->cursub = s;
    if ( break_subr )
	StartNextSubroutine(gb,hdb);
}

static void _moveto(GrowBuf *gb,DBasePoint *current,BasePoint *to,int instance_count,
	int line, int round, struct hintdb *hdb) {
    BasePoint temp[MmMax];
    int i, samex, samey;
    bigreal data[MmMax][6];

    if ( gb->pt+18 >= gb->end )
	GrowBuffer(gb);

    for ( i=0; i<instance_count; ++i ) {
	temp[i].x = myround(to[i].x,round);
	temp[i].y = myround(to[i].y,round);
    }
    to = temp;
    samex = samey = true;
    for ( i=0; i<instance_count; ++i ) {
	if ( current[i].x!=to[i].x ) samex = false;
	if ( current[i].y!=to[i].y ) samey = false;
    }
    if ( samex ) {
	for ( i=0; i<instance_count; ++i )
	    data[i][0] = to[i].y-current[i].y;
	AddData(gb,data,instance_count,1,round);
	*(gb->pt)++ = line ? 7 : 4;		/* v move/line to */
	for ( i=0; i<instance_count; ++i )
	    current[i].y += data[i][0];
    } else if ( samey ) {
	for ( i=0; i<instance_count; ++i )
	    data[i][0] = to[i].x-current[i].x;
	AddData(gb,data,instance_count,1,round);
	*(gb->pt)++ = line ? 6 : 22;		/* h move/line to */
	for ( i=0; i<instance_count; ++i )
	    current[i].x += data[i][0];
    } else {
	for ( i=0; i<instance_count; ++i ) {
	    data[i][0] = to[i].x-current[i].x;
	    data[i][1] = to[i].y-current[i].y;
	}
	AddData(gb,data,instance_count,2,round);
	*(gb->pt)++ = line ? 5 : 21;		/* r move/line to */
	for ( i=0; i<instance_count; ++i ) {
	    current[i].x += data[i][0];
	    current[i].y += data[i][1];
	}
    }
    if ( !line )
	StartNextSubroutine(gb,hdb);
}

static void moveto(GrowBuf *gb,DBasePoint *current,Spline *splines[MmMax],
	int instance_count, int line, int round, struct hintdb *hdb) {
    BasePoint to[MmMax];
    int i;

    if ( !line) BreakSubroutine(gb,hdb);
    if ( hdb!=NULL ) HintSetup(gb,hdb,splines[0]->to,round,line);
    for ( i=0; i<instance_count; ++i )
	to[i] = splines[i]->to->me;
    _moveto(gb,current,to,instance_count,line,round,hdb);
}

static void splmoveto(GrowBuf *gb,DBasePoint *current,SplineSet *spl[MmMax],
	int instance_count, int line, int round, struct hintdb *hdb) {
    BasePoint to[MmMax];
    int i;

    if ( !line) BreakSubroutine(gb,hdb);
    if ( hdb!=NULL ) HintSetup(gb,hdb,spl[0]->first,round,line);
    for ( i=0; i<instance_count; ++i )
	to[i] = spl[i]->first->me;
    _moveto(gb,current,to,instance_count,line,round,hdb);
}

static int NeverConflicts(RefChar *refs[MmMax], int instance_count) {
    int i;
    for ( i=0; i<instance_count; ++i )
	if ( refs[i]->sc->hconflicts || refs[i]->sc->vconflicts )
return( false );

return( true );
}

static int AllStationary(RefChar *refs[MmMax], BasePoint trans[MmMax], int instance_count) {
    int i;
    for ( i=0; i<instance_count; ++i )
	if ( !refs[i]->justtranslated ||
		refs[i]->transform[4]+trans[i].x!=0 || 
		refs[i]->transform[5]+trans[i].y!=0 )
return( false );

return( true );
}

static int AnyRefs(SplineChar *sc,int layer) {

return( sc->layers[layer].refs!=NULL );
}

static void refmoveto(GrowBuf *gb,DBasePoint *current,BasePoint rpos[MmMax],
	int instance_count, int line, int round, struct hintdb *hdb, RefChar *refs[MmMax]) {
    BasePoint to[MmMax];
    int i;

    if ( !line) BreakSubroutine(gb,hdb);
    for ( i=0; i<instance_count; ++i ) {
	to[i] = rpos[i];
	if ( refs!=NULL ) {
	    to[i].x += refs[i]->transform[4];
	    to[i].y += refs[i]->transform[5];
	}
    }
    _moveto(gb,current,to,instance_count,line,round,hdb);
}

static void curveto(GrowBuf *gb,DBasePoint *current,Spline *splines[MmMax],int instance_count,
	int round, struct hintdb *hdb) {
    BasePoint temp1[MmMax], temp2[MmMax], temp3[MmMax], *c0[MmMax], *c1[MmMax], *s1[MmMax];
    bigreal data[MmMax][6];
    int i, op, opcnt;
    int vh, hv;

    if ( hdb!=NULL ) HintSetup(gb,hdb,splines[0]->to,round,true);

    if ( gb->pt+50 >= gb->end )
	GrowBuffer(gb);

    vh = hv = true;
    for ( i=0; i<instance_count; ++i ) {
	c0[i] = &splines[i]->from->nextcp;
	c1[i] = &splines[i]->to->prevcp;
	s1[i] = &splines[i]->to->me;
	temp1[i].x = myround(c0[i]->x,round);
	temp1[i].y = myround(c0[i]->y,round);
	c0[i] = &temp1[i];
	temp2[i].x = myround(c1[i]->x,round);
	temp2[i].y = myround(c1[i]->y,round);
	c1[i] = &temp2[i];
	temp3[i].x = myround(s1[i]->x,round);
	temp3[i].y = myround(s1[i]->y,round);
	s1[i] = &temp3[i];
	if ( current[i].x != c0[i]->x || c1[i]->y!=s1[i]->y ) vh = false;
	if ( current[i].y != c0[i]->y || c1[i]->x!=s1[i]->x ) hv = false;
    }
    if ( vh ) {
	for ( i=0; i<instance_count; ++i ) {
	    data[i][0] = c0[i]->y-current[i].y;
	    data[i][1] = c1[i]->x-c0[i]->x;
	    data[i][2] = c1[i]->y-c0[i]->y;
	    data[i][3] = s1[i]->x-c1[i]->x;
	}
	op = 30;		/* vhcurveto */
	opcnt = 4;
	for ( i=0; i<instance_count; ++i ) {
	    current[i].x += data[i][1]+data[i][3];
	    current[i].y += data[i][0]+data[i][2];
	}
    } else if ( hv ) {
	for ( i=0; i<instance_count; ++i ) {
	    data[i][0] = c0[i]->x-current[i].x;
	    data[i][1] = c1[i]->x-c0[i]->x;
	    data[i][2] = c1[i]->y-c0[i]->y;
	    data[i][3] = s1[i]->y-c1[i]->y;
	}
	op = 31;		/* hvcurveto */
	opcnt = 4;
	for ( i=0; i<instance_count; ++i ) {
	    current[i].x += data[i][0]+data[i][1];
	    current[i].y += data[i][2]+data[i][3];
	}
    } else {
	for ( i=0; i<instance_count; ++i ) {
	    data[i][0] = c0[i]->x-current[i].x;
	    data[i][1] = c0[i]->y-current[i].y;
	    data[i][2] = c1[i]->x-c0[i]->x;
	    data[i][3] = c1[i]->y-c0[i]->y;
	    data[i][4] = s1[i]->x-c1[i]->x;
	    data[i][5] = s1[i]->y-c1[i]->y;
	}
	op = 8;		/* rrcurveto */
	opcnt=6;
	for ( i=0; i<instance_count; ++i ) {
	    current[i].x += data[i][0]+data[i][2]+data[i][4];
	    current[i].y += data[i][1]+data[i][3]+data[i][5];
	}
    }
    AddData(gb,data,instance_count,opcnt,false);
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = op;
}

static int SplinesAreFlexible(Spline *splines[MmMax], int instance_count) {
    int i, x=false, y=false;

    for ( i=0; i<instance_count; ++i ) {
	if ( !splines[i]->to->flexx && !splines[i]->to->flexy )
return( false );
	if (( x && splines[i]->to->flexy ) || ( y && splines[i]->to->flexx ))
return( false );
	x = splines[i]->to->flexx;
	y = splines[i]->to->flexy;
    }
return( true );
}

static void flexto(GrowBuf *gb,DBasePoint current[MmMax],Spline *pspline[MmMax],
	int instance_count,int round, struct hintdb *hdb) {
    BasePoint *c0, *c1, *mid, *end=NULL;
    Spline *nspline;
    BasePoint offsets[MmMax][8];
    int i,j;
    BasePoint temp1, temp2, temp3, temp;
    bigreal data[MmMax][6];

    for ( j=0; j<instance_count; ++j ) {
	c0 = &pspline[j]->from->nextcp;
	c1 = &pspline[j]->to->prevcp;
	mid = &pspline[j]->to->me;

	temp1.x = myround(c0->x,round);
	temp1.y = myround(c0->y,round);
	c0 = &temp1;
	temp2.x = myround(c1->x,round);
	temp2.y = myround(c1->y,round);
	c1 = &temp2;
	temp.x = myround(mid->x,round);
	temp.y = myround(mid->y,round);
	mid = &temp;
/* reference point is same level as current point */
	if ( current[j].y==pspline[j]->to->next->to->me.y ) {
	    offsets[j][0].x = mid->x-current[j].x;	offsets[j][0].y = 0;
	    offsets[j][1].x = c0->x-mid->x;		offsets[j][1].y = c0->y-current[j].y;
	} else {
	    offsets[j][0].x = 0;			offsets[j][0].y = mid->y-current[j].y;
	    offsets[j][1].x = c0->x-current[j].x;	offsets[j][1].y = c0->y-mid->y;
	}
	offsets[j][2].x = c1->x-c0->x;		offsets[j][2].y = c1->y-c0->y;
	offsets[j][3].x = mid->x-c1->x;		offsets[j][3].y = mid->y-c1->y;
	nspline = pspline[j]->to->next;
	c0 = &nspline->from->nextcp;
	c1 = &nspline->to->prevcp;
	end = &nspline->to->me;

	temp1.x = myround(c0->x,round);
	temp1.y = myround(c0->y,round);
	c0 = &temp1;
	temp2.x = myround(c1->x,round);
	temp2.y = myround(c1->y,round);
	c1 = &temp2;
	temp3.x = myround(end->x,round);
	temp3.y = myround(end->y,round);
	end = &temp3;

	offsets[j][4].x = c0->x-mid->x;		offsets[j][4].y = c0->y-mid->y;
	offsets[j][5].x = c1->x-c0->x;		offsets[j][5].y = c1->y-c0->y;
	offsets[j][6].x = end->x-c1->x;		offsets[j][6].y = end->y-c1->y;
	offsets[j][7].x = end->x;		offsets[j][7].y = end->y;
	current[j].x = end->x;
	current[j].y = end->y;
    }

    if ( hdb!=NULL )
	HintSetup(gb,hdb,pspline[0]->to->next->to,round,false);

    if ( gb->pt+2 >= gb->end )
	GrowBuffer(gb);

    *(gb->pt)++ = 1+139;		/* 1 */
    *(gb->pt)++ = 10;			/* callsubr */
    for ( i=0; i<7; ++i ) {
	if ( gb->pt+20 >= gb->end )
	    GrowBuffer(gb);
	for ( j=0; j<instance_count; ++j ) {
	    data[j][0] = offsets[j][i].x;
	    data[j][1] = offsets[j][i].y;
	}
	AddData(gb,data,instance_count,2,round);
	*(gb->pt)++ = 21;		/* rmoveto */
	*(gb->pt)++ = 2+139;		/* 2 */
	*(gb->pt)++ = 10;		/* callsubr */
    }
    if ( gb->pt+20 >= gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 50+139;		/* 50, .50 pixels */
    for ( j=0; j<instance_count; ++j ) {
	data[j][0] = offsets[j][7].x;
	data[j][1] = offsets[j][7].y;
    }
    AddData(gb,data,instance_count,2,round);
    *(gb->pt)++ = 0+139;		/* 0 */
    *(gb->pt)++ = 10;			/* callsubr */

    current->x = end->x;
    current->y = end->y;
}

static void _CvtPsSplineSet(GrowBuf *gb, SplinePointList *spl[MmMax], int instance_count,
	DBasePoint current[MmMax],
	int round, struct hintdb *hdb, int is_order2, int stroked ) {
    Spline *spline[MmMax], *first;
    SplinePointList temp[MmMax], *freeme=NULL;
    int i;

    if ( is_order2 ) {
	freeme = spl[0] = SplineSetsPSApprox(spl[0]);
	instance_count = 1;
    }
    while ( spl[0]!=NULL ) {
	first = NULL;
	for ( i=0; i<instance_count; ++i )
	    SplineSetReverse(spl[i]);
	/* For some reason fontographer reads its spline in in the reverse */
	/*  order from that I use. I'm not sure how they do that. The result */
	/*  being that what I call clockwise they call counter. Oh well. */
	/*  If I reverse the splinesets after reading them in, and then again*/
	/*  when saving them out, all should be well */
	if ( spl[0]->first->flexy || spl[0]->first->flexx ) {
	    /* can't handle a flex (mid) point as the first point. rotate the */
	    /* list by one, this is possible because only closed paths have */
	    /* points marked as flex, and because we can't have two flex mid- */
	    /* points in a row */
	    for ( i = 0; i<instance_count; ++i ) {
		temp[i] = *spl[i];
		temp[i].first = temp[i].last = spl[i]->first->next->to;
		spl[i] = &temp[i];
	    }
	    if ( spl[0]->first->flexy || spl[0]->first->flexx ) {
		/* well, well, well. We did have two flexes in a row */
		for ( i = 0; i<instance_count; ++i ) {
		    spl[i]->first->flexx = spl[i]->first->flexy = false;
		}
	    }
	}
	splmoveto(gb,current,spl,instance_count,false,round,hdb);
	for ( i=0; i<instance_count; ++i )
	    spline[i] = spl[i]->first->next;
	while ( spline[0]!=NULL && spline[0]!=first ) {
	    if ( first==NULL ) first = spline[0];
	    if ( SplinesAreFlexible(spline,instance_count) &&
		    (hdb->noconflicts || spline[0]->to->hintmask==NULL)) {
		flexto(gb,current,spline,instance_count,round,hdb);	/* does two adjacent splines */
		for ( i=0; i<instance_count; ++i )
		    spline[i] = spline[i]->to->next;
	    } else if ( spline[0]->knownlinear && spline[0]->to==spl[0]->first ) {
		/* We can finish this off with the closepath */
	break;
	    } else if ( spline[0]->knownlinear )
		moveto(gb,current,spline,instance_count,true,round,hdb);
	    else
		curveto(gb,current,spline,instance_count,round,hdb);
	    for ( i=0; i<instance_count; ++i )
		spline[i] = spline[i]->to->next;
	}
	if ( !stroked || spl[0]->first->prev!=NULL ) {
	    if ( gb->pt+1 >= gb->end )
		GrowBuffer(gb);
	    *(gb->pt)++ = 9;			/* closepath */
	}
	for ( i=0; i<instance_count; ++i ) {
	    SplineSetReverse(spl[i]);
	    /* Of course, I have to Reverse again to get back to my convention after*/
	    /*  saving */
	    spl[i] = spl[i]->next;
	}
    }
    SplinePointListsFree(freeme);
}

static int IsPSSeacable(SplineChar *sc,int layer) {
    RefChar *ref;

    if ( sc->layers[layer].refs==NULL || sc->layers[layer].splines!=NULL )
return( false );

    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	if ( !ref->justtranslated )
return( false );
    }
return( true );
}

static RefChar *RefFindAdobe(RefChar *r, RefChar *t,int layer) {
    *t = *r;
    while ( t->adobe_enc==-1 && t->sc->layers[layer].refs!=NULL &&
	    t->sc->layers[layer].refs->next==NULL &&
	    t->sc->layers[layer].splines==NULL &&
	    t->sc->layers[layer].refs->justtranslated ) {
	t->transform[4] += t->sc->layers[layer].refs->transform[4];
	t->transform[5] += t->sc->layers[layer].refs->transform[5];
	t->adobe_enc = t->sc->layers[layer].refs->adobe_enc;
	t->orig_pos = t->sc->layers[layer].refs->orig_pos;
	t->sc = t->sc->layers[layer].refs->sc;
    }
return( t );
}

static int IsSeacable(GrowBuf *gb, SplineChar *scs[MmMax],
	int instance_count, int round,int layer) {
    /* can be at most two chars in a seac (actually must be exactly 2, but */
    /*  I'll put in a space if there's only one (and if splace is blank) */
    RefChar *r1, *r2, *rt, *refs;
    RefChar space, t1, t2;
    DBounds b;
    int i, j, swap;
    bigreal data[MmMax][6];

    for ( j=0 ; j<instance_count; ++j )
	if ( !IsPSSeacable(scs[j],layer))
return( false );

    refs = scs[0]->layers[layer].refs;
    if ( refs==NULL )
return( false );

    r1 = refs;
    if ((r2 = r1->next)==NULL ) {
	RefChar *refs = r1->sc->layers[layer].refs;
	if ( refs!=NULL && refs->next!=NULL && refs->next->next==NULL &&
		r1->sc->layers[layer].splines==NULL &&
		refs->adobe_enc!=-1 && refs->next->adobe_enc!=-1 ) {
	    r2 = refs->next;
	    r1 = refs;
	}
    }
    if ( r2==NULL ) {
	r2 = &space;
	memset(r2,'\0',sizeof(space));
	space.adobe_enc = ' ';
	space.transform[0] = space.transform[3] = 1.0;
	for ( i=0; i<scs[0]->parent->glyphcnt; ++i )
	    if ( scs[0]->parent->glyphs[i]!=NULL &&
		    strcmp(scs[0]->parent->glyphs[i]->name,"space")==0 )
	break;
	if ( i==scs[0]->parent->glyphcnt )
	    r2 = NULL;			/* No space???? */
	else {
	    space.sc = scs[0]->parent->glyphs[i];
	    if ( space.sc->layers[layer].splines!=NULL || space.sc->layers[layer].refs!=NULL )
		r2 = NULL;
	}
    } else if ( r2->next!=NULL )
	r2 = NULL;

    /* check for something like "AcyrillicBreve" which has a ref to Acyril */
    /*  (which doesn't have an adobe enc) which in turn has a ref to A (which */
    /*  does) */
    if ( r2!=NULL ) {
	if ( r1->adobe_enc==-1 )
	    r1 = RefFindAdobe(r1,&t1,layer);
	if ( r2->adobe_enc==-1 )
	    r2 = RefFindAdobe(r2,&t2,layer);
    }

/* CID fonts have no encodings. So we can't use seac to reference characters */
/*  in them. The other requirements are just those of seac */
    if ( r2==NULL ||
	    r1->adobe_enc==-1 ||
	    r2->adobe_enc==-1 ||
	    ((r1->transform[4]!=0 || r1->transform[5]!=0 || r1->sc->width!=scs[0]->width ) &&
		 (r2->transform[4]!=0 || r2->transform[5]!=0 || r2->sc->width!=scs[0]->width)) )
return( false );

    swap = false;
    if ( r1->transform[4]!=0 || r1->transform[5]!=0 ) {
	rt = r1; r1 = r2; r2 = rt;
	swap = !swap;
    }

    SplineCharFindBounds(r1->sc,&b);
    r1->sc->lsidebearing = myround(b.minx,round);
    SplineCharFindBounds(r2->sc,&b);
    r2->sc->lsidebearing = myround(b.minx,round);

    if ( (r1->sc->width!=scs[0]->width || r1->sc->lsidebearing!=scs[0]->lsidebearing) &&
	 r2->sc->width==scs[0]->width && r2->sc->lsidebearing==scs[0]->lsidebearing &&
	    r2->transform[4]==0 && r2->transform[5]==0 ) {
	rt = r1; r1 = r2; r2 = rt;
	swap = !swap;
    }
    if ( r1->sc->width!=scs[0]->width || r1->sc->lsidebearing!=scs[0]->lsidebearing ||
	 r1->transform[4]!=0 || r1->transform[5]!=0 )
return( false );

    for ( j=0; j<instance_count; ++j ) {
	SplineChar *r2sc = scs[j]->parent->glyphs[r2->sc->orig_pos];
	RefChar *r3, t3;

	SplineCharFindBounds(r2sc,&b);
	if ( scs[j]->layers[layer].refs!=NULL && scs[j]->layers[layer].refs->next==NULL )
	    r3 = r2;		/* Space, not offset */
	else if ( swap )
	    r3 = RefFindAdobe(scs[j]->layers[layer].refs,&t3,layer);
	else
	    r3 = RefFindAdobe(scs[j]->layers[layer].refs->next,&t3,layer);

	b.minx = myround(b.minx,round);
	data[j][0] = b.minx;
	data[j][1] = r3->transform[4] + b.minx-scs[j]->lsidebearing;
	data[j][2] = r3->transform[5];
    }
    AddData(gb,data,instance_count,3,round);
    AddNumber(gb,r1->adobe_enc,round);
    AddNumber(gb,r2->adobe_enc,round);
    if ( gb->pt+2>gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 12;
    *(gb->pt)++ = 6;			/* seac 12,6 */

return( true );
}

static int _SCNeedsSubsPts(SplineChar *sc,int layer) {
    RefChar *ref;

    if ( sc->hstem==NULL && sc->vstem==NULL )
return( false );

    if ( sc->layers[layer].splines!=NULL )
return( sc->layers[layer].splines->first->hintmask==NULL );

    for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	if ( ref->layers[0].splines!=NULL )
return( ref->layers[0].splines->first->hintmask==NULL );

return( false );		/* It's empty. that's easy. */
}

static int SCNeedsSubsPts(SplineChar *sc,enum fontformat format,int layer) {
    if ( (format!=ff_mma && format!=ff_mmb) || sc->parent->mm==NULL ) {
	if ( !sc->hconflicts && !sc->vconflicts )
return( false );		/* No conflicts, no swap-over points needed */
return( _SCNeedsSubsPts(sc,layer));
    } else {
	MMSet *mm = sc->parent->mm;
	int i;
	for ( i=0; i<mm->instance_count; ++i ) if ( sc->orig_pos<mm->instances[i]->glyphcnt ) {
	    if ( _SCNeedsSubsPts(mm->instances[i]->glyphs[sc->orig_pos],layer) )
return( true );
	}
return( false );
    }
}

static void ExpandRef1(GrowBuf *gb, SplineChar *scs[MmMax], int instance_count,
	struct hintdb *hdb, RefChar *r[MmMax], BasePoint trans[MmMax],
	DBasePoint current[MmMax],
	struct pschars *subrs, int round, int iscjk, int layer) {
    BasePoint *bpt;
    BasePoint rtrans[MmMax], rpos[MmMax];
    int i;

    for ( i=0; i<instance_count; ++i ) {
	rtrans[i].x = r[i]->transform[4]+trans[i].x;
	rtrans[i].y = r[i]->transform[5]+trans[i].y;
	if ( round ) {
	    rtrans[i].x = rint(rtrans[i].x);
	    rtrans[i].y = rint(rtrans[i].y);
	}
    }

    BreakSubroutine(gb,hdb);
    if ( r[0]->sc == scs[0] ) {
	/* Hints for self */
	if ( hdb->cnt>0 && !hdb->noconflicts && NeverConflicts(r,instance_count)) {
	    CvtPsHints(gb,scs,instance_count,true,round,iscjk,NULL);
	    CvtPsHints(gb,scs,instance_count,false,round,iscjk,NULL);
	}
    } else {
	/* Hints for a real reference */
	if ( !NeverConflicts(r,instance_count) || r[0]->sc->layers[layer].anyflexes || AnyRefs(r[0]->sc,layer) )
	    /* Hints already done */;
	else if ( hdb->noconflicts )
	    /* Hints already done */;
	else if ( r[0]->sc->hstem!=NULL || r[0]->sc->vstem!=NULL )
	    CallTransformedHintSubr(gb,hdb,scs,r,trans,instance_count,round);
    }

    if ( hdb!=NULL && hdb->gi!=NULL )
	bpt = hdb->gi->psubrs[r[0]->sc->ttf_glyph].startstop;
    else
	bpt = (BasePoint *) (subrs->keys[r[0]->sc->ttf_glyph]);
    for ( i=0; i<instance_count; ++i ) {
	rpos[i].x = bpt[2*i].x + rtrans[i].x;
	rpos[i].y = bpt[2*i].y + rtrans[i].y;
    }
    refmoveto(gb,current,rpos, instance_count,false,round,hdb,NULL);
    hdb->startset = true;

    if ( hdb!=NULL && hdb->gi!=NULL ) {
	GlyphInfo *gi = hdb->gi;
	StartNextSubroutine(gb,hdb);
	gi->bits[gi->bcnt].psub_index = r[0]->sc->ttf_glyph;
	++gi->bcnt;
	gi->justbroken = true;
    } else {
	AddNumber(gb,r[0]->sc->ttf_glyph,round);
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*gb->pt++ = 10;
    }

    for ( i=0; i<instance_count; ++i ) {
	current[i].x = bpt[2*i+1].x + rtrans[i].x;
	current[i].y = bpt[2*i+1].y + rtrans[i].y;
    }
}

static void RSC2PS1(GrowBuf *gb, SplineChar *base[MmMax],SplineChar *rsc[MmMax],
	struct hintdb *hdb, BasePoint *trans, struct pschars *subrs,
	DBasePoint current[MmMax], int flags, int iscjk,
	int instance_count, int layer ) {
    BasePoint subtrans[MmMax];
    SplineChar *rscs[MmMax];
    int round = (flags&ps_flag_round)? true : false;
    RefChar *refs[MmMax];
    SplineSet *spls[MmMax], *freeme[MmMax];
    int i;
    int wasntconflicted = hdb->noconflicts;

    for ( i=0; i<instance_count; ++i ) {
	spls[i] = rsc[i]->layers[layer].splines;
	if ( base[0]!=rsc[0] )
	    spls[i] = freeme[i] = SPLCopyTranslatedHintMasks(spls[i],base[i],rsc[i],&trans[i]);
    }
    _CvtPsSplineSet(gb,spls,instance_count,current,round,hdb,
	    base[0]->layers[layer].order2,base[0]->parent->strokedfont);
    if ( base[0]!=rsc[0] )
	for ( i=0; i<instance_count; ++i )
	    SplinePointListsFree(freeme[i]);

    for ( i=0; i<instance_count; ++i )
	refs[i] = rsc[i]->layers[layer].refs;
    while ( refs[0]!=NULL ) {
	for ( i=0; i<instance_count; ++i )
	    spls[i] = refs[i]->layers[0].splines;
	if ( !refs[0]->justtranslated ) {
	    for ( i=0; i<instance_count; ++i )
		spls[i] = freeme[i] = SPLCopyTransformedHintMasks(refs[i],base[i],&trans[i],layer);
	    if ( NeverConflicts(refs,instance_count) && !hdb->noconflicts &&
		    refs[0]->transform[1]==0 && refs[0]->transform[2]==0 )
		CallTransformedHintSubr(gb,hdb,base,refs,trans,instance_count,round);
	    _CvtPsSplineSet(gb,spls,instance_count,current,round,hdb,
		    base[0]->layers[layer].order2,base[0]->parent->strokedfont);
	    for ( i=0; i<instance_count; ++i )
		SplinePointListsFree(freeme[i]);
	} else if ( refs[0]->sc->ttf_glyph!=0x7fff &&
		((flags&ps_flag_nohints) ||
		 !refs[0]->sc->layers[layer].anyflexes ||
		 (refs[0]->transform[4]+trans[0].x==0 && refs[0]->transform[5]+trans[0].y==0)) &&
		((flags&ps_flag_nohints) ||
		 NeverConflicts(refs,instance_count) ||
		 AllStationary(refs,trans,instance_count)) ) {
	    ExpandRef1(gb,base,instance_count,hdb,refs,trans,
		    current,subrs,round,iscjk,layer);
	} else {
	    for ( i=0; i<instance_count; ++i ) {
		subtrans[i].x = trans[i].x + refs[i]->transform[4];
		subtrans[i].y = trans[i].y + refs[i]->transform[5];
		rscs[i] = refs[i]->sc;
	    }
	    if ( !hdb->noconflicts && NeverConflicts(refs,instance_count)) {
		CallTransformedHintSubr(gb,hdb,base,refs,trans,instance_count,round);
		hdb->noconflicts = true;
	    }
	    RSC2PS1(gb,base,rscs,hdb,subtrans,subrs,current,flags,iscjk,
		    instance_count,layer);
	    hdb->noconflicts = wasntconflicted;
	}
	for ( i=0; i<instance_count; ++i )
	    refs[i] = refs[i]->next;
    }
}

static unsigned char *SplineChar2PS(SplineChar *sc,int *len,int round,int iscjk,
	struct pschars *subrs,int flags,enum fontformat format,
	GlyphInfo *gi) {
    DBounds b;
    GrowBuf gb;
    DBasePoint current[MmMax];
    unsigned char *ret;
    struct hintdb hintdb, *hdb=NULL;
    StemInfo *oldh[MmMax], *oldv[MmMax];
    int hc[MmMax], vc[MmMax];
    BasePoint trans[MmMax];
    int instance_count, i;
    SplineChar *scs[MmMax];
    bigreal data[MmMax][6];
    MMSet *mm = sc->parent->mm;
    HintMask *hm[MmMax];
    int fixuphm = false;

    if ( !(flags&ps_flag_nohints) && SCNeedsSubsPts(sc,format,gi->layer))
	SCFigureHintMasks(sc,gi->layer);

    if ( (format==ff_mma || format==ff_mmb) && mm!=NULL ) {
	instance_count = mm->instance_count;
	if ( instance_count>16 )
	    instance_count = 16;
	for ( i=0; i<instance_count; ++i )
	    scs[i] = mm->instances[i]->glyphs[sc->orig_pos];
    } else {
	instance_count = 1;
	scs[0] = sc;
	mm = NULL;
    }

    if ( flags&ps_flag_nohints ) {
	for ( i=0; i<instance_count; ++i ) {
	    oldh[i] = scs[i]->hstem; oldv[i] = scs[i]->vstem;
	    hc[i] = scs[i]->hconflicts; vc[i] = scs[i]->vconflicts;
	    scs[i]->hstem = NULL; scs[i]->vstem = NULL;
	    scs[i]->hconflicts = false; scs[i]->vconflicts = false;
	}
    } else {
	for ( i=0; i<instance_count; ++i )
	    if ( scs[i]->vconflicts || scs[i]->hconflicts )
	break;
	if ( scs[0]->layers[gi->layer].splines!=NULL && i==instance_count ) {	/* No conflicts */
	    fixuphm = true;
	    for ( i=0; i<instance_count; ++i ) {
		hm[i] = scs[i]->layers[gi->layer].splines->first->hintmask;
		scs[i]->layers[gi->layer].splines->first->hintmask = NULL;
	    }
	}
    }

    memset(&gb,'\0',sizeof(gb));
    memset(current,'\0',sizeof(current));
    for ( i=0; i<instance_count; ++i ) {
	SplineCharFindBounds(scs[i],&b);
	scs[i]->lsidebearing = current[i].x = myround(b.minx,round);
	data[i][0] = current[i].x;
	data[i][1] = scs[i]->width;
    }
    AddData(&gb,data,instance_count,2,round);
    *gb.pt++ = 13;				/* hsbw, lbearing & width */

    memset(&hintdb,0,sizeof(hintdb));
    hintdb.subrs = subrs; hintdb.iscjk = iscjk&~0x100; hintdb.scs = scs;
    hintdb.instance_count = instance_count;
    hintdb.cnt = NumberHints(scs,instance_count);
    hintdb.noconflicts = true;
    hintdb.gi = gi;
    for ( i=0; i<instance_count; ++i )
	if ( scs[i]->hconflicts || scs[i]->vconflicts )
	    hintdb.noconflicts = false;
    hdb = &hintdb;
    if ( gi!=NULL )
	gi->bcnt = -1;

    /* If this char is being placed in a subroutine, then we don't want to */
    /*  use seac because somebody is going to call that subroutine and */
    /*  add another reference to it later. CID keyed fonts also can't use */
    /*  seac (they have no encoding so it doesn't work), that's what iscjk&0x100 */
    /*  tests for */
    if ( scs[0]->ttf_glyph==0x7fff && !(iscjk&0x100) && !(flags&ps_flag_noseac) &&
	    IsSeacable(&gb,scs,instance_count,round,gi->layer)) {
	if ( gi )
	    gi->active->wasseac = true;
	/* in MM fonts, all should share the same refs, so all should be */
	/* seac-able if one is */
    } else {
	iscjk &= ~0x100;
	if ( iscjk && instance_count==1 )
	    CounterHints1(&gb,sc,round);	/* Must come immediately after hsbw */
	if ( hintdb.noconflicts ) {
	    CvtPsHints(&gb,scs,instance_count,true,round,iscjk,NULL);
	    CvtPsHints(&gb,scs,instance_count,false,round,iscjk,NULL);
	}
	memset(&trans,0,sizeof(trans));
	RSC2PS1(&gb,scs,scs,hdb,trans,subrs,current,flags,iscjk,
		instance_count,gi->layer);
    }
    if ( gi->bcnt==-1 ) {	/* If it's whitespace */
	gi->bcnt = 0;
	StartNextSubroutine(&gb,hdb);
    }
    BreakSubroutine(&gb,hdb);
    MoveSubrsToChar(gi);
    ret = NULL;

    if ( hdb!=NULL ) {
	struct mhlist *mh, *mhnext;
	for ( mh=hdb->sublist; mh!=NULL; mh=mhnext ) {
	    mhnext = mh->next;
	    free(mh);
	}
    }
    free(gb.base);
    if ( flags&ps_flag_nohints ) {
	for ( i=0; i<instance_count; ++i ) {
	    scs[i]->hstem = oldh[i]; scs[i]->vstem = oldv[i];
	    scs[i]->hconflicts = hc[i]; scs[i]->vconflicts = vc[i];
	}
    } else if ( fixuphm ) {
	for ( i=0; i<instance_count; ++i )
	    scs[i]->layers[gi->layer].splines->first->hintmask = hm[i];
    }
return( ret );
}

#ifdef FONTFORGE_CONFIG_PS_REFS_GET_SUBRS
static int AlwaysSeacable(SplineChar *sc,int flags) {
    struct splinecharlist *d;
    RefChar *r;

    if ( sc->parent->cidmaster!=NULL )	/* Can't use seac in CID fonts, no encoding */
return( false );
    if ( flags&ps_flag_noseac )
return( false );

    for ( d=sc->dependents; d!=NULL; d = d->next ) {
	if ( d->sc->layers[layer].splines!=NULL )	/* I won't deal with things with both splines and refs. */
    continue;				/*  skip it */
	for ( r=d->sc->layers[layer].refs; r!=NULL; r=r->next ) {
	    if ( !r->justtranslated )
	break;				/* Can't deal with it either way */
	}
	if ( r!=NULL )		/* Bad transform matrix */
    continue;			/* Can't handle either way, skip */

	for ( r=d->sc->layers[layer].refs; r!=NULL; r=r->next ) {
	    if ( r->adobe_enc==-1 )
return( false );			/* not seacable, but could go in subr */
	}
	r = d->sc->layers[layer].refs;
	if ( r->next!=NULL && r->next->next!=NULL )
return( false );		/* seac only takes 2 glyphs */
	if ( r->next!=NULL &&
		((r->transform[4]!=0 || r->transform[5]!=0 || r->sc->width!=d->sc->width) &&
		 (r->next->transform[4]!=0 || r->next->transform[5]!=0 || r->next->sc->width!=d->sc->width)))
return( false );		/* seac only allows one to be translated, and the untranslated one must have the right width */
	if ( r->next==NULL &&
		(r->transform[4]!=0 || r->transform[5]!=0 || r->sc->width!=d->sc->width))
return( false );
    }
    /* Either always can be represented by seac, or sometimes by neither */
return( true );
}

/* normally we can't put a character with hint conflicts into a subroutine */
/*  (because when we would have to invoke the hints within the subr and */
/*   hints are expressed as absolute positions, so if the char has been */
/*   translated we can't do the hints right). BUT if the character is not */
/*  translated, and if it has the right lbearing, then the hints in the */
/*  ref will match those in the character and we can use a subroutine for */
/*  both */
/* If at least one ref fits our requirements then return true */
/* The same reasoning applies to flex hints. There are absolute expressions */
/*  in them too. */
static int SpecialCaseConflicts(SplineChar *sc) {
    struct splinecharlist *d;
    RefChar *r;
    DBounds sb, db;

    SplineCharFindBounds(sc,&sb);
    for ( d=sc->dependents; d!=NULL; d = d->next ) {
	SplineCharFindBounds(d->sc,&db);
	if ( db.minx != sb.minx )
    continue;
	for ( r=d->sc->layers[layer].refs; r!=NULL; r=r->next )
	    if ( r->sc == sc && r->justtranslated &&
		    r->transform[4]==0 && r->transform[5]==0 )
return( true );
    }
return( false );
}

static BasePoint *FigureStartStop(SplineChar *sc, GlyphInfo *gi ) {
    int m, didfirst;
    SplineChar *msc;
    SplineSet *spl;
    RefChar *r;
    BasePoint *startstop;

    /* We need to know the location of the first point on the */
    /*  first path (need to rmoveto it, and the location of the */
    /*  last point on the last path (will need to move from it */
    /*  for the next component) */

    startstop = calloc(2*gi->instance_count,sizeof(BasePoint));
    for ( m=0; m<gi->instance_count; ++m ) {
	if ( gi->instance_count==1 || sc->parent->mm==NULL )
	    msc = sc;
	else
	    msc = sc->parent->mm->instances[m]->glyphs[sc->orig_pos];
	didfirst = false;
	spl = msc->layers[layer].splines;
	if ( spl!=NULL ) {
	    startstop[0] = spl->first->me;
	    didfirst = true;
	    while ( spl!=NULL ) {
		/* Closepath does NOT set the current point */
		/* Remember we reverse PostScript */
		if ( spl->last==spl->first && spl->first->next!=NULL &&
			spl->first->next->knownlinear )
		    startstop[1] = spl->first->next->to->me;
		else
		    startstop[1] = spl->last->me;
		spl = spl->next;
	    }
	}
	for ( r=msc->layers[layer].refs; r!=NULL; r=r->next ) {
	    spl = r->layers[0].splines;
	    if ( spl!=NULL ) {
		if ( !didfirst )
		    startstop[0] = spl->first->me;
		didfirst = true;
	    }
	    while ( spl!=NULL ) {
		/* Closepath does NOT set the current point */
		/* Remember we reverse PostScript */
		if ( spl->last==spl->first && spl->first->next!=NULL &&
			spl->first->next->knownlinear )
		    startstop[1] = spl->first->next->to->me;
		else
		    startstop[1] = spl->last->me;
		spl = spl->next;
	    }
	}
    }
return( startstop );
}
#endif	/* FONTFORGE_CONFIG_PS_REFS_GET_SUBRS */

/* Mark those glyphs which can live totally in subrs */
static void SplineFont2FullSubrs1(int flags,GlyphInfo *gi) {
    int i;
    SplineChar *sc;
#ifdef FONTFORGE_CONFIG_PS_REFS_GET_SUBRS
    int anydone, cc;
    struct potentialsubrs *ps;
    SplineSet *spl;
    RefChar *r;
#endif	/* FONTFORGE_CONFIG_PS_REFS_GET_SUBRS */

    if ( !autohint_before_generate && !(flags&ps_flag_nohints))
	SplineFontAutoHintRefs(gi->sf,gi->layer);
    for ( i=0; i<gi->glyphcnt; ++i ) if ( (sc=gi->gb[i].sc)!=NULL )
	sc->ttf_glyph = 0x7fff;

#ifdef FONTFORGE_CONFIG_PS_REFS_GET_SUBRS
    anydone = true;
    while ( anydone ) {
	anydone = false;
	for ( i=0; i<gi->glyphcnt; ++i ) if ( (sc=gi->gb[i].sc)!=NULL ) {
	    if ( !SCWorthOutputting(sc) || sc->ttf_glyph!=0x7fff )
	continue;
	    /* if the glyph is a single contour with no hintmasks then */
	    /*  our single contour code will find it. If we do it here too */
	    /*  we'll get a subr which points to another subr. Very dull and */
	    /*  a waste of space */
	    cc = 0;
	    for ( spl=sc->layers[layer].splines; spl!=NULL; spl=spl->next )
		++cc;
	    for ( r= sc->layers[layer].refs; r!=NULL && cc<2 ; r=r->next ) {
		for ( spl=r->layers[0].splines; spl!=NULL; spl=spl->next )
		    ++cc;
	    }
	    if ( cc<2 )
	continue;
	    /* Put the */
	    /*  character into a subr if it is referenced by other characters */
	    if ( (sc->dependents!=NULL &&
		     ((!sc->hconflicts && !sc->vconflicts && !sc->layers[gi->layer].anyflexes) ||
			 SpecialCaseConflicts(sc)) &&
		     !AlwaysSeacable(sc,flags))) {
		RefChar *r;

		for ( r=sc->layers[layer].refs; r!=NULL; r=r->next )
		    if ( r->sc->ttf_glyph==0x7fff )
		break;
		if ( r!=NULL )	/* Contains a reference to something which is */
	continue;		/* not in a sub itself. Skip it for now, we'll*/
				/* come back to it next pass when perhaps the */
			        /* reference will be nicely ensconsed itself */
		if ( gi->pcnt>=gi->pmax )
		    gi->psubrs = realloc(gi->psubrs,(gi->pmax+=gi->glyphcnt)*sizeof(struct potentialsubrs));
		ps = &gi->psubrs[gi->pcnt];
		memset(ps,0,sizeof(*ps));	/* set cnt to 0 */
		ps->idx = gi->pcnt++;
		ps->full_glyph_index = i;
		sc->ttf_glyph = gi->pcnt-1;
		ps->startstop = FigureStartStop(sc,gi);
		anydone = true;
	    }
	}
    }
#endif	/* FONTFORGE_CONFIG_PS_REFS_GET_SUBRS */
}

int SFOneWidth(SplineFont *sf) {
    int width, i;

    width = -2;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i]) &&
	    (strcmp(sf->glyphs[i]->name,".notdef")!=0 || sf->glyphs[i]->layers[ly_fore].splines!=NULL)) {
	/* Only trust the width of notdef if it's got some content */
	/* (at least as far as fixed pitch determination goes) */
	if ( width==-2 ) width = sf->glyphs[i]->width;
	else if ( width!=sf->glyphs[i]->width ) {
	    width = -1;
    break;
	}
    }
return(width);
}

int CIDOneWidth(SplineFont *_sf) {
    int width, i;
    int k;
    SplineFont *sf;

    if ( _sf->cidmaster!=NULL ) _sf = _sf->cidmaster;
    width = -2;
    k=0;
    do {
	sf = _sf->subfonts==NULL? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i]) &&
		strcmp(sf->glyphs[i]->name,".null")!=0 &&
		strcmp(sf->glyphs[i]->name,"nonmarkingreturn")!=0 &&
		(strcmp(sf->glyphs[i]->name,".notdef")!=0 || sf->glyphs[i]->layers[ly_fore].splines!=NULL)) {
	    /* Only trust the width of notdef if it's got some content */
	    /* (at least as far as fixed pitch determination goes) */
	    if ( width==-2 ) width = sf->glyphs[i]->width;
	    else if ( width!=sf->glyphs[i]->width ) {
		width = -1;
	break;
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
return(width);
}

int SFOneHeight(SplineFont *sf) {
    int width, i;

    if ( !sf->hasvmetrics )
return( sf->ascent+sf->descent );

    width = -2;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i]) &&
	    (strcmp(sf->glyphs[i]->name,".notdef")!=0 || sf->glyphs[i]->layers[ly_fore].splines!=NULL)) {
	/* Only trust the width of notdef if it's got some content */
	/* (at least as far as fixed pitch determination goes) */
	if ( width==-2 ) width = sf->glyphs[i]->vwidth;
	else if ( width!=sf->glyphs[i]->vwidth ) {
	    width = -1;
    break;
	}
    }
return(width);
}

int SFIsCJK(SplineFont *sf,EncMap *map) {
    char *val;

    if ( (val = PSDictHasEntry(sf->private,"LanguageGroup"))!=NULL )
return( strtol(val,NULL,10));

    if ( map->enc->is_japanese || map->enc->is_korean ||
	    map->enc->is_tradchinese || map->enc->is_simplechinese )
return( true );
    if ( (map->enc->is_unicodebmp || map->enc->is_unicodefull) &&
	    sf->glyphcnt>0x3000 &&
	    SCWorthOutputting(sf->glyphs[0x3000]) &&
	    !SCWorthOutputting(sf->glyphs['A']) )
return( true );
    if ( map->enc==&custom ) {
	/* If it's in a CID font and it doesn't contain alphabetics, then */
	/*  it's assumed to be CJK */
	if ( sf->cidmaster!=NULL )
return( !SCWorthOutputting(SFGetChar(sf,'A',NULL)) &&
	!SCWorthOutputting(SFGetChar(sf,0x391,NULL)) &&		/* Alpha */
	!SCWorthOutputting(SFGetChar(sf,0x410,NULL)) &&		/* Cyrillic A */
	!SCWorthOutputting(SFGetChar(sf,-1,"uni0041.hw")) );	/* Halfwidth A, non standard name, from my cidmap */
    }

return( false );
}

static void SetupType1Subrs(struct pschars *subrs,GlyphInfo *gi) {
    int scnt, call_size;
    int i;

    scnt = subrs->next;
    call_size = gi->pcnt+scnt<1131 ? 3 : 6;
    for ( i=0; i<gi->pcnt; ++i ) {
	/* A subroutine call takes somewhere between 2 and 6 bytes itself. */
	/*  and we must add a return statement to the end. We don't want to */
	/*  make things bigger */
	if ( gi->psubrs[i].full_glyph_index!=-1 )
	    gi->psubrs[i].idx = scnt++;
	else if ( gi->psubrs[i].cnt*gi->psubrs[i].len>(gi->psubrs[i].cnt*call_size)+gi->psubrs[i].len+1 )
	    gi->psubrs[i].idx = scnt++;
	else
	    gi->psubrs[i].idx = -1;
    }

    subrs->cnt = scnt;
    subrs->next = scnt;
    subrs->lens = realloc(subrs->lens,scnt*sizeof(int));
    subrs->values = realloc(subrs->values,scnt*sizeof(unsigned char *));

    for ( i=0; i<gi->pcnt; ++i ) {
	scnt = gi->psubrs[i].idx;
	if ( scnt==-1 || gi->psubrs[i].full_glyph_index != -1 )
    continue;
	subrs->lens[scnt] = gi->psubrs[i].len+1;
	subrs->values[scnt] = malloc(subrs->lens[scnt]);
	memcpy(subrs->values[scnt],gi->psubrs[i].data,gi->psubrs[i].len);
	subrs->values[scnt][gi->psubrs[i].len] = 11;	/* Add a return to end of subr */
    }
}

static void SetupType1Chrs(struct pschars *chrs,struct pschars *subrs,GlyphInfo *gi, int iscid) {
    int i,k,j;

    /* If a glyph lives entirely in a subroutine then we need to create both */
    /*  the subroutine entry, and the char entry which calls the subr. */
    /* The subroutine entry will be everything EXCEPT the glyph header */
    /* the char entry will be the glyph header and a subroutine call */
    /* If the glyph does not go into a subr then everything goes into the char */
    for ( i=0; i<gi->glyphcnt; ++i ) {
	int len=0;
	struct glyphbits *gb = &gi->gb[i];
	if ( gb->sc==NULL )
    continue;
	if ( !iscid )
	    chrs->keys[i] = copy(gb->sc->name);
	for ( k=0; k<2; ++k ) if ( k!=0 || gb->sc->ttf_glyph!=0x7fff ) {
	    uint8 *vals;
	    for ( j=0; j<gb->bcnt; ++j ) {
		if ( k!=0 || j!=0 )
		    len += gb->bits[j].dlen;
		if ( k==1 && gb->sc->ttf_glyph!=0x7fff ) {
		    int si = gi->psubrs[ gb->sc->ttf_glyph ].idx;
		    len += 1 + (si<=107?1:si<=1131?2:5);
	    break;
		}
		if ( gi->psubrs[ gb->bits[j].psub_index ].idx==-1 )
		    len += gi->psubrs[ gb->bits[j].psub_index ].len;
		else {
		    int si = gi->psubrs[ gb->bits[j].psub_index ].idx;
		    len += 1 + (si<=107?1:si<=1131?2:5);
		    /* Space for a subr call & the sub number to call */
		}
	    }
	    if ( k==0 ) {
		int si = gi->psubrs[ gb->sc->ttf_glyph ].idx;
		subrs->lens[si] = len+1;
		vals = subrs->values[si] = malloc(len+2);
	    } else {
		/* Don't need or want and endchar if we are using seac */
		chrs->lens[i] = len + !gb->wasseac;
		vals = chrs->values[i] = malloc(len+2); /* space for endchar and a final NUL (which is really meaningless, but makes me feel better) */
	    }

	    len = 0;
	    for ( j=0; j<gb->bcnt; ++j ) {
		int si;
		if ( k!=0 || j!=0 ) {
		    memcpy(vals+len,gb->bits[j].data,gb->bits[j].dlen);
		    len += gb->bits[j].dlen;
		}
		si = -1;
		if ( k==1 && gb->sc->ttf_glyph!=0x7fff )
		    si = gi->psubrs[ gb->sc->ttf_glyph ].idx;
		else if ( gi->psubrs[ gb->bits[j].psub_index ].idx==-1 ) {
		    memcpy(vals+len,gi->psubrs[ gb->bits[j].psub_index ].data,
			    gi->psubrs[ gb->bits[j].psub_index ].len);
		    len += gi->psubrs[ gb->bits[j].psub_index ].len;
		} else
		    si = gi->psubrs[ gb->bits[j].psub_index ].idx;
		if ( si!=-1 ) {
		    /* space for the number (subroutine index) */
		    if ( si<=107 )
			vals[len++] = si+139;
		    else if ( si>0 && si<=1131 ) {
			si-=108;
			vals[len++] = (si>>8)+247;
			vals[len++] = si&0xff;
		    } else {
			vals[len++] = '\377';
			vals[len++] = (si>>24)&0xff;
			vals[len++] = (si>>16)&0xff;
			vals[len++] = (si>>8)&0xff;
			vals[len++] = si&0xff;
		    }
		    /* space for the subroutine operator */
		    vals[len++] = 10;
		}
		if ( k==1 && gb->sc->ttf_glyph!=0x7fff )
	    break;
	    }
	    if ( k==0 ) {
		vals[len++] = 11;	/* return */
		vals[len] = '\0';
	    } else if ( gb->wasseac ) {
		/* Don't want an endchar */
		vals[len] = '\0';
	    } else {
		vals[len++] = 14;	/* endchar */
		vals[len] = '\0';
	    }
	}
    }
}

struct pschars *SplineFont2ChrsSubrs(SplineFont *sf, int iscjk,
	struct pschars *subrs,int flags, enum fontformat format, int layer) {
    struct pschars *chrs = calloc(1,sizeof(struct pschars));
    int i, cnt, instance_count;
    int fixed;
    int notdef_pos;
    MMSet *mm = sf->mm;
    int round = (flags&ps_flag_round)? true : false;
    GlyphInfo gi;
    SplineChar dummynotdef, *sc;

    if ( (format==ff_mma || format==ff_mmb) && mm!=NULL ) {
	instance_count = mm->instance_count;
	sf = mm->instances[0];
	fixed = 0;
	for ( i=0; i<instance_count; ++i ) {
	    MarkTranslationRefs(mm->instances[i],layer);
	    fixed = SFOneWidth(mm->instances[i]);
	    if ( fixed==-1 )
	break;
	}
    } else {
	MarkTranslationRefs(sf,layer);
	fixed = SFOneWidth(sf);
	instance_count = 1;
    }

    notdef_pos = SFFindNotdef(sf,fixed);
    cnt = 0;
    for ( i=0; i<sf->glyphcnt; ++i )
#if HANYANG
	if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->compositionunit )
	    /* Don't count it */;
	else
#endif
	if ( SCWorthOutputting(sf->glyphs[i]) &&
		( i==notdef_pos || strcmp(sf->glyphs[i]->name,".notdef")!=0))
	    ++cnt;
/* only honor the width on .notdef in non-fixed pitch fonts (or ones where there is an actual outline in notdef) */
    if ( notdef_pos==-1 )
	++cnt;		/* one notdef entry */

    memset(&gi,0,sizeof(gi));
    memset(&gi.hashed,-1,sizeof(gi.hashed));
    gi.instance_count = 1;
    gi.sf = sf;
    gi.layer = layer;
    gi.glyphcnt = cnt;
    gi.gb = calloc(cnt,sizeof(struct glyphbits));
    gi.pmax = 3*cnt;
    gi.psubrs = malloc(gi.pmax*sizeof(struct potentialsubrs));
    gi.instance_count = instance_count;

    if ( notdef_pos==-1 ) {
	memset(&dummynotdef,0,sizeof(dummynotdef));
	dummynotdef.name = ".notdef";
	dummynotdef.parent = sf;
	dummynotdef.layer_cnt = sf->layer_cnt;
	dummynotdef.layers = calloc(sf->layer_cnt,sizeof(Layer));
	dummynotdef.width = SFOneWidth(sf);
	if ( dummynotdef.width==-1 )
	    dummynotdef.width = (sf->ascent+sf->descent)/2;
	gi.gb[0].sc = &dummynotdef;
    } else
	gi.gb[0].sc = sf->glyphs[notdef_pos];
    cnt = 1;
    for ( i=0 ; i<sf->glyphcnt; ++i ) {
#if HANYANG
	if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->compositionunit )
	    /* don't output it, should be in a subroutine */;
	else
#endif
	if ( SCWorthOutputting(sf->glyphs[i]) &&
		strcmp(sf->glyphs[i]->name,".notdef")!=0)	/* We've already added .notdef */
	    gi.gb[cnt++].sc = sf->glyphs[i];
    }

    SplineFont2FullSubrs1(flags,&gi);

    for ( i=0; i<cnt; ++i ) {
	if ( (sc = gi.gb[i].sc)==NULL )
    continue;
	gi.active = &gi.gb[i];
	SplineChar2PS(sc,NULL, round,iscjk,subrs,flags,format,&gi);
	if ( !ff_progress_next()) {
	    PSCharsFree(chrs);
	    GIFree(&gi,&dummynotdef);
return( NULL );
	}
    }

    SetupType1Subrs(subrs,&gi);

    chrs->cnt = cnt;
    chrs->keys = malloc(cnt*sizeof(char *));
    chrs->lens = malloc(cnt*sizeof(int));
    chrs->values = malloc(cnt*sizeof(unsigned char *));

    SetupType1Chrs(chrs,subrs,&gi,false);

    GIFree(&gi,&dummynotdef);

    chrs->next = cnt;
    if ( chrs->next>chrs->cnt )
	IError("Character estimate failed, about to die..." );
return( chrs );
}

struct pschars *CID2ChrsSubrs(SplineFont *cidmaster,struct cidbytes *cidbytes,int flags,int layer) {
    struct pschars *chrs = calloc(1,sizeof(struct pschars));
    int i, cnt, cid;
    SplineFont *sf = NULL;
    struct fddata *fd;
    int round = (flags&ps_flag_round)? true : false;
    /* I don't support mm cid files. I don't think adobe does either */
    GlyphInfo gi;
    int notdef_subfont;
    SplineChar dummynotdef, *sc;

    cnt = 0; notdef_subfont = -1;
    for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	if ( cnt<cidmaster->subfonts[i]->glyphcnt )
	    cnt = cidmaster->subfonts[i]->glyphcnt;
	if ( cidmaster->subfonts[i]->glyphcnt>0 &&
		SCWorthOutputting(cidmaster->subfonts[i]->glyphs[0]) )
	    notdef_subfont = i;
    }
    cidbytes->cidcnt = cnt;

    if ( notdef_subfont==-1 ) {
	memset(&dummynotdef,0,sizeof(dummynotdef));
	dummynotdef.name = ".notdef";
	dummynotdef.parent = cidmaster->subfonts[0];
	dummynotdef.layer_cnt = layer+1;
	dummynotdef.layers = calloc(layer+1,sizeof(Layer));;
	dummynotdef.width = SFOneWidth(dummynotdef.parent);
	if ( dummynotdef.width==-1 )
	    dummynotdef.width = (dummynotdef.parent->ascent+dummynotdef.parent->descent);
    }

    memset(&gi,0,sizeof(gi));
    gi.instance_count = 1;
    gi.glyphcnt = cnt;
    gi.gb = malloc(cnt*sizeof(struct glyphbits));
    gi.pmax = 3*cnt;
    gi.psubrs = malloc(gi.pmax*sizeof(struct potentialsubrs));
    gi.layer = layer;

    chrs->cnt = cnt;
    chrs->lens = calloc(cnt,sizeof(int));
    chrs->values = calloc(cnt,sizeof(unsigned char *));
    cidbytes->fdind = malloc(cnt*sizeof(unsigned char *));
    memset(cidbytes->fdind,-1,cnt*sizeof(unsigned char *));

    /* In a type1 CID-keyed font we must handle subroutines subfont by subfont*/
    /*  as there are no global subrs */
    for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	gi.sf = sf = cidmaster->subfonts[i];
	MarkTranslationRefs(sf,layer);
	fd = &cidbytes->fds[i];
	memset(&gi.hashed,-1,sizeof(gi.hashed));
	gi.instance_count = 1;
	gi.glyphcnt = sf->glyphcnt;
	memset(gi.gb,0,sf->glyphcnt*sizeof(struct glyphbits));
	for ( cid=0; cid<cnt && cid<sf->glyphcnt; ++cid ) {
	    if ( cid==0 && notdef_subfont==-1 && i==cidmaster->subfontcnt-1 )
		gi.gb[0].sc = &dummynotdef;
	    else if ( SCWorthOutputting(sf->glyphs[cid]) &&
		    ((i==notdef_subfont && cid==0 ) ||
		     strcmp(sf->glyphs[cid]->name,".notdef")!=0))	/* We've already added .notdef */
		gi.gb[cid].sc = sf->glyphs[cid];
	    if ( gi.gb[cid].sc!=NULL )
		cidbytes->fdind[cid] = i;
	}
	SplineFont2FullSubrs1(flags,&gi);

	for ( cid=0; cid<cnt && cid<sf->glyphcnt; ++cid ) {
	    if ( (sc = gi.gb[cid].sc)==NULL )
	continue;
	    gi.active = &gi.gb[cid];
	    SplineChar2PS(sc,NULL, round,fd->iscjk|0x100,fd->subrs,
		    flags,ff_cid,&gi);
	    if ( !ff_progress_next()) {
		PSCharsFree(chrs);
		GIFree(&gi,&dummynotdef);
return( NULL );
	    }	
	}

	SetupType1Subrs(fd->subrs,&gi);
	SetupType1Chrs(chrs,fd->subrs,&gi,true);
	GIContentsFree(&gi,&dummynotdef);
    }
    GIFree(&gi,&dummynotdef);
    chrs->next = cnt;
return( chrs );
}

/* ************************************************************************** */
/* ********************** Type2 PostScript CharStrings ********************** */
/* ************************************************************************** */

static real myround2(real pos, int round) {
    if ( round )
return( rint(pos));

return( rint(65536*pos)/65536 );
}

static void AddNumber2(GrowBuf *gb, real pos, int round) {
    int val, factor;
    unsigned char *str;

    if ( gb->pt+5>=gb->end )
	GrowBuffer(gb);

    pos = rint(65536*pos)/65536;
    if ( round )
	pos = rint(pos);

    str = gb->pt;
    if ( pos>32767.99 || pos<-32768 ) {
	/* same logic for big ints and reals */
	if ( pos>0x3fffffff || pos<-0x40000000 ) {
	    LogError( _("Number out of range: %g in type2 output (must be [-65536,65535])\n"),
		    pos );
	    if ( pos>0 ) pos = 0x3fffffff; else pos = -0x40000000;
	}
	for ( factor=2; factor<32768; factor<<=2 )
	    if ( pos/factor<32767.99 && pos/factor>-32768 )
	break;
	AddNumber2(gb,pos/factor,false);
	AddNumber2(gb,factor,false);
	if ( gb->pt+2>=gb->end )
	    GrowBuffer(gb);
	*(gb->pt++) = 0x0c;		/* Multiply operator */
	*(gb->pt++) = 0x18;
    } else if ( pos!=floor(pos )) {
	val = pos*65536;
	*str++ = '\377';
	*str++ = (val>>24)&0xff;
	*str++ = (val>>16)&0xff;
	*str++ = (val>>8)&0xff;
	*str++ = val&0xff;
    } else {
	val = rint(pos);
	if ( pos>=-107 && pos<=107 )
	    *str++ = val+139;
	else if ( pos>=108 && pos<=1131 ) {
	    val -= 108;
	    *str++ = (val>>8)+247;
	    *str++ = val&0xff;
	} else if ( pos>=-1131 && pos<=-108 ) {
	    val = -val;
	    val -= 108;
	    *str++ = (val>>8)+251;
	    *str++ = val&0xff;
	} else {
	    *str++ = 28;
	    *str++ = (val>>8)&0xff;
	    *str++ = val&0xff;
	}
    }
    gb->pt = str;
}

static void AddMask2(GrowBuf *gb,uint8 mask[12],int cnt, int oper) {
    int i;

    if ( gb->pt+1+((cnt+7)>>3)>=gb->end )
	GrowBuffer(gb);
    *gb->pt++ = oper;					/* hintmask,cntrmask */
    for ( i=0; i< ((cnt+7)>>3); ++i )
	*gb->pt++ = mask[i];
}

static void CounterHints2(GrowBuf *gb, SplineChar *sc, int hcnt) {
    int i;

    for ( i=0; i<sc->countermask_cnt; ++i )
	AddMask2(gb,sc->countermasks[i],hcnt,20);	/* cntrmask */
}

static int HintSetup2(GrowBuf *gb,struct hintdb *hdb, SplinePoint *to, int break_subr ) {

    /* We might get a point with a hintmask in a glyph with no conflicts */
    /* (ie. the initial point when we return to it at the end of the splineset*/
    /* in that case hdb->cnt will be 0 and we should ignore it */
    /* components in subroutines depend on not having any hintmasks */
    if ( to->hintmask==NULL || hdb->cnt==0 || hdb->noconflicts || hdb->skiphm )
return( false );

    if ( memcmp(hdb->mask,*to->hintmask,(hdb->cnt+7)/8)==0 )
return( false );

    if ( break_subr )
	BreakSubroutine(gb,hdb);

    AddMask2(gb,*to->hintmask,hdb->cnt,19);		/* hintmask */
    memcpy(hdb->mask,*to->hintmask,sizeof(HintMask));
    hdb->donefirsthm = true;
    if ( break_subr )
	StartNextSubroutine(gb,hdb);
return( true );
}

static void moveto2(GrowBuf *gb,struct hintdb *hdb,SplinePoint *to, int round) {
    BasePoint temp, *tom;

    if ( gb->pt+18 >= gb->end )
	GrowBuffer(gb);

    BreakSubroutine(gb,hdb);
    HintSetup2(gb,hdb,to,false);
    tom = &to->me;
    if ( round ) {
	temp.x = rint(tom->x);
	temp.y = rint(tom->y);
	tom = &temp;
    }
    if ( hdb->current.x==tom->x ) {
	AddNumber2(gb,tom->y-hdb->current.y,round);
	*(gb->pt)++ = 4;		/* v move to */
    } else if ( hdb->current.y==tom->y ) {
	AddNumber2(gb,tom->x-hdb->current.x,round);
	*(gb->pt)++ = 22;		/* h move to */
    } else {
	AddNumber2(gb,tom->x-hdb->current.x,round);
	AddNumber2(gb,tom->y-hdb->current.y,round);
	*(gb->pt)++ = 21;		/* r move to */
    }
    hdb->current.x = rint(32768*tom->x)/32768;
    hdb->current.y = rint(32768*tom->y)/32768;
    StartNextSubroutine(gb,hdb);
}

static Spline *lineto2(GrowBuf *gb,struct hintdb *hdb,Spline *spline, Spline *done, int round) {
    int cnt, hv, hvcnt;
    Spline *test, *lastgood, *lasthvgood;
    BasePoint temp1, temp2, *tom, *fromm;
    int donehm;

    lastgood = NULL;
    for ( test=spline, cnt=0; test->knownlinear && cnt<15; ) {
	++cnt;
	lastgood = test;
	test = test->to->next;
	/* it will be smaller to use a closepath operator so ignore the */
	/*  ultimate spline */
	if ( test==done || test==NULL || test->to->next==done )
    break;
    }

    HintSetup2(gb,hdb,spline->to,true);

    hv = -1; hvcnt=1; lasthvgood = NULL;
    if ( spline->from->me.x==spline->to->me.x )
	hv = 1;		/* Vertical */
    else if ( spline->from->me.y==spline->to->me.y )
	hv = 0;		/* Horizontal */
    donehm = true;
    if ( hv!=-1 ) {
	lasthvgood = spline; hvcnt = 1;
	if ( cnt!=1 ) {
	    for ( test=spline->to->next; test!=NULL ; test = test->to->next ) {
		fromm = &test->from->me;
		if ( round ) {
		    temp2.x = rint(fromm->x);
		    temp2.y = rint(fromm->y);
		    fromm = &temp2;
		}
		tom = &test->to->me;
		if ( round ) {
		    temp1.x = rint(tom->x);
		    temp1.y = rint(tom->y);
		    tom = &temp1;
		}
		if ( hv==1 && tom->y==fromm->y )
		    hv = 0;
		else if ( hv==0 && tom->x==fromm->x )
		    hv = 1;
		else
	    break;
		lasthvgood = test;
		++hvcnt;
		if ( test==lastgood )
	    break;
	    }
	}
	donehm = true;
	if ( hvcnt==cnt || hvcnt>=2 ) {
	    /* It's more efficient to do some h/v linetos */
	    for ( test=spline; ; test = test->to->next ) {
		if ( !donehm && test->to->hintmask!=NULL )
	    break;
		donehm = false;
		fromm = &test->from->me;
		if ( round ) {
		    temp2.x = rint(fromm->x);
		    temp2.y = rint(fromm->y);
		    fromm = &temp2;
		}
		tom = &test->to->me;
		if ( round ) {
		    temp1.x = rint(tom->x);
		    temp1.y = rint(tom->y);
		    tom = &temp1;
		}
		if ( fromm->x==tom->x )
		    AddNumber2(gb,tom->y-fromm->y,round);
		else
		    AddNumber2(gb,tom->x-fromm->x,round);
		hdb->current.x = rint(32768*tom->x)/32768;
		hdb->current.y = rint(32768*tom->y)/32768;
		if ( test==lasthvgood ) {
		    test = test->to->next;
	    break;
		}
	    }
	    if ( gb->pt+1 >= gb->end )
		GrowBuffer(gb);
	    *(gb->pt)++ = spline->from->me.x==spline->to->me.x? 7 : 6;
return( test );
	}
    }

    for ( test=spline; test!=NULL; test = test->to->next ) {
	if ( !donehm && test->to->hintmask!=NULL )
    break;
	donehm = false;
	fromm = &test->from->me;
	if ( round ) {
	    temp2.x = rint(fromm->x);
	    temp2.y = rint(fromm->y);
	    fromm = &temp2;
	}
	tom = &test->to->me;
	if ( round ) {
	    temp1.x = rint(tom->x);
	    temp1.y = rint(tom->y);
	    tom = &temp1;
	}
	AddNumber2(gb,tom->x-fromm->x,round);
	AddNumber2(gb,tom->y-fromm->y,round);
	hdb->current.x = rint(32768*tom->x)/32768;
	hdb->current.y = rint(32768*tom->y)/32768;
	if ( test==lastgood ) {
	    test = test->to->next;
    break;
	}
    }
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 5;		/* r line to */
return( test );
}

static Spline *curveto2(GrowBuf *gb,struct hintdb *hdb,Spline *spline, Spline *done, int round) {
    int cnt=0, hv;
    Spline *first;
    DBasePoint start;
    int donehm;

    HintSetup2(gb,hdb,spline->to,true);

    hv = -1;
    if ( hdb->current.x==myround2(spline->from->nextcp.x,round) &&
	    myround2(spline->to->prevcp.y,round)==myround2(spline->to->me.y,round) )
	hv = 1;
    else if ( hdb->current.y==myround2(spline->from->nextcp.y,round) &&
	    myround2(spline->to->prevcp.x,round)==myround2(spline->to->me.x,round) )
	hv = 0;
    donehm = true;
    if ( hv!=-1 ) {
	first = spline; start = hdb->current;
	while (
		(hv==1 && hdb->current.x==myround2(spline->from->nextcp.x,round) &&
			myround2(spline->to->prevcp.y,round)==myround2(spline->to->me.y,round) ) ||
		(hv==0 && hdb->current.y==myround2(spline->from->nextcp.y,round) &&
			myround2(spline->to->prevcp.x,round)==myround2(spline->to->me.x,round) ) ) {
	    if ( !donehm && spline->to->hintmask!=NULL )
	break;
	    donehm = false;
	    if ( hv==1 ) {
		AddNumber2(gb,myround2(spline->from->nextcp.y,round)-hdb->current.y,round);
		AddNumber2(gb,myround2(spline->to->prevcp.x,round)-myround2(spline->from->nextcp.x,round),round);
		AddNumber2(gb,myround2(spline->to->prevcp.y,round)-myround2(spline->from->nextcp.y,round),round);
		AddNumber2(gb,myround2(spline->to->me.x,round)-myround2(spline->to->prevcp.x,round),round);
		hv = 0;
	    } else {
		AddNumber2(gb,myround2(spline->from->nextcp.x,round)-hdb->current.x,round);
		AddNumber2(gb,myround2(spline->to->prevcp.x,round)-myround2(spline->from->nextcp.x,round),round);
		AddNumber2(gb,myround2(spline->to->prevcp.y,round)-myround2(spline->from->nextcp.y,round),round);
		AddNumber2(gb,myround2(spline->to->me.y,round)-myround2(spline->to->prevcp.y,round),round);
		hv = 1;
	    }
	    hdb->current.x = myround2(spline->to->me.x,round);
	    hdb->current.y = myround2(spline->to->me.y,round);
	    ++cnt;
	    spline = spline->to->next;
	    if ( spline==done || spline==NULL || cnt>9 || spline->knownlinear )
	break;
	}
	if ( gb->pt+1 >= gb->end )
	    GrowBuffer(gb);
	*(gb->pt)++ = ( start.x==myround2(first->from->nextcp.x,round) && myround2(first->to->prevcp.y,round)==myround2(first->to->me.y,round) )?
		30:31;		/* vhcurveto:hvcurveto */
return( spline );
    }
    while ( cnt<6 ) {
	if ( !donehm && spline->to->hintmask!=NULL )
    break;
	donehm = false;
	hv = -1;
	if ( hdb->current.x==myround2(spline->from->nextcp.x,round) &&
		myround2(spline->to->prevcp.y,round)==myround2(spline->to->me.y,round) &&
		spline->to->next!=NULL &&
		myround2(spline->to->me.y,round)==myround2(spline->to->nextcp.y,round) &&
		myround2(spline->to->next->to->prevcp.x,round)==myround2(spline->to->next->to->me.x,round) )
    break;
	else if ( hdb->current.y==myround2(spline->from->nextcp.y,round) &&
		myround2(spline->to->prevcp.x,round)==myround2(spline->to->me.x,round) &&
		spline->to->next!=NULL &&
		myround2(spline->to->me.x,round)==myround2(spline->to->nextcp.x,round) &&
		myround2(spline->to->next->to->prevcp.y,round)==myround2(spline->to->next->to->me.y,round) )
    break;
	AddNumber2(gb,myround2(spline->from->nextcp.x,round)-hdb->current.x,round);
	AddNumber2(gb,myround2(spline->from->nextcp.y,round)-hdb->current.y,round);
	AddNumber2(gb,myround2(spline->to->prevcp.x,round)-myround2(spline->from->nextcp.x,round),round);
	AddNumber2(gb,myround2(spline->to->prevcp.y,round)-myround2(spline->from->nextcp.y,round),round);
	AddNumber2(gb,myround2(spline->to->me.x,round)-myround2(spline->to->prevcp.x,round),round);
	AddNumber2(gb,myround2(spline->to->me.y,round)-myround2(spline->to->prevcp.y,round),round);
	hdb->current.x = myround2(spline->to->me.x,round);
	hdb->current.y = myround2(spline->to->me.y,round);
	++cnt;
	spline = spline->to->next;
	if ( spline==done || spline==NULL || spline->knownlinear )
    break;
    }
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 8;		/* rrcurveto */
return( spline );
}

static void flexto2(GrowBuf *gb,struct hintdb *hdb,Spline *pspline,int round) {
    BasePoint *c0, *c1, *mid, *end, *nc0, *nc1;
    Spline *nspline;

    c0 = &pspline->from->nextcp;
    c1 = &pspline->to->prevcp;
    mid = &pspline->to->me;
    nspline = pspline->to->next;
    nc0 = &nspline->from->nextcp;
    nc1 = &nspline->to->prevcp;
    end = &nspline->to->me;

    HintSetup2(gb,hdb,nspline->to,true);

    if ( myround2(c0->y,round)==hdb->current.y && myround2(nc1->y,round)==hdb->current.y &&
	    myround2(end->y,round)==hdb->current.y &&
	    myround2(c1->y,round)==myround2(mid->y,round) && myround2(nc0->y,round)==myround2(mid->y,round) ) {
	if ( gb->pt+7*6+2 >= gb->end )
	    GrowBuffer(gb);
	AddNumber2(gb,myround2(c0->x,round)-hdb->current.x,round);
	AddNumber2(gb,myround2(c1->x,round)-myround2(c0->x,round),round);
	AddNumber2(gb,myround2(c1->y,round)-myround2(c0->y,round),round);
	AddNumber2(gb,myround2(mid->x,round)-myround2(c1->x,round),round);
	AddNumber2(gb,myround2(nc0->x,round)-myround2(mid->x,round),round);
	AddNumber2(gb,myround2(nc1->x,round)-myround2(nc0->x,round),round);
	AddNumber2(gb,myround2(end->x,round)-myround2(nc1->x,round),round);
	*gb->pt++ = 12; *gb->pt++ = 34;		/* hflex */
    } else {
	if ( gb->pt+11*6+2 >= gb->end )
	    GrowBuffer(gb);
	AddNumber2(gb,myround2(c0->x,round)-hdb->current.x,round);
	AddNumber2(gb,myround2(c0->y,round)-hdb->current.y,round);
	AddNumber2(gb,myround2(c1->x,round)-myround2(c0->x,round),round);
	AddNumber2(gb,myround2(c1->y,round)-myround2(c0->y,round),round);
	AddNumber2(gb,myround2(mid->x,round)-myround2(c1->x,round),round);
	AddNumber2(gb,myround2(mid->y,round)-myround2(c1->y,round),round);
	AddNumber2(gb,myround2(nc0->x,round)-myround2(mid->x,round),round);
	AddNumber2(gb,myround2(nc0->y,round)-myround2(mid->y,round),round);
	AddNumber2(gb,myround2(nc1->x,round)-myround2(nc0->x,round),round);
	AddNumber2(gb,myround2(nc1->y,round)-myround2(nc0->y,round),round);
	if ( hdb->current.y==myround2(end->y,round) )
	    AddNumber2(gb,myround2(end->x,round)-myround2(nc1->x,round),round);
	else
	    AddNumber2(gb,myround2(end->y,round)-myround2(nc1->y,round),round);
	*gb->pt++ = 12; *gb->pt++ = 37;		/* flex1 */
    }

    hdb->current.x = rint(32768*end->x)/32768;
    hdb->current.y = rint(32768*end->y)/32768;
}

static void CvtPsSplineSet2(GrowBuf *gb, SplinePointList *spl,
	struct hintdb *hdb, int is_order2,int round ) {
    Spline *spline, *first;
    SplinePointList temp, *freeme = NULL;
    int unhinted = true;;

    if ( is_order2 )
	freeme = spl = SplineSetsPSApprox(spl);

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL;
	SplineSetReverse(spl);
	/* PostScript and TrueType store their splines in in reverse */
	/*  orientations. Annoying. Oh well. I shall adopt TrueType and */
	/*  If I reverse the PS splinesets after reading them in, and then */
	/*  again when saving them out, all should be well */
	if ( spl->first->flexy || spl->first->flexx ) {
	    /* can't handle a flex (mid) point as the first point. rotate the */
	    /* list by one, this is possible because only closed paths have */
	    /* points marked as flex, and because we can't have two flex mid- */
	    /* points in a row */
	    if ( spl->first->hintmask==NULL || spl->first->next->to->hintmask!=NULL ) {
		/* But we can't rotate it if we expect it to provide us with */
		/*  a hintmask.                 			     */
		temp = *spl;
		temp.first = temp.last = spl->first->next->to;
		spl = &temp;
	    }
	    if ( spl->first->flexy || spl->first->flexx ) {
		/* If we couldn't rotate, or if we rotated to something that */
		/*  also is flexible, then just turn off flex. That's safe   */
		spl->first->flexx = spl->first->flexy = false;
	    }
	}
	if ( unhinted && hdb->cnt>0 && spl->first->hintmask!=NULL ) {
	    hdb->mask[0] = ~(*spl->first->hintmask)[0];	/* Make it different */
	    unhinted = false;
	}
	moveto2(gb,hdb,spl->first,round);
	for ( spline = spl->first->next; spline!=NULL && spline!=first; ) {
	    if ( first==NULL ) first = spline;
	    else if ( first->from==spline->to )
		hdb->skiphm = true;
	    if ( spline->to->flexx || spline->to->flexy ) {
		flexto2(gb,hdb,spline,round);	/* does two adjacent splines */
		spline = spline->to->next->to->next;
	    } else if ( spline->knownlinear && spline->to == spl->first )
		/* In Type2 we don't even need a closepath to finish this off */
		/*  (which is good, because there isn't a close path) */
	break;
	    else if ( spline->knownlinear )
		spline = lineto2(gb,hdb,spline,first,round);
	    else
		spline = curveto2(gb,hdb,spline,first,round);
	}
	hdb->skiphm = false;
	/* No closepath oper in type2 fonts, it's implied */
	SplineSetReverse(spl);
	/* Of course, I have to Reverse again to get back to my convention after*/
	/*  saving */
    }
    SplinePointListsFree(freeme);
}

void debug_printHintInstance( HintInstance* hi, int hin, char* msg )
{
    printf("___ hint instance %d %s\n", hin, msg );
    if( !hi )
	return;
    
    printf("hi.begin      %f\n", hi->begin );
    printf("hi.end        %f\n", hi->end );
    printf("hi.closed     %d\n", hi->closed );
    printf("hi.cnum       %d\n", hi->counternumber );
    printf("hi.next       %p\n", hi->next );
    if( hi->next )
	debug_printHintInstance( hi->next, hin+1, msg );
}


void debug_printHint( StemInfo *h, char* msg )
{
    printf("==============================\n");
    printf("debug_printHint(%p)... %s\n", h, msg );
    if( h )
    {
	printf("start         %f\n", h->start );
	printf("width         %f\n", h->width );
	printf("hinttype      %d\n", h->hinttype );
	printf("ghost         %d\n", h->ghost );
	printf("haspointleft  %d\n", h->haspointleft );
	printf("haspointright %d\n", h->haspointright );
	printf("hasconflicts  %d\n", h->hasconflicts );
	printf("used          %d\n", h->used );
	printf("tobeused      %d\n", h->tobeused );
	printf("active        %d\n", h->active );
	printf("enddone       %d\n", h->enddone );
	printf("startdone     %d\n", h->startdone );
	printf("reordered     %d\n", h->reordered );
	printf("pendingpt     %d\n", h->pendingpt );
	printf("linearedges   %d\n", h->linearedges );
	printf("hintnumber    %d\n", h->hintnumber );
	if( h->where )
	    debug_printHintInstance( h->where, 1, "" );
    }
    printf("==============================\n");    
}

bool equalWithTolerence( real a, real b, real tolerence )
{
//    printf("equalWithTolerence(1) a:%f b:%f tol:%f\n", a, b, tolerence );
//    printf("equalWithTolerence(2) a:%lf b:%lf tol:%lf\n", a, b, tolerence );
    
    if( tolerence == 0.0 )
	return a == b;

    return(    (b - tolerence < a)
	    && (b + tolerence > a ));
}

static void DumpHints(GrowBuf *gb,StemInfo *h,int oper,int midoper,int round) {
    real last = 0, cur;
    int cnt;

    if ( h==NULL )
return;
    cnt = 0;
    while ( h && h->hintnumber!=-1 ) {
	/* Type2 hints do not support negative widths except in the case of */
	/*  ghost (now called edge) hints */
	if ( cnt>24-2 ) {	/* stack max = 48 numbers, => 24 hints, leave a bit of slop for the width */
	    if ( gb->pt+1>=gb->end )
		GrowBuffer(gb);
	    *gb->pt++ = midoper;
	    cnt = 0;
	}
	cur = myround2(h->start,round) + myround2(h->width,round);
	if ( h->width<0 ) {
	    AddNumber2(gb,cur-last,round);
	    AddNumber2(gb,-myround2(h->width,round),round);
	    cur -= myround2(h->width,round);
	} else if ( h->ghost ) {
	    if ( equalWithTolerence( h->width, 20, GenerateHintWidthEqualityTolerance )) {
		AddNumber2(gb,myround2(h->start,round)-last+20,round);
		AddNumber2(gb,-20,round);
		cur = myround2(h->start,round);
	    } else {
		AddNumber2(gb,myround2(h->start+21,round)-last,round);
		AddNumber2(gb,-21,round);
		cur = myround2(h->start+21,round)-21;
	    }
	} else {
	    AddNumber2(gb,myround2(h->start,round)-last,round);
	    AddNumber2(gb,myround2(h->width,round),round);
	}
	last = cur;
	h = h->next;
	++cnt;
    }
    if ( oper!=-1 ) {
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*gb->pt++ = oper;
    }
}

static void DumpRefsHints(GrowBuf *gb, struct hintdb *hdb,RefChar *cur,StemInfo *h,StemInfo *v,
	BasePoint *trans, int round,int layer) {
    uint8 masks[12];
    int cnt, sets=0;
    StemInfo *rs;

    /* trans has already been rounded (whole char is translated by an integral amount) */

    /* If we have a subroutine containing conflicts, then its hints will match*/
    /*  ours exactly, and we can use its hintmasks directly */
    if (( cur->sc->hconflicts || cur->sc->vconflicts ) &&
	    cur->sc->layers[layer].splines!=NULL &&
	    cur->sc->layers[layer].splines->first->hintmask!=NULL ) {
	AddMask2(gb,*cur->sc->layers[layer].splines->first->hintmask,hdb->cnt,19);		/* hintmask */
	hdb->donefirsthm = true;
	memcpy(hdb->mask,*cur->sc->layers[layer].splines->first->hintmask,sizeof(HintMask));
return;
    }

    if ( h==NULL && v==NULL )
	IError("hintmask invoked when there are no hints");
    memset(masks,'\0',sizeof(masks));
    cnt = 0;
    while ( h!=NULL && h->hintnumber>=0 ) {
	/* Horizontal stems are defined by vertical bounds */
	real pos = (round ? rint(h->start) : h->start) - trans->y;
	for ( rs = cur->sc->hstem; rs!=NULL; rs=rs->next ) {
	    real rpos = round ? rint(rs->start) : rs->start;
	    if ( rpos==pos && (round ? (rint(rs->width)==rint(h->width)) : (rs->width==h->width)) ) {
		masks[h->hintnumber>>3] |= 0x80>>(h->hintnumber&7);
		++sets;
	break;
	    } else if ( rpos>pos )
	break;
	}
	h = h->next; ++cnt;
    }
    while ( v!=NULL && v->hintnumber>=0 ) {
	real pos = (round ? rint(v->start) : v->start) - trans->x;
	for ( rs = cur->sc->vstem; rs!=NULL; rs=rs->next ) {
	    real rpos = round ? rint(rs->start) : rs->start;
	    if ( rpos==pos && (round ? (rint(rs->width)==rint(v->width)) : (rs->width==v->width)) ) {
		masks[v->hintnumber>>3] |= 0x80>>(v->hintnumber&7);
		++sets;
	break;
	    } else if ( rpos>pos )
	break;
	}
	v = v->next; ++cnt;
    }
    BreakSubroutine(gb,hdb);
    hdb->donefirsthm = true;
    /* if ( sets!=0 ) */	/* First ref will need a hintmask even if it has no hints (if there are conflicts) */
	AddMask2(gb,masks,cnt,19);		/* hintmask */
}

static void DummyHintmask(GrowBuf *gb,struct hintdb *hdb) {
    HintMask hm;

    memset(hm,0,sizeof(hm));
    if ( hdb->cnt!=0 ) {
	BreakSubroutine(gb,hdb);
	hdb->donefirsthm = true;
	AddMask2(gb,hm,hdb->cnt,19);		/* hintmask */
    }
}

static void SetTransformedHintMask(GrowBuf *gb,struct hintdb *hdb,
	SplineChar *sc, RefChar *ref, BasePoint *trans, int round) {
    HintMask hm;

    if ( HintMaskFromTransformedRef(ref,trans,sc,&hm)!=NULL ) {
	BreakSubroutine(gb,hdb);
	hdb->donefirsthm = true;
	AddMask2(gb,hm,hdb->cnt,19);		/* hintmask */
    } else if ( !hdb->donefirsthm )
	DummyHintmask(gb,hdb);
}

static void ExpandRef2(GrowBuf *gb, SplineChar *sc, struct hintdb *hdb,
	RefChar *r, BasePoint *trans,
	struct pschars *subrs, int round,int layer) {
    BasePoint *bpt;
    BasePoint temp, rtrans;
    GlyphInfo *gi;
    /* The only refs I deal with here have no hint conflicts within them */
    
    rtrans.x = r->transform[4]+trans->x;
    rtrans.y = r->transform[5]+trans->y;
    if ( round ) {
	rtrans.x = rint(rtrans.x);
	rtrans.y = rint(rtrans.y);
    }

    BreakSubroutine(gb,hdb);
    if ( hdb->cnt>0 && !hdb->noconflicts )
	DumpRefsHints(gb,hdb,r,sc->hstem,sc->vstem,&rtrans,round,layer);

    /* Translate from end of last character to where this one should */
    /*  start (we must have one moveto operator to start off, none */
    /*  in the subr) */
    bpt = hdb->gi->psubrs[r->sc->lsidebearing].startstop;
    temp.x = bpt[0].x+rtrans.x;
    temp.y = bpt[0].y+rtrans.y;
    if ( hdb->current.x!=temp.x )
	AddNumber2(gb,temp.x-hdb->current.x,round);
    if ( hdb->current.y!=temp.y || hdb->current.x==temp.x )
	AddNumber2(gb,temp.y-hdb->current.y,round);
    if ( gb->pt+1>=gb->end )
	GrowBuffer(gb);
    *gb->pt++ = hdb->current.x==temp.x?4:	/* vmoveto */
		hdb->current.y==temp.y?22:	/* hmoveto */
		21;				/* rmoveto */
    if ( r->sc->lsidebearing==0x7fff )
	IError("Attempt to reference an unreferenceable glyph %s", r->sc->name );

    gi = hdb->gi;
    StartNextSubroutine(gb,hdb);
    gi->bits[gi->bcnt].psub_index = r->sc->lsidebearing;
    ++gi->bcnt;
    gi->justbroken = true;
    hdb->current.x = bpt[1].x+rtrans.x;
    hdb->current.y = bpt[1].y+rtrans.y;
}

static void RSC2PS2(GrowBuf *gb, SplineChar *base,SplineChar *rsc,
	struct hintdb *hdb, BasePoint *trans, struct pschars *subrs,
	int flags, int layer ) {
    BasePoint subtrans;
    int stationary = trans->x==0 && trans->y==0;
    RefChar *r, *unsafe=NULL;
    int unsafecnt=0, allwithouthints=true;
    int round = (flags&ps_flag_round)? true : false;
    StemInfo *oldh, *oldv;
    int hc, vc;
    SplineSet *freeme, *temp;
    int wasntconflicted = hdb->noconflicts;

    if ( flags&ps_flag_nohints ) {
	oldh = rsc->hstem; oldv = rsc->vstem;
	hc = rsc->hconflicts; vc = rsc->vconflicts;
	rsc->hstem = NULL; rsc->vstem = NULL;
	rsc->hconflicts = false; rsc->vconflicts = false;
    } else {
	for ( r=rsc->layers[layer].refs; r!=NULL; r=r->next ) {
	    /* Ensure hintmask on refs are set correctly */
	    if (SCNeedsSubsPts(r->sc, ff_otf, layer))
	        SCFigureHintMasks(r->sc, layer);

	    if ( !r->justtranslated )
	continue;
	    if ( r->sc->hconflicts || r->sc->vconflicts ) {
		++unsafecnt;
		unsafe = r;
	    } else if ( r->sc->hstem!=NULL || r->sc->vstem!=NULL )
		allwithouthints = false;
	}
	if ( !stationary )
	    allwithouthints = false;
	if ( allwithouthints && unsafe!=NULL && hdb->cnt!=NumberHints(&unsafe->sc,1)) 
	    allwithouthints = false;		/* There are other hints elsewhere in the base glyph */
    }

    if ( unsafe && allwithouthints ) {
	if ( unsafe->sc->lsidebearing!=0x7fff ) {
	    ExpandRef2(gb,base,hdb,unsafe,trans,subrs,round,layer);
	} else if ( unsafe->transform[4]==0 && unsafe->transform[5]==0 )
	    RSC2PS2(gb,base,unsafe->sc,hdb,trans,subrs,flags,layer);
	else
	    unsafe = NULL;
    } else
	unsafe = NULL;

    /* What is the hintmask state here? It should not matter */
    freeme = NULL; temp = rsc->layers[layer].splines;
    if ( base!=rsc )
	temp = freeme = SPLCopyTranslatedHintMasks(temp,base,rsc,trans);
    CvtPsSplineSet2(gb,temp,hdb,rsc->layers[layer].order2,round);
    SplinePointListsFree(freeme);

    for ( r = rsc->layers[layer].refs; r!=NULL; r = r->next ) if ( r!=unsafe ) {
	if ( !r->justtranslated ) {
	    if ( !r->sc->hconflicts && !r->sc->vconflicts && !hdb->noconflicts &&
		    r->transform[1]==0 && r->transform[2]==0 &&
		    r->transform[0]>0 && r->transform[3]>0 )
		SetTransformedHintMask(gb,hdb,base,r,trans,round);
	    if ( !hdb->donefirsthm )
		DummyHintmask(gb,hdb);
	    temp = SPLCopyTransformedHintMasks(r,base,trans,layer);
	    CvtPsSplineSet2(gb,temp,hdb,rsc->layers[layer].order2,round);
	    SplinePointListsFree(temp);
	} else if ( r->sc->lsidebearing!=0x7fff &&
		((flags&ps_flag_nohints) ||
		 (!r->sc->hconflicts && !r->sc->vconflicts)) ) {
	    ExpandRef2(gb,base,hdb,r,trans,subrs,round,layer);
	} else {
	    subtrans.x = trans->x + r->transform[4];
	    subtrans.y = trans->y + r->transform[5];
	    if ( !hdb->noconflicts && !r->sc->hconflicts && !r->sc->vconflicts) {
		SetTransformedHintMask(gb,hdb,base,r,trans,round);
		hdb->noconflicts = true;
	    }
	    RSC2PS2(gb,base,r->sc,hdb,&subtrans,subrs,flags,layer);
	    hdb->noconflicts = wasntconflicted;
	}
    }

    if ( flags&ps_flag_nohints ) {
	rsc->hstem = oldh; rsc->vstem = oldv;
	rsc->hconflicts = hc; rsc->vconflicts = vc;
    }
}

static unsigned char *SplineChar2PS2(SplineChar *sc,int *len, int nomwid,
	int defwid, struct pschars *subrs, int flags,
	GlyphInfo *gi) {
    GrowBuf gb;
    unsigned char *ret;
    struct hintdb hdb;
    StemInfo *oldh, *oldv;
    int hc, vc;
    SplineChar *scs[MmMax];
    int round = (flags&ps_flag_round)? true : false;
    HintMask *hm = NULL;
    BasePoint trans;

    if ( autohint_before_generate && sc->changedsincelasthinted &&
	    !sc->manualhints && !(flags&ps_flag_nohints))
	SplineCharAutoHint(sc,gi->layer,NULL);
    if ( !(flags&ps_flag_nohints) && SCNeedsSubsPts(sc,ff_otf,gi->layer))
	SCFigureHintMasks(sc,gi->layer);

    if ( flags&ps_flag_nohints ) {
	oldh = sc->hstem; oldv = sc->vstem;
	hc = sc->hconflicts; vc = sc->vconflicts;
	sc->hstem = NULL; sc->vstem = NULL;
	sc->hconflicts = false; sc->vconflicts = false;
    } else if ( sc->layers[gi->layer].splines!=NULL && !sc->vconflicts &&
	    !sc->hconflicts ) {
	hm = sc->layers[gi->layer].splines->first->hintmask;
	sc->layers[gi->layer].splines->first->hintmask = NULL;
    }

    memset(&gb,'\0',sizeof(gb));

    GrowBuffer(&gb);

    /* store the width on the stack */
    if ( sc->width==defwid )
	/* Don't need to do anything for the width */;
    else
	AddNumber2(&gb,sc->width-nomwid,round);

    memset(&trans,'\0',sizeof(trans));
    memset(&hdb,'\0',sizeof(hdb));
    hdb.scs = scs;
    hdb.gi = gi;
    if ( gi!=NULL )
	gi->bcnt = -1;
    scs[0] = sc;
    hdb.noconflicts = !sc->hconflicts && !sc->vconflicts;
    hdb.cnt = NumberHints(hdb.scs,1);
    DumpHints(&gb,sc->hstem,sc->hconflicts || sc->vconflicts?18:1,
			    sc->hconflicts || sc->vconflicts?18:1,round);
    DumpHints(&gb,sc->vstem,sc->hconflicts || sc->vconflicts?-1:3,
			    sc->hconflicts || sc->vconflicts?23:3,round);
    CounterHints2(&gb, sc, hdb.cnt );
    RSC2PS2(&gb,sc,sc,&hdb,&trans,subrs,flags,gi->layer);

    if ( gi->bcnt==-1 ) {	/* If it's whitespace */
	gi->bcnt = 0;
	StartNextSubroutine(&gb,&hdb);
    }
    BreakSubroutine(&gb,&hdb);
    MoveSubrsToChar(gi);
    ret = NULL;

    free(gb.base);
    if ( flags&ps_flag_nohints ) {
	sc->hstem = oldh; sc->vstem = oldv;
	sc->hconflicts = hc; sc->vconflicts = vc;
    } else if ( hm!=NULL )
	sc->layers[gi->layer].splines->first->hintmask = hm;
return( ret );
}

static SplinePoint *LineTo(SplinePoint *last, int x, int y) {
    SplinePoint *sp = SplinePointCreate(x,y);
    SplineMake3(last,sp);
return( sp );
}

static void Type2NotDefSplines(SplineFont *sf,SplineChar *sc,int layer) {
    /* I'd always assumed that Type2 notdefs would look like type1 notdefs */
    /*  but they don't, they look like truetype notdefs. And Ralf Stubner */
    /*  points out that the spec says they should. So make a box here */
    int stem, ymax;
    SplineSet *inner, *ss;
    StemInfo *h, *hints;

    stem = (sf->ascent+sf->descent)/20;
    ymax = 2*sf->ascent/3;

    ss = chunkalloc(sizeof(SplineSet));
    ss->first = ss->last = SplinePointCreate(stem,0);
    ss->last = LineTo(ss->last,stem,ymax);
    ss->last = LineTo(ss->last,sc->width-stem,ymax);
    ss->last = LineTo(ss->last,sc->width-stem,0);
    SplineMake3(ss->last,ss->first);
    ss->last = ss->first;

    ss->next = inner = chunkalloc(sizeof(SplineSet));
    inner->first = inner->last = SplinePointCreate(2*stem,stem);
    inner->last = LineTo(inner->last,sc->width-2*stem,stem);
    inner->last = LineTo(inner->last,sc->width-2*stem,ymax-stem);
    inner->last = LineTo(inner->last,2*stem,ymax-stem);
    SplineMake3(inner->last,inner->first);
    inner->last = inner->first;

    sc->layers[layer].splines = ss;

    hints = chunkalloc(sizeof(StemInfo));
    hints->start = stem;
    hints->width = stem;
    hints->next = h = chunkalloc(sizeof(StemInfo));
    h->start = sc->width-2*stem;
    h->width = stem;
    sc->vstem = hints;

    hints = chunkalloc(sizeof(StemInfo));
    hints->start = 0;
    hints->width = stem;
    hints->next = h = chunkalloc(sizeof(StemInfo));
    h->start = ymax-stem;
    h->width = stem;
    sc->hstem = hints;
}

#ifdef FONTFORGE_CONFIG_PS_REFS_GET_SUBRS
/* This char has hint conflicts. Check to see if we can put it into a subr */
/*  in spite of that. If there is at least one dependent character which: */
/*	refers to us without translating us */
/*	and all its other refs contain no hints at all */
static int Type2SpecialCase(SplineChar *sc) {
    struct splinecharlist *d;
    RefChar *r;

    for ( d=sc->dependents; d!=NULL; d=d->next ) {
	for ( r=d->sc->layers[layer].refs; r!=NULL; r = r->next ) {
	    if ( autohint_before_generate && r->sc!=NULL &&
		    r->sc->changedsincelasthinted && !r->sc->manualhints )
		SplineCharAutoHint(r->sc,NULL);
	    if ( r->transform[0]!=1 || r->transform[1]!=0 ||
		    r->transform[2]!=0 || r->transform[3]!=1 )
	break;
	    if ( r->sc!=sc && (r->sc->hstem!=NULL || r->sc->vstem!=NULL))
	break;
	    if ( r->sc==sc && (r->transform[4]!=0 || r->transform[5]!=0))
	break;
	}
	if ( r==NULL )
return( true );
    }
return( false );
}
#endif	/* FONTFORGE_CONFIG_PS_REFS_GET_SUBRS */

/* Mark those glyphs which can live totally in subrs */
static void SplineFont2FullSubrs2(int flags,GlyphInfo *gi) {
    int i;
    SplineChar *sc;
#ifdef FONTFORGE_CONFIG_PS_REFS_GET_SUBRS
    int cc;
    RefChar *r;
    struct potentialsubrs *ps;
    SplineSet *spl;
#endif	/* FONTFORGE_CONFIG_PS_REFS_GET_SUBRS */

    if ( !autohint_before_generate && !(flags&ps_flag_nohints))
	SplineFontAutoHintRefs(gi->sf,gi->layer);

    for ( i=0; i<gi->glyphcnt; ++i ) if ( (sc=gi->gb[i].sc)!=NULL )
	sc->lsidebearing = 0x7fff;

/* This code allows us to put whole glyphs into subroutines */
/* I found slight improvements in space on some fonts, and large increases */
/*  in others. So I'm disabling it for now */
#ifdef FONTFORGE_CONFIG_PS_REFS_GET_SUBRS
    /* We don't allow refs to refs. It's too complex */
    for ( i=0; i<gi->glyphcnt; ++i ) if ( (sc=gi->gb[i].sc)!=NULL ) {
	if ( SCWorthOutputting(sc) &&
	    (( sc->layers[layer].refs==NULL && sc->dependents!=NULL &&
		    ( (!sc->hconflicts && !sc->vconflicts) ||
			Type2SpecialCase(sc)) )  )) {
	    /* if the glyph is a single contour with no hintmasks then */
	    /*  our single contour code will find it. If we do it here too */
	    /*  we'll get a subr which points to another subr. Very dull and */
	    /*  a waste of space */
	    cc = 0;
	    for ( spl=sc->layers[layer].splines; spl!=NULL; spl=spl->next )
		++cc;
	    for ( r= sc->layers[layer].refs; r!=NULL && cc<2 ; r=r->next ) {
		for ( spl=r->layers[0].splines; spl!=NULL; spl=spl->next )
		    ++cc;
	    }
	    if ( cc<2 )
    continue;
	    /* Put the */
	    /*  character into a subr if it is referenced by other characters */
	    if ( gi->pcnt>=gi->pmax )
		gi->psubrs = realloc(gi->psubrs,(gi->pmax+=gi->glyphcnt)*sizeof(struct potentialsubrs));
	    ps = &gi->psubrs[gi->pcnt];
	    memset(ps,0,sizeof(*ps));	/* set cnt to 0 */
	    ps->idx = gi->pcnt++;
	    ps->full_glyph_index = i;
	    sc->lsidebearing = gi->pcnt-1;
	    ps->startstop = FigureStartStop(sc,gi);
	}
    }
#endif	/* FONTFORGE_CONFIG_PS_REFS_GET_SUBRS */
}

struct pschars *SplineFont2ChrsSubrs2(SplineFont *sf, int nomwid, int defwid,
	const int *bygid, int cnt, int flags, struct pschars **_subrs, int layer) {
    struct pschars *subrs, *chrs;
    int i,j,k,scnt;
    SplineChar *sc;
    GlyphInfo gi;
    SplineChar dummynotdef;

    if ( !autohint_before_generate && !(flags&ps_flag_nohints))
	SplineFontAutoHintRefs(sf,layer);

    memset(&gi,0,sizeof(gi));
    memset(&gi.hashed,-1,sizeof(gi.hashed));
    gi.instance_count = 1;
    gi.sf = sf;
    gi.layer = layer;
    gi.glyphcnt = cnt;
    gi.bygid = bygid;
    gi.gb = calloc(cnt,sizeof(struct glyphbits));
    gi.pmax = 3*cnt;
    gi.psubrs = malloc(gi.pmax*sizeof(struct potentialsubrs));
    for ( i=0; i<cnt; ++i ) {
	int gid = bygid[i];
	if ( i==0 && gid==-1 ) {
	    sc = &dummynotdef;
	    memset(sc,0,sizeof(dummynotdef));
	    dummynotdef.name = ".notdef";
	    dummynotdef.parent = sf;
	    dummynotdef.layer_cnt = sf->layer_cnt;
	    dummynotdef.layers = calloc(sf->layer_cnt,sizeof(Layer));
	    dummynotdef.width = SFOneWidth(sf);
	    if ( dummynotdef.width==-1 )
		dummynotdef.width = (sf->ascent+sf->descent)/2;
	    Type2NotDefSplines(sf,&dummynotdef,layer);
	} else if ( gid!=-1 )
	    sc = sf->glyphs[gid];
	else
    continue;
	gi.gb[i].sc = sc;
	if ( autohint_before_generate && sc!=NULL &&
		sc->changedsincelasthinted && !sc->manualhints &&
		!(flags&ps_flag_nohints))
	    SplineCharAutoHint(sc,layer,NULL);
	sc->lsidebearing = 0x7fff;
    }
    MarkTranslationRefs(sf,layer);
    SplineFont2FullSubrs2(flags,&gi);

    for ( i=0; i<cnt; ++i ) {
	if ( (sc = gi.gb[i].sc)==NULL )
    continue;
	gi.active = &gi.gb[i];
	SplineChar2PS2(sc,NULL,nomwid,defwid,NULL,flags,&gi);
	ff_progress_next();
    }

    for ( i=scnt=0; i<gi.pcnt; ++i ) {
	/* A subroutine call takes somewhere between 2 and 4 bytes itself. */
	/*  and we must add a return statement to the end. We don't want to */
	/*  make things bigger */
	/* if we have more than 65535 subrs a subr call can take 9 bytes */
	if ( gi.psubrs[i].full_glyph_index!=-1 )
	    gi.psubrs[i].idx = scnt++;
	else if ( gi.psubrs[i].cnt*gi.psubrs[i].len>(gi.psubrs[i].cnt*4)+gi.psubrs[i].len+1 )
	    gi.psubrs[i].idx = scnt++;
	else
	    gi.psubrs[i].idx = -1;
    }
    subrs = calloc(1,sizeof(struct pschars));
    subrs->cnt = scnt;
    subrs->next = scnt;
    subrs->lens = malloc(scnt*sizeof(int));
    subrs->values = malloc(scnt*sizeof(unsigned char *));
    subrs->bias = scnt<1240 ? 107 :
		  scnt<33900 ? 1131 : 32768;
    for ( i=0; i<gi.pcnt; ++i ) {
	if ( gi.psubrs[i].idx != -1 ) {
	    scnt = gi.psubrs[i].idx;
	    subrs->lens[scnt] = gi.psubrs[i].len+1;
	    subrs->values[scnt] = malloc(subrs->lens[scnt]);
	    memcpy(subrs->values[scnt],gi.psubrs[i].data,gi.psubrs[i].len);
	    subrs->values[scnt][gi.psubrs[i].len] = 11;	/* Add a return to end of subr */
	}
    }

    chrs = calloc(1,sizeof(struct pschars));
    chrs->cnt = cnt;
    chrs->next = cnt;
    chrs->lens = malloc(cnt*sizeof(int));
    chrs->values = malloc(cnt*sizeof(unsigned char *));
    chrs->keys = malloc(cnt*sizeof(char *));
    for ( i=0; i<cnt; ++i ) {
	int len=0;
	uint8 *vals;
	struct glyphbits *gb = &gi.gb[i];
	if ( gb->sc==NULL )
    continue;
	chrs->keys[i] = copy(gb->sc->name);
	for ( k=0; k<2; ++k ) if ( k!=0 || gb->sc->lsidebearing!=0x7fff ) {
	    for ( j=0; j<gb->bcnt; ++j ) {
		if ( k!=0 || j!=0 )
		    len += gb->bits[j].dlen;
		if ( k==1 && gb->sc->lsidebearing!=0x7fff ) {
		    int si = gi.psubrs[ gb->sc->lsidebearing ].idx;
		    len += 1 + (si<=107 && si>=-107?1:si<=1131 && si>=-1131?2:si>=-32768 && si<32767?3:8);
	    break;
		}
		if ( gi.psubrs[ gb->bits[j].psub_index ].idx==-1 )
		    len += gi.psubrs[ gb->bits[j].psub_index ].len;
		else {
		    int si = gi.psubrs[ gb->bits[j].psub_index ].idx - subrs->bias;
		    /* space for the number (subroutine index) */
		    if ( si>=-107 && si<=107 )
			++len;
		    else if ( si>=-1131 && si<=1131 )
			len += 2;
		    else if ( si>=-32768 && si<=32767 )
			len += 3;
		    else
			len += 8;
		    /* space for the subroutine operator */
		    ++len;
		}
	    }
	    if ( k==0 ) {
		int si = gi.psubrs[ gb->sc->lsidebearing ].idx;
		subrs->lens[si] = len+1;
		vals = subrs->values[si] = malloc(len+2);
	    } else {
		chrs->lens[i] = len+1;
		vals = chrs->values[i] = malloc(len+2); /* space for endchar and a final NUL (which is really meaningless, but makes me feel better) */
	    }

	    len = 0;
	    for ( j=0; j<gb->bcnt; ++j ) {
		int si;
		if ( k!=0 || j!=0 ) {
		    memcpy(vals+len,gb->bits[j].data,gb->bits[j].dlen);
		    len += gb->bits[j].dlen;
		}
		si = 0x80000000;
		if ( k==1 && gb->sc->lsidebearing!=0x7fff )
		    si = gi.psubrs[ gb->sc->lsidebearing ].idx - subrs->bias;
		else if ( gi.psubrs[ gb->bits[j].psub_index ].idx==-1 ) {
		    memcpy(vals+len,gi.psubrs[ gb->bits[j].psub_index ].data,
			    gi.psubrs[ gb->bits[j].psub_index ].len);
		    len += gi.psubrs[ gb->bits[j].psub_index ].len;
		} else
		    si = gi.psubrs[ gb->bits[j].psub_index ].idx - subrs->bias;
		if ( si!=0x80000000 ) {
		    /* space for the number (subroutine index) */
		    if ( si>=-107 && si<=107 )
			vals[len++] = si+139;
		    else if ( si>0 && si<=1131 ) {
			si-=108;
			vals[len++] = (si>>8)+247;
			vals[len++] = si&0xff;
		    } else if ( si>=-1131 && si<0 ) {
			si=(-si)-108;
			vals[len++] = (si>>8)+251;
			vals[len++] = si&0xff;
		    } else if ( si>=-32768 && si<=32767 ) {
			vals[len++] = 28;
			vals[len++] = (si>>8)&0xff;
			vals[len++] = si&0xff;
		    } else {
			/* store as fixed point, then multiply by 64. Takes 8 bytes */
			si *= (65536/64);
			vals[len++] = '\377';
			vals[len++] = (si>>24)&0xff;
			vals[len++] = (si>>16)&0xff;
			vals[len++] = (si>>8)&0xff;
			vals[len++] = si&0xff;
			vals[len++] = 64 + 139;
			vals[len++] = 0xc; vals[len++] = 0x18;	/* Multiply */
		    }
    
		    /* space for the subroutine operator */
		    vals[len++] = 10;
		}
		if ( k==1 && gb->sc->lsidebearing!=0x7fff )
	    break;
	    }
	    if ( k==0 ) {
		vals[len++] = 11;	/* return */
		vals[len] = '\0';
	    } else {
		vals[len++] = 14;	/* endchar */
		vals[len] = '\0';
	    }
	}
    }
    
    GIFree(&gi,&dummynotdef);
    *_subrs = subrs;
return( chrs );
}

struct pschars *CID2ChrsSubrs2(SplineFont *cidmaster,struct fd2data *fds,
	int flags, struct pschars **_glbls, int layer) {
    struct pschars *chrs, *glbls;
    int i, j, cnt, cid, max, fd;
    int *scnts;
    SplineChar *sc;
    SplineFont *sf = NULL;
    /* In a cid-keyed font, cid 0 is defined to be .notdef so there are no */
    /*  special worries. If it is defined we use it. If it is not defined */
    /*  we add it. */
    GlyphInfo gi;
    SplineChar dummynotdef;

    max = 0;
    for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	if ( max<cidmaster->subfonts[i]->glyphcnt )
	    max = cidmaster->subfonts[i]->glyphcnt;
	MarkTranslationRefs(cidmaster->subfonts[i],layer);
    }
    cnt = 1;			/* for .notdef */
    for ( cid = 1; cid<max; ++cid ) {
	for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	    sf = cidmaster->subfonts[i];
	    if ( cid<sf->glyphcnt && (sc=sf->glyphs[cid])!=NULL ) {
		sc->ttf_glyph = -1;
		sc->lsidebearing = 0x7fff;
		if ( SCWorthOutputting(sc))
		    ++cnt;
	break;
	    }
	}
    }

    memset(&gi,0,sizeof(gi));
    memset(&gi.hashed,-1,sizeof(gi.hashed));
    gi.instance_count = 1;
    gi.sf = sf;
    gi.glyphcnt = cnt;
    gi.bygid = NULL;
    gi.gb = calloc(cnt,sizeof(struct glyphbits));
    gi.pmax = 3*cnt;
    gi.psubrs = malloc(gi.pmax*sizeof(struct potentialsubrs));
    gi.layer = layer;

    for ( cid = cnt = 0; cid<max; ++cid ) {
	sf = NULL;
	for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	    sf = cidmaster->subfonts[i];
	    if ( cid<sf->glyphcnt && SCWorthOutputting(sf->glyphs[cid]) )
	break;
	}
	if ( cid!=0 && i==cidmaster->subfontcnt ) {
	    sc=NULL;
	} else if ( i==cidmaster->subfontcnt ) {
	    /* They didn't define CID 0 */
	    sc = &dummynotdef;
	    /* Place it in the final subfont (which is what sf points to) */
	    memset(sc,0,sizeof(dummynotdef));
	    dummynotdef.name = ".notdef";
	    dummynotdef.parent = sf;
	    dummynotdef.layer_cnt = layer+1;
	    dummynotdef.layers = calloc(layer+1,sizeof(Layer));
	    dummynotdef.width = SFOneWidth(sf);
	    if ( dummynotdef.width==-1 )
		dummynotdef.width = (sf->ascent+sf->descent);
	    Type2NotDefSplines(sf,&dummynotdef,layer);
	    gi.gb[cnt].sc = sc;
	    gi.gb[cnt].fd = i = cidmaster->subfontcnt-1;
	} else {
	    gi.gb[cnt].sc = sc = sf->glyphs[cid];
	    gi.gb[cnt].fd = i;
	}
	if ( sc!=NULL ) {
	    sc->lsidebearing = 0x7fff;
	    gi.active = &gi.gb[cnt];
	    sc->ttf_glyph = cnt++;
	    SplineChar2PS2(sc,NULL,fds[i].nomwid,fds[i].defwid,NULL,flags,&gi);
	}
	ff_progress_next();
    }

    scnts = calloc( cidmaster->subfontcnt+1,sizeof(int));
    for ( i=0; i<gi.pcnt; ++i ) {
	gi.psubrs[i].idx = -1;
	if ( gi.psubrs[i].cnt*gi.psubrs[i].len>(gi.psubrs[i].cnt*4)+gi.psubrs[i].len+1 )
	    gi.psubrs[i].idx = scnts[gi.psubrs[i].fd+1]++;
    }

    glbls = calloc(1,sizeof(struct pschars));
    glbls->cnt = scnts[0];
    glbls->next = scnts[0];
    glbls->lens = malloc(scnts[0]*sizeof(int));
    glbls->values = malloc(scnts[0]*sizeof(unsigned char *));
    glbls->bias = scnts[0]<1240 ? 107 :
		  scnts[0]<33900 ? 1131 : 32768;
    for ( fd=0; fd<cidmaster->subfontcnt; ++fd ) {
	fds[fd].subrs = calloc(1,sizeof(struct pschars));
	fds[fd].subrs->cnt = scnts[fd+1];
	fds[fd].subrs->next = scnts[fd+1];
	fds[fd].subrs->lens = malloc(scnts[fd+1]*sizeof(int));
	fds[fd].subrs->values = malloc(scnts[fd+1]*sizeof(unsigned char *));
	fds[fd].subrs->bias = scnts[fd+1]<1240 ? 107 :
			      scnts[fd+1]<33900 ? 1131 : 32768;
    }
    free( scnts);

    for ( i=0; i<gi.pcnt; ++i ) {
	if ( gi.psubrs[i].idx != -1 ) {
	    struct pschars *subrs = gi.psubrs[i].fd==-1 ? glbls : fds[gi.psubrs[i].fd].subrs;
	    int scnt = gi.psubrs[i].idx;
	    subrs->lens[scnt] = gi.psubrs[i].len+1;
	    subrs->values[scnt] = malloc(subrs->lens[scnt]);
	    memcpy(subrs->values[scnt],gi.psubrs[i].data,gi.psubrs[i].len);
	    subrs->values[scnt][gi.psubrs[i].len] = 11;	/* Add a return to end of subr */
	}
    }


    chrs = calloc(1,sizeof(struct pschars));
    chrs->cnt = cnt;
    chrs->next = cnt;
    chrs->lens = malloc(cnt*sizeof(int));
    chrs->values = malloc(cnt*sizeof(unsigned char *));
    chrs->keys = malloc(cnt*sizeof(char *));
    for ( i=0; i<cnt; ++i ) {
	int len=0;
	struct glyphbits *gb = &gi.gb[i];
	chrs->keys[i] = copy(gb->sc->name);
	for ( j=0; j<gb->bcnt; ++j ) {
	    len += gb->bits[j].dlen;
	    if ( gi.psubrs[ gb->bits[j].psub_index ].idx==-1 )
		len += gi.psubrs[ gb->bits[j].psub_index ].len;
	    else {
		struct pschars *subrs = gi.psubrs[gb->bits[j].psub_index].fd==-1 ? glbls : fds[gi.psubrs[gb->bits[j].psub_index].fd].subrs;
		int si = gi.psubrs[ gb->bits[j].psub_index ].idx - subrs->bias;
		/* space for the number (subroutine index) */
		if ( si>=-107 && si<=107 )
		    ++len;
		else if ( si>=-1131 && si<=1131 )
		    len += 2;
		else if ( si>=-32768 && si<=32767 )
		    len += 3;
		else
		    len += 8;
		/* space for the subroutine operator */
		++len;
	    }
	}
	chrs->lens[i] = len+1;
	chrs->values[i] = malloc(len+2); /* space for endchar and a final NUL (which is really meaningless, but makes me feel better) */

	len = 0;
	for ( j=0; j<gb->bcnt; ++j ) {
	    memcpy(chrs->values[i]+len,gb->bits[j].data,gb->bits[j].dlen);
	    len += gb->bits[j].dlen;
	    if ( gi.psubrs[ gb->bits[j].psub_index ].idx==-1 ) {
		memcpy(chrs->values[i]+len,gi.psubrs[ gb->bits[j].psub_index ].data,
			gi.psubrs[ gb->bits[j].psub_index ].len);
		len += gi.psubrs[ gb->bits[j].psub_index ].len;
	    } else {
		struct pschars *subrs = gi.psubrs[gb->bits[j].psub_index].fd==-1 ? glbls : fds[gi.psubrs[gb->bits[j].psub_index].fd].subrs;
		int si = gi.psubrs[ gb->bits[j].psub_index ].idx - subrs->bias;
		/* space for the number (subroutine index) */
		if ( si>=-107 && si<=107 )
		    chrs->values[i][len++] = si+139;
		else if ( si>0 && si<=1131 ) {
		    si-=108;
		    chrs->values[i][len++] = (si>>8)+247;
		    chrs->values[i][len++] = si&0xff;
		} else if ( si>=-1131 && si<0 ) {
		    si=(-si)-108;
		    chrs->values[i][len++] = (si>>8)+251;
		    chrs->values[i][len++] = si&0xff;
		} else if ( si>=-32768 && si<=32767 ) {
		    chrs->values[i][len++] = 28;
		    chrs->values[i][len++] = (si>>8)&0xff;
		    chrs->values[i][len++] = si&0xff;
		} else {
		    /* store as fixed point, then multiply by 64. Takes 8 bytes */
		    si *= (65536/64);
		    chrs->values[i][len++] = '\377';
		    chrs->values[i][len++] = (si>>24)&0xff;
		    chrs->values[i][len++] = (si>>16)&0xff;
		    chrs->values[i][len++] = (si>>8)&0xff;
		    chrs->values[i][len++] = si&0xff;
		    chrs->values[i][len++] = 64 + 139;
		    chrs->values[i][len++] = 0xc; chrs->values[i][len++] = 0x18;	/* Multiply */
		}
		/* space for the subroutine operator */
		if ( gi.psubrs[ gb->bits[j].psub_index ].fd==-1 ) {
		    chrs->values[i][len++] = 29;
		} else
		    chrs->values[i][len++] = 10;
	    }
	}
	chrs->values[i][len++] = 14;	/* endchar */
	chrs->values[i][len] = '\0';
    }
    GIFree(&gi,&dummynotdef);
    *_glbls = glbls;
return( chrs );
}

