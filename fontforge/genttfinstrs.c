/* Copyright (C) 2000-2007 by George Williams */
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

struct glyphinstrs {
    SplineFont *sf;
    BlueData *bd;
    int fudge;
};

extern int autohint_before_generate;

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


static uint8 *SetRP0To(uint8 *instrs, real hbase, BasePoint *bp, int ptcnt,
	int xdir, real fudge) {
    int i;

    for ( i=0; i<ptcnt; ++i ) {
	if ( (xdir && bp[i].x==hbase) || (!xdir && bp[i].y==hbase) ) {
	    instrs = pushpoint(instrs,i);
	    *instrs++ = 0x10;		/* Set RP0, SRP0 */
return( instrs );
	}
    }
    /* couldn't find anything, fudge a little... */
    for ( i=0; i<ptcnt; ++i ) {
	if ( (xdir && bp[i].x>=hbase-fudge && bp[i].x<=hbase+fudge) ||
		(!xdir && bp[i].y>=hbase-fudge && bp[i].y<=hbase+fudge) ) {
	    instrs = pushpoint(instrs,i);
	    *instrs++ = 0x10;		/* Set RP0, SRP0 */
return( instrs );
	}
    }
return( instrs );
}

static real BDFindValue(real base, BlueData *bd, int isbottom ) {
    real replace = 0x80000000;

    if ( isbottom ) {
	if ( base>= bd->basebelow && base<=bd->base )
	    replace = 0;
    } else {
	if ( base>=bd->caph && base<=bd->caphtop )
	    replace = bd->caph;
	else if ( base>=bd->xheight && base<=bd->xheighttop )
	    replace = bd->xheight;
    }
    /* I'm allowing ascent and descent's to wiggle */
return( replace );
}

