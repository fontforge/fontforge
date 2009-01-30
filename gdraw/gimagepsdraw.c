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
#include "gpsdrawP.h"
#include "colorP.h"

static void InitFilter(GPSWindow ps) {
    ps->ascii85encode = 0;
    ps->ascii85n = 0;
    ps->ascii85bytes_per_line = 0;
}

static void Filter(GPSWindow ps,uint8 ch) {
    ps->ascii85encode = (ps->ascii85encode<<8) | ch;
    if ( ++ps->ascii85n == 4 ) {
	int c5, c4, c3, c2, c1;
	uint32 val = ps->ascii85encode;
	if ( val==0 ) {
	    putc('z',ps->output_file);
	    ps->ascii85n = 0;
	    if ( ++ps->ascii85bytes_per_line >= 76 ) {
		putc('\n',ps->output_file);
		ps->ascii85bytes_per_line = 0;
	    }
	} else {
	    c5 = val%85; val /= 85;
	    c4 = val%85; val /= 85;
	    c3 = val%85; val /= 85;
	    c2 = val%85;
	    c1 = val/85;
	    fprintf(ps->output_file, "%c%c%c%c%c",
		    c1+'!', c2+'!', c3+'!', c4+'!', c5+'!' );
	    ps->ascii85encode = 0;
	    ps->ascii85n = 0;
	    if (( ps->ascii85bytes_per_line+=5) >= 80 ) {
		putc('\n',ps->output_file);
		ps->ascii85bytes_per_line = 0;
	    }
	}
    }
}

static void FlushFilter(GPSWindow ps) {
    uint32 val = ps->ascii85encode;
    int n = ps->ascii85n;
    if ( n!=0 ) {
	int c5, c4, c3, c2, c1;
	while ( n++<4 )
	    val<<=8;
	c5 = val%85; val /= 85;
	c4 = val%85; val /= 85;
	c3 = val%85; val /= 85;
	c2 = val%85;
	c1 = val/85;
	putc(c1+'!',ps->output_file);
	putc(c2+'!',ps->output_file);
	if ( ps->ascii85n>=2 )
	    putc(c3+'!',ps->output_file);
	if ( ps->ascii85n>=3 )
	    putc(c4+'!',ps->output_file);
    }
    putc('~',ps->output_file);
    putc('>',ps->output_file);
    putc('\n',ps->output_file);
}

static int IsImageStringable(struct _GImage *base,int size,int do_color) {
    if ( base->image_type==it_true ) {
	if ( size>(do_color?21000:65000) )
return( false );
    } else if ( base->image_type == it_index ) {
	if ( size>65000 )
return( false );
    } else {
	if ( size>65000*8 )
return( false );
    }
return( true );
}
	
static void PSBuildImageMonoString(GPSWindow ps,struct _GImage *base,
	GRect *src) {
    register int j,jj;
    int i;
    register uint8 *pt;
    register int val, res, resbit;

    InitFilter(ps);
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data + i*base->bytes_per_line);
	jj = 1<<(7-(src->x&7)); res=0; resbit=0x80;
	res=0; resbit = 0x80;
	for ( j=src->width-1; j>=0; ) {
	    val = *pt++;
	    for ( ; jj!=0 &&j>=0; jj>>=1, --j ) {
		if ( val&jj )
		    res |= resbit;
		if (( resbit>>=1 )==0 ) {
		    Filter(ps,res);
		    res = 0; resbit=0x80;
		}
	    }
	    jj = 0x80;
	}
	if ( resbit!=0x80 ) {
	    Filter(ps,res);
	}
    }
    FlushFilter(ps);
}

