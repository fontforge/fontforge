/* Copyright (C) 2000-2002 by George Williams */
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
#include "nomen.h"
#include <math.h>
#include <ustring.h>
#include <gkeysym.h>
#include <utype.h>
#include <locale.h>

extern int _GScrollBar_Width;
extern struct lconv localeinfo;
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
	0		/* mark extrema */
};
Color nextcpcol = 0x007090;
Color prevcpcol = 0xff00ff;

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
    GDrawSetXORMode(pixmap);
    GDrawSetXORBase(pixmap,GDrawGetDefaultBackground(NULL));
    GDrawDrawRect(pixmap,&r,0x000000);
    GDrawSetCopyMode(pixmap);
    GDrawSetDashedLine(pixmap,0,0,0);
}

static void CVDrawBB(CharView *cv, GWindow pixmap, DBounds *bb) {
    GRect r;

    r.x =  cv->xoff + rint(bb->minx*cv->scale);
    r.y = -cv->yoff + cv->height - rint(bb->maxy*cv->scale);
    r.width = rint((bb->maxx-bb->minx)*cv->scale);
    r.height = rint((bb->maxy-bb->miny)*cv->scale);
    GDrawSetDashedLine(pixmap,1,1,0);
    GDrawDrawRect(pixmap,&r,0x000000);
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
		y = -(l->next->asend.y-l->asstart.y)*(real)l->asstart.x/(l->next->asend.x-l->asstart.x) +
			l->asstart.y;
		if ( l->asstart.x<0 ) {
		    l->asstart.x = 0;
		    l->asstart.y = y;
		} else {
		    l->next->asend.x = 0;
		    l->next->asend.y = y;
		}
	    }
	    if (( l->asstart.x<cv->width && l->next->asend.x>cv->width ) ||
		    ( l->asstart.x>cv->width && l->next->asend.x<cv->width )) {
		y = (l->next->asend.y-l->asstart.y)*(real)(cv->width-l->asstart.x)/(l->next->asend.x-l->asstart.x) +
			l->asstart.y;
		if ( l->asstart.x>cv->width ) {
		    l->asstart.x = cv->width;
		    l->asstart.y = y;
		} else {
		    l->next->asend.x = cv->width;
		    l->next->asend.y = y;
		}
	    }
	    if (( l->asstart.y<0 && l->next->asend.y>0 ) ||
		    ( l->asstart.y>0 && l->next->asend.y<0 )) {
		x = -(l->next->asend.x-l->asstart.x)*(real)l->asstart.y/(l->next->asend.y-l->asstart.y) +
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
	    }
	    if (( l->asstart.y<cv->height && l->next->asend.y>cv->height ) ||
		    ( l->asstart.y>cv->height && l->next->asend.y<cv->height )) {
		x = (l->next->asend.x-l->asstart.x)*(real)(cv->height-l->asstart.y)/(l->next->asend.y-l->asstart.y) +
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
	    }
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
	    if ( !CVSplineOutside(cv,spline) ) {
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

static void DrawPoint(CharView *cv, GWindow pixmap, SplinePoint *sp, SplineSet *spl) {
    GRect r;
    int x, y, cx, cy;
    Color col = sp==spl->first ? 0x707000 : 0xff0000;

    if ( cv->markextrema && !sp->nonextcp && !sp->noprevcp &&
	    ((sp->nextcp.x==sp->me.x && sp->prevcp.x==sp->me.x) ||
	     (sp->nextcp.y==sp->me.y && sp->prevcp.y==sp->me.y)) )
	 col = 0xc00080;

    x =  cv->xoff + rint(sp->me.x*cv->scale);
    y = -cv->yoff + cv->height - rint(sp->me.y*cv->scale);
    if ( x<-4000 || y<-4000 || x>cv->width+4000 || y>=cv->height+4000 )
return;

    /* draw the control points if it's selected */
    if ( sp->selected ) {
	if ( !sp->nonextcp ) {
	    cx =  cv->xoff + rint(sp->nextcp.x*cv->scale);
	    cy = -cv->yoff + cv->height - rint(sp->nextcp.y*cv->scale);
	    GDrawDrawLine(pixmap,x,y,cx,cy, nextcpcol);
	    GDrawDrawLine(pixmap,cx-3,cy-3,cx+3,cy+3,nextcpcol);
	    GDrawDrawLine(pixmap,cx+3,cy-3,cx-3,cy+3,nextcpcol);
	}
	if ( !sp->noprevcp ) {
	    cx =  cv->xoff + rint(sp->prevcp.x*cv->scale);
	    cy = -cv->yoff + cv->height - rint(sp->prevcp.y*cv->scale);
	    GDrawDrawLine(pixmap,x,y,cx,cy, prevcpcol);
	    GDrawDrawLine(pixmap,cx-3,cy-3,cx+3,cy+3,prevcpcol);
	    GDrawDrawLine(pixmap,cx+3,cy-3,cx-3,cy+3,prevcpcol);
	}
    }

    if ( x<-4 || y<-4 || x>cv->width+4 || y>=cv->height+4 )
return;
    r.x = x-2;
    r.y = y-2;
    r.width = r.height = 5;
    if ( sp->pointtype==pt_curve ) {
	--r.x; --r.y; r.width +=2; r.height += 2;
	if ( sp->selected )
	    GDrawDrawElipse(pixmap,&r,col);
	else
	    GDrawFillElipse(pixmap,&r,col);
    } else if ( sp->pointtype==pt_corner ) {
	if ( sp->selected )
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
	if ( sp->selected )
	    GDrawDrawPoly(pixmap,gp,4,col);
	else
	    GDrawFillPoly(pixmap,gp,4,col);
    }
    if (( sp->roundx || sp->roundy ) &&
	    (((cv->showrounds&1) && cv->scale>=.3) || (cv->showrounds&2)) ) {
	r.x = x-5; r.y = y-5;
	r.width = r.height = 11;
	GDrawDrawElipse(pixmap,&r,col);
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

void CVDrawSplineSet(CharView *cv, GWindow pixmap, SplinePointList *set,
	Color fg, int dopoints, DRect *clip ) {
    Spline *spline, *first;
    SplinePointList *spl;

    for ( spl = set; spl!=NULL; spl = spl->next ) {
	GPointList *gpl = MakePoly(cv,spl), *cur;
	if ( dopoints ) {
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		DrawPoint(cv,pixmap,spline->from,spl);
		if ( first==NULL ) first = spline;
	    }
	    if ( spline==NULL )
		DrawPoint(cv,pixmap,spl->last,spl);
	}
	for ( cur=gpl; cur!=NULL; cur=cur->next )
	    GDrawDrawPoly(pixmap,cur->gp,cur->cnt,fg);
	GPLFree(gpl);
    }
}

static void CVDrawTemplates(CharView *cv,GWindow pixmap,SplineChar *template,DRect *clip) {
    RefChar *r;

    CVDrawSplineSet(cv,pixmap,template->splines,0x009800,false,clip);
    for ( r=template->refs; r!=NULL; r=r->next )
	CVDrawSplineSet(cv,pixmap,r->splines,0x009800,false,clip);
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
    GDrawFillPoly(pixmap,clipped,j,0xd0a0a0);
}

static void CVShowMinimumDistance(CharView *cv, GWindow pixmap,MinimumDistance *md) {
    int x1,y1, x2,y2;
    int xa, ya;

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
	GDrawDrawArrow(pixmap, x1,ya, x2,ya, 2, 0xe04040);
	GDrawSetDashedLine(pixmap,5,5,0);
	GDrawDrawLine(pixmap, x1,ya, x1,y1, 0xe04040);
	GDrawDrawLine(pixmap, x2,ya, x2,y2, 0xe04040);
    } else {
	xa = (x1+x2)/2;
	GDrawDrawArrow(pixmap, xa,y1, xa,y2, 2, 0xe04040);
	GDrawSetDashedLine(pixmap,5,5,0);
	GDrawDrawLine(pixmap, xa,y1, x1,y1, 0xe04040);
	GDrawDrawLine(pixmap, xa,y2, x2,y2, 0xe04040);
    }
    GDrawSetDashedLine(pixmap,0,0,0);
}

static void CVShowHints(CharView *cv, GWindow pixmap) {
    StemInfo *hint;
    GRect r;
    HintInstance *hi;
    int end;
    Color col;
    DStemInfo *dstem;
    MinimumDistance *md;

    GDrawSetDashedLine(pixmap,5,5,0);

    if ( cv->showdhints ) for ( dstem = cv->sc->dstem; dstem!=NULL; dstem = dstem->next ) {
	CVShowDHint(cv,pixmap,dstem);
    }

    if ( cv->showhhints ) for ( hint = cv->sc->hstem; hint!=NULL; hint = hint->next ) {
	if ( hint->width<0 ) {
	    r.y = -cv->yoff + cv->height - rint(hint->start*cv->scale);
	    r.height = rint(-hint->width*cv->scale)+1;
	} else {
	    r.y = -cv->yoff + cv->height - rint((hint->start+hint->width)*cv->scale);
	    r.height = rint(hint->width*cv->scale)+1;
	}
	col = hint->active ? 0x00a000 : 0xa0d0a0;
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
	col = hint->hasconflicts? 0x00ffff : col;
	if ( r.y>=0 && r.y<=cv->height )
	    GDrawDrawLine(pixmap,0,r.y,cv->width,r.y,col);
	if ( r.y+r.height>=0 && r.y+r.height<=cv->width )
	    GDrawDrawLine(pixmap,0,r.y+r.height-1,cv->width,r.y+r.height-1,col);
    }
    if ( cv->showvhints ) for ( hint = cv->sc->vstem; hint!=NULL; hint = hint->next ) {
	if ( hint->width<0 ) {
	    r.x = cv->xoff + rint( (hint->start+hint->width)*cv->scale );
	    r.width = rint(-hint->width*cv->scale)+1;
	} else {
	    r.x = cv->xoff + rint(hint->start*cv->scale);
	    r.width = rint(hint->width*cv->scale)+1;
	}
	col = hint->active ? 0x0000ff : 0xc0c0ff;
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
	col = hint->hasconflicts? 0x00ffff : col;
	if ( r.x>=0 && r.x<=cv->width )
	    GDrawDrawLine(pixmap,r.x,0,r.x,cv->height,col);
	if ( r.x+r.width>=0 && r.x+r.width<=cv->width )
	    GDrawDrawLine(pixmap,r.x+r.width-1,0,r.x+r.width-1,cv->height,col);
    }
    GDrawSetDashedLine(pixmap,0,0,0);

    for ( md=cv->sc->md; md!=NULL; md=md->next )
	CVShowMinimumDistance(cv, pixmap,md);
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

static void DrawSelImageList(CharView *cv,GWindow pixmap,ImageList *backimages) {
    if ( cv->drawmode==dm_back ) {
	while ( backimages!=NULL ) {
	    if ( backimages->selected )
		CVDrawBB(cv,pixmap,&backimages->bb);
	    backimages = backimages->next;
	}
    }
}

static void DrawOldState(CharView *cv, GWindow pixmap, Undoes *undo, DRect *clip) {
    RefChar *refs;

    CVDrawSplineSet(cv,pixmap,undo->u.state.splines,0x008000,false,clip);
    for ( refs=undo->u.state.refs; refs!=NULL; refs=refs->next )
	if ( refs->splines!=NULL )
	    CVDrawSplineSet(cv,pixmap,refs->splines,0x008000,false,clip);
    /* Don't do images... */
}
    
static void DrawTransOrigin(CharView *cv, GWindow pixmap) {
    int x = rint(cv->p.cx*cv->scale) + cv->xoff, y = cv->height-cv->yoff-rint(cv->p.cy*cv->scale);

    GDrawDrawLine(pixmap,x-4,y,x+4,y,0x000000);
    GDrawDrawLine(pixmap,x,y-4,x,y+4,0x000000);
}

static void CVExpose(CharView *cv, GWindow pixmap, GEvent *event ) {
    SplineFont *sf = cv->sc->parent;
    RefChar *rf;
    GRect old;
    DRect clip;

    GDrawPushClip(pixmap,&event->u.expose.rect,&old);

    clip.width = event->u.expose.rect.width/cv->scale;
    clip.height = event->u.expose.rect.height/cv->scale;
    clip.x = (event->u.expose.rect.x-cv->xoff)/cv->scale;
    clip.y = (cv->height-event->u.expose.rect.y-event->u.expose.rect.height-cv->yoff)/cv->scale;

    if ( *cv->uheads[cv->drawmode]!=NULL && (*cv->uheads[cv->drawmode])->undotype==ut_tstate )
	DrawOldState(cv,pixmap,*cv->uheads[cv->drawmode], &clip);

    /* if we've got bg images (and we're showing them) then the hints live in */
    /*  the bg image pixmap (else they get overwritten by the pixmap) */
    if ( (cv->showhhints || cv->showvhints || cv->showdhints) && ( cv->sc->backimages==NULL || !cv->showback) )
	CVShowHints(cv,pixmap);

    if (( cv->showback || cv->drawmode==dm_back ) && cv->sc->backimages!=NULL ) {
	/* This really should be after the grids, but then it would completely*/
	/*  hide them. */
	GRect r;
	if ( cv->backimgs==NULL )
	    cv->backimgs = GDrawCreatePixmap(GDrawGetDisplayOfWindow(cv->v),cv->width,cv->height);
	if ( cv->back_img_out_of_date ) {
	    GDrawFillRect(cv->backimgs,NULL,GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(cv->v)));
	    if ( cv->showhhints || cv->showvhints || cv->showdhints)
		CVShowHints(cv,cv->backimgs);
	    DrawImageList(cv,cv->backimgs,cv->sc->backimages);
	    cv->back_img_out_of_date = false;
	}
	r.x = r.y = 0; r.width = cv->width; r.height = cv->height;
	GDrawDrawPixmap(pixmap,cv->backimgs,&r,0,0);
    }
    if ( cv->showgrids || cv->drawmode==dm_grid ) {
	CVDrawSplineSet(cv,pixmap,cv->fv->sf->gridsplines,0x808080,
		cv->showpoints && cv->drawmode==dm_grid,&clip);
    }
    if ( cv->showhmetrics ) {
	DrawLine(cv,pixmap,0,-8096,0,8096,0x808080);
	DrawLine(cv,pixmap,-8096,0,8096,0,0x808080);
	DrawLine(cv,pixmap,-8096,sf->ascent,8096,sf->ascent,0x808080);
	DrawLine(cv,pixmap,-8096,-sf->descent,8096,-sf->descent,0x808080);
    }
    if ( cv->showvmetrics ) {
	DrawLine(cv,pixmap,(sf->ascent+sf->descent)/2,-8096,(sf->ascent+sf->descent)/2,8096,0x808080);
	DrawLine(cv,pixmap,-8096,sf->vertical_origin,8096,sf->vertical_origin,0x808080);
    }

    if ( cv->showback || cv->drawmode==dm_back )
	DrawSelImageList(cv,pixmap,cv->sc->backimages);
    if (( cv->showfore || cv->drawmode==dm_fore ) && cv->showfilled ) {
	/* Wrong order, I know. But it is useful to have the background */
	/*  visible on top of the fill... */
	GDrawDrawImage(pixmap, &cv->gi, NULL, cv->xoff + cv->filled->xmin,
		-cv->yoff + cv->height-cv->filled->ymax);
    }

    if ( cv->showback || cv->drawmode==dm_back ) {
	/* Used to draw the image list here, but that's too slow. Optimization*/
	/*  is to draw to pixmap, dump pixmap a bit earlier */
	/* Then when we moved the fill image around, we had to deal with the */
	/*  images before the fill... */
	CVDrawSplineSet(cv,pixmap,cv->sc->backgroundsplines,0x009800,
		cv->showpoints && cv->drawmode==dm_back,&clip);
	if ( cv->template1!=NULL )
	    CVDrawTemplates(cv,pixmap,cv->template1,&clip);
	if ( cv->template2!=NULL )
	    CVDrawTemplates(cv,pixmap,cv->template2,&clip);
    }

    if ( cv->showfore || cv->drawmode==dm_fore ) {
	for ( rf=cv->sc->refs; rf!=NULL; rf = rf->next ) {
	    CVDrawSplineSet(cv,pixmap,rf->splines,0,false,&clip);
	    if ( rf->selected )
		CVDrawBB(cv,pixmap,&rf->bb);
	}

	CVDrawSplineSet(cv,pixmap,cv->sc->splines,0,
		cv->showpoints && cv->drawmode==dm_fore,&clip);
    }

    if ( cv->showhmetrics )
	DrawLine(cv,pixmap,cv->sc->width,-32768,cv->sc->width,32767,0x0);
    if ( cv->showvmetrics )
	DrawLine(cv,pixmap,-32768,sf->vertical_origin-cv->sc->vwidth,
			    32767,sf->vertical_origin-cv->sc->vwidth,0x0);

    if ( cv->p.rubberbanding )
	CVDrawRubberRect(pixmap,cv);
    if (( cv->active_tool >= cvt_scale && cv->active_tool <= cvt_skew ) &&
	    cv->p.pressed )
	DrawTransOrigin(cv,pixmap);

    GDrawPopClip(pixmap,&old);
}

void SCUpdateAll(SplineChar *sc) {
    CharView *cv;
    struct splinecharlist *dlist;

    for ( cv=sc->views; cv!=NULL; cv=cv->next )
	GDrawRequestExpose(cv->v,NULL,false);
    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	SCUpdateAll(dlist->sc);
}

void SCOutOfDateBackground(SplineChar *sc) {
    CharView *cv;

    for ( cv=sc->views; cv!=NULL; cv=cv->next )
	cv->back_img_out_of_date = true;
}

static void CVRegenFill(CharView *cv) {
    if ( cv->showfilled ) {
	BDFCharFree(cv->filled);
	cv->filled = SplineCharRasterize(cv->sc,cv->scale*(cv->fv->sf->ascent+cv->fv->sf->descent));
	cv->gi.u.image->data = cv->filled->bitmap;
	cv->gi.u.image->bytes_per_line = cv->filled->bytes_per_line;
	cv->gi.u.image->width = cv->filled->xmax-cv->filled->xmin+1;
	cv->gi.u.image->height = cv->filled->ymax-cv->filled->ymin+1;
    }
}

static void FVRedrawAllCharViews(FontView *fv) {
    int i;
    CharView *cv;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL )
	for ( cv = fv->sf->chars[i]->views; cv!=NULL; cv=cv->next )
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

    GScrollBarSetBounds(cv->vsb,-8000*cv->scale,8000*cv->scale,cv->height);
    GScrollBarSetBounds(cv->hsb,-8000*cv->scale,8000*cv->scale,cv->width);
    GScrollBarSetPos(cv->vsb,cv->yoff);
    GScrollBarSetPos(cv->hsb,cv->xoff);

    GDrawRequestExpose(cv->v,NULL,false);
    if ( cv->showrulers )
	GDrawRequestExpose(cv->gw,NULL,false);
    GDrawGetPointerPosition(cv->v,&e);
    CVUpdateInfo(cv,&e);
}

