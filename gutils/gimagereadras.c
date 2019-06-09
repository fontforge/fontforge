/* Copyright (C) 2000-2012 by George Williams */
/* 2013jan30..feb5, additional fixes and error checks done, Jose Da Silva */
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

#include "gimage.h"

typedef struct _SunRaster {
  long MagicNumber;	/* Magic (identification) number */
  long Width;		/* Width of image in pixels */
  long Height;		/* Height of image in pixels */
  long Depth;		/* Number of bits per pixel */
  long Length;		/* Size of image data in bytes */
  long Type;		/* Type of raster file */
  long ColorMapType;	/* Type of color map */
  long ColorMapLength;	/* Size of the color map in bytes */
} SUNRASTER;

#define SUN_RAS_MAGIC	0x59a66a95
#define LITTLE_ENDIAN_MAGIC	0x956aa659	/* if we read this value, must byte swap */

enum types { TypeOld, TypeStandard, TypeByteEncoded, TypeRGB, TypeTIFF, TypeIFF };
enum cluts { ClutNone, ClutRGB, ClutRaw };

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

static int getrasheader(SUNRASTER *head, FILE *fp) {
/* Get Header info. Return 0 if read input file okay, -1 if read error	*/
    if ( getlong(fp,&head->MagicNumber)	 || \
	 (head->MagicNumber!=SUN_RAS_MAGIC && head->MagicNumber!=LITTLE_ENDIAN_MAGIC) || \
	 getlong(fp,&head->Width)	 || \
	 getlong(fp,&head->Height)	 || \
	 getlong(fp,&head->Depth)	 || \
	 getlong(fp,&head->Length)	 || \
	 getlong(fp,&head->Type)	 || \
	 getlong(fp,&head->ColorMapType) || \
	 getlong(fp,&head->ColorMapLength) )
	return( -1 );

    /* Check if header information okay (only try Big-Endian for now).	*/
    if ( head->MagicNumber!=SUN_RAS_MAGIC || \
	 head->Type<0 || head->Type>TypeRGB || \
	 (head->ColorMapType!=ClutNone && head->ColorMapType!=ClutRGB) || \
	 (head->Depth!=1 && head->Depth!=8 && head->Depth!=24 && head->Depth!=32) || \
	 (head->Depth>=24 && head->ColorMapType!=ClutNone) || \
	 head->ColorMapLength>3*256 )
	return( -1 );

    return( 0 );
}

static GImage *ReadRasBitmap(GImage *ret,int width, int height, FILE *fp ) {
    struct _GImage *base = ret->u.image;
    int i,j,len;
    unsigned char *pt, *buf;

    len = ((width+15)/16)*2;	/* pad out to 16 bits */
    if ( (buf=(unsigned char *) malloc(len*sizeof(unsigned char)))==NULL ) {
	NoMoreMemMessage();
	GImageDestroy(ret);
	return( NULL );
    }

    for ( i=0; i<height; ++i ) {
	if ( fread(buf,len,1,fp)<1 ) {
	    free(buf);
	    GImageDestroy(ret);
	    return( NULL );
	}
	pt = (unsigned char *) (base->data + i*base->bytes_per_line);
	for ( j=0; j<(width+7)>>3; ++j )
	    pt[j]=256-buf[j];
    }
    free(buf);
    return( ret );
}

static GImage *ReadRas8Bit(GImage *ret,int width, int height, FILE *fp ) {
    struct _GImage *base = ret->u.image;
    int i;

    for ( i=0; i<height; ++i ) {
	if ( fread((base->data + i*base->bytes_per_line),width,1,fp)<1 ) {
	    goto errorReadRas8Bit;
	}
	if ( width&1 )		/* pad out to 16 bits */
	    if ( fgetc(fp)<0 )
		goto errorReadRas8Bit;
    }
    return( ret );

errorReadRas8Bit:
    GImageDestroy(ret);
    return( NULL );
}

