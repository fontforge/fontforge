/* Copyright (C) 2000-2007 by George Williams */
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
/* #define TEST_O2 */

#include "pfaeditui.h"
#include <math.h>
#include <locale.h>
#ifndef FONTFORGE_CONFIG_GTK
# include <ustring.h>
# include <utype.h>
# include <gresource.h>
# ifdef FONTFORGE_CONFIG_GDRAW
extern int _GScrollBar_Width;
#  include <gkeysym.h>
# endif
#endif
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
extern struct lconv localeinfo;
extern char *coord_sep;
struct cvshows CVShows = {
	1,		/* show foreground */
	1,		/* show background */
	1,		/* show grid plane */
	1,		/* show horizontal hints */
	1,		/* show vertical hints */
	1,		/* show diagonal hints */
	1,		/* show points */
	0,		/* show filled */
	1,		/* show rulers */
	1,		/* show points which are to be rounded to the ttf grid and aren't on hints */
	1,		/* show x minimum distances */
	1,		/* show y minimum distances */
	1,		/* show horizontal metrics */
	0,		/* show vertical metrics */
	0,		/* mark extrema */
	0,		/* show points of inflection */
	1,		/* show blue values */
	1,		/* show family blues too */
	1,		/* show anchor points */
	1,		/* show control point info when moving them */
	1		/* show tabs containing names of former glyphs */
};
static Color pointcol = 0xff0000;
static Color firstpointcol = 0x707000;
static Color selectedpointcol = 0xc8c800;
static int selectedpointwidth = 2;
static Color extremepointcol = 0xc00080;
static Color pointofinflectioncol = 0x008080;
Color nextcpcol = 0x007090;
Color prevcpcol = 0xcc00cc;
static Color selectedcpcol = 0xffffff;
static Color coordcol = 0x808080;
Color widthcol = 0x000000;
static Color widthselcol = 0x00ff00;
static Color widthgridfitcol = 0x009800;
static Color lcaretcol = 0x909040;
static Color rastercol = 0xa0a0a0;
static Color rasternewcol = 0x909090;
static Color rasteroldcol = 0xc0c0c0;
static Color rastergridcol = 0xb0b0ff;
static Color rasterdarkcol = 0x606060;
static Color italiccoordcol = 0x909090;
static Color metricslabelcol = 0x00000;
static Color hintlabelcol = 0x00ffff;
static Color bluevalstipplecol = 0x8080ff;
static Color fambluestipplecol = 0xff7070;
static Color mdhintcol = 0xe04040;
static Color dhintcol = 0xd0a0a0;
static Color hhintcol = 0xa0d0a0;
static Color vhintcol = 0xc0c0ff;
static Color hflexhintcol = 0x00ff00;
static Color vflexhintcol = 0x00ff00;
static Color conflicthintcol = 0x00ffff;
static Color hhintactivecol = 0x00a000;
static Color vhintactivecol = 0x0000ff;
static Color anchorcol = 0x0040ff;
static Color templateoutlinecol = 0x009800;
static Color oldoutlinecol = 0x008000;
static Color transformorigincol = 0x000000;
static Color guideoutlinecol = 0x808080;
static Color gridfitoutlinecol = 0x009800;
static Color backoutlinecol = 0x009800;
static Color foreoutlinecol = 0x000000;
static Color backimagecol = 0x707070;
static Color fillcol = 0x707070;
static Color tracecol = 0x008000;

static int cvcolsinited = false;

static void CVColInit( void ) {
    GResStruct cvcolors[] = {
	{ "PointColor", rt_color, &pointcol },
	{ "FirstPointColor", rt_color, &firstpointcol },
	{ "SelectedPointColor", rt_color, &selectedpointcol },
	{ "SelectedPointWidth", rt_int, &selectedpointwidth },
	{ "ExtremePointColor", rt_color, &extremepointcol },
	{ "PointOfInflectionColor", rt_color, &pointofinflectioncol },
	{ "NextCPColor", rt_color, &nextcpcol },
	{ "PrevCPColor", rt_color, &prevcpcol },
	{ "SelectedCPColor", rt_color, &selectedcpcol },
	{ "CoordinateLineColor", rt_color, &coordcol },
	{ "WidthColor", rt_color, &widthcol },
	{ "WidthSelColor", rt_color, &widthselcol },
	{ "GridFitWidthColor", rt_color, &widthgridfitcol },
	{ "LigatureCaretColor", rt_color, &lcaretcol },
	{ "RasterColor", rt_color, &rastercol },
	{ "RasterNewColor", rt_color, &rasternewcol },
	{ "RasterOldColor", rt_color, &rasteroldcol },
	{ "RasterGridColor", rt_color, &rastergridcol },
	{ "RasterDarkColor", rt_color, &rasterdarkcol },
	{ "ItalicCoordColor", rt_color, &italiccoordcol },
	{ "MetricsLabelColor", rt_color, &metricslabelcol },
	{ "HintLabelColor", rt_color, &hintlabelcol },
	{ "BlueValuesStippledColor", rt_color, &bluevalstipplecol },
	{ "FamilyBlueStippledColor", rt_color, &fambluestipplecol },
	{ "MDHintColor", rt_color, &mdhintcol },
	{ "DHintColor", rt_color, &dhintcol },
	{ "HHintColor", rt_color, &hhintcol },
	{ "VHintColor", rt_color, &vhintcol },
	{ "HFlexHintColor", rt_color, &hflexhintcol },
	{ "VFlexHintColor", rt_color, &vflexhintcol },
	{ "ConflictHintColor", rt_color, &conflicthintcol },
	{ "HHintActiveColor", rt_color, &hhintactivecol },
	{ "VHintActiveColor", rt_color, &vhintactivecol },
	{ "AnchorColor", rt_color, &anchorcol },
	{ "TemplateOutlineColor", rt_color, &templateoutlinecol },
	{ "OldOutlineColor", rt_color, &oldoutlinecol },
	{ "TransformOriginColor", rt_color, &transformorigincol },
	{ "GuideOutlineColor", rt_color, &guideoutlinecol },
	{ "GridFitOutlineColor", rt_color, &gridfitoutlinecol },
	{ "BackgroundOutlineColor", rt_color, &backoutlinecol },
	{ "ForegroundOutlineColor", rt_color, &foreoutlinecol },
	{ "BackgroundImageColor", rt_color, &backimagecol },
	{ "FillColor", rt_color, &fillcol },
	{ "TraceColor", rt_color, &tracecol },
	{ NULL }
    };
    GResourceFind( cvcolors, "CharView.");
}


GDevEventMask input_em[] = {
	/* Event masks for wacom devices */
    /* negative utility in opening Mouse1 */
    /* No point in distinguishing cursor from core mouse */
    { (1<<et_mousemove)|(1<<et_mousedown)|(1<<et_mouseup)|(1<<et_char), "stylus" },
    { (1<<et_mousemove)|(1<<et_mousedown)|(1<<et_mouseup), "eraser" },
    { 0, NULL }
};
const int input_em_cnt = sizeof(input_em)/sizeof(input_em[0])-1;

/* Positions on the info line */
#define RPT_BASE	5		/* Place to draw the pointer icon */
#define RPT_DATA	13		/* x,y text after above */
#define SPT_BASE	73		/* Place to draw selected pt icon */
#define SPT_DATA	87		/* Any text for it */
#define SOF_BASE	147		/* Place to draw selection to pointer icon */
#define SOF_DATA	169		/* Any text for it */
#define SDS_BASE	229		/* Place to draw distance icon */
#define SDS_DATA	251		/* Any text for it */
#define SAN_BASE	281		/* Place to draw angle icon */
#define SAN_DATA	303		/* Any text for it */
#define MAG_BASE	333		/* Place to draw magnification icon */
#define MAG_DATA	344		/* Any text for it */
#define LAYER_DATA	404		/* Text to show the current layer */
#define CODERANGE_DATA	474		/* Text to show the current code range (if the debugger be active) */

void CVDrawRubberRect(GWindow pixmap, CharView *cv) {
    GRect r;
    if ( !cv->p.rubberbanding )
return;
    r.x =  cv->xoff + rint(cv->p.cx*cv->scale);
    r.y = -cv->yoff + cv->height - rint(cv->p.cy*cv->scale);
    r.width = rint( (cv->p.ex-cv->p.cx)*cv->scale );
    r.height = -rint( (cv->p.ey-cv->p.cy)*cv->scale );
    if ( r.width<0 ) {
	r.x += r.width;
	r.width = -r.width;
    }
    if ( r.height<0 ) {
	r.y += r.height;
	r.height = -r.height;
    }
    GDrawSetDashedLine(pixmap,2,2,0);
    GDrawSetLineWidth(pixmap,0);
    GDrawSetXORMode(pixmap);
    GDrawSetXORBase(pixmap,GDrawGetDefaultBackground(NULL));
    GDrawDrawRect(pixmap,&r,0x000000);
    GDrawSetCopyMode(pixmap);
    GDrawSetDashedLine(pixmap,0,0,0);
}

static void CVDrawRubberLine(GWindow pixmap, CharView *cv) {
    int x,y, xend,yend;
    if ( !cv->p.rubberlining )
return;
    x =  cv->xoff + rint(cv->p.cx*cv->scale);
    y = -cv->yoff + cv->height - rint(cv->p.cy*cv->scale);
    xend =  cv->xoff + rint(cv->info.x*cv->scale);
    yend = -cv->yoff + cv->height - rint(cv->info.y*cv->scale);
    GDrawSetXORMode(pixmap);
    GDrawSetLineWidth(pixmap,0);
    GDrawSetXORBase(pixmap,GDrawGetDefaultBackground(NULL));
    GDrawDrawLine(pixmap,x,y,xend,yend,0x000000);
    GDrawSetCopyMode(pixmap);
}

static void CVDrawBB(CharView *cv, GWindow pixmap, DBounds *bb) {
    GRect r;
    int off = cv->xoff+cv->height-cv->yoff;

    r.x =  cv->xoff + rint(bb->minx*cv->scale);
    r.y = -cv->yoff + cv->height - rint(bb->maxy*cv->scale);
    r.width = rint((bb->maxx-bb->minx)*cv->scale);
    r.height = rint((bb->maxy-bb->miny)*cv->scale);
    GDrawSetDashedLine(pixmap,1,1,off);
    GDrawDrawRect(pixmap,&r,GDrawGetDefaultForeground(NULL));
    GDrawSetDashedLine(pixmap,0,0,0);
}

/* Sigh. I have to do my own clipping because at large magnifications */
/*  things can easily exceed 16 bits */
static int CVSplineOutside(CharView *cv, Spline *spline) {
    int x[4], y[4];

    x[0] =  cv->xoff + rint(spline->from->me.x*cv->scale);
    y[0] = -cv->yoff + cv->height - rint(spline->from->me.y*cv->scale);

    x[1] =  cv->xoff + rint(spline->to->me.x*cv->scale);
    y[1] = -cv->yoff + cv->height - rint(spline->to->me.y*cv->scale);

    if ( spline->from->nonextcp && spline->to->noprevcp ) {
	if ( (x[0]<0 && x[1]<0) || (x[0]>=cv->width && x[1]>=cv->width) ||
		(y[0]<0 && y[1]<0) || (y[0]>=cv->height && y[1]>=cv->height) )
return( true );
    } else {
	x[2] =  cv->xoff + rint(spline->from->nextcp.x*cv->scale);
	y[2] = -cv->yoff + cv->height - rint(spline->from->nextcp.y*cv->scale);
	x[3] =  cv->xoff + rint(spline->to->prevcp.x*cv->scale);
	y[3] = -cv->yoff + cv->height - rint(spline->to->prevcp.y*cv->scale);
	if ( (x[0]<0 && x[1]<0 && x[2]<0 && x[3]<0) ||
		(x[0]>=cv->width && x[1]>=cv->width && x[2]>=cv->width && x[3]>=cv->width ) ||
		(y[0]<0 && y[1]<0 && y[2]<0 && y[3]<0 ) ||
		(y[0]>=cv->height && y[1]>=cv->height && y[2]>=cv->height && y[3]>=cv->height) )
return( true );
    }

return( false );
}

static int CVLinesIntersectScreen(CharView *cv, LinearApprox *lap) {
    LineList *l;
    int any = false;
    int x,y;
    int bothout;

    for ( l=lap->lines; l!=NULL; l=l->next ) {
	l->asend.x = l->asstart.x = cv->xoff + l->here.x;
	l->asend.y = l->asstart.y = -cv->yoff + cv->height-l->here.y;
	l->flags = 0;
	if ( l->asend.x<0 || l->asend.x>=cv->width || l->asend.y<0 || l->asend.y>=cv->height ) {
	    l->flags = cvli_clipped;
	    any = true;
	}
    }
    if ( !any ) {
	for ( l=lap->lines; l!=NULL; l=l->next )
	    l->flags = cvli_onscreen;
	lap->any = true;
return( true );
    }

    any = false;
    for ( l=lap->lines; l->next!=NULL; l=l->next ) {
	if ( !(l->flags&cvli_clipped) && !(l->next->flags&cvli_clipped) )
	    l->flags = cvli_onscreen;
	else {
	    bothout = (l->flags&cvli_clipped) && (l->next->flags&cvli_clipped);
	    if (( l->asstart.x<0 && l->next->asend.x>0 ) ||
		    ( l->asstart.x>0 && l->next->asend.x<0 )) {
		y = -(l->next->asend.y-l->asstart.y)*(double)l->asstart.x/(l->next->asend.x-l->asstart.x) +
			l->asstart.y;
		if ( l->asstart.x<0 ) {
		    l->asstart.x = 0;
		    l->asstart.y = y;
		} else {
		    l->next->asend.x = 0;
		    l->next->asend.y = y;
		}
	    } else if ( l->asstart.x<0 && l->next->asend.x<0 )
    continue;
	    if (( l->asstart.x<cv->width && l->next->asend.x>cv->width ) ||
		    ( l->asstart.x>cv->width && l->next->asend.x<cv->width )) {
		y = (l->next->asend.y-l->asstart.y)*(double)(cv->width-l->asstart.x)/(l->next->asend.x-l->asstart.x) +
			l->asstart.y;
		if ( l->asstart.x>cv->width ) {
		    l->asstart.x = cv->width;
		    l->asstart.y = y;
		} else {
		    l->next->asend.x = cv->width;
		    l->next->asend.y = y;
		}
	    } else if ( l->asstart.x>cv->width && l->next->asend.x>cv->width )
    continue;
	    if (( l->asstart.y<0 && l->next->asend.y>0 ) ||
		    ( l->asstart.y>0 && l->next->asend.y<0 )) {
		x = -(l->next->asend.x-l->asstart.x)*(double)l->asstart.y/(l->next->asend.y-l->asstart.y) +
			l->asstart.x;
		if (( x<0 || x>=cv->width ) && bothout )
    continue;			/* Not on screen */;
		if ( l->asstart.y<0 ) {
		    l->asstart.y = 0;
		    l->asstart.x = x;
		} else {
		    l->next->asend.y = 0;
		    l->next->asend.x = x;
		}
	    } else if ( l->asstart.y<0 && l->next->asend.y< 0 )
    continue;
	    if (( l->asstart.y<cv->height && l->next->asend.y>cv->height ) ||
		    ( l->asstart.y>cv->height && l->next->asend.y<cv->height )) {
		x = (l->next->asend.x-l->asstart.x)*(double)(cv->height-l->asstart.y)/(l->next->asend.y-l->asstart.y) +
			l->asstart.x;
		if (( x<0 || x>=cv->width ) && bothout )
    continue;			/* Not on screen */;
		if ( l->asstart.y>cv->height ) {
		    l->asstart.y = cv->height;
		    l->asstart.x = x;
		} else {
		    l->next->asend.y = cv->height;
		    l->next->asend.x = x;
		}
	    } else if ( l->asstart.y>cv->height && l->next->asend.y>cv->height )
    continue;
	    l->flags |= cvli_onscreen;
	    any = true;
	}
    }
    lap->any = any;
return( any );
}

typedef struct gpl { struct gpl *next; GPoint *gp; int cnt; } GPointList;

static void GPLFree(GPointList *gpl) {
    GPointList *next;

    while ( gpl!=NULL ) {
	next = gpl->next;
	free( gpl->gp );
	free( gpl );
	gpl = next;
    }
}

/* Before we did clipping this was a single polygon. Now it is a set of */
/*  sets of line segments. If no clipping is done, then we end up with */
/*  one set which is the original polygon, otherwise we get the segments */
/*  which are inside the screen. Each set of segments is contiguous */
static GPointList *MakePoly(CharView *cv, SplinePointList *spl) {
    int i, len;
    LinearApprox *lap;
    LineList *line, *prev;
    Spline *spline, *first;
    GPointList *head=NULL, *last=NULL, *cur;
    int closed;

    for ( i=0; i<2; ++i ) {
	len = 0; first = NULL;
	closed = true;
	cur = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( !CVSplineOutside(cv,spline) && !isnan(spline->splines[0].a) && !isnan(spline->splines[1].a)) {
		lap = SplineApproximate(spline,cv->scale);
		if ( i==0 )
		    CVLinesIntersectScreen(cv,lap);
		if ( lap->any ) {
		    for ( prev = lap->lines, line=prev->next; line!=NULL; prev=line, line=line->next ) {
			if ( !(prev->flags&cvli_onscreen) ) {
			    closed = true;
		    continue;
			}
			if ( closed || (prev->flags&cvli_clipped) ) {
			    if ( i==0 ) {
				cur = gcalloc(1,sizeof(GPointList));
				if ( head==NULL )
				    head = cur;
				else {
				    last->cnt = len;
				    last->next = cur;
				}
				last = cur;
			    } else {
				if ( cur==NULL )
				    cur = head;
				else
				    cur = cur->next;
				cur->gp = galloc(cur->cnt*sizeof(GPoint));
				cur->gp[0].x = prev->asstart.x;
				cur->gp[0].y = prev->asstart.y;
			    }
			    len=1;
			    closed = false;
			}
			if ( i!=0 ) {
			    if ( len>=cur->cnt )
				fprintf( stderr, "Clipping is screwed up, about to die %d (should be less than %d)\n", len, cur->cnt );
			    cur->gp[len].x = line->asend.x;
			    cur->gp[len].y = line->asend.y;
			}
			++len;
			if ( line->flags&cvli_clipped )
			    closed = true;
		    }
		} else
		    closed = true;
	    } else
		closed = true;
	    if ( first==NULL ) first = spline;
	}
	if ( i==0 && cur!=NULL )
	    cur->cnt = len;
    }
return( head );
}

static void DrawPoint(CharView *cv, GWindow pixmap, SplinePoint *sp,
	SplineSet *spl, int onlynumber) {
    GRect r;
    int x, y, cx, cy;
    Color col = sp==spl->first ? firstpointcol : pointcol, subcol;
    int pnum;
    char buf[12]; unichar_t ubuf[12];
    int isfake;

    if ( cv->markextrema && !sp->nonextcp && !sp->noprevcp &&
	    ((sp->nextcp.x==sp->me.x && sp->prevcp.x==sp->me.x) ||
	     (sp->nextcp.y==sp->me.y && sp->prevcp.y==sp->me.y)) )
	 col = extremepointcol;
    if ( sp->selected )
	 col = selectedpointcol;

    x =  cv->xoff + rint(sp->me.x*cv->scale);
    y = -cv->yoff + cv->height - rint(sp->me.y*cv->scale);
    if ( x<-4000 || y<-4000 || x>cv->width+4000 || y>=cv->height+4000 )
return;

    /* draw the control points if it's selected */
    if ( sp->selected || cv->showpointnumbers || cv->show_ft_results || cv->dv ) {
	int iscurrent = sp==(cv->p.sp!=NULL?cv->p.sp:cv->lastselpt);
	if ( !sp->nonextcp ) {
	    cx =  cv->xoff + rint(sp->nextcp.x*cv->scale);
	    cy = -cv->yoff + cv->height - rint(sp->nextcp.y*cv->scale);
	    if ( cx<-100 ) {		/* Clip */
		cy = cx==x ? x : (cy-y) * (double)(-100-x)/(cx-x) + y;
		cx = -100;
	    } else if ( cx>cv->width+100 ) {
		cy = cx==x ? x : (cy-y) * (double)(cv->width+100-x)/(cx-x) + y;
		cx = cv->width+100;
	    }
	    if ( cy<-100 ) {
		cx = cy==y ? y : (cx-x) * (double)(-100-y)/(cy-y) + x;
		cy = -100;
	    } else if ( cy>cv->height+100 ) {
		cx = cy==y ? y : (cx-x) * (double)(cv->height+100-y)/(cy-y) + x;
		cy = cv->height+100;
	    }
	    subcol = nextcpcol;
	    if ( iscurrent && cv->p.nextcp && !onlynumber ) {
		r.x = cx-3; r.y = cy-3; r.width = r.height = 7;
		GDrawFillRect(pixmap,&r, nextcpcol);
		subcol = selectedcpcol;
	    }
	    if ( !onlynumber ) {
		GDrawDrawLine(pixmap,x,y,cx,cy, nextcpcol);
		GDrawDrawLine(pixmap,cx-3,cy-3,cx+3,cy+3,subcol);
		GDrawDrawLine(pixmap,cx+3,cy-3,cx-3,cy+3,subcol);
	    }
	    if ( cv->showpointnumbers || cv->show_ft_results || cv->dv ) {
		pnum = sp->nextcpindex;
		if ( pnum!=0xffff && pnum!=0xfffe ) {
		    sprintf( buf,"%d", pnum );
		    uc_strcpy(ubuf,buf);
		    GDrawDrawText(pixmap,cx,cy-6,ubuf,-1,NULL,nextcpcol);
		}
	    }
	}
	if ( !sp->noprevcp ) {
	    cx =  cv->xoff + rint(sp->prevcp.x*cv->scale);
	    cy = -cv->yoff + cv->height - rint(sp->prevcp.y*cv->scale);
	    if ( cx<-100 ) {		/* Clip */
		cy = cx==x ? x : (cy-y) * (double)(-100-x)/(cx-x) + y;
		cx = -100;
	    } else if ( cx>cv->width+100 ) {
		cy = cx==x ? x : (cy-y) * (double)(cv->width+100-x)/(cx-x) + y;
		cx = cv->width+100;
	    }
	    if ( cy<-100 ) {
		cx = cy==y ? y : (cx-x) * (double)(-100-y)/(cy-y) + x;
		cy = -100;
	    } else if ( cy>cv->height+100 ) {
		cx = cy==y ? y : (cx-x) * (double)(cv->height+100-y)/(cy-y) + x;
		cy = cv->height+100;
	    }
	    subcol = prevcpcol;
	    if ( iscurrent && cv->p.prevcp && !onlynumber ) {
		r.x = cx-3; r.y = cy-3; r.width = r.height = 7;
		GDrawFillRect(pixmap,&r, prevcpcol);
		subcol = selectedcpcol;
	    }
	    if ( !onlynumber ) {
		GDrawDrawLine(pixmap,x,y,cx,cy, prevcpcol);
		GDrawDrawLine(pixmap,cx-3,cy-3,cx+3,cy+3,subcol);
		GDrawDrawLine(pixmap,cx+3,cy-3,cx-3,cy+3,subcol);
	    }
	}
    }

    if ( x<-4 || y<-4 || x>cv->width+4 || y>=cv->height+4 )
return;
    r.x = x-2;
    r.y = y-2;
    r.width = r.height = 5;
    if ( sp->selected )
	GDrawSetLineWidth(pixmap,selectedpointwidth);
    isfake = false;
    if ( cv->fv->sf->order2 && cv->drawmode==dm_fore &&
	    cv->sc->layers[ly_fore].refs==NULL ) {
	int mightbe_fake = SPInterpolate(sp);
        if ( !mightbe_fake && sp->ttfindex==0xffff )
	    sp->ttfindex = 0xfffe;	/* if we have no instructions we won't call instrcheck and won't notice when a point stops being fake */
	else if ( mightbe_fake )
	    sp->ttfindex = 0xffff;
	isfake = sp->ttfindex==0xffff;
    }
    if ( onlynumber )
	/* Draw Nothing */;
    else if ( sp->pointtype==pt_curve ) {
	--r.x; --r.y; r.width +=2; r.height += 2;
	if ( sp->selected || isfake )
	    GDrawDrawElipse(pixmap,&r,col);
	else
	    GDrawFillElipse(pixmap,&r,col);
    } else if ( sp->pointtype==pt_corner ) {
	if ( sp->selected || isfake )
	    GDrawDrawRect(pixmap,&r,col);
	else
	    GDrawFillRect(pixmap,&r,col);
    } else {
	GPoint gp[5];
	int dir;
	BasePoint *cp=NULL;
	if ( !sp->nonextcp )
	    cp = &sp->nextcp;
	else if ( !sp->noprevcp )
	    cp = &sp->prevcp;
	dir = 0;
	if ( cp!=NULL ) {
	    float dx = cp->x-sp->me.x, dy = cp->y-sp->me.y;
	    if ( dx<0 ) dx= -dx;
	    if ( dy<0 ) dy= -dy;
	    if ( dx>2*dy ) {
		if ( cp->x>sp->me.x ) dir = 0 /* right */;
		else dir = 1 /* left */;
	    } else if ( dy>2*dx ) {
		if ( cp->y>sp->me.y ) dir = 2 /* up */;
		else dir = 3 /* down */;
	    } else {
		if ( cp->y>sp->me.y && cp->x>sp->me.x ) dir=4;
		else if ( cp->x>sp->me.x ) dir=5;
		else if ( cp->y>sp->me.y ) dir=7;
		else dir = 6;
	    }
	}
	if ( dir==1 /* left */ || dir==0 /* right */) {
	    gp[0].y = y; gp[0].x = (dir==0)?x+4:x-4;
	    gp[1].y = y-4; gp[1].x = x;
	    gp[2].y = y+4; gp[2].x = x;
	} else if ( dir==2 /* up */ || dir==3 /* down */ ) {
	    gp[0].x = x; gp[0].y = dir==2?y-4:y+4;	/* remember screen coordinates are backwards in y from character coords */
	    gp[1].x = x-4; gp[1].y = y;
	    gp[2].x = x+4; gp[2].y = y;
	} else {
	    /* at a 45 angle, a value of 4 looks too small. I probably want 4*1.414 */
	    int xdiff= cp->x>sp->me.x?5:-5, ydiff = cp->y>sp->me.y?-5:5;
	    gp[0].x = x+xdiff/2; gp[0].y = y+ydiff/2;
	    gp[1].x = gp[0].x-xdiff; gp[1].y = gp[0].y;
	    gp[2].x = gp[0].x; gp[2].y = gp[0].y-ydiff;
	}
	gp[3] = gp[0];
	if ( sp->selected || isfake )
	    GDrawDrawPoly(pixmap,gp,4,col);
	else
	    GDrawFillPoly(pixmap,gp,4,col);
    }
    GDrawSetLineWidth(pixmap,0);
    if ( (cv->showpointnumbers || cv->show_ft_results|| cv->dv ) && sp->ttfindex!=0xffff ) {
	if ( sp->ttfindex==0xfffe )
	    strcpy(buf,"?");
	else
	    sprintf( buf,"%d", sp->ttfindex );
	GDrawDrawText8(pixmap,x,y-6,buf,-1,NULL,col);
    }
    if ( !onlynumber ) {
	if ((( sp->roundx || sp->roundy ) &&
		 (((cv->showrounds&1) && cv->scale>=.3) || (cv->showrounds&2))) ||
		(sp->watched && cv->dv!=NULL) ||
		sp->hintmask!=NULL ) {
	    r.x = x-5; r.y = y-5;
	    r.width = r.height = 11;
	    GDrawDrawElipse(pixmap,&r,col);
	}
	if (( sp->flexx && cv->showhhints ) || (sp->flexy && cv->showvhints)) {
	    r.x = x-5; r.y = y-5;
	    r.width = r.height = 11;
	    GDrawDrawElipse(pixmap,&r,sp->flexx ? hflexhintcol : vflexhintcol );
	}
    }
}

static void DrawLine(CharView *cv, GWindow pixmap,
	real x1, real y1, real x2, real y2, Color fg) {
    int ix1 = cv->xoff + rint(x1*cv->scale);
    int iy1 = -cv->yoff + cv->height - rint(y1*cv->scale);
    int ix2 = cv->xoff + rint(x2*cv->scale);
    int iy2 = -cv->yoff + cv->height - rint(y2*cv->scale);
    if ( iy1==iy2 ) {
	if ( iy1<0 || iy1>cv->height )
return;
	if ( ix1<0 ) ix1 = 0;
	if ( ix2>cv->width ) ix2 = cv->width;
    } else if ( ix1==ix2 ) {
	if ( ix1<0 || ix1>cv->width )
return;
	if ( iy1<0 ) iy1 = 0;
	if ( iy2<0 ) iy2 = 0;
	if ( iy1>cv->height ) iy1 = cv->height;
	if ( iy2>cv->height ) iy2 = cv->height;
    }
    GDrawDrawLine(pixmap, ix1,iy1, ix2,iy2, fg );
}

static void DrawDirection(CharView *cv,GWindow pixmap, SplinePoint *sp) {
    BasePoint dir, *other;
    double len;
    int x,y,xe,ye;
    SplinePoint *test;

    if ( sp->next==NULL )
return;

    x = cv->xoff + rint(sp->me.x*cv->scale);
    y = -cv->yoff + cv->height - rint(sp->me.y*cv->scale);
    if ( x<0 || y<0 || x>cv->width || y>cv->width )
return;

    /* Werner complained when the first point and the second point were at */
    /*  the same location... */ /* Damn. They weren't at the same location */
    /*  the were off by a rounding error. I'm not going to fix for that */
    for ( test=sp; ; ) {
	if ( test->me.x!=sp->me.x || test->me.y!=sp->me.y ) {
	    other = &test->me;
    break;
	} else if ( !test->nonextcp ) {
	    other = &test->nextcp;
    break;
	}
	if ( test->next==NULL )
return;
	test = test->next->to;
	if ( test==sp )
return;
    }

    dir.x = other->x-sp->me.x;
    dir.y = sp->me.y-other->y;		/* screen coordinates are the mirror of user coords */
    len = sqrt(dir.x*dir.x + dir.y*dir.y);
    dir.x /= len; dir.y /= len;

    x += rint(5*dir.y);
    y -= rint(5*dir.x);
    xe = x + rint(7*dir.x);
    ye = y + rint(7*dir.y);
    GDrawDrawLine(pixmap,x,y,xe,ye,firstpointcol);
    GDrawDrawLine(pixmap,xe,ye,xe+rint(2*(dir.y-dir.x)),ye+rint(2*(-dir.y-dir.x)), firstpointcol);
    GDrawDrawLine(pixmap,xe,ye,xe+rint(2*(-dir.y-dir.x)),ye+rint(2*(dir.x-dir.y)), firstpointcol);
}

static void CVMarkInterestingLocations(CharView *cv, GWindow pixmap,
	SplinePointList *spl) {
    Spline *s, *first;
    extended interesting[6];
    int i, ecnt, cnt;
    GRect r;

    for ( s=spl->first->next, first=NULL; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	cnt = ecnt = 0;
	if ( cv->markextrema )
	    ecnt = cnt = Spline2DFindExtrema(s,interesting);

	if ( cv->markpoi ) {
	    cnt += Spline2DFindPointsOfInflection(s,interesting+cnt);
	}
	r.width = r.height = 9;
	for ( i=0; i<cnt; ++i ) if ( interesting[i]>0 && interesting[i]<1.0 ) {
	    Color col = i<ecnt ? extremepointcol : pointofinflectioncol;
	    double x = ((s->splines[0].a*interesting[i]+s->splines[0].b)*interesting[i]+s->splines[0].c)*interesting[i]+s->splines[0].d;
	    double y = ((s->splines[1].a*interesting[i]+s->splines[1].b)*interesting[i]+s->splines[1].c)*interesting[i]+s->splines[1].d;
	    double sx =  cv->xoff + rint(x*cv->scale);
	    double sy = -cv->yoff + cv->height - rint(y*cv->scale);
	    if ( sx<-5 || sy<-5 || sx>10000 || sy>10000 )
	continue;
	    GDrawDrawLine(pixmap,sx-4,sy,sx+4,sy, col);
	    GDrawDrawLine(pixmap,sx,sy-4,sx,sy+4, col);
	    r.x = sx-4; r.y = sy-4;
	    GDrawDrawElipse(pixmap,&r,col);
	}
    }
}

void CVDrawSplineSet(CharView *cv, GWindow pixmap, SplinePointList *set,
	Color fg, int dopoints, DRect *clip ) {
    Spline *spline, *first;
    SplinePointList *spl;

    if ( cv->inactive )
	dopoints = false;

    GDrawSetFont(pixmap,cv->small);		/* For point numbers */
    for ( spl = set; spl!=NULL; spl = spl->next ) {
	GPointList *gpl = MakePoly(cv,spl), *cur;
	if ( dopoints>0 || (dopoints==-1 && cv->showpointnumbers) ) {
	    first = NULL;
	    if ( dopoints>0 )
		DrawDirection(cv,pixmap,spl->first);
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		DrawPoint(cv,pixmap,spline->from,spl,dopoints<0);
		if ( first==NULL ) first = spline;
	    }
	    if ( spline==NULL )
		DrawPoint(cv,pixmap,spl->last,spl,dopoints<0);
	}
	for ( cur=gpl; cur!=NULL; cur=cur->next )
	    GDrawDrawPoly(pixmap,cur->gp,cur->cnt,fg);
	GPLFree(gpl);
	if (( cv->markextrema || cv->markpoi ) && dopoints )
	    CVMarkInterestingLocations(cv,pixmap,spl);
    }
}

static void CVDrawLayerSplineSet(CharView *cv, GWindow pixmap, Layer *layer,
	Color fg, int dopoints, DRect *clip ) {
#ifdef FONTFORGE_CONFIG_TYPE3
    int active = cv->layerheads[cv->drawmode]==layer;

    if ( layer->dostroke ) {
	if ( layer->stroke_pen.brush.col!=COLOR_INHERITED &&
		layer->stroke_pen.brush.col!=GDrawGetDefaultBackground(NULL))
	    fg = layer->stroke_pen.brush.col;
#if 0
	if ( layer->stroke_pen.width!=WIDTH_INHERITED )
	    GDrawSetLineWidth(pixmap,rint(layer->stroke_pen.width*layer->stroke_pen.trans[0]*cv->scale));
#endif
    }
    if ( layer->dofill ) {
	if ( layer->fill_brush.col!=COLOR_INHERITED &&
		layer->fill_brush.col!=GDrawGetDefaultBackground(NULL))
	    fg = layer->fill_brush.col;
    }
    if ( !active && layer!=&cv->sc->layers[ly_back] )
	GDrawSetDashedLine(pixmap,5,5,cv->xoff+cv->height-cv->yoff);
    CVDrawSplineSet(cv,pixmap,layer->splines,fg,dopoints && active,clip);
    if ( !active && layer!=&cv->sc->layers[ly_back] )
	GDrawSetDashedLine(pixmap,0,0,0);
#if 0
    if ( layer->dostroke && layer->stroke_pen.width!=WIDTH_INHERITED )
	GDrawSetLineWidth(pixmap,0);
#endif
#else
    CVDrawSplineSet(cv,pixmap,layer->splines,fg,dopoints,clip);
#endif
}

static void CVDrawTemplates(CharView *cv,GWindow pixmap,SplineChar *template,DRect *clip) {
    RefChar *r;

    CVDrawSplineSet(cv,pixmap,template->layers[ly_fore].splines,templateoutlinecol,false,clip);
    for ( r=template->layers[ly_fore].refs; r!=NULL; r=r->next )
	CVDrawSplineSet(cv,pixmap,r->layers[0].splines,templateoutlinecol,false,clip);
}

static void CVShowDHint(CharView *cv, GWindow pixmap, DStemInfo *dstem) {
    IPoint ip[40], ip2[40];
    GPoint clipped[13];
    int i,j, tot,last;

    ip[0].x = cv->xoff + rint(dstem->leftedgetop.x*cv->scale);
    ip[0].y = -cv->yoff + cv->height - rint(dstem->leftedgetop.y*cv->scale);
    ip[1].x = cv->xoff + rint(dstem->rightedgetop.x*cv->scale);
    ip[1].y = -cv->yoff + cv->height - rint(dstem->rightedgetop.y*cv->scale);
    ip[2].x = cv->xoff + rint(dstem->rightedgebottom.x*cv->scale);
    ip[2].y = -cv->yoff + cv->height - rint(dstem->rightedgebottom.y*cv->scale);
    ip[3].x = cv->xoff + rint(dstem->leftedgebottom.x*cv->scale);
    ip[3].y = -cv->yoff + cv->height - rint(dstem->leftedgebottom.y*cv->scale);

    if (( ip[0].x<0 && ip[1].x<0 && ip[2].x<0 && ip[3].x<0 ) ||
	    ( ip[0].x>=cv->width && ip[1].x>=cv->width && ip[2].x>=cv->width && ip[3].x>=cv->width ) ||
	    ( ip[0].y<0 && ip[1].y<0 && ip[2].y<0 && ip[3].y<0 ) ||
	    ( ip[0].y>=cv->height && ip[1].y>=cv->height && ip[2].y>=cv->height && ip[3].y>=cv->height ))
return;		/* Offscreen */

    /* clip to left edge */
    tot = 4;
    for ( i=j=0; i<tot; ++i ) {
	last = i==0?tot-1:i-1;
	if ( ip[i].x>=0 && ip[last].x>=0) {
	    ip2[j++] = ip[i];
	} else if ( ip[i].x<0 && ip[last].x<0 ) {
	    if ( j==0 || ip2[j-1].x!=0 || ip2[j-1].y!=ip[i].y ) {
		ip2[j].x = 0;
		ip2[j++].y = ip[i].y;
	    }
	} else {
	    ip2[j].x = 0;
	    ip2[j++].y = ip[last].y - ip[last].x * ((real) (ip[i].y-ip[last].y))/(ip[i].x-ip[last].x);
	    if ( ip[i].x>0 )
		ip2[j++] = ip[i];
	    else {
		ip2[j].x = 0;
		ip2[j++].y = ip[i].y;
	    }
	}
    }
    /* clip to right edge */
    tot = j;
    for ( i=j=0; i<tot; ++i ) {
	last = i==0?tot-1:i-1;
	if ( ip2[i].x<cv->width && ip2[last].x<cv->width ) {
	    ip[j++] = ip2[i];
	} else if ( ip2[i].x>=cv->width && ip2[last].x>=cv->width ) {
	    if ( j==0 || ip[j-1].x!=cv->width-1 || ip[j-1].y!=ip2[i].y ) {
		ip[j].x = cv->width-1;
		ip[j++].y = ip2[i].y;
	    }
	} else {
	    ip[j].x = cv->width-1;
	    ip[j++].y = ip2[last].y + (cv->width-1- ip2[last].x) * ((real) (ip2[i].y-ip2[last].y))/(ip2[i].x-ip2[last].x);
	    if ( ip2[i].x<cv->width )
		ip[j++] = ip2[i];
	    else {
		ip[j].x = cv->width-1;
		ip[j++].y = ip2[i].y;
	    }
	}
    }
    /* clip to bottom edge */
    tot = j;
    for ( i=j=0; i<tot; ++i ) {
	last = i==0?tot-1:i-1;
	if ( ip[i].y>=0 && ip[last].y>=0) {
	    ip2[j++] = ip[i];
	} else if ( ip[i].y<0 && ip[last].y<0 ) {
	    ip2[j].y = 0;
	    ip2[j++].x = ip[i].x;
	} else {
	    ip2[j].y = 0;
	    ip2[j++].x = ip[last].x - ip[last].y * ((real) (ip[i].x-ip[last].x))/(ip[i].y-ip[last].y);
	    if ( ip[i].y>0 )
		ip2[j++] = ip[i];
	    else {
		ip2[j].y = 0;
		ip2[j++].x = ip[i].x;
	    }
	}
    }
    /* clip to top edge */
    tot = j;
    for ( i=j=0; i<tot; ++i ) {
	last = i==0?tot-1:i-1;
	if ( ip2[i].y<cv->height && ip2[last].y<cv->height ) {
	    ip[j++] = ip2[i];
	} else if ( ip2[i].y>=cv->height && ip2[last].y>=cv->height ) {
	    ip[j].y = cv->height-1;
	    ip[j++].x = ip2[i].x;
	} else {
	    ip[j].y = cv->height-1;
	    ip[j++].x = ip2[last].x + (cv->height-1- ip2[last].y) * ((real) (ip2[i].x-ip2[last].x))/(ip2[i].y-ip2[last].y);
	    if ( ip2[i].y<cv->width )
		ip[j++] = ip2[i];
	    else {
		ip[j].y = cv->height-1;
		ip[j++].x = ip2[i].x;
	    }
	}
    }

    tot=j;
    clipped[0].x = ip[0].x; clipped[0].y = ip[0].y;
    for ( i=j=1; i<tot; ++i ) {
	if ( ip[i].x!=ip[i-1].x || ip[i].y!=ip[i-1].y ) {
	    clipped[j].x = ip[i].x; clipped[j++].y = ip[i].y;
	}
    }
    clipped[j++] = clipped[0];
    GDrawFillPoly(pixmap,clipped,j,dhintcol);
}

static void CVShowMinimumDistance(CharView *cv, GWindow pixmap,MinimumDistance *md) {
    int x1,y1, x2,y2;
    int xa, ya;
    int off = cv->xoff+cv->height-cv->yoff;

    if (( md->x && !cv->showmdx ) || (!md->x && !cv->showmdy))
return;
    if ( md->sp1==NULL && md->sp2==NULL )
return;
    if ( md->sp1!=NULL ) {
	x1 = cv->xoff + rint( md->sp1->me.x*cv->scale );
	y1 = -cv->yoff + cv->height - rint(md->sp1->me.y*cv->scale);
    } else {
	x1 = cv->xoff + rint( cv->sc->width*cv->scale );
	y1 = 0x80000000;
    }
    if ( md->sp2!=NULL ) {
	x2 = cv->xoff + rint( md->sp2->me.x*cv->scale );
	y2 = -cv->yoff + cv->height - rint(md->sp2->me.y*cv->scale);
    } else {
	x2 = cv->xoff + rint( cv->sc->width*cv->scale );
	y2 = y1-8;
    }
    if ( y1==0x80000000 )
	y1 = y2-8;
    if ( md->x ) {
	ya = (y1+y2)/2;
	GDrawDrawArrow(pixmap, x1,ya, x2,ya, 2, mdhintcol);
	GDrawSetDashedLine(pixmap,5,5,off);
	GDrawDrawLine(pixmap, x1,ya, x1,y1, mdhintcol);
	GDrawDrawLine(pixmap, x2,ya, x2,y2, mdhintcol);
    } else {
	xa = (x1+x2)/2;
	GDrawDrawArrow(pixmap, xa,y1, xa,y2, 2, mdhintcol);
	GDrawSetDashedLine(pixmap,5,5,off);
	GDrawDrawLine(pixmap, xa,y1, x1,y1, mdhintcol);
	GDrawDrawLine(pixmap, xa,y2, x2,y2, mdhintcol);
    }
    GDrawSetDashedLine(pixmap,0,0,0);
}

static void dtos(char *buf,real val) {
    char *pt;

    sprintf( buf,"%.1f", (double) val);
    pt = buf+strlen(buf);
    if ( pt[-1]=='0' && pt[-2]=='.' ) pt[-2] = '\0';
}

static void CVDrawBlues(CharView *cv,GWindow pixmap,char *bluevals,char *others,
	Color col) {
    double blues[24];
    char *pt, *end;
    int i=0, bcnt=0;
    GRect r;
    char buf[20];
    int len,len2;

    if ( bluevals!=NULL ) {
	for ( pt = bluevals; isspace( *pt ) || *pt=='['; ++pt);
	while ( i<14 && *pt!='\0' && *pt!=']' ) {
	    blues[i] = strtod(pt,&end);
	    if ( pt==end )
	break;
	    ++i;
	    pt = end;
	    while ( isspace( *pt )) ++pt;
	}
	if ( i&1 ) --i;
    }
    if ( others!=NULL ) {
	for ( pt = others; isspace( *pt ) || *pt=='['; ++pt);
	while ( i<24 && *pt!='\0' && *pt!=']' ) {
	    blues[i] = strtod(pt,&end);
	    if ( pt==end )
	break;
	    ++i;
	    pt = end;
	    while ( isspace( *pt )) ++pt;
	}
	if ( i&1 ) --i;
    }
    bcnt = i;
    if ( i==0 )
return;

    r.x = 0; r.width = cv->width;
    for ( i=0; i<bcnt; i += 2 ) {
	int first, other;
	first = -cv->yoff + cv->height - rint(blues[i]*cv->scale);
	other = -cv->yoff + cv->height - rint(blues[i+1]*cv->scale);
	r.y = first;
	if ( ( r.y<0 && other<0 ) || (r.y>cv->height && other>cv->height))
    continue;
	if ( r.y<0 ) r.y = 0;
	else if ( r.y>cv->height ) r.y = cv->height;
	if ( other<0 ) other = 0;
	else if ( other>cv->height ) other = cv->height;
	if ( other<r.y ) {
	    r.height = r.y-other;
	    r.y = other;
	} else
	    r.height = other-r.y;
	if ( r.height==0 ) r.height = 1;	/* show something */
	GDrawSetStippled(pixmap,2, 0,0);
	GDrawFillRect(pixmap,&r,col);
	GDrawSetStippled(pixmap,0, 0,0);

	if ( first>-20 && first<cv->height+20 ) {
	    dtos( buf, blues[i]);
	    len = GDrawGetText8Width(pixmap,buf,-1,NULL);
	    GDrawDrawText8(pixmap,cv->width-len-5,first-3,buf,-1,NULL,hintlabelcol);
	} else
	    len = 0;
	if ( other>-20 && other<cv->height+20 ) {
	    dtos( buf, blues[i+1]-blues[i]);
	    len2 = GDrawGetText8Width(pixmap,buf,-1,NULL);
	    GDrawDrawText8(pixmap,cv->width-len-5-len2-5,other+cv->sas-3,buf,-1,NULL,hintlabelcol);
	}
    }
}

static void CVShowHints(CharView *cv, GWindow pixmap) {
    StemInfo *hint;
    GRect r;
    HintInstance *hi;
    int end;
    Color col;
    DStemInfo *dstem;
    MinimumDistance *md;
    char *blues, *others;
    struct psdict *private = cv->sc->parent->private;
    char buf[20];
    int len, len2;
    SplinePoint *sp;
    SplineSet *spl;

    GDrawSetFont(pixmap,cv->small);
    blues = PSDictHasEntry(private,"BlueValues"); others = PSDictHasEntry(private,"OtherBlues");
    if ( cv->showblues && (blues!=NULL || others!=NULL))
	CVDrawBlues(cv,pixmap,blues,others,bluevalstipplecol);
    blues = PSDictHasEntry(private,"FamilyBlues"); others = PSDictHasEntry(private,"FamilyOtherBlues");
    if ( cv->showfamilyblues && (blues!=NULL || others!=NULL))
	CVDrawBlues(cv,pixmap,blues,others,fambluestipplecol);

    if ( cv->showdhints ) for ( dstem = cv->sc->dstem; dstem!=NULL; dstem = dstem->next ) {
	CVShowDHint(cv,pixmap,dstem);
    }

    if ( cv->showhhints && cv->sc->hstem!=NULL ) {
	GDrawSetDashedLine(pixmap,5,5,cv->xoff);
	for ( hint = cv->sc->hstem; hint!=NULL; hint = hint->next ) {
	    if ( hint->width<0 ) {
		r.y = -cv->yoff + cv->height - rint(hint->start*cv->scale);
		r.height = rint(-hint->width*cv->scale)+1;
	    } else {
		r.y = -cv->yoff + cv->height - rint((hint->start+hint->width)*cv->scale);
		r.height = rint(hint->width*cv->scale)+1;
	    }
	    col = hint->active ? hhintactivecol : hhintcol;
	    /* XRectangles are shorts! */
	    if ( r.y<32767 && r.y+r.height>-32768 ) {
		if ( r.y<-32768 ) {
		    r.height -= (-32768-r.y);
		    r.y = -32768;
		}
		if ( r.y+r.height>32767 )
		    r.height = 32767-r.y;
		for ( hi=hint->where; hi!=NULL; hi=hi->next ) {
		    r.x = cv->xoff + rint(hi->begin*cv->scale);
		    end = cv->xoff + rint(hi->end*cv->scale);
		    if ( end>=0 && r.x<=cv->width ) {
			r.width = end-r.x+1;
			GDrawFillRect(pixmap,&r,col);
		    }
		}
	    }
	    col = (!hint->active && hint->hasconflicts) ? conflicthintcol : col;
	    if ( r.y>=0 && r.y<=cv->height )
		GDrawDrawLine(pixmap,0,r.y,cv->width,r.y,col);
	    if ( r.y+r.height>=0 && r.y+r.height<=cv->width )
		GDrawDrawLine(pixmap,0,r.y+r.height-1,cv->width,r.y+r.height-1,col);

	    r.y = -cv->yoff + cv->height - rint(hint->start*cv->scale);
	    r.y += ( hint->width>0 ) ? -3 : cv->sas+3;
	    if ( r.y>-20 && r.y<cv->height+20 ) {
		dtos( buf, hint->start);
		len = GDrawGetText8Width(pixmap,buf,-1,NULL);
		GDrawDrawText8(pixmap,cv->width-len-5,r.y,buf,-1,NULL,hintlabelcol);
	    } else
		len = 0;
	    r.y = -cv->yoff + cv->height - rint((hint->start+hint->width)*cv->scale);
	    r.y += ( hint->width>0 ) ? cv->sas+3 : -3;
	    if ( r.y>-20 && r.y<cv->height+20 ) {
		if ( hint->ghost ) {
		    buf[0] = 'G';
		    buf[1] = ' ';
		    dtos(buf+2, hint->width);
		} else
		    dtos( buf, hint->width);
		len2 = GDrawGetText8Width(pixmap,buf,-1,NULL);
		GDrawDrawText8(pixmap,cv->width-len-5-len2-5,r.y,buf,-1,NULL,hintlabelcol);
	    }
	}
    }
    if ( cv->showvhints && cv->sc->vstem!=NULL ) {
	GDrawSetDashedLine(pixmap,5,5,cv->height-cv->yoff);
	for ( hint = cv->sc->vstem; hint!=NULL; hint = hint->next ) {
	    if ( hint->width<0 ) {
		r.x = cv->xoff + rint( (hint->start+hint->width)*cv->scale );
		r.width = rint(-hint->width*cv->scale)+1;
	    } else {
		r.x = cv->xoff + rint(hint->start*cv->scale);
		r.width = rint(hint->width*cv->scale)+1;
	    }
	    col = hint->active ? vhintactivecol : vhintcol;
	    if ( r.x<32767 && r.x+r.width>-32768 ) {
		if ( r.x<-32768 ) {
		    r.width -= (-32768-r.x);
		    r.x = -32768;
		}
		if ( r.x+r.width>32767 )
		    r.width = 32767-r.x;
		for ( hi=hint->where; hi!=NULL; hi=hi->next ) {
		    r.y = -cv->yoff + cv->height - rint(hi->end*cv->scale);
		    end = -cv->yoff + cv->height - rint(hi->begin*cv->scale);
		    if ( end>=0 && r.y<=cv->height ) {
			r.height = end-r.y+1;
			GDrawFillRect(pixmap,&r,col);
		    }
		}
	    }
	    col = (!hint->active && hint->hasconflicts) ? conflicthintcol : col;
	    if ( r.x>=0 && r.x<=cv->width )
		GDrawDrawLine(pixmap,r.x,0,r.x,cv->height,col);
	    if ( r.x+r.width>=0 && r.x+r.width<=cv->width )
		GDrawDrawLine(pixmap,r.x+r.width-1,0,r.x+r.width-1,cv->height,col);

	    r.x = cv->xoff + rint(hint->start*cv->scale);
	    if ( r.x>-60 && r.x<cv->width+20 ) {
		dtos( buf, hint->start);
		len = GDrawGetText8Width(pixmap,buf,-1,NULL);
		r.x += ( hint->width>0 ) ? 3 : -len-3;
		GDrawDrawText8(pixmap,r.x,cv->sas+3,buf,-1,NULL,hintlabelcol);
	    }
	    r.x = cv->xoff + rint((hint->start+hint->width)*cv->scale);
	    if ( r.x>-60 && r.x<cv->width+20 ) {
		if ( hint->ghost ) {
		    buf[0] = 'G';
		    buf[1] = ' ';
		    dtos(buf+2, hint->width);
		} else
		    dtos( buf, hint->width);
		len = GDrawGetText8Width(pixmap,buf,-1,NULL);
		r.x += ( hint->width>0 ) ? -len-3 : 3;
		GDrawDrawText8(pixmap,r.x,cv->sas+cv->sfh+3,buf,-1,NULL,hintlabelcol);
	    }
	}
    }
    GDrawSetDashedLine(pixmap,0,0,0);

    for ( md=cv->sc->md; md!=NULL; md=md->next )
	CVShowMinimumDistance(cv, pixmap,md);

    if ( cv->showvhints || cv->showhhints ) {
	for ( spl=cv->sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	    if ( spl->first->prev!=NULL ) for ( sp=spl->first ; ; ) {
		if ( cv->showhhints && sp->flexx ) {
		    double x,y,end;
		    x = cv->xoff + rint(sp->me.x*cv->scale);
		    y = -cv->yoff + cv->height - rint(sp->me.y*cv->scale);
		    end = cv->xoff + rint(sp->next->to->me.x*cv->scale);
		    if ( x>-4096 && x<32767 && y>-4096 && y<32767 ) {
			GDrawDrawLine(pixmap,x,y,end,y,hflexhintcol);
		    }
		}
		if ( cv->showvhints && sp->flexy ) {
		    double x,y,end;
		    x = cv->xoff + rint(sp->me.x*cv->scale);
		    y = -cv->yoff + cv->height - rint(sp->me.y*cv->scale);
		    end = -cv->yoff + cv->height - rint(sp->next->to->me.y*cv->scale);
		    if ( x>-4096 && x<32767 && y>-4096 && y<32767 ) {
			GDrawDrawLine(pixmap,x,y,x,end,vflexhintcol);
		    }
		}
		if ( sp->next==NULL )		/* This can happen if we get an internal error inside of RemoveOverlap when the pointlist is not in good shape */
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }
}

static void CVDrawRefName(CharView *cv,GWindow pixmap,RefChar *ref,int fg) {
    unichar_t namebuf[100];
    int x,y, len;

    x = cv->xoff + rint(ref->top.x*cv->scale);
    y = -cv->yoff + cv->height - rint(ref->top.y*cv->scale);
    y -= 5;
    if ( x<-400 || y<-40 || x>cv->width+400 || y>cv->height )
return;

    uc_strncpy(namebuf,ref->sc->name,sizeof(namebuf)/sizeof(namebuf[0]));
    GDrawSetFont(pixmap,cv->small);
    len = GDrawGetTextWidth(pixmap,namebuf,-1,NULL);
    GDrawDrawText(pixmap,x-len/2,y,namebuf,-1,NULL,fg);
}

void DrawAnchorPoint(GWindow pixmap,int x, int y,int selected) {
    GPoint gp[9];
    Color col = anchorcol;

    gp[0].x = x-1; gp[0].y = y-1;
    gp[1].x = x;   gp[1].y = y-6;
    gp[2].x = x+1; gp[2].y = y-1;
    gp[3].x = x+6; gp[3].y = y;
    gp[4].x = x+1; gp[4].y = y+1;
    gp[5].x = x;   gp[5].y = y+6;
    gp[6].x = x-1; gp[6].y = y+1;
    gp[7].x = x-6; gp[7].y = y;
    gp[8] = gp[0];
    if ( selected )
	GDrawDrawPoly(pixmap,gp,9,col);
    else
	GDrawFillPoly(pixmap,gp,9,col);
}

static void CVDrawAnchorPoints(CharView *cv,GWindow pixmap) {
    int x,y, len, sel;
    Color col = anchorcol;
    AnchorPoint *ap;
    char *name, ubuf[40];
    GRect r;

    if ( cv->drawmode!=dm_fore || cv->sc->anchor==NULL || !cv->showanchor )
return;
    GDrawSetFont(pixmap,cv->normal);

    for ( sel=0; sel<2; ++sel ) {
	for ( ap = cv->sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->selected==sel ) {
	    x = cv->xoff + rint(ap->me.x*cv->scale);
	    y = -cv->yoff + cv->height - rint(ap->me.y*cv->scale);
	    if ( x<-400 || y<-40 || x>cv->width+400 || y>cv->height )
	continue;

	    DrawAnchorPoint(pixmap,x,y,ap->selected);
	    if ( ap->anchor->type==act_mkmk ) {
		strncpy(ubuf,ap->anchor->name,20);
		strcat(ubuf," ");
		strcat(ubuf,ap->type==at_basemark ? _("Base") : _("Mark") );
		name = ubuf;
	    } else if ( ap->type==at_basechar || ap->type==at_mark || ap->type==at_basemark ) {
		name = ap->anchor->name;
	    } else if ( ap->type==at_centry || ap->type==at_cexit ) {
		strncpy(ubuf,ap->anchor->name,20);
		strcat(ubuf,ap->type==at_centry ? _("Entry") : _("Exit") );
		name = ubuf;
	    } else if ( ap->type==at_baselig ) {
		strncpy(ubuf,ap->anchor->name,20);
		sprintf(ubuf+strlen(ubuf),"#%d", ap->lig_index);
		name = ubuf;
	    } else
		name = NULL;		/* Should never happen */
	    len = GDrawGetText8Width(pixmap,name,-1,NULL);
	    r.x = x-len/2; r.width = len;
	    r.y = y+7; r.height = cv->nfh;
	    GDrawFillRect(pixmap,&r,GDrawGetDefaultBackground(NULL));
	    GDrawDrawText8(pixmap,x-len/2,y+7+cv->nas,name,-1,NULL,col);
	}
    }
}

static void DrawImageList(CharView *cv,GWindow pixmap,ImageList *backimages) {
    GRect size, temp;
    int x,y;

    GDrawGetSize(pixmap,&size);

    while ( backimages!=NULL ) {
	struct _GImage *base = backimages->image->list_len==0?
		backimages->image->u.image:backimages->image->u.images[0];

	temp = size;
	x = (int) (cv->xoff + rint(backimages->xoff * cv->scale));
	y = (int) (-cv->yoff + cv->height - rint(backimages->yoff*cv->scale));
	temp.x -= x; temp.y -= y;

	GDrawDrawImageMagnified(pixmap, backimages->image, &temp,
		x,y,
		(int) rint((base->width*backimages->xscale*cv->scale)),
		(int) rint((base->height*backimages->yscale*cv->scale)));
	backimages = backimages->next;
    }
}

static void DrawSelImageList(CharView *cv,GWindow pixmap,ImageList *images) {
    while ( images!=NULL ) {
	if ( images->selected )
	    CVDrawBB(cv,pixmap,&images->bb);
	images = images->next;
    }
}

static void DrawOldState(CharView *cv, GWindow pixmap, Undoes *undo, DRect *clip) {
    RefChar *refs;

    if ( undo==NULL )
return;

    CVDrawSplineSet(cv,pixmap,undo->u.state.splines,oldoutlinecol,false,clip);
    for ( refs=undo->u.state.refs; refs!=NULL; refs=refs->next )
	if ( refs->layers[0].splines!=NULL )
	    CVDrawSplineSet(cv,pixmap,refs->layers[0].splines,oldoutlinecol,false,clip);
    /* Don't do images... */
}
    
static void DrawTransOrigin(CharView *cv, GWindow pixmap) {
    int x = rint(cv->p.cx*cv->scale) + cv->xoff, y = cv->height-cv->yoff-rint(cv->p.cy*cv->scale);

    GDrawDrawLine(pixmap,x-4,y,x+4,y,transformorigincol);
    GDrawDrawLine(pixmap,x,y-4,x,y+4,transformorigincol);
}

static void DrawVLine(CharView *cv,GWindow pixmap,real pos,Color fg, int flags) {
    char buf[20];
    int x = cv->xoff + rint(pos*cv->scale);
    DrawLine(cv,pixmap,pos,-32768,pos,32767,fg);
    if ( flags&1 ) {
	if ( x>-400 && x<cv->width+400 ) {
	    dtos( buf, pos);
	    GDrawSetFont(pixmap,cv->small);
	    GDrawDrawText8(pixmap,x+5,cv->sas+3,buf,-1,NULL,metricslabelcol);
	}
    }
    if ( ItalicConstrained && cv->sc->parent->italicangle!=0 ) {
	double s = sin(-cv->sc->parent->italicangle*3.1415926535897932/180.);
	int xoff = rint(8096*s);
	DrawLine(cv,pixmap,pos-xoff,-8096,pos+xoff,8096,italiccoordcol);
    }
}

static void DrawMMGhosts(CharView *cv,GWindow pixmap,DRect *clip) {
    /* In an MM font, draw any selected alternate versions of the current char */
    MMSet *mm = cv->sc->parent->mm;
    int j;
    SplineFont *sub;
    SplineChar *sc;
    RefChar *rf;

    if ( mm==NULL )
return;
    for ( j = 0; j<mm->instance_count+1; ++j ) {
	if ( j==0 )
	    sub = mm->normal;
	else
	    sub = mm->instances[j-1];
	sc = NULL;
	if ( cv->sc->parent!=sub && (cv->mmvisible & (1<<j)) &&
		cv->sc->orig_pos<sub->glyphcnt )
	    sc = sub->glyphs[cv->sc->orig_pos];
	if ( sc!=NULL ) {
	    for ( rf=sc->layers[ly_fore].refs; rf!=NULL; rf = rf->next )
		CVDrawSplineSet(cv,pixmap,rf->layers[0].splines,backoutlinecol,false,clip);
	    CVDrawSplineSet(cv,pixmap,sc->layers[ly_fore].splines,backoutlinecol,false,clip);
	}
    }
}

static void CVDrawGridRaster(CharView *cv, GWindow pixmap, DRect *clip ) {
    if ( cv->showgrids ) {
	/* Draw ppem grid, and the raster for truetype debugging, grid fit */
	GRect pixel;
	real grid_spacing = (cv->sc->parent->ascent+cv->sc->parent->descent) / (real) cv->ft_ppem;
	int max,jmax,ii,i,jj,j;
	int minx, maxx, miny, maxy, r,or;
	Color clut[256];

	pixel.width = pixel.height = grid_spacing*cv->scale+1;
	if ( cv->raster!=NULL ) {
	    if ( cv->raster->num_greys>2 ) {
		int rb, gb, bb, rd, gd, bd;
		clut[0] = GDrawGetDefaultBackground(NULL);
		rb = COLOR_RED(clut[0]); gb = COLOR_GREEN(clut[0]); bb = COLOR_BLUE(clut[0]);
		rd = COLOR_RED(rasterdarkcol)-rb;
		gd = COLOR_GREEN(rasterdarkcol)-gb;
		bd = COLOR_BLUE(rasterdarkcol)-bb;
		for ( i=1; i<256; ++i ) {
		    clut[i] = ( (rb +rd*(i)/0xff)<<16 ) |
			    ( (gb+gd*(i)/0xff)<<8 ) |
			    ( (bb+bd*(i)/0xff) );
		}
	    }
	    minx = cv->raster->lb; maxx = minx+cv->raster->cols;
	    maxy = cv->raster->as; miny = maxy-cv->raster->rows;
	    if ( cv->oldraster!=NULL ) {
		if ( cv->oldraster->lb<minx ) minx = cv->oldraster->lb;
		if ( cv->oldraster->lb+cv->oldraster->cols>maxx ) maxx = cv->oldraster->lb+cv->oldraster->cols;
		if ( cv->oldraster->as>maxy ) maxy = cv->oldraster->as;
		if ( cv->oldraster->as-cv->oldraster->rows<miny ) miny = cv->oldraster->as-cv->oldraster->rows;
	    }
	    for ( ii=maxy; ii>miny; --ii ) {
		for ( jj=minx; jj<maxx; ++jj ) {
		    i = cv->raster->as-ii; j = jj-cv->raster->lb;
		    if ( i<0 || i>=cv->raster->rows || j<0 || j>=cv->raster->cols )
			r = 0;
		    else if ( cv->raster->num_greys<=2 )
			r = cv->raster->bitmap[i*cv->raster->bytes_per_row+(j>>3)] & (1<<(7-(j&7)));
		    else
			r = cv->raster->bitmap[i*cv->raster->bytes_per_row+j];
		    if ( cv->oldraster==NULL || cv->oldraster->num_greys!=cv->raster->num_greys)
			or = r;
		    else {
			i = cv->oldraster->as-ii; j = jj-cv->oldraster->lb;
			if ( i<0 || i>=cv->oldraster->rows || j<0 || j>=cv->oldraster->cols )
			    or = 0;
			else if ( cv->raster->num_greys<=2 )
			    or = cv->oldraster->bitmap[i*cv->oldraster->bytes_per_row+(j>>3)] & (1<<(7-(j&7)));
			else
			    or = cv->oldraster->bitmap[i*cv->oldraster->bytes_per_row+j];
		    }
		    if ( r || or ) {
			pixel.x = jj*grid_spacing*cv->scale + cv->xoff;
			pixel.y = cv->height-cv->yoff - rint(ii*grid_spacing*cv->scale);
			if ( cv->raster->num_greys<=2 )
			    GDrawFillRect(pixmap,&pixel,(r && or) ? rastercol : r ? rasternewcol : rasteroldcol );
			else
			    GDrawFillRect(pixmap,&pixel,(r-or>-16 && r-or<16) ? clut[r] : (clut[r]&0x00ff00) );
		    }
		}
	    }
	}

	for ( i = floor( clip->x/grid_spacing ), max = ceil((clip->x+clip->width)/grid_spacing);
		i<=max; ++i )
	    DrawLine(cv,pixmap,i*grid_spacing,-32768,i*grid_spacing,32767,i==0?coordcol:rastergridcol);
	for ( i = floor( clip->y/grid_spacing ), max = ceil((clip->y+clip->height)/grid_spacing);
		i<=max; ++i )
	    DrawLine(cv,pixmap,-32768,i*grid_spacing,32767,i*grid_spacing,i==0?coordcol:rastergridcol);
	if ( grid_spacing*cv->scale>=7 ) {
	    for ( i = floor( clip->x/grid_spacing ), max = ceil((clip->x+clip->width)/grid_spacing);
		    i<=max; ++i )
		for ( j = floor( clip->y/grid_spacing ), jmax = ceil((clip->y+clip->height)/grid_spacing);
			j<=jmax; ++j ) {
		    int x = (i+.5)*grid_spacing*cv->scale + cv->xoff;
		    int y = cv->height-cv->yoff - rint((j+.5)*grid_spacing*cv->scale);
		    GDrawDrawLine(pixmap,x-2,y,x+2,y,rastergridcol);
		    GDrawDrawLine(pixmap,x,y-2,x,y+2,rastergridcol);
		}
	}
    }
    if ( cv->showback ) {
	CVDrawSplineSet(cv,pixmap,cv->gridfit,gridfitoutlinecol,
		cv->showpoints,clip);
    }
}

static void CVExpose(CharView *cv, GWindow pixmap, GEvent *event ) {
    SplineFont *sf = cv->sc->parent;
    RefChar *rf;
    GRect old;
    DRect clip;
    char buf[20];
    PST *pst; int i, layer, rlayer;

    GDrawPushClip(pixmap,&event->u.expose.rect,&old);

    clip.width = event->u.expose.rect.width/cv->scale;
    clip.height = event->u.expose.rect.height/cv->scale;
    clip.x = (event->u.expose.rect.x-cv->xoff)/cv->scale;
    clip.y = (cv->height-event->u.expose.rect.y-event->u.expose.rect.height-cv->yoff)/cv->scale;

    GDrawSetFont(pixmap,cv->small);
    GDrawSetLineWidth(pixmap,0);

    if ( !cv->show_ft_results && cv->dv==NULL ) {
	/* if we've got bg images (and we're showing them) then the hints live in */
	/*  the bg image pixmap (else they get overwritten by the pixmap) */
	if ( (cv->showhhints || cv->showvhints || cv->showdhints) && ( cv->sc->layers[ly_back].images==NULL || !cv->showback) )
	    CVShowHints(cv,pixmap);

	for ( layer = ly_back; layer<cv->sc->layer_cnt; ++layer ) if ( cv->sc->layers[layer].images!=NULL ) {
	    if ( (( cv->showback || cv->drawmode==dm_back ) && layer==ly_back ) ||
		    (( cv->showfore || cv->drawmode==dm_fore ) && layer>ly_back )) {
		/* This really should be after the grids, but then it would completely*/
		/*  hide them. */
		GRect r;
		if ( cv->backimgs==NULL )
		    cv->backimgs = GDrawCreatePixmap(GDrawGetDisplayOfWindow(cv->v),cv->width,cv->height);
		if ( cv->back_img_out_of_date ) {
		    GDrawFillRect(cv->backimgs,NULL,GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(cv->v)));
		    if ( cv->showhhints || cv->showvhints || cv->showdhints)
			CVShowHints(cv,cv->backimgs);
		    DrawImageList(cv,cv->backimgs,cv->sc->layers[layer].images);
		    cv->back_img_out_of_date = false;
		}
		r.x = r.y = 0; r.width = cv->width; r.height = cv->height;
		GDrawDrawPixmap(pixmap,cv->backimgs,&r,0,0);
	    }
	}
	if ( cv->showgrids || cv->drawmode==dm_grid ) {
	    CVDrawSplineSet(cv,pixmap,cv->fv->sf->grid.splines,guideoutlinecol,
		    cv->showpoints && cv->drawmode==dm_grid,&clip);
	}
	if ( cv->showhmetrics ) {
	    DrawVLine(cv,pixmap,0,coordcol,false);
	    DrawLine(cv,pixmap,-8096,0,8096,0,coordcol);
	    DrawLine(cv,pixmap,-8096,sf->ascent,8096,sf->ascent,coordcol);
	    DrawLine(cv,pixmap,-8096,-sf->descent,8096,-sf->descent,coordcol);
	}
	if ( cv->showvmetrics ) {
	    DrawLine(cv,pixmap,(sf->ascent+sf->descent)/2,-8096,(sf->ascent+sf->descent)/2,8096,coordcol);
	    DrawLine(cv,pixmap,-8096,sf->vertical_origin,8096,sf->vertical_origin,coordcol);
	}

	DrawSelImageList(cv,pixmap,cv->layerheads[cv->drawmode]->images);

	if (( cv->showfore || cv->drawmode==dm_fore ) && cv->showfilled ) {
	    /* Wrong order, I know. But it is useful to have the background */
	    /*  visible on top of the fill... */
	    cv->gi.u.image->clut->clut[1] = fillcol;
	    GDrawDrawImage(pixmap, &cv->gi, NULL, cv->xoff + cv->filled->xmin,
		    -cv->yoff + cv->height-cv->filled->ymax);
	    cv->gi.u.image->clut->clut[1] = backimagecol;
	}
    } else {
	/* Draw FreeType Results */
	CVDrawGridRaster(cv,pixmap,&clip);
    }

    if ( cv->layerheads[cv->drawmode]->undoes!=NULL && cv->layerheads[cv->drawmode]->undoes->undotype==ut_tstate )
	DrawOldState(cv,pixmap,cv->layerheads[cv->drawmode]->undoes, &clip);

    if ( !cv->show_ft_results && cv->dv==NULL ) {
	if ( cv->showback || cv->drawmode==dm_back ) {
	    /* Used to draw the image list here, but that's too slow. Optimization*/
	    /*  is to draw to pixmap, dump pixmap a bit earlier */
	    /* Then when we moved the fill image around, we had to deal with the */
	    /*  images before the fill... */
	    CVDrawLayerSplineSet(cv,pixmap,&cv->sc->layers[ly_back],backoutlinecol,
		    cv->showpoints && cv->drawmode==dm_back,&clip);
	    if ( cv->template1!=NULL )
		CVDrawTemplates(cv,pixmap,cv->template1,&clip);
	    if ( cv->template2!=NULL )
		CVDrawTemplates(cv,pixmap,cv->template2,&clip);
	}
	if ( cv->mmvisible!=0 )
	    DrawMMGhosts(cv,pixmap,&clip);
    }

    if ( cv->showfore || (cv->drawmode==dm_fore && !cv->show_ft_results && cv->dv==NULL))  {
	CVDrawAnchorPoints(cv,pixmap);
	for ( layer=ly_fore ; layer<cv->sc->layer_cnt; ++layer ) {
	    for ( rf=cv->sc->layers[layer].refs; rf!=NULL; rf = rf->next ) {
		CVDrawRefName(cv,pixmap,rf,0);
		for ( rlayer=0; rlayer<rf->layer_cnt; ++rlayer )
		    CVDrawSplineSet(cv,pixmap,rf->layers[rlayer].splines,foreoutlinecol,-1,&clip);
		if ( rf->selected && cv->layerheads[cv->drawmode]==&cv->sc->layers[layer])
		    CVDrawBB(cv,pixmap,&rf->bb);
	    }

	    CVDrawLayerSplineSet(cv,pixmap,&cv->sc->layers[layer],foreoutlinecol,
		    cv->showpoints && cv->drawmode==dm_fore,&clip);
	}
    }

    if ( cv->freehand.current_trace!=NULL )
	CVDrawSplineSet(cv,pixmap,cv->freehand.current_trace,tracecol,
		false,&clip);

    if ( cv->showhmetrics && cv->searcher==NULL ) {
	DrawVLine(cv,pixmap,cv->sc->width,(!cv->inactive && cv->widthsel)?widthselcol:widthcol,true);
	for ( pst=cv->sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
	if ( pst!=NULL ) {
	    for ( i=0; i<pst->u.lcaret.cnt; ++i )
		DrawVLine(cv,pixmap,pst->u.lcaret.carets[i],lcaretcol,true);
	}
	if ( cv->show_ft_results || cv->dv!=NULL )
	    DrawVLine(cv,pixmap,cv->ft_gridfitwidth,widthgridfitcol,true);
    }
    if ( cv->showvmetrics ) {
	int len, y = -cv->yoff + cv->height - rint((sf->vertical_origin-cv->sc->vwidth)*cv->scale);
	DrawLine(cv,pixmap,-32768,sf->vertical_origin-cv->sc->vwidth,
			    32767,sf->vertical_origin-cv->sc->vwidth,
		(!cv->inactive && cv->vwidthsel)?widthselcol:widthcol);
	if ( y>-40 && y<cv->height+40 ) {
	    dtos( buf, cv->sc->vwidth);
	    GDrawSetFont(pixmap,cv->small);
	    len = GDrawGetText8Width(pixmap,buf,-1,NULL);
	    GDrawDrawText8(pixmap,cv->width-len-5,y,buf,-1,NULL,metricslabelcol);
	}
    }

    if ( cv->p.rubberbanding )
	CVDrawRubberRect(pixmap,cv);
    if ( cv->p.rubberlining )
	CVDrawRubberLine(pixmap,cv);
    if ((( cv->active_tool >= cvt_scale && cv->active_tool <= cvt_perspective ) ||
		cv->active_shape!=NULL ) &&
	    cv->p.pressed )
	DrawTransOrigin(cv,pixmap);

    GDrawPopClip(pixmap,&old);
}
#endif

void SCUpdateAll(SplineChar *sc) {
#if defined(FONTFORGE_CONFIG_GTK)
    CharView *cv;
    struct splinecharlist *dlist;

    for ( cv=sc->views; cv!=NULL; cv=cv->next )
	gtk_widget_queue_draw(cv->v);
    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	SCUpdateAll(dlist->sc);
#elif defined(FONTFORGE_CONFIG_GDRAW)
    CharView *cv;
    struct splinecharlist *dlist;

    for ( cv=sc->views; cv!=NULL; cv=cv->next )
	GDrawRequestExpose(cv->v,NULL,false);
    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	SCUpdateAll(dlist->sc);
#endif
}

void SCOutOfDateBackground(SplineChar *sc) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    CharView *cv;

    for ( cv=sc->views; cv!=NULL; cv=cv->next )
	cv->back_img_out_of_date = true;
#endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void CVRegenFill(CharView *cv) {
    if ( cv->showfilled ) {
	BDFCharFree(cv->filled);
	cv->filled = SplineCharRasterize(cv->sc,cv->scale*(cv->fv->sf->ascent+cv->fv->sf->descent)+.1);
	cv->gi.u.image->data = cv->filled->bitmap;
	cv->gi.u.image->bytes_per_line = cv->filled->bytes_per_line;
	cv->gi.u.image->width = cv->filled->xmax-cv->filled->xmin+1;
	cv->gi.u.image->height = cv->filled->ymax-cv->filled->ymin+1;
	GDrawRequestExpose(cv->v,NULL,false);
    }
}

static void FVRedrawAllCharViews(FontView *fv) {
    int i;
    CharView *cv;

    for ( i=0; i<fv->sf->glyphcnt; ++i ) if ( fv->sf->glyphs[i]!=NULL )
	for ( cv = fv->sf->glyphs[i]->views; cv!=NULL; cv=cv->next )
	    GDrawRequestExpose(cv->v,NULL,false);
}

static void SCRegenFills(SplineChar *sc) {
    struct splinecharlist *dlist;
    CharView *cv;

    for ( cv = sc->views; cv!=NULL; cv=cv->next )
	CVRegenFill(cv);
    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	SCRegenFills(dlist->sc);
}

static void SCRegenDependents(SplineChar *sc) {
    struct splinecharlist *dlist;
    FontView *fv;

    for ( fv = sc->parent->fv; fv!=NULL; fv=fv->nextsame ) {
	if ( fv->sv!=NULL ) {
	    SCReinstanciateRef(&fv->sv->sc_srch,sc);
	    SCReinstanciateRef(&fv->sv->sc_rpl,sc);
	}
    }

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	SCReinstanciateRef(dlist->sc,sc);
	SCRegenDependents(dlist->sc);
    }
}

static void CVUpdateInfo(CharView *cv, GEvent *event) {

    cv->info_within = true;
    cv->e.x = event->u.mouse.x; cv->e.y = event->u.mouse.y;
    cv->info.x = (event->u.mouse.x-cv->xoff)/cv->scale;
    cv->info.y = (cv->height-event->u.mouse.y-cv->yoff)/cv->scale;
    CVInfoDraw(cv,cv->gw);
}

static void CVNewScale(CharView *cv) {
    GEvent e;

    CVRegenFill(cv);
    cv->back_img_out_of_date = true;

    GScrollBarSetBounds(cv->vsb,-20000*cv->scale,8000*cv->scale,cv->height);
    GScrollBarSetBounds(cv->hsb,-8000*cv->scale,32000*cv->scale,cv->width);
    GScrollBarSetPos(cv->vsb,cv->yoff-cv->height);
    GScrollBarSetPos(cv->hsb,-cv->xoff);

    GDrawRequestExpose(cv->v,NULL,false);
    if ( cv->showrulers )
	GDrawRequestExpose(cv->gw,NULL,false);
    GDrawGetPointerPosition(cv->v,&e);
    CVUpdateInfo(cv,&e);
}

static void _CVFit(CharView *cv,DBounds *b) {
    real left, right, top, bottom, hsc, wsc;
    extern int palettes_docked;
    int offset = palettes_docked ? 90 : 0;
    int em = cv->sc->parent->ascent + cv->sc->parent->descent;
    int hsmall = true;

    if ( offset>cv->width ) offset = 0;

    bottom = b->miny;
    top = b->maxy;
    left = b->minx;
    right = b->maxx;

    if ( top<bottom ) IError("Bottom bigger than top!");
    if ( right<left ) IError("Left bigger than right!");
    top -= bottom;
    right -= left;
    if ( top==0 ) top = em;
    if ( right==0 ) right = em;
    wsc = (cv->width-offset) / right;
    hsc = cv->height / top;
    if ( wsc<hsc ) { hsc = wsc; hsmall = false ; }

    cv->scale = hsc;
    if ( cv->scale > 1.0 ) {
	cv->scale = floor(2*cv->scale)/2;
    } else {
	cv->scale = 2/ceil(2/cv->scale);
    }

    cv->xoff = -left *cv->scale + offset;
    if ( hsmall )
	cv->yoff = -bottom*cv->scale;
    else
	cv->yoff = -(bottom+top/2)*cv->scale + cv->height/2;

    CVNewScale(cv);
}

static void CVFit(CharView *cv) {
    DBounds b;
    double center;

    SplineCharFindBounds(cv->sc,&b);
    if ( b.miny>=0 ) b.miny = -cv->sc->parent->descent;
    if ( b.minx>0 ) b.minx = 0;
    if ( b.maxx<0 ) b.maxx = 0;
    if ( b.maxy<0 ) b.maxy = 0;
    if ( b.maxx<cv->sc->width ) b.maxx = cv->sc->width;
    /* Now give some extra space around the interesting stuff */
    center = (b.maxx+b.minx)/2;
    b.minx = center - (center - b.minx)*1.2;
    b.maxx = center + (b.maxx - center)*1.2;
    center = (b.maxy+b.miny)/2;
    b.miny = center - (center - b.miny)*1.2;
    b.maxy = center + (b.maxy - center)*1.2;

    _CVFit(cv,&b);
}

static void CVUnlinkView(CharView *cv ) {
    CharView *test;

    if ( cv->sc->views == cv ) {
	cv->sc->views = cv->next;
    } else {
	for ( test=cv->sc->views; test->next!=cv && test->next!=NULL; test=test->next );
	if ( test->next==cv )
	    test->next = cv->next;
    }
}

static GWindow CharIcon(CharView *cv, FontView *fv) {
    SplineChar *sc = cv->sc;
    BDFFont *bdf, *bdf2;
    BDFChar *bdfc;
    GWindow icon = cv->icon;
    GRect r;

    r.x = r.y = 0; r.width = r.height = fv->cbw-1;
    if ( icon == NULL )
	cv->icon = icon = GDrawCreateBitmap(NULL,r.width,r.width,NULL);
    GDrawFillRect(icon,&r,0x0);		/* for some reason icons seem to be color reversed by my defn */

    bdf = NULL; bdfc = NULL;
    if ( sc->layers[ly_fore].refs!=NULL || sc->layers[ly_fore].splines!=NULL ) {
	bdf = fv->show;
	if ( sc->orig_pos>=bdf->glyphcnt || bdf->glyphs[sc->orig_pos]==NULL )
	    bdf = fv->filled;
	if ( sc->orig_pos>=bdf->glyphcnt || bdf->glyphs[sc->orig_pos]==NULL ) {
	    bdf2 = NULL; bdfc = NULL;
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL && bdf->pixelsize<24 ; bdf=bdf->next )
		bdf2 = bdf;
	    if ( bdf2!=NULL && bdf!=NULL ) {
		if ( 24-bdf2->pixelsize < bdf->pixelsize-24 )
		    bdf = bdf2;
	    } else if ( bdf==NULL )
		bdf = bdf2;
	}
	if ( bdf!=NULL && sc->orig_pos<bdf->glyphcnt )
	    bdfc = bdf->glyphs[sc->orig_pos];
    }

    if ( bdfc!=NULL ) {
	GClut clut;
	struct _GImage base;
	GImage gi;
	/* if not empty, use the font's own shape, otherwise use a standard */
	/*  font */
	memset(&gi,'\0',sizeof(gi));
	memset(&base,'\0',sizeof(base));
	memset(&clut,'\0',sizeof(clut));
	gi.u.image = &base;
	base.trans = -1;
	base.clut = &clut;
	if ( bdfc->byte_data ) { int i;
	    base.image_type = it_index;
	    clut.clut_len = bdf->clut->clut_len;
	    for ( i=0; i<clut.clut_len; ++i ) {
		int v = 255-i*255/(clut.clut_len-1);
		clut.clut[i] = COLOR_CREATE(v,v,v);
	    }
	    clut.trans_index = -1;
	} else {
	    base.image_type = it_mono;
	    clut.clut_len = 2;
	    clut.clut[1] = 0xffffff;
	}
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	GDrawDrawImage(icon,&gi,NULL,(r.width-base.width)/2,(r.height-base.height)/2);
    } else if ( sc->unicodeenc!=-1 ) {
	FontRequest rq;
	GFont *font;
	static unichar_t times[] = { 't', 'i', 'm', 'e', 's',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t', '\0' };
	unichar_t text[2];
	int as, ds, ld, width;

	memset(&rq,0,sizeof(rq));
	rq.family_name = times;
	rq.point_size = 24;
	rq.weight = 400;
	font = GDrawInstanciateFont(NULL,&rq);
	GDrawSetFont(icon,font);
	text[0] = sc->unicodeenc; text[1] = 0;
	GDrawFontMetrics(font,&as,&ds,&ld);
	width = GDrawGetTextWidth(icon,text,1,NULL);
	GDrawDrawText(icon,(r.width-width)/2,(r.height-as-ds)/2+as,text,1,NULL,0xffffff);
    }
return( icon );
}

static int CVCurEnc(CharView *cv) {
    if ( cv->map_of_enc == cv->fv->map && cv->enc!=-1 )
return( cv->enc );

return( cv->fv->map->backmap[cv->sc->orig_pos] );
}

static char *CVMakeTitles(CharView *cv,char *buf) {
    char *title;
    SplineChar *sc = cv->sc;

/* GT: This is the title for a window showing an outline character */
/* GT: It will look something like: */
/* GT:  exclam at 33 from Arial */
/* GT: $1 is the name of the glyph */
/* GT: $2 is the glyph's encoding */
/* GT: $3 is the font name */
    sprintf(buf,_("%1$.80s at %2$d from %3$.90s"),
	    sc->name, CVCurEnc(cv), sc->parent->fontname);
    if ( sc->changed )
	strcat(buf," *");
    title = copy(buf);
    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x110000 && _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[sc->unicodeenc>>16][(sc->unicodeenc>>8)&0xff][sc->unicodeenc&0xff].name!=NULL ) {
	strcat(buf, " ");
	latin1_2_utf8_strcpy(buf+strlen(buf), _UnicodeNameAnnot[sc->unicodeenc>>16][(sc->unicodeenc>>8)&0xff][sc->unicodeenc&0xff].name);
    }
    if ( cv->show_ft_results || cv->dv )
	sprintf(buf+strlen(buf), " (%gpt, %ddpi)", (double) cv->ft_pointsize, cv->ft_dpi );
return( title );
}

void SCRefreshTitles(SplineChar *sc) {
    /* Called if the user changes the unicode encoding or the character name */
    CharView *cv;
    char buf[300], *title;

    if ( sc->views==NULL )
return;
    for ( cv = sc->views; cv!=NULL; cv=cv->next ) {
	title = CVMakeTitles(cv,buf);
	/* Could be different if one window is debugging and one is not */
	GDrawSetWindowTitles8(cv->gw,buf,title);
	free(title);
    }
}

static void CVChangeTabsVisibility(CharView *cv,int makevisible) {
    GRect gsize, pos, sbsize;

    if ( cv->tabs==NULL || GGadgetIsVisible(cv->tabs)==makevisible )
return;
    GGadgetGetSize(cv->tabs,&gsize);
    GGadgetGetSize(cv->vsb,&sbsize);
    if ( makevisible ) {
	cv->mbh += gsize.height;
	cv->height -= gsize.height;
	GGadgetMove(cv->vsb,sbsize.x,sbsize.y+gsize.height);
	GGadgetResize(cv->vsb,sbsize.width,sbsize.height-gsize.height);
    } else {
	cv->mbh -= gsize.height;
	cv->height += gsize.height;
	GGadgetMove(cv->vsb,sbsize.x,sbsize.y-gsize.height);
	GGadgetResize(cv->vsb,sbsize.width,sbsize.height+gsize.height);
    }
    GGadgetSetVisible(cv->tabs,makevisible);
    cv->back_img_out_of_date = true;
    pos.x = 0; pos.y = cv->mbh+cv->infoh;
    pos.width = cv->width; pos.height = cv->height;
    if ( cv->showrulers ) {
	pos.x += cv->rulerh;
	pos.y += cv->rulerh;
    }
    GDrawMoveResize(cv->v,pos.x,pos.y,pos.width,pos.height);
    GDrawSync(NULL);
    GDrawRequestExpose(cv->v,NULL,false);
    GDrawRequestExpose(cv->gw,NULL,false);
}

void CVChangeSC(CharView *cv, SplineChar *sc ) {
    char *title;
    char buf[300];
    extern int updateflex;
    int i;

    CVDebugFree(cv->dv);

    if ( cv->expandedge != ee_none ) {
	GDrawSetCursor(cv->v,ct_mypointer);
	cv->expandedge = ee_none;
    }

    SplinePointListsFree(cv->gridfit); cv->gridfit = NULL;
    FreeType_FreeRaster(cv->oldraster); cv->oldraster = NULL;
    FreeType_FreeRaster(cv->raster); cv->raster = NULL;

    SCLigCaretCheck(sc,false);

    CVUnlinkView(cv);
    cv->p.nextcp = cv->p.prevcp = cv->widthsel = cv->vwidthsel = false;
    if ( sc->views==NULL && updateflex )
	SplineCharIsFlexible(sc);
    cv->sc = sc;
    cv->next = sc->views;
    sc->views = cv;
    cv->layerheads[dm_fore] = &sc->layers[ly_fore];
    cv->layerheads[dm_back] = &sc->layers[ly_back];
    cv->layerheads[dm_grid] = &sc->parent->grid;
    cv->p.sp = cv->lastselpt = NULL;
    cv->template1 = cv->template2 = NULL;
#if HANYANG
    if ( cv->sc->parent->rules!=NULL && cv->sc->compositionunit )
	Disp_DefaultTemplate(cv);
#endif
    if ( cv->showpointnumbers || cv->show_ft_results )
	SCNumberPoints(sc);
    if ( cv->show_ft_results )
	CVGridFitChar(cv);

    CVNewScale(cv);

    CharIcon(cv,cv->fv);
    title = CVMakeTitles(cv,buf);
    GDrawSetWindowTitles8(cv->gw,buf,title);
    cv->lastselpt = NULL; cv->p.sp = NULL;
    CVInfoDraw(cv,cv->gw);
    free(title);
    _CVPaletteActivate(cv,true);

    if ( cv->tabs!=NULL ) {
	for ( i=0; i<cv->former_cnt; ++i )
	    if ( strcmp(cv->former_names[i],sc->name)==0 )
	break;
	if ( i!=cv->former_cnt && cv->showtabs )
	    GTabSetSetSel(cv->tabs,i);
	else {
	    if ( cv->former_cnt==FORMER_MAX )
		free(cv->former_names[FORMER_MAX-1]);
	    for ( i=cv->former_cnt<FORMER_MAX?cv->former_cnt-1:FORMER_MAX-2; i>=0; --i )
		cv->former_names[i+1] = cv->former_names[i];
	    cv->former_names[0] = copy(sc->name);
	    if ( cv->former_cnt<FORMER_MAX )
		++cv->former_cnt;
	    for ( i=0; i<cv->former_cnt; ++i )
		GTabSetChangeTabName(cv->tabs,cv->former_names[i],i);
	    GTabSetRemetric(cv->tabs);
	    GTabSetSetSel(cv->tabs,0);	/* This does a redraw */
	    if ( !GGadgetIsVisible(cv->tabs) && cv->showtabs )
		CVChangeTabsVisibility(cv,true);
	}
    }
}

static void CVChangeChar(CharView *cv, int i ) {
    SplineChar *sc;
    SplineFont *sf = cv->sc->parent;
    EncMap *map = cv->fv->map;
    int gid = i<0 || i>= map->enccount ? -2 : map->map[i];

    if ( gid == -2 )
return;
    if ( gid==-1 || (sc = sf->glyphs[gid])==NULL )
	sc = SFMakeChar(sf,map,i);

    if ( sc==NULL || cv->sc == sc )
return;
    cv->map_of_enc = map;
    cv->enc = i;
    CVChangeSC(cv,sc);
}

static int CVChangeToFormer( GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CharView *cv = GDrawGetUserData(GGadgetGetWindow(g));
	int new_aspect = GTabSetGetSel(g);
	SplineFont *sf = cv->sc->parent;
	int gid;

	for ( gid=sf->glyphcnt-1; gid>=0; --gid )
	    if ( sf->glyphs[gid]!=NULL &&
		    strcmp(sf->glyphs[gid]->name,cv->former_names[new_aspect])==0 )
	break;
	if ( gid<0 )
return( true );
	CVChangeSC(cv,sf->glyphs[gid]);
    }
return( true );
}

static void CVClear(GWindow,GMenuItem *mi, GEvent *);
static void CVMouseMove(CharView *cv, GEvent *event );
static void CVMouseUp(CharView *cv, GEvent *event );
static void CVHScroll(CharView *cv,struct sbevent *sb);
static void CVVScroll(CharView *cv,struct sbevent *sb);
static void CVElide(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void CVMerge(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void SVElide(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void SVMerge(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void CVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void CVMenuSimplifyMore(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void SVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e);
#if 0
static void SVMenuSimplifyMore(GWindow gw,struct gmenuitem *mi,GEvent *e);
#endif


static void CVFakeMove(CharView *cv, GEvent *event) {
    GEvent e;

    memset(&e,0,sizeof(e));
    e.type = et_mousemove;
    e.w = cv->v;
    if ( event->w!=cv->v ) {
	GPoint p;
	p.x = event->u.chr.x; p.y = event->u.chr.y;
	GDrawTranslateCoordinates(event->w,cv->v,&p);
	event->u.chr.x = p.x; event->u.chr.y = p.y;
    }
    e.u.mouse.state = TrueCharState(event);
    e.u.mouse.x = event->u.chr.x;
    e.u.mouse.y = event->u.chr.y;
    e.u.mouse.device = NULL;
    CVMouseMove(cv,&e);
}
	
void CVChar(CharView *cv, GEvent *event ) {
    extern float arrowAmount;

#if _ModKeysAutoRepeat
	/* Under cygwin these keys auto repeat, they don't under normal X */
	if ( cv->autorpt!=NULL ) {
	    GDrawCancelTimer(cv->autorpt); cv->autorpt = NULL;
	    if ( cv->keysym == event->u.chr.keysym )	/* It's an autorepeat, ignore it */
return;
	    CVToolsSetCursor(cv,cv->oldstate,NULL);
	}
#endif

#if MyMemory
    if ( event->u.chr.keysym == GK_F2 ) {
	fprintf( stderr, "Malloc debug on\n" );
	__malloc_debug(5);
    } else if ( event->u.chr.keysym == GK_F3 ) {
	fprintf( stderr, "Malloc debug off\n" );
	__malloc_debug(0);
    }
#endif
#if 0
    if ( event->u.chr.keysym == GK_F4 ) {
	RepeatFromFile(cv);
    }
#endif

    CVPaletteActivate(cv);
    CVToolsSetCursor(cv,TrueCharState(event),NULL);
	/* The isalpha check is to prevent infinite loops since DVChar can */
	/*  call CVChar too */
    if ( cv->dv!=NULL && isalpha(event->u.chr.chars[0]) && DVChar(cv->dv,event))
	/* All Done */;
    else if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuSaveAll(NULL,NULL,NULL);
    else if ( event->u.chr.keysym=='q' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuExit(NULL,NULL,NULL);
    else if (( event->u.chr.keysym=='M' ||event->u.chr.keysym=='m' ) &&
	    (event->u.chr.state&ksm_control) ) {
	if ( (event->u.chr.state&ksm_meta) && (event->u.chr.state&ksm_shift))
	    CVMenuSimplifyMore(cv->gw,NULL,NULL);
	else if ( (event->u.chr.state&ksm_shift))
	    CVMenuSimplify(cv->gw,NULL,NULL);
	else if ( (event->u.chr.state&ksm_meta) && (event->u.chr.state&ksm_control))
	    CVElide(cv->gw,NULL,NULL);
	else
	    CVMerge(cv->gw,NULL,NULL);
    } else if ( event->u.chr.keysym == GK_Shift_L || event->u.chr.keysym == GK_Shift_R ||
	     event->u.chr.keysym == GK_Alt_L || event->u.chr.keysym == GK_Alt_R ||
	     event->u.chr.keysym == GK_Meta_L || event->u.chr.keysym == GK_Meta_R ) {
	CVFakeMove(cv, event);
    } else if ( (event->u.chr.state&ksm_meta) &&
	    !(event->u.chr.state&(ksm_control|ksm_shift)) &&
	    event->u.chr.chars[0]!='\0' ) {
	CVPaletteMnemonicCheck(event);
    } else if ( !(event->u.chr.state&(ksm_control|ksm_meta)) &&
	    event->u.chr.keysym == GK_BackSpace ) {
	/* Menu does delete */
	CVClear(cv->gw,NULL,NULL);
    } else if ( event->u.chr.keysym == GK_Help ) {
	MenuHelp(NULL,NULL,NULL);	/* Menu does F1 */
    } else if ( (event->u.chr.keysym=='[' || event->u.chr.keysym==']') &&
	    (event->u.chr.state&ksm_control) ) {
	/* some people have remapped keyboards so that shift is needed to get [] */
	int pos;
	if ( event->u.chr.keysym=='[' ) {
	    pos = CVCurEnc(cv)-1;
	} else {
	    pos = CVCurEnc(cv)+1;
	}
#if 0		/* Werner doesn't want it to wrap */
	if ( pos<0 ) pos = cv->fv->map->enccount-1;
	else if ( pos>= cv->fv->map->enccount ) pos = 0;
#endif
	if ( pos>=0 && pos<cv->fv->map->enccount )
	    CVChangeChar(cv,pos);
    } else if ( event->u.chr.keysym == GK_Left ||
	    event->u.chr.keysym == GK_Up ||
	    event->u.chr.keysym == GK_Right ||
	    event->u.chr.keysym == GK_Down ||
	    event->u.chr.keysym == GK_KP_Left ||
	    event->u.chr.keysym == GK_KP_Up ||
	    event->u.chr.keysym == GK_KP_Right ||
	    event->u.chr.keysym == GK_KP_Down ) {
	real dx=0, dy=0; int anya;
	switch ( event->u.chr.keysym ) {
	  case GK_Left: case GK_KP_Left:
	    dx = -1;
	  break;
	  case GK_Right: case GK_KP_Right:
	    dx = 1;
	  break;
	  case GK_Up: case GK_KP_Up:
	    dy = 1;
	  break;
	  case GK_Down: case GK_KP_Down:
	    dy = -1;
	  break;
	}
	if ( event->u.chr.state & (ksm_control|ksm_capslock) ) {
	    struct sbevent sb;
	    sb.type = dy>0 || dx<0 ? et_sb_halfup : et_sb_halfdown;
	    if ( dx==0 )
		CVVScroll(cv,&sb);
	    else
		CVHScroll(cv,&sb);
	} else {
	    if ( event->u.chr.state & ksm_meta ) {
		dx *= 10; dy *= 10;
	    }
	    if ( event->u.chr.state & (ksm_shift) )
		dx -= dy*tan((cv->sc->parent->italicangle)*(3.1415926535897932/180) );
	    if (( cv->p.sp!=NULL || cv->lastselpt!=NULL ) &&
		    (cv->p.nextcp || cv->p.prevcp) ) {
		SplinePoint *sp = cv->p.sp ? cv->p.sp : cv->lastselpt;
		SplinePoint *old = cv->p.sp;
		BasePoint *which = cv->p.nextcp ? &sp->nextcp : &sp->prevcp;
		BasePoint to;
		to.x = which->x + dx*arrowAmount;
		to.y = which->y + dy*arrowAmount;
		cv->p.sp = sp;
		CVPreserveState(cv);
		CVAdjustControl(cv,which,&to);
		cv->p.sp = old;
		SCUpdateAll(cv->sc);
	    } else if ( CVAnySel(cv,NULL,NULL,NULL,&anya) || cv->widthsel || cv->vwidthsel ) {
		CVPreserveState(cv);
		CVMoveSelection(cv,dx*arrowAmount,dy*arrowAmount, event->u.chr.state);
		if ( cv->widthsel )
		    SCSynchronizeWidth(cv->sc,cv->sc->width,cv->sc->width-dx,NULL);
		CVCharChangedUpdate(cv);
		CVInfoDraw(cv,cv->gw);
	    }
	}
    } else if ( event->u.chr.keysym == GK_Page_Up ||
	    event->u.chr.keysym == GK_KP_Page_Up ||
	    event->u.chr.keysym == GK_Prior ||
	    event->u.chr.keysym == GK_Page_Down ||
	    event->u.chr.keysym == GK_KP_Page_Down ||
	    event->u.chr.keysym == GK_Next ) {
	/* Um... how do we scroll horizontally??? */
	struct sbevent sb;
	sb.type = et_sb_uppage;
	if ( event->u.chr.keysym == GK_Page_Down ||
		event->u.chr.keysym == GK_KP_Page_Down ||
		event->u.chr.keysym == GK_Next )
	    sb.type = et_sb_downpage;
	CVVScroll(cv,&sb);
    } else if ( event->u.chr.keysym == GK_Home ) {
	CVFit(cv);
    } else if ( !(event->u.chr.state&(ksm_control|ksm_meta)) &&
	    event->type == et_char &&
	    cv->searcher==NULL &&
	    cv->dv==NULL &&
	    event->u.chr.chars[0]>=' ' && event->u.chr.chars[1]=='\0' ) {
	SplineFont *sf = cv->sc->parent;
	int i;
	EncMap *map = cv->fv->map;
	extern int cv_auto_goto;
	if ( cv_auto_goto ) {
	    i = SFFindSlot(sf,map,event->u.chr.chars[0],NULL);
	    if ( i!=-1 )
		CVChangeChar(cv,i);
	}
    }
}

static void CVCharUp(CharView *cv, GEvent *event ) {
#if _ModKeysAutoRepeat
    /* Under cygwin these keys auto repeat, they don't under normal X */
    if ( event->u.chr.keysym == GK_Shift_L || event->u.chr.keysym == GK_Shift_R ||
	    event->u.chr.keysym == GK_Control_L || event->u.chr.keysym == GK_Control_R ||
	    event->u.chr.keysym == GK_Meta_L || event->u.chr.keysym == GK_Meta_R ||
	    event->u.chr.keysym == GK_Alt_L || event->u.chr.keysym == GK_Alt_R ||
	    event->u.chr.keysym == GK_Super_L || event->u.chr.keysym == GK_Super_R ||
	    event->u.chr.keysym == GK_Hyper_L || event->u.chr.keysym == GK_Hyper_R ) {
	if ( cv->autorpt!=NULL ) {
	    GDrawCancelTimer(cv->autorpt);
	    CVToolsSetCursor(cv,cv->oldstate,NULL);
	}
	cv->keysym = event->u.chr.keysym;
	cv->oldkeyx = event->u.chr.x;
	cv->oldkeyy = event->u.chr.y;
	cv->oldkeyw = event->w;
	cv->oldstate = TrueCharState(event);
	cv->autorpt = GDrawRequestTimer(cv->v,100,0,NULL);
    } else {
	if ( cv->autorpt!=NULL ) {
	    GDrawCancelTimer(cv->autorpt); cv->autorpt=NULL;
	    CVToolsSetCursor(cv,cv->oldstate,NULL);
	}
	CVToolsSetCursor(cv,TrueCharState(event),NULL);
    }
#else
    CVToolsSetCursor(cv,TrueCharState(event),NULL);
    if ( event->u.chr.keysym == GK_Shift_L || event->u.chr.keysym == GK_Shift_R ||
	     event->u.chr.keysym == GK_Alt_L || event->u.chr.keysym == GK_Alt_R ||
	     event->u.chr.keysym == GK_Meta_L || event->u.chr.keysym == GK_Meta_R )
	CVFakeMove(cv, event);
#endif
}

static void CVInfoDrawText(CharView *cv, GWindow pixmap ) {
    GRect r;
    Color bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap));
    Color fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap));
    char buffer[50];
    unichar_t ubuffer[50];
    int ybase = cv->mbh+(cv->infoh-cv->sfh)/2+cv->sas;
    real xdiff, ydiff;
    SplinePoint *sp, dummy;

    GDrawSetFont(pixmap,cv->small);
    r.x = RPT_DATA; r.width = 60;
    r.y = cv->mbh; r.height = cv->infoh-1;
    GDrawFillRect(pixmap,&r,bg);
    r.x = SPT_DATA; r.width = 60;
    GDrawFillRect(pixmap,&r,bg);
    r.x = SOF_DATA; r.width = 60;
    GDrawFillRect(pixmap,&r,bg);
    r.x = SDS_DATA; r.width = 30;
    GDrawFillRect(pixmap,&r,bg);
    r.x = SAN_DATA; r.width = 30;
    GDrawFillRect(pixmap,&r,bg);
    r.x = MAG_DATA; r.width = 60;
    GDrawFillRect(pixmap,&r,bg);
    r.x = LAYER_DATA; r.width = 60;
    GDrawFillRect(pixmap,&r,bg);
    r.x = CODERANGE_DATA; r.width = 60;
    GDrawFillRect(pixmap,&r,bg);

    if ( cv->info_within ) {
	if ( cv->info.x>=1000 || cv->info.x<=-1000 || cv->info.y>=1000 || cv->info.y<=-1000 )
	    sprintf(buffer,"%d%s%d", (int) cv->info.x, coord_sep, (int) cv->info.y );
	else
	    sprintf(buffer,"%.4g%s%.4g", (double) cv->info.x, coord_sep, (double) cv->info.y );
	buffer[11] = '\0';
	uc_strcpy(ubuffer,buffer);
	GDrawDrawText(pixmap,RPT_DATA,ybase,ubuffer,-1,NULL,fg);
    }
    if ( cv->scale>=.25 )
	sprintf( buffer, "%d%%", (int) (100*cv->scale));
    else
	sprintf( buffer, "%.3g%%", (double) (100*cv->scale));
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,MAG_DATA,ybase,ubuffer,-1,NULL,fg);
    GDrawDrawText8(pixmap,LAYER_DATA,ybase,
/* GT: Foreground, make it short */
		cv->drawmode==dm_fore ? _("Fore") :
/* GT: Background, make it short */
		cv->drawmode==dm_back ? _("Back") :
/* GT: Guide layer, make it short */
		_("Guide"),
	    -1,NULL,fg);
    if ( cv->coderange!=cr_none )
	GDrawDrawText8(pixmap,CODERANGE_DATA,ybase,
		cv->coderange==cr_fpgm ? _("'fpgm'") :
		cv->coderange==cr_prep ? _("'prep'") : _("Glyph"),
	    -1,NULL,fg);
    sp = cv->p.sp!=NULL ? cv->p.sp : cv->lastselpt;
    if ( sp==NULL ) if ( cv->active_tool==cvt_rect || cv->active_tool==cvt_elipse ||
	    cv->active_tool==cvt_poly || cv->active_tool==cvt_star ) {
	dummy.me.x = cv->p.cx; dummy.me.y = cv->p.cy;
	sp = &dummy;
    }
    if ( sp ) {
	real selx, sely;
	if ( cv->pressed && sp==cv->p.sp ) {
	    selx = cv->p.constrain.x;
	    sely = cv->p.constrain.y;
	} else {
	    selx = sp->me.x;
	    sely = sp->me.y;
	}
	xdiff=cv->info.x-selx;
	ydiff = cv->info.y-sely;

	if ( selx>=1000 || selx<=-1000 || sely>=1000 || sely<=-1000 )
	    sprintf(buffer,"%d%s%d", (int) selx, coord_sep, (int) sely );
	else
	    sprintf(buffer,"%.4g%s%.4g", (double) selx, coord_sep, (double) sely );
	buffer[11] = '\0';
	uc_strcpy(ubuffer,buffer);
	GDrawDrawText(pixmap,SPT_DATA,ybase,ubuffer,-1,NULL,fg);
    } else if ( cv->widthsel && cv->info_within ) {
	xdiff = cv->info.x-cv->p.cx;
	ydiff = 0;
    } else if ( cv->p.rubberbanding && cv->info_within ) {
	xdiff=cv->info.x-cv->p.cx;
	ydiff = cv->info.y-cv->p.cy;
    } else
return;
    if ( !cv->info_within )
return;

    if ( xdiff>=1000 || xdiff<=-1000 || ydiff>=1000 || ydiff<=-1000 )
	sprintf(buffer,"%d%s%d", (int) xdiff, coord_sep, (int) ydiff );
    else
	sprintf(buffer,"%.4g%s%.4g", (double) xdiff, coord_sep, (double) ydiff );
    buffer[11] = '\0';
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,SOF_DATA,ybase,ubuffer,-1,NULL,fg);

    sprintf( buffer, "%.1f", sqrt(xdiff*xdiff+ydiff*ydiff));
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,SDS_DATA,ybase,ubuffer,-1,NULL,fg);

    sprintf( buffer, "%d\260", (int) rint(180*atan2(ydiff,xdiff)/3.1415926535897932));
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,SAN_DATA,ybase,ubuffer,-1,NULL,fg);
}

static void CVInfoDrawRulers(CharView *cv, GWindow pixmap ) {
    int rstart = cv->mbh+cv->infoh;
    GDrawSetXORMode(pixmap);
    GDrawSetXORBase(pixmap,GDrawGetDefaultBackground(NULL));
    GDrawSetLineWidth(pixmap,0);
    if ( cv->olde.x!=-1 ) {
	GDrawDrawLine(pixmap,cv->olde.x+cv->rulerh,rstart,cv->olde.x+cv->rulerh,rstart+cv->rulerh,0xff0000);
	GDrawDrawLine(pixmap,0,cv->olde.y+rstart+cv->rulerh,cv->rulerh,cv->olde.y+rstart+cv->rulerh,0xff0000);
    }
    GDrawDrawLine(pixmap,cv->e.x+cv->rulerh,rstart,cv->e.x+cv->rulerh,rstart+cv->rulerh,0xff0000);
    GDrawDrawLine(pixmap,0,cv->e.y+rstart+cv->rulerh,cv->rulerh,cv->e.y+rstart+cv->rulerh,0xff0000);
    cv->olde = cv->e;
    GDrawSetCopyMode(pixmap);
}

void CVInfoDraw(CharView *cv, GWindow pixmap ) {
    CVInfoDrawText(cv,pixmap);
    if ( cv->showrulers )
	CVInfoDrawRulers(cv,pixmap);
}

static void CVCrossing(CharView *cv, GEvent *event ) {
    CVToolsSetCursor(cv,event->u.mouse.state,event->u.mouse.device);
    cv->info_within = event->u.crossing.entered;
    cv->info.x = (event->u.crossing.x-cv->xoff)/cv->scale;
    cv->info.y = (cv->height-event->u.crossing.y-cv->yoff)/cv->scale;
    CVInfoDraw(cv,cv->gw);
    CPEndInfo(cv);
}

static int CheckPoint(FindSel *fs, SplinePoint *sp, SplineSet *spl) {

    if ( fs->xl<=sp->me.x && fs->xh>=sp->me.x &&
	    fs->yl<=sp->me.y && fs->yh >= sp->me.y ) {
	fs->p->sp = sp;
	fs->p->spline = NULL;
	fs->p->anysel = true;
	fs->p->spl = spl;
	if ( !fs->seek_controls )
return( true );
    }
    if ( (sp->selected && fs->select_controls) || fs->all_controls ) {
	if ( fs->xl<=sp->nextcp.x && fs->xh>=sp->nextcp.x &&
		fs->yl<=sp->nextcp.y && fs->yh >= sp->nextcp.y ) {
	    fs->p->sp = sp;
	    fs->p->spline = NULL;
	    fs->p->spl = spl;
	    fs->p->nextcp = true;
	    fs->p->anysel = true;
	    fs->p->cp = sp->nextcp;
	    if ( sp->nonextcp && sp->pointtype==pt_curve ) {
		fs->p->cp.x = sp->me.x + (sp->me.x-sp->prevcp.x);
		fs->p->cp.y = sp->me.y + (sp->me.y-sp->prevcp.y);
	    }
	    sp->selected = true;
return( true );
	} else if ( fs->xl<=sp->prevcp.x && fs->xh>=sp->prevcp.x &&
		fs->yl<=sp->prevcp.y && fs->yh >= sp->prevcp.y ) {
	    fs->p->sp = sp;
	    fs->p->spline = NULL;
	    fs->p->spl = spl;
	    fs->p->prevcp = true;
	    fs->p->anysel = true;
	    fs->p->cp = sp->prevcp;
	    if ( sp->noprevcp && sp->pointtype==pt_curve ) {
		fs->p->cp.x = sp->me.x + (sp->me.x-sp->nextcp.x);
		fs->p->cp.y = sp->me.y + (sp->me.y-sp->nextcp.y);
	    }
	    sp->selected = true;
return( true );
	}
    }
return( false );
}

static int CheckSpline(FindSel *fs, Spline *spline, SplineSet *spl) {

    /* Anything else is better than a spline */
    if ( fs->p->anysel )
return( false );

    if ( NearSpline(fs,spline)) {
	fs->p->spline = spline;
	fs->p->spl = spl;
	fs->p->anysel = true;
return( false /*true*/ );	/* Check if there's a point where we are first */
	/* if there is use it, if not (because anysel is true) we'll fall back */
	/* here */
    }
return( false );
}

static int InImage( FindSel *fs, ImageList *img) {
    int x,y;

    x = floor((fs->p->cx-img->xoff)/img->xscale);
    y = floor((img->yoff-fs->p->cy)/img->yscale);
    if ( x<0 || y<0 || x>=GImageGetWidth(img->image) || y>=GImageGetHeight(img->image))
return ( false );
    if ( GImageGetPixelColor(img->image,x,y)&0x80000000 )	/* Transparent */
return( false );

return( true );
}

static int InSplineSet( FindSel *fs, SplinePointList *set) {
    SplinePointList *spl;
    Spline *spline, *first;

    for ( spl = set; spl!=NULL; spl = spl->next ) {
	if ( CheckPoint(fs,spl->first,spl) && ( !fs->seek_controls || fs->p->nextcp || fs->p->prevcp )) {
return( true );
	}
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( (CheckPoint(fs,spline->to,spl) && ( !fs->seek_controls || fs->p->nextcp || fs->p->prevcp )) ||
		    ( CheckSpline(fs,spline,spl) && !fs->seek_controls )) {
return( true );
	    }
	    if ( first==NULL ) first = spline;
	}
    }
return( fs->p->anysel );
}

static int NearSplineSetPoints( FindSel *fs, SplinePointList *set) {
    SplinePointList *spl;
    Spline *spline, *first;

    for ( spl = set; spl!=NULL; spl = spl->next ) {
	if ( CheckPoint(fs,spl->first,spl)) {
return( true );
	}
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( CheckPoint(fs,spline->to,spl) ) {
return( true );
	    }
	    if ( first==NULL ) first = spline;
	}
    }
return( fs->p->anysel );
}

static void SetFS( FindSel *fs, PressedOn *p, CharView *cv, GEvent *event) {
    extern float snapdistance;

    memset(p,'\0',sizeof(PressedOn));
    p->pressed = true;

    memset(fs,'\0',sizeof(*fs));
    fs->p = p;
    fs->e = event;
    p->x = event->u.mouse.x;
    p->y = event->u.mouse.y;
    p->cx = (event->u.mouse.x-cv->xoff)/cv->scale;
    p->cy = (cv->height-event->u.mouse.y-cv->yoff)/cv->scale;

    fs->fudge = snapdistance/cv->scale;		/* 3.5 pixel fudge */
    fs->xl = p->cx - fs->fudge;
    fs->xh = p->cx + fs->fudge;
    fs->yl = p->cy - fs->fudge;
    fs->yh = p->cy + fs->fudge;
}

int CVMouseAtSpline(CharView *cv,GEvent *event) {
    FindSel fs;
    int pressed = cv->p.pressed;

    SetFS(&fs,&cv->p,cv,event);
    cv->p.pressed = pressed;
return( InSplineSet(&fs,cv->layerheads[cv->drawmode]->splines));
}

static GEvent *CVConstrainedMouseDown(CharView *cv,GEvent *event, GEvent *fake) {
    SplinePoint *base;
    int basex, basey, dx, dy;
    int sign;

    base = CVAnySelPoint(cv);
    if ( base==NULL )
return( event );

    basex =  cv->xoff + rint(base->me.x*cv->scale);
    basey = -cv->yoff + cv->height - rint(base->me.y*cv->scale);

    dx= event->u.mouse.x-basex, dy = event->u.mouse.y-basey;
    sign = dx*dy<0?-1:1;

    fake->u.mouse = event->u.mouse;
    if ( dx<0 ) dx = -dx; if ( dy<0 ) dy = -dy;
    if ( dy >= 2*dx ) {
	cv->p.x = fake->u.mouse.x = basex;
	cv->p.cx = base->me.x;
	if ( !(event->u.mouse.state&ksm_alt) &&
		ItalicConstrained && cv->sc->parent->italicangle!=0 ) {
	    double off = tan(cv->sc->parent->italicangle*3.1415926535897932/180)*
		    (cv->p.cy-base->me.y);
	    double aoff = off<0 ? -off : off;
	    if ( dx>=aoff*cv->scale/2 && (event->u.mouse.x-basex<0)!=(off<0) ) {
		cv->p.cx -= off;
		cv->p.x = fake->u.mouse.x = cv->xoff + rint(cv->p.cx*cv->scale);
	    }
	}
    } else if ( dx >= 2*dy ) {
	fake->u.mouse.y = basey;
	cv->p.cy = base->me.y;
    } else if ( dx > dy ) {
	fake->u.mouse.x = basex + sign * (event->u.mouse.y-cv->p.y);
	cv->p.cx = base->me.x - sign * (cv->p.cy-base->me.y);
    } else {
	fake->u.mouse.y = basey + sign * (event->u.mouse.x-cv->p.x);
	cv->p.cy = base->me.y - sign * (cv->p.cx-base->me.x);
    }

return( fake );
}

static void CVSetConstrainPoint(CharView *cv, GEvent *event) {
    SplineSet *sel;

    if ( (sel = CVAnySelPointList(cv))!=NULL ) {
	if ( sel->first->selected ) cv->p.constrain = sel->first->me;
	else cv->p.constrain = sel->last->me;
    } else if ( cv->p.sp!=NULL ) {
	cv->p.constrain = cv->p.sp->me;
    } else {
	cv->p.constrain.x = cv->info.x;
	cv->p.constrain.y = cv->info.y;
    }
}

static void CVDoSnaps(CharView *cv, FindSel *fs) {
    PressedOn *p = fs->p;

#if 1
    if ( cv->drawmode!=dm_grid && cv->layerheads[dm_grid]->splines!=NULL ) {
	PressedOn temp;
	int oldseek = fs->seek_controls;
	temp = *p;
	fs->p = &temp;
	fs->seek_controls = false;
	if ( InSplineSet( fs, cv->layerheads[dm_grid]->splines)) {
	    if ( temp.spline!=NULL ) {
		p->cx = ((temp.spline->splines[0].a*temp.t+
			    temp.spline->splines[0].b)*temp.t+
			    temp.spline->splines[0].c)*temp.t+
			    temp.spline->splines[0].d;
		p->cy = ((temp.spline->splines[1].a*temp.t+
			    temp.spline->splines[1].b)*temp.t+
			    temp.spline->splines[1].c)*temp.t+
			    temp.spline->splines[1].d;
	    } else if ( temp.sp!=NULL ) {
		p->cx = temp.sp->me.x;
		p->cy = temp.sp->me.y;
	    }
	}
	fs->p = p;
	fs->seek_controls = oldseek;
    }
#endif
    if ( p->cx>-fs->fudge && p->cx<fs->fudge )
	p->cx = 0;
    else if ( p->cx>cv->sc->width-fs->fudge && p->cx<cv->sc->width+fs->fudge &&
	    !cv->widthsel)
	p->cx = cv->sc->width;
    else if ( cv->widthsel && p!=&cv->p &&
	    p->cx>cv->oldwidth-fs->fudge && p->cx<cv->oldwidth+fs->fudge )
	p->cx = cv->oldwidth;
#if 0
    else if ( cv->sc->parent->hsnaps!=NULL && cv->drawmode!=dm_grid ) {
	int i, *hsnaps = cv->sc->parent->hsnaps;
	for ( i=0; hsnaps[i]!=0x80000000; ++i ) {
	    if ( p->cx>hsnaps[i]-fs->fudge && p->cx<hsnaps[i]+fs->fudge ) {
		p->cx = hsnaps[i];
	break;
	    }
	}
    }
#endif
    if ( p->cy>-fs->fudge && p->cy<fs->fudge )
	p->cy = 0;
#if 0
    else if ( cv->sc->parent->vsnaps!=NULL && cv->drawmode!=dm_grid ) {
	int i, *vsnaps = cv->sc->parent->vsnaps;
	for ( i=0; vsnaps[i]!=0x80000000; ++i ) {
	    if ( p->cy>vsnaps[i]-fs->fudge && p->cy<vsnaps[i]+fs->fudge ) {
		p->cy = vsnaps[i];
	break;
	    }
	}
    }
#endif
}

static int _CVTestSelectFromEvent(CharView *cv,FindSel *fs) {
    PressedOn temp;
    ImageList *img;

    if ( !InSplineSet(fs,cv->layerheads[cv->drawmode]->splines)) {
	if ( cv->drawmode==dm_fore) {
	    RefChar *rf;
	    temp = cv->p;
	    fs->p = &temp;
	    fs->seek_controls = false;
	    for ( rf=cv->sc->layers[ly_fore].refs; rf!=NULL; rf = rf->next ) {
		if ( InSplineSet(fs,rf->layers[0].splines)) {
		    cv->p.ref = rf;
		    cv->p.anysel = true;
	    break;
		}
	    }
	    if ( cv->showanchor && !cv->p.anysel ) {
		AnchorPoint *ap, *found=NULL;
		/* I do this pecular search because: */
		/*	1) I expect there to be lots of times we get multiple */
		/*     anchors at the same location */
		/*  2) The anchor points are drawn so that the bottommost */
		/*	   is displayed */
		for ( ap=cv->sc->anchor; ap!=NULL; ap=ap->next )
		    if ( fs->xl<=ap->me.x && fs->xh>=ap->me.x &&
			    fs->yl<=ap->me.y && fs->yh >= ap->me.y )
			found = ap;
		if ( found!=NULL ) {
		    cv->p.ap = found;
		    cv->p.anysel = true;
		}
	    }
	}
	for ( img = cv->layerheads[cv->drawmode]->images; img!=NULL; img=img->next ) {
	    if ( InImage(fs,img)) {
		cv->p.img = img;
		cv->p.anysel = true;
	break;
	    }
	}
    }
return( cv->p.anysel );
}

int CVTestSelectFromEvent(CharView *cv,GEvent *event) {
    FindSel fs;

    SetFS(&fs,&cv->p,cv,event);
return( _CVTestSelectFromEvent(cv,&fs));
}

static void CVMouseDown(CharView *cv, GEvent *event ) {
    FindSel fs;
    GEvent fake;

    if ( event->u.mouse.button==2 && event->u.mouse.device!=NULL &&
	    strcmp(event->u.mouse.device,"stylus")==0 )
return;		/* I treat this more like a modifier key change than a button press */

    if ( cv->expandedge != ee_none )
	GDrawSetCursor(cv->v,ct_mypointer);
    if ( event->u.mouse.button==3 ) {
	CVToolsPopup(cv,event);
return;
    }
    CVToolsSetCursor(cv,event->u.mouse.state|(1<<(7+event->u.mouse.button)), event->u.mouse.device );
    cv->active_tool = cv->showing_tool;
    cv->needsrasterize = false;
    cv->recentchange = false;

    SetFS(&fs,&cv->p,cv,event);
    if ( event->u.mouse.state&ksm_shift )
	event = CVConstrainedMouseDown(cv,event,&fake);

    if ( cv->active_tool == cvt_pointer ) {
	fs.select_controls = true;
	if ( event->u.mouse.state&ksm_alt ) fs.seek_controls = true;
	if ( cv->showpointnumbers && cv->fv->sf->order2 ) fs.all_controls = true;
	cv->lastselpt = NULL;
	_CVTestSelectFromEvent(cv,&fs);
	fs.p = &cv->p;
    } else if ( cv->active_tool == cvt_curve || cv->active_tool == cvt_corner ||
	    cv->active_tool == cvt_tangent || cv->active_tool == cvt_pen ) {
	InSplineSet(&fs,cv->layerheads[cv->drawmode]->splines);
	if ( fs.p->sp==NULL && fs.p->spline==NULL )
	    CVDoSnaps(cv,&fs);
    } else {
	NearSplineSetPoints(&fs,cv->layerheads[cv->drawmode]->splines);
	if ( fs.p->sp==NULL && fs.p->spline==NULL )
	    CVDoSnaps(cv,&fs);
    }

    cv->e.x = event->u.mouse.x; cv->e.y = event->u.mouse.y;
    if ( cv->p.sp!=NULL ) {
	BasePoint *p;
	if ( cv->p.nextcp )
	    p = &cv->p.sp->nextcp;
	else if ( cv->p.prevcp )
	    p = &cv->p.sp->prevcp;
	else
	    p = &cv->p.sp->me;
	cv->info.x = p->x;
	cv->info.y = p->y;
	cv->p.cx = p->x; cv->p.cy = p->y;
    } else {
	cv->info.x = cv->p.cx;
	cv->info.y = cv->p.cy;
    }
    cv->info_within = true;
    CVInfoDraw(cv,cv->gw);
    CVSetConstrainPoint(cv,event);

    switch ( cv->active_tool ) {
      case cvt_pointer:
	CVMouseDownPointer(cv, &fs, event);
	cv->lastselpt = fs.p->sp;
      break;
      case cvt_magnify: case cvt_minify:
      break;
      case cvt_hand:
	CVMouseDownHand(cv);
      break;
      case cvt_freehand:
	CVMouseDownFreeHand(cv,event);
      break;
      case cvt_curve: case cvt_corner: case cvt_tangent: case cvt_pen:
	CVMouseDownPoint(cv,event);
      break;
      case cvt_ruler:
	CVMouseDownRuler(cv,event);
      break;
      case cvt_rotate: case cvt_flip: case cvt_scale: case cvt_skew:
      case cvt_3d_rotate: case cvt_perspective:
	CVMouseDownTransform(cv);
      break;
      case cvt_knife:
	CVMouseDownKnife(cv);
      break;
      case cvt_rect: case cvt_elipse: case cvt_poly: case cvt_star:
	CVMouseDownShape(cv,event);
      break;
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static int _SCRefNumberPoints2(SplineSet **_rss,SplineChar *sc,int pnum) {
    SplineSet *ss, *rss = *_rss;
    SplinePoint *sp, *rsp;
    RefChar *r;
    int starts_with_cp, startcnt;

    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next, rss=rss->next ) {
	if ( rss==NULL )		/* Can't happen */
    break;
	starts_with_cp = !ss->first->noprevcp &&
		((ss->first->ttfindex == pnum+1 && ss->first->prev!=NULL &&
		    ss->first->prev->from->nextcpindex==pnum ) ||
		 ((ss->first->ttfindex==0xffff || SPInterpolate( ss->first ))));
	startcnt = pnum;
	if ( starts_with_cp ) ++pnum;
	for ( sp = ss->first, rsp=rss->first; ; ) {
	    if ( sp->ttfindex==0xffff || SPInterpolate( sp ))
		rsp->ttfindex = 0xffff;
	    else
		rsp->ttfindex = pnum++;
	    if ( sp->next==NULL )
	break;
	    if ( sp->next->to == ss->first ) {
		if ( sp->nonextcp )
		    rsp->nextcpindex = 0xffff;
		else if ( starts_with_cp )
		    rsp->nextcpindex = startcnt;
		else
		    rsp->nextcpindex = pnum++;
	break;
	    }
	    if ( sp->nonextcp )
		rsp->nextcpindex = 0xffff;
	    else
		rsp->nextcpindex = pnum++;
	    sp = sp->next->to;
	    rsp = rsp->next->to;
	}
    }

    *_rss = rss;
    for ( r = sc->layers[ly_fore].refs; r!=NULL; r=r->next )
	pnum = _SCRefNumberPoints2(_rss,r->sc,pnum);
return( pnum );
}

static int SCRefNumberPoints2(RefChar *ref,int pnum) {
    SplineSet *rss;

    rss = ref->layers[0].splines;
return( _SCRefNumberPoints2(&rss,ref->sc,pnum));
}

int SSTtfNumberPoints(SplineSet *ss) {
    int pnum=0;
    SplinePoint *sp;
    int starts_with_cp;

    for ( ; ss!=NULL; ss=ss->next ) {
	starts_with_cp = !ss->first->noprevcp &&
		((ss->first->ttfindex == pnum+1 && ss->first->prev!=NULL &&
		  ss->first->prev->from->nextcpindex==pnum ) ||
		 SPInterpolate( ss->first ));
	if ( starts_with_cp && ss->first->prev!=NULL )
	    ss->first->prev->from->nextcpindex = pnum++;
	for ( sp=ss->first; ; ) {
	    if ( SPInterpolate(sp) )
		sp->ttfindex = 0xffff;
	    else
		sp->ttfindex = pnum++;
	    if ( sp->nonextcp )
		sp->nextcpindex = 0xffff;
	    else if ( !starts_with_cp || (sp->next!=NULL && sp->next->to!=ss->first) )
		sp->nextcpindex = pnum++;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
return( pnum );
}

int SCNumberPoints(SplineChar *sc) {
    int pnum=0;
    SplineSet *ss;
    SplinePoint *sp;
    RefChar *ref;

    if ( sc->parent->order2 ) {		/* TrueType and its complexities. I ignore svg here */
	if ( sc->layers[ly_fore].refs!=NULL ) {
	    /* if there are references there can't be splines. So if we've got*/
	    /*  splines mark all point numbers on them as meaningless */
	    for ( ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
		for ( sp=ss->first; ; ) {
		    sp->ttfindex = 0xfffe;
		    if ( !sp->nonextcp )
			sp->nextcpindex = 0xfffe;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==ss->first )
		break;
		}
	    }
	    for ( ref = sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
		pnum = SCRefNumberPoints2(ref,pnum);
	} else {
	    pnum = SSTtfNumberPoints(sc->layers[ly_fore].splines);
	}
    } else {		/* cubic (PostScript/SVG) splines */
	int layer;
	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	    for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
		for ( sp=ss->first; ; ) {
		    sp->ttfindex = pnum++;
		    sp->nextcpindex = 0xffff;
		    if ( sc->numberpointsbackards ) {
			if ( sp->prev==NULL )
		break;
			if ( !sp->noprevcp || !sp->prev->from->nonextcp )
			    pnum += 2;
			sp = sp->prev->from;
		    } else {
			if ( sp->next==NULL )
		break;
			if ( !sp->nonextcp || !sp->next->to->noprevcp )
			    pnum += 2;
			sp = sp->next->to;
		    }
		    if ( sp==ss->first )
		break;
		}
	    }
	}
    }
return( pnum );
}

int SCPointsNumberedProperly(SplineChar *sc) {
    int pnum=0, skipit;
    SplineSet *ss;
    SplinePoint *sp;
    int starts_with_cp;
    int start_pnum;

    if ( sc->layers[ly_fore].splines!=NULL &&
	    sc->layers[ly_fore].refs!=NULL )
return( false );	/* TrueType can't represent this, so always remove instructions. They can't be meaningful */

    for ( ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	starts_with_cp = (ss->first->ttfindex == pnum+1 || ss->first->ttfindex==0xffff) &&
		!ss->first->noprevcp;
	start_pnum = pnum;
	if ( starts_with_cp ) ++pnum;
	for ( sp=ss->first; ; ) {
	    skipit = SPInterpolate(sp);
	    if ( sp->nonextcp || sp->noprevcp ) skipit = false;
	    if ( sp->ttfindex==0xffff && skipit )
		/* Doesn't count */;
	    else if ( sp->ttfindex!=pnum )
return( false );
	    else
		++pnum;
	    if ( sp->nonextcp && sp->nextcpindex==0xffff )
		/* Doesn't count */;
	    else if ( sp->nextcpindex==pnum )
		++pnum;
	    else if ( sp->nextcpindex==start_pnum && starts_with_cp &&
		    (sp->next!=NULL && sp->next->to==ss->first) )
		/* Ok */;
	    else
return( false );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
	if ( starts_with_cp ) --pnum;
    }
return( true );
}

static void instrcheck(SplineChar *sc) {
    uint8 *instrs = sc->ttf_instrs==NULL && sc->parent->mm!=NULL && sc->parent->mm->apple ?
		sc->parent->mm->normal->glyphs[sc->orig_pos]->ttf_instrs : sc->ttf_instrs;
    struct splinecharlist *dep;
    int any_ptnumbers_shown = false;
    CharView *cv;
    extern int clear_tt_instructions_when_needed;
    SplineSet *ss;
    SplinePoint *sp;
    AnchorPoint *ap;
    int had_ap, had_dep, had_instrs;

    if ( !sc->parent->order2 )
return;

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    for ( cv=sc->views; cv!=NULL; cv=cv->next )
	if ( cv->showpointnumbers ) {
	    any_ptnumbers_shown = true;
    break;
	}
#endif	/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    if ( sc->instructions_out_of_date && !any_ptnumbers_shown && sc->anchor==NULL )
return;
    if ( instrs==NULL && sc->dependents==NULL && !any_ptnumbers_shown && sc->anchor==NULL )
return;
    /* If the points are no longer in order then the instructions are not valid */
    /*  (because they'll refer to the wrong points) and should be removed */
    /* Except that annoys users who don't expect it */
    had_ap = had_dep = had_instrs = 0;
    if ( !SCPointsNumberedProperly(sc)) {
	if ( instrs!=NULL ) {
	    if ( clear_tt_instructions_when_needed ) {
		free(sc->ttf_instrs); sc->ttf_instrs = NULL;
		sc->ttf_instrs_len = 0;
# ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		SCMarkInstrDlgAsChanged(sc);
# endif	/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
		had_instrs = 1;
	    } else {
		sc->instructions_out_of_date = true;
		had_instrs = 2;
	    }
	}
	for ( dep=sc->dependents; dep!=NULL; dep=dep->next ) {
	    RefChar *ref;
	    if ( dep->sc->ttf_instrs_len!=0 ) {
		if ( clear_tt_instructions_when_needed ) {
		    free(dep->sc->ttf_instrs); dep->sc->ttf_instrs = NULL;
		    dep->sc->ttf_instrs_len = 0;
# ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		    SCMarkInstrDlgAsChanged(dep->sc);
# endif	/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
		    had_instrs = 1;
		} else {
		    dep->sc->instructions_out_of_date = true;
		    had_instrs = 2;
		}
	    }
	    for ( ref=dep->sc->layers[ly_fore].refs; ref!=NULL && ref->sc!=sc; ref=ref->next );
	    for ( ; ref!=NULL ; ref=ref->next ) {
#if 0
		ref->point_match = false;
#endif
		if ( ref->point_match )
		    ref->point_match_out_of_date = true;
		had_dep = true;
	    }
	}
	SCNumberPoints(sc);
	for ( ap=sc->anchor ; ap!=NULL; ap=ap->next ) {
	    if ( ap->has_ttf_pt ) {
		had_ap = true;
		ap->has_ttf_pt = false;
		for ( ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
		    for ( sp=ss->first; ; ) {
			if ( sp->me.x==ap->me.x && sp->me.y==ap->me.y && sp->ttfindex!=0xffff ) {
			    ap->has_ttf_pt = true;
			    ap->ttf_pt_index = sp->ttfindex;
		goto found;
			} else if ( sp->nextcp.x==ap->me.x && sp->nextcp.y==ap->me.y && sp->nextcpindex!=0xffff ) {
			    ap->has_ttf_pt = true;
			    ap->ttf_pt_index = sp->nextcpindex;
		goto found;
			}
			if ( sp->next==NULL )
		    break;
			sp = sp->next->to;
			if ( sp==ss->first )
		    break;
		    }
		}
		found: ;
	    }
	}
    }
    if ( no_windowing_ui )
	/* If we're in a script it's annoying (and pointless) to get this message */;
    else if ( sc->complained_about_ptnums )
	/* It's annoying to get the same message over and over again as you edit a glyph */;
    else if ( had_ap || had_dep || had_instrs ) {
	ff_post_notice(_("You changed the point numbering"),
		_("You have just changed the point numbering of glyph %s.%s%s%s"),
			sc->name,
			had_instrs==0 ? "" :
			had_instrs==1 ? _(" Instructions in this glyph (or one that refers to it) have been lost.") :
			                _(" Instructions in this glyph (or one that refers to it) are now out of date."),
			had_dep ? _(" At least one reference to this glyph used point matching. That match is now out of date.")
				: "",
			had_ap ? _(" At least one anchor point used point matching. It may be out of date now.")
				: "" );
	sc->complained_about_ptnums = true;
	if ( had_instrs==2 )
	    GDrawRequestExpose(sc->parent->fv->v,NULL,false);
    }
}

static void _SCHintsChanged(SplineChar *sc) {
    struct splinecharlist *dlist;

    if ( !sc->changedsincelasthinted ) {
	sc->changedsincelasthinted = true;
	FVMarkHintsOutOfDate(sc);
    }

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	_SCHintsChanged(dlist->sc);
}

void SCHintsChanged(SplineChar *sc) {
    struct splinecharlist *dlist;
    int was = sc->changedsincelasthinted;

    if ( sc->parent->onlybitmaps || sc->parent->multilayer || sc->parent->strokedfont || sc->parent->order2 )
return;
    sc->changedsincelasthinted = false;		/* We just applied a hinting change */
    if ( !sc->changed ) {
	sc->changed = true;
	FVToggleCharChanged(sc);
	SCRefreshTitles(sc);
	if ( !sc->parent->changed ) {
	    sc->parent->changed = true;
	    FVSetTitle(sc->parent->fv);
	}
    }
    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	_SCHintsChanged(dlist->sc);
    if ( was ) {
	FontView *fvs;
	for ( fvs = sc->parent->fv; fvs!=NULL; fvs=fvs->nextsame )
	    GDrawRequestExpose(fvs->v,NULL,false);
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void CVSetCharChanged(CharView *cv,int changed) {
    FontView *fv = cv->fv;
    SplineFont *sf = fv->sf;
    SplineChar *sc = cv->sc;
    int oldchanged = sf->changed;

    if ( changed )
	SFSetModTime(sf);
    if ( cv->drawmode==dm_grid ) {
	if ( changed ) {
	    sf->changed = true;
	    if ( fv->cidmaster!=NULL )
		fv->cidmaster->changed = true;
	}
    } else {
	if ( cv->drawmode==dm_fore )
	    sf->onlybitmaps = false;
	if ( sc->changed != changed ) {
	    sc->changed = changed;
	    FVToggleCharChanged(sc);
	    SCRefreshTitles(sc);
	    if ( changed ) {
		sf->changed = true;
		if ( fv->cidmaster!=NULL )
		    fv->cidmaster->changed = true;
	    }
	}
	if ( changed ) {
	    instrcheck(sc);
	    SCDeGridFit(sc);
	    if ( sc->parent->onlybitmaps )
		/* Do nothing */;
	    else if ( sc->parent->multilayer || sc->parent->strokedfont || sc->parent->order2 )
		sc->changed_since_search = true;
	    else if ( cv->drawmode==dm_fore )
		sc->changed_since_search = sc->changedsincelasthinted = true;
	    sc->changed_since_autosave = true;
	    sf->changed_since_autosave = true;
	    sf->changed_since_xuidchanged = true;
	    if ( fv->cidmaster!=NULL ) {
		fv->cidmaster->changed_since_autosave = true;
		fv->cidmaster->changed_since_xuidchanged = true;
	    }
	    _SCHintsChanged(cv->sc);
	}
	if ( cv->drawmode==dm_fore ) {
	    cv->needsrasterize = true;
	}
    }
    cv->recentchange = true;
    if ( !oldchanged )
	FVSetTitle(fv);
}

void SCClearSelPt(SplineChar *sc) {
    CharView *cv;

    for ( cv=sc->views; cv!=NULL; cv=cv->next ) {
	cv->lastselpt = cv->p.sp = NULL;
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static void TTFPointMatches(SplineChar *sc,int top) {
    AnchorPoint *ap;
    BasePoint here, there;
    struct splinecharlist *deps;
    RefChar *ref;

    if ( !sc->parent->order2 )
return;
    for ( ap=sc->anchor ; ap!=NULL; ap=ap->next ) {
	if ( ap->has_ttf_pt )
	    if ( ttfFindPointInSC(sc,ap->ttf_pt_index,&ap->me,NULL)!=-1 )
		ap->has_ttf_pt = false;
    }
    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->point_match ) {
	    if ( ttfFindPointInSC(sc,ref->match_pt_base,&there,ref)==-1 &&
		    ttfFindPointInSC(ref->sc,ref->match_pt_ref,&here,NULL)==-1 ) {
		if ( ref->transform[4]!=there.x-here.x ||
			ref->transform[5]!=there.y-here.y ) {
		    ref->transform[4] = there.x-here.x;
		    ref->transform[5] = there.y-here.y;
		    SCReinstanciateRefChar(sc,ref);
		    if ( !top )
			_SCCharChangedUpdate(sc,true);
		}
	    } else
		ref->point_match = false;		/* one of the points no longer exists */
	}
    }
    for ( deps = sc->dependents; deps!=NULL; deps = deps->next )
	TTFPointMatches(deps->sc,false);
}

void _SCCharChangedUpdate(SplineChar *sc,int changed) {
    SplineFont *sf = sc->parent;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontView *fv;
    extern int updateflex;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    TTFPointMatches(sc,true);
    if ( changed != -1 ) {
	sc->changed_since_autosave = true;
	SFSetModTime(sf);
	if ( sc->changed!=changed ) {
	    sc->changed = changed;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    FVToggleCharChanged(sc);
	    SCRefreshTitles(sc);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !sf->changed && sf->fv!=NULL )
	    FVSetTitle(sf->fv);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	if ( changed ) {
	    instrcheck(sc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    SCDeGridFit(sc);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	}
	if ( !sc->parent->onlybitmaps && !sc->parent->multilayer && !sc->parent->strokedfont && !sc->parent->order2 )
	    sc->changedsincelasthinted = true;
	sc->changed_since_search = true;
	sf->changed = true;
	sf->changed_since_autosave = true;
	sf->changed_since_xuidchanged = true;
	_SCHintsChanged(sc);
    }
    if ( sf->cidmaster!=NULL )
	sf->cidmaster->changed = sf->cidmaster->changed_since_autosave =
		sf->cidmaster->changed_since_xuidchanged = true;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    SCRegenDependents(sc);		/* All chars linked to this one need to get the new splines */
    if ( updateflex && sc->views!=NULL )
	SplineCharIsFlexible(sc);
    SCUpdateAll(sc);
# ifdef FONTFORGE_CONFIG_TYPE3
    SCLayersChange(sc);
# endif
    SCRegenFills(sc);
    for ( fv = sf->fv; fv!=NULL; fv=fv->nextsame )
	FVRegenChar(fv,sc);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}

void SCCharChangedUpdate(SplineChar *sc) {
    _SCCharChangedUpdate(sc,true);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void _CVCharChangedUpdate(CharView *cv,int changed) {
    extern int updateflex;
    FontView *fv;

    CVSetCharChanged(cv,changed);
#ifdef FONTFORGE_CONFIG_TYPE3
    CVLayerChange(cv);
#endif
    if ( cv->needsrasterize ) {
	TTFPointMatches(cv->sc,true);		/* Must precede regen dependents, as this can change references */
	SCRegenDependents(cv->sc);		/* All chars linked to this one need to get the new splines */
	if ( updateflex )
	    SplineCharIsFlexible(cv->sc);
	SCUpdateAll(cv->sc);
	SCRegenFills(cv->sc);
	for ( fv = cv->sc->parent->fv; fv!=NULL; fv=fv->nextsame )
	    FVRegenChar(fv,cv->sc);
	cv->needsrasterize = false;
    } else if ( cv->drawmode==dm_back ) {
	/* If we changed the background then only views of this character */
	/*  need to know about it. No dependents needed, but why write */
	/*  another routine for a rare case... */
	SCUpdateAll(cv->sc);
    } else if ( cv->drawmode==dm_grid ) {
	/* If we changed the grid then any character needs to know it */
	FVRedrawAllCharViews(cv->fv);
    }
    cv->recentchange = false;
    cv->p.sp = NULL;		/* Might have been deleted */
}

void CVCharChangedUpdate(CharView *cv) {
    _CVCharChangedUpdate(cv,true);
}

static void CVMouseMove(CharView *cv, GEvent *event ) {
    real cx, cy;
    PressedOn p;
    FindSel fs;
    GEvent fake;
    int stop_motion = false;

#if 0		/* Debug wacom !!!! */
 printf( "dev=%s (%d,%d) 0x%x\n", event->u.mouse.device!=NULL?event->u.mouse.device:"<None>",
     event->u.mouse.x, event->u.mouse.y, event->u.mouse.state);
#endif

    if ( event->u.mouse.device!=NULL )
	CVToolsSetCursor(cv,event->u.mouse.state,event->u.mouse.device);

    if ( !cv->p.pressed ) {
	CVUpdateInfo(cv, event);
	if ( cv->showing_tool == cvt_pointer ) {
	    CVCheckResizeCursors(cv);
	    if ( cv->dv!=NULL )
		CVDebugPointPopup(cv);
	} else if ( cv->showing_tool == cvt_ruler )
	    CVMouseMoveRuler(cv,event);
return;
    }

    SetFS(&fs,&p,cv,event);
    if ( cv->active_tool == cvt_freehand )
	/* freehand does it's own kind of constraining */;
    else if ( (event->u.mouse.state&ksm_shift) && !cv->p.rubberbanding ) {
	/* Constrained */

	fake.u.mouse = event->u.mouse;
	if ( ((event->u.mouse.state&ksm_alt) ||
		    (!cv->cntrldown && (event->u.mouse.state&ksm_control))) &&
		(cv->p.nextcp || cv->p.prevcp)) {
	    real dot = (cv->p.cp.x-cv->p.constrain.x)*(p.cx-cv->p.constrain.x) +
		    (cv->p.cp.y-cv->p.constrain.y)*(p.cy-cv->p.constrain.y);
	    real len = (cv->p.cp.x-cv->p.constrain.x)*(cv->p.cp.x-cv->p.constrain.x)+
		    (cv->p.cp.y-cv->p.constrain.y)*(cv->p.cp.y-cv->p.constrain.y);
	    if ( len!=0 ) {
		dot /= len;
		/* constrain control point to same angle with respect to base point*/
		if ( dot<0 ) dot = 0;
		p.cx = cv->p.constrain.x + dot*(cv->p.cp.x-cv->p.constrain.x);
		p.cy = cv->p.constrain.y + dot*(cv->p.cp.y-cv->p.constrain.y);
		p.x = fake.u.mouse.x = cv->xoff + rint(p.cx*cv->scale);
		p.y = fake.u.mouse.y = -cv->yoff + cv->height - rint(p.cy*cv->scale);
	    }
	} else {
	    /* Constrain mouse to hor/vert/45 from base point */
	    int basex = cv->active_tool!=cvt_hand ? cv->xoff + rint(cv->p.constrain.x*cv->scale) : cv->p.x;
	    int basey = cv->active_tool!=cvt_hand ?-cv->yoff + cv->height - rint(cv->p.constrain.y*cv->scale) : cv->p.y;
	    int dx= event->u.mouse.x-basex, dy = event->u.mouse.y-basey;
	    int sign = dx*dy<0?-1:1;
	    double aspect = 1.0;

	    if ( dx<0 ) dx = -dx; if ( dy<0 ) dy = -dy;
	    if ( cv->p.img!=NULL && cv->p.img->bb.minx!=cv->p.img->bb.maxx )
		aspect = (cv->p.img->bb.maxy - cv->p.img->bb.miny) / (cv->p.img->bb.maxx - cv->p.img->bb.minx);
	    else if ( cv->p.ref!=NULL && cv->p.ref->bb.minx!=cv->p.ref->bb.maxx )
		aspect = (cv->p.ref->bb.maxy - cv->p.ref->bb.miny) / (cv->p.ref->bb.maxx - cv->p.ref->bb.minx);
	    if ( dy >= 2*dx ) {
		p.x = fake.u.mouse.x = basex;
		p.cx = cv->p.constrain.x;
		if ( ItalicConstrained && cv->sc->parent->italicangle!=0 ) {
		    double off = tan(cv->sc->parent->italicangle*3.1415926535897932/180)*
			    (p.cy-cv->p.constrain.y);
		    double aoff = off<0 ? -off : off;
		    if ( dx>=aoff*cv->scale/2 && (event->u.mouse.x-basex<0)!=(off<0) ) {
			p.cx -= off;
			p.x = fake.u.mouse.x = cv->xoff + rint(p.cx*cv->scale);
		    }
		}
	    } else if ( dx >= 2*dy ) {
		p.y = fake.u.mouse.y = basey;
		p.cy = cv->p.constrain.y;
	    } else if ( dx > dy ) {
		p.x = fake.u.mouse.x = basex + sign * (event->u.mouse.y-basey)/aspect;
		p.cx = cv->p.constrain.x - sign * (p.cy-cv->p.constrain.y)/aspect;
	    } else {
		p.y = fake.u.mouse.y = basey + sign * (event->u.mouse.x-basex)*aspect;
		p.cy = cv->p.constrain.y - sign * (p.cx-cv->p.constrain.x)*aspect;
	    }
	}
	event = &fake;
    }

    /* If we've changed the character (recentchange is true) we want to */
    /*  snap to the original location, otherwise we'll keep snapping to the */
    /*  current point as it moves across the screen (jerkily) */
    if ( cv->active_tool == cvt_hand || cv->active_tool == cvt_freehand )
	/* Don't snap to points */;
    else if ( cv->active_tool == cvt_pointer &&
	    ( cv->p.nextcp || cv->p.prevcp))
	/* Don't snap to points when moving control points */;
    else if ( !cv->joinvalid || !CheckPoint(&fs,&cv->joinpos,NULL)) {
	SplinePointList *spl;
	spl = cv->layerheads[cv->drawmode]->splines;
	if ( cv->recentchange && cv->active_tool==cvt_pointer &&
		cv->layerheads[cv->drawmode]->undoes!=NULL &&
		(cv->layerheads[cv->drawmode]->undoes->undotype==ut_state ||
		 cv->layerheads[cv->drawmode]->undoes->undotype==ut_tstate ))
	    spl = cv->layerheads[cv->drawmode]->undoes->u.state.splines;
	if ( cv->active_tool != cvt_knife )
	    NearSplineSetPoints(&fs,spl);
	else 
	    InSplineSet(&fs,spl);
    }
    if ( p.sp!=NULL && p.sp!=cv->active_sp ) {		/* Snap to points */
	p.cx = p.sp->me.x;
	p.cy = p.sp->me.y;
    } else {
	CVDoSnaps(cv,&fs);
    }
    cx = (p.cx -cv->p.cx) / cv->scale;
    cy = (p.cy - cv->p.cy) / cv->scale;
    if ( cx<0 ) cx = -cx;
    if ( cy<0 ) cy = -cy;

	/* If they haven't moved far from the start point, then snap to it */
    if ( cx+cy < 4 ) {
	p.x = cv->p.x; p.y = cv->p.y;
    }

    cv->info.x = p.cx; cv->info.y = p.cy;
    cv->info_sp = p.sp;
    cv->e.x = event->u.mouse.x; cv->e.y = event->u.mouse.y;
    CVInfoDraw(cv,cv->gw);

    switch ( cv->active_tool ) {
      case cvt_pointer:
	stop_motion = CVMouseMovePointer(cv,event);
      break;
      case cvt_magnify: case cvt_minify:
	if ( !cv->p.rubberbanding ) {
	    cv->p.ex = cv->p.cx;
	    cv->p.ey = cv->p.cy;
	}
	if ( cv->p.rubberbanding )
	    CVDrawRubberRect(cv->v,cv);
	cv->p.ex = cv->info.x;
	cv->p.ey = cv->info.y;
	cv->p.rubberbanding = true;
	CVDrawRubberRect(cv->v,cv);
      break;
      case cvt_hand:
	CVMouseMoveHand(cv,event);
      break;
      case cvt_freehand:
	CVMouseMoveFreeHand(cv,event);
      break;
      case cvt_curve: case cvt_corner: case cvt_tangent: 
	CVMouseMovePoint(cv,&p);
      break;
      case cvt_pen:
	CVMouseMovePen(cv,&p,event);
      break;
      case cvt_ruler:
	CVMouseMoveRuler(cv,event);
      break;
      case cvt_rotate: case cvt_flip: case cvt_scale: case cvt_skew:
      case cvt_3d_rotate: case cvt_perspective:
	CVMouseMoveTransform(cv);
      break;
      case cvt_knife:
	CVMouseMoveKnife(cv,&p);
      break;
      case cvt_rect: case cvt_elipse: case cvt_poly: case cvt_star:
	CVMouseMoveShape(cv);
      break;
    }
    if ( stop_motion ) {
	event->type = et_mouseup;
	CVMouseUp(cv,event);
    }
}

static void CVMagnify(CharView *cv, real midx, real midy, int bigger) {
    static float scales[] = { 1, 2, 3, 4, 6, 8, 11, 16, 23, 32, 45, 64, 90, 128, 181, 256, 512, 1024, 0 };
    int i, j;

    if ( bigger!=0 ) {
	if ( cv->scale>=1 ) {
	    for ( i=0; scales[i]!=0 && cv->scale>scales[i]; ++i );
	    if ( scales[i]==0 ) i=j= i-1;
	    else if ( RealNear(scales[i],cv->scale) ) j=i;
	    else if ( i!=0 && RealNear(scales[i-1],cv->scale) ) j= --i; /* Round errors! */
	    else j = i-1;
	} else { real sinv = 1/cv->scale; int t;
	    for ( i=0; scales[i]!=0 && sinv>scales[i]; ++i );
	    if ( scales[i]==0 ) i=j= i-1;
	    else if ( RealNear(scales[i],sinv) ) j=i;
	    else if ( i!=0 && RealNear(scales[i-1],sinv) ) j= --i; /* Round errors! */
	    else j = i-1;
	    t = j;
	    j = -i; i = -t;
	}
	if ( bigger==1 ) {
	    if ( i==j ) ++i;
	    if ( i>0 && scales[i]==0 ) --i;
	    if ( i>=0 )
		cv->scale = scales[i];
	    else
		cv->scale = 1/scales[-i];
	} else {
	    if ( i==j ) --j;
	    if ( j<0 && scales[-j]==0 ) ++j;
	    if ( j>=0 )
		cv->scale = scales[j];
	    else
		cv->scale = 1/scales[-j];
	}
    }
    cv->xoff = -(rint(midx*cv->scale) - cv->width/2);
    cv->yoff = -(rint(midy*cv->scale) - cv->height/2);
    CVNewScale(cv);
}

static void CVMouseUp(CharView *cv, GEvent *event ) {

    CVMouseMove(cv,event);
    if ( cv->pressed!=NULL ) {
	GDrawCancelTimer(cv->pressed);
	cv->pressed = NULL;
    }
    cv->p.pressed = false;

    if ( cv->p.rubberbanding ) {
	CVDrawRubberRect(cv->v,cv);
	cv->p.rubberbanding = false;
    } else if ( cv->p.rubberlining ) {
	CVDrawRubberLine(cv->v,cv);
	cv->p.rubberlining = false;
    }
    switch ( cv->active_tool ) {
      case cvt_pointer:
	CVMouseUpPointer(cv);
      break;
      case cvt_ruler:
	CVMouseUpRuler(cv,event);
      break;
      case cvt_hand:
	CVMouseUpHand(cv);
      break;
      case cvt_freehand:
	CVMouseUpFreeHand(cv,event);
      break;
      case cvt_curve: case cvt_corner: case cvt_tangent: case cvt_pen:
	CVMouseUpPoint(cv,event);
      break;
      case cvt_magnify: case cvt_minify:
	if ( cv->p.x>=event->u.mouse.x-6 && cv->p.x<=event->u.mouse.x+6 &&
		 cv->p.y>=event->u.mouse.y-6 && cv->p.y<=event->u.mouse.y+6 ) {
	    real cx, cy;
	    cx = (event->u.mouse.x-cv->xoff)/cv->scale ;
	    cy = (cv->height-event->u.mouse.y-cv->yoff)/cv->scale ;
	    CVMagnify(cv,cx,cy,cv->active_tool==cvt_minify?-1:1);
        } else {
	    DBounds b;
	    double oldscale = cv->scale;
	    if ( cv->p.cx>cv->info.x ) {
		b.minx = cv->info.x;
		b.maxx = cv->p.cx;
	    } else {
		b.minx = cv->p.cx;
		b.maxx = cv->info.x;
	    }
	    if ( cv->p.cy>cv->info.y ) {
		b.miny = cv->info.y;
		b.maxy = cv->p.cy;
	    } else {
		b.miny = cv->p.cy;
		b.maxy = cv->info.y;
	    }
	    _CVFit(cv,&b);
	    if ( oldscale==cv->scale ) {
		cv->scale += .5;
		CVNewScale(cv);
	    }
	}
      break;
      case cvt_rotate: case cvt_flip: case cvt_scale: case cvt_skew:
      case cvt_3d_rotate: case cvt_perspective:
	CVMouseUpTransform(cv);
      break;
      case cvt_knife:
	CVMouseUpKnife(cv,event);
      break;
      case cvt_rect: case cvt_elipse: case cvt_poly: case cvt_star:
	CVMouseUpShape(cv);
      break;
    }
    cv->active_tool = cvt_none;
    CVToolsSetCursor(cv,event->u.mouse.state&~(1<<(7+event->u.mouse.button)),event->u.mouse.device);		/* X still has the buttons set in the state, even though we just released them. I don't want em */
    if ( cv->needsrasterize || cv->recentchange )
	CVCharChangedUpdate(cv);
}

static void CVTimer(CharView *cv,GEvent *event) {

    if ( event->u.timer.timer==cv->pressed ) {
	GEvent e;
	GDrawGetPointerPosition(cv->v,&e);
	if ( e.u.mouse.x<0 || e.u.mouse.y<0 ||
		e.u.mouse.x>=cv->width || e.u.mouse.y >= cv->height ) {
	    real dx = 0, dy = 0;
	    if ( e.u.mouse.x<0 )
		dx = cv->width/8;
	    else if ( e.u.mouse.x>=cv->width )
		dx = -cv->width/8;
	    if ( e.u.mouse.y<0 )
		dy = -cv->height/8;
	    else if ( e.u.mouse.y>=cv->height )
		dy = cv->height/8;
	    cv->xoff += dx; cv->yoff += dy;
	    cv->back_img_out_of_date = true;
	    if ( dy!=0 )
		GScrollBarSetPos(cv->vsb,cv->yoff-cv->height);
	    if ( dx!=0 )
		GScrollBarSetPos(cv->hsb,-cv->xoff);
	    GDrawRequestExpose(cv->v,NULL,false);
	}
#if _ModKeysAutoRepeat
	/* Under cygwin the modifier keys auto repeat, they don't under normal X */
    } else if ( cv->autorpt==event->u.timer.timer ) {
	cv->autorpt = NULL;
	CVToolsSetCursor(cv,cv->oldstate,NULL);
	if ( cv->keysym == GK_Shift_L || cv->keysym == GK_Shift_R ||
		 cv->keysym == GK_Alt_L || cv->keysym == GK_Alt_R ||
		 cv->keysym == GK_Meta_L || cv->keysym == GK_Meta_R ) {
	    GEvent e;
	    e.w = cv->oldkeyw;
	    e.u.chr.keysym = cv->keysym;
	    e.u.chr.x = cv->oldkeyx;
	    e.u.chr.y = cv->oldkeyy;
	    CVFakeMove(cv,&e);
	}
#endif
    }
}

static void CVDrop(CharView *cv,GEvent *event) {
    /* We should get a list of character names. Add each as a RefChar */
    int32 len;
    int ch, first = true;
    char *start, *pt, *cnames;
    SplineChar *rsc;
    RefChar *new;

    if ( cv->drawmode!=dm_fore ) {
	gwwv_post_error(_("Not Foreground"),_("References may be dragged only to the foreground layer"));
return;
    }
    if ( !GDrawSelectionHasType(cv->gw,sn_drag_and_drop,"STRING"))
return;
    cnames = GDrawRequestSelection(cv->gw,sn_drag_and_drop,"STRING",&len);
    if ( cnames==NULL )
return;

    start = cnames;
    while ( *start ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	if ( (rsc=SFGetChar(cv->sc->parent,-1,start))!=NULL && rsc!=cv->sc ) {
	    if ( first ) {
		CVPreserveState(cv);
		first =false;
	    }
	    new = RefCharCreate();
	    new->transform[0] = new->transform[3] = 1.0;
	    new->layers[0].splines = NULL;
	    new->sc = rsc;
	    new->next = cv->sc->layers[ly_fore].refs;
	    cv->sc->layers[ly_fore].refs = new;
	    SCReinstanciateRefChar(cv->sc,new);
	    SCMakeDependent(cv->sc,rsc);
	}
	*pt = ch;
	start = pt;
    }

    free(cnames);
    CVCharChangedUpdate(cv);
}

#ifdef TEST_O2
static int o2_cp_fixup( SplineSet *spl) {
    SplinePoint *sp, *n;
    int ret = false, spi, ni;

    while ( spl!=NULL ) {
	for ( sp=spl->first ; ; ) {
	    if ( sp->next==NULL )
	break;
	    n = sp->next->to;
	    if ( !sp->next->order2 )
		/* No constraints on cubics */;
	    else if ( sp->nonextcp && n->noprevcp )
		/* A line is ok */;
	    else if ( RealNear(sp->nextcp.x,n->prevcp.x) &&
		    RealNear(sp->nextcp.y,n->prevcp.y))
		/* That's ok as well */;
	    else {
		ret = true;
		fprintf( stderr, "Control point between (%g,%g) and (%g,%g) is wrong.\n",
			sp->me.x, sp->me.y, n->me.x, n->me.y );
		spi = SPInterpolate(sp);
		ni = SPInterpolate(n);
		sp->nextcp.x = (sp->nextcp.x+n->prevcp.x)/2;
		sp->nextcp.y = (sp->nextcp.y+n->prevcp.y)/2;
		n->prevcp = sp->nextcp;
		if ( spi ) {
		    sp->me.x = (sp->prevcp.x + sp->nextcp.x)/2;
		    sp->me.y = (sp->prevcp.y + sp->nextcp.y)/2;
		}
		if ( ni ) {
		    n->me.x = (n->prevcp.x + n->nextcp.x)/2;
		    n->me.y = (n->prevcp.y + n->nextcp.y)/2;
		}
	    }
	    sp = n;
	    if ( sp==spl->first )
	break;
	}
	spl = spl->next;
    }
return( ret );
}
#endif

static int v_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    GGadgetPopupExternalEvent(event);
    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
	if ( !(event->u.mouse.state&(ksm_shift|ksm_control)) )	/* bind shift to magnify/minify */
return( GGadgetDispatchEvent(cv->vsb,event));
    }

    switch ( event->type ) {
      case et_expose:
	GDrawSetLineWidth(gw,0);
	CVExpose(cv,gw,event);
#ifdef TEST_O2
	if ( o2_cp_fixup(cv->layerheads[cv->drawmode]->splines) )
	    IError("Mismatch control points" );
#endif
      break;
      case et_crossing:
	CVCrossing(cv,event);
      break;
      case et_mousedown:
	CVPaletteActivate(cv);
	GDrawSetGIC(gw,cv->gic,0,20);
	if ( cv->inactive )
	    SVMakeActive(cv->searcher,cv);
	CVMouseDown(cv,event);
      break;
      case et_mousemove:
	if ( cv->p.pressed )
	    GDrawSkipMouseMoveEvents(cv->v,event);
	CVMouseMove(cv,event);
      break;
      case et_mouseup:
	CVMouseUp(cv,event);
      break;
      case et_char:
	if ( cv->searcher!=NULL )
	    SVChar(cv->searcher,event);
	else
	    CVChar(cv,event);
      break;
      case et_charup:
	CVCharUp(cv,event);
      break;
      case et_timer:
	CVTimer(cv,event);
      break;
      case et_drop:
	CVDrop(cv,event);
      break;
      case et_focus:
	if ( event->u.focus.gained_focus ) {
	    GDrawSetGIC(gw,cv->gic,0,20);
#if 0
	    CVPaletteActivate(cv);
#endif
	}
      break;
    }
return( true );
}

static void CVDrawNum(CharView *cv,GWindow pixmap,int x, int y, char *format,real val, int align) {
    char buffer[40];
    unichar_t ubuf[40];
    int len;

    if ( val==0 ) val=0;		/* avoid -0 */
    sprintf(buffer,format,val);
    uc_strcpy(ubuf,buffer);
    if ( align!=0 ) {
	len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	if ( align==1 )
	    x-=len/2;
	else
	    x-=len;
    }
    GDrawDrawText(pixmap,x,y,ubuf,-1,NULL,GDrawGetDefaultForeground(NULL));
}

static void CVDrawVNum(CharView *cv,GWindow pixmap,int x, int y, char *format,real val, int align) {
    char buffer[40];
    unichar_t ubuf[40], *upt;
    int len;

    if ( val==0 ) val=0;		/* avoid -0 */
    sprintf(buffer,format,val);
    uc_strcpy(ubuf,buffer);
    if ( align!=0 ) {
	len = strlen(buffer)*cv->sfh;
	if ( align==1 )
	    y-=len/2;
	else
	    y-=len;
    }
    for ( upt=ubuf; *upt; ++upt ) {
	GDrawDrawText(pixmap,x,y,upt,1,NULL,GDrawGetDefaultForeground(NULL));
	y += cv->sfh;
    }
}


static void CVExposeRulers(CharView *cv, GWindow pixmap ) {
    real xmin, xmax, ymin, ymax;
    real onehundred, pos;
    real units, littleunits;
    int ybase = cv->mbh+cv->infoh;
    int x,y;
    GRect rect;
    Color def_fg = GDrawGetDefaultForeground(NULL);

    xmin = -cv->xoff/cv->scale;
    xmax = (cv->width-cv->xoff)/cv->scale;
    ymin = -cv->yoff/cv->scale;
    ymax = (cv->height-cv->yoff)/cv->scale;
    onehundred = 100/cv->scale;

    if ( onehundred<5 ) {
	units = 1; littleunits=0;
    } else if ( onehundred<10 ) {
	units = 5; littleunits=1;
    } else if ( onehundred<50 ) {
	units = 10; littleunits=2;
    } else if ( onehundred<100 ) {
	units = 25; littleunits=5;
    } else if ( onehundred<500 ) {
	units = 100; littleunits=20;
    } else if ( onehundred<1000 ) {
	units = 250; littleunits=50;
    } else if ( onehundred<5000 ) {
	units = 1000; littleunits=200;
    } else if ( onehundred<10000 ) {
	units = 2500; littleunits=500;
    } else if ( onehundred<10000 ) {
	units = 10000; littleunits=2000;
    } else {
	for ( units=1 ; units<onehundred; units *= 10 );
	units/=10; littleunits = units/2;
    }

    rect.x = 0; rect.width = cv->width+cv->rulerh; rect.y = ybase; rect.height = cv->rulerh;
    GDrawFillRect(pixmap,&rect,GDrawGetDefaultBackground(NULL));
    rect.y = ybase; rect.height = cv->height+cv->rulerh; rect.x = 0; rect.width = cv->rulerh;
    GDrawFillRect(pixmap,&rect,GDrawGetDefaultBackground(NULL));
    GDrawSetLineWidth(pixmap,0);
    GDrawDrawLine(pixmap,cv->rulerh,cv->mbh+cv->infoh+cv->rulerh-1,8096,cv->mbh+cv->infoh+cv->rulerh-1,def_fg);
    GDrawDrawLine(pixmap,cv->rulerh-1,cv->mbh+cv->infoh+cv->rulerh,cv->rulerh-1,8096,def_fg);

    GDrawSetFont(pixmap,cv->small);
    if ( xmax-xmin<1 && cv->width>100 ) {
	CVDrawNum(cv,pixmap,cv->rulerh,ybase+cv->sas,"%.3f",xmin,0);
	CVDrawNum(cv,pixmap,cv->rulerh+cv->width,ybase+cv->sas,"%.3f",xmax,2);
    }
    if ( ymax-ymin<1 && cv->height>100 ) {
	CVDrawVNum(cv,pixmap,1,ybase+cv->rulerh+cv->height+cv->sas,"%.3f",ymin,0);
	CVDrawVNum(cv,pixmap,1,ybase+cv->rulerh+cv->sas,"%.3f",ymax,2);
    }
    if ( fabs(xmin/units) < 1e5 && fabs(ymin/units)<1e5 && fabs(xmax/units)<1e5 && fabs(ymax/units)<1e5 ) {
	if ( littleunits!=0 ) {
	    for ( pos=littleunits*ceil(xmin/littleunits); pos<xmax; pos += littleunits ) {
		x = cv->xoff + rint(pos*cv->scale);
		GDrawDrawLine(pixmap,x+cv->rulerh,ybase+cv->rulerh-4,x+cv->rulerh,ybase+cv->rulerh, def_fg);
	    }
	    for ( pos=littleunits*ceil(ymin/littleunits); pos<ymax; pos += littleunits ) {
		y = -cv->yoff + cv->height - rint(pos*cv->scale);
		GDrawDrawLine(pixmap,cv->rulerh-4,ybase+cv->rulerh+y,cv->rulerh,ybase+cv->rulerh+y, def_fg);
	    }
	}
	for ( pos=units*ceil(xmin/units); pos<xmax; pos += units ) {
	    x = cv->xoff + rint(pos*cv->scale);
	    GDrawDrawLine(pixmap,x+cv->rulerh,ybase+cv->rulerh-6,x+cv->rulerh,ybase+cv->rulerh, def_fg);
	    CVDrawNum(cv,pixmap,x+cv->rulerh,ybase+cv->sas,"%g",pos,1);
	}
	for ( pos=units*ceil(ymin/units); pos<ymax; pos += units ) {
	    y = -cv->yoff + cv->height - rint(pos*cv->scale);
	    GDrawDrawLine(pixmap,cv->rulerh-6,ybase+cv->rulerh+y,cv->rulerh,ybase+cv->rulerh+y, def_fg);
	    CVDrawVNum(cv,pixmap,1,y+ybase+cv->rulerh+cv->sas,"%g",pos,1);
	}
    }
}

static void InfoExpose(CharView *cv, GWindow pixmap, GEvent *expose) {
    GRect old1, old2;
    Color def_fg = GDrawGetDefaultForeground(NULL);

    if ( expose->u.expose.rect.y + expose->u.expose.rect.height < cv->mbh ||
	    (!cv->showrulers && expose->u.expose.rect.y >= cv->mbh+cv->infoh ))
return;

    GDrawPushClip(pixmap,&expose->u.expose.rect,&old1);
    GDrawSetLineWidth(pixmap,0);
    if ( expose->u.expose.rect.y< cv->mbh+cv->infoh ) {
#if 0
	r.x = 0; r.width = 8096;
	r.y = cv->mbh; r.height = cv->infoh;
#endif
	GDrawPushClip(pixmap,&expose->u.expose.rect,&old2);
    
	GDrawDrawLine(pixmap,0,cv->mbh+cv->infoh-1,8096,cv->mbh+cv->infoh-1,def_fg);
	GDrawDrawImage(pixmap,&GIcon_rightpointer,NULL,RPT_BASE,cv->mbh+2);
	GDrawDrawImage(pixmap,&GIcon_selectedpoint,NULL,SPT_BASE,cv->mbh+2);
	GDrawDrawImage(pixmap,&GIcon_sel2ptr,NULL,SOF_BASE,cv->mbh+2);
	GDrawDrawImage(pixmap,&GIcon_distance,NULL,SDS_BASE,cv->mbh+2);
	GDrawDrawImage(pixmap,&GIcon_angle,NULL,SAN_BASE,cv->mbh+2);
	GDrawDrawImage(pixmap,&GIcon_mag,NULL,MAG_BASE,cv->mbh+2);
	CVInfoDrawText(cv,pixmap);
	GDrawPopClip(pixmap,&old2);
    }
    if ( cv->showrulers ) {
	CVExposeRulers(cv,pixmap);
	cv->olde.x = -1;
	CVInfoDrawRulers(cv,pixmap);
    }
    GDrawPopClip(pixmap,&old1);
}

void CVResize(CharView *cv ) {
    int sbsize = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
    GRect size;
    GDrawGetSize(cv->gw,&size);
    {
	int newwidth = size.width-sbsize,
	    newheight = size.height-sbsize - cv->mbh-cv->infoh;
	int sbwidth = newwidth, sbheight = newheight;

	if ( cv->dv!=NULL ) {
	    newwidth -= cv->dv->dwidth;
	    sbwidth -= cv->dv->dwidth;
	}
	if ( newwidth<30 || newheight<50 ) {
	    if ( newwidth<30 )
		newwidth = 30+sbsize+(cv->dv!=NULL ? cv->dv->dwidth : 0);
	    if ( newheight<50 )
		newheight = 50+sbsize+cv->mbh+cv->infoh;
	    GDrawResize(cv->gw,newwidth,newheight);
return;
	}

	if ( cv->dv!=NULL ) {
	    int dvheight = size.height-(cv->mbh+cv->infoh);
	    GDrawMove(cv->dv->dv,size.width-cv->dv->dwidth,cv->mbh+cv->infoh);
	    GDrawResize(cv->dv->dv,cv->dv->dwidth,dvheight);
	    GDrawResize(cv->dv->ii.v,cv->dv->dwidth-sbsize,dvheight-cv->dv->toph);
	    GGadgetResize(cv->dv->ii.vsb,sbsize,dvheight-cv->dv->toph);
	    cv->dv->ii.vheight = dvheight-cv->dv->toph;
	    GDrawRequestExpose(cv->dv->dv,NULL,false);
	    GDrawRequestExpose(cv->dv->ii.v,NULL,false);
	    GScrollBarSetBounds(cv->dv->ii.vsb,0,cv->dv->ii.lheight+1,
		    cv->dv->ii.vheight/cv->dv->ii.fh);
	}
	if ( cv->showrulers ) {
	    newheight -= cv->rulerh;
	    newwidth -= cv->rulerh;
	}

	if ( newwidth == cv->width && newheight == cv->height )
return;
	if ( cv->backimgs!=NULL )
	    GDrawDestroyWindow(cv->backimgs);
	cv->backimgs = NULL;

	/* MenuBar takes care of itself */
	GDrawResize(cv->v,newwidth,newheight);
	GGadgetMove(cv->vsb,sbwidth, cv->mbh+cv->infoh);
	GGadgetResize(cv->vsb,sbsize,sbheight);
	GGadgetMove(cv->hsb,0,size.height-sbsize);
	GGadgetResize(cv->hsb,sbwidth,sbsize);
	cv->width = newwidth; cv->height = newheight;
	CVFit(cv);
	CVPalettesRaise(cv);
    }
}

static void CVHScroll(CharView *cv,struct sbevent *sb) {
    int newpos = cv->xoff;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos += 9*cv->width/10;
      break;
      case et_sb_up:
        newpos += cv->width/15;
      break;
      case et_sb_down:
        newpos -= cv->width/15;
      break;
      case et_sb_downpage:
        newpos -= 9*cv->width/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = -sb->pos;
      break;
      case et_sb_halfup:
        newpos += cv->width/30;
      break;
      case et_sb_halfdown:
        newpos -= cv->width/30;
      break;
    }
    if ( newpos<-(32000*cv->scale-cv->width) )
        newpos = -(32000*cv->scale-cv->width);
    if ( newpos>8000*cv->scale ) newpos = 8000*cv->scale;
    if ( newpos!=cv->xoff ) {
	int diff = newpos-cv->xoff;
	cv->xoff = newpos;
	cv->back_img_out_of_date = true;
	GScrollBarSetPos(cv->hsb,-newpos);
	GDrawScroll(cv->v,NULL,diff,0);
	if (( cv->showhhints && cv->sc->hstem!=NULL ) || cv->showblues || cv->showvmetrics ) {
	    GRect r;
	    r.y = 0; r.height = cv->height;
	    r.width = 6*cv->sfh+10;
	    if ( diff>0 )
		r.x = cv->width-r.width;
	    else
		r.x = cv->width+diff-r.width;
	    GDrawRequestExpose(cv->v,&r,false);
	}
	if ( cv->showrulers ) {
	    GRect r;
	    r.y = cv->infoh+cv->mbh; r.height = cv->rulerh; r.x = 0; r.width = cv->rulerh+cv->width;
	    GDrawRequestExpose(cv->gw,&r,false);
	}
    }
}

static void CVVScroll(CharView *cv,struct sbevent *sb) {
    int newpos = cv->yoff;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*cv->height/10;
      break;
      case et_sb_up:
        newpos -= cv->height/15;
      break;
      case et_sb_down:
        newpos += cv->height/15;
      break;
      case et_sb_downpage:
        newpos += 9*cv->height/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos+cv->height;
      break;
      case et_sb_halfup:
        newpos -= cv->height/30;
      break;
      case et_sb_halfdown:
        newpos += cv->height/30;
      break;
    }
    if ( newpos<-(20000*cv->scale-cv->height) )
        newpos = -(20000*cv->scale-cv->height);
    if ( newpos>8000*cv->scale ) newpos = 8000*cv->scale;
    if ( newpos!=cv->yoff ) {
	int diff = newpos-cv->yoff;
	cv->yoff = newpos;
	cv->back_img_out_of_date = true;
	GScrollBarSetPos(cv->vsb,newpos-cv->height);
	GDrawScroll(cv->v,NULL,0,diff);
	if (( cv->showvhints && cv->sc->vstem!=NULL) || cv->showhmetrics ) {
	    GRect r;
	    r.x = 0; r.width = cv->width;
	    r.height = 2*cv->sfh+6;
	    if ( diff>0 )
		r.y = 0;
	    else
		r.y = -diff;
	    GDrawRequestExpose(cv->v,&r,false);
	}
	if ( cv->showrulers ) {
	    GRect r;
	    r.x = 0; r.width = cv->rulerh; r.y = cv->infoh+cv->mbh; r.height = cv->rulerh+cv->height;
	    GDrawRequestExpose(cv->gw,&r,false);
	}
    }
}

void LogoExpose(GWindow pixmap,GEvent *event, GRect *r,enum drawmode dm) {
    int sbsize = GDrawPointsToPixels(pixmap,_GScrollBar_Width);
    GRect old;

    r->width = r->height = sbsize;
    if ( event->u.expose.rect.x+event->u.expose.rect.width >= r->x &&
	    event->u.expose.rect.y+event->u.expose.rect.height >= r->y ) {
	/* Put something into the little box where the two scroll bars meet */
	int xoff, yoff;
	GImage *which = (dm==dm_fore) ? &GIcon_FontForgeLogo :
			(dm==dm_back) ? &GIcon_FontForgeBack :
			    &GIcon_FontForgeGuide;
	struct _GImage *base = which->u.image;
	xoff = (sbsize-base->width);
	yoff = (sbsize-base->height);
	GDrawPushClip(pixmap,r,&old);
	GDrawDrawImage(pixmap,which,NULL,
		r->x+(xoff-xoff/2),r->y+(yoff-yoff/2));
	GDrawPopClip(pixmap,&old);
	/*GDrawDrawLine(pixmap,r->x+sbsize-1,r->y,r->x+sbsize-1,r->y+sbsize,0x000000);*/
    }
}

static void CVLogoExpose(CharView *cv,GWindow pixmap,GEvent *event) {
    int rh = cv->showrulers ? cv->rulerh : 0;
    GRect r;

    r.x = cv->width+rh;
    r.y = cv->height+cv->mbh+cv->infoh+rh;
    LogoExpose(pixmap,event,&r,cv->drawmode);
}

static int cv_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
return( GGadgetDispatchEvent(cv->vsb,event));
    }

    switch ( event->type ) {
      case et_selclear:
	ClipboardClear();
      break;
      case et_expose:
	GDrawSetLineWidth(gw,0);
	InfoExpose(cv,gw,event);
	CVLogoExpose(cv,gw,event);
      break;
      case et_char:
	if ( cv->searcher!=NULL )
	    SVChar(cv->searcher,event);
	else
	    CVChar(cv,event);
      break;
      case et_charup:
	CVCharUp(cv,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    CVResize(cv);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == cv->hsb )
		CVHScroll(cv,&event->u.control.u.sb);
	    else
		CVVScroll(cv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_map:
	if ( event->u.map.is_visible )
	    CVPaletteActivate(cv);
	else
	    CVPalettesHideIfMine(cv);
      break;
      case et_destroy:
	CVUnlinkView(cv);
	CVPalettesHideIfMine(cv);
	if ( cv->backimgs!=NULL ) {
	    GDrawDestroyWindow(cv->backimgs);
	    cv->backimgs = NULL;
	}
	if ( cv->icon!=NULL ) {
	    GDrawDestroyWindow(cv->icon);
	    cv->icon = NULL;
	}
	CharViewFree(cv);
      break;
      case et_close:
	GDrawDestroyWindow(gw);
      break;
      case et_mouseup: case et_mousedown:
	GGadgetEndPopup();
	CVPaletteActivate(cv);
	if ( cv->inactive )
	    SVMakeActive(cv->searcher,cv);
      break;
      case et_mousemove:
	if ( event->u.mouse.y>cv->mbh ) {
	    int enc = CVCurEnc(cv);
	    SCPreparePopup(cv->gw,cv->sc,cv->fv->map->remap,enc,
		    UniFromEnc(enc,cv->fv->map->enc));
	}
      break;
      case et_drop:
	CVDrop(cv,event);
      break;
      case et_focus:
	if ( event->u.focus.gained_focus ) {
	    GDrawSetGIC(gw,cv->gic,0,20);
#if 0
	    CVPaletteActivate(cv);
#endif
	}
      break;
    }
return( true );
}

#define MID_Fit		2001
#define MID_ZoomIn	2002
#define MID_ZoomOut	2003
#define MID_HidePoints	2004
#define MID_Fill	2006
#define MID_Next	2007
#define MID_Prev	2008
#define MID_HideRulers	2009
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_DisplayCompositions	2014
#define MID_MarkExtrema	2015
#define MID_Goto	2016
#define MID_FindInFontView	2017
#define MID_KernPairs	2018
#define MID_AnchorPairs	2019
#define MID_ShowGridFit 2020
#define MID_PtsNone	2021
#define MID_PtsTrue	2022
#define MID_PtsPost	2023
#define MID_PtsSVG	2024
#define MID_Ligatures	2025
#define MID_Former	2026
#define MID_MarkPointsOfInflection	2027
#define MID_ShowCPInfo	2028
#define MID_ShowTabs	2029
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_Merge	2105
#define MID_SelAll	2106
#define MID_CopyRef	2107
#define MID_UnlinkRef	2108
#define MID_Undo	2109
#define MID_Redo	2110
#define MID_CopyWidth	2111
#define MID_RemoveUndoes	2112
#define MID_CopyFgToBg	2115
#define MID_NextPt	2116
#define MID_PrevPt	2117
#define MID_FirstPt	2118
#define MID_NextCP	2119
#define MID_PrevCP	2120
#define MID_SelNone	2121
#define MID_SelectWidth	2122
#define MID_SelectVWidth	2123
#define MID_CopyLBearing	2124
#define MID_CopyRBearing	2125
#define MID_CopyVWidth	2126
#define MID_Join	2127
#define MID_CopyGridFit	2128
#define MID_Elide	2129
#define MID_SelectAllPoints	2130
#define MID_SelectAnchors	2131
#define MID_FirstPtNextCont	2132
#define MID_Contours	2133
#define MID_SelectHM	2134
#define MID_CopyFeatures	2135
#define MID_SelInvert	2136
#define MID_CopyBgToFg	2137
#define MID_SelPointAt	2138
#define MID_Clockwise	2201
#define MID_Counter	2202
#define MID_GetInfo	2203
#define MID_Correct	2204
#define MID_AvailBitmaps	2210
#define MID_RegenBitmaps	2211
#define MID_Stroke	2205
#define MID_RmOverlap	2206
#define MID_Simplify	2207
#define MID_BuildAccent	2208
#define MID_Autotrace	2212
#define MID_Round	2213
#define MID_MetaFont	2217
#define MID_Average	2219
#define MID_SpacePts	2220
#define MID_SpaceRegion	2221
#define MID_MakeParallel	2222
#define MID_ShowDependentRefs	2223
#define MID_AddExtrema	2224
#define MID_CleanupGlyph	2225
#define MID_TilePath	2226
#define MID_BuildComposite	2227
#define MID_Exclude	2228
#define MID_Intersection	2229
#define MID_FindInter	2230
#define MID_Effects	2231
#define MID_SimplifyMore	2232
#define MID_First	2233
#define MID_Earlier	2234
#define MID_Later	2235
#define MID_Last	2236
#define MID_CharInfo	2240
#define MID_ShowDependentSubs	2241
#define MID_CanonicalStart	2242
#define MID_CanonicalContours	2243
#define MID_RemoveBitmaps	2244
#define MID_Corner	2301
#define MID_Tangent	2302
#define MID_Curve	2303
#define MID_MakeFirst	2304
#define MID_MakeLine	2305
#define MID_CenterCP	2306
#define MID_ImplicitPt	2307
#define MID_NoImplicitPt	2308
#define MID_InsertPtOnSplineAt	2309
#define MID_AddAnchor	2310
#define MID_AutoHint	2400
#define MID_ClearHStem	2401
#define MID_ClearVStem	2402
#define MID_AddHHint	2404
#define MID_AddVHint	2405
#define MID_AddDHint	2406
#define MID_ReviewHints	2407
#define MID_CreateHHint	2408
#define MID_CreateVHint	2409
#define MID_MinimumDistance	2410
#define MID_AutoInstr	2411
#define MID_ClearInstr	2412
#define MID_EditInstructions 2413
#define MID_Debug	2414
#define MID_HintSubsPt	2415
#define MID_AutoCounter	2416
#define MID_DontAutoHint	2417
#define MID_Tools	2501
#define MID_Layers	2502
#define MID_DockPalettes	2503
#define MID_Center	2600
#define MID_SetWidth	2601
#define MID_SetLBearing	2602
#define MID_SetRBearing	2603
#define MID_Thirds	2604
#define MID_RemoveKerns	2605
#define MID_SetVWidth	2606
#define MID_RemoveVKerns	2607
#define MID_KPCloseup	2608
#define MID_AnchorsAway	2609
#define MID_OpenBitmap	2700
#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Display	2706
#define MID_RevertGlyph	2707

#define MID_MMReblend	2800
#define MID_MMAll	2821
#define MID_MMNone	2822

#define MID_Warnings	3000

static void CVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GDrawDestroyWindow(gw);
}

static void CVMenuOpenBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( cv->fv->sf->bitmaps==NULL )
return;
    BitmapViewCreatePick(CVCurEnc(cv),cv->fv);
}

static void CVMenuOpenMetrics(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    MetricsViewCreate(cv->fv,cv->sc,NULL);
}

static void CVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _FVMenuSave(cv->fv);
}

static void CVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _FVMenuSaveAs(cv->fv);
}

static void CVMenuGenerate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _FVMenuGenerate(cv->fv,false);
}

static void CVMenuGenerateFamily(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _FVMenuGenerate(cv->fv,true);
}

static void CVMenuExport(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVExport(cv);
}

static void CVMenuImport(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVImport(cv);
}

static void CVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    FVDelay(cv->fv,FVRevert);		/* The revert command can potentially */
			    /* destroy our window (if the char weren't in the */
			    /* old font). If that happens before the menu finishes */
			    /* we get a crash. So delay till after the menu completes */
}

void RevertedGlyphReferenceFixup(SplineChar *sc, SplineFont *sf) {
    RefChar *refs, *prev, *next;

    for ( prev=NULL, refs = sc->layers[ly_fore].refs ; refs!=NULL; refs = next ) {
	next = refs->next;
	if ( refs->orig_pos<sf->glyphcnt && sf->glyphs[refs->orig_pos]!=NULL ) {
	    prev = refs;
	    refs->sc = sf->glyphs[refs->orig_pos];
	    refs->unicode_enc = refs->sc->unicodeenc;
	    SCReinstanciateRefChar(sc,refs);
	    SCMakeDependent(sc,refs->sc);
	} else {
	    if ( prev==NULL )
		sc->layers[ly_fore].refs = next;
	    else
		prev->next = next;
	    RefCharFree(refs);
	}
    }
}

static void CVMenuRevertGlyph(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineChar *sc, temp;
#ifdef FONTFORGE_CONFIG_TYPE3
    Undoes **undoes;
    int layer, lc;
#endif
    CharView *cvs;

    if ( cv->sc->parent->filename==NULL || cv->sc->namechanged || cv->sc->parent->mm!=NULL )
return;
    sc = SFDReadOneChar(cv->sc->parent,cv->sc->name);
    if ( sc==NULL ) {
	gwwv_post_error(_("Can't Find Glyph"),_("The glyph, %.80s, can't be found in the sfd file"),cv->sc->name);
	cv->sc->namechanged = true;
    } else {
	SCPreserveState(cv->sc,true);
	SCPreserveBackground(cv->sc);
	temp = *cv->sc;
	cv->sc->dependents = NULL;
#ifdef FONTFORGE_CONFIG_TYPE3
	lc = cv->sc->layer_cnt;
	undoes = galloc(lc*sizeof(Undoes *));
	for ( layer=0; layer<lc; ++layer ) {
	    undoes[layer] = cv->sc->layers[layer].undoes;
	    cv->sc->layers[layer].undoes = NULL;
	}
#else
	cv->sc->layers[ly_fore].undoes = cv->sc->layers[ly_back].undoes = NULL;
#endif
	SplineCharFreeContents(cv->sc);
	*cv->sc = *sc;
	chunkfree(sc,sizeof(SplineChar));
	cv->sc->parent = temp.parent;
	cv->sc->dependents = temp.dependents;
#ifdef FONTFORGE_CONFIG_TYPE3
	for ( layer = 0; layer<lc && layer<cv->sc->layer_cnt; ++layer )
	    cv->sc->layers[layer].undoes = undoes[layer];
	for ( ; layer<lc; ++layer )
	    UndoesFree(undoes[layer]);
	free(undoes);
#else
	cv->sc->layers[ly_fore].undoes = temp.layers[ly_fore].undoes;
	cv->sc->layers[ly_back].undoes = temp.layers[ly_back].undoes;
#endif
	cv->sc->views = temp.views;
	/* cv->sc->changed = temp.changed; */
	for ( cvs=cv->sc->views; cvs!=NULL; cvs=cvs->next ) {
	    cvs->layerheads[dm_back] = &cv->sc->layers[ly_back];
	    cvs->layerheads[dm_fore] = &cv->sc->layers[ly_fore];
	}
	RevertedGlyphReferenceFixup(cv->sc, temp.parent);
	_CVCharChangedUpdate(cv,false);
    }
}

static void CVMenuPrint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    PrintDlg(NULL,cv->sc,NULL);
}

static void CVMenuDisplay(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    DisplayDlg(cv->sc->parent);
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Revert:
	    mi->ti.disabled = cv->fv->sf->origname==NULL || cv->fv->sf->new;
	  break;
	  case MID_RevertGlyph:
	    mi->ti.disabled = cv->fv->sf->filename==NULL ||
		    cv->sc->namechanged ||
		    cv->fv->sf->mm!=NULL;
	  break;
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	  case MID_Display:
	    mi->ti.disabled = (cv->fv->sf->onlybitmaps && cv->fv->sf->bitmaps==NULL) ||
		    cv->fv->sf->multilayer;
	  break;
	}
    }
}

static void CVMenuFontInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    DelayEvent(FontMenuFontInfo,cv->fv);
}

static void CVMenuFindProblems(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    FindProblems(NULL,cv,NULL);
}

static void CVMenuMetaFont(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
#ifdef GWW_TEST
    extern int ChangeWeight(SplineChar *sc,double factor,double add);
    ChangeWeight(cv->sc,2.0,25);
#else
    MetaFont(NULL,cv,NULL);
#endif
}

static void CVMenuInline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    OutlineDlg(NULL,cv,NULL,true);
}

static void CVMenuOutline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    OutlineDlg(NULL,cv,NULL,false);
}

static void CVMenuShadow(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    ShadowDlg(NULL,cv,NULL,false);
}

static void CVMenuWireframe(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    ShadowDlg(NULL,cv,NULL,true);
}

static void _CVMenuScale(CharView *cv,struct gmenuitem *mi) {

    if ( mi->mid == MID_Fit ) {
	CVFit(cv);
    } else {
	BasePoint c;
	c.x = (cv->width/2-cv->xoff)/cv->scale;
	c.y = (cv->height/2-cv->yoff)/cv->scale;
	if ( CVAnySel(cv,NULL,NULL,NULL,NULL))
	    CVFindCenter(cv,&c,false);
	CVMagnify(cv,c.x,c.y,mi->mid==MID_ZoomOut?-1:1);
    }
}

static void CVMenuScale(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuScale(cv,mi);
}

static void CVMenuShowHide(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showpoints = cv->showpoints = !cv->showpoints;
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuNumberPoints(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( mi->mid ) {
      case MID_PtsNone:
	cv->showpointnumbers = false;
      break;
      case MID_PtsTrue:
	cv->showpointnumbers = true;
      break;
      case MID_PtsPost:
	cv->showpointnumbers = true;
	cv->sc->numberpointsbackards = true;
      break;
      case MID_PtsSVG:
	cv->showpointnumbers = true;
	cv->sc->numberpointsbackards = false;
      break;
    }
    SCNumberPoints(cv->sc);
    SCUpdateAll(cv->sc);
}

static void CVMenuMarkExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.markextrema = cv->markextrema = !cv->markextrema;
    SavePrefs();
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuMarkPointsOfInflection(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.markpoi = cv->markpoi = !cv->markpoi;
    SavePrefs();
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuShowCPInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showcpinfo = cv->showcpinfo = !cv->showcpinfo;
    SavePrefs();
    /* Nothing to update, only show this stuff in the user is moving a cp */
    /*  which s/he is currently not, s/he is manipulating the menu */
}

static void CVMenuShowTabs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showtabs = cv->showtabs = !cv->showtabs;
    CVChangeTabsVisibility(cv,cv->showtabs);
    SavePrefs();
}

static void _CVMenuShowHideRulers(CharView *cv) {
    GRect pos;

    CVShows.showrulers = cv->showrulers = !cv->showrulers;
    pos.y = cv->mbh+cv->infoh;
    pos.x = 0;
    if ( cv->showrulers ) {
	cv->height -= cv->rulerh;
	cv->width -= cv->rulerh;
	pos.y += cv->rulerh;
	pos.x += cv->rulerh;
    } else {
	cv->height += cv->rulerh;
	cv->width += cv->rulerh;
    }
    cv->back_img_out_of_date = true;
    pos.width = cv->width; pos.height = cv->height;
    GDrawMoveResize(cv->v,pos.x,pos.y,pos.width,pos.height);
    GDrawSync(NULL);
    GDrawRequestExpose(cv->v,NULL,false);
    SavePrefs();
}

static void CVMenuShowHideRulers(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuShowHideRulers(cv);
}

static void CVMenuFill(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showfilled = cv->showfilled = !cv->showfilled;
    CVRegenFill(cv);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuShowGridFit(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( !hasFreeType() || cv->drawmode!=dm_fore || cv->dv!=NULL )
return;
    CVFtPpemDlg(cv,false);
}

static void CVMenuEditInstrs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    SCEditInstructions(cv->sc);
}

static void CVMenuDebug(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( !hasFreeTypeDebugger())
return;
    CVFtPpemDlg(cv,true);
}

static void CVMenuClearInstrs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->ttf_instrs_len!=0 ) {
	free(cv->sc->ttf_instrs);
	cv->sc->ttf_instrs = NULL;
	cv->sc->ttf_instrs_len = 0;
	cv->sc->instructions_out_of_date = false;
	SCCharChangedUpdate(cv->sc);
	SCMarkInstrDlgAsChanged(cv->sc);
	cv->sc->complained_about_ptnums = false;	/* Should be after CharChanged */
    }
}

static void CVMenuKernPairs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SFShowKernPairs(cv->sc->parent,cv->sc,NULL);
}

static void CVMenuLigatures(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SFShowLigatures(cv->fv->sf,cv->sc);
}

static void CVMenuAnchorPairs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SFShowKernPairs(cv->sc->parent,cv->sc,(AnchorClass *) (-1));
}

static void CVMenuChangeChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineFont *sf = cv->sc->parent;
    int pos = -1;
    int gid;
    EncMap *map = cv->fv->map;
    Encoding *enc = map->enc;

    if ( cv->searcher!=NULL )
return;
    if ( mi->mid == MID_Next ) {
	pos = CVCurEnc(cv)+1;
    } else if ( mi->mid == MID_Prev ) {
	pos = CVCurEnc(cv)-1;
    } else if ( mi->mid == MID_NextDef ) {
	for ( pos = CVCurEnc(cv)+1; pos<map->enccount &&
		((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid])); ++pos );
	if ( pos>=map->enccount ) {
	    if ( enc->is_tradchinese ) {
		if ( strstrmatch(enc->enc_name,"hkscs")!=NULL ) {
		    if ( CVCurEnc(cv)<0x8140 )
			pos = 0x8140;
		} else {
		    if ( CVCurEnc(cv)<0xa140 )
			pos = 0xa140;
		}
	    } else if ( CVCurEnc(cv)<0x8431 && strstrmatch(enc->enc_name,"johab")!=NULL )
		pos = 0x8431;
	    else if ( CVCurEnc(cv)<0xa1a1 && strstrmatch(enc->iconv_name?enc->iconv_name:enc->enc_name,"EUC")!=NULL )
		pos = 0xa1a1;
	    else if ( CVCurEnc(cv)<0x8140 && strstrmatch(enc->enc_name,"sjis")!=NULL )
		pos = 0x8140;
	    else if ( CVCurEnc(cv)<0xe040 && strstrmatch(enc->enc_name,"sjis")!=NULL )
		pos = 0xe040;
	    if ( pos>=map->enccount )
return;
	}
    } else if ( mi->mid == MID_PrevDef ) {
	for ( pos = CVCurEnc(cv)-1; pos>=0 &&
		((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid])); --pos );
	if ( pos<0 )
return;
    } else if ( mi->mid == MID_Former ) {
	if ( cv->former_cnt<=1 )
return;
	for ( gid=sf->glyphcnt-1; gid>=0; --gid )
	    if ( sf->glyphs[gid]!=NULL &&
		    strcmp(sf->glyphs[gid]->name,cv->former_names[1])==0 )
	break;
	if ( gid<0 )
return;
	pos = map->backmap[gid];
    }
    /* Werner doesn't think it should wrap */
    if ( pos<0 ) /* pos = map->enccount-1; */
return;
    else if ( pos>= map->enccount ) /* pos = 0; */
return;
    if ( pos>=0 && pos<map->enccount )
	CVChangeChar(cv,pos);
}

void CVShowPoint(CharView *cv, BasePoint *me) {
    int x, y;
    int fudge = 30;

    if ( cv->width<60 )
	fudge = cv->width/3;
    if ( cv->height<60 && fudge>cv->height/3 )
	fudge = cv->height/3;

    /* Make sure the point is visible and has some context around it */
    x =  cv->xoff + rint(me->x*cv->scale);
    y = -cv->yoff + cv->height - rint(me->y*cv->scale);
    if ( x<fudge || y<fudge || x>cv->width-fudge || y>cv->height-fudge )
	CVMagnify(cv,me->x,me->y,0);
}

static void CVSelectContours(CharView *cv,struct gmenuitem *mi) {
    SplineSet *spl;
    SplinePoint *sp;
    int sel;

    for ( spl=cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl=spl->next ) {
	sel = false;
	for ( sp=spl->first ; ; ) {
	    if ( sp->selected ) {
		sel = true;
	break;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( sel ) {
	    for ( sp=spl->first ; ; ) {
		sp->selected = true;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }
    SCUpdateAll(cv->sc);
}

static void CVMenuSelectContours(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSelectContours(cv,mi);
}

static void CVMenuSelectPointAt(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSelectPointAt(cv);
}

static void CVNextPrevPt(CharView *cv,struct gmenuitem *mi) {
    SplinePoint *selpt=NULL, *other;
    RefChar *r; ImageList *il;
    SplineSet *spl, *ss;
    int x, y;

    if ( mi->mid != MID_FirstPt && !CVOneThingSel(cv,&selpt,&spl,&r,&il,NULL))
return;
    other = selpt;
    if ( mi->mid == MID_NextPt ) {
	if ( other->next!=NULL && other->next->to!=spl->first )
	    other = other->next->to;
	else {
	    if ( spl->next == NULL )
		spl = cv->layerheads[cv->drawmode]->splines;
	    else
		spl = spl->next;
	    other = spl->first;
	}
    } else if ( mi->mid == MID_PrevPt ) {
	if ( other!=spl->first ) {
	    other = other->prev->from;
	} else {
	    if ( spl==cv->layerheads[cv->drawmode]->splines ) {
		for ( ss = cv->layerheads[cv->drawmode]->splines; ss->next!=NULL; ss=ss->next );
	    } else {
		for ( ss = cv->layerheads[cv->drawmode]->splines; ss->next!=spl; ss=ss->next );
	    }
	    spl = ss;
	    other = ss->last;
	    if ( spl->last==spl->first && spl->last->prev!=NULL )
		other = other->prev->from;
	}
    } else if ( mi->mid == MID_FirstPt ) {
	if ( cv->layerheads[cv->drawmode]->splines==NULL )
return;
	other = (cv->layerheads[cv->drawmode]->splines)->first;
	CVClearSel(cv);
    } else if ( mi->mid == MID_FirstPtNextCont ) {
	if ( spl->next!=NULL )
	    other = spl->next->first;
	else
	    other = NULL;
    }
    if ( selpt!=NULL )
	selpt->selected = false;
    if ( other!=NULL )
	other->selected = true;
    cv->p.sp = NULL;
    cv->lastselpt = other;

    /* Make sure the point is visible and has some context around it */
    if ( other!=NULL ) {
	x =  cv->xoff + rint(other->me.x*cv->scale);
	y = -cv->yoff + cv->height - rint(other->me.y*cv->scale);
	if ( x<40 || y<40 || x>cv->width-40 || y>cv->height-40 )
	    CVMagnify(cv,other->me.x,other->me.y,0);
    }

    CVInfoDraw(cv,cv->gw);
    SCUpdateAll(cv->sc);
}

static void CVMenuNextPrevPt(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVNextPrevPt(cv,mi);
}

static void CVNextPrevCPt(CharView *cv,struct gmenuitem *mi) {
    SplinePoint *selpt=NULL;
    RefChar *r; ImageList *il;
    SplineSet *spl;

    if ( !CVOneThingSel(cv,&selpt,&spl,&r,&il,NULL))
return;
    if ( selpt==NULL )
return;
    cv->p.nextcp = mi->mid==MID_NextCP;
    cv->p.prevcp = mi->mid==MID_PrevCP;
    SCUpdateAll(cv->sc);
}

static void CVMenuNextPrevCPt(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVNextPrevCPt(cv,mi);
}

static void CVMenuGotoChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int pos = GotoChar(cv->fv->sf,cv->fv->map);

    if ( pos!=-1 )
	CVChangeChar(cv,pos);
}

static void CVMenuFindInFontView(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    FVChangeChar(cv->fv,CVCurEnc(cv));
    GDrawSetVisible(cv->fv->gw,true);
    GDrawRaise(cv->fv->gw);
}

static void CVMenuPalettesDock(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    PalettesChangeDocking();
}

static void CVMenuPaletteShow(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVPaletteSetVisible(cv, mi->mid==MID_Tools, !CVPaletteIsVisible(cv, mi->mid==MID_Tools));
}

static void cv_pllistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e) {
    extern int palettes_docked;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Tools:
	    mi->ti.checked = CVPaletteIsVisible(cv,1);
	  break;
	  case MID_Layers:
	    mi->ti.checked = CVPaletteIsVisible(cv,0);
	  break;
	  case MID_DockPalettes:
	    mi->ti.checked = palettes_docked;
	  break;
	}
    }
}

static void pllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_pllistcheck(cv,mi,e);
}

static void CVUndo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVDoUndo(cv);
}

static void CVRedo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVDoRedo(cv);
}

static void _CVCopy(CharView *cv) {
    int desel = false, anya;

    if ( !CVAnySel(cv,NULL,NULL,NULL,&anya))
	if ( !(desel = CVSetSel(cv,-1)))
return;
    CopySelected(cv);
    if ( desel )
	CVClearSel(cv);
}

static void CVCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVCopy(cv);
}

static void CVCopyRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CopyReference(cv->sc);
}

static void CVMenuCopyGridFit(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVCopyGridFit(cv);
}

static void CVCopyWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( mi->mid==MID_CopyVWidth && !cv->sc->parent->hasvmetrics )
return;
    CopyWidth(cv,mi->mid==MID_CopyWidth?ut_width:
		 mi->mid==MID_CopyVWidth?ut_vwidth:
		 mi->mid==MID_CopyLBearing?ut_lbearing:
					 ut_rbearing);
}

static void CVCopyFeatures(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCCopyFeatures(cv->sc);
}

static void CVDoClear(CharView *cv) {
    ImageList *prev, *imgs, *next;

    CVPreserveState(cv);
    if ( cv->drawmode==dm_fore )
	SCRemoveSelectedMinimumDistances(cv->sc,2);
    cv->layerheads[cv->drawmode]->splines = SplinePointListRemoveSelected(cv->sc,
	    cv->layerheads[cv->drawmode]->splines);
    if ( cv->drawmode==dm_fore ) {
	RefChar *refs, *next;
	AnchorPoint *ap, *aprev=NULL, *anext;
	for ( refs=cv->sc->layers[ly_fore].refs; refs!=NULL; refs = next ) {
	    next = refs->next;
	    if ( refs->selected )
		SCRemoveDependent(cv->sc,refs);
	}
	if ( cv->showanchor ) for ( ap=cv->sc->anchor; ap!=NULL; ap=anext ) {
	    anext = ap->next;
	    if ( ap->selected ) {
		if ( aprev!=NULL )
		    aprev->next = anext;
		else
		    cv->sc->anchor = anext;
		ap->next = NULL;
		AnchorPointsFree(ap);
	    } else
		aprev = ap;
	}
    }
    for ( prev = NULL, imgs=cv->layerheads[cv->drawmode]->images; imgs!=NULL; imgs = next ) {
	next = imgs->next;
	if ( !imgs->selected )
	    prev = imgs;
	else {
	    if ( prev==NULL )
		cv->layerheads[cv->drawmode]->images = next;
	    else
		prev->next = next;
	    chunkfree(imgs,sizeof(ImageList));
	    /* garbage collection of images????!!!! */
	}
    }
    if ( cv->lastselpt!=NULL || cv->p.sp!=NULL ) {
	cv->lastselpt = NULL; cv->p.sp = NULL;
	CVInfoDraw(cv,cv->gw);
    }
}

static void CVClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anyanchor;

    if ( !CVAnySel(cv,NULL,NULL,NULL,&anyanchor))
return;
    CVDoClear(cv);
    CVCharChangedUpdate(cv);
}

static void CVClearBackground(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    SCClearBackground(cv->sc);
}

static void _CVPaste(CharView *cv) {
    enum undotype ut = CopyUndoType();
    int was_empty = cv->drawmode==dm_fore && cv->sc->hstem==NULL && cv->sc->vstem==NULL && cv->sc->layers[ly_fore].splines==NULL && cv->sc->layers[ly_fore].refs==NULL;
    if ( ut!=ut_lbearing )	/* The lbearing code does this itself */
	CVPreserveStateHints(cv);
    if ( ut!=ut_width && ut!=ut_vwidth && ut!=ut_lbearing && ut!=ut_rbearing && ut!=ut_possub )
	CVClearSel(cv);
    PasteToCV(cv);
    CVCharChangedUpdate(cv);
    if ( was_empty && (cv->sc->hstem != NULL || cv->sc->vstem!=NULL ))
	cv->sc->changedsincelasthinted = false;
}

static void CVPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVPaste(cv);
}

static void _CVMerge(CharView *cv,int elide) {
    int anyp = 0;

    if ( !CVAnySel(cv,&anyp,NULL,NULL,NULL) || !anyp)
return;
    CVPreserveState(cv);
    SplineCharMerge(cv->sc,&cv->layerheads[cv->drawmode]->splines,!elide);
    SCClearSelPt(cv->sc);
    CVCharChangedUpdate(cv);
}

static void CVMerge(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMerge(cv,false);
}

static void CVElide(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMerge(cv,true);
}

static void _CVJoin(CharView *cv) {
    int anyp = 0, changed;
    extern float joinsnap;

    CVAnySel(cv,&anyp,NULL,NULL,NULL);
    CVPreserveState(cv);
    cv->layerheads[cv->drawmode]->splines = SplineSetJoin(cv->layerheads[cv->drawmode]->splines,!anyp,joinsnap/cv->scale,&changed);
    if ( changed )
	CVCharChangedUpdate(cv);
}

static void CVJoin(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVJoin(cv);
}

static void _CVCut(CharView *cv) {
    int anya;

    if ( !CVAnySel(cv,NULL,NULL,NULL,&anya))
return;
    _CVCopy(cv);
    CVDoClear(cv);
    CVCharChangedUpdate(cv);
}

static void CVCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVCut(cv);
}

static void CVCopyFgBg(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->layers[ly_fore].splines==NULL )
return;
    SCCopyFgToBg(cv->sc,true);
}

#ifdef FONTFORGE_CONFIG_COPY_BG_TO_FG
static void CVCopyBgFg(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->layers[ly_back].splines==NULL )
return;
    SCCopyBgToFg(cv->sc,true);
}
#endif

static void CVSelectAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int mask = -1;

    if ( mi->mid==MID_SelectAllPoints )
	mask = 1;
    else if ( mi->mid==MID_SelectAnchors )
	mask = 2;
    if ( CVSetSel(cv,mask))
	SCUpdateAll(cv->sc);
}

static void CVSelectNone(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( CVClearSel(cv))
	SCUpdateAll(cv->sc);
}

static void CVSelectInvert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVInvertSel(cv);
    SCUpdateAll(cv->sc);
}

static void CVSelectWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv->widthsel = !cv->widthsel;
    cv->oldwidth = cv->sc->width;
    SCUpdateAll(cv->sc);
}

static void CVSelectVWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( !cv->showvmetrics || !cv->sc->parent->hasvmetrics )
return;
    cv->vwidthsel = !cv->widthsel;
    cv->oldvwidth = cv->sc->vwidth;
    SCUpdateAll(cv->sc);
}

static void CVSelectHM(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp; SplineSet *spl; RefChar *r; ImageList *im;
    int exactlyone = CVOneThingSel(cv,&sp,&spl,&r,&im,NULL);

    if ( !exactlyone || sp==NULL || sp->hintmask == NULL || spl==NULL )
return;
    while ( sp!=NULL ) {
	if ( sp->prev==NULL )
    break;
	sp = sp->prev->from;
	if ( sp == spl->first )
    break;
	if ( sp->hintmask!=NULL )
 goto done;
	sp->selected = true;
    }
    for ( spl = spl->next; spl!=NULL; spl = spl->next ) {
	for ( sp=spl->first; sp!=NULL;  ) {
	    if ( sp->hintmask!=NULL )
 goto done;
	    sp->selected = true;
	    if ( sp->prev==NULL )
	break;
	    sp = sp->prev->from;
	    if ( sp == spl->first )
	break;
	}
    }
 done:
    SCUpdateAll(cv->sc);
}

static void _CVUnlinkRef(CharView *cv) {
    int anyrefs=0;
    RefChar *rf, *next;

    if ( cv->drawmode==dm_fore && cv->layerheads[dm_fore]->refs!=NULL ) {
	CVPreserveState(cv);
	for ( rf=cv->layerheads[dm_fore]->refs; rf!=NULL && !anyrefs; rf=rf->next )
	    if ( rf->selected ) anyrefs = true;
	for ( rf=cv->layerheads[dm_fore]->refs; rf!=NULL ; rf=next ) {
	    next = rf->next;
	    if ( rf->selected || !anyrefs) {
		SCRefToSplines(cv->sc,rf);
	    }
	}
	CVSetCharChanged(cv,true);
	SCUpdateAll(cv->sc);
	/* Don't need to update dependancies, their splines won't have changed*/
    }
}

static void CVUnlinkRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVUnlinkRef(cv);
}

static void CVRemoveUndoes(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    UndoesFree(cv->layerheads[cv->drawmode]->undoes);
    UndoesFree(cv->layerheads[cv->drawmode]->redoes);
    cv->layerheads[cv->drawmode]->undoes = cv->layerheads[cv->drawmode]->redoes = NULL;
}

/* We can only paste if there's something in the copy buffer */
/* we can only copy if there's something selected to copy */
/* figure out what things are possible from the edit menu before the user */
/*  pulls it down */
static void cv_edlistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e,int is_cv) {
    int anypoints, anyrefs, anyimages, anyanchor;

    CVAnySel(cv,&anypoints,&anyrefs,&anyimages,&anyanchor);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Join:
	    mi->ti.disabled = !anypoints;
	  break;
	  case MID_Merge:
	    mi->ti.disabled = !anypoints;
	  break;
	  case MID_Elide:
	    mi->ti.disabled = !anypoints;
	  break;
	  case MID_Clear: case MID_Cut: /*case MID_Copy:*/
	    /* If nothing is selected, copy copies everything */
	    mi->ti.disabled = !anypoints && !anyrefs && !anyimages && !anyanchor;
	  break;
	  case MID_CopyLBearing: case MID_CopyRBearing:
	    mi->ti.disabled = cv->drawmode!=dm_fore ||
		    (cv->sc->layers[ly_fore].splines==NULL && cv->sc->layers[ly_fore].refs==NULL);
	  break;
#ifdef FONTFORGE_CONFIG_COPY_BG_TO_FG
	  case MID_CopyBgToFg:
	    mi->ti.disabled = cv->sc->layers[ly_back].splines==NULL;
	  break;
#endif
	  case MID_CopyFgToBg:
	    mi->ti.disabled = cv->sc->layers[ly_fore].splines==NULL;
	  break;
	  case MID_CopyFeatures:
	    mi->ti.disabled = !SCAnyFeatures(cv->sc);
	  break;
	  case MID_CopyGridFit:
	    mi->ti.disabled = cv->gridfit==NULL;
	  break;
	  case MID_Paste:
	    mi->ti.disabled = !CopyContainsSomething() &&
#ifndef _NO_LIBPNG
		    !GDrawSelectionHasType(cv->gw,sn_clipboard,"image/png") &&
#endif
#ifndef _NO_LIBXML
		    !GDrawSelectionHasType(cv->gw,sn_clipboard,"image/svg") &&
#endif
		    !GDrawSelectionHasType(cv->gw,sn_clipboard,"image/bmp") &&
		    !GDrawSelectionHasType(cv->gw,sn_clipboard,"image/eps") &&
		    !GDrawSelectionHasType(cv->gw,sn_clipboard,"image/ps");
	  break;
	  case MID_Undo:
	    mi->ti.disabled = cv->layerheads[cv->drawmode]->undoes==NULL;
	  break;
	  case MID_Redo:
	    mi->ti.disabled = cv->layerheads[cv->drawmode]->redoes==NULL;
	  break;
	  case MID_RemoveUndoes:
	    mi->ti.disabled = cv->layerheads[cv->drawmode]->undoes==NULL && cv->layerheads[cv->drawmode]->redoes==NULL;
	  break;
	  case MID_CopyRef:
	    mi->ti.disabled = cv->drawmode!=dm_fore || cv->searcher!=NULL;
	  break;
	  case MID_UnlinkRef:
	    mi->ti.disabled = cv->drawmode!=dm_fore || cv->sc->layers[ly_fore].refs==NULL;
	  break;
	}
    }
}

static void edlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_edlistcheck(cv,mi,e,true);
}

int BpColinear(BasePoint *first, BasePoint *mid, BasePoint *last) {
    BasePoint dist_f, unit_f, dist_l, unit_l;
    double len, off_l, off_f;

    dist_f.x = first->x - mid->x; dist_f.y = first->y - mid->y;
    len = sqrt( dist_f.x*dist_f.x + dist_f.y*dist_f.y );
    if ( len==0 )
return( false );
    unit_f.x = dist_f.x/len; unit_f.y = dist_f.y/len;

    dist_l.x = last->x - mid->x; dist_l.y = last->y - mid->y;
    len = sqrt( dist_l.x*dist_l.x + dist_l.y*dist_l.y );
    if ( len==0 )
return( false );
    unit_l.x = dist_l.x/len; unit_l.y = dist_l.y/len;

    off_f = dist_l.x*unit_f.y - dist_l.y*unit_f.x;
    off_l = dist_f.x*unit_l.y - dist_f.y*unit_l.x;
    if ( ( off_f<-1.5 || off_f>1.5 ) && ( off_l<-1.5 || off_l>1.5 ))
return( false );

return( true );
}

void SPChangePointType(SplinePoint *sp, int pointtype) {
    BasePoint unitnext, unitprev;
    double nextlen, prevlen;
    int makedflt;
    /*int oldpointtype = sp->pointtype;*/

    if ( sp->pointtype==pointtype ) {
	if ( pointtype==pt_curve ) {
	    if ( !sp->nextcpdef && sp->next!=NULL && !sp->next->order2 )
		SplineCharDefaultNextCP(sp);
	    if ( !sp->prevcpdef && sp->prev!=NULL && !sp->prev->order2 )
		SplineCharDefaultPrevCP(sp);
	}
return;
    }
    sp->pointtype = pointtype;

    if ( pointtype==pt_corner ) {
	/* Leave control points as they are */;
	sp->nextcpdef = sp->nonextcp;
	sp->prevcpdef = sp->noprevcp;
    } else if ( pointtype==pt_tangent ) {
	if ( sp->next!=NULL && !sp->nonextcp && sp->next->knownlinear ) {
	    sp->nonextcp = true;
	    sp->nextcp = sp->me;
	} else if ( sp->prev!=NULL && !sp->nonextcp &&
		BpColinear(&sp->prev->from->me,&sp->me,&sp->nextcp) ) {
	    /* The current control point is reasonable */
	} else {
	    SplineCharTangentNextCP(sp);
	    if ( sp->next ) SplineRefigure(sp->next);
	}
	if ( sp->prev!=NULL && !sp->noprevcp && sp->prev->knownlinear ) {
	    sp->noprevcp = true;
	    sp->prevcp = sp->me;
	} else if ( sp->next!=NULL && !sp->noprevcp &&
		BpColinear(&sp->next->to->me,&sp->me,&sp->prevcp) ) {
	    /* The current control point is reasonable */
	} else {
	    SplineCharTangentPrevCP(sp);
	    if ( sp->prev ) SplineRefigure(sp->prev);
	}
    } else if ( BpColinear(&sp->prevcp,&sp->me,&sp->nextcp) ||
	    ( sp->nonextcp ^ sp->noprevcp )) {
	/* Retain the old control points */
    } else {
	unitnext.x = sp->nextcp.x-sp->me.x; unitnext.y = sp->nextcp.y-sp->me.y;
	nextlen = sqrt(unitnext.x*unitnext.x + unitnext.y*unitnext.y);
	unitprev.x = sp->me.x-sp->prevcp.x; unitprev.y = sp->me.y-sp->prevcp.y;
	prevlen = sqrt(unitprev.x*unitprev.x + unitprev.y*unitprev.y);
	makedflt=true;
	if ( nextlen!=0 && prevlen!=0 ) {
	    unitnext.x /= nextlen; unitnext.y /= nextlen;
	    unitprev.x /= prevlen; unitprev.y /= prevlen;
	    if ( unitnext.x*unitprev.x + unitnext.y*unitprev.y>=.95 ) {
		/* If the control points are essentially in the same direction*/
		/*  (so valid for a curve) then leave them as is */
		makedflt = false;
	    }
	}
	if ( makedflt ) {
	    sp->nextcpdef = sp->prevcpdef = true;
	    if (( sp->prev!=NULL && sp->prev->order2 ) ||
		    (sp->next!=NULL && sp->next->order2)) {
		if ( sp->prev!=NULL )
		    SplineRefigureFixup(sp->prev);
		if ( sp->next!=NULL )
		    SplineRefigureFixup(sp->next);
	    } else {
		SplineCharDefaultPrevCP(sp);
		SplineCharDefaultNextCP(sp);
	    }
	}
    }
}

static void _CVMenuPointType(CharView *cv,struct gmenuitem *mi) {
    int pointtype = mi->mid==MID_Corner?pt_corner:mi->mid==MID_Tangent?pt_tangent:pt_curve;
    SplinePointList *spl;
    Spline *spline, *first;

    CVPreserveState(cv);	/* We should only get here if there's a selection */
    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL ; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) {
	    if ( spl->first->pointtype!=pointtype )
		SPChangePointType(spl->first,pointtype);
	}
	for ( spline=spl->first->next; spline!=NULL && spline!=first ; spline = spline->to->next ) {
	    if ( spline->to->selected ) {
	    if ( spline->to->pointtype!=pointtype )
		SPChangePointType(spline->to,pointtype);
	    }
	    if ( first == NULL ) first = spline;
	}
    }
    CVCharChangedUpdate(cv);
}

static void CVMenuPointType(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuPointType(cv,mi);
}

static void _CVMenuImplicit(CharView *cv,struct gmenuitem *mi) {
    SplinePointList *spl;
    Spline *spline, *first;
    int dontinterpolate = mi->mid==MID_NoImplicitPt;

    if ( !cv->sc->parent->order2 )
return;
    CVPreserveState(cv);	/* We should only get here if there's a selection */
    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL ; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) {
	    spl->first->dontinterpolate = dontinterpolate;
	}
	for ( spline=spl->first->next; spline!=NULL && spline!=first ; spline = spline->to->next ) {
	    if ( spline->to->selected ) {
		spline->to->dontinterpolate = dontinterpolate;
	    }
	    if ( first == NULL ) first = spline;
	}
    }
    SCNumberPoints(cv->sc);
    CVCharChangedUpdate(cv);
}

static void CVMenuImplicit(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuImplicit(cv,mi);
}

static void cv_ptlistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e) {
    int type = -2, cnt=0, ccp_cnt=0, spline_selected=0;
    SplinePointList *spl, *sel=NULL;
    Spline *spline, *first;
    SplinePoint *selpt=NULL;
    int notimplicit = -1;

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) {
	    sel = spl;
	    selpt = spl->first; ++cnt;
	    if ( type==-2 ) type = spl->first->pointtype;
	    else if ( type!=spl->first->pointtype ) type = -1;
	    if ( !spl->first->nonextcp && !spl->first->noprevcp && spl->first->prev!=NULL )
		++ccp_cnt;
	    if ( notimplicit==-1 ) notimplicit = spl->first->dontinterpolate;
	    else if ( notimplicit!=spl->first->dontinterpolate ) notimplicit = -2;
	}
	for ( spline=spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	    if ( spline->to->selected ) {
		if ( type==-2 ) type = spline->to->pointtype;
		else if ( type!=spline->to->pointtype ) type = -1;
		selpt = spline->to;
		sel = spl; ++cnt;
		if ( !spline->to->nonextcp && !spline->to->noprevcp && spline->to->next!=NULL )
		    ++ccp_cnt;
		if ( notimplicit==-1 ) notimplicit = spline->to->dontinterpolate;
		else if ( notimplicit!=spline->to->dontinterpolate ) notimplicit = -2;
		if ( spline->from->selected )
		    ++spline_selected;
	    }
	    if ( first == NULL ) first = spline;
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Corner:
	    mi->ti.disabled = type==-2;
	    mi->ti.checked = type==pt_corner;
	  break;
	  case MID_Tangent:
	    mi->ti.disabled = type==-2;
	    mi->ti.checked = type==pt_tangent;
	  break;
	  case MID_Curve:
	    mi->ti.disabled = type==-2;
	    mi->ti.checked = type==pt_curve;
	  break;
	  case MID_MakeFirst:
	    mi->ti.disabled = cnt!=1 || sel->first->prev==NULL || sel->first==selpt;
	  break;
	  case MID_MakeLine:
	    mi->ti.disabled = cnt==0;
	  break;
	  case MID_InsertPtOnSplineAt:
	    mi->ti.disabled = spline_selected!=1;
	  break;
	  case MID_CenterCP:
	    mi->ti.disabled = ccp_cnt==0;
	  break;
	  case MID_ImplicitPt:
	    mi->ti.disabled = !cv->sc->parent->order2;
	    mi->ti.checked = notimplicit==0;
	  break;
	  case MID_NoImplicitPt:
	    mi->ti.disabled = !cv->sc->parent->order2;
	    mi->ti.checked = notimplicit==1;
	  break;
#if 0
	  case MID_AddAnchor:
	    mi->ti.disabled = AnchorClassUnused(cv->sc,&waslig)==NULL;
	  break;
#endif
	}
    }
}

static void ptlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_ptlistcheck(cv,mi,e);
}

static void _CVMenuDir(CharView *cv,struct gmenuitem *mi) {
    int splinepoints, dir;
    SplinePointList *spl;
    Spline *spline, *first;
    int needsrefresh = false;

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	splinepoints = 0;
	if ( spl->first->selected ) splinepoints = true;
	for ( spline=spl->first->next; spline!=NULL && spline!=first && !splinepoints; spline = spline->to->next ) {
	    if ( spline->to->selected ) splinepoints = true;
	    if ( first == NULL ) first = spline;
	}
	if ( splinepoints && spl->first==spl->last && spl->first->next!=NULL ) {
	    dir = SplinePointListIsClockwise(spl);
	    if ( (mi->mid==MID_Clockwise && !dir) || (mi->mid==MID_Counter && dir)) {
		if ( !needsrefresh )
		    CVPreserveState(cv);
		SplineSetReverse(spl);
		needsrefresh = true;
	    }
	}
    }
    if ( needsrefresh )
	CVCharChangedUpdate(cv);
}

static void CVMenuDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuDir(cv,mi);
}

static int getorigin(void *d,BasePoint *base,int index) {
    CharView *cv = (CharView *) d;

    base->x = base->y = 0;
    switch ( index ) {
      case 0:		/* Character origin */
	/* all done */
      break;
      case 1:		/* Center of selection */
	CVFindCenter(cv,base,!CVAnySel(cv,NULL,NULL,NULL,NULL));
      break;
      case 2:		/* last press */
	base->x = cv->p.cx;
	base->y = cv->p.cy;
	/* I don't have any way of telling if a press has happened. if one */
	/*  hasn't they'll just get a 0,0 origin. oh well */
      break;
      default:
return( false );
    }
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void BackgroundImageTransform(SplineChar *sc, ImageList *img,real transform[6]) {
    if ( transform[1]==0 && transform[2]==0 && transform[0]>0 && transform[3]>0 ) {
	img->xoff = transform[0]*img->xoff + transform[4];
	img->yoff = transform[3]*img->yoff + transform[5];
	if (( img->xscale *= transform[0])<0 ) img->xscale = -img->xscale;
	if (( img->yscale *= transform[3])<0 ) img->yscale = -img->yscale;
	img->bb.minx = img->xoff; img->bb.maxy = img->yoff;
	img->bb.maxx = img->xoff + GImageGetWidth(img->image)*img->xscale;
	img->bb.miny = img->yoff - GImageGetHeight(img->image)*img->yscale;
    } else
	/* Don't support rotating, flipping or skewing images */;
    SCOutOfDateBackground(sc);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void CVTransFunc(CharView *cv,real transform[6], enum fvtrans_flags flags) {
    int anysel = cv->p.transany;
    RefChar *refs;
    ImageList *img;
    real t[6];
    AnchorPoint *ap;
    KernPair *kp;
    PST *pst;
    int j;

    SplinePointListTransform(cv->layerheads[cv->drawmode]->splines,transform,!anysel);
    if ( flags&fvt_round_to_int )
	SplineSetsRound2Int(cv->layerheads[cv->drawmode]->splines,1.0);
    if ( cv->layerheads[cv->drawmode]->images!=NULL ) {
	ImageListTransform(cv->layerheads[cv->drawmode]->images,transform);
	SCOutOfDateBackground(cv->sc);
    }
    if ( cv->drawmode==dm_fore ) {
	for ( refs = cv->layerheads[cv->drawmode]->refs; refs!=NULL; refs=refs->next )
	    if ( refs->selected || !anysel ) {
		for ( j=0; j<refs->layer_cnt; ++j )
		    SplinePointListTransform(refs->layers[j].splines,transform,true);
		t[0] = refs->transform[0]*transform[0] +
			    refs->transform[1]*transform[2];
		t[1] = refs->transform[0]*transform[1] +
			    refs->transform[1]*transform[3];
		t[2] = refs->transform[2]*transform[0] +
			    refs->transform[3]*transform[2];
		t[3] = refs->transform[2]*transform[1] +
			    refs->transform[3]*transform[3];
		t[4] = refs->transform[4]*transform[0] +
			    refs->transform[5]*transform[2] +
			    transform[4];
		t[5] = refs->transform[4]*transform[1] +
			    refs->transform[5]*transform[3] +
			    transform[5];
		if ( flags&fvt_round_to_int ) {
		    t[4] = rint( t[4] );
		    t[5] = rint( t[5] );
		}
		memcpy(refs->transform,t,sizeof(t));
		RefCharFindBounds(refs);
	    }
	if ( cv->showanchor ) {
	    for ( ap=cv->sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->selected || !anysel )
		ApTransform(ap,transform);
	}
	if ( flags & fvt_scalepstpos ) {
	    for ( kp=cv->sc->kerns; kp!=NULL; kp=kp->next )
		kp->off = rint(kp->off*transform[0]);
	    for ( kp=cv->sc->vkerns; kp!=NULL; kp=kp->next )
		kp->off = rint(kp->off*transform[3]);
	    for ( pst = cv->sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_position )
		    VrTrans(&pst->u.pos,transform);
		else if ( pst->type==pst_pair ) {
		    VrTrans(&pst->u.pair.vr[0],transform);
		    VrTrans(&pst->u.pair.vr[1],transform);
		}
	    }
	}
	if ( transform[1]==0 && transform[2]==0 ) {
	    TransHints(cv->sc->hstem,transform[3],transform[5],transform[0],transform[4],flags&fvt_round_to_int);
	    TransHints(cv->sc->vstem,transform[0],transform[4],transform[3],transform[5],flags&fvt_round_to_int);
	}
	if ( transform[0]==1 && transform[3]==1 && transform[1]==0 &&
		transform[2]==0 && transform[5]==0 &&
		transform[4]!=0 && CVAllSelected(cv) &&
		cv->sc->unicodeenc!=-1 && isalpha(cv->sc->unicodeenc)) {
	    SCUndoSetLBearingChange(cv->sc,(int) rint(transform[4]));
	    SCSynchronizeLBearing(cv->sc,transform[4]);
	}
	if ( !(flags&fvt_dontmovewidth) && (cv->widthsel || !anysel))
	    if ( transform[0]>0 && transform[3]>0 && transform[1]==0 &&
		    transform[2]==0 && transform[4]!=0 )
		SCSynchronizeWidth(cv->sc,cv->sc->width*transform[0]+transform[4],cv->sc->width,NULL);
	if ( !(flags&fvt_dontmovewidth) && (cv->vwidthsel || !anysel))
	    if ( transform[0]==1 && transform[3]==1 && transform[1]==0 &&
		    transform[2]==0 && transform[5]!=0 )
		cv->sc->vwidth+=transform[5];
	if ( (flags&fvt_dobackground) && !anysel ) {
	    SCPreserveBackground(cv->sc);
	    for ( img = cv->sc->layers[ly_back].images; img!=NULL; img=img->next )
		BackgroundImageTransform(cv->sc, img, transform);
	    SplinePointListTransform(cv->layerheads[cv->drawmode]->splines,
		    transform,true);
	}
    }
}

static void transfunc(void *d,real transform[6],int otype,BVTFunc *bvts,
	enum fvtrans_flags flags) {
    CharView *cv = (CharView *) d;
    int anya;

    cv->p.transany = CVAnySel(cv,NULL,NULL,NULL,&anya);
    CVPreserveStateHints(cv);
    if ( cv->drawmode==dm_fore && (flags&fvt_dobackground) )
	SCPreserveBackground(cv->sc);
    CVTransFunc(cv,transform,flags);
    CVCharChangedUpdate(cv);
}

void CVDoTransform(CharView *cv, enum cvtools cvt ) {
    int anysel = CVAnySel(cv,NULL,NULL,NULL,NULL);
    TransformDlgCreate(cv,transfunc,getorigin,!anysel && cv->drawmode==dm_fore,
	cvt);
}

static void CVMenuTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVDoTransform(cv,cvt_none);
}

static void CVMenuPOV(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    struct pov_data pov_data;
    if ( PointOfViewDlg(&pov_data,cv->sc->parent,true)==-1 )
return;
    CVPointOfView(cv,&pov_data);
}

static void CVMenuNLTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    NonLinearDlg(NULL,cv);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static void SplinePointRound(SplinePoint *sp,real factor) {

    sp->nextcp.x = rint(sp->nextcp.x*factor)/factor;
    sp->nextcp.y = rint(sp->nextcp.y*factor)/factor;
    if ( sp->next!=NULL && sp->next->order2 )
	sp->next->to->prevcp = sp->nextcp;
    sp->prevcp.x = rint(sp->prevcp.x*factor)/factor;
    sp->prevcp.y = rint(sp->prevcp.y*factor)/factor;
    if ( sp->prev!=NULL && sp->prev->order2 )
	sp->prev->from->nextcp = sp->prevcp;
    if ( sp->prev!=NULL && sp->next!=NULL && sp->next->order2 &&
	    sp->ttfindex == 0xffff ) {
	sp->me.x = (sp->nextcp.x + sp->prevcp.x)/2;
	sp->me.y = (sp->nextcp.y + sp->prevcp.y)/2;
    } else {
	sp->me.x = rint(sp->me.x*factor)/factor;
	sp->me.y = rint(sp->me.y*factor)/factor;
    }
}

void SplineSetsRound2Int(SplineSet *spl,real factor) {
    SplinePoint *sp;

    for ( ; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    SplinePointRound(sp,factor);
	    if ( sp->prev!=NULL )
		SplineRefigure(sp->prev);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( spl->first->prev!=NULL )
	    SplineRefigure(spl->first->prev);
    }
}

static void SplineSetsChangeCoord(SplineSet *spl,real old, real new,int isy) {
    SplinePoint *sp;

    for ( ; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( isy ) {
		if ( RealNear(sp->me.y,old) ) {
		    if ( RealNear(sp->nextcp.y,old))
			sp->nextcp.y = new;
		    else
			sp->nextcp.y += new-sp->me.y;
		    if ( RealNear(sp->prevcp.y,old))
			sp->prevcp.y = new;
		    else
			sp->prevcp.y += new-sp->me.y;
		    sp->me.y = new;
		    /* we expect to be called before SplineSetRound2Int and will */
		    /*  allow it to do any SplineRefigures */
		}
	    } else {
		if ( RealNear(sp->me.x,old) ) {
		    if ( RealNear(sp->nextcp.x,old))
			sp->nextcp.x = new;
		    else
			sp->nextcp.x += new-sp->me.x;
		    if ( RealNear(sp->prevcp.x,old))
			sp->prevcp.x = new;
		    else
			sp->prevcp.x += new-sp->me.x;
		    sp->me.x = new;
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( spl->first->prev!=NULL )
	    SplineRefigure(spl->first->prev);
    }
}

void SCRound2Int(SplineChar *sc,real factor) {
    RefChar *r;
    StemInfo *stems;
    real old, new;
    int layer;

    for ( stems = sc->hstem; stems!=NULL; stems=stems->next ) {
	old = stems->start+stems->width;
	stems->start = rint(stems->start*factor)/factor;
	stems->width = rint(stems->width*factor)/factor;
	new = stems->start+stems->width;
	if ( old!=new )
	    SplineSetsChangeCoord(sc->layers[ly_fore].splines,old,new,true);
    }
    for ( stems = sc->vstem; stems!=NULL; stems=stems->next ) {
	old = stems->start+stems->width;
	stems->start = rint(stems->start*factor)/factor;
	stems->width = rint(stems->width*factor)/factor;
	new = stems->start+stems->width;
	if ( old!=new )
	    SplineSetsChangeCoord(sc->layers[ly_fore].splines,old,new,false);
    }

    for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
	SplineSetsRound2Int(sc->layers[layer].splines,factor);
	for ( r=sc->layers[layer].refs; r!=NULL; r=r->next ) {
	    r->transform[4] = rint(r->transform[4]*factor)/factor;
	    r->transform[5] = rint(r->transform[5]*factor)/factor;
	    RefCharFindBounds(r);
	}
    }
    SCCharChangedUpdate(sc);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void CVMenuConstrain(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVConstrainSelection(cv,mi->mid==MID_Average?0:
			    mi->mid==MID_SpacePts?1:
			    2);
}

static void CVMenuMakeParallel(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVMakeParallel(cv);
}

static void _CVMenuRound2Int(CharView *cv, double factor) {
    int anysel = CVAnySel(cv,NULL,NULL,NULL,NULL);
    SplinePointList *spl;
    SplinePoint *sp;
    RefChar *r;

    CVPreserveState(cv);
    for ( spl= cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( sp->selected || !anysel )
		SplinePointRound(sp,factor);
	    if ( sp->prev!=NULL )
		SplineRefigure(sp->prev);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( spl->first->prev!=NULL )
	    SplineRefigure(spl->first->prev);
    }
    if ( cv->drawmode==dm_fore ) {
	for ( r=cv->sc->layers[ly_fore].refs; r!=NULL; r=r->next ) {
	    if ( r->selected || !anysel ) {
		r->transform[4] = rint(r->transform[4]*factor)/factor;
		r->transform[5] = rint(r->transform[5]*factor)/factor;
	    }
	}
    }
    CVCharChangedUpdate(cv);
}

static void CVMenuRound2Int(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuRound2Int(cv,1.0);
}

static void CVMenuRound2Hundredths(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuRound2Int(cv,100.0);
}

static void CVMenuCluster(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int layer = cv->drawmode == dm_grid ? -1 :
		cv->drawmode == dm_back ? 0
					: cv->layerheads[dm_fore] - cv->sc->layers;
    SCRoundToCluster(cv->sc,layer,true,.1,.5);
}

static void CVMenuStroke(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVStroke(cv);
}

#ifdef FONTFORGE_CONFIG_TILEPATH
static void CVMenuTilePath(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVTile(cv);
}
#endif

static void _CVMenuOverlap(CharView *cv,enum overlap_type ot) {
    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    int layer = cv->drawmode == dm_grid ? -1 :
		cv->drawmode == dm_back ? 0
					: cv->layerheads[dm_fore] - cv->sc->layers;

    DoAutoSaves();
    if ( !SCRoundToCluster(cv->sc,layer,false,.03,.12))
	CVPreserveState(cv);	/* SCRound2Cluster does this when it makes a change, not otherwise */
    if ( cv->drawmode==dm_fore ) {
	MinimumDistancesFree(cv->sc->md);
	cv->sc->md = NULL;
    }
    cv->layerheads[cv->drawmode]->splines = SplineSetRemoveOverlap(cv->sc,cv->layerheads[cv->drawmode]->splines,ot);
    CVCharChangedUpdate(cv);
}

static void CVMenuOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anysel;

    (void) CVAnySel(cv,&anysel,NULL,NULL,NULL);
    _CVMenuOverlap(cv,mi->mid==MID_RmOverlap ? (anysel ? over_rmselected: over_remove) :
		      mi->mid==MID_Intersection ? (anysel ? over_intersel : over_intersect ) :
		      mi->mid==MID_Exclude ? over_exclude :
			  (anysel ? over_fisel : over_findinter));
}

static void CVMenuOrder(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePointList *spl;
    RefChar *r;
    ImageList *im;
    int exactlyone = CVOneContourSel(cv,&spl,&r,&im);

    if ( !exactlyone )
return;

    CVPreserveState(cv);
    if ( spl!=NULL ) {
	SplinePointList *p, *pp, *t;
	p = pp = NULL;
	for ( t=cv->layerheads[cv->drawmode]->splines; t!=NULL && t!=spl; t=t->next ) {
	    pp = p; p = t;
	}
	switch ( mi->mid ) {
	  case MID_First:
	    if ( p!=NULL ) {
		p->next = spl->next;
		spl->next = cv->layerheads[cv->drawmode]->splines;
		cv->layerheads[cv->drawmode]->splines = spl;
	    }
	  break;
	  case MID_Earlier:
	    if ( p!=NULL ) {
		p->next = spl->next;
		spl->next = p;
		if ( pp==NULL ) {
		    cv->layerheads[cv->drawmode]->splines = spl;
		} else {
		    pp->next = spl;
		}
	    }
	  break;
	  case MID_Last:
	    if ( spl->next!=NULL ) {
		for ( t=cv->layerheads[cv->drawmode]->splines; t->next!=NULL; t=t->next );
		t->next = spl;
		if ( p==NULL )
		    cv->layerheads[cv->drawmode]->splines = spl->next;
		else
		    p->next = spl->next;
		spl->next = NULL;
	    }
	  break;
	  case MID_Later:
	    if ( spl->next!=NULL ) {
		t = spl->next;
		spl->next = t->next;
		t->next = spl;
		if ( p==NULL )
		    cv->layerheads[cv->drawmode]->splines = t;
		else
		    p->next = t;
	    }
	  break;
	}
    } else if ( r!=NULL ) {
	RefChar *p, *pp, *t;
	p = pp = NULL;
	for ( t=cv->sc->layers[ly_fore].refs; t!=NULL && t!=r; t=t->next ) {
	    pp = p; p = t;
	}
	switch ( mi->mid ) {
	  case MID_First:
	    if ( p!=NULL ) {
		p->next = r->next;
		r->next = cv->sc->layers[ly_fore].refs;
		cv->sc->layers[ly_fore].refs = r;
	    }
	  break;
	  case MID_Earlier:
	    if ( p!=NULL ) {
		p->next = r->next;
		r->next = p;
		if ( pp==NULL ) {
		    cv->sc->layers[ly_fore].refs = r;
		} else {
		    pp->next = r;
		}
	    }
	  break;
	  case MID_Last:
	    if ( r->next!=NULL ) {
		for ( t=cv->sc->layers[ly_fore].refs; t->next!=NULL; t=t->next );
		t->next = r;
		if ( p==NULL )
		    cv->sc->layers[ly_fore].refs = r->next;
		else
		    p->next = r->next;
		r->next = NULL;
	    }
	  break;
	  case MID_Later:
	    if ( r->next!=NULL ) {
		t = r->next;
		r->next = t->next;
		t->next = r;
		if ( p==NULL )
		    cv->sc->layers[ly_fore].refs = t;
		else
		    p->next = t;
	    }
	  break;
	}
    } else if ( im!=NULL ) {
	ImageList *p, *pp, *t;
	p = pp = NULL;
	for ( t=cv->layerheads[cv->drawmode]->images; t!=NULL && t!=im; t=t->next ) {
	    pp = p; p = t;
	}
	switch ( mi->mid ) {
	  case MID_First:
	    if ( p!=NULL ) {
		p->next = im->next;
		im->next = cv->layerheads[cv->drawmode]->images;
		cv->layerheads[cv->drawmode]->images = im;
	    }
	  break;
	  case MID_Earlier:
	    if ( p!=NULL ) {
		p->next = im->next;
		im->next = p;
		if ( pp==NULL ) {
		    cv->layerheads[cv->drawmode]->images = im;
		} else {
		    pp->next = im;
		}
	    }
	  break;
	  case MID_Last:
	    if ( im->next!=NULL ) {
		for ( t=cv->layerheads[cv->drawmode]->images; t->next!=NULL; t=t->next );
		t->next = im;
		if ( p==NULL )
		    cv->layerheads[cv->drawmode]->images = im->next;
		else
		    p->next = im->next;
		im->next = NULL;
	    }
	  break;
	  case MID_Later:
	    if ( im->next!=NULL ) {
		t = im->next;
		im->next = t->next;
		t->next = im;
		if ( p==NULL )
		    cv->layerheads[cv->drawmode]->images = t;
		else
		    p->next = t;
	    }
	  break;
	}
    }
    CVCharChangedUpdate(cv);
}

static void _CVMenuAddExtrema(CharView *cv) {
    int anysel;

    (void) CVAnySel(cv,&anysel,NULL,NULL,NULL);
    CVPreserveState(cv);
    SplineCharAddExtrema(cv->sc,cv->layerheads[cv->drawmode]->splines,
	    anysel?ae_between_selected:ae_only_good,cv->sc->parent);
    CVCharChangedUpdate(cv);
}

static void CVMenuAddExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuAddExtrema(cv);
}

static void CVSimplify(CharView *cv,int type) {
    static struct simplifyinfo smpls[] = {
	    { sf_normal },
	    { sf_normal,.75,.05,0,-1 },
	    { sf_normal,.75,.05,0,-1 }};
    struct simplifyinfo *smpl = &smpls[type+1];

    if ( smpl->linelenmax==-1 || (type==0 && !smpl->set_as_default)) {
	smpl->err = (cv->sc->parent->ascent+cv->sc->parent->descent)/1000.;
	smpl->linelenmax = (cv->sc->parent->ascent+cv->sc->parent->descent)/100.;
    }

    if ( type==1 ) {
	if ( !SimplifyDlg(cv->sc->parent,smpl))
return;
	if ( smpl->set_as_default )
	    smpls[1] = *smpl;
    }

    CVPreserveState(cv);
    cv->layerheads[cv->drawmode]->splines = SplineCharSimplify(cv->sc,cv->layerheads[cv->drawmode]->splines,
	    smpl);
    CVCharChangedUpdate(cv);
}

static void CVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSimplify(cv,0);
}

static void CVMenuSimplifyMore(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSimplify(cv,1);
}

static void CVMenuCleanupGlyph(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSimplify(cv,-1);
}

static int SPLSelected(SplineSet *ss) {
    SplinePoint *sp;

    for ( sp=ss->first ;; ) {
	if ( sp->selected )
return( true );
	if ( sp->next==NULL )
return( false );
	sp = sp->next->to;
	if ( sp==ss->first )
return( false );
    }
}

static void CVCanonicalStart(CharView *cv) {
    SplineSet *ss;
    int changed = 0;

    for ( ss = cv->layerheads[cv->drawmode]->splines; ss!=NULL; ss=ss->next )
	if ( ss->first==ss->last && SPLSelected(ss))
	    SPLStartToLeftmost(cv->sc,ss,&changed);
}

static void CVMenuCanonicalStart(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVCanonicalStart(cv);
}

static void CVCanonicalContour(CharView *cv) {
    if ( cv->drawmode==dm_fore )
	CanonicalContours(cv->sc);
}

static void CVMenuCanonicalContours(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVCanonicalContour(cv);
}

static void _CVMenuMakeFirst(CharView *cv) {
    SplinePoint *selpt = NULL;
    int anypoints = 0, splinepoints;
    SplinePointList *spl, *sel;
    Spline *spline, *first;

    sel = NULL;
    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	splinepoints = 0;
	if ( spl->first->selected ) { splinepoints = 1; sel = spl; selpt=spl->first; }
	for ( spline=spl->first->next; spline!=NULL && spline!=first && !splinepoints; spline = spline->to->next ) {
	    if ( spline->to->selected ) { ++splinepoints; sel = spl; selpt=spline->to; }
	    if ( first == NULL ) first = spline;
	}
	anypoints += splinepoints;
    }

    if ( anypoints!=1 || sel->first->prev==NULL || sel->first==selpt )
return;

    CVPreserveState(cv);
    sel->first = sel->last = selpt;
    CVCharChangedUpdate(cv);
}

static void CVMenuMakeFirst(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuMakeFirst(cv);
}

static void _CVMenuMakeLine(CharView *cv) {
    SplinePointList *spl;
    SplinePoint *sp;
    int changed = false;

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( sp->selected ) {
		int any = false;
		if ( !changed ) {
		    CVPreserveState(cv);
		    changed = true;
		}
		if ( sp->prev!=NULL && sp->prev->from->selected ) {
		    any = true;
		    sp->prevcp = sp->me;
		    sp->noprevcp = true;
		    sp->prev->from->nextcp = sp->prev->from->me;
		    sp->prev->from->nonextcp = true;
		    SplineRefigure(sp->prev);
		}
		if ( sp->next!=NULL && sp->next->to->selected ) {
		    any = true;
		    sp->nextcp = sp->me;
		    sp->nonextcp = true;
		    sp->next->to->prevcp = sp->next->to->me;
		    sp->next->to->noprevcp = true;
		    SplineRefigure(sp->next);
		}
		if ( !any ) {
		    sp->nextcp = sp->me;
		    sp->prevcp = sp->me;
		    sp->nonextcp = sp->noprevcp = true;
		    if ( sp->prev!=NULL ) SplineRefigure(sp->prev);
		    if ( sp->next!=NULL ) SplineRefigure(sp->next);
		}
		changed = true;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    if ( changed )
	CVCharChangedUpdate(cv);
}

static void CVMenuMakeLine(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuMakeLine(cv);
}

struct insertonsplineat {
    int done;
    GWindow gw;
    Spline *s;
    CharView *cv;
};

#define CID_X	1001
#define CID_Y	1002
#define CID_XR	1003
#define CID_YR	1004

static int IOSA_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int err = false;
	struct insertonsplineat *iosa = GDrawGetUserData(GGadgetGetWindow(g));
	double val;
	extended ts[3];
	int which;
	SplinePoint *sp;

	if ( GGadgetIsChecked(GWidgetGetControl(iosa->gw,CID_XR)) ) {
	    val = GetReal8(iosa->gw,CID_X,"X",&err);
	    which = 0;
	} else {
	    val = GetReal8(iosa->gw,CID_Y,"Y",&err);
	    which = 1;
	}
	if ( err )
return(true);
	if ( SplineSolveFull(&iosa->s->splines[which],val,ts)==0 ) {
	    gwwv_post_error(_("Out of Range"),_("The spline does not reach %g"), (double) val );
return( true );
	}
	iosa->done = true;
	CVPreserveState(iosa->cv);
	forever {
	    sp = SplineBisect(iosa->s,ts[0]);
	    SplinePointCatagorize(sp);
	    if ( which==0 ) {
		double off = val-sp->me.x;
		sp->me.x = val; sp->nextcp.x += off; sp->prevcp.x += off;
	    } else {
		double off = val-sp->me.y;
		sp->me.y = val; sp->nextcp.y += off; sp->prevcp.y += off;
	    }
	    SplineRefigure(sp->prev); SplineRefigure(sp->next);
	    if ( ts[1]==-1 ) {
		CVCharChangedUpdate(iosa->cv);
return( true );
	    }
	    iosa->s = sp->next;
	    if ( SplineSolveFull(&iosa->s->splines[which],val,ts)==0 ) {
		/* Odd. We found one earlier */
		CVCharChangedUpdate(iosa->cv);
return( true );
	    }
	}
    }
return( true );
}

static int IOSA_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct insertonsplineat *iosa = GDrawGetUserData(GGadgetGetWindow(g));
	iosa->done = true;
    }
return( true );
}

static int IOSA_FocusChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged ) {
	struct insertonsplineat *iosa = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = (intpt) GGadgetGetUserData(g);
	GGadgetSetChecked(GWidgetGetControl(iosa->gw,cid),true);
    }
return( true );
}

static int IOSA_RadioChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct insertonsplineat *iosa = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = (intpt) GGadgetGetUserData(g);
	GWidgetIndicateFocusGadget(GWidgetGetControl(iosa->gw,cid));
	GTextFieldSelect(GWidgetGetControl(iosa->gw,cid),0,-1);
    }
return( true );
}

static int iosa_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct insertonsplineat *iosa = GDrawGetUserData(gw);
	iosa->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}
static void _CVMenuInsertPt(CharView *cv) {
    SplineSet *spl;
    Spline *s, *found=NULL, *first;
    struct insertonsplineat iosa;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[11], boxes[2], topbox[2], *hvs[13], *varray[8], *buttons[6];
    GTextInfo label[11];

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( s=spl->first->next; s!=NULL && s!=first ; s = s->to->next ) {
	    if ( first==NULL ) first=s;
	    if ( s->from->selected && s->to->selected ) {
		if ( found!=NULL )
return;		/* Can only work with one spline */
		found = s;
	    }
	}
    }
    if ( found==NULL )
return;		/* Need a spline */

    memset(&iosa,0,sizeof(iosa));
    iosa.s = found;
    iosa.cv = cv;
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Insert a point on the given spline at either...");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,210));
    pos.height = GDrawPointsToPixels(NULL,120);
    iosa.gw = GDrawCreateTopWindow(NULL,&pos,iosa_e_h,&iosa,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _("_X:");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[0].gd.cid = CID_XR;
    gcd[0].gd.handle_controlevent = IOSA_RadioChange;
    gcd[0].data = (void *) CID_X;
    gcd[0].creator = GRadioCreate;

    label[1].text = (unichar_t *) _("_Y:");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 32; 
    gcd[1].gd.flags = gg_enabled|gg_visible|gg_rad_continueold ;
    gcd[1].gd.cid = CID_YR;
    gcd[1].gd.handle_controlevent = IOSA_RadioChange;
    gcd[1].data = (void *) CID_Y;
    gcd[1].creator = GRadioCreate;

    gcd[2].gd.pos.x = 131; gcd[2].gd.pos.y = 5;  gcd[2].gd.pos.width = 60;
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].gd.cid = CID_X;
    gcd[2].gd.handle_controlevent = IOSA_FocusChange;
    gcd[2].data = (void *) CID_XR;
    gcd[2].creator = GTextFieldCreate;

    gcd[3].gd.pos.x = 131; gcd[3].gd.pos.y = 32;  gcd[3].gd.pos.width = 60;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Y;
    gcd[3].gd.handle_controlevent = IOSA_FocusChange;
    gcd[3].data = (void *) CID_YR;
    gcd[3].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 120-32-3;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _("_OK");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.mnemonic = 'O';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = IOSA_OK;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = -20; gcd[5].gd.pos.y = 120-32;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _("_Cancel");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.mnemonic = 'C';
    gcd[5].gd.handle_controlevent = IOSA_Cancel;
    gcd[5].creator = GButtonCreate;

    hvs[0] = &gcd[0]; hvs[1] = &gcd[2]; hvs[2] = NULL;
    hvs[3] = &gcd[1]; hvs[4] = &gcd[3]; hvs[5] = NULL;
    hvs[6] = NULL;

    buttons[0] = buttons[2] = buttons[4] = GCD_Glue; buttons[5] = NULL;
    buttons[1] = &gcd[4]; buttons[3] = &gcd[5];

    varray[0] = &boxes[1]; varray[1] = NULL;
    varray[2] = GCD_Glue; varray[3] = NULL;
    varray[4] = &boxes[0]; varray[5] = NULL;
    varray[6] = NULL;

    memset(boxes,0,sizeof(boxes));
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = buttons;
    boxes[0].creator = GHBoxCreate;

    boxes[1].gd.flags = gg_enabled|gg_visible;
    boxes[1].gd.u.boxelements = hvs;
    boxes[1].creator = GHVBoxCreate;

    memset(topbox,0,sizeof(topbox));
    topbox[0].gd.pos.x = topbox[0].gd.pos.y = 2;
    topbox[0].gd.pos.width = pos.width-4; topbox[0].gd.pos.height = pos.height-4;
    topbox[0].gd.flags = gg_enabled|gg_visible;
    topbox[0].gd.u.boxelements = varray;
    topbox[0].creator = GHVGroupCreate;
	

    GGadgetsCreate(iosa.gw,topbox);
    GHVBoxSetExpandableRow(topbox[0].ret,1);
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[1].ret,1);
    GWidgetIndicateFocusGadget(GWidgetGetControl(iosa.gw,CID_X));
    GTextFieldSelect(GWidgetGetControl(iosa.gw,CID_X),0,-1);
    GHVBoxFitWindow(topbox[0].ret);

    GDrawSetVisible(iosa.gw,true);
    while ( !iosa.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(iosa.gw);
}

static void CVMenuInsertPt(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuInsertPt(cv);
}

static void _CVCenterCP(CharView *cv) {
    SplinePointList *spl;
    SplinePoint *sp;
    int changed = false;
    enum movething { mt_pt, mt_ncp, mt_pcp } movething = mt_pt;

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( sp->selected && sp->prev!=NULL && sp->next!=NULL &&
		    !sp->noprevcp && !sp->nonextcp ) {
		if ( sp->me.x != (sp->nextcp.x+sp->prevcp.x)/2 ||
			sp->me.y != (sp->nextcp.y+sp->prevcp.y)/2 ) {
		    if ( !changed ) {
			CVPreserveState(cv);
			changed = true;
		    }
		    switch ( movething ) {
		      case mt_pt:
			sp->me.x = (sp->nextcp.x+sp->prevcp.x)/2;
			sp->me.y = (sp->nextcp.y+sp->prevcp.y)/2;
			SplineRefigure(sp->prev);
			SplineRefigure(sp->next);
		      break;
		      case mt_ncp:
			sp->nextcp.x = sp->me.x - (sp->prevcp.x-sp->me.x);
			sp->nextcp.y = sp->me.y - (sp->prevcp.y-sp->me.y);
			if ( sp->next->order2 ) {
			    sp->next->to->prevcp = sp->nextcp;
			    sp->next->to->noprevcp = false;
			}
			SplineRefigure(sp->prev);
			SplineRefigureFixup(sp->next);
		      break;
		      case mt_pcp:
			sp->prevcp.x = sp->me.x - (sp->nextcp.x-sp->me.x);
			sp->prevcp.y = sp->me.y - (sp->nextcp.y-sp->me.y);
			if ( sp->prev->order2 ) {
			    sp->prev->from->nextcp = sp->prevcp;
			    sp->prev->from->nonextcp = false;
			}
			SplineRefigureFixup(sp->prev);
			SplineRefigure(sp->next);
		      break;
		    }
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    if ( changed )
	CVCharChangedUpdate(cv);
}

static void CVMenuCenterCP(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVCenterCP(cv);
}

void CVAddAnchor(CharView *cv) {
    int waslig;

    if ( AnchorClassUnused(cv->sc,&waslig)==NULL ) {
	ff_post_notice(_("Make a new anchor class"),_("I cannot find an unused anchor class\nto assign a new point to. If you\nwish a new anchor point you must\ndefine a new anchor class with\nElement->Font Info"));
	FontInfo(cv->sc->parent,8,true);		/* Anchor Class */
	if ( AnchorClassUnused(cv->sc,&waslig)==NULL )
return;
    }
    ApGetInfo(cv,NULL);
}

static void CVMenuAddAnchor(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVAddAnchor(cv);
}

static void CVMenuAutotrace(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCAutoTrace(cv->sc,cv->v,e!=NULL && (e->u.mouse.state&ksm_shift));
}

static void CVMenuBuildAccent(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    extern int onlycopydisplayed;

    if ( SFIsRotatable(cv->fv->sf,cv->sc))
	/* It's ok */;
    else if ( !SFIsSomethingBuildable(cv->fv->sf,cv->sc,true) )
return;
    SCBuildComposit(cv->fv->sf,cv->sc,!onlycopydisplayed,cv->fv);
}

static void CVMenuBuildComposite(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    extern int onlycopydisplayed;

    if ( SFIsRotatable(cv->fv->sf,cv->sc))
	/* It's ok */;
    else if ( !SFIsCompositBuildable(cv->fv->sf,cv->sc->unicodeenc,cv->sc) )
return;
    SCBuildComposit(cv->fv->sf,cv->sc,!onlycopydisplayed,cv->fv);
}

static void CVMenuCorrectDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int changed=false, refchanged=false;
    RefChar *ref;
    int asked=-1;

    if ( cv->drawmode==dm_fore ) for ( ref=cv->sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[0]*ref->transform[3]<0 ||
		(ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
	    if ( asked==-1 ) {
		char *buts[4];
		buts[0] = _("_Unlink");
#if defined(FONTFORGE_CONFIG_GDRAW)
		buts[1] = _("_No");
		buts[2] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
		buts[1] = GTK_STOCK_NO;
		buts[2] = GTK_STOCK_CANCEL;
#endif
		buts[3] = NULL;
		asked = gwwv_ask(_("Flipped Reference"),(const char **) buts,0,2,_("%.50s contains a flipped reference. This cannot be corrected as is. Would you like me to unlink it and then correct it?"), cv->sc->name );
		if ( asked==2 )
return;
		else if ( asked==1 )
    break;
	    }
	    if ( asked==0 ) {
		if ( !refchanged ) {
		    refchanged = true;
		    CVPreserveState(cv);
		}
		SCRefToSplines(cv->sc,ref);
	    }
	}
    }

    if ( !refchanged )
	CVPreserveState(cv);
	
    cv->layerheads[cv->drawmode]->splines = SplineSetsCorrect(cv->layerheads[cv->drawmode]->splines,&changed);
    if ( changed || refchanged )
	CVCharChangedUpdate(cv);
}

static void CVMenuGetInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVGetInfo(cv);
}

static void CVMenuCharInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCCharInfo(cv->sc,cv->fv->map,CVCurEnc(cv));
}

static void CVMenuShowDependentRefs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCRefBy(cv->sc);
}

static void CVMenuShowDependentSubs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCSubBy(cv->sc);
}

static void CVMenuBitmaps(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    BitmapDlg(cv->fv,cv->sc,mi->mid==MID_RemoveBitmaps?-1: (mi->mid==MID_AvailBitmaps) );
}

static void cv_allistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e) {
    int selpoints = 0;
    SplinePointList *spl;
    SplinePoint *sp=NULL;

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	sp=spl->first;
	while ( 1 ) {
	    if ( sp->selected )
		++selpoints;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Average:
	    mi->ti.disabled = selpoints<2;
	  break;
	  case MID_SpacePts:
	    mi->ti.disabled = ((selpoints<3) && (selpoints!=1));
	  break;
	  case MID_SpaceRegion:
	    mi->ti.disabled = selpoints<3;
	  break;
	  case MID_MakeParallel:
	    mi->ti.disabled = selpoints!=4;
	  break;
        }
    }
}

static void allistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_allistcheck(cv,mi,e);
}

static void cv_balistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e) {

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_BuildAccent:
	    mi->ti.disabled = !SFIsSomethingBuildable(cv->fv->sf,cv->sc,true);
	  break;
	  case MID_BuildComposite:
	    mi->ti.disabled = !SFIsSomethingBuildable(cv->fv->sf,cv->sc,false);
	  break;
        }
    }
}

static void balistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_balistcheck(cv,mi,e);
}

static void delistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_ShowDependentRefs:
	    mi->ti.disabled = cv->sc->dependents==NULL;
	  break;
	  case MID_ShowDependentSubs:
	    mi->ti.disabled = !SCUsedBySubs(cv->sc);
	  break;
	}
    }
}

static void cv_ellistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e,int is_cv) {
    int anypoints = 0, splinepoints, dir = -2;
    SplinePointList *spl;
    Spline *spline, *first;
    AnchorPoint *ap;
    int order2 = cv->sc->parent->order2;

#ifdef FONTFORGE_CONFIG_TILEPATH
    int badsel = false;
    RefChar *ref;
    ImageList *il;

    for ( ref=cv->layerheads[cv->drawmode]->refs; ref!=NULL; ref=ref->next )
	if ( ref->selected )
	    badsel = true;

    for ( il=cv->layerheads[cv->drawmode]->images; il!=NULL; il=il->next )
	if ( il->selected )
	    badsel = true;
#endif

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	splinepoints = 0;
	if ( spl->first->selected ) { splinepoints = 1; }
	for ( spline=spl->first->next; spline!=NULL && spline!=first && !splinepoints; spline = spline->to->next ) {
	    if ( spline->to->selected ) { ++splinepoints; }
	    if ( first == NULL ) first = spline;
	}
	if ( splinepoints ) {
	    anypoints += splinepoints;
	    if ( dir==-1 )
		/* Do nothing */;
	    else if ( spl->first!=spl->last || spl->first->next==NULL ) {
		if ( dir==-2 || dir==2 )
		    dir = 2;	/* Not a closed path, no direction */
		else
		    dir = -1;
	    } else if ( dir==-2 )
		dir = SplinePointListIsClockwise(spl);
	    else {
		int subdir = SplinePointListIsClockwise(spl);
		if ( subdir!=dir )
		    dir = -1;
	    }
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_GetInfo:
	    {
		SplinePoint *sp; SplineSet *spl; RefChar *ref; ImageList *img;
		mi->ti.disabled = !CVOneThingSel(cv,&sp,&spl,&ref,&img,&ap);
	    }
	  break;
	  case MID_Clockwise:
	    mi->ti.disabled = !anypoints || dir==2;
	    mi->ti.checked = dir==1;
	  break;
	  case MID_Counter:
	    mi->ti.disabled = !anypoints || dir==2;
	    mi->ti.checked = dir==0;
	  break;
	  case MID_Correct:
	    mi->ti.disabled = !anypoints || dir==2;
	  break;
	  case MID_MetaFont:
	    mi->ti.disabled = cv->drawmode!=dm_fore || cv->sc->layers[ly_fore].refs!=NULL || order2;
	  break;
	  case MID_Stroke:
	  case MID_RmOverlap:
	    mi->ti.disabled = cv->layerheads[cv->drawmode]->splines==NULL;
	  break;
	  case MID_Effects:
	    mi->ti.disabled = ( cv->layerheads[cv->drawmode]->splines==NULL || order2 );
	  break;
#ifdef FONTFORGE_CONFIG_TILEPATH
	  case MID_TilePath:
	    mi->ti.disabled = badsel || ClipBoardToSplineSet()==NULL || order2;
	  break;
#endif
	  case MID_RegenBitmaps: case MID_RemoveBitmaps:
	    mi->ti.disabled = cv->fv->sf->bitmaps==NULL;
	  break;
	  case MID_AddExtrema:
	    mi->ti.disabled = cv->layerheads[cv->drawmode]->splines==NULL;
	  /* Like Simplify, always available, but may not do anything if */
	  /*  all extrema have points. I'm not going to check for that, too hard */
	  break;
	  case MID_Simplify:
	    mi->ti.disabled = cv->layerheads[cv->drawmode]->splines==NULL;
	  /* Simplify is always available (it may not do anything though) */
	  /*  well, ok. Disable it if there is absolutely nothing to work on */
	  break;
	  case MID_BuildAccent:
	    mi->ti.disabled = !SFIsSomethingBuildable(cv->fv->sf,cv->sc,false);
	  break;
	  case MID_Autotrace:
	    mi->ti.disabled = FindAutoTraceName()==NULL || cv->sc->layers[ly_back].images==NULL;
	  break;
	}
    }
}

static void ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_ellistcheck(cv,mi,e,true);
}

static void CVMenuAutoHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    /*int removeOverlap = e==NULL || !(e->u.mouse.state&ksm_shift);*/
    int was = cv->sc->changedsincelasthinted;

    /* Hint undoes are done in _SplineCharAutoHint */
    cv->sc->manualhints = false;
    SplineCharAutoHint(cv->sc,NULL);
    SCUpdateAll(cv->sc);
    if ( was ) {
	FontView *fvs;
	for ( fvs=cv->fv; fvs!=NULL; fvs=fvs->nextsame )
	    GDrawRequestExpose(fvs->v,NULL,false);	/* Clear any changedsincelasthinted marks */
    }
}

static void CVMenuAutoHintSubs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    SCFigureHintMasks(cv->sc);
    SCUpdateAll(cv->sc);
}

static void CVMenuAutoCounter(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    SCFigureCounterMasks(cv->sc);
}

static void CVMenuDontAutoHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    cv->sc->manualhints = !cv->sc->manualhints;
}

static void CVMenuAutoInstr(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    SCAutoInstr(cv->sc,NULL);
}

static void CVMenuClearHints(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    SCPreserveHints(cv->sc);
    SCHintsChanged(cv->sc);
    if ( mi->mid==MID_ClearHStem ) {
	StemInfosFree(cv->sc->hstem);
	cv->sc->hstem = NULL;
	cv->sc->hconflicts = false;
    } else if ( mi->mid==MID_ClearVStem ) {
	StemInfosFree(cv->sc->vstem);
	cv->sc->vstem = NULL;
	cv->sc->vconflicts = false;
    }
    cv->sc->manualhints = true;
    SCClearHintMasks(cv->sc,true);
    SCOutOfDateBackground(cv->sc);
    SCUpdateAll(cv->sc);
}

static void CVMenuAddHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp1, *sp2, *sp3, *sp4;
    StemInfo *h=NULL;
    DStemInfo *d;

    if ( !CVTwoForePointsSelected(cv,&sp1,&sp2))
return;

    SCPreserveHints(cv->sc);
    SCHintsChanged(cv->sc);
    if ( mi->mid==MID_AddHHint ) {
	if ( sp1->me.y==sp2->me.y )
return;
	h = chunkalloc(sizeof(StemInfo));
	if ( sp2->me.y>sp1->me.y ) {
	    h->start = sp1->me.y;
	    h->width = sp2->me.y-sp1->me.y;
	} else {
	    h->start = sp2->me.y;
	    h->width = sp1->me.y-sp2->me.y;
	}
	SCGuessHHintInstancesAndAdd(cv->sc,h,sp1->me.x,sp2->me.x);
	cv->sc->hconflicts = StemListAnyConflicts(cv->sc->hstem);
    } else if ( mi->mid==MID_AddVHint ) {
	if ( sp1->me.x==sp2->me.x )
return;
	h = chunkalloc(sizeof(StemInfo));
	if ( sp2->me.x>sp1->me.x ) {
	    h->start = sp1->me.x;
	    h->width = sp2->me.x-sp1->me.x;
	} else {
	    h->start = sp2->me.x;
	    h->width = sp1->me.x-sp2->me.x;
	}
	SCGuessVHintInstancesAndAdd(cv->sc,h,sp1->me.y,sp2->me.y);
	cv->sc->vconflicts = StemListAnyConflicts(cv->sc->vstem);
    } else {
	if ( !CVIsDiagonalable(sp1,sp2,&sp3,&sp4))
return;
	/* Make sure sp1<->sp3 is further left than sp2<->sp4 */
	if ( sp1->me.x > sp2->me.x + (sp1->me.y-sp2->me.y) * (sp4->me.x-sp2->me.x)/(sp4->me.y-sp2->me.y) ) {
	    SplinePoint *temp;
	    temp = sp1; sp1 = sp2; sp2 = temp;
	    temp = sp3; sp3=sp4; sp4=temp;
	}
	/* Make sure sp1,sp2 are at the top */
	if ( sp1->me.y<sp3->me.y ) {
	    SplinePoint *temp;
	    temp = sp1; sp1=sp3; sp3=temp;
	    temp = sp2; sp2=sp4; sp4=temp;
	}
	d = chunkalloc(sizeof(DStemInfo));
	d->next = cv->sc->dstem;
	cv->sc->dstem = d;
	d->leftedgetop = sp1->me;
	d->rightedgetop = sp2->me;
	d->leftedgebottom = sp3->me;
	d->rightedgebottom = sp4->me;
    }
    cv->sc->manualhints = true;
    if ( h!=NULL && cv->sc->parent->mm==NULL )
	SCModifyHintMasksAdd(cv->sc,h);
    else
	SCClearHintMasks(cv->sc,true);
    SCOutOfDateBackground(cv->sc);
    SCUpdateAll(cv->sc);
}

static void CVMenuCreateHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVCreateHint(cv,mi->mid==MID_CreateHHint,true);
}

static void CVMenuReviewHints(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->hstem==NULL && cv->sc->vstem==NULL )
return;
    CVReviewHints(cv);
}

static void htlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp1, *sp2, *sp3, *sp4;
    int multilayer = cv->sc->parent->multilayer;

    sp1 = sp2 = NULL;
    CVTwoForePointsSelected(cv,&sp1,&sp2);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AutoHint:
	    mi->ti.disabled = multilayer;
	  break;
	  case MID_HintSubsPt: case MID_AutoCounter:
	    mi->ti.disabled = cv->sc->parent->order2 || multilayer;
	  break;
	  case MID_DontAutoHint:
	    mi->ti.disabled = cv->sc->parent->order2 || multilayer;
	    mi->ti.checked = cv->sc->manualhints;
	  break;
	  case MID_AutoInstr:
	  case MID_EditInstructions:
	    mi->ti.disabled = !cv->fv->sf->order2 || multilayer;
	  break;
	  case MID_Debug:
	    mi->ti.disabled = !cv->fv->sf->order2 || multilayer || !hasFreeTypeDebugger();
	  break;
	  case MID_ClearInstr:
	    mi->ti.disabled = cv->sc->ttf_instrs_len==0;
	  break;
	  case MID_AddHHint:
	    mi->ti.disabled = sp2==NULL || sp2->me.y==sp1->me.y || multilayer;
	  break;
	  case MID_AddVHint:
	    mi->ti.disabled = sp2==NULL || sp2->me.x==sp1->me.x || multilayer;
	  break;
	  case MID_AddDHint:
	    mi->ti.disabled = !CVIsDiagonalable(sp1,sp2,&sp3,&sp4) || multilayer;
	  break;
	  case MID_ReviewHints:
	    mi->ti.disabled = (cv->sc->hstem==NULL && cv->sc->vstem==NULL ) || multilayer;
	  break;
	}
    }
}

static void mtlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_RemoveKerns:
	    mi->ti.disabled = cv->sc->kerns==NULL;
	  break;
	  case MID_RemoveVKerns:
	    mi->ti.disabled = cv->sc->vkerns==NULL;
	  break;
	  case MID_SetVWidth:
	    mi->ti.disabled = !cv->sc->parent->hasvmetrics;
	  break;
	  case MID_AnchorsAway:
	    mi->ti.disabled = cv->sc->anchor==NULL;
	  break;
	}
    }
}

static void cv_sllistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e) {
    SplinePoint *sp; SplineSet *spl; RefChar *r; ImageList *im;
    int exactlyone = CVOneThingSel(cv,&sp,&spl,&r,&im,NULL);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_NextPt: case MID_PrevPt:
	  case MID_NextCP: case MID_PrevCP:
	  case MID_FirstPtNextCont:
	    mi->ti.disabled = !exactlyone || sp==NULL;
	  break;
	  case MID_FirstPt: case MID_SelPointAt:
	    mi->ti.disabled = cv->layerheads[cv->drawmode]->splines==NULL;
	  break;
	  case MID_Contours:
	    mi->ti.disabled = CVAnySelPoints(cv)==NULL;
	  break;
	  case MID_SelectWidth:
	    mi->ti.disabled = !cv->showhmetrics;
	    if ( !mi->ti.disabled ) {
		free(mi->ti.text);
		mi->ti.text = utf82u_copy(cv->widthsel?_("Deselect Width"):_("Width"));
	    }
	  break;
	  case MID_SelectVWidth:
	    mi->ti.disabled = !cv->showvmetrics || !cv->sc->parent->hasvmetrics;
	    if ( !mi->ti.disabled ) {
		free(mi->ti.text);
		mi->ti.text = utf82u_copy(cv->vwidthsel?_("Deselect VWidth"):_("VWidth"));
	    }
	  break;
	  case MID_SelectHM:
	    mi->ti.disabled = !exactlyone || sp==NULL || sp->hintmask==NULL;
	  break;
	}
    }
}

static void sllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_sllistcheck(cv,mi,e);
}

static void cv_cblistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e) {
    int i;
    KernPair *kp;
    SplineChar *sc = cv->sc;
    SplineFont *sf = sc->parent;
    PST *pst;
    char *name;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AnchorPairs:
	    mi->ti.disabled = sc->anchor==NULL;
	  break;
	  case MID_KernPairs:
	    mi->ti.disabled = sc->kerns==NULL;
	    if ( sc->kerns==NULL ) {
		for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		    for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
			if ( kp->sc == sc ) {
			    mi->ti.disabled = false;
		goto out;
			}
		    }
		}
	      out:;
	    }
	  break;
	  case MID_Ligatures:
	    name = sc->name;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		for ( pst=sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->type==pst_ligature &&
			    PSTContains(pst->u.lig.components,name)) {
			mi->ti.disabled = false;
	  goto break_out_2;
		    }
		}
	    }
	    mi->ti.disabled = true;
	  break_out_2:;
	  break;
	}
    }
}

static void cv_nplistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e) {
    SplineChar *sc = cv->sc;
    int order2 = sc->parent->order2;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_PtsNone:
	    mi->ti.checked = !cv->showpointnumbers;
	  break;
	  case MID_PtsTrue:
	    mi->ti.disabled = !order2;
	    mi->ti.checked = cv->showpointnumbers && order2;
	  break;
	  case MID_PtsPost:
	    mi->ti.disabled = order2;
	    mi->ti.checked = cv->showpointnumbers && !order2 && sc->numberpointsbackards;
	  break;
	  case MID_PtsSVG:
	    mi->ti.disabled = order2;
	    mi->ti.checked = cv->showpointnumbers && !order2 && !sc->numberpointsbackards;
	  break;
	}
    }
}

static void cv_vwlistcheck(CharView *cv,struct gmenuitem *mi,GEvent *e) {
    int pos, gid;
    SplineFont *sf = cv->sc->parent;
    EncMap *map = cv->fv->map;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_NextDef:
	    for ( pos = CVCurEnc(cv)+1; pos<map->enccount && ((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid])); ++pos );
	    mi->ti.disabled = pos==map->enccount || cv->searcher!=NULL;
	  break;
	  case MID_PrevDef:
	    for ( pos = CVCurEnc(cv)-1; pos>=0 && ((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid])); --pos );
	    mi->ti.disabled = pos<0 || cv->searcher!=NULL;
	  break;
	  case MID_Next:
	    mi->ti.disabled = cv->searcher!=NULL || CVCurEnc(cv)==map->enccount-1;
	  break;
	  case MID_Prev:
	    mi->ti.disabled = cv->searcher!=NULL || CVCurEnc(cv)==0;
	  break;
	  case MID_Former:
	    if ( cv->former_cnt<=1 )
		pos = -1;
	    else for ( pos = sf->glyphcnt-1; pos>=0 ; --pos )
		if ( sf->glyphs[pos]!=NULL && strcmp(sf->glyphs[pos]->name,cv->former_names[1])==0 )
	    break;
	    mi->ti.disabled = pos==-1 || cv->searcher!=NULL;
	  break;
	  case MID_Goto: case MID_FindInFontView:
	    mi->ti.disabled = cv->searcher!=NULL;
	  break;
	  case MID_MarkExtrema:
	    mi->ti.checked = cv->markextrema;
	  break;
	  case MID_MarkPointsOfInflection:
	    mi->ti.checked = cv->markpoi;
	  break;
	  case MID_ShowCPInfo:
	    mi->ti.checked = cv->showcpinfo;
	  break;
	  case MID_ShowTabs:
	    mi->ti.checked = cv->showtabs;
	    mi->ti.disabled = cv->former_cnt<=1;
	  break;
	  case MID_HidePoints:
	    free(mi->ti.text);
	    mi->ti.text = utf82u_copy(cv->showpoints?_("Hide Points"):_("Show Points"));
	  break;
	  case MID_HideRulers:
	    free(mi->ti.text);
	    mi->ti.text = utf82u_copy(cv->showrulers?_("Hide Rulers"):_("Show Rulers"));
	  break;
	  case MID_ShowGridFit:
	    mi->ti.disabled = !hasFreeType() || cv->drawmode!=dm_fore || cv->dv!=NULL;
	    mi->ti.checked = cv->show_ft_results;
	  break;
	  case MID_Fill:
	    mi->ti.checked = cv->showfilled;
	  break;
#if HANYANG
	  case MID_DisplayCompositions:
	    mi->ti.disabled = !cv->sc->compositionunit || cv->sc->parent->rules==NULL;
	  break;
#endif
	}
    }
}

static void cblistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_cblistcheck(cv,mi,e);
}

static void nplistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_nplistcheck(cv,mi,e);
}

static void vwlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_vwlistcheck(cv,mi,e);
}

static void CVMenuCenter(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    DBounds bb;
    real transform[6];
    int drawmode = cv->drawmode;

    cv->drawmode = dm_fore;

    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = transform[5] = 0.0;
    SplineCharFindBounds(cv->sc,&bb);
    if ( mi->mid==MID_Center )
	transform[4] = (cv->sc->width-(bb.maxx-bb.minx))/2 - bb.minx;
    else
	transform[4] = (cv->sc->width-(bb.maxx-bb.minx))/3 - bb.minx;
    if ( transform[4]!=0 ) {
	cv->p.transany = false;
	CVPreserveState(cv);
	CVTransFunc(cv,transform,fvt_dontmovewidth);
	CVCharChangedUpdate(cv);
    }
    cv->drawmode = drawmode;
}

static void CVMenuSetWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( mi->mid == MID_SetVWidth && !cv->sc->parent->hasvmetrics )
return;
    CVSetWidth(cv,mi->mid==MID_SetWidth?wt_width:
		  mi->mid==MID_SetLBearing?wt_lbearing:
		  mi->mid==MID_SetRBearing?wt_rbearing:
		  wt_vwidth);
}

static void CVMenuRemoveKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->kerns!=NULL ) {
	KernPairsFree(cv->sc->kerns);
	cv->sc->kerns = NULL;
	cv->sc->parent->changed = true;
	if ( cv->fv->cidmaster!=NULL )
	    cv->fv->cidmaster->changed = true;
    }
}

static void CVMenuRemoveVKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->vkerns!=NULL ) {
	KernPairsFree(cv->sc->vkerns);
	cv->sc->vkerns = NULL;
	cv->sc->parent->changed = true;
	if ( cv->fv->cidmaster!=NULL )
	    cv->fv->cidmaster->changed = true;
    }
}

static void CVMenuKPCloseup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    KernPairD(cv->sc->parent,cv->sc,NULL,false);
}

static void CVMenuAnchorsAway(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    AnchorPoint *ap;

    for ( ap = cv->sc->anchor; ap!=NULL && !ap->selected; ap = ap->next );
    if ( ap==NULL ) ap= cv->sc->anchor;
    if ( ap==NULL )
return;

    GDrawSetCursor(cv->v,ct_watch);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    AnchorControl(cv->sc,ap);
    GDrawSetCursor(cv->v,ct_pointer);
}

static GMenuItem wnmenu[] = {
    { { (unichar_t *) N_("New O_utline Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'u' }, 'H', ksm_control, NULL, NULL, /* No function, never avail */NULL },
    { { (unichar_t *) N_("New _Bitmap Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, 'J', ksm_control, NULL, NULL, CVMenuOpenBitmap, MID_OpenBitmap },
    { { (unichar_t *) N_("New _Metrics Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 'K', ksm_control, NULL, NULL, CVMenuOpenMetrics },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Warnings"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, _MenuWarnings, MID_Warnings },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { NULL }
};

static void CVWindowMenuBuild(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    struct gmenuitem *wmi;

    for ( wmi = wnmenu; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
	switch ( wmi->mid ) {
	  case MID_OpenBitmap:
	    wmi->ti.disabled = cv->sc->parent->bitmaps==NULL;
	  break;
	  case MID_Warnings:
	    wmi->ti.disabled = ErrorWindowExists();
	  break;
	}
    }
    WindowMenuBuild(gw,mi,e,wnmenu);
}

static void SVWindowMenuBuild(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct gmenuitem *wmi;

    for ( wmi = wnmenu; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
	switch ( wmi->mid ) {
	  case MID_OpenBitmap:
	    wmi->ti.disabled = true;
	  break;
	}
    }
    WindowMenuBuild(gw,mi,e,wnmenu);
}

static GMenuItem dummyitem[] = { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, 'N', ksm_control, NULL, NULL, MenuNew },
    { { (unichar_t *) N_("_Open"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) N_("Recen_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) N_("_Close"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, CVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Save"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, CVMenuSave },
    { { (unichar_t *) N_("S_ave as..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, CVMenuSaveAs },
    { { (unichar_t *) N_("_Generate Fonts..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'G' }, 'G', ksm_control|ksm_shift, NULL, NULL, CVMenuGenerate },
    { { (unichar_t *) N_("Generate Mac _Family..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 'G', ksm_control|ksm_meta, NULL, NULL, CVMenuGenerateFamily },
    { { (unichar_t *) N_("Expor_t..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuExport },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Import..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, 'I', ksm_control|ksm_shift, NULL, NULL, CVMenuImport },
    { { (unichar_t *) N_("_Revert File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, CVMenuRevert, MID_Revert },
    { { (unichar_t *) N_("Revert Gl_yph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, 'R', ksm_control|ksm_meta, NULL, NULL, CVMenuRevertGlyph, MID_RevertGlyph },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Print..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 'P', ksm_control, NULL, NULL, CVMenuPrint },
    { { (unichar_t *) N_("_Display..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, 'P', ksm_control|ksm_meta, NULL, NULL, CVMenuDisplay, MID_Display },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Pr_eferences..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Quit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem sllist[] = {
    { { (unichar_t *) N_("Select _All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, CVSelectAll, MID_SelAll },
    { { (unichar_t *) N_("_Invert Selection"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, GK_Escape, ksm_control, NULL, NULL, CVSelectInvert, MID_SelInvert },
    { { (unichar_t *) N_("_Deselect All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, GK_Escape, 0, NULL, NULL, CVSelectNone, MID_SelNone },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_First Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, '.', ksm_control, NULL, NULL, CVMenuNextPrevPt, MID_FirstPt },
    { { (unichar_t *) N_("First P_oint, Next Contour"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, '.', ksm_meta|ksm_control, NULL, NULL, CVMenuNextPrevPt, MID_FirstPtNextCont },
    { { (unichar_t *) N_("_Next Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, '}', ksm_shift|ksm_control, NULL, NULL, CVMenuNextPrevPt, MID_NextPt },
    { { (unichar_t *) N_("_Prev Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '{', ksm_shift|ksm_control, NULL, NULL, CVMenuNextPrevPt, MID_PrevPt },
    { { (unichar_t *) N_("Ne_xt Control Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'x' }, ';', ksm_control, NULL, NULL, CVMenuNextPrevCPt, MID_NextCP },
    { { (unichar_t *) N_("P_rev Control Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, ':', ksm_shift|ksm_control, NULL, NULL, CVMenuNextPrevCPt, MID_PrevCP },
    { { (unichar_t *) N_("Points on Selected _Contours"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, '\0', ksm_shift|ksm_control, NULL, NULL, CVMenuSelectContours, MID_Contours },
    { { (unichar_t *) N_("Point A_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, ',', ksm_control, NULL, NULL, CVMenuSelectPointAt, MID_SelPointAt },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Select All _Points & Refs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 'A', ksm_control|ksm_meta, NULL, NULL, CVSelectAll, MID_SelectAllPoints },
    { { (unichar_t *) N_("Sele_ct Anchors"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'c' }, '\0', ksm_control, NULL, NULL, CVSelectAll, MID_SelectAnchors },
    { { (unichar_t *) N_("_Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVSelectWidth, MID_SelectWidth },
    { { (unichar_t *) N_("_VWidth"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_shift|ksm_control, NULL, NULL, CVSelectVWidth, MID_SelectVWidth },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Select Points Affected by HM"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, '\0', ksm_shift|ksm_control, NULL, NULL, CVSelectHM, MID_SelectHM },
    { NULL }
};

static GMenuItem edlist[] = {
    { { (unichar_t *) N_("_Undo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL, CVUndo, MID_Undo },
    { { (unichar_t *) N_("_Redo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL, CVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Cu_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, CVCut, MID_Cut },
    { { (unichar_t *) N_("_Copy"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, CVCopy, MID_Copy },
    { { (unichar_t *) N_("C_opy Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, 'G', ksm_control, NULL, NULL, CVCopyRef, MID_CopyRef },
    { { (unichar_t *) N_("Copy _Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, 'W', ksm_control, NULL, NULL, CVCopyWidth, MID_CopyWidth },
    { { (unichar_t *) N_("Co_py LBearing"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'p' }, '\0', ksm_control, NULL, NULL, CVCopyWidth, MID_CopyLBearing },
    { { (unichar_t *) N_("Copy RBearin_g"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'g' }, '\0', ksm_control, NULL, NULL, CVCopyWidth, MID_CopyRBearing },
    { { (unichar_t *) N_("Copy Glyph Features..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'g' }, '\0', ksm_control, NULL, NULL, CVCopyFeatures, MID_CopyFeatures },
    { { (unichar_t *) N_("_Paste"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, CVPaste, MID_Paste },
    { { (unichar_t *) N_("C_lear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, GK_Delete, 0, NULL, NULL, CVClear, MID_Clear },
    { { (unichar_t *) N_("Clear _Background"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, 0, 0, NULL, NULL, CVClearBackground },
    { { (unichar_t *) N_("_Merge"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 'M', ksm_control, NULL, NULL, CVMerge, MID_Merge },
    { { (unichar_t *) N_("_Elide"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 'M', ksm_meta|ksm_control, NULL, NULL, CVElide, MID_Elide },
    { { (unichar_t *) N_("_Join"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'J' }, 'J', ksm_control|ksm_shift, NULL, NULL, CVJoin, MID_Join },
    { { (unichar_t *) N_("Copy _Fg To Bg"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 'C', ksm_control|ksm_shift, NULL, NULL, CVCopyFgBg, MID_CopyFgToBg },
#ifdef FONTFORGE_CONFIG_COPY_BG_TO_FG
    { { (unichar_t *) N_("Cop_y Bg To Fg"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVCopyBgFg, MID_CopyBgToFg },
#endif
    { { (unichar_t *) N_("Copy Gri_d Fit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuCopyGridFit, MID_CopyGridFit },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Select"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 0, ksm_control, sllist, sllistcheck },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("U_nlink Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, 'U', ksm_control, NULL, NULL, CVUnlinkRef, MID_UnlinkRef },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Remo_ve Undoes"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'e' }, '\0', 0, NULL, NULL, CVRemoveUndoes, MID_RemoveUndoes },
    { NULL }
};

static GMenuItem ptlist[] = {
    { { (unichar_t *) N_("_Curve"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'C' }, '2', ksm_control, NULL, NULL, CVMenuPointType, MID_Curve },
    { { (unichar_t *) N_("C_orner"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'o' }, '3', ksm_control, NULL, NULL, CVMenuPointType, MID_Corner },
    { { (unichar_t *) N_("_Tangent"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'T' }, '4', ksm_control, NULL, NULL, CVMenuPointType, MID_Tangent },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
/* GT: Make this (selected) point the first point in the glyph */
    { { (unichar_t *) N_("_Make First"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, '1', ksm_control, NULL, NULL, CVMenuMakeFirst, MID_MakeFirst },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Can Be _Interpolated"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, CVMenuImplicit, MID_ImplicitPt },
    { { (unichar_t *) N_("Can't Be _Interpolated"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, CVMenuImplicit, MID_NoImplicitPt },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Add Anchor"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, '0', ksm_control, NULL, NULL, CVMenuAddAnchor, MID_AddAnchor },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Make _Line"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, CVMenuMakeLine, MID_MakeLine },
    { { (unichar_t *) N_("Insert Point On _Spline At..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, CVMenuInsertPt, MID_InsertPtOnSplineAt },
    { { (unichar_t *) N_("Center _Between Control Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, CVMenuCenterCP, MID_CenterCP },
    { NULL }
};

static GMenuItem allist[] = {
/* GT: Align these points to their average position */
    { { (unichar_t *) N_("_Average Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, '@', ksm_control|ksm_shift, NULL, NULL, CVMenuConstrain, MID_Average },
    { { (unichar_t *) N_("_Space Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, '#', ksm_control|ksm_shift, NULL, NULL, CVMenuConstrain, MID_SpacePts },
    { { (unichar_t *) N_("Space _Regions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuConstrain, MID_SpaceRegion },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Make _Parallel..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuMakeParallel, MID_MakeParallel },
    { NULL }
};

static GMenuItem smlist[] = {
    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 'M', ksm_control|ksm_shift, NULL, NULL, CVMenuSimplify, MID_Simplify },
    { { (unichar_t *) N_("Simplify More..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 'M', ksm_control|ksm_shift|ksm_meta, NULL, NULL, CVMenuSimplifyMore, MID_SimplifyMore },
    { { (unichar_t *) N_("Clea_nup Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuCleanupGlyph, MID_CleanupGlyph },
    { { (unichar_t *) N_("Canonical Start _Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuCanonicalStart, MID_CanonicalStart },
    { { (unichar_t *) N_("Canonical _Contours"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuCanonicalContours, MID_CanonicalContours },
    { NULL }
};

static void smlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Simplify:
	  case MID_CleanupGlyph:
	  case MID_SimplifyMore:
	  case MID_CanonicalStart:
	    mi->ti.disabled = cv->layerheads[cv->drawmode]->splines==NULL;
	  break;
	  case MID_CanonicalContours:
	    mi->ti.disabled = cv->layerheads[cv->drawmode]->splines==NULL ||
		cv->layerheads[cv->drawmode]->splines->next==NULL ||
		cv->drawmode!=dm_fore;
	  break;
	}
    }
}

static GMenuItem orlist[] = {
    { { (unichar_t *) N_("_First"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuOrder, MID_First },
    { { (unichar_t *) N_("_Earlier"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control|ksm_shift|ksm_meta, NULL, NULL, CVMenuOrder, MID_Earlier },
    { { (unichar_t *) N_("L_ater"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuOrder, MID_Later },
    { { (unichar_t *) N_("_Last"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuOrder, MID_Last },
    { NULL }
};

static void orlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePointList *spl;
    RefChar *r;
    ImageList *im;
    int exactlyone = CVOneContourSel(cv,&spl,&r,&im);
    int isfirst, islast;

    isfirst = islast = false;
    if ( spl!=NULL ) {
	isfirst = cv->layerheads[cv->drawmode]->splines==spl;
	islast = spl->next==NULL;
    } else if ( r!=NULL ) {
	isfirst = cv->layerheads[cv->drawmode]->refs==r;
	islast = r->next==NULL;
    } else if ( im!=NULL ) {
	isfirst = cv->layerheads[cv->drawmode]->images==im;
	islast = im->next!=NULL;
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_First:
	  case MID_Earlier:
	    mi->ti.disabled = !exactlyone || isfirst;
	  break;
	  case MID_Last:
	  case MID_Later:
	    mi->ti.disabled = !exactlyone || islast;
	  break;
	}
    }
}

static GMenuItem rmlist[] = {
    { { (unichar_t *) N_("_Remove Overlap"), &GIcon_rmoverlap, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'R' }, 'O', ksm_control|ksm_shift, NULL, NULL, CVMenuOverlap, MID_RmOverlap },
    { { (unichar_t *) N_("_Intersect"), &GIcon_intersection, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuOverlap, MID_Intersection },
    { { (unichar_t *) N_("_Exclude"), &GIcon_exclude, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'E' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuOverlap, MID_Exclude },
    { { (unichar_t *) N_("_Find Intersections"), &GIcon_findinter, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'F' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuOverlap, MID_FindInter },
    { NULL }
};

static GMenuItem eflist[] = {
    { { (unichar_t *) N_("_Inline"), &GIcon_inline, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuInline },
    { { (unichar_t *) N_("_Outline"), &GIcon_outline, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuOutline },
    { { (unichar_t *) N_("_Shadow"), &GIcon_shadow, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'S' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuShadow },
    { { (unichar_t *) N_("_Wireframe"), &GIcon_wireframe, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'W' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuWireframe },
    { NULL }
};

static GMenuItem balist[] = {
    { { (unichar_t *) N_("_Build Accented Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'u' }, 'A', ksm_control|ksm_shift, NULL, NULL, CVMenuBuildAccent, MID_BuildAccent },
    { { (unichar_t *) N_("Build _Composite Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuBuildComposite, MID_BuildComposite },
    { NULL }
};

static GMenuItem delist[] = {
    { { (unichar_t *) N_("_References..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'u' }, 'I', ksm_control|ksm_meta, NULL, NULL, CVMenuShowDependentRefs, MID_ShowDependentRefs },
    { { (unichar_t *) N_("_Substitutions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuShowDependentSubs, MID_ShowDependentSubs },
    { NULL }
};

static GMenuItem trlist[] = {
    { { (unichar_t *) N_("_Transform..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\\', ksm_control, NULL, NULL, CVMenuTransform },
    { { (unichar_t *) N_("_Point of View Projection..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '<', ksm_shift|ksm_control, NULL, NULL, CVMenuPOV },
    { { (unichar_t *) N_("_Non Linear Transform..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '|', ksm_shift|ksm_control, NULL, NULL, CVMenuNLTransform },
    { NULL }
};

static GMenuItem rndlist[] = {
    { { (unichar_t *) N_("To _Int"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, '_', ksm_control|ksm_shift, NULL, NULL, CVMenuRound2Int, MID_Round },
    { { (unichar_t *) N_("To _Hundredths"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuRound2Hundredths, 0 },
    { { (unichar_t *) N_("_Cluster"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuCluster },
    { NULL }
};

static GMenuItem ellist[] = {
    { { (unichar_t *) N_("_Font Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 'F', ksm_control|ksm_shift, NULL, NULL, CVMenuFontInfo },
    { { (unichar_t *) N_("Glyph _Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, 'I', ksm_control|ksm_shift|ksm_meta, NULL, NULL, CVMenuCharInfo, MID_CharInfo },
    { { (unichar_t *) N_("Get _Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, 'I', ksm_control, NULL, NULL, CVMenuGetInfo, MID_GetInfo },
    { { (unichar_t *) N_("S_how Dependent"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, '\0', ksm_control|ksm_meta, delist, delistcheck },
    { { (unichar_t *) N_("Find Pr_oblems..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, 'E', ksm_control, NULL, NULL, CVMenuFindProblems },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Bitm_ap strikes Available..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, 'B', ksm_control|ksm_shift, NULL, NULL, CVMenuBitmaps, MID_AvailBitmaps },
    { { (unichar_t *) N_("Regenerate _Bitmap Glyphs..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, 'B', ksm_control, NULL, NULL, CVMenuBitmaps, MID_RegenBitmaps },
    { { (unichar_t *) N_("Remove Bitmap Glyphs..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuBitmaps, MID_RemoveBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Transformations"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, 0, ksm_control, trlist },
    { { (unichar_t *) N_("_Expand Stroke..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, 'E', ksm_control|ksm_shift, NULL, NULL, CVMenuStroke, MID_Stroke },
#ifdef FONTFORGE_CONFIG_TILEPATH
    { { (unichar_t *) N_("Tile _Path..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuTilePath, MID_TilePath },
#endif
    { { (unichar_t *) N_("O_verlap"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'v' }, '\0', ksm_control|ksm_shift, rmlist, NULL, NULL, MID_RmOverlap },
    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, '\0', ksm_control|ksm_shift, smlist, smlistcheck, NULL, MID_Simplify },
    { { (unichar_t *) N_("Add E_xtrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'x' }, 'X', ksm_control|ksm_shift, NULL, NULL, CVMenuAddExtrema, MID_AddExtrema },
    { { (unichar_t *) N_("Effects"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control|ksm_shift, eflist, NULL, NULL, MID_Effects },
    { { (unichar_t *) N_("_Meta Font..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, '!', ksm_control|ksm_shift, NULL, NULL, CVMenuMetaFont, MID_MetaFont },
    { { (unichar_t *) N_("Autot_race"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, 'T', ksm_control|ksm_shift, NULL, NULL, CVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("A_lign"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, '\0', ksm_control|ksm_shift, allist, allistcheck },
    { { (unichar_t *) N_("Roun_d"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control|ksm_shift, rndlist, NULL, NULL, MID_Round },
    { { (unichar_t *) N_("Order"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control|ksm_shift, orlist, orlistcheck },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Cl_ockwise"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'o' }, '\0', 0, NULL, NULL, CVMenuDir, MID_Clockwise },
    { { (unichar_t *) N_("Cou_nter Clockwise"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'n' }, '\0', 0, NULL, NULL, CVMenuDir, MID_Counter },
    { { (unichar_t *) N_("_Correct Direction"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, 'D', ksm_control|ksm_shift, NULL, NULL, CVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("B_uild"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'u' }, '\0', ksm_control|ksm_shift, balist, balistcheck, NULL, MID_BuildAccent },
    { NULL }
};

static GMenuItem htlist[] = {
    { { (unichar_t *) N_("Auto_Hint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, 'H', ksm_control|ksm_shift, NULL, NULL, CVMenuAutoHint, MID_AutoHint },
    { { (unichar_t *) N_("Hint _Substitution Pts"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuAutoHintSubs, MID_HintSubsPt },
    { { (unichar_t *) N_("Auto _Counter Hint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuAutoCounter, MID_AutoCounter },
    { { (unichar_t *) N_("_Don't AutoHint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'H' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuDontAutoHint, MID_DontAutoHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Auto_Instr"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, 'T', ksm_control, NULL, NULL, CVMenuAutoInstr, MID_AutoInstr },
    { { (unichar_t *) N_("_Edit Instructions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, '\0', 0, NULL, NULL, CVMenuEditInstrs, MID_EditInstructions },
    { { (unichar_t *) N_("_Debug..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, '\0', 0, NULL, NULL, CVMenuDebug, MID_Debug },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Clear HStem"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearHStem },
    { { (unichar_t *) N_("Clear _VStem"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearVStem },
    { { (unichar_t *) N_("Clear Instructions"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuClearInstrs, MID_ClearInstr },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Add HHint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, '\0', ksm_control, NULL, NULL, CVMenuAddHint, MID_AddHHint },
    { { (unichar_t *) N_("Add VHi_nt"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 's' }, '\0', ksm_control, NULL, NULL, CVMenuAddHint, MID_AddVHint },
    { { (unichar_t *) N_("Add DHint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, CVMenuAddHint, MID_AddDHint },
    { { (unichar_t *) N_("Crea_te HHint..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, '\0', ksm_control, NULL, NULL, CVMenuCreateHint, MID_CreateHHint },
    { { (unichar_t *) N_("Cr_eate VHint..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, CVMenuCreateHint, MID_CreateVHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Review Hints..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, 'H', ksm_control|ksm_meta, NULL, NULL, CVMenuReviewHints, MID_ReviewHints },
    { NULL }
};

static GMenuItem mtlist[] = {
    { { (unichar_t *) N_("_Center in Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, CVMenuCenter, MID_Center },
    { { (unichar_t *) N_("_Thirds in Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, CVMenuCenter, MID_Thirds },
    { { (unichar_t *) N_("Set _Width..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, 'L', ksm_control|ksm_shift, NULL, NULL, CVMenuSetWidth, MID_SetWidth },
    { { (unichar_t *) N_("Set _LBearing..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'L' }, 'L', ksm_control, NULL, NULL, CVMenuSetWidth, MID_SetLBearing },
    { { (unichar_t *) N_("Set _RBearing..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, 'R', ksm_control, NULL, NULL, CVMenuSetWidth, MID_SetRBearing },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Remove Kern _Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuRemoveKern, MID_RemoveKerns },
    { { (unichar_t *) N_("Remove VKern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuRemoveVKern, MID_RemoveVKerns },
    { { (unichar_t *) N_("Kern Pair Closeup..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuKPCloseup, MID_KPCloseup },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Set _Vertical Advance..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuSetWidth, MID_SetVWidth },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Anchor Control..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuAnchorsAway, MID_AnchorsAway },
    { NULL }
};

static GMenuItem pllist[] = {
    { { (unichar_t *) N_("_Tools"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, CVMenuPaletteShow, MID_Tools },
    { { (unichar_t *) N_("_Layers"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'L' }, '\0', ksm_control, NULL, NULL, CVMenuPaletteShow, MID_Layers },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Docked Palettes"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'D' }, '\0', ksm_control, NULL, NULL, CVMenuPalettesDock, MID_DockPalettes },
    { NULL }
};

static GMenuItem cblist[] = {
    { { (unichar_t *) N_("_Kern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, '\0', 0, NULL, NULL, CVMenuKernPairs, MID_KernPairs },
    { { (unichar_t *) N_("_Anchored Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, '\0', 0, NULL, NULL, CVMenuAnchorPairs, MID_AnchorPairs },
    { { (unichar_t *) N_("_Ligatures"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'L' }, '\0', ksm_shift|ksm_control, NULL, NULL, CVMenuLigatures, MID_Ligatures },
    NULL
};

static GMenuItem nplist[] = {
    { { (unichar_t *) N_("PointNumbers|_None"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'K' }, '\0', 0, NULL, NULL, CVMenuNumberPoints, MID_PtsNone },
    { { (unichar_t *) N_("TrueType"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'A' }, '\0', 0, NULL, NULL, CVMenuNumberPoints, MID_PtsTrue },
    { { (unichar_t *) NU_("PostScript®"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'L' }, '\0', ksm_shift|ksm_control, NULL, NULL, CVMenuNumberPoints, MID_PtsPost },
    { { (unichar_t *) N_("SVG"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'L' }, '\0', ksm_shift|ksm_control, NULL, NULL, CVMenuNumberPoints, MID_PtsSVG },
    NULL
};

static GMenuItem vwlist[] = {
    { { (unichar_t *) N_("_Fit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 'F', ksm_control, NULL, NULL, CVMenuScale, MID_Fit },
    { { (unichar_t *) N_("Z_oom out"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '-', ksm_control|ksm_meta, NULL, NULL, CVMenuScale, MID_ZoomOut },
    { { (unichar_t *) N_("Zoom _in"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'i' }, '+', ksm_shift|ksm_control|ksm_meta, NULL, NULL, CVMenuScale, MID_ZoomIn },
#if HANYANG
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Display Compositions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'i' }, '\0', ksm_control, NULL, NULL, CVDisplayCompositions, MID_DisplayCompositions },
#endif
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Next Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, ']', ksm_control, NULL, NULL, CVMenuChangeChar, MID_Next },
    { { (unichar_t *) N_("_Prev Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '[', ksm_control, NULL, NULL, CVMenuChangeChar, MID_Prev },
    { { (unichar_t *) N_("Next _Defined Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, ']', ksm_control|ksm_meta, NULL, NULL, CVMenuChangeChar, MID_NextDef },
    { { (unichar_t *) N_("Prev Defined Gl_yph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, '[', ksm_control|ksm_meta, NULL, NULL, CVMenuChangeChar, MID_PrevDef },
    { { (unichar_t *) N_("Form_er Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, '<', ksm_shift|ksm_control, NULL, NULL, CVMenuChangeChar, MID_Former },
    { { (unichar_t *) N_("_Goto"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'G' }, '>', ksm_shift|ksm_control, NULL, NULL, CVMenuGotoChar, MID_Goto },
    { { (unichar_t *) N_("Find In Font _View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, '<', ksm_shift|ksm_control, NULL, NULL, CVMenuFindInFontView, MID_FindInFontView },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Hide Poin_ts"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, 'D', ksm_control, NULL, NULL, CVMenuShowHide, MID_HidePoints },
    { { (unichar_t *) N_("_Number Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, nplist, nplistcheck },
    { { (unichar_t *) N_("_Mark Extrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, CVMenuMarkExtrema, MID_MarkExtrema },
    { { (unichar_t *) N_("M_ark Points of Inflection"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, CVMenuMarkPointsOfInflection, MID_MarkPointsOfInflection },
    { { (unichar_t *) N_("Show _Control Point Info"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, CVMenuShowCPInfo, MID_ShowCPInfo },
    { { (unichar_t *) N_("Fi_ll"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'l' }, '\0', 0, NULL, NULL, CVMenuFill, MID_Fill },
    { { (unichar_t *) N_("Sho_w Grid Fit..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'l' }, '\0', 0, NULL, NULL, CVMenuShowGridFit, MID_ShowGridFit },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, }},
    { { (unichar_t *) N_("Com_binations"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'b' }, '\0', ksm_shift|ksm_control, cblist, cblistcheck },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, }},
    { { (unichar_t *) N_("Palette_s"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', 0, pllist, pllistcheck },
    { { (unichar_t *) N_("S_how Glyph Tabs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'R' }, '\0', ksm_control, NULL, NULL, CVMenuShowTabs, MID_ShowTabs },
    { { (unichar_t *) N_("Hide _Rulers"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, '\0', ksm_control, NULL, NULL, CVMenuShowHideRulers, MID_HideRulers },
    { NULL }
};

static void CVMenuShowMMMask(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    uint32 changemask = (uint32) (intpt) mi->ti.userdata;
    /* Change which mms get displayed in the "background" */

    if ( mi->mid==MID_MMAll ) {
	if ( (cv->mmvisible&changemask)==changemask ) cv->mmvisible = 0;
	else cv->mmvisible = changemask;
    } else if ( mi->mid == MID_MMNone ) {
	if ( cv->mmvisible==0 ) cv->mmvisible = (1<<(cv->sc->parent->mm->instance_count+1))-1;
	else cv->mmvisible = 0;
    } else
	cv->mmvisible ^= changemask;
    GDrawRequestExpose(cv->v,NULL,false);
}

static GMenuItem mvlist[] = {
    { { (unichar_t *) N_("SubFonts|_All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffffffff, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuShowMMMask, MID_MMAll },
    { { (unichar_t *) N_("SubFonts|_None"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuShowMMMask, MID_MMNone },
    { NULL }
};

static void mvlistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int i, base, j;
    extern void GMenuItemArrayFree(GMenuItem *mi);
    extern GMenuItem *GMenuItemArrayCopy(GMenuItem *mi, uint16 *cnt);
    MMSet *mm = cv->sc->parent->mm;
    uint32 submask;
    SplineFont *sub;
    GMenuItem *mml;

    base = 3;
    if ( mm==NULL )
	mml = mvlist;
    else {
	mml = gcalloc(base+mm->instance_count+2,sizeof(GMenuItem));
	memcpy(mml,mvlist,sizeof(mvlist));
	mml[base-1].ti.fg = mml[base-1].ti.bg = COLOR_DEFAULT;
	mml[base-1].ti.line = true;
	submask = 0;
	for ( j = 0, i=base; j<mm->instance_count+1; ++i, ++j ) {
	    if ( j==0 )
		sub = mm->normal;
	    else
		sub = mm->instances[j-1];
	    mml[i].ti.text = uc_copy(sub->fontname);
	    mml[i].ti.checkable = true;
	    mml[i].ti.checked = (cv->mmvisible & (1<<j))?1:0;
	    mml[i].ti.userdata = (void *) (intpt) (1<<j);
	    mml[i].invoke = CVMenuShowMMMask;
	    mml[i].ti.fg = mml[i].ti.bg = COLOR_DEFAULT;
	    if ( sub==cv->sc->parent )
		submask = (1<<j);
	}
	/* All */
	mml[0].ti.userdata = (void *) (intpt) ((1<<j)-1);
	mml[0].ti.checked = (cv->mmvisible == (uint32) (intpt) mml[0].ti.userdata);
	    /* None */
	mml[1].ti.checked = (cv->mmvisible == 0 || cv->mmvisible == submask);
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItemArrayCopy(mml,NULL);
    if ( mml!=mvlist ) {
	for ( i=base; mml[i].ti.text!=NULL; ++i )
	    free( mml[i].ti.text);
	free(mml);
    }
}

static void CVMenuReblend(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    char *err;
    MMSet *mm = cv->sc->parent->mm;

    if ( mm==NULL || mm->apple || cv->sc->parent!=mm->normal )
return;
    err = MMBlendChar(mm,cv->sc->orig_pos);
    if ( mm->normal->glyphs[cv->sc->orig_pos]!=NULL )
	_SCCharChangedUpdate(mm->normal->glyphs[cv->sc->orig_pos],-1);
    if ( err!=0 )
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	gwwv_post_error(_("Bad Multiple Master Font"),err);
#endif
}

static GMenuItem mmlist[] = {
/* GT: Here (and following) MM means "MultiMaster" */
    { { (unichar_t *) N_("MM _Reblend"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, 0, 0, NULL, NULL, CVMenuReblend, MID_MMReblend },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, }},
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, 0, 0, mvlist, mvlistcheck },
    { NULL }
};

static void CVMenuShowSubChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineFont *new = mi->ti.userdata;
    /* Change to the same char in a different instance font of the mm */

    CVChangeSC(cv,SFMakeChar(new,cv->fv->map,CVCurEnc(cv)));
    cv->layerheads[dm_grid] = &new->grid;
}

static void mmlistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int i, base, j;
    extern void GMenuItemArrayFree(GMenuItem *mi);
    extern GMenuItem *GMenuItemArrayCopy(GMenuItem *mi, uint16 *cnt);
    MMSet *mm = cv->sc->parent->mm;
    SplineFont *sub;
    GMenuItem *mml;

    base = sizeof(mmlist)/sizeof(mmlist[0]);
    if ( mm==NULL )
	mml = mmlist;
    else {
	mml = gcalloc(base+mm->instance_count+2,sizeof(GMenuItem));
	memcpy(mml,mmlist,sizeof(mmlist));
	mml[base-1].ti.fg = mml[base-1].ti.bg = COLOR_DEFAULT;
	mml[base-1].ti.line = true;
	for ( j = 0, i=base; j<mm->instance_count+1; ++i, ++j ) {
	    if ( j==0 )
		sub = mm->normal;
	    else
		sub = mm->instances[j-1];
	    mml[i].ti.text = uc_copy(sub->fontname);
	    mml[i].ti.checkable = true;
	    mml[i].ti.checked = sub==cv->sc->parent;
	    mml[i].ti.userdata = sub;
	    mml[i].invoke = CVMenuShowSubChar;
	    mml[i].ti.fg = mml[i].ti.bg = COLOR_DEFAULT;
	}
    }
    mml[0].ti.disabled = (mm==NULL || cv->sc->parent!=mm->normal || mm->apple);
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItemArrayCopy(mml,NULL);
    if ( mml!=mmlist ) {
	for ( i=base; mml[i].ti.text!=NULL; ++i )
	    free( mml[i].ti.text);
	free(mml);
    }
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void CVMenuContextualHelp(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void CharViewMenu_ContextualHelp(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    help("charview.html");
}

static GMenuItem mblist[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, 0, 0, edlist, edlistcheck },
    { { (unichar_t *) N_("_Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 0, 0, ptlist, ptlistcheck },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, 0, 0, ellist, ellistcheck },
    { { (unichar_t *) N_("H_ints"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, 0, 0, htlist, htlistcheck },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, 0, 0, vwlist, vwlistcheck },
    { { (unichar_t *) N_("_Metrics"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 0, 0, mtlist, mtlistcheck },
/* GT: Here (and following) MM means "MultiMaster" */
    { { (unichar_t *) N_("MM"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, 0, 0, mmlist, mmlistcheck },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, 0, 0, wnmenu, CVWindowMenuBuild },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};

static GMenuItem mblist_nomm[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, 0, 0, edlist, edlistcheck },
    { { (unichar_t *) N_("_Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 0, 0, ptlist, ptlistcheck },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, 0, 0, ellist, ellistcheck },
    { { (unichar_t *) N_("H_ints"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, 0, 0, htlist, htlistcheck },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, 0, 0, vwlist, vwlistcheck },
    { { (unichar_t *) N_("_Metrics"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 0, 0, mtlist, mtlistcheck },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, 0, 0, wnmenu, CVWindowMenuBuild },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};

static void _CharViewCreate(CharView *cv, SplineChar *sc, FontView *fv,int enc) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetData gd;
    int sbsize;
    FontRequest rq;
    int as, ds, ld;
    extern int updateflex;
    static unichar_t fixed[] = { 'f','i','x','e','d',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t', '\0' };
    static unichar_t *infofamily=NULL;

    if ( !cvcolsinited )
	CVColInit();

    if ( sc->views==NULL && updateflex )
	SplineCharIsFlexible(sc);

    cv->sc = sc;
    cv->scale = .5;
    cv->xoff = cv->yoff = 20;
    cv->next = sc->views;
    sc->views = cv;
    cv->fv = fv;
    cv->map_of_enc = fv->map;
    cv->enc = enc;

    cv->drawmode = dm_fore;

    cv->showback = CVShows.showback;
    cv->showfore = CVShows.showfore;
    cv->showgrids = CVShows.showgrids;
    cv->showhhints = CVShows.showhhints;
    cv->showvhints = CVShows.showvhints;
    cv->showdhints = CVShows.showdhints;
    cv->showpoints = CVShows.showpoints;
    cv->showrulers = CVShows.showrulers;
    cv->showfilled = CVShows.showfilled;
    cv->showrounds = CVShows.showrounds;
    cv->showmdx = CVShows.showmdx;
    cv->showmdy = CVShows.showmdy;
    cv->showhmetrics = CVShows.showhmetrics;
    cv->showvmetrics = CVShows.showvmetrics;
    cv->markextrema = CVShows.markextrema;
    cv->markpoi = CVShows.markpoi;
    cv->showblues = CVShows.showblues;
    cv->showfamilyblues = CVShows.showfamilyblues;
    cv->showanchor = CVShows.showanchor;
    cv->showcpinfo = CVShows.showcpinfo;
    cv->showtabs = CVShows.showtabs;

    cv->infoh = 13;
    cv->rulerh = 13;

    GDrawGetSize(cv->gw,&pos);
    memset(&gd,0,sizeof(gd));
    gd.pos.y = cv->mbh+cv->infoh;
    gd.pos.width = sbsize = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
    gd.pos.height = pos.height-cv->mbh-cv->infoh - sbsize;
    gd.pos.x = pos.width-sbsize;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    cv->vsb = GScrollBarCreate(cv->gw,&gd,cv);

    gd.pos.y = pos.height-sbsize; gd.pos.height = sbsize;
    gd.pos.width = pos.width - sbsize;
    gd.pos.x = 0;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    cv->hsb = GScrollBarCreate(cv->gw,&gd,cv);

    GDrawGetSize(cv->gw,&pos);
    pos.y = cv->mbh+cv->infoh; pos.height -= cv->mbh + sbsize + cv->infoh;
    pos.x = 0; pos.width -= sbsize;
    if ( cv->showrulers ) {
	pos.y += cv->rulerh; pos.height -= cv->rulerh;
	pos.x += cv->rulerh; pos.width -= cv->rulerh;
    }
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    cv->v = GWidgetCreateSubWindow(cv->gw,&pos,v_e_h,cv,&wattrs);

    if ( GDrawRequestDeviceEvents(cv->v,input_em_cnt,input_em)>0 ) {
	/* Success! They've got a wacom tablet */
    }

    if ( infofamily==NULL ) {
	infofamily = uc_copy(GResourceFindString("CharView.InfoFamily"));
	if ( infofamily==NULL )
	    infofamily = fixed;
    }

    memset(&rq,0,sizeof(rq));
    rq.family_name = infofamily;
    rq.point_size = -7;
    rq.weight = 400;
    cv->small = GDrawInstanciateFont(GDrawGetDisplayOfWindow(cv->gw),&rq);
    GDrawFontMetrics(cv->small,&as,&ds,&ld);
    cv->sfh = as+ds; cv->sas = as;
    rq.point_size = 10;
    cv->normal = GDrawInstanciateFont(GDrawGetDisplayOfWindow(cv->gw),&rq);
    GDrawFontMetrics(cv->normal,&as,&ds,&ld);
    cv->nfh = as+ds; cv->nas = as;

    cv->height = pos.height; cv->width = pos.width;
    cv->gi.u.image = gcalloc(1,sizeof(struct _GImage));
    cv->gi.u.image->image_type = it_mono;
    cv->gi.u.image->clut = gcalloc(1,sizeof(GClut));
    cv->gi.u.image->clut->trans_index = cv->gi.u.image->trans = 0;
    cv->gi.u.image->clut->clut_len = 2;
    cv->gi.u.image->clut->clut[0] = 0xffffff;
    cv->gi.u.image->clut->clut[1] = backimagecol;
    cv->b1_tool = cvt_pointer; cv->cb1_tool = cvt_pointer;
    cv->b2_tool = cvt_magnify; cv->cb2_tool = cvt_ruler;
    cv->s1_tool = cvt_freehand; cv->s2_tool = cvt_pen;
    cv->er_tool = cvt_knife;
    cv->showing_tool = cvt_pointer;
    cv->pressed_tool = cv->pressed_display = cv->active_tool = cvt_none;
    cv->layerheads[dm_fore] = &sc->layers[ly_fore];
    cv->layerheads[dm_back] = &sc->layers[ly_back];
    cv->layerheads[dm_grid] = &fv->sf->grid;

#if HANYANG
    if ( sc->parent->rules!=NULL && sc->compositionunit )
	Disp_DefaultTemplate(cv);
#endif

    cv->olde.x = -1;

    cv->ft_dpi = 72; cv->ft_pointsize = 12.0; cv->ft_ppem = 12;

    /*GWidgetHidePalettes();*/
    /*cv->tools = CVMakeTools(cv);*/
    /*cv->layers = CVMakeLayers(cv);*/

    CVFit(cv);
    GDrawSetVisible(cv->v,true);
    cv->gic = GDrawCreateInputContext(cv->v,gic_root|gic_orlesser);
    GDrawSetVisible(cv->gw,true);
}

void DefaultY(GRect *pos) {
    static int nexty=0;
    GRect size;

    GDrawGetSize(GDrawGetRoot(NULL),&size);
    if ( nexty!=0 ) {
	FontView *fv;
	int any=0, i;
	BDFFont *bdf;
	/* are there any open cv/bv windows? */
	for ( fv = fv_list; fv!=NULL && !any; fv = fv->next ) {
	    for ( i=0; i<fv->sf->glyphcnt; ++i ) if ( fv->sf->glyphs[i]!=NULL ) {
		if ( fv->sf->glyphs[i]->views!=NULL ) {
		    any = true;
	    break;
		}
	    }
	    for ( bdf = fv->sf->bitmaps; bdf!=NULL && !any; bdf=bdf->next ) {
		for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
		    if ( bdf->glyphs[i]->views!=NULL ) {
			any = true;
		break;
		    }
		}
	    }
	}
	if ( !any ) nexty = 0;
    }
    pos->y = nexty;
    nexty += 200;
    if ( nexty+pos->height > size.height )
	nexty = 0;
}

static void CharViewInit(void);

CharView *CharViewCreate(SplineChar *sc, FontView *fv,int enc) {
    CharView *cv = gcalloc(1,sizeof(CharView));
    GWindowAttrs wattrs;
    GRect pos, zoom;
    GWindow gw;
    GGadgetData gd;
    GTabInfo aspects[2];
    GRect gsize;
    char buf[300];

    CharViewInit();

    cv->sc = sc;
    cv->fv = fv;
    cv->enc = enc;
    cv->map_of_enc = fv->map;		/* I know this is done again in _CharViewCreate, but it needs to be done before creating the title */

    SCLigCaretCheck(sc,false);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_utf8_ititle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.utf8_icon_title = CVMakeTitles(cv,buf);
    wattrs.utf8_window_title = buf;
    wattrs.icon = CharIcon(cv, fv);
    if ( wattrs.icon )
	wattrs.mask |= wam_icon;
    pos.x = GGadgetScale(104)+6; pos.width=pos.height = 540;
    DefaultY(&pos);

    cv->gw = gw = GDrawCreateTopWindow(NULL,&pos,cv_e_h,cv,&wattrs);
    free( (unichar_t *) wattrs.icon_title );

    GDrawGetSize(GDrawGetRoot(screen_display),&zoom);
    zoom.x = CVPalettesWidth(); zoom.width -= zoom.x-10;
    zoom.height -= 30;			/* Room for title bar & such */
    GDrawSetZoom(gw,&zoom,-1);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = CVMenuContextualHelp;
    gd.u.menu = sc->parent->mm==NULL ? mblist_nomm : mblist;
    cv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(cv->mb,&gsize);
    cv->mbh = gsize.height;

    memset(aspects,0,sizeof(aspects));
    aspects[0].text = (unichar_t *) sc->name;
    aspects[0].text_is_1byte = true;
	/* NOT visible until we add a second glyph to the stack */
    gd.flags = gg_enabled|gg_tabset_nowindow|gg_tabset_scroll|gg_pos_in_pixels;
    gd.u.menu = NULL;
    gd.u.tabs = aspects;
    gd.pos.x = 0;
    gd.pos.y = cv->mbh;
    gd.handle_controlevent = CVChangeToFormer;
    cv->tabs = GTabSetCreate( gw, &gd, NULL );
    cv->former_cnt = 1;
    cv->former_names[0] = copy(sc->name);

    _CharViewCreate(cv,sc,fv,enc);
return( cv );
}

void CharViewFree(CharView *cv) {
    int i;

    BDFCharFree(cv->filled);
    if ( cv->ruler_w ) {
	GDrawDestroyWindow(cv->ruler_w);
	cv->ruler_w = NULL;
    }
    free(cv->gi.u.image->clut);
    free(cv->gi.u.image);
#if HANYANG
    if ( cv->jamodisplay!=NULL )
	Disp_DoFinish(cv->jamodisplay,true);
#endif

    CVDebugFree(cv->dv);

    SplinePointListsFree(cv->gridfit);
    FreeType_FreeRaster(cv->oldraster);
    FreeType_FreeRaster(cv->raster);

    CVDebugFree(cv->dv);

    for ( i=0; i<cv->former_cnt; ++i )
	free(cv->former_names[i]);

    free(cv);
}

int CVValid(SplineFont *sf, SplineChar *sc, CharView *cv) {
    /* A charview may have been closed. A splinechar may have been removed */
    /*  from a font */
    CharView *test;

    if ( cv->sc!=sc || sc->parent!=sf )
return( false );
    if ( sc->orig_pos<0 || sc->orig_pos>sf->glyphcnt )
return( false );
    if ( sf->glyphs[sc->orig_pos]!=sc )
return( false );
    for ( test=sc->views; test!=NULL; test=test->next )
	if ( test==cv )
return( true );

return( false );
}

static void sv_fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static void sv_pllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    cv_pllistcheck(cv,mi,e);
}

static void sv_edlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    cv_edlistcheck(cv,mi,e,false);
}

static void sv_ptlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    cv_ptlistcheck(cv,mi,e);
}

static void sv_allistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    cv_allistcheck(cv,mi,e);
}

static void sv_ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    cv_ellistcheck(cv,mi,e,false);
}

static void sv_sllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    cv_sllistcheck(cv,mi,e);
}

static void sv_vwlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    cv_vwlistcheck(cv,mi,e);
}

static void SVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    _FVMenuSave(sv->fv);
}

static void SVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    _FVMenuSaveAs(sv->fv);
}

static void SVMenuGenerate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    _FVMenuGenerate(sv->fv,false);
}

static void SVSelectAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    if ( CVSetSel(cv,-1))
	SCUpdateAll(cv->sc);
}

static void SVSelectNone(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    if ( CVClearSel(cv))
	SCUpdateAll(cv->sc);
}

static void SVMenuNextPrevPt(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    CVNextPrevPt(cv,mi);
}

static void SVMenuNextPrevCPt(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    CVNextPrevCPt(cv,mi);
}

static void SVUndo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    CVDoUndo(cv);
}

static void SVRedo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    CVDoRedo(cv);
}

static void SVCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVCut(cv);
}

static void SVCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVCopy(cv);
}

static void SVPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVPaste(cv);
}

static void SVClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;

    if ( !CVAnySel(cv,NULL,NULL,NULL,NULL))
return;
    CVDoClear(cv);
    CVCharChangedUpdate(cv);
}

static void SVMerge(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVMerge(cv,false);
}

static void SVElide(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVMerge(cv,true);
}

static void SVJoin(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVJoin(cv);
}

static void SVUnlinkRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVUnlinkRef(cv);
}

static void SVRemoveUndoes(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);

    UndoesFree(sv->cv_srch.layerheads[sv->cv_srch.drawmode]->undoes);
    UndoesFree(sv->cv_srch.layerheads[sv->cv_srch.drawmode]->redoes);
    sv->cv_srch.layerheads[sv->cv_srch.drawmode]->undoes = sv->cv_srch.layerheads[sv->cv_srch.drawmode]->redoes = NULL;

    UndoesFree(sv->cv_rpl.layerheads[sv->cv_rpl.drawmode]->undoes);
    UndoesFree(sv->cv_rpl.layerheads[sv->cv_rpl.drawmode]->redoes);
    sv->cv_rpl.layerheads[sv->cv_rpl.drawmode]->undoes = sv->cv_rpl.layerheads[sv->cv_rpl.drawmode]->redoes = NULL;
}

static void SVMenuPointType(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVMenuPointType(cv,mi);
}

static void SVMenuMakeFirst(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVMenuMakeFirst(cv);
}

static void SVMenuConstrain(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    CVConstrainSelection(cv,mi->mid==MID_Average?0:
			    mi->mid==MID_SpacePts?1:
			    2);
}

static void SVMenuMakeParallel(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    CVMakeParallel(cv);
}

static void SVMenuFontInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    DelayEvent(FontMenuFontInfo,sv->fv);
}

static void SVMenuGetInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    SplinePoint *sp; SplineSet *spl; RefChar *r; ImageList *im;

    if( !CVOneThingSel(cv,&sp,&spl,&r,&im,NULL))
return;
    CVGetInfo(cv);
}

static void SVMenuTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    int anysel = CVAnySel(cv,NULL,NULL,NULL,NULL);
    TransformDlgCreate(cv,transfunc,getorigin,!anysel && cv->drawmode==dm_fore,
	cvt_none);
}

static void SVMenuStroke(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    CVStroke(cv);
}

static void SVMenuOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    int anysel;

    (void) CVAnySel(cv,&anysel,NULL,NULL,NULL);
    _CVMenuOverlap(cv,anysel ? over_rmselected: over_remove);
}

static void SVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    static struct simplifyinfo smpl = { sf_normal,.75,.05,0 };
    smpl.err = (cv->sc->parent->ascent+cv->sc->parent->descent)/1000.;
    smpl.linelenmax = (cv->sc->parent->ascent+cv->sc->parent->descent)/100.;
    CVPreserveState(cv);
    cv->layerheads[cv->drawmode]->splines = SplineCharSimplify(cv->sc,cv->layerheads[cv->drawmode]->splines,&smpl);
    CVCharChangedUpdate(cv);
}

static void SVMenuCleanupGlyph(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    static struct simplifyinfo smpl = { sf_cleanup };
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    CVPreserveState(cv);
    cv->layerheads[cv->drawmode]->splines = SplineCharSimplify(cv->sc,cv->layerheads[cv->drawmode]->splines,&smpl);
    CVCharChangedUpdate(cv);
}

static void SVMenuAddExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVMenuAddExtrema(cv);
}

static void SVMenuRound2Int(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVMenuRound2Int(cv,1.0);
}

static void SVMenuDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVMenuDir(cv,mi);
}

static void SVMenuCorrectDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    int changed=false;
    CVPreserveState(cv);
    cv->layerheads[cv->drawmode]->splines = SplineSetsCorrect(cv->layerheads[cv->drawmode]->splines,&changed);
    if ( changed )
	CVCharChangedUpdate(cv);
}

static void SVMenuPaletteShow(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;

    CVPaletteSetVisible(cv, mi->mid==MID_Tools, !CVPaletteIsVisible(cv, mi->mid==MID_Tools));
}

static void SVMenuScale(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;
    _CVMenuScale(cv,mi);
}

static void SVMenuShowHide(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;

    cv->showpoints = !cv->showpoints;
    GDrawRequestExpose(cv->v,NULL,false);
}

static void SVMenuMarkExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;

    cv->markextrema = !cv->markextrema;
    GDrawRequestExpose(cv->v,NULL,false);
}

static void SVMenuShowHideRulers(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);
    CharView *cv = sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch;

    if ( cv->showrulers == sv->cv_srch.showrulers )
	_CVMenuShowHideRulers(&sv->cv_srch);
    if ( cv->showrulers == sv->cv_rpl.showrulers )
	_CVMenuShowHideRulers(&sv->cv_rpl);
}

static GMenuItem sv_fllist[] = {
    { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, 'N', ksm_control, NULL, NULL, MenuNew },
    { { (unichar_t *) N_("_Open"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) N_("Recen_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) N_("_Close"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, SVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Save"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, SVMenuSave },
    { { (unichar_t *) N_("S_ave as..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, SVMenuSaveAs },
    { { (unichar_t *) N_("_Generate Fonts..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'G' }, 'G', ksm_control|ksm_shift, NULL, NULL, SVMenuGenerate },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Quit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem sv_sllist[] = {
    { { (unichar_t *) N_("Select _All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, SVSelectAll, MID_SelAll },
    { { (unichar_t *) N_("_Deselect All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, GK_Escape, 0, NULL, NULL, SVSelectNone, MID_SelNone },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_First Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, '.', ksm_control, NULL, NULL, SVMenuNextPrevPt, MID_FirstPt },
    { { (unichar_t *) N_("First P_oint, Next Contour"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, '.', ksm_meta|ksm_control, NULL, NULL, SVMenuNextPrevPt, MID_FirstPtNextCont },
    { { (unichar_t *) N_("_Next Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, '}', ksm_shift|ksm_control, NULL, NULL, SVMenuNextPrevPt, MID_NextPt },
    { { (unichar_t *) N_("_Prev Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '{', ksm_shift|ksm_control, NULL, NULL, SVMenuNextPrevPt, MID_PrevPt },
    { { (unichar_t *) N_("Ne_xt Control Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'x' }, ';', ksm_control, NULL, NULL, SVMenuNextPrevCPt, MID_NextCP },
    { { (unichar_t *) N_("P_rev Control Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, ':', ksm_shift|ksm_control, NULL, NULL, SVMenuNextPrevCPt, MID_PrevCP },
    { NULL }
};

static GMenuItem sv_edlist[] = {
    { { (unichar_t *) N_("_Undo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL, SVUndo, MID_Undo },
    { { (unichar_t *) N_("_Redo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL, SVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Cu_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, SVCut, MID_Cut },
    { { (unichar_t *) N_("_Copy"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, SVCopy, MID_Copy },
    { { (unichar_t *) N_("_Paste"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, SVPaste, MID_Paste },
    { { (unichar_t *) N_("C_lear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, GK_Delete, 0, NULL, NULL, SVClear, MID_Clear },
    { { (unichar_t *) N_("_Merge"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, 'M', ksm_control, NULL, NULL, SVMerge, MID_Merge },
    { { (unichar_t *) N_("_Elide"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, 'M', ksm_meta|ksm_control, NULL, NULL, SVElide, MID_Elide },
    { { (unichar_t *) N_("_Join"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'J' }, 'J', ksm_control|ksm_shift, NULL, NULL, SVJoin, MID_Join },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Select"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 0, ksm_control, sv_sllist, sv_sllistcheck },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("U_nlink Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, 'U', ksm_control, NULL, NULL, SVUnlinkRef, MID_UnlinkRef },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Remo_ve Undoes"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'e' }, '\0', 0, NULL, NULL, SVRemoveUndoes, MID_RemoveUndoes },
    { NULL }
};

static GMenuItem sv_ptlist[] = {
    { { (unichar_t *) N_("_Curve"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'C' }, '2', ksm_control, NULL, NULL, SVMenuPointType, MID_Curve },
    { { (unichar_t *) N_("C_orner"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'o' }, '3', ksm_control, NULL, NULL, SVMenuPointType, MID_Corner },
    { { (unichar_t *) N_("_Tangent"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'T' }, '4', ksm_control, NULL, NULL, SVMenuPointType, MID_Tangent },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Make First"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, '1', ksm_control, NULL, NULL, SVMenuMakeFirst, MID_MakeFirst },
    { NULL }
};

static GMenuItem sv_allist[] = {
/* GT: Align these points to their average position */
    { { (unichar_t *) N_("_Average Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, '@', ksm_control|ksm_shift, NULL, NULL, SVMenuConstrain, MID_Average },
    { { (unichar_t *) N_("_Space Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, '#', ksm_control|ksm_shift, NULL, NULL, SVMenuConstrain, MID_SpacePts },
    { { (unichar_t *) N_("Space _Regions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, '\0', ksm_control|ksm_shift, NULL, NULL, SVMenuConstrain, MID_SpaceRegion },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Make _Parallel..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, SVMenuMakeParallel, MID_MakeParallel },
    { NULL }
};

static GMenuItem sv_ellist[] = {
    { { (unichar_t *) N_("_Font Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 'F', ksm_control|ksm_shift, NULL, NULL, SVMenuFontInfo },
    { { (unichar_t *) N_("Get _Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, 'I', ksm_control, NULL, NULL, SVMenuGetInfo, MID_GetInfo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Transform..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\\', ksm_control, NULL, NULL, SVMenuTransform },
    { { (unichar_t *) N_("_Expand Stroke..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, 'E', ksm_control|ksm_shift, NULL, NULL, SVMenuStroke, MID_Stroke },
    { { (unichar_t *) N_("_Remove Overlap"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'v' }, 'O', ksm_control|ksm_shift, NULL, NULL, SVMenuOverlap, MID_RmOverlap },
    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 'M', ksm_control|ksm_shift, NULL, NULL, SVMenuSimplify, MID_Simplify },
    { { (unichar_t *) N_("Clea_nup Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, '\0', ksm_control|ksm_shift, NULL, NULL, SVMenuCleanupGlyph, MID_CleanupGlyph },
    { { (unichar_t *) N_("Add E_xtrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'x' }, 'X', ksm_control|ksm_shift, NULL, NULL, SVMenuAddExtrema, MID_AddExtrema },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("A_lign"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, '\0', ksm_control|ksm_shift, sv_allist, sv_allistcheck },
    { { (unichar_t *) N_("To _Int"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, '_', ksm_control|ksm_shift, NULL, NULL, SVMenuRound2Int, MID_Round },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Cl_ockwise"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'o' }, '\0', 0, NULL, NULL, SVMenuDir, MID_Clockwise },
    { { (unichar_t *) N_("Cou_nter Clockwise"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'n' }, '\0', 0, NULL, NULL, SVMenuDir, MID_Counter },
    { { (unichar_t *) N_("_Correct Direction"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, 'D', ksm_control|ksm_shift, NULL, NULL, SVMenuCorrectDir, MID_Correct },
    { NULL }
};

static GMenuItem sv_pllist[] = {
    { { (unichar_t *) N_("_Tools"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, SVMenuPaletteShow, MID_Tools },
    { { (unichar_t *) N_("_Layers"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'L' }, '\0', ksm_control, NULL, NULL, SVMenuPaletteShow, MID_Layers },
    { { (unichar_t *) N_("_Docked Palettes"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'D' }, '\0', ksm_control, NULL, NULL, CVMenuPalettesDock, MID_DockPalettes },
    { NULL }
};

static GMenuItem sv_vwlist[] = {
    { { (unichar_t *) N_("_Fit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 'F', ksm_control, NULL, NULL, SVMenuScale, MID_Fit },
    { { (unichar_t *) N_("Z_oom out"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '-', ksm_control|ksm_meta, NULL, NULL, SVMenuScale, MID_ZoomOut },
    { { (unichar_t *) N_("Zoom _in"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'i' }, '+', ksm_shift|ksm_control|ksm_meta, NULL, NULL, SVMenuScale, MID_ZoomIn },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Hide Poin_ts"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, 'D', ksm_control, NULL, NULL, SVMenuShowHide, MID_HidePoints },
    { { (unichar_t *) N_("_Mark Extrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, SVMenuMarkExtrema, MID_MarkExtrema },
    /*{ { (unichar_t *) N_("Fi_ll"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'l' }, '\0', 0, NULL, NULL, SVMenuFill, MID_Fill },*/
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, }},
    { { (unichar_t *) N_("_Palettes"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', 0, sv_pllist, sv_pllistcheck },
    { { (unichar_t *) N_("Hide _Rulers"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, '\0', ksm_control, NULL, NULL, SVMenuShowHideRulers, MID_HideRulers },
    { NULL }
};

static GMenuItem sv_mblist[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, 0, 0, sv_fllist, sv_fllistcheck },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, 0, 0, sv_edlist, sv_edlistcheck },
    { { (unichar_t *) N_("_Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 0, 0, sv_ptlist, sv_ptlistcheck },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, 0, 0, sv_ellist, sv_ellistcheck },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, 0, 0, sv_vwlist, sv_vwlistcheck },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, 0, 0, wnmenu, SVWindowMenuBuild },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};

static void CharViewInit(void) {
    int i;
    static int done = false;

    if ( done )
return;
    done = true;

    mbDoGetText(sv_mblist);
    mbDoGetText(mblist);
    for ( i=0; mblist_nomm[i].ti.text!=NULL; ++i )
	mblist_nomm[i].ti.text = (unichar_t *) _((char *) mblist_nomm[i].ti.text);
}

static int sv_cv_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	InfoExpose(cv,gw,event);
	CVLogoExpose(cv,gw,event);
      break;
      case et_char:
	SVChar(cv->searcher,event);
      break;
      case et_charup:
	CVCharUp(cv,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == cv->hsb )
		CVHScroll(cv,&event->u.control.u.sb);
	    else
		CVVScroll(cv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_map:
	if ( event->u.map.is_visible )
	    CVPaletteActivate(cv);
	else
	    CVPalettesHideIfMine(cv);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    CVResize(cv);
      break;
      case et_destroy:
	if ( cv->backimgs!=NULL ) {
	    GDrawDestroyWindow(cv->backimgs);
	    cv->backimgs = NULL;
	}
      break;
      case et_mouseup: case et_mousedown:
	GGadgetEndPopup();
	CVPaletteActivate(cv);
      break;
      case et_mousemove:
	if ( event->u.mouse.y>cv->mbh ) {
	    int enc = CVCurEnc(cv);
	    SCPreparePopup(cv->gw,cv->sc,cv->fv->map->remap,enc,
		    UniFromEnc(enc,cv->fv->map->enc));
	}
      break;
    }
return( true );
}

void SVCharViewInits(SearchView *sv) {
    GGadgetData gd;
    GWindowAttrs wattrs;
    GRect pos, gsize;

    CharViewInit();

    sv->cv_srch.searcher = sv;
    sv->cv_rpl.inactive = true;
    sv->cv_rpl.searcher = sv;

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = CVMenuContextualHelp;
    gd.u.menu = sv_mblist;
    sv->mb = GMenuBarCreate( sv->gw, &gd, NULL);
    GGadgetGetSize(sv->mb,&gsize);
    sv->mbh = gsize.height;

    sv->dummy_fv.map = &sv->dummy_map;
    sv->dummy_map.map = sv->map;
    sv->dummy_map.backmap = sv->backmap;
    sv->dummy_map.enccount = sv->dummy_map.encmax = sv->dummy_map.backmax = 2;
    sv->dummy_map.enc = &custom;

    pos.y = sv->mbh+sv->fh+10; pos.height = 220;
    pos.width = pos.height; pos.x = 10+pos.width+20;	/* Do replace first so palettes appear propperly */
    sv->rpl_x = pos.x; sv->cv_y = pos.y;
    sv->cv_height = pos.height; sv->cv_width = pos.width;
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    sv->cv_rpl.gw = GWidgetCreateSubWindow(sv->gw,&pos,sv_cv_e_h,&sv->cv_rpl,&wattrs);
    _CharViewCreate(&sv->cv_rpl, &sv->sc_rpl, &sv->dummy_fv, 1);

    pos.x = 10;
    sv->cv_srch.gw = GWidgetCreateSubWindow(sv->gw,&pos,sv_cv_e_h,&sv->cv_srch,&wattrs);
    _CharViewCreate(&sv->cv_srch, &sv->sc_srch, &sv->dummy_fv, 0);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
