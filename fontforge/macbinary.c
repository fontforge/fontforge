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

#include "macbinary.h"

#include "bvedit.h"
#include "crctab.h"
#include "dumppfa.h"
#include "encoding.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "http.h"
#include "lookups.h"
#include "mem.h"
#include "parsepfa.h"
#include "parsettf.h"
#include "splinefill.h"
#include "splinesave.h"
#include "splinesaveafm.h"
#include "splineutil2.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ustring.h>
#include "ttf.h"
#include "psfont.h"
#if __Mac
# include <ctype.h>
# include "carbon.h"
#else
# include <utype.h>
#undef __Mac
#define __Mac 0
#endif

const int mac_dpi = 72;
/* I had always assumed that the mac still believed in 72dpi screens, but I */
/*  see that in geneva under OS/9, the pointsize does not match the pixel */
/*  size of the font. But the dpi is not constant (and the differences */
/*  excede those supplied by rounding errors) varying between 96 and 84dpi */

/* A Mac Resource fork */
/*  http://developer.apple.com/techpubs/mac/MoreToolbox/MoreToolbox-9.html */
/*    begins with a 16 byte header containing: */
/*	resource start offset */
/*	map start offset */
/*	resource length */
/*	map length */
/*    then 256-16 bytes of zeros */
/*    the resource section consists of (many) */
/*	4 byte length count */
/*	resource data	*/
/*    the map section contains */
/*	A copy of the 16 byte header */
/*	a 4 byte mac internal value (I hope) */
/*	another 4 bytes of mac internal values (I hope) */
/*	a 2 byte offset from the start of the map section to the list of resource types */
/*	a 2 byte offset from the start of the map section to the list of resource names */
/*	The resource type list consists of */
/*	    a 2 byte count of the number of resource types (-1) */
/*	    (many copies of) */
/*		a 4 byte resource type ('FOND' for example) */
/*		a 2 byte count of the number of resources of this type (-1) */
/*		a 2 byte offset from the type list start to the resource table */
/*	    a resource table looks like */
/*		a 2 byte offset from the resource name table to a pascal */
/*			string containing this resource's name (or 0xffff for none) */
/*		1 byte of resource flags */
/*		3 bytes of offset from the resource section to the length & */
/*			data of this instance of the resource type */
/*		4 bytes of 0 */
/*	The resource name section consists of */
/*	    a bunch of pascal strings (ie. preceded by a length byte) */

/* The POST resource isn't noticeably documented, it's pretty much a */
/*  straight copy of the pfb file cut up into 0x800 byte chunks. */
/*  (each section of the pfb file has it's own set of chunks, the last may be smaller than 0x800) */
/* The NFNT resource http://developer.apple.com/techpubs/mac/Text/Text-250.html */
/* The FOND resource http://developer.apple.com/techpubs/mac/Text/Text-269.html */
/* The sfnt resource is basically a copy of the ttf file */

/* A MacBinary file */
/*  http://www.lazerware.com/formats/macbinary.html */
/*    begins with a 128 byte header */
/*	(which specifies lengths for data/resource forks) */
/*	(and contains mac type/creator data) */
/*	(and other stuff) */
/*	(and finally a crc checksum) */
/*    is followed by the data section (padded to a mult of 128 bytes) */
/*    is followed by the resource section (padded to a mult of 128 bytes) */

/* Crc code taken from: */
/* http://www.ctan.org/tex-archive/tools/macutils/crc/ */
/* MacBinary files use the same CRC that binhex does (in the MacBinary header) */

/* ******************************** Creation ******************************** */

static uint16 HashToId(char *fontname,SplineFont *sf,EncMap *map) {
    int low = 128, high = 0x3fff;
    /* A FOND ID should be between these two numbers for roman script (I think) */
    uint32 hash = 0;
    int i, gid;
    SplineChar *sc;

    /* Figure out what language we've got */
    /*  I'm not bothering with all of the apple scripts, and I don't know */
    /*  what to do about cjk fonts */
    if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 ) {
	if ( sf->cidmaster != NULL ) sf = sf->cidmaster;
	if ( sf->ordering != NULL ) {
	    if ( strstrmatch(sf->ordering,"Japan")!=NULL ) {
		low = 0x4000; high = 0x41ff;
	    } else if ( strstrmatch(sf->ordering,"Korea")!=NULL ) {
		low = 0x4400; high = 0x45ff;
	    } else if ( strstrmatch(sf->ordering,"CNS")!=NULL ) {
		low = 0x4200; high = 0x43ff;
	    } else if ( strstrmatch(sf->ordering,"GB")!=NULL ) {
		low = 0x7200; high = 0x73ff;
	    }
	}
    } else if ( map->enc->is_tradchinese ) {
	low = 0x4200; high = 0x43ff;
    } else if ( map->enc->is_japanese ) {
	low = 0x4000; high = 0x41ff;
    } else if ( map->enc->is_korean ) {
	low = 0x4400; high = 0x45ff;
    } else if ( map->enc->is_simplechinese ) {
	low = 0x7200; high = 0x73ff;
    } else for ( i=0; i<map->enccount && i<256; ++i ) if ( (gid=map->map[i])!=-1 && (sc = sf->glyphs[gid])!=NULL ) {
	/* Japanese between	0x4000 and 0x41ff */
	/* Trad Chinese		0x4200 and 0x43ff */
	/*  Simp Chinese	0x7200 and 0x73ff */
	/* Korean		0x4400 and 0x45ff */
	if (( sc->unicodeenc>=0x0600 && sc->unicodeenc<0x0700 ) ||
		( sc->unicodeenc>=0xFB50 && sc->unicodeenc<0xfe00 )) {
	    /* arabic */
	    low = 0x4600; high = 0x47ff;
    break;
	} else if (( sc->unicodeenc>=0x0590 && sc->unicodeenc<0x0600 ) ||
		( sc->unicodeenc>=0xFB1d && sc->unicodeenc<0xFB50 )) {
	    /* hebrew */
	    low = 0x4800; high = 0x49ff;
    break;
	} else if ((( sc->unicodeenc>=0x0370 && sc->unicodeenc<0x0400 ) ||
		( sc->unicodeenc>=0x1f00 && sc->unicodeenc<0x2000 )) &&
		sc->unicodeenc!=0x3c0 ) {	/* In mac encoding */
	    /* greek */
	    low = 0x4a00; high = 0x4bff;
    break;
	} else if ( sc->unicodeenc>=0x0400 && sc->unicodeenc<0x0530 ) {
	    /* cyrillic */
	    low = 0x4c00; high = 0x4dff;
    break;
	/* hebrew/arabic symbols 4e00-4fff */
	} else if ( sc->unicodeenc>=0x0900 && sc->unicodeenc<0x0980 ) {
	    /* devanagari */
	    low = 0x5000; high = 0x51ff;
    break;
	} else if ( sc->unicodeenc>=0x0980 && sc->unicodeenc<0x0a00 ) {
	    /* bengali: script=13 */
	    low = 0x5800; high = 0x59ff;
    break;
	}
    }
    while ( *fontname ) {
	int temp = (hash>>28)&0xf;
	hash = (hash<<4) | temp;
	hash ^= *fontname++-' ';
    }
    hash %= (high-low+1);
    hash += low;
return( hash );
}

static int IsMacMonospaced(SplineFont *sf,EncMap *map) {
    /* Only look at first 256 */
    int i;
    double width = 0x80000000;

    for ( i=0; i<256 && i<map->enccount; ++i ) {
	int gid = map->map[i];
	if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
	    if ( width == 0x80000000 )
		width = sf->glyphs[gid]->width;
	    else if ( sf->glyphs[gid]->width!=width )
return( false );
	}
    }
return( true );
}

SplineChar *SFFindExistingCharMac(SplineFont *sf,EncMap *map,int unienc) {
    int i, gid;

    for ( i=0; i<map->enccount && i<256; ++i )
	if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && sf->glyphs[gid]->unicodeenc==unienc )
return( sf->glyphs[gid] );
return( NULL );
}

static double SFMacWidthMax(SplineFont *sf, EncMap *map) {
    /* Only look at first 256 */
    int i, gid;
    double width = -1;

    for ( i=0; i<256 && i<map->enccount; ++i ) {
	if ( (gid=map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
	    if ( sf->glyphs[gid]->width>width )
		width = sf->glyphs[gid]->width;
	}
    }
    if ( width<0 )	/* No chars, or widths the mac doesn't support */
return( 0 );

return( width );
}

static int SFMacAnyKerns(SplineFont *sf, EncMap *map) {
    /* Only look at first 256 */
    int i, cnt=0, gid;
    KernPair *kp;

    for ( i=0; i<256 && i<map->enccount; ++i ) {
	if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	    for ( kp=sf->glyphs[gid]->kerns; kp!=NULL; kp=kp->next )
		if ( map->backmap[kp->sc->orig_pos]<256 )
		    ++cnt;
	}
    }
return( cnt );
}

struct resource {
    uint32 pos;
    uint8 flags;
    uint16 id;
    char *name;
    uint32 nameloc;
    uint32 nameptloc;
};

struct resourcetype {
    uint32 tag;
    struct resource *res;
    uint32 resloc;
};

struct macbinaryheader {
    char *macfilename;
    char *binfilename;		/* if macfilename is null and this is set we will figure out macfilename by removing .bin */
    uint32 type;
    uint32 creator;
};

static struct resource *PSToResources(FILE *res,FILE *pfbfile) {
    /* split the font up into as many small resources as we need and return */
    /*  an array pointing to the start of each */
    struct stat statb;
    int cnt, type;
    struct resource *resstarts;
    int len,i;

    fstat(fileno(pfbfile),&statb);
    cnt = 3*(statb.st_size+0x800)/(0x800-2)+1;		/* should be (usually) a vast over estimate */
    resstarts = calloc(cnt+1,sizeof(struct resource));

    cnt = 0;
    for (;;) {
	if ( getc(pfbfile)!=0x80 ) {
	    IError("We made a pfb file, but didn't get one. Hunh?" );
return( NULL );
	}
	type = getc(pfbfile);
	if ( type==3 ) {
	/* 501 appears to be magic */
	/* postscript resources seem to have flags of 0 */
	    resstarts[cnt].id = 501+cnt;
	    resstarts[cnt++].pos = ftell(res);
	    putlong(res,2);	/* length */
	    putc(5,res);	/* eof mark */
	    putc(0,res);
    break;
	}
	len = getc(pfbfile);
	len |= (getc(pfbfile))<<8;
	len |= (getc(pfbfile))<<16;
	len |= (getc(pfbfile))<<24;
	while ( len>0 ) {
	    int ilen = len;
	    if ( ilen>0x800-2 )
		ilen = 0x800-2;
	    len -= ilen;
	    resstarts[cnt].id = 501+cnt;
	    resstarts[cnt++].pos = ftell(res);
	    putlong(res,ilen+2);	/* length */
	    putc(type,res);		/* section type mark */
	    putc(0,res);
	    for ( i=0; i<ilen; ++i )
		putc(getc(pfbfile),res);
	}
    }
    resstarts[cnt].pos = 0;
return( resstarts );
}

static uint32 TTFToResource(FILE *res,FILE *ttffile) {
    /* A truetype font just gets dropped into a resource */
    struct stat statb;
    int ch;
    uint32 resstart;

    fstat(fileno(ttffile),&statb);
    resstart = ftell(res);

    putlong(res,statb.st_size);
    while ( (ch=getc(ttffile))!=EOF )
	putc(ch,res);
return( resstart );
}

static int BDFCCopyBitmaps(uint8 **rows,int offset,BDFChar *bdfc, BDFFont *bdf) {
    int i, r, y, ipos, j, c;

    y = bdf->ascent-1; r = i = 0;
    if ( bdfc->ymax > bdf->ascent-1 )
	i = bdfc->ymax-(bdf->ascent-1);
    else if ( bdfc->ymax<bdf->ascent-1 ) {
	r = bdf->ascent-1-bdfc->ymax;
	y = bdfc->ymax;
    }
    for ( ; y>=bdfc->ymin && y>=-bdf->descent; --y, ++i ) {
	/* Mac characters may not extend above the ascent or below the descent */
	/*  but bdf chars can, so if a bdf char does, just ignore that part */
	ipos = i*bdfc->bytes_per_line;
	for ( j=0,c=offset; j<=bdfc->xmax-bdfc->xmin; ++j, ++c ) {
	    if ( bdfc->bitmap[ipos+(j>>3)] & (1<<(7-(j&7))) )
		rows[r][c>>3] |= (1<<(7-(c&7)));
	}
	++r;
    }
return( offset + bdfc->xmax-bdfc->xmin+1 );
}

static uint32 BDFToNFNT(FILE *res, BDFFont *bdf, EncMap *map) {
    short widths[258], lbearings[258], locs[258]/*, idealwidths[256]*/;
    uint8 **rows = malloc(bdf->pixelsize*sizeof(uint8 *));
    int i, k, width, kernMax=1, descentMax=bdf->descent-1, rectMax=1, widMax=3;
    uint32 rlenpos = ftell(res), end, owloc, owpos;
    int gid;
    BDFChar *bdfc;

    for ( i=0; i<map->enccount; i++ ) if (( gid=map->map[i])!=-1 && ( bdfc = bdf->glyphs[gid] ) != NULL )
	BCPrepareForOutput( bdfc,true );

    for ( i=width=0; i<256 && i<map->enccount; ++i ) {
	if ( (gid = map->map[i])!=-1 && gid<bdf->glyphcnt && bdf->glyphs[gid]!=NULL ) {
	    width += bdf->glyphs[gid]->xmax+1-bdf->glyphs[gid]->xmin;
	    if ( bdf->glyphs[gid]->width>widMax )
		widMax = bdf->glyphs[gid]->width;
	    if ( bdf->glyphs[gid]->xmax+1-bdf->glyphs[gid]->xmin>rectMax )
		rectMax = bdf->glyphs[gid]->xmax+1-bdf->glyphs[gid]->xmin;
	    if ( bdf->glyphs[gid]->xmin<kernMax )
		kernMax = bdf->glyphs[gid]->xmin;
	    if ( bdf->glyphs[gid]->ymin<-descentMax )
		descentMax = -bdf->glyphs[gid]->ymin;
	}
    }
    if ( descentMax>bdf->descent ) descentMax = bdf->descent;
    ++width;			/* For the "undefined character */
    for ( k=0; k<bdf->pixelsize; ++k )
	rows[k] = calloc((width+7)/8 + 4 , sizeof(uint8));
    for ( i=width=0; i<256 ; ++i ) {
	locs[i] = width;
	if ( i>=map->enccount || (gid=map->map[i])==-1 || gid>=bdf->glyphcnt || bdf->glyphs[gid]==NULL ||
		!SCWorthOutputting(bdf->glyphs[gid]->sc) ) {
	    lbearings[i] = 0xff;
	    widths[i] = 0xff;
	    /*idealwidths[i] = 1<<12; */		/* 1 em */
	} else {
	    lbearings[i] = bdf->glyphs[gid]->xmin-kernMax;
	    widths[i] = bdf->glyphs[gid]->width<0?0:
			bdf->glyphs[gid]->width>=256?255:
			bdf->glyphs[gid]->width;
	    /*idealwidths[i] = bdf->glyphs[gid]->sc->width*(1<<12)/(bdf->sf->ascent+bdf->sf->descent);*/
	    width = BDFCCopyBitmaps(rows,width,bdf->glyphs[gid],bdf);
	}
    }
    /* Now for the "undefined character", just a simple vertical bar */
    locs[i] = width;
    lbearings[i] = 1;
    widths[i++] = 3;
    /*idealwidths[i++] = (3<<12)/bdf->pixelsize;*/
    for ( k = 1; k<bdf->pixelsize-1; ++k )
	rows[k][width>>3] |= (1<<(7-(width&7)));
    /* And one more entry to give a size to the last character */
    locs[i] = ++width;
    lbearings[i] = widths[i] = 0xff;
    /*idealwidths[i] = 0;*/

    /* Mac magic */
    lbearings[0] = widths[0] = 0;
    lbearings['\r'] = widths['\r'] = 0;
    lbearings['\t'] = 0; widths['\t'] = 6;

    for ( i=0; i<map->enccount; i++ ) if (( gid=map->map[i])!=-1 && ( bdfc = bdf->glyphs[gid] ) != NULL )
	BCRestoreAfterOutput( bdfc );

    /* We've finished the bitmap conversion, now save it... */
    putlong(res,0);		/* Length, to be filled in later */
    putshort(res,IsMacMonospaced(bdf->sf,map)?0xb000:0x9000);	/* fontType */
    putshort(res,0);
    putshort(res,255);
    putshort(res,widMax);
    putshort(res,kernMax);
    putshort(res,-descentMax);
    putshort(res,rectMax);
    putshort(res,bdf->pixelsize);
    owpos = ftell(res);
    putshort(res,0);
    putshort(res,bdf->ascent);
    putshort(res,bdf->descent);
    putshort(res,(short) (bdf->sf->pfminfo.linegap*bdf->pixelsize/(bdf->sf->ascent+bdf->sf->descent)) );
    putshort(res,(width+15)>>4);
    /* bitmaps */
    for ( k=0; k<bdf->pixelsize; ++k ) {
	for ( i=0; i<((width+15)>>4) ; ++i ) {
	    putc(rows[k][2*i],res);
	    putc(rows[k][2*i+1],res);
	}
    }
    for ( i=0; i<258; ++i )
	putshort(res,locs[i]);
    owloc = ftell(res);			/* valgrind reports an error here (but not above). god knows why */
    for ( i=0; i<258; ++i ) {
	putc(lbearings[i],res);
	putc(widths[i],res);
    }
    end = ftell(res);
    fseek(res,rlenpos,SEEK_SET);
    putlong(res,end-rlenpos-4);
    fseek(res,owpos,SEEK_SET);
    putshort(res,(owloc-owpos)/2);
    fseek(res,0,SEEK_END);

    for ( k=0; k<bdf->pixelsize; ++k )
	free(rows[k]);
    free(rows);

return(rlenpos);
}

