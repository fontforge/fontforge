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

/* ************************************************************************** */
/* ********************** Type1 PostScript CharStrings ********************** */
/* ************************************************************************** */

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

static void moveto(GrowBuf *gb,BasePoint *current,BasePoint *to,
	int line, int round) {
    BasePoint temp;

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
	int round) {
    BasePoint temp1, temp2, temp3, *c0, *c1, *s1;

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

static void flexto(GrowBuf *gb,BasePoint *current,Spline *pspline,int round) {
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

static void CvtPsSplineSet(GrowBuf *gb, SplinePointList *spl, BasePoint *current, int round) {
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
	moveto(gb,current,&spl->first->me,false,round);
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( first==NULL ) first = spline;
	    if ( spline->to->flexx || spline->to->flexy ) {
		flexto(gb,current,spline,round);	/* does two adjacent splines */
		spline = spline->to->next;
	    } else if ( spline->islinear )
		moveto(gb,current,&spline->to->me,true,round);
	    else
		curveto(gb,current,spline,round);
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
    /* TrueType can only reference things where the elements of the transform-*/
    /*  ation matrix are less than 2 (and arbetrary translations) */
    for ( ref=ret; ref!=NULL; ref=ref->next ) {
	if ( isps ) {
	    if ( ref->transform[0]!=1 || ref->transform[3]!=1 ||
		    ref->transform[1]!=0 || ref->transform[2]!=0 )
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
	rt = r1; r1 = r2; r2 = r1;
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

static int CvtPsStem3(GrowBuf *gb, Hints *h1, Hints *h2, Hints *h3, double off,
	int ishstem, int round) {
    Hints _h1, _h2, _h3;

    if ( h1->width<0 ) {
	_h1 = *h1;
	_h1.base += _h1.width;
	_h1.width = -_h1.width;
	h1 = &_h1;
    }
    if ( h2->width<0 ) {
	_h2 = *h2;
	_h2.base += _h2.width;
	_h2.width = -_h2.width;
	h2 = &_h2;
    }
    if ( h3->width<0 ) {
	_h3 = *h3;
	_h3.base += _h3.width;
	_h3.width = -_h3.width;
	h3 = &_h3;
    }

    if ( h1->base>h2->base ) {
	Hints *ht = h1; h1 = h2; h2 = ht;
    }
    if ( h1->base>h3->base ) {
	Hints *ht = h1; h1 = h3; h3 = ht;
    }
    if ( h2->base>h3->base ) {
	Hints *ht = h2; h2 = h3; h3 = ht;
    }
    if ( h1->width != h3->width )
return( false );
    if ( (h2->base+h2->width/2) - (h1->base+h1->width/2) !=
	    (h3->base+h3->width/2) - (h2->base+h2->width/2) )
return( false );
    if ( gb->pt+51>=gb->end )
	GrowBuffer(gb);
    AddNumber(gb,h1->base-off,round);
    AddNumber(gb,h1->width,round);
    AddNumber(gb,h2->base-off,round);
    AddNumber(gb,h2->width,round);
    AddNumber(gb,h3->base-off,round);
    AddNumber(gb,h3->width,round);
    *(gb->pt)++ = 12;
    *(gb->pt)++ = ishstem?2:1;				/* h/v stem3 */
return( true );
}

static void CvtPsHints(GrowBuf *gb, Hints *h, double off, int ishstem, int round) {

    if ( h!=NULL && h->next!=NULL && h->next->next!=NULL &&
	    h->next->next->next==NULL )
	if ( CvtPsStem3(gb, h, h->next, h->next->next, off, ishstem, round))
return;

    while ( h!=NULL ) {
	if ( gb->pt+18>=gb->end )
	    GrowBuffer(gb);
	AddNumber(gb,h->base-off,round);
	AddNumber(gb,h->width,round);
	*(gb->pt)++ = ishstem?1:3;			/* h/v stem */
	h = h->next;
    }
}

static unsigned char *SplineChar2PS(SplineChar *sc,int *len,int round) {
    DBounds b;
    GrowBuf gb;
    BasePoint current;
    RefChar *rf;
    unsigned char *ret;

    memset(&gb,'\0',sizeof(gb));
    SplineCharFindBounds(sc,&b);
    AddNumber(&gb,b.minx,round);
    AddNumber(&gb,sc->width,round);
    *gb.pt++ = 13;				/* hsbw, lbearing & width */
    current.x = round?floor(b.minx):b.minx; current.y = 0;
    sc->lsidebearing = current.x;

    if ( IsSeacable(&gb,sc,round)) {
	/* All Done */;
    } else {
	if ( sc->changedsincelasthhinted && !sc->manualhints )
	    SplineCharAutoHint(sc);
	CvtPsHints(&gb,sc->hstem,0,true,round);
	CvtPsHints(&gb,sc->vstem,sc->lsidebearing,false,round);
	CvtPsSplineSet(&gb,sc->splines,&current,round);
	for ( rf = sc->refs; rf!=NULL; rf = rf->next )
	    CvtPsSplineSet(&gb,rf->splines,&current,round);
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

struct pschars *SplineFont2Chrs(SplineFont *sf, int round) {
    struct pschars *chrs = calloc(1,sizeof(struct pschars));
    int i, cnt;
    char notdefentry[20];

    cnt = 0;
    for ( i=0; i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL && strcmp(sf->chars[i]->name,".notdef")!=0 )
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
	if ( sf->chars[0]->origtype1!=NULL ) {
	    chrs->values[0] = (uint8 *) copyn((char *) sf->chars[0]->origtype1,sf->chars[0]->origlen);
	    chrs->lens[0] = sf->chars[0]->origlen;
	} else
	    chrs->values[0] = SplineChar2PS(sf->chars[0],&chrs->lens[0],round);
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
	    if ( sf->chars[i]->origtype1!=NULL ) {
		chrs->values[cnt] = (uint8 *) copyn((char *) sf->chars[i]->origtype1,sf->chars[i]->origlen);
		chrs->lens[cnt] = sf->chars[i]->origlen;
	    } else
		chrs->values[cnt] = SplineChar2PS(sf->chars[i],&chrs->lens[cnt],round);
	    ++cnt;
	}
	GProgressNext();
    }
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

static void moveto2(GrowBuf *gb,BasePoint *current,BasePoint *to) {

    if ( gb->pt+18 >= gb->end )
	GrowBuffer(gb);

#if 0
    if ( current->x==to->x && current->y==to->y ) {
	/* we're already here */
	/* Yes, but a move is required anyway at char start */
    } else
#endif
    if ( current->x==to->x ) {
	AddNumber2(gb,to->y-current->y);
	*(gb->pt)++ = 4;		/* v move to */
    } else if ( current->y==to->y ) {
	AddNumber2(gb,to->x-current->x);
	*(gb->pt)++ = 22;		/* h move to */
    } else {
	AddNumber2(gb,to->x-current->x);
	AddNumber2(gb,to->y-current->y);
	*(gb->pt)++ = 21;		/* r move to */
    }
    *current = *to;
}

static Spline *lineto2(GrowBuf *gb,BasePoint *current,Spline *spline, Spline *done) {
    int cnt, hv, hvcnt;
    Spline *test, *lastgood, *lasthvgood;

    for ( test=spline, cnt=0; test->islinear && cnt<15; ) {
	++cnt;
	lastgood = test;
	test = test->to->next;
	if ( test==done )
    break;
    }

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
		if ( test->from->me.x==test->to->me.x )
		    AddNumber2(gb,test->to->me.y-current->y);
		else
		    AddNumber2(gb,test->to->me.x-current->x);
		*current = test->to->me;
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
	AddNumber2(gb,test->to->me.x-current->x);
	AddNumber2(gb,test->to->me.y-current->y);
	*current = test->to->me;
	if ( test==lastgood )
    break;
    }
    if ( gb->pt+1 >= gb->end )
	GrowBuffer(gb);
    *(gb->pt)++ = 5;		/* r line to */
return( test->to->next );
}

static Spline *curveto2(GrowBuf *gb,BasePoint *current,Spline *spline, Spline *done) {
    int cnt=0, hv;
    Spline *first;
    BasePoint start;

    hv = -1;
    if ( current->x==spline->from->nextcp.x && spline->to->prevcp.y==spline->to->me.y )
	hv = 1;
    else if ( current->y==spline->from->nextcp.y && spline->to->prevcp.x==spline->to->me.x )
	hv = 0;
    if ( hv!=-1 ) {
	first = spline; start = *current;
	while (
		(hv==1 && current->x==spline->from->nextcp.x && spline->to->prevcp.y==spline->to->me.y ) ||
		(hv==0 && current->y==spline->from->nextcp.y && spline->to->prevcp.x==spline->to->me.x ) ) {
	    if ( hv==1 ) {
		AddNumber2(gb,spline->from->nextcp.y-current->y);
		AddNumber2(gb,spline->to->prevcp.x-spline->from->nextcp.x);
		AddNumber2(gb,spline->to->prevcp.y-spline->from->nextcp.y);
		AddNumber2(gb,spline->to->me.x-spline->to->prevcp.x);
		hv = 0;
	    } else {
		AddNumber2(gb,spline->from->nextcp.x-current->x);
		AddNumber2(gb,spline->to->prevcp.x-spline->from->nextcp.x);
		AddNumber2(gb,spline->to->prevcp.y-spline->from->nextcp.y);
		AddNumber2(gb,spline->to->me.y-spline->to->prevcp.y);
		hv = 1;
	    }
	    *current = spline->to->me;
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
	AddNumber2(gb,spline->from->nextcp.x-current->x);
	AddNumber2(gb,spline->from->nextcp.y-current->y);
	AddNumber2(gb,spline->to->prevcp.x-spline->from->nextcp.x);
	AddNumber2(gb,spline->to->prevcp.y-spline->from->nextcp.y);
	AddNumber2(gb,spline->to->me.x-spline->to->prevcp.x);
	AddNumber2(gb,spline->to->me.y-spline->to->prevcp.y);
	*current = spline->to->me;
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

static void flexto2(GrowBuf *gb,BasePoint *current,Spline *pspline) {
    BasePoint *c0, *c1, *mid, *end, *nc0, *nc1;
    Spline *nspline;

    c0 = &pspline->from->nextcp;
    c1 = &pspline->to->prevcp;
    mid = &pspline->to->me;
    nspline = pspline->to->next;
    nc0 = &nspline->from->nextcp;
    nc1 = &nspline->to->prevcp;
    end = &nspline->to->me;

    if ( c0->y==current->y && nc1->y==current->y && end->y==current->y &&
	    c1->y==mid->y && nc0->y==mid->y ) {
	if ( gb->pt+7*6+2 >= gb->end )
	    GrowBuffer(gb);
	AddNumber2(gb,c0->x-current->x);
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
	AddNumber2(gb,c0->x-current->x);
	AddNumber2(gb,c0->y-current->y);
	AddNumber2(gb,c1->x-c0->x);
	AddNumber2(gb,c1->y-c0->y);
	AddNumber2(gb,mid->x-c1->x);
	AddNumber2(gb,mid->y-c1->y);
	AddNumber2(gb,nc0->x-mid->x);
	AddNumber2(gb,nc0->y-mid->y);
	AddNumber2(gb,nc1->x-nc0->x);
	AddNumber2(gb,nc1->y-nc0->y);
	if ( current->y==end->y )
	    AddNumber2(gb,end->x-nc1->x);
	else
	    AddNumber2(gb,end->y-nc1->y);
	*gb->pt++ = 12; *gb->pt++ = 37;		/* flex1 */
    }

    *current = *end;
}

static void CvtPsSplineSet2(GrowBuf *gb, SplinePointList *spl, BasePoint *current) {
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
	moveto2(gb,current,&spl->first->me);
	/* Many improvements could be made here by giving multiple args to */
	/*  the commands, but I'm tired... !!!!! */
	for ( spline = spl->first->next; spline!=NULL && spline!=first; ) {
	    if ( first==NULL ) first = spline;
	    if ( spline->to->flexx || spline->to->flexy ) {
		flexto2(gb,current,spline);	/* does two adjacent splines */
		spline = spline->to->next->to->next;
	    } else if ( spline->islinear )
		spline = lineto2(gb,current,spline,first);
	    else
		spline = curveto2(gb,current,spline,first);
	}
	SplineSetReverse(spl);
	/* Of course, I have to Reverse again to get back to my convention after*/
	/*  saving */
    }
}

typedef struct mh {
    double base,width;
    int mask;
    struct mh *next;
} MaskedHint;

static void MaskedHintFree(MaskedHint *mh) {
    MaskedHint *next;

    while ( mh!=NULL ) {
	next = mh->next;
	free(mh);
	mh = next;
    }
}

static MaskedHint *OrderHints(MaskedHint *mh, Hints *h, double offset, int mask) {
    MaskedHint *prev, *new, *test;

    while ( h!=NULL ) {
	prev = NULL;
	for ( test=mh; test!=NULL && (h->base+offset>test->base ||
		(h->base+offset==test->base && h->width<test->width)); test=test->next )
	    prev = test;
	if ( test!=NULL && test->base==h->base+offset && test->width==h->width )
	    test->mask |= mask;		/* Actually a bit */
	else {
	    new = galloc(sizeof(MaskedHint));
	    new->next = test;
	    new->base = h->base+offset;
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

static void DumpHints(GrowBuf *gb,Hints *h,int oper) {
    double last = 0;

    if ( h==NULL )
return;
    while ( h!=NULL ) {
	AddNumber2(gb,h->base-last);
	AddNumber2(gb,h->width);
	last = h->base + h->width;
	h = h->next;
    }
    if ( oper!=-1 ) {
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*gb->pt++ = oper;
    }
}

static void DumpOrderedHints(GrowBuf *gb,MaskedHint *h,int oper) {
    double last = 0;

    while ( h!=NULL ) {
	AddNumber2(gb,h->base-last);
	AddNumber2(gb,h->width);
	last = h->base + h->width;
	h = h->next;
    }
    if ( oper!=-1 ) {
	if ( gb->pt+1>=gb->end )
	    GrowBuffer(gb);
	*gb->pt++ = oper;
    }
}

static void DumpHintMask(GrowBuf *gb,RefChar *cur,MaskedHint *h,MaskedHint *v) {
    char masks[12];
    int cnt, i;

    if ( h==NULL && v==NULL )
	GDrawIError("hintmask invoked when there are no hints");
    memset(masks,'\0',sizeof(masks));
    cnt = 0;
    while ( h!=NULL ) {
	if ( h->mask&cur->adobe_enc )
	    masks[cnt>>3] |= 1<<(7-(cnt&7));
	h = h->next; ++cnt;
    }
    while ( v!=NULL ) {
	if ( v->mask&cur->adobe_enc )
	    masks[cnt>>3] |= 1<<(7-(cnt&7));
	v = v->next; ++cnt;
    }
    if ( gb->pt+2+((cnt+7)>>3)>=gb->end )
	GrowBuffer(gb);
    *gb->pt++ = 19;				/* hintmask */
    for ( i=0; i< ((cnt+7)>>3); ++i )
	*gb->pt++ = masks[i];
}

static void ExpandRefList2(GrowBuf *gb, RefChar *refs, struct pschars *subrs) {
    RefChar *r;
    int hsccnt=0;
    BasePoint current, *bpt;
    MaskedHint *h, *v;

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
    if ( h!=NULL )
	DumpOrderedHints(gb,h,hsccnt>1?18:1);		/* hstemhm/hstem */
    if ( v!=NULL )
	DumpOrderedHints(gb,v, hsccnt<=1 ? 3:-1);	/* vstem */

    for ( r=refs; r!=NULL; r=r->next ) {
	if ( hsccnt>1 )
	    DumpHintMask(gb,r,h,v);
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
    MaskedHintFree(h); MaskedHintFree(v);
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
    BasePoint current;
    RefChar *rf;
    unsigned char *ret;

    memset(&gb,'\0',sizeof(gb));
    memset(&current,'\0',sizeof(current));

    CvtPsSplineSet2(&gb,sc->splines,&current);
    for ( rf = sc->refs; rf!=NULL; rf = rf->next )
	CvtPsSplineSet2(&gb,rf->splines,&current);
    if ( gb.pt+1>=gb.end )
	GrowBuffer(&gb);
    *gb.pt++ = 11;				/* return */
    ret = (unsigned char *) copyn((char *) gb.base,gb.pt-gb.base);
    *len = gb.pt-gb.base;
    free(gb.base);
    *_current = current;
return( ret );
}

static unsigned char *SplineChar2PS2(SplineChar *sc,int *len, int nomwid, int defwid, struct pschars *subrs) {
    GrowBuf gb;
    BasePoint current;
    RefChar *rf;
    unsigned char *ret;

    memset(&gb,'\0',sizeof(gb));
    memset(&current,'\0',sizeof(current));

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
	if ( sc->changedsincelasthhinted && !sc->manualhints )
	    SplineCharAutoHint(sc);
	DumpHints(&gb,sc->hstem,1);
	DumpHints(&gb,sc->vstem,3);
	CvtPsSplineSet2(&gb,sc->splines,&current);
	for ( rf = sc->refs; rf!=NULL; rf = rf->next )
	    CvtPsSplineSet2(&gb,rf->splines,&current);
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
	if ( sc!=NULL && sc->changedsincelasthhinted && !sc->manualhints )
	    SplineCharAutoHint(sc);
	if ( sc==NULL )
	    /* Do Nothing */;
	else if ( SCWorthOutputting(sc) &&
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
	else if ( SCWorthOutputting(sc) &&
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
