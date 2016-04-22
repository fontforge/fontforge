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
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#if !defined(__MINGW32__)
#include <sys/wait.h>
#endif
#include "fontforge-config.h"
#include <unistd.h>

#include "gpsdrawP.h"
#include "gresource.h"
#include "colorP.h"
#include "fontP.h"
#include "ustring.h"
#include "fileutil.h"

/* ************************************************************************** */
/* ********************** Noops & Meaningless functions ********************* */
/* ************************************************************************** */

static void PSDrawInit(GDisplay *UNUSED(gdisp)) {
    /* delay any real initialization until they actually want to print */
    /*  no point in reading up on a bunch of fonts which never get used */
}

static void PSDrawTerm(GDisplay *UNUSED(gdisp)) {
}

static void *PSDrawNativeDisplay(GDisplay *UNUSED(gdisp)) {
return( NULL );
}

static void PSDrawSetDefaultIcon(GWindow UNUSED(icon)) {
}

static GWindow PSDrawCreateSubWindow(GWindow UNUSED(w), GRect *UNUSED(pos),
        int (*eh)(GWindow,GEvent *), void *UNUSED(user_data),
        GWindowAttrs *UNUSED(wattrs)) {
    (void)eh;
    fprintf( stderr, "CreateSubWindow not implemented for postscript\n" );
return NULL;
}

static GWindow PSDrawCreatePixmap(GDisplay *UNUSED(gdisp), uint16 UNUSED(width),
        uint16 UNUSED(height)) {
    fprintf( stderr, "CreatePixmap not implemented for postscript\n" );
return NULL;
}

static GWindow PSDrawCreateBitmap(GDisplay *UNUSED(gdisp), uint16 UNUSED(width),
        uint16 UNUSED(height), uint8 *UNUSED(data)) {
    fprintf( stderr, "CreateBitmap not implemented for postscript\n" );
return NULL;
}

static GCursor PSDrawCreateCursor(GWindow UNUSED(src), GWindow UNUSED(mask),
        Color UNUSED(fg), Color UNUSED(bg), int16 UNUSED(x), int16 UNUSED(y) ) {
    fprintf( stderr, "CreateCursor not implemented for postscript\n" );
return 0;
}

static void PSSetZoom(GWindow UNUSED(w), GRect *UNUSED(r),
        enum gzoom_flags UNUSED(flags) ) {
    fprintf( stderr, "SetZoom not implemented for postscript\n" );
}

static void PSDestroyCursor(GDisplay *UNUSED(gdisp), GCursor UNUSED(ct) ) {
    fprintf( stderr, "DestroyCursor not implemented for postscript\n" );
}

static int PSNativeWindowExists(GDisplay *UNUSED(gdisp), void *UNUSED(native) ) {
return( false );
}

static void PSSetWindowBorder(GWindow UNUSED(w), int UNUSED(width),
        Color UNUSED(col) ) {
    fprintf( stderr, "SetWindowBorder not implemented for postscript\n" );
}

static void PSSetWindowBackground(GWindow UNUSED(w), Color UNUSED(col) ) {
    fprintf( stderr, "SetWindowBackground not implemented for postscript\n" );
}

static int PSSetDither(GDisplay *UNUSED(gdisp), int UNUSED(dither) ) {
    fprintf( stderr, "SetDither not implemented for postscript\n" );
return( true );
}

static void PSDrawReparentWindow(GWindow UNUSED(child),
        GWindow UNUSED(newparent), int UNUSED(x), int UNUSED(y)) {
    /* It's a noop */
}

static void PSDrawSetVisible(GWindow UNUSED(w), int UNUSED(vis) ) {
    /* It's a noop */
}

static void PSDrawMove(GWindow UNUSED(w), int32 UNUSED(x), int32 UNUSED(y)) {
    /* Not meaningful */
}

static void PSDrawResize(GWindow UNUSED(w),
        int32 UNUSED(width), int32 UNUSED(height)) {
    /* Not meaningful */
}

static void PSDrawMoveResize(GWindow UNUSED(w), int32 UNUSED(x), int32 UNUSED(y),
        int32 UNUSED(width), int32 UNUSED(height)) {
    /* Not meaningful */
}

static void PSDrawRaise(GWindow UNUSED(w)) {
    /* Not meaningful */
}

static void PSDrawRaiseAbove(GWindow UNUSED(w), GWindow UNUSED(below)) {
    /* Not meaningful */
}

static int PSDrawIsAbove(GWindow UNUSED(w), GWindow UNUSED(below)) {
return( -1 );		/* Not meaningful */
}

static void PSDrawLower(GWindow UNUSED(w)) {
    /* Not meaningful */
}

static void PSDrawSetWindowTitles(GWindow UNUSED(w),
        const unichar_t *UNUSED(title), const unichar_t *UNUSED(icontit)) {
    /* Not meaningful */
}

static void PSDrawSetWindowTitles8(GWindow UNUSED(w),
        const char *UNUSED(title), const char *UNUSED(icontit)) {
    /* Not meaningful */
}

static void PSDrawSetTransientFor(GWindow UNUSED(transient), GWindow UNUSED(owner)) {
    /* Not meaningful */
}

static void PSDrawGetPointerPosition(GWindow UNUSED(w), GEvent *ret) {
    /* Not meaningful */

    ret->u.mouse.state = 0;
    ret->u.mouse.x = 0;
    ret->u.mouse.y = 0;
}

static GWindow PSDrawGetPointerWindow(GWindow UNUSED(w)) {
    /* Not meaningful */
return( NULL );
}

static void PSDrawSetCursor(GWindow UNUSED(w), GCursor UNUSED(ct)) {
    /* Not meaningful */
}

static GCursor PSDrawGetCursor(GWindow UNUSED(w)) {
    /* Not meaningful */
return( ct_default );
}

static GWindow PSDrawGetRedirectWindow(GDisplay *UNUSED(gd)) {
    /* Not meaningful */
return( NULL );
}

static unichar_t *PSDrawGetWindowTitle(GWindow UNUSED(w)) {
return(NULL);
}

static char *PSDrawGetWindowTitle8(GWindow UNUSED(w)) {
return(NULL);
}

static void PSDrawTranslateCoordinates(GWindow UNUSED(_from),
        GWindow UNUSED(_to), GPoint *UNUSED(pt)) {
    /* Only one window active at a time, translation is a noop */
}

static void PSDrawScroll(GWindow UNUSED(w), GRect *UNUSED(rect),
        int32 UNUSED(hor), int32 UNUSED(vert)) {
    /* Not meaningful */
}

static void PSDrawBeep(GDisplay *UNUSED(gdisp)) {
    /* Not meaningful */
}

static void PSDrawFlush(GDisplay *gdisp) {
    /* Not meaningful */
    if ( gdisp->groot!=NULL )
	fflush(((GPSWindow) (gdisp->groot))->output_file);
}

static GIC *PSDrawCreateInputContext(GWindow UNUSED(w),
        enum gic_style UNUSED(def_style)) {
    /* Not meaningful */
return( NULL );
}

static void PSDrawSetGIC(GWindow UNUSED(w), GIC *UNUSED(gic),
        int UNUSED(x), int UNUSED(y)) {
    /* Not meaningful */
}

static int PSDrawKeyState(GWindow UNUSED(w), int UNUSED(keysym)) {
    /* Not meaningful */
    return 0;
}

static void PSDrawPointerUngrab(GDisplay *UNUSED(gdisp)) {
    /* Not meaningful */
}

static void PSDrawPointerGrab(GWindow UNUSED(w)) {
    /* Not meaningful */
}

static void PSDrawGrabSelection(GWindow UNUSED(w), enum selnames UNUSED(sel)) {
    /* Not meaningful */
}

