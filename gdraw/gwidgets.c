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

#include "gdrawP.h"
#include "ggadget.h"
#include "gwidgetP.h"

/* Temporarily do all drawing in this widget to a pixmap rather than the window */
/*  if events are orderly then we can share one pixmap for all windows */
static GWindow pixmap, cairo_pixmap;
/*  otherwise we create and destroy pixmaps */

GWindow _GWidget_GetPixmap(GWindow gw,GRect *rect) {
#ifndef FONTFORGE_CAN_USE_GDK
    GWindow ours;

    if ( gw->display!=screen_display )
return( gw );
    if ( gw->is_pixmap )
return( gw );
#ifdef UsingPThreads
    this is a critical section if there are multiple pthreads
#endif
    if ( GDrawHasCairo(gw)&gc_alpha ) {
	if ( cairo_pixmap==NULL || cairo_pixmap->pos.width<rect->x+rect->width ||
			     cairo_pixmap->pos.height<rect->y+rect->height ) {
	    if ( cairo_pixmap!=NULL )
		GDrawDestroyWindow(cairo_pixmap);
	    /* The 0x8000 on width is a hack to tell create pixmap to use*/
	    /*  cairo convas */
	    cairo_pixmap = GDrawCreatePixmap(gw->display,gw,0x8000|gw->pos.width,gw->pos.height);
	}
	ours = cairo_pixmap;
	cairo_pixmap = NULL;
    } else {
	if ( pixmap==NULL || pixmap->pos.width<rect->x+rect->width ||
			     pixmap->pos.height<rect->y+rect->height ) {
	    if ( pixmap!=NULL )
		GDrawDestroyWindow(pixmap);
	    pixmap = GDrawCreatePixmap(gw->display,gw,gw->pos.width,gw->pos.height);
	}
	ours = pixmap;
	pixmap = NULL;
    }
#ifdef UsingPThreads
    End critical section
#endif
    if ( ours==NULL )
	ours = gw;
    else {
	GWidgetD *gd = (GWidgetD *) (gw->widget_data);
	ours->widget_data = gd;
	gd->w = ours;
	GDrawFillRect(ours,rect,gw->ggc->bg);
    }
return( ours );
#else
    GDrawFillRect(gw, rect, gw->ggc->bg);
    return gw;
#endif /* FONTFORGE_CAN_USE_GDK */
}

void _GWidget_RestorePixmap(GWindow gw, GWindow ours, GRect *rect) {
#ifndef FONTFORGE_CAN_USE_GDK
    GWidgetD *gd = (GWidgetD *) (gw->widget_data);

    if ( gw==ours )
return;				/* it wasn't a pixmap, all drawing was to real window */
    GDrawDrawPixmap(gw, ours, rect, rect->x, rect->y);
    
#ifdef UsingPThreads
    this is a critical section if there are multiple pthreads
#endif
    if ( GDrawHasCairo(gw)&gc_alpha ) {
	if ( cairo_pixmap!=NULL )
	    GDrawDestroyWindow(ours);
	else {
	    cairo_pixmap = ours;
	    ours->widget_data = NULL;
	}
    } else {
	if ( pixmap!=NULL )
	    GDrawDestroyWindow(ours);
	else {
	    pixmap = ours;
	    ours->widget_data = NULL;
	}
    }
    gd->w = gw;
#ifdef UsingPThreads
    End critical section
#endif
#endif /* FONTFORGE_CAN_USE_GDK */
}