static uint32 DummyNFNT(FILE *res, BDFFont *bdf, EncMap *map) {
    /* This produces a stub NFNT which appears when the real data lives inside */
    /*  an sfnt (truetype) resource. We still need to produce an NFNT to tell */
    /*  the system that the pointsize is available. This NFNT has almost nothing */
    /*  in it, just the initial header, no metrics, no bitmaps */
    int i, width, kernMax=1, descentMax=bdf->descent-1, rectMax=1, widMax=3;
    uint32 rlenpos = ftell(res);
    int gid;

    for ( i=width=0; i<256 && i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && gid<bdf->glyphcnt && bdf->glyphs[gid]!=NULL ) {
	width += bdf->glyphs[gid]->xmax+1-bdf->glyphs[gid]->xmin;
	if ( bdf->glyphs[gid]->width>widMax )
	    widMax = bdf->glyphs[gid]->width;
	if ( bdf->glyphs[gid]->xmax+1-bdf->glyphs[gid]->xmin>rectMax )
	    rectMax = bdf->glyphs[gid]->xmax+1-bdf->glyphs[gid]->xmin;
	if ( bdf->glyphs[gid]->xmin<kernMax )
	    kernMax = bdf->glyphs[gid]->xmin;
	if ( bdf->glyphs[gid]->ymin<-descentMax )
	    descentMax = -bdf->glyphs[gid]->ymin;
    }
    if ( descentMax>bdf->descent ) descentMax = bdf->descent;

    putlong(res,26);		/* Length */
    putshort(res,SFOneWidth(bdf->sf)!=-1?0xf000:0xd000);	/* fontType */
    putshort(res,0);
    putshort(res,255);
    putshort(res,widMax);
    putshort(res,kernMax);
    putshort(res,-descentMax);
    putshort(res,rectMax);
    putshort(res,bdf->pixelsize);
    putshort(res,0);
    putshort(res,bdf->ascent);
    putshort(res,bdf->descent);
    putshort(res,(short) (bdf->sf->pfminfo.linegap*bdf->pixelsize/(bdf->sf->ascent+bdf->sf->descent)) );
    putshort(res,0);

return(rlenpos);
}

static struct resource *SFToNFNTs(FILE *res, SplineFont *sf, int32 *sizes,
	EncMap *map) {
    int i, baseresid = HashToId(sf->fontname,sf,map);
    struct resource *resstarts;
    BDFFont *bdf;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( i=0; sizes[i]!=0; ++i );
    resstarts = calloc(i+1,sizeof(struct resource));

    for ( i=0; sizes[i]!=0; ++i ) {
	if ( (sizes[i]>>16)!=1 )
    continue;
	if ( (sizes[i]&0xffff)>=256 )
    continue;
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=1); bdf=bdf->next );
	if ( bdf==NULL )
    continue;
	resstarts[i].id = baseresid+bdf->pixelsize;
	resstarts[i].pos = BDFToNFNT(res,bdf,map);
	/* NFNTs seem to have resource flags of 0 */
    }
return(resstarts);
}

static struct resource *SFsToNFNTs(FILE *res, struct sflist *sfs,int baseresid) {
    int i, j, cnt;
    struct resource *resstarts;
    BDFFont *bdf;
    struct sflist *sfi;
    SplineFont *sf;

    cnt = 0;
    for ( sfi=sfs; sfi!=NULL; sfi=sfi->next ) {
	if ( sfi->sizes!=NULL ) {
	    for ( i=0; sfi->sizes[i]!=0; ++i );
	    cnt += i;
	    sfi->ids = calloc(i+1,sizeof(int));
	    sfi->bdfs = calloc(i+1,sizeof(BDFFont *));
	}
    }

    resstarts = calloc(cnt+1,sizeof(struct resource));

    cnt = 0;
    for ( sfi=sfs; sfi!=NULL; sfi=sfi->next ) {
	sf = sfi->sf;
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	j=0;
	if ( sfi->sizes ) for ( i=0; sfi->sizes[i]!=0; ++i ) {
	    if ( (sfi->sizes[i]>>16)!=1 )
	continue;
	    if ( (sfi->sizes[i]&0xffff)>=256 )
	continue;
	    for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sfi->sizes[i]&0xffff) || BDFDepth(bdf)!=1); bdf=bdf->next );
	    if ( bdf==NULL )
	continue;
	    sfi->ids[j] = baseresid;
	    sfi->bdfs[j] = bdf;
	    resstarts[cnt+j].id = baseresid++;
	    resstarts[cnt+j++].pos = BDFToNFNT(res,bdf,sfi->map);
	    /* NFNTs seem to have resource flags of 0 */
	}
	cnt += j;
    }
return(resstarts);
}

static struct resource *BuildDummyNFNTlist(FILE *res, SplineFont *sf,
	int32 *sizes, int baseresid, EncMap *map) {
    int i;
    struct resource *resstarts;
    BDFFont *bdf;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( i=0; sizes[i]!=0; ++i );
    resstarts = calloc(i+1,sizeof(struct resource));

    for ( i=0; sizes[i]!=0; ++i ) {
	if ( (sizes[i]>>16)!=1 )
    continue;
	if ( (sizes[i]&0xffff)>=256 )
    continue;
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=1); bdf=bdf->next );
	if ( bdf==NULL )
    continue;
	resstarts[i].id = baseresid+(sizes[i]&0xffff);
	resstarts[i].pos = DummyNFNT(res,bdf,map);
	/* NFNTs seem to have resource flags of 0 */
    }
return(resstarts);
}

static struct resource *BuildDummyNFNTfamilyList(FILE *res, struct sflist *sfs,
	int baseresid) {
    int i,j,cnt;
    struct resource *resstarts;
    BDFFont *bdf;
    struct sflist *sfi;
    SplineFont *sf;

    cnt = 0;
    for ( sfi=sfs; sfi!=NULL; sfi=sfi->next ) {
	if ( sfi->sizes!=NULL ) {
	    for ( i=0; sfi->sizes[i]!=0; ++i );
	    cnt += i;
	    sfi->ids = calloc(i+1,sizeof(int));
	    sfi->bdfs = calloc(i+1,sizeof(BDFFont *));
	}
    }

    resstarts = calloc(cnt+1,sizeof(struct resource));

    cnt = 0;
    for ( sfi=sfs; sfi!=NULL; sfi=sfi->next ) {
	sf = sfi->sf;
	j = 0;
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	if ( sfi->sizes ) for ( i=0; sfi->sizes[i]!=0; ++i ) {
	    if ( (sfi->sizes[i]>>16)!=1 )
	continue;
	    for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sfi->sizes[i]&0xffff) || BDFDepth(bdf)!=1); bdf=bdf->next );
	    if ( bdf==NULL )
	continue;
	    sfi->ids[j] = baseresid;
	    sfi->bdfs[j] = bdf;
	    resstarts[cnt+j].id = baseresid++;
	    resstarts[cnt+j++].pos = DummyNFNT(res,bdf,sfi->map);
	    /* NFNTs seem to have resource flags of 0 */
	}
	cnt += j;
    }
return(resstarts);
}

enum psstyle_flags { psf_bold = 1, psf_italic = 2, psf_outline = 4,
	psf_shadow = 0x8, psf_condense = 0x10, psf_extend = 0x20 };

uint16 _MacStyleCode( const char *styles, SplineFont *sf, uint16 *psstylecode ) {
    unsigned short stylecode= 0, psstyle=0;

    if ( strstrmatch( styles, "Bold" ) || strstrmatch(styles,"Demi") ||
	    strstrmatch( styles,"Heav") || strstrmatch(styles,"Blac") ||
/* A few fonts have German/French styles in their names */
	    strstrmatch( styles,"Fett") || strstrmatch(styles,"Gras") ) {
	stylecode = sf_bold;
	psstyle = psf_bold;
    } else if ( sf!=NULL && sf->weight!=NULL &&
	    (strstrmatch( sf->weight, "Bold" ) || strstrmatch(sf->weight,"Demi") ||
	     strstrmatch( sf->weight,"Heav") || strstrmatch(sf->weight,"Blac") ||
	     strstrmatch( sf->weight,"Fett") || strstrmatch(sf->weight,"Gras")) ) {
	stylecode = sf_bold;
	psstyle = psf_bold;
    }
    /* URW uses four leter abbreviations of Italic and Oblique */
    /* Somebody else uses two letter abbrevs */
    if ( (sf!=NULL && sf->italicangle!=0) ||
	    strstrmatch( styles, "Ital" ) ||
	    strstrmatch( styles, "Obli" ) ||
	    strstrmatch(styles, "Slanted") ||
	    strstrmatch(styles, "Kurs") ||
	    strstr( styles,"It" ) ) {
	stylecode |= sf_italic;
	psstyle |= psf_italic;
    }
    if ( strstrmatch( styles, "Underline" ) ) {
	stylecode |= sf_underline;
    }
    if ( strstrmatch( styles, "Outl" ) ) {
	stylecode |= sf_outline;
	psstyle |= psf_outline;
    }
    if ( strstr(styles,"Shadow")!=NULL ) {
	stylecode |= sf_shadow;
	psstyle |= psf_shadow;
    }
    if ( strstrmatch( styles, "Cond" ) || strstr( styles,"Cn") ||
	    strstrmatch( styles, "Narrow") ) {
	stylecode |= sf_condense;
	psstyle |= psf_condense;
    }
    if ( strstrmatch( styles, "Exte" ) || strstr( styles,"Ex") ) {
	stylecode |= sf_extend;
	psstyle |= psf_extend;
    }
    if ( (psstyle&psf_extend) && (psstyle&psf_condense) ) {
	if ( sf!=NULL )
	    LogError( _("Warning: %s(%s) is both extended and condensed. That's impossible.\n"),
		    sf->fontname, sf->origname );
	else
	    LogError( _("Warning: Both extended and condensed. That's impossible.\n") );
	psstyle &= ~psf_extend;
	stylecode &= ~sf_extend;
    }
    if ( psstylecode!=NULL )
	*psstylecode = psstyle;
return( stylecode );
}

uint16 MacStyleCode( SplineFont *sf, uint16 *psstylecode ) {
    const char *styles;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;

    if ( sf->macstyle!=-1 ) {
	if ( psstylecode!=NULL )
	    *psstylecode = (sf->macstyle&0x3)|((sf->macstyle&0x6c)>>1);
return( sf->macstyle );
    }

    styles = SFGetModifiers(sf);
return( _MacStyleCode(styles,sf,psstylecode));
}

