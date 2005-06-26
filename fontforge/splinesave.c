/* Copyright (C) 2000-2005 by George Williams */
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
#include <stdio.h>
#include <math.h>
#include "splinefont.h"
#include "psfont.h"
#include <ustring.h>
#include <string.h>
#include <utype.h>
#include <gwidget.h>

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

typedef struct growbuf {
    unsigned char *pt;
    unsigned char *base;
    unsigned char *end;
} GrowBuf;

static void GrowBuffer(GrowBuf *gb) {
    if ( gb->base==NULL ) {
	gb->base = gb->pt = galloc(200);
	gb->end = gb->base + 200;
    } else {
	int len = (gb->end-gb->base) + 400;
	int off = gb->pt-gb->base;
	gb->base = grealloc(gb->base,len);
	gb->end = gb->base + len;
	gb->pt = gb->base+off;
    }
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
return( i );
}

void RefCharsFreeRef(RefChar *ref) {
    RefChar *rnext;

    while ( ref!=NULL ) {
	rnext = ref->next;
	/* don't free the splines */
#ifdef FONTFORGE_CONFIG_TYPE3
	free(ref->layers);
#endif
	chunkfree(ref,sizeof(RefChar));
	ref = rnext;
    }
}

/* ************************************************************************** */
/* ********************** Type1 PostScript CharStrings ********************** */
/* ************************************************************************** */

struct mhlist {
    uint8 mask[12];
    int subr;
    struct mhlist *next;
};

struct hintdb {
    uint8 mask[12];
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
    int cursub;				/* Current subr number */
    BasePoint current;
};

static real myround( real pos, int round ) {
    if ( round )
return( rint( pos ));
    else
return( rint( pos*100. )/100. );
}

static void AddNumber(GrowBuf *gb, real pos, int round) {
    int dodiv = 0;
    int val;
    unsigned char *str;

    if ( gb->pt+8>=gb->end )
	GrowBuffer(gb);

    pos = rint(100*pos)/100;

    if ( !round && pos!=floor(pos)) {
	pos *= 100;
	dodiv = true;
    }
    str = gb->pt;
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
	*str++ = '\377';
	*str++ = (val>>24)&0xff;
	*str++ = (val>>16)&0xff;
	*str++ = (val>>8)&0xff;
	*str++ = val&0xff;
    }
    if ( dodiv ) {
	*str++ = 100+139;	/* 100 */
	*str++ = 12;		/* div (byte1) */
	*str++ = 12;		/* div (byte2) */
    }
    gb->pt = str;
}