static void PSDrawAddSelectionType(GWindow UNUSED(w), enum selnames UNUSED(sel),
        char *UNUSED(type), void *UNUSED(data), int32 UNUSED(len),
        int32 UNUSED(unitsize), void *(*gendata)(void *,int32 *len),
        void (*freedata)(void *)) {
    /* Not meaningful */
    (void)gendata;
    (void)freedata;
}

static void *PSDrawRequestSelection(GWindow UNUSED(w), enum selnames UNUSED(sn),
        char *UNUSED(typename), int32 *UNUSED(len)) {
    /* Not meaningful */
return( NULL );
}

static int PSDrawSelectionHasType(GWindow UNUSED(w), enum selnames UNUSED(sn),
        char *UNUSED(typename)) {
    /* Not meaningful */
return( false );
}

static void PSDrawBindSelection(GDisplay *UNUSED(disp), enum selnames UNUSED(sn),
        char *UNUSED(atomname)) {
    /* Not meaningful */
}

static int PSDrawSelectionHasOwner(GDisplay *UNUSED(disp), enum selnames UNUSED(sn)) {
    /* Not meaningful */
return( false );
}

static void PSDrawRequestExpose(GWindow UNUSED(gw), GRect *UNUSED(rect),
    int UNUSED(doclear)) {
    /* Not meaningful */
}

static GTimer *PSDrawRequestTimer(GWindow UNUSED(w), int32 UNUSED(time_from_now),
        int32 UNUSED(frequency), void *UNUSED(userdata)) {
return( NULL );
}

static void PSDrawCancelTimer(GTimer *UNUSED(timer)) {
}

static void PSDrawSyncThread(GDisplay *UNUSED(gdisp), void (*func)(void *), void *data) {
    (func)(data);
}

static void PSDrawForceUpdate(GWindow UNUSED(gw)) {
}

static void PSDrawSync(GDisplay *UNUSED(gdisp)) {
}

static void PSDrawSkipMouseMoveEvents(GWindow UNUSED(gw), GEvent *UNUSED(last)) {
}

static void PSDrawProcessPendingEvents(GDisplay *UNUSED(gdisp)) {
}

static void PSDrawProcessWindowEvents(GWindow UNUSED(gw)) {
}

static void PSDrawEventLoop(GDisplay *UNUSED(gd)) {
}

static void PSDrawPostEvent(GEvent *UNUSED(e)) {
}

static void PSDrawPostDragEvent(GWindow UNUSED(w), GEvent *UNUSED(mouse),
        enum event_type UNUSED(et)) {
}

static int  PSDrawRequestDeviceEvents(GWindow UNUSED(w), int UNUSED(devcnt),
        struct gdeveventmask *UNUSED(de)) {
return( 0 );
}

static GImage *_PSDraw_CopyScreenToImage(GWindow UNUSED(w), GRect *UNUSED(rect)) {
return( NULL );
}

static void _PSDraw_Pixmap( GWindow UNUSED(_w), GWindow UNUSED(_pixmap),
        GRect *UNUSED(src), int32 UNUSED(x), int32 UNUSED(y)) {
}

static void _PSDraw_TilePixmap( GWindow UNUSED(_w), GWindow UNUSED(_pixmap),
        GRect *UNUSED(src), int32 UNUSED(x), int32 UNUSED(y)) {
}

/* ************************************************************************** */
/* ******************************* Draw Stuff ******************************* */
/* ************************************************************************** */

double _GSPDraw_Distance(GPSWindow ps,int x) {
return( 72.0*(double) x/(double)ps->display->res );
}

double _GSPDraw_XPos(GPSWindow ps,int x) {
return( 72.0*(double) x/(double)ps->display->res );
}

double _GSPDraw_YPos(GPSWindow ps,int y) {
return( -72.0*(double) y/(double)ps->display->res );
}

static void PSUnbufferLine(GPSWindow ps) {
    if ( ps->buffered_line ) {
	fprintf( ps->output_file,"  %g %g lineto\n", _GSPDraw_XPos(ps,ps->line_x), _GSPDraw_YPos(ps,ps->line_y) );
	++ps->pnt_cnt;
	ps->buffered_line = false;
    }
}

void _GPSDraw_FlushPath(GPSWindow ps) {
    if ( ps->buffered_line )
	PSUnbufferLine(ps);
    if ( ps->pnt_cnt>0 ) {
	fprintf( ps->output_file,"stroke\n" );
	ps->pnt_cnt = 0;
	ps->cur_x = ps->cur_y = -1;
    }
}

static void PSDrawNewpath(GPSWindow ps) {
    if ( ps->pnt_cnt!=0 ) {
	_GPSDraw_FlushPath(ps);
	fprintf( ps->output_file,"newpath\n" );
	ps->pnt_cnt = 0;
	ps->cur_x = ps->cur_y = -1;
    }
}

static void PSMoveTo(GPSWindow ps, int x, int y) {
    if ( ps->pnt_cnt>=STROKE_CACHE )
	_GPSDraw_FlushPath(ps);
    if ( ps->pnt_cnt==-1 )
	PSDrawNewpath(ps);
    if ( ps->cur_x!=x || ps->cur_y!=y ) {
	if ( ps->buffered_line )
	    PSUnbufferLine(ps);
	fprintf( ps->output_file,"  %g %g moveto\n", _GSPDraw_XPos(ps,x), _GSPDraw_YPos(ps,y) );
	++ps->pnt_cnt;
	ps->cur_x = x; ps->cur_y = y;
    }
}

static void PSLineTo(GPSWindow ps, int x, int y) {
    if ( ps->pnt_cnt>=STROKE_CACHE )
	_GPSDraw_FlushPath(ps);
    if ( ps->pnt_cnt==-1 )
	PSDrawNewpath(ps);
    if ( ps->cur_x!=x || ps->cur_y!=y ) {
	if ( !ps->buffered_line && ps->cur_y==y ) {
	    ps->line_x = x; ps->line_y = y;
	    ps->buffered_line = true;
	} else if ( ps->buffered_line && ps->cur_y==y ) {
	    ps->line_x = x;
	} else {
	    if ( ps->buffered_line )
		PSUnbufferLine(ps);
	    fprintf( ps->output_file,"  %g %g lineto\n", _GSPDraw_XPos(ps,x), _GSPDraw_YPos(ps,y) );
	    ++ps->pnt_cnt;
	}
	ps->cur_x = x; ps->cur_y = y;
    }
}

/* This is my version of the postscript "arc" function. I've written it because*/
/*  I can't use arc for stroking elipses. The obvious thing to do is to scale */
/*  x and y differently so that the circle becomes an elipse. Sadly that also */
/*  scales the line width so I get a different width at different points on   */
/*  the elipse. Can't have that. */
/* So here we approximate an elipse by four bezier curves. There is one curve */
/*  for each quadrant of the elipse (0->90), (90->180), (180->270), (270-360) */
/*  of course if we're just doing bits of the arc (rather than the whole      */
/*  elipse) we'll not have all of those, nor will they be filled in. The      */
/*  magic numbers here do indeed produce a good fit to an elipse */
static void PSDoArc(GPSWindow ps, double cx, double cy, double radx, double rady,
	double sa, double ea ) {
    double ss, sc, es, ec;
    double lenx, leny;
    double sx, sy, ex, ey;
    double c1x, c1y, c2x, c2y;

    lenx = ((ea-sa)/90.0) * radx * .552;
    leny = ((ea-sa)/90.0) * rady * .552;
    sa *= 3.1415926535897932/180; ea *= 3.1415926535897932/180;
    ss = -sin(sa); sc = cos(sa); es = -sin(ea); ec = cos(ea);
    sx = cx+sc*radx; sy = cy+ss*rady;
    PSMoveTo(ps,sx,sy);
    ex = cx+ec*radx; ey = cy+es*rady;
    c1x = sx + lenx*ss; c1y = sy - leny*sc;
    c2x = ex - lenx*es; c2y = ey + leny*ec;
    fprintf( ps->output_file, " %g %g %g %g %g %g curveto",
	    _GSPDraw_XPos(ps,c1x), _GSPDraw_YPos(ps,c1y),
	    _GSPDraw_XPos(ps,c2x), _GSPDraw_YPos(ps,c2y),
	    _GSPDraw_XPos(ps,ex), _GSPDraw_YPos(ps,ey) );
    ps->cur_x = ex; ps->cur_y = ey;
}

