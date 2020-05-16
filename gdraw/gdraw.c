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

#include <fontforge-config.h>

#include "gdraw.h"
#include "gdrawP.h"
#include "gkeysym.h"
#include "ustring.h"

#if __Mac
#  include <sys/select.h>
#endif

/* Functions for font metrics:
    rectangle of text (left side bearing of first char, right of last char)
*/

GDisplay *screen_display = NULL;
void (*_GDraw_BuildCharHook)(GDisplay *) = NULL;
void (*_GDraw_InsCharHook)(GDisplay *,unichar_t) = NULL;

int GDrawPointsToPixels(GWindow gw,int points) {
    if ( gw==NULL ) {
	if ( screen_display==NULL )
return( PointToPixel(points,100));
	gw = screen_display->groot;
    }
return( PointToPixel(points,gw->display->res));
}

int GDrawPixelsToPoints(GWindow gw,int pixels) {
    if ( gw==NULL ) {
	if ( screen_display==NULL )
return( PixelToPoint(pixels,100));
	gw = screen_display->groot;
    }
return( PixelToPoint(pixels,gw->display->res));
}

void GDrawSetDefaultIcon(GWindow icon) {
    (icon->display->funcs->setDefaultIcon)(icon);
}

GWindow GDrawCreateTopWindow(GDisplay *gdisp, GRect *pos,
	int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
    if ( gdisp==NULL ) gdisp = screen_display;
return( (gdisp->funcs->createTopWindow)(gdisp,pos,eh,user_data,wattrs) );
}

GWindow GDrawCreateSubWindow(GWindow w, GRect *pos,
	int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
return( (w->display->funcs->createSubWindow)(w,pos,eh,user_data,wattrs) );
}

GWindow GDrawCreatePixmap(GDisplay *gdisp, GWindow similar, uint16 width, uint16 height) {
    if ( gdisp==NULL ) gdisp = screen_display;
return( (gdisp->funcs->createPixmap)(gdisp,similar,width,height));
}

GWindow GDrawCreateBitmap(GDisplay *gdisp, uint16 width, uint16 height, uint8 *data) {
    if ( gdisp==NULL ) gdisp = screen_display;
return( (gdisp->funcs->createBitmap)(gdisp,width,height,data));
}

GCursor GDrawCreateCursor(GWindow src,GWindow mask,Color fg,Color bg,
	int16 x, int16 y ) {
return( (src->display->funcs->createCursor)(src,mask,fg,bg,x,y));
}

void GDrawDestroyWindow(GWindow w) {
    (w->display->funcs->destroyWindow)(w);
}

void GDrawSetZoom(GWindow w,GRect *zoom, enum gzoom_flags flags) {
    (w->display->funcs->setZoom)(w,zoom,flags);
}

void GDrawDestroyCursor(GDisplay *gdisp, GCursor ct) {
    if ( gdisp==NULL ) gdisp = screen_display;
    (gdisp->funcs->destroyCursor)(gdisp,ct);
}

int GDrawNativeWindowExists(GDisplay *gdisp, void *native) {
    if ( gdisp==NULL ) gdisp = screen_display;
return( (gdisp->funcs->nativeWindowExists)(gdisp,native) );
}

void GDrawSetWindowBackground(GWindow w,Color col) {
    (w->display->funcs->setWindowBackground)(w,col);
}

int GDrawSetDither(GDisplay *gdisp, int dither) {
    if ( gdisp==NULL ) gdisp = screen_display;
return( (gdisp->funcs->setDither)(gdisp,dither) );
}

void GDrawSetVisible(GWindow w, int visible) {
    (w->display->funcs->setVisible)(w,visible);
}

int GDrawIsVisible(GWindow w) {
    if ( w==NULL )
return( false );
    while ( w!=NULL && ( w->is_visible || w->is_pixmap ))
	w = w->parent;
return( w==NULL );
}

void GDrawMove(GWindow w, int32 x, int32 y) {
    (w->display->funcs->move)(w,x,y);
}

void GDrawTrueMove(GWindow w, int32 x, int32 y) {
    (w->display->funcs->trueMove)(w,x,y);
}

void GDrawResize(GWindow w, int32 width, int32 height) {
    (w->display->funcs->resize)(w,width,height);
}

void GDrawMoveResize(GWindow w, int32 x, int32 y, int32 width, int32 height) {
    (w->display->funcs->moveResize)(w,x,y,width,height);
}