static void CVFit(CharView *cv) {
    DBounds b;
    real left, right, top, bottom, hsc, wsc;

    SplineCharFindBounds(cv->sc,&b);
    bottom = b.miny;
    top = b.maxy;
    left = b.minx;
    right = b.maxx;

    if ( bottom>0 ) bottom = 0;
    if ( left>0 ) left = 0;
    if ( right<cv->sc->width ) right = cv->sc->width;
    if ( top<bottom ) GDrawIError("Bottom bigger than top!");
    if ( right<left ) GDrawIError("Left bigger than right!");
    top -= bottom;
    right -= left;
    if ( top==0 ) top = 1000;
    if ( right==0 ) right = 1000;
    wsc = cv->width / right;
    hsc = cv->height / top;
    if ( wsc<hsc ) hsc = wsc;

    cv->scale = hsc/1.2;
    if ( cv->scale > 1.0 ) {
	cv->scale = floor(cv->scale);
    } else {
	cv->scale = 1/ceil(1/cv->scale);
    }

    cv->xoff = -(left - (right/10))*cv->scale;
    cv->yoff = -(bottom - (top/10))*cv->scale;

    CVNewScale(cv);
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
    if ( sc->refs!=NULL || sc->splines!=NULL ) {
	bdf = fv->show;
	if ( bdf->chars[sc->enc]==NULL )
	    bdf = fv->filled;
	if ( bdf->chars[sc->enc]==NULL ) {
	    bdf2 = NULL; bdfc = NULL;
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL && bdf->pixelsize<24 ; bdf=bdf->next )
		bdf2 = bdf;
	    if ( bdf2!=NULL && bdf!=NULL ) {
		if ( 24-bdf2->pixelsize < bdf->pixelsize-24 )
		    bdf = bdf2;
	    } else if ( bdf==NULL )
		bdf = bdf2;
	}
	if ( bdf!=NULL )
	    bdfc = bdf->chars[sc->enc];
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

static unichar_t *CVMakeTitles(CharView *cv,unichar_t *ubuf) {
    unichar_t *title;
    SplineChar *sc = cv->sc;

    uc_strncpy(ubuf,sc->name,90);
    u_strcat(ubuf,GStringGetResource(_STR_Bvfrom,NULL));
    uc_strncat(ubuf,sc->parent->fontname,90);
    title = u_copy(ubuf);
    if ( sc->unicodeenc!=-1 && sc->unicodeenc<65536 &&
	    UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]!=NULL ) {
	uc_strcat(ubuf," ");
	u_strcat(ubuf, UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]);
    }
return( title );
}

void SCRefreshTitles(SplineChar *sc) {
    /* Called if the user changes the unicode encoding or the character name */
    CharView *cv;
    unichar_t ubuf[300], *title;

    if ( sc->views==NULL )
return;
    title = CVMakeTitles(sc->views,ubuf);
    for ( cv = sc->views; cv!=NULL; cv=cv->next )
	GDrawSetWindowTitles(cv->gw,ubuf,title);
    free(title);
}

void CVChangeSC(CharView *cv, SplineChar *sc ) {
    unichar_t *title;
    unichar_t ubuf[300];

    if ( cv->expandedge != ee_none ) {
	GDrawSetCursor(cv->v,ct_mypointer);
	cv->expandedge = ee_none;
    }

    CVUnlinkView(cv);
    cv->sc = sc;
    cv->next = sc->views;
    sc->views = cv;
    cv->heads[dm_fore] = &sc->splines; cv->heads[dm_back] = &sc->backgroundsplines;
    cv->uheads[dm_fore] = &sc->undoes[dm_fore]; cv->rheads[dm_back] = &sc->undoes[dm_back];
    cv->rheads[dm_fore] = &sc->redoes[dm_fore]; cv->rheads[dm_back] = &sc->redoes[dm_back];
    cv->p.sp = cv->lastselpt = NULL;
    cv->template1 = cv->template2 = NULL;

    CVNewScale(cv);

    CharIcon(cv,cv->fv);
    title = CVMakeTitles(cv,ubuf);
    GDrawSetWindowTitles(cv->gw,ubuf,title);
    cv->lastselpt = NULL; cv->p.sp = NULL;
    CVInfoDraw(cv,cv->gw);
    free(title);
#if HANYANG
    if ( cv->jamodisplay!=NULL )
	Disp_JamoSetup(cv->jamodisplay,cv);
#endif
}

static void CVChangeChar(CharView *cv, int i ) {
    SplineChar *sc;
    SplineFont *sf = cv->sc->parent;

    if ( i<0 || i>=sf->charcnt )
return;
    if ( (sc = sf->chars[i])==NULL )
	sc = SFMakeChar(sf,i);

    if ( sc==NULL || cv->sc == sc )
return;
    CVChangeSC(cv,sc);
}

static void CVClear(GWindow,GMenuItem *mi, GEvent *);
static void CVMouseMove(CharView *cv, GEvent *event );
static void CVHScroll(CharView *cv,struct sbevent *sb);
static void CVVScroll(CharView *cv,struct sbevent *sb);
static void CVElide(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void CVMerge(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void CVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void CVMenuSimplifyMore(GWindow gw,struct gmenuitem *mi,GEvent *e);


void CVChar(CharView *cv, GEvent *event ) {

#if MyMemory
    if ( event->u.chr.keysym == GK_F2 ) {
	fprintf( stderr, "Malloc debug on\n" );
	__malloc_debug(5);
    } else if ( event->u.chr.keysym == GK_F3 ) {
	fprintf( stderr, "Malloc debug off\n" );
	__malloc_debug(0);
    }
#endif

    CVPaletteActivate(cv);
    CVToolsSetCursor(cv,TrueCharState(event));
    if (( event->u.chr.keysym=='M' ||event->u.chr.keysym=='m' ) &&
	    (event->u.chr.state&ksm_control) ) {
	if ( (event->u.chr.state&ksm_meta) && (event->u.chr.state&ksm_shift))
	    CVMenuSimplifyMore(cv->gw,NULL,NULL);
	else if ( (event->u.chr.state&ksm_shift))
	    CVMenuSimplify(cv->gw,NULL,NULL);
	else if ( (event->u.chr.state&ksm_meta) && (event->u.chr.state&ksm_control))
	    CVElide(cv->gw,NULL,NULL);
	else
	    CVMerge(cv->gw,NULL,NULL);
    } else if ( event->u.chr.keysym == GK_Shift_L || event->u.chr.keysym == GK_Shift_R ) {
	GEvent e;
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
	CVMouseMove(cv,&e);
    } else if ( !(event->u.chr.state&(ksm_control|ksm_meta)) &&
	    event->u.chr.keysym == GK_BackSpace ) {
	/* Menu does delete */
	CVClear(cv->gw,NULL,NULL);
    } else if ( event->u.chr.keysym == GK_Escape ) {
	if ( CVClearSel(cv))
	    SCUpdateAll(cv->sc);
    } else if ( event->u.chr.keysym == GK_Help ) {
	MenuHelp(NULL,NULL,NULL);	/* Menu does F1 */
    } else if ( event->u.chr.keysym == GK_Left ||
	    event->u.chr.keysym == GK_Up ||
	    event->u.chr.keysym == GK_Right ||
	    event->u.chr.keysym == GK_Down ||
	    event->u.chr.keysym == GK_KP_Left ||
	    event->u.chr.keysym == GK_KP_Up ||
	    event->u.chr.keysym == GK_KP_Right ||
	    event->u.chr.keysym == GK_KP_Down ) {
	real dx=0, dy=0;
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
	if ( event->u.chr.state & (ksm_meta|ksm_control) ) {
	    struct sbevent sb;
	    sb.type = dy>0 || dx<0 ? et_sb_halfup : et_sb_halfdown;
	    if ( dx==0 )
		CVVScroll(cv,&sb);
	    else
		CVHScroll(cv,&sb);
	} else if ( CVAnySel(cv,NULL,NULL,NULL)) {
	    extern float arrowAmount;
	    CVPreserveState(cv);
	    CVMoveSelection(cv,dx*arrowAmount,dy*arrowAmount);
	    /* Check for merge!!!! */
	    CVCharChangedUpdate(cv);
	    CVInfoDraw(cv,cv->gw);
	}
    } else if ( !(event->u.chr.state&(ksm_control|ksm_meta)) &&
	    event->type == et_char &&
	    event->u.chr.chars[0]>=' ' && event->u.chr.chars[1]=='\0' ) {
	SplineFont *sf = cv->sc->parent;
	int i;
	for ( i=0; i<sf->charcnt; ++i )
	    if ( sf->chars[i]!=NULL )
		if ( sf->chars[i]->unicodeenc==event->u.chr.chars[0] )
		    CVChangeChar(cv,i);
    }
}

static void CVInfoDrawText(CharView *cv, GWindow pixmap ) {
    GRect r;
    Color bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap));
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

    if ( !cv->info_within )
return;

    if ( cv->info.x>=1000 || cv->info.x<=-1000 || cv->info.y>=1000 || cv->info.y<=-1000 )
	sprintf(buffer,"%d%s%d", (int) cv->info.x, localeinfo.thousands_sep, (int) cv->info.y );
    else
	sprintf(buffer,"%.4g%s%.4g", cv->info.x, localeinfo.thousands_sep, cv->info.y );
    buffer[11] = '\0';
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,RPT_DATA,ybase,ubuffer,-1,NULL,0);
    if ( cv->scale>=.25 )
	sprintf( buffer, "%d%%", (int) (100*cv->scale));
    else
	sprintf( buffer, "%.3g%%", (100*cv->scale));
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,MAG_DATA,ybase,ubuffer,-1,NULL,0);
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
	    sprintf(buffer,"%d%s%d", (int) selx, localeinfo.thousands_sep, (int) sely );
	else
	    sprintf(buffer,"%.4g%s%.4g", selx, localeinfo.thousands_sep, sely );
	buffer[11] = '\0';
	uc_strcpy(ubuffer,buffer);
	GDrawDrawText(pixmap,SPT_DATA,ybase,ubuffer,-1,NULL,0);
    } else if ( cv->p.width ) {
	xdiff = cv->info.x-cv->p.cx;
	ydiff = 0;
    } else if ( cv->p.rubberbanding ) {
	xdiff=cv->info.x-cv->p.cx;
	ydiff = cv->info.y-cv->p.cy;
    } else
