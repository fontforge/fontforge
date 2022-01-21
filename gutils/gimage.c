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

#include "basics.h"
#include "gimage.h"

GImage *GImageCreate(enum image_type type, int32_t width, int32_t height) {
/* Prepare to get a bitmap image. Cleanup and return NULL if not enough memory */
    GImage *gi;
    struct _GImage *base;

    if ( type<it_mono || type>it_rgba )
	return( NULL );

    gi = (GImage *) calloc(1,sizeof(GImage));
    base = (struct _GImage *) malloc(sizeof(struct _GImage));
    if ( gi==NULL || base==NULL )
	goto errorGImageCreate;

    gi->u.image = base;
    base->image_type = type;
    base->width = width;
    base->height = height;
    base->bytes_per_line = (type==it_true || type==it_rgba)?4*width:type==it_index?width:(width+7)/8;
    base->data = NULL;
    base->clut = NULL;
    base->trans = COLOR_UNKNOWN;
    if ( (base->data = (uint8_t *) malloc(height*base->bytes_per_line))==NULL )
	goto errorGImageCreate;
    if ( type==it_index ) {
	if ( (base->clut = (GClut *) calloc(1,sizeof(GClut)))==NULL ) {
	    free(base->data);
	    goto errorGImageCreate;
 	}
	base->clut->trans_index = COLOR_UNKNOWN;
    }
    return( gi );

errorGImageCreate:
    free(base);
    free(gi);
    NoMoreMemMessage();
    return( NULL );
}


GImage *_GImage_Create(enum image_type type, int32_t width, int32_t height) {
    GImage *gi;
    struct _GImage *base;

    if ( type<it_mono || type>it_rgba )
	return( NULL );

    gi = (GImage *) calloc(1,sizeof(GImage));
    base = (struct _GImage *) malloc(sizeof(struct _GImage));
    if ( gi==NULL || base==NULL )
	goto error_GImage_Create;

    gi->u.image = base;
    base->image_type = type;
    base->width = width;
    base->height = height;
    base->bytes_per_line = (type==it_true || type==it_rgba)?4*width:type==it_index?width:(width+7)/8;
    base->data = NULL;
    base->clut = NULL;
    if ( type==it_index ) {
	if ( (base->clut = (GClut *) calloc(1,sizeof(GClut)))==NULL )
	  goto error_GImage_Create;
    }
    return( gi );

error_GImage_Create:
    free(base);
    free(gi);
    NoMoreMemMessage();
    return( NULL );
}

void GImageDestroy(GImage *gi) {
/* Free memory (if GImage exists) */
    int i;

    if (gi!=NULL ) {
	if ( gi->list_len!=0 ) {
	    for ( i=0; i<gi->list_len; ++i ) {
		free(gi->u.images[i]->clut);
		free(gi->u.images[i]->data);
		free(gi->u.images[i]);
	    }
	    free(gi->u.images);
	} else {
	    free(gi->u.image->clut);
	    free(gi->u.image->data);
	    free(gi->u.image);
	}
	free(gi);
    }
}

GImage *GImageCreateAnimation(GImage **images, int n) {
/* Create an animation using n "images". Return gi and free "images" if	*/
/* okay, else return NULL and keep "images" if memory error occurred,	*/
    GImage *gi;
    struct _GImage **imgs;
    int i;

    /* Check if "images" are okay to copy before creating an animation.	*/
    /* We expect to find single images (not an array). Type must match.	*/
    for ( i=0; i<n; ++i ) {
	if ( images[i]->list_len!=0 || \
	     images[i]->u.image->image_type!=images[0]->u.image->image_type ) {
	    fprintf( stderr, "Images are not compatible to make an Animation\n" );
	    return( NULL );
	}
    }

    /* First, create enough memory space to hold the complete animation	*/
    gi = (GImage *) calloc(1,sizeof(GImage));
    imgs = (struct _GImage **) malloc(n*sizeof(struct _GImage *));
    if ( gi==NULL || imgs==NULL ) {
	free(gi);
	free(imgs);
	NoMoreMemMessage();
	return( NULL );
    }

    /* Copy images[i] pointer into 'gi', then release each "images[i]".	*/
    gi->list_len = n;
    gi->u.images = imgs;
    for ( i=0; i<n; ++i ) {
	imgs[i] = images[i]->u.image;
	free(images[i]);
    }
    return( gi );
}