GWindow GDrawGetRoot(GDisplay *gdisp) {
    if ( gdisp==NULL ) gdisp = screen_display;
return(gdisp->groot);
}

Color GDrawGetDefaultBackground(GDisplay *gdisp) {
    if ( gdisp==NULL ) gdisp = screen_display;
return(gdisp->def_background);
}

Color GDrawGetDefaultForeground(GDisplay *gdisp) {
    if ( gdisp==NULL ) gdisp = screen_display;
return(gdisp->def_foreground);
}

GRect *GDrawGetSize(GWindow w, GRect *ret) {
    *ret = w->pos;
return(ret);
}

GDrawEH GDrawGetEH(GWindow w) {
return( w->eh );
}

void GDrawSetEH(GWindow w,GDrawEH eh) {
    w->eh = eh;
}

void GDrawGetPointerPosition(GWindow w, GEvent *ret) {
    (w->display->funcs->getPointerPos)(w,ret);
}

GWindow GDrawGetPointerWindow(GWindow w) {
return( (w->display->funcs->getPointerWindow)(w));
}

void GDrawRaise(GWindow w) {
    (w->display->funcs->raise)(w);
}

void GDrawRaiseAbove(GWindow w,GWindow below) {
    (w->display->funcs->raiseAbove)(w,below);
}

int GDrawIsAbove(GWindow w,GWindow other) {
return( (w->display->funcs->isAbove)(w,other) );
}

void GDrawLower(GWindow w) {
    (w->display->funcs->lower)(w);
}

void GDrawSetWindowTitles(GWindow w, const unichar_t *title, const unichar_t *icontit) {
    (w->display->funcs->setWindowTitles)(w,title,icontit);
}

unichar_t *GDrawGetWindowTitle(GWindow w) {
return( (w->display->funcs->getWindowTitle)(w) );
}

void GDrawSetWindowTitles8(GWindow w, const char *title, const char *icontit) {
    (w->display->funcs->setWindowTitles8)(w,title,icontit);
}

char *GDrawGetWindowTitle8(GWindow w) {
return( (w->display->funcs->getWindowTitle8)(w) );
}

void GDrawSetTransientFor(GWindow transient, GWindow owner) {
    (transient->display->funcs->setTransientFor)(transient,owner);
}

void GDrawSetCursor(GWindow w, GCursor ct) {
    (w->display->funcs->setCursor)(w,ct);
}

GCursor GDrawGetCursor(GWindow w) {
return( (w->display->funcs->getCursor)(w) );
}

GWindow GDrawGetRedirectWindow(GDisplay *gd) {
    if ( gd==NULL ) gd = screen_display;
return( (gd->funcs->getRedirectWindow)(gd) );
}

void GDrawTranslateCoordinates(GWindow from,GWindow to, GPoint *pt) {
    GDisplay *gd;
    if ( from!=NULL )
	gd = from->display;
    else if ( to!=NULL )
	gd = to->display;
    else
return;
    (gd->funcs->translateCoordinates)(from,to,pt);
}

int32 GDrawEventInWindow(GWindow inme,GEvent *event) {
    GPoint pt;
    if ( event->type<et_char || event->type>et_crossing )
return( false );
    pt.x = event->u.mouse.x; pt.y = event->u.mouse.y;
    (inme->display->funcs->translateCoordinates)(event->w,inme,&pt);
    if ( pt.x<0 || pt.y<0 || pt.x >= inme->pos.width || pt.y >= inme->pos.height )
return( false );

return( true );
}

GWindow GDrawGetParentWindow(GWindow gw) {
return( gw->parent );
}

int GDrawWindowIsAncestor(GWindow ancester, GWindow descendent) {
    while ( descendent!=NULL && descendent!=ancester )
	descendent = descendent->parent;
return( descendent==ancester );
}

void GDrawSetUserData(GWindow gw, void *ud) {
    gw->user_data = ud;
}

void *GDrawGetUserData(GWindow gw) {
    if( !gw )
	return 0;
return( gw->user_data );
}

void GDrawSetWindowTypeName(GWindow gw, char* name)
{
    gw->window_type_name = name;
}

char* GDrawGetWindowTypeName(GWindow gw)
{
    if(!gw)
	return 0;
    
    return(gw->window_type_name);
}


