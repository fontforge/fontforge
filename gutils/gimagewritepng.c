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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef _NO_LIBPNG
static int a_file_must_define_something=0;	/* ANSI says so */
#elif !defined(_STATIC_LIBPNG) && !defined(NODYNAMIC) /* I don't know how to deal with dynamic libs on mac OS/X, hence this */
#include <dynamic.h>
# include <png.h>

#define int32 _int32
#define uint32 _uint32
#define int16 _int16
#define uint16 _uint16
#define int8 _int8
#define uint8 _uint8

#include "gimage.h"

static DL_CONST void *libpng=NULL;
static png_structp (*_png_create_write_struct)(char *, png_voidp, png_error_ptr, png_error_ptr);
static png_infop (*_png_create_info_struct)(png_structp);
static void (*_png_destroy_write_struct)(png_structpp, png_infopp);
static void (*_png_init_io)(png_structp, FILE *);
static void (*_png_write_info)(png_structp, png_infop);
static void (*_png_set_packing)(png_structp);
static void (*_png_set_filler)(png_structp,png_uint_32,int);
static void (*_png_write_image)(png_structp,png_bytep*);
static void (*_png_write_end)(png_structp,png_infop);
#if (PNG_LIBPNG_VER >= 10500)
static void (*_png_longjmp)(png_structp, int);
static jmp_buf* (*_png_set_longjmp_fn)(png_structp,png_longjmp_ptr, size_t);
#endif
static void (*_png_set_IHDR)(png_structp,png_infop,png_uint_32,png_uint_32,int,int,int,int,int);
static void (*_png_set_PLTE)(png_structp,png_infop,png_colorp,int);
static void (*_png_set_tRNS)(png_structp,png_infop,png_bytep,int,png_color_16p);

static int loadpng() {
    /* Oops someone might have libpng without libz. If we try to load libpng */
    /*  first we crash and burn horribly, so... */
    if ( dlopen("libz" SO_EXT,RTLD_GLOBAL|RTLD_LAZY)==NULL
#ifdef SO_1_EXT
	    && dlopen("libz" SO_1_EXT,RTLD_LAZY)==NULL
#endif
	    ) {
	fprintf(stderr,"%s\n", dlerror());
return( 0 );
    }
#  if !defined(PNG_LIBPNG_VER_MAJOR) || (PNG_LIBPNG_VER_MAJOR==1 && PNG_LIBPNG_VER_MINOR<2)
/* Early versions are called libpng. Later libpng10/libpng12/libpng14... */
    libpng = dlopen("libpng" SO_EXT,RTLD_LAZY);
#    ifdef SO_2_EXT
    if ( libpng==NULL )
	libpng = dlopen("libpng" SO_2_EXT,RTLD_LAZY);
#    endif
#  else		/* After version 1.2.1 (I think) dynamic libpng is called libpng12/libpng14... */
#    define xstr(s) str(s)
#    define str(s) #s
#    define PNGLIBNAME	"libpng" xstr(PNG_LIBPNG_VER_MAJOR) xstr(PNG_LIBPNG_VER_MINOR)
    libpng = dlopen(PNGLIBNAME SO_EXT,RTLD_LAZY);
#    ifdef SO_0_EXT
    if ( libpng==NULL )
	libpng = dlopen(PNGLIBNAME SO_0_EXT,RTLD_LAZY);
#    endif
    if ( libpng==NULL ) {
	libpng = dlopen("libpng" SO_EXT,RTLD_LAZY);
#    ifdef SO_2_EXT
	if ( libpng==NULL )
	    libpng = dlopen("libpng" SO_2_EXT,RTLD_LAZY);
#    endif
    }
#  endif
    if ( libpng==NULL ) {
	fprintf(stderr,"%s", dlerror());
return( 0 );
    }
    _png_create_write_struct = (png_structp (*)(char *, png_voidp, png_error_ptr, png_error_ptr))
	    dlsym(libpng,"png_create_write_struct");
    _png_create_info_struct = (png_infop (*)(png_structp))
	    dlsym(libpng,"png_create_info_struct");
    _png_destroy_write_struct = (void (*)(png_structpp, png_infopp))
	    dlsym(libpng,"png_destroy_write_struct");
    _png_init_io = (void (*)(png_structp, FILE *))
	    dlsym(libpng,"png_init_io");
    _png_write_info = (void (*)(png_structp, png_infop))
	    dlsym(libpng,"png_write_info");
    _png_set_packing = (void (*)(png_structp))
	    dlsym(libpng,"png_set_packing");
    _png_set_filler = (void (*)(png_structp,png_uint_32,int))
	    dlsym(libpng,"png_set_filler");
    _png_write_image = (void (*)(png_structp,png_bytep*))
	    dlsym(libpng,"png_write_image");
    _png_write_end = (void (*)(png_structp,png_infop))
	    dlsym(libpng,"png_write_end");
#if (PNG_LIBPNG_VER >= 10500)
    _png_longjmp = (void (*)(png_structp, int))
	    dlsym(libpng,"png_longjmp");
    _png_set_longjmp_fn = (jmp_buf* (*)(png_structp,png_longjmp_ptr,size_t))
	    dlsym(libpng,"png_set_longjmp_fn");
#endif
    _png_set_IHDR = (void (*)(png_structp,png_infop,png_uint_32,png_uint_32,int,int,int,int,int))
	    dlsym(libpng,"png_set_IHDR");
    _png_set_PLTE = (void (*)(png_structp,png_infop,png_colorp,int))
	    dlsym(libpng,"png_set_PLTE");
    _png_set_tRNS = (void (*)(png_structp,png_infop,png_bytep,int,png_color_16p))
	    dlsym(libpng,"png_set_tRNS");
    if ( _png_create_write_struct && _png_create_info_struct && _png_destroy_write_struct &&
	    _png_init_io && _png_set_filler && _png_write_info && _png_set_packing &&
	    _png_write_image && _png_write_end)
return( 1 );
    dlclose(libpng);
    fprintf(stderr,"%s", dlerror());
return( 0 );
}

