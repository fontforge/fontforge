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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#ifdef _NO_LIBPNG
static void *a_file_must_define_something=(void *) &a_file_must_define_something;
		/* ANSI says so */
#else
# if !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC)	/* I don't know how to deal with dynamic libs on mac OS/X, hence this */
#  include <dynamic.h>
# endif
# include <png.h>

# define int32 _int32
# define uint32 _uint32
# define int16 _int16
# define uint16 _uint16
# define int8 _int8
# define uint8 _uint8

# include "gimage.h"

# if !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC)
static DL_CONST void *libpng=NULL;
static png_structp (*_png_create_read_struct)(char *, png_voidp, png_error_ptr, png_error_ptr);
static png_infop (*_png_create_info_struct)(png_structp);
static void (*_png_destroy_read_struct)(png_structpp, png_infopp, png_infopp);
static void (*_png_init_io)(png_structp, FILE *);
static void (*_png_read_info)(png_structp, png_infop);
static void (*_png_set_strip_16)(png_structp);
static void (*_png_set_strip_alpha)(png_structp);
static void (*_png_set_packing)(png_structp);
static void (*_png_set_filler)(png_structp,png_uint_32,int);
static void (*_png_read_image)(png_structp,png_bytep*);
static void (*_png_read_end)(png_structp,png_infop);

#ifndef RTLD_GLOBAL		/* OSF on Alpha doesn't define this */
# define RTLD_GLOBAL 0
#endif

static int loadpng() {
    /* Oops someone might have libpng without libz. If we try to load libpng */
    /*  first we crash and burn horribly, so... */
    if ( dlopen("libz" SO_EXT,RTLD_GLOBAL|RTLD_LAZY)==NULL
#ifdef SO_1_EXT
	    && dlopen("libz" SO_1_EXT,RTLD_LAZY)==NULL
#endif
	    ) {
	fprintf(stderr,"libz: %s\n", dlerror());
return( 0 );
    }

#  if !defined(_LIBPNG12)
    libpng = dlopen("libpng" SO_EXT,RTLD_LAZY);
#    ifdef SO_2_EXT
    if ( libpng==NULL )
	libpng = dlopen("libpng" SO_2_EXT,RTLD_LAZY);
#    endif
#  else		/* After version 1.2.1 (I think) dynamic libpng is called libpng12 */
    libpng = dlopen("libpng12" SO_EXT,RTLD_LAZY);
#    ifdef SO_0_EXT
    if ( libpng==NULL )
	libpng = dlopen("libpng12" SO_0_EXT,RTLD_LAZY);
#    endif
#  endif
    if ( libpng==NULL ) {
	fprintf(stderr,"libpng: %s\n", dlerror());
return( 0 );
    }
    _png_create_read_struct = (png_structp (*)(char *, png_voidp, png_error_ptr, png_error_ptr)) dlsym(libpng,"png_create_read_struct");
    _png_create_info_struct = (png_infop (*)(png_structp)) dlsym(libpng,"png_create_info_struct");
    _png_destroy_read_struct = (void (*)(png_structpp, png_infopp, png_infopp)) dlsym(libpng,"png_destroy_read_struct");
    _png_init_io = (void (*)(png_structp, FILE *)) dlsym(libpng,"png_init_io");
    _png_read_info = (void (*)(png_structp, png_infop)) dlsym(libpng,"png_read_info");
    _png_set_strip_16 = (void (*)(png_structp)) dlsym(libpng,"png_set_strip_16");
    _png_set_strip_alpha = (void (*)(png_structp)) dlsym(libpng,"png_set_strip_alpha");
    _png_set_packing = (void (*)(png_structp)) dlsym(libpng,"png_set_packing");
    _png_set_filler = (void (*)(png_structp,png_uint_32,int)) dlsym(libpng,"png_set_filler");
    _png_read_image = (void (*)(png_structp,png_bytep*)) dlsym(libpng,"png_read_image");
    _png_read_end = (void (*)(png_structp,png_infop)) dlsym(libpng,"png_read_end");
    if ( _png_create_read_struct && _png_create_info_struct && _png_destroy_read_struct &&
	    _png_init_io && _png_read_info && _png_set_strip_16 && _png_set_packing &&
	    _png_set_filler && _png_read_image && _png_read_end &&
	    _png_set_strip_alpha )
return( 1 );
    dlclose(libpng);
    fprintf(stderr,"%s\n", dlerror());
return( 0 );
}
# else
#  define _png_create_read_struct png_create_read_struct
#  define _png_create_info_struct png_create_info_struct
#  define _png_destroy_read_struct png_destroy_read_struct
#  define _png_init_io png_init_io
#  define _png_read_info png_read_info
#  define _png_set_strip_16 png_set_strip_16
#  define _png_set_strip_alpha png_set_strip_alpha
#  define _png_set_packing png_set_packing
#  define _png_set_filler png_set_filler
#  define _png_read_image png_read_image
#  define _png_read_end png_read_end
static void *libpng=(void *) 1;

static int loadpng() { return true; }
# endif
 
static void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    fprintf(stderr,"%s\n", error_msg);
    longjmp(png_ptr->jmpbuf,1);
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
    fprintf(stderr,"%s\n", warning_msg);
}