GDisplay *GDrawGetDisplayOfWindow(GWindow gw) {
    if ( gw==NULL )
return( screen_display );

return( gw->display );
}

void GDrawBeep(GDisplay *gdisp) {
    if ( gdisp==NULL ) gdisp = screen_display;
    (gdisp->funcs->beep)(gdisp);
}

void GDrawGetClip(GWindow w, GRect *ret) {
    *ret = w->ggc->clip;
}

void GDrawSetClip(GWindow w, GRect *rct) {
    if ( rct==NULL ) {
	w->ggc->clip.x = w->ggc->clip.y = 0;
	w->ggc->clip.width = w->ggc->clip.height = 0x7fff;
    } else
	w->ggc->clip = *rct;
}

void GDrawPushClip(GWindow w, GRect *rct, GRect *old) {
    (w->display->funcs->pushClip)(w,rct,old);
}

void GDrawPushClipOnly(GWindow w)
{
    if( w->display->funcs->PushClipOnly )
        (w->display->funcs->PushClipOnly)( w );
}

void GDrawClipPreserve(GWindow w)
{
    if( w->display->funcs->ClipPreserve )
        (w->display->funcs->ClipPreserve)( w );
}


void GDrawPopClip(GWindow w, GRect *old) {
    (w->display->funcs->popClip)(w,old);
}


GGC *GDrawGetWindowGGC(GWindow w) {
return( w->ggc );
}

void GDrawSetDifferenceMode(GWindow w) {
    (w->display->funcs->setDifferenceMode)(w);
}

void GDrawSetDashedLine(GWindow w,int16 dash_len, int16 skip_len, int16 off) {
    w->ggc->dash_offset = off;
    w->ggc->dash_len = dash_len;
    w->ggc->skip_len = skip_len;
}

void GDrawSetStippled(GWindow w,int16 ts, int32 yoff,int32 xoff) {
    w->ggc->ts = ts;
    w->ggc->ts_xoff = xoff; w->ggc->ts_yoff = yoff;
}

void GDrawSetLineWidth(GWindow w,int16 width) {
    w->ggc->line_width = width;
}

int16 GDrawGetLineWidth( GWindow w ) 
{
    return w->ggc->line_width;
}


void GDrawSetForeground(GWindow w,Color col) {
    w->ggc->fg = col;
}

void GDrawSetBackground(GWindow w,Color col) {
    w->ggc->bg = col;
}

void GDrawClear(GWindow w, GRect *rect) {
    (w->display->funcs->clear)(w,rect);
}

void GDrawDrawLine(GWindow w, int32 x,int32 y, int32 xend,int32 yend, Color col) {
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->drawLine)(w,x,y,xend,yend,col);
}

void GDrawDrawArrow(GWindow w, int32 x,int32 y, int32 xend,int32 yend, int arrows, Color col) {
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->drawArrow)(w,x,y,xend,yend,arrows,col);
}

void GDrawDrawRect(GWindow w, GRect *rect, Color col) {
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->drawRect)(w,rect,col);
}

void GDrawFillRect(GWindow w, GRect *rect, Color col) {
    GRect temp;
    if ( rect==NULL ) {
	temp.x = temp.y = 0; temp.width = w->pos.width; temp.height = w->pos.height;
	rect = &temp;
    }
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->fillRect)(w,rect,col);
}

void GDrawFillRoundRect(GWindow w, GRect *rect, int radius, Color col) {
    GRect temp;
    if ( rect==NULL ) {
	temp.x = temp.y = 0; temp.width = w->pos.width; temp.height = w->pos.height;
	rect = &temp;
    }
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->fillRoundRect)(w,rect,radius,col);
}

void GDrawDrawElipse(GWindow w, GRect *rect, Color col) {
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->drawElipse)(w,rect,col);
}

void GDrawFillElipse(GWindow w, GRect *rect, Color col) {
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->fillElipse)(w,rect,col);
}

/* angles expressed as in X, in 64's of a degree with 0 angle at 3 o'clock */
/*  and positive measured counter-clockwise (or the same way as in polar coords)*/
/* tangle is NOT the end angle, it's the angle offset from the first to the end*/
void GDrawDrawArc(GWindow w, GRect *rect, int32 sangle, int32 tangle, Color col) {
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->drawArc)(w,rect,sangle,tangle,col);
}

void GDrawDrawPoly(GWindow w, GPoint *pts, int16 cnt, Color col) {
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->drawPoly)(w,pts,cnt,col);
}