return;

    if ( xdiff>=1000 || xdiff<=-1000 || ydiff>=1000 || ydiff<=-1000 )
	sprintf(buffer,"%d%s%d", (int) xdiff, localeinfo.thousands_sep, (int) ydiff );
    else
	sprintf(buffer,"%.4g%s%.4g", xdiff, localeinfo.thousands_sep, ydiff );
    buffer[11] = '\0';
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,SOF_DATA,ybase,ubuffer,-1,NULL,0);

    sprintf( buffer, "%.1f", sqrt(xdiff*xdiff+ydiff*ydiff));
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,SDS_DATA,ybase,ubuffer,-1,NULL,0);

    sprintf( buffer, "%d\260", (int) rint(180*atan2(ydiff,xdiff)/3.1415926535897932));
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,SAN_DATA,ybase,ubuffer,-1,NULL,0);
}

static void CVInfoDrawRulers(CharView *cv, GWindow pixmap ) {
    int rstart = cv->mbh+cv->infoh;
    GDrawSetXORMode(pixmap);
    GDrawSetXORBase(pixmap,GDrawGetDefaultBackground(NULL));
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
    CVToolsSetCursor(cv,event->u.mouse.state);
    cv->info_within = event->u.crossing.entered;
    cv->info.x = (event->u.crossing.x-cv->xoff)/cv->scale;
    cv->info.y = (cv->height-event->u.crossing.y-cv->yoff)/cv->scale;
    CVInfoDraw(cv,cv->gw);
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
    if ( sp->selected && fs->select_controls ) {
	if ( fs->xl<=sp->nextcp.x && fs->xh>=sp->nextcp.x &&
		fs->yl<=sp->nextcp.y && fs->yh >= sp->nextcp.y ) {
	    fs->p->sp = sp;
	    fs->p->spline = NULL;
	    fs->p->spl = spl;
	    fs->p->nextcp = true;
	    fs->p->anysel = true;
	    fs->p->cp = sp->nextcp;
return( true );
	} else if ( fs->xl<=sp->prevcp.x && fs->xh>=sp->prevcp.x &&
		fs->yl<=sp->prevcp.y && fs->yh >= sp->prevcp.y ) {
	    fs->p->sp = sp;
	    fs->p->spline = NULL;
	    fs->p->spl = spl;
	    fs->p->prevcp = true;
	    fs->p->anysel = true;
	    fs->p->cp = sp->prevcp;
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
	if ( !(event->u.mouse.state&ksm_meta) &&
		ItalicConstrained && cv->sc->parent->italicangle!=0 ) {
	    cv->p.cx -= tan(cv->sc->parent->italicangle*3.1415926535897932/180)*
		    (cv->p.cy-base->me.y);
	    cv->p.x = fake->u.mouse.x = cv->xoff + rint(cv->p.cx*cv->scale);
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
    if ( cv->drawmode!=dm_grid && *cv->heads[dm_grid]!=NULL ) {
	PressedOn temp;
	int oldseek = fs->seek_controls;
	temp = *p;
	fs->p = &temp;
	fs->seek_controls = false;
	if ( InSplineSet( fs, *cv->heads[dm_grid])) {
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
	    !cv->p.width)
	p->cx = cv->sc->width;
    else if ( cv->p.width && p->cx>cv->p.cx-fs->fudge && p->cx<cv->p.cx+fs->fudge )
	p->cx = cv->p.cx;		/* cx contains the old width */
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

static void CVMouseDown(CharView *cv, GEvent *event ) {
    FindSel fs;
    PressedOn temp;
    GEvent fake;

    if ( cv->expandedge != ee_none )
	GDrawSetCursor(cv->v,ct_mypointer);
    if ( event->u.mouse.button==3 ) {
	CVToolsPopup(cv,event);
return;
    }
    CVToolsSetCursor(cv,event->u.mouse.state|(1<<(7+event->u.mouse.button)) );
    cv->active_tool = cv->showing_tool;
    cv->needsrasterize = false;
    cv->recentchange = false;

    SetFS(&fs,&cv->p,cv,event);
    if ( event->u.mouse.state&ksm_shift )
	event = CVConstrainedMouseDown(cv,event,&fake);

    if ( cv->active_tool == cvt_pointer ) {
	fs.select_controls = true;
	if ( event->u.mouse.state&ksm_meta ) fs.seek_controls = true;
	cv->lastselpt = NULL;
	if ( !InSplineSet(&fs,*cv->heads[cv->drawmode])) {
	    if ( cv->drawmode==dm_fore) {
		RefChar *rf;
		temp = cv->p;
		fs.p = &temp;
		fs.seek_controls = false;
		for ( rf=cv->sc->refs; rf!=NULL; rf = rf->next ) {
		    if ( InSplineSet(&fs,rf->splines)) {
			cv->p.ref = rf;
			cv->p.anysel = true;
		break;
		    }
		}
	    } else if ( cv->drawmode==dm_back ) {
		ImageList *img;
		for ( img = cv->sc->backimages; img!=NULL; img=img->next ) {
		    if ( InImage(&fs,img)) {
			cv->p.img = img;
			cv->p.anysel = true;
		break;
		    }
		}
	    }
	}
	fs.p = &cv->p;
    } else if ( cv->active_tool == cvt_curve || cv->active_tool == cvt_corner ||
	    cv->active_tool == cvt_tangent || cv->active_tool == cvt_pen ) {
	InSplineSet(&fs,*cv->heads[cv->drawmode]);
	if ( fs.p->sp==NULL && fs.p->spline==NULL )
	    CVDoSnaps(cv,&fs);
    } else {
	NearSplineSetPoints(&fs,*cv->heads[cv->drawmode]);
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
      case cvt_curve: case cvt_corner: case cvt_tangent: case cvt_pen:
	CVMouseDownPoint(cv);
      break;
      case cvt_ruler:
	CVMouseDownRuler(cv,event);
      break;
      case cvt_rotate: case cvt_flip: case cvt_scale: case cvt_skew:
	CVMouseDownTransform(cv);
      break;
      case cvt_knife:
	CVMouseDownKnife(cv);
      break;
      case cvt_rect: case cvt_elipse: case cvt_poly: case cvt_star:
	CVMouseDownShape(cv);
      break;
    }
}

void CVSetCharChanged(CharView *cv,int changed) {
    if ( cv->drawmode==dm_grid ) {
	if ( changed ) {
	    cv->fv->sf->changed = true;
	    if ( cv->fv->cidmaster!=NULL )
		cv->fv->cidmaster->changed = true;
#if 0
	    SFFigureGrid(cv->fv->sf);
#endif
	}
    } else {
	if ( cv->drawmode==dm_fore )
	    cv->sc->parent->onlybitmaps = false;
	if ( cv->sc->changed != changed ) {
	    cv->sc->changed = changed;
	    FVToggleCharChanged(cv->fv,cv->sc);
	    if ( changed ) {
		cv->sc->parent->changed = true;
		if ( cv->fv->cidmaster!=NULL )
		    cv->fv->cidmaster->changed = true;
	    }
	}
	if ( changed ) {
	    cv->sc->changed_since_autosave = true;
	    cv->sc->parent->changed_since_autosave = true;
	    if ( cv->fv->cidmaster!=NULL )
		cv->fv->cidmaster->changed_since_autosave = true;
	}
	if ( cv->drawmode==dm_fore ) {
	    if ( changed )
		cv->sc->changedsincelasthinted = true;
	    cv->needsrasterize = true;
	}
    }
    cv->recentchange = true;
}

void SCClearSelPt(SplineChar *sc) {
    CharView *cv;

    for ( cv=sc->views; cv!=NULL; cv=cv->next ) {
	cv->lastselpt = cv->p.sp = NULL;
    }
}

void SCCharChangedUpdate(SplineChar *sc) {
    FontView *fv = sc->parent->fv;
    sc->changed_since_autosave = true;
    if ( !sc->changed && !sc->parent->onlybitmaps ) {
	sc->changed = true;
	FVToggleCharChanged(fv,sc);
    }
    sc->changedsincelasthinted = true;
    fv->sf->changed = true;
    fv->sf->changed_since_autosave = true;
    if ( fv->cidmaster!=NULL )
	fv->cidmaster->changed = fv->cidmaster->changed_since_autosave = true;
    SCRegenDependents(sc);		/* All chars linked to this one need to get the new splines */
    SCUpdateAll(sc);
    SCRegenFills(sc);
    FVRegenChar(fv,sc);
}

void CVCharChangedUpdate(CharView *cv) {
    CVSetCharChanged(cv,true);
    if ( cv->needsrasterize ) {
	SCRegenDependents(cv->sc);		/* All chars linked to this one need to get the new splines */
	SCUpdateAll(cv->sc);
	SCRegenFills(cv->sc);
	FVRegenChar(cv->fv,cv->sc);
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

static void CVMouseMove(CharView *cv, GEvent *event ) {
    real cx, cy;
    PressedOn p;
    FindSel fs;
    GEvent fake;

    if ( !cv->p.pressed ) {
	CVUpdateInfo(cv, event);
	if ( cv->showing_tool == cvt_pointer )
	    CVCheckResizeCursors(cv);
return;
    }

    SetFS(&fs,&p,cv,event);
    if ( (event->u.mouse.state&ksm_shift) && !cv->p.rubberbanding ) {
	/* Constrained */

	fake.u.mouse = event->u.mouse;
	if ( ((event->u.mouse.state&ksm_meta) ||
		    (!cv->cntrldown && (event->u.mouse.state&ksm_control))) &&
		(cv->p.nextcp || cv->p.prevcp)) {
	    real dot = (cv->p.cp.x-cv->p.constrain.x)*(p.cx-cv->p.constrain.x) +
		    (cv->p.cp.y-cv->p.constrain.y)*(p.cy-cv->p.constrain.y);
	    real len = (cv->p.cp.x-cv->p.constrain.x)*(cv->p.cp.x-cv->p.constrain.x)+
		    (cv->p.cp.y-cv->p.constrain.y)*(cv->p.cp.y-cv->p.constrain.y);
	    dot /= len;
	    /* constrain control point to same angle with respect to base point*/
	    if ( dot<0 ) dot = 0;
	    p.cx = cv->p.constrain.x + dot*(cv->p.cp.x-cv->p.constrain.x);
	    p.cy = cv->p.constrain.y + dot*(cv->p.cp.y-cv->p.constrain.y);
	    p.x = fake.u.mouse.x = cv->xoff + rint(p.cx*cv->scale);
	    p.y = fake.u.mouse.y = -cv->yoff + cv->height - rint(p.cy*cv->scale);
	} else {
	    /* Constrain mouse to hor/vert/45 from base point */
	    int basex =  cv->xoff + rint(cv->p.constrain.x*cv->scale);
	    int basey = -cv->yoff + cv->height - rint(cv->p.constrain.y*cv->scale);
	    int dx= event->u.mouse.x-basex, dy = event->u.mouse.y-basey;
	    int sign = dx*dy<0?-1:1;

	    if ( dx<0 ) dx = -dx; if ( dy<0 ) dy = -dy;
	    if ( dy >= 2*dx ) {
		p.x = fake.u.mouse.x = basex;
		p.cx = cv->p.constrain.x;
		if ( ItalicConstrained && cv->sc->parent->italicangle!=0 ) {
		    p.cx -= tan(cv->sc->parent->italicangle*3.1415926535897932/180)*
			    (p.cy-cv->p.constrain.y);
		    p.x = fake.u.mouse.x = cv->xoff + rint(p.cx*cv->scale);
		}
	    } else if ( dx >= 2*dy ) {
		p.y = fake.u.mouse.y = basey;
		p.cy = cv->p.constrain.y;
	    } else if ( dx > dy ) {
		p.x = fake.u.mouse.x = basex + sign * (event->u.mouse.y-basey);
		p.cx = cv->p.constrain.x - sign * (p.cy-cv->p.constrain.y);
	    } else {
		p.y = fake.u.mouse.y = basey + sign * (event->u.mouse.x-basex);
		p.cy = cv->p.constrain.y - sign * (p.cx-cv->p.constrain.x);
	    }
	}
	event = &fake;
    }

    /* If we've changed the character (recentchange is true) we want to */
    /*  snap to the original location, otherwise we'll keep snapping to the */
    /*  current point as it moves across the screen (jerkily) */
    if ( !cv->joinvalid || !CheckPoint(&fs,&cv->joinpos,NULL)) {
	SplinePointList *spl;
	spl = *cv->heads[cv->drawmode];
	if ( cv->recentchange && cv->active_tool==cvt_pointer &&
		((*cv->uheads[cv->drawmode])->undotype==ut_state ||
		 (*cv->uheads[cv->drawmode])->undotype==ut_tstate ))
	    spl = (*cv->uheads[cv->drawmode])->u.state.splines;
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
    cv->e.x = event->u.mouse.x; cv->e.y = event->u.mouse.y;
    CVInfoDraw(cv,cv->gw);

    switch ( cv->active_tool ) {
      case cvt_pointer:
	CVMouseMovePointer(cv);
      break;
      case cvt_magnify: case cvt_minify:
      break;
      case cvt_curve: case cvt_corner: case cvt_tangent: 
	CVMouseMovePoint(cv,&p);
      break;
      case cvt_pen:
	CVMouseMovePen(cv,&p);
      break;
      case cvt_ruler:
	CVMouseMoveRuler(cv,event);
      break;
      case cvt_rotate: case cvt_flip: case cvt_scale: case cvt_skew:
	CVMouseMoveTransform(cv);
      break;
      case cvt_knife:
	CVMouseMoveKnife(cv,&p);
      break;
      case cvt_rect: case cvt_elipse: case cvt_poly: case cvt_star:
	CVMouseMoveShape(cv);
      break;
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

    if ( cv->p.rubberbanding ) {
	CVDrawRubberRect(cv->v,cv);
	cv->p.rubberbanding = false;
    }
    switch ( cv->active_tool ) {
      case cvt_pointer:
	CVMouseUpPointer(cv);
      break;
      case cvt_ruler:
	CVMouseUpRuler(cv,event);
      break;
      case cvt_curve: case cvt_corner: case cvt_tangent: case cvt_pen:
	CVMouseUpPoint(cv);
      break;
      case cvt_magnify: case cvt_minify: {
	real cx, cy;
	cx = (event->u.mouse.x-cv->xoff)/cv->scale ;
	cy = (cv->height-event->u.mouse.y-cv->yoff)/cv->scale ;
	CVMagnify(cv,cx,cy,event->u.mouse.state&ksm_meta?-1:1);
      } break;
      case cvt_rotate: case cvt_flip: case cvt_scale: case cvt_skew:
	CVMouseUpTransform(cv);
      break;
      case cvt_knife:
	CVMouseUpKnife(cv);
      break;
      case cvt_rect: case cvt_elipse: case cvt_poly: case cvt_star:
	CVMouseUpShape(cv);
      break;
    }
    cv->active_tool = cvt_none;
    CVToolsSetCursor(cv,event->u.mouse.state&~(1<<(7+event->u.mouse.button)));		/* X still has the buttons set in the state, even though we just released them. I don't want em */
    cv->p.pressed = false;
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
		GScrollBarSetPos(cv->vsb,cv->yoff);
	    if ( dx!=0 )
		GScrollBarSetPos(cv->hsb,cv->xoff);
	    GDrawRequestExpose(cv->v,NULL,false);
	}
    }
}

static int v_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	CVExpose(cv,gw,event);
      break;
      case et_crossing:
	CVCrossing(cv,event);
      break;
      case et_mousedown:
	CVPaletteActivate(cv);
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
	CVChar(cv,event);
      break;
      case et_charup:
	CVToolsSetCursor(cv,TrueCharState(event));
      break;
      case et_timer:
	CVTimer(cv,event);
      break;
      case et_focus:
#if 0
	if ( event->u.focus.gained_focus )
	    CVPaletteActivate(cv);
#endif
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
    GDrawDrawText(pixmap,x,y,ubuf,-1,NULL,0x000000);
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
	GDrawDrawText(pixmap,x,y,upt,1,NULL,0x000000);
	y += cv->sfh;
    }
}


static void CVExposeRulers(CharView *cv, GWindow pixmap ) {
    real xmin, xmax, ymin, ymax;
    real onehundred, pos;
    int units, littleunits;
    int ybase = cv->mbh+cv->infoh;
    int x,y;
    GRect rect;

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
    } else {
	units = 10000; littleunits=2000;
    }

    rect.x = 0; rect.width = cv->width+cv->rulerh; rect.y = ybase; rect.height = cv->rulerh;
    GDrawFillRect(pixmap,&rect,GDrawGetDefaultBackground(NULL));
    rect.y = ybase; rect.height = cv->height+cv->rulerh; rect.x = 0; rect.width = cv->rulerh;
    GDrawFillRect(pixmap,&rect,GDrawGetDefaultBackground(NULL));
    GDrawDrawLine(pixmap,cv->rulerh,cv->mbh+cv->infoh+cv->rulerh-1,8096,cv->mbh+cv->infoh+cv->rulerh-1,0x000000);
    GDrawDrawLine(pixmap,cv->rulerh-1,cv->mbh+cv->infoh+cv->rulerh,cv->rulerh-1,8096,0x000000);

    GDrawSetFont(pixmap,cv->small);
    if ( xmax-xmin<1 && cv->width>100 ) {
	CVDrawNum(cv,pixmap,cv->rulerh,ybase+cv->sas,"%.3f",xmin,0);
	CVDrawNum(cv,pixmap,cv->rulerh+cv->width,ybase+cv->sas,"%.3f",xmax,2);
    }
    if ( ymax-ymin<1 && cv->height>100 ) {
	CVDrawVNum(cv,pixmap,1,ybase+cv->rulerh+cv->sas,"%.3f",ymin,0);
	CVDrawVNum(cv,pixmap,1,ybase+cv->rulerh+cv->height+cv->sas,"%.3f",ymax,2);
    }
    if ( littleunits!=0 ) {
	for ( pos=littleunits*ceil(xmin/littleunits); pos<xmax; pos += littleunits ) {
	    x = cv->xoff + rint(pos*cv->scale);
	    GDrawDrawLine(pixmap,x+cv->rulerh,ybase+cv->rulerh-4,x+cv->rulerh,ybase+cv->rulerh, 0x000000);
	}
	for ( pos=littleunits*ceil(ymin/littleunits); pos<ymax; pos += littleunits ) {
	    y = -cv->yoff + cv->height - rint(pos*cv->scale);
	    GDrawDrawLine(pixmap,cv->rulerh-4,ybase+cv->rulerh+y,cv->rulerh,ybase+cv->rulerh+y, 0x000000);
	}
    }
    for ( pos=units*ceil(xmin/units); pos<xmax; pos += units ) {
	x = cv->xoff + rint(pos*cv->scale);
	GDrawDrawLine(pixmap,x+cv->rulerh,ybase+cv->rulerh-6,x+cv->rulerh,ybase+cv->rulerh, 0x000000);
	CVDrawNum(cv,pixmap,x+cv->rulerh,ybase+cv->sas,"%g",pos,1);
    }
    for ( pos=units*ceil(ymin/units); pos<ymax; pos += units ) {
	y = -cv->yoff + cv->height - rint(pos*cv->scale);
	GDrawDrawLine(pixmap,cv->rulerh-6,ybase+cv->rulerh+y,cv->rulerh,ybase+cv->rulerh+y, 0x000000);
	CVDrawVNum(cv,pixmap,1,y+ybase+cv->rulerh+cv->sas,"%g",pos,1);
    }
}

static void InfoExpose(CharView *cv, GWindow pixmap, GEvent *expose) {
    GRect old1, old2, r;

    if ( expose->u.expose.rect.y + expose->u.expose.rect.height < cv->mbh ||
	    (!cv->showrulers && expose->u.expose.rect.y >= cv->mbh+cv->infoh ))
return;

    GDrawPushClip(pixmap,&expose->u.expose.rect,&old1);
    if ( expose->u.expose.rect.y< cv->mbh+cv->infoh ) {
	r.x = 0; r.width = 8096;
	r.y = cv->mbh; r.height = cv->infoh;
	GDrawPushClip(pixmap,&expose->u.expose.rect,&old2);
    
	GDrawDrawLine(pixmap,0,cv->mbh+cv->infoh-1,8096,cv->mbh+cv->infoh-1,0);
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

static void CVResize(CharView *cv, GEvent *event ) {
    int sbsize = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
    int newwidth = event->u.resize.size.width-sbsize,
	newheight = event->u.resize.size.height-sbsize - cv->mbh-cv->infoh;
    int sbwidth = newwidth, sbheight = newheight;

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
    GGadgetMove(cv->hsb,0,event->u.resize.size.height-sbsize);
    GGadgetResize(cv->hsb,sbwidth,sbsize);
    cv->width = newwidth; cv->height = newheight;
    CVFit(cv);
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
    if ( newpos>8000*cv->scale-cv->width )
        newpos = 8000*cv->scale-cv->width;
    if ( newpos<-8000*cv->scale ) newpos = -8000*cv->scale;
    if ( newpos!=cv->xoff ) {
	int diff = newpos-cv->xoff;
	cv->xoff = newpos;
	cv->back_img_out_of_date = true;
	GScrollBarSetPos(cv->hsb,-newpos);
	GDrawScroll(cv->v,NULL,diff,0);
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
        newpos = sb->pos;
      break;
      case et_sb_halfup:
        newpos -= cv->height/30;
      break;
      case et_sb_halfdown:
        newpos += cv->height/30;
      break;
    }
    if ( newpos>8000*cv->scale-cv->height )
        newpos = 8000*cv->scale-cv->height;
    if ( newpos<-8000*cv->scale ) newpos = -8000*cv->scale;
    if ( newpos!=cv->yoff ) {
	int diff = newpos-cv->yoff;
	cv->yoff = newpos;
	cv->back_img_out_of_date = true;
	GScrollBarSetPos(cv->vsb,newpos);
	GDrawScroll(cv->v,NULL,0,diff);
	if ( cv->showrulers ) {
	    GRect r;
	    r.x = 0; r.width = cv->rulerh; r.y = cv->infoh+cv->mbh; r.height = cv->rulerh+cv->height;
	    GDrawRequestExpose(cv->gw,&r,false);
	}
    }
}

static int cv_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	InfoExpose(cv,gw,event);
      break;
      case et_char: case et_charup:
	CVChar(cv,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    CVResize(cv,event);
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
      break;
      case et_mousemove:
	if ( event->u.mouse.y>cv->mbh )
	    SCPreparePopup(cv->gw,cv->sc);
      break;
      case et_focus:
#if 0
	if ( event->u.focus.gained_focus )
	    CVPaletteActivate(cv);
#endif
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
#define MID_NextPt	2010
#define MID_PrevPt	2011
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_DisplayCompositions	2014
#define MID_MarkExtrema	2015
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
#define MID_MakeFirst	2218
#define MID_Average	2219
#define MID_SpacePts	2220
#define MID_SpaceRegion	2221
#define MID_MakeParallel	2222
#define MID_ShowDependents	2223
#define MID_AddExtrema	2224
#define MID_CleanupChar	2225
#define MID_Corner	2301
#define MID_Tangent	2302
#define MID_Curve	2303
#define MID_AutoHint	2400
#define MID_ClearHStem	2401
#define MID_ClearVStem	2402
#define MID_ClearDStem	2403
#define MID_AddHHint	2404
#define MID_AddVHint	2405
#define MID_AddDHint	2406
#define MID_ReviewHints	2407
#define MID_CreateHHint	2408
#define MID_CreateVHint	2409
#define MID_MinimumDistance	2410
#define MID_ClearAllMD		2451
#define MID_ClearSelMDX		2452
#define MID_ClearSelMDY		2453
#define MID_AddxMD		2454
#define MID_AddyMD		2455
#define MID_RoundX		2456
#define MID_NoRoundX		2457
#define MID_RoundY		2458
#define MID_NoRoundY		2459
#define MID_ClearWidthMD	2460
#define MID_Tools	2501
#define MID_Layers	2502
#define MID_Center	2600
#define MID_SetWidth	2601
#define MID_SetLBearing	2602
#define MID_SetRBearing	2603
#define MID_Thirds	2604
#define MID_RemoveKerns	2605
#define MID_SetVWidth	2606
#define MID_OpenBitmap	2700
#define MID_Revert	2702
#define MID_Recent	2703

static void CVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GDrawDestroyWindow(gw);
}

static void CVMenuOpenBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( cv->fv->sf->bitmaps==NULL )
return;
    BitmapViewCreatePick(cv->sc->enc,cv->fv);
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
    _FVMenuGenerate(cv->fv);
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

static void CVMenuPrint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    PrintDlg(NULL,cv->sc,NULL);
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_OpenBitmap:
	    mi->ti.disabled = cv->sc->parent->bitmaps==NULL;
	  break;
	  case MID_Revert:
	    mi->ti.disabled = cv->fv->sf->origname==NULL;
	  break;
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
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
    FindProblems(NULL,cv);
}

static void CVMenuMetaFont(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    MetaFont(NULL,cv);
}

static void CVMenuScale(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( mi->mid == MID_Fit ) {
	CVFit(cv);
    } else {
	BasePoint c;
	c.x = (cv->width/2-cv->xoff)/cv->scale;
	c.y = (cv->height/2-cv->yoff)/cv->scale;
	if ( CVAnySel(cv,NULL,NULL,NULL))
	    CVFindCenter(cv,&c,false);
	CVMagnify(cv,c.x,c.y,mi->mid==MID_ZoomOut?-1:1);
    }
}

static void CVMenuShowHide(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showpoints = cv->showpoints = !cv->showpoints;
    GMenuBarSetItemName(cv->mb,MID_HidePoints,
	    GStringGetResource(CVShows.showpoints?_STR_Hidepoints:_STR_Showpoints,NULL));
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuMarkExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.markextrema = cv->markextrema = !cv->markextrema;
    SavePrefs();
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuShowHideRulers(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    GRect pos;

    CVShows.showrulers = cv->showrulers = !cv->showrulers;
    GMenuBarSetItemName(cv->mb,MID_HideRulers,
	    GStringGetResource(CVShows.showrulers?_STR_Hiderulers:_STR_Showrulers,NULL));
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

static void CVMenuFill(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showfilled = cv->showfilled = !cv->showfilled;
    CVRegenFill(cv);
    GDrawRequestExpose(cv->v,NULL,false);
    GMenuBarSetItemChecked(cv->mb,MID_Fill,CVShows.showfilled);
}

static void CVMenuChangeChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineFont *sf = cv->sc->parent;
    int pos = -1;

    if ( mi->mid == MID_Next ) {
	pos = cv->sc->enc+1;
    } else if ( mi->mid == MID_Prev ) {
	pos = cv->sc->enc-1;
    } else if ( mi->mid == MID_NextDef ) {
	for ( pos = cv->sc->enc+1; pos<sf->charcnt && sf->chars[pos]==NULL; ++pos );
	if ( pos>=sf->charcnt ) {
	    if ( cv->sc->enc<0xa140 && sf->encoding_name==em_big5 )
		pos = 0xa140;
	    else if ( cv->sc->enc<0x8431 && sf->encoding_name==em_johab )
		pos = 0x8431;
	    else if ( cv->sc->enc<0xa1a1 && sf->encoding_name==em_wansung )
		pos = 0xa1a1;
	    else if ( cv->sc->enc<0x8100 && sf->encoding_name==em_sjis )
		pos = 0x8100;
	    else if ( cv->sc->enc<0xb000 && sf->encoding_name==em_sjis )
		pos = 0xb000;
	    if ( pos>=sf->charcnt )
return;
	}
    } else if ( mi->mid == MID_PrevDef ) {
	for ( pos = cv->sc->enc-1; pos>=0 && sf->chars[pos]==NULL; --pos );
	if ( pos<0 )
return;
    }
    if ( pos<0 ) pos = cv->sc->parent->charcnt-1;
    else if ( pos>= cv->sc->parent->charcnt ) pos = 0;
    if ( pos>=0 && pos<cv->sc->parent->charcnt )
	CVChangeChar(cv,pos);
}

static void CVMenuNextPrevPt(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *selpt, *other;
    RefChar *r; ImageList *il;
    SplineSet *spl, *ss;
    int x, y;

    if ( !CVOneThingSel(cv,&selpt,&spl,&r,&il))
return;
    other = selpt;
    if ( mi->mid == MID_NextPt ) {
	if ( other->next!=NULL && other->next->to!=spl->first )
	    other = other->next->to;
	else {
	    if ( spl->next == NULL )
		spl = *cv->heads[cv->drawmode];
	    else
		spl = spl->next;
	    other = spl->first;
	}
    } else {
	if ( other!=spl->first ) {
	    other = other->prev->from;
	} else {
	    if ( spl==*cv->heads[cv->drawmode] ) {
		for ( ss = *cv->heads[cv->drawmode]; ss->next!=NULL; ss=ss->next );
	    } else {
		for ( ss = *cv->heads[cv->drawmode]; ss->next!=spl; ss=ss->next );
	    }
	    spl = ss;
	    other = ss->last;
	    if ( spl->last==spl->first && spl->last->prev!=NULL )
		other = other->prev->from;
	}
    }
    selpt->selected = false;
    other->selected = true;
    cv->p.sp = NULL;
    cv->lastselpt = other;

    /* Make sure the point is visible and has some context around it */
    x =  cv->xoff + rint(other->me.x*cv->scale);
    y = -cv->yoff + cv->height - rint(other->me.y*cv->scale);
    if ( x<40 || y<40 || x>cv->width-40 || y>cv->height-40 )
	CVMagnify(cv,other->me.x,other->me.y,0);

    CVInfoDraw(cv,cv->gw);
    SCUpdateAll(cv->sc);
}

int GotoChar(SplineFont *sf) {
    int pos = -1;
    unichar_t *ret, *end;
    int i;

    ret = GWidgetAskStringR(_STR_Goto,NULL,_STR_Enternameofchar);
    if ( ret==NULL )
return(-1);
    for ( i=0; i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL && uc_strcmp(ret,sf->chars[i]->name)==0 ) {
	    pos = i;
    break;
	}
    if ( pos==-1 ) {
	int temp=-1;
	if ( (((*ret=='U' || *ret=='u') && ret[1]=='+') ||
		     (*ret=='0' && (ret[1]=='x' || ret[1]=='X'))) ) {
	    temp = u_strtol(ret+2,&end,16);
	    if ( *end!='\0' )
		temp = -1;
	} else if ( ret[0]=='u' && ret[1]=='n' && ret[2]=='i' ) {
	    temp = u_strtol(ret+3,&end,16);
	    if ( *end!='\0' )
		temp = -1;
	} else {
	    for ( temp=psunicodenames_cnt-1; temp>=0 ; --temp )
		if ( psunicodenames[temp]!=NULL )
		    if ( uc_strmatch(ret,psunicodenames[temp])== 0 )
	    break;
	    if ( temp<0 ) {
		for ( temp=65535; temp>=0 ; --temp )
		    if ( UnicodeCharacterNames[temp>>8][temp&0xff]!=NULL )
			if ( u_strmatch(ret,UnicodeCharacterNames[temp>>8][temp&0xff])== 0 )
		break;
	    }
	}
	if ( temp!=-1 ) {
	    for ( i=0; i<sf->charcnt; ++i )
		if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc==temp ) {
		    pos = i;
	    break;
		}
	    if ( pos==-1 && temp<sf->charcnt &&
		    ( sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4 ||
			    *ret=='0'))
		pos = temp;
	}
    }
    if ( pos==-1 ) {
	pos = u_strtol(ret,&end,10);
	if ( *end==',' && ((sf->encoding_name>=em_jis208 && sf->encoding_name<=em_last94x94) ||
		sf->encoding_name == em_unicode )) {
	    int j = u_strtol(end+1,&end,10);
	    /* kuten */
	    if ( *end!='\0' )
		pos = -1;
	    else if ( sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4 ) {
		if ( pos>=0 && pos<256 && j>=0 && j<256 )
		    pos = (pos<<8) |j;
		else
		    pos = -1;
	    } else {
		if ( pos>=1 && pos<=94 && j>=1 && j<=94 )
		    pos = (pos-1)*96 + j;
		else
		    pos = -1;
	    }
	} else if ( *end!='\0' )
	    pos = -1;
    }
    if ( pos<0 || pos>=sf->charcnt )
	pos = -1;
    if ( pos==-1 ) {
	unichar_t ubuf[100];
	u_strcpy( ubuf, GStringGetResource(_STR_Couldntfindchar,NULL));
	u_strncat(ubuf,ret,70);
	GWidgetPostNotice(GStringGetResource(_STR_Goto,NULL),ubuf);
    }
return( pos );
}

static void CVMenuGotoChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int pos = GotoChar(cv->sc->parent);

    if ( pos!=-1 )
	CVChangeChar(cv,pos);
}

static void CVMenuPaletteShow(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVPaletteSetVisible(cv, mi->mid==MID_Tools, !CVPaletteIsVisible(cv, mi->mid==MID_Tools));
}

static void pllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Tools:
	    mi->ti.checked = CVPaletteIsVisible(cv,1);
	  break;
	  case MID_Layers:
	    mi->ti.checked = CVPaletteIsVisible(cv,0);
	  break;
	}
    }
}

static void CVUndo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVDoUndo(cv);
}

