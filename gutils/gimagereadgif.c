/* Copyright (C) 2000-2012 by George Williams */
/* 2013jan18..22, several fixes + interlacing, Jose Da Silva */
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

#ifdef _NO_LIBUNGIF

static int a_file_must_define_something=0;	/* ANSI says so */

#else /* We can build with gif_lib - therefore import gif files */

#include "basics.h"
#include "gimage.h"

#include <gif_lib.h>

#if defined(GIFLIB_MAJOR) && defined(GIFLIB_MINOR) && ((GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1) || GIFLIB_MAJOR > 5)
#define _GIFLIB_51PLUS
#endif

static GImage *ProcessSavedImage(GifFileType *gif,struct SavedImage *si,int il) {
/* Process each gif image into an internal FF format. Return NULL if error */
    GImage *ret;
    struct _GImage *base;
    ColorMapObject *m = gif->SColorMap;  /* gif_lib.h, NULL if not exists. */
    int i,j,k,*id = NULL;
    long l;
    uint8 *d,*iv = NULL;

    /* Create memory to hold image, exit with NULL if not enough memory */
    if ( si->ImageDesc.ColorMap!=NULL ) m=si->ImageDesc.ColorMap;
    if ( m==NULL )
	return( NULL );
    if ( m->BitsPerPixel==1 ) {
	if ( (ret=GImageCreate(it_bitmap,si->ImageDesc.Width,si->ImageDesc.Height))==NULL )
	    return( NULL );
	if ( m->ColorCount==2 &&
		m->Colors[0].Red==0 && m->Colors[0].Green==0 && m->Colors[0].Blue==0 &&
		m->Colors[1].Red==255 && m->Colors[1].Green==255 && m->Colors[1].Blue==255 )
	    /* Don't need a clut */;
	else
	    if ( (ret->u.image->clut = (GClut *) calloc(1,sizeof(GClut)))==NULL ) {
		free(ret);
		NoMoreMemMessage();
		return( NULL );
	    }
    } else
	if ( (ret=GImageCreate(it_index,si->ImageDesc.Width,si->ImageDesc.Height))==NULL )
	    return( NULL );
    if ( il && ((id=(int *) malloc(si->ImageDesc.Height*sizeof(int)))==NULL || \
		(iv=(uint8 *) malloc(si->ImageDesc.Height*sizeof(uint8)))==NULL) ) {
	free(ret->u.image->clut);
	free(ret);
	free(id);
	free(iv);
	NoMoreMemMessage();
	return( NULL );
    }

    /* Process gif image into an internal FF usable format */
    base = ret->u.image;
    if ( base->clut!=NULL ) {
	base->clut->clut_len = m->ColorCount;
	for ( i=0; i<m->ColorCount; ++i )
	    base->clut->clut[i] = COLOR_CREATE(m->Colors[i].Red,m->Colors[i].Green,m->Colors[i].Blue);
    }
    if ( m->BitsPerPixel!=1 )
	memcpy(base->data,si->RasterBits,base->width*base->height);
    else if ( m->BitsPerPixel==1 ) {
	l=0;
	for ( i=0; i<base->height; ++i ) {
	    d = (base->data + i*base->bytes_per_line);
	    memset(d,'\0',base->bytes_per_line);
	    for ( j=0; j<base->width; ++j ) {
		if ( si->RasterBits[l] )
		    d[j>>3] |= (1<<(7-(j&7)));
		++l;
	    }
	}
    }
    if ( il ) {
	/* Convert interlaced image into a sequential image */
	j=0; k=0;
	for ( i=0; i<base->height; ++i ) {
	    id[i]=k;
	    if ( j==0 ) {
		k += 8;
		if ( k>=base->height ) {
		    j++; k=4;
		}
	    } else if ( j==1 ) {
		k += 8;
		if ( k>=base->height ) {
		    j++; k=2;
		}
	    } else if ( j==2 ) {
		k += 4;
		if ( k>=base->height ) {
		    j++; k=1;
		}
	    } else
		k += 2;
	}
	for ( j=0; j<base->bytes_per_line; ++j ) {
	    for ( i=1; i<base->height; ++i )
		iv[id[i]]=base->data[i*base->bytes_per_line+j];
	    for ( i=1; i<base->height; ++i )
		base->data[i*base->bytes_per_line+j]=iv[i];
	}
	free(id);
	free(iv);
    }
    for ( i=0; i<si->ExtensionBlockCount; ++i ) {
	if ( si->ExtensionBlocks[i].Function==0xf9 &&
		si->ExtensionBlocks[i].ByteCount>=4 ) {
	    base->delay = (si->ExtensionBlocks[i].Bytes[2]<<8) |
		    (si->ExtensionBlocks[i].Bytes[2]&0xff);
	    if ( si->ExtensionBlocks[i].Bytes[0]&1 ) {
		base->trans = (unsigned char) si->ExtensionBlocks[i].Bytes[3];
		if ( base->clut!=NULL )
		    base->clut->trans_index = base->trans;
	    }
	}
    }
    return( ret );
}

GImage *GImageReadGif(char *filename) {
/* Import a gif image (or gif animation), else return NULL if error  */
    GImage *ret, **images;
    GifFileType *gif;
    int i,il;

    //
    // MIQ: As at mid 2013 giflib version 4 is still the current
    // version in some environments. Given that this function call
    // seems to be the only incompatible change in what FontForge uses
    // as at that time, I added a smoother macro here to allow v4 to
    // still work OK for the time being.
    // 
#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR >= 5 || defined(_GIFLIB_5PLUS)
    if ( (gif=DGifOpenFileName(filename,NULL))==NULL ) {
#else
    if ( (gif=DGifOpenFileName(filename))==NULL ) {
#endif
	fprintf( stderr,"Can't open \"%s\"\n",filename );
	return( NULL );
    }

    if ( DGifSlurp(gif)!=GIF_OK ) {
	fprintf(stderr,"Bad input file \"%s\"\n",filename );
#ifdef _GIFLIB_51PLUS
	DGifCloseFile(gif, NULL);
#else
	DGifCloseFile(gif);
#endif
	return( NULL );
    }

    /* Process each image so that it/they can be imported into FF. */
    if ( (images=(GImage **) malloc(gif->ImageCount*sizeof(GImage *)))==NULL ) {
#ifdef  _GIFLIB_51PLUS
        DGifCloseFile(gif, NULL);
#else
        DGifCloseFile(gif);
#endif
        NoMoreMemMessage();
        return( NULL );
    }
    il=gif->SavedImages[0].ImageDesc.Interlace;
    for ( i=0; i<gif->ImageCount; ++i ) {
	if ( (images[i]=ProcessSavedImage(gif,&gif->SavedImages[i],il))==NULL ) {
	    while ( --i>=0 ) free(images[i]);
	    free(images);
#ifdef  _GIFLIB_51PLUS
	    DGifCloseFile(gif, NULL);
#else
	    DGifCloseFile(gif);
#endif
	    return( NULL );
	}
    }

    /* All okay if you reached here. We have 1 image or several images */
    if ( gif->ImageCount==1 )
	ret = images[0];
    else
	ret = GImageCreateAnimation(images,gif->ImageCount);
#ifdef  _GIFLIB_51PLUS
    DGifCloseFile(gif, NULL);
#else
    DGifCloseFile(gif);
#endif
    free(images);
    return( ret );
}

#endif /* ! _NO_LIBUNGIF */