void GDrawFillPoly(GWindow w, GPoint *pts, int16 cnt, Color col) {
    if ( col!=COLOR_UNKNOWN )
	(w->display->funcs->fillPoly)(w,pts,cnt,col);
}

void GDrawScroll(GWindow w, GRect *rect, int32 hor, int32 vert) {
    (w->display->funcs->scroll)(w,rect,hor,vert);
}

/* draws the subset of the image specified by src starting at loc (x,y) */
void GDrawDrawImage(GWindow w, GImage *img, GRect *src, int32 x, int32 y) {
    int width = GImageGetWidth(img);
    int height = GImageGetHeight(img);
    GRect r = src ? *src : (GRect){0, 0, width, height};

    if (x < w->ggc->clip.x) {
        r.x += w->ggc->clip.x - x;
        r.width -= w->ggc->clip.x - x;
        x = w->ggc->clip.x;
    }
    if (y < w->ggc->clip.y) {
        r.y += w->ggc->clip.y - y;
        r.height -= w->ggc->clip.y - y;
        y = w->ggc->clip.y;
    }
    if (r.x < 0) {
        r.width += r.x;
        r.x = 0;
    }
    if (r.y < 0) {
        r.height += r.y;
        r.y = 0;
    }
    if (r.width > width)
        r.width = width;
    if (r.height > height)
        r.height = height;
    if (x + r.width > w->ggc->clip.x + w->ggc->clip.width)
        r.width = w->ggc->clip.x + w->ggc->clip.width - x;
    if (y + r.height > w->ggc->clip.y + w->ggc->clip.height)
        r.height = w->ggc->clip.y + w->ggc->clip.height - y;
    if (r.width <= 0 || r.height <= 0)
        return;

    (w->display->funcs->drawImage)(w,img,&r,x,y);
}

/* Draw the entire image so that it is approximately the same size on other */
/*  displays as on the screen */
void GDrawDrawScaledImage(GWindow w, GImage *img, int32 x, int32 y) {
    GRect r;

    r.x = r.y = 0;
    r.width = GImageGetScaledWidth(w,img);
    r.height = GImageGetScaledHeight(w,img);
    (w->display->funcs->drawImage)(w,img,&r,x,y);
}

/* Similar to DrawImage, but can in some cases make improvements -- if the */
/*  is an indexed image, then treat as the alpha channel rather than a color */
/*  in its own right */
void GDrawDrawGlyph(GWindow w, GImage *img, GRect *src, int32 x, int32 y) {
    GRect r;
    if ( src==NULL ) {
	struct _GImage *base = img->list_len==0?img->u.image:img->u.images[0];
	r.x = r.y = 0;
	r.width = base->width; r.height = base->height;
	src = &r;
    }
    (w->display->funcs->drawGlyph)(w,img,src,x,y);
}

/* same as drawImage except with pixmaps */
void GDrawDrawPixmap(GWindow w, GWindow pixmap, GRect *src, int32 x, int32 y) {
    (w->display->funcs->drawPixmap)(w,pixmap,src,x,y);
}

/* We assume the full image is drawn starting at (x,y) and scaled to (width,height) */
/*  this routine updates the rectangle on the screen			 */
/*		(x+src->x,y+src->y,x+src->width,y+src->height)		 */
/* Ie. if you get an expose event in the middle of the image subtract off the */
/*  image base (x,y) and pass in the exposed rectangle */
void GDrawDrawImageMagnified(GWindow w, GImage *img, GRect *dest, int32 x, int32 y,
	int32 width, int32 height) {
    GRect temp;
    struct _GImage *base = img->list_len==0?img->u.image:img->u.images[0];

    /* Not magnified after all */
    if ( base->width==width && base->height==height ) {
        GDrawDrawImage(w, img, dest, x, y);
        return;
    }
    if ( dest==NULL ) {
	temp.x = temp.y = 0;
	temp.width = width; temp.height = height;
	dest = &temp;
    } else if ( dest->x<0 || dest->y<0 ||
	    dest->x+dest->width > width || dest->y+dest->height > height ) {
	temp = *dest;
	if ( temp.x<0 ) { temp.width += temp.x; temp.x = 0; }
	if ( temp.y<0 ) { temp.height += temp.y; temp.y = 0; }
	if ( temp.x+temp.width>width ) temp.width = width-temp.x;
	if ( temp.y+temp.height>height ) temp.height = height-temp.y;
	dest = &temp;
    }
    (w->display->funcs->drawImageMag)(w,img,dest,x,y, width, height);
}

