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
#include "pfaedit.h"
#include <stdio.h>
#include <math.h>
#include "splinefont.h"
#include "psfont.h"
#include "ustring.h"
#include "string.h"
#include "utype.h"

typedef struct growbuf {
    unsigned char *pt;
    unsigned char *base;
    unsigned char *end;
} GrowBuf;

static void GrowBuffer(GrowBuf *gb) {
    if ( gb->base==NULL ) {
	gb->base = gb->pt = malloc(200);
	gb->end = gb->base + 200;
    } else {
	int len = (gb->end-gb->base) + 400;
	int off = gb->pt-gb->base;
	gb->base = realloc(gb->base,len);
	gb->end = gb->base + len;
	gb->pt = gb->base+off;
    }
}

static int NumberHints(SplineChar *sc) {
    int i;
    StemInfo *s;

    for ( s=sc->hstem, i=0; s!=NULL; s=s->next ) {
	if ( i<96 )
	    s->hintnumber = i++;
	else
	    s->hintnumber = -1;
    }
    for ( s=sc->vstem; s!=NULL; s=s->next ) {
	if ( i<96 )
	    s->hintnumber = i++;
	else
	    s->hintnumber = -1;
    }
return( i );
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
    SplineChar *sc;
    unsigned int iscjk: 1;		/* If cjk then don't do stem3 hints */
					/* Will be done with counters instead */
	/* actually, most of the time we can't use stem3s, only if those three*/
	/* stems are always active and there are no other stems !(h/v)hasoverlap*/
    unsigned int noconflicts: 1;
    int cursub;				/* Current subr number */
    BasePoint current;
};

static void AddNumber(GrowBuf *gb, double pos, int round) {
    int dodiv = 0;
    int val;
    unsigned char *str;

    if ( gb->pt+8>=gb->end )
	GrowBuffer(gb);

    pos = rint(64*pos)/64;

    if ( !round && pos!=floor(pos)) {
	pos *= 64;
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
	*str++ = 64+139;	/* 64 */
	*str++ = 12;		/* div (byte1) */
	*str++ = 12;		/* div (byte2) */
    }
    gb->pt = str;
}

static int CvtPsStem3(GrowBuf *gb, StemInfo *h1, StemInfo *h2, StemInfo *h3, double off,
	int ishstem, int round) {
    StemInfo _h1, _h2, _h3;

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
    if ( gb->pt+51>=gb->end )
	GrowBuffer(gb);
    AddNumber(gb,h1->start-off,round);
    AddNumber(gb,h1->width,round);
    AddNumber(gb,h2->start-off,round);
    AddNumber(gb,h2->width,round);
    AddNumber(gb,h3->start-off,round);
    AddNumber(gb,h3->width,round);
    *(gb->pt)++ = 12;
    *(gb->pt)++ = ishstem?2:1;				/* h/v stem3 */
return( true );
}

static void HintDirection(StemInfo *h) {
    StemInfo *t, *u;

    for ( t=h; t!=NULL; t=t->next ) {
	t->backwards = false;
	for ( u=t->next; u!=NULL && u->start<t->start+t->width; u = u->next )
	    if ( t->start+t->width == u->start+u->width )
		t->backwards = u->backwards = true;
    }
    for ( t=h; t!=NULL; t=t->next ) {
	for ( u=t->next; u!=NULL && u->start<t->start+t->width; u = u->next )
	    if ( t->start == u->start )
		t->backwards = u->backwards = false;
    }
}

static void CvtPsHints(GrowBuf *gb, StemInfo *h, double off, int ishstem,
	int round, int iscjk ) {

    if ( h!=NULL && h->next!=NULL && h->next->next!=NULL &&
	    h->next->next->next==NULL )
	if ( !iscjk && CvtPsStem3(gb, h, h->next, h->next->next, off, ishstem, round))
return;

    while ( h!=NULL ) {
	if ( h->backwards ) {
	    AddNumber(gb,h->start-off+h->width,round);
	    AddNumber(gb,-h->width,round);
	} else {
	    AddNumber(gb,h->start-off,round);
	    AddNumber(gb,h->width,round);
	}
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*(gb->pt)++ = ishstem?1:3;			/* h/v stem */
	h = h->next;
    }
}

static void CvtPsMasked(GrowBuf *gb, StemInfo *h, double off, int ishstem,
	int round, uint8 mask[12] ) {

    while ( h!=NULL ) {
	if ( h->hintnumber!=-1 &&
		(mask[h->hintnumber>>3]&(0x80>>(h->hintnumber&7))) ) {
	    if ( h->backwards ) {
		AddNumber(gb,h->start-off+h->width,round);
		AddNumber(gb,-h->width,round);
	    } else {
		AddNumber(gb,h->start-off,round);
		AddNumber(gb,h->width,round);
	    }
	    if ( gb->pt+1>=gb->end )
		GrowBuffer(gb);
	    *(gb->pt)++ = ishstem?1:3;			/* h/v stem */
	}
	h = h->next;
    }
}

/* find all the other stems (after main) which seem to form a counter group */
/*  with main. That is their stems have a considerable overlap (in the other */
/*  coordinate) with main */
static int stemmatches(StemInfo *main) {
    StemInfo *last=main, *test;
    double mlen, olen;
    int cnt;

    cnt = 1;		/* for the main stem */
    main->tobeused = true;
    mlen = HIlen(main);
    for ( test=main->next; test!=NULL; test=test->next )
	test->tobeused = false;
    for ( test=main->next; test!=NULL; test=test->next ) {
	if ( test->used || last->start+last->width>test->start || test->hintnumber==-1 )
    continue;
	olen = HIoverlap(main->where,test->where);
	if ( olen>mlen/3 && olen>HIlen(test)/3 ) {
	    test->tobeused = true;
	    ++cnt;
	}
    }
return( cnt );
}

static int FigureCounters(StemInfo *stems,double *hints,int base,double offset ) {
    StemInfo *h, *first;
    int pos = base+1, cnt=0;
    double last = offset;

    for ( h=stems; h!=NULL ; h=h->next )
	h->used = false;
    while ( stems!=NULL ) {
	for ( first=stems; first!=NULL && first->used; first = first->next );
	if ( first==NULL )
    break;
	if ( first->where==NULL || first->hintnumber==-1 || stemmatches(first)<=2 ) {
	    first->used = true;
	    stems = first->next;
    continue;
	}
	for ( h = first; h!=NULL; h = h->next ) {
	    if ( h->tobeused ) {
		h->used = true;
		hints[pos++] = h->start-last;
		hints[pos++] = h->width;
		last = h->start+h->width;
	    }
	}
	hints[pos-2] += hints[pos-1];
	hints[pos-1] = -hints[pos-1];		/* Mark end of group */
	last = offset;				/* Each new group starts at 0 or lbearing */
	stems = first->next;
	++cnt;
    }
    hints[base] = cnt;
return( pos );
}

