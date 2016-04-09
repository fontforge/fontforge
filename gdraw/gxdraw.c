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

#include "fontforge-config.h"

#ifndef FONTFORGE_CAN_USE_GDK

#if defined(__MINGW32__)
#include <winsock2.h>
#include <windows.h>
#endif
 
#include "gxdrawP.h"
#include "gxcdrawP.h"

#include <stdlib.h>
#include <math.h>

#if !defined(__MINGW32__)
#include <unistd.h>		/* for timers & select */
#endif
#include <sys/types.h>		/* for timers & select */
#include <sys/time.h>		/* for timers & select */
#include <signal.h>		/* error handler */
#include <locale.h>		/* for setting the X locale properly */

#ifdef HAVE_PTHREAD_H
# ifndef __MINGW32__
#  include <sys/socket.h>
#  include <sys/un.h>
# endif
#endif

#include <ustring.h>
#include <utype.h>
#include "fontP.h"
#include <gresource.h>

enum cm_type { cmt_default=-1, cmt_current, cmt_copy, cmt_private };

void GDrawIErrorRun(const char *fmt,...);
void GDrawIError(const char *fmt,...);


#ifndef X_DISPLAY_MISSING
# include <X11/Xatom.h>
# include <X11/keysym.h>
# include <X11/cursorfont.h>
# include <X11/Xresource.h>

#define XKeysym_Mask	0x01000000

#define XKEYSYM_TOP	8364
extern int gdraw_xkeysym_2_unicode[];


extern int cmdlinearg_forceUIHidden;

static void GXDrawTransmitSelection(GXDisplay *gd,XEvent *event);
static void GXDrawClearSelData(GXDisplay *gd,enum selnames sel);

/* ************************************************************************** */
/* ******************************* Font Stuff ******************************* */
/* ************************************************************************** */

static void _GXDraw_InitFonts(GXDisplay *gxdisplay) {
    FState *fs = calloc(1,sizeof(FState));

    /* In inches, because that's how fonts are measured */
    gxdisplay->fontstate = fs;
    fs->res = gxdisplay->res;
}

/* ************************************************************************** */
/* ****************************** COLOR Stuff ******************************* */
/* ************************************************************************** */

#define DIV_BY_0x33(x) (((x)*1286)>>16)		/* Fast way to do x/0x33 */
#define RND_BY_0x33(x) (((x+0x19)*1286)>>16)
#define DIV_BY_0x11(x) (((x)*241)>>12)		/* Fast way to do x/0x11 */
#define RND_BY_0x11(x) (((x+8)*241)>>12)

static void _GXDraw_FindVisual(GXDisplay *gdisp) {
    /* In the old days before 24bit color was common, a 32 bit visual was */
    /* the standard way to go (extra 8 bits were ignored). Then came the */
    /* composite extension which supported alpha channels in images, and */
    /* we had to be careful what went into the extra byte, 0 doesn't work */
    static int oldvsearch[][2] = {{ 32, TrueColor },
				{ 24, TrueColor },
				{ 16, TrueColor },
				{ 15, TrueColor },
				{ 12, TrueColor },
			        {0}};
    static int newvsearch[][2] = {{ 24, TrueColor },
				{ 16, TrueColor },
				{ 15, TrueColor },
				{ 12, TrueColor },
			        {0}};
    static int v2search[][2] = {{ 8, PseudoColor },
				{ 8, GrayScale },
			        { 1, GrayScale },
			        { 1, StaticGray }};
    int (*vsearch)[2] = gdisp->supports_alpha_images || gdisp->supports_alpha_windows
	    ? newvsearch : oldvsearch;
    Display *display = gdisp->display;
    XVisualInfo vinf, *ret;
    int pixel_size, vc, i, j, first;
    ScreenFormat *sf;
    int user=false, cnt;

    if ( gdisp->desired_depth!=-1 || gdisp->desired_vc!=-1 ) {
	vinf.depth = gdisp->desired_depth;
	vinf.class = gdisp->desired_vc;
	ret = XGetVisualInfo(display,
		(gdisp->desired_depth==-1?0:VisualDepthMask )|
		(gdisp->desired_vc==-1?0:VisualClassMask ),
		&vinf, &cnt);
	if ( cnt>0 ) {
	    for ( i=0; i<cnt; ++i )
		if ( ret[i].visual==DefaultVisual(display,gdisp->screen))
	    break;
	    if ( i==cnt )
		i=0;
	    gdisp->visual = ret[i].visual;
	    gdisp->depth = ret[i].depth;
	    user = true;
	    XFree(ret);
	} else
	    fprintf( stderr, "Failed to find your desired visual structure\n" );
    }

    /* I'd like TrueColor if I can get it */
    if ( gdisp->visual == NULL ) {
	for ( i=0; vsearch[i][0]!=0 ; ++i ) {
	    vinf.depth = vsearch[i][0];
	    vinf.class = vsearch[i][1];
	    ret = XGetVisualInfo(display,
		    (gdisp->desired_depth==-1?0:VisualDepthMask )|
		    (gdisp->desired_vc==-1?0:VisualClassMask ),
		    &vinf, &cnt);
	    if ( cnt>0 ) {
		for ( j=0; j<cnt; ++j )
		    if ( ret[j].visual==DefaultVisual(display,gdisp->screen))
		break;
		if ( j==cnt )
		    j=0;
		gdisp->visual = ret[j].visual;
		gdisp->depth = ret[j].depth;
		user = true;
		XFree(ret);
	break;
	    }
	}
    }
    if ( gdisp->visual == NULL ) {
	gdisp->visual = DefaultVisual(display,gdisp->screen);
	gdisp->depth = DefaultDepth(display,gdisp->screen);
    }
    /* Failing TrueColor I'd like PseudoColor on color displays, and GreyScale*/
    /*  on monochrom displays */
    if ( !user ) {
	if ( gdisp->visual->class==DirectColor || gdisp->visual->class==StaticColor ) {
	    if ( XMatchVisualInfo(display,gdisp->screen,8,PseudoColor,&vinf)) {
		gdisp->visual = vinf.visual;
		gdisp->depth = 8;
	    }
	} else if ( gdisp->visual->class==GrayScale || gdisp->visual->class==StaticGray ) {
	    if ( XMatchVisualInfo(display,gdisp->screen,8,GrayScale,&vinf)) {
		gdisp->visual = vinf.visual;
		gdisp->depth = 8;
	    }
	}
    }

    for ( first = true; ; ) {
	/* I want not only the number of meaningful bits in a pixel (which is the */
	/*  depth) but also the number of bits in pixel when writing an image */
	/* I wish I knew how to do this without diving into hidden X structures */
	gdisp->bitmap_pad = gdisp->pixel_size = gdisp->depth;
	for ( i=0; i<((_XPrivDisplay) display)->nformats; ++i ) {
	    sf = &((_XPrivDisplay) display)->pixmap_format[i];
	    if ( sf->depth == gdisp->depth ) { 
		gdisp->pixel_size = sf->bits_per_pixel;
		gdisp->bitmap_pad = sf->scanline_pad;
	break;
	    }
	}

	if ( gdisp->pixel_size==1 || gdisp->pixel_size==8 || gdisp->pixel_size==16 ||
		gdisp->pixel_size==24 || gdisp->pixel_size==32 )
    break;	/* We like it */
	else if ( first ) {
	    /* If I still don't have something I can work with, try for black&white */
	    for ( i=0; i<sizeof(v2search)/(2*sizeof(int)); ++i ) {
		if ( XMatchVisualInfo(display,gdisp->screen,v2search[i][0],v2search[i][1],&vinf)) {
		    gdisp->visual = vinf.visual;
		    gdisp->depth = vinf.depth;
	    break;
		}
	    }
	    first = false;
	} else
    break;
    }

    pixel_size = gdisp->pixel_size; vc = gdisp->visual->class;
    if ( pixel_size==1 || pixel_size==8 ||
	    ((pixel_size==16 || pixel_size==24 || pixel_size==32) && vc==TrueColor))
	/* We can deal with it */;
    else if ( vc==TrueColor || ( vc==PseudoColor && pixel_size<8)) {
       fprintf( stderr, "%s will not work well with this visual.  Colored images will be displayed as bitmaps\n", GResourceProgramName );
    } else {
       fprintf( stderr, "%s will not work with this visual.  Restart your X server with a TrueColor\n", GResourceProgramName );
       fprintf( stderr, " visual (You do this on an SGI by adding an argument \"-class TrueColor\" to\n" );
       fprintf( stderr, " the command which starts up X, which is probably in /var/X11/xdm/Xservers.\n" );
       fprintf( stderr, " On a sun you add \"-cc 4\" to the server start line, probably found in\n" );
       fprintf( stderr, " /usr/lib/X11/xdm/Xservers).\n" );
       exit(1);
   }

    if ( gdisp->visual == DefaultVisual(display,gdisp->screen)) {
	gdisp->cmap = DefaultColormap(display,gdisp->screen);
	gdisp->default_visual = true;
    } else {
	gdisp->cmap = XCreateColormap(display,gdisp->root,gdisp->visual,
		AllocNone);
	XInstallColormap(display,gdisp->cmap);
    }
}

static int _GXDraw_AllocColors(GXDisplay *gdisp,XColor *x_colors) {
    /* Try to insure that the standard colours we expect to use are available */
    /*  in the default colormap */
    Display *display = gdisp->display;
    XColor *acolour;
    static unsigned short rgb[][3]={
	    {0x8000,0x8000,0x8000}, {0x4000,0x4000,0x4000},
	    {0xc000,0xc000,0xc000},
	    {0x1100,0x1100,0x1100}, {0x2200,0x2200,0x2200},
	    /*{0x3300,0x3300,0x3300},*/ {0x5500,0x5500,0x5500},
	    /*{0x6600,0x6600,0x6600},*/ {0x7700,0x7700,0x7700},
	    {0xaa00,0xaa00,0xaa00}, {0xbb00,0xbb00,0xbb00},
	    {0xdd00,0xdd00,0xdd00}, {0xee00,0xee00,0xee00},
	    };
    int i, r,g,b;
    static int cube[] = { 0x00, 0x33, 0x66, 0x99, 0xcc, 0xff };

    acolour = x_colors;

    for(r = 5; r >= 0; --r) {
      for(g = 5; g >= 0; --g) {
	for(b = 5; b >= 0; --b) {
	acolour->red = (cube[r]<<8)|cube[r];
	acolour->green = (cube[g]<<8)|cube[g];
	acolour->blue = (cube[b]<<8)|cube[b];
	acolour->pixel = 0;
	acolour->flags = 7;
	if ( XAllocColor(display,gdisp->cmap,acolour))
	    ++acolour;
    }}}

    for ( i=0; i<sizeof(rgb)/sizeof(rgb[0]); ++i ) {
	acolour->red = rgb[i][0];
	acolour->green = rgb[i][1];
	acolour->blue = rgb[i][2];
	if ( XAllocColor(display,gdisp->cmap,acolour))
	    ++acolour;
    }
return( acolour-x_colors );
}

static void _GXDraw_AllocGreys(GXDisplay *gdisp) {
    Display *display = gdisp->display;
    XColor xcolour;
    int step = 255/((1<<gdisp->depth)-1), r;

    for(r = 0; r < 256; r+=step ) {
	xcolour.red = r<<8;
	xcolour.green = r<<8;
	xcolour.blue = r<<8;
	XAllocColor(display,gdisp->cmap,&xcolour);
    }
}

static int _GXDraw_CopyColors(GXDisplay *gdisp, XColor *x_colors, Colormap new) {
    int i;

    for ( i=0; i<(1<<gdisp->depth); ++i )
	x_colors[i].pixel = i;
    XQueryColors(gdisp->display,gdisp->cmap,x_colors,1<<gdisp->depth);
    XStoreColors(gdisp->display,new,x_colors,1<<gdisp->depth);
    gdisp->cmap = new;
return( 1<<gdisp->depth );
}

static int FindAllColors(GXDisplay *gdisp, XColor *x_colors) {
    int i;

    for ( i=0; i<(1<<gdisp->depth); ++i )
	x_colors[i].pixel = i;
    XQueryColors(gdisp->display,gdisp->cmap,x_colors,1<<gdisp->depth);
return( 1<<gdisp->depth );
}

static void InitTrueColor(GXDisplay *gdisp) {
    Visual *vis = gdisp->visual;
    int red_shift, green_shift, blue_shift;
    int red_bits_shift, blue_bits_shift, green_bits_shift;

    /* The SGI display is not RGB but BGR.  X says TrueColor maps are increasing */
    for ( red_shift=0; red_shift<24 && !(vis->red_mask&(1<<red_shift)); ++red_shift );
    for ( green_shift=0; green_shift<24 && !(vis->green_mask&(1<<green_shift)); ++green_shift );
    for ( blue_shift=0; blue_shift<24 && !(vis->blue_mask&(1<<blue_shift)); ++blue_shift );
    gdisp->cs.red_shift = red_shift;
    gdisp->cs.green_shift = green_shift;
    gdisp->cs.blue_shift = blue_shift;

    gdisp->cs.red_bits_mask = vis->red_mask>>red_shift;
    gdisp->cs.green_bits_mask = vis->green_mask>>green_shift;
    gdisp->cs.blue_bits_mask = vis->blue_mask>>blue_shift;
    for ( red_bits_shift=0; gdisp->cs.red_bits_mask&(1<<red_bits_shift) ;
	    ++red_bits_shift );
    for ( green_bits_shift=0; gdisp->cs.green_bits_mask&(1<<green_bits_shift) ;
	    ++green_bits_shift );
    for ( blue_bits_shift=0; gdisp->cs.blue_bits_mask&(1<<blue_bits_shift) ;
	    ++blue_bits_shift );
    gdisp->cs.red_bits_shift = 24-red_bits_shift;
    gdisp->cs.green_bits_shift = 16-green_bits_shift;
    gdisp->cs.blue_bits_shift = 8-blue_bits_shift;

    gdisp->cs.alpha_bits = 0xffffffff&~(vis->red_mask|vis->green_mask|vis->blue_mask);
}

static void _GXDraw_InitCols(GXDisplay *gdisp) {
    int i;
    XColor x_colors[256];
    GClut clut;
    int x_color_max;
    int vclass, depth;
    int n=0;
    char **extlist = XListExtensions(gdisp->display,&n);

    for ( i=0; i<n; ++i ) {
	if ( strcasecmp(extlist[i],"Composite")==0 ) {	/* "Render"??? */
/* Silly me. I had assumed that writing an alpha channel image to a window*/
/*  would do overlay composition on what was in the window. Instead it */
/*  overwrites the window, and retains the alpha channel so that the */
/*  window becomes translucent, rather than the image. */
/* In otherwords, it is totally useless to me */
	    /* gdisp->supports_alpha_images = true; */
	    gdisp->supports_alpha_windows = true;
    break;
	}
    }
    if ( extlist!=NULL )
	XFreeExtensionList(extlist);

    _GXDraw_FindVisual(gdisp);
    if ( gdisp->depth!=32 && gdisp->supports_alpha_images )
	gdisp->supports_alpha_images = false;

    vclass = gdisp->visual->class;
    depth = gdisp->depth;

    if ( depth<=8 ) {
	memset(&clut,'\0',sizeof(clut));
	if ( vclass==StaticGray || vclass == GrayScale ) {
	    _GXDraw_AllocGreys(gdisp);
	    gdisp->cs.is_grey = clut.is_grey = true;
	    x_color_max = FindAllColors(gdisp,x_colors);
	} else if ( gdisp->desired_cm==cmt_private ) {
	    gdisp->cmap = XCreateColormap(gdisp->display,gdisp->root,
		    gdisp->visual,AllocNone);
	    XInstallColormap(gdisp->display,gdisp->cmap);
	    x_color_max = _GXDraw_AllocColors(gdisp,x_colors);
	} else {
	    x_color_max = _GXDraw_AllocColors(gdisp,x_colors);
	    if (( gdisp->desired_cm==cmt_default && x_color_max<30 ) ||
		    gdisp->desired_cm==cmt_copy ) {
		x_color_max = _GXDraw_CopyColors(gdisp,x_colors,
			XCreateColormap(gdisp->display,gdisp->root,
				gdisp->visual,AllocAll));
		XInstallColormap(gdisp->display,gdisp->cmap);
	    }
	}
	clut.clut_len = x_color_max;
	for ( i=0; i<x_color_max; ++i )
	    clut.clut[x_colors[i].pixel] = COLOR_CREATE(x_colors[i].red>>8,x_colors[i].green>>8,x_colors[i].blue>>8);
	gdisp->cs.rev = GClutReverse(&clut,8);
    } else if ( vclass==TrueColor )
       InitTrueColor(gdisp);
}

unsigned long _GXDraw_GetScreenPixel(GXDisplay *gdisp, Color col) {

    if ( gdisp->depth==24 )
return( Pixel24(gdisp,col) );
    else if ( gdisp->depth==32 )
return( Pixel32(gdisp,col) );
    else if ( gdisp->depth>8 )
return( Pixel16(gdisp,col));

return( _GImage_GetIndexedPixel/*Precise*/(col, gdisp->cs.rev)->pixel );
}

/* ****************************** Error Handler ***************************** */

static char *XProtocolCodes[] = {
    "X_Undefined_0",
    "X_CreateWindow",
    "X_ChangeWindowAttributes",
    "X_GetWindowAttributes",
    "X_DestroyWindow",
    "X_DestroySubwindows",
    "X_ChangeSaveSet",
    "X_ReparentWindow",
    "X_MapWindow",
    "X_MapSubwindows",
    "X_UnmapWindow",
    "X_UnmapSubwindows",
    "X_ConfigureWindow",
    "X_CirculateWindow",
    "X_GetGeometry",
    "X_QueryTree",
    "X_InternAtom",
    "X_GetAtomName",
    "X_ChangeProperty",
    "X_DeleteProperty",
    "X_GetProperty",
    "X_ListProperties",
    "X_SetSelectionOwner",
    "X_GetSelectionOwner",
    "X_ConvertSelection",
    "X_SendEvent",
    "X_GrabPointer",
    "X_UngrabPointer",
    "X_GrabButton",
    "X_UngrabButton",
    "X_ChangeActivePointerGrab",
    "X_GrabKeyboard",
    "X_UngrabKeyboard",
    "X_GrabKey",
    "X_UngrabKey",
    "X_AllowEvents",
    "X_GrabServer",
    "X_UngrabServer",
    "X_QueryPointer",
    "X_GetMotionEvents",
    "X_TranslateCoords",
    "X_WarpPointer",
    "X_SetInputFocus",
    "X_GetInputFocus",
    "X_QueryKeymap",
    "X_OpenFont",
    "X_CloseFont",
    "X_QueryFont",
    "X_QueryTextExtents",
    "X_ListFonts",
    "X_ListFontsWithInfo",
    "X_SetFontPath",
    "X_GetFontPath",
    "X_CreatePixmap",
    "X_FreePixmap",
    "X_CreateGC",
    "X_ChangeGC",
    "X_CopyGC",
    "X_SetDashes",
    "X_SetClipRectangles",
    "X_FreeGC",
    "X_ClearArea",
    "X_CopyArea",
    "X_CopyPlane",
    "X_PolyPoint",
    "X_PolyLine",
    "X_PolySegment",
    "X_PolyRectangle",
    "X_PolyArc",
    "X_FillPoly",
    "X_PolyFillRectangle",
    "X_PolyFillArc",
    "X_PutImage",
    "X_GetImage",
    "X_PolyText8",
    "X_PolyText16",
    "X_ImageText8",
    "X_ImageText16",
    "X_CreateColormap",
    "X_FreeColormap",
    "X_CopyColormapAndFree",
    "X_InstallColormap",
    "X_UninstallColormap",
    "X_ListInstalledColormaps",
    "X_AllocColor",
    "X_AllocNamedColor",
    "X_AllocColorCells",
    "X_AllocColorPlanes",
    "X_FreeColors",
    "X_StoreColors",
    "X_StoreNamedColor",
    "X_QueryColors",
    "X_LookupColor",
    "X_CreateCursor",
    "X_CreateGlyphCursor",
    "X_FreeCursor",
    "X_RecolorCursor",
    "X_QueryBestSize",
    "X_QueryExtension",
    "X_ListExtensions",
    "X_ChangeKeyboardMapping",
    "X_GetKeyboardMapping",
    "X_ChangeKeyboardControl",
    "X_GetKeyboardControl",
    "X_Bell",
    "X_ChangePointerControl",
    "X_GetPointerControl",
    "X_SetScreenSaver",
    "X_GetScreenSaver",
    "X_ChangeHosts",
    "X_ListHosts",
    "X_SetAccessControl",
    "X_SetCloseDownMode",
    "X_KillClient",
    "X_RotateProperties",
    "X_ForceScreenSaver",
    "X_SetPointerMapping",
    "X_GetPointerMapping",
    "X_SetModifierMapping",
    "X_GetModifierMapping",
    "X_Undefined_120",
    "X_Undefined_121",
    "X_Undefined_122",
    "X_Undefined_123",
    "X_Undefined_124",
    "X_Undefined_125",
    "X_Undefined_126",
    "X_NoOperation"
};

static char *lastfontrequest;

static int myerrorhandler(Display *disp, XErrorEvent *err) {
    /* Under twm I get a bad match, under kde a bad window? */
    char buffer[200], *majorcode;

    if (err->request_code>0 && err->request_code<128)
	majorcode = XProtocolCodes[err->request_code];
    else
	majorcode = "";
    if ( err->request_code==45 && lastfontrequest!=NULL )
	fprintf( stderr, "Error attempting to load font:\n  %s\nThe X Server claimed the font existed, but when I asked for it,\nI got this error instead:\n\n", lastfontrequest );
    XGetErrorText(disp,err->error_code,buffer,sizeof(buffer));
    fprintf( stderr, "X Error of failed request: %s\n", buffer );
    fprintf( stderr, "  Major opcode of failed request:  %d.%d (%s)\n",
	    err->request_code, err->minor_code, majorcode );
    fprintf( stderr, "  Serial number of failed request:  %ld\n", (long) err->serial );
    fprintf( stderr, "  Failed resource ID:  %x\n", (unsigned int) err->resourceid );
    raise(SIGABRT);	/* I want something that alerts the debugger, not a semi-successful exit */
return( 1 );
}

/* ************************************************************************** */

static void _GXDraw_InitAtoms( GXDisplay *gdisp) {
    Display *display = gdisp->display;
    gdisp->atoms.wm_del_window = XInternAtom(display,"WM_DELETE_WINDOW",False);
    gdisp->atoms.wm_protocols = XInternAtom(display,"WM_PROTOCOLS",False);
    gdisp->atoms.drag_and_drop = XInternAtom(display,"DRAG_AND_DROP",False);
}

static Cursor StdCursor[ct_user] = { 0 };
static int cursor_map[ct_user] = { 0,
	XC_left_ptr,		/* pointer */
	XC_right_ptr,		/* backpointer */
	XC_hand2,		/* hand */
	XC_question_arrow,	/* question */
	XC_tcross,		/* cross */
	XC_fleur,		/* 4way */
	XC_xterm,		/* text */
	XC_watch,		/* watch */
	XC_right_ptr };		/* drag and drop */

