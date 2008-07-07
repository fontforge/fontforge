/* Copyright (C) 2000-2008 by George Williams */
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
#include "ustring.h"

void skewselect(BVTFunc *bvtf,real t) {
    real off, bestoff;
    int i, best;

    bestoff = 10; best = 0;
    for ( i=1; i<=10; ++i ) {
	if (  (off = t*i-rint(t*i))<0 ) off = -off;
	if ( off<bestoff ) {
	    bestoff = off;
	    best = i;
	}
    }

    bvtf->func = bvt_skew;
    bvtf->x = rint(t*best);
    bvtf->y = best;
}

void BCTransFunc(BDFChar *bc,enum bvtools type,int xoff,int yoff) {
    int i, j;
    uint8 *pt, *end, *pt2, *bitmap;
    int bpl, temp;
    int xmin, xmax;

    BCFlattenFloat(bc);
    if ( type==bvt_transmove ) {
	bc->xmin += xoff; bc->xmax += xoff;
	bc->ymin += yoff; bc->ymax += yoff;
	bitmap = NULL;
    } else if ( type==bvt_flipv ) {
	for ( i=0, j=bc->ymax-bc->ymin; i<j; ++i, --j ) {
	    pt = bc->bitmap + i*bc->bytes_per_line;
	    pt2 = bc->bitmap + j*bc->bytes_per_line;
	    end = pt+bc->bytes_per_line;
	    while ( pt<end ) {
		*pt ^= *pt2;
		*pt2 ^= *pt;
		*pt++ ^= *pt2++;
	    }
	}
	bitmap = NULL;
    } else if ( !bc->byte_data ) {
	if ( type==bvt_fliph ) {
	    bitmap = gcalloc((bc->ymax-bc->ymin+1)*bc->bytes_per_line,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		pt2 =    bitmap + i*bc->bytes_per_line;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    if ( pt[j>>3]&(1<<(7-(j&7))) ) {
			int nj = bc->xmax-bc->xmin-j;
			pt2[nj>>3] |= (1<<(7-(nj&7)));
		    }
		}
	    }
	} else if ( type==bvt_rotate180 ) {
	    bitmap = gcalloc((bc->ymax-bc->ymin+1)*bc->bytes_per_line,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		pt2 = bitmap + (bc->ymax-bc->ymin-i)*bc->bytes_per_line;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    if ( pt[j>>3]&(1<<(7-(j&7))) ) {
			int nj = bc->xmax-bc->xmin-j;
			pt2[nj>>3] |= (1<<(7-(nj&7)));
		    }
		}
	    }
	} else if ( type==bvt_rotate90cw ) {
	    bpl = ((bc->ymax-bc->ymin)>>3) + 1;
	    bitmap = gcalloc((bc->xmax-bc->xmin+1)*bpl,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    if ( pt[j>>3]&(1<<(7-(j&7))) ) {
			int nx = bc->ymax-bc->ymin-i;
			bitmap[j*bpl+(nx>>3)] |= (1<<(7-(nx&7)));
		    }
		}
	    }
	    bc->bytes_per_line = bpl;
	    temp = bc->xmax; bc->xmax = bc->ymax; bc->ymax = temp;
	    temp = bc->xmin; bc->xmin = bc->ymin; bc->ymin = temp;
	} else if ( type==bvt_rotate90ccw ) {
	    bpl = ((bc->ymax-bc->ymin)>>3) + 1;
	    bitmap = gcalloc((bc->xmax-bc->xmin+1)*bpl,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    if ( pt[j>>3]&(1<<(7-(j&7))) ) {
			int ny = bc->xmax-bc->xmin-j;
			bitmap[ny*bpl+(i>>3)] |= (1<<(7-(i&7)));
		    }
		}
	    }
	    bc->bytes_per_line = bpl;
	    temp = bc->xmax; bc->xmax = bc->ymax; bc->ymax = temp;
	    temp = bc->xmin; bc->xmin = bc->ymin; bc->ymin = temp;
	} else /* if ( type==bvt_skew ) */ {
	    if ( xoff>0 ) {
		xmin = bc->xmin+(xoff*bc->ymin)/yoff;
		xmax = bc->xmax+(xoff*bc->ymax)/yoff;
	    } else {
		xmin = bc->xmin+(xoff*bc->ymax)/yoff;
		xmax = bc->xmax+(xoff*bc->ymin)/yoff;
	    }
	    bpl = ((xmax-xmin)>>3) + 1;
	    bitmap = gcalloc((bc->ymax-bc->ymin+1)*bpl,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		pt2 = bitmap + i*bpl;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    if ( pt[j>>3]&(1<<(7-(j&7))) ) {
			int nj = j+bc->xmin-xmin + (xoff*(bc->ymax-i))/yoff;
			pt2[nj>>3] |= (1<<(7-(nj&7)));
		    }
		}
	    }
	    bc->xmax = xmax; bc->xmin = xmin; bc->bytes_per_line = bpl;
	}
    } else {		/* Byte data */
	if ( type==bvt_fliph ) {
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		for ( j=0; j<=(bc->xmax-bc->xmin)/2; ++j ) {
		    int nj = bc->xmax-bc->xmin-j;
		    int temp = pt[nj];
		    pt[nj] = pt[j];
		    pt[j] = temp;
		}
	    }
	    bitmap = NULL;
	} else if ( type==bvt_rotate180 ) {
	    bitmap = gcalloc((bc->ymax-bc->ymin+1)*bc->bytes_per_line,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		pt2 = bitmap + (bc->ymax-bc->ymin-i)*bc->bytes_per_line;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    int nj = bc->xmax-bc->xmin-j;
		    pt2[nj] = pt[j];
		}
	    }
	} else if ( type==bvt_rotate90cw ) {
	    bpl = bc->ymax-bc->ymin + 1;
	    bitmap = gcalloc((bc->xmax-bc->xmin+1)*bpl,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    int nx = bc->ymax-bc->ymin-i;
		    bitmap[j*bpl+nx] = pt[j];
		}
	    }
	    bc->bytes_per_line = bpl;
	    temp = bc->xmax; bc->xmax = bc->ymax; bc->ymax = temp;
	    temp = bc->xmin; bc->xmin = bc->ymin; bc->ymin = temp;
	} else if ( type==bvt_rotate90ccw ) {
	    bpl = bc->ymax-bc->ymin + 1;
	    bitmap = gcalloc((bc->xmax-bc->xmin+1)*bpl,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    int ny = bc->xmax-bc->xmin-j;
		    bitmap[ny*bpl+i] = pt[j];
		}
	    }
	    bc->bytes_per_line = bpl;
	    temp = bc->xmax; bc->xmax = bc->ymax; bc->ymax = temp;
	    temp = bc->xmin; bc->xmin = bc->ymin; bc->ymin = temp;
	} else /* if ( type==bvt_skew ) */ {
	    if ( xoff>0 ) {
		xmin = bc->xmin+(xoff*bc->ymin)/yoff;
		xmax = bc->xmax+(xoff*bc->ymax)/yoff;
	    } else {
		xmin = bc->xmin+(xoff*bc->ymax)/yoff;
		xmax = bc->xmax+(xoff*bc->ymin)/yoff;
	    }
	    bpl = xmax-xmin + 1;
	    bitmap = gcalloc((bc->ymax-bc->ymin+1)*bpl,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		pt2 = bitmap + i*bpl;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    int nj = j+bc->xmin-xmin + (xoff*(bc->ymax-i))/yoff;
		    pt2[nj] = pt[j];
		}
	    }
	    bc->xmax = xmax; bc->xmin = xmin; bc->bytes_per_line = bpl;
	}
    }
    if ( bitmap!=NULL ) {
	free(bc->bitmap);
	bc->bitmap = bitmap;
    }
    BCCompressBitmap(bc);
}