/* Run through all points. For any on this hint's start, position them */
/*  If the first point of this hint falls in a blue zone, do a cvt based */
/*	positioning, else
/*  The first point on the first hint is positioned to where it is (dull) */
/*	(or if the last hint overlaps this hint) */
/*  The first point of this hint is positioned offset from the last hint */
/*	(unless the two overlap) */
/*  Any other points are moved by the same amount as the last move */
/* Run through the points again, for any pts on this hint's end: */
/*  Position them offset from the hint's start */
/* Complications: */
/*  If the hint start isn't in a blue zone, but the hint end is, then */
/*	treat the end as the start with a negative width */
/*  If we have overlapping hints then either the points on the start or end */
/*	may already have been positioned. We won't be called if both have been*/
/*	positioned. If the end points have been positioned then reverse the */
/*	hint again. If either edge has been done, then all we need to do is */
/*	establish a reference point on that edge, don't need to position all */
/*	the points again. */
static uint8 *geninstrs(struct glyphinstrs *gi, uint8 *instrs,StemInfo *hint,
	int *contourends, BasePoint *bp, int ptcnt, StemInfo *firsthint, int xdir,
	uint8 *touched) {
    int i;
    int last= -1;
    int stem, basecvt=-1;
    real hbase, base, width, newbase;
    StemInfo *h;
    real fudge = gi->fudge;
    int inrp;
    StemInfo *lasthint=NULL, *testhint;
    int first;

    for ( testhint = firsthint; testhint!=NULL && testhint!=hint; testhint = testhint->next ) {
	if ( HIoverlap(testhint->where,hint->where)!=0 )
	    lasthint = testhint;
    }
    first = lasthint==NULL;
    if ( hint->hasconflicts )
	first = true;		/* if this hint has conflicts don't try to establish a minimum distance between it and the last stem, there might not be one */

    hbase = base = rint(hint->start); width = rint(hint->width);
    if ( !xdir ) {
	/* check the "bluevalues" for things like cap height, xheight and */
	/*  baseline. But only check if the top of a stem is at capheight */
	/*  or xheight, and only if the bottom is at baseline */
	if ( width<0 ) {
	    hbase = (base += width);
	    width = -width;
	}
	if ( (newbase = BDFindValue(base,gi->bd,true))!= 0x80000000 ) {
	    base = newbase;
	    basecvt = TTF__getcvtval(gi->sf,(int)base);
	}
	if ( basecvt == -1 && !hint->startdone ) {
	    hbase = (base += width);
	    if ( (newbase = BDFindValue(base,gi->bd,false))!= 0x80000000 ) {
		base = newbase;
		basecvt = TTF__getcvtval(gi->sf,(int)base);
	    }
	    if ( basecvt!=-1 )
		width = -width;
	    else
		hbase = (base -= width);
	}
    }
    if ( hbase==rint(hint->start) && hint->enddone ) {
	base = (hbase += width);
	width = -width;
	basecvt = -1;
    }

    /* Position all points on this hint's base */
    if (( width>0 && !hint->startdone) || (width<0 && !hint->enddone)) {
	inrp = -1;
	for ( i=0; i<ptcnt; ++i ) {
	    if ( (xdir && bp[i].x>=hbase-fudge && bp[i].x<=hbase+fudge) ||
		    (!xdir && bp[i].y>=hbase-fudge && bp[i].y<=hbase+fudge) ) {
		if ( basecvt!=-1 && last==-1 ) {
		    instrs = pushpointstem(instrs,i,basecvt);
		    *instrs++ = 0x3f;		/* MIAP, rounded, set rp0,rp1 */
		    first = false;
		    inrp = 1;
		} else {
		    instrs = pushpoint(instrs,i);
		    if ( first ) {
			/* set rp0 */
			*instrs++ = 0x2f;	/* MDAP, rounded, set rp0,rp1 */
			first = false;
			inrp = 1;
		    } else if ( last==-1 ) {
			/* set rp0 relative to last hint */
			instrs = SetRP0To(instrs,lasthint->width>0?lasthint->start+lasthint->width:lasthint->start,
				bp,ptcnt,xdir,fudge);
			*instrs++ = 0xc0+0x1e;	/* MDRP, set rp0,rp2, minimum, rounded, white */
			inrp = 2;
		    } else {
			*instrs++ = inrp==1?0x33:0x32;	/* SHP, rp1 or rp2 */
		    }
		}
		touched[i] |= (xdir?1:2);
		last = i;
	    }
	}
	if ( last==-1 )		/* I'm confused. But if the hint doesn't start*/
return(instrs);			/*  anywhere, can't give it a width */
				/* Some hints aren't associated with points */
    } else {
	/* Need to find something to be a reference point, doesn't matter */
	/*  what. Note that it should already have been positioned */
	instrs = SetRP0To(instrs,hbase, bp,ptcnt,xdir,fudge);
    }

    /* Position all points on this hint's base+width */
    stem = TTF_getcvtval(gi->sf,width);
    last = -1;
    for ( i=0; i<ptcnt; ++i ) {
	if ( (xdir && bp[i].x>=hbase+width-fudge && bp[i].x<=hbase+width+fudge) ||
		(!xdir && bp[i].y>=hbase+width-fudge && bp[i].y<=hbase+width+fudge) ) {
	    instrs = pushpointstem(instrs,i,stem);
	    *instrs++ = 0xe0+0x0d;	/* MIRP, minimum, rounded, black */
	    touched[i] |= (xdir?1:2);
	    last = i;
	}
    }

    for ( h=hint->next; h!=NULL && h->start<=hint->start+hint->width; h=h->next ) {
	if ( (h->start>=hint->start-gi->fudge && h->start<=hint->start+gi->fudge) ||
		(h->start>=hint->start+hint->width-gi->fudge && h->start<=hint->start+hint->width+gi->fudge) )
	    h->startdone = true;
	if ( (h->start+h->width>=hint->start-gi->fudge && h->start+h->width<=hint->start+gi->fudge) ||
		(h->start+h->width>=hint->start+hint->width-gi->fudge && h->start+h->width<=hint->start+hint->width+gi->fudge) )
	    h->enddone = true;
    }

return( instrs );
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

static int CoLinear(BasePoint *top1, BasePoint *bottom1,
			BasePoint *top2, BasePoint *bottom2 ) {
    double scale, slope, y;

    if ( top1->y==bottom1->y )
return( RealWithin(top1->y,top2->y,1) && RealWithin(top1->y,bottom2->y,1) );
    else if ( top2->y==bottom2->y )
return( RealWithin(top2->y,top1->y,1) && RealWithin(top2->y,bottom1->y,1) );

    scale = (top2->y-bottom2->y)/(top1->y-bottom1->y);
    slope = (top1->y-bottom1->y)/(top1->x-bottom1->x);
    if ( !RealWithin(top2->x,bottom2->x+(top1->x-bottom1->x)*scale,10) &&
	    !RealWithin(top2->y,bottom2->y+slope*(top2->x-bottom2->x),12) )
return( false );

    y = bottom1->y + slope*(top2->x-bottom1->x);
    if ( !RealWithin(y,top2->y,6) )
return( false );

return( true );
}

static int BpIndex(BasePoint *search,BasePoint *bp,int ptcnt) {
    int i;

    for ( i=0; i<ptcnt; ++i )
	if ( rint(search->x) == bp[i].x && rint(search->y)==bp[i].y )
return( i );

return( -1 );
}

static DStem *DStemMerge(DStemInfo *d, BasePoint *bp, int ptcnt, uint8 *touched) {
    DStemInfo *di, *di2;
    DStem *head=NULL, *cur;
    int i, j, nexti;
    BasePoint *top, *bottom;
    DStem **map[2];

    for ( di=d; di!=NULL; di=di->next ) di->used = false;
    for ( di=d; di!=NULL; di=di->next ) if ( !di->used ) {
	cur = chunkalloc(sizeof(DStem));
	memset(cur->pnum,-1,sizeof(cur->pnum));
	cur->used = gcalloc(ptcnt,sizeof(uint8));
	cur->leftedgetop = di->leftedgetop;
	cur->pnum[0] = BpIndex(&di->leftedgetop,bp,ptcnt);
	cur->leftedgebottom = di->leftedgebottom;
	cur->pnum[1] = BpIndex(&di->leftedgebottom,bp,ptcnt);
	cur->rightedgetop = di->rightedgetop;
	cur->pnum[2] = BpIndex(&di->rightedgetop,bp,ptcnt);
	cur->rightedgebottom = di->rightedgebottom;
	cur->pnum[3] = BpIndex(&di->rightedgebottom,bp,ptcnt);
	cur->used[cur->pnum[0]] = cur->used[cur->pnum[1]] = 1;
	cur->used[cur->pnum[2]] = cur->used[cur->pnum[3]] = 2;
	cur->next = head;
	head = cur;
	for ( di2 = di->next; di2!=NULL; di2 = di2->next ) if ( !di2->used ) {
	    if (( CoLinear(&di->leftedgetop, &di->leftedgebottom,
			&di2->leftedgetop, &di2->leftedgebottom) &&
		    CoLinear(&di->rightedgetop, &di->rightedgebottom,
			&di2->rightedgetop, &di2->rightedgebottom) ) ||
		    (di->leftedgetop.x==di2->leftedgetop.x &&
		     di->leftedgetop.y==di2->leftedgetop.y &&
		     di->leftedgebottom.x==di2->leftedgebottom.x &&
		     di->leftedgebottom.y==di2->leftedgebottom.y ) ||
		    (di->rightedgetop.x==di2->rightedgetop.x &&
		     di->rightedgetop.y==di2->rightedgetop.y &&
		     di->rightedgebottom.x==di2->rightedgebottom.x &&
		     di->rightedgebottom.y==di2->rightedgebottom.y )) {
		if ( di->leftedgetop.x!=di2->leftedgetop.x ||
			 di->leftedgetop.y!=di2->leftedgetop.y ) {
		    i = BpIndex(&di2->leftedgetop,bp,ptcnt);
		    cur->used[i] = 1;
		    if ( cur->leftedgetop.y<di2->leftedgetop.y ) {
			cur->leftedgetop = di2->leftedgetop;
			cur->pnum[0] = i;
		    }
		}
		if ( di->leftedgebottom.x!=di2->leftedgebottom.x ||
			 di->leftedgebottom.y!=di2->leftedgebottom.y ) {
		    i = BpIndex(&di2->leftedgebottom,bp,ptcnt);
		    cur->used[i] = 1;
		    if ( cur->leftedgebottom.y>di2->leftedgebottom.y ) {
			cur->leftedgebottom = di2->leftedgebottom;
			cur->pnum[1] = i;
		    }
		}
		if ( di->rightedgetop.x!=di2->rightedgetop.x ||
			 di->rightedgetop.y!=di2->rightedgetop.y ) {
		    i = BpIndex(&di2->rightedgetop,bp,ptcnt);
		    cur->used[i] = 2;
		    if ( cur->rightedgetop.y<di2->rightedgetop.y ) {
			cur->rightedgetop = di2->rightedgetop;
			cur->pnum[2] = i;
		    }
		}
		if ( di->rightedgebottom.x!=di2->rightedgebottom.x ||
			 di->rightedgebottom.y!=di2->rightedgebottom.y ) {
		    i = BpIndex(&di2->rightedgebottom,bp,ptcnt);
		    cur->used[i] = 2;
		    if ( cur->rightedgebottom.y>di2->rightedgebottom.y ) {
			cur->rightedgebottom = di2->rightedgebottom;
			cur->pnum[3] = i;
		    }
		}
		di2->used = true;
	    }
	}
    }

    map[0] = gcalloc(ptcnt,sizeof(DStem *));
    map[1] = gcalloc(ptcnt,sizeof(DStem *));
    for ( cur = head; cur!=NULL; cur=cur->next ) {
	for ( i=0; i<ptcnt; ++i ) {
	    if ( cur->used[i]) {
		if ( map[0][i]==NULL ) map[0][i] = cur;
		else map[1][i] = cur;
	    }
	}
    }

    /* sometimes we don't find all the bits of a diagonal stem */
    /* "k" is an example, we don't notice the stub between the vertical stem and the lower diagonal stem */
    for ( cur = head; cur!=NULL; cur=cur->next ) {
	for ( i=0; i<ptcnt; ++i ) {
	    nexti = i+1;
	    if ( touched[i]&tf_endcontour ) {
		for ( j=i; j>0 && !(touched[j]&tf_startcontour); --j);
		nexti = j;
	    }
	    if ( map[1][i]!=NULL || map[1][nexti]!=NULL ||
		    (map[0][i]!=NULL && map[0][i]==map[0][nexti] ))
	continue;	/* These points are already on a diagonal, off just enough that we can't find a line... */
	    if ( CoLinear(&cur->leftedgetop,&cur->leftedgebottom,&bp[i],&bp[nexti])) {
		top = &bp[i]; bottom = &bp[nexti];
		if ( top->y<bottom->y ) { top = bottom; bottom = &bp[i]; }
		cur->used[i] = cur->used[nexti] = 1;
		if ( cur->leftedgetop.y>top->y ) {
		    cur->leftedgetop = *top;
		    cur->pnum[0] = i;
		}
		if ( cur->leftedgebottom.y>bottom->y ) {
		    cur->leftedgebottom = *bottom;
		    cur->pnum[1] = i;
		}
		if ( map[0][i]==NULL ) map[0][i] = cur;
		else map[1][i] = cur;
		if ( map[0][nexti]==NULL ) map[0][nexti] = cur;
		else map[1][nexti] = cur;
	    } else if ( CoLinear(&cur->rightedgetop,&cur->rightedgebottom,&bp[i],&bp[nexti])) {
		top = &bp[i]; bottom = &bp[nexti];
		if ( top->y<bottom->y ) { top = bottom; bottom = &bp[i]; }
		cur->used[i] = cur->used[nexti] = 2;
		if ( cur->rightedgetop.y>top->y ) {
		    cur->rightedgetop = *top;
		    cur->pnum[2] = i;
		}
		if ( cur->rightedgebottom.y>bottom->y ) {
		    cur->rightedgebottom = *bottom;
		    cur->pnum[3] = i;
		}
		if ( map[0][i]==NULL ) map[0][i] = cur;
		else map[1][i] = cur;
		if ( map[0][nexti]==NULL ) map[0][nexti] = cur;
		else map[1][nexti] = cur;
	    }
	}
    }

    free(map[0]);
    free(map[1]);

return( head );
}

static void DStemFree(DStem *d) {
    DStem *next;
    struct dstemlist *dsl, *dslnext;

    while ( d!=NULL ) {
	next = d->next;
	for ( dsl=d->intersects; dsl!=NULL; dsl=dslnext ) {
	    dslnext = dsl->next;
	    chunkfree(dsl,sizeof(struct dstemlist));
	}
	free(d->used);
	chunkfree(d,sizeof(DStem));
	d = next;
    }
}

static struct dstemlist *BuildDStemIntersection(DStem *d,DStem *ds,
	struct dstemlist *i1, struct dstemlist **_i2,
	BasePoint *inter, enum intersect which, int pnum) {
    struct dstemlist *i2 = *_i2;

    if ( i1==NULL ) {
	i1 = chunkalloc(sizeof(struct dstemlist));
	memset(i1->pnum,-1,sizeof(i1->pnum));
	i1->ds = ds;
	i1->next = d->intersects;
	d->intersects = i1;
	*_i2 = i2 = chunkalloc(sizeof(struct dstemlist));
	memset(i2->pnum,-1,sizeof(i2->pnum));
	i2->ds = d;
	i2->next = ds->intersects;
	ds->intersects = i2;
    }
    if ( i1->is[which]!=NULL )
	IError("BuildDStemIntersection: is[%d] set twice", which);
    i1->is[which] = inter;
    i1->pnum[which] = pnum;
    if ( which==in_rl ) which = in_lr;
    else if ( which==in_lr ) which = in_rl;
    i2->is[which] = inter;
    i2->pnum[which] = pnum;
return( i1 );
}

static void DStemIntersect(DStem *d,BasePoint *bp,int ptcnt) {
    /* For each DStem, see if it intersects any other dstems */
    /* This is combinatorial hell. the only relief is that there are usually */
    /*  not very many dstems */
    DStem *ds;
    struct dstemlist *i1, *i2;
    int i;
    real maxy1, miny1, maxy2, miny2;

    while ( d!=NULL ) {
	maxy1 = d->leftedgetop.y>d->rightedgetop.y?d->leftedgetop.y:d->rightedgetop.y;
	miny1 = d->leftedgebottom.y<d->rightedgebottom.y?d->leftedgebottom.y:d->rightedgebottom.y;
	for ( ds=d->next; ds!=NULL; ds=ds->next ) {
	    maxy2 = ds->leftedgetop.y>ds->rightedgetop.y?ds->leftedgetop.y:ds->rightedgetop.y;
	    miny2 = ds->leftedgebottom.y<ds->rightedgebottom.y?ds->leftedgebottom.y:ds->rightedgebottom.y;
	    if ( maxy1<miny2 || maxy2<miny1 )
	continue;

	    i1 = i2 = NULL;
	    for ( i=0; i<ptcnt; ++i ) {
		if ( d->used[i] && ds->used[i] ) {
		    i1 = BuildDStemIntersection(d,ds,i1,&i2,
			    &bp[i],(d->used[i]==1?0:1)+(ds->used[i]==1?0:2),i);
		}
	    }
	}
	d = d->next;
    }
}

/* There is always the possibility that the top might intersect two different*/
/*  dstems (an example would be a misaligned X) */
static struct dstemlist *DStemTopAtIntersection(DStem *ds) {
    struct dstemlist *dl, *found=NULL;

    for ( dl=ds->intersects; dl!=NULL; dl=dl->next ) {
	/* check left and right top points */
	if ( ds->pnum[0] == dl->pnum[in_ll] || ds->pnum[0]==dl->pnum[in_lr] || ds->pnum[0]==dl->pnum[in_rl] ||
		ds->pnum[2] == dl->pnum[in_rr] || ds->pnum[2]==dl->pnum[in_lr] || ds->pnum[2]==dl->pnum[in_rl] ) {
	    if ( found!=NULL )
return( NULL );
	    else
		found = dl;
	}
    }
return( found );
}

static struct dstemlist *DStemBottomAtIntersection(DStem *ds) {
    struct dstemlist *dl, *found=NULL;

    for ( dl=ds->intersects; dl!=NULL; dl=dl->next ) {
	/* check left and right bottom points */
	if ( ds->pnum[1] == dl->pnum[in_ll] || ds->pnum[1]==dl->pnum[in_lr] || ds->pnum[1]==dl->pnum[in_rl] ||
		ds->pnum[3] == dl->pnum[in_rr] || ds->pnum[3]==dl->pnum[in_lr] || ds->pnum[3]==dl->pnum[in_rl] ) {
	    if ( found!=NULL )
return( NULL );
	    else
		found = dl;
	}
    }
return( found );
}

static int IsVStemIntersection(struct dstemlist *dl) {
    DStem *ds = dl->ds;

    /* We are passed an intersection at the end point of one stem. We want to */
    /*  know if the intersection is also at an end-point of the other stem */
    /*  if it is at the end-point then it is a "V" intersection, otherwise */
    /*  it might be a "k". */
    if ( ds->pnum[0] == dl->pnum[in_ll] || ds->pnum[0]==dl->pnum[in_lr] || ds->pnum[0]==dl->pnum[in_rl] ||
	    ds->pnum[1] == dl->pnum[in_ll] || ds->pnum[1]==dl->pnum[in_lr] || ds->pnum[1]==dl->pnum[in_rl] ||
	    ds->pnum[2] == dl->pnum[in_rr] || ds->pnum[2]==dl->pnum[in_lr] || ds->pnum[2]==dl->pnum[in_rl] ||
	    ds->pnum[3] == dl->pnum[in_rr] || ds->pnum[3]==dl->pnum[in_lr] || ds->pnum[3]==dl->pnum[in_rl] )
return( true );

return( false );
}

static int DStemWidth(DStem *ds) {
    /* find the orthogonal distance from the left stem to the right. Make it positive */
    /*  (just a dot product with the unit vector orthog to the left edge) */
    double tempx, tempy, len, stemwidth;

    tempx = ds->leftedgetop.x-ds->leftedgebottom.x;
    tempy = ds->leftedgetop.y-ds->leftedgebottom.y;
    len = sqrt(tempx*tempx+tempy*tempy);
    stemwidth = ((ds->rightedgetop.x-ds->leftedgetop.x)*tempy -
	    (ds->rightedgetop.y-ds->leftedgetop.y)*tempx)/len;
    if ( stemwidth<0 ) stemwidth = -stemwidth;
return( rint(stemwidth));
}

static int ExamineFreedom(int index,BasePoint *bp,int ptcnt,int def) {
    if ( index>0 && bp[index-1].x==bp[index].x )
	def = 1;
    else if ( index>0 && bp[index-1].y==bp[index].y )
	def = 0;
    else if ( index==0 && bp[ptcnt-1].x==bp[index].x )
	def = 1;
    else if ( index==0 && bp[ptcnt-1].y==bp[index].y )
	def = 0;
    else if ( index<ptcnt-1 && bp[index+1].x==bp[index].x )
	def = 1;
    else if ( index<ptcnt-1 && bp[index+1].y==bp[index].y )
	def = 0;
    else if ( index==ptcnt-1 && bp[0].x==bp[index].x )
	def = 1;
    else if ( index==ptcnt-1 && bp[0].y==bp[index].y )
	def = 0;
return( def );
}

static uint8 *KStemMoveToEdge(struct glyphinstrs *gi, uint8 *pt,
	struct dstemlist *dl,DStem *ds,int top,uint8 *touched) {
    /* we want to insure that either the top two points of ds or the bottom */
    /*  two are on the line specified by dl->ds */
    int e1, e2, m1, m2;
    real d1, d2;
    BasePoint *bp = top ? &ds->leftedgetop : &ds->leftedgebottom;
    int zerocvt = TTF_getcvtval(gi->sf,0);
    int data[7];
    int isword, i;

    d1 = (dl->ds->leftedgetop.y-dl->ds->leftedgebottom.y)*(bp->x-dl->ds->leftedgebottom.x) -
	 (dl->ds->leftedgetop.x-dl->ds->leftedgebottom.x)*(bp->y-dl->ds->leftedgebottom.y);
    d2 = (dl->ds->rightedgetop.y-dl->ds->rightedgebottom.y)*(bp->x-dl->ds->rightedgebottom.x) -
	 (dl->ds->rightedgetop.x-dl->ds->rightedgebottom.x)*(bp->y-dl->ds->rightedgebottom.y);
    if ( d1<0 ) d1 = -d1;
    if ( d2<0 ) d2 = -d2;

    if ( d1<d2 ) {
	e1 = dl->ds->pnum[0];
	e2 = dl->ds->pnum[1];
    } else {
	e1 = dl->ds->pnum[2];
	e2 = dl->ds->pnum[3];
    }
    m1 = ds->pnum[1-top];
    m2 = ds->pnum[3-top];

    data[0] = e2;
    data[1] = e1;
    data[2] = e1;
    data[3] = zerocvt;
    data[4] = m1;
    data[5] = zerocvt;
    data[6] = m2;
    isword = false;
    for ( i=0; i<7 ; ++i )
	if ( data[i]<0 || data[i]>255 ) {
	    isword = true;
    break;
	}
    pt = pushheader(pt,isword,7);
    for ( i=6; i>=0; --i )
	pt = addpoint(pt,isword,data[i]);

    *pt++ = 0x07;		/* SPVTL[orthog] */
    *pt++ = 0x0e;		/* SFVTP */
    *pt++ = 0x10;		/* SRP0 */
    *pt++ = 0xe0;		/* MIRP[00000] */
    *pt++ = 0xe0;		/* MIRP[00000] */
	/* If we set rnd then cvt cutin can screw us up */
	/* If we set min then... well we won't get a 0 distance */

    dl->done = true;

    touched[m1] |= 7;
    touched[m2] |= 7;
return( pt );
}

/* If the dot product of the freedom and projection vectors is negative */
/*  then I am unable to understand truetype's behavior. So insure it is */
/*  positive by flipping the stem used to compute it */
static uint8 *fixProjectionDir(uint8 *pt,int **ppt,BasePoint *bp,int top,
	int bottom, real freedomx, real freedomy, int *olddir) {
    int dir;

    dir = ((bp[top].y-bp[bottom].y)*freedomx - (bp[top].x-bp[bottom].x)*freedomy>0);
    if ( dir==*olddir )
return(pt);
    *olddir = dir;
    if ( dir ) {
	int temp = bottom;
	bottom = top;
	top = temp;
    }
    *(*ppt)++ = bottom;
    *(*ppt)++ = top;
    *pt++ = 7;			/* SPVTL[orthog] */
return( pt );
}

static uint8 *fixProjectionPtDir(uint8 *pt,int **ppt,BasePoint *bp,int top,
	int bottom, int free1, int free2, int parallel, int *olddir) {
    if ( parallel ) {
return( fixProjectionDir(pt,ppt,bp,top,bottom,
	bp[free1].x-bp[free2].x,bp[free1].y-bp[free2].y,olddir));
    } else {
return( fixProjectionDir(pt,ppt,bp,top,bottom,
	bp[free1].y-bp[free2].y,bp[free2].x-bp[free1].x,olddir));
    }
}

/* This is a simple stem (like "/" or "N" or "X") where none of the end points*/
/*  are at intersections with other diagonal stems (there may be intersections*/
/*  in the middle of the stem, but we're only interested in the edges so that */
/*  is ok */
/* I've extended it to handle the case of the lower stem in "k" where the stem*/
/*  does end at an interesection, but that intersection is NOT the endpoint */
/*  of the other stem */
/* This routine has far too many cases. */
/*  If either the top or bottom (or both) ends in another diagonal stem (I */
/*   refer to this as a "kstem" because that's what the lower stem of k does) */
/*   then we fix the two points at top or bottom so that they are on the */
/*   other diagonal stem. */
/*  Then we fix one edge (we choose the edge which is already most touched) */
/*   we fix each point on that edge in both axes (assuming it hasn't already */
/*   been fixed in that axis) */
/*  The other edge is harder. */
/*   We pick one of the two points on that edge, we prefer a point which is on*/
/*    another edge (diagonal, horizontal, vertical) */
/*   If we find an edge we will move the current point parallel to that edge */
/*    until it is stemwidth from the other side of the diagonal we are fixing */
/*   If we don't find an edge then move it perpendicular to our own diagonal */
/*   Then do the same for the other point, except that if we don't have an */
/*    edge then move the other point so that it is zero distance from the point*/
/*    we fixed above */
static uint8 *DStemFix(DStem *ds,struct glyphinstrs *gi,uint8 *pt,uint8 *touched,
	BasePoint *bp, int ptcnt) {
    int topvert, bottomvert;
    int topclosed, bottomclosed;
    int stemwidth = DStemWidth(ds);
    int stemcvt = TTF_getcvtval(gi->sf,stemwidth);
    int dx, dy, tdiff;
    int movement_axis[4];
    int freedom;
    int t1,t2, b1,b2, topfixed, rp0= -1, isword;
    int pushes[40], *ppt = pushes;
    uint8 tempinstrs[40], *ipt = tempinstrs;
    int i;
    int kstemtop, kstembottom;
    int move, based;
    int examined;
    int projection_dir;

    if ( (dx = ds->rightedgetop.x - ds->leftedgetop.x )<0 ) dx = -dx;
    if ( (dy = ds->rightedgetop.y - ds->leftedgetop.y )<0 ) dy = -dy;
    topvert = dy>dx;
    tdiff = dy+dx;
    if ( (dx = ds->rightedgebottom.x - ds->leftedgebottom.x )<0 ) dx = -dx;
    if ( (dy = ds->rightedgebottom.y - ds->leftedgebottom.y )<0 ) dy = -dy;
    bottomvert = dy>dx;
    topfixed = ( tdiff < dy+dx );

    /* A stem which is closed (like that of / or x) where the two bottom(top) */
    /*  end points are connected is much easier to deal with than an open one */
    /*  like N where there is no connection */
    topclosed = (ds->pnum[0]==ds->pnum[2]+1) || (ds->pnum[0]==ds->pnum[2]-1);
    bottomclosed = (ds->pnum[1]==ds->pnum[3]+1) || (ds->pnum[1]==ds->pnum[3]-1);

    movement_axis[0] = movement_axis[2] = topvert;
    movement_axis[1] = movement_axis[3] = bottomvert;
    if ( !topclosed ) {
	movement_axis[0] = ExamineFreedom(ds->pnum[0],bp,ptcnt,topvert);
	movement_axis[2] = ExamineFreedom(ds->pnum[2],bp,ptcnt,topvert);
    }
    if ( !bottomclosed ) {
	movement_axis[1] = ExamineFreedom(ds->pnum[1],bp,ptcnt,bottomvert);
	movement_axis[3] = ExamineFreedom(ds->pnum[3],bp,ptcnt,bottomvert);
    }

    /* which of the two edges is most heavily positioned? */
    if ( (touched[ds->pnum[0]]&(1<<movement_axis[0])) + (touched[ds->pnum[1]]&(1<<movement_axis[1])) >=
	    (touched[ds->pnum[2]]&(1<<movement_axis[2])) + (touched[ds->pnum[3]]&(1<<movement_axis[3])) ) {
	t1 = 0; t2 = 2;
	b1 = 1; b2 = 3;
    } else {
	t1 = 2; t2 = 0;
	b1 = 3; b2 = 1;
    }

    /* freedom vector starts in x direction */
    freedom = 0;

    /* I build the push stack sort of backwards */

    /* First we fix whichever edge we found above (if it needs to be), then */
    /*  establish the projection vector, then fix the other edge */
    /* (We fix the one edge before we define the projection vector as rounding*/
    /*  the points may warp the edge meaning the projection won't be right... */
    kstemtop = (ds->top!=NULL && !IsVStemIntersection(ds->top));
    if ( kstemtop ) {
	pt = KStemMoveToEdge(gi,pt,ds->top,ds,1,touched);
	freedom = 2;
    }
    kstembottom = (ds->bottom!=NULL && !IsVStemIntersection(ds->bottom));
    if ( kstembottom ) {
	pt = KStemMoveToEdge(gi,pt,ds->bottom,ds,0,touched);
	freedom = 3;
    }
    for ( i=0; i<2; ++i ) {
	if ( (touched[ds->pnum[t1]]&(1<<i)) || kstemtop )
	    /* Don't worry about it, already fixed in this axis */;
	else {
	    if ( freedom!=i ) { *ipt++ = 1-i; freedom = i; }
	    if ( i!=movement_axis[t1] ) {
		*ppt++ = ds->pnum[t1];
		*ipt++ = 0x2e;		/* MDAP [nornd] */
	    } else if ( (touched[ds->pnum[t2]]&(1<<i)) ) {
		/* if this point has not been touched, but it's opposite point */
		/*  has been then reestablish the distance between them. */
		/*  Note that we are interested in whether the opposite point has */
		/*  been touched in the direction that T1 is free to move, not the*/
		/*  dir t2 is free in */
		*ppt++ = ds->pnum[t2];
		*ipt++ = 0x10;		/* SRP0 */
		*ppt++ = ds->pnum[t1];
		*ipt++ = 0xcd;		/* MDRP [rp0, min, rnd, black] */
	    } else {
		*ppt++ = ds->pnum[t1];
		*ipt++ = 0x2f;		/* MDAP[1round] */
	    }
	    rp0 = ds->pnum[t1];
	    touched[ds->pnum[t1]] |= (1<<i)|4;
	}
	if ( (touched[ds->pnum[b1]]&(1<<i)) || kstembottom )
	    /* Don't worry about it */;
	else {
	    if ( freedom!=i ) { *ipt++ = 1-i; freedom = i; }
	    if ( i!=movement_axis[b1] ) {
		*ppt++ = ds->pnum[b1];
		*ipt++ = 0x2e;		/* MDAP [nornd] */
	    } else if ( (touched[ds->pnum[b2]]&(1<<i)) ) {
		*ppt++ = ds->pnum[b2];
		*ipt++ = 0x10;		/* SRP0 */
		*ppt++ = ds->pnum[b1];
		*ipt++ = 0xdd;		/* MDRP [rp0, min, rnd, black] */
	    } else {
		*ppt++ = ds->pnum[b1];
		*ipt++ = 0x2f;		/* MDAP[1round] */
	    }
	    rp0 = ds->pnum[b1];
	    touched[ds->pnum[b1]] |= (1<<i)|4;
	}
    }

    projection_dir = -1;

    if ( kstemtop || ds->rightedgetop.x == ds->leftedgetop.x ||
		ds->rightedgetop.y == ds->leftedgetop.y ||
	    (topfixed && !(kstembottom || ds->rightedgebottom.x == ds->leftedgebottom.x ||
		ds->rightedgetop.y == ds->leftedgetop.y )) ) {
	examined = ExamineFreedom(ds->pnum[t2],bp,ptcnt,-1);
	if ( examined==-1 || topclosed ) {
	    if ( ds->rightedgetop.x == ds->leftedgetop.x )
		examined = 1;
	    else if ( ds->rightedgetop.y == ds->leftedgetop.y )
		examined = 0;
	}
	if ( kstemtop ) {
	    /* set the freedom vector along the edge of the other stem */
	    *ppt++ = ds->top->ds->pnum[0];
	    *ppt++ = ds->top->ds->pnum[1];
	    *ipt++ = 8;			/* SFVTL[parallel] */
	    freedom = 3;
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->top->ds->pnum[0],ds->top->ds->pnum[1],1,&projection_dir);
	} else if ( examined!=-1 ) {
	    if ( freedom!=examined ) { *ipt++ = 0x05-examined; freedom = examined; }
	    ipt = fixProjectionDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    !freedom,freedom,&projection_dir);
	} else {
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->pnum[t1],ds->pnum[b1],0,&projection_dir);
	    *ipt++ = 0x0e;		/* SFVTPV */
	    freedom = 4;
	}
	if ( freedom<2 ) movement_axis[t2] = freedom;
	move = ds->pnum[t2];
	based = ds->pnum[t1];
    } else {
	examined = ExamineFreedom(ds->pnum[b2],bp,ptcnt,-1);
	if ( examined==-1 || bottomclosed ) {
	    if ( ds->rightedgebottom.x == ds->leftedgebottom.x )
		examined = 1;
	    else if ( ds->rightedgebottom.y == ds->leftedgebottom.y )
		examined = 0;
	}
	if ( kstembottom ) {
	    /* set the freedom vector along the edge of the other stem */
	    *ppt++ = ds->bottom->ds->pnum[0];
	    *ppt++ = ds->bottom->ds->pnum[1];
	    *ipt++ = 8;			/* SFVTL[parallel] */
	    freedom = 3;
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->bottom->ds->pnum[0],ds->bottom->ds->pnum[1],1,
		    &projection_dir);
	} else if ( examined!=-1 ) {
	    if ( freedom!=examined ) { *ipt++ = 0x05-examined; freedom = examined; }
	    ipt = fixProjectionDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    !freedom,freedom,&projection_dir);
	} else {
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->pnum[t1],ds->pnum[b1],0,&projection_dir);
	    *ipt++ = 0x0e;		/* SFVTPV */
	    freedom = 4;
	}
	if ( freedom<2 ) movement_axis[b2] = freedom;
	move = ds->pnum[b2];
	based = ds->pnum[b1];
    }
    if ( rp0!=based ) {
	*ppt++ = based;
	*ipt++ = 16;		/* SRP0 */
    }
    *ppt++ = stemcvt;
    *ppt++ = move;
    *ipt++ = 0xed;		/* MIRP[01101] */
    if ( freedom==1 || freedom==0 )
	touched[move] |= (1<<freedom)|4;
    else
	touched[move] |= 7;
    if ( move==ds->pnum[t2] ) {
	move = ds->pnum[b2];
	based = ds->pnum[b1];
	examined = ExamineFreedom(ds->pnum[b2],bp,ptcnt,-1);
	if ( examined==-1 || bottomclosed ) {
	    if ( ds->rightedgebottom.x == ds->leftedgebottom.x )
		examined = 1;
	    else if ( ds->rightedgebottom.y == ds->leftedgebottom.y )
		examined = 0;
	}
	if ( kstembottom ) {
	    /* set the freedom vector along the edge of the other stem */
	    *ppt++ = ds->bottom->ds->pnum[0];
	    *ppt++ = ds->bottom->ds->pnum[1];
	    *ipt++ = 8;			/* SFVTL[parallel] */
	    freedom = 3;
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->bottom->ds->pnum[0],ds->bottom->ds->pnum[1],1,&projection_dir);
	} else if ( examined!=-1 ) {
	    if ( freedom!=examined ) { *ipt++ = 0x05-examined; freedom = examined; }
	    ipt = fixProjectionDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    !freedom,freedom,&projection_dir);
	} else {
	    *ipt++ = 0x0e;		/* SFVTPV */
	    freedom = 4;
	    based = ds->pnum[t2];
	    stemcvt = TTF_getcvtval(gi->sf,0);
	}
	if ( freedom<2 ) movement_axis[b2] = freedom;
    } else {
	/* If the top were a kstem, or if either of the coord were equal */
	/*  then we would not have taken this branch. The only choice if */
	/*  we get here is the final case above where the best we can do */
	/*  is make the point be on the stem */
	*ipt++ = 0x0e;			/* SFVTPV */
	freedom = 4;
	based = ds->pnum[b2];
	stemcvt = TTF_getcvtval(gi->sf,0);
	move = ds->pnum[t2];
    }
    *ppt++ = based;
    *ipt++ = 16;		/* SRP0 */
    *ppt++ = stemcvt;
    *ppt++ = move;
    *ipt++ = stemcvt!=TTF_getcvtval(gi->sf,0)?0xed:0xe1;	/* MIRP[0??01] */
    if ( freedom==1 || freedom==0 )
	touched[move] |= (1<<freedom)|4;
    else
	touched[move] |= 7;

    freedom = 5;	/* Actually the freedom vector might be ok, but the projection won't be and we are interested in both now */

    /* Make sure all points are touched in both directions */
    if ( touched[ds->pnum[t2]]&(1<<(1-movement_axis[t2])) ) {
	if ( freedom!=1-movement_axis[t2] ) { *ipt++ = movement_axis[t2]; freedom = 1-movement_axis[t2]; }
	*ppt++ = ds->pnum[t2];
	*ipt++ = 0x2e;		/* MDAP[noround] */
	touched[ds->pnum[t2]] |= (1<<(1-movement_axis[t2]));
    }
    if ( touched[ds->pnum[b2]]&(1<<(1-movement_axis[b2])) ) {
	if ( freedom!=1-movement_axis[b2] ) { *ipt++ = movement_axis[b2]; freedom = 1-movement_axis[b2]; }
	*ppt++ = ds->pnum[b2];
	*ipt++ = 0x2e;		/* MDAP[noround] */
	touched[ds->pnum[b2]] |= (1<<(1-movement_axis[b2]));
    }
	
    if ( freedom!=0 )
	*ipt++ = 1;			/* always leave both vectors in x */

    /* Ok, now figure out the instructions for the push stack */
    isword = false;
    for ( i=0; pushes+i < ppt ; ++i )
	if ( pushes[i]<0 || pushes[i]>255 ) {
	    isword = true;
    break;
	}
    pt = pushheader(pt,isword,ppt-pushes);
    while ( ppt>pushes )
	pt = addpoint(pt,isword,*--ppt);
    for ( i=0; tempinstrs+i<ipt; ++i )
	*pt++ = tempinstrs[i];

    ds->done = true;