static Cursor _GXDraw_GetCursor( GXDisplay *gdisp, enum cursor_types ct ) {
    Display *display = gdisp->display;

    if ( ct>=ct_user )
return( ct-ct_user );
    else if ( ct==ct_default )
return( CopyFromParent );
    if ( StdCursor[ct]==0 ) {
	XColor fb[2];
	fb[0].red = COLOR_RED(gdisp->def_foreground)*0x101; fb[0].green = COLOR_GREEN(gdisp->def_foreground)*0x101; fb[0].blue = COLOR_BLUE(gdisp->def_foreground)*0x101;
	fb[1].red = COLOR_RED(gdisp->def_background)*0x101; fb[1].green = COLOR_GREEN(gdisp->def_background)*0x101; fb[1].blue = COLOR_BLUE(gdisp->def_background)*0x101;
	if ( ct==ct_invisible ) {
	    static short zeros[16]={0};
	    Pixmap temp = XCreatePixmapFromBitmapData(display,gdisp->root,
		(char *) zeros,16,16,1,0,1);
	    StdCursor[ct] = XCreatePixmapCursor(display,temp,temp,&fb[0],&fb[1],0,0);
	    XFreePixmap(display,temp);
	} else {
	    StdCursor[ct] = XCreateFontCursor(display,cursor_map[ct]);
	    /*XRecolorCursor(display,StdCursor[ct],&fb[0],&fb[1]);*/
	}
    }
return( StdCursor[ct]);
}

/* ************************************************************************** */
/* ************************** Utility Routines ****************************** */
/* ************************************************************************** */

static void initParentissimus(GXDisplay *gdisp, Window wind) {
    Display *display = gdisp->display;
    Window par, *children, root;
    unsigned int junk,width,height; int sjunk;

    while (1) {
	XQueryTree(display,wind,&root,&par,&children,&junk);
	XFree(children);
	if ( par==root )
    break;
	wind = par;
    }
    XGetGeometry(display,wind,&root,&sjunk,&sjunk,&width,&height,
	    &junk,&junk);
    if (( width>DisplayWidth(display,gdisp->screen) &&
	    height>=DisplayHeight(display,gdisp->screen)) ||
	( width>=DisplayWidth(display,gdisp->screen) &&
	    height>DisplayHeight(display,gdisp->screen)) )
	gdisp->virtualRoot = wind;
    else
	gdisp->virtualRoot = root;
}

static int qterror(Display *disp, XErrorEvent *err) {
    /* under kde a bad window? */
    if ( err->error_code == BadMatch || err->error_code == BadWindow ) {
    } else {
	myerrorhandler(disp,err);
    }
return( 1 );
}

static Window GetParentissimus(GXWindow gw) {
    GXDisplay *gdisp = gw->display;
    Display *display = gdisp->display;
    Window par, *children, root, wind = gw->w;
    unsigned int junk;
    fd_set junkset;
    struct timeval tenthsec;

    if ( gw->parentissimus )
return( gw->parentissimus );
    if ( gdisp->virtualRoot==BadAlloc )        /* Check for vtwm */
	initParentissimus(gdisp,wind);

    /* For reasons quite obscure to me, XQueryTree gives a BadWindow error */
    /*  under kde and gnome if there is no pause in the loop. XQueryTree */
    /*  isn't even documented to fail, so it's all very strange */
    FD_ZERO(&junkset);
    tenthsec.tv_sec = 0;
    tenthsec.tv_usec = 100000;

    XSync(gdisp->display,false);
    GDrawProcessPendingEvents((GDisplay *) gdisp);
    XSetErrorHandler(/*gdisp->display,*/qterror);

    /* Find the top window, some window managers will add two layers of */
    /*  decoration windows, (some might add none), some will have a virtualRoot*/
    while (1) {
	if ( XQueryTree(display,wind,&root,&par,&children,&junk)==0 ) {
	    XSetErrorHandler(/*gdisp->display,*/myerrorhandler);
return( gw->w );		/* How can it fail? It does though */
	}
	if (children)
	    XFree(children);
	if ( par==root || par==gdisp->virtualRoot ) {
	    gw->parentissimus = wind;
	    XSetErrorHandler(/*gdisp->display,*/myerrorhandler);
return(wind);
	}
	wind = par;
	select(0,&junkset,&junkset,&junkset,&tenthsec);
    }
}

/* ************************************************************************** */
/* ************************** Control Routines ****************************** */
/* ************************************************************************** */

static void GXDrawInit(GDisplay *gdisp) {
    _GXDraw_InitCols( (GXDisplay *) gdisp);
    _GXDraw_InitAtoms( (GXDisplay *) gdisp);
    _GXDraw_InitFonts((GXDisplay *) gdisp);
}

static void GXDrawTerm(GDisplay *gdisp) {
}

static void *GXDrawNativeDisplay(GDisplay *gdisp) {
return( ((GXDisplay *) gdisp)->display );
}

static GGC *_GXDraw_NewGGC() {
    GGC *ggc = calloc(1,sizeof(GGC));
    ggc->clip.width = ggc->clip.height = 0x7fff;
    ggc->fg = 0;
    ggc->bg = 0xffffff;
return( ggc );
}

static void GXDrawSetDefaultIcon(GWindow icon) {
    GXDisplay *gdisp = (GXDisplay *) (icon->display);

    gdisp->default_icon = (GXWindow) icon;
}

static Window MakeIconWindow(GXDisplay *gdisp, GXWindow pixmap) {
    XSetWindowAttributes attrs;
    unsigned long wmask = 0;
    
    if ( !gdisp->default_visual ) {
	attrs.colormap = gdisp->cmap;
	wmask |= CWColormap;
    }
    wmask |= CWBackPixmap;
    attrs.background_pixmap = pixmap->w;
return( XCreateWindow(gdisp->display, gdisp->root,
	    0, 0, pixmap->pos.width, pixmap->pos.height,
	    0,
	    gdisp->depth, InputOutput, gdisp->visual, wmask, &attrs));
}

#if 0
static void _GXDraw_DestroyWindow(GXDisplay *gdisp, GWindow input) {
  // TODO: Reconcile differences between this function (written from _GXDraw_CreateWindow)
  // with the actual function GXDrawDestroyWindow below.
  GXWindow inputc = (GXWindow)input;
  if (inputc->w != NULL) { 
    if (inputc->is_pixmap) {
      XFreePixmap(gdisp->display, inputc->w);
    } else {
      XDestroyWindow(gdisp->display, inputc->w);
    }
    inputc->w = NULL;
  }
  if (inputc->gc != NULL) { XFreeGC(gdisp->display, inputc->gc); inputc->gc = NULL; }
  free(input);
}
#endif // 0

static GWindow _GXDraw_CreateWindow(GXDisplay *gdisp, GXWindow gw, GRect *pos,
	int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
    Window parent;
    Display *display = gdisp->display;
    GXWindow nw = calloc(1,sizeof(struct gxwindow));
    XSetWindowAttributes attrs;
    static GWindowAttrs temp = GWINDOWATTRS_EMPTY;
    unsigned long wmask = 0;
    XClassHint ch;
    char *pt;

    if ( gw==NULL )
	gw = gdisp->groot;
    parent = gw->w;
    if ( wattrs==NULL ) wattrs = &temp;
    if ( nw==NULL )
return( NULL );
    nw->ggc = _GXDraw_NewGGC();
    if ( nw->ggc==NULL ) {
	free(nw);
return( NULL );
    }
    nw->display = gdisp;
    nw->eh = eh;
    nw->parent = gw;
    nw->pos = *pos;
    nw->user_data = user_data;

    attrs.bit_gravity = NorthWestGravity;
    wmask |= CWBitGravity;
    if ( (wattrs->mask&wam_bordcol) && wattrs->border_color!=COLOR_UNKNOWN ) {
	attrs.border_pixel = _GXDraw_GetScreenPixel(gdisp,wattrs->border_color);
	wmask |= CWBorderPixel;
    }
    if ( !(wattrs->mask&wam_backcol) || wattrs->background_color==COLOR_DEFAULT )
	wattrs->background_color = gdisp->def_background;
    if ( wattrs->background_color != COLOR_UNKNOWN ) {
	attrs.background_pixel = _GXDraw_GetScreenPixel(gdisp,wattrs->background_color);
	wmask |= CWBackPixel;
    }
    nw->ggc->bg = wattrs->background_color;
    if ( (wattrs->mask&wam_cursor) && wattrs->cursor!=0 ) {
	attrs.cursor = _GXDraw_GetCursor(gdisp,wattrs->cursor);
	wmask |= CWCursor;
    }
    if ( (wattrs->mask&wam_nodecor) && wattrs->nodecoration ) {
	attrs.override_redirect = true;
	wmask |= CWOverrideRedirect;
	nw->is_popup = true;
	nw->is_dlg = true;
	nw->not_restricted = true;
    }
    if ( (wattrs->mask&wam_isdlg) && wattrs->is_dlg ) {
	nw->is_dlg = true;
    }
    if ( (wattrs->mask&wam_notrestricted) && wattrs->not_restricted ) {
	nw->not_restricted = true;
    }
    if ( !gdisp->default_visual ) {
	attrs.colormap = gdisp->cmap;
	wmask |= CWColormap|CWBackPixel|CWBorderPixel;
	/* CopyFromParent doesn't work if we've got different visuals!!!! */
    }
    wmask |= CWEventMask;
    attrs.event_mask = ExposureMask|StructureNotifyMask/*|PropertyChangeMask*/;
    if ( gw==gdisp->groot )
	attrs.event_mask |= FocusChangeMask|EnterWindowMask|LeaveWindowMask;
    if ( wattrs->mask&wam_events ) {
	if ( wattrs->event_masks&(1<<et_char) )
	    attrs.event_mask |= KeyPressMask;
	if ( wattrs->event_masks&(1<<et_charup) )
	    attrs.event_mask |= KeyReleaseMask;
	if ( wattrs->event_masks&(1<<et_mousemove) )
	    attrs.event_mask |= PointerMotionMask;
	if ( wattrs->event_masks&(1<<et_mousedown) )
	    attrs.event_mask |= ButtonPressMask;
	if ( wattrs->event_masks&(1<<et_mouseup) )
	    attrs.event_mask |= ButtonReleaseMask;
	if ( (wattrs->event_masks&(1<<et_mouseup)) && (wattrs->event_masks&(1<<et_mousedown)) )
	    attrs.event_mask |= OwnerGrabButtonMask;
	if ( wattrs->event_masks&(1<<et_visibility) )
	    attrs.event_mask |= VisibilityChangeMask;
    }

    /* Only put the new dlgs underneath the cursor if focusfollows mouse, where */
    /*  they need to be there... */
    if ( gw == gdisp->groot &&
	    ( ((wattrs->mask&wam_centered) && wattrs->centered) ||
	      ((wattrs->mask&wam_undercursor) && wattrs->undercursor && !gdisp->focusfollowsmouse)) ) {
	pos->x = (gdisp->groot->pos.width-pos->width)/2;
	pos->y = (gdisp->groot->pos.height-pos->height)/2;
	if ( wattrs->centered==2 )
	    pos->y = (gdisp->groot->pos.height-pos->height)/3;
	nw->pos = *pos;
    } else if ( (wattrs->mask&wam_undercursor) && wattrs->undercursor && gw == gdisp->groot) {
	int junk;
	Window wjunk;
	int x, y; unsigned int state;

	XQueryPointer(display,gw->w,&wjunk,&wjunk,&junk,&junk,&x,&y,&state);
	pos->x = x-pos->width/2;
	pos->y = y-pos->height/2-(!gdisp->top_offsets_set?20:gdisp->off_y);
	if ( pos->x+pos->width>gdisp->groot->pos.width ) pos->x = gdisp->groot->pos.width-pos->width;
	if ( pos->x<0 ) pos->x = 0;
	if ( pos->y+pos->height>gdisp->groot->pos.height ) pos->y = gdisp->groot->pos.height-pos->height;
	if ( pos->y<0 ) pos->y = 0;
	nw->pos = *pos;
    }
    nw->w = XCreateWindow(display, parent,
	    pos->x, pos->y, pos->width, pos->height,
	    (wattrs->mask&wam_bordwidth)?wattrs->border_width:0,
	    gdisp->depth, InputOutput, gdisp->visual, wmask, &attrs);
    if ( gdisp->gcstate[0].gc==NULL ) {
	XGCValues vals;
	gdisp->gcstate[0].gc = XCreateGC(display,nw->w,0,&vals);
    }

    if ( gw == gdisp->groot ) {
	XWMHints wm_hints;
	XSizeHints s_h;
	wm_hints.flags = InputHint | StateHint;
	wm_hints.input = True;
	wm_hints.initial_state = NormalState;
	if ( ((wattrs->mask&wam_icon) && wattrs->icon!=NULL ) ||
		( !(wattrs->mask&wam_icon) && gdisp->default_icon!=NULL )) {
	    GXWindow icon = (wattrs->mask&wam_icon)? (GXWindow) (wattrs->icon) : gdisp->default_icon;
	    wm_hints.icon_pixmap = icon->w;
	    wm_hints.flags |= IconPixmapHint;
	    if ( !icon->ggc->bitmap_col && gdisp->depth!=1 ) {
		/* X Icons are bitmaps. If we want a pixmap we create a dummy */
		/*  window with the pixmap as background */
		wm_hints.icon_window = MakeIconWindow(gdisp,icon);
		wm_hints.flags |= IconWindowHint;
	    }
	}
	XSetWMHints(display,nw->w,&wm_hints);
	if ( (wattrs->mask&wam_wtitle) && wattrs->window_title!=NULL ) {
	    XmbSetWMProperties(display,nw->w,(pt = u2def_copy(wattrs->window_title)),NULL,NULL,0,NULL,NULL,NULL);
	    free(pt);
	}
	if ( (wattrs->mask&wam_ititle) && wattrs->icon_title!=NULL ) {
	    XmbSetWMProperties(display,nw->w,NULL,(pt = u2def_copy(wattrs->icon_title)),NULL,0,NULL,NULL,NULL);
	    free(pt);
	}
	if ( (wattrs->mask&wam_utf8_wtitle) && wattrs->utf8_window_title!=NULL ) {
#ifdef X_HAVE_UTF8_STRING
        Xutf8SetWMProperties(display, nw->w, wattrs->utf8_window_title, NULL, NULL, 0, NULL, NULL, NULL);
#else
	    unichar_t *tit = utf82u_copy(wattrs->utf8_window_title);
	    XmbSetWMProperties(display,nw->w,(pt = u2def_copy(tit)),NULL,NULL,0,NULL,NULL,NULL);
	    free(pt); free(tit);
#endif
	}
	if ( (wattrs->mask&wam_utf8_ititle) && wattrs->utf8_icon_title!=NULL ) {
#ifdef X_HAVE_UTF8_STRING
        Xutf8SetWMProperties(display, nw->w, NULL, wattrs->utf8_icon_title, NULL, 0, NULL, NULL, NULL);
#else
	    unichar_t *tit = utf82u_copy(wattrs->utf8_icon_title);
	    XmbSetWMProperties(display,nw->w,NULL,(pt = u2def_copy(tit)),NULL,0,NULL,NULL,NULL);
	    free(pt); free(tit);
#endif
	}
	s_h.x = pos->x; s_h.y = pos->y;
	s_h.base_width = s_h.width = pos->width; s_h.base_height = s_h.height = pos->height;
	s_h.min_width = s_h.max_width = s_h.width;
	s_h.min_height = s_h.max_height = s_h.height;
	s_h.flags = PPosition | PSize | PBaseSize;
	if (( (wattrs->mask&wam_positioned) && wattrs->positioned ) ||
		((wattrs->mask&wam_centered) && wattrs->centered ) ||
		((wattrs->mask&wam_undercursor) && wattrs->undercursor )) {
	    s_h.flags = USPosition | USSize | PBaseSize;
	    nw->was_positioned = true;
	}
	if ( (wattrs->mask&wam_noresize) && wattrs->noresize )
	    s_h.flags |= PMinSize | PMaxSize;
	XSetNormalHints(display,nw->w,&s_h);
	XSetWMProtocols(display,nw->w,&gdisp->atoms.wm_del_window,1);
	if ( wattrs->mask&wam_restrict )
	    nw->restrict_input_to_me = wattrs->restrict_input_to_me;
	if ( wattrs->mask&wam_redirect ) {
	    nw->redirect_chars_to_me = wattrs->redirect_chars_to_me;
	    nw->redirect_from = wattrs->redirect_from;
	}
	if ( (wattrs->mask&wam_transient) && wattrs->transient!=NULL ) {
	    XSetTransientForHint(display,nw->w,((GXWindow) (wattrs->transient))->w);
	    nw->istransient = true;
	    nw->transient_owner = ((GXWindow) (wattrs->transient))->w;
	    nw->is_dlg = true;
	} else if ( !nw->is_dlg )
	    ++gdisp->top_window_count;
	else if ( nw->restrict_input_to_me && gdisp->last_nontransient_window!=0 ) {
	    XSetTransientForHint(display,nw->w,gdisp->last_nontransient_window);
	    nw->transient_owner = gdisp->last_nontransient_window;
	    nw->istransient = true;
	}
	nw->isverytransient = (wattrs->mask&wam_verytransient)?1:0;
	nw->is_toplevel = true;
	XChangeProperty(display,nw->w,gdisp->atoms.wm_protocols,XA_ATOM,32,
		PropModeReplace,(unsigned char *) &gdisp->atoms.wm_del_window, 1);
    }
    ch.res_class = GResourceProgramName;
    ch.res_name = GResourceProgramName;
    XSetClassHint(display,nw->w,&ch);
    XSaveContext(display,nw->w,gdisp->mycontext,(void *) nw);
    if ( eh!=NULL ) {
	GEvent e;
	memset(&e,0,sizeof(e));
	e.type = et_create;
	e.w = (GWindow) nw;
	e.native_window = (void *) (intpt) nw->w;
	(eh)((GWindow) nw,&e);
    }
#ifndef _NO_LIBCAIRO
    /* Only do sub-pixel/anti-alias stuff if we've got truecolor */
    if ( gdisp->visual->class==TrueColor && !(wattrs->mask&wam_nocairo) &&_GXCDraw_hasCairo() )
	_GXCDraw_NewWindow(nw);
#endif
    /* Must come after the cairo init so pango will know to use cairo or xft */
    /* I think we will always want to use pango, so it isn't conditional on a wam */
    _GXPDraw_NewWindow(nw);
return( (GWindow) nw );
}

static GWindow GXDrawCreateTopWindow(GDisplay *gdisp, GRect *pos,
	int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
return( _GXDraw_CreateWindow((GXDisplay *) gdisp,NULL,pos,eh,user_data, wattrs));
}

static GWindow GXDrawCreateSubWindow(GWindow w, GRect *pos,
	int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
return( _GXDraw_CreateWindow(((GXWindow) w)->display,(GXWindow) w,pos,eh,user_data, wattrs));
}

static void GXDrawSetZoom(GWindow w, GRect *pos, enum gzoom_flags flags) {
    XSizeHints zoom, normal;
    Display *display = ((GXWindow) w)->display->display;
    long supplied_return;

    memset(&zoom,0,sizeof(zoom));
    if ( flags&gzf_pos ) {
	zoom.x = pos->x;
	zoom.y = pos->y;
	zoom.flags = PPosition;
    }
    if ( flags&gzf_size ) {
	zoom.width = zoom.base_width = zoom.max_width = pos->width;
	zoom.height = zoom.base_height = zoom.max_height = pos->height;
	zoom.flags |= PSize | PBaseSize | PMaxSize;
	XGetWMNormalHints(display,((GXWindow) w)->w,&normal,&supplied_return);
	normal.flags |= PMaxSize;
	normal.max_width = pos->width;
	normal.max_height = pos->height;
	XSetWMNormalHints(display,((GXWindow) w)->w,&normal);
    }
    XSetWMSizeHints(display,((GXWindow) w)->w,&zoom,XA_WM_ZOOM_HINTS);
}

static GWindow GXDrawCreatePixmap(GDisplay *gdisp, uint16 width, uint16 height) {
    GXWindow gw = calloc(1,sizeof(struct gxwindow));
    int wamcairo = false;

    if ( gw==NULL )
return( NULL );
    gw->ggc = _GXDraw_NewGGC();
    gw->ggc->bg = ((GXDisplay *) gdisp)->def_background;
    if ( gw->ggc==NULL ) {
	free(gw);
return( NULL );
    }
    if ( width&0x8000 ) {
	width &= 0x7fff;
	wamcairo = true;
    }
    // icon windows.
    if ( width == 48 ) {
	wamcairo = true;
    }
    
    gw->display = (GXDisplay *) gdisp;
    gw->is_pixmap = 1;
    gw->parent = NULL;
    gw->pos.x = gw->pos.y = 0;
    gw->pos.width = width; gw->pos.height = height;
    gw->w = XCreatePixmap(gw->display->display, gw->display->root, width, height, gw->display->depth);
#ifndef _NO_LIBCAIRO
    /* Only do sub-pixel/anti-alias stuff if we've got truecolor */
    if ( ((GXDisplay *) gdisp)->visual->class==TrueColor && wamcairo &&
	    _GXCDraw_hasCairo() )
	_GXCDraw_NewWindow(gw);
#endif
    /* Must come after the cairo init so pango will know to use cairo or xft */
    /* I think we will always want to use pango, so it isn't conditional */
    _GXPDraw_NewWindow(gw);
return( (GWindow) gw );
}

static GWindow GXDrawCreateBitmap(GDisplay *disp, uint16 width, uint16 height, uint8 *data) {
    GXDisplay *gdisp = (GXDisplay *) disp;
    GXWindow gw = calloc(1,sizeof(struct gxwindow));

    if ( gw==NULL )
return( NULL );
    gw->ggc = _GXDraw_NewGGC();
    if ( gw->ggc==NULL ) {
	free(gw);
return( NULL );
    }
    gw->ggc->bitmap_col = true;
    gw->display = (GXDisplay *) gdisp;
    gw->is_pixmap = 1;
    gw->parent = NULL;
    gw->pos.x = gw->pos.y = 0;
    gw->pos.width = width; gw->pos.height = height;
    if ( data==NULL )
	gw->w = XCreatePixmap(gdisp->display, gw->display->root, width, height, 1);
    else
	gw->w = XCreateBitmapFromData(gdisp->display, gw->display->root,
		(char *) data, width, height );
    if ( gdisp->gcstate[1].gc==NULL ) {
	XGCValues vals;
	gdisp->gcstate[1].gc = XCreateGC(gdisp->display,gw->w,0,&vals);
    }
return( (GWindow) gw );
}

static GCursor GXDrawCreateCursor(GWindow src,GWindow mask,Color fg,Color bg,
	int16 x, int16 y ) {
    GXDisplay *gdisp = (GXDisplay *) (src->display);
    Display *display = gdisp->display;
    XColor fgc, bgc;
    /* The XServer shipping with redhat 7.1 seems to suffer a protocol change */
    /*  with the red and blue members of XColor structure reversed */
    /* The XServer runing on Mac OS/X can only handle 16x16 cursors */

    fgc.red = COLOR_RED(fg)*0x101; fgc.green = COLOR_GREEN(fg)*0x101; fgc.blue = COLOR_BLUE(fg)*0x101;
    bgc.red = COLOR_RED(bg)*0x101; bgc.green = COLOR_GREEN(bg)*0x101; bgc.blue = COLOR_BLUE(bg)*0x101;
    fgc.pixel = _GXDraw_GetScreenPixel(gdisp,fg); fgc.flags = -1;
    bgc.pixel = _GXDraw_GetScreenPixel(gdisp,bg); bgc.flags = -1;
return( ct_user + XCreatePixmapCursor(display,((GXWindow) src)->w, ((GXWindow) mask)->w,
	&fgc,&bgc, x,y));
}