void GDrawWindowFontMetrics(GWindow w,FontInstance *fi,int *as, int *ds, int *ld) {
    (w->display->funcs->getFontMetrics)(w,fi,as,ds,ld);
}


enum gcairo_flags GDrawHasCairo(GWindow w) {
return( (w->display->funcs->hasCairo)(w));
}

void GDrawPathStartNew(GWindow w) {
    (w->display->funcs->startNewPath)(w);
}

void GDrawPathStartSubNew(GWindow w) {
    (w->display->funcs->startNewSubPath)(w);
}

int GDrawFillRuleSetWinding(GWindow w) {
    return (w->display->funcs->fillRuleSetWinding)(w);
}

void GDrawPathClose(GWindow w) {
    (w->display->funcs->closePath)(w);
}

void GDrawPathMoveTo(GWindow w,double x, double y) {
    (w->display->funcs->moveto)(w,x,y);
}

void GDrawPathLineTo(GWindow w,double x, double y) {
    (w->display->funcs->lineto)(w,x,y);
}

void GDrawPathCurveTo(GWindow w,
		    double cx1, double cy1,
		    double cx2, double cy2,
		    double x, double y) {
    (w->display->funcs->curveto)(w,cx1,cy1,cx2,cy2,x,y);
}

void GDrawPathStroke(GWindow w,Color col) {
    (w->display->funcs->stroke)(w,col);
}

void GDrawPathFill(GWindow w,Color col) {
    (w->display->funcs->fill)(w,col);
}

void GDrawPathFillAndStroke(GWindow w,Color fillcol, Color strokecol) {
    (w->display->funcs->fillAndStroke)(w,fillcol,strokecol);
}

void GDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi) {
    (w->display->funcs->layoutInit)(w,text,cnt,fi);
}

void GDrawLayoutDraw(GWindow w, int32 x, int32 y, Color fg) {
    (w->display->funcs->layoutDraw)(w,x,y,fg);
}

void GDrawLayoutIndexToPos(GWindow w, int index, GRect *pos) {
    (w->display->funcs->layoutIndexToPos)(w,index,pos);
}

int GDrawLayoutXYToIndex(GWindow w, int x, int y) {
return( (w->display->funcs->layoutXYToIndex)(w,x,y) );
}

void GDrawLayoutExtents(GWindow w, GRect *size) {
    (w->display->funcs->layoutExtents)(w,size);
}

void GDrawLayoutSetWidth(GWindow w, int width) {
    (w->display->funcs->layoutSetWidth)(w,width);
}

int  GDrawLayoutLineCount(GWindow w) {
return( (w->display->funcs->layoutLineCount)(w) );
}

int  GDrawLayoutLineStart(GWindow w,int line) {
return( (w->display->funcs->layoutLineStart)(w,line) );
}


GIC *GDrawCreateInputContext(GWindow w,enum gic_style def_style) {
return(w->display->funcs->createInputContext)(w,def_style);
}

void GDrawSetGIC(GWindow w, GIC *gic, int x, int y) {
    (w->display->funcs->setGIC)(w,gic,x,y);
}

int GDrawKeyState(GWindow w, int keysym) {
    return (w->display->funcs->keyState)(w, keysym);
}

void GDrawGrabSelection(GWindow w,enum selnames sel) {
    (w->display->funcs->grabSelection)(w,sel);
}

void GDrawAddSelectionType(GWindow w,enum selnames sel,char *type,
	void *data,int32 cnt,int32 unitsize,void *(*gendata)(void *,int32 *len),
	void (*freedata)(void *)) {
    (w->display->funcs->addSelectionType)(w,sel,type,data,cnt,unitsize,gendata,freedata);
}

void *GDrawRequestSelection(GWindow w,enum selnames sn, char *typename, int32 *len) {
return( (w->display->funcs->requestSelection)(w,sn,typename,len));
}

int GDrawSelectionHasType(GWindow w,enum selnames sn, char *typename) {
return( (w->display->funcs->selectionHasType)(w,sn,typename));
}

void GDrawBindSelection(GDisplay *disp,enum selnames sel, char *atomname) {
    if ( disp==NULL )
	disp = screen_display;
    if (disp != NULL)
        (disp->funcs->bindSelection)(disp,sel,atomname);
}