static void CounterHints1(GrowBuf *gb, SplineChar *sc, int round) {
    double hints[96*2+2];		/* At most 96 hints, no hint used more than once */
    int pos, i, j;

    pos = FigureCounters(sc->hstem,hints,0,0);
    /* Adobe's docs (T1_Supp.pdf, section 2.4) say these should be offset from*/
    /*  the left side bearing. The example (T1_Supp.pdf, 2.6) shows them offset*/
    /*  from 0. I've no idea which is correct, so I'll follow the words, think-*/
    /*  that the lbearing might have been set to 0 even though it shouldn't */
    /*  have been. */
    pos = FigureCounters(sc->vstem,hints,pos,sc->lsidebearing);
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

static StemInfo *OnHHint(SplinePoint *sp, StemInfo *s) {
    StemInfo *possible=NULL;
    HintInstance *hi;

    for ( ; s!=NULL; s=s->next ) {
	if ( sp->me.y<s->start )
return( possible );
	if ( s->start==sp->me.y || s->start+s->width==sp->me.y ) {
	    if ( !s->hasconflicts )
return( s );
	    for ( hi=s->where; hi!=NULL; hi=hi->next ) {
		if ( hi->begin<=sp->me.x && hi->end>=sp->me.x )
return( s );
	    }
	    possible = s;
	}
    }
return( possible );
}

static StemInfo *OnVHint(SplinePoint *sp, StemInfo *s) {
    StemInfo *possible=NULL;
    HintInstance *hi;

    for ( ; s!=NULL; s=s->next ) {
	if ( sp->me.x<s->start )
return( possible );
	if ( s->start==sp->me.x || s->start+s->width==sp->me.x ) {
	    if ( !s->hasconflicts )
return( s );
	    for ( hi=s->where; hi!=NULL; hi=hi->next ) {
		if ( hi->begin<=sp->me.y && hi->end>=sp->me.y )
return( s );
	    }
	    possible = s;
	}
    }
return( possible );
}

/* Does h have a conflict with any of the stems in the list which have bits */
/*  set in the mask */
static int ConflictsWithMask(StemInfo *stems, uint8 mask[12],StemInfo *h) {
    while ( stems!=NULL && stems->start<h->start+h->width ) {
	if ( stems->start+stems->width>=h->start && stems!=h ) {
	    if ( stems->hintnumber!=-1 &&
		    (mask[stems->hintnumber>>3]&(0x80>>(stems->hintnumber&7))) )
return( true );
	}
	stems = stems->next;
    }
return( false );
}

/* Does the hintmask we need already exist in a subroutine? if so return that */
/*  subr. Else build a new subr with the hints we need. Note we can only use */
/*  *stem3 commands if there are no conflicts in that coordinate, it isn't cjk*/
/*  and all the other conditions are met */
static int FindOrBuildSubr(struct hintdb *hdb, uint8 mask[12], int round) {
    struct mhlist *mh;
	/* <subr number presumed to be on stack> 1 3 callother pop callsubr */
    static uint8 special[] = { 1+139, 3+139, 12, 16, 12, 17, 10, 11 };
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
    if ( hdb->subrs->next+(hdb->subrs->next==4)>=hdb->subrs->cnt ) {
	hdb->subrs->cnt += 100;
	hdb->subrs->values = grealloc(hdb->subrs->values,hdb->subrs->cnt*sizeof(uint8 *));
	hdb->subrs->lens = grealloc(hdb->subrs->lens,hdb->subrs->cnt*sizeof(int));
    }
    if ( hdb->subrs->next==4 ) {
	/* Add a special subr that invokes othersubr 3 */
	hdb->subrs->values[4] = (uint8 *) copyn( (char *) special, sizeof(special));
	hdb->subrs->lens[4] = sizeof(special);
	++hdb->subrs->next;
    }

    memset(&gb,0,sizeof(gb));
    if ( !hdb->sc->hconflicts )
	CvtPsHints(&gb,hdb->sc->hstem,0,true,round,hdb->iscjk);
    else
	CvtPsMasked(&gb,hdb->sc->hstem,0,true,round,mask);
    if ( !hdb->sc->vconflicts )
	CvtPsHints(&gb,hdb->sc->vstem,hdb->sc->lsidebearing,false,round,hdb->iscjk);
    else
	CvtPsMasked(&gb,hdb->sc->vstem,hdb->sc->lsidebearing,false,round,mask);
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

static int NeedsNewHintMask(struct hintdb *hdb, SplinePoint *to ) {
    StemInfo *h=NULL, *v=NULL;

    if ( hdb==NULL || hdb->noconflicts )
return(false);

    /* Does this point lie on any hints? */
    if ( hdb->sc->hconflicts )
	h = OnHHint(to,hdb->sc->hstem);
    if ( hdb->sc->vconflicts )
	v = OnVHint(to,hdb->sc->vstem);

    /* Nothing to set, or already set */
    if ( (h==NULL || h->hintnumber==-1 || (hdb->mask[h->hintnumber>>3]&(0x80>>(h->hintnumber&7)))) &&
	 (v==NULL || v->hintnumber==-1 || (hdb->mask[v->hintnumber>>3]&(0x80>>(v->hintnumber&7)))) )
return(false);

return( true );
}

static int FigureHintMask(struct hintdb *hdb, SplinePoint *to, uint8 mask[12] ) {
    StemInfo *h=NULL, *v=NULL, *s;
    SplinePoint *tsp;
    double lastx, lasty;
    SplineChar *sc;
    HintInstance *hi;

    if ( hdb==NULL || hdb->noconflicts )
return(false);
    sc = hdb->sc;

    /* Does this point lie on any hints? */
    if ( hdb->sc->hconflicts )
	h = OnHHint(to,sc->hstem);
    if ( hdb->sc->vconflicts )
	v = OnVHint(to,sc->vstem);

    /* Nothing to set, or already set */
    if ( (h==NULL || h->hintnumber==-1 || (hdb->mask[h->hintnumber>>3]&(0x80>>(h->hintnumber&7)))) &&
	 (v==NULL || v->hintnumber==-1 || (hdb->mask[v->hintnumber>>3]&(0x80>>(v->hintnumber&7)))) )
return(false);

    memset(mask,'\0',sizeof(uint8 [12]));
    /* Install all hints that are always active */
    for ( s=sc->hstem; s!=NULL; s=s->next )
	if ( s->hintnumber!=-1 && !s->hasconflicts )
	    mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));
    for ( s=sc->vstem; s!=NULL; s=s->next )
	if ( s->hintnumber!=-1 && !s->hasconflicts )
	    mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));

    /* Install the hints we think we need for this point */
    if ( h!=NULL )
	mask[h->hintnumber>>3] |= (0x80>>(h->hintnumber&7));
    if ( v!=NULL )
	mask[v->hintnumber>>3] |= (0x80>>(v->hintnumber&7));
    
    if ( hdb->sc->hconflicts ) {
	/* Install all hints that should be active when the minor coord is that */
	/*  of this point. So horizontal hints become active if the x coord matches */
	for ( s=sc->hstem; s!=NULL; s=s->next )
	    if ( s->hintnumber!=-1 && s->hasconflicts &&
		    !ConflictsWithMask(sc->hstem,mask,s)) {
		for ( hi=s->where; hi!=NULL; hi=hi->next )
		    if ( hi->begin<=to->me.x && hi->end>=to->me.x ) {
			mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));
		break;
		    }
	    }
    }
    if ( hdb->sc->vconflicts ) {
	/* Same for v. So vertical hints become active if the y coord matches */
	for ( s=sc->vstem; s!=NULL; s=s->next )
	    if ( s->hintnumber!=-1 && s->hasconflicts &&
		    ConflictsWithMask(sc->vstem,mask,s)) {
		for ( hi=s->where; hi!=NULL; hi=hi->next )
		    if ( hi->begin<=to->me.y && hi->end>=to->me.y ) {
			mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));
		break;
		    }
	    }
    }
    if ( hdb->sc->hconflicts || hdb->sc->vconflicts ) {
	lastx = to->me.x; lasty = to->me.y;
	/* Now walk along the splineset adding what hints we can (so we don't */
	/*  have to call another subroutine on the next point. Give up when we*/
	/*  get a conflict */
	if ( to->next!=NULL )
	for ( tsp=to->next->to; tsp!=to; tsp = tsp->next->to ) {
	    if ( hdb->sc->hconflicts && tsp->me.y!=lasty ) {
		h = OnHHint(tsp,sc->hstem);
		if ( h!=NULL && h->hintnumber!=-1 && !(mask[h->hintnumber>>3]&(0x80>>(h->hintnumber&7))) ) {
		    if ( h->hasconflicts && ConflictsWithMask(sc->hstem,mask,h))
	break;
		    mask[h->hintnumber>>3] |= (0x80>>(h->hintnumber&7));
		}
	    }
	    if ( hdb->sc->vconflicts && tsp->me.x!=lastx ) {
		h = OnVHint(tsp,sc->vstem);
		if ( h!=NULL && h->hintnumber!=-1 && !(mask[h->hintnumber>>3]&(0x80>>(h->hintnumber&7))) ) {
		    if ( h->hasconflicts && ConflictsWithMask(sc->vstem,mask,h))
	break;
		    mask[h->hintnumber>>3] |= (0x80>>(h->hintnumber&7));
		}
	    }
	    if ( tsp->next==NULL )
	break;
	}
    }