static void GTimerRemoveWindowTimers(GXWindow gw);

static void GXDrawDestroyWindow(GWindow w) {
    GXWindow gw = (GXWindow) w;

#ifndef _NO_LIBCAIRO
    if ( gw->usecairo )
	_GXCDraw_DestroyWindow(gw);
#endif
    _GXPDraw_DestroyWindow(gw);

    if ( gw->is_pixmap ) {
	XFreePixmap(gw->display->display,gw->w);
	free(gw->ggc);
	free(gw);
    } else {
	/*GTimerRemoveWindowTimers(gw);*/ /* Moved to _GXDraw_CleanUpWindow, not all windows are actively destroyed */
	gw->is_dying = true;
	if ( gw->display->grab_window==w ) gw->display->grab_window = NULL;
	XDestroyWindow(gw->display->display,gw->w);
	/* Windows should be freed when we get the destroy event */
    }
}

static void GXDestroyCursor(GDisplay *gdisp,GCursor ct) {
    XFreeCursor(((GXDisplay *) gdisp)->display, ct-ct_user);
}

static int GXNativeWindowExists(GDisplay *gdisp,void *native) {
    void *ret;

    if ( XFindContext(((GXDisplay *) gdisp)->display,(Window) (intpt) native,((GXDisplay *) gdisp)->mycontext,(void *) &ret)==0 &&
	    ret!=NULL )
return( true );

return( false );
}

static void GXDrawSetWindowBorder(GWindow w,int width,Color col) {
    GXWindow gw = (GXWindow) w;

    if ( width>=0 )
	XSetWindowBorderWidth(gw->display->display,gw->w,width);
    if ( col!=COLOR_DEFAULT )
	XSetWindowBorder(gw->display->display,gw->w,
		_GXDraw_GetScreenPixel(gw->display,col));
}

static void GXDrawSetWindowBackground(GWindow w,Color col) {
    GXWindow gw = (GXWindow) w;

    if ( col!=COLOR_DEFAULT )
	XSetWindowBackground(gw->display->display,gw->w,
		_GXDraw_GetScreenPixel(gw->display,col));
}

static int GXSetDither(GDisplay *gdisp,int dither) {
    int old = ((GXDisplay *) gdisp)->do_dithering;
    ((GXDisplay *) gdisp)->do_dithering = dither;
return( old );
}

static void _GXDraw_RemoveRedirects(GXDisplay *gdisp,GXWindow gw) {
    if ( gdisp->input!=NULL ) {
	struct inputRedirect *next=gdisp->input, *test;
	if ( next->cur_dlg == (GWindow) gw ) {
	    gdisp->input = next->prev;
	    free(next);
	} else for ( test=next->prev; test!=NULL; test=test->prev ) {
	    if ( test->cur_dlg == (GWindow) gw ) {
		next->prev = test->prev;
		free( test );
	break;
	    }
	}
    }
}

static void _GXDraw_CleanUpWindow( GWindow w ) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    int i;
    struct gxinput_context *gic, *next;

    XSaveContext(gdisp->display,gw->w,gdisp->mycontext,NULL);
    if ( gdisp->grab_window==w ) gdisp->grab_window = NULL;
    if ( gdisp->last_dd.gw==w ) {
	gdisp->last_dd.gw = NULL;
	gdisp->last_dd.w = None;
    }

    GTimerRemoveWindowTimers(gw);
    _GXDraw_RemoveRedirects(gdisp,gw);
    if ( gdisp->groot == gw->parent && !gw->is_dlg )
	--gdisp->top_window_count;

    /* If the window owns any selection it just lost them, cleanup our data */
    /*  structures... */
    for ( i = 0; i<sn_max; ++i ) {
	if ( gdisp->selinfo[i].owner == gw ) {
	    GXDrawClearSelData(gdisp,i);
	    gdisp->selinfo[i].owner = NULL;
	}
    }

    /* Does the window have any input contexts? If so get rid of them all */
    for ( gic = gw->all; gic!=NULL; gic = next ) {
	next = gic->next;
	XDestroyIC(gic->ic);
	free(gic);
    }

    free(gw->ggc);
    memset(gw,'\0',sizeof(*gw));
    free(gw);
}

static void GXDrawReparentWindow(GWindow child,GWindow newparent, int x,int y) {
    GXWindow gchild = (GXWindow) child, gpar = (GXWindow) newparent;
    GXDisplay *gdisp = gchild->display;
    /* Gnome won't let me reparent a top level window */
    /* It only pays attention to override-redirect if the window hasn't been mapped */
    XReparentWindow(gdisp->display,gchild->w,gpar->w,x,y);
}

static void GXDrawSetVisible(GWindow w, int visible) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;

    if( cmdlinearg_forceUIHidden )
	visible = false;
    
    gw->visible_request = visible;
    if ( visible ) {
	XMapWindow(gdisp->display,gw->w);
	if ( gw->restrict_input_to_me || gw->redirect_chars_to_me ||
		gw->redirect_from!=NULL ) {
	    struct inputRedirect *ir = calloc(1,sizeof(struct inputRedirect));
	    if ( ir!=NULL ) {
		ir->prev = gdisp->input;
		gdisp->input = ir;
		ir->cur_dlg = (GWindow) gw;
		if ( gw->redirect_from!=NULL ) {
		    ir->it = it_targetted;
		    ir->inactive = gw->redirect_from;
		} else if ( gw->redirect_chars_to_me )
		    ir->it = it_redirected;
		else
		    ir->it = it_restricted;
	    }
	}
    } else if ( !visible ) {
	if ( gw->is_toplevel && gw->is_visible ) {
	    /* Save the current position in the size hints. Otherwise some */
	    /*  window managers will pop it up where originally positioned */
	    /*  or if unpositioned ask user to position it. Ug */
	    XSizeHints s_h;
	    s_h.flags = USPosition;
	    s_h.x = gw->pos.x + gdisp->off_x;
	    s_h.y = gw->pos.y + gdisp->off_y;
	    XSetNormalHints(gdisp->display,gw->w,&s_h);
	}
	if (gw->is_toplevel)
		XWithdrawWindow(gdisp->display,gw->w,gdisp->screen);
	else
		XUnmapWindow(gdisp->display,gw->w);
	_GXDraw_RemoveRedirects(gdisp,gw);
    }
}

static void GXDrawMove(GWindow w, int32 x, int32 y) {
    GXWindow gw = (GXWindow) w;

    if ( gw->is_toplevel ) {
	/* Save the current position in the size hints. Otherwise some */
	/*  (if unmapped) window managers will pop it up where originally */
	/*  positioned or if unpositioned ask user to position it. Ug */
	XSizeHints s_h;
	s_h.flags = USPosition;
	s_h.x = x;
	s_h.y = y;
	XSetNormalHints(gw->display->display,gw->w,&s_h);
    }
    XMoveWindow(gw->display->display,gw->w,x,y);
}

static void GXDrawTrueMove(GWindow w, int32 x, int32 y) {
    GXWindow gw = (GXWindow) w;

    if ( gw->is_toplevel && !gw->is_popup && !gw->istransient ) {
	x -= gw->display->off_x;
	y -= gw->display->off_y;
    }
    GXDrawMove(w,x,y);
}

static void GXDrawResize(GWindow w, int32 width, int32 height) {
    GXWindow gw = (GXWindow) w;

    XResizeWindow(gw->display->display,gw->w,width,height);
    if ( gw->is_toplevel ) {
	XSizeHints s_h;
	/* for some reason the USPosition bit gets unset if I just set the width */
	s_h.flags = -1;		/* I don't know if this is needed, but let's be paranoid */
	XGetNormalHints(gw->display->display,gw->w,&s_h);
	s_h.flags |= USSize;
	s_h.width = width;
	s_h.height = height;
	XSetNormalHints(gw->display->display,gw->w,&s_h);
    }
}

static void GXDrawMoveResize(GWindow w, int32 x, int32 y, int32 width, int32 height) {
    GXWindow gw = (GXWindow) w;

    if ( gw->is_toplevel ) {
	/* Save the current position in the size hints. Otherwise some */
	/*  window managers will pop it up where originally positioned */
	/*  or if unpositioned ask user to position it. Ug */
	/* Might as well do the size too... */
	XSizeHints s_h;
	s_h.flags = USPosition|USSize;
	s_h.x = x;
	s_h.y = y;
	s_h.width = width;
	s_h.height = height;
	XSetNormalHints(gw->display->display,gw->w,&s_h);
    }
    XMoveResizeWindow(gw->display->display,gw->w,x,y,width,height);
}

static void GXDrawRaise(GWindow w) {
    GXWindow gw = (GXWindow) w;

    XRaiseWindow(gw->display->display,gw->w);
}

static GXDisplay *edisp;
static int error(Display *disp, XErrorEvent *err) {
    /* Under twm I get a bad match, under kde a bad window? */
    if ( err->error_code == BadMatch || err->error_code == BadWindow ) {
	if ( edisp!=NULL ) edisp->wm_breaks_raiseabove = true;
    } else {
	myerrorhandler(disp,err);
    }
return( 1 );
}

static void GXDrawRaiseAbove(GWindow w,GWindow below) {
    GXWindow gw = (GXWindow) w, gbelow = (GXWindow) below;
    Window gxw = gw->w, gxbelow = gbelow->w;
    GXDisplay *gdisp = gw->display;
    XWindowChanges ch;

    /* Sometimes we get a BadWindow error here for no good reason */
    XSync(gdisp->display,false);
    GDrawProcessPendingEvents((GDisplay *) gdisp);
    XSetErrorHandler(/*gdisp->display,*/error);
    if ( !gdisp->wm_raiseabove_tested ) {
	edisp = gdisp;
    } else
	edisp = NULL;
 retry:
    if ( gdisp->wm_breaks_raiseabove ) {
	/* If we do this code in gnome it breaks things */
	/* if we don't do it in twm it breaks things. Sigh */
	if ( gw->is_toplevel )
	    gxw = GetParentissimus(gw);
	if ( gbelow->is_toplevel )
	    gxbelow = GetParentissimus(gbelow);
    }
    ch.sibling = gxbelow;
    ch.stack_mode = Above;
    XConfigureWindow(gdisp->display,gxw,CWSibling|CWStackMode,&ch);
    XSync(gdisp->display,false);
    GDrawProcessPendingEvents((GDisplay *) gdisp);
    if ( !gdisp->wm_raiseabove_tested ) {
	gdisp->wm_raiseabove_tested = true;
	if ( gdisp->wm_breaks_raiseabove )
 goto retry;
    }
    XSetErrorHandler(/*gdisp->display,*/myerrorhandler);
}

static int GXDrawIsAbove(GWindow w,GWindow other) {
    GXWindow gw = (GXWindow) w, gother = (GXWindow) other;
    Window gxw = gw->w, gxother = gother->w, parent;
    GXDisplay *gdisp = (GXDisplay *) (gw->display);
    Window par, *children, root;
    unsigned int nkids; int i;

    if ( gw->is_toplevel && gother->is_toplevel ) {
	gxw = GetParentissimus(gw);
	gxother = GetParentissimus(gother);
	parent = gdisp->root;
    } else if ( gw->parent!=gother->parent )
return( -1 );			/* Incommensurate */
    else
	parent = gw->parent->w;

    XQueryTree(gdisp->display,parent,&root,&par,&children,&nkids);
    /* bottom-most child is children[0], topmost is children[nkids-1] */
    for ( i=nkids-1; i>=0; --i ) {
	if ( children[i] == gxw )
return( true );
	if ( children[i] == gxother )
return( false );
    }
    if ( children )
	XFree(children);
return( -1 );
}

static void GXDrawLower(GWindow w) {
    GXWindow gw = (GXWindow) w;

    XLowerWindow(gw->display->display,gw->w);
}

static void GXDrawSetWindowTitles(GWindow w, const unichar_t *title, const unichar_t *icontit) {
    GXWindow gw = (GXWindow) w;
    Display *display = gw->display->display;
    char *ipt, *tpt;

    XmbSetWMProperties(display,gw->w,(tpt = u2def_copy(title)),
			(ipt = u2def_copy(icontit)),
			NULL,0,NULL,NULL,NULL);
    free(ipt); free(tpt);
}

static void GXDrawSetWindowTitles8(GWindow w, const char *title, const char *icontit) {
    GXWindow gw = (GXWindow) w;
    Display *display = gw->display->display;
#ifdef X_HAVE_UTF8_STRING
    Xutf8SetWMProperties(display, gw->w, title, icontit, NULL, 0, NULL, NULL, NULL);
#else
    unichar_t *tit = utf82u_copy(title), *itit = utf82u_copy(icontit);
    char *ipt, *tpt;

    XmbSetWMProperties(display,gw->w,(tpt = u2def_copy(tit)),
			(ipt = u2def_copy(itit)),
			NULL,0,NULL,NULL,NULL);
    free(tit); free(tpt);
    free(itit); free(ipt);
#endif
}

static void GXDrawSetTransientFor(GWindow transient, GWindow owner) {
    GXWindow gw = (GXWindow) transient;
    GXDisplay *gdisp = gw->display;
    Display *display = gdisp->display;
    Window ow;

    if ( owner==(GWindow) -1 )
	ow = gdisp->last_nontransient_window;
    else if ( owner==NULL )
	ow = 0;
    else
	ow = ((GXWindow) owner)->w;
    XSetTransientForHint(display,gw->w, ow );
    gw->transient_owner = ow;
    gw->istransient = ow!=0;
}

static void GXDrawSetCursor(GWindow w, GCursor ct) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    Cursor cur = _GXDraw_GetCursor(gdisp,ct);

    XDefineCursor(gdisp->display,gw->w,cur);
    gw->cursor = ct;
}

static GCursor GXDrawGetCursor(GWindow w) {
    GXWindow gw = (GXWindow) w;

return( gw->cursor );
}

static GWindow GXDrawGetRedirectWindow(GDisplay *gd) {
    GXDisplay *gdisp = (GXDisplay *) gd;

    if ( gdisp->input==NULL )
return( NULL );

return( gdisp->input->cur_dlg );
}

static void GXDrawGetPointerPosition(GWindow w, GEvent *ret) {
    GXWindow gw = (GXWindow) w;
    Display *display = gw->display->display;
    int junk;
    Window wjunk;
    int x, y; unsigned int state;

    XQueryPointer(display,gw->w,&wjunk,&wjunk,&junk,&junk,&x,&y,&state);
    ret->u.mouse.state = state;
    ret->u.mouse.x = x;
    ret->u.mouse.y = y;
}

static Window _GXDrawGetPointerWindow(GWindow w) {
    GXWindow gw = (GXWindow) w;
    Display *display = gw->display->display;
    int junk;
    Window parent, child, wjunk;
    int x, y; unsigned int state;

    parent = gw->display->groot->w;
    for (;;) {
	child = None;
	if ( !XQueryPointer(display,parent,&wjunk,&child,&junk,&junk,&x,&y,&state))
    break;
	if ( child==None )
    break;
	parent = child;
    }
return( parent );
}

static GWindow GXDrawGetPointerWindow(GWindow w) {
    GXWindow gw = (GXWindow) w;
    Display *display = gw->display->display;
    void *ret;
    Window parent;

    parent = _GXDrawGetPointerWindow(w);
    if ( (gw->w&0xfff00000) == (parent&0xfff00000)) {
	/* It is one of our windows, so it is safe to look for it */
	if ( XFindContext(display,parent,gw->display->mycontext,(void *) &ret)==0 )
return( (GWindow) ret );
    }
return( NULL );
}

static char *GXDrawGetWindowTitle8(GWindow w);

static unichar_t *GXDrawGetWindowTitle(GWindow w) {
#if X_HAVE_UTF8_STRING
    char *ret1 = GXDrawGetWindowTitle8(w);
    unichar_t *ret = utf82u_copy(ret1);

    free(ret1);
return( ret );
#else
    GXWindow gw = (GXWindow) w;
    Display *display = gw->display->display;
    char *pt;
    unichar_t *ret;

    XFetchName(display,gw->w,&pt);
    ret = def2u_copy(pt);
    XFree(pt);
return( ret );
#endif
}

static char *GXDrawGetWindowTitle8(GWindow w) {
#if X_HAVE_UTF8_STRING
    GXWindow gw = (GXWindow) w;
    Display *display = gw->display->display;
    XTextProperty prop;
    char **propret;
    int cnt, i, len;
    char *ret;

    memset(&prop,0,sizeof(prop));
    XGetTextProperty(display,gw->w, &prop, XA_WM_NAME );
    if ( prop.value == NULL )
return( NULL );
    Xutf8TextPropertyToTextList(display,&prop,&propret,&cnt);
    XFree(prop.value);
    for ( i=len=0; i<cnt; ++i )
	len += strlen( propret[i]);
    ret = malloc(len+1);
    for ( i=len=0; i<cnt; ++i ) {
	strcpy(ret+len,propret[i]);
	len += strlen( propret[i]);
    }
    XFreeStringList( propret );
return( ret );
#else
    unichar_t *ret1 = GXDrawGetWindowTitle(w);
    char *ret = u2utf8_copy(ret1);

    free(ret1);
return( ret );
#endif
}

static void GXDrawTranslateCoordinates(GWindow _from,GWindow _to, GPoint *pt) {
    GXDisplay *gd = (GXDisplay *) ((_from!=NULL)?_from->display:_to->display);
    Window from = (_from==NULL)?gd->root:((GXWindow) _from)->w;
    Window to = (_to==NULL)?gd->root:((GXWindow) _to)->w;
    int x,y;
    Window child;

    XTranslateCoordinates(gd->display,from,to,pt->x,pt->y,&x,&y,&child);
    pt->x = x; pt->y = y;
}

static void GXDrawBeep(GDisplay *gdisp) {
    XBell(((GXDisplay *) gdisp)->display,80);
}

static void GXDrawFlush(GDisplay *gdisp) {
    XFlush(((GXDisplay *) gdisp)->display);
}
/* ************************************************************************** */
/* **************************** Draw Routines ******************************* */
/* ************************************************************************** */

void _GXDraw_SetClipFunc(GXDisplay *gdisp, GGC *mine) {
    XRectangle clip;
    XGCValues vals;
    long mask=0;
    GCState *gcs = &gdisp->gcstate[mine->bitmap_col];

    if ( mine->clip.x!=gcs->clip.x ||
	    mine->clip.width!=gcs->clip.width ||
	    mine->clip.y!=gcs->clip.y ||
	    mine->clip.height!=gcs->clip.height ) {
	clip.x = mine->clip.x; clip.y = mine->clip.y;
	clip.width = mine->clip.width;
	clip.height = mine->clip.height;
	XSetClipRectangles(gdisp->display,gcs->gc,0,0,&clip,1,YXBanded);
	gcs->clip = mine->clip;
    }
    if ( mine->copy_through_sub_windows != gcs->copy_through_sub_windows ) {
	vals.subwindow_mode = mine->copy_through_sub_windows?IncludeInferiors:ClipByChildren;
	mask |= GCSubwindowMode;
	gcs->copy_through_sub_windows = mine->copy_through_sub_windows;
    }
    if ( mask!=0 )
	XChangeGC(gdisp->display,gcs->gc,mask,&vals);
}

static int GXDrawSetcolfunc(GXDisplay *gdisp, GGC *mine) {
    XGCValues vals;
    long mask=0;
    GCState *gcs = &gdisp->gcstate[mine->bitmap_col];

    _GXDraw_SetClipFunc(gdisp,mine);
    if ( mine->fg!=gcs->fore_col ) {
	if ( mine->bitmap_col ) {
	    vals.foreground = mine->fg;
	} else {
	    vals.foreground = _GXDraw_GetScreenPixel(gdisp,mine->fg);
	}
	gcs->fore_col = mine->fg;
	mask |= GCForeground;
    }
    if ( mine->bg!=gcs->back_col ) {
	vals.background = _GXDraw_GetScreenPixel(gdisp,mine->bg);
	mask |= GCBackground;
	gcs->back_col = mine->bg;
    }
    if ( mine->ts != gcs->ts || mine->ts != 0 ||
	    mine->ts_xoff != gcs->ts_xoff ||
	    mine->ts_yoff != gcs->ts_yoff ) {
	if ( mine->ts!=0 ) {
	    vals.stipple = mine->ts==1?gdisp->grey_stipple: gdisp->fence_stipple;
	    mask |= GCStipple;
	}
	vals.fill_style = (mine->ts?FillStippled:FillSolid);
	vals.ts_x_origin = mine->ts_xoff;
	vals.ts_y_origin = mine->ts_yoff;
	mask |= GCTileStipXOrigin|GCTileStipYOrigin|GCFillStyle;
	gcs->ts = mine->ts;
	gcs->ts_xoff = mine->ts_xoff;
	gcs->ts_yoff = mine->ts_yoff;
    }
    if ( mask!=0 )
	XChangeGC(gdisp->display,gcs->gc,mask,&vals);
return( true );
}

static int GXDrawSetline(GXDisplay *gdisp, GGC *mine) {
    XGCValues vals;
    long mask=0;
    GCState *gcs = &gdisp->gcstate[mine->bitmap_col];

    _GXDraw_SetClipFunc(gdisp,mine);
    if ( mine->fg!=gcs->fore_col ) {
	if ( mine->bitmap_col ) {
	    vals.foreground = mine->fg;
	} else {
	    vals.foreground = _GXDraw_GetScreenPixel(gdisp,mine->fg);
	}
	gcs->fore_col = mine->fg;
	mask |= GCForeground;
    }
    if ( mine->line_width==1 ) mine->line_width = 0;
    if ( mine->line_width!=gcs->line_width ) {
	vals.line_width = mine->line_width;
	mask |= GCLineWidth;
	gcs->line_width = mine->line_width;
    }
    if ( mine->dash_len != gcs->dash_len || mine->skip_len != gcs->skip_len ||
	    mine->dash_offset != gcs->dash_offset ) {
	vals.line_style = mine->dash_len==0?LineSolid:LineOnOffDash;
	mask |= GCLineStyle;
	if ( vals.line_style!=LineSolid ) {
	    if ( mine->dash_len==mine->skip_len ) {
		vals.dash_offset = mine->dash_offset;
		vals.dashes = mine->dash_len;
		mask |= GCDashOffset|GCDashList;
	    } else {
		char dashes[2];
		dashes[0] = mine->dash_len; dashes[1] = mine->skip_len;
		XSetDashes(gdisp->display,gcs->gc,mine->dash_offset,dashes,2);
	    }
	}
	gcs->dash_offset = mine->dash_offset;
	gcs->dash_len = mine->dash_len;
	gcs->skip_len = mine->skip_len;
    }
    if ( mine->ts != gcs->ts ||
	    mine->ts_xoff != gcs->ts_xoff ||
	    mine->ts_yoff != gcs->ts_yoff ) {
	if ( mine->ts!=0 ) {
	    vals.stipple = mine->ts==1 ? gdisp->grey_stipple : gdisp->fence_stipple;
	    mask |= GCStipple;
	    if ( !mine->bitmap_col ) {
		/* For reasons inexplicable to me, X sometimes draws with OpaqueStippled */
		vals.background = _GXDraw_GetScreenPixel(gdisp,gcs->back_col);
		mask |= GCBackground;
	    }
	}
	vals.fill_style = (mine->ts?FillStippled:FillSolid);
	vals.ts_x_origin = mine->ts_xoff;
	vals.ts_y_origin = mine->ts_yoff;
	mask |= GCTileStipXOrigin|GCTileStipYOrigin|GCFillStyle;
	gcs->ts = mine->ts;
	gcs->ts_xoff = mine->ts_xoff;
	gcs->ts_yoff = mine->ts_yoff;
    }
    if ( mask!=0 )
	XChangeGC(gdisp->display,gcs->gc,mask,&vals);
return( true );
}