static uint32 SFToFOND(FILE *res,SplineFont *sf,uint32 id,int dottf,
	int32 *sizes, EncMap *map) {
    uint32 rlenpos = ftell(res), widoffpos, widoffloc, kernloc, styleloc, end;
    int i,j,k,cnt, strcnt, fontclass, stylecode, glyphenc, geoffset, realstylecode;
    int gid;
    KernPair *kp;
    DBounds b;
    char *pt;
    /* Fonds are generally marked system heap and sometimes purgeable (resource flags) */

    putlong(res,0);			/* Fill in length later */
    putshort(res,IsMacMonospaced(sf,map)?0x9000:0x1000);
    putshort(res,id);
    putshort(res,0);			/* First character */
    putshort(res,255);			/* Last character */
    putshort(res,(short) ((sf->ascent*(1<<12))/(sf->ascent+sf->descent)));
    putshort(res,-(short) ((sf->descent*(1<<12))/(sf->ascent+sf->descent)));
    putshort(res,(short) ((sf->pfminfo.linegap*(1<<12))/(sf->ascent+sf->descent)));
    putshort(res,(short) ((SFMacWidthMax(sf,map)*(1<<12))/(sf->ascent+sf->descent)));
    widoffpos = ftell(res);
    putlong(res,0);			/* Fill in width offset later */
    putlong(res,0);			/* Fill in kern offset later */
    putlong(res,0);			/* Fill in style offset later */
    for ( i=0; i<9; ++i )
	putshort(res,0);		/* Extra width values */
    putlong(res,0);			/* Script for international */
    putshort(res,2);			/* FOND version */

    /* Font association table */
    stylecode = realstylecode = MacStyleCode( sf, NULL );
    stylecode = 0;		/* Debug !!!! */
    for ( i=j=0; sizes!=NULL && sizes[i]!=0; ++i )
	if ( (sizes[i]>>16)==1 && (sizes[i]&0xffff)<256 )
	    ++j;
    if ( dottf ) {
	putshort(res,j+1-1);		/* Number of faces */
	putshort(res,0);		/* it's scaleable */
	putshort(res,stylecode);
	putshort(res,id);		/* Give it the same ID as the fond */
    } else
	putshort(res,j-1);		/* Number of faces */
    if ( sizes!=NULL ) {
	for ( i=0; sizes[i]!=0; ++i ) if (( sizes[i]>>16) == 1 ) {
	    putshort(res,sizes[i]&0xffff);
	    putshort(res,stylecode);
	    putshort(res,id+(sizes[i]&0xffff));	/* make up a unique ID */
	}
    }

    /* offset table */
    putshort(res,1-1);			/* One table */
    putlong(res,6);			/* Offset from start of otab to next byte */

    /* bounding box table */
    putshort(res,1-1);			/* One bounding box */
    SplineFontFindBounds(sf,&b);
    putshort(res,stylecode);
    putshort(res,b.minx*(1<<12)/(sf->ascent+sf->descent));
    putshort(res,b.miny*(1<<12)/(sf->ascent+sf->descent));
    putshort(res,b.maxx*(1<<12)/(sf->ascent+sf->descent));
    putshort(res,b.maxy*(1<<12)/(sf->ascent+sf->descent));

    widoffloc = ftell(res);
    putshort(res,1-1);			/* One style in the width table too */
    putshort(res,stylecode);
    for ( k=0; k<=256; ++k ) {
	if ( k>=map->enccount || k==256 || (gid=map->map[k])==-1 || sf->glyphs[gid]==NULL )
	    putshort(res,1<<12);	/* 1 em is default size */
	else
	    putshort(res,sf->glyphs[gid]->width*(1<<12)/(sf->ascent+sf->descent));
    }

    kernloc = 0;
    if (( cnt = SFMacAnyKerns(sf,map))>0 ) {
	kernloc = ftell(res);
	putshort(res,1-1);		/* One style in the width table too */
	putshort(res,stylecode);	/* style */
	putshort(res,cnt);		/* Count of kerning pairs */
	for ( k=0; k<256 && k<map->enccount; ++k ) {
	    if ( (gid=map->map[k])!=-1 && sf->glyphs[gid]!=NULL ) {
		for ( kp=sf->glyphs[gid]->kerns; kp!=NULL; kp=kp->next )
		    if ( map->backmap[kp->sc->orig_pos]<256 ) {
			putc(k,res);
			putc(map->backmap[kp->sc->orig_pos],res);
			putshort(res,kp->off*(1<<12)/(sf->ascent+sf->descent));
		    }
	    }
	}
    }

    /* Fontographer referenced a postscript font even in truetype FONDs */
    styleloc = ftell(res);
    fontclass = 0x1;		/* font name needs coordinating? Font has its own encoding */
    if ( !(realstylecode&sf_outline) ) fontclass |= 4;
    if ( realstylecode&sf_bold ) fontclass |= 0x18;
    if ( realstylecode&psf_italic ) fontclass |= 0x40;
    if ( realstylecode&psf_condense ) fontclass |= 0x80;
    if ( realstylecode&psf_extend ) fontclass |= 0x100;
    putshort(res,fontclass);		/* fontClass */
    geoffset = ftell(res);
    putlong(res,0);			/* Offset to glyph encoding table */ /* Fill in later */
    putlong(res,0);			/* Reserved, MBZ */
    if ( !sf->familyname || strnmatch(sf->familyname,sf->fontname,strlen(sf->familyname))!=0 )
	strcnt = 1;
    else if ( strmatch(sf->familyname,sf->fontname)==0 )
	strcnt = 1;
    else if ( sf->fontname[strlen(sf->familyname)]=='-' )
	strcnt = 4;
    else
	strcnt = 3;
    for ( k=0; k<48; ++k )
	putc(strcnt==1?1:2,res);	/* All indeces point to this font */
    putshort(res,strcnt);		/* strcnt strings */
    if ( strcnt==1 ) {
	putc(strlen(sf->fontname),res);	/* basename is full name */
	/* Mac expects this to be upper case */
	if ( islower(*sf->fontname)) putc(toupper(*sf->fontname),res);
	else putc(*sf->fontname,res);
	fwrite(sf->fontname+1,1,strlen(sf->fontname+1),res);
    } else {
        pt = sf->fontname+strlen(sf->familyname);
	putc(strlen(sf->familyname),res);/* basename */
	if ( islower(*sf->familyname)) putc(toupper(*sf->familyname),res);
	else putc(*sf->familyname,res);
	fwrite(sf->familyname+1,1,strlen(sf->familyname+1),res);
	if ( strcnt==3 ) {
	    putc(1,res);			/* index string is one byte long */
	    putc(3,res);			/* plain name is basename with string 2 */
	    putc(strlen(pt),res);		/* length of... */
	    fwrite(pt,1,strlen(pt),res);	/* everything else */
	} else {
	    putc(2,res);			/* index string is two bytes long */
	    putc(3,res);			/* plain name is basename with hyphen */
	    putc(4,res);			/* and everything else */
	    putc(1,res);			/* Length of ... */
	    putc('-',res);			/* String containing hyphen */
	    ++pt;			/* skip over hyphen */
	    putc(strlen(pt),res);		/* length of ... */
	    fwrite(pt,1,strlen(pt),res);	/* everything else */
	}
    }
    /* Greg: record offset for glyph encoding table */
    /* We assume that the bitmap and postscript fonts are encoded similarly */
    /*  and so a null vector will do. */
    /* GWW: Hmm. ATM refuses to use postscript fonts that have */
    /*  glyph encoding tables. Printer drivers use them ok. ATM will only */
    /*  work on fonts with mac roman encodings */
    if ( strmatch(map->enc->enc_name,"mac")!=0 &&
	    strmatch(map->enc->enc_name,"macintosh")!=0 &&
	    strmatch(map->enc->enc_name,"macroman")!=0 ) {
	if ( !dottf ) ff_post_notice(_("The generated font won't work with ATM"),_("ATM requires that fonts be encoded with the Macintosh Latin encoding. This postscript font will print fine, but only the bitmap versions will be displayed on the screen"));
	glyphenc = ftell( res );
	fseek(res,geoffset,SEEK_SET);
	putlong(res,glyphenc-geoffset+2);
	fseek(res,glyphenc,SEEK_SET);
	putshort(res,0);
    }

    end = ftell(res);
    fseek(res,widoffpos,SEEK_SET);
    putlong(res,widoffloc-rlenpos-4);	/* Fill in width offset */
    putlong(res,kernloc!=0?kernloc-rlenpos-4:0);	/* Fill in kern offset */
    putlong(res,styleloc!=0?styleloc-rlenpos-4:0);	/* Fill in style offset */

    fseek(res,rlenpos,SEEK_SET);
    putlong(res,end-rlenpos-4);		/* resource length */
    fseek(res,end,SEEK_SET);
return(rlenpos);
}

static void putpnsstring(FILE *res,char *fontname,int len) {
    putc(len,res);
    if ( *fontname && len>0 ) {
	if ( islower(*fontname))
	    putc(toupper(*fontname),res);
	else
	    putc(*fontname,res);
	--len;
	for ( ++fontname; *fontname && len>0; ++fontname, --len )
	    putc(*fontname,res);
    }
}

static void putpsstring(FILE *res,char *fontname) {
    putc(strlen(fontname),res);
    if ( *fontname ) {
	for ( ; *fontname; ++fontname )
	    putc(*fontname,res);
    }
}

struct sflistlist {
    struct sflist *sfs;
    struct sflistlist *next;
    char *fondname;
};

static struct sflistlist *FondSplitter(struct sflist *sfs,int *fondcnt) {
    struct sflist *psfaces[48], *sfi, *last, *start;
    struct sflistlist *sfsl=NULL, *lastl=NULL, *cur, *test;
    uint16 psstyle;
    int fc = 0;

    sfi = sfs;
    if ( sfi->sf->fondname==NULL ) {
	memset(psfaces,0,sizeof(psfaces));
	MacStyleCode(sfi->sf,&psstyle);
	last = NULL;
	while ( sfi->sf->fondname==NULL && psfaces[psstyle]==NULL ) {
	    psfaces[psstyle] = sfi;
	    last = sfi;
	    sfi = sfi->next;
	    if ( sfi==NULL )
	break;
	    MacStyleCode(sfi->sf,&psstyle);
	}
	cur = calloc(1,sizeof(struct sflistlist));
	cur->sfs = sfs;
	last->next = NULL;
	for ( last=sfi; last!=NULL; last=last->next )
	    if ( last->sf->fondname!=NULL && strcmp(last->sf->fondname,sfs->sf->familyname)==0 )
	break;
	cur->fondname = copy( last==NULL ? sfs->sf->familyname : sfs->sf->fontname );
	lastl = sfsl = cur;
	++fc;
    }
    while ( sfi!=NULL ) {
	start = sfi;
	if ( sfi->sf->fondname==NULL ) {
	    last = sfi;
	    sfi = sfi->next;
	} else {
	    memset(psfaces,0,sizeof(psfaces));
	    MacStyleCode(sfi->sf,&psstyle);
	    while ( sfi!=NULL && sfi->sf->fondname!=NULL &&
		    strcmp(sfi->sf->fondname,start->sf->fondname)==0 &&
		    psfaces[psstyle]==NULL ) {
		psfaces[psstyle] = sfi;
		last = sfi;
		sfi = sfi->next;
		if ( sfi==NULL )
	    break;
		MacStyleCode(sfi->sf,&psstyle);
	    }
	}
	cur = calloc(1,sizeof(struct sflistlist));
	test = NULL;
	if ( start->sf->fondname!=NULL ) {
	    for ( test = sfsl; test!=NULL; test=test->next )
		if ( strcmp(test->fondname,start->sf->fondname)==0 )
	    break;
	    if ( test==NULL )
		cur->fondname = copy(start->sf->fondname);
	}
	if ( cur->fondname==NULL )
	    cur->fondname = copy(start->sf->fontname);
	if ( lastl!=NULL ) lastl->next = cur;
	lastl = cur;
	cur->sfs = start;
	last->next = NULL;
	if ( sfsl==NULL )
	    sfsl = cur;
	++fc;
    }
    *fondcnt = fc;
return( sfsl );
}

static void SFListListFree(struct sflistlist *sfsl) {
    struct sflist *last = NULL;
    struct sflistlist *sfli, *sflnext;
    /* free the fond list and restore the sfs list */

    for ( sfli=sfsl; sfli!=NULL; sfli = sflnext ) {
	sflnext = sfli->next;
	if ( last!=NULL )
	    last->next = sfli->sfs;
	for ( last = sfli->sfs; last->next!=NULL; last = last->next );
	free(sfli->fondname);
	free(sfli);
    }
}

static uint32 SFsToFOND(FILE *res,struct sflist *sfs,uint32 id,int format) {
    uint32 rlenpos = ftell(res), widoffpos, widoffloc, kernloc, styleloc, end;
    int i,j,k,cnt, scnt, kcnt, pscnt, strcnt, fontclass, glyphenc, geoffset;
    int gid;
    int size;
    uint16 psstyle, stylecode;
    int exact, famlen, has_hyphen;
    char *familyname;
    KernPair *kp;
    DBounds b;
    /* Fonds are generally marked system heap and sometimes purgeable (resource flags) */
    struct sflist *faces[96];
    struct sflist *psfaces[48];
    SplineFont *sf;
    struct sflist *sfi;
    char *pt;

    memset(faces,0,sizeof(faces));
    memset(psfaces,0,sizeof(psfaces));
    for ( sfi = sfs ; sfi!=NULL; sfi = sfi->next ) {
	stylecode = MacStyleCode(sfi->sf,&psstyle);
	if ( sfs->next==NULL )		/* FONDs with a single entry should make that entry be "regular" no matter what we think it really is */
	    stylecode = psstyle = 0;
	if ( faces[stylecode]==NULL )
	    faces[stylecode] = sfi;
	if ( psfaces[psstyle]==NULL )
	    psfaces[psstyle] = sfi;
    }
    sf = faces[0]->sf;

    putlong(res,0);			/* Fill in length later */
    putshort(res,IsMacMonospaced(sf,faces[0]->map)?0x9000:0x1000);
    putshort(res,id);
    putshort(res,0);			/* First character */
    putshort(res,255);			/* Last character */
    putshort(res,(short) ((sf->ascent*(1<<12))/(sf->ascent+sf->descent)));
    putshort(res,-(short) ((sf->descent*(1<<12))/(sf->ascent+sf->descent)));
    putshort(res,(short) ((sf->pfminfo.linegap*(1<<12))/(sf->ascent+sf->descent)));
    putshort(res,(short) ((SFMacWidthMax(sf,faces[0]->map)*(1<<12))/(sf->ascent+sf->descent)));
    widoffpos = ftell(res);
    putlong(res,0);			/* Fill in width offset later */
    putlong(res,0);			/* Fill in kern offset later */
    putlong(res,0);			/* Fill in style offset later */
    for ( i=0; i<9; ++i )
	putshort(res,0);		/* Extra width values */
    putlong(res,0);			/* Script for international */
    putshort(res,2);			/* FOND version */

    /* Font association table */
    for ( i=cnt=scnt=kcnt=0; i<96; ++i ) if ( faces[i]!=NULL ) {
	++scnt;
	if ( faces[i]->id!=0 ) ++cnt;
	if ( faces[i]->ids!=NULL )
	    for ( j=0; faces[i]->ids[j]!=0; ++j ) ++cnt;
	if ( SFMacAnyKerns(faces[i]->sf,faces[i]->map)>0 )
	    ++kcnt;
    }
    putshort(res,cnt-1);		/* Number of faces */
    /* do ttf faces (if any) first */
    for ( i=cnt=0; i<96; ++i ) if ( faces[i]!=NULL && faces[i]->id!=0 ) {
	putshort(res,0);		/* it's scaleable */
	putshort(res,i);		/* style */
	putshort(res,faces[i]->id);
    }
    /* then do bitmap faces (if any) */ /* Ordered by size damn it */
    for ( size=1; size<256; ++size ) {
	for ( i=0; i<96; ++i ) if ( faces[i]!=NULL && faces[i]->ids!=NULL ) {
	    for ( j=0; faces[i]->ids[j]!=0 ; ++j ) {
		int pointsize = rint( (faces[i]->bdfs[j]->pixelsize*72.0/mac_dpi) );
		if ( pointsize==size ) {
		    putshort(res,size);
		    putshort(res,i);		/* style */
		    putshort(res,faces[i]->ids[j]);
		}
	    }
	}
    }

    /* offset table */
    putshort(res,1-1);			/* One table */
    putlong(res,6);			/* Offset from start of otab to next byte */

    /* bounding box table */
    putshort(res,scnt-1);		/* One bounding box per style */
    for ( i=0; i<96; ++i ) if ( faces[i]!=NULL ) {
	SplineFontFindBounds(faces[i]->sf,&b);
	putshort(res,i);			/* style */
	putshort(res,b.minx*(1<<12)/(faces[i]->sf->ascent+faces[i]->sf->descent));
	putshort(res,b.miny*(1<<12)/(faces[i]->sf->ascent+faces[i]->sf->descent));
	putshort(res,b.maxx*(1<<12)/(faces[i]->sf->ascent+faces[i]->sf->descent));
	putshort(res,b.maxy*(1<<12)/(faces[i]->sf->ascent+faces[i]->sf->descent));
    }

    widoffloc = ftell(res);
    putshort(res,scnt-1);		/* One set of width metrics per style */
    for ( i=0; i<96; ++i ) if ( faces[i]!=NULL ) {
	putshort(res,i);
	for ( k=0; k<=257; ++k ) {
	    if ( k>=faces[i]->map->enccount || k>=256 ||
		    (gid=faces[i]->map->map[k])==-1 || faces[i]->sf->glyphs[gid]==NULL )
		putshort(res,1<<12);	/* 1 em is default size */
	    else
		putshort(res,faces[i]->sf->glyphs[gid]->width*(1<<12)/(faces[i]->sf->ascent+faces[i]->sf->descent));
	}
    }

    kernloc = 0;
    if ( kcnt>0 ) {
	kernloc = ftell(res);
	putshort(res,kcnt-1);		/* Number of styles with kern pairs */
	for ( i=0; i<96; ++i ) if ( faces[i]!=NULL && ( cnt = SFMacAnyKerns(faces[i]->sf,faces[i]->map))>0 ) {
	    putshort(res,i);		/* style */
	    putshort(res,cnt);		/* Count of kerning pairs */
	    for ( k=0; k<256 && k<faces[i]->map->enccount; ++k ) {
		if ( (gid=faces[i]->map->map[k])!=-1 && faces[i]->sf->glyphs[gid]!=NULL ) {
		    for ( kp=faces[i]->sf->glyphs[gid]->kerns; kp!=NULL; kp=kp->next )
			if ( faces[i]->map->backmap[kp->sc->orig_pos]<256 ) {
			    putc(k,res);
			    putc(faces[i]->map->backmap[kp->sc->orig_pos],res);
			    putshort(res,kp->off*(1<<12)/(sf->ascent+sf->descent));
			}
		}
	    }
	}
    }

    /* Fontographer referenced a postscript font even in truetype FONDs */
    styleloc = ftell(res);

    exact = false;
    familyname = psfaces[0]->sf->fontname;
    famlen = strlen(familyname);
    if ( (pt=strchr(familyname,'-'))!=NULL )
	famlen = pt-familyname;
    else if ( strnmatch(psfaces[0]->sf->familyname,psfaces[0]->sf->fontname,
	    strlen(psfaces[0]->sf->familyname))==0 )
	famlen = strlen(psfaces[0]->sf->familyname);
    has_hyphen = (familyname[famlen]=='-');
    for ( i=pscnt=0; i<48; ++i ) if ( psfaces[i]!=NULL ) {
	++pscnt;
	if ( strncmp(psfaces[i]->sf->fontname,familyname,famlen)!=0 ) {
	    while ( famlen>0 ) {
		--famlen;
		if ( strncmp(psfaces[i]->sf->fontname,familyname,famlen)==0 )
	    break;
	    }
	}
    }
    if ( famlen!=0 ) for ( i=0; i<48; ++i ) if ( psfaces[i]!=NULL ) {
	if ( psfaces[i]->sf->fontname[famlen]==0 )
	    exact = true;
    }
    fontclass = 0x1;
    if ( psfaces[psf_outline]==NULL ) fontclass |= 4;
    if ( psfaces[psf_bold]!=NULL ) fontclass |= 0x18;
    if ( psfaces[psf_italic]!=NULL ) fontclass |= 0x40;
    if ( psfaces[psf_condense]!=NULL ) fontclass |= 0x80;
    if ( psfaces[psf_extend]!=NULL ) fontclass |= 0x100;
    putshort(res,fontclass);		/* fontClass */
    geoffset = ftell(res);
    putlong(res,0);			/* Offset to glyph encoding table */ /* Fill in later */
    putlong(res,0);			/* Reserved, MBZ */
    strcnt = 1/* Family Name */ + pscnt-exact /* count of format strings */ +
	    has_hyphen +
	    pscnt-exact /* count of additional strings */;
    /* indeces to format strings */
    for ( i=0,pscnt=2; i<48; ++i )
	if ( psfaces[i]!=NULL && psfaces[i]->sf->fontname[famlen]!=0 )
	    putc(pscnt++,res);
	else if ( exact )
	    putc(1,res);
	else
	    putc(2,res);
    putshort(res,strcnt);		/* strcnt strings */
    putpnsstring(res,familyname,famlen);
    if ( has_hyphen ) has_hyphen = pscnt++;	/* Space for hyphen if present */
    /* Now the format strings */
    for ( i=0; i<48; ++i ) if ( psfaces[i]!=NULL ) {
	if ( psfaces[i]->sf->fontname[famlen]!=0 ) {
	    if ( has_hyphen && psfaces[i]->sf->fontname[famlen]==' ' ) {
		putc(2,res);
		putc(has_hyphen,res);
		putc(pscnt++,res);
	    } else {
		putc(1,res);		/* Familyname with the following */
		putc(pscnt++,res);
	    }
	}
    }
    if ( has_hyphen ) {
	putc(1,res);
	putc('-',res);
    }
    /* Now the additional names */
    for ( i=0; i<48; ++i ) if ( psfaces[i]!=NULL ) {
	if ( psfaces[i]->sf->fontname[famlen]!=0 )
	    putpsstring(res,psfaces[i]->sf->fontname+famlen);
    }
    /* Greg: record offset for glyph encoding table */
    /* We assume that the bitmap and postscript fonts are encoded similarly */
    /*  and so a null vector will do. */
    /* GWW: Hmm. ATM refuses to use postscript fonts that have */
    /*  glyph encoding tables. Printer drivers use them ok. ATM will only */
    /*  work on fonts with mac roman encodings */
    if ( strmatch(psfaces[0]->map->enc->enc_name,"mac")!=0 &&
	    strmatch(psfaces[0]->map->enc->enc_name,"macintosh")!=0 &&
	    strmatch(psfaces[0]->map->enc->enc_name,"macroman")!=0 ) {
	if ( format==ff_pfbmacbin )
	    ff_post_notice(_("The generated font won't work with ATM"),_("ATM requires that fonts be encoded with the Macintosh Latin encoding. This postscript font will print fine, but only the bitmap versions will be displayed on the screen"));
	glyphenc = ftell( res );
	fseek(res,geoffset,SEEK_SET);
	putlong(res,glyphenc-geoffset+2);
	fseek(res,glyphenc,SEEK_SET);
	putshort(res,0); /* Greg: an empty Glyph encoding table */
    }

    end = ftell(res);
    fseek(res,widoffpos,SEEK_SET);
    putlong(res,widoffloc-rlenpos-4);	/* Fill in width offset */
    putlong(res,kernloc!=0?kernloc-rlenpos-4:0);	/* Fill in kern offset */
    putlong(res,styleloc!=0?styleloc-rlenpos-4:0);	/* Fill in style offset */

    fseek(res,rlenpos,SEEK_SET);
    putlong(res,end-rlenpos-4);		/* resource length */
    fseek(res,end,SEEK_SET);
return(rlenpos);
}