return( pt );
}

static uint8 *DStemFixConnected(DStem *ds,int top_is_inter,struct glyphinstrs *gi,
	uint8 *pt, uint8 *touched, BasePoint *bp, int ptcnt) {
    struct dstemlist *dl = top_is_inter ? ds->top : ds->bottom;
    DStem *nextds;
    int next_top_is_inter;
    int inner, outer1, outer2;
    int t1, t2, b1, b2;
    real sw1, sw2;
    int pushes[8];
    int isword, i;
    real dx, dy;

return( pt );
    /* This method doesn't work, I can't project backwards, or at least */
    /*  freetype won't let me. That may be a bug on their part, but even if */
    /*  it is I need to work around it */
#if 0
    SetupFPGM(gi);
#endif

    pt = DStemFix(ds,gi,pt,touched,bp,ptcnt);
    dl = top_is_inter ? ds->top : ds->bottom;
    sw1 = DStemWidth(ds);
    do {
	nextds = dl->ds;
	sw2 = DStemWidth(nextds);
	if ( top_is_inter ) {
	    if ( ds->pnum[0]==nextds->pnum[2] || ds->pnum[2]==nextds->pnum[0] ) {
		next_top_is_inter = true;
		if ( ds->pnum[0]!=nextds->pnum[2] ) {
		    inner = ds->pnum[2];
		    outer1 = ds->pnum[0];
		    outer2 = nextds->pnum[2];
		    t1 = outer1; t2 = outer2; b1 = ds->pnum[1]; b2 = nextds->pnum[3];
		} else if ( ds->pnum[2]!=nextds->pnum[0] ) {
		    inner = ds->pnum[0];
		    outer1 = ds->pnum[2];
		    outer2 = nextds->pnum[0];
		    t1 = outer1; t2 = outer2; b1 = ds->pnum[3]; b2 = nextds->pnum[1];
		} else if ( ds->leftedgetop.y>ds->rightedgetop.y ) {
		    inner = ds->pnum[2];
		    outer1 = outer2 = ds->pnum[0];
		    t1 = t2 = outer1; b1 = ds->pnum[1]; b2 = nextds->pnum[3];
		} else {
		    inner = ds->pnum[0];
		    outer1 = outer2 = ds->pnum[2];
		    t1 = outer1; t2 = outer2; b1 = ds->pnum[1]; b2 = nextds->pnum[3];
		}
	    } else if ( ds->pnum[0]==nextds->pnum[1] || ds->pnum[2] == nextds->pnum[3] ) {
		next_top_is_inter = false;
		if ( ds->pnum[0]!=nextds->pnum[3] ) {
		    inner = ds->pnum[2];
		    outer1 = ds->pnum[0];
		    outer2 = nextds->pnum[3];
		    t1 = outer1; b2 = outer2; b1 = ds->pnum[1]; t2 = nextds->pnum[2];
		} else if ( ds->pnum[2]!=nextds->pnum[1] ) {
		    inner = ds->pnum[0];
		    outer1 = ds->pnum[2];
		    outer2 = nextds->pnum[1];
		    t1 = outer1; b2 = outer2; b1 = ds->pnum[3]; t2 = nextds->pnum[0];
		} else {
		    /* This case doesn't matter much, so don't work hard to get it right */
		    inner = ds->pnum[0];
		    outer1 = outer2 = ds->pnum[2];
		    t1 = outer1; b2 = outer2; b1 = ds->pnum[3]; t2 = nextds->pnum[0];
		}
	    } else {
		IError( "Failed to find intersection in DStemFixConnected" );
return( pt );
	    }
	    if ( inner == ds->pnum[0] ) sw1 = -sw1;
	    else sw2 = -sw2;
	} else {
	    if ( ds->pnum[1]==nextds->pnum[3] || ds->pnum[3]==nextds->pnum[1] ) {
		next_top_is_inter = false;
		if ( ds->pnum[1]!=nextds->pnum[3] ) {
		    inner = ds->pnum[3];
		    outer1 = ds->pnum[1];
		    outer2 = nextds->pnum[3];
		    b1 = outer1; b2 = outer2; t1 = ds->pnum[0]; t2 = nextds->pnum[2];
		} else if ( ds->pnum[3]!=nextds->pnum[1] ) {
		    inner = ds->pnum[1];
		    outer1 = ds->pnum[3];
		    outer2 = nextds->pnum[1];
		    b1 = outer1; b2 = outer2; t1 = ds->pnum[2]; t2 = nextds->pnum[0];
		} else if ( ds->leftedgetop.y>ds->rightedgetop.y ) {
		    inner = ds->pnum[3];
		    outer1 = outer2 = ds->pnum[1];
		    b1 = b2 = outer1; t1 = ds->pnum[0]; t2 = nextds->pnum[2];
		} else {
		    inner = ds->pnum[1];
		    outer1 = outer2 = ds->pnum[3];
		    b1 = b2 = outer1; t1 = ds->pnum[2]; t2 = nextds->pnum[0];
		}
	    } else if ( ds->pnum[1]==nextds->pnum[0] || ds->pnum[3] == nextds->pnum[2] ) {
		next_top_is_inter = false;
		if ( ds->pnum[1]!=nextds->pnum[2] ) {
		    inner = ds->pnum[3];
		    outer1 = ds->pnum[1];
		    outer2 = nextds->pnum[2];
		} else if ( ds->pnum[3]!=nextds->pnum[0] ) {
		    inner = ds->pnum[1];
		    outer1 = ds->pnum[3];
		    outer2 = nextds->pnum[0];
		} else {
		    /* This case doesn't matter much, so don't work hard to get it right */
		    inner = ds->pnum[1];
		    outer1 = outer2 = ds->pnum[3];
		}
	    } else {
		IError( "Failed to find intersection in DStemFixConnected" );
return( pt );
	    }
	    if ( inner == ds->pnum[1] ) sw1 = -sw1;
	    else sw2 = -sw2;
	}
	if ( outer1!=outer2 ) {
	    /* if the outer edges don't come to a point, then make sure */
	    /*  there's a minimum distance between them */
	    if ( (dx = bp[outer2].x - bp[outer1].x )<0 ) dx = -dx;
	    if ( (dy = bp[outer2].y - bp[outer1].y )<0 ) dy = -dy;
	    /* outer1 should have been fixed already */
	    pt = pushpoint(pt,outer1);
	    *pt++ = 0x10;		/* SRP0 */
	    if ( dx>dy ) {
		pt = pushpointstem(pt,outer2,TTF_getcvtval(gi->sf,bp[outer1].x-bp[outer2].x));
		*pt++ = 0xe0+0x0d;	/* MIRP, minimum, rounded, black */
		touched[outer2] |= 1|4;
	    } else {
		*pt++ = 0x00;		/* SVTCA[y] */
		pt = pushpointstem(pt,outer2,TTF_getcvtval(gi->sf,bp[outer1].y-bp[outer2].y));
		*pt++ = 0xe0+0x0d;	/* MIRP, minimum, rounded, black */
		*pt++ = 0x01;		/* SVTCA[x] */
		touched[outer2] |= 2|4;
	    }
	}
	pt = DStemFix(nextds,gi,pt,touched,bp,ptcnt);

    /* I push the bottom then the top because that way the projection vector */
    /*  dot the freedom vector is positive. It it is negative the bytecode */
    /*  interpretters produce strange results */
	/* Do Intersect */
	pushes[0] = inner;
	pushes[1] = TTF__getcvtval(gi->sf,-sw1);
	pushes[3] = b1;
	pushes[2] = t1;
	pushes[4] = TTF__getcvtval(gi->sf,-sw2);
	pushes[6] = b2;
	pushes[5] = t2;
	pushes[7] = 0;
	isword = false;
	for ( i=0; i<8 ; ++i )
	    if ( pushes[i]<0 || pushes[i]>255 ) {
		isword = true;
	break;
	    }
	pt = pushheader(pt,isword,8);
	for ( i=0; i<8 ; ++i )
	    pt = addpoint(pt,isword,pushes[i]);
	*pt++ = 0x2b;			/* CALL (Function 0) */

	dl->done = true;

	ds = nextds;
	sw1 = sw2;
	if ( sw1<0 ) sw1 = -sw1;
	top_is_inter = !next_top_is_inter;
	dl = top_is_inter ? ds->top : ds->bottom;
    } while ( dl!=NULL && IsVStemIntersection(dl));

return( pt );
}