static void CVRedo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVDoRedo(cv);
}

static void CVCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int desel = false;
    if ( !CVAnySel(cv,NULL,NULL,NULL))
	if ( !(desel = CVSetSel(cv)))
return;
    CopySelected(cv);
    if ( desel )
	CVClearSel(cv);
}

static void CVCopyRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CopyReference(cv->sc);
}

static void CVCopyWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CopyWidth(cv);
}

static void CVDoClear(CharView *cv) {

    CVPreserveState(cv);
    if ( cv->drawmode==dm_fore )
	SCRemoveSelectedMinimumDistances(cv->sc,2);
    *cv->heads[cv->drawmode] = SplinePointListRemoveSelected(cv->sc,
	    *cv->heads[cv->drawmode]);
    if ( cv->drawmode==dm_fore ) {
	RefChar *refs, *next;
	for ( refs=cv->sc->refs; refs!=NULL; refs = next ) {
	    next = refs->next;
	    if ( refs->selected )
		SCRemoveDependent(cv->sc,refs);
	}
    }
    if ( cv->drawmode==dm_back ) {
	ImageList *prev, *imgs, *next;
	for ( prev = NULL, imgs=cv->sc->backimages; imgs!=NULL; imgs = next ) {
	    next = imgs->next;
	    if ( !imgs->selected )
		prev = imgs;
	    else {
		if ( prev==NULL )
		    cv->sc->backimages = next;
		else
		    prev->next = next;
		chunkfree(imgs,sizeof(ImageList));
		/* garbage collection of images????!!!! */
	    }
	}
    }
    if ( cv->lastselpt!=NULL || cv->p.sp!=NULL ) {
	cv->lastselpt = NULL; cv->p.sp = NULL;
	CVInfoDraw(cv,cv->gw);
    }
}