void BCTrans(BDFFont *bdf,BDFChar *bc,BVTFunc *bvts,FontViewBase *fv ) {
    int xoff=0, yoff=0, i;

    if ( bvts[0].func==bvt_none )
return;
    BCPreserveState(bc);
    for ( i=0; bvts[i].func!=bvt_none; ++i ) {
	if ( bvts[i].func==bvt_transmove ) {
	    xoff = rint(bvts[i].x*bdf->pixelsize/(double) (fv->sf->ascent+fv->sf->descent));
	    yoff = rint(bvts[i].y*bdf->pixelsize/(double) (fv->sf->ascent+fv->sf->descent));
	} else if ( bvts[i].func==bvt_skew ) {
	    xoff = bvts[i].x;
	    yoff = bvts[i].y;
	}
	BCTransFunc(bc,bvts[i].func,xoff,yoff);
    }
    BCCharChangedUpdate(bc);
}

void BCRotateCharForVert(BDFChar *bc,BDFChar *from, BDFFont *frombdf) {
    /* Take the image in from, make a copy of it, put it in bc, rotate it */
    /*  shift it around slightly so it is aligned properly to work as a CJK */
    /*  vertically displayed latin letter */
    int xmin, ymax;

    BCPreserveState(bc);
    BCFlattenFloat(from);
    free(bc->bitmap);
    bc->xmin = from->xmin; bc->xmax = from->xmax; bc->ymin = from->ymin; bc->ymax = from->ymax;
    bc->width = from->width; bc->bytes_per_line = from->bytes_per_line;
    bc->bitmap = galloc(bc->bytes_per_line*(bc->ymax-bc->ymin+1));
    memcpy(bc->bitmap,from->bitmap,bc->bytes_per_line*(bc->ymax-bc->ymin+1));
    BCTransFunc(bc,bvt_rotate90cw,0,0);
    xmin = frombdf->descent + from->ymin;
    ymax = frombdf->ascent - from->xmin;
    bc->xmax += xmin-bc->xmin; bc->xmin = xmin;
    bc->ymin += ymax-bc->ymax-1; bc->ymax = ymax-1;
    bc->width = frombdf->pixelsize;
}