static uint8 *DStemCheckKStems(DStem *ds,struct glyphinstrs *gi,
	uint8 *pt, uint8 *touched, BasePoint *bp, int ptcnt) {
    /* Find all dstems like the lower stem of "k" where the other stem */
    /*  has already been set */
    DStem *ds2;
    int any;

    do {
	any = false;
	for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) if ( !ds2->done ) {
	    if ( (ds2->top==NULL || (ds2->top->done && !IsVStemIntersection(ds2->top))) &&
		    (ds2->bottom==NULL || (ds2->bottom->done && !IsVStemIntersection(ds2->bottom))) ) {
		pt = DStemFix(ds2,gi,pt,touched,bp,ptcnt);
		any = true;
	    }
	}
    } while ( any );
return( pt );
}

/* EndPoints have several different forms:
    /	easy, no complications
    x	easy, no (end point) complications
    N	mild difficulties in that some points are free to move in x and others
	in y (the points in the armpits of the stems move in y, the points
	outside move in x)
    V	have to go through machinations to figure out the bottom end point
    W	machinations for all the middle end points
    M
    k	Special case for the vertical end point
	Special case for diagonal end point
    25CA (diamond character) similar to V except all points are intersections
To establish a simple end point we pick one side of the stem and MDAP it (both top and bottom)
    then we set the projection vector orthogonal to the stem
    Usually we set the freedom vector to x (but for vertical k set to y)
    And move the other end point the desired stem width from the picked one

So...
    Find all endpoints with no intersections (simple end points) and establish them
    Find all intersection (V like) end points
	Try to find one where at least one of the sides is fixed above
		(if there are none then just pick one and fix it arbetrarily)
	Does the V come to a point? (or is its base flattened)
	    MDAP the pointy end
	else
	    MDAP one of the ends
	    Make the other end an minimum distance from it
	Call our intersection routine to figure where the intersection of the
		other sides goes
    Find all diagonal (k like) end points
	Make sure that the stem which does not end here has its endpoints fixed
		(the upper stem of "k")
	ISECT that stem with one of the edges of the other stem (lower k) and
		fix one point there.
	Set the projection vector orthog to the other stem (lower k) and move
		the end points of its untouched edge to be the stem width away
		from the fixed edge
	ISECT that edge with the non-ending stem (upper k)
*/
static uint8 *DStemEstablishEndPoints(struct glyphinstrs *gi,uint8 *pt,DStem *ds,
	uint8 *touched, BasePoint *bp, int ptcnt) {
    DStem *ds2;

    for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) {
	ds2->top = DStemTopAtIntersection(ds2);
	ds2->bottom = DStemBottomAtIntersection(ds2);
    }

    /* Find all dstems where neither end is an intersection and position them */
    /*  handles things like "/", "x", and the upper stem of "k" */
    for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) {
	if ( ds2->top==NULL && ds2->bottom==NULL )
	    pt = DStemFix(ds2,gi,pt,touched,bp,ptcnt);
    }

    /* Find all dstems like the lower stem of "k" where the other stem */
    /*  has already been set */
    pt = DStemCheckKStems(ds,gi,pt,touched,bp,ptcnt);

    /* Find all dstems where one end is not an intersection and position them */
    /*  and position anything connected to them */
    /*  handles things like "V", "W" */
    for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) if ( !ds2->done ) {
	if ( ds2->top!=NULL && IsVStemIntersection(ds2->top) &&
		(ds2->bottom==NULL || (ds2->bottom->done && !IsVStemIntersection(ds2->bottom))))
	    pt = DStemFixConnected(ds2,1,gi,pt,touched,bp,ptcnt);
	else if ( ds2->bottom!=NULL && IsVStemIntersection(ds2->bottom) &&
		(ds2->top==NULL || (ds2->top->done && !IsVStemIntersection(ds2->top))))
	    pt = DStemFixConnected(ds2,0,gi,pt,touched,bp,ptcnt);
    }

    pt = DStemCheckKStems(ds,gi,pt,touched,bp,ptcnt);

    /* Find all remaining dstems and arbetarily fix something and then */
    /*  position anything connected */
    /*  handles things like diamond */
    for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) if ( !ds2->done ) {
	if ( ds2->top && IsVStemIntersection(ds2->top))
	    pt = DStemFixConnected(ds2,1,gi,pt,touched,bp,ptcnt);
	else if ( ds2->bottom && IsVStemIntersection(ds2->bottom))
	    pt = DStemFixConnected(ds2,0,gi,pt,touched,bp,ptcnt);
	else
	    pt = DStemFix(ds2,gi,pt,touched,bp,ptcnt);
    }