static void CVClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( !CVAnySel(cv,NULL,NULL,NULL))
return;
    CVDoClear(cv);
    CVCharChangedUpdate(cv);
}

static void CVPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVPreserveState(cv);
    CVClearSel(cv);
    PasteToCV(cv);
    CVCharChangedUpdate(cv);
}

static void CVMerge(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anyp = 0;

    if ( !CVAnySel(cv,&anyp,NULL,NULL) || !anyp)
return;
    CVPreserveState(cv);
    SplineCharMerge(cv->sc,cv->heads[cv->drawmode],true);
    SCClearSelPt(cv->sc);
    CVCharChangedUpdate(cv);
}

static void CVElide(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anyp = 0;

    if ( !CVAnySel(cv,&anyp,NULL,NULL) || !anyp)
return;
    CVPreserveState(cv);
    SplineCharMerge(cv->sc,cv->heads[cv->drawmode],false);
    SCClearSelPt(cv->sc);
    CVCharChangedUpdate(cv);
}

static void CVCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( !CVAnySel(cv,NULL,NULL,NULL))
return;
    CVCopy(gw,mi,NULL);
    CVDoClear(cv);
    CVCharChangedUpdate(cv);
}

static void CVCopyFgBg(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->splines==NULL )
return;
    SCCopyFgToBg(cv->sc,true);
}

static void CVSelectAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( CVSetSel(cv))
	SCUpdateAll(cv->sc);
}

static void CVUnlinkRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anyrefs=0;
    RefChar *rf, *next;

    if ( cv->drawmode==dm_fore && cv->sc->refs!=NULL ) {
	CVPreserveState(cv);
	for ( rf=cv->sc->refs; rf!=NULL && !anyrefs; rf=rf->next )
	    if ( rf->selected ) anyrefs = true;
	for ( rf=cv->sc->refs; rf!=NULL ; rf=next ) {
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

static void CVRemoveUndoes(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    UndoesFree(*cv->uheads[cv->drawmode]);
    UndoesFree(*cv->rheads[cv->drawmode]);
    *cv->uheads[cv->drawmode] = *cv->rheads[cv->drawmode] = NULL;
}

/* We can only paste if there's something in the copy buffer */
/* we can only copy if there's something selected to copy */
/* figure out what things are possible from the edit menu before the user */
/*  pulls it down */
static void edlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anypoints, anyrefs, anyimages;

    CVAnySel(cv,&anypoints,&anyrefs,&anyimages);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Merge:
	    mi->ti.disabled = !anypoints;
	    free(mi->ti.text);
	    if ( e==NULL || !(e->u.mouse.state&ksm_shift) ) {
		mi->ti.text = u_copy(GStringGetResource(_STR_Merge,NULL));
		mi->short_mask = ksm_control;
		mi->invoke = CVMerge;
	    } else {
		mi->ti.text = u_copy(GStringGetResource(_STR_Elide,NULL));
		mi->short_mask = (ksm_control|ksm_meta);
		mi->invoke = CVElide;
	    }
	  break;
	  case MID_Cut: /*case MID_Copy:*/ case MID_Clear:
	    /* If nothing is selected, copy copies everything */
	    mi->ti.disabled = !anypoints && !anyrefs && !anyimages;
	  break;
	  case MID_CopyFgToBg:
	    mi->ti.disabled = cv->sc->splines==NULL;
	  break;
	  case MID_Paste:
	    mi->ti.disabled = !CopyContainsSomething();
	  break;
	  case MID_Undo:
	    mi->ti.disabled = *cv->uheads[cv->drawmode]==NULL;
	  break;
	  case MID_Redo:
	    mi->ti.disabled = *cv->rheads[cv->drawmode]==NULL;
	  break;
	  case MID_RemoveUndoes:
	    mi->ti.disabled = *cv->rheads[cv->drawmode]==NULL && *cv->uheads[cv->drawmode]==NULL;
	  break;
	  case MID_CopyRef:
	    mi->ti.disabled = cv->drawmode!=dm_fore;
	  break;
	  case MID_UnlinkRef:
	    mi->ti.disabled = cv->drawmode!=dm_fore || cv->sc->refs==NULL;
	  break;
	}
    }
}