/* When doing a multiple master font we have multiple instances of the same data */
/*  which must all be added, and then a call made to the appropriate blend routine */
/* This is complicated because all the data may not fit on the stack so we */
/*  may need to make multiple calls */
static void AddData(GrowBuf *gb, real data[MmMax][6], int instances, int num_coords,
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
    real data[MmMax][6];
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
    real data[MmMax][6];
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
	    if ( hs[i]->backwards ) {
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
    real data[MmMax][6], off;
    int i;

    for ( i=0; i<instance_count; ++i )
	hs[i] = ishstem ? scs[i]->hstem : scs[i]->vstem;

    while ( hs[0]!=NULL ) {
	if ( hs[0]->hintnumber!=-1 &&
		(mask[hs[0]->hintnumber>>3]&(0x80>>(hs[0]->hintnumber&7))) ) {
	    for ( i=0; i<instance_count; ++i ) {
		off = ishstem ? 0 : scs[i]->lsidebearing;
		if ( hs[i]->backwards ) {
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
	subrs->values = grealloc(subrs->values,subrs->cnt*sizeof(uint8 *));
	subrs->lens = grealloc(subrs->lens,subrs->cnt*sizeof(int));
	if ( subrs->keys!=NULL ) {
	    int i;
	    subrs->keys = grealloc(subrs->keys,subrs->cnt*sizeof(char *));
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
    int i;

    for ( mh=hdb->sublist; mh!=NULL; mh=mh->next ) {
	if ( memcmp(mask,mh->mask,sizeof(mask))==0 )
return( mh->subr );
	/* If we find a subr for which we have all the bits set (with extras */
	/*  since we didn't match) then it is safe to replace the old subr */
	/*  with ours. This will save use one subr entry, and maybe a call */
	for ( i=0; i<12; ++i )
	    if ( (mh->mask[i]&mask[i])!=mh->mask[i] )
	break;
	if ( i==12 )
    break;
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
    } else {
	hdb->subrs->values[hdb->subrs->next] = (uint8 *) copyn((char *) gb.base,gb.pt-gb.base);
	hdb->subrs->lens[hdb->subrs->next] = gb.pt-gb.base;

	mh = gcalloc(1,sizeof(struct mhlist));
	memcpy(mh->mask,mask,sizeof(mh->mask));
	mh->subr = hdb->subrs->next++;
	mh->next = hdb->sublist;
	hdb->sublist = mh;
    }
    free(gb.base);

return( mh->subr );
}

static int BuildTranslatedHintSubr(struct pschars *subrs, SplineChar *scs[MmMax],
	RefChar *refs[MmMax], int instance_count, int round) {
    GrowBuf gb;
    real offsets[MmMax];
    SplineChar *rscs[MmMax];
    int j;

    memset(&gb,0,sizeof(gb));
    for ( j=0; j<instance_count; ++j ) {
	offsets[j] = -refs[j]->transform[5];
	rscs[j] = refs[j]->sc;
    }
    CvtPsHints(&gb,rscs,instance_count,true,round,true,offsets);
	    /* I claim to be cjk to avoid getting h/vstem3 */
	    /* which are illegal in hint replacement */
    for ( j=0; j<instance_count; ++j )
	offsets[j] = scs[j]->lsidebearing-refs[j]->transform[4];
    CvtPsHints(&gb,rscs,instance_count,false,round,true,offsets);
    if ( gb.pt+1 >= gb.end )
	GrowBuffer(&gb);
    *gb.pt++ = 11;			/* return */

    SubrsCheck(subrs);
    subrs->values[subrs->next] = (uint8 *) copyn((char *) gb.base,gb.pt-gb.base);
    subrs->lens[subrs->next] = gb.pt-gb.base;
    free(gb.base);
return( subrs->next++ );
}

static void HintSetup(GrowBuf *gb,struct hintdb *hdb, SplinePoint *to,
	int round ) {
    int s;
    int i;

    if ( to->hintmask==NULL )
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

    AddNumber(gb,s,round);
    AddNumber(gb,4,round);		/* subr 4 is (my) magic subr that does the hint subs call */
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *gb->pt++ = 10;			/* callsubr */
    hdb->cursub = s;
}

static void _moveto(GrowBuf *gb,BasePoint *current,BasePoint *to,int instance_count,
	int line, int round, struct hintdb *hdb) {
    BasePoint temp[MmMax];
    int i, samex, samey;
    real data[MmMax][6];

    if ( gb->pt+18 >= gb->end )
	GrowBuffer(gb);

#if 0
    if ( current->x==to->x && current->y==to->y ) {
	/* we're already here */ /* Yes, but sometimes a move is required anyway */
    } else
#endif
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
    } else if ( samey ) {
	for ( i=0; i<instance_count; ++i )
	    data[i][0] = to[i].x-current[i].x;
	AddData(gb,data,instance_count,1,round);
	*(gb->pt)++ = line ? 6 : 22;		/* h move/line to */
    } else {
	for ( i=0; i<instance_count; ++i ) {
	    data[i][0] = to[i].x-current[i].x;
	    data[i][1] = to[i].y-current[i].y;
	}
	AddData(gb,data,instance_count,2,round);
	*(gb->pt)++ = line ? 5 : 21;		/* r move/line to */
    }
    for ( i=0; i<instance_count; ++i )
	current[i] = to[i];
}

static void moveto(GrowBuf *gb,BasePoint *current,Spline *splines[MmMax],
	int instance_count, int line, int round, struct hintdb *hdb) {
    BasePoint to[MmMax];
    int i;

    if ( hdb!=NULL ) HintSetup(gb,hdb,splines[0]->to,round);
    for ( i=0; i<instance_count; ++i )
	to[i] = splines[i]->to->me;
    _moveto(gb,current,to,instance_count,line,round,hdb);
}

static void splmoveto(GrowBuf *gb,BasePoint *current,SplineSet *spl[MmMax],
	int instance_count, int line, int round, struct hintdb *hdb) {
    BasePoint to[MmMax];
    int i;

    if ( hdb!=NULL ) HintSetup(gb,hdb,spl[0]->first,round);
    for ( i=0; i<instance_count; ++i )
	to[i] = spl[i]->first->me;
    _moveto(gb,current,to,instance_count,line,round,hdb);
}

static int AnyRefs(SplineChar *sc) {
    int i;

    for ( i=ly_fore; i<sc->layer_cnt; ++i )
	if ( sc->layers[i].refs!=NULL )
return( true );

return( false );
}

static void refmoveto(GrowBuf *gb,BasePoint *current,BasePoint startstop[MmMax*2],
	int instance_count, int line, int round, struct hintdb *hdb, RefChar *refs[MmMax]) {
    BasePoint to[MmMax];
    int i;

    if ( refs!=NULL && AnyRefs(refs[0]->sc))		/* if it contains references, they must be in exactly the right places */
return;

    for ( i=0; i<instance_count; ++i ) {
	to[i] = startstop[i*2+0];
	if ( refs!=NULL ) {
	    to[i].x += refs[i]->transform[4];
	    to[i].y += refs[i]->transform[5];
	}
    }
    _moveto(gb,current,to,instance_count,line,round,hdb);
}

static void curveto(GrowBuf *gb,BasePoint *current,Spline *splines[MmMax],int instance_count,
	int round, struct hintdb *hdb) {
    BasePoint temp1[MmMax], temp2[MmMax], temp3[MmMax], *c0[MmMax], *c1[MmMax], *s1[MmMax];
    real data[MmMax][6];
    int i, op, opcnt;
    int vh, hv;

    if ( hdb!=NULL ) HintSetup(gb,hdb,splines[0]->to,round);

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
    } else if ( hv ) {
	for ( i=0; i<instance_count; ++i ) {
	    data[i][0] = c0[i]->x-current[i].x;
	    data[i][1] = c1[i]->x-c0[i]->x;
	    data[i][2] = c1[i]->y-c0[i]->y;
	    data[i][3] = s1[i]->y-c1[i]->y;
	}
	op = 31;		/* hvcurveto */
	opcnt = 4;
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
    }
    AddData(gb,data,instance_count,opcnt,false);
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = op;

    for ( i=0; i<instance_count; ++i )
	current[i] = *s1[i];
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

static void flexto(GrowBuf *gb,BasePoint current[MmMax],Spline *pspline[MmMax],
	int instance_count,int round, struct hintdb *hdb) {
    BasePoint *c0, *c1, *mid, *end;
    Spline *nspline;
    BasePoint offsets[MmMax][8];
    int i,j;
    BasePoint temp1, temp2, temp3, temp;
    real data[MmMax][6];

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
	current[j] = *end;
    }

    if ( hdb!=NULL )
	HintSetup(gb,hdb,pspline[0]->to->next->to,round);

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

    *current = *end;
}

static void _CvtPsSplineSet(GrowBuf *gb, SplinePointList *spl[MmMax], int instance_count,
	BasePoint current[MmMax],
	int round, struct hintdb *hdb, BasePoint *start, int is_order2, int stroked ) {
    Spline *spline[MmMax], *first;
    SplinePointList temp[MmMax], *freeme=NULL;
    int init=true;
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
	if ( start==NULL || !init )
	    splmoveto(gb,current,spl,instance_count,false,round,hdb);
	else {
	    if ( hdb!=NULL ) HintSetup(gb,hdb,spl[0]->first,round);
	    for ( i=0; i<instance_count; ++i )
		current[i] = start[2*i] = spl[i]->first->me;
	    init = false;
	}
	for ( i=0; i<instance_count; ++i )
	    spline[i] = spl[i]->first->next;
	while ( spline[0]!=NULL && spline[0]!=first ) {
	    if ( first==NULL ) first = spline[0];
	    if ( SplinesAreFlexible(spline,instance_count) ) {
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
    SplinePointListFree(freeme);
}

static void CvtPsSplineSet(GrowBuf *gb, SplineChar *scs[MmMax], int instance_count,
	BasePoint current[MmMax],
	int round, struct hintdb *hdb, BasePoint *start, int is_order2,
	int stroked ) {
    SplinePointList *spl[MmMax];
    int i;

    for ( i=0; i<instance_count; ++i )
	spl[i] = scs[i]->layers[ly_fore].splines;
    _CvtPsSplineSet(gb,spl,instance_count,current,round,hdb,start,is_order2,stroked);
}

static void CvtPsRSplineSet(GrowBuf *gb, SplineChar *scs[MmMax], int instance_count,
	BasePoint *current,
	int round, struct hintdb *hdb, BasePoint *startend, int is_order2,
	int stroked ) {
    RefChar *refs[MmMax];
    SplineSet *spls[MmMax];
    int i;

    for ( i=0; i<instance_count; ++i )
	refs[i] = scs[i]->layers[ly_fore].refs;
    while ( refs[0]!=NULL ) {
	for ( i=0; i<instance_count; ++i ) {
	    spls[i] = refs[i]->layers[0].splines;
	    refs[i] = refs[i]->next;
	}
	_CvtPsSplineSet(gb,spls,instance_count,current,round,hdb,startend,scs[0]->parent->order2,stroked);
    }
}

static RefChar *IsRefable(RefChar *ref, int isps, real transform[6], RefChar *sofar) {
    real trans[6];
    RefChar *sub;
#ifdef FONTFORGE_CONFIG_TYPE3
    struct reflayer *rl;
#endif

    trans[0] = ref->transform[0]*transform[0] +
		ref->transform[1]*transform[2];
    trans[1] = ref->transform[0]*transform[1] +
		ref->transform[1]*transform[3];
    trans[2] = ref->transform[2]*transform[0] +
		ref->transform[3]*transform[2];
    trans[3] = ref->transform[2]*transform[1] +
		ref->transform[3]*transform[3];
    trans[4] = ref->transform[4]*transform[0] +
		ref->transform[5]*transform[2] +
		transform[4];
    trans[5] = ref->transform[4]*transform[1] +
		ref->transform[5]*transform[3] +
		transform[5];

    if (( isps==1 && ref->adobe_enc!=-1 ) ||
	    (/*isps!=1 &&*/ (ref->sc->layers[ly_fore].splines!=NULL || ref->sc->layers[ly_fore].refs==NULL))) {
	/* If we're in postscript mode and the character we are refering to */
	/*  has an adobe encoding then we are done. */
	/* In TrueType mode, if the character has no refs itself then we are */
	/*  done, but if it has splines as well as refs we are also done */
	/*  because it will have to be dumped out as splines */
	/* Type2 PS (opentype) is the same as truetype here */
	/* Now that I allow refs to be subrs in type1, it also uses the ttf test */
	sub = RefCharCreate();
#ifdef FONTFORGE_CONFIG_TYPE3
	rl = sub->layers;
	*sub = *ref;
	sub->layers = rl;
	*rl = ref->layers[0];
#else
	*sub = *ref;
#endif
	sub->next = sofar;
	/*sub->layers[0].splines = NULL;*/
	memcpy(sub->transform,trans,sizeof(trans));
return( sub );
    } else if ( /* isps &&*/ ( ref->sc->layers[ly_fore].refs==NULL || ref->sc->layers[ly_fore].splines!=NULL) ) {
	RefCharsFreeRef(sofar);
return( NULL );
    }
    for ( sub=ref->sc->layers[ly_fore].refs; sub!=NULL; sub=sub->next ) {
	sofar = IsRefable(sub,isps,trans, sofar);
	if ( sofar==NULL )
return( NULL );
    }
return( sofar );
}
	
static RefChar *reverserefs(RefChar *cur) {
    RefChar *n, *p;

    p = NULL;
    while ( cur!=NULL ) {
	n = cur->next;
	cur->next = p;
	p = cur;
	cur = n;
    }
return( p );
}

/* Postscript can only make refs to things which are in the Adobe encoding */
/*  Suppose Cyrillic A with breve consists of two refs, one to cyrillic A and */
/*  one to nospacebreve (neither in adobe). But if cyrillic A was just a ref */
/*  to A, and if nospacebreve was a ref to breve (both in adobe), then by going*/
/*  one more level of indirection we can use refs in the font. */
/* Apple's docs say TrueType only allows refs to things which are themselves */
/*  simple (ie. contain no refs). So if we use the above example we'd still */
/*  end up with A and breve. I'm told that modern TT doesn't have this */
/*  restriction, but it won't hurt to do things this way */
/* They way I do Type2 postscript I can have as many refs as I like, they */
/*  can only have translations (no scale, skew, rotation), they can't be */
/*  refs themselves. They can't use hintmask commands inside */
/* I'm going to do something similar for Type1s now (use seac if I can, else */
/*  use a subr if I can) */
RefChar *SCCanonicalRefs(SplineChar *sc, int isps) {
    RefChar *ret = NULL, *ref;
    real noop[6];

    /* Neither allows mixing splines and refs */
    if ( sc->layers[ly_fore].splines!=NULL )
return(NULL);
    noop[0] = noop[3] = 1.0; noop[2]=noop[1]=noop[4]=noop[5] = 0;
    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	ret = IsRefable(ref,isps,noop, ret);
	if ( ret==NULL )
return( NULL );
    }

    /* Postscript can only reference things which are translations, no other */
    /*  transformations are allowed. Check for that too. */
    /* My current approach to Type2 fonts requires that the refs have no */
    /*  conflicts */
    /* TrueType can only reference things where the elements of the transform-*/
    /*  ation matrix are less than 2 (and arbetrary translations) */
    for ( ref=ret; ref!=NULL; ref=ref->next ) {
	if ( isps ) {
	    if ( ref->transform[0]!=1 || ref->transform[3]!=1 ||
		    ref->transform[1]!=0 || ref->transform[2]!=0 )
    break;
	    if ( isps==2 && (/*ref->sc->hconflicts || ref->sc->vconflicts ||*/
		    ref->sc->lsidebearing == 0x7fff))
    break;
	} else {
	    if ( ref->transform[0]>=2 || ref->transform[0]<=-2 ||
		    ref->transform[1]>=2 || ref->transform[1]<=-2 ||
		    ref->transform[2]>=2 || ref->transform[2]<=-2 ||
		    ref->transform[3]>=2 || ref->transform[3]<=-2 )
    break;
	}
    }
    if ( ref!=NULL ) {
	RefCharsFreeRef(ret);
return( NULL );
    }
return( reverserefs(ret) );
}

static int TrySubrRefs(GrowBuf *gb, struct pschars *subrs, SplineChar *scs[MmMax],
	int instance_count, int round, int self) {
    RefChar *refs[MmMax];
    BasePoint current[MmMax], *bp;
    DBounds sb, rb;
    int j;

    for ( j=0; j<instance_count; ++j ) {
	SplineCharFindBounds(scs[j],&sb);
	current[j].x = round?rint(sb.minx):sb.minx; current[j].y = 0;
    }
    if ( self ) {
	refmoveto(gb,current,(BasePoint *) (subrs->keys[scs[0]->ttf_glyph]),
		instance_count,false,round,NULL,NULL);
	AddNumber(gb,scs[0]->ttf_glyph,round);
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*gb->pt++ = 10;
return( true );
    } else {
	for ( j=0; j<instance_count; ++j ) {
	    RefChar *r;
	    refs[j] = scs[j]->layers[ly_fore].refs;
	    for ( r=scs[j]->layers[ly_fore].refs; r!=NULL; r=r->next ) {
		if ( r->sc->hconflicts || r->sc->vconflicts || r->sc->anyflexes || AnyRefs(r->sc) )
		    SplineCharFindBounds(r->sc,&rb);
		if ( r->sc->ttf_glyph==0x7fff ||
			(( r->sc->hconflicts || r->sc->vconflicts || r->sc->anyflexes || AnyRefs(r->sc) ) &&
				(r->transform[4]!=0 || r->transform[5]!=0 ||
					current[j].x!=round ? rint(rb.minx) : rb.minx))) {
return( false );
		}
	    }
	}
    }

    /* ok, we should be able to turn it into a series of subr calls */
    /* we need to establish hints for each subr (and remember they may be */
    /*  translated from those normally used by the character) */
    /* Exception: If a reffed char has hint conflicts (and it isn't translated) */
    /*  then its hints are built in */
    /* Then we need to do an rmoveto to do the appropriate translation */
    while ( refs[0]!=NULL ) {
	if ( refs[0]->sc->hconflicts || refs[0]->sc->vconflicts || refs[0]->sc->anyflexes || AnyRefs(refs[0]->sc) )
	    /* Hints already done */;
	else if ( refs[0]->sc->hstem!=NULL || refs[0]->sc->vstem!=NULL ) {
	    int s = BuildTranslatedHintSubr(subrs,scs,refs,instance_count,round);
	    AddNumber(gb,s,round);
	    AddNumber(gb,4,round);		/* subr 4 is (my) magic subr that does the hint subs call */
	    if ( gb->pt+1 >= gb->end )
		GrowBuffer(gb);
	    *gb->pt++ = 10;			/* callsubr */
 
	}
	if ( gb->pt+20>=gb->end )
	    GrowBuffer(gb);
	bp = (BasePoint *) (subrs->keys[refs[0]->sc->ttf_glyph]);
	refmoveto(gb,current,bp,instance_count,false,round,NULL,refs);
	AddNumber(gb,refs[0]->sc->ttf_glyph,round);
	*gb->pt++ = 10;				/* callsubr */
	for ( j=0; j<instance_count; ++j ) {
	    current[j].x = refs[j]->transform[4] + bp[2*j+1].x; current[j].y = refs[j]->transform[5]+bp[2*j+1].y;
	    refs[j] = refs[j]->next;
	}
    }

return( true );
}

static int IsPSRefable(SplineChar *sc) {
    RefChar *ref;

    if ( sc->layers[ly_fore].refs==NULL || sc->layers[ly_fore].splines!=NULL )
return( false );

    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[0]!=1 ||
		ref->transform[1]!=0 ||
		ref->transform[2]!=0 ||
		ref->transform[3]!=1 )
return( false );
    }
return( true );
}

static RefChar *RefFindAdobe(RefChar *r, RefChar *t) {
    *t = *r;
    while ( t->adobe_enc==-1 && t->sc->layers[ly_fore].refs!=NULL &&
	    t->sc->layers[ly_fore].refs->next==NULL &&
	    t->sc->layers[ly_fore].splines==NULL &&
	    t->sc->layers[ly_fore].refs->transform[0]==1.0 &&
	    t->sc->layers[ly_fore].refs->transform[1]==0.0 &&
	    t->sc->layers[ly_fore].refs->transform[2]==0.0 &&
	    t->sc->layers[ly_fore].refs->transform[3]==1.0 ) {
	t->transform[4] += t->sc->layers[ly_fore].refs->transform[4];
	t->transform[5] += t->sc->layers[ly_fore].refs->transform[5];
	t->adobe_enc = t->sc->layers[ly_fore].refs->adobe_enc;
	t->orig_pos = t->sc->layers[ly_fore].refs->orig_pos;
	t->sc = t->sc->layers[ly_fore].refs->sc;
    }
return( t );
}

static int IsSeacable(GrowBuf *gb, struct pschars *subrs, SplineChar *scs[MmMax],
    int instance_count, int round, int dontseac) {
    /* can be at most two chars in a seac (actually must be exactly 2, but */
    /*  I'll put in a space if there's only one */
    RefChar *r1, *r2, *rt, *refs;
    RefChar space, t1, t2;
    DBounds b;
    int i, j, swap;
    real data[MmMax][6];

    if ( scs[0]->ttf_glyph!=0x7fff )
return( TrySubrRefs(gb,subrs,scs,instance_count,round,true));

    for ( j=0 ; j<instance_count; ++j )
	if ( !IsPSRefable(scs[j]))
return( false );

    refs = scs[0]->layers[ly_fore].refs;
    if ( refs==NULL )
return( false );

    r1 = refs;
    if ((r2 = r1->next)==NULL ) {
	RefChar *refs = r1->sc->layers[ly_fore].refs;
	if ( refs!=NULL && refs->next!=NULL && refs->next->next==NULL &&
		r1->sc->layers[ly_fore].splines==NULL &&
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
	    if ( space.sc->layers[ly_fore].splines!=NULL || space.sc->layers[ly_fore].refs!=NULL )
		r2 = NULL;
	}
    } else if ( r2->next!=NULL )
	r2 = NULL;

    /* check for something like "AcyrillicBreve" which has a ref to Acyril */
    /*  (which doesn't have an adobe enc) which in turn has a ref to A (which */
    /*  does) */
    if ( r2!=NULL ) {
	if ( r1->adobe_enc==-1 )
	    r1 = RefFindAdobe(r1,&t1);
	if ( r2->adobe_enc==-1 )
	    r2 = RefFindAdobe(r2,&t2);
    }

/* CID fonts have no encodings. So we can't use seac to reference characters */
/*  in them. The other requirements are just those of seac */
    if ( dontseac || r2==NULL ||
	    r1->adobe_enc==-1 ||
	    r2->adobe_enc==-1 ||
	    ((r1->transform[4]!=0 || r1->transform[5]!=0 || r1->sc->width!=scs[0]->width ) &&
		 (r2->transform[4]!=0 || r2->transform[5]!=0 || r2->sc->width!=scs[0]->width)) )
return( TrySubrRefs(gb,subrs,scs,instance_count,round,false));

    swap = false;
    if ( r1->transform[4]!=0 || r1->transform[5]!=0 ) {
	rt = r1; r1 = r2; r2 = rt;
	swap = !swap;
    }
    if ( r1->sc->width!=scs[0]->width && r2->sc->width==scs[0]->width &&
	    r2->transform[4]==0 && r2->transform[5]==0 ) {
	rt = r1; r1 = r2; r2 = rt;
	swap = !swap;
    }
    if ( r1->sc->width!=scs[0]->width || r1->transform[4]!=0 || r1->transform[5]!=0 )
return( false );

    for ( j=0; j<instance_count; ++j ) {
	SplineChar *r2sc = scs[j]->parent->glyphs[r2->sc->orig_pos];
	RefChar *r3, t3;

	SplineCharFindBounds(r2sc,&b);
	if ( scs[j]->layers[ly_fore].refs!=NULL && scs[j]->layers[ly_fore].refs->next==NULL )
	    r3 = r2;		/* Space, not offset */
	else if ( swap )
	    r3 = RefFindAdobe(scs[j]->layers[ly_fore].refs,&t3);
	else
	    r3 = RefFindAdobe(scs[j]->layers[ly_fore].refs->next,&t3);

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

static int _SCNeedsSubsPts(SplineChar *sc) {
    RefChar *ref;

    if ( sc->hstem==NULL && sc->vstem==NULL )
return( false );

    if ( sc->layers[ly_fore].splines!=NULL )
return( sc->layers[ly_fore].splines->first->hintmask==NULL );

    for ( ref = sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
	if ( ref->layers[0].splines!=NULL )
return( ref->layers[0].splines->first->hintmask==NULL );

return( false );		/* It's empty. that's easy. */
}

static int SCNeedsSubsPts(SplineChar *sc,enum fontformat format) {
    if ( (format!=ff_mma && format!=ff_mmb) || sc->parent->mm==NULL ) {
	if ( !sc->hconflicts && !sc->vconflicts )
return( false );		/* No conflicts, no swap-over points needed */
return( _SCNeedsSubsPts(sc));
    } else {
	MMSet *mm = sc->parent->mm;
	int i;
	for ( i=0; i<mm->instance_count; ++i ) if ( sc->orig_pos<mm->instances[i]->glyphcnt ) {
	    if ( _SCNeedsSubsPts(mm->instances[i]->glyphs[sc->orig_pos]) )
return( true );
	}
return( false );
    }
}

static unsigned char *SplineChar2PS(SplineChar *sc,int *len,int round,int iscjk,
	struct pschars *subrs,BasePoint *startend,int flags,enum fontformat format) {
    DBounds b;
    GrowBuf gb;
    BasePoint current[MmMax];
    unsigned char *ret;
    struct hintdb hintdb, *hdb=NULL;
    StemInfo *oldh[MmMax], *oldv[MmMax];
    int hc[MmMax], vc[MmMax];
    int instance_count, i;
    SplineChar *scs[MmMax];
    real data[MmMax][6];
    MMSet *mm = sc->parent->mm;
    HintMask *hm[MmMax];
    int fixuphm = false;

    if ( !(flags&ps_flag_nohints) && SCNeedsSubsPts(sc,format))
	SCFigureHintMasks(sc);

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
	if ( scs[0]->layers[ly_fore].splines!=NULL && i==instance_count ) {	/* No conflicts */
	    fixuphm = true;
	    for ( i=0; i<instance_count; ++i ) {
		hm[i] = scs[i]->layers[ly_fore].splines->first->hintmask;
		scs[i]->layers[ly_fore].splines->first->hintmask = NULL;
	    }
	}
    }

    memset(&gb,'\0',sizeof(gb));
    memset(current,'\0',sizeof(current));
    for ( i=0; i<instance_count; ++i ) {
	SplineCharFindBounds(scs[i],&b);
	scs[i]->lsidebearing = current[i].x = round?rint(b.minx):b.minx;
	data[i][0] = b.minx;
	data[i][1] = scs[i]->width;
    }
    if ( startend==NULL ) {
	AddData(&gb,data,instance_count,2,round);
	*gb.pt++ = 13;				/* hsbw, lbearing & width */
    }
    NumberHints(scs,instance_count);

    /* If this char is being placed in a subroutine, then we don't want to */
    /*  use seac because somebody is going to call that subroutine and */
    /*  add another reference to it later. CID keyed fonts also can't use */
    /*  seac (they have no encoding so it doesn't work), that's what iscjk&0x100 */
    /*  tests for */
    if ( IsSeacable(&gb,subrs,scs,instance_count,round,startend!=NULL || (iscjk&0x100))) {
	/* All Done */;
	/* All should share the same refs, so all should be seac-able if one is */
    } else {
	iscjk &= ~0x100;
	hdb = NULL;
	if ( iscjk && instance_count==1 )
	    CounterHints1(&gb,sc,round);	/* Must come immediately after hsbw */
	if ( !scs[0]->vconflicts && !scs[0]->hconflicts && instance_count==1 ) {
	    CvtPsHints(&gb,scs,instance_count,true,round,iscjk,NULL);
	    CvtPsHints(&gb,scs,instance_count,false,round,iscjk,NULL);
	} else {
	    memset(&hintdb,0,sizeof(hintdb));
	    hintdb.subrs = subrs; hintdb.iscjk = iscjk; hintdb.scs = scs;
	    hintdb.instance_count = instance_count;
	    hdb = &hintdb;
	}
	CvtPsSplineSet(&gb,scs,instance_count,current,round,hdb,startend,sc->parent->order2,sc->parent->strokedfont);
	CvtPsRSplineSet(&gb,scs,instance_count,current,round,hdb,NULL,sc->parent->order2,sc->parent->strokedfont);
    }
    if ( gb.pt+1>=gb.end )
	GrowBuffer(&gb);
    *gb.pt++ = startend==NULL ? 14 : 11;	/* endchar / return */
    ret = (unsigned char *) copyn((char *) gb.base,gb.pt-gb.base);
    *len = gb.pt-gb.base;
    if ( hdb!=NULL ) {
	struct mhlist *mh, *mhnext;
	for ( mh=hdb->sublist; mh!=NULL; mh=mhnext ) {
	    mhnext = mh->next;
	    free(mh);
	}
    }
    free(gb.base);
    if ( startend!=NULL )
	for ( i=0; i<instance_count; ++i )
	    startend[2*i+1] = current[i];
    if ( flags&ps_flag_nohints ) {
	for ( i=0; i<instance_count; ++i ) {
	    scs[i]->hstem = oldh[i]; scs[i]->vstem = oldv[i] = scs[i]->vstem;
	    scs[i]->hconflicts = hc[i]; scs[i]->vconflicts = vc[i];
	}
    } else if ( fixuphm ) {
	for ( i=0; i<instance_count; ++i )
	    scs[i]->layers[ly_fore].splines->first->hintmask = hm[i];
    }
return( ret );
}

static void CvtSimpleHints(GrowBuf *gb, SplineChar *sc,BasePoint *startend,
	int round, enum fontformat format,int iscjk) {
    MMSet *mm = sc->parent->mm;
    int i,instance_count;
    SplineChar *scs[MmMax];
    BasePoint current[MmMax];
    DBounds b;

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
	/* I don't think I can put counter hints into a type1 subroutine */
	/*  but this would be the natural place to do it if I could */
	/*  (counter hints must come immediately after the hbsw, and we've */
	/*  already seen the procedure call to invoke us */
	/* if ( iscjk ) CounterHints1(&gb,sc,round);*/
    }
    for ( i=0; i<instance_count; ++i ) {
	SplineCharFindBounds(scs[i],&b);
	scs[i]->lsidebearing = round?rint(b.minx):b.minx;
    }
    CvtPsHints(gb,scs,instance_count,true,round,iscjk,NULL);
    CvtPsHints(gb,scs,instance_count,false,round,iscjk,NULL);
    for ( i=0; i<instance_count; ++i )
	scs[i]->lsidebearing = 0;
    memset(current,0,instance_count*sizeof(BasePoint));
    CvtPsSplineSet(gb,scs,instance_count,current,round,NULL,startend,
	    sc->parent->order2,sc->parent->strokedfont);
    for ( i=0; i<instance_count; ++i )
	startend[2*i+1] = current[i];
}

static int AlwaysSeacable(SplineChar *sc) {
    struct splinecharlist *d;
    RefChar *r;
    int iscid = sc->parent->cidmaster!=NULL;

    for ( d=sc->dependents; d!=NULL; d = d->next ) {
	if ( d->sc->layers[ly_fore].splines!=NULL )	/* I won't deal with things with both splines and refs. */
    continue;				/*  skip it */
	for ( r=d->sc->layers[ly_fore].refs; r!=NULL; r=r->next ) {
	    if ( r->transform[0]!=1 || r->transform[1]!=0 ||
		    r->transform[2]!=0 || r->transform[3]!=1 )
	break;				/* Can't deal with it either way */
	}
	if ( r!=NULL )		/* Bad transform matrix */
    continue;			/* Can't handle either way, skip */

	for ( r=d->sc->layers[ly_fore].refs; r!=NULL; r=r->next ) {
	    if ( iscid || r->adobe_enc==-1 )
return( false );			/* not seacable, but could go in subr */
	}
	r = d->sc->layers[ly_fore].refs;
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
	for ( r=d->sc->layers[ly_fore].refs; r!=NULL; r=r->next )
	    if ( r->sc == sc && r->transform[4]==0 && r->transform[5]==0 )
return( true );
    }
return( false );
}

static void SplineFont2Subrs1(SplineFont *sf,int round, int iscjk,
	struct pschars *subrs,int flags,enum fontformat format) {
    int i,j,cnt, anydone;
    SplineChar *sc;
    uint8 *temp;
    int len;
    GrowBuf gb;
    MMSet *mm = (format==ff_mma || format==ff_mmb) ? sf->mm : NULL;
    int instance_count = mm!=NULL ? mm->instance_count : 1;

    for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL )
	sc->ttf_glyph = 0x7fff;
    anydone = true;
    while ( anydone ) {
	anydone = false;
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    if ( !SCWorthOutputting(sc) || sc->ttf_glyph!=0x7fff )
	continue;
	    /* Put the */
	    /*  character into a subr if it is referenced by other characters */
	    if ( (sc->dependents!=NULL &&
		     ((!sc->hconflicts && !sc->vconflicts && !sc->anyflexes) ||
			 SpecialCaseConflicts(sc)) &&
		     !AlwaysSeacable(sc))) {
		BasePoint *bp;
		RefChar *r;

		for ( r=sc->layers[ly_fore].refs; r!=NULL; r=r->next )
		    if ( r->sc->ttf_glyph==0x7fff )
		break;
		if ( r!=NULL )	/* Contains a reference to something which is not in a sub itself */
	continue;

		bp = gcalloc(2*instance_count,sizeof(BasePoint));
		    /* This contains the start and stop points of the splinesets */
		    /* we need this later when we invoke the subrs */

		/* None of the generated subrs starts with a moveto operator */
		/*  The invoker is expected to move to the appropriate place */
		if ( sc->hconflicts || sc->vconflicts || sc->anyflexes || AnyRefs(sc) ) {
		    temp = SplineChar2PS(sc,&len,round,iscjk,subrs,bp,flags,format);
		} else {
		    memset(&gb,'\0',sizeof(gb));
		    for ( j=0; j<instance_count; ++j )
			bp[j*2+1] = bp[j*2+0];
		    CvtSimpleHints(&gb,sc,bp,round,format,iscjk);
		    if ( gb.pt+1>=gb.end )
			GrowBuffer(&gb);
		    *gb.pt++ = 11;				/* return */
		    temp = (unsigned char *) copyn((char *) gb.base,gb.pt-gb.base);
		    len = gb.pt-gb.base;
		    free(gb.base);
		}

		SubrsCheck(subrs);
		subrs->values[subrs->next] = temp;
		subrs->lens[subrs->next] = len;
		if ( subrs->keys==NULL )
		    subrs->keys = gcalloc(subrs->cnt,sizeof(char *));
		subrs->keys[subrs->next] = (void *) bp;
		sc->ttf_glyph = subrs->next++;
		anydone = true;
	    }
	}
    }
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

struct pschars *SplineFont2Chrs(SplineFont *sf, int iscjk,
	struct pschars *subrs,int flags, enum fontformat format) {
    struct pschars *chrs = gcalloc(1,sizeof(struct pschars));
    int i, cnt, instance_count;
    int fixed;
    int notdef_pos;
    MMSet *mm = sf->mm;
    real data[MmMax][6];
    GrowBuf gb;
    int round = (flags&ps_flag_round)? true : false;

    if ( (format==ff_mma || format==ff_mmb) && mm!=NULL ) {
	instance_count = mm->instance_count;
	sf = mm->instances[0];
	fixed = 0;
	for ( i=0; i<instance_count; ++i ) {
	    data[i][0] = fixed = SFOneWidth(mm->instances[i]);
	    if ( fixed==-1 )
	break;
	}
	if ( fixed==-1 ) {
	    for ( i=0; i<instance_count; ++i )
		data[i][0] = mm->instances[i]->ascent+mm->instances[i]->descent;
	}
    } else {
	data[0][0] = fixed = SFOneWidth(sf);
	if ( fixed == -1 )
	    data[0][0] = sf->ascent+sf->descent;
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

    chrs->cnt = cnt;
    chrs->keys = galloc(cnt*sizeof(char *));
    chrs->lens = galloc(cnt*sizeof(int));
    chrs->values = galloc(cnt*sizeof(unsigned char *));

    SplineFont2Subrs1(sf,round,iscjk,subrs,flags,format);

    cnt = 0;
    chrs->keys[0] = copy(".notdef");
    if ( notdef_pos==-1 ) {
	memset(&gb,'\0',sizeof(gb));
	GrowBuffer(&gb);
	*gb.pt++ = '\213';		/* 0 */	/* left side bearing */
	AddData(&gb,data,instance_count,1,round);
	if ( gb.pt+4>gb.end )
	    GrowBuffer(&gb);
	*gb.pt++ = '\015';	/* hsbw */
	*gb.pt++ = '\016';	/* endchar */
	*gb.pt = '\0';
	chrs->values[0] = (unsigned char *) copyn((char *) gb.base,gb.pt-gb.base);	/* 0 <w> hsbw endchar */
	chrs->lens[0] = gb.pt-gb.base;
	cnt = 1;
    }
    for ( i=0 ; i<sf->glyphcnt; ++i ) {
#if HANYANG
	if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->compositionunit )
	    /* don't output it, should be in a subroutine */;
	else
#endif
	if ( SCWorthOutputting(sf->glyphs[i]) &&
		( i==notdef_pos || strcmp(sf->glyphs[i]->name,".notdef")!=0)) {
	    chrs->keys[cnt] = copy(sf->glyphs[i]->name);
	    chrs->values[cnt] = SplineChar2PS(sf->glyphs[i],&chrs->lens[cnt],
		    round,iscjk,subrs,NULL,flags,format);
	    ++cnt;
	}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( !GProgressNext()) {
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( !gwwv_progress_next()) {
#endif
	    PSCharsFree(chrs);
return( NULL );
	}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
    chrs->next = cnt;
    if ( chrs->next>chrs->cnt )
	IError("Character estimate failed, about to die..." );
return( chrs );
}

struct pschars *CID2Chrs(SplineFont *cidmaster,struct cidbytes *cidbytes,int flags) {
    struct pschars *chrs = gcalloc(1,sizeof(struct pschars));
    int i, cnt, cid;
    char notdefentry[20];
    SplineFont *sf = NULL;
    struct fddata *fd;
    int round = (flags&ps_flag_round)? true : false;
    /* I don't support mm cid files. I don't think adobe does either */

    cnt = 0;
    for ( i=0; i<cidmaster->subfontcnt; ++i )
	if ( cnt<cidmaster->subfonts[i]->glyphcnt )
	    cnt = cidmaster->subfonts[i]->glyphcnt;
    cidbytes->cidcnt = cnt;

    for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	fd = &cidbytes->fds[i];
	SplineFont2Subrs1(sf,round,fd->iscjk|0x100,fd->subrs,flags,ff_cid);
    }

    chrs->cnt = cnt;
    chrs->lens = galloc(cnt*sizeof(int));
    chrs->values = galloc(cnt*sizeof(unsigned char *));
    cidbytes->fdind = galloc(cnt*sizeof(unsigned char *));

    for ( cid = 0; cid<cnt; ++cid ) {
	sf = NULL;
	for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	    sf = cidmaster->subfonts[i];
	    if ( cid<sf->glyphcnt && SCWorthOutputting(sf->glyphs[cid]) )
	break;
	}
	if ( cid!=0 && i==cidmaster->subfontcnt ) {
	    chrs->lens[cid] = 0;
	    chrs->values[cid] = NULL;
	    cidbytes->fdind[cid] = -1;		/* that's what Adobe uses */
	} else if ( i==cidmaster->subfontcnt ) {
	    /* Must have something for .notdef */
	    /* sf corresponds to cidmaster->subfontcnt-1, generally the kanji font */
	    int w = sf->ascent+sf->descent; char *pt = notdefentry;
	    *pt++ = '\213';		/* 0 */
	    if ( w>=-107 && w<=107 )
		*pt++ = w+139;
	    else if ( w>=108 && w<=1131 ) {
		w -= 108;
		*pt++ = (w>>8)+247;
		*pt++ = w&0xff;
	    } else if ( w>=-1131 && w<=-108 ) {
		w = -w;
		w -= 108;
		*pt++ = (w>>8)+251;
		*pt++ = w&0xff;
	    } else {
		*pt++ = '\377';
		*pt++ = (w>>24)&0xff;
		*pt++ = (w>>16)&0xff;
		*pt++ = (w>>8)&0xff;
		*pt++ = w&0xff;
	    }
	    *pt++ = '\015';	/* hsbw */
	    *pt++ = '\016';	/* endchar */
	    *pt = '\0';
	    chrs->values[0] = (unsigned char *) copyn(notdefentry,pt-notdefentry);	/* 0 <w> hsbw endchar */
	    chrs->lens[0] = pt-notdefentry;
	    cidbytes->fdind[0] = cidmaster->subfontcnt-1;
	} else {
	    fd = &cidbytes->fds[i];
	    cidbytes->fdind[cid] = i;
	    chrs->values[cid] = SplineChar2PS(sf->glyphs[cid],&chrs->lens[cid],
		    round,fd->iscjk|0x100,fd->subrs,NULL,flags,ff_cid);
	}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( !GProgressNext()) {
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( !gwwv_progress_next()) {
#endif
	    PSCharsFree(chrs);
return( NULL );
	}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
    chrs->next = cid;
return( chrs );
}

/* ************************************************************************** */
/* ********************** Type2 PostScript CharStrings ********************** */
/* ************************************************************************** */

#if 0
static int real_warn = false;
#endif

static real myround2(real pos, int round) {
    if ( round )
return( rint(pos));

return( rint(65536*pos)/65536 );
}

static void AddNumber2(GrowBuf *gb, real pos, int round) {
    int val;
    unsigned char *str;

    if ( gb->pt+5>=gb->end )
	GrowBuffer(gb);

    pos = rint(65536*pos)/65536;
    if ( round )
	pos = rint(pos);
    if ( pos>65535 || pos<-65536 ) {
	fprintf( stderr, "Number out of range: %g in type2 output (must be [-65536,65535])\n",
		pos );
	if ( pos>0 ) pos = 65535; else pos = -65536;
    }

    str = gb->pt;
    if ( pos!=floor(pos )) {
#if 0
	if ( !real_warn ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetPostNoticeR(_STR_NotIntegral,_STR_TryRoundToInt);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_notice(_("Not integral"),_("This font contains at least one non-integral coordinate.\nThis is perfectly legal, but it is unusual and does\nincrease the size of the generated font file. You might\nwant to try using\n  Element->Round To Int\non the entire font."));
#endif
	    real_warn = true;
	}
#endif

	val = pos*65536;
#ifdef PSFixed_Is_TTF	/* The type2 spec is contradictory. It says this is a */
			/*  two's complement number, but it also says it is a */
			/*  Fixed, which in truetype is not two's complement */
			/*  (mantisa is always unsigned) */
	if ( val<0 ) {
	    int mant = (-val)&0xffff;
	    val = (val&0xffff0000) | mant;
	}
#endif
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

static int HintSetup2(GrowBuf *gb,struct hintdb *hdb, SplinePoint *to ) {

    /* We might get a point with a hintmask in a glyph with no conflicts */
    /* (ie. the initial point when we return to it at the end of the splineset*/
    /* in that case hdb->cnt will be 0 and we should ignore it */
    /* components in subroutines depend on not having any hintmasks */
    if ( to->hintmask==NULL || hdb->cnt==0 )
return( false );

    if ( memcmp(hdb->mask,*to->hintmask,(hdb->cnt+7)/8)==0 )
return( false );

    AddMask2(gb,*to->hintmask,hdb->cnt,19);		/* hintmask */
    memcpy(hdb->mask,*to->hintmask,sizeof(HintMask));
return( true );
}

static void moveto2(GrowBuf *gb,struct hintdb *hdb,SplinePoint *to, int round) {
    BasePoint temp, *tom;

    if ( gb->pt+18 >= gb->end )
	GrowBuffer(gb);

    HintSetup2(gb,hdb,to);
    tom = &to->me;
    if ( round ) {
	temp.x = rint(tom->x);
	temp.y = rint(tom->y);
	tom = &temp;
    }
#if 0
    if ( hdb->current.x==tom->x && hdb->current.y==tom->y ) {
	/* we're already here */
	/* Yes, but a move is required anyway at char start */
    } else
#endif
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
    hdb->current = *tom;
}

static Spline *lineto2(GrowBuf *gb,struct hintdb *hdb,Spline *spline, Spline *done, int round) {
    int cnt, hv, hvcnt;
    Spline *test, *lastgood, *lasthvgood;
    BasePoint temp1, temp2, *tom, *fromm;
    int donehm;

    for ( test=spline, cnt=0; test->knownlinear && cnt<15; ) {
	++cnt;
	lastgood = test;
	test = test->to->next;
	if ( test==done || test==NULL )
    break;
    }

    HintSetup2(gb,hdb,spline->to);

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
		hdb->current = *tom;
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
	hdb->current = *tom;
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
    BasePoint start;
    int donehm;

    HintSetup2(gb,hdb,spline->to);

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

    HintSetup2(gb,hdb,nspline->to);

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

    hdb->current = *end;
}

static void CvtPsSplineSet2(GrowBuf *gb, SplinePointList *spl,
	struct hintdb *hdb, BasePoint *start,int is_order2,int round) {
    Spline *spline, *first;
    SplinePointList temp, *freeme = NULL;
    int init = true, unhinted = true;;

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
	if ( start==NULL || !init ) {
	    if ( unhinted && hdb->cnt>0 && spl->first->hintmask!=NULL ) {
		hdb->mask[0] = ~(*spl->first->hintmask)[0];	/* Make it different */
		unhinted = false;
	    }
	    moveto2(gb,hdb,spl->first,round);
	} else {
	    hdb->current = *start = spl->first->me;
	    init = false;
	}
	for ( spline = spl->first->next; spline!=NULL && spline!=first; ) {
	    if ( first==NULL ) first = spline;
	    if ( spline->to->flexx || spline->to->flexy ) {
		flexto2(gb,hdb,spline,round);	/* does two adjacent splines */
		spline = spline->to->next->to->next;
	    } else if ( spline->knownlinear )
		spline = lineto2(gb,hdb,spline,first,round);
	    else
		spline = curveto2(gb,hdb,spline,first,round);
	}
	/* No closepath oper in type2 fonts */
	SplineSetReverse(spl);
	/* Of course, I have to Reverse again to get back to my convention after*/
	/*  saving */
    }
    SplinePointListFree(freeme);
}

static StemInfo *OrderHints(StemInfo *mh, StemInfo *h, real offset, real otheroffset, int mask) {
    StemInfo *prev, *new, *test;

    while ( h!=NULL ) {
	prev = NULL;
	for ( test=mh; test!=NULL && (h->start+offset>test->start ||
		(h->start+offset==test->start && h->width<test->width)); test=test->next )
	    prev = test;
	if ( test!=NULL && test->start==h->start+offset && test->width==h->width )
	    test->u.mask |= mask;		/* Actually a bit */
	else {
	    new = chunkalloc(sizeof(StemInfo));
	    new->next = test;
	    new->start = h->start+offset;
	    new->width = h->width;
	    new->where = HICopyTrans(h->where,1,otheroffset);
	    new->u.mask = mask;
	    if ( prev==NULL )
		mh = new;
	    else
		prev->next = new;
	}
	h = h->next;
    }
return( mh );
}

static void DumpHints(GrowBuf *gb,StemInfo *h,int oper,int midoper,int round) {
    real last = 0, cur;
    int cnt;

    if ( h==NULL )
return;
    cnt = 0;
    while ( h!=NULL && h->hintnumber!=-1 ) {
	/* Type2 hints do not support negative widths except in the case of */
	/*  ghost (now called edge) hints */
	if ( cnt>24-1 ) {	/* stack max = 48 numbers, => 24 hints, leave a bit of slop for the width */
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
	    if ( h->width==20 ) {
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

static void DumpHintMasked(GrowBuf *gb,RefChar *cur,StemInfo *h,StemInfo *v) {
    uint8 masks[12];
    int cnt;

    if ( h==NULL && v==NULL )
	IError("hintmask invoked when there are no hints");
    memset(masks,'\0',sizeof(masks));
    cnt = 0;
    while ( h!=NULL && h->hintnumber>=0 ) {
	if ( h->u.mask&cur->adobe_enc )
	    masks[h->hintnumber>>3] |= 0x80>>(h->hintnumber&7);
	h = h->next; ++cnt;
    }
    while ( v!=NULL && v->hintnumber>=0 ) {
	if ( v->u.mask&cur->adobe_enc )
	    masks[v->hintnumber>>3] |= 0x80>>(v->hintnumber&7);
	v = v->next; ++cnt;
    }
    AddMask2(gb,masks,cnt,19);		/* hintmask */
}

static void ExpandRefList2(GrowBuf *gb, SplineChar *sc, RefChar *refs, RefChar *unsafe,
	struct pschars *subrs, int round) {
    RefChar *r;
    int hsccnt=0;
    BasePoint current, *bpt;
    StemInfo *h=NULL, *v=NULL, *s;
    int cnt;
    /* The only refs I deal with have no hint conflicts within them */
    /* Unless unsafe is set. In which case it will contain conflicts and */
    /*  none of the other refs will contain any hints at all */

    memset(&current,'\0',sizeof(current));

    if ( unsafe!=NULL ) {
	/* All the hints live in this one reference, and its subroutine */
	/*  contains them and it has its own rmoveto */
	if ( unsafe->sc->lsidebearing==0x7fff )
	    IError("Attempt to reference an unreferenceable glyph %s", unsafe->sc->name );
	AddNumber2(gb,unsafe->sc->lsidebearing,round);
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*gb->pt++ = 10;					/* callsubr */
	hsccnt = 0;
	bpt = (BasePoint *) (subrs->keys[unsafe->sc->lsidebearing+subrs->bias]);
	current = bpt[1];
    } else {
	for ( r=refs; r!=NULL; r=r->next )
	    if ( r->sc->hstem!=NULL || r->sc->vstem!=NULL )
		r->adobe_enc = 1<<hsccnt++;
	    else
		r->adobe_enc = 0;
    
	for ( r=refs, h=v=NULL; r!=NULL; r=r->next ) {
	    h = OrderHints(h,r->sc->hstem,r->transform[5],r->transform[4],r->adobe_enc);
	    v = OrderHints(v,r->sc->vstem,r->transform[4],r->transform[5],r->adobe_enc);
	}
	for ( s=h, cnt=0; s!=NULL; s=s->next, ++cnt )
	    s->hintnumber = cnt>=HntMax?-1:cnt;
	for ( s=v; s!=NULL; s=s->next, ++cnt )
	    s->hintnumber = cnt>=HntMax?-1:cnt;
    
	if ( h!=NULL )
	    DumpHints(gb,h,hsccnt>1?18:1,hsccnt>1?18:1,round);	/* hstemhm/hstem */
	if ( v!=NULL )
	    DumpHints(gb,v, hsccnt<=1 ? 3:-1, hsccnt<=1 ? 3:23,round);	/* vstem */
	CounterHints2(gb,sc,cnt);			/* Precedes first hintmask */
    }

    for ( r=refs; r!=NULL; r=r->next ) if ( r!=unsafe ) {
	if ( hsccnt>1 )
	    DumpHintMasked(gb,r,h,v);
	/* Translate from end of last character to where this one should */
	/*  start (we must have one moveto operator to start off, none */
	/*  in the subr) */
	bpt = (BasePoint *) (subrs->keys[r->sc->lsidebearing+subrs->bias]);
	if ( current.x!=bpt[0].x+r->transform[4] )
	    AddNumber2(gb,bpt[0].x+r->transform[4]-current.x,round);
	if ( current.y!=bpt[0].y+r->transform[5] || current.x==bpt[0].x+r->transform[4] )
	    AddNumber2(gb,bpt[0].y+r->transform[5]-current.y,round);
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*gb->pt++ = current.x==bpt[0].x+r->transform[4]?4:	/* vmoveto */
		    current.y==bpt[0].y+r->transform[5]?22:	/* hmoveto */
		    21;						/* rmoveto */
	if ( r->sc->lsidebearing==0x7fff )
	    IError("Attempt to reference an unreferenceable glyph %s", r->sc->name );
	AddNumber2(gb,r->sc->lsidebearing,round);
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*gb->pt++ = 10;					/* callsubr */
	current.x = bpt[1].x+r->transform[4];
	current.y = bpt[1].y+r->transform[5];
    }
    StemInfosFree(h); StemInfosFree(v);
}

static int IsRefable2(GrowBuf *gb, SplineChar *sc, struct pschars *subrs,int round) {
    RefChar *refs, *r, *unsafe=NULL;
    int allsafe=true, unsafecnt=0, allwithouthints=true, ret;

    refs = SCCanonicalRefs(sc,2);
    if ( refs==NULL )
return( false );
    for ( r=refs; r!=NULL; r=r->next ) {
	if ( r->sc->hconflicts || r->sc->vconflicts ) {
	    ++unsafecnt;
	    allsafe = false;
	    unsafe = r;
	} else if ( r->sc->hstem!=NULL || r->sc->vstem!=NULL )
	    allwithouthints = false;
    }
    if ( allsafe || ( unsafecnt==1 && allwithouthints )) {
	ExpandRefList2(gb,sc,refs,unsafe,subrs,round);
	ret = true;
    } else
	ret = false;
    RefCharsFreeRef(refs);
return( ret );
}

static unsigned char *SplineChar2PSOutline2(SplineChar *sc,int *len,
	BasePoint *startend, int flags ) {
    GrowBuf gb;
    struct hintdb hdb;
    RefChar *rf;
    unsigned char *ret;
    StemInfo *oldh, *oldv;
    int hc, vc;
    SplineChar *scs[MmMax];
    int round = (flags&ps_flag_round)? true : false;
    HintMask *hm = NULL;

    if ( flags&ps_flag_nohints ) {
	oldh = sc->hstem; oldv = sc->vstem;
	hc = sc->hconflicts; vc = sc->vconflicts;
	sc->hstem = NULL; sc->vstem = NULL;
	sc->hconflicts = false; sc->vconflicts = false;
    } else if ( sc->layers[ly_fore].splines!=NULL && !sc->vconflicts &&
	    !sc->hconflicts ) {
	hm = sc->layers[ly_fore].splines->first->hintmask;
	sc->layers[ly_fore].splines->first->hintmask = NULL;
    }

    memset(&gb,'\0',sizeof(gb));
    memset(&hdb,'\0',sizeof(hdb));
    scs[0] = sc;
    hdb.scs = scs;

    CvtPsSplineSet2(&gb,sc->layers[ly_fore].splines,&hdb,startend,sc->parent->order2,round);
    for ( rf = sc->layers[ly_fore].refs; rf!=NULL; rf = rf->next )
	CvtPsSplineSet2(&gb,rf->layers[0].splines,&hdb,NULL,sc->parent->order2,round);
    if ( gb.pt+1>=gb.end )
	GrowBuffer(&gb);
    *gb.pt++ = 11;				/* return */
    ret = (unsigned char *) copyn((char *) gb.base,gb.pt-gb.base);
    *len = gb.pt-gb.base;
    free(gb.base);
    startend[1] = hdb.current;
    if ( flags&ps_flag_nohints ) {
	sc->hstem = oldh; sc->vstem = oldv;
	sc->hconflicts = hc; sc->vconflicts = vc;
    } else if ( hm!=NULL )
	sc->layers[ly_fore].splines->first->hintmask = hm;
return( ret );
}

static unsigned char *SplineChar2PS2(SplineChar *sc,int *len, int nomwid,
	int defwid, struct pschars *subrs, BasePoint *startend, int flags ) {
    GrowBuf gb;
    RefChar *rf;
    unsigned char *ret;
    struct hintdb hdb;
    StemInfo *oldh, *oldv;
    int hc, vc;
    SplineChar *scs[MmMax];
    int round = (flags&ps_flag_round)? true : false;
    HintMask *hm = NULL;

    if ( autohint_before_generate && sc->changedsincelasthinted &&
	    !sc->manualhints && !(flags&ps_flag_nohints))
	SplineCharAutoHint(sc,NULL);
    if ( !(flags&ps_flag_nohints) && SCNeedsSubsPts(sc,ff_otf))
	SCFigureHintMasks(sc);

    if ( flags&ps_flag_nohints ) {
	oldh = sc->hstem; oldv = sc->vstem;
	hc = sc->hconflicts; vc = sc->vconflicts;
	sc->hstem = NULL; sc->vstem = NULL;
	sc->hconflicts = false; sc->vconflicts = false;
    } else if ( sc->layers[ly_fore].splines!=NULL && !sc->vconflicts &&
	    !sc->hconflicts ) {
	hm = sc->layers[ly_fore].splines->first->hintmask;
	sc->layers[ly_fore].splines->first->hintmask = NULL;
    }

    memset(&gb,'\0',sizeof(gb));

    GrowBuffer(&gb);
    if ( startend==NULL ) {
	/* store the width on the stack */
	if ( sc->width==defwid )
	    /* Don't need to do anything for the width */;
	else
	    AddNumber2(&gb,sc->width-nomwid,round);
    }
    if ( IsRefable2(&gb,sc,subrs,round))
	/* All Done */;
    else if ( startend==NULL && sc->lsidebearing!=0x7fff ) {
	RefChar refs;
	memset(&refs,'\0',sizeof(refs));
	refs.sc = sc;
	refs.transform[0] = refs.transform[3] = 1.0;
	ExpandRefList2(&gb,sc,&refs,sc->hconflicts||sc->vconflicts?&refs:NULL,subrs,round);
    } else {
	memset(&hdb,'\0',sizeof(hdb));
	hdb.scs = scs;
	scs[0] = sc;
	hdb.noconflicts = !sc->hconflicts && !sc->vconflicts;
	hdb.cnt = NumberHints(hdb.scs,1);
	DumpHints(&gb,sc->hstem,sc->hconflicts || sc->vconflicts?18:1,
				sc->hconflicts || sc->vconflicts?18:1,round);
	DumpHints(&gb,sc->vstem,sc->hconflicts || sc->vconflicts?-1:3,
				sc->hconflicts || sc->vconflicts?23:3,round);
	CounterHints2(&gb, sc, hdb.cnt );
	CvtPsSplineSet2(&gb,sc->layers[ly_fore].splines,&hdb,NULL,sc->parent->order2,round);
	for ( rf = sc->layers[ly_fore].refs; rf!=NULL; rf = rf->next )
	    CvtPsSplineSet2(&gb,rf->layers[0].splines,&hdb,NULL,sc->parent->order2,round);
    }
    if ( gb.pt+1>=gb.end )
	GrowBuffer(&gb);
    *gb.pt++ = startend==NULL?14:11;		/* endchar / return */
    if ( startend!=NULL )
	startend[1] = hdb.current;
    ret = (unsigned char *) copyn((char *) gb.base,gb.pt-gb.base);
    *len = gb.pt-gb.base;
    free(gb.base);
    if ( flags&ps_flag_nohints ) {
	sc->hstem = oldh; sc->vstem = oldv;
	sc->hconflicts = hc; sc->vconflicts = vc;
    } else if ( hm!=NULL )
	sc->layers[ly_fore].splines->first->hintmask = hm;
return( ret );
}

/* This char has hint conflicts. Check to see if we can put it into a subr */
/*  in spite of that. If there is at least one dependent character which: */
/*	refers to us without translating us */
/*	and all its other refs contain no hints at all */
static int Type2SpecialCase(SplineChar *sc) {
    struct splinecharlist *d;
    RefChar *r;

    for ( d=sc->dependents; d!=NULL; d=d->next ) {
	for ( r=d->sc->layers[ly_fore].refs; r!=NULL; r = r->next ) {
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

struct pschars *SplineFont2Subrs2(SplineFont *sf,int flags) {
    struct pschars *subrs = gcalloc(1,sizeof(struct pschars));
    int i,cnt;
    SplineChar *sc;

    /*real_warn = false;*/

    /* We don't allow refs to refs. It confuses the hintmask operators */
    /*  instead we track down to the base ref */
    for ( i=cnt=0; i<sf->glyphcnt; ++i ) {
	sc = sf->glyphs[i];
	if ( autohint_before_generate && sc!=NULL &&
		sc->changedsincelasthinted && !sc->manualhints &&
		!(flags&ps_flag_nohints))
	    SplineCharAutoHint(sc,NULL);
	if ( sc==NULL )
	    /* Do Nothing */;
	else if ( SCWorthOutputting(sc) &&
	    (( sc->layers[ly_fore].refs==NULL && sc->dependents!=NULL &&
		    ( (!sc->hconflicts && !sc->vconflicts) ||
			Type2SpecialCase(sc)) )  )) {
	/* Put the */
	/*  character into a subr if it is referenced by other characters */
	    ++cnt;
	    sc->lsidebearing = 0;	/* anything other than 0x7fff */
	} else
	    sc->lsidebearing = 0x7fff;
    }

    subrs->cnt = cnt;
    if ( cnt==0 )
return( subrs);
    subrs->lens = galloc(cnt*sizeof(int));
    subrs->values = galloc(cnt*sizeof(unsigned char *));
    subrs->keys = galloc(cnt*sizeof(BasePoint *));
    subrs->bias = cnt<1240 ? 107 :
		  cnt<33900 ? 1131 : 32768;

    for ( i=cnt=0; i<sf->glyphcnt; ++i ) {
	sc = sf->glyphs[i];
	if ( sc==NULL )
	    /* Do Nothing */;
	else if ( sc->lsidebearing!=0x7fff ) {
	    subrs->keys[cnt] = gcalloc(2,sizeof(BasePoint));
	    if (( !sc->hconflicts && !sc->vconflicts ) || (flags&ps_flag_nohints))
		subrs->values[cnt] = SplineChar2PSOutline2(sc,&subrs->lens[cnt],
			(BasePoint *) (subrs->keys[cnt]),flags);
	    else
		subrs->values[cnt] = SplineChar2PS2(sc,&subrs->lens[cnt],0,0,
			subrs,(BasePoint *) (subrs->keys[cnt]),flags);
	    sc->lsidebearing = cnt++ - subrs->bias;
	}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( !GProgressNext()) {
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( !gwwv_progress_next()) {
#endif
	    PSCharsFree(subrs);
return( NULL );
	}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
    subrs->next = cnt;
return( subrs );
}
    
struct pschars *SplineFont2Chrs2(SplineFont *sf, int nomwid, int defwid,
	struct pschars *subrs,int flags,const int *bygid, int cnt) {
    struct pschars *chrs = gcalloc(1,sizeof(struct pschars));
    int i;
    char notdefentry[20];
    SplineChar *sc;
    int fixed = SFOneWidth(sf), notdefwidth;

    /* real_warn = false; */ /* Should have been done by SplineFont2Subrs2 */

    notdefwidth = fixed;
    if ( notdefwidth==-1 ) notdefwidth = sf->ascent+sf->descent;

/* only honor the width on .notdef in non-fixed pitch fonts (or ones where there is an actual outline in notdef) */
    chrs->cnt = cnt;
    chrs->lens = galloc(cnt*sizeof(int));
    chrs->values = galloc(cnt*sizeof(unsigned char *));

    /* notdef needs to be glyph 0 in an sfnt */
    if ( bygid[0]!=-1 ) {
	chrs->values[0] = SplineChar2PS2(sf->glyphs[bygid[0]],&chrs->lens[0],nomwid,defwid,subrs,NULL,flags);
    } else {
	char *pt = notdefentry;
	if ( notdefwidth==defwid )
	    /* Don't need to specify it */;
	else {
	    notdefwidth -= nomwid;
	    if ( notdefwidth>=-107 && notdefwidth<=107 )
		*pt++ = notdefwidth+139;
	    else if ( notdefwidth>=108 && notdefwidth<=1131 ) {
		notdefwidth -= 108;
		*pt++ = (notdefwidth>>8)+247;
		*pt++ = notdefwidth&0xff;
	    } else if ( notdefwidth>=-1131 && notdefwidth<=-108 ) {
		notdefwidth = -notdefwidth;
		notdefwidth -= 108;
		*pt++ = (notdefwidth>>8)+251;
		*pt++ = notdefwidth&0xff;
	    } else {
		*pt++ = 28;
		*pt++ = (notdefwidth>>8)&0xff;
		*pt++ = notdefwidth&0xff;
	    }
	}
	*pt++ = '\016';	/* endchar */
	*pt = '\0';
	chrs->values[0] = (unsigned char *) copyn(notdefentry,pt-notdefentry);	/* 0 <w> hsbw endchar */
	chrs->lens[0] = pt-notdefentry;
    }
    for ( i=1 ; i<cnt; ++i ) {
	sc = sf->glyphs[bygid[i]];
	chrs->values[i] = SplineChar2PS2(sc,&chrs->lens[i],nomwid,defwid,subrs,NULL,flags);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( !GProgressNext()) {
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( !gwwv_progress_next()) {
#endif
	    PSCharsFree(chrs);
return( NULL );
	}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
    chrs->next = cnt;
return( chrs );
}
    
struct pschars *CID2Chrs2(SplineFont *cidmaster,struct fd2data *fds,int flags) {
    struct pschars *chrs = gcalloc(1,sizeof(struct pschars));
    int i, cnt, cid, max;
    char notdefentry[20];
    SplineChar *sc;
    SplineFont *sf = NULL;
    /* In a cid-keyed font, cid 0 is defined to be .notdef so there are no */
    /*  special worries. If it is defined we use it. If it is not defined */
    /*  we add it. */

    /* real_warn = false; */

    max = 0;
    for ( i=0; i<cidmaster->subfontcnt; ++i )
	if ( max<cidmaster->subfonts[i]->glyphcnt )
	    max = cidmaster->subfonts[i]->glyphcnt;
    cnt = 1;			/* for .notdef */
    for ( cid = 1; cid<max; ++cid ) {
	for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	    sf = cidmaster->subfonts[i];
	    if ( cid<sf->glyphcnt && SCWorthOutputting(sf->glyphs[cid])) {
		++cnt;
	break;
	    }
	}
    }

    chrs->cnt = cnt;
    chrs->lens = galloc(cnt*sizeof(int));
    chrs->values = galloc(cnt*sizeof(unsigned char *));

    for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	for ( cid=0; cid<sf->glyphcnt; ++cid ) if ( sf->glyphs[cid]!=NULL )
	    sf->glyphs[cid]->ttf_glyph = -1;
    }

    for ( cid = cnt = 0; cid<max; ++cid ) {
	sf = NULL;
	for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	    sf = cidmaster->subfonts[i];
	    if ( cid<sf->glyphcnt && SCWorthOutputting(sf->glyphs[cid]) )
	break;
	}
	if ( cid!=0 && i==cidmaster->subfontcnt ) {
	    /* Do nothing */;
	} else if ( i==cidmaster->subfontcnt ) {
	    /* They didn't define CID 0 */
	    /* Place it in the final subfont (which is what sf points to) */
	    int w = sf->ascent+sf->descent; char *pt = notdefentry;
	    --i;
	    if ( w==fds[i].defwid )
		/* Don't need to specify it */;
	    else {
		w -= fds[i].nomwid;
		if ( w>=-107 && w<=107 )
		    *pt++ = w+139;
		else if ( w>=108 && w<=1131 ) {
		    w -= 108;
		    *pt++ = (w>>8)+247;
		    *pt++ = w&0xff;
		} else if ( w>=-1131 && w<=-108 ) {
		    w = -w;
		    w -= 108;
		    *pt++ = (w>>8)+251;
		    *pt++ = w&0xff;
		} else {
		    *pt++ = 28;
		    *pt++ = (w>>8)&0xff;
		    *pt++ = w&0xff;
		}
	    }
	    *pt++ = '\016';	/* endchar */
	    *pt = '\0';
	    chrs->values[0] = (unsigned char *) copyn(notdefentry,pt-notdefentry);	/* 0 <w> hsbw endchar */
	    chrs->lens[0] = pt-notdefentry;
	    ++cnt;
#if 0 && HANYANG			/* Too much stuff knows the glyph cnt, can't refigure it here at the end */
	} else if ( sf->glyphs[cid]->compositionunit ) {
	    /* don't output it, should be in a subroutine */;
#endif
	} else {
	    sc = sf->glyphs[cid];
	    chrs->values[cnt] = SplineChar2PS2(sc,&chrs->lens[cnt],
		    fds[i].nomwid,fds[i].defwid,fds[i].subrs,NULL,flags);
	    sc->ttf_glyph = cnt++;
	}
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressNext();
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next();
#endif
    }
    chrs->next = cnt;
return( chrs );
}
