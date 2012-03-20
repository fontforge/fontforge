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
#include "gimagebmpP.h"
#include <string.h>
GImage *_GImage_Create(enum image_type type, int32 width, int32 height);

static int getshort(FILE *file) {
    int temp = getc(file);
return( (getc(file)<<8) | temp );
}

static long getl(FILE *file) {
    long temp1 = getc(file);
    long temp2 = getc(file);
    long temp3 = getc(file);
return( (getc(file)<<24) | (temp3<<16) | (temp2<<8) | temp1 );
}

static int bitshift(unsigned long mask) {
    int off=0, len=0, bit;

    if ( mask==0 )
return( 0 );
    for ( off=0; !(mask&1); mask>>=1, ++off );
    for ( len=0, bit=1; (mask&bit) && len<32; ++len, bit<<=1 );
return( off+(8-len) );
}

static int fillbmpheader(FILE *file,struct bmpheader *head) {
    int i;

    memset(head,'\0',sizeof(*head));
    if ( getc(file)!='B' || getc(file)!='M' )
return 0;			/* Bad format */
    head->size = getl(file);
    head->mbz1 = getshort(file);
    head->mbz2 = getshort(file);
    head->offset = getl(file);
    head->headersize = getl(file);
    if ( head->headersize==12 ) {	/* Windows 2.0 format, also OS/2 */
	head->width = getshort(file);
	head->height = getshort(file);
	head->planes = getshort(file);
	head->bitsperpixel = getshort(file);
	head->colorsused = 0;
	head->compression = 0;
    } else {
	head->width = getl(file);
	head->height = getl(file);
	head->planes = getshort(file);
	head->bitsperpixel = getshort(file);
	head->compression = getl(file);
	head->imagesize = getl(file);
	head->ignore1 = getl(file);
	head->ignore2 = getl(file);
	head->colorsused = getl(file);
	head->colorsimportant = getl(file);
    }
    if ( head->height<0 )
	head->height = -head->height;
    else
	head->invert = true;
    if ( head->bitsperpixel!=1 && head->bitsperpixel!=4 && head->bitsperpixel!=8 &&
	    head->bitsperpixel!=16 && head->bitsperpixel!=24 && head->bitsperpixel!=32 )
return( 0 );
    if ( head->compression==3 && ( head->bitsperpixel==16 || head->bitsperpixel==32 ))
	/* Good */;
    else if ( head->compression==0 && ( head->bitsperpixel<=8 || head->bitsperpixel==24 || head->bitsperpixel==32 ))
	/* Good */;
    else if ( head->compression==1 && head->bitsperpixel==8 )
	/* Good */;
    else if ( head->compression==2 && head->bitsperpixel==4 )
	/* Good */;
    else
return( 0 );
    if ( head->colorsused==0 )
	head->colorsused = 1<<head->bitsperpixel;
    if ( head->bitsperpixel>=16 )
	head->colorsused = 0;
    if ( head->colorsused>(1<<head->bitsperpixel) )
return( 0 );
    for ( i=0; i<head->colorsused; ++i ) {
	int b = getc(file), g = getc(file), r=getc(file);
	head->clut[i] = COLOR_CREATE(r,g,b);
	if ( head->headersize!=12 )
	    getc(file);
    }
    if ( head->compression==3 || head->headersize==108 ) {
	head->red_mask = getl(file);
	head->green_mask = getl(file);
	head->blue_mask = getl(file);
	head->red_shift = bitshift(head->red_mask);
	head->green_shift = bitshift(head->green_mask);
	head->blue_shift = bitshift(head->blue_mask);
    }
    if ( head->headersize==108 ) {
	getl(file);		/* alpha_mask */
	getl(file);		/* color space type */
	getl(file);		/* redx */
	getl(file);		/* redy */
	getl(file);		/* redz */
	getl(file);		/* greenx */
	getl(file);		/* greeny */
	getl(file);		/* greenz */
	getl(file);		/* bluex */
	getl(file);		/* bluey */
	getl(file);		/* bluez */
	getl(file);		/* gammared */
	getl(file);		/* gammagreen */
	getl(file);		/* gammablue */
    }
return( 1 );
}