static void SPChangePointType(SplinePoint *sp, int pointtype) {

    if ( sp->pointtype==pointtype )
return;
    sp->pointtype = pointtype;
    
    if ( pointtype==pt_corner ) {
	/* Leave control points as they are */;
	sp->nextcpdef = sp->nonextcp;
	sp->prevcpdef = sp->noprevcp;
    } else if ( pointtype==pt_tangent ) {
	SplineCharTangentPrevCP(sp);
	SplineCharTangentNextCP(sp);
    } else {
	sp->nextcpdef = sp->prevcpdef = true;
	SplineCharDefaultPrevCP(sp,sp->prev==NULL?NULL:sp->prev->from);
	SplineCharDefaultNextCP(sp,sp->next==NULL?NULL:sp->next->to);
    }
}

static void CVMenuPointType(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int pointtype = mi->mid==MID_Corner?pt_corner:mi->mid==MID_Tangent?pt_tangent:pt_curve;
    SplinePointList *spl;
    Spline *spline, *first;

    CVPreserveState(cv);	/* We should only get here if there's a selection */
    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL ; spl = spl->next ) {
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

static void ptlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int type = -2, cnt=0;
    SplinePointList *spl, *sel=NULL;
    Spline *spline, *first;
    SplinePoint *selpt=NULL;

    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL && type!=-1; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) {
	    sel = spl;
	    selpt = spl->first; ++cnt;
	    if ( type==-2 ) type = spl->first->pointtype;
	    else if ( type!=spl->first->pointtype ) type = -1;
	}
	for ( spline=spl->first->next; spline!=NULL && spline!=first && type!=-1; spline = spline->to->next ) {
	    if ( spline->to->selected ) {
		if ( type==-2 ) type = spline->to->pointtype;
		else if ( type!=spline->to->pointtype ) type = -1;
		selpt = spline->to;
		sel = spl; ++cnt;
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
	}
    }
}

static void CVMenuDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int splinepoints, dir;
    SplinePointList *spl;
    Spline *spline, *first;
    int needsrefresh = false;

    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
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

static int getorigin(void *d,BasePoint *base,int index) {
    CharView *cv = (CharView *) d;

    base->x = base->y = 0;
    switch ( index ) {
      case 0:		/* Character origin */
	/* all done */
      break;
      case 1:		/* Center of selection */
	CVFindCenter(cv,base,!CVAnySel(cv,NULL,NULL,NULL));
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

void CVTransFunc(CharView *cv,real transform[6], int doback) {
    int anysel = cv->p.transany;
    RefChar *refs;
    ImageList *img;
    real t[6];

    SplinePointListTransform(*cv->heads[cv->drawmode],transform,!anysel);
    if ( cv->drawmode==dm_back ) {
	for ( img = cv->sc->backimages; img!=NULL; img=img->next )
	    if ( img->selected || !anysel ) {
		BackgroundImageTransform(cv->sc, img, transform);
	    }
    } else if ( cv->drawmode==dm_fore ) {
	for ( refs = cv->sc->refs; refs!=NULL; refs=refs->next )
	    if ( refs->selected || !anysel ) {
		SplinePointListTransform(refs->splines,transform,true);
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
		memcpy(refs->transform,t,sizeof(t));
		SplineSetFindBounds(refs->splines,&refs->bb);
	    }
	if ( transform[0]==1 && transform[3]==1 && transform[1]==0 &&
		transform[2]==0 && transform[5]==0 &&
		transform[4]!=0 && CVAllSelected(cv) &&
		cv->sc->unicodeenc!=-1 && isalpha(cv->sc->unicodeenc)) {
	    SCUndoSetLBearingChange(cv->sc,(int) rint(transform[4]));
	    SCSynchronizeLBearing(cv->sc,NULL,transform[4]);
	}
	if ( doback && !anysel ) {
	    SCPreserveBackground(cv->sc);
	    for ( img = cv->sc->backimages; img!=NULL; img=img->next )
		BackgroundImageTransform(cv->sc, img, transform);
	    SplinePointListTransform(*cv->heads[dm_back],transform,true);
	}
    }
}

static void transfunc(void *d,real transform[6],int otype,BVTFunc *bvts,
	int dobackground) {
    CharView *cv = (CharView *) d;

    cv->p.transany = CVAnySel(cv,NULL,NULL,NULL);
    CVPreserveState(cv);
    CVTransFunc(cv,transform,dobackground);
    CVCharChangedUpdate(cv);
}

static void CVMenuTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anysel = CVAnySel(cv,NULL,NULL,NULL);
    TransformDlgCreate(cv,transfunc,getorigin,!anysel && cv->drawmode==dm_fore);
}

static void SplinePointRound(SplinePoint *sp) {
    sp->me.x = rint(sp->me.x);
    sp->me.y = rint(sp->me.y);
    sp->nextcp.x = rint(sp->nextcp.x);
    sp->nextcp.y = rint(sp->nextcp.y);
    sp->prevcp.x = rint(sp->prevcp.x);
    sp->prevcp.y = rint(sp->prevcp.y);
}

void SCRound2Int(SplineChar *sc,FontView *fv) {
    SplinePointList *spl;
    SplinePoint *sp;
    RefChar *r;

    SCPreserveState(sc,false);
    for ( spl= sc->splines; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    SplinePointRound(sp);
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
    for ( r=sc->refs; r!=NULL; r=r->next ) {
	r->transform[4] = rint(r->transform[4]);
	r->transform[5] = rint(r->transform[5]);
    }
    SCCharChangedUpdate(sc);
}

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

static void CVMenuRound2Int(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anysel = CVAnySel(cv,NULL,NULL,NULL);
    SplinePointList *spl;
    SplinePoint *sp;
    RefChar *r;

    CVPreserveState(cv);
    for ( spl= *cv->heads[cv->drawmode]; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( sp->selected || !anysel )
		SplinePointRound(sp);
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
	for ( r=cv->sc->refs; r!=NULL; r=r->next ) {
	    if ( r->selected || !anysel ) {
		r->transform[4] = rint(r->transform[4]);
		r->transform[5] = rint(r->transform[5]);
	    }
	}
    }
    CVCharChangedUpdate(cv);
}

static void CVMenuStroke(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVStroke(cv);
}

static void CVMenuOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    DoAutoSaves();

    CVPreserveState(cv);
    if ( cv->drawmode==dm_fore ) {
	MinimumDistancesFree(cv->sc->md);
	cv->sc->md = NULL;
    }
    *cv->heads[cv->drawmode] = SplineSetRemoveOverlap(*cv->heads[cv->drawmode]);
    CVCharChangedUpdate(cv);
}

static void CVMenuAddExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anysel;

    (void) CVAnySel(cv,&anysel,NULL,NULL);
    CVPreserveState(cv);
    SplineCharAddExtrema(*cv->heads[cv->drawmode],anysel);
    CVCharChangedUpdate(cv);
}

static void CVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVPreserveState(cv);
    *cv->heads[cv->drawmode] = SplineCharSimplify(cv->sc,*cv->heads[cv->drawmode],false);
    CVCharChangedUpdate(cv);
}

static void CVMenuSimplifyMore(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVPreserveState(cv);
    *cv->heads[cv->drawmode] = SplineCharSimplify(cv->sc,*cv->heads[cv->drawmode],true);
    CVCharChangedUpdate(cv);
}

static void CVMenuCleanupChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVPreserveState(cv);
    *cv->heads[cv->drawmode] = SplineCharSimplify(cv->sc,*cv->heads[cv->drawmode],-1);
    CVCharChangedUpdate(cv);
}

static void CVMenuMakeFirst(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *selpt;
    int anypoints = 0, splinepoints;
    SplinePointList *spl, *sel;
    Spline *spline, *first;

    sel = NULL;
    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
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

static void CVMenuAutotrace(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVAutoTrace(cv,e!=NULL && (e->u.mouse.state&ksm_shift));
}

static void CVMenuBuildAccent(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int onlyaccents = e==NULL || !(e->u.mouse.state&ksm_shift);
    extern int onlycopydisplayed;

    if ( SFIsRotatable(cv->fv->sf,cv->sc))
	/* It's ok */;
    else if ( !SFIsCompositBuildable(cv->fv->sf,cv->sc->unicodeenc) ||
	    (onlyaccents && !hascomposing(cv->fv->sf,cv->sc->unicodeenc)))
return;
    SCBuildComposit(cv->fv->sf,cv->sc,!onlycopydisplayed,cv->fv);
}

static void CVMenuCorrectDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVPreserveState(cv);
    *cv->heads[cv->drawmode] = SplineSetsCorrect(*cv->heads[cv->drawmode]);
    CVCharChangedUpdate(cv);
}

static void CVMenuGetInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVGetInfo(cv);
}

static void CVMenuShowDependents(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCRefBy(cv->sc);
}

static void CVMenuBitmaps(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    BitmapDlg(cv->fv,cv->sc,mi->mid==MID_AvailBitmaps );
}

static void allistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int selpoints = 0;
    SplinePointList *spl;
    SplinePoint *sp=NULL;

    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
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

static void ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anypoints = 0, splinepoints, dir = -2;
    SplinePointList *spl, *sel;
    SplinePoint *selpt=NULL;
    Spline *spline, *first;
    int onlyaccents;

    sel = NULL;
    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
	first = NULL;
	splinepoints = 0;
	if ( spl->first->selected ) { splinepoints = 1; sel = spl; selpt = spl->first;}
	for ( spline=spl->first->next; spline!=NULL && spline!=first && !splinepoints; spline = spline->to->next ) {
	    if ( spline->to->selected ) { ++splinepoints; sel = spl; selpt = spline->to; }
	    if ( first == NULL ) first = spline;
	}
	if ( splinepoints ) {
	    anypoints += splinepoints;
	    if ( dir==-1 )
		/* Do nothing */;
	    else if ( spl->first!=spl->last || spl->first->next==NULL )
		dir = -1;	/* Not a closed path, no direction */
	    else if ( dir==-2 )
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
	    if ( cv->fv->cidmaster==NULL )
		mi->ti.disabled = false;
	    else {
		SplinePoint *sp; SplineSet *spl; RefChar *ref; ImageList *img;
		mi->ti.disabled = !CVOneThingSel(cv,&sp,&spl,&ref,&img);
	    }
	  break;
	  case MID_ShowDependents:
	    mi->ti.disabled = cv->sc->dependents==NULL;
	  break;
	  case MID_Clockwise:
	    mi->ti.disabled = !anypoints;
	    mi->ti.checked = dir==1;
	  break;
	  case MID_Counter:
	    mi->ti.disabled = !anypoints;
	    mi->ti.checked = dir==0;
	  break;
	  case MID_MetaFont:
	    mi->ti.disabled = cv->drawmode!=dm_fore || cv->sc->refs!=NULL;
	  break;
	  case MID_Stroke: case MID_RmOverlap:
	    mi->ti.disabled = ( *cv->heads[cv->drawmode]==NULL );
	  break;
	  case MID_RegenBitmaps:
	    mi->ti.disabled = cv->fv->sf->bitmaps==NULL;
	  break;
	  case MID_AddExtrema:
	    mi->ti.disabled = *cv->heads[cv->drawmode]==NULL;
	  /* Like Simplify, always available, but may not do anything if */
	  /*  all extrema have points. I'm not going to check for that, too hard */
	  break;
	  case MID_CleanupChar:
	    mi->ti.disabled = *cv->heads[cv->drawmode]==NULL;
	  break;
	  case MID_Simplify:
	    mi->ti.disabled = *cv->heads[cv->drawmode]==NULL;
	  /* Simplify is always available (it may not do anything though) */
	  /*  well, ok. Disable it if there is absolutely nothing to work on */
	    free(mi->ti.text);
	    if ( e==NULL || !(e->u.mouse.state&ksm_shift) ) {
		mi->ti.text = u_copy(GStringGetResource(_STR_Simplify,NULL));
		mi->short_mask = ksm_control|ksm_shift;
		mi->invoke = CVMenuSimplify;
	    } else {
		mi->ti.text = u_copy(GStringGetResource(_STR_SimplifyMore,NULL));
		mi->short_mask = (ksm_control|ksm_meta|ksm_shift);
		mi->invoke = CVMenuSimplifyMore;
	    }
	  break;
	  case MID_BuildAccent:
	    mi->ti.disabled = !SFIsSomethingBuildable(cv->fv->sf,cv->sc);
	    onlyaccents = e==NULL || !(e->u.mouse.state&ksm_shift);
	    if ( onlyaccents ) {
		if ( SFIsRotatable(cv->fv->sf,cv->sc))
		    /* It's ok */;
		else if ( !hascomposing(cv->fv->sf,cv->sc->unicodeenc))
		    mi->ti.disabled = true;
	    }
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(onlyaccents?_STR_Buildaccent:_STR_Buildcomposit,NULL));
	  break;
	  case MID_Autotrace:
	    mi->ti.disabled = FindAutoTraceName()==NULL || cv->sc->backimages==NULL;
	  break;
	}
    }
}

