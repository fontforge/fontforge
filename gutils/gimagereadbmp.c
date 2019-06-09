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

#include "gimage.h"
#include "gimagebmpP.h"
GImage *_GImage_Create(enum image_type type, int32 width, int32 height);

static int getshort(FILE *fp) {
/* Get Little-Endian short 16bit value. Return value if okay, -1 if error */
    int ch1, ch2;

    if ( (ch1=fgetc(fp))<0 || (ch2=fgetc(fp))<0 )
	return( -1 );

    return( (ch2<<8) | ch1 );
}

static long getlong(FILE *fp, long *value) {
/* Get Little-Endian long 32bit int value. Return 0 if okay, -1 if error. */
    int ch1, ch2, ch3, ch4;

    if ( (ch1=fgetc(fp))<0 || (ch2=fgetc(fp))<0 || \
	 (ch3=fgetc(fp))<0 || (ch4=fgetc(fp))<0 ) {
	*value=0;
	return( -1 );
    }
    *value=(long)( (ch4<<24)|(ch3<<16)|(ch2<<8)|ch1 );
    return( 0 );
}

static int bitshift(unsigned long mask) {
    int off=0, len=0, bit;

    if ( mask==0 )
return( 0 );
    for ( off=0; !(mask&1); mask>>=1, ++off );
    for ( len=0, bit=1; (mask&bit) && len<32; ++len, bit<<=1 );
return( off+(8-len) );
}

static int fillbmpheader(FILE *fp,struct bmpheader *head) {
/* Get BMPheader info. Return 0 if read the header okay, -1 if read error */
    int i;
    long temp;

    if ( fgetc(fp)!='B' || getc(fp)!='M' ||	/* Bad format */ \
	 getlong(fp,&head->size)	|| \
	 (head->mbz1=getshort(fp))<0	|| \
	 (head->mbz2=getshort(fp))<0	|| \
	 getlong(fp,&head->offset)	|| \
	 getlong(fp,&head->headersize)	)
	return( -1 );

    if ( head->headersize==12 ) {	/* Windows 2.0 format, also OS/2 */
	if ( (head->width=getshort(fp))<0	|| \
	     (head->height=getshort(fp))<0	|| \
	     (head->planes=getshort(fp))<0	|| \
	     (head->bitsperpixel=getshort(fp))<0 )
	    return( -1 );
	//head->colorsused=0;
	//head->compression=0;
    } else {
	if ( getlong(fp,&head->width)		|| \
	     getlong(fp,&head->height)		|| \
	     (head->planes=getshort(fp))<0	|| \
	     (head->bitsperpixel=getshort(fp))<0 || \
	     getlong(fp,&head->compression)	|| \
	     getlong(fp,&head->imagesize)	|| \
	     getlong(fp,&head->ignore1)		|| \
	     getlong(fp,&head->ignore2)		|| \
	     getlong(fp,&head->colorsused)	|| \
	     getlong(fp,&head->colorsimportant) )
	    return( -1 );
    }
    if ( head->height<0 )
	head->height = -head->height;
    else
	head->invert = true;

    if ( head->bitsperpixel!=1 && head->bitsperpixel!=4 && head->bitsperpixel!=8 &&
	    head->bitsperpixel!=16 && head->bitsperpixel!=24 && head->bitsperpixel!=32 )
	return( -1 );

    if ( head->compression==3 && ( head->bitsperpixel==16 || head->bitsperpixel==32 ) )
	/* Good */;
    else if ( head->compression==0 && ( head->bitsperpixel<=8 || head->bitsperpixel==24 || head->bitsperpixel==32 ) )
	/* Good */;
    else if ( head->compression==1 && head->bitsperpixel==8 )
	/* Good */;
    else if ( head->compression==2 && head->bitsperpixel==4 )
	/* Good */;
    else
	return( -1 );

    if ( head->colorsused==0 )
	head->colorsused = 1<<head->bitsperpixel;
    if ( head->bitsperpixel>=16 )
	head->colorsused = 0;
    if ( head->colorsused>(1<<head->bitsperpixel) )
	return( -1 );

    for ( i=0; i<head->colorsused; ++i ) {
	int b,g,r;
	if ( (b=fgetc(fp))<0 || (g=fgetc(fp))<0 || (r=fgetc(fp))<0 )
	    return( -1 );
	head->clut[i]=COLOR_CREATE(r,g,b);
	if ( head->headersize!=12 && fgetc(fp)<0 )
	    return( -1 );
    }
    if ( head->compression==3 || head->headersize==108 ) {
	if ( getlong(fp,&head->red_mask)	|| \
	     getlong(fp,&head->green_mask)	|| \
	     getlong(fp,&head->blue_mask) )
	    return( -1 );
	head->red_shift = bitshift(head->red_mask);
	head->green_shift = bitshift(head->green_mask);
	head->blue_shift = bitshift(head->blue_mask);
    }

    if ( head->headersize==108 && (
	 getlong(fp,&temp) ||	/* alpha_mask */ \
	 getlong(fp,&temp) ||	/* color space type */ \
	 getlong(fp,&temp) ||	/* redx */ \
	 getlong(fp,&temp) ||	/* redy */ \
	 getlong(fp,&temp) ||	/* redz */ \
	 getlong(fp,&temp) ||	/* greenx */ \
	 getlong(fp,&temp) ||	/* greeny */ \
	 getlong(fp,&temp) ||	/* greenz */ \
	 getlong(fp,&temp) ||	/* bluex */ \
	 getlong(fp,&temp) ||	/* bluey */ \
	 getlong(fp,&temp) ||	/* bluez */ \
	 getlong(fp,&temp) ||	/* gammared */ \
	 getlong(fp,&temp) ||	/* gammagreen */ \
	 getlong(fp,&temp) )	/* gammablue */ )
	return( -1 );

    return( 0 );
}