GImage *GImageRead_Png(FILE *fp) {
    GImage *ret=NULL;
    struct _GImage *base;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers=NULL;
    int i;

    if ( libpng==NULL )
	if ( !loadpng())
return( NULL );

   png_ptr = _png_create_read_struct(PNG_LIBPNG_VER_STRING,
      (void *)NULL, user_error_fn, user_warning_fn);

   if (!png_ptr)
return( NULL );

    info_ptr = _png_create_info_struct(png_ptr);
    if (!info_ptr) {
      _png_destroy_read_struct(&png_ptr,  (png_infopp)NULL, (png_infopp)NULL);
return( NULL );
    }

    if (setjmp(png_ptr->jmpbuf)) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      _png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      if ( ret!=NULL ) {
	  GImageDestroy(ret);
	  gfree(row_pointers);
      }
      /* If we get here, we had a problem reading the file */
return( NULL );
    }

    _png_init_io(png_ptr, fp);
    _png_read_info(png_ptr, info_ptr);
    _png_set_strip_16(png_ptr);
    if ( (info_ptr->color_type==PNG_COLOR_TYPE_GRAY || info_ptr->color_type==PNG_COLOR_TYPE_PALETTE ) &&
	    info_ptr->bit_depth == 1 )
	/* Leave bitmaps packed */;
    else
	_png_set_packing(png_ptr);
    if ( info_ptr->color_type==PNG_COLOR_TYPE_GRAY_ALPHA )
	_png_set_strip_alpha(png_ptr);
    if (info_ptr->color_type == PNG_COLOR_TYPE_RGB)
	_png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

    if ( info_ptr->color_type==PNG_COLOR_TYPE_GRAY && info_ptr->bit_depth == 1 ) {
	ret = GImageCreate(it_mono,info_ptr->width,info_ptr->height);
    } else if ( info_ptr->color_type==PNG_COLOR_TYPE_GRAY || info_ptr->color_type==PNG_COLOR_TYPE_GRAY_ALPHA ) {
	GClut *clut;
	ret = GImageCreate(it_index,info_ptr->width,info_ptr->height);
	clut = ret->u.image->clut;
	clut->is_grey = true;
	clut->clut_len = 256;
	for ( i=0; i<256; ++i )
	    clut->clut[i] = COLOR_CREATE(i,i,i);
    } else if ( info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA ) {
	ret = GImageCreate(it_rgba,info_ptr->width,info_ptr->height);
    } else if ( info_ptr->color_type==PNG_COLOR_TYPE_RGB || info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA )
	ret = GImageCreate(it_true,info_ptr->width,info_ptr->height);
    else {
	GClut *clut;
	ret = GImageCreate(info_ptr->bit_depth != 1? it_index : it_mono,
		info_ptr->width,info_ptr->height);
	clut = ret->u.image->clut;
	if ( clut==NULL )
	    clut = ret->u.image->clut = gcalloc(1,sizeof(GClut));
	clut->is_grey = true;
	clut->clut_len = info_ptr->num_palette;
	for ( i=0; i<info_ptr->num_palette; ++i )
	    clut->clut[i] = COLOR_CREATE(info_ptr->palette[i].red,
			info_ptr->palette[i].green,
			info_ptr->palette[i].blue);
    }
    base = ret->u.image;
    if ( (info_ptr->valid&PNG_INFO_tRNS) && info_ptr->num_trans>0 ) {
	if ( info_ptr->color_type==PNG_COLOR_TYPE_RGB || info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA )
	    base->trans = COLOR_CREATE(
		    (info_ptr->trans_values.red>>8),
		    (info_ptr->trans_values.green>>8),
		    (info_ptr->trans_values.blue>>8));
	else if ( base->image_type == it_mono )
	    base->trans = info_ptr->trans[0];
	else
	    base->clut->trans_index = base->trans = info_ptr->trans[0];
    }

    row_pointers = galloc(info_ptr->height*sizeof(png_bytep));
    for ( i=0; i<info_ptr->height; ++i )
	row_pointers[i] = (png_bytep) (base->data + i*base->bytes_per_line);

    /* Ignore progressive loads for now */
    /* libpng wants me to do it with callbacks, but that doesn't sit well */
    /*  with my wish to be in control... */
    _png_read_image(png_ptr,row_pointers);
    _png_read_end(png_ptr, NULL);

    if ( info_ptr->color_type==PNG_COLOR_TYPE_RGB || info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA ) {
	/* PNG orders its bytes as AABBGGRR instead of 00RRGGBB */
	uint32 *ipt, *iend;
	for ( ipt = (uint32 *) (base->data), iend=ipt+base->width*base->height; ipt<iend; ++ipt ) {
#if 0
	    /* Minimal support for alpha channel. Assume default background of white */
	    if ( __gimage_can_support_alpha )
		/* Leave alpha channel unchanged */;
	    else if ( (*ipt&0xff000000)==0xff000000 )
		*ipt = COLOR_CREATE( ((*ipt)&0xff) , ((*ipt>>8)&0xff) , (*ipt>>16)&0xff );
	    else {
		int r, g, b, a = (*ipt>>24)&0xff;
		r = ( ((*ipt    )&0xff) * a + (255-a)*0xff ) / 255;
		g = ( ((*ipt>>8 )&0xff) * a + (255-a)*0xff ) / 255;
		b = ( ((*ipt>>16)&0xff) * a + (255-a)*0xff ) / 255;
		*ipt = COLOR_CREATE( r,g,b );
	    }
#else
	    uint32 r, g, b, a = *ipt&0xff000000;
	    r = (*ipt    )&0xff;
	    g = (*ipt>>8 )&0xff;
	    b = (*ipt>>16)&0xff;
	    *ipt = COLOR_CREATE( r,g,b ) | a;
#endif
	}
    }

    _png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    gfree(row_pointers);
    /* Note png b&w images come out as indexed */
return( ret );
}

GImage *GImageReadPng(char *filename) {
    GImage *ret=NULL;
    FILE *fp;

    fp = fopen(filename, "rb");
    if (!fp)
return( NULL );

    ret = GImageRead_Png(fp);
    fclose(fp);
return( ret );
}
#endif