static void BCExpandBitmap(BDFChar *bc, int x, int y) {
    int xmin, xmax, bpl, ymin, ymax;
    uint8 *bitmap;
    int i,j,nj;
    uint8 *pt, *npt;
    SplineChar *sc;

    if ( x<bc->xmin || x>bc->xmax || y<bc->ymin || y>bc->ymax ) {
	xmin = x<bc->xmin?x:bc->xmin;
	xmax = x>bc->xmax?x:bc->xmax;
	ymin = y<bc->ymin?y:bc->ymin;
	ymax = y>bc->ymax?y:bc->ymax;
	if ( !bc->byte_data ) {
	    bpl = ((xmax-xmin)>>3) + 1;
	    bitmap = gcalloc((ymax-ymin+1)*bpl,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		npt = bitmap + (i+ymax-bc->ymax)*bpl;
		for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
		    if ( pt[j>>3] & (1<<(7-(j&7))) ) {
			nj = j+bc->xmin-xmin;
			npt[nj>>3] |= (1<<(7-(nj&7)));
		    }
		}
	    }
	} else {
	    bpl = xmax-xmin + 1;
	    bitmap = gcalloc((ymax-ymin+1)*bpl,sizeof(uint8));
	    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
		pt = bc->bitmap + i*bc->bytes_per_line;
		npt = bitmap + (i+ymax-bc->ymax)*bpl;
		memcpy(npt+bc->xmin-xmin,pt,bc->bytes_per_line);
	    }
	}
	free(bc->bitmap);
	bc->bitmap = bitmap;
	bc->xmin = xmin; bc->xmax = xmax; bc->bytes_per_line = bpl;
	bc->ymin = ymin; bc->ymax = ymax;

	sc = bc->sc;			/* sc can be NULL when loading bitmap references from an sfnt */
	if ( sc!=NULL && sc->parent!=NULL && sc->parent->onlybitmaps )
	    sc->widthset = true;	/* Mark it as used */
    }
}

