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

#include <fontforge-config.h>

#include "colorP.h"
#include "gdrawP.h"
#include "ustring.h"

static void RevColListFree(struct revcol *rc) {
    struct revcol *next;

    while ( rc!=NULL ) {
	next = rc->next;
	free(rc);
	rc = next;
    }
}

static const GCol white = { 0xff, 0xff, 0xff, 1 }, black = { 0, 0, 0, 0 };

const GCol *_GImage_GetIndexedPixel(Color col,RevCMap *rev) {
    int val, t, best, r, g, b;
    struct revitem *this;
    struct revcol *test, *bestcol;

    if ( rev==NULL ) {		/* Mono */
	if ( COLOR2WHITE(col) )
return( &white );
	else
return( &black );
    }
    if ( rev->is_grey ) {
	val = COLOR2GREY(col);
return( &rev->greys[val]);
    }

  reverse_clut:
    r = COLOR_RED(col); g = COLOR_GREEN(col); b = COLOR_BLUE(col);
    if ( rev->div_mul==1 ) {
	val = r>>rev->div_shift;
	val = (val<<rev->side_shift)+(g>>rev->div_shift);
	val = (val<<rev->side_shift)+(b>>rev->div_shift);
    } else {
	val = ((r+rev->div_add)*rev->div_mul)>>rev->div_shift;
	t = ((g+rev->div_add)*rev->div_mul)>>rev->div_shift;
	val = (val*rev->side_cnt)+t;
	t = ((b+rev->div_add)*rev->div_mul)>>rev->div_shift;
	val = (val*rev->side_cnt)+t;
    }
    this = &rev->cube[val];
    if ( this->sub!=NULL ) {
	col &= rev->mask;
	rev = this->sub;
  goto reverse_clut;
    }

    test = this->cols[0];
    if ( test->next==NULL )
return( (GCol *) test );

    if (( best = (r-test->red))<0 ) best = -best;
    if (( t = (g-test->green))<0 ) t = -t; best += t;
    if (( t = (b-test->blue))<0 ) t = -t; best += t;
    bestcol = test;
    for ( test=test->next; test!=NULL; test = test->next ) {
	if (( val = (r-test->red))<0 ) val = -val;
	if (( t = (g-test->green))<0 ) t = -t; val += t;
	if (( t = (b-test->blue))<0 ) t = -t; val += t;
	if ( val<best ) {
	    val = best;
	    bestcol = test;
	}
    }
return( (GCol *) bestcol );
}

static int rccnt( struct revcol *cols) {
    int cnt = 0;
    while ( cols!=NULL ) {
	++cnt;
	cols = cols->next;
    }
return( cnt );
}

static struct revcol *add_adjacent(struct revcol *test, struct revcol *old, Color col, int dmax) {
    int r=COLOR_RED(col), g=COLOR_GREEN(col), b=COLOR_BLUE(col);
    int off, t, bestoff;
    struct revcol *best=NULL;

    if ( test==NULL || test->dist>dmax )
return( old );
    bestoff = 3*255;
    for ( test=test; test!=NULL; test = test->next ) {
	if ( (off = (r-test->red))<0 ) off = -off;
	if ( (t = (g-test->green))<0 ) t = -t; off +=t;
	if ( (b = (g-test->blue))<0 ) t = -t; off +=t;
	if ( off<bestoff ) {
	    bestoff = off;
	    best = test;
	}
    }
    if ( old!=NULL ) {
	if ( (off = (r-old->red))<0 ) off = -off;
	if ( (t = (g-old->green))<0 ) t = -t; off +=t;
	if ( (b = (g-old->blue))<0 ) t = -t; off +=t;
	if ( off<bestoff ) {
	    bestoff = off;
	    best = old;
	}
    }
    if ( best!=old ) {
	if ( old==NULL ) old = calloc(1,sizeof(struct revcol));
	*old = *best;
	old->next = NULL;
	++old->dist;
    }
return( old );
}

static struct revcol *addrevcol(struct revcol *copy,struct revcol *old, int dist) {
    struct revcol *rc = malloc(sizeof(struct revcol));

    *rc = *copy;
    rc->next = old;
    rc->dist = dist;
return( rc );
}