int GDrawSelectionOwned(GDisplay *disp,enum selnames sel) {
    if ( disp==NULL )
	disp = screen_display;
    if (disp != NULL)
        return( (disp->funcs->selectionHasOwner)(disp,sel));
    else
        return -1;
}

int GDrawEnableExposeRequests(GWindow w,int enabled) {
    int old = w->disable_expose_requests;
    w->disable_expose_requests = enabled;
return( old );
}

void GDrawRequestExpose(GWindow w, GRect *rect, int doclear) {
    if ( !GDrawIsVisible(w) || w->disable_expose_requests )
return;
    (w->display->funcs->requestExpose)(w,rect,doclear);
}

void GDrawForceUpdate(GWindow w) {
    (w->display->funcs->forceUpdate)(w);
}

void GDrawSync(GDisplay *gdisp) {
    if ( gdisp==NULL ) gdisp=screen_display;
    if (gdisp != NULL)
    (gdisp->funcs->sync)(gdisp);
}

void GDrawPointerUngrab(GDisplay *gdisp) {
    if ( gdisp==NULL ) gdisp=screen_display;
    if (gdisp != NULL)
    (gdisp->funcs->pointerUngrab)(gdisp);
}

void GDrawPointerGrab(GWindow w) {
    (w->display->funcs->pointerGrab)(w);
}

void GDrawProcessOneEvent(GDisplay *gdisp) {
    if ( gdisp==NULL ) gdisp=screen_display;
    if (gdisp != NULL)
    (gdisp->funcs->processOneEvent)(gdisp);
}

void GDrawSkipMouseMoveEvents(GWindow w,GEvent *last) {
    (w->display->funcs->skipMouseMoveEvents)(w,last);
}

void GDrawProcessPendingEvents(GDisplay *gdisp) {
    if ( gdisp==NULL ) gdisp=screen_display;
    if (gdisp != NULL)
    (gdisp->funcs->processPendingEvents)(gdisp);
}

void GDrawProcessWindowEvents(GWindow w) {
    (w->display->funcs->processWindowEvents)(w);
}

void GDrawEventLoop(GDisplay *gdisp) {
    if ( gdisp==NULL ) gdisp=screen_display;
    if (gdisp != NULL)
    (gdisp->funcs->eventLoop)(gdisp);
}

void GDrawPostEvent(GEvent *e) {
    GDisplay *gdisp = e->w->display;
    if ( gdisp==NULL ) gdisp=screen_display;
    if (gdisp != NULL)
    (gdisp->funcs->postEvent)(e);
}

void GDrawPostDragEvent(GWindow w,GEvent *mouse,enum event_type et) {
    GDisplay *gdisp = w->display;
    (gdisp->funcs->postDragEvent)(w,mouse,et);
}

GTimer *GDrawRequestTimer(GWindow w,int32 time_from_now,int32 frequency,
	void *userdata) {
return( (w->display->funcs->requestTimer)(w,time_from_now,frequency,userdata));
}

void GDrawCancelTimer(GTimer *timer) {
    GDisplay *gdisp;
    if ( timer==NULL )
return;
    gdisp=timer->owner->display;
    (gdisp->funcs->cancelTimer)(timer);
}

int GDrawRequestDeviceEvents(GWindow w,int devcnt,struct gdeveventmask *de) {
return( (w->display->funcs->requestDeviceEvents)(w,devcnt,de) );
}

void GDrawSetBuildCharHooks(void (*hook)(GDisplay *),void (*inshook)(GDisplay *,unichar_t)) {
    _GDraw_BuildCharHook = hook;
    _GDraw_InsCharHook = inshook;
}

