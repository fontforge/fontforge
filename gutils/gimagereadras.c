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
	 getlong(fp,&head->Width)	 || \
	 getlong(fp,&head->Height)	 || \
	 getlong(fp,&head->Depth)	 || \
	 getlong(fp,&head->Length)	 || \
	 getlong(fp,&head->Type)	 || \
	 getlong(fp,&head->ColorMapType) || \
	 getlong(fp,&head->ColorMapLength) )
	return( -1 );

    /* Check if header information okay (only try Big-Endian for now).	*/
    if ( head->MagicNumber!=SUN_RAS_MAGIC ||
	 head->Type<0 || head->Type>TypeRGB ||
	 (head->ColorMapType!=ClutNone && head->ColorMapType!=ClutRGB) ||
	 (head->Depth!=1 && head->Depth!=8 && head->Depth!=24 && head->Depth!=32) ||
	 (head->Depth>=24 && head->ColorMapType!=ClutNone) ||
	 head->ColorMapLength>3*256 )
	return( -1 );

    return( 0 );
}

static GImage *ReadRasBitmap(GImage *ret,int width, int height, FILE *fp ) {
    struct _GImage *base = ret->u.image;
    int i,j,len;
    unsigned char *pt, *buf;

    len = ((width+15)/16)*2;
    if ( (buf=(unsigned char *) malloc(len*sizeof(unsigned char)))==NULL ) {
	NoMoreMemMessage();
	return( NULL);
    }

    for ( i=0; i<height; ++i ) {
	if ( fread(buf,len,1,fp)==EOF ) {
	    GImageDestroy(ret);
return( NULL);
	}
	pt = (unsigned char *) (base->data + i*base->bytes_per_line);
	for ( j=0 ; j<width; ++j )
	    if ( buf[j>>3]&(1<<(j&7)) )
		*pt++ = 1;
	    else
		*pt++ = 0;
    }
    gfree(buf);

return ret;
}

static GImage *ReadRas8Bit(GImage *ret,int width, int height, FILE *fp ) {
    struct _GImage *base = ret->u.image;
    int i;

    for ( i=0; i<height; ++i ) {
	if ( fread((base->data + i*base->bytes_per_line),width,1,fp)<0 ) {
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
return ret;
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
    struct _GImage *base = ret->u.image;
    int i, tot, lineend, modwid;
    int cnt=0,val=0,ch;
    unsigned char *pt=NULL;

    if ((modwid = ((width+1)&~1))==0 )
	modwid = 2;;
    tot = modwid*height; lineend = 0;
    for ( i=0; i<tot; ++i ) {
	if ( i==lineend ) {
	    pt = (unsigned char *) (base->data + (lineend/modwid)*base->bytes_per_line);
	    lineend += modwid;
	}
	if ( cnt==0 ) {
	    ch = fgetc(fp);
	    if ( ch==0x80 ) {
		cnt = fgetc(fp);
		if ( cnt!=0 ) {	/* if ==0 then ch is 0x80 and correct */
		    ch = val = fgetc(fp);
		    --cnt;
		}
	    }
	} else {
	    ch = val;
	    --cnt;
	}
	if ( width&1 && i==lineend-1 )
	    /* Skip the pad byte */;
	else
	    *pt++ = ch;
    }
return ret;
}

GImage *GImageReadRas(char *filename) {
    FILE *fp;			/* source file */
    struct _SunRaster header;
    int i;
    GImage *ret;
    struct _GImage *base;

    if ( (fp=fopen(filename,"rb"))==NULL ) {
	fprintf(stderr,"Can't open \"%s\"\n", filename);
	return( NULL );
    }

    if ( getrasheader(&header,fp) ) {
	fprintf(stderr,"Bad input file \"%s\"\n",filename );
	fclose(fp);
	return( NULL );
    }

    /* Create memory to hold image, exit with NULL if not enough memory */
    if ( (ret=GImageCreate(header.Depth==24?it_true:it_index,header.Width,header.Height))==NULL ) {
	fclose(fp);
	return( NULL );
    }

    base = ret->u.image;
    if ( header.ColorMapLength!=0 && base->clut!=NULL ) {
	char clutb[3*256]; int n;
	fread(clutb,header.ColorMapLength,1,fp);
	n = header.ColorMapLength/3;
	base->clut->clut_len = n;
	for ( i=0; i<n; ++i )
	    base->clut->clut[i] = COLOR_CREATE(clutb[i],clutb[i+n],clutb[i+2*n]);
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
    } else if ( header.Type==TypeRGB ) {
	/* Don't bother with most of the rle formats */
	if ( header.Depth==8 )
	    ret = ReadRle8Bit(ret,header.Width,header.Height,fp);
	else {
	    GImageDestroy(ret);
	    ret = NULL;
	}
    }
    fclose(fp);
return( ret );
}