static void PSDrawMonoImg(GPSWindow ps,struct _GImage *base,GRect *src, int usefile) {
    int width, height;
    Color col0=0, col1 = COLOR_CREATE(0xff,0xff,0xff);

    if ( base->clut!=NULL ) {
	col0 = base->clut->clut[0];
	col1 = base->clut->clut[1];
    }
    if ( base->trans==0 )
	_GPSDraw_SetColor(ps,col1);
    else if ( base->trans==1 )
	_GPSDraw_SetColor(ps,col0);

    width = src->width;
    height = src->height;

    if ( base->trans==COLOR_UNKNOWN && ps->display->do_color ) {
	/* I can't get image to work with the RGB colorspace, but I can */
	/*  get it to work with an Indexed space. Ghostview crashes when */
	/*  given a 6 element decode matrix and my printer gives up in */
	/*  the middle of the blt */
	fprintf( ps->output_file, "[/Indexed /DeviceRGB 1 < %06X %06X >] setcolorspace\n",
		(int) col0, (int) col1 );
    }
    fprintf(ps->output_file, "<<\n" );
    fprintf(ps->output_file, "  /ImageType 1\n" );
    fprintf(ps->output_file, "  /Width %d\n", (int) src->width );
    fprintf(ps->output_file, "  /Height %d\n", (int) src->height );
    fprintf(ps->output_file, "  /ImageMatrix [%d 0 0 %d 0 %d]\n",
	    (int) src->width, (int) -src->height, (int) src->height);
    fprintf(ps->output_file, "  /MultipleDataSources false\n" );
    fprintf(ps->output_file, "  /BitsPerComponent 1\n" );
    if ( base->trans != COLOR_UNKNOWN ) {
	if ( base->trans==0 )
	    fprintf(ps->output_file, "  /Decode [1 0]\n" );
	else
	    fprintf(ps->output_file, "  /Decode [0 1]\n" );
    } else if ( !ps->display->do_color ) {
	fprintf(ps->output_file, "  /Decode [%g %g]\n",
		COLOR2GREYR(col0), COLOR2GREYR(col1));
    } else {
	fprintf(ps->output_file, "  /Decode [0 1]\n" );
#if 0
	fprintf(ps->output_file, "  /Decode [%g %g %g %g %g %g]\n",
		COLOR_RED(col0)/255., COLOR_RED(col1)/255.,
		COLOR_GREEN(col0)/255., COLOR_GREEN(col1)/255.,
		COLOR_BLUE(col0)/255., COLOR_BLUE(col1)/255. );
#endif
    }
    fprintf(ps->output_file, "  /Interpolate true\n" );
    fprintf(ps->output_file, "  /DataSource " );
    if ( usefile ) {
	fprintf(ps->output_file, "currentfile /ASCII85Decode filter\n" );
	fprintf(ps->output_file, ">> %s\n",
		base->trans==COLOR_UNKNOWN?"image":"imagemask" );
	PSBuildImageMonoString(ps,base,src);
    } else {
	fprintf(ps->output_file, "<~\n" );
	PSBuildImageMonoString(ps,base,src);
	fprintf(ps->output_file, ">> %s\n",
		base->trans==COLOR_UNKNOWN?"image":"imagemask" );
    }
}

static void PSSetIndexColors(GPSWindow ps,GClut *clut) {
    int i;

    fprintf( ps->output_file, "[/Indexed /DeviceRGB %d <\n", clut->clut_len-1 );
    for ( i=0; i<clut->clut_len; ++i )
	fprintf(ps->output_file, "%02X%02X%02X%s",
		(unsigned int) COLOR_RED(clut->clut[i]),
		(unsigned int) COLOR_GREEN(clut->clut[i]),
		(unsigned int) COLOR_BLUE(clut->clut[i]),
		i%11==10?"\n":" ");
    fprintf(ps->output_file,">\n] setcolorspace\n");
}

static void PSBuildImageIndexString(GPSWindow ps,struct _GImage *base,GRect *src) {
    GCol clut[256];
    register int i,val;
    register uint8 *pt, *end;
    int do_color = ps->display->do_color;
    int clut_len = base->clut->clut_len;

    if ( base->clut->is_grey )
	do_color = false;

    for ( i=0; i<256; ++i ) {
	clut[i].red = COLOR_RED(base->clut->clut[i]);
	clut[i].green = COLOR_GREEN(base->clut->clut[i]);
	clut[i].blue = COLOR_BLUE(base->clut->clut[i]);
	if ( i==base->trans )
	    clut[i].red = clut[i].green = clut[i].blue = 0xff;
	if ( !do_color )
	    clut[i].red = RGB2GREY(clut[i].red,clut[i].green,clut[i].blue);
    }

    InitFilter(ps);
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data + i*base->bytes_per_line) + src->x;
	end = pt + src->width;
	while ( pt<end ) {
	    val = *pt++;
	    if ( do_color ) {
#if 0
		fprintf(ps->output_file,"%02lX%02lX%02lX",clut[val].red,clut[val].green,clut[val].blue);
#else
		if ( val>=clut_len )
		    val = clut_len-1;
		Filter(ps,val);
#endif
	    } else {
		Filter(ps,clut[val].red);
	    }
	}
    }
    FlushFilter(ps);
}