static int readpixels(FILE *file,struct bmpheader *head) {
    int i,ii,j,ll,excess;

    fseek(file,head->offset,0);

    ll = head->width;
    if ( head->bitsperpixel==8 && head->compression==0 ) {
	excess = ((ll+3)/4)*4 -ll;
	for ( i=0; i<head->height; ++i ) {
	    fread(head->byte_pixels+i*ll,1,ll,file);
	    for ( j=0; j<excess; ++j )
		(void) getc(file);
	}
    } else if ( head->bitsperpixel==8 ) {
	/* 8 bit RLE */
	int ii = 0;
	while ( ii<head->height*head->width ) {
	    int cnt = getc(file);
	    if ( cnt!=0 ) {
		int ch = getc(file);
		while ( --cnt>=0 )
		    head->byte_pixels[ii++] = ch;
	    } else {
		cnt = getc(file);
		if ( cnt>= 3 ) {
		    int odd = cnt&1;
		    while ( --cnt>=0 )
			head->byte_pixels[ii++] = getc(file);
		    if ( odd )
			getc(file);
		} else if ( cnt==0 ) {	/* end of scan line */
		    ii = ((ii+head->width-1)/head->width) * head->width;
		} else if ( cnt==1 ) {
	break;
		} else if ( cnt==2 ) {
		    int x=getc(file);
		    int y = getc(file);
		    y += ii/head->width;
		    x += ii%head->width;
		    ii = y*head->width + x;
		}
	    }
	}
    } else if ( head->bitsperpixel==4 && head->compression==0 ) {
	excess = (ll+1)/2;
	excess = ((excess+3)/4)*4 -excess;
	for ( i=0; i<head->height; ++i ) {
	    ii = i*ll;
	    for ( j=0; j<((head->width+7)/8)*8; j+=8 ) {
		int b1 = getc(file);
		int b2 = getc(file);
		int b3 = getc(file);
		int b4 = getc(file);
		head->byte_pixels[ii+j] = (b1>>4);
		if ( j+1<ll ) head->byte_pixels[ii+j+1] = (b1&0xf);
		if ( j+2<ll ) head->byte_pixels[ii+j+2] = (b2>>4);
		if ( j+3<ll ) head->byte_pixels[ii+j+3] = (b2&0xf);
		if ( j+4<ll ) head->byte_pixels[ii+j+4] = (b3>>4);
		if ( j+5<ll ) head->byte_pixels[ii+j+5] = (b3&0xf);
		if ( j+6<ll ) head->byte_pixels[ii+j+6] = (b4>>4);
		if ( j+7<ll ) head->byte_pixels[ii+j+7] = (b4&0xf);
	    }
	    for ( j=0; j<excess; ++j )
		(void) getc(file);
	}
    } else if ( head->bitsperpixel==4 ) {
	/* 4 bit RLE */
	int ii = 0;
	while ( ii<head->height*head->width ) {
	    int cnt = getc(file);
	    if ( cnt!=0 ) {
		int ch = getc(file);
		while ( (cnt-=2)>=-1 ) {
		    head->byte_pixels[ii++] = ch>>4;
		    head->byte_pixels[ii++] = ch&0xf;
		}
		if ( cnt==-1 ) --ii;
	    } else {
		cnt = getc(file);
		if ( cnt>= 3 ) {
		    int odd = cnt&2;
		    while ( (cnt-=2)>=-1 ) {
		    	int ch = getc(file);
			head->byte_pixels[ii++] = ch>>4;
			head->byte_pixels[ii++] = ch&0xf;
		    }
		    if ( cnt==-1 ) --ii;
		    if ( odd )
			getc(file);
		} else if ( cnt==0 ) {	/* end of scan line */
		    ii = ((ii+head->width-1)/head->width) * head->width;
		} else if ( cnt==1 ) {
	break;
		} else if ( cnt==2 ) {
		    int x=getc(file);
		    int y = getc(file);
		    y += ii/head->width;
		    x += ii%head->width;
		    ii = y*head->width + x;
		}
	    }
	}
    } else if ( head->bitsperpixel==1 ) {
	excess = (ll+7)/8;
	excess = ((excess+3)/4)*4 -excess;
	for ( i=0; i<head->height; ++i ) {
	    ii = i*((ll+7)/8);
	    for ( j=0; j<((head->width+7)/8); ++j ) {
		head->byte_pixels[ii+j] = getc(file);
	    }
	    for ( j=0; j<excess; ++j )
		(void) getc(file);
	}
    } else if ( head->bitsperpixel==24 ) {
	excess = ((3*head->width+3)/4)*4 - 3*head->width;
	for ( i=0; i<head->height; ++i ) {
	    ii = i*head->width;
	    for ( j=0; j<head->width; ++j ) {
		int b = getc(file);
		int g = getc(file);
		int r = getc(file);
		head->int32_pixels[ii+j] = COLOR_CREATE(r,g,b);
	    }
	    for ( j=0; j<excess; ++j )
		(void) getc(file);	/* ignore padding */
	}
    } else if ( head->bitsperpixel==32 && head->compression==0 ) {
	for ( i=0; i<head->height; ++i ) {
	    ii = i*head->width;
	    for ( j=0; j<head->width; ++j ) {
		int b,g,r;
		b = getc(file);
		g = getc(file);
		r = getc(file);
		(void) getc(file);	/* Ignore the alpha channel */
		head->int32_pixels[ii+j] = COLOR_CREATE(r,g,b);
	    }
	}
    } else if ( head->bitsperpixel==16 ) {
	for ( i=0; i<head->height; ++i ) {
	    ii = i*head->width;
	    for ( j=0; j<head->width; ++j ) {
		int pix = getshort(file);
		head->int32_pixels[ii+j] = COLOR_CREATE((pix&head->red_mask)>>head->red_shift,
			(pix&head->green_mask)>>head->green_shift,
			(pix&head->blue_mask)>>head->blue_shift);
	    }
	}
	if ( head->width&1 )
	    getshort(file);
    } else if ( head->bitsperpixel==32 ) {
	for ( i=0; i<head->height; ++i ) {
	    ii = i*head->width;
	    for ( j=0; j<head->width; ++j ) {
		long pix;
		if ( getlong(file,&pix) )
		    return( 1 );
		head->int32_pixels[ii+j] = COLOR_CREATE((pix&head->red_mask)>>head->red_shift,
			(pix&head->green_mask)>>head->green_shift,
			(pix&head->blue_mask)>>head->blue_shift);
	    }
	}
    }

    if ( feof(file )) {		/* Did we get an incomplete file? */
	return( 0 );
    }

return( 1 );
}