void BCExpandBitmapToEmBox(BDFChar *bc, int xmin, int ymin, int xmax, int ymax) {
    /* Expand a bitmap so it will always contain 0..width, ascent..descent */
    BCExpandBitmap(bc,xmin,ymin);
    BCExpandBitmap(bc,xmax,ymax);
}

void BCSetPoint(BDFChar *bc, int x, int y, int color ) {

    if ( x<bc->xmin || x>bc->xmax || y<bc->ymin || y>bc->ymax ) {
	if ( color==0 )
return;		/* Already clear */
	BCExpandBitmap(bc,x,y);
    }
    y = bc->ymax-y;
    x -= bc->xmin;
    if ( bc->byte_data )
	bc->bitmap[y*bc->bytes_per_line+x] = color;
    else if ( color==0 )
	bc->bitmap[y*bc->bytes_per_line+(x>>3)] &= ~(1<<(7-(x&7)));
    else
	bc->bitmap[y*bc->bytes_per_line+(x>>3)] |= (1<<(7-(x&7)));
}

void BDFFloatFree(BDFFloat *sel) {
    if ( sel==NULL )
return;
    free(sel->bitmap);
    free(sel);
}

BDFFloat *BDFFloatCopy(BDFFloat *sel) {
    BDFFloat *new;
    if ( sel==NULL )
return(NULL);
    new = galloc(sizeof(BDFFloat));
    *new = *sel;
    new->bitmap = galloc(sel->bytes_per_line*(sel->ymax-sel->ymin+1));
    memcpy(new->bitmap,sel->bitmap,sel->bytes_per_line*(sel->ymax-sel->ymin+1));
return( new );
}

BDFFloat *BDFFloatConvert(BDFFloat *sel,int todepth,int fromdepth) {
    BDFFloat *new;
    int i,j,fdiv,tdiv;
    if ( sel==NULL )
return(NULL);

    if ( todepth==fromdepth )
return( BDFFloatCopy(sel));

    new = galloc(sizeof(BDFFloat));
    *new = *sel;
    new->byte_data = todepth!=1;
    new->depth = todepth;
    new->bytes_per_line = new->byte_data ? new->xmax-new->xmin+1 : ((new->xmax-new->xmin)>>3)+1;
    new->bitmap = gcalloc(new->bytes_per_line*(sel->ymax-sel->ymin+1),1);
    if ( fromdepth==1 ) {
	tdiv = ((1<<todepth)-1);
	for ( i=0; i<=sel->ymax-sel->ymin; ++i ) {
	    for ( j=0; j<=sel->xmax-sel->xmin; ++j ) {
		if ( sel->bitmap[i*sel->bytes_per_line+(j>>3)]&(0x80>>(j&7)) )
		    new->bitmap[i*new->bytes_per_line+j] = tdiv;
	    }
	}
    } else if ( todepth==1 ) {
	fdiv = (1<<fromdepth)/2;
	for ( i=0; i<=sel->ymax-sel->ymin; ++i ) {
	    for ( j=0; j<=sel->xmax-sel->xmin; ++j ) {
		if ( sel->bitmap[i*sel->bytes_per_line+j]>=fdiv )
		    new->bitmap[i*new->bytes_per_line+(j>>3)] |= (0x80>>(j&7));
	    }
	}
    } else {
	fdiv = 255/((1<<fromdepth)-1); tdiv = 255/((1<<todepth)-1);
	memcpy(new->bitmap,sel->bitmap,sel->bytes_per_line*(sel->ymax-sel->ymin+1));
	for ( i=0 ; i<sel->bytes_per_line*(sel->ymax-sel->ymin+1); ++i )
	    new->bitmap[i] = (sel->bitmap[i]*fdiv + tdiv/2)/tdiv;
    }
return( new );
}