return( true );
}

static void HintSetup(GrowBuf *gb,struct hintdb *hdb, SplinePoint *to, int round ) {
    uint8 mask[12];
    int s;

    if ( !FigureHintMask(hdb, to, mask ) )
return;

    s = FindOrBuildSubr(hdb,mask,round);
    if ( hdb->cursub == s ) {			/* If we were able to redefine */
	memcpy(hdb->mask,mask,sizeof(mask));	/* the subroutine currently */
return;						/* active then we are done */
    }

    AddNumber(gb,s,round);
    AddNumber(gb,4,round);		/* subr 4 is (my) magic subr that does the hint subs call */
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *gb->pt++ = 10;			/* callsubr */
    memcpy(hdb->mask,mask,sizeof(mask));
    hdb->cursub = s;
}

/* We always want to start out with some set of hints, for those interpreters */
/*  which can't deal with hint substitution. I will put all hints with no */
/*  conflicts, and pick (arbetrarily) the first hint out of any conflict group*/
/* A more efficient approach would be for this routine to pick the first pt */
/*  on a hint and call HintSetup with it, that way the hints will be right */
/*  for the first point and we won't need a subr call when we get to it */
static int InitialHintMask(struct hintdb *hdb, uint8 mask[12] ) {
    StemInfo *s=NULL;
    SplineChar *sc = hdb->sc;
    double max;
    SplineSet *spl;
    SplinePoint *sp;

    if ( sc->hstem==NULL && sc->vstem==NULL )
return(false);		/* Hint setup is trivial with no hints */

    for ( spl=sc->splines; spl!=NULL; spl=spl->next ) {
	for ( sp = spl->first; ; ) {
	    if ( OnHHint(sp,sc->hstem)!=NULL || OnVHint(sp,sc->vstem)!=NULL )
return( FigureHintMask(hdb, sp, mask ) );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    memset(mask,'\0',sizeof(uint8 [12]));
    for ( s=sc->hstem; s!=NULL; ) {
	if ( s->hintnumber==-1 )
    break;
	mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));
	max = s->start+s->width;
	for ( s=s->next; s!=NULL && s->start<max; s=s->next )
	    if ( s->start+s->width>max ) max = s->start+s->width;
    }
    for ( s=sc->vstem; s!=NULL; ) {
	if ( s->hintnumber==-1 )
    break;
	mask[s->hintnumber>>3] |= (0x80>>(s->hintnumber&7));
	max = s->start+s->width;
	for ( s=s->next; s!=NULL && s->start<max; s=s->next )
	    if ( s->start+s->width>max ) max = s->start+s->width;
    }
return( true );
}

static void InitialHintSetup(GrowBuf *gb,struct hintdb *hdb, int round ) {
    uint8 mask[12];

    if ( !InitialHintMask(hdb,mask))
return;

#if 0		/* Everybody else seems to put the initial hints in line, but a subr call seems to work and (potentially) saves space if we can reuse the subr */
    if ( !hdb->sc->hconflicts )
	CvtPsHints(gb,hdb->sc->hstem,0,true,round,hdb->iscjk);
    else
	CvtPsMasked(gb,hdb->sc->hstem,0,true,round,mask);
    if ( !hdb->sc->vconflicts )
	CvtPsHints(gb,hdb->sc->vstem,hdb->sc->lsidebearing,false,round,hdb->iscjk);
    else
	CvtPsMasked(gb,hdb->sc->vstem,hdb->sc->lsidebearing,false,round,mask);
#else
    AddNumber(gb,FindOrBuildSubr(hdb,mask,round),round);
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *gb->pt++ = 10;			/* callsubr */
#endif

    memcpy(hdb->mask,mask,sizeof(mask));
}

static void moveto(GrowBuf *gb,BasePoint *current,SplinePoint *_to,
	int line, int round, struct hintdb *hdb) {
    BasePoint temp, *to = &_to->me;

    if ( hdb!=NULL ) HintSetup(gb,hdb,_to,round);

    if ( gb->pt+18 >= gb->end )
	GrowBuffer(gb);

#if 0
    if ( current->x==to->x && current->y==to->y ) {
	/* we're already here */ /* Yes, but sometimes a move is required anyway */
    } else
#endif
    if ( current->x==to->x ) {
	AddNumber(gb,to->y-current->y,round);
	*(gb->pt)++ = line ? 7 : 4;		/* v move/line to */
    } else if ( current->y==to->y ) {
	AddNumber(gb,to->x-current->x,round);
	*(gb->pt)++ = line ? 6 : 22;		/* h move/line to */
    } else {
	AddNumber(gb,to->x-current->x,round);
	AddNumber(gb,to->y-current->y,round);
	*(gb->pt)++ = line ? 5 : 21;		/* r move/line to */
    }
    if ( round ) {
	temp.x = rint(to->x);
	temp.y = rint(to->y);
	to = &temp;
    }
    *current = *to;
}

static void curveto(GrowBuf *gb,BasePoint *current,Spline *spline,
	int round, struct hintdb *hdb) {
    BasePoint temp1, temp2, temp3, *c0, *c1, *s1;

    if ( hdb!=NULL ) HintSetup(gb,hdb,spline->to,round);

    if ( gb->pt+50 >= gb->end )
	GrowBuffer(gb);

    c0 = &spline->from->nextcp;
    c1 = &spline->to->prevcp;
    s1 = &spline->to->me;
    if ( round ) {
	temp1.x = rint(c0->x);
	temp1.y = rint(c0->y);
	c0 = &temp1;
	temp2.x = rint(c1->x);
	temp2.y = rint(c1->y);
	c1 = &temp2;
	temp3.x = rint(s1->x);
	temp3.y = rint(s1->y);
	s1 = &temp3;
	round = false;
    }
    if ( current->x==c0->x && c1->y==s1->y ) {
	AddNumber(gb,c0->y-current->y,round);
	AddNumber(gb,c1->x-c0->x,round);
	AddNumber(gb,c1->y-c0->y,round);
	AddNumber(gb,s1->x-c1->x,round);
	*(gb->pt)++ = 30;		/* vhcurveto */
    } else if ( current->y==c0->y && c1->x==s1->x ) {
	AddNumber(gb,c0->x-current->x,round);
	AddNumber(gb,c1->x-c0->x,round);
	AddNumber(gb,c1->y-c0->y,round);
	AddNumber(gb,s1->y-c1->y,round);
	*(gb->pt)++ = 31;		/* hvcurveto */
    } else {
	AddNumber(gb,c0->x-current->x,round);
	AddNumber(gb,c0->y-current->y,round);
	AddNumber(gb,c1->x-c0->x,round);
	AddNumber(gb,c1->y-c0->y,round);
	AddNumber(gb,s1->x-c1->x,round);
	AddNumber(gb,s1->y-c1->y,round);
	*(gb->pt)++ = 8;		/* rrcurveto */
    }
    *current = *s1;
}