static void GXDrawPushClipOnly(GWindow w)
{
#ifndef _NO_LIBCAIRO
    if ( ((GXWindow) w)->usecairo )
        _GXCDraw_PushClipOnly((GXWindow) w);
#endif
}

static void GXDrawClipPreserve(GWindow w)
{
#ifndef _NO_LIBCAIRO
    if ( ((GXWindow) w)->usecairo )
        _GXCDraw_ClipPreserve((GXWindow) w);
#endif
}

static void GXDrawSetDifferenceMode(GWindow w) {
#ifndef _NO_LIBCAIRO
    if (((GXWindow) w)->usecairo) {
        _GXCDraw_SetDifferenceMode((GXWindow)w);
    } else
#endif
    {
        GXDisplay *gdisp = ((GXWindow) w)->display; ;
        XSetFunction(gdisp->display, gdisp->gcstate[((GXWindow) w)->ggc->bitmap_col].gc, GXxor);
    }
}

static void GXDrawPushClip(GWindow w, GRect *rct, GRect *old) {
    /* return the current clip, and intersect the current clip with the desired */
    /*  clip to get the new */
    *old = w->ggc->clip;
    w->ggc->clip = *rct;
    if ( w->ggc->clip.x+w->ggc->clip.width>old->x+old->width )
	w->ggc->clip.width = old->x+old->width-w->ggc->clip.x;
    if ( w->ggc->clip.y+w->ggc->clip.height>old->y+old->height )
	w->ggc->clip.height = old->y+old->height-w->ggc->clip.y;
    if ( w->ggc->clip.x<old->x ) {
	if ( w->ggc->clip.width > (old->x-w->ggc->clip.x))
	    w->ggc->clip.width -= (old->x-w->ggc->clip.x);
	else
	    w->ggc->clip.width = 0;
	w->ggc->clip.x = old->x;
    }
    if ( w->ggc->clip.y<old->y ) {
	if ( w->ggc->clip.height > (old->y-w->ggc->clip.y))
	    w->ggc->clip.height -= (old->y-w->ggc->clip.y);
	else
	    w->ggc->clip.height = 0;
	w->ggc->clip.y = old->y;
    }
    if ( w->ggc->clip.height<0 || w->ggc->clip.width<0 ) {
	/* Negative values mean large positive values, so if we want to clip */
	/*  to nothing force clip outside window */
	w->ggc->clip.x = w->ggc->clip.y = -100;
	w->ggc->clip.height = w->ggc->clip.width = 1;
    }
#ifndef _NO_LIBCAIRO
    if ( ((GXWindow) w)->usecairo )
	_GXCDraw_PushClip((GXWindow) w);
#endif
}

static void GXDrawPopClip(GWindow w, GRect *old) {
    w->ggc->clip = *old;
#ifndef _NO_LIBCAIRO
    if ( ((GXWindow) w)->usecairo )
	_GXCDraw_PopClip((GXWindow) w);
    else
#endif
    {
        GXDisplay *gdisp = ((GXWindow) w)->display; ;
        XSetFunction(gdisp->display, gdisp->gcstate[((GXWindow) w)->ggc->bitmap_col].gc, GXcopy);
    }
}



static void GXDrawClear(GWindow gw, GRect *rect) {
    GXWindow gxw = (GXWindow) gw;
#ifndef _NO_LIBCAIRO
    if ( gxw->usecairo )
	_GXCDraw_Clear(gxw,rect);
    else
#endif
    {
	GXDisplay *display = (GXDisplay *) (gw->display);

	if ( rect==NULL )
	    XClearWindow(display->display,gxw->w);
	else
	    XClearArea(display->display,gxw->w,
		    rect->x,rect->y,rect->width,rect->height, false );
    }
 }

static void GXDrawDrawLine(GWindow w, int32 x,int32 y, int32 xend,int32 yend, Color col) {
    w->ggc->fg = col;

#ifndef _NO_LIBCAIRO
    if ( ((GXWindow) w)->usecairo ) {
	_GXCDraw_DrawLine((GXWindow) w,x,y,xend,yend);
    } else
#endif
    {
	GXDisplay *display = (GXDisplay *) (w->display);
	GXDrawSetline(display,w->ggc);
	XDrawLine(display->display,((GXWindow) w)->w,display->gcstate[w->ggc->bitmap_col].gc,x,y,xend,yend);
    }
}

static void _DrawArrow(GXWindow gxw, int32 x, int32 y, int32 xother, int32 yother ) {
    GXDisplay *display = gxw->display;
    XPoint points[3];
    double a;
    int off1, off2;
    double len;

    if ( x==xother && y==yother )
return;
    a = atan2(y-yother,x-xother);
    len = sqrt((double) (x-xother)*(x-xother)+(y-yother)*(y-yother));
    if ( len>20 ) len = 10; else len = 2*len/3;
    if ( len<2 )
return;

    points[0].x = x; points[0].y = y;
    off1 = len*sin(a+3.1415926535897932/8)+.5; off2 = len*cos(a+3.1415926535897932/8)+.5;
    points[1].x = x-off2; points[1].y = y-off1;
    off1 = len*sin(a-3.1415926535897932/8)+.5; off2 = len*cos(a-3.1415926535897932/8)+.5;
    points[2].x = x-off2; points[2].y = y-off1;
    XFillPolygon(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,points,3,Complex,CoordModeOrigin);
    XDrawLines(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,points,3,CoordModeOrigin);
}

static void GXDrawDrawArrow(GWindow gw, int32 x,int32 y, int32 xend,int32 yend, int16 arrows, Color col) {
    GXWindow gxw = (GXWindow) gw;
    GXDisplay *display = gxw->display;
    gxw->ggc->fg = col;

#ifndef _NO_LIBCAIRO
    if ( gxw->usecairo )
	GDrawIError("DrawArrow not supported");
#endif
    GXDrawSetline(display,gxw->ggc);
    XDrawLine(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,x,y,xend,yend);
    if ( arrows&1 )
	_DrawArrow(gxw,x,y,xend,yend);
    if ( arrows&2 )
	_DrawArrow(gxw,xend,yend,x,y);
}

static void GXDrawDrawRect(GWindow gw, GRect *rect, Color col) {
    GXWindow gxw = (GXWindow) gw;

    gxw->ggc->fg = col;
#ifndef _NO_LIBCAIRO
    if (gxw->usecairo)
        _GXCDraw_DrawRect(gxw, rect); //Assume copy, ignore XOR?
    else
#endif
    {
	GXDisplay *display = gxw->display;

	GXDrawSetline(display,gxw->ggc);
	XDrawRectangle(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,rect->x,rect->y,
		rect->width,rect->height);
    }
}

static void GXDrawFillRect(GWindow gw, GRect *rect, Color col) {
    GXWindow gxw = (GXWindow) gw;

    gxw->ggc->fg = col;
#ifndef _NO_LIBCAIRO
    if (gxw->usecairo)
        _GXCDraw_FillRect(gxw,rect);
    else
#endif
    {
	GXDisplay *display = gxw->display;

	GXDrawSetcolfunc(display,gxw->ggc);
	XFillRectangle(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,rect->x,rect->y,
		rect->width,rect->height);
    }
}

static void GXDrawFillRoundRect(GWindow gw, GRect *rect, int radius, Color col) {
    GXWindow gxw = (GXWindow) gw;
    int rr = radius <= (rect->height+1)/2 ? (radius > 0 ? radius : 0) : (rect->height+1)/2;

    gxw->ggc->fg = col;
#ifndef _NO_LIBCAIRO
    if (gxw->usecairo)
        _GXCDraw_FillRoundRect( gxw,rect,rr );
    else
#endif
    {
	GRect middle = {rect->x, rect->y + radius, rect->width, rect->height - 2 * radius};
	int xend = rect->x + rect->width - 1;
	int yend = rect->y + rect->height - 1;
	int precalc = rr * 2 - 1;
	int i, xoff;

	for (i = 0; i < rr; i++) {
	    xoff = rr - lrint(sqrt( (double)(i * (precalc - i)) ));
	    GXDrawDrawLine(gw, rect->x + xoff, rect->y + i, xend - xoff, rect->y + i, col);
	    GXDrawDrawLine(gw, rect->x + xoff, yend - i, xend - xoff, yend - i, col);
	}
	GXDrawFillRect(gw, &middle, col);
    }
}

static void GXDrawDrawElipse(GWindow gw, GRect *rect, Color col) {
    GXWindow gxw = (GXWindow) gw;

    gxw->ggc->fg = col;
#ifndef _NO_LIBCAIRO
    if ( gxw->usecairo ) {
	_GXCDraw_DrawEllipse( gxw,rect);
    } else
#endif
    {
	GXDisplay *display = gxw->display;

	GXDrawSetline(display,gxw->ggc);
	XDrawArc(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,rect->x,rect->y,
		rect->width,rect->height,0,360*64);
    }
}

static void GXDrawDrawArc(GWindow gw, GRect *rect, int32 sangle, int32 tangle, Color col) {
    GXWindow gxw = (GXWindow) gw;
    GXDisplay *display = gxw->display;
    gxw->ggc->fg = col;
#ifndef _NO_LIBCAIRO
    if (gxw->usecairo) {
        // Leftover from XDrawArc: sangle/tangle in degrees*64.
        _GXCDraw_DrawArc(gxw, rect, -(sangle+tangle)*M_PI/11520., -sangle*M_PI/11520.);
    } else
#endif
    {
    GXDrawSetline(display,gxw->ggc);
    XDrawArc(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,rect->x,rect->y,
	    rect->width,rect->height,
	    sangle,tangle );
    }
}

static void GXDrawFillElipse(GWindow gw, GRect *rect, Color col) {
    GXWindow gxw = (GXWindow) gw;
    GXDisplay *display = gxw->display;

    gxw->ggc->fg = col;
#ifndef _NO_LIBCAIRO
    if ( gxw->usecairo ) {
	_GXCDraw_FillEllipse( gxw,rect);
    } else
#endif
    {
	GXDrawSetcolfunc(display,gxw->ggc);
	XFillArc(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,rect->x,rect->y,
		rect->width,rect->height,0,360*64);
    }
}

static void GXDrawDrawPoly(GWindow gw, GPoint *pts, int16 cnt, Color col) {
    GXWindow gxw = (GXWindow) gw;
    GXDisplay *display = gxw->display;

    gxw->ggc->fg = col;
#ifndef _NO_LIBCAIRO
    if ( gxw->usecairo ) {
	_GXCDraw_DrawPoly( gxw,pts,cnt);
    } else
#endif
    {
	GXDrawSetline(display,gxw->ggc);
	XDrawLines(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,(XPoint *) pts,cnt,CoordModeOrigin);
    }
}

static void GXDrawFillPoly(GWindow gw, GPoint *pts, int16 cnt, Color col) {
    GXWindow gxw = (GXWindow) gw;
    GXDisplay *display = gxw->display;

    gxw->ggc->fg = col;
#ifndef _NO_LIBCAIRO
    if ( gxw->usecairo ) {
	_GXCDraw_FillPoly( gxw,pts,cnt);
    } else
#endif
    {
	GXDrawSetline(display,gxw->ggc);		/* Polygons draw their borders too! so we need the line mode */
	GXDrawSetcolfunc(display,gxw->ggc);	
	XFillPolygon(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,(XPoint *) pts,cnt,Complex,CoordModeOrigin);
	XDrawLines(display->display,gxw->w,display->gcstate[gxw->ggc->bitmap_col].gc,(XPoint *) pts,cnt,CoordModeOrigin);
    }
}

#ifndef _NO_LIBCAIRO
static enum gcairo_flags GXDrawHasCairo(GWindow w) {
    if ( ((GXWindow) w)->usecairo )
return( _GXCDraw_CairoCapabilities( (GXWindow) w));

return( gc_xor );
}

static void GXDrawPathStartNew(GWindow w) {
    if ( !((GXWindow) w)->usecairo )
return;
    _GXCDraw_PathStartNew(w);
}

static void GXDrawPathStartSubNew(GWindow w) {
    if ( !((GXWindow) w)->usecairo )
return;
    _GXCDraw_PathStartSubNew(w);
}

static int GXDrawFillRuleSetWinding(GWindow w) {
    if ( !((GXWindow) w)->usecairo )
return 0;
    return _GXCDraw_FillRuleSetWinding(w);
}

static void GXDrawPathClose(GWindow w) {
    if ( !((GXWindow) w)->usecairo )
return;
    _GXCDraw_PathClose(w);
}

static void GXDrawPathMoveTo(GWindow w,double x, double y) {
    if ( !((GXWindow) w)->usecairo )
return;
    _GXCDraw_PathMoveTo(w,x,y);
}

static void GXDrawPathLineTo(GWindow w,double x, double y) {
    if ( !((GXWindow) w)->usecairo )
return;
    _GXCDraw_PathLineTo(w,x,y);
}

static void GXDrawPathCurveTo(GWindow w,
		    double cx1, double cy1,
		    double cx2, double cy2,
		    double x, double y) {
    if ( !((GXWindow) w)->usecairo )
return;
    _GXCDraw_PathCurveTo(w,cx1,cy1,cx2,cy2,x,y);
}

static void GXDrawPathStroke(GWindow w,Color col) {
    if ( !((GXWindow) w)->usecairo )
return;
    _GXCDraw_PathStroke(w,col);
}

static void GXDrawPathFill(GWindow w,Color col) {
    if ( !((GXWindow) w)->usecairo )
return;
    _GXCDraw_PathFill(w,col);
}

static void GXDrawPathFillAndStroke(GWindow w,Color fillcol, Color strokecol) {
    if ( !((GXWindow) w)->usecairo )
return;
    _GXCDraw_PathFillAndStroke(w,fillcol,strokecol);
}

#else
static enum gcairo_flags GXDrawHasCairo(GWindow w) {
return( gc_xor );
}

static void GXDrawPathStartNew(GWindow w) {
}

static void GXDrawPathStartSubNew(GWindow w) {
}

static int GXDrawFillRuleSetWinding(GWindow w) {
    return 0;
}

static void GXDrawPathClose(GWindow w) {
}

static void GXDrawPathMoveTo(GWindow w,double x, double y) {
}

static void GXDrawPathLineTo(GWindow w,double x, double y) {
}

static void GXDrawPathCurveTo(GWindow w,
		    double cx1, double cy1,
		    double cx2, double cy2,
		    double x, double y) {
}

static void GXDrawPathStroke(GWindow w,Color col) {
}

static void GXDrawPathFill(GWindow w,Color col) {
}

static void GXDrawPathFillAndStroke(GWindow w,Color fillcol, Color strokecol) {
}

#endif

static void GXDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi) {
    _GXPDraw_LayoutInit(w,text,cnt,fi);
}

static void GXDraw_LayoutDraw(GWindow w, int32 x, int32 y, Color fg) {
    _GXPDraw_LayoutDraw(w,x,y,fg);
}

static void GXDraw_LayoutIndexToPos(GWindow w, int index, GRect *pos) {
    _GXPDraw_LayoutIndexToPos(w,index,pos);
}

static int GXDraw_LayoutXYToIndex(GWindow w, int x, int y) {
return( _GXPDraw_LayoutXYToIndex(w,x,y));
}

static void GXDraw_LayoutExtents(GWindow w, GRect *size) {
    _GXPDraw_LayoutExtents(w,size);
}

static void GXDraw_LayoutSetWidth(GWindow w, int width) {
    _GXPDraw_LayoutSetWidth(w,width);
}

static int GXDraw_LayoutLineCount(GWindow w) {
return( _GXPDraw_LayoutLineCount(w));
}

static int GXDraw_LayoutLineStart(GWindow w, int l) {
return( _GXPDraw_LayoutLineStart(w, l));
}

static void GXDrawSendExpose(GXWindow gw, int x,int y,int wid,int hei ) {
    if ( gw->eh!=NULL ) {
	struct gevent event;
	memset(&event,0,sizeof(event));
	event.type = et_expose;
	if ( x<0 ) { wid += x; x = 0; }
	if ( y<0 ) { hei += y; y = 0; }
	event.u.expose.rect.x = x;
	event.u.expose.rect.y = y;
	if ( x+wid>gw->pos.width ) wid = gw->pos.width-x;
	if ( y+hei>gw->pos.height ) hei = gw->pos.height-y;
	if ( wid<0 || hei<0 )
return;
	event.u.expose.rect.width = wid;
	event.u.expose.rect.height = hei;
	event.w = (GWindow) gw;
	event.native_window = ((GWindow) gw)->native_window;
	(gw->eh)((GWindow ) gw,&event);
    }
}

static void GXDrawScroll(GWindow _w, GRect *rect, int32 hor, int32 vert) {
    GXWindow gw = (GXWindow) _w;
    GXDisplay *gdisp = gw->display;
    GRect temp, old;

    vert = -vert;

    if ( rect == NULL ) {
	temp.x = temp.y = 0; temp.width = gw->pos.width; temp.height = gw->pos.height;
	rect = &temp;
    }

    /*GDrawForceUpdate((GWindow) gw); */	/* need to make sure the screen holds what it should */
		/* but user has to do it, it's probably too late here */
    GDrawPushClip(_w,rect,&old);
#ifdef _COMPOSITE_BROKEN
    GXDrawSendExpose(gw,0,0,gw->pos.width,gw->pos.height);
#else
    _GXDraw_SetClipFunc(gdisp,gw->ggc);
#ifndef _NO_LIBCAIRO
    if ( gw->usecairo ) {
	/* Cairo can happily scroll the window -- except it doesn't know about*/
	/*  child windows, and so we don't get the requisit events to redraw */
	/*  areas covered by children. Rats. */
	GXDrawSendExpose(gw,rect->x,rect->y,rect->x+rect->width,rect->y+rect->height);
	GXDrawPopClip(_w,&old);
return;
	/* _GXCDraw_CopyArea(gw,gw,rect,rect->x+hor,rect->y+vert); */
    } else
#endif
	XCopyArea(gdisp->display,gw->w,gw->w,gdisp->gcstate[gw->ggc->bitmap_col].gc,
		rect->x,rect->y,	rect->width,rect->height,
		rect->x+hor,rect->y+vert);
    if ( hor>0 )
	GXDrawSendExpose(gw,rect->x,rect->y, hor,rect->height);
    else if ( hor<0 )
	GXDrawSendExpose(gw,rect->x+rect->width+hor,rect->y,-hor,rect->height);
    if ( vert>0 )
	GXDrawSendExpose(gw,rect->x,rect->y,rect->width,vert);
    else if ( vert<0 )
	GXDrawSendExpose(gw,rect->x,rect->y+rect->height+vert,rect->width,-vert);
#endif
    GXDrawPopClip(_w,&old);
}

static void _GXDraw_Pixmap( GWindow _w, GWindow _pixmap, GRect *src, int32 x, int32 y) {
    GXWindow gw = (GXWindow) _w, pixmap = (GXWindow) _pixmap;
    GXDisplay *gdisp = gw->display;

    if ( pixmap->ggc->bitmap_col ) {
	GXDrawSetcolfunc(gdisp,gw->ggc);
	XCopyPlane(gdisp->display,pixmap->w,gw->w,gdisp->gcstate[gw->ggc->bitmap_col].gc,
		src->x,src->y,	src->width,src->height,
		x,y,1);
    } else {
	_GXDraw_SetClipFunc(gdisp,gw->ggc);
#ifndef _NO_LIBCAIRO
	/* FIXME: _GXCDraw_CopyArea makes the glyph dissabear in the class kern
	 * dialog */
	if ( 0 && gw->usecairo )
	    _GXCDraw_CopyArea(pixmap,gw,src,x,y);
	else
#endif
	    XCopyArea(gdisp->display,pixmap->w,gw->w,gdisp->gcstate[gw->ggc->bitmap_col].gc,
		    src->x,src->y,	src->width,src->height,
		    x,y);
    }
}

static void _GXDraw_TilePixmap( GWindow _w, GWindow _pixmap, GRect *src, int32 x, int32 y) {
    GXWindow gw = (GXWindow) _w, pixmap = (GXWindow) _pixmap;
    GXDisplay *gdisp = gw->display;
    GRect old;
    int i,j;

    GDrawPushClip(_w,src,&old);
    GXDrawSetcolfunc(gdisp,gw->ggc);
    for ( i=y; i<gw->ggc->clip.y+gw->ggc->clip.height; i+=pixmap->pos.height ) {
	if ( i+pixmap->pos.height<gw->ggc->clip.y )
    continue;
	for ( j=x; j<gw->ggc->clip.x+gw->ggc->clip.width; j+=pixmap->pos.width ) {
	    if ( j+pixmap->pos.width<gw->ggc->clip.x )
	continue;
	    if ( pixmap->ggc->bitmap_col ) {
		XCopyPlane(gdisp->display,((GXWindow) pixmap)->w,gw->w,gdisp->gcstate[1].gc,
			0,0,  pixmap->pos.width, pixmap->pos.height,
			j,i,1);
#ifndef _NO_LIBCAIRO
	    } else if ( gw->usecairo ) {
		_GXCDraw_CopyArea(pixmap,gw,&pixmap->pos,j,i);
#endif
	    } else {
		XCopyArea(gdisp->display,((GXWindow) pixmap)->w,gw->w,gdisp->gcstate[0].gc,
			0,0,  pixmap->pos.width, pixmap->pos.height,
			j,i);
	    }
	}
    }
    GDrawPopClip(_w,&old);
}

static void GXDrawFontMetrics( GWindow w,GFont *fi,int *as, int *ds, int *ld) {
    _GXPDraw_FontMetrics(w, fi, as, ds, ld);
}
    