static void PSBuildImageIndexDict(GPSWindow ps,struct _GImage *base,GRect *src, int usefile) {
    fprintf(ps->output_file, "<<\n" );
    fprintf(ps->output_file, "  /ImageType 1\n" );
    fprintf(ps->output_file, "  /Width %d\n", (int) src->width );
    fprintf(ps->output_file, "  /Height %d\n", (int) src->height );
    fprintf(ps->output_file, "  /ImageMatrix [%d 0 0 %d 0 %d]\n",
	    (int) src->width, (int) -src->height, (int) src->height);
    fprintf(ps->output_file, "  /MultipleDataSources false\n" );
    fprintf(ps->output_file, "  /BitsPerComponent 8\n" );
    fprintf(ps->output_file, "  /Decode [0 255]\n" );
    fprintf(ps->output_file, "  /Interpolate false\n" );
    fprintf(ps->output_file, "  /DataSource " );
    if ( usefile ) {
	fprintf(ps->output_file, "currentfile /ASCII85Decode filter\n" );
	fprintf(ps->output_file, ">> image\n" );
	PSBuildImageIndexString(ps,base,src);
    } else {
	fprintf(ps->output_file, "<~\n" );
	PSBuildImageIndexString(ps,base,src);
	fprintf(ps->output_file, "\n>> image\n" );
    }
}

static void PSBuildImage24String(GPSWindow ps,struct _GImage *base,GRect *src) {
    int i;
    register long val, trans = base->trans;
    register uint32 *pt, *end;
    int do_color = ps->display->do_color;

    InitFilter(ps);
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	end = pt + src->width;
	while ( pt<end ) {
	    if ((val = *pt++)==trans ) val = COLOR_CREATE(0xff,0xff,0xff);
	    if ( do_color ) {
		Filter(ps,COLOR_RED(val));
		Filter(ps,COLOR_GREEN(val));
		Filter(ps,COLOR_BLUE(val));
	    } else {
		Filter(ps,COLOR2GREY(val));
	    }
	}
    }
    FlushFilter(ps);
}

static void PSBuildImageClutMaskString(GPSWindow ps,struct _GImage *base,GRect *src) {
    int i;
    int trans = base->trans;
    register uint8 *pt, *end;
    register int res,val, resbit;

    InitFilter(ps);
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data + i*base->bytes_per_line) + src->x;
	end = pt + src->width;
	res=0; resbit = 0x80;
	while ( pt<end ) {
	    val = *pt++;
	    if ( val!=trans )		/* Want to draw non transparent bits */
		res |= resbit;
	    if (( resbit>>=1 )==0 ) {
		Filter(ps,res);
		res = 0; resbit=0x80;
	    }
	}
	if ( resbit!=0x80 ) {
	    Filter(ps,res);
	}
    }
    FlushFilter(ps);
}

static void PSBuildImage24MaskString(GPSWindow ps,struct _GImage *base,GRect *src) {
    int i;
    register Color val, trans = base->trans;
    register uint32 *pt, *end;
    register int res, resbit;

    InitFilter(ps);
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	end = pt + src->width;
	res=0; resbit = 0x80;
	while ( pt<end ) {
	    val = *pt++;
	    if ( val!=trans )		/* Want to draw non transparent bits */
		res |= resbit;
	    if (( resbit>>=1 )==0 ) {
		Filter(ps,res);
		res = 0; resbit=0x80;
	    }
	}
	if ( resbit!=0x80 ) {
	    Filter(ps,res);
	}
    }
    FlushFilter(ps);
}

