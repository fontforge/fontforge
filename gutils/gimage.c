/* Copyright (C) 2000-2007 by George Williams */
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

GImage *GImageCreate(enum image_type type, int32 width, int32 height) {
    GImage *gi;
    struct _GImage *base;

    if ( type<it_mono || type>it_true )
return( NULL );

    gi = gcalloc(1,sizeof(GImage));
    base = galloc(sizeof(struct _GImage));
    if ( gi==NULL || base==NULL ) {
	free(gi); free(base);
return( NULL );
    }
    gi->u.image = base;
    base->image_type = type;
    base->width = width;
    base->height = height;
    base->bytes_per_line = type==it_true?4*width:type==it_index?width:(width+7)/8;
    base->data = NULL;
    base->clut = NULL;
    base->trans = COLOR_UNKNOWN;
    base->data = galloc(height*base->bytes_per_line);
    if ( base->data==NULL ) {
	free(base);
	free(gi);
return( NULL );
    }
    if ( type==it_index ) {
	base->clut = gcalloc(1,sizeof(GClut));
	base->clut->trans_index = COLOR_UNKNOWN;
    }
return( gi );
}


GImage *_GImage_Create(enum image_type type, int32 width, int32 height) {
    GImage *gi;
    struct _GImage *base;

    if ( type<it_mono || type>it_true )
return( NULL );

    gi = gcalloc(1,sizeof(GImage));
    base = galloc(sizeof(struct _GImage));
    if ( gi==NULL || base==NULL ) {
	free(gi); free(base);
return( NULL );
    }
    gi->u.image = base;
    base->image_type = type;
    base->width = width;
    base->height = height;
    base->bytes_per_line = type==it_true?4*width:type==it_index?width:(width+7)/8;
    base->data = NULL;
    base->clut = NULL;
    if ( type==it_index )
	base->clut = gcalloc(1,sizeof(GClut));
return( gi );
}

void GImageDestroy(GImage *gi) {
    int i;

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

GImage *GImageCreateAnimation(GImage **images, int n) {
    struct _GImage **imgs = galloc(n*sizeof(struct _GImage *));
    GImage *gi = gcalloc(1,sizeof(GImage));
    int i;

    gi->list_len = n;
    gi->u.images = imgs;
    for ( i=0; i<n; ++i ) {
	if ( images[i]->list_len!=0 ) {
	    free(gi);
return( NULL );
	}
	if ( images[i]->u.image->image_type!=images[0]->u.image->image_type )
return( NULL );
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
    imgs = galloc(n*sizeof(struct _GImage *));

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
	if ( src->u.image->image_type!=it )
return( NULL );
	imgs[j++] = src->u.image;
    } else {
	for ( ; j<i+src->list_len; ++j ) {
	    if ( src->u.images[j-i]->image_type!=it )
return( NULL );
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

void GImageDrawImage(GImage *dest,GImage *src,void *junk,int x, int y) {
    struct _GImage *sbase, *dbase;
    int i,j, di, sbi, dbi, val;

    dbase = dest->u.image;
    sbase =  src->u.image;

    for ( i=0; i<sbase->height; ++i ) {
	di = y + i;
	if ( di<0 || di>=dbase->height )
    continue;
	sbi = i*sbase->bytes_per_line;
	dbi = di*dbase->bytes_per_line;
	for ( j=0; j<sbase->width; ++j ) {
	    if ( x+j<0 || x+j>=dbase->width )
	continue;
	    val = dbase->data[dbi+x+j] + sbase->data[sbi+j];
	    if ( val>255 ) val = 255;
	    dbase->data[dbi+x+j] = val;
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