return( pt );
}

static uint8 *DStemISectStem(uint8 *pt,DStem *ds,struct dstemlist *dl,
	uint8 *touched) {
    int pushes[32], *ppt = pushes;
    int cnt, isword, i;
    struct dstemlist *dl2;


    for ( dl2 = dl->ds->intersects; dl2!=NULL && dl2->ds!=ds; dl2=dl2->next );
    if ( dl2->done )
return( pt );
    if ( dl2!=NULL ) dl2->done = true;

    /* I have to push things backwards */
    for ( i=cnt=0; i<4; ++i ) if ( dl->is[i]!=NULL && dl->pnum[i]!=-1 ) {
	if ( i&1 ) {
	    *ppt++ = ds->pnum[2];
	    *ppt++ = ds->pnum[3];
	} else {
	    *ppt++ = ds->pnum[0];
	    *ppt++ = ds->pnum[1];
	}
	if ( i&2 ) {
	    *ppt++ = dl->ds->pnum[2];
	    *ppt++ = dl->ds->pnum[3];
	} else {
	    *ppt++ = dl->ds->pnum[0];
	    *ppt++ = dl->ds->pnum[1];
	}
	touched[dl->pnum[i]] |= 3|4;
	*ppt++ = dl->pnum[i];
	++cnt;
    }

    /* Ok, now figure out the instructions for the push stack */
    isword = false;
    for ( i=0; pushes+i < ppt ; ++i )
	if ( pushes[i]<0 || pushes[i]>255 ) {
	    isword = true;
    break;
	}
    pt = pushheader(pt,isword,ppt-pushes);
    while ( ppt>pushes )
	pt = addpoint(pt,isword,*--ppt);

    for ( i=0; i<cnt; ++i )
	*pt++ = 0x0f;		/* ISECT */
return( pt );
}