static void CVMenuAutoHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int removeOverlap = e==NULL || !(e->u.mouse.state&ksm_shift);

    /* !!!! Hint undoes???? */
    cv->sc->manualhints = false;
    SplineCharAutoHint(cv->sc,removeOverlap);
    SCUpdateAll(cv->sc);
}

static void CVMenuClearHints(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    MinimumDistance *md, *prev, *next;

    if ( mi->mid==MID_ClearHStem ) {
	StemInfosFree(cv->sc->hstem);
	cv->sc->hstem = NULL;
	cv->sc->hconflicts = false;
    } else if ( mi->mid==MID_ClearVStem ) {
	StemInfosFree(cv->sc->vstem);
	cv->sc->vstem = NULL;
	cv->sc->vconflicts = false;
    } else if ( mi->mid==MID_ClearDStem ) {
	DStemInfosFree(cv->sc->dstem);
	cv->sc->dstem = NULL;
    } else if ( mi->mid==MID_ClearAllMD ) {
	MinimumDistancesFree(cv->sc->md);
	cv->sc->md = NULL;
	SCClearRounds(cv->sc);
    } else if ( mi->mid==MID_ClearWidthMD ) {
	prev=NULL;
	for ( md=cv->sc->md; md!=NULL; md=next ) {
	    next = md->next;
	    if ( md->sp2==NULL ) {
		if ( prev==NULL )
		    cv->sc->md = next;
		else
		    prev->next = next;
		chunkfree(md,sizeof(MinimumDistance));
	    } else
		prev = md;
	}
    } else {
	SCRemoveSelectedMinimumDistances(cv->sc,mi->mid==MID_ClearSelMDX);
    }
    cv->sc->manualhints = true;
    SCOutOfDateBackground(cv->sc);
    SCUpdateAll(cv->sc);
}

static void CVMenuAddHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp1, *sp2, *sp3, *sp4;
    StemInfo *h;
    DStemInfo *d;

    if ( !CVTwoForePointsSelected(cv,&sp1,&sp2))
return;

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
    SCOutOfDateBackground(cv->sc);
    SCUpdateAll(cv->sc);
}

static void CVMenuCreateHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVCreateHint(cv,mi->mid==MID_CreateHHint);
}

static void CVMenuReviewHints(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->hstem==NULL && cv->sc->vstem==NULL )
return;
    CVReviewHints(cv);
}

static void CVMenuRoundHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineSet *ss;
    SplinePoint *sp;

    for ( ss=cv->sc->splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->selected ) {
		if ( mi->mid==MID_RoundX )
		    sp->roundx = true;
		else if ( mi->mid==MID_NoRoundX )
		    sp->roundx = false;
		else if ( mi->mid==MID_RoundY )
		    sp->roundy = true;
		else
		    sp->roundy = false;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
}

static void CVMenuAddMD(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineSet *ss;
    SplinePoint *sp, *sel1=NULL, *sel2=NULL;

    for ( ss=cv->sc->splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->selected ) {
		if ( sel1==NULL )
		    sel1 = sp;
		else if ( sel2==NULL )
		    sel2 = sp;
		else
return;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    if ( sel1==NULL )
return;
    if ( sel2==NULL && mi->mid==MID_AddyMD )
return;

    if ( sel2!=NULL && sel1==cv->lastselpt ) {
	sel1 = sel2;
	sel2 = cv->lastselpt;
    }
    MDAdd(cv->sc,mi->mid==MID_AddxMD,sel1,sel2);
    SCOutOfDateBackground(cv->sc);
    SCUpdateAll(cv->sc);
}

static void mdlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp, *sp1, *sp2;
    SplineSet *ss;
    int allrx=-1, allry=-1, cnt=0;

    sp1 = sp2 = NULL;
    for ( ss=cv->sc->splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->selected ) {
		++cnt;
		if ( sp1==NULL )
		    sp1 = sp;
		else if ( sp2==NULL )
		    sp2 = sp;
		if ( allrx==-1 )
		    allrx = sp->roundx;
		else if ( allrx!=sp->roundx )
		    allrx = -2;
		if ( allry==-1 )
		    allry = sp->roundy;
		else if ( allry!=sp->roundy )
		    allry = -2;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_ClearAllMD: case MID_ClearWidthMD:
	    mi->ti.disabled = cv->sc->md==NULL;
	  break;
	  case MID_ClearSelMDX: case MID_ClearSelMDY:
	    mi->ti.disabled = cv->sc->md==NULL || cnt==0;
	  break;
	  case MID_AddyMD:
	    mi->ti.disabled = cnt!=2 || sp2->me.y==sp1->me.y;
	  break;
	  case MID_AddxMD:
	    mi->ti.disabled = cnt==0 || cnt>2 || (sp2!=NULL && sp2->me.x==sp1->me.x);
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(cnt==1?_STR_AddMD2Width: _STR_AddxMD,NULL));
	  break;
	  case MID_RoundX:
	    mi->ti.disabled = cnt==0;
	    mi->ti.checked = allrx==1;
	  break;
	  case MID_NoRoundX:
	    mi->ti.disabled = cnt==0;
	    mi->ti.checked = allrx==0;
	  break;
	  case MID_RoundY:
	    mi->ti.disabled = cnt==0;
	    mi->ti.checked = allry==1;
	  break;
	  case MID_NoRoundY:
	    mi->ti.disabled = cnt==0;
	    mi->ti.checked = allry==0;
	  break;
	}
    }
}

static void htlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp1, *sp2, *sp3, *sp4;
    int removeOverlap;

    sp1 = sp2 = NULL;
    CVTwoForePointsSelected(cv,&sp1,&sp2);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AutoHint:
	    removeOverlap = e==NULL || !(e->u.mouse.state&ksm_shift);
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(removeOverlap?_STR_Autohint: _STR_FullAutohint,NULL));
	  break;
	  case MID_AddHHint:
	    mi->ti.disabled = sp2==NULL || sp2->me.y==sp1->me.y;
	  break;
	  case MID_AddVHint:
	    mi->ti.disabled = sp2==NULL || sp2->me.x==sp1->me.x;
	  break;
	  case MID_AddDHint:
	    mi->ti.disabled = !CVIsDiagonalable(sp1,sp2,&sp3,&sp4);
	  break;
	  case MID_ReviewHints:
	    mi->ti.disabled = (cv->sc->hstem==NULL && cv->sc->vstem==NULL );
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
	}
    }
}

static void vwlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp; SplineSet *spl; RefChar *r; ImageList *im;
    int exactlyone = CVOneThingSel(cv,&sp,&spl,&r,&im);
    int pos;
    SplineFont *sf = cv->sc->parent;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_NextPt: case MID_PrevPt:
	    mi->ti.disabled = !exactlyone;
	  break;
	  case MID_NextDef:
	    for ( pos = cv->sc->enc+1; pos<sf->charcnt && sf->chars[pos]==NULL; ++pos );
	    mi->ti.disabled = pos==sf->charcnt;
	  break;
	  case MID_PrevDef:
	    for ( pos = cv->sc->enc-1; pos>=0 && sf->chars[pos]==NULL; --pos );
	    mi->ti.disabled = pos==-1;
	  break;
	  case MID_MarkExtrema:
	    mi->ti.checked = cv->markextrema;
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
	CVTransFunc(cv,transform,false);
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

static GMenuItem dummyitem[] = { { (unichar_t *) _STR_New, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) _STR_New, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, 'N', ksm_control, NULL, NULL, MenuNew },
    { { (unichar_t *) _STR_Open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) _STR_Close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, CVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Openoutline, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'u' }, 'H', ksm_control, NULL, NULL, /* No function, never avail */NULL },
    { { (unichar_t *) _STR_Openbitmap, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'B' }, 'J', ksm_control, NULL, NULL, CVMenuOpenBitmap, MID_OpenBitmap },
    { { (unichar_t *) _STR_Openmetrics, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'M' }, 'K', ksm_control, NULL, NULL, CVMenuOpenMetrics },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, CVMenuSave },
    { { (unichar_t *) _STR_Saveas, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, CVMenuSaveAs },
    { { (unichar_t *) _STR_Generate, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'G' }, 'G', ksm_control|ksm_shift, NULL, NULL, CVMenuGenerate },
    { { (unichar_t *) _STR_Export, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuExport },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Import, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'I' }, 'I', ksm_control|ksm_shift, NULL, NULL, CVMenuImport },
    { { (unichar_t *) _STR_Revertfile, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, CVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Print, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 'P', ksm_control, NULL, NULL, CVMenuPrint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { (unichar_t *) _STR_Undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL, CVUndo, MID_Undo },
    { { (unichar_t *) _STR_Redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL, CVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, CVCut, MID_Cut },
    { { (unichar_t *) _STR_Copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, CVCopy, MID_Copy },
    { { (unichar_t *) _STR_Copyref, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'o' }, 'G', ksm_control, NULL, NULL, CVCopyRef, MID_CopyRef },
    { { (unichar_t *) _STR_Copywidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 'W', ksm_control, NULL, NULL, CVCopyWidth, MID_CopyWidth },
    { { (unichar_t *) _STR_Paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, CVPaste, MID_Paste },
    { { (unichar_t *) _STR_Clear, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'l' }, GK_Delete, 0, NULL, NULL, CVClear, MID_Clear },
    { { (unichar_t *) _STR_Merge, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'M' }, 'M', ksm_control, NULL, NULL, CVMerge, MID_Merge },
    { { (unichar_t *) _STR_CopyFgToBg, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 'C', ksm_control|ksm_shift, NULL, NULL, CVCopyFgBg, MID_CopyFgToBg },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_SelectAll, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, CVSelectAll, MID_SelAll },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Unlinkref, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'U' }, 'U', ksm_control, NULL, NULL, CVUnlinkRef, MID_UnlinkRef },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_RemoveUndoes, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', 0, NULL, NULL, CVRemoveUndoes, MID_RemoveUndoes },
    { NULL }
};

static GMenuItem ptlist[] = {
    { { (unichar_t *) _STR_Curve, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'C' }, '2', ksm_control, NULL, NULL, CVMenuPointType, MID_Curve },
    { { (unichar_t *) _STR_Corner, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'o' }, '3', ksm_control, NULL, NULL, CVMenuPointType, MID_Corner },
    { { (unichar_t *) _STR_Tangent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'T' }, '4', ksm_control, NULL, NULL, CVMenuPointType, MID_Tangent },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_MakeFirst, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'r' }, '1', ksm_control, NULL, NULL, CVMenuMakeFirst, MID_MakeFirst },
    { NULL }
};

static GMenuItem allist[] = {
    { { (unichar_t *) _STR_AveragePts, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, '@', ksm_control|ksm_shift, NULL, NULL, CVMenuConstrain, MID_Average },
    { { (unichar_t *) _STR_SpacePts, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, '#', ksm_control|ksm_shift, NULL, NULL, CVMenuConstrain, MID_SpacePts },
    { { (unichar_t *) _STR_SpaceRegions, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuConstrain, MID_SpaceRegion },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_MakeParallel, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuMakeParallel, MID_MakeParallel },
    { NULL }
};

static GMenuItem ellist[] = {
    { { (unichar_t *) _STR_Fontinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 'F', ksm_control|ksm_shift, NULL, NULL, CVMenuFontInfo },
    { { (unichar_t *) _STR_Getinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'I' }, 'I', ksm_control, NULL, NULL, CVMenuGetInfo, MID_GetInfo },
    { { (unichar_t *) _STR_ShowDependents, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'D' }, 'I', ksm_control|ksm_meta, NULL, NULL, CVMenuShowDependents, MID_ShowDependents },
    { { (unichar_t *) _STR_Findprobs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'o' }, 'E', ksm_control, NULL, NULL, CVMenuFindProblems },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Bitmapsavail, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'B', ksm_control|ksm_shift, NULL, NULL, CVMenuBitmaps, MID_AvailBitmaps },
    { { (unichar_t *) _STR_Regenbitmaps, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'B' }, 'B', ksm_control, NULL, NULL, CVMenuBitmaps, MID_RegenBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Transform, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'T' }, '\\', ksm_control, NULL, NULL, CVMenuTransform },
    { { (unichar_t *) _STR_Stroke, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'E' }, 'E', ksm_control|ksm_shift, NULL, NULL, CVMenuStroke, MID_Stroke },
    { { (unichar_t *) _STR_Rmoverlap, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'v' }, 'O', ksm_control|ksm_shift, NULL, NULL, CVMenuOverlap, MID_RmOverlap },
    { { (unichar_t *) _STR_Simplify, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'M', ksm_control|ksm_shift, NULL, NULL, CVMenuSimplify, MID_Simplify },
    { { (unichar_t *) _STR_CleanupChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'n' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuCleanupChar, MID_CleanupChar },
    { { (unichar_t *) _STR_AddExtrema, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'x' }, 'X', ksm_control|ksm_shift, NULL, NULL, CVMenuAddExtrema, MID_AddExtrema },
    { { (unichar_t *) _STR_MetaFont, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'M' }, '!', ksm_control|ksm_shift, NULL, NULL, CVMenuMetaFont, MID_MetaFont },
    { { (unichar_t *) _STR_Autotrace, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'r' }, 'T', ksm_control|ksm_shift, NULL, NULL, CVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Align, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'l' }, '\0', ksm_control|ksm_shift, allist, allistcheck },
    { { (unichar_t *) _STR_Round2int, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'I' }, '_', ksm_control|ksm_shift, NULL, NULL, CVMenuRound2Int, MID_Round },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Clockwise, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'o' }, '\0', 0, NULL, NULL, CVMenuDir, MID_Clockwise },
    { { (unichar_t *) _STR_Cclockwise, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'n' }, '\0', 0, NULL, NULL, CVMenuDir, MID_Counter },
    { { (unichar_t *) _STR_Correct, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'D' }, 'D', ksm_control|ksm_shift, NULL, NULL, CVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Buildaccent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'u' }, 'A', ksm_control|ksm_shift, NULL, NULL, CVMenuBuildAccent, MID_BuildAccent },
    { NULL }
};