static void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    fprintf(stderr,"%s", error_msg );
#if (PNG_LIBPNG_VER < 10500)
    longjmp(png_ptr->jmpbuf,1);
#else
    _png_longjmp (png_ptr, 1);
#endif
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
    fprintf(stderr,"%s\n", warning_msg);
}

int GImageWrite_Png(GImage *gi, FILE *fp, int progressive) {
    struct _GImage *base = gi->list_len==0?gi->u.image:gi->u.images[0];
    png_structp png_ptr;
    png_infop info_ptr;
    png_byte **rows;
    int i;
    int bit_depth;
    int color_type;
    int num_palette;
    png_bytep trans_alpha = NULL;
    png_color_16p trans_color = NULL;
    png_colorp palette = NULL;

    if ( libpng==NULL )
	if ( !loadpng())
return( false );

   png_ptr = _png_create_write_struct(PNG_LIBPNG_VER_STRING,
      (void *)NULL, user_error_fn, user_warning_fn);

   if (!png_ptr) {
return(false);
   }

   info_ptr = _png_create_info_struct(png_ptr);
   if (!info_ptr) {
      _png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
return(false);
   }

#if (PNG_LIBPNG_VER < 10500)
    if (setjmp(png_ptr->jmpbuf))
#else
    if (setjmp(*_png_set_longjmp_fn(png_ptr, longjmp, sizeof (jmp_buf))))
#endif
    {
	_png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
return(false);
    }

   _png_init_io(png_ptr, fp);

   bit_depth = 8;
   num_palette = base->clut==NULL?2:base->clut->clut_len;
   if ( base->image_type==it_index || base->image_type==it_bitmap ) {
       color_type = PNG_COLOR_TYPE_PALETTE;
       if ( num_palette<=2 )
	   bit_depth=1;
       else if ( num_palette<=4 )
	   bit_depth=2;
       else if ( num_palette<=16 )
	   bit_depth=4;
   } else {
       color_type = PNG_COLOR_TYPE_RGB;
       if ( base->image_type == it_rgba )
	   color_type = PNG_COLOR_TYPE_RGB_ALPHA;
   }

   _png_set_IHDR(png_ptr, info_ptr, base->width, base->height,
		 bit_depth, color_type, progressive,
		 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
   if ( base->image_type==it_index || base->image_type==it_bitmap ) {
       palette = (png_color *) galloc(num_palette*sizeof(png_color));
       if ( base->clut==NULL ) {
	    palette[0].red = palette[0].green = palette[0].blue = 0;
	    palette[1].red = palette[1].green = palette[1].blue = 0xff;
       } else {
	   for ( i=0; i<num_palette; ++i ) {
		long col = base->clut->clut[i];
		palette[i].red = COLOR_RED(col);
		palette[i].green = COLOR_GREEN(col);
		palette[i].blue = COLOR_BLUE(col);
	   }
       }
       _png_set_PLTE(png_ptr, info_ptr, palette, num_palette);
       if ( num_palette<=16 )
	   _png_set_packing(png_ptr);

       if ( base->trans!=-1 ) {
	   trans_alpha = galloc(1);
	   trans_alpha[0] = base->trans;
       }
   } else {
       if ( base->trans!=-1 ) {
	   trans_color = galloc(sizeof(png_color_16));
	   trans_color->red = COLOR_RED(base->trans);
	   trans_color->green = COLOR_GREEN(base->trans);
	   trans_color->blue = COLOR_BLUE(base->trans);
       }
   }
   if ( base->trans!=-1 ) {
       _png_set_tRNS(png_ptr, info_ptr, trans_alpha, 1, trans_color);
   }
   _png_write_info(png_ptr, info_ptr);

    if (color_type == PNG_COLOR_TYPE_RGB)
	_png_set_filler(png_ptr, '\0', PNG_FILLER_BEFORE);

    rows = galloc(base->height*sizeof(png_byte *));
    for ( i=0; i<base->height; ++i )
	rows[i] = (png_byte *) (base->data + i*base->bytes_per_line);

    _png_write_image(png_ptr,rows);

    _png_write_end(png_ptr, info_ptr);

    if ( trans_alpha!=NULL ) gfree(trans_alpha);
    if ( trans_color!=NULL ) gfree(trans_color);
    if ( palette!=NULL ) gfree(palette);
    _png_destroy_write_struct(&png_ptr, &info_ptr);
    gfree(rows);
return( 1 );
}

int GImageWritePng(GImage *gi, char *filename, int progressive) {
    FILE *fp;
    int ret;

    if ( libpng==NULL ) {
	if ( !loadpng())
return( false );
    }
    fp = fopen(filename, "wb");
    if (!fp)
return(false);
    ret = GImageWrite_Png(gi,fp,progressive);
    fclose(fp);
return( ret );
}
#else
#include <png.h>

#define int32 _int32
#define uint32 _uint32
#define int16 _int16
#define uint16 _uint16
#define int8 _int8
#define uint8 _uint8

#include "gimage.h"

static void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    fprintf(stderr, "%s\n", error_msg );
#if (PNG_LIBPNG_VER < 10500)
    longjmp(png_ptr->jmpbuf,1);
#else
    png_longjmp (png_ptr, 1);
#endif
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
    fprintf(stderr,"%s\n", warning_msg);
}