static uint8 *DStemISectMidPoints(uint8 *pt,DStem *ds,uint8 *touched) {
    struct dstemlist *dl;

    while ( ds!=NULL ) {
	for ( dl=ds->intersects; dl!=NULL; dl=dl->next ) if ( !dl->done )
	    pt = DStemISectStem(pt,ds,dl,touched);
	ds = ds->next;
    }
return( pt );
}

static uint8 *SetPFVectorsToSlope(uint8 *pt,int e1, int e2) {
    pt = pushpointstem(pt,e1,e2);
    *pt++ = 0x07;		/* SPVTL[orthog] */
    *pt++ = 0x0e;		/* SFVTP */
return( pt );
}

/* There may be other points on the diagonal stems which aren't at intersections */
/*  they still need to be moved so that they lie on the (new position of the) */
/*  stem */
static uint8 *DStemMakeColinear(struct glyphinstrs *gi,uint8 *pt,DStem *ds,
	uint8 *touched, BasePoint *bp, int ptcnt) {
    int i, established, positioned, any=0;
    int zerocvt = TTF_getcvtval(gi->sf,0);

    while ( ds!=NULL ) {
	/* touched[]&4 indicates a diagonal touch, only interested in points */
	/*  which have not been diagonally touched */
	established = positioned = 0;
	for ( i=0; i<ptcnt; ++i ) if ( !(touched[i]&4) && ds->used[i]==1 &&
		i!=ds->pnum[0] && i!=ds->pnum[1]) {
	    if ( !established ) {
		pt = SetPFVectorsToSlope(pt,ds->pnum[0],ds->pnum[1]);
		pt = pushpoint(pt,ds->pnum[0]);
		*pt++ = 0x10;		/* SRP0 */
		established = any = true;
	    }
	    pt = pushpointstem(pt,i,zerocvt);
	    *pt++ = 0xe0;			/* MIRP[00000] */
	    touched[i] |= 4;
	}
	for ( i=0; i<ptcnt; ++i ) if ( !(touched[i]&4) && ds->used[i]==2 &&
		i!=ds->pnum[2] && i!=ds->pnum[3]) {
	    if ( !established ) {
		pt = SetPFVectorsToSlope(pt,ds->pnum[0],ds->pnum[1]);
		established = any = true;
	    }
	    if ( !positioned ) {
		pt = pushpoint(pt,ds->pnum[2]);
		*pt++ = 0x10;		/* SRP0 */
		positioned = true;
	    }
	    pt = pushpointstem(pt,i,zerocvt);
	    *pt++ = 0xe0;			/* MIRP[00000] */
	    touched[i] |= 4;
	}
	ds = ds->next;
    }
    if ( any )
	*pt++ = 0x01;				/* SVTCA[x] */
return( pt );
}