static GMenuItem mdlist[] = {
    { { (unichar_t *) _STR_ClearAllMD, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearAllMD },
    { { (unichar_t *) _STR_ClearSelXMD, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 's' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearSelMDX },
    { { (unichar_t *) _STR_ClearSelYMD, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearSelMDY },
    { { (unichar_t *) _STR_ClearWidthMD, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearWidthMD },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_AddxMD, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'x' }, GK_F5, 0, NULL, NULL, CVMenuAddMD, MID_AddxMD },
    { { (unichar_t *) _STR_AddyMD, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'y' }, GK_F6, 0, NULL, NULL, CVMenuAddMD, MID_AddyMD },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_RoundX, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'r' }, GK_F7, 0, NULL, NULL, CVMenuRoundHint, MID_RoundX },
    { { (unichar_t *) _STR_NoRoundX, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'n' }, GK_F8, 0, NULL, NULL, CVMenuRoundHint, MID_NoRoundX },
    { { (unichar_t *) _STR_RoundY, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'u' }, GK_F9, 0, NULL, NULL, CVMenuRoundHint, MID_RoundY },
    { { (unichar_t *) _STR_NoRoundY, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'o' }, GK_F10, 0, NULL, NULL, CVMenuRoundHint, MID_NoRoundY },
    { NULL }
};

static GMenuItem htlist[] = {
    { { (unichar_t *) _STR_Autohint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 'H', ksm_control|ksm_shift, NULL, NULL, CVMenuAutoHint, MID_AutoHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_MinimumDistance, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 'H', ksm_control|ksm_shift, mdlist, mdlistcheck, NULL, MID_MinimumDistance },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Clearhstem, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearHStem },
    { { (unichar_t *) _STR_Clearvstem, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'V' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearVStem },
    { { (unichar_t *) _STR_Cleardstem, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'D' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearDStem },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Addhhint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, '\0', ksm_control, NULL, NULL, CVMenuAddHint, MID_AddHHint },
    { { (unichar_t *) _STR_Addvhint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 's' }, '\0', ksm_control, NULL, NULL, CVMenuAddHint, MID_AddVHint },
    { { (unichar_t *) _STR_Adddhint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, CVMenuAddHint, MID_AddDHint },
    { { (unichar_t *) _STR_Createhhint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'r' }, '\0', ksm_control, NULL, NULL, CVMenuCreateHint, MID_CreateHHint },
    { { (unichar_t *) _STR_Createvhint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, CVMenuCreateHint, MID_CreateVHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Reviewhints, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, '\0', ksm_control, NULL, NULL, CVMenuReviewHints, MID_ReviewHints },
    { NULL }
};

static GMenuItem mtlist[] = {
    { { (unichar_t *) _STR_Center, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, CVMenuCenter, MID_Center },
    { { (unichar_t *) _STR_Thirds, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, CVMenuCenter, MID_Thirds },
    { { (unichar_t *) _STR_Setwidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 'L', ksm_control|ksm_shift, NULL, NULL, CVMenuSetWidth, MID_SetWidth },
    { { (unichar_t *) _STR_Setlbearing, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'L' }, 'L', ksm_control, NULL, NULL, CVMenuSetWidth, MID_SetLBearing },
    { { (unichar_t *) _STR_Setrbearing, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'R', ksm_control, NULL, NULL, CVMenuSetWidth, MID_SetRBearing },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Removekern, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuRemoveKern, MID_RemoveKerns },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_SetVWidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'V' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuSetWidth, MID_SetVWidth },
    { NULL }
};

static GMenuItem pllist[] = {
    { { (unichar_t *) _STR_Tools, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, CVMenuPaletteShow, MID_Tools },
    { { (unichar_t *) _STR_Layers, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'L' }, '\0', ksm_control, NULL, NULL, CVMenuPaletteShow, MID_Layers },
    { NULL }
};

static GMenuItem vwlist[] = {
    { { (unichar_t *) _STR_Fit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 'F', ksm_control, NULL, NULL, CVMenuScale, MID_Fit },
    { { (unichar_t *) _STR_Zoomout, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'o' }, '-', ksm_control|ksm_meta, NULL, NULL, CVMenuScale, MID_ZoomOut },
    { { (unichar_t *) _STR_Zoomin, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'i' }, '+', ksm_shift|ksm_control|ksm_meta, NULL, NULL, CVMenuScale, MID_ZoomIn },
#if HANYANG
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_DisplayCompositions, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'i' }, '\0', ksm_control, NULL, NULL, CVDisplayCompositions, MID_DisplayCompositions },
#endif
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_NextChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, ']', ksm_control, NULL, NULL, CVMenuChangeChar, MID_Next },
    { { (unichar_t *) _STR_PrevChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, '[', ksm_control, NULL, NULL, CVMenuChangeChar, MID_Prev },
    { { (unichar_t *) _STR_NextDefChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'D' }, ']', ksm_control|ksm_meta, NULL, NULL, CVMenuChangeChar, MID_NextDef },
    { { (unichar_t *) _STR_PrevDefChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, '[', ksm_control|ksm_meta, NULL, NULL, CVMenuChangeChar, MID_PrevDef },
    { { (unichar_t *) _STR_Goto, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'G' }, '>', ksm_shift|ksm_control, NULL, NULL, CVMenuGotoChar },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Hidepoints, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'o' }, 'D', ksm_control, NULL, NULL, CVMenuShowHide, MID_HidePoints },
    { { (unichar_t *) _STR_MarkExtrema, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'M' }, '\0', ksm_control, NULL, NULL, CVMenuMarkExtrema, MID_MarkExtrema },
    { { (unichar_t *) _STR_Nextpoint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '}', ksm_shift|ksm_control, NULL, NULL, CVMenuNextPrevPt, MID_NextPt },
    { { (unichar_t *) _STR_Prevpoint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'v' }, '{', ksm_shift|ksm_control, NULL, NULL, CVMenuNextPrevPt, MID_PrevPt },
    { { (unichar_t *) _STR_Fill, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, 'l' }, '\0', 0, NULL, NULL, CVMenuFill, MID_Fill },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, }},
    { { (unichar_t *) _STR_Palettes, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, '\0', 0, pllist, pllistcheck },
    { { (unichar_t *) _STR_Hiderulers, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, '\0', ksm_control, NULL, NULL, CVMenuShowHideRulers, MID_HideRulers },
    { NULL }
};

static GMenuItem mblist[] = {
    { { (unichar_t *) _STR_File, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) _STR_Edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'E' }, 0, 0, edlist, edlistcheck },
    { { (unichar_t *) _STR_Point, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 0, 0, ptlist, ptlistcheck },
    { { (unichar_t *) _STR_Element, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'l' }, 0, 0, ellist, ellistcheck },
    { { (unichar_t *) _STR_Hints, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 0, 0, htlist, htlistcheck },
    { { (unichar_t *) _STR_View, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'V' }, 0, 0, vwlist, vwlistcheck },
    { { (unichar_t *) _STR_Metric, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'M' }, 0, 0, mtlist, mtlistcheck },
    { { (unichar_t *) _STR_Window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 0, 0, NULL, WindowMenuBuild },
    { { (unichar_t *) _STR_Help, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};

CharView *CharViewCreate(SplineChar *sc, FontView *fv) {
    CharView *cv = gcalloc(1,sizeof(CharView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    GRect gsize;
    int sbsize;
    FontRequest rq;
    int as, ds, ld;
    static unichar_t fixed[] = { 'f','i','x','e','d',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t', '\0' };
    unichar_t ubuf[300];

    cv->sc = sc;
    cv->scale = .5;
    cv->xoff = cv->yoff = 20;
    cv->next = sc->views;
    sc->views = cv;
    cv->fv = fv;

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

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_ititle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.icon_title = CVMakeTitles(cv,ubuf);
    wattrs.window_title = ubuf;
    wattrs.icon = CharIcon(cv, fv);
    if ( wattrs.icon )
	wattrs.mask |= wam_icon;
    pos.x = pos.y = 0; pos.width=pos.height = 540;

    cv->gw = gw = GDrawCreateTopWindow(NULL,&pos,cv_e_h,cv,&wattrs);
    free( (unichar_t *) wattrs.icon_title );

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    cv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(cv->mb,&gsize);
    cv->mbh = gsize.height;
    cv->infoh = 13;
    cv->rulerh = 13;

    gd.pos.y = cv->mbh+cv->infoh;
    gd.pos.width = sbsize = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.height = pos.height-cv->mbh-cv->infoh - sbsize;
    gd.pos.x = pos.width-sbsize;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    cv->vsb = GScrollBarCreate(gw,&gd,cv);

    gd.pos.y = pos.height-sbsize; gd.pos.height = sbsize;
    gd.pos.width = pos.width - sbsize;
    gd.pos.x = 0;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    cv->hsb = GScrollBarCreate(gw,&gd,cv);

    pos.y = cv->mbh+cv->infoh; pos.height -= cv->mbh + sbsize + cv->infoh;
    pos.x = 0; pos.width -= sbsize;
    if ( cv->showrulers ) {
	pos.y += cv->rulerh; pos.height -= cv->rulerh;
	pos.x += cv->rulerh; pos.width -= cv->rulerh;
    }
    wattrs.mask = wam_events|wam_cursor;
    wattrs.event_masks = -1;
    cv->v = GWidgetCreateSubWindow(gw,&pos,v_e_h,cv,&wattrs);

    memset(&rq,0,sizeof(rq));
    rq.family_name = fixed;
    rq.point_size = -7;
    rq.weight = 400;
    cv->small = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(cv->small,&as,&ds,&ld);
    cv->sfh = as+ds; cv->sas = as;

    cv->height = pos.height; cv->width = pos.width;
    cv->gi.u.image = gcalloc(1,sizeof(struct _GImage));
    cv->gi.u.image->image_type = it_mono;
    cv->gi.u.image->clut = gcalloc(1,sizeof(GClut));
    cv->gi.u.image->clut->trans_index = cv->gi.u.image->trans = 0;
    cv->gi.u.image->clut->clut_len = 2;
    cv->gi.u.image->clut->clut[0] = 0xffffff;
    cv->gi.u.image->clut->clut[1] = 0x707070;
    cv->b1_tool = cvt_pointer; cv->cb1_tool = cvt_pointer;
    cv->b2_tool = cvt_magnify; cv->cb2_tool = cvt_ruler;
    cv->showing_tool = cvt_pointer;
    cv->pressed_tool = cv->pressed_display = cv->active_tool = cvt_none;
    cv->heads[dm_fore] = &sc->splines; cv->heads[dm_back] = &sc->backgroundsplines;
    cv->heads[dm_grid] = &fv->sf->gridsplines;
    cv->uheads[dm_fore] = &sc->undoes[dm_fore]; cv->uheads[dm_back] = &sc->undoes[dm_back];
    cv->uheads[dm_grid] = &fv->sf->gundoes;
    cv->rheads[dm_fore] = &sc->redoes[dm_fore]; cv->rheads[dm_back] = &sc->redoes[dm_back];
    cv->rheads[dm_grid] = &fv->sf->gredoes;

    cv->olde.x = -1;

    GMenuBarSetItemName(cv->mb,MID_HidePoints,
	    GStringGetResource(CVShows.showpoints?_STR_Hidepoints:_STR_Showpoints,NULL));
    GMenuBarSetItemName(cv->mb,MID_HideRulers,
	    GStringGetResource(CVShows.showrulers?_STR_Hiderulers:_STR_Showrulers,NULL));
    GMenuBarSetItemChecked(cv->mb,MID_Fill,CVShows.showfilled);

    /*GWidgetHidePalettes();*/
    /*cv->tools = CVMakeTools(cv);*/
    /*cv->layers = CVMakeLayers(cv);*/

    CVFit(cv);
    GDrawSetVisible(cv->v,true);
    GDrawSetVisible(gw,true);
return( cv );
}

void CharViewFree(CharView *cv) {
    BDFCharFree(cv->filled);
    free(cv->gi.u.image->clut);
    free(cv->gi.u.image);
#if HANYANG
    if ( cv->jamodisplay!=NULL )
	Disp_DoFinish(cv->jamodisplay,true);
#endif
    free(cv);
}

int CVValid(SplineFont *sf, SplineChar *sc, CharView *cv) {
    /* A charview may have been closed. A splinechar may have been removed */
    /*  from a font */
    CharView *test;

    if ( cv->sc!=sc || sc->parent!=sf )
return( false );
    if ( sc->enc<0 || sc->enc>sf->charcnt )
return( false );
    if ( sf->chars[sc->enc]!=sc )
return( false );
    for ( test=sc->views; test!=NULL; test=test->next )
	if ( test==cv )
return( true );

return( false );
}