/* I presume this routine is called after all resources have been written */
static void DumpResourceMap(FILE *res,struct resourcetype *rtypes,enum fontformat format) {
    uint32 rfork_base = format>=ff_ttfdfont?0:128;	/* space for mac binary header */
    uint32 resource_base = rfork_base+0x100;
    uint32 rend, rtypesstart, mend, namestart;
    int i,j;

    fseek(res,0,SEEK_END);
    rend = ftell(res);

    if ( format<ff_ttfdfont ) {
	/* Duplicate resource header */
	putlong(res,0x100);			/* start of resource data */
	putlong(res,rend-rfork_base);		/* start of resource map */
	putlong(res,rend-rfork_base-0x100);	/* length of resource data */
	putlong(res,0);				/* don't know the length of the map section yet */
    } else {
	for ( i=0; i<16; ++i )			/* 16 bytes of zeroes */
	    putc(0,res);
    }

    putlong(res,0);			/* Some mac specific thing I don't understand */
    putshort(res,0);			/* another */
    putshort(res,0);			/* another */

    putshort(res,4+ftell(res)-rend);	/* Offset to resource types */
    putshort(res,0);			/* Don't know where the names go yet */

    rtypesstart = ftell(res);
    for ( i=0; rtypes[i].tag!=0; ++i );
    putshort(res,i-1);			/* Count of different types */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	putlong(res,rtypes[i].tag);	/* Resource type */
	putshort(res,0);		/* Number of resources of this type */
	putshort(res,0);		/* Offset to the resource list */
    }

    /* Now the resource lists... */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	rtypes[i].resloc = ftell(res);
	for ( j=0; rtypes[i].res[j].pos!=0; ++j ) {
	    putshort(res,rtypes[i].res[j].id);
	    rtypes[i].res[j].nameptloc = ftell(res);
	    putshort(res,0xffff);		/* assume no name at first */
	    putc(rtypes[i].res[j].flags,res);	/* resource flags */
		/* three byte resource offset */
	    putc( ((rtypes[i].res[j].pos-resource_base)>>16)&0xff, res );
	    putc( ((rtypes[i].res[j].pos-resource_base)>>8)&0xff, res );
	    putc( ((rtypes[i].res[j].pos-resource_base)&0xff), res );
	    putlong(res,0);
	}
    }
    namestart = ftell(res);
    /* Now the names, if any */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	for ( j=0; rtypes[i].res[j].pos!=0; ++j ) {
	    if ( rtypes[i].res[j].name!=NULL ) {
		rtypes[i].res[j].nameloc = ftell(res);
		putc(strlen(rtypes[i].res[j].name),res);	/* Length */
		fwrite(rtypes[i].res[j].name,1,strlen(rtypes[i].res[j].name),res);
	    }
	}
    }
    mend = ftell(res);

    /* Repeat the rtypes list now we know where they go */
    fseek(res,rtypesstart+2,SEEK_SET);		/* skip over the count */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	putlong(res,rtypes[i].tag);	/* Resource type */
	for ( j=0; rtypes[i].res[j].pos!=0; ++j );
	putshort(res,j-1);		/* Number of resources of this type */
	putshort(res,rtypes[i].resloc-rtypesstart);
    }
    /* And go back and fixup any name pointers */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	for ( j=0; rtypes[i].res[j].pos!=0; ++j ) {
	    if ( rtypes[i].res[j].name!=NULL ) {
		fseek(res,rtypes[i].res[j].nameptloc,SEEK_SET);
		putshort(res,rtypes[i].res[j].nameloc-namestart);
	    }
	}
    }

    fseek(res,rend,SEEK_SET);
    	/* Fixup duplicate header (and offset to the name list) */
    if ( format<ff_ttfdfont ) {
	putlong(res,0x100);			/* start of resource data */
	putlong(res,rend-rfork_base);		/* start of resource map */
	putlong(res,rend-rfork_base-0x100);	/* length of resource data */
	putlong(res,mend-rend);			/* length of map section */
    } else {
	for ( i=0; i<16; ++i )
	    putc(0,res);
    }

    putlong(res,0);			/* Some mac specific thing I don't understand */
    putshort(res,0);			/* another */
    putshort(res,0);			/* another */

    putshort(res,4+ftell(res)-rend);	/* Offset to resource types */
    putshort(res,namestart-rend);	/* name section */

    fseek(res,rfork_base,SEEK_SET);
    	/* Fixup main resource header */
    putlong(res,0x100);			/* start of resource data */
    putlong(res,rend-rfork_base);	/* start of resource map */
    putlong(res,rend-rfork_base-0x100);	/* length of resource data */
    putlong(res,mend-rend);		/* length of map section */
}

long mactime(void) {
    time_t now;

    time(&now);
    /* convert from 1970 based time to 1904 based time */
    now += (1970-1904)*365L*24*60*60+((1970-1904)>>2)*24*60*60;
    /* Ignore any leap seconds -- Sorry Steve */
    return( now );
}

static int DumpMacBinaryHeader(FILE *res,struct macbinaryheader *mb) {
#if !__Mac
    uint8 header[128], *hpt; char buffer[256], *pt, *dpt;
    uint32 len;
    time_t now;
    int crc;

    if ( mb->macfilename==NULL ) {
	char *pt = strrchr(mb->binfilename,'/');
	if ( pt==NULL ) pt = mb->binfilename;
	else ++pt;
	strncpy(buffer,pt,sizeof(buffer)-1);
	dpt = strrchr(buffer,'.');
	if ( dpt==NULL ) {
	    buffer[0] = '_';
	    strcpy(buffer+1,pt);
	} else
	    *dpt = '\0';
	mb->macfilename = buffer;
	buffer[63] = '\0';
    }

    memset(header,'\0',sizeof(header));
    hpt = header;
    *hpt++ = '\0';		/* version number */
    /* Mac Filename */
    pt = mb->macfilename;
    *hpt++ = strlen( pt );
    while ( *pt )
	*hpt++ = *pt++;
    while ( hpt<header+65 )
	*hpt++ = '\0';
    /* Mac File Type */
    *hpt++ = mb->type>>24; *hpt++ = mb->type>>16; *hpt++ = mb->type>>8; *hpt++ = mb->type;
    /* Mac Creator */
    *hpt++ = mb->creator>>24; *hpt++ = mb->creator>>16; *hpt++ = mb->creator>>8; *hpt++ = mb->creator;
    *hpt++ = '\0';		/* No finder flags set */
    *hpt++ = '\0';		/* (byte 74) MBZ */
    *hpt++ = '\0'; *hpt++ = '\0';	/* Vert Position in folder */
    *hpt++ = '\0'; *hpt++ = '\0';	/* Hor Position in folder */
    *hpt++ = '\0'; *hpt++ = '\0';	/* window or folder id??? */
    *hpt++ = '\0';		/* protected bit ??? */
    *hpt++ = '\0';		/* (byte 82) MBZ */
	/* Data fork length */
    *hpt++ = '\0'; *hpt++ = '\0'; *hpt++ = '\0'; *hpt++ = '\0';
	/* Resource fork length */
    fseek(res,0,SEEK_END);
    len = ftell(res)-sizeof(header);
    *hpt++ = len>>24; *hpt++ = len>>16; *hpt++ = len>>8; *hpt++ = len;
	/* Pad resource fork to be a multiple of 128 bytes */
    while ( (len&127)!=0 )
	{ putc('\0',res); ++len; }

	/* Creation time, (seconds from 1/1/1904) */
    now = mactime();
    time(&now);
    *hpt++ = now>>24; *hpt++ = now>>16; *hpt++ = now>>8; *hpt++ = now;
	/* Modification time, (seconds from 1/1/1904) */
    *hpt++ = now>>24; *hpt++ = now>>16; *hpt++ = now>>8; *hpt++ = now;

    *hpt++ = '\0'; *hpt++ = '\0';	/* Get Info comment length */
    *hpt++ = 0;				/* More finder flags */

/* MacBinary 3 */
    memcpy(header+102,"mBIN",4);
    header[106] = 0;			/* Script. I assume 0 is latin */
    header[107] = 0;			/* extended finder flags */
/* End of MacBinary 3 */
    header[122] = 130;			/* MacBinary version 3, written in (129 is MB2) */
    header[123] = 129;			/* MacBinary Version 2, needed to read */

    crc = binhex_crc(header,124);
    header[124] = crc>>8;
    header[125] = crc;

    fseek(res,0,SEEK_SET);
    fwrite(header,1,sizeof(header),res);
return( true );
#else
    int ret;
    FSRef ref, parentref;
#if __LP64__
    FSIORefNum macfile;
#else
    short macfile;
#endif
    char *buf, *dirname, *pt, *fname;
    HFSUniStr255 resforkname;
    FSCatalogInfo info;
    long len;
    unichar_t *filename;
    ByteCount whocares;
    /* When on the mac let's just create a real resource fork. We do this by */
    /*  creating a mac file with a resource fork, opening that fork, and */
    /*  dumping all the data in the temporary file after the macbinary header */

    /* The mac file routines are really lovely. I can't convert a pathspec to */
    /*  an FSRef unless the file exists.  That is incredibly stupid and annoying of them */
    /* But the directory should exist... */
    fname = mb->macfilename?mb->macfilename:mb->binfilename;
    dirname = copy(fname);
    pt = strrchr(dirname,'/');
    if ( pt!=NULL )
	pt[1] = '\0';
    else {
	free(dirname);
	dirname = copy(".");
    }
    ret=FSPathMakeRef( (uint8 *) dirname,&parentref,NULL);
    free(dirname);
    if ( ret!=noErr )
return( false );

    memset(&info,0,sizeof(info));
    ((FInfo *) (info.finderInfo))->fdType = mb->type;
    ((FInfo *) (info.finderInfo))->fdCreator = mb->creator;
    pt = strrchr(fname,'/');
    filename = def2u_copy(pt==NULL?fname:pt+1);
    { UniChar *ucs2fn = malloc((u_strlen(filename)+1) * sizeof(UniChar));
      int i;
	for ( i=0; filename[i]!=0; ++i )
	    ucs2fn[i] = filename[i];
	ucs2fn[i] = 0;
	ret = FSCreateFileUnicode(&parentref,u_strlen(filename), ucs2fn,
		    kFSCatInfoFinderInfo, &info, &ref, NULL);
	free(ucs2fn);
    }
    free(filename);
    if ( ret==dupFNErr ) {
    	/* File already exists, create failed, didn't get an FSRef */
	ret=FSPathMakeRef( (uint8 *) fname,&ref,NULL);
    }
    if ( ret!=noErr )
return( false );

    FSGetResourceForkName(&resforkname);
    FSCreateFork(&ref,resforkname.length,resforkname.unicode);	/* I don't think this is needed, but it doesn't hurt... */
    ret = FSOpenFork(&ref,resforkname.length,resforkname.unicode,fsWrPerm,&macfile);
    if ( ret!=noErr )
return( false );
    FSSetForkSize(macfile,fsFromStart,0);/* Truncate it just in case it existed... */
    fseek(res,128,SEEK_SET);	/* Everything after the mac binary header in */
	/* the temp file is resource fork */
    buf = malloc(8*1024);
    while ( (len=fread(buf,1,8*1024,res))>0 )
	FSWriteFork(macfile,fsAtMark,0,len,buf,&whocares);
    FSCloseFork(macfile);
    free(buf);
return( true );
#endif
}