/* Creates a floating selection, and clears out the underlying bitmap */
BDFFloat *BDFFloatCreate(BDFChar *bc,int xmin,int xmax,int ymin,int ymax, int clear) {
    BDFFloat *new;
    int x,y;
    uint8 *bpt, *npt;

    if ( bc->selection!=NULL ) {
	BCFlattenFloat(bc);
	bc->selection = NULL;
    }
    BCCompressBitmap(bc);

    if ( xmin>xmax ) {
	xmin ^= xmax; xmax ^= xmin; xmin ^= xmax;
    }
    if ( ymin>ymax ) {
	ymin ^= ymax; ymax ^= ymin; ymin ^= ymax;
    }
    if ( xmin<bc->xmin ) xmin = bc->xmin;
    if ( xmax>bc->xmax ) xmax = bc->xmax;
    if ( ymin<bc->ymin ) ymin = bc->ymin;
    if ( ymax>bc->ymax ) ymax = bc->ymax;
    if ( xmin>xmax || ymin>ymax )
return( NULL );
    new = galloc(sizeof(BDFFloat));
    new->xmin = xmin;
    new->xmax = xmax;
    new->ymin = ymin;
    new->ymax = ymax;
    new->byte_data = bc->byte_data;
    new->depth = bc->depth;
    if ( bc->byte_data ) {
	new->bytes_per_line = xmax-xmin+1;
	new->bitmap = gcalloc(new->bytes_per_line*(ymax-ymin+1),sizeof(uint8));
	for ( y=ymin; y<=ymax; ++y ) {
	    bpt = bc->bitmap + (bc->ymax-y)*bc->bytes_per_line;
	    npt = new->bitmap + (ymax-y)*new->bytes_per_line;
	    memcpy(npt,bpt+xmin-bc->xmin,xmax-xmin+1);
	    if ( clear )
		memset(bpt+xmin-bc->xmin,0,xmax-xmin+1);
	}
    } else {
	new->bytes_per_line = ((xmax-xmin)>>3)+1;
	new->bitmap = gcalloc(new->bytes_per_line*(ymax-ymin+1),sizeof(uint8));
	for ( y=ymin; y<=ymax; ++y ) {
	    bpt = bc->bitmap + (bc->ymax-y)*bc->bytes_per_line;
	    npt = new->bitmap + (ymax-y)*new->bytes_per_line;
	    for ( x=xmin; x<=xmax; ++x ) {
		int bx = x-bc->xmin, nx = x-xmin;
		if ( bpt[bx>>3]&(1<<(7-(bx&7))) ) {
		    npt[nx>>3] |= (1<<(7-(nx&7)));
		    if ( clear )
			bpt[bx>>3] &= ~(1<<(7-(bx&7)));
		}
	    }
	}
    }
    if ( clear )
	bc->selection = new;
return( new );
}

void BCFlattenFloat(BDFChar *bc ) {
    /* flatten any floating selection */
    BDFFloat *sel = bc->selection;
    int x,y;
    uint8 *bpt, *spt;

    if ( sel!=NULL ) {
	BCExpandBitmap(bc,sel->xmin,sel->ymin);
	BCExpandBitmap(bc,sel->xmax,sel->ymax);
	if ( bc->byte_data ) {
	    for ( y=sel->ymin; y<=sel->ymax; ++y ) {
		bpt = bc->bitmap + (bc->ymax-y)*bc->bytes_per_line;
		spt = sel->bitmap + (sel->ymax-y)*sel->bytes_per_line;
		memcpy(bpt+sel->xmin-bc->xmin,spt,sel->xmax-sel->xmin+1);
	    }
	} else {
	    for ( y=sel->ymin; y<=sel->ymax; ++y ) {
		bpt = bc->bitmap + (bc->ymax-y)*bc->bytes_per_line;
		spt = sel->bitmap + (sel->ymax-y)*sel->bytes_per_line;
		for ( x=sel->xmin; x<=sel->xmax; ++x ) {
		    int bx = x-bc->xmin, sx = x-sel->xmin;
		    if ( spt[sx>>3]&(1<<(7-(sx&7))) )
			bpt[bx>>3] |= (1<<(7-(bx&7)));
		    else
			bpt[bx>>3] &= ~(1<<(7-(bx&7)));
		}
	    }
	}
	BDFFloatFree(sel);
	bc->selection = NULL;
    }
}

