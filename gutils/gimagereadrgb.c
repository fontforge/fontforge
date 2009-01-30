/* Copyright (C) 2000-2009 by George Williams */
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
#include "gimage.h"
#include <string.h>

struct sgiheader {
    short magic;
    char format;
    char bpc;
    unsigned short dim;
    unsigned short width;
    unsigned short height;
    unsigned short chans;
    long pixmin;
    long pixmax;
    char dummy[4];
    char imagename[80];
    long colormap;
    char pad[404];
};

#define SGI_MAGIC	474
#define VERBATIM	0
#define RLE		1

static long getlong(FILE *fp) {
    int ch1, ch2, ch3, ch4;

    ch1 = fgetc(fp); ch2 = fgetc(fp); ch3=fgetc(fp); ch4=fgetc(fp);
return( (ch1<<24) | (ch2<<16) | (ch3<<8) | ch4 );
}

static int getshort(FILE *fp) {
    int ch1, ch2;

    ch1 = fgetc(fp); ch2 = fgetc(fp);
return( (ch1<<8) | ch2);
}

static void getsgiheader(struct sgiheader *head, FILE *fp) {
    head->magic = getshort(fp);
    head->format = getc(fp);
    head->bpc = getc(fp);
    head->dim = getshort(fp);
    head->width = getshort(fp);
    head->height = getshort(fp);
    head->chans = getshort(fp);
    head->pixmin = getlong(fp);
    head->pixmax = getlong(fp);
    fread(head->dummy,sizeof(head->dummy),1,fp);
    fread(head->imagename,sizeof(head->imagename),1,fp);
    head->colormap = getlong(fp);
    fread(head->pad,sizeof(head->pad),1,fp);
}

static void readlongtab(FILE *fp,unsigned long *tab,int tablen) {
    int i;

    for ( i=0; i<tablen; ++i )
	tab[i] = getlong(fp);
}

static void find_scanline(FILE *fp,struct sgiheader *header,int cur,
	unsigned long *starttab,unsigned char **ptrtab) {
    extern int fgetc(FILE *);
    int (*getthingamy)(FILE *) = header->bpc==1?fgetc:getshort;
    int ch,i,cnt,val;
    unsigned char *pt;

    for ( i=0; i<cur; ++i )
	if ( starttab[i]==starttab[cur] ) {
	    ptrtab[cur] = ptrtab[i];
return;
	}
    pt = ptrtab[cur] = galloc(header->width);
    fseek(fp,starttab[cur],0);
    while (1) {
	ch = getthingamy(fp);
	if (( cnt = (ch&0x7f))==0 )
return;
	if ( ch&0x80 ) {
	    while ( --cnt>=0 )
		*pt++ = getthingamy(fp)*255L/header->pixmax;
	} else {
	    val = getthingamy(fp)*255L/header->pixmax;
	    while ( --cnt>= 0 )
		*pt++ = val;
	}
    }
}

static void freeptrtab(unsigned char **ptrtab,int tot) {
    int i,j;

    for ( i=0; i<tot; ++i )
	if ( ptrtab[i]!=NULL ) {
	    for ( j=i+1; j<tot; ++j )
		if ( ptrtab[j]==ptrtab[i] )
		    ptrtab[j] = NULL;
	    gfree(ptrtab[i]);
	}
}

