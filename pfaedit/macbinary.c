/* Copyright (C) 2001-2002 by George Williams */
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "ustring.h"
#include "ttf.h"
#include "psfont.h"
#if __Mac
# include <MacFiles.h>
#else
#undef __Mac
#define __Mac 0
#endif

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
/*	resource data
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
/*	(and finally a crc checksum)
/*    is followed by the data section */
/*    is followed by the resource section */

/* MacBinary files use the same CRC that XModem does (in the MacBinary header) */
/* Taken from zglobal.h in lrzsz-0.12.20.tar, an xmodem package */
/*  http://www.ohse.de/uwe/software/lrzsz.html */
 /* crctab.c */
 extern unsigned short crctab[256];
# define updcrc(cp, crc) ( crctab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp)
 extern long cr3tab[];
# define UPDC32(b, c) (cr3tab[((int)c ^ b) & 0xff] ^ ((c >> 8) & 0x00FFFFFF))
/* End of borrowed code */

/* ******************************** Creation ******************************** */
#if !__Mac
static int crcbuffer(uint8 *buffer,int size) {
    int crc = 0, i;

    for ( i=0; size>=2; i+=2, size-=2 )
	crc = updcrc( ((buffer[i]<<8)|buffer[i+1]) , crc );
return( crc );
}
#endif

static uint16 HashToId(char *fontname,SplineFont *sf) {
    int low = 128, high = 0x3fff;
    /* A FOND ID should be between these two numbers for roman script (I think) */
    uint32 hash = 0;
    int i;
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
    } else if ( sf->encoding_name == em_big5 ) {
	low = 0x4200; high = 0x43ff;
    } else if ( sf->encoding_name == em_jis208 ||
	    sf->encoding_name == em_jis212 ||
	    sf->encoding_name == em_sjis ) {
	low = 0x4000; high = 0x41ff;
    } else if ( sf->encoding_name == em_johab ||
	    sf->encoding_name == em_ksc5601 ||
	    sf->encoding_name == em_wansung ) {
	low = 0x4400; high = 0x45ff;
    } else if ( sf->encoding_name == em_gb2312 ) {
	low = 0x7200; high = 0x73ff;
    } else for ( i=0; i<sf->charcnt && i<256; ++i ) if ( (sc = sf->chars[i])!=NULL ) {
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

static int IsMacMonospaced(SplineFont *sf) {
    /* Only look at first 256 */
    int i;
    double width = 0x80000000;

    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i]) ) {
	if ( width == 0x80000000 )
	    width = sf->chars[i]->width;
	else if ( sf->chars[i]->width!=width )
return( false );
    }
return( true );
}

SplineChar *SFFindExistingCharMac(SplineFont *sf,int unienc) {
    int i;

    for ( i=0; i<sf->charcnt && i<256; ++i )
	if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc==unienc )
return( sf->chars[i] );
return( NULL );
}

static double SFMacWidthMax(SplineFont *sf) {
    /* Only look at first 256 */
    int i;
    double width = -1;

    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i]) ) {
	if ( sf->chars[i]->width>width )
	    width = sf->chars[i]->width;
    }
    if ( width<0 )	/* No chars, or widths the mac doesn't support */
return( 0 );

return( width );
}

