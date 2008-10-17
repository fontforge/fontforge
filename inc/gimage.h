/* Copyright (C) 2000-2008 by George Williams */
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
#ifndef _GIMAGE_H
#define _GIMAGE_H
#include "basics.h"

typedef uint32 Color;

#define COLOR_UNKNOWN		((Color) 0xffffffff)
#define COLOR_TRANSPARENT	((Color) 0xffffffff)
#define COLOR_DEFAULT		((Color) 0xfffffffe)
#define COLOR_CREATE(r,g,b)	(((r)<<16) | ((g)<<8) | (b))
#define COLOR_ALPHA(col)	(((col)>>24))
#define COLOR_RED(col)		(((col)>>16) & 0xff)
#define COLOR_GREEN(col)	(((col)>>8) & 0xff)
#define COLOR_BLUE(col)		((col)&0xff)

struct hslrgb {
    double h,s,l,v;
    double r,g,b;
    uint8 rgb, hsl, hsv;
};

typedef struct clut {
    int16 clut_len;
    unsigned int is_grey: 1;
    uint32 trans_index;		/* will be ignored for cluts in images, use base->trans instead */
    Color clut[256];
} GClut;

typedef struct revcmap RevCMap;

enum image_type { it_mono, it_bitmap=it_mono, it_index, it_true, it_rgba };

struct _GImage {
/* Format: bitmaps are stored with the most significant bit first in byte units
	    indexed    images are stored in byte units
	    true color images are stored in 4 byte units, 0,red,green,blue
	    rgba       images are stored in 4 byte units, alpha,red,green blue
*/
    unsigned int image_type: 2;
    int16 delay;		/* for animated GIFs, delay to next frame */
    int32 width, height;
    int32 bytes_per_line;
    uint8 *data;
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
    int32 x,y,width,height;
} GRect;

typedef struct gpoint {
    int16 x,y;
} GPoint;

extern GImage *GImageCreate(enum image_type type, int32 width, int32 height);
extern GImage *_GImage_Create(enum image_type type, int32 width, int32 height);
extern void GImageDestroy(GImage *gi);
extern GImage *GImageCreateAnimation(GImage **images, int n);
extern GImage *GImageAddImageBefore(GImage *dest, GImage *src, int pos);

extern GImage *GImageBaseGetSub(struct _GImage *base, enum image_type it, GRect *src, GClut *nclut, RevCMap *rev);
extern GImage *GImageGetSub(GImage *image,enum image_type it, GRect *src, GClut *nclut, RevCMap *rev);
extern int GImageInsertToBase(struct _GImage *tobase, GImage *from, GRect *src, RevCMap *rev,
	int to_x, int to_y, enum pastetrans_type ptt );
extern int GImageInsert(GImage *to, GImage *from, GRect *src, RevCMap *rev,
	int to_x, int to_y, enum pastetrans_type ptt );
extern Color _GImageGetPixelColor(struct _GImage *base,int x, int y);	/* Obsolete */
extern Color GImageGetPixelColor(GImage *base,int x, int y);		/* Obsolete */
extern Color GImageGetPixelRGBA(GImage *base,int x, int y);
extern int GImageGetWidth(GImage *);
extern int GImageGetHeight(GImage *);
extern void *GImageGetUserData(GImage *img);
extern void GImageSetUserData(GImage *img,void *userdata);
extern void GImageResize(struct _GImage *tobase, struct _GImage *fbase,
	GRect *src, RevCMap *rev);
extern GImage *GImageResize32(GImage *from, GRect *src, int width, int height, Color trans);
extern GImage *GImageResizeSame(GImage *from, GRect *src, int width, int height, RevCMap *rev);
extern RevCMap *GClutReverse(GClut *clut,int side_size);
void GClut_RevCMapFree(RevCMap *rev);
extern GClut *GImageFindCLUT(GImage *image,GClut *clut,int clutmax);
extern int GImageSameClut(GClut *clut,GClut *nclut);
extern int GImageGreyClut(GClut *clut);
extern Color GImageColourFName(unichar_t *name);
extern Color _GImage_ColourFName(char *name);
extern char *GImageNameFColour(Color col);
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
extern int GImageWriteEps(GImage *gi, char *filename);
extern GImage *GImageReadTiff(char *filename);
extern GImage *GImageReadJpeg(char *filename);
extern GImage *GImageRead_Jpeg(FILE *fp);
extern int GImageWrite_Jpeg(GImage *gi, FILE *outfile, int quality, int progressive);
extern int GImageWriteJpeg(GImage *gi, char *filename, int quality, int progressive);
extern GImage *GImageRead_Png(FILE *fp);
extern GImage *GImageReadPng(char *filename);
extern int GImageWrite_Png(GImage *gi, FILE *fp, int progressive);
extern int GImageWritePng(GImage *gi, char *filename, int progressive);
extern GImage *GImageReadGif(char *filename);
extern int GImageWriteGif(GImage *gi,char *filename,int progressive);
extern GImage *GImageReadRas(char *filename);		/* Sun Raster */
extern GImage *GImageReadRgb(char *filename);		/* SGI */
extern GImage *GImageRead(char *filename);

extern void GImageDrawRect(GImage *img,GRect *r,Color col);
extern void GImageDrawImage(GImage *dest,GImage *src,void *junk,int x, int y);

extern void gRGB2HSL(struct hslrgb *col);
extern void gHSL2RGB(struct hslrgb *col);
extern void gRGB2HSV(struct hslrgb *col);
extern void gHSV2RGB(struct hslrgb *col);
extern void gColor2Hslrgb(struct hslrgb *col,Color from);
extern Color gHslrgb2Color(struct hslrgb *col);

#endif