static uint8 *DStemInfoGeninst(struct glyphinstrs *gi,uint8 *pt,DStemInfo *d,
	uint8 *touched, BasePoint *bp, int ptcnt) {
    DStem *ds;

#if TEST_DIAGHINTS
    if ( ds==NULL )		/* Comment out this line to turn off diagonal hinting !!!*/
#endif
return( pt );
    ds = DStemMerge(d,bp,ptcnt,touched);

    *pt++ = 0x30;	/* Interpolate Untouched Points y */
    *pt++ = 0x31;	/* Interpolate Untouched Points x */

    DStemIntersect(ds,bp,ptcnt);
    pt = DStemEstablishEndPoints(gi,pt,ds,touched,bp,ptcnt);
    pt = DStemISectMidPoints(pt,ds,touched);
    pt = DStemMakeColinear(gi,pt,ds,touched,bp,ptcnt);
    DStemFree(ds);
return( pt );
}

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

static uint8 *gen_extremum_instrs(struct glyphinstrs *gi, uint8 *instrs,
	BasePoint *bp, int ptcnt, uint8 *touched) {
    int i;
    real min, max;

    max = (int) 0x80000000; min = 0x7fffffff;
    for ( i=0; i<ptcnt; ++i ) {
	if ( min>bp[i].y ) min = bp[i].y;
	else if ( max<bp[i].y ) max = bp[i].y;
    }
    for ( i=0; i<ptcnt; ++i ) if ( !(touched[i]&2) && (bp[i].y==min || bp[i].y==max) ) {
	instrs = pushpoint(instrs,i);
	*instrs++ = 0x2f;			/* MDAP[rnd] */
	touched[i] |= 0x2;
    }
return( instrs );
}