void BCPasteInto(BDFChar *bc,BDFChar *rbc,int ixoff,int iyoff, int invert, int cleartoo) {
    int x,y;
    uint8 *bpt, *rpt;

    x = 0;
    BCExpandBitmap(bc,rbc->xmin+ixoff,rbc->ymin+iyoff);
    BCExpandBitmap(bc,rbc->xmax+ixoff,rbc->ymax+iyoff);
    for ( y=rbc->ymin; y<=rbc->ymax; ++y ) {
	bpt = bc->bitmap + (bc->ymax-(y+iyoff))*bc->bytes_per_line;
	if ( invert )
	    rpt = rbc->bitmap + y*rbc->bytes_per_line;
	else
	    rpt = rbc->bitmap + (rbc->ymax-y)*rbc->bytes_per_line;
	if ( bc->byte_data )
	    memcpy(bpt+x+ixoff-bc->xmin,rpt,rbc->xmax-rbc->xmin+1);
	else {
	    for ( x=rbc->xmin; x<=rbc->xmax; ++x ) {
		int bx = x+ixoff-bc->xmin, rx = x-rbc->xmin;
		if ( rpt[rx>>3]&(1<<(7-(rx&7))) )
		    bpt[bx>>3] |= (1<<(7-(bx&7)));
		else if ( cleartoo )
		    bpt[bx>>3] &= ~(1<<(7-(bx&7)));
	    }
	}
    }
    BCCompressBitmap(bc);
}

void BDFCharFindBounds(BDFChar *bc,IBounds *bb) {
    int r,c,first=true;

    if ( bc->byte_data ) {
	for ( r=0; r<=bc->ymax-bc->ymin; ++r ) {
	    for ( c=0; c<=bc->xmax-bc->xmin; ++c ) {
		if ( bc->bitmap[r*bc->bytes_per_line+c] ) {
		    if ( first ) {
			bb->minx = bb->maxx = bc->xmin+c;
			bb->miny = bb->maxy = bc->ymax-r;
			first = false;
		    } else {
			if ( bc->xmin+c<bb->minx ) bb->minx = bc->xmin+c;
			if ( bc->xmin+c>bb->maxx ) bb->maxx = bc->xmin+c;
			bb->miny = bc->ymax-r;
		    }
		}
	    }
	}
    } else {
	for ( r=0; r<=bc->ymax-bc->ymin; ++r ) {
	    for ( c=0; c<=bc->xmax-bc->xmin; ++c ) {
		if ( bc->bitmap[r*bc->bytes_per_line+(c>>3)]&(0x80>>(c&7)) ) {
		    if ( first ) {
			bb->minx = bb->maxx = bc->xmin+c;
			bb->miny = bb->maxy = bc->ymax-r;
			first = false;
		    } else {
			if ( bc->xmin+c<bb->minx ) bb->minx = bc->xmin+c;
			if ( bc->xmin+c>bb->maxx ) bb->maxx = bc->xmin+c;
			bb->miny = bc->ymax-r;
		    }
		}
	    }
	}
    }
    if ( first )
	memset(bb,0,sizeof(*bb));
}

