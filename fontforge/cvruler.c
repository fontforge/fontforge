/* Copyright (C) 2000-2006 by George Williams */
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

static void SlopeToBuf(char *buf,char *label,double dx, double dy) {
    if ( dx==0 && dy==0 )
	sprintf( buf, _("%sNo Slope"), label );
    else if ( dx==0 )
	sprintf( buf, "%sdy/dx= ∞, %4g°", label, atan2(dy,dx)*180/3.1415926535897932);
    else
	sprintf( buf, "%sdy/dx= %4g, %4g°", label, dy/dx, atan2(dy,dx)*180/3.1415926535897932);
}

static void CurveToBuf(char *buf,CharView *cv,Spline *s, double t) {
    double kappa, emsize;

    kappa = SplineCurvature(s,t);
    if ( kappa==CURVATURE_ERROR )
	strcpy(buf,_("No Curvature"));
    else {
	emsize = cv->sc->parent->ascent + cv->sc->parent->descent;
	if ( kappa==0 )
	    sprintf(buf,_(" Curvature: %g"), kappa*emsize);
	else
	    sprintf(buf,_(" Curvature: %g  Radius: %g"), kappa*emsize, 1.0/kappa );
    }
}

static int RulerText(CharView *cv, unichar_t *ubuf, int line) {
    char buf[80];
    double len;
    double dx, dy;
    Spline *s;
    double t;

    switch ( line ) {
      case 0: {
	real xoff = cv->info.x-cv->p.cx, yoff = cv->info.y-cv->p.cy;
	real len = sqrt(xoff*xoff+yoff*yoff);

	if ( cv->autonomous_ruler_w ) {
	    xoff = last_ruler_offset[0].x;
	    yoff = last_ruler_offset[0].y;
	}

	if ( !cv->autonomous_ruler_w && !cv->p.pressed )
	    /* Give current location accurately */
	    sprintf( buf, "%f,%f", (double) cv->info.x, (double) cv->info.y);
	else
	    sprintf( buf, "%f %.0f° (%f,%f)", (double) len,
		    atan2(yoff,xoff)*180/3.1415926535897932,
		    (double) xoff,(double) yoff);
      break; }
      case 1:
	if ( cv->p.pressed ) {
	    if ( cv->p.sp!=NULL && cv->info_sp!=NULL &&
		    ((cv->p.sp->next!=NULL && cv->p.sp->next->to==cv->info_sp) ||
		     (cv->p.sp->prev!=NULL && cv->p.sp->prev->from==cv->info_sp)) ) {
		if ( cv->p.sp->next!=NULL && cv->p.sp->next->to==cv->info_sp )
		    len = SplineLength(cv->p.sp->next);
		else
		    len = SplineLength(cv->p.sp->prev);
		if ( len>1 )
		    sprintf( buf, "Spline Length=%.1f", len);
		else
		    sprintf( buf, "Spline Length=%g", len);
	    } else
return( false );
	} else if ( cv->dv!=NULL || cv->gridfit!=NULL ) {
	    double scale = scale = (cv->sc->parent->ascent+cv->sc->parent->descent)/(rint(cv->ft_pointsize*cv->ft_dpi/72.0));
	    sprintf( buf, "%.2f,%.2f", (double) (cv->info.x/scale), (double) (cv->info.y/scale));
	} else if ( cv->p.spline!=NULL ) {
	    s = cv->p.spline;
	    t = cv->p.t;
	    sprintf( buf, _("Near (%f,%f)"),
		    (double) (((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d),
		    (double) (((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d) );
	} else if ( cv->p.sp!=NULL ) {
	    sprintf( buf, _("Near (%f,%f)"),(double) cv->p.sp->me.x,(double) cv->p.sp->me.y );
	} else
return( false );
      break;
      case 2:
	if ( cv->p.pressed )
return( false );
	else if ( cv->p.spline!=NULL ) {
	    s = cv->p.spline;
	    t = cv->p.t;
	    dx = (3*s->splines[0].a*t+2*s->splines[0].b)*t+s->splines[0].c;
	    dy = (3*s->splines[1].a*t+2*s->splines[1].b)*t+s->splines[1].c;
	    SlopeToBuf(buf," ",dx,dy);
	} else if ( cv->p.sp!=NULL ) {
	    if ( cv->p.sp->nonextcp )
		strcpy(buf,_("No Next Control Point"));
	    else
		sprintf(buf,_("Next CP: (%f,%f)"), (double) cv->p.sp->nextcp.x, (double) cv->p.sp->nextcp.y);
	} else
return( false );
      break;
      case 3:
	if ( cv->p.pressed )
return( false );
	else if ( cv->p.spline!=NULL ) {
	    CurveToBuf(buf,cv,cv->p.spline,cv->p.t);
	} else if ( cv->p.sp!=NULL && cv->p.sp->next!=NULL ) {
	    s = cv->p.sp->next;
	    dx = s->splines[0].c;
	    dy = s->splines[1].c;
	    SlopeToBuf(buf,_(" Next "),dx,dy);
	} else if ( cv->p.sp!=NULL ) {
	    if ( cv->p.sp->noprevcp )
		strcpy(buf,_("No Previous Control Point"));
	    else
		sprintf(buf,_("Prev CP: (%f,%f)"), (double) cv->p.sp->prevcp.x, (double) cv->p.sp->prevcp.y);
	} else
return( false );
      break;
      case 4:
	if ( cv->p.spline!=NULL )
return( false );
	else if ( cv->p.sp->next!=NULL ) {
	    CurveToBuf(buf,cv,cv->p.sp->next,0);
	} else if ( cv->p.sp->prev!=NULL ) {
	    s = cv->p.sp->prev;
	    dx = (3*s->splines[0].a*1+2*s->splines[0].b)*1+s->splines[0].c;
	    dy = (3*s->splines[1].a*1+2*s->splines[1].b)*1+s->splines[1].c;
	    SlopeToBuf(buf,_(" Prev "),dx,dy);
	} else
return( false );
      break;
      case 5:
	if ( cv->p.sp->next!=NULL ) {
	    if ( cv->p.sp->noprevcp )
		strcpy(buf,_("No Previous Control Point"));
	    else
		sprintf(buf,_("Prev CP: (%f,%f)"), (double) cv->p.sp->prevcp.x, (double) cv->p.sp->prevcp.y);
	} else {
	    CurveToBuf(buf,cv,cv->p.sp->prev,1);
	}
      break;
      case 6:
	if ( cv->p.sp->next!=NULL && cv->p.sp->prev!=NULL ) {
	    s = cv->p.sp->prev;
	    dx = (3*s->splines[0].a*1+2*s->splines[0].b)*1+s->splines[0].c;
	    dy = (3*s->splines[1].a*1+2*s->splines[1].b)*1+s->splines[1].c;
	    SlopeToBuf(buf,_(" Prev "),dx,dy);
	} else
return( false );
      break;
      case 7:
	if ( cv->p.sp->next!=NULL && cv->p.sp->prev!=NULL ) {
	    CurveToBuf(buf,cv,cv->p.sp->prev,1);
	} else
return( false );
      break;
      default:
return( false );
    }
    utf82u_strcpy(ubuf,buf);
return( true );
}

static int ruler_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    unichar_t ubuf[80];
    int line;

    switch ( event->type ) {
      case et_expose:
	GDrawSetFont(gw,cv->rfont);
	/*GDrawFillRect(gw,NULL,0xe0e0c0);*/
	for ( line=0; RulerText(cv,ubuf,line); ++line )
	    GDrawDrawText(gw,2,line*cv->rfh+cv->ras+1,ubuf,-1,NULL,0x000000);
      break;
      case et_mousedown:
	cv->autonomous_ruler_w = false;
	GDrawDestroyWindow(gw);
	cv->ruler_w = NULL;
      break;
    }
return( true );
}
	
static void RulerPlace(CharView *cv, GEvent *event) {
    unichar_t ubuf[80];
    int width, x;
    GRect size;
    GPoint pt;
    int i,h,w;
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
    } else
	GDrawRaise(cv->ruler_w);

    GDrawSetFont(cv->ruler_w,cv->rfont);
    width = h = 0;
    for ( i=0; RulerText(cv,ubuf,i); ++i ) {
	w = GDrawGetTextWidth(cv->ruler_w,ubuf,-1,NULL);
	if ( w>width ) width = w;
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

    if ( !cv->p.pressed && (event->u.mouse.state&ksm_alt) ) {
	if ( cv->ruler_w!=NULL && GDrawIsVisible(cv->ruler_w)) {
	    GDrawDestroyWindow(cv->ruler_w);
	    cv->ruler_w = NULL;
	}
return;
    }
    if ( !cv->p.pressed )
	CVMouseAtSpline(cv,event);
    RulerPlace(cv,event);
    if ( !cv->p.pressed )
	GDrawSetVisible(cv->ruler_w,true);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);		/* The resize needs to happen before the expose */
    if ( !cv->p.pressed && (event->u.mouse.state&ksm_alt) ) /* but a mouse up might sneak in... */
return;
    GDrawRequestExpose(cv->ruler_w,NULL,false);
}

void CVMouseUpRuler(CharView *cv, GEvent *event) {
    if ( cv->ruler_w!=NULL ) {

	last_ruler_offset[1] = last_ruler_offset[0];
	last_ruler_offset[0].x = cv->info.x-cv->p.cx;
	last_ruler_offset[0].y = cv->info.y-cv->p.cy;

	if ( !(event->u.mouse.state & ksm_alt) ) {
	    /*cv->autonomous_ruler_w = true;*/
return;
	}

	GDrawDestroyWindow(cv->ruler_w);
	cv->ruler_w = NULL;
    }
}

/* ************************************************************************** */

static char *PtInfoText(CharView *cv, int lineno, int active, char *buffer, int blen) {
    BasePoint *cp;
    double t;
    Spline *s;
    SplinePoint *sp = cv->p.sp;
    extern char *coord_sep;
    double dx, dy, kappa, kappa2;
    int emsize;

    if ( !cv->p.prevcp && !cv->p.nextcp ) {
	sp = cv->active_sp;
	if ( sp==NULL )
return( NULL );
    }

    if ( active==-1 ) {
	if ( lineno>0 )
return( NULL );
	if ( sp->next==NULL || sp->prev==NULL )
return( NULL );
	kappa = SplineCurvature(sp->next,0);
	kappa2 = SplineCurvature(sp->prev,1);
	emsize = cv->sc->parent->ascent + cv->sc->parent->descent;
	if ( kappa == CURVATURE_ERROR || kappa2 == CURVATURE_ERROR )
	    strncpy(buffer,_("No curvature info"), blen);
	else
	    snprintf( buffer, blen, U_("∆Curvature: %g"), (kappa-kappa2)*emsize );
return( buffer );
    }

    if ( (!cv->p.prevcp && active ) || (!cv->p.nextcp && !active)) {
	cp = &sp->nextcp;
	t = 0;
	s = sp->next;
    } else {
	cp = &sp->prevcp;
	t = 1;
	s = sp->prev;
    }
	
    switch( lineno ) {
      case 0:
	if ( t==0 )
	    strncpy( buffer, _(" Next CP"), blen);
	else
	    strncpy( buffer, _(" Prev CP"), blen);
      break;
      case 1:
	snprintf( buffer, blen, "(%g%s%g)", (double) cp->x, coord_sep, (double) cp->y);
      break;
      case 2:
	snprintf( buffer, blen, "∆ (%g%s%g)", (double) (cp->x-sp->me.x), coord_sep, (double) (cp->y-sp->me.y));
      break;
      case 3:
	dx = cp->x - sp->me.x; dy = cp->y - sp->me.y;
	if ( dx==0 && dy==0 )
	    snprintf( buffer, blen, _("No Slope") );
	else if ( dx==0 )
	    snprintf( buffer, blen, "∆y/∆x= ∞" );
	else
	    snprintf( buffer, blen, "∆y/∆x= %g", dy/dx );
      break;
      case 4:
	dx = cp->x - sp->me.x; dy = cp->y - sp->me.y;
	snprintf( buffer, blen, "� %g°", atan2(dy,dx)*180/3.1415926535897932 );
      break;
      case 5:
	if ( s==NULL )
return( NULL );
	kappa = SplineCurvature(s,t);
	if ( kappa==CURVATURE_ERROR )
return( NULL );
	emsize = cv->sc->parent->ascent + cv->sc->parent->descent;
	/* If we normalize by the em-size, the curvature is often more */
	/*  readable */
	snprintf( buffer, blen, _("Curvature: %g"), kappa*emsize);
      break;
      default:
return( NULL );
    }
return( buffer );
}

static int cpinfo_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    char buf[100];
    int line, which, y;

    switch ( event->type ) {
      case et_expose:
	y = cv->ras+1;
	GDrawSetFont(gw,cv->rfont);
	for ( which = 1; which>=0; --which ) {
	    for ( line=0; PtInfoText(cv,line,which,buf,sizeof(buf))!=NULL; ++line ) {
		GDrawDrawText8(gw,2,y,buf,-1,NULL,0x000000);
		y += cv->rfh+1;
	    }
	    GDrawDrawLine(gw,0,y+2-cv->ras,2000,y+2-cv->ras,0x000000);
	    y += 4;
	}
	if ( PtInfoText(cv,0,-1,buf,sizeof(buf))!=NULL )
	    GDrawDrawText8(gw,2,y,buf,-1,NULL,0x000000);
      break;
    }
return( true );
}
	
static void CpInfoPlace(CharView *cv, GEvent *event) {
    char buf[100];
    int line, which;
    int width, x, y;
    GRect size;
    GPoint pt, pt2;
    int h,w;
    GWindowAttrs wattrs;
    GRect pos;
    FontRequest rq;
    static unichar_t fixed[] = { 'f','i','x','e','d', ',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    int as, ds, ld;
    SplinePoint *sp;

    if ( cv->ruler_w==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_positioned|wam_nodecor|wam_backcol|wam_bordwidth;
	wattrs.event_masks = (1<<et_expose)|(1<<et_resize)|(1<<et_mousedown);
	wattrs.cursor = ct_mypointer;
	wattrs.background_color = 0xe0e0c0;
	wattrs.nodecoration = 1;
	wattrs.border_width = 1;
	pos.x = pos.y = 0; pos.width=pos.height = 20;
	cv->ruler_w = GWidgetCreateTopWindow(NULL,&pos,cpinfo_e_h,cv,&wattrs);

	memset(&rq,0,sizeof(rq));
	rq.family_name = fixed;
	rq.point_size = -12;
	rq.weight = 400;
	cv->rfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(cv->ruler_w),&rq);
	GDrawFontMetrics(cv->rfont,&as,&ds,&ld);
	cv->rfh = as+ds; cv->ras = as;
    } else
	GDrawRaise(cv->ruler_w);

    GDrawSetFont(cv->ruler_w,cv->rfont);
    h = 0; width = 0;
    for ( which = 0; which<2; ++which ) {
	for ( line=0; PtInfoText(cv,line,which,buf,sizeof(buf))!=NULL; ++line ) {
	    w = GDrawGetText8Width(cv->ruler_w,buf,-1,NULL);
	    if ( w>width ) width = w;
	    h += cv->rfh+1;
	}
	h += 4;
    }
    if ( PtInfoText(cv,0,-1,buf,sizeof(buf))!=NULL ) {
	w = GDrawGetText8Width(cv->ruler_w,buf,-1,NULL);
	if ( w>width ) width = w;
	h += cv->rfh+1;
    }
    
    GDrawGetSize(GDrawGetRoot(NULL),&size);
    pt.x = event->u.mouse.x; pt.y = event->u.mouse.y;	/* Address of cp */
    GDrawTranslateCoordinates(cv->v,GDrawGetRoot(NULL),&pt);

    sp = cv->p.sp;
    if ( !cv->p.prevcp && !cv->p.nextcp )
	sp = cv->active_sp;
    if ( sp!=NULL ) {
	x =  cv->xoff + rint(sp->me.x*cv->scale);
	y = -cv->yoff + cv->height - rint(sp->me.y*cv->scale);
	if ( x>=0 && y>=0 && x<cv->width && y<cv->height ) {
	    pt2.x = x; pt2.y = y;
	    GDrawTranslateCoordinates(cv->v,GDrawGetRoot(NULL),&pt2);
	} else
	    sp = NULL;
    }

    x = pt.x + 26;
    y = pt.y - cv->ras-2;
    if ( sp!=NULL && x<=pt2.x-4 && x+width>=pt2.x+4 && y<=pt2.y-4 && y+h>=pt2.y+4 )
	x = pt2.x + 4;
    if ( x+width > size.width ) {
	x = pt.x - width-30;
	if ( sp!=NULL && x<=pt2.x-4 && x+width>=pt2.x+4 && y<=pt2.y-4 && y+h>=pt2.y+4 )
	    x = pt2.x - width - 4;
	if ( x<0 ) {
	    x = pt.x + 10;
	    y = pt.y - h - 26;
	    if ( sp!=NULL && x<=pt2.x-4 && x+width>=pt2.x+4 && y<=pt2.y-4 && y+h>=pt2.y+4 )
		y = pt2.y - h - 4;
	    if ( y<0 )
		y = pt.y+26;	/* If this doesn't work we have nowhere else to */
				/* try so don't check */
	}
    }
    GDrawMoveResize(cv->ruler_w,x,pt.y-cv->ras-2,width+4,h+4);
}

void CPStartInfo(CharView *cv, GEvent *event) {

    if ( !cv->showcpinfo )
return;
    cv->autonomous_ruler_w = false;

    CpInfoPlace(cv,event);
    GDrawSetVisible(cv->ruler_w,true);
}

void CPUpdateInfo(CharView *cv, GEvent *event) {

    if ( !cv->showcpinfo )
return;
    if ( !cv->p.pressed ) {
	if ( cv->ruler_w!=NULL && GDrawIsVisible(cv->ruler_w)) {
	    GDrawDestroyWindow(cv->ruler_w);
	    cv->ruler_w = NULL;
	}
return;
    }
    if ( cv->ruler_w==NULL )
	CPStartInfo(cv,event);
    else {
	CpInfoPlace(cv,event);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);		/* The resize needs to happen before the expose */
	if ( !cv->p.pressed  ) /* but a mouse up might sneak in... */
return;
	GDrawRequestExpose(cv->ruler_w,NULL,false);
    }
}

void CPEndInfo(CharView *cv) {
    if ( cv->ruler_w!=NULL ) {
	GDrawDestroyWindow(cv->ruler_w);
	cv->ruler_w = NULL;
    }
}

#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