/* We are in compose characters mode. The gdisp->mykey_state argument tells us*/
/*  how many accent keys have been pressed. When we finally get a non-accent */
/*  we try to look through our rules for composing letters given this set of */
/*  accents and this base character. If we find something, great, install it */
/*  and return. If there's nothing then see if we get anywhere by removing */
/*  one of the accents (if so use it, but continue with the remain accent in */
/*  the state). Finally we use the base character followed by all the accents */
/*  left unaccounted for in the mask */
void _GDraw_ComposeChars(GDisplay *gdisp,GEvent *gevent) {
    unichar_t ch = gevent->u.chr.keysym;
    struct gchr_transform *strt = NULL, *trans, *end=NULL;
    extern struct gchr_lookup _gdraw_chrlookup[];
    extern struct gchr_accents _gdraw_accents[];
    extern uint32 _gdraw_chrs_ctlmask, _gdraw_chrs_metamask, _gdraw_chrs_any;
    int i,mask;
    unichar_t hold[_GD_EVT_CHRLEN], *pt, *ept, *hpt;
    uint32 mykey_state = gdisp->mykey_state;

    if ( gevent->u.chr.chars[0]=='\0' )		/* ignore things like the shift key */
return;
    if ( gevent->u.chr.keysym==GK_Escape ) {
	gevent->u.chr.chars[0] = '\0';
	gevent->u.chr.keysym = '\0';
	gdisp->mykeybuild = false;
return;
    }
    if ( gevent->u.chr.state&ksm_control )
	mykey_state |= _gdraw_chrs_ctlmask;
    if ( gevent->u.chr.state&ksm_meta )
	mykey_state |= _gdraw_chrs_metamask;
    if ( ch>' ' && ch<0x7f ) {
	for ( trans = strt = _gdraw_chrlookup[ch-' '].transtab, end=trans+_gdraw_chrlookup[ch-' '].cnt;
		trans<end; ++trans ) {
	    if ( trans->oldstate==mykey_state ) {
		gdisp->mykey_state = trans->newstate;
		if ( trans->resch=='\0' )
		    u_strcpy(gevent->u.chr.chars,gevent->u.chr.chars+1);
		else {
		    gevent->u.chr.chars[0] = trans->resch;
		    gdisp->mykeybuild = false;
		}
return;
	    } else if ( trans->oldstate==_gdraw_chrs_any ) {
		gdisp->mykey_state |= trans->newstate;
		u_strcpy(gevent->u.chr.chars,gevent->u.chr.chars+1);
return;
	    }
	}
    }

    GDrawBeep(gdisp);
    if ( mykey_state==0 || mykey_state==0x8000000 )
return;
    u_strcpy(hold,gevent->u.chr.chars+1);
    if ( strt!=NULL ) for ( mask=0x1; mask<0x8000000; mask<<=1 ) {
	if ( (mykey_state&~mask)== 0 )
    break;			/* otherwise dotabove a gives us ae */
	for ( trans=strt; trans<end; ++trans ) {
	    if ( trans->oldstate==(mykey_state&~mask) && trans->resch!='\0' ) {
		mykey_state = mask;
		gevent->u.chr.chars[0] = trans->resch;
    goto break_2_loops;
	    }
	}
    }
    break_2_loops:;
    pt = gevent->u.chr.chars+1; ept = gevent->u.chr.chars+_GD_EVT_CHRLEN-1;
    for ( i=0; _gdraw_accents[i].accent!=0 && pt<ept; ++i ) {
	if ( (_gdraw_accents[i].mask&mykey_state)==_gdraw_accents[i].mask ) {
	    *pt++ = _gdraw_accents[i].accent;
	    mykey_state &= ~_gdraw_accents[i].mask;
	}
    }
    for ( hpt = hold; pt<ept && *hpt!='\0'; )
	*pt++ = *hpt++;
    *pt = '\0';
    gdisp->mykeybuild = false;
}

void GDrawDestroyDisplays() {
  if (screen_display != NULL) {
#ifndef FONTFORGE_CAN_USE_GDK
    _GXDraw_DestroyDisplay(screen_display);
#else
    _GGDKDraw_DestroyDisplay(screen_display);
#endif
    screen_display = NULL;
  }
}

void GDrawCreateDisplays(char *displayname,char *programname) {
#ifndef FONTFORGE_CAN_USE_GDK
    screen_display = _GXDraw_CreateDisplay(displayname,programname);
#else
    screen_display = _GGDKDraw_CreateDisplay(displayname, programname);
#endif
    if ( screen_display==NULL ) {
	fprintf( stderr, "Could not open screen.\n" );
#if __Mac
	fprintf( stderr, "You must start X11 before you can start %s\n", programname);
	fprintf( stderr, " X11 is optional software found on your install DVD.\n" );
#elif __CygWin
	fprintf( stderr, "You must start X11 before you can start %s\n", programname);
	fprintf( stderr, " X11 may be obtained from the cygwin site in a separate package.\n" );
#endif
exit(1);
    }
}