static void WriteDummyMacHeaders(FILE *res) {
    /* Leave space for the mac binary header (128bytes) and the mac resource */
    /*  file header (256 bytes) */
    int i;
    for ( i=0; i<128; ++i )
	putc(0,res);
    for ( i=0; i<256; ++i )
	putc(0,res);
}

static void WriteDummyDFontHeaders(FILE *res) {
    /* Leave space for the mac resource file header (256 bytes) */
    /*  dfonts have the format of a data fork resource file (which I've never */
    /*  seen documented, but appears to be just like a resource fork except */
    /*  the first 16 bytes are not duplicated at the map */
    int i;
    for ( i=0; i<256; ++i )
	putc(0,res);
}

	/* The mac has rules about what the filename should be for a postscript*/
	/*  font. If you deviate from those rules the font will not be found */
	/*  The font name must begin with a capital letter */
	/*  The filename is designed by modifying the font name */
	/*  After the initial capital there can be at most 4 lower case letters (or digits) */
	/*   in the filename, any additional lc letters (or digits) in the fontname are ignored */
	/*  Every subsequent capital will be followed by at most 2 lc letters */
	/*  special characters ("-$", etc.) are removed entirely */
	/* So Times-Bold => TimesBol, HelveticaDemiBold => HelveDemBol */
	/* MacBinary limits the name to 63 characters, I dunno what happens if */
	/*  we excede that */
static void MakeMacPSName(char buffer[63],SplineFont *sf) {
    char *pt, *spt, *lcpt;

    for ( pt = buffer, spt = sf->fontname; *spt && pt<buffer+63-1; ++spt ) {
	if ( isupper(*spt) || spt==sf->fontname ) {
	    *pt++ = *spt;
	    lcpt = (spt==sf->fontname?spt+5:spt+3);
	} else if ( (islower(*spt) || isdigit(*spt)) && spt<lcpt )
	    *pt++ = *spt;
    }
    *pt = '\0';
}

int WriteMacPSFont(char *filename,SplineFont *sf,enum fontformat format,
	int flags,EncMap *map,int layer) {
    FILE *res, *temppfb;
    int ret = 1;
    struct resourcetype resources[2];
    struct macbinaryheader header;
    int lcfn = false, lcfam = false;
    char buffer[63];
#if __Mac
    char *pt;
#endif

    temppfb = tmpfile();
    if ( temppfb==NULL )
return( 0 );

	/* The mac has rules about what the filename should be for a postscript*/
	/*  font. If you deviate from those rules the font will not be found */
	/*  The font name must begin with a capital letter */
	/*  The filename is designed by modifying the font name */
	/*  After the initial capital there can be at most 4 lower case letters (or digits) */
	/*   in the filename, any additional lc letters (or digits) in the fontname are ignored */
	/*  Every subsequent capital will be followed by at most 2 lc letters */
	/*  special characters ("-$", etc.) are removed entirely */
	/* So Times-Bold => TimesBol, HelveticaDemiBold => HelveDemBol */
	/* MacBinary limits the name to 63 characters, I dunno what happens if */
	/*  we excede that */
    if ( islower(*sf->fontname)) { *sf->fontname = toupper(*sf->fontname); lcfn = true; }
    if ( islower(*sf->familyname)) { *sf->familyname = toupper(*sf->familyname); lcfam = true; }
    MakeMacPSName(buffer,sf);

    ret = _WritePSFont(temppfb,sf,ff_pfb,flags,map,NULL,layer);
    if ( lcfn ) *sf->fontname = tolower(*sf->fontname);
    if ( lcfam ) *sf->familyname = tolower(*sf->familyname);
    if ( ret==0 || ferror(temppfb) ) {
	fclose(temppfb);
return( 0 );
    }

    if ( __Mac && format==ff_pfbmacbin )
	res = tmpfile();
    else
	res = fopen(filename,"wb+");
    if ( res==NULL ) {
	fclose(temppfb);
return( 0 );
    }

    WriteDummyMacHeaders(res);
    memset(resources,'\0',sizeof(resources));
    rewind(temppfb);

    resources[0].tag = CHR('P','O','S','T');
    resources[0].res = PSToResources(res,temppfb);
    fclose(temppfb);
    DumpResourceMap(res,resources,format);
    free( resources[0].res );

#if __Mac
    header.macfilename = malloc(strlen(filename)+strlen(buffer)+1);
    strcpy(header.macfilename,filename);
    pt = strrchr(header.macfilename,'/');
    if ( pt==NULL ) pt=header.macfilename-1;
    strcpy(pt+1,buffer);
#else
    header.macfilename = buffer;
#endif
	/* Adobe uses a creator of ASPF (Adobe Systems PostScript Font I assume) */
	/* Fontographer uses ACp1 (Altsys Corp. PostScript type 1???) */
	/* Both include an FREF, BNDL, ICON* and comment */
	/* I shan't bother with that... It'll look ugly with no icon, but oh well */
    header.type = CHR('L','W','F','N');
    header.creator = CHR('G','W','p','1');
    ret = DumpMacBinaryHeader(res,&header);
    if ( ferror(res) ) ret = 0;
    if ( fclose(res)==-1 ) ret = 0;
#if __Mac
    free(header.macfilename);
#endif
return( ret );
}

int WriteMacTTFFont(char *filename,SplineFont *sf,enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags,EncMap *map,int layer) {
    FILE *res, *tempttf;
    int ret = 1, r;
    struct resourcetype resources[4];
    struct resource rlist[3][2], *dummynfnts=NULL;
    struct macbinaryheader header;

    tempttf = tmpfile();
    if ( tempttf==NULL )
return( 0 );

    if ( _WriteTTFFont(tempttf,sf,format==ff_none?ff_none:
				  format==ff_ttfmacbin?ff_ttf:
			          format-1,bsizes,bf,flags,map,layer)==0 || ferror(tempttf) ) {
	fclose(tempttf);
return( 0 );
    }
    if ( bf!=bf_ttf && bf!=bf_sfnt_dfont )
	bsizes = NULL;		/* as far as the FOND for the truetype is concerned anyway */

    if ( (__Mac && format==ff_ttfmacbin) || strstr(filename,"://")!=NULL )
	res = tmpfile();
    else
	res = fopen(filename,"wb+");
    if ( res==NULL ) {
	fclose(tempttf);
return( 0 );
    }

    if ( format!=ff_ttfmacbin )
	WriteDummyDFontHeaders(res);
    else
	WriteDummyMacHeaders(res);
    memset(rlist,'\0',sizeof(rlist));
    memset(resources,'\0',sizeof(resources));
    rewind(tempttf);

    r = 0;
    resources[r].tag = CHR('s','f','n','t');
    resources[r++].res = rlist[0];
    rlist[0][0].pos = TTFToResource(res,tempttf);
    rlist[0][0].id = HashToId(sf->fontname,sf,map);
    rlist[0][0].flags = 0x00;	/* sfnts generally have resource flags 0x20 */
    if ( bsizes!=NULL ) {
	resources[r].tag = CHR('N','F','N','T');
	resources[r++].res = dummynfnts = BuildDummyNFNTlist(res,sf,bsizes,rlist[0][0].id,map);
    }
    resources[r].tag = CHR('F','O','N','D');
    resources[r].res = rlist[1];
    rlist[1][0].pos = SFToFOND(res,sf,rlist[0][0].id,true,bsizes,map);
    rlist[1][0].flags = 0x00;	/* I've seen FONDs with resource flags 0, 0x20, 0x60 */
    rlist[1][0].id = rlist[0][0].id;
    rlist[1][0].name = sf->fondname ? sf->fondname : sf->familyname;
    fclose(tempttf);
    DumpResourceMap(res,resources,format);
    free(dummynfnts);

    ret = true;
    if ( format==ff_ttfmacbin ) {
	header.macfilename = NULL;
	header.binfilename = filename;
	    /* Fontographer uses the old suitcase format for both bitmaps and ttf */
	header.type = CHR('F','F','I','L');
	header.creator = CHR('D','M','O','V');
	ret = DumpMacBinaryHeader(res,&header);
    }
    if ( ferror(res) ) ret = false;
    if ( ret && strstr(filename,"://")!=NULL )
	ret = URLFromFile(filename,res);
    if ( fclose(res)==-1 ) ret = 0;
return( ret );
}

int WriteMacBitmaps(char *filename,SplineFont *sf, int32 *sizes, int is_dfont,
	EncMap *map) {
    FILE *res;
    int ret = 1;
    struct resourcetype resources[3];
    struct resource rlist[2][2];
    struct macbinaryheader header;
    char *binfilename, *pt, *dpt;

    /* The filename we've been given is for the outline font, which might or */
    /*  might not be stuffed inside a bin file */
    binfilename = malloc(strlen(filename)+strlen(".bmap.dfont")+1);
    strcpy(binfilename,filename);
    pt = strrchr(binfilename,'/');
    if ( pt==NULL ) pt = binfilename; else ++pt;
    dpt = strrchr(pt,'.');
    if ( dpt==NULL )
	dpt = pt+strlen(pt);
    else if ( strmatch(dpt,".bin")==0 || strmatch(dpt,".dfont")==0 ) {
	*dpt = '\0';
	dpt = strrchr(pt,'.');
	if ( dpt==NULL )
	    dpt = pt+strlen(pt);
    }
    strcpy(dpt,is_dfont?".bmap.dfont":__Mac?".bmap":".bmap.bin");

    if ( __Mac && !is_dfont )
	res = tmpfile();
    else
	res = fopen(binfilename,"wb+");
    if ( res==NULL ) {
	free(binfilename);
return( 0 );
    }

    if ( is_dfont )
	WriteDummyDFontHeaders(res);
    else
	WriteDummyMacHeaders(res);
    memset(rlist,'\0',sizeof(rlist));
    memset(resources,'\0',sizeof(resources));

    resources[0].tag = CHR('N','F','N','T');
    resources[0].res = SFToNFNTs(res,sf,sizes,map);
    resources[1].tag = CHR('F','O','N','D');
    resources[1].res = rlist[1];
    rlist[1][0].id = HashToId(sf->fontname,sf,map);
    rlist[1][0].pos = SFToFOND(res,sf,rlist[1][0].id,false,sizes,map);
    rlist[1][0].name = sf->fondname ? sf->fondname : sf->familyname;
    DumpResourceMap(res,resources,is_dfont?ff_ttfdfont:ff_ttfmacbin);

    ret = true;
    if ( !is_dfont ) {
	header.macfilename = NULL;
	header.binfilename = binfilename;
	    /* Fontographer uses the old suitcase format for both bitmaps and ttf */
	header.type = CHR('F','F','I','L');
	header.creator = CHR('D','M','O','V');
	ret = DumpMacBinaryHeader(res,&header);
    }
    if ( ferror(res)) ret = false;
    if ( fclose(res)==-1 ) ret = 0;
    free(resources[0].res);
    free(binfilename);
return( ret );
}

/* We have to worry about these font formats:
  ff_pfbmacbin, ff_ttfmacbin, ff_ttfdfont, ff_otfdfont, ff_otfciddfont, ff_none
  bf_ttf, bf_sfnt_dfont, bf_nfntmacbin,
  bf_nfntdfont	I no long support this. OS/X doesn't support NFNTs so there's no point
*/

int WriteMacFamily(char *filename,struct sflist *sfs,enum fontformat format,
	enum bitmapformat bf,int flags, int layer) {
    FILE *res;
    int ret = 1, r, i;
    struct resourcetype resources[4];
    struct resource *rlist;
    struct macbinaryheader header;
    struct sflist *sfi, *sfsub;
    char buffer[80];
    int freefilename = 0;
    char *pt;
    int id, fondcnt;
    struct sflistlist *sfsl, *sfli;

    if ( format==ff_pfbmacbin ) {
	for ( sfi=sfs; sfi!=NULL; sfi = sfi->next ) {
	    char *tempname;
	    MakeMacPSName(buffer,sfi->sf);
#if !__Mac
	    strcat(buffer,".bin");
#endif
	    tempname = malloc(strlen(filename)+strlen(buffer)+1);
	    strcpy(tempname,filename);
	    pt = strrchr(tempname,'/');
	    if ( pt==NULL ) pt=tempname-1;
	    strcpy(pt+1,buffer);
	    if ( strcmp(tempname,filename)==0 ) {
		char *tf = malloc(strlen(filename)+20);
		strcpy(tf,filename);
		filename = tf;
		freefilename = true;
		pt = strrchr(filename,'.');
		if ( pt==NULL || pt<strrchr(filename,'/'))
		    pt = filename+strlen(filename)+1;
#if __Mac
		strcpy(pt-1,".fam");
#else
		strcpy(pt-1,".fam.bin");
#endif
	    }
	    if ( WriteMacPSFont(tempname,sfi->sf,format,flags,sfi->map,layer)==0 )
return( 0 );
	    free(tempname);
	}
    } else if ( format!=ff_none || bf==bf_sfnt_dfont ) {
	for ( sfi=sfs; sfi!=NULL; sfi = sfi->next ) {
	    sfi->tempttf = tmpfile();
	    if ( sfi->tempttf==NULL ||
		    _WriteTTFFont(sfi->tempttf,sfi->sf,format==ff_none?ff_none:
					  format==ff_ttfmacbin?ff_ttf:
					  format-1,sfi->sizes,bf,flags,sfi->map,layer)==0 ||
		    ferror(sfi->tempttf) ) {
		for ( sfsub=sfs; sfsub!=sfi; sfsub=sfsub->next )
		    fclose( sfsub->tempttf );
return( 0 );
	    }
	    rewind(sfi->tempttf);
	}
    }

    if ( __Mac && (format==ff_ttfmacbin || format==ff_pfbmacbin ||
	    (format==ff_none && bf==bf_nfntmacbin)))
	res = tmpfile();
    else
	res = fopen(filename,"wb+");
    if ( res==NULL ) {
	for ( sfsub=sfs; sfsub!=NULL; sfsub=sfsub->next )
	    fclose( sfsub->tempttf );
return( 0 );
    }

    if ( format==ff_ttfdfont || format==ff_otfdfont || format==ff_otfciddfont ||
	    bf==bf_sfnt_dfont /*|| (format==ff_none && bf==bf_nfntdfont)*/)
	WriteDummyDFontHeaders(res);
    else
	WriteDummyMacHeaders(res);
    memset(resources,'\0',sizeof(resources));

    id = HashToId(sfs->sf->fontname,sfs->sf,sfs->map);

    r = 0;
    if ( format==ff_ttfmacbin || format==ff_ttfdfont || format==ff_otfdfont ||
	    format==ff_otfciddfont || (format==ff_none && bf==bf_sfnt_dfont )) {
	resources[r].tag = CHR('s','f','n','t');
	for ( sfi=sfs, i=0; sfi!=NULL; sfi=sfi->next, ++i );
	resources[r].res = calloc(i+1,sizeof(struct resource));
	for ( sfi=sfs, i=0; sfi!=NULL; sfi=sfi->next, ++i ) {
	    resources[r].res[i].pos = TTFToResource(res,sfi->tempttf);
	    resources[r].res[i].id = sfi->id = id+i;
	    resources[r].res[i].flags = 0x00;	/* sfnts generally have resource flags 0x20 */
	    fclose(sfi->tempttf);
	}
	++r;
	if ( bf==bf_ttf || bf==bf_sfnt_dfont ) {
	    resources[r].tag = CHR('N','F','N','T');
	    resources[r++].res = BuildDummyNFNTfamilyList(res,sfs,id);
	}
    }
    if ( bf==bf_nfntmacbin /*|| bf==bf_nfntdfont */) {
	resources[r].tag = CHR('N','F','N','T');
	resources[r++].res = SFsToNFNTs(res,sfs,id);
    }

    sfsl = FondSplitter(sfs,&fondcnt);
    rlist = calloc(fondcnt+1,sizeof(struct resource));
    resources[r].tag = CHR('F','O','N','D');
    resources[r++].res = rlist;
    for ( i=0, sfli=sfsl; i<fondcnt && sfli!=NULL; ++i, sfli = sfli->next ) {
	rlist[i].pos = SFsToFOND(res,sfli->sfs,id,format);
	rlist[i].flags = 0x00;	/* I've seen FONDs with resource flags 0, 0x20, 0x60 */
	rlist[i].id = id+i;
	rlist[i].name = sfli->fondname;
    }
    DumpResourceMap(res,resources,format!=ff_none?format:
	    bf==bf_nfntmacbin?ff_ttfmacbin:ff_ttfdfont);
    SFListListFree(sfsl);
    for ( i=0; i<r; ++i )
	free(resources[i].res);

    ret = true;
    if ( format==ff_ttfmacbin || format==ff_pfbmacbin ||
	    (format==ff_none && bf==bf_nfntmacbin) ) {
	header.macfilename = NULL;
	header.binfilename = filename;
	    /* Fontographer uses the old suitcase format for both bitmaps and ttf */
	header.type = CHR('F','F','I','L');
	header.creator = CHR('D','M','O','V');
	ret = DumpMacBinaryHeader(res,&header);
    }
    if ( ferror(res) ) ret = false;
    if ( fclose(res)==-1 ) ret = 0;
    if ( freefilename )
	free(filename);
    for ( sfi=sfs; sfi!=NULL; sfi=sfi->next ) {
	free( sfi->ids );
	free( sfi->bdfs );
    }
return( ret );
}