static GImage *ReadRas24Bit(GImage *ret,int width, int height, FILE *fp ) {
    struct _GImage *base = ret->u.image;
    int ch1,ch2,ch3=0;
    int i;
    long *ipt,*end;

    for ( i=0; i<height; ++i ) {
	for ( ipt = (long *) (base->data + i*base->bytes_per_line), end = ipt+width; ipt<end; ) {
	    if ( (ch1=fgetc(fp))<0 || (ch2=fgetc(fp))<0 || (ch3=fgetc(fp))<0 )
		goto errorReadRas24Bit;
	    *ipt++ = COLOR_CREATE(ch3,ch2,ch1);
	}
	if ( width&1 )		/* pad out to 16 bits */
	    if ( fgetc(fp)<0 )
		goto errorReadRas24Bit;
    }
    return( ret );

errorReadRas24Bit:
    GImageDestroy(ret);
    return( NULL );
}

static GImage *ReadRas32Bit(GImage *ret,int width, int height, FILE *fp ) {
    struct _GImage *base = ret->u.image;
    int ch1,ch2,ch3=0;
    int i;
    long *ipt, *end;

    for ( i=0; i<height; ++i )
	for ( ipt = (long *) (base->data + i*base->bytes_per_line), end = ipt+width; ipt<end; ) {
	    fgetc(fp);	/* pad byte */
	    ch1 = fgetc(fp); ch2 = fgetc(fp); ch3 = fgetc(fp);
	    *ipt++ = COLOR_CREATE(ch3,ch2,ch1);
	}
    if ( ch3==EOF ) {
	GImageDestroy(ret);
	ret = NULL;
    }
return ret;
}

static GImage *ReadRas24RBit(GImage *ret,int width, int height, FILE *fp ) {
    struct _GImage *base = ret->u.image;
    int ch1,ch2,ch3=0;
    int i;
    long *ipt,*end;

    for ( i=0; i<height; ++i ) {
	for ( ipt = (long *) (base->data + i*base->bytes_per_line), end = ipt+width; ipt<end; ) {
	    if ( (ch1=fgetc(fp))<0 || (ch2=fgetc(fp))<0 || (ch3=fgetc(fp))<0 )
		goto errorReadRas24RBit;
	    *ipt++ = COLOR_CREATE(ch1,ch2,ch3);
	}
	if ( width&1 )		/* pad out to 16 bits */
	    if ( fgetc(fp)<0 )
		goto errorReadRas24RBit;
    }
    return( ret );

errorReadRas24RBit:
    GImageDestroy(ret);
    return( NULL );
}

static GImage *ReadRas32RBit(GImage *ret,int width, int height, FILE *fp ) {
    struct _GImage *base = ret->u.image;
    int ch1,ch2,ch3=0;
    int i;
    long *ipt, *end;

    for ( i=0; i<height; ++i )
	for ( ipt = (long *) (base->data + i*base->bytes_per_line), end = ipt+width; ipt<end; ) {
	    fgetc(fp);	/* pad byte */
	    ch1 = fgetc(fp); ch2 = fgetc(fp); ch3 = fgetc(fp);
	    *ipt++ = COLOR_CREATE(ch1,ch2,ch3);
	}
    if ( ch3==EOF ) {
	GImageDestroy(ret);
	ret = NULL;
    }
return ret;
}

static GImage *ReadRle8Bit(GImage *ret,int width, int height, FILE *fp ) {
/* TODO: Make this an input filter that goes in front of other routines	*/
/* above so that in can be re-used by the different converters above.	*/
    struct _GImage *base = ret->u.image;
    int x,y,cnt,val = 0;
    unsigned char *pt = NULL;

    x=0; y=0; cnt=0;
    while ( 1 ) {
	while ( cnt && x ) {
	    --cnt; if ( --x || (width&1)-1 ) * pt++ = val;
	}
	if ( x==0 ) {
	    pt = (unsigned char *) (base->data + y*base->bytes_per_line);
	    if ( ++y>height )
		return( ret );
	    x=((width+1)>>1)<<1;
	}
	if ( cnt==0 ) {
	    if ( (val=fgetc(fp))<0 ) goto errorReadRle8Bit;
	    cnt++;
	    if ( val==0x80 ) {
		if ( (cnt=fgetc(fp))<0 ) goto errorReadRle8Bit;
		if ( cnt++!=0 )
		    /* prepare to go insert 'val', 'cnt' times */
		    if ( (val=fgetc(fp))<0 ) goto errorReadRle8Bit;
	    }
	}
    }

errorReadRle8Bit:
    GImageDestroy(ret);
    return( NULL );
}