static BDFChar *BCScale(BDFChar *old,int from, int to) {
    BDFChar *new;
    int x,y, ox,oy, oxs,oys, oxend, oyend;
    real tot, scale;
    real yscale, xscale;
    real dto = to;

    if ( old==NULL || old->byte_data )
return( NULL );
    new = chunkalloc(sizeof(BDFChar));
    new->sc = old->sc;
    new->xmin = rint(old->xmin*dto/from);
    new->ymin = rint(old->ymin*dto/from);
    new->xmax = new->xmin + ceil((old->xmax-old->xmin+1)*dto/from-1);
    new->ymax = new->ymin + ceil((old->ymax-old->ymin+1)*dto/from-1);
    if ( new->sc!=NULL && new->sc->width != new->sc->parent->ascent+new->sc->parent->descent )
	new->width = rint(new->sc->width*dto/(new->sc->parent->ascent+new->sc->parent->descent)+.5);
    else
	new->width = rint(old->width*dto/from+.5);
    if ( new->sc!=NULL && new->sc->vwidth != new->sc->parent->ascent+new->sc->parent->descent )
	new->vwidth = rint(new->sc->vwidth*dto/(new->sc->parent->ascent+new->sc->parent->descent)+.5);
    else
	new->vwidth = rint(old->vwidth*dto/from+.5);
    new->bytes_per_line = (new->xmax-new->xmin)/8+1;
    new->bitmap = gcalloc((new->ymax-new->ymin+1)*new->bytes_per_line,sizeof(char));
    new->orig_pos = old->orig_pos;

    scale = from/dto;
    scale *= scale;
    scale /= 2;

    for ( y=0; y<=new->ymax-new->ymin; ++y ) for ( x=0; x<=new->xmax-new->xmin; ++x ) {
	tot = 0;
	oys = floor(y*from/dto); oyend = ceil((y+1)*from/dto);
	oxs = floor(x*from/dto); oxend = ceil((x+1)*from/dto);
	for ( oy = oys; oy<oyend && oy<=old->ymax-old->ymin; ++oy ) {
	    yscale = 1;
	    if ( oy==oys && oy==oyend-1 )
		yscale = 1-(oyend - (y+1)*from/dto) - (y*from/dto - oys);
	    else if ( oy==oys )
		yscale = 1-(y*from/dto-oys);
	    else if ( oy==oyend-1 )
		yscale = 1-(oyend - (y+1)*from/dto);
	    for ( ox = oxs; ox<oxend && ox<=old->xmax-old->xmin; ++ox ) {
		if ( old->bitmap[oy*old->bytes_per_line + (ox>>3)] & (1<<(7-(ox&7))) ) {
		    xscale = 1;
		    if ( ox==oxs && ox==oxend-1 )
			xscale = 1-(oxend - (x+1)*from/dto) - (x*from/dto - oxs);
		    else if ( ox==oxs )
			xscale = 1-(x*from/dto-oxs);
		    else if ( ox==oxend-1 )
			xscale = 1-(oxend - (x+1)*from/dto);
		    tot += yscale*xscale;
		}
	    }
	}
	if ( tot>=scale )
	    new->bitmap[y*new->bytes_per_line + (x>>3)] |= (1<<(7-(x&7)));
    }

return( new );
}