void SfListFree(struct sflist *sfs) {
    struct sflist *sfi;

    while ( sfs!=NULL ) {
	sfi = sfs->next;
	free(sfs->sizes);
	EncMapFree(sfs->map);
	chunkfree(sfs,sizeof(struct sflist));
	sfs = sfi;
    }
}

/* ******************************** Reading ********************************* */

static SplineFont *SearchPostScriptResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	int flags) {
    long here = ftell(f);
    long *offsets, lenpos;
    int rname = -1, tmp;
    int ch1, ch2;
    int len, type, i, j, rlen;
    unsigned short id, *rsrcids;
    /* I don't pretend to understand the rational behind the format of a */
    /*  postscript font. It appears to be split up into chunks where the */
    /*  maximum chunk size is 0x800, each section (ascii, binary, ascii, eof) */
    /*  has its own set of chunks (ie chunks don't cross sections) */
    char *buffer=NULL;
    int max = 0;
    FILE *pfb;
    FontDict *fd;
    SplineFont *sf;

    fseek(f,rlistpos,SEEK_SET);
    rsrcids = calloc(subcnt,sizeof(short));
    offsets = calloc(subcnt,sizeof(long));
    for ( i=0; i<subcnt; ++i ) {
	rsrcids[i] = getushort(f);
	tmp = (short) getushort(f);
	if ( rname==-1 ) rname = tmp;
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	offsets[i] = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
    }

    pfb = tmpfile();
    if ( pfb==NULL ) {
	LogError( _("Can't open temporary file for postscript output\n") );
	fseek(f,here,SEEK_SET );
	free(offsets);
return(NULL);
    }

    putc(0x80,pfb);
    putc(1,pfb);
    lenpos = ftell(pfb);
    putc(0,pfb);
    putc(0,pfb);
    putc(0,pfb);
    putc(0,pfb);
    len = 0; type = 1;
    id = 501;
    for ( i=0; i<subcnt; ++i ) {
	for ( j=0; j<subcnt; ++j )
	    if ( rsrcids[j]==id )
		break;
	if ( j == subcnt ) {
	    LogError( _("Missing POST resource %u\n"), id );
	    break;
	}
	id = id + 1;
	fseek(f,offsets[j],SEEK_SET);
	rlen = getlong(f);
	ch1 = getc(f); ch2 = getc(f);
	rlen -= 2;	/* those two bytes don't count as real data */
	if ( ch1==type )
	    len += rlen;
	else {
	    long hold = ftell(pfb);
	    fseek(pfb,lenpos,SEEK_SET);
	    putc(len>>24,pfb);
	    putc((len>>16)&0xff,pfb);
	    putc((len>>8)&0xff,pfb);
	    putc(len&0xff,pfb);
	    fseek(pfb,hold,SEEK_SET);
	    if ( ch1==5 )	/* end of font mark */
    break;
	    putc(0x80,pfb);
	    putc(ch1,pfb);
	    lenpos = ftell(pfb);
	    putc(0,pfb);
	    putc(0,pfb);
	    putc(0,pfb);
	    putc(0,pfb);
	    type = ch1;
	    len = rlen;
	}
	if ( rlen>max ) {
	    free(buffer);
	    max = rlen;
	    if ( max<0x800 ) max = 0x800;
	    buffer=malloc(max);
	    if ( buffer==NULL ) {
		LogError( _("Out of memory\n") );
		exit( 1 );
	    }
	}
	fread(buffer,1,rlen,f);
	fwrite(buffer,1,rlen,pfb);
    }
    free(buffer);
    free(offsets);
	free(rsrcids);
    putc(0x80,pfb);
    putc(3,pfb);
    fseek(pfb,lenpos,SEEK_SET);
    putc(len>>24,pfb);
    putc((len>>16)&0xff,pfb);
    putc((len>>8)&0xff,pfb);
    putc(len&0xff,pfb);
    fseek(f,here,SEEK_SET);
    rewind(pfb);
    if ( flags&ttf_onlynames )
return( (SplineFont *) _NamesReadPostScript(pfb) );	/* This closes the font for us */

    fd = _ReadPSFont(pfb);
    sf = NULL;
    if ( fd!=NULL ) {
	    sf = SplineFontFromPSFont(fd);
	    PSFontFree(fd);
	    /* There is no FOND in a postscript file, so we can't read any kerning*/
        fclose(pfb);
    }
return( sf );
}

static SplineFont *SearchTtfResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list,char *filename,int flags,enum openflags openflags) {
    long here, start = ftell(f);
    long roff;
    int ch1, ch2;
    int len, i, rlen, ilen;
    /* The sfnt resource is just a copy of the ttf file */
    char *buffer=NULL;
    int max = 0;
    FILE *ttf;
    SplineFont *sf;
    int which = 0;
    char **names;
    char *pt,*lparen, *rparen;
    char *chosenname=NULL;

    fseek(f,rlistpos,SEEK_SET);
    if ( subcnt>1 || (flags&ttf_onlynames) ) {
	names = calloc(subcnt+1,sizeof(char *));
	for ( i=0; i<subcnt; ++i ) {
	    /* resource id = */ getushort(f);
	    /* rname = (short) */ getushort(f);
	    /* flags = */ getc(f);
	    ch1 = getc(f); ch2 = getc(f);
	    roff = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	    /* mbz = */ getlong(f);
	    here = ftell(f);
	    names[i] = TTFGetFontName(f,roff+4,roff+4);
	    if ( names[i]==NULL ) {
		char buffer[32];
		sprintf( buffer, "Nameless%d", i );
		names[i] = copy(buffer);
	    }
	    fseek(f,here,SEEK_SET);
	}
	if ( flags&ttf_onlynames ) {
return( (SplineFont *) names );
	}
	if ((pt = strrchr(filename,'/'))==NULL ) pt = filename;
	/* Someone gave me a font "Nafees Nastaleeq(Updated).ttf" and complained */
	/*  that ff wouldn't open it */
	/* Now someone will complain about "Nafees(Updated).ttc(fo(ob)ar)" */
	if ( (lparen = strrchr(pt,'('))!=NULL &&
		(rparen = strrchr(lparen,')'))!=NULL &&
		rparen[1]=='\0' ) {
	    char *find = copy(lparen+1);
	    pt = strchr(find,')');
	    if ( pt!=NULL ) *pt='\0';
	    for ( which=subcnt-1; which>=0; --which )
		if ( strcmp(names[which],find)==0 )
	    break;
	    if ( which==-1 ) {
		char *end;
		which = strtol(find,&end,10);
		if ( *end!='\0' )
		    which = -1;
	    }
	    if ( which==-1 ) {
		char *fn = copy(filename);
		fn[lparen-filename] = '\0';
		ff_post_error(_("Not in Collection"),_("%s is not in %.100s"),find,fn);
		free(fn);
	    }
	    free(find);
	} else if ( no_windowing_ui )
	    which = 0;
	else
	    which = ff_choose(_("Pick a font, any font..."),(const char **) names,subcnt,0,_("There are multiple fonts in this file, pick one"));
	if ( lparen==NULL && which!=-1 )
	    chosenname = copy(names[which]);
	for ( i=0; i<subcnt; ++i )
	    free(names[i]);
	free(names);
	fseek(f,rlistpos,SEEK_SET);
    }

    for ( i=0; i<subcnt; ++i ) {
	/* resource id = */ getushort(f);
	/* rname = */ (short) getushort(f);
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	roff = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	if ( i!=which )
    continue;
	here = ftell(f);

	ttf = tmpfile();
	if ( ttf==NULL ) {
	    LogError( _("Can't open temporary file for truetype output.\n") );
    continue;
	}

	fseek(f,roff,SEEK_SET);
	ilen = rlen = getlong(f);
	if ( rlen>16*1024 )
	    ilen = 16*1024;
	if ( ilen>max ) {
	    free(buffer);
	    max = ilen;
	    if ( max<0x800 ) max = 0x800;
	    buffer=malloc(max);
	}
	for ( len=0; len<rlen; ) {
	    int temp = ilen;
	    if ( rlen-len<ilen ) temp = rlen-len;
	    temp = fread(buffer,1,temp,f);
	    if ( temp==EOF )
	break;
	    fwrite(buffer,1,temp,ttf);
	    len += temp;
	}
	rewind(ttf);
	sf = _SFReadTTF(ttf,flags,openflags,NULL,NULL);
	fclose(ttf);
	if ( sf!=NULL ) {
	    free(buffer);
	    fseek(f,start,SEEK_SET);
	    if ( sf->chosenname==NULL ) sf->chosenname = chosenname;
return( sf );
	}
	fseek(f,here,SEEK_SET);
    }
    free(chosenname);
    free(buffer);
    fseek(f,start,SEEK_SET);
return( NULL );
}

struct assoc {
    short size, style, id;
	/* size==0 => scalable */
	/* style>>8 is the bit depth (0=>1, 1=>2, 2=>4, 3=>8) */
	/* search order for ID is sfnt, NFNT, FONT */
};

struct stylewidths {
    short style;
    short *widthtab;	/* 4.12 fixed number with the width specified as a fraction of an em */
};

struct kerns {
    unsigned char ch1, ch2;
    short offset;		/* 4.12 */
};

struct stylekerns {
    short style;
    int kernpairs;
    struct kerns *kerns;
};

typedef struct fond {
    char *fondname;
    int first, last;
    int assoc_cnt;
    struct assoc *assoc;
    int stylewidthcnt;
    struct stylewidths *stylewidths;
    int stylekerncnt;
    struct stylekerns *stylekerns;
    char *psnames[48];
    struct fond *next;
} FOND;

struct MacFontRec {
   short fontType;
   short firstChar;
   short lastChar;
   short widthMax;
   short kernMax;		/* bb learing */
   short Descent;		/* maximum negative distance below baseline*/
   short fRectWidth;		/* bounding box width */
   short fRectHeight;		/* bounding box height */
   unsigned short *offsetWidths;/* offset to start of offset/width table */
   	/* 0xffff => undefined, else high byte is offset in locTable, */
   	/*  low byte is width */
   short ascent;
   short descent;
   short leading;
   short rowWords;		/* shorts per row */
   unsigned short *fontImage;	/* rowWords*fRectHeight */
   	/* Images for all characters plus one extra for undefined */
   unsigned short *locs;	/* lastchar-firstchar+3 words */
   	/* Horizontal offset to start of n'th character. Note: applies */
   	/*  to each row. Missing characters have same loc as following */
};

static void FondListFree(FOND *list) {
    FOND *next;
    int i;

    while ( list!=NULL ) {
	next = list->next;
	free(list->assoc);
	for ( i=0; i<list->stylewidthcnt; ++i )
	    free(list->stylewidths[i].widthtab);
	free(list->stylewidths);
	for ( i=0; i<list->stylekerncnt; ++i )
	    free(list->stylekerns[i].kerns);
	free(list->stylekerns);
	for ( i=0; i<48; ++i )
	    free(list->psnames[i]);
	free(list);
	list = next;
    }
}

/* There's probably only one fond in the file, but there could be more so be */
/*  prepared... */
/* I want the fond: */
/*  to get the fractional widths for the SWIDTH entry on bdf */
/*  to get the font name */
/*  to get the font association tables */
/*  to get the style flags */
/* http://developer.apple.com/techpubs/mac/Text/Text-269.html */
static FOND *BuildFondList(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list,int flags) {
    long here, start = ftell(f);
    long offset;
    int rname = -1;
    char name[300];
    int ch1, ch2;
    int i, j, k, cnt;
    FOND *head=NULL, *cur;
    long widoff, kernoff, styleoff;

    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	/* resource id = */ getushort(f);
	rname = (short) getushort(f);
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	offset = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	here = ftell(f);

	cur = calloc(1,sizeof(FOND));
	cur->next = head;
	head = cur;

	if ( rname!=-1 ) {
	    fseek(f,name_list+rname,SEEK_SET);
	    ch1 = getc(f);
	    fread(name,1,ch1,f);
	    name[ch1] = '\0';
	    cur->fondname = copy(name);
	}

	offset += 4;
	fseek(f,offset,SEEK_SET);
	/* isfixed = */ getushort(f) /* &0x8000?1:0 */;
	/* family id = */ getushort(f);
	cur->first = getushort(f);
	cur->last = getushort(f);
/* on a 1 point font... */
	/* ascent = */ getushort(f);
	/* descent = (short) */ getushort(f);
	/* leading = */ getushort(f);
	/* widmax = */ getushort(f);
	if ( (widoff = getlong(f))!=0 ) widoff += offset;
	if ( (kernoff = getlong(f))!=0 ) kernoff += offset;
	if ( (styleoff = getlong(f))!=0 ) styleoff += offset;
	for ( j=0; j<9; ++j )
	    getushort(f);
	/* internal & undefined, for international scripts = */ getlong(f);
	/* version = */ getushort(f);
	cur->assoc_cnt = getushort(f)+1;
	cur->assoc = calloc(cur->assoc_cnt,sizeof(struct assoc));
	for ( j=0; j<cur->assoc_cnt; ++j ) {
	    cur->assoc[j].size = getushort(f);
	    cur->assoc[j].style = getushort(f);
	    cur->assoc[j].id = getushort(f);
	}
	if ( widoff!=0 ) {
	    fseek(f,widoff,SEEK_SET);
	    cnt = getushort(f)+1;
	    cur->stylewidthcnt = cnt;
	    cur->stylewidths = calloc(cnt,sizeof(struct stylewidths));
	    for ( j=0; j<cnt; ++j ) {
		cur->stylewidths[j].style = getushort(f);
		cur->stylewidths[j].widthtab = malloc((cur->last-cur->first+3)*sizeof(short));
		for ( k=cur->first; k<=cur->last+2; ++k )
		    cur->stylewidths[j].widthtab[k] = getushort(f);
	    }
	}
	if ( kernoff!=0 && (flags&ttf_onlykerns) ) {
	    fseek(f,kernoff,SEEK_SET);
	    cnt = getushort(f)+1;
	    cur->stylekerncnt = cnt;
	    cur->stylekerns = calloc(cnt,sizeof(struct stylekerns));
	    for ( j=0; j<cnt; ++j ) {
		cur->stylekerns[j].style = getushort(f);
		cur->stylekerns[j].kernpairs = getushort(f);
		cur->stylekerns[j].kerns = malloc(cur->stylekerns[j].kernpairs*sizeof(struct kerns));
		for ( k=0; k<cur->stylekerns[j].kernpairs; ++k ) {
		    cur->stylekerns[j].kerns[k].ch1 = getc(f);
		    cur->stylekerns[j].kerns[k].ch2 = getc(f);
		    cur->stylekerns[j].kerns[k].offset = getushort(f);
		}
	    }
	}
	if ( styleoff!=0 ) {
	    uint8 stringoffsets[48];
	    int strcnt, stringlen, format;
	    char **strings, *pt;
	    fseek(f,styleoff,SEEK_SET);
	    /* class = */ getushort(f);
	    /* glyph encoding offset = */ getlong(f);
	    /* reserved = */ getlong(f);
	    for ( j=0; j<48; ++j )
		stringoffsets[j] = getc(f);
	    strcnt = getushort(f);
	    strings = malloc(strcnt*sizeof(char *));
	    for ( j=0; j<strcnt; ++j ) {
		stringlen = getc(f);
		strings[j] = malloc(stringlen+2);
		strings[j][0] = stringlen;
		strings[j][stringlen+1] = '\0';
		for ( k=0; k<stringlen; ++k )
		    strings[j][k+1] = getc(f);
	    }
	    for ( j=0; j<48; ++j ) {
		for ( k=j-1; k>=0; --k )
		    if ( stringoffsets[j]==stringoffsets[k] )
		break;
		if ( k!=-1 )
	    continue;		/* this style doesn't exist */
		format = stringoffsets[j]-1;
		stringlen = strings[0][0];
		if ( format!=0 )
		    for ( k=0; k<strings[format][0]; ++k )
			stringlen += strings[ strings[format][k+1]-1 ][0];
		pt = cur->psnames[j] = malloc(stringlen+1);
		strcpy(pt,strings[ 0 ]+1);
		pt += strings[ 0 ][0];
		if ( format!=0 )
		    for ( k=0; k<strings[format][0]; ++k ) {
			strcpy(pt,strings[ strings[format][k+1]-1 ]+1);
			pt += strings[ strings[format][k+1]-1 ][0];
		    }
		*pt = '\0';
	    }
	    for ( j=0; j<strcnt; ++j )
		free(strings[j]);
	    free(strings);
	}
	fseek(f,here,SEEK_SET);
    }
    fseek(f,start,SEEK_SET);