GImage *GImageReadRgb(char *filename) {
    FILE *fp = fopen(filename,"rb");
    struct sgiheader header;
    int j,i;
    unsigned char *pt, *end;
    unsigned long *ipt, *iend;
    GImage *ret;
    struct _GImage *base;

    if ( fp==NULL )
return( NULL );
    getsgiheader(&header,fp);
    if ( header.magic!=SGI_MAGIC ||
	    (header.format!=VERBATIM && header.format!=RLE) ||
	    (header.bpc!=1 && header.bpc!=2) ||
	    header.dim<1 || header.dim>3 ||
	    header.pixmax>65535 || (header.pixmax>255 && header.bpc==1) ||
	    (header.chans!=1 && header.chans!=3 && header.chans!=4) ||
	    header.pixmax<0 || header.pixmin<0 || header.pixmin>header.pixmax ||
	    header.colormap != 0 ) {
	fclose(fp);
return( NULL );
    }

    ret = GImageCreate(header.dim==3?it_true:it_index,header.width,header.height);
    base = ret->u.image;

    if ( header.format==RLE ) {
	unsigned long *starttab/*, *lengthtab*/;
	unsigned char **ptrtab;
	long tablen;

	tablen = header.height*header.chans;
	starttab = (unsigned long *)galloc(tablen*sizeof(long));
	/*lengthtab = (unsigned long *)galloc(tablen*sizeof(long));*/
	ptrtab = (unsigned char **)galloc(tablen*sizeof(unsigned char *));
	readlongtab(fp,starttab,tablen);
	/*readlongtab(fp,lengthtab,tablen);*/
	for ( i=0; i<tablen; ++i )
	    find_scanline(fp,&header,i,starttab,ptrtab);
	if ( header.chans==1 ) {
	    for ( i=0; i<header.height; ++i )
		memcpy(base->data + (header.height-1-i)*base->bytes_per_line,ptrtab[i],header.width);
	} else {
	    unsigned long *ipt;
	    for ( i=0; i<header.height; ++i ) {
		ipt = (unsigned long *) (base->data + (header.height-1-i)*base->bytes_per_line);
		for ( j=0; j<header.width; ++j )
		    *ipt++ = COLOR_CREATE(ptrtab[i][j],
			    ptrtab[i+header.height][j], ptrtab[i+2*header.height][j]);
	    }
	}
	freeptrtab(ptrtab,tablen);
	gfree(ptrtab); gfree(starttab); /*gfree(lengthtab);*/
    } else {
	if ( header.chans==1 && header.bpc==1 ) {
	    for ( i=0; i<header.height; ++i ) {
		fread(base->data + (header.height-1-i)*base->bytes_per_line,header.width,1,fp);
		if ( header.pixmax!=255 ) {
		    pt = (unsigned char *) (base->data+(header.height-1-i)*base->bytes_per_line);
		    for ( end=pt+header.width; pt<end; ++pt )
			*pt = (*pt*255)/header.pixmax;
		}
	    }
	} else if ( header.chans==1 ) {
	    for ( i=0; i<header.height; ++i ) {
		pt = (unsigned char *) (base->data + (header.height-1-i)*base->bytes_per_line);
		for ( end=pt+header.width; pt<end; )
		    *pt++ = (getshort(fp)*255L)/header.pixmax;
	    }
	} else if ( header.bpc==1 ) {
	    unsigned char *r,*g,*b, *a=NULL;
	    unsigned char *rpt, *gpt, *bpt;
	    r = galloc(header.width); g=galloc(header.width); b=galloc(header.width);
	    if ( header.chans==4 )
		a = galloc(header.width);
	    for ( i=0; i<header.height; ++i ) {
		fread(r,header.width,1,fp);
		fread(g,header.width,1,fp);
		fread(b,header.width,1,fp);
		if ( header.chans==4 )
		    fread(a,header.width,1,fp);
		ipt = (unsigned long *) (base->data + (header.height-1-i)*base->bytes_per_line);
		rpt = r; gpt = g; bpt = b;
		for ( iend=ipt+header.width; ipt<iend; )
		    *ipt++ = COLOR_CREATE(*rpt++*255L/header.pixmax,
			    *gpt++*255L/header.pixmax,*bpt++*255L/header.pixmax);
	    }
	    gfree(r); gfree(g); gfree(b); gfree(a);
	} else {
	    unsigned char *r,*g,*b, *a=NULL;
	    unsigned char *rpt, *gpt, *bpt;
	    r = galloc(header.width); g=galloc(header.width); b=galloc(header.width);
	    if ( header.chans==4 )
		a = galloc(header.width);
	    for ( i=0; i<header.height; ++i ) {
		for ( j=0; j<header.width; ++j )
		    r[j] = getshort(fp)*255L/header.pixmax;
		for ( j=0; j<header.width; ++j )
		    g[j] = getshort(fp)*255L/header.pixmax;
		for ( j=0; j<header.width; ++j )
		    b[j] = getshort(fp)*255L/header.pixmax;
		if ( header.chans==4 ) {
		    fread(a,header.width,1,fp);
		    fread(a,header.width,1,fp);
		}
		ipt = (unsigned long *) (base->data + (header.height-1-i)*base->bytes_per_line);
		rpt = r; gpt = g; bpt = b;
		for ( iend=ipt+header.width; ipt<iend; )
		    *ipt++ = COLOR_CREATE(*rpt++,*gpt++,*bpt++);
	    }
	    gfree(r); gfree(g); gfree(b); gfree(a);
	}
    }
	    
return( ret );
}
