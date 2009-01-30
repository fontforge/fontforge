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
#include <basics.h>

/* There doesn't seem to be any difference between libgif and libungif (on my
 *  system they are symbolic linked together). Either libungif doesn't support
 *  interlaced gifs, or I'm not using it properly. At the moment I don't care
*/

#ifdef _NO_LIBUNGIF
static int a_file_must_define_something=0;	/* ANSI says so */
#elif !defined(_STATIC_LIBUNGIF) && !defined(NODYNAMIC)	/* I don't know how to deal with dynamic libs on mac OS/X, hence this */
#include <dynamic.h>
#include <string.h>

#include "gimage.h"

#include <gif_lib.h>

static DL_CONST void *libgif=NULL;
static GifFileType *(*_DGifOpenFileName)(char *);
static int (*_DGifSlurp)(GifFileType *);
static int (*_DGifCloseFile)(GifFileType *);

static int loadgif() {
    char *err;

    libgif = dlopen("libungif" SO_EXT,RTLD_LAZY);
    if ( libgif==NULL )
	libgif = dlopen("libgif" SO_EXT,RTLD_LAZY);
    if ( libgif==NULL ) {
	fprintf(stderr,"%s\n", dlerror());
return( 0 );
    }
    _DGifOpenFileName = (GifFileType *(*)(char *)) dlsym(libgif,"DGifOpenFileName");
    _DGifSlurp = (int (*)(GifFileType *)) dlsym(libgif,"DGifSlurp");
    _DGifCloseFile = (int (*)(GifFileType *)) dlsym(libgif,"DGifCloseFile");
    if ( _DGifOpenFileName && _DGifSlurp && _DGifCloseFile )
return( 1 );
    dlclose(libgif);
    err = dlerror();
    if ( err==NULL )
	err = "Couldn't load needed symbol from libgif.so";
    fprintf(stderr,"%s\n", err);
return( 0 );
}

static GImage *ProcessSavedImage(GifFileType *gif,struct SavedImage *si) {
    GImage *ret;
    struct _GImage *base;
    ColorMapObject *m = gif->SColorMap;
    int i,j,l;
    uint8 *d;

    if ( si->ImageDesc.ColorMap!=NULL )
	m = si->ImageDesc.ColorMap;
    if ( m->BitsPerPixel==1 ) {
	ret = GImageCreate(it_bitmap,si->ImageDesc.Width,si->ImageDesc.Height);
	if ( m->ColorCount==2 &&
		m->Colors[0].Red==0 && m->Colors[0].Green==0 && m->Colors[0].Blue==0 &&
		m->Colors[1].Red==255 && m->Colors[1].Green==255 && m->Colors[1].Blue==255 )
	    /* Don't need a clut */;
	else
	    ret->u.image->clut = gcalloc(1,sizeof(GClut));
    } else
	ret = GImageCreate(it_index,si->ImageDesc.Width,si->ImageDesc.Height);
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
    for ( i=0; i<si->ExtensionBlockCount; ++i ) {
	if ( si->ExtensionBlocks[i].Function==0xf9 &&
		si->ExtensionBlocks[i].ByteCount>=4 ) {
	    base->delay = (si->ExtensionBlocks[i].Bytes[2]<<8) |
		    (si->ExtensionBlocks[i].Bytes[2]&&0xff);
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
    GImage *ret, **images;
    GifFileType *gif;
    int i;

    if ( libgif==NULL )
	if ( !loadgif())
return( NULL );

    if ((gif = _DGifOpenFileName(filename)) == NULL) {
	fprintf( stderr, "can't open %s\n", filename);
return( NULL );
    }

    if ( _DGifSlurp(gif)==GIF_ERROR ) {
	_DGifCloseFile(gif);
	fprintf( stderr, "Bad gif file %s\n", filename );
return( NULL );
    }

    images = galloc(gif->ImageCount*sizeof(GImage *));
    for ( i=0; i<gif->ImageCount; ++i )
	images[i] = ProcessSavedImage(gif,&gif->SavedImages[i]);
    if ( gif->ImageCount==1 )
	ret = images[0];
    else
	ret = GImageCreateAnimation(images,gif->ImageCount);
    _DGifCloseFile(gif);
    free(images);
return( ret );
}
#else
#include <string.h>

#include "gimage.h"

#include <gif_lib.h>

static GImage *ProcessSavedImage(GifFileType *gif,struct SavedImage *si) {
    GImage *ret;
    struct _GImage *base;
    ColorMapObject *m = gif->SColorMap;
    int i,j,l;
    uint8 *d;

    if ( si->ImageDesc.ColorMap!=NULL )
	m = si->ImageDesc.ColorMap;
    if ( m->BitsPerPixel==1 ) {
	ret = GImageCreate(it_bitmap,si->ImageDesc.Width,si->ImageDesc.Height);
	if ( m->ColorCount==2 &&
		m->Colors[0].Red==0 && m->Colors[0].Green==0 && m->Colors[0].Blue==0 &&
		m->Colors[1].Red==255 && m->Colors[1].Green==255 && m->Colors[1].Blue==255 )
	    /* Don't need a clut */;
	else
	    ret->u.image->clut = gcalloc(1,sizeof(GClut));
    } else
	ret = GImageCreate(it_index,si->ImageDesc.Width,si->ImageDesc.Height);
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
    for ( i=0; i<si->ExtensionBlockCount; ++i ) {
	if ( si->ExtensionBlocks[i].Function==0xf9 &&
		si->ExtensionBlocks[i].ByteCount>=4 ) {
	    base->delay = (si->ExtensionBlocks[i].Bytes[2]<<8) |
		    (si->ExtensionBlocks[i].Bytes[2]&&0xff);
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
    GImage *ret, **images;
    GifFileType *gif;
    int i;

    if ((gif = DGifOpenFileName(filename)) == NULL) {
	fprintf( stderr, "can't open %s\n", filename);
return( NULL );
    }

    if ( DGifSlurp(gif)==GIF_ERROR ) {
	DGifCloseFile(gif);
	fprintf(stderr,"Bad gif file %s\n", filename );
return( NULL );
    }

    images = galloc(gif->ImageCount*sizeof(GImage *));
    for ( i=0; i<gif->ImageCount; ++i )
	images[i] = ProcessSavedImage(gif,&gif->SavedImages[i]);
    if ( gif->ImageCount==1 )
	ret = images[0];
    else
	ret = GImageCreateAnimation(images,gif->ImageCount);
    DGifCloseFile(gif);
    free(images);
return( ret );
}
#endif