/* -1 => add it at the end */
GImage *GImageAddImageBefore(GImage *dest, GImage *src, int pos) {
    struct _GImage **imgs;
    int n, i, j;
    enum image_type it;

    n = (src->list_len==0?1:src->list_len) + (dest->list_len==0?1:dest->list_len);
    imgs = (struct _GImage **) malloc(n*sizeof(struct _GImage *));
    if ( imgs==NULL ) {
	NoMoreMemMessage();
	return( NULL );
    }

    i = 0;
    if ( dest->list_len==0 ) {
	it = dest->u.image->image_type;
	if ( pos==-1 ) pos = 1;
	if ( pos!=0 ) imgs[i++] = dest->u.image;
    } else {
	it = dest->u.images[0]->image_type;
	if ( pos==-1 ) pos = dest->list_len;
	for ( i=0; i<pos; ++i )
	    imgs[i] = dest->u.images[i];
    }
    j = i;
    if ( src->list_len==0 ) {
	if ( src->u.image->image_type!=it ) {
            free(imgs);
return( NULL );
}
	imgs[j++] = src->u.image;
    } else {
	for ( ; j<i+src->list_len; ++j ) {
	    if ( src->u.images[j-i]->image_type!=it ) {
                free(imgs);
return( NULL );
}
	    imgs[j] = src->u.images[j-i];
	}
	free(src->u.images);
    }
    if ( dest->list_len==0 ) {
	if ( pos==0 ) imgs[j++] = dest->u.image;
    } else {
	for ( ; j<n; ++j )
	    imgs[j] = dest->u.images[i++];
    }
    dest->u.images = imgs;
    dest->list_len = n;
    free(src);
return( dest );
}

void GImageDrawRect(GImage *img,GRect *r,Color col) {
    struct _GImage *base;
    int i;

    base = img->u.image;
    if ( r->y>=base->height || r->x>=base->width )
return;

    for ( i=0; i<r->width; ++i ) {
	if ( i+r->x>=base->width )
    break;
	base->data[r->y*base->bytes_per_line + i + r->x] = col;
	if ( r->y+r->height-1<base->height )
	    base->data[(r->y+r->height-1)*base->bytes_per_line + i + r->x] = col;
    }
    for ( i=0; i<r->height; ++i ) {
	if ( i+r->y>=base->height )
    break;
	base->data[(r->y+i)*base->bytes_per_line + r->x] = col;
	if ( r->x+r->width-1<base->width )
	    base->data[(r->y+i)*base->bytes_per_line + r->x+r->width-1] = col;
    }
}

void GImageDrawImage(GImage *dest,GImage *src,GRect *UNUSED(junk),int x, int y) {
    struct _GImage *sbase, *dbase;
    int i,j, di, sbi, dbi, val, factor, maxpix, sbit;

    /* This is designed to merge images which should be treated as alpha */
    /* channels. dest must be indexed, src may be either indexed or mono */
    dbase = dest->u.image;
    sbase =  src->u.image;

    if ( dbase->image_type != it_index ) {
	fprintf( stderr, "Bad call to GImageMaxImage\n" );
return;
    }

    maxpix = 1;
    if ( dbase->clut!=NULL )
	maxpix = dbase->clut->clut_len - 1;

    if ( dbase->clut!=NULL && sbase->clut!=NULL && sbase->clut->clut_len>1 ) {
	factor = (dbase->clut->clut_len - 1) / (sbase->clut->clut_len - 1);
	if ( factor==0 ) factor=1;
    } else
	factor = 1;

    if ( sbase->image_type == it_index ) {
	for ( i=0; i<sbase->height; ++i ) {
	    di = y + i;
	    if ( di<0 || di>=dbase->height )
	continue;
	    sbi = i*sbase->bytes_per_line;
	    dbi = di*dbase->bytes_per_line;
	    for ( j=0; j<sbase->width; ++j ) {
		if ( x+j<0 || x+j>=dbase->width )
	    continue;
		val = dbase->data[dbi+x+j] + sbase->data[sbi+j]*factor;
		if ( val>255 ) val = 255;
		dbase->data[dbi+x+j] = val;
	    }
	}
    } else if ( sbase->image_type == it_mono ) {
	for ( i=0; i<sbase->height; ++i ) {
	    di = y + i;
	    if ( di<0 || di>=dbase->height )
	continue;
	    sbi = i*sbase->bytes_per_line;
	    dbi = di*dbase->bytes_per_line;
	    for ( j=0, sbit=0x80; j<sbase->width; ++j ) {
		if ( x+j<0 || x+j>=dbase->width )
	    continue;
		if ( sbase->data[sbi+(j>>3)] & sbit )
		    dbase->data[dbi+x+j] = maxpix;
		if ( (sbit>>=1) == 0 )
		    sbit = 0x80;
	    }
	}
    }
}

