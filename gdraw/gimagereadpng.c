/* Copyright (C) 2000,2001 by George Williams */
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

#ifdef _NO_LIBPNG
static int a_file_must_define_something=0;	/* ANSI says so */
#else
#include <dlfcn.h>
#include <png.h>

#define int32 _int32
#define uint32 _uint32
#define int16 _int16
#define uint16 _uint16
#define int8 _int8
#define uint8 _uint8

#include "gdraw.h"

static void *libpng=NULL;
static png_structp (*_png_create_read_struct)(char *, png_voidp, png_error_ptr, png_error_ptr);
static png_infop (*_png_create_info_struct)(png_structp);
static void (*_png_destroy_read_struct)(png_structpp, png_infopp, png_infopp);
static void (*_png_init_io)(png_structp, FILE *);
static void (*_png_read_info)(png_structp, png_infop);
static void (*_png_set_strip_16)(png_structp);
static void (*_png_set_packing)(png_structp);
static void (*_png_set_filler)(png_structp,png_uint_32,int);
static void (*_png_set_gray_to_rgb)(png_structp);
static void (*_png_read_image)(png_structp,png_bytep*);
static void (*_png_read_end)(png_structp,png_infop);

static int loadpng() {
    libpng = dlopen("libpng.so",RTLD_LAZY);
    if ( libpng==NULL ) {
	GDrawIError("%s", dlerror());
return( 0 );
    }
    _png_create_read_struct = dlsym(libpng,"png_create_read_struct");
    _png_create_info_struct = dlsym(libpng,"png_create_info_struct");
    _png_destroy_read_struct = dlsym(libpng,"png_destroy_read_struct");
    _png_init_io = dlsym(libpng,"png_init_io");
    _png_read_info = dlsym(libpng,"png_read_info");
    _png_set_strip_16 = dlsym(libpng,"png_set_strip_16");
    _png_set_packing = dlsym(libpng,"png_set_packing");
    _png_set_filler = dlsym(libpng,"png_set_filler");
    _png_set_gray_to_rgb = dlsym(libpng,"png_set_gray_to_rgb");
    _png_read_image = dlsym(libpng,"png_read_image");
    _png_read_end = dlsym(libpng,"png_read_end");
    if ( _png_create_read_struct && _png_create_info_struct && _png_destroy_read_struct &&
	    _png_init_io && _png_read_info && _png_set_strip_16 && _png_set_packing &&
	    _png_set_filler && _png_set_gray_to_rgb && _png_read_image && _png_read_end)
return( 1 );
    dlclose(libpng);
    GDrawIError("%s", dlerror());
return( 0 );
}
 
static void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    GDrawError( error_msg );
    longjmp(png_ptr->jmpbuf,1);
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
    fprintf(stderr,"%s\n", warning_msg);
}
   
GImage *GImageReadPng(char *filename) {
    GImage *ret=NULL;
    struct _GImage *base;
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers=NULL;
    int i;

    if ( libpng==NULL )
	if ( !loadpng())
return( NULL );

    fp = fopen(filename, "rb");
    if (!fp)
return( NULL );

   png_ptr = _png_create_read_struct(PNG_LIBPNG_VER_STRING,
      (void *)NULL, user_error_fn, user_warning_fn);

   if (!png_ptr) {
      fclose(fp);
return( NULL );
   }

    info_ptr = _png_create_info_struct(png_ptr);
    if (!info_ptr) {
      fclose(fp);
      _png_destroy_read_struct(&png_ptr,  (png_infopp)NULL, (png_infopp)NULL);
return( NULL );
    }

    if (setjmp(png_ptr->jmpbuf)) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      _png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      fclose(fp);
      if ( ret!=NULL ) {
	  GImageDestroy(ret);
	  gfree(row_pointers);
      }
      /* If we get here, we had a problem reading the file */
return( NULL );
    }

    _png_init_io(png_ptr, fp);
    _png_read_info(png_ptr, info_ptr);
    if (info_ptr->bit_depth == 16)
	_png_set_strip_16(png_ptr);
    else if (info_ptr->bit_depth < 8)
	_png_set_packing(png_ptr);
    if (info_ptr->color_type == PNG_COLOR_TYPE_RGB)
	_png_set_filler(png_ptr, '\0', PNG_FILLER_BEFORE);
    if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
         _png_set_gray_to_rgb(png_ptr);	
    /* Can I remove alpha channels somehow? */
    /* I hope I've stuffed them into the 4th (unused) colour byte where they */
    /*  can happily be ignored */

    if ( info_ptr->color_type==PNG_COLOR_TYPE_GRAY || info_ptr->color_type==PNG_COLOR_TYPE_GRAY_ALPHA ) {
	GClut *clut;
	ret = GImageCreate(it_index,info_ptr->width,info_ptr->height);
	clut = ret->u.image->clut;
	clut->is_grey = true;
	clut->clut_len = 256;
	for ( i=0; i<256; ++i )
	    clut->clut[i] = COLOR_CREATE(i,i,i);
    } else if ( info_ptr->color_type==PNG_COLOR_TYPE_RGB || info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA )
	ret = GImageCreate(it_true,info_ptr->width,info_ptr->height);
    else {
	GClut *clut;
	ret = GImageCreate(it_index,info_ptr->width,info_ptr->height);
	clut = ret->u.image->clut;
	clut->is_grey = true;
	clut->clut_len = info_ptr->num_palette;
	for ( i=0; i<info_ptr->num_palette; ++i )
	    clut->clut[i] = COLOR_CREATE(info_ptr->palette[i].red,
			info_ptr->palette[i].green,
			info_ptr->palette[i].blue);
    }
    base = ret->u.image;
    if ( info_ptr->color_type==PNG_COLOR_TYPE_RGB || info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA ) {
	if ( (info_ptr->valid&PNG_INFO_tRNS) && info_ptr->num_trans>0 )
	    base->trans = COLOR_CREATE(
		    (info_ptr->trans_values.red>>8),
		    (info_ptr->trans_values.green>>8),
		    (info_ptr->trans_values.blue>>8));
    } else {
	if ( (info_ptr->valid&PNG_INFO_tRNS) && info_ptr->num_trans>0 )
	    base->clut->trans_index = base->trans = info_ptr->trans[0];
    }

    row_pointers = galloc(info_ptr->height*sizeof(png_bytep));
    for ( i=0; i<info_ptr->height; ++i )
	row_pointers[i] = (unsigned char *) (base->data + i*base->bytes_per_line);

    /* Ignore progressive loads for now */
    /* libpng wants me to do it with callbacks, but that doesn't sit well */
    /*  with our asynchronous cruds... */
    _png_read_image(png_ptr,row_pointers);
    _png_read_end(png_ptr, NULL);
    _png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    gfree(row_pointers);
return( ret );
}
#endif
