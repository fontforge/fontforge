/* Copyright (C) 2000-2004 by George Williams */
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
#include "pfaeditui.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <math.h>
#include <ustring.h>

BasePoint last_ruler_offset[2] = { {0,0}, {0,0} };

static void RulerText(CharView *cv, unichar_t *ubuf) {
    char buf[60];
    real xoff = cv->info.x-cv->p.cx, yoff = cv->info.y-cv->p.cy;

    if ( cv->autonomous_ruler_w ) {
	xoff = last_ruler_offset[0].x;
	yoff = last_ruler_offset[0].y;
    }

    if ( !cv->autonomous_ruler_w && !cv->p.pressed )
	/* Give current location accurately */
	sprintf( buf, "%f,%f", cv->info.x, cv->info.y);
    else			/* Give displacement from press point */
	sprintf( buf, "%.1f %.0f\260 (%.0f,%.0f)", sqrt(xoff*xoff+yoff*yoff),
		atan2(yoff,xoff)*180/3.1415926535897932,
		xoff,yoff);
    uc_strcpy(ubuf,buf);
}

static int RulerText2(CharView *cv, unichar_t *ubuf) {
    char buf[60];
    double len;

    if ( !cv->p.pressed )
return( false );
    if ( cv->p.sp!=NULL && cv->info_sp!=NULL &&
	    ((cv->p.sp->next!=NULL && cv->p.sp->next->to==cv->info_sp) ||
	     (cv->p.sp->prev!=NULL && cv->p.sp->prev->from==cv->info_sp)) ) {
	 if ( cv->p.sp->next!=NULL && cv->p.sp->next->to==cv->info_sp )
	     len = SplineLength(cv->p.sp->next);
	 else
	     len = SplineLength(cv->p.sp->prev);
	sprintf( buf, "Spline Length=%.1f", len);
	uc_strcpy(ubuf,buf);
return( true );
    }
return( false );
}

static int ruler_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    unichar_t ubuf[60];

    switch ( event->type ) {
      case et_expose:
	GDrawSetFont(gw,cv->rfont);
	RulerText(cv,ubuf);
	/*GDrawFillRect(gw,NULL,0xe0e0c0);*/
	GDrawDrawText(gw,2,cv->ras+1,ubuf,-1,NULL,0x000000);
	if ( RulerText2(cv,ubuf))
	    GDrawDrawText(gw,2,cv->rfh+cv->ras+2,ubuf,-1,NULL,0x000000);
      break;
      case et_mousedown:
	cv->autonomous_ruler_w = false;
      break;
    }
return( true );
}
	
static void RulerPlace(CharView *cv, GEvent *event) {
    unichar_t ubuf[60];
    int width, x;
    GRect size;
    GPoint pt;
    int h,w;
    GWindowAttrs wattrs;
    GRect pos;
    FontRequest rq;
    static unichar_t fixed[] = { 'f','i','x','e','d', ',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    int as, ds, ld;

    if ( cv->ruler_w==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_positioned|wam_nodecor|wam_backcol|wam_bordwidth;
	wattrs.event_masks = (1<<et_expose)|(1<<et_resize)|(1<<et_mousedown);
	wattrs.cursor = ct_mypointer;
	wattrs.background_color = 0xe0e0c0;
	wattrs.nodecoration = 1;
	wattrs.border_width = 1;
	pos.x = pos.y = 0; pos.width=pos.height = 20;
	cv->ruler_w = GWidgetCreateTopWindow(NULL,&pos,ruler_e_h,cv,&wattrs);

	memset(&rq,0,sizeof(rq));
	rq.family_name = fixed;
	rq.point_size = -12;
	rq.weight = 400;
	cv->rfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(cv->ruler_w),&rq);
	GDrawFontMetrics(cv->rfont,&as,&ds,&ld);
	cv->rfh = as+ds; cv->ras = as;
    }

    GDrawSetFont(cv->ruler_w,cv->rfont);
    RulerText(cv,ubuf);
    width = GDrawGetTextWidth(cv->ruler_w,ubuf,-1,NULL);
    h = cv->rfh;
    if ( RulerText2(cv,ubuf)) {
	w = GDrawGetTextWidth(cv->ruler_w,ubuf,-1,NULL);
	if ( width<w ) width = w;
	h += cv->rfh;
    }
    GDrawGetSize(GDrawGetRoot(NULL),&size);
    pt.x = event->u.mouse.x; pt.y = event->u.mouse.y;
    GDrawTranslateCoordinates(cv->v,GDrawGetRoot(NULL),&pt);
    x = pt.x + 26;
    if ( x+width > size.width )
	x = pt.x - width-30;
    GDrawMoveResize(cv->ruler_w,x,pt.y-cv->ras-2,width+4,h+4);
}

void CVMouseDownRuler(CharView *cv, GEvent *event) {

    cv->autonomous_ruler_w = false;

    RulerPlace(cv,event);
    GDrawSetVisible(cv->ruler_w,true);
}

void CVMouseMoveRuler(CharView *cv, GEvent *event) {
    if ( cv->autonomous_ruler_w )
return;

    if ( !cv->p.pressed && !(event->u.mouse.state&ksm_meta) ) {
	if ( cv->ruler_w!=NULL && GDrawIsVisible(cv->ruler_w)) {
	    GDrawDestroyWindow(cv->ruler_w);
	    cv->ruler_w = NULL;
	}
return;
    }
    RulerPlace(cv,event);
    if ( !cv->p.pressed )
	GDrawSetVisible(cv->ruler_w,true);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);		/* The resize needs to happen before the expose */
    if ( !cv->p.pressed && !(event->u.mouse.state&ksm_meta) ) /* but a mouse up might sneak in... */
return;
    GDrawRequestExpose(cv->ruler_w,NULL,false);
}

void CVMouseUpRuler(CharView *cv, GEvent *event) {
    if ( cv->ruler_w!=NULL ) {

	last_ruler_offset[1] = last_ruler_offset[0];
	last_ruler_offset[0].x = cv->info.x-cv->p.cx;
	last_ruler_offset[0].y = cv->info.y-cv->p.cy;

	if ( event->u.mouse.state & ksm_meta ) {
	    cv->autonomous_ruler_w = true;
return;
	}

	GDrawDestroyWindow(cv->ruler_w);
	cv->ruler_w = NULL;
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