void SCinitforinstrs(SplineChar *sc) {
}

static uint8 *dogeninstructions(SplineChar *sc, struct glyphinstrs *gi,
	int *contourends, BasePoint *bp, int ptcnt, SplineSet *ttfss,
	uint8 *touched) {
    StemInfo *hint;
    uint8 *instrs, *pt;
    int max;
    DStemInfo *dstem;

    /* Maximum instruction length is 6 bytes for each point in each dimension */
    /*  2 extra bytes to finish up. And one byte to switch from x to y axis */
    /* Diagonal take more space because we need to set the orientation on */
    /*  each stem, and worry about intersections, etc. */
    /*  That should be an over-estimate */
    max=2;
    if ( sc->vstem!=NULL ) max += 6*ptcnt;
    if ( sc->hstem!=NULL ) max += 6*ptcnt+1;
    for ( dstem=sc->dstem; dstem!=NULL; max+=7+4*6+100, dstem=dstem->next );
    if ( sc->md!=NULL ) max += 12*ptcnt;
    max += 6*ptcnt;			/* in case there are any rounds */
    max += 6*ptcnt;			/* paranoia */
    instrs = pt = galloc(max);

    for ( hint=sc->vstem; hint!=NULL; hint=hint->next )
	hint->enddone = hint->startdone = false;
    for ( hint=sc->hstem; hint!=NULL; hint=hint->next )
	hint->enddone = hint->startdone = false;

    /* first instruct horizontal stems (=> movement in y) */
    /*  do this first so that the diagonal hinter will have everything moved */
    /*  properly when it sets the projection vector */
    if ( sc->hstem!=NULL ) {
	*pt++ = 0x00;	/* Set Vectors to y */
	for ( hint=sc->hstem; hint!=NULL; hint=hint->next ) {
	    if ( !hint->startdone || !hint->enddone )
		pt = geninstrs(gi,pt,hint,contourends,bp,ptcnt,sc->hstem,false,touched);
	}
    }
    pt = gen_md_instrs(gi,pt,sc->md,ttfss,bp,ptcnt,false,touched);
    pt = gen_rnd_instrs(gi,pt,ttfss,bp,ptcnt,false,touched);
    pt = gen_extremum_instrs(gi,pt,bp,ptcnt,touched);

    /* next instruct vertical stems (=> movement in x) */
    if ( pt != instrs )
	*pt++ = 0x01;	/* Set Vectors to x */
    for ( hint=sc->vstem; hint!=NULL; hint=hint->next ) {
	if ( !hint->startdone || !hint->enddone )
	    pt = geninstrs(gi,pt,hint,contourends,bp,ptcnt,sc->vstem,true,touched);
    }

    /* finally instruct diagonal stems (=> movement in x) */
    pt = DStemInfoGeninst(gi,pt,sc->dstem,touched,bp,ptcnt);

    pt = gen_md_instrs(gi,pt,sc->md,ttfss,bp,ptcnt,true,touched);
    pt = gen_rnd_instrs(gi,pt,ttfss,bp,ptcnt,true,touched);

    /* there seems some discention as to which of these does x and which does */
    /*  y. So rather than try and be clever, let's always do both */
    *pt++ = 0x30;	/* Interpolate Untouched Points y */
    *pt++ = 0x31;	/* Interpolate Untouched Points x */
    if ( pt-instrs > max ) {
	fprintf(stderr,"We're about to crash.\nWe miscalculated the glyph's instruction set length\n" );
	fprintf(stderr,"When processing TTF instructions (hinting) of %s\n", sc->name );
    }
    sc->instructions_out_of_date = false;
    sc->ttf_instrs_len = (pt-instrs);
return( sc->ttf_instrs = grealloc(instrs,pt-instrs));
}

void SCAutoInstr(SplineChar *sc, BlueData *bd) {
    BlueData _bd;
    struct glyphinstrs gi;
    int cnt, contourcnt;
    BasePoint *bp;
    int *contourends;
    uint8 *touched;
    SplineSet *ss;
    RefChar *ref;

    if ( !sc->parent->order2 )
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

    if ( sc->vstem==NULL && sc->hstem==NULL && sc->dstem==NULL && sc->md==NULL )
return;
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

    dogeninstructions(sc, &gi, contourends, bp, cnt, sc->layers[ly_fore].splines, touched);

    free(touched);
    free(bp);
    free(contourends);

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    SCMarkInstrDlgAsChanged(sc);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    sc->complained_about_ptnums = false;
}