/* Scale a greymap character to either another greymap or a bitmap */
static BDFChar *BCScaleGrey(BDFChar *old,int from, int from_depth, int to, int to_depth) {
    BDFChar *new;
    int x,y, ox,oy, oxs,oys, oxend, oyend;
    real tot, scale, bscale;
    real yscale, xscale;
    real dto = to;
    real max = (1<<to_depth);

    if ( old==NULL || !old->byte_data )
return( NULL );
    new = chunkalloc(sizeof(BDFChar));
    new->sc = old->sc;
    new->xmin = rint(old->xmin*dto/from);
    new->ymin = rint(old->ymin*dto/from);
    new->xmax = new->xmin + ceil((old->xmax-old->xmin+1)*dto/from-1);
    new->ymax = new->ymin + ceil((old->ymax-old->ymin+1)*dto/from-1);
    if ( new->sc!=NULL && new->sc->width != new->sc->parent->ascent+new->sc->parent->descent )
	new->width = rint(new->sc->width*dto/(new->sc->parent->ascent+new->sc->parent->descent)+.5);
    else
	new->width = rint(old->width*dto/from+.5);
    if ( new->sc!=NULL && new->sc->vwidth != new->sc->parent->ascent+new->sc->parent->descent )
	new->vwidth = rint(new->sc->vwidth*dto/(new->sc->parent->ascent+new->sc->parent->descent)+.5);
    else
	new->vwidth = rint(old->vwidth*dto/from+.5);
    if ( to_depth==1 ) {
	new->bytes_per_line = (new->xmax-new->xmin)/8+1;
	new->bitmap = gcalloc((new->ymax-new->ymin+1)*new->bytes_per_line,sizeof(char));
    } else {
	new->bytes_per_line = (new->xmax-new->xmin)+1;
	new->bitmap = gcalloc((new->ymax-new->ymin+1)*new->bytes_per_line,sizeof(char));
	new->byte_data = true;
    }
    new->orig_pos = old->orig_pos;

    scale = from/dto;
    scale *= scale;
    scale *= (1<<from_depth);
    bscale = scale/2;

    for ( y=0; y<=new->ymax-new->ymin; ++y ) for ( x=0; x<=new->xmax-new->xmin; ++x ) {
	tot = 0;
	oys = floor(y*from/dto); oyend = ceil((y+1)*from/dto);
	oxs = floor(x*from/dto); oxend = ceil((x+1)*from/dto);
	for ( oy = oys; oy<oyend && oy<=old->ymax-old->ymin; ++oy ) {
	    yscale = 1;
	    if ( oy==oys && oy==oyend-1 )
		yscale = 1-(oyend - (y+1)*from/dto) - (y*from/dto - oys);
	    else if ( oy==oys )
		yscale = 1-(y*from/dto-oys);
	    else if ( oy==oyend-1 )
		yscale = 1-(oyend - (y+1)*from/dto);
	    for ( ox = oxs; ox<oxend && ox<=old->xmax-old->xmin; ++ox ) {
		xscale = 1;
		if ( ox==oxs && ox==oxend-1 )
		    xscale = 1-(oxend - (x+1)*from/dto) - (x*from/dto - oxs);
		else if ( ox==oxs )
		    xscale = 1-(x*from/dto-oxs);
		else if ( ox==oxend-1 )
		    xscale = 1-(oxend - (x+1)*from/dto);
		tot += yscale*xscale* old->bitmap[oy*old->bytes_per_line + ox];
	    }
	}
	if ( to_depth!=1 ) {
	    real temp = tot*max/scale;
	    if ( temp>=max )
		temp = max-1;
	    new->bitmap[y*new->bytes_per_line + x] = (int) rint(temp);
	} else if ( tot>=bscale ) {
	    new->bitmap[y*new->bytes_per_line + (x>>3)] |= (1<<(7-(x&7)));
	}
    }

return( new );
}

BDFFont *BitmapFontScaleTo(BDFFont *old, int to) {
    BDFFont *new = chunkalloc(sizeof(BDFFont));
    int i;
    int to_depth = (to>>16), old_depth = 1;
    int linear_scale = 1<<(to_depth/2);

    to &= 0xffff;

    if ( old->clut!=NULL )
	old_depth = BDFDepth(old);

    new->sf = old->sf;
    new->glyphcnt = new->glyphmax = old->glyphcnt;
    new->glyphs = galloc(new->glyphcnt*sizeof(BDFChar *));
    new->pixelsize = to;
    new->ascent = (old->ascent*to+.5)/old->pixelsize;
    new->descent = to-new->ascent;
    new->foundry = copy(old->foundry);
    new->res = -1;
    for ( i=0; i<old->glyphcnt; ++i ) {
	if ( old->clut==NULL ) {
	    new->glyphs[i] = BCScale(old->glyphs[i],old->pixelsize,to*linear_scale);
	    if ( linear_scale!=1 )
		BDFCAntiAlias(new->glyphs[i],linear_scale);
	} else {
	    new->glyphs[i] = BCScaleGrey(old->glyphs[i],old->pixelsize,old_depth,to,to_depth);
	}
    }
    if ( linear_scale!=1 )
	BDFClut(new,linear_scale);
return( new );
}
