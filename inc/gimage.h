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

#ifndef FONTFORGE_GIMAGE_H
#define FONTFORGE_GIMAGE_H

#include "basics.h"

typedef uint32_t Color;

#define COLOR_UNKNOWN		((Color) 0xffffffff)
#define COLOR_TRANSPARENT	((Color) 0xffffffff)
#define COLOR_DEFAULT		((Color) 0xfffffffe)
#define COLOR_WARNING		((Color) 0xfffffffd)
#define COLOR_WHITE		((Color) 0xffffff)
#define COLOR_CREATE(r,g,b)	(((r)<<16) | ((g)<<8) | (b))
#define COLOR_ALPHA(col)	(((col)>>24))
#define COLOR_RED(col)		(((col)>>16) & 0xff)
#define COLOR_GREEN(col)	(((col)>>8) & 0xff)
#define COLOR_BLUE(col)		((col)&0xff)

struct hslrgb {
    double h,s,l,v;
    double r,g,b;
    uint8_t rgb, hsl, hsv;
};

struct hslrgba {
    double h,s,l,v;
    double r,g,b;
    uint8_t rgb, hsl, hsv, has_alpha;
    double alpha;
};

typedef struct clut {
    int16_t clut_len;
    unsigned int is_grey: 1;
    uint32_t trans_index;		/* will be ignored for cluts in images, use base->trans instead */
    Color clut[256];
} GClut;

#define GCLUT_CLUT_EMPTY \
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
}


typedef struct revcmap RevCMap;

enum image_type { it_mono, it_bitmap=it_mono, it_index, it_true, it_rgba };

struct _GImage {
/* Format: bitmaps are stored with the most significant bit first in byte units
	    indexed    images are stored in byte units
	    true color images are stored in 4 byte units, 0,red,green,blue
	    rgba       images are stored in 4 byte units, alpha,red,green blue
*/
    enum image_type image_type: 2;
    int16_t delay;		/* for animated GIFs, delay to next frame */
    int32_t width, height;
    int32_t bytes_per_line;
    uint8_t *data;
    GClut *clut;
    Color trans;		/* PNG supports more than one transparent color, we don't */
				/* for non-true color images this is the index, not a color */
};

/* We deal with 1 bit, 8 bit and 32 bit images internal. 1 bit images may have*/
/*  a clut (if they don't assume bw, 0==black, 1==white), 8 bit must have a */
/*  clut, 32bit are actually 24 bit RGB images, but we pad them for easy */
/*  accessing. it_screen means that we've got an image that can be drawn */
/*  directly on the screen */
typedef struct gimage {
    short list_len;		/* length of list */
    union {			/* depends on whether has_list is set */
	struct _GImage *image;
    	struct _GImage **images;
    } u;
    void *userdata;
} GImage;

enum pastetrans_type { ptt_paste_trans_to_trans, ptt_old_shines_through};

typedef struct grect {
    int32_t x,y,width,height;
} GRect;

#define GRECT_EMPTY { 0, 0, 0, 0 }


typedef struct gpoint {
    int16_t x,y;
} GPoint;

#define GPOINT_EMPTY { 0, 0 }

#ifdef __cplusplus
extern "C" {
#endif

extern GImage *GImageCreate(enum image_type type, int32_t width, int32_t height);
extern GImage *_GImage_Create(enum image_type type, int32_t width, int32_t height);
extern void GImageDestroy(GImage *gi);
extern GImage *GImageCreateAnimation(GImage **images, int n);
extern GImage *GImageAddImageBefore(GImage *dest, GImage *src, int pos);

extern Color GImageGetPixelRGBA(GImage *base,int x, int y);
extern int GImageGetWidth(GImage *);
extern int GImageGetHeight(GImage *);
extern void *GImageGetUserData(GImage *img);
extern void GImageSetUserData(GImage *img,void *userdata);
extern RevCMap *GClutReverse(GClut *clut,int side_size);
void GClut_RevCMapFree(RevCMap *rev);
extern GClut *GImageFindCLUT(GImage *image,GClut *clut,int clutmax);
extern int GImageGreyClut(GClut *clut);
extern Color _GImage_ColourFName(char *name);
extern Color GDrawColorDarken(Color col, int by);
extern Color GDrawColorBrighten(Color col, int by);

extern int GImageWriteGImage(GImage *gi, char *filename);
extern int GImageWrite_Bmp(GImage *gi, FILE *fp);
extern int GImageWriteBmp(GImage *gi, char *filename);
extern GImage *GImageRead_Bmp(FILE *file);
extern GImage *GImageReadBmp(char *filename);
extern int GImageWriteXbm(GImage *gi, char *filename);
extern GImage *GImageReadXbm(char *filename);
extern int GImageWriteXpm(GImage *gi, char *filename);
extern GImage *GImageReadXpm(char *filename);
extern GImage *GImageReadTiff(char *filename);
extern GImage *GImageReadJpeg(char *filename);
extern GImage *GImageRead_Jpeg(FILE *fp);
extern int GImageWrite_Jpeg(GImage *gi, FILE *outfile, int quality, int progressive);
extern int GImageWriteJpeg(GImage *gi, char *filename, int quality, int progressive);
extern GImage *GImageRead_Png(FILE *fp);
extern GImage *GImageReadPng(char *filename);
extern GImage *GImageReadPngBuf(char* buf, size_t sz);
extern int GImageWrite_Png(GImage *gi, FILE *fp, int progressive);
extern int GImageWritePng(GImage *gi, char *filename, int progressive);
extern int GImageWritePngBuf(GImage *gi, char** buf, size_t* sz, int compression_level, int progressive);
extern GImage *GImageReadGif(char *filename);
extern int GImageWriteGif(GImage *gi,char *filename,int progressive);
extern GImage *GImageReadRas(char *filename);		/* Sun Raster */
extern GImage *GImageReadRgb(char *filename);		/* SGI */
extern GImage *GImageRead(char *filename);

extern void GImageDrawRect(GImage *img,GRect *r,Color col);
extern void GImageDrawImage(GImage *dest,GImage *src,GRect *junk,int x, int y);
extern void GImageBlendOver(GImage *dest,GImage *src,GRect *from,int x, int y);

extern void gRGB2HSL(struct hslrgb *col);
extern void gHSL2RGB(struct hslrgb *col);
extern void gRGB2HSV(struct hslrgb *col);
extern void gHSV2RGB(struct hslrgb *col);
extern void gColor2Hslrgb(struct hslrgb *col,Color from);
extern void gColor2Hslrgba(struct hslrgba *col,Color from);
extern Color gHslrgb2Color(struct hslrgb *col);
extern Color gHslrgba2Color(struct hslrgba *col);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_GIMAGE_H */