static void flexto(GrowBuf *gb,BasePoint *current,Spline *pspline,int round,
	struct hintdb *hdb) {
    BasePoint *c0, *c1, *mid, *end;
    Spline *nspline;
    BasePoint offsets[7];
    int i;
    BasePoint temp1, temp2, temp3, temp;

    c0 = &pspline->from->nextcp;
    c1 = &pspline->to->prevcp;
    mid = &pspline->to->me;
    if ( round ) {
	temp1.x = rint(c0->x);
	temp1.y = rint(c0->y);
	c0 = &temp1;
	temp2.x = rint(c1->x);
	temp2.y = rint(c1->y);
	c1 = &temp2;
	temp.x = rint(mid->x);
	temp.y = rint(mid->y);
	mid = &temp;
    }
    offsets[0].x = mid->x-current->x;	offsets[0].y = mid->y-current->y;
    offsets[1].x = c0->x-mid->x;	offsets[1].y = c0->y-mid->y;
    offsets[2].x = c1->x-c0->x;		offsets[2].y = c1->y-c0->y;
    offsets[3].x = mid->x-c1->x;	offsets[3].y = mid->y-c1->y;
    nspline = pspline->to->next;
    c0 = &nspline->from->nextcp;
    c1 = &nspline->to->prevcp;
    end = &nspline->to->me;
    if ( round ) {
	temp1.x = rint(c0->x);
	temp1.y = rint(c0->y);
	c0 = &temp1;
	temp2.x = rint(c1->x);
	temp2.y = rint(c1->y);
	c1 = &temp2;
	temp3.x = rint(end->x);
	temp3.y = rint(end->y);
	end = &temp3;
    }
    offsets[4].x = c0->x-mid->x;	offsets[4].y = c0->y-mid->y;
    offsets[5].x = c1->x-c0->x;		offsets[5].y = c1->y-c0->y;
    offsets[6].x = end->x-c1->x;	offsets[6].y = end->y-c1->y;

    if ( hdb!=NULL ) HintSetup(gb,hdb,nspline->to,round);

    if ( gb->pt+2 >= gb->end )
	GrowBuffer(gb);

    *(gb->pt)++ = 1+139;		/* 1 */
    *(gb->pt)++ = 10;			/* callsubr */
    for ( i=0; i<7; ++i ) {
	if ( gb->pt+20 >= gb->end )
	    GrowBuffer(gb);
	AddNumber(gb,offsets[i].x,round);
	AddNumber(gb,offsets[i].y,round);
	*(gb->pt)++ = 21;		/* rmoveto */
	*(gb->pt)++ = 2+139;		/* 2 */
	*(gb->pt)++ = 10;		/* callsubr */
    }
    if ( gb->pt+20 >= gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 50+139;		/* 50, .50 pixels */
    AddNumber(gb,end->x,round);
    AddNumber(gb,end->y,round);		/* final location, absolute position */
    *(gb->pt)++ = 0+139;		/* 0 */
    *(gb->pt)++ = 10;			/* callsubr */

    *current = *end;
}

static void CvtPsSplineSet(GrowBuf *gb, SplinePointList *spl, BasePoint *current,
	int round, struct hintdb *hdb ) {
    Spline *spline, *first;
    SplinePointList temp;

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL;
	SplineSetReverse(spl);
	/* For some reason fontographer reads its spline in in the reverse */
	/*  order from that I use. I'm not sure how they do that. The result */
	/*  being that what I call clockwise they call counter. Oh well. */
	/*  If I reverse the splinesets after reading them in, and then again*/
	/*  when saving them out, all should be well */
	if ( spl->first->flexy || spl->first->flexx ) {
	    /* can't handle a flex (mid) point as the first point. rotate the */
	    /* list by one, this is possible because only closed paths have */
	    /* points marked as flex, and because we can't have two flex mid- */
	    /* points in a row */
	    temp = *spl;
	    temp.first = temp.last = spl->first->next->to;
	    spl = &temp;
	}
	moveto(gb,current,spl->first,false,round,hdb);
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( first==NULL ) first = spline;
	    if ( spline->to->flexx || spline->to->flexy ) {
		flexto(gb,current,spline,round,hdb);	/* does two adjacent splines */
		spline = spline->to->next;
	    } else if ( spline->knownlinear )
		moveto(gb,current,spline->to,true,round,hdb);
	    else
		curveto(gb,current,spline,round,hdb);
	}
	if ( gb->pt+1 >= gb->end )
	    GrowBuffer(gb);
	*(gb->pt)++ = 9;			/* closepath */
	SplineSetReverse(spl);
	/* Of course, I have to Reverse again to get back to my convention after*/
	/*  saving */
    }
}

static RefChar *IsRefable(RefChar *ref, int isps, double transform[6], RefChar *sofar) {
    double trans[6];
    RefChar *sub;

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
	    (isps!=1 && (ref->sc->splines!=NULL || ref->sc->refs==NULL))) {
	/* If we're in postscript mode and the character we are refering to */
	/*  has an adobe encoding then we are done. */
	/* In TrueType mode, if the character has no refs itself then we are */
	/*  done, but if it has splines as well as refs we are also done */
	/*  because it will have to be dumped out as splines */
	/* Type2 PS (opentype) is the same as truetype here */
	sub = galloc(sizeof(RefChar));
	*sub = *ref;
	sub->next = sofar;
	sub->splines = NULL;
	memcpy(sub->transform,trans,sizeof(trans));
return( sub );
    } else if ( /* isps &&*/ ( ref->sc->refs==NULL || ref->sc->splines!=NULL) ) {
	RefCharsFree(sofar);
return( NULL );
    }
    for ( sub=ref->sc->refs; sub!=NULL; sub=sub->next ) {
	sofar = IsRefable(sub,isps,trans, sofar);
	if ( sofar==NULL )
return( NULL );
    }
return( sofar );
}
	
/* Postscript can only make refs to things which are in the Adobe encoding */
/*  Suppose Cyrillic A with breve consists of two refs, one to cyrillic A and */
/*  one to nospacebreve (neither in adobe). But if cyrillic A was just a ref */
/*  to A, and if nospacebreve was a ref to breve (both in adobe), then by going*/
/*  one more level of indirection we can use refs in the font. */
/* TrueType only allows refs to things which are themselves simple (ie. contain */
/*  no refs. So if we use the above example we'd still end up with A and breve */
/* They way I do Type2 postscript I can have as many refs as I like, they */
/*  can only have translations (no scale, skew, rotation), they can't be */
/*  refs themselves. They can't use hintmask commands inside */
RefChar *SCCanonicalRefs(SplineChar *sc, int isps) {
    RefChar *ret = NULL, *ref;
    double noop[6];

    /* Neither allows mixing splines and refs */
    if ( sc->splines!=NULL )
return(NULL);
    noop[0] = noop[3] = 1.0; noop[2]=noop[1]=noop[4]=noop[5] = 0;
    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) {
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
	    if ( isps==2 && (ref->sc->hconflicts || ref->sc->vconflicts))
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
	RefCharsFree(ret);
return( NULL );
    }
return( ret );
}