static void PSDrawImg(GPSWindow ps,struct _GImage *base,GRect *src, int usefile) {
    short width, height;
    /* when building up a pattern we put the image into a string rather than */
    /*  trying to read from a file (file is long past that point).  This */
    /*  doesn't work normally as the string can get too long.  We limit */
    /*  the images that we are willing to display transparently... */
    int do_color = ps->display->do_color;
    int usedict = false;

    if ( base->image_type == it_index && GImageGreyClut(base->clut))
	do_color = false;
    if ( base->image_type == it_index && do_color )
	usedict = true;

    width = src->width;
    height = src->height;

    if ( usedict ) {
	PSSetIndexColors(ps,base->clut);
	PSBuildImageIndexDict(ps,base,src,usefile);
	fprintf(ps->output_file, "[/DeviceRGB] setcolorspace\n" );
	ps->cur_fg = COLOR_CREATE(0,0,0);
    } else {
	fprintf(ps->output_file, "%d %d 8 [%d 0 0 %d 0 %d] ",
		width, height,  width, -height, height);
	if ( !usefile )
	    fprintf(ps->output_file, "<~\n" );	/* start a string */
	else {
	    fprintf(ps->output_file, "currentfile /ASCII85Decode filter " );
	    if ( do_color )
		fprintf(ps->output_file, "false 3 colorimage\n" );
	    else
		fprintf(ps->output_file, "image\n" );
	}
	if ( base->image_type==it_index )
	    PSBuildImageIndexString(ps,base,src);
	else
	    PSBuildImage24String(ps,base,src);
	if ( !usefile ) {
	    if ( ps->display->do_color )
		fprintf(ps->output_file, "false 3 colorimage\n" );
	    else
		fprintf(ps->output_file, "image\n" );
	}
    }
}

static void PSDrawImage(GPSWindow ps,GImage *image, GRect *dest, GRect *src) {
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    Color trans = base->trans;
    short width, height;

    _GPSDraw_SetClip(ps);

    width = src->width;
    height = src->height;

    if ( trans!=COLOR_UNKNOWN ) {
	/* We don't try to get tranparent images to work if they are bigger */
	/*  than 64k.  Strings are limited to that amount and we use strings */
	/*  for our masks. (well, mono images are a special case) */
	if ( !ps->display->do_transparent )
	    trans = COLOR_UNKNOWN;
	else if ( !IsImageStringable(base,width*height,ps->display->do_color) )
	    trans = COLOR_UNKNOWN;
    }

    fprintf( ps->output_file, "  gsave %g %g translate %g %g scale\n",
	    _GSPDraw_XPos(ps,dest->x), _GSPDraw_YPos(ps,dest->y+dest->height),
	    _GSPDraw_Distance(ps,dest->width),_GSPDraw_Distance(ps,dest->height) );
    if ( base->image_type==it_mono ) {
	PSDrawMonoImg(ps,base,src,true);
    } else if ( trans==COLOR_UNKNOWN ) {
	/* Just draw the image, don't worry about if it's transparent */
	PSDrawImg(ps,base,src,true);
    } else {
	fprintf( ps->output_file, "    save mark\t%% Create a temporary pattern for trans image\n");
	fprintf( ps->output_file, "<< /PatternType 1\n" );
	fprintf( ps->output_file, "   /PaintType 1\n" );	/* coloured pattern */
	fprintf( ps->output_file, "   /TilingType 2\n" );	/* Allow PS to distort pattern to make it fit */
	fprintf( ps->output_file, "   /BBox [0 0 1 1]\n" );
	fprintf( ps->output_file, "   /XStep 1 /YStep 1\n" );
	fprintf( ps->output_file, "   /PaintProc { pop " );
	PSDrawImg(ps,base,src,false);
	fprintf( ps->output_file, "} >> matrix makepattern /TransPattern exch def\n" );
	fprintf( ps->output_file, "    TransPattern setpattern\n");
	fprintf(ps->output_file, "%d %d true [%d 0 0 %d 0 %d] currentfile /ASCII85Decode filter imagemask\n",
		(int) base->width, (int) base->height,  (int) base->width, (int) -base->height, (int) base->height);
	if ( base->image_type==it_index )
	    PSBuildImageClutMaskString(ps,base,src);
	else
	    PSBuildImage24MaskString(ps,base,src);
	fprintf( ps->output_file, "    /TransPattern /Pattern undefineresource cleartomark restore\n");
    }
    fprintf( ps->output_file, "  grestore\n" );
}

