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
#include "gimage.h"
#include <string.h>

struct sgiheader {
    short magic;		/* Magic (SGI identification) number	*/
    char format;		/* Storage format (RLE=1 or VERBATIM=0)	*/
    char bpc;			/* Bytes per pixel channel (1or2 bytes)	*/
    unsigned short dim;		/* Number of dimensions (1,2,3)		*/
    unsigned short width;	/* Width of image in pixels	X-size	*/
    unsigned short height;	/* Height of image in pixels	Y-size	*/
    unsigned short chans;	/* Number of channels (1,3,4)	Z-size	*/
    long pixmin;		/* Minimum pixel value	(darkest)	*/
    long pixmax;		/* Maximum pixel value	(brightest)	*/
    char dummy[4];		/* Ignored				*/
    char imagename[80];		/* Image name	(0..79chars + '\0')	*/
    long colormap;		/* Colormap ID	(0,1,2,3)		*/
    char pad[404];		/* Ignored	(total=512bytes)	*/
};

#define SGI_MAGIC	474
#define VERBATIM	0
#define RLE		1

static int getlong(FILE *fp, long *value) {
/* Get Big-Endian long (32bit int) value. Return 0 if okay, -1 if error	*/
    int ch1, ch2, ch3, ch4;

    if ( (ch1=fgetc(fp))<0 || (ch2=fgetc(fp))<0 || \
	 (ch3=fgetc(fp))<0 || (ch4=fgetc(fp))<0 ) {
	*value=0;
	return( -1 );
    }
    *value=(long)( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
    return( 0 );
}

static int getshort(FILE *fp) {
/* Get Big-Endian short 16bit value. Return value if okay, -1 if error	*/
    int ch1, ch2;

    if ( (ch1=fgetc(fp))<0 || (ch2=fgetc(fp))<0 )
	return( -1 );

    return( (ch1<<8) | ch2 );
}

static int getsgiheader(struct sgiheader *head,FILE *fp) {
/* Get Header info. Return 0 if read input file okay, -1 if read error	*/
    if ( (head->magic=getshort(fp))<0	|| head->magic!=SGI_MAGIC || \
	 (head->format=fgetc(fp))<0	|| \
	 (head->bpc=fgetc(fp))<0	|| \
	 (head->dim=getshort(fp))==(unsigned short)-1	||      \
	 (head->width=getshort(fp))==(unsigned short)-1	||      \
	 (head->height=getshort(fp))==(unsigned short)-1||      \
	 (head->chans=getshort(fp))==(unsigned short)-1	||      \
	 getlong(fp,&head->pixmin)	|| \
	 getlong(fp,&head->pixmax)	|| \
	 fread(head->dummy,sizeof(head->dummy),1,fp)<1 || \
	 fread(head->imagename,sizeof(head->imagename),1,fp)<1 || \
	 getlong(fp,&head->colormap)	|| \
	 fread(head->pad,sizeof(head->pad),1,fp)<1 )
    return( -1 );

    /* Check if header information okay (for us to use here ) */
    if ( (head->format!=VERBATIM && head->format!=RLE) || \
	 (head->bpc!=1 && head->bpc!=2) || \
	 head->dim<1 || head->dim>3 || \
	 head->pixmax>65535 || (head->pixmax>255 && head->bpc==1) || \
	 (head->chans!=1 && head->chans!=3 && head->chans!=4) || \
	 head->pixmax<0 || head->pixmin<0 || head->pixmin>=head->pixmax || \
	 head->colormap!=0 )
    return( -1 );

    return( 0 );
}

static long scalecolor(struct sgiheader *header,long value) {
    return( (value-header->pixmin)*255L/(header->pixmax-header->pixmin) );
}

static int readlongtab(FILE *fp,unsigned long *tab,long tablen) {
/* RLE table info, exit=0 if okay, return -1 if error */
    long i;

    for ( i=0; i<tablen; ++i )
	if ( getlong(fp,(long *)&tab[i]) )
	    return( -1 ); /* had a read error */

    return( 0 ); /* read everything okay */
}

static int find_scanline(FILE *fp,struct sgiheader *header,int cur,
	unsigned long *starttab,unsigned char **ptrtab) {
/* Find and expand a scanline. Return 0 if okay, else -ve if error */
    int ch = 0,i,cnt;
    long val;
    unsigned char *pt;

    for ( i=0; i<cur; ++i )
	if ( starttab[i]==starttab[cur] ) {
	    ptrtab[cur] = ptrtab[i];
	    return( 0 );
	}
    if ( (pt=ptrtab[cur]=(unsigned char *) malloc(header->width))==NULL ) {
	NoMoreMemMessage();
	return( -1 );
    }
    if ( fseek(fp,starttab[cur],0)!= 0 ) return( -2 );
    while ( header->bpc==1 ) {
	if ( (ch=fgetc(fp))<0 ) return( -2 );
	if ( (cnt=(ch&0x7f))==0 ) return( 0 );
	if ( ch&0x80 ) {
	    while ( --cnt>=0 ) {
		if ( (ch=fgetc(fp))<0 ) return( -2 );
		*pt++ = scalecolor(header,ch);
	    }
	} else {
	    if ( (ch=fgetc(fp))<0 ) return( -2 );
	    ch = scalecolor(header,ch);
	    while ( --cnt>=0 )
		*pt++ = ch;
	}
    }
    while ( header->bpc!=1 ) {
	if ( getlong(fp,&val) ) return( -2 );
	if ( (cnt=(ch&0x7f))==0 ) return( 0 );
	if ( ch&0x80 ) {
	    while ( --cnt>=0 ) {
		if ( getlong(fp,&val) ) return( -2 );
		*pt++ = scalecolor(header,val);
	    }
	} else {
	    if ( getlong(fp,&val) ) return( -2 );
	    val = scalecolor(header,val);
	    while ( --cnt>=0 )
		*pt++ = val;
	}
    }
    return( -2 );
}

static void freeptrtab(unsigned char **ptrtab,long tot) {
    long i,j;

    if ( ptrtab!=NULL )
	for ( i=0; i<tot; ++i )
	    if ( ptrtab[i]!=NULL ) {
		for ( j=i+1; j<tot; ++j )
		    if ( ptrtab[j]==ptrtab[i] )
			ptrtab[j] = NULL;
		free(ptrtab[i]);
	    }
}

GImage *GImageReadRgb(char *filename) {
    FILE *fp;			/* source file */
    struct sgiheader header;
    int i,j,k;
    unsigned char *pt, *end;
    unsigned char *r=NULL,*g=NULL,*b=NULL,*a=NULL;	/* Colors */
    unsigned long *starttab=NULL /*, *lengthtab=NULL */;
    unsigned char **ptrtab=NULL;
    long tablen=0;
    unsigned long *ipt, *iend;
    GImage *ret = NULL;
    struct _GImage *base;

    if ( (fp=fopen(filename,"rb"))==NULL ) {
	fprintf(stderr,"Can't open \"%s\"\n", filename);
	return( NULL );
    }

    /* Check, and Get, Header information */
    if ( getsgiheader(&header,fp) )
	goto errorGImageReadRgbFile;

    /* Create memory to hold image, exit with NULL if not enough memory */
    if ( (ret=GImageCreate(header.dim==3?it_true:it_index,header.width,header.height))==NULL ) {
	fclose(fp);
	return( NULL );
    }
    base = ret->u.image;

    if ( header.format==RLE ) {
	/* Working with RLE image data*/

	/* First, get offset table info */
	tablen = header.height*header.chans;
	if ( (starttab=(unsigned long *)calloc(1,tablen*sizeof(long)))==NULL || \
	   /*(lengthtab=(unsigned long *)calloc(1,tablen*sizeof(long)))==NULL || \ */
	     (ptrtab=(unsigned char **)calloc(1,tablen*sizeof(unsigned char *)))==NULL ) {
	    NoMoreMemMessage();
	    goto errorGImageReadRgbMem;
	}
	if ( readlongtab(fp,starttab,tablen) )
	    /* || readlongtab(fp,lengthtab,tablen) */
	    goto errorGImageReadRgbFile;

	/* Next, get image data */
	for ( i=0; i<tablen; ++i )
	    if ( (k=find_scanline(fp,&header,i,starttab,ptrtab)) ) {
		if ( k==-1 ) goto errorGImageReadRgbMem; else goto errorGImageReadRgbFile;
	    }

	/* Then, build image */
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
	free(ptrtab); free(starttab); /*free(lengthtab);*/
    } else {
	/* working with Verbatim image data*/
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
		for ( end=pt+header.width; pt<end; ) {
		    if ( (k=getshort(fp))<0 ) goto errorGImageReadRgbFile;
		    *pt++ = (k*255L)/header.pixmax;
		}
	    }
	} else {
	    /* import RGB=3 or RGBA=4 Verbatim images */
	    unsigned char *rpt, *gpt, *bpt;
	    if ( (r=(unsigned char *) malloc(header.width*sizeof(unsigned char)))==NULL || \
		 (g=(unsigned char *) malloc(header.width*sizeof(unsigned char)))==NULL || \
		 (b=(unsigned char *) malloc(header.width*sizeof(unsigned char)))==NULL || \
		 (header.chans==4 && \
		 (a=(unsigned char *) malloc(header.width*sizeof(unsigned char)))==NULL) ) {
		NoMoreMemMessage();
		goto errorGImageReadRgbMem;
	    }
	    if ( header.bpc==1 ) {
		for ( i=0; i<header.height; ++i ) {
		    if ( (fread(r,header.width,1,fp))<1 || \
			 (fread(g,header.width,1,fp))<1 || \
			 (fread(b,header.width,1,fp))<1 || \
			 (header.chans==4 && (fread(a,header.width,1,fp))<1) )
			goto errorGImageReadRgbFile;
		ipt = (unsigned long *) (base->data + (header.height-1-i)*base->bytes_per_line);
		rpt = r; gpt = g; bpt = b;
		for ( iend=ipt+header.width; ipt<iend; )
		    *ipt++ = COLOR_CREATE(*rpt++*255L/header.pixmax,
			    *gpt++*255L/header.pixmax,*bpt++*255L/header.pixmax);
		}
	    } else {
	    for ( i=0; i<header.height; ++i ) {
		for ( j=0; j<header.width; ++j ) {
		    if ( (k=getshort(fp))<0 ) goto errorGImageReadRgbFile;
		    r[j] = k*255L/header.pixmax;
		}
		for ( j=0; j<header.width; ++j ) {
		    if ( (k=getshort(fp))<0 ) goto errorGImageReadRgbFile;
		    g[j] = k*255L/header.pixmax;
		}
		for ( j=0; j<header.width; ++j ) {
		    if ( (k=getshort(fp))<0 ) goto errorGImageReadRgbFile;
		    b[j] = k*255L/header.pixmax;
		}
		if ( header.chans==4 ) {
		    fread(a,header.width,1,fp);
		    fread(a,header.width,1,fp);
		}
		ipt = (unsigned long *) (base->data + (header.height-1-i)*base->bytes_per_line);
		rpt = r; gpt = g; bpt = b;
		for ( iend=ipt+header.width; ipt<iend; )
		    *ipt++ = COLOR_CREATE(*rpt++,*gpt++,*bpt++);
	    }
	    }
	    free(r); free(g); free(b); free(a);
	}
    }
    fclose(fp);
    return( ret );

errorGImageReadRgbFile:
    fprintf(stderr,"Bad input file \"%s\"\n",filename );
errorGImageReadRgbMem:
    freeptrtab(ptrtab,tablen);
    free(ptrtab); free(starttab); /*free(lengthtab);*/
    free(r); free(g); free(b); free(a);
    GImageDestroy(ret);
    fclose(fp);
    return( NULL );
}