static GIC *GXDrawCreateInputContext(GWindow w,enum gic_style def_style) {
    static int styles[] = { XIMPreeditNone | XIMStatusNone,
	    XIMPreeditNothing | XIMStatusNothing,
	    XIMPreeditPosition | XIMStatusNothing };
    int i;
    XIC ic = 0;
    struct gxinput_context *gic;
    GXDisplay *gdisp = (GXDisplay *) (w->display);
    unsigned long fevent;
    XWindowAttributes win_attrs;
    XVaNestedList listp, lists;

    if ( gdisp->im==NULL )
return( NULL );

    gic = calloc(1,sizeof(struct gxinput_context));
    gic->w = w;
    gic->ploc.y = 20; gic->sloc.y = 40;
    listp = XVaCreateNestedList(0, XNFontSet, gdisp->def_im_fontset,
		    XNForeground, _GXDraw_GetScreenPixel(gdisp,gdisp->def_foreground),
		    XNBackground, _GXDraw_GetScreenPixel(gdisp,gdisp->def_background),
		    XNSpotLocation, &gic->ploc, NULL);
    lists = XVaCreateNestedList(0, XNFontSet, gdisp->def_im_fontset,
		    XNForeground, _GXDraw_GetScreenPixel(gdisp,gdisp->def_foreground),
		    XNBackground, _GXDraw_GetScreenPixel(gdisp,gdisp->def_background),
		    XNSpotLocation, &gic->sloc, NULL);
    for ( i=(def_style&gic_type); i>=gic_hidden; --i ) {
	ic = XCreateIC(gdisp->im,XNInputStyle,styles[i],
		    XNClientWindow, ((GXWindow) w)->w,
		    XNFocusWindow, ((GXWindow) w)->w,
		    XNPreeditAttributes, listp,
		    XNStatusAttributes, lists,
		    NULL );
	if ( ic!=0 )
    break;
	if ( !(def_style&gic_orlesser) )
    break;
    }
    XFree(lists); XFree(listp);
    if ( ic==0 ) {
	free(gic);
return( NULL );
    }

    gic->style = i;
    gic->w = w;
    gic->ic = ic;
    gic->next = ((GXWindow) w)->all;
    ((GXWindow) w)->all = gic;

    /* Now make sure we get all the events the IC needs */
    XGetWindowAttributes(gdisp->display, ((GXWindow) w)->w, &win_attrs);
    XGetICValues(ic, XNFilterEvents, &fevent, NULL);
    XSelectInput(gdisp->display, ((GXWindow) w)->w, fevent|win_attrs.your_event_mask);

return( (GIC *) gic );
}

static void GXDrawSetGIC(GWindow w, GIC *_gic, int x, int y) {
    struct gxinput_context *gic = (struct gxinput_context *) _gic;
    XVaNestedList listp, lists;
    GXDisplay *gdisp = (GXDisplay *) (w->display);

    if ( x==10000 && y==x && gic!=NULL ) {
	XUnsetICFocus(gic->ic);
    } else if ( gic!=NULL ) {
	gic->ploc.x = x;
	gic->ploc.y = y;
	gic->sloc.x = x;
	gic->sloc.y = y+20;
	XSetICFocus(gic->ic);
	if ( gic->style==gic_overspot ) {
	    listp = XVaCreateNestedList(0, XNFontSet, gdisp->def_im_fontset,
		    XNForeground, _GXDraw_GetScreenPixel(gdisp,gdisp->def_foreground),
		    XNBackground, _GXDraw_GetScreenPixel(gdisp,gdisp->def_background),
		    XNSpotLocation, &gic->ploc, NULL);
	    lists = XVaCreateNestedList(0, XNFontSet, gdisp->def_im_fontset,
		    XNForeground, _GXDraw_GetScreenPixel(gdisp,gdisp->def_foreground),
		    XNBackground, _GXDraw_GetScreenPixel(gdisp,gdisp->def_background),
		    XNSpotLocation, &gic->sloc, NULL);
	    XSetICValues(gic->ic,
		    XNPreeditAttributes, listp,
		    XNStatusAttributes, lists,
		    NULL );
	    XFree(listp); XFree(lists);
	}
    }
    ((GXWindow) w)->gic = gic;
}

static int GXDrawKeyState(int keysym) {
    char key_map_stat[32];
    Display *xdisplay = ((GXDisplay *)screen_display)->display;
    KeyCode code;

    XQueryKeymap(xdisplay, key_map_stat);

    code = XKeysymToKeycode(xdisplay, GDrawKeyToXK(keysym));
    if ( !code ) {
abort();
return 0;
    }
return ((key_map_stat[code >> 3] >> (code & 7)) & 1);
}

int _GXDraw_WindowOrParentsDying(GXWindow gw) {
    while ( gw!=NULL ) {
	if ( gw->is_dying )
return( true );
	if ( gw->is_toplevel )
return( false );
	gw = gw->parent;
    }
return( false );
}

static void GXDrawRequestExpose(GWindow gw, GRect *rect,int doclear) {
    GXWindow gxw = (GXWindow) gw;
    GXDisplay *display = (GXDisplay *) (gw->display);
    GRect temp;

    if ( !gw->is_visible || _GXDraw_WindowOrParentsDying(gxw) )
return;
    if ( rect==NULL ) {
	temp.x = temp.y = 0;
	temp.width = gxw->pos.width; temp.height = gxw->pos.height;
	rect = &temp;
    } else if ( rect->x<0 || rect->y<0 || rect->x+rect->width>gw->pos.width ||
	    rect->y+rect->height>gw->pos.height ) {
	temp = *rect;
	if ( temp.x < 0 ) { temp.width += temp.x; temp.x = 0; }
	if ( temp.y < 0 ) { temp.height += temp.y; temp.y = 0; }
	if ( temp.x+temp.width>gw->pos.width )
	    temp.width = gw->pos.width - temp.x;
	if ( temp.y+temp.height>gw->pos.height )
	    temp.height = gw->pos.height - temp.y;
	if ( temp.height<=0 || temp.width <= 0 )
return;
	rect = &temp;
    }
    /* Don't simply XClearArea with exposures == True, flicker is noticeable */
    if ( doclear )
	XClearArea(display->display,gxw->w,rect->x,rect->y,rect->width,rect->height, false );
    if ( gw->eh!=NULL ) {
	struct gevent event;
	memset(&event,0,sizeof(event));
	event.type = et_expose;
	event.u.expose.rect = *rect;
	event.w = gw;
	event.native_window = gw->native_window;
	(gw->eh)(gw,&event);
    }
}

static void GTimerSetNext(GTimer *timer,int32 time_from_now) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    timer->time_sec  = tv.tv_sec +time_from_now/1000;
    timer->time_usec = tv.tv_usec+(time_from_now%1000)*1000;
    if ( timer->time_usec>=1000000 ) {
	++timer->time_sec;
	timer->time_usec-=1000000;
    }
}

static void GTimerInsertOrdered(GXDisplay *gdisp,GTimer *timer) {
    GTimer *prev, *test;

    if ( gdisp->timers==NULL ) {
	gdisp->timers = timer;
	timer->next = NULL;
    } else if ( gdisp->timers->time_sec>timer->time_sec ||
	    ( gdisp->timers->time_sec==timer->time_sec && gdisp->timers->time_usec>timer->time_usec )) {
	timer->next = gdisp->timers;
	gdisp->timers = timer;
    } else {
	prev = gdisp->timers;
	for ( test = prev->next; test!=NULL; prev=test, test=test->next )
	    if ( test->time_sec>timer->time_sec ||
		    ( test->time_sec==timer->time_sec && test->time_usec>timer->time_usec ))
	break;
	timer->next = test;
	prev->next = timer;
    }
}

static int GTimerRemove(GXDisplay *gdisp,GTimer *timer) {
    GTimer *prev, *test;

    if ( gdisp->timers==timer )
	gdisp->timers = timer->next;
    else {
	prev = gdisp->timers;
	if ( prev==NULL )
return( false );
	for ( test = prev->next; test!=NULL && test!=timer; prev=test, test=test->next );
	if ( test==NULL )		/* Wasn't in the list, oh well */
return(false);
	prev->next = timer->next;
    }
return( true );
}

static void GTimerRemoveWindowTimers(GXWindow gw) {
    GTimer *prev, *test, *next;
    GXDisplay *gdisp = gw->display;

    while ( gdisp->timers && gdisp->timers->owner==(GWindow) gw )
	gdisp->timers = gdisp->timers->next;
    prev = gdisp->timers;
    if ( prev==NULL )
return;
    for ( test = prev->next; test!=NULL; ) {
	next = test->next;
	if ( test->owner==(GWindow) gw ) {
	    prev->next = next;
	    free(test);
	} else
	    prev = test;
	test = next;
    }
}

static int GTimerInList(GXDisplay *gdisp,GTimer *timer) {
    GTimer *test;

    for ( test=gdisp->timers; test!=NULL; test = test->next )
	if ( test==timer )
return( true );

return( false );
}

static void GTimerReinstall(GXDisplay *gdisp,GTimer *timer) {

    GTimerRemove(gdisp,timer);
    if ( timer->repeat_time!=0 ) {
	GTimerSetNext(timer,timer->repeat_time);
	GTimerInsertOrdered(gdisp,timer);
    } else
	free(timer);
}

static GTimer *GXDrawRequestTimer(GWindow w,int32 time_from_now,int32 frequency,
	void *userdata) {
    GTimer *timer = calloc(1,sizeof(GTimer));

    GTimerSetNext(timer,time_from_now);

    timer->owner = w;
    timer->repeat_time = frequency;
    timer->userdata = userdata;
    timer->active = false;
    GTimerInsertOrdered(((GXWindow) w)->display,timer);
return( timer );
}

static void GXDrawCancelTimer(GTimer *timer) {
    GXDisplay *gdisp = ((GXWindow) (timer->owner))->display;

    if ( GTimerRemove(gdisp,timer))
	free(timer);
}

static void GXDrawSyncThread(GDisplay *gd, void (*func)(void *), void *data) {
#ifdef HAVE_PTHREAD_H
    GXDisplay *gdisp = (GXDisplay *) gd;
    struct things_to_do *ttd;

    pthread_mutex_lock(&gdisp->xthread.sync_mutex);
    if ( gdisp->xthread.sync_sock==-1 ) {
	#if !defined(__MINGW32__)
	int sv[2];
	socketpair(PF_UNIX,SOCK_DGRAM,0,sv);
	gdisp->xthread.sync_sock = sv[0];
	gdisp->xthread.send_sock = sv[1];
	#endif
    }
    if ( func==NULL ) {
	/* what's the point in calling this routine with no function? */
	/*  it sets things up so that the event loop is prepared to be */
	/*  stopped by the new socket. (otherwise it doesn't stop till */
	/*  it gets its next event. ie. if the eventloop is entered with */
	/*  sync_sock==-1 it won't wait on it, but next time it's entered*/
	/*  it won't be -1. This just allows us to make that condition */
	/*  true a little earlier */
    } else {
	for ( ttd=gdisp->xthread.things_to_do; ttd!=NULL &&
		(ttd->func!=func || ttd->data!=data); ttd = ttd->next );
	if ( ttd==NULL ) {
	    ttd = malloc(sizeof(struct things_to_do));
	    if ( gdisp->xthread.things_to_do==NULL )
		send(gdisp->xthread.send_sock," ",1,0);
	    ttd->func = func;
	    ttd->data = data;
	    ttd->next = gdisp->xthread.things_to_do;
	    gdisp->xthread.things_to_do = ttd;
	}
    }
    pthread_mutex_unlock(&gdisp->xthread.sync_mutex);
#else
    (func)(data);
#endif
}

static int GXDrawProcessTimerEvent(GXDisplay *gdisp,GTimer *timer) {
    struct gevent gevent;
    GWindow o;
    int ret = false;

    if ( timer->active )
return( false );
    timer->active = true;
    for ( o = timer->owner; o!=NULL && !o->is_dying; o=o->parent );
    if ( timer->owner!=NULL && timer->owner->eh!=NULL && o==NULL ) {
	memset(&gevent,0,sizeof(gevent));
	gevent.type = et_timer;
	gevent.w = timer->owner;
	gevent.native_window = timer->owner->native_window;
	gevent.u.timer.timer = timer;
	gevent.u.timer.userdata = timer->userdata;
	(timer->owner->eh)(timer->owner,&gevent);
	    /* If this routine calls something that checks events then */
	    /*  without the active flag above we'd loop forever half-invoking*/
	    /*  this timer */
	ret = true;
    }
    if ( GTimerInList(gdisp,timer)) {		/* carefull, they might have cancelled it */
	timer->active = false;
	if ( timer->repeat_time==0 )
	    GXDrawCancelTimer(timer);
	else
	    GTimerReinstall(gdisp,timer);
	ret = true;
    }
return(ret);
}

static void GXDrawCheckPendingTimers(GXDisplay *gdisp) {
    struct timeval tv;
    GTimer *timer, *next;

    gettimeofday(&tv,NULL);
    for ( timer = gdisp->timers; timer!=NULL; timer=next ) {
	next = timer->next;
	if ( timer->time_sec>tv.tv_sec ||
		(timer->time_sec == tv.tv_sec && timer->time_usec>tv.tv_usec ))
    break;
	if ( GXDrawProcessTimerEvent(gdisp,timer))
    break;
    }
}

#ifdef HAVE_PTHREAD_H
static void GXDrawDoThings(GXDisplay *gdisp) {
    char buffer[10];
    /* we enter and leave with the mutex locked */

    while ( gdisp->xthread.things_to_do!=NULL ) {
	struct things_to_do *ttd, *next;
	recv(gdisp->xthread.sync_sock,buffer,sizeof(buffer),0);
	ttd = gdisp->xthread.things_to_do;
	gdisp->xthread.things_to_do = NULL;
	pthread_mutex_unlock(&gdisp->xthread.sync_mutex);
	/* Don't let the user do stuff with the mutex locked */
	while ( ttd!=NULL ) {
	    next = ttd->next;
	    (ttd->func)(ttd->data);
	    free(ttd);
	    ttd = next;
	}
	pthread_mutex_lock(&gdisp->xthread.sync_mutex);
    }
}
#endif

static void GXDrawWaitForEvent(GXDisplay *gdisp) {
    struct timeval tv;
    Display *display = gdisp->display;
    struct timeval offset, *timeout;
    fd_set read, write, except;
    int fd,ret;
    int idx = 0;

    for (;;) {
	gettimeofday(&tv,NULL);
	GXDrawCheckPendingTimers(gdisp);

#ifdef _WACOM_DRV_BROKEN
	_GXDraw_Wacom_TestEvents(gdisp);
#endif

	if ( XEventsQueued(display,QueuedAfterFlush))
return;
#ifdef HAVE_PTHREAD_H
	if ( gdisp->xthread.sync_sock!=-1 ) {
	    pthread_mutex_lock(&gdisp->xthread.sync_mutex);
	    if ( gdisp->xthread.things_to_do )
		GXDrawDoThings(gdisp);
	    pthread_mutex_unlock(&gdisp->xthread.sync_mutex);
	}
#endif
	if ( gdisp->timers==NULL )
	    timeout = NULL;
	else {
	    offset.tv_sec = gdisp->timers->time_sec - tv.tv_sec;
	    if (( offset.tv_usec= gdisp->timers->time_usec- tv.tv_usec)<0 ) {
		offset.tv_usec += 1000000;
		--offset.tv_sec;
	    }
	    if ( offset.tv_sec<0 || (offset.tv_sec==0 && offset.tv_usec==0))
    continue;
	    timeout = &offset;
	}
	fd = XConnectionNumber(display);
//    printf("gxdraw.... x connection number:%d\n", fd );
	FD_ZERO(&read); FD_ZERO(&write); FD_ZERO(&except);
	FD_SET(fd,&read);
	FD_SET(fd,&except);
	if ( gdisp->xthread.sync_sock!=-1 ) {
	    FD_SET(gdisp->xthread.sync_sock,&read);
	    if ( gdisp->xthread.sync_sock>fd )
		fd = gdisp->xthread.sync_sock;
	}
#ifdef _WACOM_DRV_BROKEN
	if ( gdisp->wacom_fd!=-1 ) {
	    FD_SET(gdisp->wacom_fd,&read);
	    if ( gdisp->wacom_fd>fd )
		fd = gdisp->wacom_fd;
	}
#endif

	for( idx = 0; idx < gdisp->fd_callbacks_last; ++idx )
	{
	    fd_callback_t* cb = &gdisp->fd_callbacks[ idx ];
	    FD_SET( cb->fd, &read );
	    fd = MAX( fd, cb->fd );
	}
	
	ret = select(fd+1,&read,&write,&except,timeout);

	for( idx = 0; idx < gdisp->fd_callbacks_last; ++idx )
	{
	    fd_callback_t* cb = &gdisp->fd_callbacks[ idx ];
	    if( FD_ISSET(cb->fd,&read))
		cb->callback( cb->fd, cb->udata );
	}
    }
}

static void GXDrawPointerUngrab(GDisplay *gdisp) {
    GXDisplay *gd = (GXDisplay *) gdisp;
    XUngrabPointer(gd->display,gd->last_event_time);
    gd->grab_window = NULL;
}

static void GXDrawPointerGrab(GWindow gw) {
    GXDisplay *gd = (GXDisplay *) (gw->display);
    GXWindow w = (GXWindow) gw;
    XGrabPointer(gd->display,w->w,false,
	    PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
	    GrabModeAsync, GrabModeAsync,None,None,gd->last_event_time);
    gd->grab_window = gw;
}


/* Any input within the current dlg is ok */
/* if input is restricted to that dlg then nothing else is permitted */
/* if input is redirected then return the redirection to the current dlg */
/* if input is within an inactive window, then return the current dlg */
/* if there are any previous redirections outstanding then test them too */
/*  (ie. if we are a dlg called from another dlg) */
/* Now it's possible that the return tries to redirect something to the */
/*  current inactive window, and if so we should redirect that to the */
/*  current dlg. Indeed this is likely in a dlg produced by a dlg */
static GWindow InputRedirection(struct inputRedirect *input,GWindow gw) {
    GWindow ret;

    if ( input==NULL || input->cur_dlg->is_dying )
return( NULL );
    if ( gw->is_toplevel && ((GXWindow) gw)->not_restricted )
return( NULL );			/* Popup windows (menus, pulldown lists) count as part of their effective parents */
    if ( GDrawWindowIsAncestor(input->cur_dlg,gw))
return( NULL );
    if ( input->it == it_restricted || input->it == it_redirected ) {
	if ( input->it == it_redirected )
return( input->cur_dlg );
	else
return( (GWindow) -1 );
    }
    if ( GDrawWindowIsAncestor(input->inactive,gw))
return( input->cur_dlg );
    ret = NULL;
    if ( input->prev!=NULL )
	ret = InputRedirection(input->prev,gw);
    if ( ret==NULL || ret==(GWindow) (-1) )
return( ret );
    if ( GDrawWindowIsAncestor(input->inactive,ret))
return( input->cur_dlg );

return( ret );
}