GImage *GImageReadRas(char *filename) {
/* Import a *.ras image (or *.im{1,8,24,32}), else return NULL if error	*/
    FILE *fp;			/* source file */
    struct _SunRaster header;
    GImage *ret = NULL;
    struct _GImage *base;

    if ( (fp=fopen(filename,"rb"))==NULL ) {
	fprintf(stderr,"Can't open \"%s\"\n", filename);
	return( NULL );
    }

    if ( getrasheader(&header,fp) )
	goto errorGImageReadRas;

    /* Create memory to hold image, exit with NULL if not enough memory */
    if ( (header.Depth==1 && \
	  (ret=GImageCreate(it_bitmap,header.Width,header.Height))==NULL) || \
	  (header.Depth!=1 && \
	  (ret=GImageCreate(header.Depth==24?it_true:it_index,header.Width,header.Height))==NULL) ) {
	fclose(fp);
	return( NULL );
    }

    /* Convert *.ras ColorMap to one that FF can use */
    base = ret->u.image;
    if ( header.ColorMapLength!=0 && base->clut!=NULL ) {
	unsigned char clutb[3*256]; int i,n;
	if ( fread(clutb,header.ColorMapLength,1,fp)<1 )
	    goto errorGImageReadRas;
//	if ( header.ColorMapType==ClutRaw ) {
//	    n = header.ColorMapLength;
//	    base->clut->clut_len = n;
//	    for ( i=0; i<n; ++i )
//		base->clut->clut[i] = clutb[i];
//	} else {
	n = header.ColorMapLength/3;
	base->clut->clut_len = n;
	for ( i=0; i<n; ++i )
	    base->clut->clut[i] = COLOR_CREATE(clutb[i],clutb[i+n],clutb[i+2*n]);
//	}
    }

    if ( header.Type==TypeOld || header.Type==TypeStandard ) {	/* Synonymous */
	    if ( header.Depth==1 )
	        ret = ReadRasBitmap(ret,header.Width,header.Height,fp);
	    else if ( header.Depth==8 )
	        ret = ReadRas8Bit(ret,header.Width,header.Height,fp);
	    else if ( header.Depth==24 )
	        ret = ReadRas24Bit(ret,header.Width,header.Height,fp);
	    else
	        ret = ReadRas32Bit(ret,header.Width,header.Height,fp);
    } else if ( header.Type==TypeRGB ) {
	    /* I think this type is the same as standard except rgb not bgr */
	    if ( header.Depth==1 )
	        ret = ReadRasBitmap(ret,header.Width,header.Height,fp);
	    else if ( header.Depth==8 )
	        ret = ReadRas8Bit(ret,header.Width,header.Height,fp);
	    else if ( header.Depth==24 )
	        ret = ReadRas24RBit(ret,header.Width,header.Height,fp);
	    else
	        ret = ReadRas32RBit(ret,header.Width,header.Height,fp);
    } else if ( header.Type==TypeByteEncoded ) {
	    if ( header.Depth==8 )
	        ret = ReadRle8Bit(ret,header.Width,header.Height,fp);
	    else {
	        /* Don't bother with most rle formats */
	        /* TODO: if someone wants to do this - accept more formats */
	        fprintf(stderr,"Unsupported input file type\n");
	        goto errorGImageReadRas;
	    }
    } else {
	    /* Don't bother with other formats */
	    /* TODO: if someone wants to do this - accept more formats */
        fprintf(stderr,"Unsupported input file type\n");
	    goto errorGImageReadRas;
	}
    if ( ret!=NULL ) {
	    /* All okay if reached here, return converted image */
	    fclose(fp);
	    return( ret );
    }

errorGImageReadRas:
    fprintf(stderr,"Bad input file \"%s\"\n",filename );
    GImageDestroy(ret);
    fclose(fp);
    return( NULL );
}