static int PSBuildImagePattern(GPSWindow ps,struct _GImage *base, char *pattern_name) {
    GPSDisplay *gdisp = ps->display;
    GRect src;
    int factor = gdisp->scale_screen_by;

    if ( !IsImageStringable(base,base->width*base->height,gdisp->do_color))
return( false );
    src.x = src.y = 0; src.width = base->width; src.height = base->height;

    fprintf( ps->output_file, "  gsave %g %g scale\n",
	    base->width*factor*72.0/ps->res,
	    base->height*factor*72.0/ps->res );

    if ( base->image_type!=it_mono && base->trans!=-1 ) {
	/* Must build a secondary pattern through which the primary pattern */
	/*  will mask */
	fprintf( ps->output_file, "<< /PatternType 1\n" );
	fprintf( ps->output_file, "   /PaintType 1\n" );	/* coloured pattern */
	fprintf( ps->output_file, "   /TilingType 2\n" );	/* Allow PS to distort pattern to make it fit */
	fprintf( ps->output_file, "   /BBox [0 0 1 1]\n" );
	fprintf( ps->output_file, "   /XStep 1 /YStep 1\n" );
	fprintf( ps->output_file, "   /PaintProc { pop " );
	PSDrawImg(ps,base,&src,false);
	fprintf( ps->output_file, "} >> matrix makepattern /%s_Secondary exch def\n", pattern_name );
    }
    fprintf( ps->output_file, "<< /PatternType 1\n" );
    fprintf( ps->output_file, "   /PaintType 1\n" );	/* coloured pattern */
    fprintf( ps->output_file, "   /TilingType 2\n" );	/* Allow PS to distort pattern to make it fit */
    fprintf( ps->output_file, "   /BBox [0 0 1 1]\n" );
    fprintf( ps->output_file, "   /XStep 1 /YStep 1\n" );
    fprintf( ps->output_file, "   /PaintProc { pop " );
    if ( base->image_type==it_mono ) {
	PSDrawMonoImg(ps,base,&src,false);
    } else if ( base->trans==COLOR_UNKNOWN || !gdisp->do_transparent ) {
	/* Just draw the image, don't worry about if it's transparent */
	PSDrawImg(ps,base,&src,false);
    } else {
	fprintf( ps->output_file, "    %s_Secondary setpattern\n", pattern_name);
	fprintf(ps->output_file, "%d %d true [%d 0 0 %d 0 %d] <~",
		(int) base->width, (int) base->height,  (int) base->width, (int) -base->height, (int) base->height);
	if ( base->image_type==it_index )
	    PSBuildImageClutMaskString(ps,base,&src);
	else
	    PSBuildImage24MaskString(ps,base,&src);
	fprintf(ps->output_file, "imagemask \n" );
    }
    fprintf( ps->output_file, "} >> matrix makepattern /%s exch def\n", pattern_name );
    fprintf( ps->output_file, "  grestore\n" );
return( true );
}

void _GPSDraw_InitPatterns(GPSWindow ps) {

    /* Need to output pattern defn for dotted filling, background fill pattern??? */
    fprintf( ps->init_file, "\n%%Global Patterns\n" );
    fprintf( ps->init_file, "%% Dithering pattern\n" );
    fprintf( ps->init_file, "<< /PatternType 1\n" );
    fprintf( ps->init_file, "   /PaintType 2\n" );	/* Uncoloured pattern */
    fprintf( ps->init_file, "   /TilingType 3\n" );	/* Allow PS to distort pattern to make it fit */
    fprintf( ps->init_file, "   /BBox [0 0 2 2]\n" );
    fprintf( ps->init_file, "   /XStep 2 /YStep 2\n" );
    fprintf( ps->init_file, "   /PaintProc { pop 0 0 moveto 1 0 rlineto 0 1 rlineto -1 0 rlineto closepath fill\n" );
    fprintf( ps->init_file, "\t\t    1 1 moveto 1 0 rlineto 0 1 rlineto -1 0 rlineto closepath fill }\n" );
    fprintf( ps->init_file, ">> matrix makepattern /DotPattern exch def\n\n" );
}