static void dispatchEvent(GXDisplay *gdisp, XEvent *event) {
    struct gevent gevent;
    GWindow gw=NULL, redirect;
    void *ret;
    char charbuf[80], *pt;
    Status status;
    KeySym keysym; int len;
    GPoint p;
    int expecting_core = gdisp->expecting_core_event;
    XEvent subevent;

    if ( XFilterEvent(event,None))	/* Make sure this happens before anything else */
return;

    gdisp->expecting_core_event = false;

    if ( XFindContext(gdisp->display,event->xany.window,gdisp->mycontext,(void *) &ret)==0 )
	gw = (GWindow) ret;
    if ( gw==NULL || (_GXDraw_WindowOrParentsDying((GXWindow) gw) && event->type!=DestroyNotify ))
return;
    memset(&gevent,0,sizeof(gevent));
    gevent.w = gw;
    gevent.native_window = (void *) event->xany.window;
    gevent.type = -1;
    if ( event->type==KeyPress || event->type==ButtonPress || event->type == ButtonRelease ) {
	if ( ((GXWindow ) gw)->transient_owner!=0 && ((GXWindow ) gw)->isverytransient )
	    gdisp->last_nontransient_window = ((GXWindow ) gw)->transient_owner;
	else
	    gdisp->last_nontransient_window = ((GXWindow ) gw)->w;
    }
    switch(event->type) {
      case KeyPress: case KeyRelease:
	gdisp->last_event_time = event->xkey.time;
	gevent.type = event->type==KeyPress?et_char:et_charup;
	gevent.u.chr.time = event->xkey.time;
	gevent.u.chr.state = event->xkey.state;
	gevent.u.chr.autorepeat = 0;
//	printf("event->xkey.state:%d\n", event->xkey.state );
/*#ifdef __Mac*/
	/* On mac os x, map the command key to the control key. So Comand-Q=>^Q=>Quit */
	/* No... don't. Let the user have access to the command key as distinct from control */
	/* I don't think it hurts to leave this enabled... */
/*	if ( (event->xkey.state&ksm_cmdmacosx) && gdisp->macosx_cmd ) gevent.u.chr.state |= ksm_control; */
/* Under 10.4 the option key generates the meta mask already */
/* Under 10.6 it does not (change may have happened in 10.5.8) */
	if ( (event->xkey.state&ksm_option) && gdisp->macosx_cmd ) gevent.u.chr.state |= ksm_meta;
/*#endif*/
	gevent.u.chr.x = event->xkey.x;
	gevent.u.chr.y = event->xkey.y;
	if ((redirect = InputRedirection(gdisp->input,gw))== (GWindow)(-1) ) {
	    len = XLookupString((XKeyEvent *) event,charbuf,sizeof(charbuf),&keysym,&gdisp->buildingkeys);
	    if ( event->type==KeyPress && len!=0 )
		GXDrawBeep((GDisplay *) gdisp);
return;
	} else if ( redirect!=NULL ) {
	    GPoint pt;
	    gevent.w = redirect;
	    pt.x = event->xkey.x; pt.y = event->xkey.y;
	    GXDrawTranslateCoordinates(gw,redirect,&pt);
	    gevent.u.chr.x = pt.x;
	    gevent.u.chr.y = pt.y;
	    gw = redirect;
	}
	if ( gevent.type==et_char ) {
	    /* The state may be modified in the gevent where a mac command key*/
	    /*  entry gets converted to control, etc. */
	    if ( ((GXWindow) gw)->gic==NULL ) {
		len = XLookupString((XKeyEvent *) event,charbuf,sizeof(charbuf),&keysym,&gdisp->buildingkeys);
		charbuf[len] = '\0';
		gevent.u.chr.keysym = keysym;
		def2u_strncpy(gevent.u.chr.chars,charbuf,
			sizeof(gevent.u.chr.chars)/sizeof(gevent.u.chr.chars[0]));
	    } else {
#ifdef X_HAVE_UTF8_STRING
/* I think there's a bug in SCIM. If I leave the meta(alt/option) modifier */
/*  bit set, then scim returns no keysym and no characters. On the other hand,*/
/*  if I don't leave that bit set, then the default input method on the mac */
/*  will not do the Option key transformations properly. What I pass should */
/*  be IM independent. So I don't think I should have to do the next line */
		event->xkey.state &= ~Mod2Mask;
/* But I do */
		len = Xutf8LookupString(((GXWindow) gw)->gic->ic,(XKeyPressedEvent*)event,
				charbuf, sizeof(charbuf), &keysym, &status);
		pt = charbuf;
		if ( status==XBufferOverflow ) {
		    pt = malloc(len+1);
		    len = Xutf8LookupString(((GXWindow) gw)->gic->ic,(XKeyPressedEvent*)&event,
				    pt, len, &keysym, &status);
		}
		if ( status!=XLookupChars && status!=XLookupBoth )
		    len = 0;
		if ( status!=XLookupKeySym && status!=XLookupBoth )
		    keysym = 0;
		pt[len] = '\0';
		gevent.u.chr.keysym = keysym;
		utf82u_strncpy(gevent.u.chr.chars,pt,
			sizeof(gevent.u.chr.chars)/sizeof(gevent.u.chr.chars[0]));
		if ( pt!=charbuf )
		    free(pt);
#else
		gevent.u.chr.keysym = keysym = 0;
		gevent.u.chr.chars[0] = 0;
#endif
	    }
	    /* Convert X11 keysym values to unicode */
	    if ( keysym>=XKeysym_Mask )
		keysym -= XKeysym_Mask;
	    else if ( keysym<=XKEYSYM_TOP && keysym>=0 )
		keysym = gdraw_xkeysym_2_unicode[keysym];
	    gevent.u.chr.keysym = keysym;
	    if ( keysym==gdisp->mykey_keysym &&
		    (event->xkey.state&(ControlMask|Mod1Mask))==gdisp->mykey_mask ) {
		gdisp->mykeybuild = !gdisp->mykeybuild;
		gdisp->mykey_state = 0;
		gevent.u.chr.chars[0] = '\0';
		gevent.u.chr.keysym = '\0';
		if ( !gdisp->mykeybuild && _GDraw_BuildCharHook!=NULL )
		    (_GDraw_BuildCharHook)((GDisplay *) gdisp);
	    } else if ( gdisp->mykeybuild )
		_GDraw_ComposeChars((GDisplay *) gdisp,&gevent);
	} else {
	    /* XLookupKeysym doesn't do shifts for us (or I don't know how to use the index arg to make it) */
	    len = XLookupString((XKeyEvent *) event,charbuf,sizeof(charbuf),&keysym,&gdisp->buildingkeys);
	    gevent.u.chr.keysym = keysym;
	    gevent.u.chr.chars[0] = '\0';
	}

	/*
	 * If we are a charup, but the very next XEvent is a chardown
	 * on the same key, then we are just an autorepeat XEvent which
	 * other code might like to ignore
	 */
	if ( gevent.type==et_charup && XEventsQueued(gdisp->display, QueuedAfterReading)) {
	    XEvent nev;
	    XPeekEvent(gdisp->display, &nev);
	    if (nev.type == KeyPress && nev.xkey.time == event->xkey.time &&
		nev.xkey.keycode == event->xkey.keycode)
	    {
		gevent.u.chr.autorepeat = 1;
	    }
	}
      break;
      case ButtonPress: case ButtonRelease: case MotionNotify:
	if ( expecting_core && gdisp->last_event_time==event->xbutton.time )
      break; /* core event is a duplicate of device event */
	     /*  (only it's not quite a duplicate, often it's a few pixels */
	     /*  off from the device location */
	if ( event->type==ButtonPress )
	    gdisp->grab_window = gw;
	else if ( gdisp->grab_window!=NULL ) {
	    if ( gw!=gdisp->grab_window ) {
		Window wjunk;
		gevent.w = gw = gdisp->grab_window;
		XTranslateCoordinates(gdisp->display,
			event->xbutton.window,((GXWindow) gw)->w,
			event->xbutton.x, event->xbutton.y,
			&event->xbutton.x, &event->xbutton.y,
			&wjunk);
	    }
	    if ( event->type==ButtonRelease )
		gdisp->grab_window = NULL;
	}

	gdisp->last_event_time = event->xbutton.time;
	gevent.u.mouse.time = event->xbutton.time;
	if ( event->type==MotionNotify && gdisp->grab_window==NULL )
	    /* Allow simple motion events to go through */;
	else if ((redirect = InputRedirection(gdisp->input,gw))!=NULL ) {
	    if ( event->type==ButtonPress )
		GXDrawBeep((GDisplay *) gdisp);
return;
	}
	gevent.u.mouse.state = event->xbutton.state;
	gevent.u.mouse.x = event->xbutton.x;
	gevent.u.mouse.y = event->xbutton.y;
	gevent.u.mouse.button = event->xbutton.button;
	gevent.u.mouse.device = NULL;
	gevent.u.mouse.pressure = gevent.u.mouse.xtilt = gevent.u.mouse.ytilt = gevent.u.mouse.separation = 0;
	if ( (event->xbutton.state&0x40) && gdisp->twobmouse_win )
	    gevent.u.mouse.button = 2;
	if ( event->type == MotionNotify ) {
#if defined (__MINGW32__) || __CygWin
        //For some reason, a mouse move event is triggered even if it hasn't moved.
        if(gdisp->mousemove_last_x == event->xbutton.x &&
           gdisp->mousemove_last_y == event->xbutton.y) {
            return;
        }
        gdisp->mousemove_last_x = event->xbutton.x;
        gdisp->mousemove_last_y = event->xbutton.y;
#endif
	    gevent.type = et_mousemove;
	    gevent.u.mouse.button = 0;
	    gevent.u.mouse.clicks = 0;
	} else if ( event->type == ButtonPress ) {
	    int diff, temp;
	    gevent.type = et_mousedown;
	    if (( diff = event->xbutton.x-gdisp->bs.release_x )<0 ) diff= -diff;
	    if (( temp = event->xbutton.y-gdisp->bs.release_y )<0 ) temp= -temp;
	    if ( diff+temp<gdisp->bs.double_wiggle &&
		    event->xbutton.window == gdisp->bs.release_w &&
		    event->xbutton.button == gdisp->bs.release_button &&
		    event->xbutton.time-gdisp->bs.release_time < gdisp->bs.double_time &&
		    event->xbutton.time >= gdisp->bs.release_time )	/* Time can wrap */
		++ gdisp->bs.cur_click;
	    else
		gdisp->bs.cur_click = 1;
	    gevent.u.mouse.clicks = gdisp->bs.cur_click;
	} else {
	    gevent.type = et_mouseup;
	    gdisp->bs.release_time = event->xbutton.time;
	    gdisp->bs.release_w = event->xbutton.window;
	    gdisp->bs.release_x = event->xbutton.x;
	    gdisp->bs.release_y = event->xbutton.y;
	    gdisp->bs.release_button = event->xbutton.button;
	    gevent.u.mouse.clicks = gdisp->bs.cur_click;
	}
      break;
      case Expose: case GraphicsExpose:
	gevent.type = et_expose;
	gevent.u.expose.rect.x = event->xexpose.x;
	gevent.u.expose.rect.y = event->xexpose.y;
	gevent.u.expose.rect.width = event->xexpose.width;
	gevent.u.expose.rect.height = event->xexpose.height;
	/* Slurp any pending exposes and merge into one big rectangle */
	while ( XCheckTypedWindowEvent(gdisp->display,event->xany.window,event->type,
		&subevent)) {
	    if ( subevent.xexpose.x+subevent.xexpose.width > gevent.u.expose.rect.x+gevent.u.expose.rect.width )
		gevent.u.expose.rect.width = subevent.xexpose.x+subevent.xexpose.width - gevent.u.expose.rect.x;
	    if ( subevent.xexpose.x < gevent.u.expose.rect.x ) {
		gevent.u.expose.rect.width += gevent.u.expose.rect.x - subevent.xexpose.x;
		gevent.u.expose.rect.x = subevent.xexpose.x;
	    }
	    if ( subevent.xexpose.y+subevent.xexpose.height > gevent.u.expose.rect.y+gevent.u.expose.rect.height )
		gevent.u.expose.rect.height = subevent.xexpose.y+subevent.xexpose.height - gevent.u.expose.rect.y;
	    if ( subevent.xexpose.y < gevent.u.expose.rect.y ) {
		gevent.u.expose.rect.height += gevent.u.expose.rect.y - subevent.xexpose.y;
		gevent.u.expose.rect.y = subevent.xexpose.y;
	    }
	}
#ifndef _NO_LIBCAIRO
	if ( ((GXWindow) gw)->usecairo )		/* X11 does this automatically. but cairo won't get the event */
	    GXDrawClear(gw,&gevent.u.expose.rect);
#endif
      break;
      case VisibilityNotify:
	gevent.type = et_visibility;
	gevent.u.visibility.state = event->xvisibility.state;
      break;
      case FocusIn: case FocusOut:	/* Should only get this on top level */
	gevent.type = et_focus;
	gevent.u.focus.gained_focus = event->type==FocusIn;
	gevent.u.focus.mnemonic_focus = false;
      break;
      case EnterNotify: case LeaveNotify: /* Should only get this on top level */
	if ( event->xcrossing.detail == NotifyInferior )
      break;
	if ( gdisp->focusfollowsmouse && gw!=NULL && gw->eh!=NULL ) {
	    gevent.type = et_focus;
	    gevent.u.focus.gained_focus = event->type==EnterNotify;
	    gevent.u.focus.mnemonic_focus = false;
	    (gw->eh)((GWindow) gw, &gevent);
	}
	gevent.type = et_crossing;
	gevent.u.crossing.x = event->xcrossing.x;
	gevent.u.crossing.y = event->xcrossing.y;
	gevent.u.crossing.state = event->xcrossing.state;
	gevent.u.crossing.entered = event->type==EnterNotify;
	gevent.u.crossing.device = NULL;
	gevent.u.crossing.time = event->xcrossing.time;
      break;
      case ConfigureNotify:
	/* Eat up multiple resize notifications in case the window manager */
	/*  does animated resizes */
	while ( XCheckTypedWindowEvent(event->xconfigure.display,
		event->xconfigure.window,ConfigureNotify,event));
	gevent.type = et_resize;
	gevent.u.resize.size.x = event->xconfigure.x;
	gevent.u.resize.size.y = event->xconfigure.y;
	gevent.u.resize.size.width = event->xconfigure.width;
	gevent.u.resize.size.height = event->xconfigure.height;
	if ( gw->is_toplevel ) {
	    p.x = 0; p.y = 0;
	    GXDrawTranslateCoordinates(gw,(GWindow) (gdisp->groot),&p);
	    gevent.u.resize.size.x = p.x;
	    gevent.u.resize.size.y = p.y;
	}
	gevent.u.resize.dx = gevent.u.resize.size.x-gw->pos.x;
	gevent.u.resize.dy = gevent.u.resize.size.y-gw->pos.y;
	gevent.u.resize.dwidth = gevent.u.resize.size.width-gw->pos.width;
	gevent.u.resize.dheight = gevent.u.resize.size.height-gw->pos.height;
	gevent.u.resize.moved = gevent.u.resize.sized = false;
	if ( gevent.u.resize.dx!=0 || gevent.u.resize.dy!=0 )
	    gevent.u.resize.moved = true;
	if ( gevent.u.resize.dwidth!=0 || gevent.u.resize.dheight!=0 ) {
	    gevent.u.resize.sized = true;
#ifndef _NO_LIBCAIRO
	    if ( ((GXWindow) gw)->usecairo )
		_GXCDraw_ResizeWindow((GXWindow) gw, &gevent.u.resize.size);
#endif
	}
	gw->pos = gevent.u.resize.size;
	if ( !gdisp->top_offsets_set && ((GXWindow) gw)->was_positioned &&
		gw->is_toplevel && !((GXWindow) gw)->is_popup &&
		!((GXWindow) gw)->istransient ) {
	    /* I don't know why I need a fudge factor here, but I do */
	    gdisp->off_x = gevent.u.resize.dx-2;
	    gdisp->off_y = gevent.u.resize.dy-1;
	    gdisp->top_offsets_set = true;
	}
      break;
      case CreateNotify:
	/* actually CreateNotify events only go to the window parent if */
	/*  substructureNotify is set. We aren't a window manager, so we */
	/*  shouldn't get any. Sigh. I simulate them instead */
	gevent.type = et_create;
      break;
      case MapNotify:
	gevent.type = et_map;
	gevent.u.map.is_visible = true;
	gw->is_visible = true;
      break;
      case UnmapNotify:
	gevent.type = et_map;
	gevent.u.map.is_visible = false;
	gw->is_visible = false;
      break;
      case DestroyNotify:
	gevent.type = et_destroy;
      break;
      case ClientMessage:
	if ((redirect = InputRedirection(gdisp->input,gw))!=NULL ) {
	    GXDrawBeep((GDisplay *) gdisp);
return;
	}
	if ( event->xclient.message_type == gdisp->atoms.wm_protocols &&
		event->xclient.data.l[0] == gdisp->atoms.wm_del_window )
	    gevent.type = et_close;
	else if ( event->xclient.message_type == gdisp->atoms.drag_and_drop ) {
	    gevent.type = event->xclient.data.l[0];
	    gevent.u.drag_drop.x = event->xclient.data.l[1];
	    gevent.u.drag_drop.y = event->xclient.data.l[2];
	}
      break;
      case SelectionClear: {
	int i;
	gdisp->last_event_time = event->xselectionclear.time;
	gevent.type = et_selclear;
	gevent.u.selclear.sel = sn_primary;
	for ( i=0; i<sn_max; ++i ) {
	    if ( event->xselectionclear.selection==gdisp->selinfo[i].sel_atom ) {
		gevent.u.selclear.sel = i;
	break;
	    }
	}
	GXDrawClearSelData(gdisp,gevent.u.selclear.sel);
      } break;
      case SelectionRequest:
	gdisp->last_event_time = event->xselectionrequest.time;
	GXDrawTransmitSelection(gdisp,event);
      break;
      case SelectionNotify:		/* !!!!! paste */
	/*gdisp->last_event_time = event->xselection.time;*/ /* it's the request's time not the current? */
      break;
      case PropertyNotify:
	gdisp->last_event_time = event->xproperty.time;
      break;
      case ReparentNotify:
	if ( event->xreparent.parent==gdisp->root ) {
	    gw->parent = (GWindow) (gdisp->groot);
	    gw->is_toplevel = true;
	} else if ( XFindContext(gdisp->display,event->xreparent.parent,gdisp->mycontext,(void *) &ret)==0 ) {
	    GWindow gparent = (GWindow) ret;
	    gw->parent = gparent;
	    gw->is_toplevel = (GXWindow) gparent==gdisp->groot;
	}
      break;
      case MappingNotify:
	XRefreshKeyboardMapping((XMappingEvent *) event);
      break;
      default:
#ifndef _NO_XKB
	if ( event->type==gdisp->xkb.event ) {
	    switch ( ((XkbAnyEvent *) event)->xkb_type ) {
	      case XkbNewKeyboardNotify:
		/* I don't think I need to do anything here. But I think I */
		/*  need to get the event since otherwise xkb restricts the */
		/*  keycodes it will send me */
	      break;
	      case XkbMapNotify:
		XkbRefreshKeyboardMapping((XkbMapNotifyEvent *) event);
	      break;
	    }
      break;
	}
#endif
#ifndef _NO_XINPUT
        if ( event->type>=LASTEvent ) {	/* An XInput event */
	    int i,j;
	    static int types[5] = { et_mousemove, et_mousedown, et_mouseup, et_char, et_charup };
	    for ( i=0 ; i<gdisp->n_inputdevices; ++i ) {
		if ( ((XDeviceButtonEvent *) event)->deviceid==gdisp->inputdevices[i].devid ) { 
		    for ( j=0; j<5; ++j )
			if ( event->type==gdisp->inputdevices[i].event_types[j] ) {
			    gevent.type = types[j];
	    goto found;
			}
		}
	    }
	    found: ;
	    if ( gevent.type != et_noevent ) {
		gdisp->last_event_time = ((XDeviceButtonEvent *) event)->time;
		gevent.u.mouse.time = ((XDeviceButtonEvent *) event)->time;
		gevent.u.mouse.device = gdisp->inputdevices[i].name;	/* Same place in key and mouse events */
		if ( j>3 ) {	/* Key event */
		    gevent.u.chr.state = ((XDeviceKeyEvent *) event)->device_state;
		    gevent.u.chr.x = ((XDeviceKeyEvent *) event)->x;
		    gevent.u.chr.y = ((XDeviceKeyEvent *) event)->y;
		    gevent.u.chr.keysym = ((XDeviceKeyEvent *) event)->keycode;
		    gevent.u.chr.chars[0] = 0;
		    if ( ((XDeviceKeyEvent *) event)->first_axis!=0 )
			gevent.type = et_noevent;	/* Repeat of previous event to add more axes */
		} else {
		    /* Pass the buttons from the device, the key modifiers from the normal state */
		    gevent.u.mouse.state =
			    ( ((XDeviceButtonEvent *) event)->device_state & 0xffffff00) |
			    ( ((XDeviceButtonEvent *) event)->state	   & 0x000000ff);
		    gevent.u.mouse.x = ((XDeviceButtonEvent *) event)->x;
		    gevent.u.mouse.y = ((XDeviceButtonEvent *) event)->y;
		    gdisp->expecting_core_event = true;
		    if ( j!=0 ) {
			gevent.u.mouse.button = ((XDeviceButtonEvent *) event)->button;
			if ( ((XDeviceButtonEvent *) event)->first_axis!=0 )
			    gevent.type = et_noevent;	/* Repeat of previous event to add more axes */
			if ( ((XDeviceButtonEvent *) event)->axes_count==6 ) {
			    gevent.u.mouse.pressure = ((XDeviceButtonEvent *) event)->axis_data[2];
			    gevent.u.mouse.xtilt = ((XDeviceButtonEvent *) event)->axis_data[3];
			    gevent.u.mouse.ytilt = ((XDeviceButtonEvent *) event)->axis_data[4];
			} else
			    gevent.u.mouse.pressure = gevent.u.mouse.xtilt = gevent.u.mouse.ytilt = gevent.u.mouse.separation = 0;
		    } else {
			if ( ((XDeviceMotionEvent *) event)->first_axis!=0 )
			    gevent.type = et_noevent;	/* Repeat of previous event to add more axes */
			gevent.u.mouse.button = 0;
			if ( ((XDeviceMotionEvent *) event)->axes_count==6 ) {
			    gevent.u.mouse.pressure = ((XDeviceMotionEvent *) event)->axis_data[2];
			    gevent.u.mouse.xtilt = ((XDeviceMotionEvent *) event)->axis_data[3];
			    gevent.u.mouse.ytilt = ((XDeviceMotionEvent *) event)->axis_data[4];
			} else
			    gevent.u.mouse.pressure = gevent.u.mouse.xtilt = gevent.u.mouse.ytilt = gevent.u.mouse.separation = 0;
		    }
		}
	    }
	}
#endif
      break;
    }

    if ( gevent.type != et_noevent && gw!=NULL && gw->eh!=NULL )
	(gw->eh)((GWindow) gw, &gevent);
    if ( event->type==DestroyNotify && gw!=NULL )
	_GXDraw_CleanUpWindow( gw );
}

static void GXDrawForceUpdate(GWindow gw) {
    XEvent event;
    Window w=((GXWindow) gw)->w;
    Display *display = ((GXDisplay *) (gw->display))->display;
    /* Do NOT check for timer events here! we are only interested in Exposes */
    /* I assume that GraphicsExposes are also caught by ExposureMask? */

    while ( XCheckWindowEvent(display,w,ExposureMask,&event))
	dispatchEvent((GXDisplay *) (gw->display), &event);
}

/* any event is good here */
static Bool allevents(Display *display, XEvent *event, char *arg) {
return( true );
}

static Bool windowevents(Display *display, XEvent *event, char *arg) {
return( event->xany.window == (Window) arg );
}

static void GXDrawProcessOneEvent(GDisplay *gdisp) {
    XEvent event;
    Display *display = ((GXDisplay *) gdisp)->display;
    /* Handle one X event (actually we might also handle a bunch of timers too) */

    GXDrawWaitForEvent((GXDisplay *) gdisp);
    XNextEvent(display,&event);
    dispatchEvent((GXDisplay *) gdisp, &event);
}

struct mmarg { Window w; int state; int stop; };

static Bool mmpred(Display *d, XEvent *e, XPointer arg) {
    struct mmarg *mmarg = (struct mmarg *) arg;

    if ( mmarg->stop )
return( False );
    if ( e->type==MotionNotify ) {
	if ( e->xmotion.window==mmarg->w && e->xmotion.state == mmarg->state )
return( True );
	mmarg->stop = true;
    } else if ( e->type == ButtonPress || e->type==ButtonRelease )
	mmarg->stop = true;
return( False );
}

static void GXDrawSkipMouseMoveEvents(GWindow w, GEvent *last) {
    XEvent event;
    GXWindow gw = (GXWindow) w;
    struct mmarg arg;

    arg.w = gw->w; arg.state = last->u.mouse.state; arg.stop = false;
    while ( XCheckIfEvent(gw->display->display,&event,mmpred,(XPointer) &arg) ) {
	last->u.mouse.x = event.xmotion.x;
	last->u.mouse.y = event.xmotion.y;
    }
}

static void GXDrawProcessPendingEvents(GDisplay *gdisp) {
    XEvent event;
    Display *display = ((GXDisplay *) gdisp)->display;
    /* We don't wait for anything. Only stuff already in the queue */

    GXDrawCheckPendingTimers((GXDisplay *) gdisp);
    while ( XCheckIfEvent(display,&event,allevents,NULL))
	dispatchEvent((GXDisplay *) gdisp, &event);
}

static void GXDrawProcessWindowEvents(GWindow w) {
    XEvent event;
    GXWindow gw = (GXWindow) w;
    Display *display = gw->display->display;

    while ( XCheckIfEvent(display,&event,windowevents,(char *) (gw->w)))
	dispatchEvent(gw->display, &event);
}

static void GXDrawSync(GDisplay *gdisp) {
    XSync(((GXDisplay *) gdisp)->display,false);
}

void dispatchError(GDisplay *gdisp) {
    if ((gdisp->err_flag) && (gdisp->err_report)) {
      GDrawIErrorRun("%s",gdisp->err_report);
    }
    if (gdisp->err_report) {
      free(gdisp->err_report); gdisp->err_report = NULL;
    }
}

/* Munch events until we no longer have any top level windows. That essentially*/
/*  means no windows (even if they got reparented, we still think they are top)*/
/*  At that point try very hard to clear out the event queue. It is conceivable*/
/*  that doing so will create a new window. If no luck then return */
static void GXDrawEventLoop(GDisplay *gd) {
    XEvent event;
    GXDisplay *gdisp = (GXDisplay *) gd;
    Display *display = gdisp->display;

    do {
	while ( gdisp->top_window_count>0 ) {
	    GXDrawWaitForEvent(gdisp);
	    XNextEvent(display,&event);
	    dispatchEvent(gdisp, &event);
	    dispatchError(gd);
	}
	XSync(display,false);
	GXDrawProcessPendingEvents(gd);
	XSync(display,false);
    } while ( gdisp->top_window_count>0 || XEventsQueued(display,QueuedAlready)>0 );
}

static void GXDrawPostEvent(GEvent *e) {
    /* Doesn't check event masks, not sure if that's desirable or not. It's easy though */
    GXWindow gw = (GXWindow) (e->w);
    e->native_window = ((GWindow) gw)->native_window;
    (gw->eh)((GWindow) gw, e);
}

/* Drag and drop works thusly:
    the user drags a selection somewhere
     as this happens we send out drag events to each window the cursor moves
      over telling the window where the cursor is (to allow the window to do
      feedback like showing a text cursor or something)
     when we exit a window we send one last event (a dragout event) to let it
      know it should clear its cursor
    when the user drops the selection
     the client grabs the DRAG_AND_DROP selection
     fills it up with whatever types are appropriate
     sends the window a drop event
     the window looks at that event and extracts a (local to it) position of
      the drop
     it queries the selection to get the data
     it performs the drop operation
*/
    