return( head );
}

static BDFChar *NFNTCvtBitmap(struct MacFontRec *font,int index,SplineFont *sf,int gid) {
    BDFChar *bdfc;
    int i,j, bits, bite, bit;

    bdfc = chunkalloc(sizeof(BDFChar));
    memset( bdfc,'\0',sizeof( BDFChar ));
    bdfc->xmin = (font->offsetWidths[index]>>8)+font->kernMax;
    bdfc->xmax = bdfc->xmin + font->locs[index+1]-font->locs[index]-1;
    if ( bdfc->xmax<bdfc->xmin )
	bdfc->xmax = bdfc->xmin;		/* Empty glyph mark */
    bdfc->ymin = -font->descent;
    bdfc->ymax = font->ascent-1;
    bdfc->width = font->offsetWidths[index]&0xff;
    bdfc->vwidth = font->ascent + font->descent;
    bdfc->bytes_per_line = ((bdfc->xmax-bdfc->xmin)>>3) + 1;
    bdfc->bitmap = calloc(bdfc->bytes_per_line*font->fRectHeight,sizeof(uint8));
    bdfc->orig_pos = gid;
    bdfc->sc = sf->glyphs[gid];

    bits = font->locs[index]; bite = font->locs[index+1];
    for ( i=0; i<font->fRectHeight; ++i ) {
	uint16 *test = font->fontImage + i*font->rowWords;
	uint8 *bpt = bdfc->bitmap + i*bdfc->bytes_per_line;
	for ( bit=bits, j=0; bit<bite; ++bit, ++j ) {
	    if ( test[bit>>4]&(0x8000>>(bit&0xf)) )
		bpt[j>>3] |= (0x80>>(j&7));
	}
    }
    BCCompressBitmap(bdfc);
return( bdfc );
}

static void LoadNFNT(FILE *f,long offset, SplineFont *sf) {
    long here = ftell(f);
    long baseow;
    long ow;
    int i,gid;
    struct MacFontRec font;
    BDFFont *bdf;
    SplineChar *sc;

    offset += 4;		/* skip over the length */
    fseek(f,offset,SEEK_SET);
    memset(&font,'\0',sizeof(struct MacFontRec));
    font.fontType = getushort(f);
    font.firstChar = getushort(f);
    font.lastChar = getushort(f);
    font.widthMax = getushort(f);
    font.kernMax = (short) getushort(f);
    font.Descent = (short) getushort(f);
    font.fRectWidth = getushort(f);
    font.fRectHeight = getushort(f);
    baseow = ftell(f);
    ow = getushort(f);
    font.ascent = getushort(f);
    font.descent = getushort(f);
    if ( font.Descent>=0 ) {
	ow |= (font.Descent<<16);
	font.Descent = -font.descent;		/* Possibly overkill, but should be safe */
    }
    font.leading = getushort(f);
    font.rowWords = getushort(f);
    if ( font.rowWords!=0 ) {
	font.fontImage = calloc(font.rowWords*font.fRectHeight,sizeof(short));
	font.locs = calloc(font.lastChar-font.firstChar+3,sizeof(short));
	font.offsetWidths = calloc(font.lastChar-font.firstChar+3,sizeof(short));
	for ( i=0; i<font.rowWords*font.fRectHeight; ++i )
	    font.fontImage[i] = getushort(f);
	for ( i=0; i<font.lastChar-font.firstChar+3; ++i )
	    font.locs[i] = getushort(f);
	fseek(f,baseow+2*ow,SEEK_SET);
	for ( i=0; i<font.lastChar-font.firstChar+3; ++i )
	    font.offsetWidths[i] = getushort(f);
    }
    fseek(f,here,SEEK_SET);
    if ( font.rowWords==0 )
return;

    /* Now convert the FONT record to one of my BDF structs */
    bdf = calloc(1,sizeof(BDFFont));
    bdf->sf = sf;
    bdf->next = sf->bitmaps;
    sf->bitmaps = bdf;
    bdf->glyphcnt = bdf->glyphmax = sf->glyphcnt;
    bdf->pixelsize = font.ascent+font.descent;
    bdf->glyphs = calloc(sf->glyphcnt,sizeof(BDFChar *));
    bdf->ascent = font.ascent;
    bdf->descent = font.descent;
    bdf->res = 72;
    for ( i=font.firstChar; i<=font.lastChar+1; ++i ) {
	if ( i!=font.lastChar+1 )
	    sc = SFMakeChar(sf,sf->map,i);
	else {
	    sc = SFGetChar(sf,-1,".notdef");
	    if ( sc==NULL )
    break;
	}
	gid = sc->orig_pos;
	bdf->glyphs[gid] = NFNTCvtBitmap(&font,i-font.firstChar,sf,gid);
	if ( !sf->glyphs[gid]->widthset ) {
	    sf->glyphs[gid]->width = bdf->glyphs[gid]->width*(sf->ascent+sf->descent)/bdf->pixelsize;
	    sf->glyphs[gid]->widthset = true;
	}
    }

    free(font.fontImage);
    free(font.locs);
    free(font.offsetWidths);
}

static char *BuildName(char *family,int style) {
    char buffer[350];

    strncpy(buffer,family,200);
    if ( style!=0 )
	strcat(buffer,"-");
    if ( style&sf_bold )
	strcat(buffer,"Bold");
    if ( style&sf_italic )
	strcat(buffer,"Italic");
    if ( style&sf_underline )
	strcat(buffer,"Underline");
    if ( style&sf_outline )
	strcat(buffer,"Outline");
    if ( style&sf_shadow )
	strcat(buffer,"Shadow");
    if ( style&sf_condense )
	strcat(buffer,"Condensed");
    if ( style&sf_extend )
	strcat(buffer,"Extended");
return( copy(buffer));
}

static int GuessStyle(char *fontname,int *styles,int style_cnt) {
    int which, style;
    const char *stylenames = _GetModifiers(fontname,NULL,NULL);

    style = _MacStyleCode(stylenames,NULL,NULL);
    for ( which = style_cnt; which>=0; --which )
	if ( styles[which] == style )
return( which );

return( -1 );
}

static FOND *PickFOND(FOND *fondlist,char *filename,char **name, int *style) {
    int i,j;
    FOND *test;
    uint8 stylesused[96];
    char **names;
    FOND **fonds, *fond;
    int *styles;
    int cnt, which;
    char *pt, *lparen;
    char *find = NULL;

    if ((pt = strrchr(filename,'/'))!=NULL ) pt = filename;
    if ( (lparen = strchr(filename,'('))!=NULL && strchr(lparen,')')!=NULL ) {
	find = copy(lparen+1);
	pt = strchr(find,')');
	if ( pt!=NULL ) *pt='\0';
	for ( test=fondlist; test!=NULL; test=test->next ) {
	    for ( i=0; i<48; ++i )
		if ( test->psnames[i]!=NULL && strcmp(find,test->psnames[i])==0 ) {
		    *style = (i&3) | ((i&~3)<<1);	/* PS styles skip underline bit */
		    *name = copy(test->psnames[i]);
return( test );
		}
	}
    }

    /* The file may contain multiple families, and each family may contain */
    /*  multiple styles (and each style may contain multiple sizes, but that's */
    /*  not an issue for us here) */
    names = NULL;
    for ( i=0; i<2; ++i ) {
	cnt = 0;
	for ( test=fondlist; test!=NULL; test=test->next ) if ( test->fondname!=NULL ) {
	    memset(stylesused,0,sizeof(stylesused));
	    for ( j=0; j<test->assoc_cnt; ++j ) {
		if ( test->assoc[j].size!=0 && !stylesused[test->assoc[j].style]) {
		    stylesused[test->assoc[j].style]=true;
		    if ( names!=NULL ) {
			names[cnt] = BuildName(test->fondname,test->assoc[j].style);
			styles[cnt] = test->assoc[j].style;
			fonds[cnt] = test;
		    }
		    ++cnt;
		}
	    }
	}
	if ( names==NULL ) {
	    names = calloc(cnt+1,sizeof(char *));
	    fonds = malloc(cnt*sizeof(FOND *));
	    styles = malloc(cnt*sizeof(int));
	}
    }

    if ( find!=NULL ) {
	for ( which=cnt-1; which>=0; --which )
	    if ( strcmp(names[which],find)==0 )
	break;
	if ( which==-1 && test!=NULL && strstrmatch(find,test->fondname)!=NULL )
	    which = GuessStyle(find,styles,cnt);
	if ( which==-1 ) {
	    char *fn = copy(filename);
	    fn[lparen-filename] = '\0';
	    ff_post_error(_("Not in Collection"),_("%s is not in %.100s"),find,fn);
	    free(fn);
	}
	free(find);
    } else if ( cnt==1 )
	which = 0;
    else if ( no_windowing_ui )
	which = 0;
    else
	which = ff_choose(_("Pick a font, any font..."),(const char **) names,cnt,0,
		_("There are multiple fonts in this file, pick one"));

    if ( which!=-1 ) {
	fond = fonds[which];
	*name = copy(names[which]);
	*style = styles[which];
    }
    for ( i=0; i<cnt; ++i )
	free(names[i]);
    free(names); free(fonds); free(styles);
    if ( which==-1 )
return( NULL );

return( fond );
}

static SplineFont *SearchBitmapResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list,char *filename,FOND *fondlist,int flags) {
    long start = ftell(f);
    long roff;
    int ch1, ch2;
    int i,j;
    int res_id;
    char *name;
    FOND *fond;
    int style;
    SplineFont *sf;
    int find_id;
    SplineChar *sc;

    fond = PickFOND(fondlist,filename,&name,&style);
    if ( fond==NULL )
return( NULL );

    find_id=-1;
    if ( flags&ttf_onlyonestrike ) {
	int biggest = 0;
	for ( i=0; i<fond->assoc_cnt; ++i ) {
	    if ( fond->assoc[i].style==style && fond->assoc[i].size>biggest ) {
		biggest = fond->assoc[i].size;
		find_id = fond->assoc[i].id;
	    }
	}
    }

    sf = SplineFontBlank(257);
    free(sf->fontname); sf->fontname = name;
    free(sf->familyname); sf->familyname = copy(fond->fondname);
    sf->fondname = copy(fond->fondname);
    free(sf->fullname); sf->fullname = copy(name);
    free(sf->origname); sf->origname = NULL;
    if ( style & sf_bold ) {
	free(sf->weight); sf->weight = copy("Bold");
    }
    free(sf->copyright); sf->copyright = NULL;
    free(sf->comments); sf->comments = NULL;

    sf->map = EncMapNew(257,257,FindOrMakeEncoding("mac"));

    for ( i=0; i<fond->stylewidthcnt; ++i ) {
	if ( fond->stylewidths[i].style == style ) {
	    short *widths = fond->stylewidths[i].widthtab;
	    for ( j=fond->first; j<=fond->last; ++j ) {
		sc = SFMakeChar(sf,sf->map,j);
		sc->width = ((widths[j-fond->first]*1000L+(1<<11))>>12);
		sc->widthset = true;
	    }
	    sc = SFSplineCharCreate(sf);
	    sc->orig_pos = sf->glyphcnt;
	    sf->glyphs[sf->glyphcnt++] = sc;
	    sc->name = copy(".notdef");
	    sc->width = (widths[fond->last+1-fond->first]*1000L+(1<<11))>>12;
	    sc->widthset = true;
	}
    }

    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	res_id = getushort(f);
	/* rname = */ getushort(f);
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	roff = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	for ( j=fond->assoc_cnt-1; j>=0; --j )
	    if ( (find_id!=-1 && res_id==find_id) ||
		    ( fond->assoc[j].style==style && fond->assoc[j].id==res_id &&
			fond->assoc[j].size!=0 ) )
		LoadNFNT(f,roff,sf);
    }
    fseek(f,start,SEEK_SET);

    sf->onlybitmaps = true;
    SFOrderBitmapList(sf);
return( sf );
}

/* Look for kerning info and merge it into the currently existing font "into" */
static SplineFont *FindFamilyStyleKerns(SplineFont *into,EncMap *map,FOND *fondlist,char *filename) {
    char *name;
    int style;
    FOND *fond;
    int i,j;
    int ch1, ch2, offset;
    KernPair *kp;
    SplineChar *sc1, *sc2;

    fond = PickFOND(fondlist,filename,&name,&style);
    if ( fond==NULL || into==NULL )
return( NULL );
    for ( i=0; i<fond->stylekerncnt; ++i )
	if ( fond->stylekerns[i].style==style )
    break;
    if ( i==fond->stylekerncnt ) {
	LogError(_("No kerning table for %s\n"), name );
	free(name);
return( NULL );
    }
    for ( j=0; j<fond->stylekerns[i].kernpairs; ++j ) {
	ch1 = fond->stylekerns[i].kerns[j].ch1;
	ch2 = fond->stylekerns[i].kerns[j].ch2;
	offset = (fond->stylekerns[i].kerns[j].offset*(into->ascent+into->descent)+(1<<11))>>12;
	sc1 = SFMakeChar(into,map,ch1);
	sc2 = SFMakeChar(into,map,ch2);
	for ( kp=sc1->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc==sc2 )
	break;
	if ( kp==NULL ) {
	    uint32 script;
	    kp = chunkalloc(sizeof(KernPair));
	    kp->sc = sc2;
	    kp->next = sc1->kerns;
	    sc1->kerns = kp;
	    script = SCScriptFromUnicode(sc1);
	    if ( script==DEFAULT_SCRIPT )
		script = SCScriptFromUnicode(sc2);
	    kp->subtable = SFSubTableFindOrMake(sc1->parent,CHR('k','e','r','n'),
		    script, gpos_pair);
	}
	kp->off = offset;
    }
return( into );
}