static RevCMap *_GClutReverse(int side_cnt,int range,struct revcol *basecol,
	struct revcol *cols,struct revcol *outercols) {
    RevCMap *rev;
    int i, s2, s3, side_size;
    struct revcol *test;
    int changed, dmax, anynulls;

    rev = calloc(1,sizeof(RevCMap));

    rev->side_cnt = side_cnt;
    rev->range = range;
    if ( side_cnt<0 ) {
	/* special cases ... */
	side_cnt = -side_cnt;
	rev->side_cnt = side_cnt;
	if ( side_cnt==6 ) {		/* Used for the inefficient standard 6 by cube */
	    rev->div_mul = div_tables[0x33][0];
	    rev->div_shift = div_tables[0x33][1];
	    rev->div_add = 0x19;
	}
	side_size = 0x33;
    } else if ( div_tables[side_cnt][0]==1 ) {
	rev->side_shift = div_tables[side_cnt][1];
	rev->div_shift = div_tables[range][1]-rev->side_shift;
	rev->div_mul = 1;
	rev->mask = ( 0x010101 * ((-1<rev->div_shift)&0xff) );
	side_size = 1<<(rev->div_shift);
    } else {
	side_size = (range+side_cnt-1)/side_cnt;
	rev->div_mul = div_tables[side_cnt][0];
	rev->div_shift = div_tables[side_cnt][1];
    }

    rev->cube = calloc(side_cnt*side_cnt*side_cnt,sizeof(struct revitem));
    for ( test = cols; test!=NULL; test = test->next ) {
	int pos, dist;
	int r,g,b;
	int rbase = (test->red-basecol->red)/side_size, gbase = (test->green-basecol->green)/side_size, bbase = (test->blue-basecol->blue)/side_size;
	for ( r = (test->red-basecol->red-side_size/2)/side_size;
		r<=(test->red-basecol->red+side_size/2)/side_size; ++r ) {
	    if ( r<0 || r==side_cnt )
	continue;
	    for ( g = (test->green-basecol->green-side_size/2)/side_size;
		    g<=(test->green-basecol->green+side_size/2)/side_size; ++g ) {
		if ( g<0 || g==side_cnt )
	    continue;
		for ( b = (test->blue-basecol->blue-side_size/2)/side_size;
			b<=(test->blue-basecol->blue+side_size/2)/side_size; ++b ) {
		    if ( b<0 || b==side_cnt )
		continue;
		    pos = (r*side_cnt + g)*side_cnt + b;
		    dist = r!=rbase || g!=gbase || b!=bbase;
		    rev->cube[pos].cols[dist] = addrevcol(test,rev->cube[pos].cols[dist],
			    dist );
		}
	    }
	}
    }

    /* Ok. We just put every color into the sub-cube that it belongs in */
    /*  but that's not good enough. Say we are looking for a color that */
    /*  is on the edge of a sub-cube, then the best match for it may be */
    /*  in one of the adjacent sub-cubes, rather than within it's own */
    /* So we also ran through the list of colors, casting our net a */
    /*  little wider. Essentially for every sub-cube we found all the colors */
    /*  within a subcube centered on the color */

    /* Some sub-cubes will probably have no information in them. Fill them */
    /*  in by looking at near-by cubes */
    s2 = side_cnt*side_cnt; s3 = s2*side_cnt;
    for ( i=0; i<s3; ++i ) {
	if ( rev->cube[i].cols[0]==NULL && rev->cube[i].cols[1]!=NULL ) {
	    rev->cube[i].cols[0] = rev->cube[i].cols[1];
	    rev->cube[i].cols[1] = NULL;
	}
    }
    changed = true; anynulls = false; dmax = 0;
    while ( changed || (anynulls && dmax<256) ) {
	changed = false; anynulls = false;
	for ( i=0; i<s3; ++i ) {
	    if ( rev->cube[i].cols[0]==NULL ) {
		int r = i/s2, g = (i/side_cnt)%side_cnt, b = i%side_cnt;
		int col = ((r*side_cnt+(side_cnt>>1))<<16) |
			  ((g*side_cnt+(side_cnt>>1))<<8) |
			  ((b*side_cnt+(side_cnt>>1))) ;
		if ( r>0 ) rev->cube[i].cols[0] = add_adjacent(rev->cube[i-s2].cols[0],NULL,col,dmax);
		if ( r+1<side_cnt ) rev->cube[i].cols[0] = add_adjacent(rev->cube[i+s2].cols[0],rev->cube[i].cols[0],col,dmax);
		if ( g>0 ) rev->cube[i].cols[0] = add_adjacent(rev->cube[i-side_cnt].cols[0],rev->cube[i].cols[0],col,dmax);
		if ( g+1<side_cnt ) rev->cube[i].cols[0] = add_adjacent(rev->cube[i+side_cnt].cols[0],rev->cube[i].cols[0],col,dmax);
		if ( b>0 ) rev->cube[i].cols[0] = add_adjacent(rev->cube[i-1].cols[0],rev->cube[i].cols[0],col,dmax);
		if ( b+1<side_cnt ) rev->cube[i].cols[0] = add_adjacent(rev->cube[i+1].cols[0],rev->cube[i].cols[0],col,dmax);
		if ( rev->cube[i].cols[0]!=NULL ) changed = true;
		else anynulls = true;
	    }
	}
	++dmax;
    }

    if ( anynulls ) {
	fprintf(stderr, "I'm sorry I cannot use this visual, please reconfigure your display\n" );
	exit(1);
    }

    if ( rev->side_shift!=0 ) {
	range >>= rev->side_shift;
	if ( range>8 ) for ( i=0; i<s3; ++i ) if ( rev->cube[i].cols[0]->dist==0 ) {
	    int ql = rccnt(rev->cube[i].cols[0]);
	    int nr= -1;
	    struct revcol nbase;
	    if ( ql>128 )
		nr = 16;
	    else if ( ql>32 )
		nr = 8;
	    else if ( ql>=8 )
		nr = 4;
	    if ( nr!=-1 ) {
		nbase.red = basecol->red+(i/s2)*range;
		nbase.green = basecol->green+(i/side_cnt)%side_cnt*range;
		nbase.blue = basecol->blue+(i%side_cnt)*range;
		while ( nr>range ) nr >>= 1;
		if ( nr!=1 )
		    rev->cube[i].sub = _GClutReverse(nr,range,&nbase,
			    rev->cube[i].cols[0],rev->cube[i].cols[1]);
	    }
	}
    }
return( rev );
}