int GImageWrite_Png(GImage *gi, FILE *fp, int progressive) {
    struct _GImage *base = gi->list_len==0?gi->u.image:gi->u.images[0];
    png_structp png_ptr;
    png_infop info_ptr;
    png_byte **rows;
    int i;
    int bit_depth;
    int color_type;
    int num_palette;
    png_bytep trans_alpha = NULL;
    png_color_16p trans_color = NULL;
    png_colorp palette = NULL;

   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      (void *)NULL, user_error_fn, user_warning_fn);

   if (!png_ptr) {
return(false);
   }

   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
return(false);
   }

#if (PNG_LIBPNG_VER < 10500)
    if (setjmp(png_ptr->jmpbuf))
#else
   if (setjmp(*png_set_longjmp_fn(png_ptr, longjmp, sizeof (jmp_buf))))
#endif
   {
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
return(false);
   }

   png_init_io(png_ptr, fp);

   bit_depth = 8;
   num_palette = base->clut==NULL?2:base->clut->clut_len;
   if ( base->image_type==it_index || base->image_type==it_bitmap ) {
       color_type = PNG_COLOR_TYPE_PALETTE;
       if ( num_palette<=2 )
	   bit_depth=1;
       else if ( num_palette<=4 )
	   bit_depth=2;
       else if ( num_palette<=16 )
	   bit_depth=4;
   } else {
       color_type = PNG_COLOR_TYPE_RGB;
       if ( base->image_type == it_rgba )
	   color_type = PNG_COLOR_TYPE_RGB_ALPHA;
   }

   png_set_IHDR(png_ptr, info_ptr, base->width, base->height,
		bit_depth, color_type, progressive,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
   if ( base->image_type==it_index || base->image_type==it_bitmap ) {
       palette = (png_color *) galloc(num_palette*sizeof(png_color));
       if ( base->clut==NULL ) {
	    palette[0].red = palette[0].green = palette[0].blue = 0;
	    palette[1].red = palette[1].green = palette[1].blue = 0xff;
       } else {
	   for ( i=0; i<num_palette; ++i ) {
		long col = base->clut->clut[i];
		palette[i].red = COLOR_RED(col);
		palette[i].green = COLOR_GREEN(col);
		palette[i].blue = COLOR_BLUE(col);
	   }
       }
       png_set_PLTE(png_ptr, info_ptr, palette, num_palette);
       if ( num_palette<=16 )
	   png_set_packing(png_ptr);

       if ( base->trans!=-1 ) {
	  trans_alpha = galloc(1);
	  trans_alpha[0] = base->trans;
       }
   } else {
       if ( base->trans!=-1 ) {
	   trans_color = galloc(sizeof(png_color_16));
	   trans_color->red = COLOR_RED(base->trans);
	   trans_color->green = COLOR_GREEN(base->trans);
	   trans_color->blue = COLOR_BLUE(base->trans);
       }
   }
   if ( base->trans!=-1 ) {
       png_set_tRNS(png_ptr, info_ptr, trans_alpha, 1, trans_color);
   }
   png_write_info(png_ptr, info_ptr);

    if (color_type == PNG_COLOR_TYPE_RGB)
	png_set_filler(png_ptr, '\0', PNG_FILLER_BEFORE);

    rows = galloc(base->height*sizeof(png_byte *));
    for ( i=0; i<base->height; ++i )
	rows[i] = (png_byte *) (base->data + i*base->bytes_per_line);

    png_write_image(png_ptr,rows);

    png_write_end(png_ptr, info_ptr);

    if ( trans_alpha!=NULL ) gfree(trans_alpha);
    if ( trans_color!=NULL ) gfree(trans_color);
    if ( palette!=NULL ) gfree(palette);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    gfree(rows);
return( 1 );
}

int GImageWritePng(GImage *gi, char *filename, int progressive) {
    FILE *fp;
    int ret;

   /* open the file */
   fp = fopen(filename, "wb");
   if (!fp)
return(false);
    ret = GImageWrite_Png(gi,fp,progressive);
    fclose(fp);
return( ret );
}
#endif