static int IsSeacable(GrowBuf *gb, SplineChar *sc, int round) {
    /* can be at most two chars in a seac (actually must be exactly 2, but */
    /*  I'll put in a space if there's only one */
    RefChar *r1, *r2, *rt, *refs;
    RefChar space;
    DBounds b;
    int i;

    refs = SCCanonicalRefs(sc,true);
    if ( refs==NULL || ( refs->next!=NULL && refs->next->next!=NULL ))
return( false );
    r1 = refs;
    if ((r2 = r1->next)==NULL ) {
	r2 = &space;
	memset(r2,'\0',sizeof(space));
	space.adobe_enc = ' ';
	space.transform[0] = 1.0;
	space.transform[3] = 1.0;
	for ( i=0; i<sc->parent->charcnt; ++i )
	    if ( sc->parent->chars[i]!=NULL &&
		    strcmp(sc->parent->chars[i]->name,"space")==0 )
	break;
	if ( i==sc->parent->charcnt )
return( false );			/* No space???? */
	space.sc = sc->parent->chars[i];
	if ( space.sc->splines!=NULL || space.sc->refs!=NULL )
return( false );
    }
    if ( r1->adobe_enc==-1 || r2->adobe_enc==-1 )
return( false );
    if ( r1->transform[0]!=1 || r1->transform[1]!=0 || r1->transform[2]!=0 ||
	    r1->transform[3]!=1 )
return( false );
    if ( r2->transform[0]!=1 || r2->transform[1]!=0 || r2->transform[2]!=0 ||
	    r2->transform[3]!=1 )
return( false );
    if ( r1->transform[4]!=0 || r1->transform[5]!=0 ) {
	if ( r2->transform[4]!=0 || r2->transform[5]!=0 )
return( false );			/* Only one can be translated */
	rt = r1; r1 = r2; r2 = rt;
    }

    SplineCharFindBounds(r2->sc,&b);

    if ( gb->pt+43>gb->end )
	GrowBuffer(gb);
    if ( round ) b.minx = floor(b.minx);
    AddNumber(gb,b.minx,round);
    AddNumber(gb,r2->transform[4] + b.minx-sc->lsidebearing,round);
    AddNumber(gb,r2->transform[5],round);
    AddNumber(gb,r1->adobe_enc,round);
    AddNumber(gb,r2->adobe_enc,round);
    *(gb->pt)++ = 12;
    *(gb->pt)++ = 6;
return( true );
}

static unsigned char *SplineChar2PS(SplineChar *sc,int *len,int round,int iscjk,
	struct pschars *subrs) {
    DBounds b;
    GrowBuf gb;
    BasePoint current;
    RefChar *rf;
    unsigned char *ret;
    struct hintdb hintdb, *hdb;

    memset(&gb,'\0',sizeof(gb));
    SplineCharFindBounds(sc,&b);
    AddNumber(&gb,b.minx,round);
    AddNumber(&gb,sc->width,round);
    *gb.pt++ = 13;				/* hsbw, lbearing & width */
    NumberHints(sc);
    current.x = round?floor(b.minx):b.minx; current.y = 0;
    sc->lsidebearing = current.x;

    if ( IsSeacable(&gb,sc,round)) {
	/* All Done */;
    } else {
	if ( sc->changedsincelasthinted && !sc->manualhints )
	    SplineCharAutoHint(sc,true);
	HintDirection(sc->hstem);
	HintDirection(sc->vstem);
	if ( iscjk )
	    CounterHints1(&gb,sc,round);	/* Must come immediately after hsbw */
	if ( !sc->vconflicts && !sc->hconflicts ) {
	    CvtPsHints(&gb,sc->hstem,0,true,round,iscjk);
	    CvtPsHints(&gb,sc->vstem,sc->lsidebearing,false,round,iscjk);
	    hdb = NULL;
	} else {
	    memset(&hintdb,0,sizeof(hintdb));
	    hintdb.subrs = subrs; hintdb.iscjk = iscjk; hintdb.sc = sc;
	    hdb = &hintdb;
	    if ( sc->enc==488 )
		sc->enc = 488;
	    InitialHintSetup(&gb,hdb,round);
	}
	CvtPsSplineSet(&gb,sc->splines,&current,round,hdb);
	for ( rf = sc->refs; rf!=NULL; rf = rf->next )
	    CvtPsSplineSet(&gb,rf->splines,&current,round,hdb);
    }
    if ( gb.pt+1>=gb.end )
	GrowBuffer(&gb);
    *gb.pt++ = 14;				/* endchar */
    ret = (unsigned char *) copyn((char *) gb.base,gb.pt-gb.base);
    *len = gb.pt-gb.base;
    free(gb.base);
return( ret );
}

int SFOneWidth(SplineFont *sf) {
    int width, i;

    width = -1;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && strcmp(sf->chars[i]->name,".notdef")!=0 ) {
	if ( width==-1 ) width = sf->chars[i]->width;
	else if ( width!=sf->chars[i]->width ) {
	    width = -2;
    break;
	}
    }
return(width!=-2);
}

int SFIsCJK(SplineFont *sf) {
    char *val;

    if ( (val = PSDictHasEntry(sf->private,"LanguageGroup"))!=NULL )
return( strtol(val,NULL,10));

    if ( sf->encoding_name>=em_first2byte && sf->encoding_name<em_unicode )
return( true );
    if ( sf->encoding_name==em_unicode && 0x3000<sf->charcnt &&
	    SCWorthOutputting(sf->chars[0x3000]) &&
	    !SCWorthOutputting(sf->chars['A']) )
return( true );

return( false );
}

struct pschars *SplineFont2Chrs(SplineFont *sf, int round, int iscjk,
	struct pschars *subrs) {
    struct pschars *chrs = calloc(1,sizeof(struct pschars));
    int i, cnt;
    char notdefentry[20];

    cnt = 0;
    for ( i=0; i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]) )
	    ++cnt;
    ++cnt;		/* one notdef entry */
    chrs->cnt = cnt;
    chrs->keys = malloc(cnt*sizeof(char *));
    chrs->lens = malloc(cnt*sizeof(int));
    chrs->values = malloc(cnt*sizeof(unsigned char *));

    i = cnt = 0;
    chrs->keys[0] = copy(".notdef");
    if ( sf->chars[0]!=NULL &&
	    (sf->chars[0]->splines!=NULL || sf->chars[0]->widthset) &&
	     sf->chars[0]->refs==NULL && strcmp(sf->chars[0]->name,".notdef")==0 ) {
#if 0
	if ( sf->chars[0]->origtype1!=NULL ) {
	    chrs->values[0] = (uint8 *) copyn((char *) sf->chars[0]->origtype1,sf->chars[0]->origlen);
	    chrs->lens[0] = sf->chars[0]->origlen;
	} else
#endif
	    chrs->values[0] = SplineChar2PS(sf->chars[0],&chrs->lens[0],round,iscjk,subrs);
	i = 1;
    } else {
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
    }
    cnt = 1;
    for ( ; i<sf->charcnt; ++i ) {
	if ( SCWorthOutputting(sf->chars[i]) ) {
	    chrs->keys[cnt] = copy(sf->chars[i]->name);
#if 0
	    if ( sf->chars[i]->origtype1!=NULL ) {
		chrs->values[cnt] = (uint8 *) copyn((char *) sf->chars[i]->origtype1,sf->chars[i]->origlen);
		chrs->lens[cnt] = sf->chars[i]->origlen;
	    } else
#endif
		chrs->values[cnt] = SplineChar2PS(sf->chars[i],&chrs->lens[cnt],
			round,iscjk,subrs);
	    ++cnt;
	}
	GProgressNext();
    }
    chrs->next = cnt;
return( chrs );
}

/* ************************************************************************** */
/* ********************** Type2 PostScript CharStrings ********************** */
/* ************************************************************************** */