static void PSMyArc(GPSWindow ps, double cx, double cy, double radx, double rady,
	double sa, double ta ) {
    double ea, temp;

    if ( ta<0 ) {
	sa += ta;
	ta = -ta;
    }
    if ( ta>360 ) ta = 360;
    if ( ta==360 ) sa = 0;
    while ( sa<0 ) sa+= 360;
    while ( sa>=360 ) sa -= 360;
    ea = sa+ta;

    while ( sa<ea ) {
	temp = ((int) ((sa+90)/90))*90;
	PSDoArc(ps,cx,cy,radx,rady,sa,ea<temp?ea:temp);
	sa = temp;
    }
}

static void PSDrawElipse(GPSWindow ps, GRect *rct,char *command) {
    float cx, cy, radx, rady;
    /* This does not make a very good stroked elipse if it isn't a circle */
    /*  the line width is scaled along with the elipse and so it varies!!! */

    _GPSDraw_FlushPath(ps);
    radx = rct->width/2.0;
    rady = rct->height/2.0;
    cx = rct->x + radx;
    cy = rct->y + rady;
    PSDrawNewpath(ps);

    /* I can't use arc to draw an elipse. Doing so would mean scaling the */
    /*  x and y axes differently, which will also scale the stroke width */
    /*  differently. So I've written my own version that will work for */
    /*  elipses without scaling. I use arc for circles because it produces */
    /*  less text in the printing file */
    if ( radx!=rady )
	PSMyArc(ps,cx,cy,radx,rady,0,360);
    else
	fprintf( ps->output_file, "  %g %g %g 0 360 arc", _GSPDraw_XPos(ps,cx), _GSPDraw_YPos(ps,cy), _GSPDraw_Distance(ps,radx));
    fprintf( ps->output_file, " closepath %s\n", command );
    ps->pnt_cnt = 0;
    ps->cur_x = ps->cur_y = -1;
}

static void PSDrawDoPoly(GPSWindow ps, GPoint *pt, int cnt, char *command) {
    int i;

    if ( pt[cnt-1].x==pt[0].x && pt[cnt-1].y==pt[0].y )
	--cnt;		/* We close paths differently in postscript */
    _GPSDraw_FlushPath(ps);
    if ( cnt==4 ) {
	fprintf( ps->output_file, "  %g %g  %g %g  %g %g  %g %g g_quad ",
		_GSPDraw_XPos(ps,pt[3].x), _GSPDraw_YPos(ps,pt[3].y),
		_GSPDraw_XPos(ps,pt[2].x), _GSPDraw_YPos(ps,pt[2].y),
		_GSPDraw_XPos(ps,pt[1].x), _GSPDraw_YPos(ps,pt[1].y),
		_GSPDraw_XPos(ps,pt[0].x), _GSPDraw_YPos(ps,pt[0].y));
    } else {
	PSMoveTo(ps,pt[0].x,pt[0].y);
	for ( i=1; i<cnt; ++i )
	    PSLineTo(ps,pt[i].x,pt[i].y);
    }
    fprintf( ps->output_file, "closepath %s %%Polygon\n", command );
    ps->pnt_cnt = 0;
    ps->cur_x = ps->cur_y = -1;
}

void _GPSDraw_SetClip(GPSWindow ps) {

    if ( ps->ggc->clip.x!=ps->cur_clip.x ||
	    ps->ggc->clip.width!=ps->cur_clip.width ||
	    ps->ggc->clip.y!=ps->cur_clip.y ||
	    ps->ggc->clip.height!=ps->cur_clip.height ) {
	_GPSDraw_FlushPath(ps);
	if ( ps->ggc->clip.x<ps->cur_clip.x ||
		ps->ggc->clip.x+ps->ggc->clip.width>ps->cur_clip.x+ps->cur_clip.width ||
		ps->ggc->clip.y<ps->cur_clip.y ||
		ps->ggc->clip.y+ps->ggc->clip.height>ps->cur_clip.y+ps->cur_clip.height )
	    fprintf( ps->output_file, "initclip " );	/* we hope never to fall into this case, postscript doesn't like it */
	fprintf( ps->output_file, "  %g %g  %g %g  %g %g  %g %g g_quad clip newpath\n",
		_GSPDraw_XPos(ps,ps->ggc->clip.x), _GSPDraw_YPos(ps,ps->ggc->clip.y+ps->ggc->clip.height),
		_GSPDraw_XPos(ps,ps->ggc->clip.x+ps->ggc->clip.width), _GSPDraw_YPos(ps,ps->ggc->clip.y+ps->ggc->clip.height),
		_GSPDraw_XPos(ps,ps->ggc->clip.x+ps->ggc->clip.width), _GSPDraw_YPos(ps,ps->ggc->clip.y),
		_GSPDraw_XPos(ps,ps->ggc->clip.x), _GSPDraw_YPos(ps,ps->ggc->clip.y));
	ps->cur_clip = ps->ggc->clip;
    }
}

void _GPSDraw_SetColor(GPSWindow ps,Color fg) {
    if ( ps->display->do_color ) {
	fprintf( ps->output_file, "%g %g %g setrgbcolor\n", COLOR_RED(fg)/255.0,
		COLOR_GREEN(fg)/255.0, COLOR_BLUE(fg)/255.0 );
    } else {
	fprintf( ps->output_file, "%g setgray\n", COLOR2GREYR(fg));
    }
    ps->cur_fg = fg;
}

static int PSDrawSetcol(GPSWindow ps) {

    _GPSDraw_SetClip(ps);
    if ( ps->ggc->fg!=ps->cur_fg || ps->ggc->ts!=ps->cur_ts ) {
	_GPSDraw_FlushPath(ps);
	if ( ps->ggc->ts!=ps->cur_ts ) {
	    if ( ps->ggc->ts ) {
		fprintf( ps->output_file, "currentcolor DotPattern setpattern\n" );
		ps->cur_ts = ps->ggc->ts;
	    } else {
		fprintf( ps->output_file, "%s setcolorspace\n",
			ps->display->do_color?"/DeviceRGB":"/DeviceGray" );
		ps->cur_ts = 0;
	    }
	}
	_GPSDraw_SetColor(ps,ps->ggc->fg);
    }
return( true );
}

static int PSDrawSetline(GPSWindow ps) {

    PSDrawSetcol(ps);
    if ( ps->ggc->line_width!=ps->cur_line_width ) {
	_GPSDraw_FlushPath(ps);
	fprintf( ps->output_file, "%g setlinewidth\n", _GSPDraw_XPos(ps,ps->ggc->line_width) );
	ps->cur_line_width = ps->ggc->line_width;
    }
    if ( ps->ggc->dash_len != ps->cur_dash_len || ps->ggc->skip_len != ps->cur_skip_len ||
	    ps->ggc->dash_offset != ps->cur_dash_offset ) {
	_GPSDraw_FlushPath(ps);
	if ( ps->ggc->dash_len==0 ) {
	    fprintf( ps->output_file, "[] 0 setdash\n" );
	} else {
	    fprintf( ps->output_file, "[%d %d] %d setdash\n",
		    ps->ggc->dash_len, ps->ggc->skip_len,
		    PixelToPoint(ps->ggc->dash_offset,ps->res)%(ps->ggc->dash_len+ps->ggc->skip_len) );
	}
	ps->cur_dash_offset = ps->ggc->dash_offset;
	ps->cur_dash_len = ps->ggc->dash_len;
	ps->cur_skip_len = ps->ggc->skip_len;
    }
return( true );
}