GImage *GImageRead_Bmp(FILE *file) {
/* Import a BMP image (based on file handle), cleanup & return NULL if error */
    struct bmpheader bmp;
    int i,l;
    GImage *ret = NULL;
    struct _GImage *base;

    if ( file==NULL )
	return( NULL );

    /* First, read-in header information */
    memset(&bmp,'\0',sizeof(bmp));
    if ( fillbmpheader(file,&bmp) )
	goto errorGImageReadBmp;

    /* Create memory-space to read-in bmp file */
    if ( (bmp.bitsperpixel>=16 && \
	 (bmp.int32_pixels=(uint32 *)(malloc(bmp.height*bmp.width*sizeof(uint32))))==NULL) || \
	 (bmp.bitsperpixel==1  && \
	 (bmp.byte_pixels=(unsigned char *)(malloc(bmp.height*((bmp.width+7)/8)*sizeof(unsigned char))))==NULL) || \
	 (bmp.byte_pixels=(unsigned char *)(malloc(bmp.height*bmp.width*sizeof(unsigned char))))==NULL ) {
	NoMoreMemMessage();
	return( NULL );
    }

    if ( !readpixels(file,&bmp) )
	goto errorGImageReadBmp;

    if ( !bmp.invert ) {
	if ( (ret=_GImage_Create(bmp.bitsperpixel>=16?it_true:bmp.bitsperpixel!=1?it_index:it_mono,
		bmp.width, bmp.height))==NULL ) {
	    NoMoreMemMessage();
	    goto errorGImageMemBmp;
	}
	if ( bmp.bitsperpixel>=16 ) {
	    ret->u.image->data = (uint8 *) bmp.int32_pixels;
	} else if ( bmp.bitsperpixel!=1 ) {
	    ret->u.image->data = (uint8 *) bmp.byte_pixels;
	}
    } else {
	if ( bmp.bitsperpixel>=16 ) {
	    if ( (ret=GImageCreate(it_true,bmp.width, bmp.height))==NULL ) {
		NoMoreMemMessage();
		goto errorGImageMemBmp;
	    }
	    base = ret->u.image;
	    for ( i=0; i<bmp.height; ++i ) {
		l = bmp.height-1-i;
		memcpy(base->data+l*base->bytes_per_line,bmp.int32_pixels+i*bmp.width,bmp.width*sizeof(uint32));
	    }
	    free(bmp.int32_pixels);
	} else if ( bmp.bitsperpixel!=1 ) {
	    if ( (ret=GImageCreate(it_index,bmp.width, bmp.height))==NULL ) {
		NoMoreMemMessage();
		goto errorGImageMemBmp;
	    }
	    base = ret->u.image;
	    for ( i=0; i<bmp.height; ++i ) {
		l = bmp.height-1-i;
		memcpy(base->data+l*base->bytes_per_line,bmp.byte_pixels+i*bmp.width,bmp.width);
	    }
	    free(bmp.byte_pixels);
	} else {
	    if ( (ret=GImageCreate(it_mono,bmp.width, bmp.height))==NULL ) {
		NoMoreMemMessage();
		goto errorGImageMemBmp;
	    }
	    base = ret->u.image;
	    for ( i=0; i<bmp.height; ++i ) {
		l = bmp.height-1-i;
		memcpy(base->data+l*base->bytes_per_line,bmp.byte_pixels+i*base->bytes_per_line,base->bytes_per_line);
	    }
	    free(bmp.byte_pixels);
	}
    }
    if ( ret->u.image->image_type==it_index ) {
	ret->u.image->clut->clut_len = bmp.colorsused;
	memcpy(ret->u.image->clut->clut,bmp.clut,bmp.colorsused*sizeof(Color));
	ret->u.image->clut->trans_index = COLOR_UNKNOWN;
    } else if ( ret->u.image->image_type==it_mono && bmp.colorsused!=0 ) {
	if ( (ret->u.image->clut=(GClut *)(calloc(1,sizeof(GClut))))==NULL ) {
	    NoMoreMemMessage();
	    goto errorGImageMemBmp;
	}
	ret->u.image->clut->clut_len = bmp.colorsused;
	memcpy(ret->u.image->clut->clut,bmp.clut,bmp.colorsused*sizeof(Color));
	ret->u.image->clut->trans_index = COLOR_UNKNOWN;
    }
    return( ret );

errorGImageReadBmp:
    fprintf(stderr,"Bad input file\n");
errorGImageMemBmp:
    GImageDestroy(ret);
    if ( bmp.bitsperpixel>=16 ) free(bmp.int32_pixels);
    else free(bmp.byte_pixels);
    return( NULL );
}

GImage *GImageReadBmp(char *filename) {
/* Import a BMP image, else cleanup and return NULL if error found */
    FILE *file;			/* source file */
    GImage *ret;

    if ( (file=fopen(filename,"rb"))==NULL ) {
	fprintf(stderr,"Can't open \"%s\"\n", filename);
	return( NULL );
    }

    ret = GImageRead_Bmp(file);
    fclose(file);
    return( ret );
}