RevCMap *GClutReverse(GClut *clut,int side_cnt) {
    struct revcol *base = NULL;
    int i;
    RevCMap *ret;
    struct revcol basecol;

    if ( GImageGreyClut(clut) ) {
	GCol *greys; int changed;
	ret = calloc(1,sizeof(RevCMap));
	ret->is_grey = 1;
	greys = ret->greys = malloc(256*sizeof(GCol));
	for ( i=0; i<256; ++i ) greys[i].pixel = 0x1000;
	for ( i=0; i<clut->clut_len; ++i ) {
	    int g = clut->clut[i]&0xff;
	    greys[g].red = greys[g].green = greys[g].blue = g;
	    greys[g].pixel = i;
	}
	changed = true;
	while ( changed ) {
	    changed = false;
	    for ( i=0; i<256; ++i ) {
		if ( greys[i].pixel!=0x1000 ) {
		    if ( i!=0 && greys[i-1].pixel == 0x1000 ) {
			greys[i-1] = greys[i]; changed = true; }
		    if ( i!=255 && greys[i+1].pixel == 0x1000 ) {
			greys[i+1] = greys[i]; changed = true; }
		}
	    }
	}
return( ret );
    }

    for ( i=0; i<clut->clut_len; ++i ) {
	struct revcol *rc = malloc(sizeof(struct revcol));
	rc->red = COLOR_RED(clut->clut[i]);
	rc->green = COLOR_GREEN(clut->clut[i]);
	rc->blue = COLOR_BLUE(clut->clut[i]);
	rc->index = i;
	rc->dist = 0;
	rc->next = base;
	base = rc;
    }
    memset(&basecol,'\0',sizeof(basecol));
    ret = _GClutReverse(side_cnt,256,&basecol,base,base);
    while ( base!=NULL ) {
	struct revcol *rc = base->next;
	free(base);
	base = rc;
    }
return( ret );
}