static void PSDrawPushClip(GWindow w,GRect *rct, GRect *old) {
    GPSWindow ps = (GPSWindow ) w;

    _GPSDraw_FlushPath(ps);
    fprintf( ps->output_file,"gsave\n" );
    ps->last_dash_len = ps->cur_dash_len;
    ps->last_skip_len = ps->cur_skip_len;
    ps->last_dash_offset = ps->cur_dash_offset;
    ps->last_line_width = ps->cur_line_width;
    ps->last_ts = ps->cur_ts;
    ps->last_fg = ps->cur_fg;
    ps->last_font = ps->cur_font;
    *old = ps->ggc->clip;
    ps->ggc->clip = *rct;
}

static void PSDrawPopClip(GWindow w,GRect *old) {
    GPSWindow ps = (GPSWindow ) w;

    _GPSDraw_FlushPath(ps);
    ps->cur_clip = *old;
    fprintf( ps->output_file,"grestore\n" );
    ps->cur_dash_len = ps->last_dash_len;
    ps->cur_skip_len = ps->last_skip_len;
    ps->cur_line_width = ps->last_line_width;
    ps->cur_dash_offset = ps->last_dash_offset;
    ps->cur_ts = ps->last_ts;
    ps->cur_fg = ps->last_fg;
    ps->cur_font = ps->last_font;
    ps->last_dash_len = -1;
    ps->last_skip_len = -1;
    ps->last_line_width = -1;
    ps->last_dash_offset = -1;
    ps->last_ts = -1;
    ps->last_fg = -1;
    ps->last_font = NULL;
    ps->ggc->clip = *old;
}

static void PSDrawSetDifferenceMode(GWindow UNUSED(w)) {
    fprintf(stderr, "PSDrawSetDifferenceMode not implemented for postscript\n" );
}

static void PSDrawDrawLine(GWindow w, int32 x,int32 y,int32 xend,int32 yend,Color col) {
    GPSWindow ps = (GPSWindow ) w;

    ps->ggc->fg = col;
    PSDrawSetline(ps);
    PSMoveTo(ps,x,y);
    PSLineTo(ps,xend,yend);
}

static void PSDrawArrow(GPSWindow ps, int32 x, int32 y, int32 xother, int32 yother ) {
    GPoint points[3];
    double a;
    int off1, off2;
    double len;
    int line_width = ps->ggc->line_width;

    if ( x==xother && y==yother )
return;
    a = atan2(y-yother,x-xother);
    len = 72.0*sqrt((double) (x-xother)*(x-xother)+(y-yother)*(y-yother))/ps->res;
    if ( len>30 ) len = 10+3*line_width/2; else len = (len+line_width)/3;
    if ( len<2 )
return;
    len *= ps->res/72.0;

    points[0].x = x; points[0].y = y;
    if ( line_width!=0 ) {
	points[0].x += line_width*1.3*cos(a); points[0].y += line_width*1.3*sin(a);
    }
    off1 = len*sin(a+3.1415926535897932/8)+.5; off2 = len*cos(a+3.1415926535897932/8)+.5;
    points[1].x = x-off2; points[1].y = y-off1;
    off1 = len*sin(a-3.1415926535897932/8)+.5; off2 = len*cos(a-3.1415926535897932/8)+.5;
    points[2].x = x-off2; points[2].y = y-off1;
    PSDrawDoPoly(ps,points,3,"fill");
}

static void PSDrawDrawArrowLine(GWindow w, int32 x,int32 y,int32 xend,int32 yend,int16 arrows,Color col) {
    GPSWindow ps = (GPSWindow ) w;

    ps->ggc->fg = col;
    PSDrawSetline(ps);
    PSMoveTo(ps,x,y);
    PSLineTo(ps,xend,yend);
    if ( arrows&1 )
	PSDrawArrow(ps,x,y,xend,yend);
    if ( arrows&2 )
	PSDrawArrow(ps,xend,yend,x,y);
}

static void PSDrawDrawRect(GWindow w, GRect *rct,Color col) {
    GPSWindow ps = (GPSWindow ) w;

    ps->ggc->fg = col;
    PSDrawSetline(ps);
    fprintf( ps->output_file, "  %g %g  %g %g  %g %g  %g %g g_quad stroke\n",
	    _GSPDraw_XPos(ps,rct->x), _GSPDraw_YPos(ps,rct->y+rct->height),
	    _GSPDraw_XPos(ps,rct->x+rct->width), _GSPDraw_YPos(ps,rct->y+rct->height),
	    _GSPDraw_XPos(ps,rct->x+rct->width), _GSPDraw_YPos(ps,rct->y),
	    _GSPDraw_XPos(ps,rct->x), _GSPDraw_YPos(ps,rct->y));
    ps->pnt_cnt = 0;
}

static void PSDrawFillRect(GWindow w, GRect *rct,Color col) {
    GPSWindow ps = (GPSWindow ) w;

    ps->ggc->fg = col;
    PSDrawSetcol(ps);
    _GPSDraw_FlushPath(ps);
    fprintf( ps->output_file, "  %g %g  %g %g  %g %g  %g %g g_quad fill\n",
	    _GSPDraw_XPos(ps,rct->x), _GSPDraw_YPos(ps,rct->y+rct->height),
	    _GSPDraw_XPos(ps,rct->x+rct->width), _GSPDraw_YPos(ps,rct->y+rct->height),
	    _GSPDraw_XPos(ps,rct->x+rct->width), _GSPDraw_YPos(ps,rct->y),
	    _GSPDraw_XPos(ps,rct->x), _GSPDraw_YPos(ps,rct->y));
    ps->pnt_cnt = 0;
}

static void PSDrawFillRoundRect(GWindow UNUSED(w), GRect *UNUSED(rct),
        int UNUSED(radius), Color UNUSED(col)) {
    fprintf( stderr, "DrawFillRoundRect not implemented for postscript\n" );
return;
}

static void PSDrawClear(GWindow w,GRect *rect) {
    GPSWindow ps = (GPSWindow ) w;
    if ( rect==NULL )
	rect = &ps->ggc->clip;
    PSDrawFillRect(w,rect,COLOR_CREATE(255,255,255));
}

static void PSDrawDrawCircle(GWindow w, GRect *rct,Color col) {
    GPSWindow ps = (GPSWindow ) w;

    ps->ggc->fg = col;
    PSDrawSetline(ps);
    PSDrawElipse(ps,rct,"stroke");
}

static void PSDrawFillCircle(GWindow w, GRect *rct,Color col) {
    GPSWindow ps = (GPSWindow ) w;

    ps->ggc->fg = col;
    PSDrawSetcol(ps);
    _GPSDraw_FlushPath(ps);
    PSDrawElipse(ps,rct,"fill");
}

static void PSDrawDrawArc(GWindow w, GRect *rct,int32 sa, int32 ta, Color col) {
    GPSWindow ps = (GPSWindow ) w;
    float cx, cy, radx, rady;

    ps->ggc->fg = col;
    PSDrawSetline(ps);
    _GPSDraw_FlushPath(ps);

    radx = rct->width/2.0;
    rady = rct->height/2.0;
    cx = rct->x + radx;
    cy = rct->y + rady;
    if ( radx==0 || rady==0 )
return;
    PSDrawNewpath(ps);
    if ( radx!=rady )
	PSMyArc(ps,cx,cy,radx,rady,sa/64.0,ta/64.0);
    else
	fprintf( ps->output_file, "  %g %g %g %g %g arc", _GSPDraw_XPos(ps,cx), _GSPDraw_YPos(ps,cy), _GSPDraw_Distance(ps,radx),
		    sa/64.0, (sa+ta)/64.0 );
    fprintf( ps->output_file, " stroke\n" );
    ps->pnt_cnt = 0;
    ps->cur_x = ps->cur_y = -1;
}