static int readpixels(FILE *file,struct bmpheader *head) {
    int i,ii,j,ll,excess;

    fseek(file,head->offset,0);
    if ( head->bitsperpixel>=16 ) {
	head->int32_pixels = galloc(head->height* 4*head->width );
	if ( head->int32_pixels==NULL )
return( 0 );
    } else if ( head->bitsperpixel!=1 ) {
	head->byte_pixels = galloc(head->height* head->width );
	if ( head->byte_pixels==NULL )
return( 0 );
    } else {
	head->byte_pixels = galloc(head->height* ((head->width+7)/8) );
	if ( head->byte_pixels==NULL )
return( 0 );
    }

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
		int pix = getl(file);
		head->int32_pixels[ii+j] = COLOR_CREATE((pix&head->red_mask)>>head->red_shift,
			(pix&head->green_mask)>>head->green_shift,
			(pix&head->blue_mask)>>head->blue_shift);
	    }
	}
    }

    if ( feof(file )) {		/* Did we get an incomplete file? */
	if ( head->bitsperpixel>=16 ) gfree( head->int32_pixels );
	else gfree( head->byte_pixels );
return( 0 );
    }

return( 1 );
}

GImage *GImageRead_Bmp(FILE *file) {
    struct bmpheader bmp;
    int i,l;
    GImage *ret;
    struct _GImage *base;

    if ( file==NULL )
return( NULL );
    if ( !fillbmpheader(file,&bmp))
return( NULL );
    if ( !readpixels(file,&bmp))
return( NULL );

    if ( !bmp.invert ) {
	ret = _GImage_Create(bmp.bitsperpixel>=16?it_true:bmp.bitsperpixel!=1?it_index:it_mono,
		bmp.width, bmp.height);
	if ( bmp.bitsperpixel>=16 ) {
	    ret->u.image->data = (uint8 *) bmp.int32_pixels;
	} else if ( bmp.bitsperpixel!=1 ) {
	    ret->u.image->data = (uint8 *) bmp.byte_pixels;
	}
    } else {
	if ( bmp.bitsperpixel>=16 ) {
	    ret = GImageCreate(it_true,bmp.width, bmp.height);
	    base = ret->u.image;
	    for ( i=0; i<bmp.height; ++i ) {
		l = bmp.height-1-i;
		memcpy(base->data+l*base->bytes_per_line,bmp.int32_pixels+i*bmp.width,bmp.width*sizeof(uint32));
	    }
	    gfree( bmp.int32_pixels );
	} else if ( bmp.bitsperpixel!=1 ) {
	    ret = GImageCreate(it_index,bmp.width, bmp.height);
	    base = ret->u.image;
	    for ( i=0; i<bmp.height; ++i ) {
		l = bmp.height-1-i;
		memcpy(base->data+l*base->bytes_per_line,bmp.byte_pixels+i*bmp.width,bmp.width);
	    }
	    gfree( bmp.byte_pixels );
	} else {
	    ret = GImageCreate(it_mono,bmp.width, bmp.height);
	    base = ret->u.image;
	    for ( i=0; i<bmp.height; ++i ) {
		l = bmp.height-1-i;
		memcpy(base->data+l*base->bytes_per_line,bmp.byte_pixels+i*base->bytes_per_line,base->bytes_per_line);
	    }
	    gfree( bmp.byte_pixels );
	}
    }
    if ( ret->u.image->image_type==it_index ) {
	ret->u.image->clut->clut_len = bmp.colorsused;
	memcpy(ret->u.image->clut->clut,bmp.clut,bmp.colorsused*sizeof(Color));
	ret->u.image->clut->trans_index = COLOR_UNKNOWN;
    } else if ( ret->u.image->image_type==it_mono && bmp.colorsused!=0 ) {
	ret->u.image->clut = gcalloc(1,sizeof(GClut));
	ret->u.image->clut->clut_len = bmp.colorsused;
	memcpy(ret->u.image->clut->clut,bmp.clut,bmp.colorsused*sizeof(Color));
	ret->u.image->clut->trans_index = COLOR_UNKNOWN;
    }
return( ret );
}

GImage *GImageReadBmp(char *filename) {
    FILE *file = fopen(filename,"rb");
    GImage *ret;

    if ( file==NULL )
return( NULL );

    ret = GImageRead_Bmp(file);
    fclose(file);
return( ret );
}