/* Blends src image with alpha channel over dest. Both images must be */
/* 32-bit truecolor. Alpha channel of dest must be all opaque.        */
void GImageBlendOver(GImage *dest,GImage *src,GRect *from,int x, int y) {
    struct _GImage *sbase, *dbase;
    int i, j, a, r, g, b;
    uint32_t *dpt, *spt;

    dbase = dest->u.image;
    sbase =  src->u.image;

    if ( dbase->image_type != it_true ) {
	fprintf( stderr, "Bad call to GImageBlendOver\n" );
return;
    }

    if ( sbase->image_type != it_rgba ) {
	fprintf( stderr, "Bad call to GImageBlendOver\n" );
return;
    }

    for ( i=0; i<from->height; ++i ) {
	dpt = (uint32_t *) (dbase->data + (i+y)*dbase->bytes_per_line + x*sizeof(uint32_t));
	spt = (uint32_t *) (sbase->data + (i+from->y)*sbase->bytes_per_line + from->x*sizeof(uint32_t));
	
	for (j=0; j<from->width; j++) {
            a = COLOR_ALPHA(*spt);
            r = ((255-a)*COLOR_RED(*dpt)  + a*COLOR_RED(*spt))/255;
            g = ((255-a)*COLOR_GREEN(*dpt)+ a*COLOR_GREEN(*spt))/255;
            b = ((255-a)*COLOR_BLUE(*dpt) + a*COLOR_BLUE(*spt))/255;
            spt++;
            *dpt++ = 0xff000000 | COLOR_CREATE(r,g,b);
	}
    }
}

int GImageGetWidth(GImage *img) {
    if ( img->list_len==0 ) {
return( img->u.image->width );
    } else {
return( img->u.images[0]->width );
    }
}

int GImageGetHeight(GImage *img) {
    if ( img->list_len==0 ) {
return( img->u.image->height );
    } else {
return( img->u.images[0]->height );
    }
}

void *GImageGetUserData(GImage *img) {
return( img->userdata );
}

void GImageSetUserData(GImage *img,void *userdata) {
    img->userdata = userdata;
}

static Color _GImageGetPixelRGBA(struct _GImage *base,int x, int y) {
    Color val;

    if ( base->image_type==it_rgba ) {
	val = ((uint32_t*) (base->data + y*base->bytes_per_line))[x] ;
return( val==base->trans?(val&0xffffff):val );
    } else if ( base->image_type==it_true ) {
	val = ((uint32_t*) (base->data + y*base->bytes_per_line))[x] ;
return( val==base->trans?(val&0xffffff):(val|0xff000000) );
    } else if ( base->image_type==it_index ) {
	uint8_t pixel = ((uint8_t*) (base->data + y*base->bytes_per_line))[x];
	val = base->clut->clut[pixel];
return( pixel==base->trans?(val&0xffffff):(val|0xff000000) );
    } else {
	uint8_t pixel = (((uint8_t*) (base->data + y*base->bytes_per_line))[x>>3]&(1<<(7-(x&7))) )?1:0;
	if ( base->clut==NULL ) {
	    if ( pixel )
		val = COLOR_CREATE(0xff,0xff,0xff);
	    else
		val = COLOR_CREATE(0,0,0);
	} else
	    val = base->clut->clut[pixel];
return( pixel==base->trans?(val&0xffffff):(val|0xff000000) );
    }
}

Color GImageGetPixelRGBA(GImage *image,int x, int y) {
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
return( _GImageGetPixelRGBA(base,x,y));
}