static void PSDrawDrawPoly(GWindow w, GPoint *pt, int16 cnt,Color col) {
    GPSWindow ps = (GPSWindow ) w;

    ps->ggc->fg = col;
    PSDrawSetline(ps);
    PSDrawDoPoly(ps,pt,cnt,"stroke");
}

static void PSDrawFillPoly(GWindow w, GPoint *pt, int16 cnt,Color col) {
    GPSWindow ps = (GPSWindow ) w;

    ps->ggc->fg = col;
    PSDrawSetcol(ps);
    _GPSDraw_FlushPath(ps);
    PSDrawDoPoly(ps,pt,cnt,"fill");
}

static void PSDrawFontMetrics(GWindow w, FontInstance *fi, int *as, int *ds, int *ld) {
    GDrawWindowFontMetrics(w, fi, as, ds, ld);
}

static enum gcairo_flags PSDrawHasCairo(GWindow UNUSED(w)) {
return( gc_buildpath );
}

static void PSDrawPathStartNew(GWindow w) {
    GPSWindow ps = (GPSWindow ) w;
    fprintf( ps->output_file,"  newpath\n" );
}

static void PSDrawPathClose(GWindow w) {
    GPSWindow ps = (GPSWindow ) w;
    fprintf( ps->output_file,"  closepath\n" );
}

static void PSDrawPathMoveTo(GWindow w,double x, double y) {
    GPSWindow ps = (GPSWindow ) w;
    fprintf( ps->output_file,"  %g %g moveto\n", _GSPDraw_XPos(ps,x), _GSPDraw_YPos(ps,y) );
}

static void PSDrawPathLineTo(GWindow w,double x, double y) {
    GPSWindow ps = (GPSWindow ) w;
    fprintf( ps->output_file,"  %g %g lineto\n", _GSPDraw_XPos(ps,x), _GSPDraw_YPos(ps,y) );
}

static void PSDrawPathCurveTo(GWindow w,
		    double cx1, double cy1,
		    double cx2, double cy2,
		    double x, double y) {
    GPSWindow ps = (GPSWindow ) w;
    fprintf( ps->output_file,"  %g %g %g %g %g %g curveto\n",
	    _GSPDraw_XPos(ps,cx1), _GSPDraw_YPos(ps,cy1),
	    _GSPDraw_XPos(ps,cx2), _GSPDraw_YPos(ps,cy2),
	    _GSPDraw_XPos(ps,x), _GSPDraw_YPos(ps,y) );
}

static void PSDrawPathStroke(GWindow w,Color col) {
    GPSWindow ps = (GPSWindow ) w;
    ps->ggc->fg = col;
    PSDrawSetcol(ps);
    fprintf( ps->output_file,"   stroke\n" );
}

static void PSDrawPathFill(GWindow w,Color col) {
    GPSWindow ps = (GPSWindow ) w;
    ps->ggc->fg = col;
    PSDrawSetcol(ps);
    fprintf( ps->output_file,"   fill\n" );
}

static void PSDrawPathFillAndStroke(GWindow w,Color fillcol, Color strokecol) {
    GPSWindow ps = (GPSWindow ) w;
    ps->ggc->fg = fillcol;
    PSDrawSetcol(ps);
    fprintf( ps->output_file,"   save fill restore " );
    ps->ggc->fg = strokecol;
    PSDrawSetcol(ps);
    fprintf( ps->output_file,"   stroke\n" );
}

static void PSPageSetup(GPSWindow ps,FILE *out, float thumbscale) {
    GPSDisplay *gdisp = ps->display;
    /* I want to put this somewhere in the document header, but DSC conventions*/
    /*  don't let me. Frustrating. Or I don't understand how, also frustrating*/

    fprintf( out, "  %g %g translate\t\t%%Left & Top Margins\n",
	    gdisp->lmargin*72.0, (gdisp->yheight-gdisp->tmargin)*72.0 );
    fprintf( out, "  %g %g  %g %g  %g %g  %g %g g_quad clip newpath\t%%Clip to margins\n",
	    _GSPDraw_XPos(ps,0), _GSPDraw_YPos(ps,0),
	    _GSPDraw_XPos(ps,0), _GSPDraw_YPos(ps,(gdisp->yheight-gdisp->tmargin-gdisp->bmargin)*gdisp->res),
	    _GSPDraw_XPos(ps,(gdisp->xwidth-gdisp->lmargin-gdisp->rmargin)*gdisp->res), _GSPDraw_YPos(ps,(gdisp->yheight-gdisp->tmargin-gdisp->bmargin)*gdisp->res),
	    _GSPDraw_XPos(ps,(gdisp->xwidth-gdisp->lmargin-gdisp->rmargin)*gdisp->res), _GSPDraw_YPos(ps,0));
    if ( gdisp->scale*thumbscale!=1 )
	fprintf( out, "  %g %g scale\n", gdisp->scale*thumbscale, gdisp->scale*thumbscale );
}

static void PSPageInit(GPSWindow ps) {
    GPSDisplay *gdisp = (GPSDisplay *) (ps->display);
    int thumb_per_page = gdisp->linear_thumb_cnt*gdisp->linear_thumb_cnt;
    int real_page = ( ps->page_cnt%thumb_per_page == 0 );

    ++ps->page_cnt;
    if ( gdisp->eps ) {
	fprintf( ps->output_file, "\n%%%%Page: 1 1\n" );
	fprintf( ps->output_file, "%%%%BeginPageSetup\n" );
	PSPageSetup(ps,ps->output_file,1.0);
	fprintf( ps->output_file, "%%%%EndPageSetup\n" );
    } else if ( real_page ) {
	int page = (ps->page_cnt+thumb_per_page-1)/thumb_per_page;
	fprintf( ps->output_file, "\n%%%%Page: %d %d\n", page, page );
	fprintf( ps->output_file, "%%%%BeginPageSetup\n" );
	fprintf( ps->output_file, "g_startpage\n" );
	fprintf( ps->output_file, "%%%%EndPageSetup\n" );
    } else
	fprintf( ps->output_file, "\n%% Psuedo Page: \ng_startpage\n" );
    ps->ggc->clip.y = ps->ggc->clip.x = 0;
    ps->ggc->clip.height = ps->pos.height; ps->ggc->clip.width = ps->pos.width;
    ps->cur_clip = ps->ggc->clip;
    ps->cur_skip_len = ps->cur_dash_len = ps->cur_line_width = ps->cur_dash_offset = ps->cur_ts = 0;
    ps->cur_font = NULL; ps->cur_fg = 0;
}

static void PSPageTerm(GPSWindow ps, int last) {
    GPSDisplay *gdisp = (GPSDisplay *) (ps->display);
    int thumb_per_page = gdisp->linear_thumb_cnt*gdisp->linear_thumb_cnt;

    _GPSDraw_FlushPath(ps);
    if ( gdisp->eps )
return;
    if ( last || ps->page_cnt%thumb_per_page==0 ) {
	fprintf( ps->output_file, "%%%%PageTrailer\n" );
	if ( last )
	    fprintf( ps->output_file, "g_finalpage\t\t%%End of Page\n" );
	else
	    fprintf( ps->output_file, "g_endpage\t\t%%End of Page\n" );
	fprintf( ps->output_file, "%%%%EndPageTrailer\n" );
    } else
	fprintf( ps->output_file, "g_endpage\t\t%%End of Psuedo Page\n" );
}