static void AddNumber2(GrowBuf *gb, double pos) {
    int val;
    unsigned char *str;

    if ( gb->pt+5>=gb->end )
	GrowBuffer(gb);

    pos = rint(65536*pos)/65536;

    str = gb->pt;
    if ( pos!=floor(pos )) {
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

static int FigureCounters2(StemInfo *stems,uint8 mask[12] ) {
    StemInfo *h, *first;

    while ( stems!=NULL ) {
	for ( first=stems; first!=NULL && first->used; first = first->next );
	if ( first==NULL )
    break;
	if ( first->where==NULL || first->hintnumber==-1 || stemmatches(first)<=2 ) {
	    first->used = true;
	    stems = first->next;
    continue;
	}
	for ( h = first; h!=NULL; h = h->next ) {
	    if ( h->tobeused ) {
		mask[h->hintnumber>>3] |= (0x80>>(h->hintnumber&7));
		h->used = true;
	    }
	}
return( true );
    }
return( false );
}

static void AddMask2(GrowBuf *gb,uint8 mask[12],int cnt, int oper) {
    int i;

    if ( gb->pt+1+((cnt+7)>>3)>=gb->end )
	GrowBuffer(gb);
    *gb->pt++ = oper;				/* hintmask */
    for ( i=0; i< ((cnt+7)>>3); ++i )
	*gb->pt++ = mask[i];
}

static void CounterHints2(GrowBuf *gb, StemInfo *hs, StemInfo *vs) {
    uint8 mask[12];
    StemInfo *h;
    int cnt=0;

    for ( h=hs; h!=NULL ; h=h->next, ++cnt )
	h->used = false;
    for ( h=vs; h!=NULL ; h=h->next, ++cnt )
	h->used = false;

    if ( cnt>96 ) cnt=96;

    while ( 1 ) {
	memset(mask,'\0',sizeof(mask));
	if ( !FigureCounters2(hs,mask) && !FigureCounters2(vs,mask))
    break;
	AddMask2(gb,mask,cnt,20);		/* cntrmask */
    }
}

static int HintSetup2(GrowBuf *gb,struct hintdb *hdb, SplinePoint *to ) {
    uint8 mask[12];

    if ( !FigureHintMask(hdb, to, mask ) )
return( false );

    AddMask2(gb,mask,hdb->cnt,19);		/* hintmask */
    memcpy(hdb->mask,mask,sizeof(mask));
return( true );
}

static void moveto2(GrowBuf *gb,struct hintdb *hdb,SplinePoint *to) {

    if ( gb->pt+18 >= gb->end )
	GrowBuffer(gb);

    HintSetup2(gb,hdb,to);
#if 0
    if ( hdb->current.x==to->me.x && hdb->current.y==to->me.y ) {
	/* we're already here */
	/* Yes, but a move is required anyway at char start */
    } else
#endif
    if ( hdb->current.x==to->me.x ) {
	AddNumber2(gb,to->me.y-hdb->current.y);
	*(gb->pt)++ = 4;		/* v move to */
    } else if ( hdb->current.y==to->me.y ) {
	AddNumber2(gb,to->me.x-hdb->current.x);
	*(gb->pt)++ = 22;		/* h move to */
    } else {
	AddNumber2(gb,to->me.x-hdb->current.x);
	AddNumber2(gb,to->me.y-hdb->current.y);
	*(gb->pt)++ = 21;		/* r move to */
    }
    hdb->current = to->me;
}

static Spline *lineto2(GrowBuf *gb,struct hintdb *hdb,Spline *spline, Spline *done) {
    int cnt, hv, hvcnt;
    Spline *test, *lastgood, *lasthvgood;

    for ( test=spline, cnt=0; test->knownlinear && cnt<15; ) {
	++cnt;
	lastgood = test;
	test = test->to->next;
	if ( test==done )
    break;
    }

    HintSetup2(gb,hdb,spline->to);

    hv = -1; hvcnt=1; lasthvgood = NULL;
    if ( spline->from->me.x==spline->to->me.x )
	hv = 1;		/* Vertical */
    else if ( spline->from->me.y==spline->to->me.y )
	hv = 0;		/* Horizontal */
    if ( hv!=-1 && cnt>1 ) {
	for ( test=spline->to->next, hvcnt=1; ; test = test->to->next ) {
	    if ( hv==1 && test->from->me.y==test->to->me.y )
		hv = 0;
	    else if ( hv==0 && test->from->me.x==test->to->me.x )
		hv = 1;
	    else
	break;
	    lasthvgood = test;
	    ++hvcnt;
	    if ( test==lastgood )
	break;
	}
	if ( hvcnt==cnt || hvcnt>=2 ) {
	    /* It's more efficeint to do some h/v linetos */
	    for ( test=spline; ; test = test->to->next ) {
		if ( NeedsNewHintMask(hdb,test->to))
	    break;
		if ( test->from->me.x==test->to->me.x )
		    AddNumber2(gb,test->to->me.y-hdb->current.y);
		else
		    AddNumber2(gb,test->to->me.x-hdb->current.x);
		hdb->current = test->to->me;
		if ( test==lasthvgood )
	    break;
	    }
	    if ( gb->pt+1 >= gb->end )
		GrowBuffer(gb);
	    *(gb->pt)++ = spline->from->me.x==spline->to->me.x? 7 : 6;
return( test->to->next );
	}
    }

    for ( test=spline; ; test = test->to->next ) {
	if ( NeedsNewHintMask(hdb,test->to))
    break;
	AddNumber2(gb,test->to->me.x-hdb->current.x);
	AddNumber2(gb,test->to->me.y-hdb->current.y);
	hdb->current = test->to->me;
	if ( test==lastgood )
    break;
    }
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 5;		/* r line to */
return( test->to->next );
}

static Spline *curveto2(GrowBuf *gb,struct hintdb *hdb,Spline *spline, Spline *done) {
    int cnt=0, hv;
    Spline *first;
    BasePoint start;

    HintSetup2(gb,hdb,spline->to);

    hv = -1;
    if ( hdb->current.x==spline->from->nextcp.x && spline->to->prevcp.y==spline->to->me.y )
	hv = 1;
    else if ( hdb->current.y==spline->from->nextcp.y && spline->to->prevcp.x==spline->to->me.x )
	hv = 0;
    if ( hv!=-1 ) {
	first = spline; start = hdb->current;
	while (
		(hv==1 && hdb->current.x==spline->from->nextcp.x && spline->to->prevcp.y==spline->to->me.y ) ||
		(hv==0 && hdb->current.y==spline->from->nextcp.y && spline->to->prevcp.x==spline->to->me.x ) ) {
	    if ( NeedsNewHintMask(hdb,spline->to))
	break;
	    if ( hv==1 ) {
		AddNumber2(gb,spline->from->nextcp.y-hdb->current.y);
		AddNumber2(gb,spline->to->prevcp.x-spline->from->nextcp.x);
		AddNumber2(gb,spline->to->prevcp.y-spline->from->nextcp.y);
		AddNumber2(gb,spline->to->me.x-spline->to->prevcp.x);
		hv = 0;
	    } else {
		AddNumber2(gb,spline->from->nextcp.x-hdb->current.x);
		AddNumber2(gb,spline->to->prevcp.x-spline->from->nextcp.x);
		AddNumber2(gb,spline->to->prevcp.y-spline->from->nextcp.y);
		AddNumber2(gb,spline->to->me.y-spline->to->prevcp.y);
		hv = 1;
	    }
	    hdb->current = spline->to->me;
	    ++cnt;
	    spline = spline->to->next;
	    if ( spline==done || cnt>9 )
	break;
	}
	if ( gb->pt+1 >= gb->end )
	    GrowBuffer(gb);
	*(gb->pt)++ = ( start.x==first->from->nextcp.x && first->to->prevcp.y==first->to->me.y )?
		30:31;		/* vhcurveto:hvcurveto */
return( spline );
    }
    while ( cnt<6 ) {
	if ( NeedsNewHintMask(hdb,spline->to))
    break;
	AddNumber2(gb,spline->from->nextcp.x-hdb->current.x);
	AddNumber2(gb,spline->from->nextcp.y-hdb->current.y);
	AddNumber2(gb,spline->to->prevcp.x-spline->from->nextcp.x);
	AddNumber2(gb,spline->to->prevcp.y-spline->from->nextcp.y);
	AddNumber2(gb,spline->to->me.x-spline->to->prevcp.x);
	AddNumber2(gb,spline->to->me.y-spline->to->prevcp.y);
	hdb->current = spline->to->me;
	++cnt;
	spline = spline->to->next;
	if ( spline==done )
    break;
    }
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 8;		/* rrhcurveto */
return( spline );
}

static void flexto2(GrowBuf *gb,struct hintdb *hdb,Spline *pspline) {
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

    if ( c0->y==hdb->current.y && nc1->y==hdb->current.y && end->y==hdb->current.y &&
	    c1->y==mid->y && nc0->y==mid->y ) {
	if ( gb->pt+7*6+2 >= gb->end )
	    GrowBuffer(gb);
	AddNumber2(gb,c0->x-hdb->current.x);
	AddNumber2(gb,c1->x-c0->x);
	AddNumber2(gb,c1->y-c0->y);
	AddNumber2(gb,mid->x-c1->x);
	AddNumber2(gb,nc0->x-mid->x);
	AddNumber2(gb,nc1->x-nc0->x);
	AddNumber2(gb,end->x-nc1->x);
	*gb->pt++ = 12; *gb->pt++ = 34;		/* hflex */
    } else {
	if ( gb->pt+11*6+2 >= gb->end )
	    GrowBuffer(gb);
	AddNumber2(gb,c0->x-hdb->current.x);
	AddNumber2(gb,c0->y-hdb->current.y);
	AddNumber2(gb,c1->x-c0->x);
	AddNumber2(gb,c1->y-c0->y);
	AddNumber2(gb,mid->x-c1->x);
	AddNumber2(gb,mid->y-c1->y);
	AddNumber2(gb,nc0->x-mid->x);
	AddNumber2(gb,nc0->y-mid->y);
	AddNumber2(gb,nc1->x-nc0->x);
	AddNumber2(gb,nc1->y-nc0->y);
	if ( hdb->current.y==end->y )
	    AddNumber2(gb,end->x-nc1->x);
	else
	    AddNumber2(gb,end->y-nc1->y);
	*gb->pt++ = 12; *gb->pt++ = 37;		/* flex1 */
    }

    hdb->current = *end;
}

static void CvtPsSplineSet2(GrowBuf *gb, SplinePointList *spl, struct hintdb *hdb) {
    Spline *spline, *first;
    SplinePointList temp;

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL;
	SplineSetReverse(spl);
	/* For some reason fontographer reads its spline in in the reverse */
	/*  order from that I use. I'm not sure how they do that. The result */
	/*  being that what I call clockwise they call counter. Oh well. */
	/*  If I reverse the splinesets after reading them in, and then again*/
	/*  when saving them out, all should be well */
	if ( spl->first->flexy || spl->first->flexx ) {
	    /* can't handle a flex (mid) point as the first point. rotate the */
	    /* list by one, this is possible because only closed paths have */
	    /* points marked as flex, and because we can't have two flex mid- */
	    /* points in a row */
	    temp = *spl;
	    temp.first = temp.last = spl->first->next->to;
	    spl = &temp;
	}
	moveto2(gb,hdb,spl->first);
	for ( spline = spl->first->next; spline!=NULL && spline!=first; ) {
	    if ( first==NULL ) first = spline;
	    if ( spline->to->flexx || spline->to->flexy ) {
		flexto2(gb,hdb,spline);	/* does two adjacent splines */
		spline = spline->to->next->to->next;
	    } else if ( spline->knownlinear )
		spline = lineto2(gb,hdb,spline,first);
	    else
		spline = curveto2(gb,hdb,spline,first);
	}
	SplineSetReverse(spl);
	/* Of course, I have to Reverse again to get back to my convention after*/
	/*  saving */
    }
}

static StemInfo *OrderHints(StemInfo *mh, StemInfo *h, double offset, int mask) {
    StemInfo *prev, *new, *test;

    while ( h!=NULL ) {
	prev = NULL;
	for ( test=mh; test!=NULL && (h->start+offset>test->start ||
		(h->start+offset==test->start && h->width<test->width)); test=test->next )
	    prev = test;
	if ( test!=NULL && test->start==h->start+offset && test->width==h->width )
	    test->mask |= mask;		/* Actually a bit */
	else {
	    new = galloc(sizeof(StemInfo));
	    new->next = test;
	    new->start = h->start+offset;
	    new->width = h->width;
	    new->mask = mask;
	    if ( prev==NULL )
		mh = new;
	    else
		prev->next = new;
	}
	h = h->next;
    }
return( mh );
}

static void DumpHints(GrowBuf *gb,StemInfo *h,int oper) {
    double last = 0;

    if ( h==NULL )
return;
    while ( h!=NULL && h->hintnumber!=-1 ) {
	if ( h->backwards ) {
	    AddNumber2(gb,h->start-last+h->width);
	    AddNumber2(gb,-h->width);
	} else {
	    AddNumber2(gb,h->start-last);
	    AddNumber2(gb,h->width);
	}
	last = h->start + h->width;
	h = h->next;
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
	GDrawIError("hintmask invoked when there are no hints");
    memset(masks,'\0',sizeof(masks));
    cnt = 0;
    while ( h!=NULL && h->hintnumber ) {
	if ( h->mask&cur->adobe_enc )
	    masks[h->hintnumber>>3] |= 0x80>>(h->hintnumber&7);
	h = h->next; ++cnt;
    }
    while ( v!=NULL && v->hintnumber ) {
	if ( v->mask&cur->adobe_enc )
	    masks[v->hintnumber>>3] |= 0x80>>(v->hintnumber&7);
	v = v->next; ++cnt;
    }
    AddMask2(gb,masks,cnt,19);		/* hintmask */
}

static void ExpandRefList2(GrowBuf *gb, RefChar *refs, struct pschars *subrs) {
    RefChar *r;
    int hsccnt=0;
    BasePoint current, *bpt;
    StemInfo *h, *v, *s;
    int cnt;
    /* The only refs I deal with have no hint conflicts within them */

    memset(&current,'\0',sizeof(current));

    for ( r=refs; r!=NULL; r=r->next )
	if ( r->sc->hstem!=NULL || r->sc->vstem!=NULL )
	    r->adobe_enc = 1<<hsccnt++;
	else
	    r->adobe_enc = 0;

    for ( r=refs, h=v=NULL; r!=NULL; r=r->next ) {
	h = OrderHints(h,r->sc->hstem,r->transform[5],r->adobe_enc);
	v = OrderHints(v,r->sc->vstem,r->transform[4],r->adobe_enc);
    }
    for ( s=h, cnt=0; s!=NULL; s=s->next, ++cnt )
	s->hintnumber = cnt>=96?-1:cnt;
    for ( s=v, cnt=0; s!=NULL; s=s->next, ++cnt )
	s->hintnumber = cnt>=96?-1:cnt;

    if ( h!=NULL )
	DumpHints(gb,h,hsccnt>1?18:1);		/* hstemhm/hstem */
    if ( v!=NULL )
	DumpHints(gb,v, hsccnt<=1 ? 3:-1);	/* vstem */
    CounterHints2(gb,h,v);			/* Precedes first hintmask */

    for ( r=refs; r!=NULL; r=r->next ) {
	if ( hsccnt>1 )
	    DumpHintMasked(gb,r,h,v);
	if ( current.x!=r->transform[4] || current.y!=r->transform[5] ) {
	    /* Translate from end of last character to where this one should */
	    /*  start */
	    if ( current.x!=r->transform[4] )
		AddNumber2(gb,r->transform[4]-current.x);
	    if ( current.y!=r->transform[5] )
		AddNumber2(gb,r->transform[5]-current.y);
	    if ( gb->pt+1>=gb->end )
		GrowBuffer(gb);
	    *gb->pt++ = current.x==r->transform[4]?4:	/* vmoveto */
	    		current.y==r->transform[5]?22:	/* hmoveto */
			21;				/* rmoveto */
	}
	if ( r->sc->lsidebearing==0x7fff )
	    GDrawIError("Attempt to reference and unreferenceable glyph %s", r->sc->name );
	AddNumber2(gb,r->sc->lsidebearing);
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*gb->pt++ = 10;					/* callsubr */
	bpt = (BasePoint *) (subrs->keys[r->sc->lsidebearing+subrs->bias]);
	current.x = bpt->x+r->transform[4];
	current.y = bpt->y+r->transform[5];
    }
    StemInfosFree(h); StemInfosFree(v);
}

static int IsRefable2(GrowBuf *gb, SplineChar *sc, struct pschars *subrs) {
    RefChar *refs;

    refs = SCCanonicalRefs(sc,2);
    if ( refs==NULL )
return( false );
    ExpandRefList2(gb,refs,subrs);
    RefCharsFree(refs);
return( true );
}

static unsigned char *SplineChar2PSOutline2(SplineChar *sc,int *len, BasePoint *_current ) {
    GrowBuf gb;
    struct hintdb hdb;
    RefChar *rf;
    unsigned char *ret;

    memset(&gb,'\0',sizeof(gb));
    memset(&hdb,'\0',sizeof(hdb));

    CvtPsSplineSet2(&gb,sc->splines,&hdb);
    for ( rf = sc->refs; rf!=NULL; rf = rf->next )
	CvtPsSplineSet2(&gb,rf->splines,&hdb);
    if ( gb.pt+1>=gb.end )
	GrowBuffer(&gb);
    *gb.pt++ = 11;				/* return */
    ret = (unsigned char *) copyn((char *) gb.base,gb.pt-gb.base);
    *len = gb.pt-gb.base;
    free(gb.base);
    *_current = hdb.current;
return( ret );
}

static unsigned char *SplineChar2PS2(SplineChar *sc,int *len, int nomwid, int defwid, struct pschars *subrs) {
    GrowBuf gb;
    RefChar *rf;
    unsigned char *ret;
    struct hintdb hdb;
    uint8 mask[12];

    memset(&gb,'\0',sizeof(gb));

    GrowBuffer(&gb);
    /* store the width on the stack */
    if ( sc->width==defwid )
	/* Don't need to do anything for the width */;
    else
	AddNumber2(&gb,sc->width-nomwid);
    if ( IsRefable2(&gb,sc,subrs))
	/* All Done */;
    else if ( sc->lsidebearing!=0x7fff ) {
	RefChar refs;
	memset(&refs,'\0',sizeof(refs));
	refs.sc = sc;
	refs.transform[0] = refs.transform[3] = 1.0;
	ExpandRefList2(&gb,&refs,subrs);
    } else {
	memset(&hdb,'\0',sizeof(hdb));
	hdb.sc = sc;
	hdb.noconflicts = !sc->hconflicts && !sc->vconflicts;
	if ( sc->changedsincelasthinted && !sc->manualhints )
	    SplineCharAutoHint(sc,true);
	HintDirection(sc->hstem);
	HintDirection(sc->vstem);
	hdb.cnt = NumberHints(sc);
	DumpHints(&gb,sc->hstem,sc->hconflicts?18:1);
	DumpHints(&gb,sc->vstem,sc->hconflicts?-1:3);
	CounterHints2(&gb, sc->hstem, sc->vstem);
	if ( (sc->hconflicts || sc->vconflicts) && InitialHintMask(&hdb,mask)) {
	    AddMask2(&gb,mask,hdb.cnt,19);		/* hintmask */
	    memcpy(hdb.mask,mask,sizeof(mask));
	}
	CvtPsSplineSet2(&gb,sc->splines,&hdb);
	for ( rf = sc->refs; rf!=NULL; rf = rf->next )
	    CvtPsSplineSet2(&gb,rf->splines,&hdb);
    }
    if ( gb.pt+1>=gb.end )
	GrowBuffer(&gb);
    *gb.pt++ = 14;				/* endchar */
    ret = (unsigned char *) copyn((char *) gb.base,gb.pt-gb.base);
    *len = gb.pt-gb.base;
    free(gb.base);
return( ret );
}

struct pschars *SplineFont2Subrs2(SplineFont *sf) {
    struct pschars *subrs = calloc(1,sizeof(struct pschars));
    int i,cnt;
    SplineChar *sc;

    /* We don't allow refs to refs. It confuses the hintmask operators */
    /*  instead we track down to the base ref */
    for ( i=cnt=0; i<sf->charcnt; ++i ) {
	sc = sf->chars[i];
	if ( sc!=NULL && sc->changedsincelasthinted && !sc->manualhints )
	    SplineCharAutoHint(sc,true);
	if ( sc==NULL )
	    /* Do Nothing */;
	else if ( SCWorthOutputting(sc) && !sc->hconflicts && !sc->vconflicts &&
		sc->refs==NULL && sc->dependents!=NULL )
	    ++cnt;
	else
	    sc->lsidebearing = 0x7fff;
    }

    subrs->cnt = cnt;
    if ( cnt==0 )
return( subrs);
    subrs->lens = malloc(cnt*sizeof(int));
    subrs->values = malloc(cnt*sizeof(unsigned char *));
    subrs->keys = malloc(cnt*sizeof(BasePoint *));
    subrs->bias = cnt<1240 ? 107 :
		  cnt<33900 ? 1131 : 32768;
    
    for ( i=cnt=0; i<sf->charcnt; ++i ) {
	sc = sf->chars[i];
	if ( sc==NULL )
	    /* Do Nothing */;
	else if ( SCWorthOutputting(sc) && !sc->hconflicts && !sc->vconflicts &&
		sc->refs==NULL && sc->dependents!=NULL ) {
	    subrs->keys[cnt] = galloc(sizeof(BasePoint));
	    subrs->values[cnt] = SplineChar2PSOutline2(sc,&subrs->lens[cnt],
		    (BasePoint *) (subrs->keys[cnt]));
	    sc->lsidebearing = cnt++ - subrs->bias;
	}
	GProgressNext();
    }
return( subrs );
}
    
struct pschars *SplineFont2Chrs2(SplineFont *sf, int nomwid, int defwid,
	struct pschars *subrs) {
    struct pschars *chrs = calloc(1,sizeof(struct pschars));
    int i, cnt;
    char notdefentry[20];
    SplineChar *sc;

    cnt = 0;
    for ( i=1; i<sf->charcnt; ++i ) {
	sc = sf->chars[i];
	if ( SCWorthOutputting(sc) )
	    ++cnt;
    }
    ++cnt;		/* one notdef entry */
    chrs->cnt = cnt;
    chrs->lens = malloc(cnt*sizeof(int));
    chrs->values = malloc(cnt*sizeof(unsigned char *));

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	sf->chars[i]->ttf_glyph = 0;
    i = cnt = 0;
    if ( sf->chars[0]!=NULL && (sf->chars[0]->splines!=NULL || sf->chars[0]->widthset) &&
	    sf->chars[0]->refs==NULL && strcmp(sf->chars[0]->name,".notdef")==0 ) {
	chrs->values[0] = SplineChar2PS2(sf->chars[0],&chrs->lens[0],nomwid,defwid,subrs);
	sf->chars[0]->ttf_glyph = 0;
	i = 1;
    } else {
	int w = sf->ascent+sf->descent; char *pt = notdefentry;
	if ( w==defwid )
	    /* Don't need to specify it */;
	else {
	    w -= defwid;
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
    }
    cnt = 1;
    for ( i=1; i<sf->charcnt; ++i ) {
	sc = sf->chars[i];
	if ( SCWorthOutputting(sc) ) {
	    chrs->values[cnt] = SplineChar2PS2(sc,&chrs->lens[cnt],nomwid,defwid,subrs);
	    sf->chars[i]->ttf_glyph = cnt++;
	}
	GProgressNext();
    }
return( chrs );
}