void GClut_RevCMapFree(RevCMap *rev) {
    int i;

    for ( i=0; i<rev->side_cnt*rev->side_cnt*rev->side_cnt; ++i ) {
	if ( rev->cube[i].sub!=NULL )
	    GClut_RevCMapFree(rev->cube[i].sub);
	RevColListFree(rev->cube[i].cols[0]);
	RevColListFree(rev->cube[i].cols[1]);
    }
    free(rev->cube);
    free(rev);
}

int GImageGreyClut(GClut *clut) {
    int i, r,b,g;

    if ( clut==NULL )
return( true );
    for ( i=0; i<clut->clut_len; ++i ) {
	r = COLOR_RED(clut->clut[i]);
	g = COLOR_GREEN(clut->clut[i]);
	b = COLOR_BLUE(clut->clut[i]);
	if ( r!=g || g!=b ) {
	    clut->is_grey = false;
return( false );
	}
    }
    clut->is_grey = true;
return( true );
}

static struct { char *name; long value; } predefn[] = {
    { "red", 0xff0000 },
    { "green", 0x008000 },
    { "blue", 0x0000ff },
    { "cyan", 0x00ffff },
    { "magenta", 0xff00ff },
    { "yellow", 0xffff00 },
    { "black", 0x000000 },
    { "gray", 0x808080 },
    { "grey", 0x808080 },
    { "white", 0xffffff },
    { "maroon", 0x800000 },
    { "olive", 0x808000 },
    { "navy", 0x000080 },
    { "purple", 0x800080 },
    { "lime", 0x00ff00 },
    { "aqua", 0x00ffff },
    { "teal", 0x008080 },
    { "fuchsia", 0xff0080 },
    { "silver", 0xcccccc },
    { NULL, 0 }
};