static void PSTrailer(GPSWindow ps) {
    GPSDisplay *gdisp = (GPSDisplay *) (ps->display);
    int thumb_per_page = gdisp->linear_thumb_cnt*gdisp->linear_thumb_cnt;

    fprintf( ps->output_file, "%%%%Trailer\n" );
    if ( !gdisp->eps )
	fprintf( ps->output_file, "%%%%Pages: %d\n",
		(int) (ps->page_cnt+thumb_per_page-1)/thumb_per_page);
    _GPSDraw_ListNeededFonts(ps);
    fprintf( ps->output_file, "%%%%EndTrailer\n" );
    fprintf( ps->output_file, "%%%%EOF\n" );
}

static void PSInitJob(GPSWindow ps, unichar_t *title) {
    GPSDisplay *gdisp = ps->display;
    char buf[200];
    time_t now;

    /* output magic line to tell lpr this is a postscript file */
    if ( gdisp->eps ) {
	fprintf( ps->init_file, "%%!PS-Adobe-3.0 EPSF-3.0\n" );
	fprintf( ps->init_file, "%%%%Pages: 1\n" );
    } else {
	fprintf( ps->init_file, "%%!PS-Adobe-3.0%s\n", gdisp->eps?" EPSF-3.0":"" );
	fprintf( ps->init_file, "%%%%Pages: (atend)\n" );
    }
    fprintf( ps->init_file, "%%%%BoundingBox: %g %g %g %g\n",
	    72.0*gdisp->lmargin, 72.0*gdisp->bmargin,
	    72.0*(gdisp->xwidth-gdisp->rmargin), 72.0*(gdisp->yheight-gdisp->tmargin));
    fprintf( ps->init_file, "%%%%Creator: %s\n", GResourceProgramName );
    time(&now);
    fprintf( ps->init_file, "%%%%CreationDate: %s", ctime(&now) );
    if ( title!=NULL )
	fprintf( ps->init_file, "%%%%Title: %s\n", u2def_strncpy(buf,title,sizeof(buf)) );
    fprintf( ps->init_file, "%%%%DocumentData: Clean7Bit\n" );
    fprintf( ps->init_file, "%%%%LanguageLevel: 2\n" );
    fprintf( ps->init_file, "%%%%Orientation: %s\n", gdisp->landscape?
	    "Landscape":"Portrait" );
    fprintf( ps->init_file, "%%%%PageOrder: Ascend\n" );
    fprintf( ps->init_file, "%%%%DocumentNeededResources: (atend)\n" );
    fprintf( ps->init_file, "%%%%DocumentSuppliedResources: (atend)\n" );
    fprintf( ps->init_file, "%%%%EndComments\n\n" );

    /* Need to output a setdevice thing to specify the page size (num copies?) ??? */
    fprintf( ps->init_file, "%%%%BeginPrologue\n" );

    /* Any predefined functions? */
	/* Need one to remap an adobe encoding to some other encoding */
    fprintf( ps->init_file, "%% <font> <encoding> font_remap <font>\t; from the cookbook\n" );
    fprintf( ps->init_file, "/reencodedict 5 dict def\n" );
    fprintf( ps->init_file, "/g_font_remap { reencodedict begin\n" );
    fprintf( ps->init_file, "  /newencoding exch def\n" );
    fprintf( ps->init_file, "  /basefont exch def\n" );
    fprintf( ps->init_file, "  /newfont basefont  maxlength dict def\n" );
    fprintf( ps->init_file, "  basefont {\n" );
    fprintf( ps->init_file, "    exch dup dup /FID ne exch /Encoding ne and\n" );
    fprintf( ps->init_file, "\t{ exch newfont 3 1 roll put }\n" );
    fprintf( ps->init_file, "\t{ pop pop }\n" );
    fprintf( ps->init_file, "    ifelse\n" );
    fprintf( ps->init_file, "  } forall\n" );
    fprintf( ps->init_file, "  newfont /Encoding newencoding put\n" );
    fprintf( ps->init_file, "  newfont\t%%Leave on stack\n" );
    fprintf( ps->init_file, "  end } def\n" );
	/* Simple routine to move somewhere and draw a string */
    fprintf( ps->init_file, "/g_show { moveto show } bind def\n" );
    fprintf( ps->init_file, "/g_ashow { moveto ashow } bind def\n" );
    fprintf( ps->init_file, "/g_quad { moveto lineto lineto lineto closepath } bind def\n" );
	/* Routines to start and end pages. simple unless we do thumbnails */
    if ( gdisp->eps ) {
	/* No page procedures at all */
    } else if ( gdisp->linear_thumb_cnt <= 1 ) {
	fprintf( ps->init_file, "/g_startpage { save \n" );
	PSPageSetup(ps,ps->init_file,1.0);
	fprintf( ps->init_file, "} bind def\n" );
	fprintf( ps->init_file, "/g_endpage { restore showpage } bind def\n" );
	fprintf( ps->init_file, "/g_finalpage { g_endpage } bind def\n" );
    } else {
	float stemp, thumbscale;
	thumbscale = (gdisp->xwidth-gdisp->lmargin-gdisp->rmargin)/
		(gdisp->linear_thumb_cnt*gdisp->xwidth-gdisp->lmargin-gdisp->rmargin);
	stemp = (gdisp->yheight-gdisp->tmargin-gdisp->bmargin)/
		(gdisp->linear_thumb_cnt*gdisp->yheight-gdisp->tmargin-gdisp->bmargin);
	if ( stemp<thumbscale ) thumbscale = stemp;
	thumbscale *= .97;	/* rounding errors make the real thing slightly too big */
	fprintf( ps->init_file, "/g_thumbnum 0 def\n" );
	fprintf( ps->init_file, "/g_startpage { \n" );
	fprintf( ps->init_file, "  g_thumbnum %d mod 0 eq { save \n",
		gdisp->linear_thumb_cnt*gdisp->linear_thumb_cnt );
	PSPageSetup(ps,ps->init_file,thumbscale);
	fprintf( ps->init_file, "  } if\n" );
	fprintf( ps->init_file, "  save\n" );
	fprintf( ps->init_file, "  g_thumbnum %d mod %d mul g_thumbnum %d idiv %d mul translate\n",
		gdisp->linear_thumb_cnt, (int) ps->pos.width,
		gdisp->linear_thumb_cnt, (int) -ps->pos.height );
	fprintf( ps->init_file, "  %g %g  %g %g  %g %g  %g %g g_quad clip newpath\n",
		_GSPDraw_XPos(ps,0), _GSPDraw_YPos(ps,0),
		_GSPDraw_XPos(ps,0), _GSPDraw_YPos(ps,ps->pos.height),
		_GSPDraw_XPos(ps,ps->pos.width), _GSPDraw_YPos(ps,ps->pos.height),
		_GSPDraw_XPos(ps,ps->pos.width), _GSPDraw_YPos(ps,0));
	fprintf(ps->init_file, "} bind def\n" );
	fprintf( ps->init_file, "/g_endpage { restore /g_thumbnum g_thumbnum 1 add def\n   g_thumbnum %d eq { restore /g_thumbnum 0 def showpage } if\n } bind def\n",
		gdisp->linear_thumb_cnt*gdisp->linear_thumb_cnt );
	fprintf( ps->init_file, "/g_finalpage { restore restore showpage } bind def\n" );
    }
    _GPSDraw_InitPatterns(ps);
    _GPSDraw_InitFonts(gdisp->fontstate);

    fprintf( ps->init_file, "%% Font Initialization (download needed fonts, remap locals)\n" );
    fprintf( ps->init_file, "/MyFontDict 100 dict def\n" );

    fprintf( ps->output_file, "\n%%%%EndProlog\n\n" );
    fprintf( ps->output_file, "\n%%%%BeginSetup\n" );
    if ( !gdisp->eps )
	fprintf( ps->output_file, "<< /PageSize [%g %g] >> setpagedevice\n\n",
		72.0*gdisp->xwidth, 72.0*gdisp->yheight );
    /* Could set NumCopies in the InputAttributes sub-dictionary but I think */
    /*  that is best done on the lpr command line */
    fprintf( ps->output_file, "%%%%EndSetup\n\n" );
    ps->cur_clip.x = ps->cur_clip.y = 0;
    ps->cur_clip.width = ps->pos.width;
    ps->cur_clip.height = ps->pos.height;
    ps->ggc->clip = ps->cur_clip;
    PSPageInit(ps);
}