static int SFMacAnyKerns(SplineFont *sf) {
    /* Only look at first 256 */
    int i, cnt=0;
    KernPair *kp;

    for ( i=0; i<256 && i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( kp=sf->chars[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc->enc<256 )
		++cnt;
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
    resstarts = gcalloc(cnt+1,sizeof(struct resource));

    cnt = 0;
    forever {
	if ( getc(pfbfile)!=0x80 ) {
	    GDrawIError("We made a pfb file, but didn't get one. Hunh?" );
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

static uint32 BDFToNFNT(FILE *res, BDFFont *bdf) {
    short widths[258], lbearings[258], locs[258]/*, idealwidths[256]*/;
    uint8 **rows = galloc(bdf->pixelsize*sizeof(uint8 *));
    int i, k, width, kernMax=1, descentMax=bdf->descent-1, rectMax=1, widMax=3;
    uint32 rlenpos = ftell(res), end, owloc, owpos;

    for ( i=width=0; i<256 && i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL ) {
	width += bdf->chars[i]->xmax+1-bdf->chars[i]->xmin;
	if ( bdf->chars[i]->width>widMax )
	    widMax = bdf->chars[i]->width;
	if ( bdf->chars[i]->xmax+1-bdf->chars[i]->xmin>rectMax )
	    rectMax = bdf->chars[i]->xmax+1-bdf->chars[i]->xmin;
	if ( bdf->chars[i]->xmin<kernMax )
	    kernMax = bdf->chars[i]->xmin;
	if ( bdf->chars[i]->ymin<-descentMax )
	    descentMax = -bdf->chars[i]->ymin;
    }
    if ( descentMax>bdf->descent ) descentMax = bdf->descent;
    ++width;			/* For the "undefined character */
    for ( k=0; k<bdf->pixelsize; ++k )
	rows[k] = gcalloc((width+7)/8 + 4 , sizeof(uint8));
    for ( i=width=0; i<256 ; ++i ) {
	locs[i] = width;
	if ( i>=bdf->charcnt || bdf->chars[i]==NULL || !SCWorthOutputting(bdf->chars[i]->sc) ) {
	    lbearings[i] = 0xff;
	    widths[i] = 0xff;
	    /*idealwidths[i] = 1<<12;		/* 1 em */
	} else {
	    lbearings[i] = bdf->chars[i]->xmin-kernMax;
	    widths[i] = bdf->chars[i]->width<0?0:
			bdf->chars[i]->width>=256?255:
			bdf->chars[i]->width;
	    /*idealwidths[i] = bdf->chars[i]->sc->width*(1<<12)/(bdf->sf->ascent+bdf->sf->descent);*/
	    width = BDFCCopyBitmaps(rows,width,bdf->chars[i],bdf);
	}
    }
    /* Now for the "undefined character", just a simple vertical bar */
    locs[i] = width;
    lbearings[i] = 1;
    widths[i] = 3;
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

    /* We've finished the bitmap conversion, now save it... */
    putlong(res,0);		/* Length, to be filled in later */
    putshort(res,IsMacMonospaced(bdf->sf)?0xb000:0x9000);	/* fontType */
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
    owloc = ftell(res);
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

static uint32 DummyNFNT(FILE *res, BDFFont *bdf) {
    /* This produces a stub NFNT which appears when the real data lives inside */
    /*  an sfnt (truetype) resource. We still need to produce an NFNT to tell */
    /*  the system that the pointsize is available. This NFNT has almost nothing */
    /*  in it, just the initial header, no metrics, no bitmaps */
    int i, width, kernMax=1, descentMax=bdf->descent-1, rectMax=1, widMax=3;
    uint32 rlenpos = ftell(res);

    for ( i=width=0; i<256 && i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL ) {
	width += bdf->chars[i]->xmax+1-bdf->chars[i]->xmin;
	if ( bdf->chars[i]->width>widMax )
	    widMax = bdf->chars[i]->width;
	if ( bdf->chars[i]->xmax+1-bdf->chars[i]->xmin>rectMax )
	    rectMax = bdf->chars[i]->xmax+1-bdf->chars[i]->xmin;
	if ( bdf->chars[i]->xmin<kernMax )
	    kernMax = bdf->chars[i]->xmin;
	if ( bdf->chars[i]->ymin<-descentMax )
	    descentMax = -bdf->chars[i]->ymin;
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

static struct resource *SFToNFNTs(FILE *res, SplineFont *sf, int32 *sizes) {
    int i, baseresid = HashToId(sf->fontname,sf);
    struct resource *resstarts;
    BDFFont *bdf;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( i=0; sizes[i]!=0; ++i );
    resstarts = gcalloc(i+1,sizeof(struct resource));

    for ( i=0; sizes[i]!=0; ++i ) {
	if ( (sizes[i]>>16)!=1 )
    continue;
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=1); bdf=bdf->next );
	if ( bdf==NULL )
    continue;
	resstarts[i].id = baseresid+bdf->pixelsize;
	resstarts[i].pos = BDFToNFNT(res,bdf);
	/* NFNTs seem to have resource flags of 0 */
    }
return(resstarts);
}

static struct resource *BuildDummyNFNTlist(FILE *res, SplineFont *sf, int32 *sizes, int baseresid) {
    int i;
    struct resource *resstarts;
    BDFFont *bdf;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( i=0; sizes[i]!=0; ++i );
    resstarts = gcalloc(i+1,sizeof(struct resource));

    for ( i=0; sizes[i]!=0; ++i ) {
	if ( (sizes[i]>>16)!=1 )
    continue;
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=1); bdf=bdf->next );
	if ( bdf==NULL )
    continue;
	resstarts[i].id = baseresid++;
	resstarts[i].pos = DummyNFNT(res,bdf);
	/* NFNTs seem to have resource flags of 0 */
    }
return(resstarts);
}

enum psstyle_flags { psf_bold = 1, psf_italic = 2, psf_outline = 4,
	psf_shadow = 0x8, psf_condense = 0x10, psf_extend = 0x20 };

uint16 MacStyleCode( SplineFont *sf ) {
    unsigned short stylecode= 0;
    char *styles = SFGetModifiers(sf);

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;

    if ( strstrmatch( styles, "Bold" ) || strstrmatch(styles,"Demi") ||
	    strstrmatch( styles,"Heav") || strstrmatch(styles,"Blac") ||
/* A few fonts have German/French styles in their names */
	    strstrmatch( styles,"Fett") || strstrmatch(styles,"Gras") ) {
	stylecode = sf_bold;
    } else if ( sf->weight!=NULL &&
	    (strstrmatch( sf->weight, "Bold" ) || strstrmatch(sf->weight,"Demi") ||
	     strstrmatch( sf->weight,"Heav") || strstrmatch(sf->weight,"Blac") ||
	     strstrmatch( sf->weight,"Fett") || strstrmatch(sf->weight,"Gras")) ) {
	stylecode = sf_bold;
    }
    /* URW uses four leter abbreviations of Italic and Oblique */
    if ( strstrmatch( styles, "Ital" ) || strstrmatch( styles, "Obli" ) ||
	    strstrmatch(styles, "Kurs")) {
	stylecode |= sf_italic;
    }
    if ( strstrmatch( styles, "Outl" ) ) {
	stylecode |= sf_outline;
    }
    if ( strstrmatch( styles, "Cond" ) ) {
	stylecode |= sf_condense;
    }
    if ( strstrmatch( styles, "Exte" ) ) {
	stylecode |= sf_extend;
    }
return( stylecode );
}

static uint32 SFToFOND(FILE *res,SplineFont *sf,uint32 id,int dottf,int32 *sizes) {
    uint32 rlenpos = ftell(res), widoffpos, widoffloc, kernloc, styleloc, end;
    int i,j,k,cnt, strcnt, fontclass, stylecode;
    KernPair *kp;
    DBounds b;
    char *pt;
    /* Fonds are generally marked system heap and sometimes purgeable (resource flags) */

    putlong(res,0);			/* Fill in length later */
    putshort(res,IsMacMonospaced(sf)?0x9000:0x1000);
    putshort(res,id);
    putshort(res,0);			/* First character */
    putshort(res,255);			/* Last character */
    putshort(res,(short) ((sf->ascent*(1<<12))/(sf->ascent+sf->descent)));
    putshort(res,-(short) ((sf->descent*(1<<12))/(sf->ascent+sf->descent)));
    putshort(res,(short) ((sf->pfminfo.linegap*(1<<12))/(sf->ascent+sf->descent)));
    putshort(res,(short) ((SFMacWidthMax(sf)*(1<<12))/(sf->ascent+sf->descent)));
    widoffpos = ftell(res);
    putlong(res,0);			/* Fill in width offset later */
    putlong(res,0);			/* Fill in kern offset later */
    putlong(res,0);			/* Fill in style offset later */
    for ( i=0; i<9; ++i )
	putshort(res,0);		/* Extra width values */
    putlong(res,0);			/* Script for international */
    putshort(res,2);			/* FOND version */

    /* Font association table */
    stylecode = MacStyleCode( sf );
    for ( i=j=0; sizes!=NULL && sizes[i]!=0; ++i )
	if ( (sizes[i]>>16)==1 )
	    ++j;
    if ( dottf ) {
	putshort(res,j+1-1);		/* Number of faces */
	putshort(res,0);		/* it's scaleable */
	putshort(res,stylecode);
	putshort(res,id);		/* Give it the same ID as the fond */
    } else
	putshort(res,i-1);		/* Number of faces */
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
    putshort(res,0);			/* plain style, No matter what it really is, pretend it's plain */
    putshort(res,b.minx*(1<<12)/(sf->ascent+sf->descent));
    putshort(res,b.miny*(1<<12)/(sf->ascent+sf->descent));
    putshort(res,b.maxx*(1<<12)/(sf->ascent+sf->descent));
    putshort(res,b.maxy*(1<<12)/(sf->ascent+sf->descent));

    widoffloc = ftell(res);
    putshort(res,1-1);			/* One style in the width table too */
    putshort(res,stylecode);
    for ( k=0; k<=256; ++k ) {
	if ( k>=sf->charcnt || k==256 || sf->chars[k]==NULL )
	    putshort(res,1<<12);	/* 1 em is default size */
	else
	    putshort(res,sf->chars[k]->width*(1<<12)/(sf->ascent+sf->descent));
    }

    kernloc = 0;
    if (( cnt = SFMacAnyKerns(sf))>0 ) {
	kernloc = ftell(res);
	putshort(res,1-1);		/* One style in the width table too */
	putshort(res,0);		/* plain style, No matter what it really is, pretend it's plain */
	putshort(res,cnt);		/* Count of kerning pairs */
	for ( k=0; k<256 && k<sf->charcnt; ++k ) if ( sf->chars[k]!=NULL ) {
	    for ( kp=sf->chars[k]->kerns; kp!=NULL; kp=kp->next )
		if ( kp->sc->enc<256 ) {
		    putshort(res,k);
		    putshort(res,kp->sc->enc);
		    putshort(res,kp->off*(1<<12)/(sf->ascent+sf->descent));
		}
	}
    }

    /* Fontographer referenced a postscript font even in truetype FONDs */
    styleloc = ftell(res);
    fontclass = 0x1;
    if ( !(stylecode&sf_outline) ) fontclass |= 4;
    if ( stylecode&sf_bold ) fontclass |= 0x8;
    if ( stylecode&psf_italic ) fontclass |= 0x40;
    if ( stylecode&psf_condense ) fontclass |= 0x80;
    if ( stylecode&psf_extend ) fontclass |= 0x100;
    putshort(res,fontclass);		/* fontClass */
    putlong(res,0);			/* Offset to glyph encoding table (which we don't use) */
    putlong(res,0);			/* Reserved, MBZ */
    if ( strnmatch(sf->familyname,sf->fontname,strlen(sf->familyname))!=0 )
	strcnt = 1;
    else if ( strmatch(sf->familyname,sf->fontname)==0 )
	strcnt = 1;
    else 
	strcnt = 3;
    for ( k=0; k<48; ++k )
	putc(strcnt,res);		/* All indeces point to this font */
    putshort(res,strcnt);		/* strcnt strings */
    pt = sf->fontname+strlen(sf->familyname);
    if ( strcnt==1 ) {
	putc(strlen(sf->fontname),res);	/* basename is full name */
	/* Mac expects this to be upper case */
	if ( islower(*sf->fontname)) putc(toupper(*sf->fontname),res);
	else putc(*sf->fontname,res);
	fwrite(sf->fontname+1,1,strlen(sf->fontname+1),res);
    } else {
	putc(strlen(sf->familyname),res);/* basename */
	if ( islower(*sf->familyname)) putc(toupper(*sf->familyname),res);
	else putc(*sf->familyname,res);
	fwrite(sf->familyname+1,1,strlen(sf->familyname+1),res);
	putc(strlen(pt),res);		/* basename */
	fwrite(pt,1,strlen(pt),res);	/* everything else */
	putc(1,res);			/* index string is one byte long */
	putc(2,res);			/* plain name is basename with string 2 */
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

static int DumpMacBinaryHeader(FILE *res,struct macbinaryheader *mb) {
#if !__Mac
    uint8 header[128], *hpt; char buffer[256], *pt, *dpt;
    uint32 len;
    time_t now;
    int i,crc;

    if ( mb->macfilename==NULL ) {
	char *pt = strrchr(mb->binfilename,'/');
	if ( pt==NULL ) pt = mb->binfilename;
	else ++pt;
	strcpy(buffer,pt);
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

	/* Creation time, (seconds from 1/1/1904) */
    time(&now);
    /* convert from 1970 based time to 1904 based time */
    now += (1970-1904)*365L*24*60*60;
    for ( i=1904; i<1970; i+=4 )
	now += 24*60*60;
    /* Ignore any leap seconds */
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

    crc = crcbuffer(header,124);
    header[124] = crc>>8;
    header[125] = crc;

    fseek(res,0,SEEK_SET);
    fwrite(header,1,sizeof(header),res);
return( true );
#else
    int ret;
    FSRef ref;
    FSSpec spec;
    short macfile;
    char *buf, *dirname, *pt, *fname;
    Str255 damnthemac;
    FSCatalogInfo info;
    int len;
    /* When on the mac let's just create a real resource fork. We do this by */
    /*  creating a mac file with a resource fork, opening that fork, and */
    /*  dumping all the data in the temporary file after the macbinary header */

    /* The mac file routines are really lovely. I can't convert a pathspec to */
    /*  an FSRef unless the file exists. So I can't get an FSSpec to create */
    /*  the file with. That is incredibly stupid and annoying of them */
    /* But the directory should exist... */
    fname = mb->macfilename?mb->macfilename:mb->binfilename;
    dirname = copy(fname);
    pt = strrchr(dirname,'/');
    if ( pt==NULL )
return( false );
    pt[1] = '\0';
    ret=FSPathMakeRef( (uint8 *) dirname,&ref,NULL);
    free(dirname);
    if ( ret!=noErr )
return( false );
    if ( FSGetCatalogInfo(&ref,kFSCatInfoNodeID,&info,NULL,&spec,NULL)!=noErr )
return( false );
    pt = strrchr(fname,'/')+1;
    damnthemac[0] = strlen(pt);
    strncpy( (char *) damnthemac+1,pt,damnthemac[0]);
    if ( (ret=FSMakeFSSpec(spec.vRefNum,info.nodeID,damnthemac,&spec))!=noErr &&
	    ret!=fnfErr )
return( false );
    if ( (ret=FSpCreateResFile(&spec,mb->creator,mb->type,smSystemScript))!=noErr &&
	    ret!=dupFNErr )
return( false );
    if ( FSpOpenRF(&spec,fsWrPerm,&macfile)!=noErr )
return( false );
    SetEOF(macfile,0);		/* Truncate it just in case it existed... */
    fseek(res,128,SEEK_SET);	/* Everything after the mac binary header in */
	/* the temp file is resource fork */
    buf = galloc(8*1024);
    while ( (len=fread(buf,1,8*1024,res))>0 )
	FSWrite(macfile,&len,buf);
    FSClose(macfile);
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

int WriteMacPSFont(char *filename,SplineFont *sf,enum fontformat format) {
    FILE *res, *temppfb;
    int ret = 1;
    struct resourcetype resources[2];
    struct macbinaryheader header;
    int lcfn = false, lcfam = false;
    char buffer[63], *pt, *spt, *lcpt=NULL;

    temppfb = tmpfile();
    if ( temppfb==NULL )
return( 0 );

	/* The mac has rules about what the filename should be for a postscript*/
	/*  font. If you deviate from those rules the font will not be found */
	/*  The font name must begin with a capital letter */
	/*  The filename is designed by modifying the font name */
	/*  After the initial capital there can be at most 4 lower case letters */
	/*   in the filename, any additional lc letters in the fontname are ignored */
	/*  Every subsequent capital will be followed by at most 2 lc letters */
	/* So Times-Bold => TimesBol, HelveticaDemiBold => HelveDemBol */
	/* MacBinary limits the name to 63 characters, I dunno what happens if */
	/*  we excede that */
    if ( islower(*sf->fontname)) { *sf->fontname = toupper(*sf->fontname); lcfn = true; }
    if ( islower(*sf->familyname)) { *sf->familyname = toupper(*sf->familyname); lcfam = true; }
    for ( pt = buffer, spt = sf->fontname; *spt && pt<buffer+sizeof(buffer)-1; ++spt ) {
	if ( isupper(*spt)) {
	    *pt++ = *spt;
	    lcpt = (spt==sf->fontname?spt+5:spt+3);
	} else if ( islower(*spt) && spt<lcpt )	/* what happens to digits? */
	    *pt++ = *spt;
    }
    *pt = '\0';

    ret = _WritePSFont(temppfb,sf,ff_pfb);
    if ( lcfn ) *sf->fontname = tolower(*sf->fontname);
    if ( lcfam ) *sf->familyname = tolower(*sf->familyname);
    if ( ret==0 || ferror(temppfb) ) {
	fclose(temppfb);
return( 0 );
    }

    if ( __Mac && format==ff_pfbmacbin )
	res = tmpfile();
    else
	res = fopen(filename,"w+");
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
	
    header.macfilename = galloc(strlen(filename)+strlen(buffer)+1);
    strcpy(header.macfilename,filename);
    pt = strrchr(header.macfilename,'/');
    if ( pt==NULL ) pt=header.macfilename-1;
    strcpy(pt+1,buffer);
	/* Adobe uses a creator of ASPF (Adobe Systems Postscript Font I assume) */
	/* Fontographer uses ACp1 (Altsys Corp. Postscript type 1???) */
	/* Both include an FREF, BNDL, ICON* and comment */
	/* I shan't bother with that... It'll look ugly with no icon, but oh well */
    header.type = CHR('L','W','F','N');
    header.creator = CHR('G','W','p','1');
    ret = DumpMacBinaryHeader(res,&header);
    if ( ferror(res) ) ret = 0;
    if ( fclose(res)==-1 ) ret = 0;
return( ret );
}

int WriteMacTTFFont(char *filename,SplineFont *sf,enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags) {
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
			          format-1,bsizes,bf,flags)==0 || ferror(tempttf) ) {
	fclose(tempttf);
return( 0 );
    }
    if ( bf!=bf_ttf_apple && bf!=bf_ttf_ms && bf!=bf_sfnt_dfont )
	bsizes = NULL;		/* as far as the FOND for the truetype is concerned anyway */

    if ( __Mac && format==ff_ttfmacbin )
	res = tmpfile();
    else
	res = fopen(filename,"w+");
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
    rlist[0][0].id = HashToId(sf->fontname,sf);
    rlist[0][0].flags = 0x00;	/* sfnts generally have resource flags 0x20 */
    if ( bsizes!=NULL ) {
	resources[r].tag = CHR('N','F','N','T');
	resources[r++].res = dummynfnts = BuildDummyNFNTlist(res,sf,bsizes,rlist[0][0].id);
    }
    resources[r].tag = CHR('F','O','N','D');
    resources[r].res = rlist[1];
    rlist[1][0].pos = SFToFOND(res,sf,rlist[0][0].id,true,bsizes);
    rlist[1][0].flags = 0x00;	/* I've seen FONDs with resource flags 0, 0x20, 0x60 */
    rlist[1][0].id = rlist[0][0].id;
    rlist[1][0].name = sf->familyname;
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
    if ( fclose(res)==-1 ) ret = 0;
return( ret );
}

int WriteMacBitmaps(char *filename,SplineFont *sf, int32 *sizes, int is_dfont) {
    FILE *res;
    int ret = 1;
    struct resourcetype resources[3];
    struct resource rlist[2][2];
    struct macbinaryheader header;
    char *binfilename, *pt, *dpt;

    /* The filename we've been given is for the outline font, which might or */
    /*  might not be stuffed inside a bin file */
    binfilename = galloc(strlen(filename)+strlen(".bmap.dfont")+1);
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
	res = fopen(binfilename,"w+");
    if ( res==NULL ) {
return( 0 );
    }

    if ( is_dfont )
	WriteDummyDFontHeaders(res);
    else
	WriteDummyMacHeaders(res);
    memset(rlist,'\0',sizeof(rlist));
    memset(resources,'\0',sizeof(resources));

    resources[0].tag = CHR('N','F','N','T');
    resources[0].res = SFToNFNTs(res,sf,sizes);
    resources[1].tag = CHR('F','O','N','D');
    resources[1].res = rlist[1];
    rlist[1][0].id = HashToId(sf->fontname,sf);
    rlist[1][0].pos = SFToFOND(res,sf,rlist[1][0].id,false,sizes);
    rlist[1][0].name = sf->familyname;
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
return( ret );
}

/* ******************************** Reading ********************************* */

static SplineFont *SearchPostscriptResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list) {
    long here = ftell(f);
    long *offsets, lenpos;
    int rname = -1, tmp;
    int ch1, ch2;
    int len, type, i, rlen;
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
    offsets = calloc(subcnt,sizeof(long));
    for ( i=0; i<subcnt; ++i ) {
	/* resource id = */ getushort(f);
	tmp = (short) getushort(f);
	if ( rname==-1 ) rname = tmp;
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	offsets[i] = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
    }

    pfb = tmpfile();
    if ( pfb==NULL ) {
	fprintf( stderr, "Can't open temporary file for postscript output\n" );
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
    for ( i=0; i<subcnt; ++i ) {
	fseek(f,offsets[i],SEEK_SET);
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
		fprintf( stderr, "Out of memory\n" );
		exit( 1 );
	    }
	}
	fread(buffer,1,rlen,f);
	fwrite(buffer,1,rlen,pfb);
    }
    free(buffer);
    free(offsets);
    putc(0x80,pfb);
    putc(3,pfb);
    fseek(pfb,lenpos,SEEK_SET);
    putc(len>>24,pfb);
    putc((len>>16)&0xff,pfb);
    putc((len>>8)&0xff,pfb);
    putc(len&0xff,pfb);
    fseek(f,here,SEEK_SET);
    rewind(pfb);
    fd = _ReadPSFont(pfb);
    sf = NULL;
    if ( fd!=NULL ) {
	sf = SplineFontFromPSFont(fd);
	PSFontFree(fd);
	/* There is no FOND in a postscript file, so we can't read any kerning*/
    }
    fclose(pfb);
return( sf );
}

static SplineFont *SearchTtfResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list,char *filename) {
    long here, start = ftell(f);
    long roff;
    int rname = -1;
    int ch1, ch2;
    int len, i, rlen, ilen;
    /* I think (hope) the sfnt resource is just a copy of the ttf file */
    char *buffer=NULL;
    int max = 0;
    FILE *ttf;
    SplineFont *sf;
    int which = 0;
    unichar_t **names;
    char *pt;

    fseek(f,rlistpos,SEEK_SET);
    if ( subcnt>1 ) {
	names = gcalloc(subcnt,sizeof(unichar_t *));
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
		names[i] = uc_copy(buffer);
	    }
	    fseek(f,here,SEEK_SET);
	}
	if ( (pt = strchr(filename,'('))!=NULL ) {
	    char *find = copy(pt+1);
	    pt = strchr(find,')');
	    if ( pt!=NULL ) *pt='\0';
	    for ( which=subcnt-1; which>=0; --which )
		if ( uc_strcmp(names[which],find)==0 )
	    break;
	    if ( which==-1 ) {
		char *fn = copy(filename);
		*strchr(fn,'(') = '\0';
		GWidgetErrorR(_STR_NotInCollection,_STR_FontNotInCollection,find,fn);
		free(fn);
	    }
	    free(find);
	} else if ( screen_display==NULL )
	    which = 0;
	else
	    which = GWidgetChoicesR(_STR_PickFont,(const unichar_t **) names,subcnt,0,_STR_MultipleFontsPick);
	for ( i=0; i<subcnt; ++i )
	    free(names[i]);
	free(names);
	fseek(f,rlistpos,SEEK_SET);
    }

    for ( i=0; i<subcnt; ++i ) {
	/* resource id = */ getushort(f);
	rname = (short) getushort(f);
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	roff = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	if ( i!=which )
    continue;
	here = ftell(f);

	ttf = tmpfile();
	if ( ttf==NULL ) {
	    fprintf( stderr, "Can't open temporary file for truetype output.\n" );
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
	sf = _SFReadTTF(ttf,0,NULL);
	fclose(ttf);
	if ( sf!=NULL ) {
	    fseek(f,start,SEEK_SET);
return( sf );
	}
	fseek(f,here,SEEK_SET);
    }
    fseek(f,start,SEEK_SET);
return( NULL );
}

static SplineFont *IsResourceFork(FILE *f, long offset,char *filename) {
    /* If it is a good resource fork then the first 16 bytes are repeated */
    /*  at the location specified in bytes 4-7 */
    /* We include an offset because if we are looking at a mac binary file */
    /*  the resource fork will actually start somewhere in the middle of the */
    /*  file, not at the beginning */
    unsigned char buffer[16], buffer2[16];
    long rdata_pos, map_pos, type_list, name_list, rpos;
    int32 rdata_len, map_len;
    unsigned long tag;
    int i, cnt, subcnt;
    SplineFont *sf;

    fseek(f,offset,SEEK_SET);
    if ( fread(buffer,1,16,f)!=16 )
return( NULL );
    rdata_pos = offset + ((buffer[0]<<24)|(buffer[1]<<16)|(buffer[2]<<8)|buffer[3]);
    map_pos = offset + ((buffer[4]<<24)|(buffer[5]<<16)|(buffer[6]<<8)|buffer[7]);
    rdata_len = ((buffer[8]<<24)|(buffer[9]<<16)|(buffer[10]<<8)|buffer[11]);
    map_len = ((buffer[12]<<24)|(buffer[13]<<16)|(buffer[14]<<8)|buffer[15]);
    if ( rdata_pos+rdata_len!=map_pos )
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
	if ( tag==CHR('P','O','S','T'))		/* No FOND */
	    sf = SearchPostscriptResources(f,rpos,subcnt,rdata_pos,name_list);
	else if ( tag==CHR('s','f','n','t'))
	    sf = SearchTtfResources(f,rpos,subcnt,rdata_pos,name_list,filename);
	if ( sf!=NULL )
return( sf );
    }
return( (SplineFont *) -1 );	/* It's a valid resource file, but just has no fonts */
}

#if __Mac
static SplineFont *HasResourceFork(char *filename) {
    /* If we're on a mac, we can try to see if we've got a real resource fork */
    /* (if we do, copy it into a temporary data file and then manipulate that)*/
    FSRef ref;
    FSSpec spec;
    short res, err;
    int cnt;
    SplineFont *ret;
    FILE *temp;
    char *buf;

    if ( FSPathMakeRef( (uint8 *) filename,&ref,NULL)!=noErr )
return( NULL );
    if ( FSGetCatalogInfo(&ref,0,NULL,NULL,&spec,NULL)!=noErr )
return( NULL );
    if ( FSpOpenRF(&spec,fsRdPerm,&res)!=noErr )
return( NULL );
    temp = tmpfile();
    buf = malloc(8192);
    while ( 1 ) {
	cnt = 8192;
	err = FSRead(res,&cnt,buf);
	if ( cnt!=0 )
	    fwrite(buf,1,cnt,temp);
	if ( err==eofErr )
    break;
	if ( err!=noErr )
    break;
    }
    free(buf);
    FSClose(res);
    rewind(temp);
    ret = IsResourceFork(temp,0,filename);
    fclose(temp);
return( ret );
}
#endif

static SplineFont *IsResourceInBinary(FILE *f,char *filename) {
    unsigned char header[128];
    unsigned long offset;

    if ( fread(header,1,128,f)!=128 )
return( NULL );
    if ( header[0]!=0 || header[74]!=0 || header[82]!=0 || header[1]<=0 ||
	    header[1]>33 || header[63]!=0 || header[2+header[1]]!=0 )
return( NULL );
    offset = 128+((header[0x53]<<24)|(header[0x54]<<16)|(header[0x55]<<8)|header[0x56]);
return( IsResourceFork(f,offset,filename));
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

static SplineFont *IsResourceInHex(FILE *f,char *filename) {
    /* convert file from 6bit to 8bit */
    /* interesting data is enclosed between two colons */
    FILE *binary = tmpfile();
    char *sixbit = "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
    int ch, val, cnt, i, dlen, rlen;
    char header[20], *pt;
    SplineFont *ret;

    if ( binary==NULL ) {
	fprintf( stderr, "can't create temporary file\n" );
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
    if ( rlen==0 ) {
	fclose(binary);
return( NULL );
    }

    ret = IsResourceFork(binary,ftell(binary)+dlen+2,filename);

    fclose(binary);
return( ret );
}

static SplineFont *IsResourceInFile(char *filename) {
    FILE *f;
    char *spt, *pt;
    SplineFont *sf;
    char *temp=filename;

    if ( strchr(filename,'(')!=NULL ) {
	temp = copy(filename);
	pt = strchr(temp,'(');
	*pt = '\0';
    }
    f = fopen(temp,"r");
    if ( temp!=filename ) free(temp);
    if ( f==NULL )
return( NULL );
    spt = strrchr(filename,'/');
    if ( spt==NULL ) spt = filename;
    pt = strrchr(spt,'.');
    if ( pt!=NULL && (pt[1]=='b' || pt[1]=='B') && (pt[2]=='i' || pt[2]=='I') &&
	    (pt[3]=='n' || pt[3]=='N') && pt[4]=='\0' ) {
	if ( (sf = IsResourceInBinary(f,filename))) {
	    fclose(f);
return( sf );
	}
    } else if ( pt!=NULL && (pt[1]=='h' || pt[1]=='H') && (pt[2]=='q' || pt[2]=='Q') &&
	    (pt[3]=='x' || pt[3]=='X') && pt[4]=='\0' ) {
	if ( (sf = IsResourceInHex(f,filename))) {
	    fclose(f);
return( sf );
	}
    }

    sf = IsResourceFork(f,0,filename);
    fclose(f);
#if __Mac
    if ( sf==NULL )
	sf = HasResourceFork(filename);
#endif
return( sf );
}

static SplineFont *FindResourceFile(char *filename) {
    char *spt, *pt, *dpt;
    char buffer[1400];
    SplineFont *sf;

    if ( (sf = IsResourceInFile(filename)))
return( sf );

    /* Well, look in the resource fork directory (if it exists), the resource */
    /*  fork is placed there in a seperate file on non-Mac disks */
    strcpy(buffer,filename);
    spt = strrchr(buffer,'/');
    if ( spt==NULL ) { spt = buffer; pt = filename; }
    else { ++spt; pt = filename + (spt-buffer); }
    strcpy(spt,"resource.frk/");
    strcat(spt,pt);
    if ( (sf=IsResourceInFile(buffer)))
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
return( IsResourceInFile(buffer));
}

SplineFont *SFReadMacBinary(char *filename) {
    SplineFont *sf = FindResourceFile(filename);

    if ( sf==NULL )
	fprintf( stderr, "Couldn't find a font file named %s\n", filename );
    else if ( sf==(SplineFont *) (-1) ) {
	fprintf( stderr, "%s is a mac resource file but contains no postscript or truetype fonts\n", filename );
	sf = NULL;
    }
return( sf );
}