static void gxdrawSendDragOut(GXDisplay *gdisp) {

    if ( gdisp->last_dd.gw!=NULL ) {
	GEvent e;
	memset(&e,0,sizeof(e));
	e.type = et_dragout;
	e.u.drag_drop.x = gdisp->last_dd.rx;
	e.u.drag_drop.y = gdisp->last_dd.ry;
	e.native_window = NULL;
	if ( gdisp->last_dd.gw->eh!=NULL )
	    (gdisp->last_dd.gw->eh)(gdisp->last_dd.gw,&e);
    } else {
	XEvent xe;
	xe.type = ClientMessage;
	xe.xclient.display = gdisp->display;
	xe.xclient.window = gdisp->last_dd.w;
	xe.xclient.message_type = gdisp->atoms.drag_and_drop;
	xe.xclient.format = 32;
	xe.xclient.data.l[0] = et_dragout;
	xe.xclient.data.l[1] = gdisp->last_dd.rx;
	xe.xclient.data.l[2] = gdisp->last_dd.ry;
	XSendEvent(gdisp->display,gdisp->last_dd.w,False,0,&xe);
    }
    gdisp->last_dd.w = None;
    gdisp->last_dd.gw = NULL;
}

static void GXDrawPostDragEvent(GWindow w,GEvent *mouse,enum event_type et) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    GEvent e;
    Window child, curwin;
    int x,y;
    void *vd;
    GWindow destw = NULL;

    /* if the cursor hasn't moved much, don't bother to send a drag event */
    if (( x = mouse->u.mouse.x-gdisp->last_dd.x )<0 ) x = -x;
    if (( y = mouse->u.mouse.y-gdisp->last_dd.y )<0 ) y = -y;
    if ( x+y < 4 && et==et_drag )
return;

    curwin = _GXDrawGetPointerWindow(w);

    if ( gdisp->last_dd.w!=None && gdisp->last_dd.w!=curwin )
	gxdrawSendDragOut(gdisp);

    memset(&e,0,sizeof(e));

    /* Are we still within the original window? */
    if ( curwin == gw->w ) {
	e.type = et;
	x = e.u.drag_drop.x = mouse->u.mouse.x;
	y = e.u.drag_drop.y = mouse->u.mouse.y;
	(gw->eh)(w, &e);
    } else {
	XTranslateCoordinates(gdisp->display,gw->w,curwin,
		mouse->u.mouse.x,mouse->u.mouse.y,
		&x,&y,&child);

	e.type = et;
	e.u.drag_drop.x = x;
	e.u.drag_drop.y = y;
	e.native_window = (void *) curwin;

	if ( (curwin&0xfff00000)==(gw->w&0xfff00000) &&
		XFindContext(gdisp->display,curwin,gdisp->mycontext,(void *) &vd)==0 ) {
	    destw = (GWindow) vd;
	    /* is it one of our windows? If so use our own event mechanism */
	    if ( destw->eh!=NULL )
		(destw->eh)(destw,&e);
	} else if ( curwin!=gdisp->root ) {
	    XEvent xe;
	    xe.type = ClientMessage;
	    xe.xclient.display = gdisp->display;
	    xe.xclient.window = curwin;
	    xe.xclient.message_type = gdisp->atoms.drag_and_drop;
	    xe.xclient.format = 32;
	    xe.xclient.data.l[0] = et;
	    xe.xclient.data.l[1] = x;
	    xe.xclient.data.l[2] = y;
	    XSendEvent(gdisp->display,curwin,False,0,&xe);
	}
    }
    if ( et!=et_drop ) {
	gdisp->last_dd.w = curwin;
	gdisp->last_dd.gw = destw;
	gdisp->last_dd.x = mouse->u.mouse.x;
	gdisp->last_dd.y = mouse->u.mouse.y;
	gdisp->last_dd.rx = x;
	gdisp->last_dd.ry = y;
    } else {
	gdisp->last_dd.w = None;
	gdisp->last_dd.gw = NULL;
    }
}

static int devopen_failed;
static char *device_name;

static int devopenerror(Display *disp, XErrorEvent *err) {
    /* Some ubuntu releases seem to give everybody wacom devices by default */
    /*  in their xorg.conf file. However if the user has no wacom tablet then */
    /*  an attempt to use one of those devices will cause X to return a */
    /*  BadDevice error */
    /* Unfortunately there is no good way to test for BadDevice (at least */
    /*  not that I am aware of). It's an extension error, so its numeric value*/
    /*  varies from machine to machine. I could grab the error message as a */
    /*  string, but it could be localized, and might change too. */
    /* So I just assume that any error in the extension error range is likely */
    /*  to be BadDevice */

    if ( err->error_code>=128 ) {
	devopen_failed = true;
	fprintf( stderr, "X11 claims there exists a device called \"%s\", but an attempt to open it fails.\n  Rerun the program with the -dontopenxdevices argument.\n",
		device_name );
    } else {
	myerrorhandler(disp,err);
    }
return( 1 );
}

static int GXDrawRequestDeviceEvents(GWindow w,int devcnt,struct gdeveventmask *de) {
#ifndef _NO_XINPUT
    GXDisplay *gdisp = (GXDisplay *) (w->display);
    int i,j,k,cnt,foo, availdevcnt;
    XEventClass *classes;
    GResStruct res[2];

    if ( !gdisp->devicesinit ) {
	int ndevs=0;
	XDeviceInfo *devs;
	int dontopentemp = 0;

	memset(res,0,sizeof(res));
	i=0;
	res[i].resname = "DontOpenXDevices"; res[i].type = rt_bool; res[i].val = &dontopentemp; ++i;
	res[i].resname = NULL;
	GResourceFind(res,NULL);
	if ( dontopentemp ) {
	    gdisp->devicesinit = true;
return( 0 );
	}
	
	devs = XListInputDevices(gdisp->display,&ndevs);
	gdisp->devicesinit = true;
	if ( ndevs==0 )
return( 0 );
	gdisp->inputdevices = calloc(ndevs+1,sizeof(struct inputdevices));
	for ( i=0; i<ndevs; ++i ) {
	    gdisp->inputdevices[i].name = copy(devs[i].name);
	    gdisp->inputdevices[i].devid = devs[i].id;
	}
	gdisp->n_inputdevices = ndevs;
	XFreeDeviceList(devs);
    }
    classes = NULL;
    for ( k=0; k<2; ++k ) {
	cnt=availdevcnt=0;
	for ( j=0; de[j].device_name!=NULL; ++j ) {
	    for ( i=0; i<gdisp->n_inputdevices; ++i )
		if ( strcmp(de[j].device_name,gdisp->inputdevices[i].name)==0 )
	    break;
	    if ( i<gdisp->n_inputdevices ) {
		if ( gdisp->inputdevices[i].dev==NULL ) {
		    XSync(gdisp->display,false);
		    GDrawProcessPendingEvents((GDisplay *) gdisp);
		    XSetErrorHandler(/*gdisp->display,*/devopenerror);
		    devopen_failed = false;
		    device_name = gdisp->inputdevices[i].name;
		    gdisp->inputdevices[i].dev = XOpenDevice(gdisp->display,gdisp->inputdevices[i].devid);
		    XSync(gdisp->display,false);
		    GDrawProcessPendingEvents((GDisplay *) gdisp);
		    XSetErrorHandler(/*gdisp->display,*/myerrorhandler);
		    if ( devopen_failed )
			gdisp->inputdevices[i].dev = NULL;
		}
		if ( gdisp->inputdevices[i].dev!=NULL ) {
		    ++availdevcnt;
		    if ( de[j].event_mask & (1<<et_mousemove) ) {
			if ( classes!=NULL )
			    DeviceMotionNotify(gdisp->inputdevices[i].dev,gdisp->inputdevices[i].event_types[0],classes[cnt]);
			++cnt;
		    }
		    if ( de[j].event_mask & (1<<et_mousedown) ) {
			if ( classes!=NULL )
			    DeviceButtonPress(gdisp->inputdevices[i].dev,gdisp->inputdevices[i].event_types[1],classes[cnt]);
			++cnt;
		    }
		    if ( de[j].event_mask & (1<<et_mouseup) ) {
			if ( classes!=NULL )
			    DeviceButtonRelease(gdisp->inputdevices[i].dev,gdisp->inputdevices[i].event_types[2],classes[cnt]);
			++cnt;
		    }
		    if ( (de[j].event_mask & (1<<et_mousedown)) && (de[j].event_mask & (1<<et_mouseup)) ) {
			if ( classes!=NULL )
			    DeviceButtonPressGrab(gdisp->inputdevices[i].dev,foo,classes[cnt]);
			++cnt;
		    }
		    if ( de[j].event_mask & (1<<et_char) ) {
			if ( classes!=NULL )
			    DeviceKeyPress(gdisp->inputdevices[i].dev,foo,classes[cnt]);
			++cnt;
		    }
		    if ( de[j].event_mask & (1<<et_charup) ) {
			if ( classes!=NULL )
			    DeviceKeyRelease(gdisp->inputdevices[i].dev,foo,classes[cnt]);
			++cnt;
		    }
		}
	    }
	}
	if ( cnt==0 )
return(0);
	if ( k==0 )
	    classes = malloc(cnt*sizeof(XEventClass));
    }
    XSelectExtensionEvent(gdisp->display,((GXWindow) w)->w,classes,cnt);
    free(classes);
return( availdevcnt );
#else
return( 0 );
#endif
}

static Bool exposeornotify(Display *d,XEvent *e,XPointer arg) {
    if ( e->type == Expose || e->type == GraphicsExpose ||
	    e->type == CreateNotify || e->type == MapNotify ||
	    e->type == DestroyNotify || e->type == UnmapNotify ||
	    (e->type == SelectionNotify && e->xselection.requestor==(Window) arg) ||
	    e->type == SelectionClear || e->type == SelectionRequest )
return( true );

return( false );
}

static int GXDrawWaitForNotifyEvent(GXDisplay *gdisp,XEvent *event, Window w) {
    struct timeval tv, giveup, timer, *which;
    Display *display = gdisp->display;
    struct timeval offset;
    fd_set read, write, except;
    int fd,ret;
    int idx = 0;
    
    gettimeofday(&giveup,NULL);
    giveup.tv_sec += gdisp->SelNotifyTimeout;
    
    for (;;) {
	gettimeofday(&tv,NULL);
	GXDrawCheckPendingTimers(gdisp);
#ifdef _WACOM_DRV_BROKEN
	_GXDraw_Wacom_TestEvents(gdisp);
#endif
#ifdef HAVE_PTHREAD_H
	if ( gdisp->xthread.sync_sock!=-1 ) {
	    pthread_mutex_lock(&gdisp->xthread.sync_mutex);
	    if ( gdisp->xthread.things_to_do )
		GXDrawDoThings(gdisp);
	    pthread_mutex_unlock(&gdisp->xthread.sync_mutex);
	}
#endif

	while ( XCheckIfEvent(display,event,exposeornotify,(XPointer) w)) {
	    if ( event->type == SelectionNotify )
return( true );
	    dispatchEvent(gdisp, event);
	}
	/* Which happens sooner? The timeout for waiting for a paste response,*/
	/*  or one of the timers? */
	if ( gdisp->timers==NULL )
	    which = &giveup;
	else if ( giveup.tv_sec<gdisp->timers->time_sec ||
		( giveup.tv_sec==gdisp->timers->time_sec && giveup.tv_usec<gdisp->timers->time_usec ))
	    which = &giveup;
	else {
	    timer.tv_usec = gdisp->timers->time_usec; timer.tv_sec = gdisp->timers->time_sec;
	    which = &timer;
	}
	offset.tv_sec = which->tv_sec - tv.tv_sec;
	if (( offset.tv_usec= which->tv_usec- tv.tv_usec)<0 ) {
	    offset.tv_usec += 1000000;
	    --offset.tv_sec;
	}
	if ( offset.tv_sec<0 || (offset.tv_sec==0 && offset.tv_usec==0)) {
	    if ( which == &giveup )
return( false );
	} else
    continue;	/* Handle timer */

	fd = XConnectionNumber(display);
	FD_ZERO(&read); FD_ZERO(&write); FD_ZERO(&except);
	FD_SET(fd,&read);
	FD_SET(fd,&except);
	if ( gdisp->xthread.sync_sock!=-1 ) {
	    FD_SET(gdisp->xthread.sync_sock,&read);
	    if ( gdisp->xthread.sync_sock>fd )
		fd = gdisp->xthread.sync_sock;
	}
#ifdef _WACOM_DRV_BROKEN
	if ( gdisp->wacom_fd!=-1 ) {
	    FD_SET(gdisp->wacom_fd,&read);
	    if ( gdisp->wacom_fd>fd )
		fd = gdisp->wacom_fd;
	}
#endif

	for( idx = 0; idx < gdisp->fd_callbacks_last; ++idx )
	{
	    fd_callback_t* cb = &gdisp->fd_callbacks[ idx ];
	    FD_SET( cb->fd, &read );
	    fd = MAX( fd, cb->fd );
	}
	
	ret = select(fd+1,&read,&write,&except,&offset);

	for( idx = 0; idx < gdisp->fd_callbacks_last; ++idx )
	{
	    fd_callback_t* cb = &gdisp->fd_callbacks[ idx ];
	    if( FD_ISSET(cb->fd,&read))
		cb->callback( cb->fd, cb->udata );
	}
    }
}

static Atom GXDrawGetAtom(GXDisplay *gd, char *name) {
    int i;

    if ( gd->atomdata==NULL ) {
	gd->atomdata = calloc(10,sizeof(struct atomdata));
	gd->amax = 10;
    }
    for ( i=0; i<gd->alen; ++i )
	if ( strcmp(name,gd->atomdata[i].atomname)==0 )
return( gd->atomdata[i].xatom );

    if ( i>=gd->amax )
	gd->atomdata = realloc(gd->atomdata,(gd->amax+=10)*sizeof(struct atomdata));
    gd->atomdata[i].atomname = copy(name);
    gd->atomdata[i].xatom = XInternAtom(gd->display,name,false);
    ++gd->alen;
return( gd->atomdata[i].xatom );
}

static void GXDrawClearSelData(GXDisplay *gd,enum selnames sel) {
    struct seldata *sd = gd->selinfo[sel].datalist, *next;

    while ( sd!=NULL ) {
	next = sd->next;
	if ( sd->freedata )
	    (sd->freedata)(sd->data);
	else
	    free(sd->data);
	free(sd);
	sd = next;
    }
    gd->selinfo[sel].datalist = NULL;
    gd->selinfo[sel].owner = NULL;
}

static void GXDrawGrabSelection(GWindow w,enum selnames sel) {
    GXDisplay *gd = (GXDisplay *) (w->display);
    GXWindow gw = (GXWindow) w;
    if ( gd->selinfo[sel].owner!=NULL && gd->selinfo[sel].datalist != NULL) {
	GEvent e;
	memset(&e,0,sizeof(e));
	e.type = et_selclear;
	e.u.selclear.sel = sel;
	e.native_window = (void *) (intpt) gd->selinfo[sel].owner->w;
	if ( gd->selinfo[sel].owner->eh!=NULL )
	    (gd->selinfo[sel].owner->eh)((GWindow) gd->selinfo[sel].owner, &e);
    }
    //Only one clipboard exists on Windows. Selectively set the selection owner
    //as otherwise the Windows clipboard will be cleared.
#ifdef _WIN32
    if (sel == sn_clipboard)
#endif
    XSetSelectionOwner(gd->display,gd->selinfo[sel].sel_atom,gw->w,gd->last_event_time);
    GXDrawClearSelData(gd,sel);
    gd->selinfo[sel].owner = gw;
    gd->selinfo[sel].timestamp = gd->last_event_time;
}

static void GXDrawAddSelectionType(GWindow w,enum selnames sel,char *type,
	void *data,int32 cnt,int32 unitsize, void *(*gendata)(void *,int32 *len),
	void (*freedata)(void *)) {
    GXDisplay *gd = (GXDisplay *) (w->display);
    int typeatom = GXDrawGetAtom(gd,type);
    struct seldata *sd;

    if ( unitsize!=1 && unitsize!=2 && unitsize!=4 ) {
	GDrawIError( "Bad unitsize to GXDrawAddSelectionType" );
	unitsize = 1;
    }
    for ( sd=gd->selinfo[sel].datalist; sd!=NULL && sd->typeatom!=typeatom;
	    sd = sd->next );
    if ( sd==NULL ) {
	sd = malloc(sizeof(struct seldata));
	sd->next = gd->selinfo[sel].datalist;
	gd->selinfo[sel].datalist = sd;
	sd->typeatom = typeatom;
    }
    sd->cnt = cnt;
    sd->data = data;
    sd->unitsize = unitsize;
    sd->gendata = gendata;
    sd->freedata = freedata;
}

static void GXDrawTransmitSelection(GXDisplay *gd,XEvent *event) {
    int prop_set = False;
    int which;
    XEvent e_to_send;
    Atom *targets;
    Atom cur_targ = event->xselectionrequest.target;
    Atom prop;
    int tlen;
    struct seldata *sd;
    int is_multiple = cur_targ == GXDrawGetAtom(gd,"MULTIPLE");
    int found = 0;
    void *temp;
    int32 proplen;

    for ( which = 0; which<sn_max; ++which )
	if ( event->xselectionrequest.selection == gd->selinfo[which].sel_atom )
    break;
    if ( which==sn_max )
return;

    e_to_send.type = SelectionNotify;
    e_to_send.xselection.display = event->xselectionrequest.display;
    e_to_send.xselection.requestor = event->xselectionrequest.requestor;
    e_to_send.xselection.selection = event->xselectionrequest.selection;
    e_to_send.xselection.target = event->xselectionrequest.target;
    e_to_send.xselection.property = event->xselectionrequest.property;
    e_to_send.xselection.time = event->xselectionrequest.time;
     /* Obsolete convention */
    if ( e_to_send.xselection.property==None )
	e_to_send.xselection.property = e_to_send.xselection.target;
    prop = e_to_send.xselection.property;

    tlen = 0;
    for ( sd = gd->selinfo[which].datalist; sd!=NULL && !found; sd = sd->next, ++tlen ) {
	if ( cur_targ==sd->typeatom || is_multiple ) {
	    found = (cur_targ==sd->typeatom);
	    prop_set = 1;
	    if (is_multiple)
		prop = sd->typeatom;
	    temp = sd->data;
	    proplen = sd->cnt;
	    if ( sd->gendata )
		temp = (sd->gendata)(temp,&proplen);
	    XChangeProperty(e_to_send.xselection.display,
					e_to_send.xselection.requestor,
					prop,
					sd->typeatom,
					8*sd->unitsize,PropModeReplace,
					temp,proplen);
	    if ( sd->gendata )
		free( temp );
	}
    }
    sd = gd->selinfo[which].datalist;
    if (sd!=NULL && ( cur_targ==GXDrawGetAtom(gd,"LENGTH") || is_multiple )) {
	if ( is_multiple )
	    prop = GXDrawGetAtom(gd,"LENGTH");
	temp = NULL; proplen = sd->cnt*sd->unitsize;
	if ( sd->gendata )
	    temp = (sd->gendata)(sd->data,&proplen);
	XChangeProperty(e_to_send.xselection.display,
				    e_to_send.xselection.requestor,
				    prop,
				    GXDrawGetAtom(gd,"LENGTH"),32,PropModeReplace,
				    (void *) &proplen,1);
	free(temp);
	prop_set = True;
    }
    if ( sd!=NULL && ( cur_targ==GXDrawGetAtom(gd,"IDENTIFY") || is_multiple )) {
	int temp = sd->typeatom;
	if (is_multiple)
	    prop = GXDrawGetAtom(gd,"IDENTIFY");
	XChangeProperty(e_to_send.xselection.display,
				    e_to_send.xselection.requestor,
				    prop,
				    GXDrawGetAtom(gd,"IDENTIFY"),32,PropModeReplace,
				    (void *) &temp,1);
	prop_set = True;
    }
    if ( cur_targ==GXDrawGetAtom(gd,"TIMESTAMP") || is_multiple ) {
	if (is_multiple)
	    prop = GXDrawGetAtom(gd,"TIMESTAMP");
	XChangeProperty(e_to_send.xselection.display,
				    e_to_send.xselection.requestor,
				    prop,
				    XA_INTEGER,32,PropModeReplace,
				    (void *) &gd->selinfo[which].timestamp,1);
	prop_set = True;
    }
    if ( cur_targ==GXDrawGetAtom(gd,"TARGETS") || is_multiple ) {
	int i;
	targets = calloc(tlen+5,sizeof(Atom));
	for ( sd = gd->selinfo[which].datalist, i=0; sd!=NULL; sd = sd->next, ++i )
	    targets[i] = sd->typeatom;
	targets[i++] = GXDrawGetAtom(gd,"LENGTH");
	targets[i++] = GXDrawGetAtom(gd,"IDENTIFY");
	targets[i++] = GXDrawGetAtom(gd,"TIMESTAMP");
	targets[i++] = GXDrawGetAtom(gd,"TARGETS");
	targets[i++] = GXDrawGetAtom(gd,"MULTIPLE");
	if (is_multiple)
	    prop = GXDrawGetAtom(gd,"TARGETS");
	XChangeProperty(e_to_send.xselection.display,
				    e_to_send.xselection.requestor,
				    prop,
				    XA_ATOM,32,PropModeReplace,
				    (void *) targets,i);
	free(targets);
	prop_set = True;
    }
    if ( is_multiple ) {
	int i;
	targets = calloc(tlen+5,sizeof(Atom));
	for ( sd = gd->selinfo[which].datalist, i=0; sd!=NULL; sd = sd->next, ++i )
	    targets[i] = sd->typeatom;
	targets[i++] = GXDrawGetAtom(gd,"LENGTH");
	targets[i++] = GXDrawGetAtom(gd,"IDENTIFY");
	targets[i++] = GXDrawGetAtom(gd,"TIMESTAMP");
	targets[i++] = GXDrawGetAtom(gd,"TARGETS");
	targets[i++] = GXDrawGetAtom(gd,"MULTIPLE");
	XChangeProperty(e_to_send.xselection.display,
				    e_to_send.xselection.requestor,
				    targets[i-1],	/* multiple */
				    XA_ATOM,32,PropModeReplace,
				    (void *) targets,i);
	XChangeProperty(e_to_send.xselection.display,
				    e_to_send.xselection.requestor,
				    e_to_send.xselection.target,
				    XA_ATOM,32,PropModeReplace,
				    (void *) targets,i);
	free(targets);
    }

    if ( !prop_set )
	e_to_send.xselection.property = None;
    XSendEvent(gd->display,e_to_send.xselection.requestor,True,0,&e_to_send);
}