static int PSQueueFile(GPSWindow ps) {
#if !defined(__MINGW32__)
    GPSDisplay *gdisp = ps->display;
    int pid = fork();

    rewind( ps->init_file );
    if ( pid==0 ) {
	/* In child */
	int in = fileno(stdin);
	char *prog;
	char *argv[30];
	int argc=0;
	char pbuf[200], cbuf[40];
	char buffer[1025];
	char *parg, *carg, *spt, *pt;

	close(in);
	dup2(fileno(ps->init_file),in);
	close(fileno(ps->init_file));

	if ( gdisp->use_lpr ) {
	    prog = "lpr";
	    parg = "P";
	    carg = "#";
	} else {
	    prog = "lp";
	    parg = "d";
	    carg = "n";
	}
	argv[argc++] = prog;
	if ( !gdisp->use_lpr )
	    argv[argc++] = "-s";
	if ( gdisp->printer_name!=NULL ) {
	    sprintf(pbuf, "-%s%s ", parg, gdisp->printer_name );
	    argv[argc++] = pbuf;
	}
	if ( gdisp->num_copies!=0 ) {
	    sprintf(cbuf, "-%s%d ", carg, gdisp->num_copies );
	    argv[argc++] = pbuf;
	}
	if ( gdisp->lpr_args!=NULL ) {
	    strncpy(buffer, gdisp->lpr_args,sizeof(buffer)-1 );
	    for ( spt = buffer; *spt==' '; ++spt );
	    while ( (pt = strchr(spt,' '))!=NULL ) {
		argv[argc++] = spt;
		*pt = '\0';
		for ( spt=pt+1; *spt==' '; ++spt );
	    }
	    if ( *spt!='\0' )
		argv[argc++] = spt;
	}
	argv[argc] = NULL;
	if ( execvp(prog,argv)==-1 )
	    _exit(1);
    } else if ( pid==-1 )
return( false );
    else {
	/* in parent */
	int status;
	if ( waitpid(pid,&status,0)==-1 )
return( false );
	if ( WIFEXITED(status))
return( true );
    }
#endif
return( false );
}

static int PSFinishJob(GPSWindow ps,int cancel) {
    GPSDisplay *gdisp = ps->display;
    int error = ferror(ps->output_file);

    if ( ps->output_file!=ps->init_file ) {
	rewind(ps->output_file);
	_GPSDraw_CopyFile(ps->init_file,ps->output_file);
	fclose(ps->output_file);
    }
    error |= ferror(ps->init_file);
    if ( error || cancel ) {
	if ( !cancel )
	    GDrawError("An error occurred while saving the print job to disk.\nNot printed." );
	if ( gdisp->filename!=NULL )
	    GFileUnlink(gdisp->filename);
	fclose(ps->init_file);
return(false);
    }
    if ( !gdisp->print_to_file ) {
	if ( !PSQueueFile(ps)) {
	    GDrawError("Could not queue print job" );
	    fclose(ps->init_file);
return( false );
	}
    }
    fclose(ps->init_file);
return( true );
}

static void PSDestroyContext(GPSDisplay *gd) {
    free(gd->groot->ggc);
    free(gd->groot);
    gd->groot = NULL;
}

static GGC *_GPSDraw_NewGGC(GPSDisplay *ps) {
    GGC *ggc = calloc(1,sizeof(GGC));
    ggc->clip.width = ggc->clip.height = 0x7fff;
    ggc->fg = 0;
    ggc->bg = 0xffffff;
    ggc->line_width = ps->scale_screen_by;
return( ggc );
}

static GWindow GPSPrinterStartJob(GDisplay *gd,void *user_data,GPrinterAttrs *attrs) {
    GPSDisplay *gdisp = (GPSDisplay *) gd;
    double factor = 1;
    char *oldpn, *oldea, *oldfn;
    FILE *output, *init;
    GPSWindow groot;

    if ( gd->groot!=NULL ) {
	GDrawError("Please wait for current print job to complete before starting a new one" );
return( NULL );
    }

    if ( attrs!=NULL ) {
	if ( attrs->units==pu_mm )
	    factor = 25.4;
	else if ( attrs->units == pu_points )
	    factor = 72;

	if ( attrs->mask&pam_pagesize ) {
	    gdisp->xwidth = attrs->width/factor;
	    gdisp->yheight = attrs->height/factor;
	}
	if ( attrs->mask&pam_margins ) {
	    gdisp->lmargin = attrs->lmargin/factor;
	    gdisp->rmargin = attrs->rmargin/factor;
	    gdisp->tmargin = attrs->tmargin/factor;
	    gdisp->bmargin = attrs->bmargin/factor;
	}
	if ( attrs->mask&pam_scale )
	    gdisp->scale = attrs->scale;
	if ( gdisp->scale<=0 ) gdisp->scale = 1.0;
	gdisp->last_units = attrs->units;
	if ( attrs->mask&pam_res )
	    gdisp->res = attrs->res;
	gdisp->scale_screen_by = gdisp->res/screen_display->res;
	if ( gdisp->scale_screen_by==0 ) gdisp->scale_screen_by =1;
	if ( attrs->mask&pam_copies )
	    gdisp->num_copies = attrs->num_copies;
	else
	    gdisp->num_copies = 1;
	if ( attrs->mask&pam_thumbnails )
	    gdisp->linear_thumb_cnt = attrs->thumbnails;
	else
	    gdisp->linear_thumb_cnt = 1;
	if ( gdisp->linear_thumb_cnt<=0 )
	    gdisp->linear_thumb_cnt = 1;
	if ( attrs->mask&pam_transparent )
	    gdisp->do_transparent = attrs->do_transparent;
	if ( attrs->mask&pam_color )
	    gdisp->do_color = attrs->do_color;
	if ( attrs->mask&pam_lpr )
	    gdisp->use_lpr = attrs->use_lpr;
	if ( attrs->mask&pam_queue )
	    gdisp->print_to_file = attrs->donot_queue;
	if ( attrs->mask&pam_eps )
	    gdisp->eps = attrs->eps;
	else
	    gdisp->eps = false;
	if ( gdisp->eps ) {
	    gdisp->print_to_file = true;
	    gdisp->linear_thumb_cnt = 1;
	    gdisp->scale_screen_by =1;
	}
	if ( attrs->mask&pam_landscape )
	    gdisp->landscape = attrs->landscape;

	oldpn = gdisp->printer_name; oldea = gdisp->lpr_args; oldfn = gdisp->filename;
	if ( gdisp->print_to_file && (attrs->mask&pam_filename) )
	    gdisp->filename = copy(attrs->file_name);
	else
	    gdisp->filename = NULL;
	if ( attrs->mask&pam_printername )
	    gdisp->printer_name = copy(attrs->printer_name);
	else
	    oldpn = NULL;
	if ( attrs->mask&pam_args )
	    gdisp->lpr_args = copy(attrs->extra_lpr_args);
	else
	    oldea = NULL;
	free(oldfn); free(oldpn); free(oldea);
    }
    if ( gdisp->filename==NULL ) {
	init = tmpfile();
	if ( init==NULL ) {
	    GDrawError("Can't open printer temporary file" );
return( NULL );
	}
    } else if (( init = fopen(gdisp->filename,"wb"))==NULL ) {
	GDrawError("Can't open %s: %s", gdisp->print_to_file?"user file":"printer spooling file",
		gdisp->filename);
return( NULL );
    }
    output = tmpfile();
    if ( output==NULL )
	output = init;

    gdisp->fontstate->res = gdisp->res;

    gdisp->groot = calloc(1,sizeof(struct gpswindow));
    groot = (GPSWindow)(gdisp->groot);
    groot->ggc = _GPSDraw_NewGGC(gdisp);
    groot->display = gdisp;
    groot->pos.width = (gdisp->xwidth-gdisp->lmargin-gdisp->rmargin)*gdisp->res/gdisp->scale;
    groot->pos.height = (gdisp->yheight-gdisp->tmargin-gdisp->bmargin)*gdisp->res/gdisp->scale;
    groot->user_data = user_data;
    groot->output_file = output;
    groot->init_file = init;
    groot->cur_x = groot->cur_y = -1;
    groot->cur_fg = COLOR_UNKNOWN;
    groot->pnt_cnt = -1;
    groot->res = gdisp->res;
    groot->cur_dash_len = groot->cur_skip_len = -1;
    groot->cur_line_width = -1;
    groot->cur_dash_offset = -1;
    groot->cur_ts = -1;
    groot->last_dash_len = groot->last_skip_len = -1;
    groot->last_line_width = -1;
    groot->last_dash_offset = -1;
    groot->last_ts = -1;
    groot->last_fg = COLOR_UNKNOWN;
    groot->is_toplevel = true;
    groot->is_visible = true;

    PSInitJob(groot,(attrs->mask&pam_title)?attrs->title:NULL);
return( (GWindow) groot);
}