static int PSTileImage(GPSWindow ps,struct _GImage *base, long x, long y,
	int repeatx, int repeaty ) {
    char *pattern_name = NULL;
    int factor = ps->display->scale_screen_by;

    _GPSDraw_SetClip(ps);
    {
	if ( !IsImageStringable(base,base->width*base->height,ps->display->do_color) )
return( false );
	if ( repeatx==1 && repeaty==1 )
return( false );		/* Not worth it for just one drawing */
	fprintf( ps->output_file, "  save mark\t%% Create a temporary pattern for tiling the background\n");
	pattern_name = "g_background_pattern";
	PSBuildImagePattern(ps,base, pattern_name);
    }

    fprintf( ps->output_file, "  %s setpattern\n", pattern_name );
    _GPSDraw_FlushPath(ps);
    fprintf( ps->output_file, "  %g %g  %g %g  %g %g  %g %g g_quad fill\n",
	    _GSPDraw_XPos(ps,x), _GSPDraw_YPos(ps,y+repeaty*base->height*factor),
	    _GSPDraw_XPos(ps,x+repeatx*base->width*factor), _GSPDraw_YPos(ps,y+repeaty*base->height*factor),
	    _GSPDraw_XPos(ps,x+repeatx*base->width*factor), _GSPDraw_YPos(ps,y),
	    _GSPDraw_XPos(ps,x), _GSPDraw_YPos(ps,y));
    if ( base->image_type!=it_mono && base->trans!=COLOR_UNKNOWN &&
	    ps->display->do_transparent )
	fprintf( ps->output_file, "  /g_background_pattern_Secondary /Pattern undefineresource\n" );
    fprintf( ps->output_file, "  /g_background_pattern /Pattern undefineresource cleartomark restore\n" );
return( true );
}

void _GPSDraw_Image(GWindow w, GImage *image, GRect *src, int32 x, int32 y) {
    GPSWindow ps = (GPSWindow) w;
    GRect dest;
    int factor = ps->display->scale_screen_by;

    dest.x = x; dest.y = y;
    dest.width = src->width*factor;
    dest.height = src->height*factor;
    PSDrawImage(ps,image,&dest,src);
}

void _GPSDraw_ImageMagnified(GWindow w, GImage *image, GRect *dest, int32 x, int32 y, int32 width, int32 height) {
    GPSWindow ps = (GPSWindow) w;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    GRect src,temp;

    src.width = (dest->width/(double) width)*base->width;
    src.height = (dest->height/(double) height)*base->height;
    src.x = dest->x * (base->width/(double) width);
    src.y = dest->y * (base->height/(double) height);
    temp.x = x; temp.y = y; temp.width = dest->width; temp.height = dest->height;
    PSDrawImage(ps,image,&temp,&src);
}

void _GPSDraw_TileImage(GWindow w, GImage *image, GRect *dest, int32 x, int32 y) {
    GPSWindow ps = (GPSWindow) w;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int xstart, ystart, xend, yend;
    int factor = ps->display->scale_screen_by;
    int width = factor*base->width, height = factor*base->height;
    int i,j;

    xstart = (dest->x-x)/width; ystart = (dest->y-y)/height;
    xend = (dest->x+dest->width-x)/width; yend = (dest->y+dest->height-y)/height;
    if ( !PSTileImage(ps,base,
	    x+xstart*width,y+ystart*height,
	    xend-xstart+1,yend-ystart+1)) {
	GRect src; src.x = src.y = 0; src.width = base->width; src.height = height;
	for ( j=ystart; j<=yend; ++j ) for ( i=xstart; i<=xend; ++i ) {
	    GRect dest;
	    dest.x = x+i*width; dest.y = y+j*height;
	    dest.width = width; dest.height = height;
	    PSDrawImage(ps,image, &dest, &src);
	}
    }
}