/* Look for a bare truetype font in a binhex/macbinary wrapper */
static SplineFont *MightBeTrueType(FILE *binary,int32 pos,int32 dlen,int flags,
	enum openflags openflags) {
    FILE *temp = tmpfile();
    char *buffer = malloc(8192);
    int len;
    SplineFont *sf;

    if ( flags&ttf_onlynames ) {
	char **ret;
	char *temp = TTFGetFontName(binary,pos,pos);
	if ( temp==NULL )
return( NULL );
	ret = malloc(2*sizeof(char *));
	ret[0] = temp;
	ret[1] = NULL;
return( (SplineFont *) ret );
    }

    fseek(binary,pos,SEEK_SET);
    while ( dlen>0 ) {
	len = dlen > 8192 ? 8192 : dlen;
	len = fread(buffer,1,dlen > 8192 ? 8192 : dlen,binary);
	if ( len==0 )
    break;
	fwrite(buffer,1,len,temp);
	dlen -= len;
    }
    rewind(temp);
    sf = _SFReadTTF(temp,flags,openflags,NULL,NULL);
    fclose(temp);
    free(buffer);
return( sf );
}

static SplineFont *IsResourceFork(FILE *f, long offset,char *filename,int flags,
	enum openflags openflags, SplineFont *into,EncMap *map) {
    /* If it is a good resource fork then the first 16 bytes are repeated */
    /*  at the location specified in bytes 4-7 */
    /* We include an offset because if we are looking at a mac binary file */
    /*  the resource fork will actually start somewhere in the middle of the */
    /*  file, not at the beginning */
    unsigned char buffer[16], buffer2[16];
    long rdata_pos, map_pos, type_list, name_list, rpos;
    int32 rdata_len;
    uint32 nfnt_pos, font_pos, fond_pos;
    unsigned long tag;
    int i, cnt, subcnt, nfnt_subcnt=0, font_subcnt=0, fond_subcnt=0;
    SplineFont *sf;
    FOND *fondlist=NULL;

    fseek(f,offset,SEEK_SET);
    if ( fread(buffer,1,16,f)!=16 )
return( NULL );
    rdata_pos = offset + ((buffer[0]<<24)|(buffer[1]<<16)|(buffer[2]<<8)|buffer[3]);
    map_pos = offset + ((buffer[4]<<24)|(buffer[5]<<16)|(buffer[6]<<8)|buffer[7]);
    rdata_len = ((buffer[8]<<24)|(buffer[9]<<16)|(buffer[10]<<8)|buffer[11]);
    /* map_len = ((buffer[12]<<24)|(buffer[13]<<16)|(buffer[14]<<8)|buffer[15]); */
    if ( rdata_pos+rdata_len!=map_pos || rdata_len==0 )
return( NULL );
    fseek(f,map_pos,SEEK_SET);
    buffer2[15] = buffer[15]+1;	/* make it be different */
    if ( fread(buffer2,1,16,f)!=16 )
return( NULL );

/* Apple's data fork resources appear to have a bunch of zeroes here instead */
/*  of a copy of the first 16 bytes */
    for ( i=0; i<16; ++i )
	if ( buffer2[i]!=0 )
    break;
    if ( i!=16 ) {
	for ( i=0; i<16; ++i )
	    if ( buffer[i]!=buffer2[i] )
return( NULL );
    }
    getlong(f);		/* skip the handle to the next resource map */
    getushort(f);	/* skip the file resource number */
    getushort(f);	/* skip the attributes */
    type_list = map_pos + getushort(f);
    name_list = map_pos + getushort(f);

    fseek(f,type_list,SEEK_SET);
    cnt = getushort(f)+1;
    for ( i=0; i<cnt; ++i ) {
	tag = getlong(f);
 /* printf( "%c%c%c%c\n", tag>>24, (tag>>16)&0xff, (tag>>8)&0xff, tag&0xff );*/
	subcnt = getushort(f)+1;
	rpos = type_list+getushort(f);
	sf = NULL;
	if ( tag==CHR('P','O','S','T') && !(flags&(ttf_onlystrikes|ttf_onlykerns)))		/* No FOND */
	    sf = SearchPostScriptResources(f,rpos,subcnt,rdata_pos,flags);
	else if ( tag==CHR('s','f','n','t') && !(flags&ttf_onlykerns))
	    sf = SearchTtfResources(f,rpos,subcnt,rdata_pos,name_list,filename,flags,openflags);
	else if ( tag==CHR('N','F','N','T') ) {
	    nfnt_pos = rpos;
	    nfnt_subcnt = subcnt;
	} else if ( tag==CHR('F','O','N','T') ) {
	    font_pos = rpos;
	    font_subcnt = subcnt;
	} else if ( tag==CHR('F','O','N','D') ) {
	    fond_pos = rpos;
	    fond_subcnt = subcnt;
	}
	if ( sf!=NULL )
return( sf );
    }
    if ( flags&ttf_onlynames )			/* Not interested in bitmap resources here */
return( NULL );

    if ( flags&ttf_onlykerns ) {		/* For kerns */
	if ( fond_subcnt!=0 )
	    fondlist = BuildFondList(f,fond_pos,fond_subcnt,rdata_pos,name_list,flags);
	into = FindFamilyStyleKerns(into,map,fondlist,filename);
	FondListFree(fondlist);
return( into );
    }
    /* Ok. If no outline font, try for a bitmap */
    if ( nfnt_subcnt==0 ) {
	nfnt_pos = font_pos;
	nfnt_subcnt = font_subcnt;
    }
    if ( nfnt_subcnt!=0 ) {
	if ( fond_subcnt!=0 )
	    fondlist = BuildFondList(f,fond_pos,fond_subcnt,rdata_pos,name_list,flags);
	sf = SearchBitmapResources(f,nfnt_pos,nfnt_subcnt,rdata_pos,name_list,
		filename,fondlist,flags);
	FondListFree(fondlist);
	if ( sf!=NULL )
return( sf );
    }
return( (SplineFont *) -1 );	/* It's a valid resource file, but just has no fonts */
}

#if __Mac
static SplineFont *HasResourceFork(char *filename,int flags,enum openflags openflags,
	SplineFont *into,EncMap *map) {
    /* If we're on a mac, we can try to see if we've got a real resource fork */
    /* (if we do, copy it into a temporary data file and then manipulate that)*/
    SplineFont *ret;
    FILE *resfork;
    char *tempfn=filename, *pt, *lparen, *respath;

    if (( pt=strrchr(filename,'/'))==NULL ) pt = filename;
    if ( (lparen = strchr(pt,'('))!=NULL && strchr(lparen,')')!=NULL ) {
	tempfn = copy(filename);
	tempfn[lparen-filename] = '\0';
    }
    respath = malloc(strlen(tempfn)+strlen("/..namedfork/rsrc")+1);
    strcpy(respath,tempfn);
    strcat(respath,"/..namedfork/rsrc");
    resfork = fopen(respath,"r");
    if ( resfork==NULL ) {
	strcpy(respath,tempfn);
	strcat(respath,"/rsrc");
	resfork = fopen(respath,"r");
    }
    free(respath);
    if ( tempfn!=filename )
	free(tempfn);
    if ( resfork==NULL )
return( NULL );
    ret = IsResourceFork(resfork,0,filename,flags,openflags,into,map);
    fclose(resfork);
return( ret );
}
#endif

static SplineFont *IsResourceInBinary(FILE *f,char *filename,int flags,
	enum openflags openflags, SplineFont *into,EncMap *map) {
    unsigned char header[128];
    unsigned long offset, dlen, rlen;

    if ( fread(header,1,128,f)!=128 )
return( NULL );
    if ( header[0]!=0 || header[74]!=0 || header[82]!=0 || header[1]<=0 ||
	    header[1]>33 || header[63]!=0 || header[2+header[1]]!=0 )
return( NULL );
    dlen = ((header[0x53]<<24)|(header[0x54]<<16)|(header[0x55]<<8)|header[0x56]);
    rlen = ((header[0x57]<<24)|(header[0x58]<<16)|(header[0x59]<<8)|header[0x5a]);
	/* 128 bytes for header, then the dlen is padded to a 128 byte boundary */
    offset = 128 + ((dlen+127)&~127);
/* Look for a bare truetype font in a binhex/macbinary wrapper */
    if ( dlen!=0 && rlen<=dlen) {
	int pos = ftell(f);
	fread(header,1,4,f);
	header[5] = '\0';
	if ( strcmp((char *) header,"OTTO")==0 || strcmp((char *) header,"true")==0 ||
		strcmp((char *) header,"ttcf")==0 ||
		(header[0]==0 && header[1]==1 && header[2]==0 && header[3]==0))
return( MightBeTrueType(f,pos,dlen,flags,openflags));
    }
return( IsResourceFork(f,offset,filename,flags,openflags,into,map));
}

static int lastch=0, repeat = 0;
static void outchr(FILE *binary, int ch) {
    int i;

    if ( repeat ) {
	if ( ch==0 ) {
	    /* no repeat, output a literal 0x90 (the repeat flag) */
	    lastch=0x90;
	    putc(lastch,binary);
	} else {
	    for ( i=1; i<ch; ++i )
		putc(lastch,binary);
	}
	repeat = 0;
    } else if ( ch==0x90 ) {
	repeat = 1;
    } else {
	putc(ch,binary);
	lastch = ch;
    }
}

static SplineFont *IsResourceInHex(FILE *f,char *filename,int flags,enum openflags openflags,
	SplineFont *into,EncMap *map) {
    /* convert file from 6bit to 8bit */
    /* interesting data is enclosed between two colons */
    FILE *binary = tmpfile();
    char *sixbit = "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
    int ch, val, cnt, i, dlen, rlen;
    unsigned char header[20]; char *pt;
    SplineFont *ret;

    if ( binary==NULL ) {
	LogError( _("can't create temporary file\n") );
return( NULL );
    }

    lastch = repeat = 0;
    while ( (ch=getc(f))!=':' );	/* There may be comments before file start */
    cnt = val = 0;
    while ( (ch=getc(f))!=':' ) {
	if ( isspace(ch))
    continue;
	for ( pt=sixbit; *pt!=ch && *pt!='\0'; ++pt );
	if ( *pt=='\0' ) {
	    fclose(binary);
return( NULL );
	}
	val = (val<<6) | (pt-sixbit);
	if ( ++cnt==4 ) {
	    outchr(binary,(val>>16)&0xff);
	    outchr(binary,(val>>8)&0xff);
	    outchr(binary,val&0xff);
	    val = cnt = 0;
	}
    }
    if ( cnt!=0 ) {
	if ( cnt==1 )
	    outchr(binary,val<<2);
	else if ( cnt==2 ) {
	    val<<=4;
	    outchr(binary,(val>>8)&0xff);
	    outchr(binary,val&0xff);
	} else if ( cnt==3 ) {
	    val<<=6;
	    outchr(binary,(val>>16)&0xff);
	    outchr(binary,(val>>8)&0xff);
	    outchr(binary,val&0xff);
	}
    }

    rewind(binary);
    ch = getc(binary);	/* Name length */
    /* skip name */
    for ( i=0; i<ch; ++i )
	getc(binary);
    if ( getc(binary)!='\0' ) {
	fclose(binary);
return( NULL );
    }
    fread(header,1,20,binary);
    dlen = (header[10]<<24)|(header[11]<<16)|(header[12]<<8)|header[13];
    rlen = (header[14]<<24)|(header[15]<<16)|(header[16]<<8)|header[17];
/* Look for a bare truetype font in a binhex/macbinary wrapper */
    if ( dlen!=0 && rlen<dlen ) {
	int pos = ftell(binary);
	fread(header,1,4,binary);
	header[5] = '\0';
	if ( strcmp((char *) header,"OTTO")==0 || strcmp((char *) header,"true")==0 ||
		strcmp((char *) header,"ttcf")==0 ||
		(header[0]==0 && header[1]==1 && header[2]==0 && header[3]==0)) {
	    ret = MightBeTrueType(binary,pos,dlen,flags,openflags);
	    fclose(binary);
return( ret );
	}
    }
    if ( rlen==0 ) {
	fclose(binary);
return( NULL );
    }

    ret = IsResourceFork(binary,ftell(binary)+dlen+2,filename,flags,openflags,into,map);

    fclose(binary);
return( ret );
}

static SplineFont *IsResourceInFile(char *filename,int flags,enum openflags openflags,
	SplineFont *into, EncMap *map) {
    FILE *f;
    char *spt, *pt;
    SplineFont *sf;
    char *temp=filename, *lparen;

    if (( pt=strrchr(filename,'/'))==NULL ) pt = filename;
    if ( (lparen = strchr(pt,'('))!=NULL && strchr(lparen,')')!=NULL ) {
	temp = copy(filename);
	temp[lparen-filename] = '\0';
    }
    f = fopen(temp,"rb");
    if ( temp!=filename ) free(temp);
    if ( f==NULL )
return( NULL );
    spt = strrchr(filename,'/');
    if ( spt==NULL ) spt = filename;
    pt = strrchr(spt,'.');
    if ( pt!=NULL && (pt[1]=='b' || pt[1]=='B') && (pt[2]=='i' || pt[2]=='I') &&
	    (pt[3]=='n' || pt[3]=='N') && (pt[4]=='\0' || pt[4]=='(') ) {
	if ( (sf = IsResourceInBinary(f,filename,flags,openflags,into,map))) {
	    fclose(f);
return( sf );
	}
    } else if ( pt!=NULL && (pt[1]=='h' || pt[1]=='H') && (pt[2]=='q' || pt[2]=='Q') &&
	    (pt[3]=='x' || pt[3]=='X') && (pt[4]=='\0' || pt[4]=='(')) {
	if ( (sf = IsResourceInHex(f,filename,flags,openflags,into,map))) {
	    fclose(f);
return( sf );
	}
    }

    sf = IsResourceFork(f,0,filename,flags,openflags,into,map);
    fclose(f);
#if __Mac
    if ( sf==NULL )
	sf = HasResourceFork(filename,flags,openflags,into,map);
#endif
return( sf );
}

static SplineFont *FindResourceFile(char *filename,int flags,enum openflags openflags,
	SplineFont *into,EncMap *map) {
    char *spt, *pt, *dpt;
    char buffer[1400];
    SplineFont *sf;

    if ( (sf = IsResourceInFile(filename,flags,openflags,into,map)))
return( sf );

    /* Well, look in the resource fork directory (if it exists), the resource */
    /*  fork is placed there in a separate file on (some) non-Mac disks */
    strcpy(buffer,filename);
    spt = strrchr(buffer,'/');
    if ( spt==NULL ) { spt = buffer; pt = filename; }
    else { ++spt; pt = filename + (spt-buffer); }
    strcpy(spt,"resource.frk/");
    strcat(spt,pt);
    if ( (sf=IsResourceInFile(buffer,flags,openflags,into,map)))
return( sf );

    /* however the resource fork does not appear to do long names properly */
    /*  names are always lower case 8.3, do some simple things to check */
    spt = strrchr(buffer,'/')+1;
    for ( pt=spt; *pt; ++pt )
	if ( isupper( *pt ))
	    *pt = tolower( *pt );
    dpt = strchr(spt,'.');
    if ( dpt==NULL ) dpt = spt+strlen(spt);
    if ( dpt-spt>8 || strlen(dpt)>4 ) {
	char exten[8];
	strncpy(exten,dpt,7);
	exten[4] = '\0';	/* it includes the dot */
	if ( dpt-spt>6 )
	    dpt = spt+6;
	*dpt++ = '~';
	*dpt++ = '1';
	strcpy(dpt,exten);
    }
return( IsResourceInFile(buffer,flags,openflags,into,map));
}

SplineFont *SFReadMacBinary(char *filename,int flags,enum openflags openflags) {
    SplineFont *sf = FindResourceFile(filename,flags,openflags,NULL,NULL);

    if ( sf==NULL )
	LogError( _("Couldn't find a font file named %s\n"), filename );
    else if ( sf==(SplineFont *) (-1) ) {
	LogError( _("%s is a mac resource file but contains no postscript or truetype fonts\n"), filename );
	sf = NULL;
    }
return( sf );
}

int LoadKerningDataFromMacFOND(SplineFont *sf, char *filename,EncMap *map) {
    if ( FindResourceFile(filename,ttf_onlykerns,0,sf,map)==NULL )
return ( false );

return( true );
}

char **NamesReadMacBinary(char *filename) {
return( (char **) FindResourceFile(filename,ttf_onlynames,0,NULL,NULL));
}