static void GPSPrinterNextPage(GWindow w) {
    GPSWindow ps = (GPSWindow) w;
    if (ps->display->eps )
	GDrawIError("Attempt to start a new page within an encapsulated postscript document");
    else {
	PSPageTerm(ps,false);
	PSPageInit(ps);
    }
}

static int GPSPrinterEndJob(GWindow w,int cancel) {
    GPSWindow ps = (GPSWindow) w;
    GPSDisplay *gdisp = ps->display;
    int ret;

    PSPageTerm(ps,true);
    PSTrailer(ps);
    ret = PSFinishJob(ps,cancel);
    _GPSDraw_ResetFonts(gdisp->fontstate);
    PSDestroyContext(gdisp);
    free(gdisp->filename); gdisp->filename=NULL;
return( ret );
}

static GWindow PSDrawCreateTopWindow(GDisplay *gdisp, GRect *UNUSED(pos),
        int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
    (void)eh;
return( GPSPrinterStartJob(gdisp, wattrs==NULL ? NULL : user_data, NULL));
}

static void PSDrawDestroyWindow(GWindow w) {
    GPSPrinterEndJob(w,true);
}

static struct displayfuncs psfuncs = {
    PSDrawInit,
    PSDrawTerm,
    PSDrawNativeDisplay,

    PSDrawSetDefaultIcon,

    PSDrawCreateTopWindow,
    PSDrawCreateSubWindow,
    PSDrawCreatePixmap,
    PSDrawCreateBitmap,
    PSDrawCreateCursor,
    PSDrawDestroyWindow,
    PSDestroyCursor,
    PSNativeWindowExists,
    PSSetZoom,
    PSSetWindowBorder,
    PSSetWindowBackground,
    PSSetDither,

    PSDrawReparentWindow,
    PSDrawSetVisible,
    PSDrawMove,
    PSDrawMove,
    PSDrawResize,
    PSDrawMoveResize,
    PSDrawRaise,
    PSDrawRaiseAbove,
    PSDrawIsAbove,
    PSDrawLower,
    PSDrawSetWindowTitles,
    PSDrawSetWindowTitles8,
    PSDrawGetWindowTitle,
    PSDrawGetWindowTitle8,
    PSDrawSetTransientFor,
    PSDrawGetPointerPosition,
    PSDrawGetPointerWindow,
    PSDrawSetCursor,
    PSDrawGetCursor,
    PSDrawGetRedirectWindow,
    PSDrawTranslateCoordinates,

    PSDrawBeep,
    PSDrawFlush,

    PSDrawPushClip,
    PSDrawPopClip,

    PSDrawSetDifferenceMode,

    PSDrawClear,
    PSDrawDrawLine,
    PSDrawDrawArrowLine,
    PSDrawDrawRect,
    PSDrawFillRect,
    PSDrawFillRoundRect,
    PSDrawDrawCircle,
    PSDrawFillCircle,
    PSDrawDrawArc,
    PSDrawDrawPoly,
    PSDrawFillPoly,
    PSDrawScroll,

    _GPSDraw_Image,
    _GPSDraw_TileImage,
    _GPSDraw_Image,
    _GPSDraw_ImageMagnified,
    _PSDraw_CopyScreenToImage,
    _PSDraw_Pixmap,
    _PSDraw_TilePixmap,

    PSDrawCreateInputContext,
    PSDrawSetGIC,
    PSDrawKeyState,

    PSDrawGrabSelection,
    PSDrawAddSelectionType,
    PSDrawRequestSelection,
    PSDrawSelectionHasType,
    PSDrawBindSelection,
    PSDrawSelectionHasOwner,

    PSDrawPointerUngrab,
    PSDrawPointerGrab,
    PSDrawRequestExpose,
    PSDrawForceUpdate,
    PSDrawSync,
    PSDrawSkipMouseMoveEvents,
    PSDrawProcessPendingEvents,
    PSDrawProcessWindowEvents,
    PSDrawProcessPendingEvents,		/* Same as for OneEvent */
    PSDrawEventLoop,
    PSDrawPostEvent,
    PSDrawPostDragEvent,
    PSDrawRequestDeviceEvents,

    PSDrawRequestTimer,
    PSDrawCancelTimer,

    PSDrawSyncThread,

    GPSPrinterStartJob,
    GPSPrinterNextPage,
    GPSPrinterEndJob,

    PSDrawFontMetrics,

    PSDrawHasCairo,

    PSDrawPathStartNew,
    PSDrawPathClose,
    PSDrawPathMoveTo,
    PSDrawPathLineTo,
    PSDrawPathCurveTo,
    PSDrawPathStroke,
    PSDrawPathFill,
    PSDrawPathFillAndStroke,

    NULL,		/* Pango layout */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

void _GPSDraw_DestroyDisplay(GDisplay *gdisp) {
  if (gdisp->fontstate != NULL) { free(gdisp->fontstate); gdisp->fontstate = NULL; }
  free(gdisp);
  return;
}

GDisplay *_GPSDraw_CreateDisplay() {
    GPSDisplay *gdisp;

    gdisp = calloc(1,sizeof(GPSDisplay));
    if ( gdisp==NULL ) {
return( NULL );
    }

    gdisp->funcs = &psfuncs;
    gdisp->res = 600;
    if ( screen_display!=NULL )
	gdisp->scale_screen_by = gdisp->res/screen_display->res;
    if ( gdisp->scale_screen_by==0 ) gdisp->scale_screen_by =1;

    gdisp->scale = 1.;
    gdisp->xwidth = 8.5;
    gdisp->yheight = 11;
    gdisp->lmargin = gdisp->rmargin = gdisp->tmargin = gdisp->bmargin = 1;
    gdisp->use_lpr = true;
    gdisp->do_transparent = true;
    gdisp->num_copies = 1;
    gdisp->linear_thumb_cnt = 1;
    gdisp->fontstate = calloc(1,sizeof(FState));
    gdisp->fontstate->res = gdisp->res;

    gdisp->def_background = COLOR_CREATE(0xff,0xff,0xff);
    gdisp->def_background = COLOR_CREATE(0x00,0x00,0x00);

    (gdisp->funcs->init)((GDisplay *) gdisp);
return( (GDisplay *) gdisp);
}