static void *GXDrawRequestSelection(GWindow w,enum selnames sn, char *typename, int32 *len) {
    GXDisplay *gd = (GXDisplay *) (w->display);
    GXWindow gw = (GXWindow) w;
    Display *display = gd->display;
    unsigned long nitems, bytes_after;
    Atom actual_type;
    int actual_format;
    char *prop;
    char *temp;
    int bytelen;
    Atom typeatom = GXDrawGetAtom(gd,typename);
    XEvent xevent;
    struct seldata *sd;

    if ( len!=NULL )
	*len = 0;

    /* Do we own the selection? If so check for the type in our list of things*/
    /*  if present return a copy (so they can free it), if absent return NULL */
    if ( gd->selinfo[sn].owner!=NULL ) {
	for ( sd=gd->selinfo[sn].datalist; sd!=NULL; sd=sd->next ) {
	    if ( sd->typeatom == typeatom ) {
		if ( sd->gendata!=NULL ) {
		    temp = (sd->gendata)(sd->data,len);
		    *len *= sd->unitsize;
		} else {
		    bytelen = sd->unitsize*sd->cnt;
		    temp = malloc(bytelen+4);
		    memcpy(temp,sd->data,bytelen);
		    temp[bytelen] = '\0';
		    temp[bytelen+1] = '\0';
		    temp[bytelen+2] = '\0';
		    temp[bytelen+3] = '\0';
		    *len = bytelen;
		}
return( temp );
	    }
	}
return( NULL );
    }

    /* Otherwise ask the owner for the selection, wait to be notified that he's*/
    /*  given it to us (we might time out, return NULL if we do) */
    XConvertSelection(display, gd->selinfo[sn].sel_atom, typeatom,
	   gd->selinfo[sn].sel_atom, gw->w,gd->last_event_time);
    if ( !GXDrawWaitForNotifyEvent(gd,&xevent, gw->w) ||
	    xevent.xselection.property == None ) {
return( NULL );
    } else if (XGetWindowProperty(display,xevent.xselection.requestor,
	      xevent.xselection.property,0L,100000000L,True,AnyPropertyType,
	      &actual_type,&actual_format,&nitems,&bytes_after,
	      (unsigned char **) &prop) != Success ||
	    prop==NULL ) {
	GDrawIError("Could not retrieve property in GXDrawRequestSelection" );
return( NULL );
    }

    bytelen = nitems * (actual_format/8);
    temp = malloc(bytelen+4);
    memcpy(temp,prop,bytelen);
    temp[bytelen]='\0';
    temp[bytelen+1]='\0';		/* Nul terminate unicode strings too */
    temp[bytelen+2] = '\0';
    temp[bytelen+3] = '\0';
    if ( len!=NULL )
	*len = bytelen;
    XFree(prop);
return(temp);
}

static int GXDrawSelectionHasType(GWindow w,enum selnames sn, char *typename) {
    GXDisplay *gd = (GXDisplay *) (w->display);
    Display *display = gd->display;
    GXWindow gw = (GXWindow) w;
    unsigned long nitems, bytes_after;
    Atom actual_type;
    int actual_format;
    char *prop;
    Atom typeatom = GXDrawGetAtom(gd,typename);
    int i;
    XEvent xevent;
    struct seldata *sd;

    /* Do we own the selection? If so check for the type in our list of things*/
    /*  if present return a copy (so they can free it), if absent return NULL */
    if ( gd->selinfo[sn].owner!=NULL ) {
	for ( sd=gd->selinfo[sn].datalist; sd!=NULL; sd=sd->next ) {
	    if ( sd->typeatom == typeatom )
return( true );
	}
return( false );
    }

    if ( gd->seltypes.timestamp!=gd->last_event_time ) {
	/* List is not up to date, ask for a new one */
	gd->seltypes.cnt = 0;
	XFree(gd->seltypes.types); gd->seltypes.types = NULL;
	XConvertSelection(display, gd->selinfo[sn].sel_atom, GXDrawGetAtom(gd,"TARGETS"),
	       gd->selinfo[sn].sel_atom, gw->w,gd->last_event_time);
	if ( !GXDrawWaitForNotifyEvent(gd,&xevent, gw->w) ||
		xevent.xselection.property == None ) {
return( false );
	} else if (XGetWindowProperty(display,xevent.xselection.requestor,
		  xevent.xselection.property,0L,100000000L,True,AnyPropertyType,
		  &actual_type,&actual_format,&nitems,&bytes_after,
		  (unsigned char **) &prop) != Success ||
		prop==NULL || actual_format!=32 ) {
	GDrawIError("Could not retrieve property in GXDrawSelectionHasType" );
return( false );
	} else {
	    gd->seltypes.cnt = nitems;
	    gd->seltypes.types = (Atom *) prop;
	    gd->seltypes.timestamp = gd->last_event_time = xevent.xselection.time;
	}
    }
    for ( i=0; i<gd->seltypes.cnt; ++i )
	if ( gd->seltypes.types[i]==typeatom )
return( true );

return( false );
}

static void GXDrawBindSelection(GDisplay *disp,enum selnames sn, char *atomname) {
    GXDisplay *gdisp = (GXDisplay *) disp;
    Display *display = gdisp->display;
    if ( sn>=0 && sn<sn_max )
	gdisp->selinfo[sn].sel_atom = XInternAtom(display,atomname,False);
}

static int GXDrawSelectionHasOwner(GDisplay *disp,enum selnames sn) {
    GXDisplay *gdisp = (GXDisplay *) disp;
    Display *display = ((GXDisplay *) gdisp)->display;
    if ( sn<0 || sn>=sn_max )
return( false );

return( XGetSelectionOwner(display,gdisp->selinfo[sn].sel_atom)!=None );
}

static int match(char **list, char *val) {
    int i;

    for ( i=0; list[i]!=NULL; ++i )
	if ( strmatch(val,list[i])==0 )
return( i );

return( -1 );
}

static void *vc_cvt(char *val, void *def) {
    static char *classes[] = { "StaticGray", "GrayScale", "StaticColor", "PsuedoColor", "TrueColor", "DirectColor", NULL };
    int ret = match(classes,val);
    if ( ret== -1 ) {
	char *ept;
	ret = strtol(val,&ept,10);
	if ( ept==val || *ept!='\0' )
return( def );
    }
return( (void *) (intpt) ret );
}

static void *cm_cvt(char *val, void *def) {
    static char *choices[] = { "default", "current", "copy", "private", NULL };
    int ret = match(choices,val);
    if ( ret== -1 )
return( (void *) -1 );

return( (void *) (intpt) (ret-1) );
}

static void GXResourceInit(GXDisplay *gdisp,char *programname) {
    Atom rmatom, type;
    int format, i; unsigned long nitems, bytes_after;
    unsigned char *ret = NULL;
    GResStruct res[21];
    int dithertemp; double sizetemp, sizetempcm;
    int depth = -1, vc = -1, cm=-1, cmpos;
    int tbf = 1;
#if __Mac
    int mxc = 1;	/* Don't leave this on by default. The cmd key uses the same bit as numlock on other systems */
#else
    int mxc = 0;
#endif

    rmatom = XInternAtom(gdisp->display,"RESOURCE_MANAGER",true);
    if ( rmatom!=None ) {
	XGetWindowProperty(gdisp->display,((GXWindow) (gdisp->groot))->w, rmatom, 0,
		0x7fffff, false, XA_STRING,
		&type, &format, &nitems, &bytes_after, &ret);
	if ( type == None )
	    ret = NULL;
	else if ( type!=XA_STRING || format!=8 ) {
	    XFree(ret);
	    ret = NULL;
	}
    }
    GResourceAddResourceString((char *) ret,programname);
    if ( ret!=NULL ) XFree(ret);

    memset(res,0,sizeof(res));
    i = 0;
    res[i].resname = "MultiClickTime"; res[i].type = rt_int; res[i].val = &gdisp->bs.double_time; ++i;
    res[i].resname = "MultiClickWiggle"; res[i].type = rt_int; res[i].val = &gdisp->bs.double_wiggle; ++i;
    res[i].resname = "SelectionNotifyTimeout"; res[i].type = rt_int; res[i].val = &gdisp->SelNotifyTimeout; ++i;
    dithertemp = gdisp->do_dithering;
    res[i].resname = "DoDithering"; res[i].type = rt_bool; res[i].val = &dithertemp; ++i;
    res[i].resname = "ScreenWidthPixels"; res[i].type = rt_int; res[i].val = &gdisp->groot->pos.width; ++i;
    res[i].resname = "ScreenHeightPixels"; res[i].type = rt_int; res[i].val = &gdisp->groot->pos.height; ++i;
    sizetemp = WidthMMOfScreen(DefaultScreenOfDisplay(gdisp->display))/25.4;
    sizetempcm = WidthMMOfScreen(DefaultScreenOfDisplay(gdisp->display))/10;
    gdisp->xres = gdisp->groot->pos.width/sizetemp;
    res[i].resname = "ScreenWidthInches"; res[i].type = rt_double; res[i].val = &sizetemp; ++i;
    cmpos = i;
    res[i].resname = "ScreenWidthCentimeters"; res[i].type = rt_double; res[i].val = &sizetempcm; ++i;
    res[i].resname = "Depth"; res[i].type = rt_int; res[i].val = &depth; ++i;
    res[i].resname = "VisualClass"; res[i].type = rt_string; res[i].val = &vc; res[i].cvt=vc_cvt; ++i;
    res[i].resname = "TwoButtonFixup"; res[i].type = rt_bool; res[i].val = &tbf; ++i;
    res[i].resname = "MacOSXCmd"; res[i].type = rt_bool; res[i].val = &mxc; ++i;
    res[i].resname = "Colormap"; res[i].type = rt_string; res[i].val = &cm; res[i].cvt=cm_cvt; ++i;
    res[i].resname = NULL;
    GResourceFind(res,NULL);

    if ( !res[cmpos].found && !res[cmpos-1].found && rint(gdisp->groot->pos.width/sizetemp) == 75 )
	gdisp->res = 100;	/* X seems to think that if it doesn't know */
				/*  the screen width, then 75 dpi is a good guess */
			        /*  Now-a-days, 100 seems better */
    else
    if ( res[cmpos].found && sizetempcm>=1 )
	gdisp->res = gdisp->groot->pos.width*2.54/sizetempcm;
    else if ( sizetemp>=1 )
	gdisp->res = gdisp->groot->pos.width/sizetemp;
    gdisp->desired_depth = depth; gdisp->desired_vc = vc;
    gdisp->desired_cm = cm;
    gdisp->macosx_cmd = mxc;
    gdisp->twobmouse_win = tbf;
}

static GWindow GXPrinterStartJob(GDisplay *gdisp,void *user_data,GPrinterAttrs *attrs) {
    fprintf(stderr, "Invalid call to GPrinterStartJob on X display\n" );
return( NULL );
}

static void GXPrinterNextPage(GWindow w) {
    fprintf(stderr, "Invalid call to GPrinterNextPage on X display\n" );
}

static int GXPrinterEndJob(GWindow w,int cancel) {
    fprintf(stderr, "Invalid call to GPrinterEndJob on X display\n" );
return( false );
}

static struct displayfuncs xfuncs = {
    GXDrawInit,
    GXDrawTerm,
    GXDrawNativeDisplay,

    GXDrawSetDefaultIcon,

    GXDrawCreateTopWindow,
    GXDrawCreateSubWindow,
    GXDrawCreatePixmap,
    GXDrawCreateBitmap,
    GXDrawCreateCursor,
    GXDrawDestroyWindow,
    GXDestroyCursor,
    GXNativeWindowExists,
    GXDrawSetZoom,
    GXDrawSetWindowBorder,
    GXDrawSetWindowBackground,
    GXSetDither,

    GXDrawReparentWindow,
    GXDrawSetVisible,
    GXDrawMove,
    GXDrawTrueMove,
    GXDrawResize,
    GXDrawMoveResize,
    GXDrawRaise,
    GXDrawRaiseAbove,
    GXDrawIsAbove,
    GXDrawLower,
    GXDrawSetWindowTitles,
    GXDrawSetWindowTitles8,
    GXDrawGetWindowTitle,
    GXDrawGetWindowTitle8,
    GXDrawSetTransientFor,
    GXDrawGetPointerPosition,
    GXDrawGetPointerWindow,
    GXDrawSetCursor,
    GXDrawGetCursor,
    GXDrawGetRedirectWindow,
    GXDrawTranslateCoordinates,

    GXDrawBeep,
    GXDrawFlush,

    GXDrawPushClip,
    GXDrawPopClip,

    GXDrawSetDifferenceMode,

    GXDrawClear,
    GXDrawDrawLine,
    GXDrawDrawArrow,
    GXDrawDrawRect,
    GXDrawFillRect,
    GXDrawFillRoundRect,
    GXDrawDrawElipse,
    GXDrawFillElipse,
    GXDrawDrawArc,
    GXDrawDrawPoly,
    GXDrawFillPoly,
    GXDrawScroll,

    _GXDraw_Image,
    _GXDraw_TileImage,
    _GXDraw_Glyph,
    _GXDraw_ImageMagnified,
    _GXDraw_CopyScreenToImage,
    _GXDraw_Pixmap,
    _GXDraw_TilePixmap,

    GXDrawCreateInputContext,
    GXDrawSetGIC,
    GXDrawKeyState,

    GXDrawGrabSelection,
    GXDrawAddSelectionType,
    GXDrawRequestSelection,
    GXDrawSelectionHasType,
    GXDrawBindSelection,
    GXDrawSelectionHasOwner,

    GXDrawPointerUngrab,
    GXDrawPointerGrab,
    GXDrawRequestExpose,
    GXDrawForceUpdate,
    GXDrawSync,
    GXDrawSkipMouseMoveEvents,
    GXDrawProcessPendingEvents,
    GXDrawProcessWindowEvents,
    GXDrawProcessOneEvent,
    GXDrawEventLoop,
    GXDrawPostEvent,
    GXDrawPostDragEvent,
    GXDrawRequestDeviceEvents,

    GXDrawRequestTimer,
    GXDrawCancelTimer,

    GXDrawSyncThread,

    GXPrinterStartJob,
    GXPrinterNextPage,
    GXPrinterEndJob,

    GXDrawFontMetrics,

    GXDrawHasCairo,
    GXDrawPathStartNew,
    GXDrawPathClose,
    GXDrawPathMoveTo,
    GXDrawPathLineTo,
    GXDrawPathCurveTo,
    GXDrawPathStroke,
    GXDrawPathFill,
    GXDrawPathFillAndStroke,

    GXDrawLayoutInit,
    GXDraw_LayoutDraw,
    GXDraw_LayoutIndexToPos,
    GXDraw_LayoutXYToIndex,
    GXDraw_LayoutExtents,
    GXDraw_LayoutSetWidth,
    GXDraw_LayoutLineCount,
    GXDraw_LayoutLineStart,
    GXDrawPathStartSubNew,
    GXDrawFillRuleSetWinding,

    _GXPDraw_DoText8,

    GXDrawPushClipOnly,
    GXDrawClipPreserve
};

static void GDrawInitXKB(GXDisplay *gdisp) {
#ifdef _NO_XKB
    gdisp->has_xkb = false;
#else
    int lib_major = XkbMajorVersion, lib_minor = XkbMinorVersion;

    gdisp->has_xkb = false;
    if ( XkbLibraryVersion(&lib_major, &lib_minor))
	gdisp->has_xkb = XkbQueryExtension(gdisp->display,
		&gdisp->xkb.opcode,&gdisp->xkb.event,&gdisp->xkb.error,
		&lib_major,&lib_minor);
    if ( gdisp->has_xkb ) {
	int mask = XkbNewKeyboardNotifyMask | XkbMapNotifyMask;
	XkbSelectEvents(gdisp->display,XkbUseCoreKbd,mask,mask);
    }
#endif
}

void _GXDraw_DestroyDisplay(GDisplay * gdisp) {
    GXDisplay* gdispc = (GXDisplay*)(gdisp);
    if (gdispc->grey_stipple != BadAlloc && gdispc->grey_stipple != BadDrawable && gdispc->grey_stipple != BadValue) {
      XFreePixmap(gdispc->display, gdispc->grey_stipple); gdispc->grey_stipple = BadAlloc;
    }
    if (gdispc->fence_stipple != BadAlloc && gdispc->fence_stipple != BadDrawable && gdispc->fence_stipple != BadValue) {
      XFreePixmap(gdispc->display, gdispc->fence_stipple); gdispc->fence_stipple = BadAlloc;
    }
    if (gdispc->pango_context != NULL) {
        g_object_unref(gdispc->pango_context); gdispc->pango_context = NULL;
    }
    if (gdispc->groot != NULL) {
      if (gdispc->groot->ggc != NULL) { free(gdispc->groot->ggc); gdispc->groot->ggc = NULL; }
      free(gdispc->groot); gdispc->groot = NULL;
    }
    if (gdispc->im != NULL) { XCloseIM(gdispc->im); gdispc->im = NULL; }
    if (gdispc->display != NULL) { XCloseDisplay(gdispc->display); gdispc->display = NULL; }
    return;
}

GDisplay *_GXDraw_CreateDisplay(char *displayname,char *programname) {
    GXDisplay *gdisp;
    Display *display;
    GXWindow groot;
    Window focus;
    int revert;
    static unsigned char grey_init[8] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
    static unsigned char fence_init[8] = { 0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88};
#ifdef HAVE_PTHREAD_H
    static pthread_mutex_t defmutex = PTHREAD_MUTEX_INITIALIZER;
#endif

    display = XOpenDisplay(displayname);
    if ( display==NULL )
return( NULL );

    setlocale(LC_ALL,"");
    XSupportsLocale();
    XSetLocaleModifiers("");

    gdisp = calloc(1,sizeof(GXDisplay));
    if ( gdisp==NULL ) {
	XCloseDisplay(display);
return( NULL );
    }

    gdisp->funcs = &xfuncs;
    gdisp->display = display;
    gdisp->screen = DefaultScreen(display);
    gdisp->root = RootWindow(display,gdisp->screen);
    gdisp->virtualRoot = BadAlloc;
    gdisp->res = (25.4*WidthOfScreen(DefaultScreenOfDisplay(display)))/
	    WidthMMOfScreen(DefaultScreenOfDisplay(display));
    gdisp->scale_screen_by = 1;
    gdisp->mykey_keysym = XK_F12;
    gdisp->mykey_mask = 0;
    gdisp->do_dithering = true;
    gdisp->desired_vc = gdisp->desired_depth = -1;

    gdisp->gcstate[0].gc = NULL;
    gdisp->gcstate[0].fore_col = 0x1000000;	/* Doesn't match any colour */
    gdisp->gcstate[0].back_col = 0x1000000;	/* Doesn't match any colour */
    gdisp->gcstate[0].clip.x = gdisp->gcstate[0].clip.y = 0;
    gdisp->gcstate[0].clip.width = gdisp->gcstate[0].clip.height = 0x7fff;

    gdisp->gcstate[1].fore_col = 0x1000000;	/* Doesn't match any colour */
    gdisp->gcstate[1].back_col = 0x1000000;	/* Doesn't match any colour */
    gdisp->gcstate[1].clip.x = gdisp->gcstate[1].clip.y = 0;
    gdisp->gcstate[1].clip.width = gdisp->gcstate[1].clip.height = 0x7fff;

    gdisp->bs.double_time = 200;
    gdisp->bs.double_wiggle = 3;
    gdisp->SelNotifyTimeout = 20;		/* wait 20 seconds for a response to a selection request */

    while ( gdisp->mycontext==0 )
	gdisp->mycontext = XUniqueContext();

    gdisp->grey_stipple = XCreatePixmapFromBitmapData(display,gdisp->root,(char *) grey_init,8,8,1,0,1);
    gdisp->fence_stipple = XCreatePixmapFromBitmapData(display,gdisp->root,(char *) fence_init,8,8,1,0,1);

    XGetInputFocus(display,&focus,&revert);
    if ( focus==PointerRoot )
	gdisp->focusfollowsmouse = true;

    gdisp->groot = calloc(1,sizeof(struct gxwindow));
    groot = (GXWindow)(gdisp->groot);
    groot->ggc = _GXDraw_NewGGC();
    groot->display = gdisp;
    groot->w = gdisp->root;
    groot->pos.width = XDisplayWidth(display,gdisp->screen);
    groot->pos.height = XDisplayHeight(display,gdisp->screen);
    groot->is_toplevel = true;
    groot->is_visible = true;
    
    GXResourceInit(gdisp,programname);

    gdisp->bs.double_time = GResourceFindInt( "DoubleClickTime", gdisp->bs.double_time );
    gdisp->def_background = GResourceFindColor( "Background", COLOR_CREATE(0xf5,0xff,0xfa));
    gdisp->def_foreground = GResourceFindColor( "Foreground", COLOR_CREATE(0x00,0x00,0x00));
    if ( GResourceFindBool("Synchronize", false ))
	XSynchronize(gdisp->display,true);

#ifdef X_HAVE_UTF8_STRING	/* Don't even try without this. I don't want to have to guess encodings myself... */
    /* X Input method initialization */
    XSetLocaleModifiers(""); // As it turns out, we can't free this here.
    gdisp->im = XOpenIM(display, XrmGetDatabase(display),
	    GResourceProgramName, GResourceProgramName);
    /* The only reason this seems to fail is if XMODIFIERS contains an @im */
    /*  which points to something that isn't running. If XMODIFIERS is not */
    /*  defined we get some kind of built-in default method. If it doesn't */
    /*  recognize the locale we still get something */
    /* If it does fail, then fall back on the old fashioned stuff */
#endif

    (gdisp->funcs->init)((GDisplay *) gdisp);
    gdisp->top_window_count = 0;
    gdisp->selinfo[sn_primary].sel_atom = XA_PRIMARY;
    gdisp->selinfo[sn_clipboard].sel_atom = XInternAtom(display,"CLIPBOARD",False);
    gdisp->selinfo[sn_drag_and_drop].sel_atom = XInternAtom(display,"DRAG_AND_DROP",False);
    gdisp->selinfo[sn_user1].sel_atom = XA_PRIMARY;
    gdisp->selinfo[sn_user2].sel_atom = XA_PRIMARY;

    gdisp->xthread.sync_sock = -1;
#ifdef HAVE_PTHREAD_H
    gdisp->xthread.sync_mutex = defmutex;
    gdisp->xthread.things_to_do = NULL;
#endif
    XSetErrorHandler(/*gdisp->display,*/myerrorhandler);
    _GDraw_InitError((GDisplay *) gdisp);

#ifdef _WACOM_DRV_BROKEN
    _GXDraw_Wacom_Init(gdisp);
#endif

    GDrawInitXKB(gdisp);

return( (GDisplay *) gdisp);
}

void _XSyncScreen() {
    XSync(((GXDisplay *) screen_display)->display,false);
}

/* map GK_ keys to X keys */
/* Assumes most are mapped 1-1, see gkeysym.h */
/* abort on unimplemented translations */
int GDrawKeyToXK(int keysym) {
    switch( keysym ) {
      default:
	if ( keysym==' ' ||
		(keysym>='0' && keysym<='9') ||
		(keysym>='A' && keysym<='Z') ||
		(keysym>='a' && keysym<='z') )
return( keysym );
    }
    abort();
return( 0 );
}

#else	/* NO X */

GDisplay *_GXDraw_CreateDisplay(char *displayname,char *programname) {
    fprintf( stderr, "This program was not compiled with X11, and cannot open the display\n" );
    exit(1);
}

void _XSyncScreen() {
}
#endif

#endif // FONTFORGE_CAN_USE_GDK