Color _GImage_ColourFName(char *name) {
    int i;
    int r,g,b,a;
    double dr,dg,db,da;
    struct hslrgb hs;
    Color col;

    for ( i=0; predefn[i].name!=NULL; ++i )
	if ( strmatch(name,predefn[i].name)==0 )
return( predefn[i].value );

    if ( sscanf(name,"%d %d %d", &r, &g, &b )==3 ||
	    sscanf(name,"%x %x %x", (unsigned *) &r, (unsigned *) &g, (unsigned *) &b )==3 ||
	    (strlen(name)==7 && sscanf(name,"#%2x%2x%2x", (unsigned *) &r, (unsigned *) &g, (unsigned *) &b )==3) ) {
	if ( r>255 ) r=255; else if ( r<0 ) r=0;
	if ( g>255 ) g=255; else if ( g<0 ) g=0;
	if ( b>255 ) b=255; else if ( b<0 ) b=0;
return( ((long) r<<16) | (g<<8) | b );
    } else if ( (strlen(name)==9 && sscanf(name,"#%2x%2x%2x%2x",
	    (unsigned *) &a, (unsigned *) &r, (unsigned *) &g, (unsigned *) &b )==4) ) {
	if ( a>255 ) a=255; else if ( a<0 ) a=0;
	if ( r>255 ) r=255; else if ( r<0 ) r=0;
	if ( g>255 ) g=255; else if ( g<0 ) g=0;
	if ( b>255 ) b=255; else if ( b<0 ) b=0;
	col = ((long) a<<24) | ((long) r<<16) | (g<<8) | b;
	if ( (col&0xfffffff0)==0xfffffff0 )
	    col &= 0xffffff;	/* I use these colors internally for things like "Undefined" */
			    /* but since I also treat colors with 0 alpha channel as fully */
			    /* opaque, I can just represent this that way */
return( col );
    } else if ( sscanf(name,"rgb(%lg,%lg,%lg)", &dr, &dg, &db)==3 ) {
	if ( dr>1.0 ) dr=1.0; else if ( dr<0 ) dr= 0;
	if ( dg>1.0 ) dg=1.0; else if ( dg<0 ) dg= 0;
	if ( db>1.0 ) db=1.0; else if ( db<0 ) db= 0;
	r = dr*255 +.5;
	g = dg*255 +.5;
	b = db*255 +.5;
return( ((long) r<<16) | (g<<8) | b );
    } else if ( sscanf(name,"argb(%lg,%lg,%lg,%lg)", &da, &dr, &dg, &db)==4 ) {
	if ( da>1.0 ) da=1.0; else if ( da<0 ) da= 0;
	if ( dr>1.0 ) dr=1.0; else if ( dr<0 ) dr= 0;
	if ( dg>1.0 ) dg=1.0; else if ( dg<0 ) dg= 0;
	if ( db>1.0 ) db=1.0; else if ( db<0 ) db= 0;
	a = da*255 +.5;
	r = dr*255 +.5;
	g = dg*255 +.5;
	b = db*255 +.5;
	col = ((long) a<<24) | ((long) r<<16) | (g<<8) | b;
	if ( (col&0xfffffff0)==0xfffffff0 )
	    col &= 0xffffff;	/* I use these colors internally for things like "Undefined" */
			    /* but since I also treat colors with 0 alpha channel as fully */
			    /* opaque, I can just represent this that way */
return( col );
    } else if ( sscanf(name,"hsv(%lg,%lg,%lg)", &hs.h, &hs.s, &hs.v)==3 ) {
	/* Hue is an angle in degrees. HS?2RGB does appropriate clipping */
	if ( hs.s>1.0 ) hs.s=1.0; else if ( hs.s<0 ) hs.s= 0;
	if ( hs.v>1.0 ) hs.v=1.0; else if ( hs.v<0 ) hs.v= 0;
	gHSV2RGB(&hs);
	r = hs.r*255 +.5;
	g = hs.g*255 +.5;
	b = hs.b*255 +.5;
return( ((long) r<<16) | (g<<8) | b );
    } else if ( sscanf(name,"hsl(%lg,%lg,%lg)", &hs.h, &hs.s, &hs.l)==3 ) {
	/* Hue is an angle in degrees. HS?2RGB does appropriate clipping */
	if ( hs.s>1.0 ) hs.s=1.0; else if ( hs.s<0 ) hs.s= 0;
	if ( hs.l>1.0 ) hs.l=1.0; else if ( hs.l<0 ) hs.l= 0;
	gHSL2RGB(&hs);
	r = hs.r*255 +.5;
	g = hs.g*255 +.5;
	b = hs.b*255 +.5;
return( ((long) r<<16) | (g<<8) | b );
    } else if ( (strlen(name)==4 && sscanf(name,"#%1x%1x%1x", (unsigned *) &r, (unsigned *) &g, (unsigned *) &b )==3) ) {
	if ( r>15 ) r=15; else if ( r<0 ) r=0;
	if ( g>15 ) g=15; else if ( g<0 ) g=0;
	if ( b>15 ) b=15; else if ( b<0 ) b=0;
return( ((long) (r*0x110000)) | (g*0x1100) | (b*0x11) );
    } else if ( (strlen(name)==17 && sscanf(name,"#%4x%4x%4x", (unsigned *) &r, (unsigned *) &g, (unsigned *) &b )==3) ) {
	r>>=8; g>>=8; b>>=8;
	if ( r>255 ) r=255; else if ( r<0 ) r=0;
	if ( g>255 ) g=255; else if ( g<0 ) g=0;
	if ( b>255 ) b=255; else if ( b<0 ) b=0;
return( ((long) r<<16) | (g<<8) | b );
    } else if ( sscanf(name,"rgb(%lg%%,%lg%%,%lg%%)", &dr, &dg, &db)==3 ) {
	if ( dr>100 ) dr=100; else if ( dr<0 ) dr= 0;
	if ( dg>100 ) dg=100; else if ( dg<0 ) dg= 0;
	if ( db>100 ) db=100; else if ( db<0 ) db= 0;
	r = (dr*255+50)/100 +.5;
	g = (dg*255+50)/100 +.5;
	b = (db*255+50)/100 +.5;
return( ((long) r<<16) | (g<<8) | b );
    }
return( -1 );
}

Color GDrawColorBrighten(Color col, int by) {
    int r, g, b;
    if (( r = COLOR_RED(col)+by )>255 ) r=255;
    if (( g=COLOR_GREEN(col)+by )>255 ) g=255;
    if (( b=COLOR_BLUE(col)+by )>255 ) b=255;
return( COLOR_CREATE(r,g,b));
}

Color GDrawColorDarken(Color col, int by) {
    int r, g, b;
    if (( r = COLOR_RED(col)-by )<0 ) r=0;
    if (( g=COLOR_GREEN(col)-by )<0 ) g=0;
    if (( b=COLOR_BLUE(col)-by )<0 ) b=0;
return( COLOR_CREATE(r,g,b));
}
