/* Copyright (C) 2000,2001 by George Williams */
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
#include <math.h>
#include <ustring.h>
#include <gkeysym.h>
#include <utype.h>

extern int _GScrollBar_Width;
struct cvshows CVShows = { 1, 1, 1, 1, 1, 1, 0, 1 };

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

    if ( spline->from->nonextcp && spline->from->noprevcp ) {
	if ( (x[0]<0 && x[1]<0) || (x[0]>=cv->width && x[1]>=cv->width) ||
		(y[0]<0 && y[1]<0) || (y[0]>=cv->height && y[1]>=cv->height) )
return( true );
    } else {
	x[2] =  cv->xoff + rint(spline->from->nextcp.x*cv->scale);
	y[2] = -cv->yoff + cv->height - rint(spline->from->nextcp.y*cv->scale);
	x[3] =  cv->xoff + rint(spline->from->prevcp.x*cv->scale);
	y[3] = -cv->yoff + cv->height - rint(spline->from->prevcp.y*cv->scale);
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
		if (( y<0 || y>=cv->height ) && bothout )
    continue;			/* Not on screen */;
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
		y = (l->next->asend.y-l->asstart.y)*(double)(cv->width-l->asstart.x)/(l->next->asend.x-l->asstart.x) +
			l->asstart.y;
		if (( y<0 || y>=cv->height ) && bothout )
    continue;			/* Not on screen */;
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
	    }
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

    x =  cv->xoff + rint(sp->me.x*cv->scale);
    y = -cv->yoff + cv->height - rint(sp->me.y*cv->scale);
    if ( x<-4000 || y<-4000 || x>cv->width+4000 || y>=cv->height+4000 )
return;

    /* draw the control points if it's selected */
    if ( sp->selected ) {
	if ( !sp->nonextcp ) {
	    cx =  cv->xoff + rint(sp->nextcp.x*cv->scale);
	    cy = -cv->yoff + cv->height - rint(sp->nextcp.y*cv->scale);
	    GDrawDrawLine(pixmap,x,y,cx,cy, 0);
	    GDrawDrawLine(pixmap,cx-2,cy-2,cx+2,cy+2,0x0000ff);
	    GDrawDrawLine(pixmap,cx+2,cy-2,cx-2,cy+2,0x0000ff);
	}
	if ( !sp->noprevcp ) {
	    cx =  cv->xoff + rint(sp->prevcp.x*cv->scale);
	    cy = -cv->yoff + cv->height - rint(sp->prevcp.y*cv->scale);
	    GDrawDrawLine(pixmap,x,y,cx,cy, 0);
	    GDrawDrawLine(pixmap,cx-2,cy-2,cx+2,cy+2,0x0000ff);
	    GDrawDrawLine(pixmap,cx+2,cy-2,cx-2,cy+2,0x0000ff);
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
}

static void DrawLine(CharView *cv, GWindow pixmap,
	double x1, double y1, double x2, double y2, Color fg) {
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

static void CVShowHints(CharView *cv, GWindow pixmap) {
    StemInfo *hint;
    GRect r;
    HintInstance *hi;
    int end;
    Color col;

    GDrawSetDashedLine(pixmap,5,5,0);

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
}

static void DrawImageList(CharView *cv,GWindow pixmap,ImageList *backimages) {
    while ( backimages!=NULL ) {
	struct _GImage *base = backimages->image->list_len==0?
		backimages->image->u.image:backimages->image->u.images[0];

	GDrawDrawImageMagnified(pixmap, backimages->image, NULL,
		(int) (cv->xoff + rint(backimages->xoff * cv->scale)),
		(int) (-cv->yoff + cv->height - rint(backimages->yoff*cv->scale)),
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
    if ( (cv->showhhints || cv->showvhints ) && ( cv->sc->backimages==NULL || !cv->showback) )
	CVShowHints(cv,pixmap);

    if (( cv->showback || cv->drawmode==dm_back ) && cv->sc->backimages!=NULL ) {
	/* This really should be after the grids, but then it would completely*/
	/*  hide them. */
	GRect r;
	if ( cv->backimgs==NULL )
	    cv->backimgs = GDrawCreatePixmap(GDrawGetDisplayOfWindow(cv->v),cv->width,cv->height);
	if ( cv->back_img_out_of_date ) {
	    GDrawFillRect(cv->backimgs,NULL,GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(cv->v)));
	    if ( cv->showhhints || cv->showvhints )
		CVShowHints(cv,cv->backimgs);
	    DrawImageList(cv,cv->backimgs,cv->sc->backimages);
	    cv->back_img_out_of_date = false;
	}
	r.x = r.y = 0; r.width = cv->width; r.height = cv->height;
	GDrawDrawPixmap(pixmap,cv->backimgs,&r,0,0);
    }
    if ( cv->showgrids || cv->drawmode==dm_grid ) {
	DrawLine(cv,pixmap,0,-8096,0,8096,0x808080);
	DrawLine(cv,pixmap,-8096,0,8096,0,0x808080);
	DrawLine(cv,pixmap,-8096,sf->ascent,8096,sf->ascent,0x808080);
	DrawLine(cv,pixmap,-8096,-sf->descent,8096,-sf->descent,0x808080);

	CVDrawSplineSet(cv,pixmap,cv->fv->sf->gridsplines,0x808080,
		cv->showpoints && cv->drawmode==dm_grid,&clip);
    }

    if ( cv->showback || cv->drawmode==dm_back ) {
	/* Used to draw the image list here, but that's too slow. Optimization*/
	/*  is to draw to pixmap, dump pixmap a bit earlier */
	DrawSelImageList(cv,pixmap,cv->sc->backimages);
	CVDrawSplineSet(cv,pixmap,cv->sc->backgroundsplines,0x808080,
		cv->showpoints && cv->drawmode==dm_back,&clip);
    }

    if ( cv->showfore || cv->drawmode==dm_fore ) {
	if ( cv->showfilled ) {
	    GDrawDrawImage(pixmap, &cv->gi, NULL, cv->xoff + cv->filled->xmin,
		    -cv->yoff + cv->height-cv->filled->ymax);
	}

	for ( rf=cv->sc->refs; rf!=NULL; rf = rf->next ) {
	    CVDrawSplineSet(cv,pixmap,rf->splines,0,false,&clip);
	    if ( rf->selected )
		CVDrawBB(cv,pixmap,&rf->bb);
	}

	CVDrawSplineSet(cv,pixmap,cv->sc->splines,0,
		cv->showpoints && cv->drawmode==dm_fore,&clip);
    }

    DrawLine(cv,pixmap,cv->sc->width,-8096,cv->sc->width,8096,0x0);

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
    double left, right, top, bottom, hsc, wsc;

    SplineCharFindBounds(cv->sc,&b);
    bottom = b.miny;
    top = b.maxy;
    left = b.minx;
    right = b.maxx;

    if ( bottom>0 ) bottom = 0;
    if ( left>0 ) left = 0;
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

    bdfc = NULL;
    if ( sc->refs!=NULL || sc->splines!=NULL ) {
	bdfc = fv->show->chars[sc->enc];
	if ( bdfc==NULL || bdfc->byte_data )
	    bdfc = fv->filled->chars[sc->enc];
	if ( bdfc==NULL || bdfc->byte_data ) {
	    bdf2 = NULL; bdfc = NULL;
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL && bdf->pixelsize<24 ; bdf=bdf->next )
		bdf2 = bdf;
	    if ( bdf2!=NULL && bdf!=NULL ) {
		if ( 24-bdf2->pixelsize < bdf->pixelsize-24 )
		    bdf = bdf2;
	    } else if ( bdf==NULL )
		bdf = bdf2;
	    if ( bdf!=NULL )
		bdfc = bdf->chars[sc->enc];
	}
    }

    if (( sc->refs!=NULL || sc->splines!=NULL ) && bdfc!=NULL ) {
	GClut clut;
	struct _GImage base;
	GImage gi;
	/* if not empty, use the font's own shape, otherwise use a standard */
	/*  font */
	memset(&gi,'\0',sizeof(gi));
	memset(&base,'\0',sizeof(base));
	memset(&clut,'\0',sizeof(clut));
	gi.u.image = &base;
	base.image_type = it_mono;
	base.clut = &clut;
	clut.clut_len = 2;
	clut.clut[1] = 0xffffff;
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	GDrawDrawImage(icon,&gi,NULL,(r.width-base.width)/2,(r.height-base.height)/2);
    } else if ( sc->unicodeenc!=-1 ) {
	FontRequest rq;
	GFont *font;
	static unichar_t times[] = { 't', 'i', 'm', 'e', 's', '\0' };
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

void CVChangeSC(CharView *cv, SplineChar *sc ) {
    char buffer[200];
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

    CVNewScale(cv);

    CharIcon(cv,cv->fv);
    sprintf( buffer, "%.90s from %.90s ", sc->name, sc->parent->fontname );
    title = uc_copy(buffer);
    u_strcpy(ubuf,title);
    if ( sc->unicodeenc!=-1 && UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]!=NULL )
	u_strcat(ubuf, UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]);
    GDrawSetWindowTitles(cv->gw,ubuf,title);
    CVInfoDraw(cv,cv->gw);
    free(title);
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

void CVChar(CharView *cv, GEvent *event ) {

    CVPaletteActivate(cv);
    CVToolsSetCursor(cv,TrueCharState(event));
    if ( event->u.chr.keysym == GK_Shift_L || event->u.chr.keysym == GK_Shift_R ) {
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
    } else if ( event->u.chr.keysym == GK_Left ||
	    event->u.chr.keysym == GK_Up ||
	    event->u.chr.keysym == GK_Right ||
	    event->u.chr.keysym == GK_Down ||
	    event->u.chr.keysym == GK_KP_Left ||
	    event->u.chr.keysym == GK_KP_Up ||
	    event->u.chr.keysym == GK_KP_Right ||
	    event->u.chr.keysym == GK_KP_Down ) {
	if ( CVAnySel(cv,NULL,NULL,NULL)) {
	    double dx=0, dy=0;
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
	    CVPreserveState(cv);
	    CVMoveSelection(cv,dx,dy);
	    /* Check for merge!!!! */
	    CVCharChangedUpdate(cv);
	    CVInfoDraw(cv,cv->gw);
	}
    } else if ( !(event->u.chr.state&(ksm_control|ksm_meta)) &&
	    event->type == et_char &&
	    event->u.chr.chars[0]!='\0' && event->u.chr.chars[1]=='\0' ) {
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
    double xdiff, ydiff;
    SplinePoint *sp;

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
	sprintf(buffer,"%d,%d", (int) cv->info.x, (int) cv->info.y );
    else
	sprintf(buffer,"%.4g,%.4g", cv->info.x, cv->info.y );
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
    if ( sp ) {
	double selx, sely;
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
	    sprintf(buffer,"%d,%d", (int) selx, (int) sely );
	else
	    sprintf(buffer,"%.4g,%.4g", selx, sely );
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
	sprintf(buffer,"%d,%d", (int) xdiff, (int) ydiff );
    else
	sprintf(buffer,"%.4g,%.4g", xdiff, ydiff );
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

    memset(p,'\0',sizeof(PressedOn));
    p->pressed = true;

    memset(fs,'\0',sizeof(*fs));
    fs->p = p;
    fs->e = event;
    p->x = event->u.mouse.x;
    p->y = event->u.mouse.y;
    p->cx = (event->u.mouse.x-cv->xoff)/cv->scale;
    p->cy = (cv->height-event->u.mouse.y-cv->yoff)/cv->scale;

    fs->fudge = 3.5/cv->scale;		/* 3.5 pixel fudge */
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
	if ( ItalicConstrained && cv->sc->parent->italicangle!=0 ) {
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
    } else {
	NearSplineSetPoints(&fs,*cv->heads[cv->drawmode]);
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

#if 0
void SCClearOrig(SplineChar *sc) {
    free(sc->origtype1);
    sc->origtype1 = NULL;
    sc->origlen = 0;
}
#endif

void CVSetCharChanged(CharView *cv,int changed) {
    if ( cv->drawmode==dm_grid ) {
	if ( changed )
	    cv->fv->sf->changed = true;
    } else {
	if ( cv->drawmode==dm_fore )
	    cv->sc->parent->onlybitmaps = false;
#if 0
	if ( cv->drawmode==dm_fore && cv->sc->origtype1!=NULL )
	    SCClearOrig(cv->sc);
#endif
	if ( cv->sc->changed != changed ) {
	    cv->sc->changed = changed;
	    FVToggleCharChanged(cv->fv,cv->sc);
	    if ( changed ) {
		cv->sc->parent->changed = true;
	    }
	}
	if ( changed ) {
	    cv->sc->changed_since_autosave = true;
	    cv->sc->parent->changed_since_autosave = true;
	}
	if ( cv->drawmode==dm_fore ) {
	    if ( changed )
		cv->sc->changedsincelasthinted = true;
	    cv->needsrasterize = true;
	}
    }
    cv->recentchange = true;
}

void SCCharChangedUpdate(SplineChar *sc,FontView *fv) {
    if ( !sc->changed ) {
	sc->changed = true;
	sc->changed_since_autosave = true;
	FVToggleCharChanged(fv,sc);
    }
#if 0
    if ( sc->origtype1!=NULL )
	SCClearOrig(sc);
#endif
    fv->sf->changed = true;
    fv->sf->changed_since_autosave = true;
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
    double cx, cy;
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
	if ( (event->u.mouse.state&ksm_meta) && (cv->p.nextcp || cv->p.prevcp)) {
	    double dot = (cv->p.cp.x-cv->p.constrain.x)*(p.cx-cv->p.constrain.x) +
		    (cv->p.cp.y-cv->p.constrain.y)*(p.cy-cv->p.constrain.y);
	    double len = (cv->p.cp.x-cv->p.constrain.x)*(cv->p.cp.x-cv->p.constrain.x)+
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
    { SplinePointList *spl;
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
	if ( p.cx>-fs.fudge && p.cx<fs.fudge )
	    p.cx = 0;
	else if ( p.cx>cv->sc->width-fs.fudge && p.cx<cv->sc->width+fs.fudge &&
		!cv->p.width)
	    p.cx = cv->sc->width;
	else if ( cv->p.width && p.cx>cv->p.cx-fs.fudge && p.cx<cv->p.cx+fs.fudge )
	    p.cx = cv->p.cx;		/* cx contains the old width */
	if ( p.cy>-fs.fudge && p.cy<fs.fudge )
	    p.cy = 0;
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

static void CVMagnify(CharView *cv, double midx, double midy, int bigger) {
    static float scales[] = { 1, 2, 3, 4, 6, 8, 11, 16, 23, 32, 45, 64, 90, 128, 181, 256, 512, 1024, 0 };
    int i, j;

    if ( cv->scale>=1 ) {
	for ( i=0; scales[i]!=0 && cv->scale>scales[i]; ++i );
	if ( scales[i]==0 ) i=j= i-1;
	else if ( scales[i]==cv->scale ) j=i;
	else j = i-1;
    } else { double sinv = 1/cv->scale; int t;
	for ( i=0; scales[i]!=0 && sinv>scales[i]; ++i );
	if ( scales[i]==0 ) i=j= i-1;
	else if ( scales[i]==sinv ) j=i;
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
	double cx, cy;
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
	    double dx = 0, dy = 0;
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

static void CVDrawNum(CharView *cv,GWindow pixmap,int x, int y, char *format,double val, int align) {
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

static void CVDrawVNum(CharView *cv,GWindow pixmap,int x, int y, char *format,double val, int align) {
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
    double xmin, xmax, ymin, ymax;
    double onehundred, pos;
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
        newpos -= 9*cv->width/10;
      break;
      case et_sb_up:
        newpos -= cv->width/15;
      break;
      case et_sb_down:
        newpos += cv->width/15;
      break;
      case et_sb_downpage:
        newpos += 9*cv->width/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
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
      case et_char:
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
#define MID_Corner	2301
#define MID_Tangent	2302
#define MID_Curve	2303
#define MID_AutoHint	2400
#define MID_ClearHStem	2401
#define MID_ClearVStem	2402
#define MID_AddHHint	2403
#define MID_AddVHint	2404
#define MID_ReviewHints	2405
#define MID_CreateHHint	2406
#define MID_CreateVHint	2407
#define MID_Tools	2501
#define MID_Layers	2502
#define MID_Center	2600
#define MID_SetWidth	2601
#define MID_SetLBearing	2602
#define MID_SetRBearing	2603
#define MID_Thirds	2604
#define MID_RemoveKerns	2605
#define MID_OpenBitmap	2700
#define MID_Revert	2702
#define MID_Recent	2703

static unichar_t file[] = { 'F', 'i', 'l', 'e', '\0' };
static unichar_t edit[] = { 'E', 'd', 'i', 't', '\0' };
static unichar_t point[] = { 'P', 'o', 'i', 'n', 't', '\0' };
static unichar_t element[] = { 'E', 'l', 'e', 'm', 'e', 'n', 't', '\0' };
static unichar_t hints[] = { 'H', 'i', 'n', 't', 's',  '\0' };
static unichar_t view[] = { 'V', 'i', 'e', 'w', '\0' };
static unichar_t window[] = { 'W', 'i', 'n', 'd', 'o', 'w', '\0' };
static unichar_t new[] = { 'N', 'e', 'w', '\0' };
static unichar_t open[] = { 'O', 'p', 'e', 'n', '\0' };
static unichar_t recent[] = { 'R', 'e', 'c', 'e', 'n', 't',  '\0' };
static unichar_t openoutline[] = { 'O', 'p', 'e', 'n', ' ', 'O', 'u', 't', 'l', 'i', 'n', 'e', '\0' };
static unichar_t openbitmap[] = { 'O', 'p', 'e', 'n', ' ', 'B', 'i', 't', 'm', 'a', 'p', '\0' };
static unichar_t openmetrics[] = { 'O', 'p', 'e', 'n', ' ', 'M', 'e', 't', 'r', 'i', 'c', 's', '\0' };
static unichar_t print[] = { 'P', 'r', 'i', 'n', 't', '.', '.', '.',  '\0' };
static unichar_t revert[] = { 'R', 'e', 'v', 'e','r','t',' ', 'F','i','l','e','\0' };
static unichar_t save[] = { 'S', 'a', 'v', 'e',  '\0' };
static unichar_t saveas[] = { 'S', 'a', 'v', 'e', ' ', 'a', 's', '.', '.', '.', '\0' };
static unichar_t generate[] = { 'G', 'e', 'n', 'e', 'r', 'a', 't', 'e', ' ', 'F','o','n','t','s', '.', '.', '.', '\0' };
static unichar_t import[] = { 'I', 'm', 'p', 'o', 'r', 't',  '.', '.', '.', '\0' };
static unichar_t close[] = { 'C', 'l', 'o', 's', 'e', '\0' };
static unichar_t prefs[] = { 'P', 'r', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e', 's', '.', '.', '.', '\0' };
static unichar_t quit[] = { 'Q', 'u', 'i', 't', '\0' };
static unichar_t fit[] = { 'F', 'i', 't', '\0' };
static unichar_t zoomin[] = { 'Z', 'o', 'o', 'm', ' ', 'i', 'n', '\0' };
static unichar_t zoomout[] = { 'Z', 'o', 'o', 'm', ' ', 'o', 'u', 't', '\0' };
static unichar_t next[] = { 'N', 'e', 'x', 't', ' ', 'C', 'h', 'a', 'r', '\0' };
static unichar_t prev[] = { 'P', 'r', 'e', 'v', ' ', 'C', 'h', 'a', 'r', '\0' };
static unichar_t _goto[] = { 'G', 'o', 't', 'o',  '\0' };
static unichar_t hidepoints[] = { 'H', 'i', 'd', 'e', ' ', 'P', 'o', 'i', 'n', 't', 's', '\0' };
static unichar_t _showpoints[] = { 'S', 'h', 'o', 'w', ' ', 'P', 'o', 'i', 'n', 't', 's', '\0' };
static unichar_t hiderulers[] = { 'H', 'i', 'd', 'e', ' ', 'R', 'u', 'l', 'e', 'r', 's', '\0' };
static unichar_t _showrulers[] = { 'S', 'h', 'o', 'w', ' ', 'R', 'u', 'l', 'e', 'r', 's', '\0' };
static unichar_t nextpoint[] = { 'N', 'e', 'x', 't', ' ', 'P', 'o', 'i', 'n', 't',  '\0' };
static unichar_t prevpoint[] = { 'P', 'r', 'e', 'v', ' ', 'P', 'o', 'i', 'n', 't',  '\0' };
static unichar_t fill[] = { 'F', 'i', 'l', 'l', '\0' };
static unichar_t undo[] = { 'U', 'n', 'd', 'o',  '\0' };
static unichar_t redo[] = { 'R', 'e', 'd', 'o',  '\0' };
static unichar_t cut[] = { 'C', 'u', 't',  '\0' };
static unichar_t _copy[] = { 'C', 'o', 'p', 'y',  '\0' };
static unichar_t copywidth[] = { 'C', 'o', 'p', 'y', ' ', 'W', 'i', 'd', 't', 'h',  '\0' };
static unichar_t copyref[] = { 'C', 'o', 'p', 'y', ' ', 'R', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e',  '\0' };
static unichar_t paste[] = { 'P', 'a', 's', 't', 'e',  '\0' };
static unichar_t clear[] = { 'C', 'l', 'e', 'a', 'r',  '\0' };
static unichar_t merge[] = { 'M', 'e', 'r', 'g', 'e',  '\0' };
static unichar_t selectall[] = { 'S', 'e', 'l', 'e', 'c', 't', ' ', 'A', 'l', 'l', '\0' };
static unichar_t unlinkref[] = { 'U', 'n', 'l', 'i', 'n', 'k', ' ', 'R', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e',  '\0' };
static unichar_t fontinfo[] = { 'F','o','n','t',' ','I','n','f','o', '.', '.', '.', '\0' };
static unichar_t privateinfo[] = { 'P','r','i','v','a','t','e',' ','I','n','f','o', '.', '.', '.', '\0' };
static unichar_t getinfo[] = { 'G','e','t',' ','I','n','f','o', '.', '.', '.', '\0' };
static unichar_t bitmapsavail[] = { 'B','i','t','m','a','p','s',' ','A','v','a','i','l','a','b','l','e','.', '.', '.',  '\0' };
static unichar_t regenbitmaps[] = { 'R','e','g','e','n','e','r','a','t','e', ' ', 'B','i','t','m','a','p','s','.', '.', '.',  '\0' };
static unichar_t autotrace[] = { 'A','u','t','o','t','r','a','c', 'e', /*'.', '.', '.',*/  '\0' };
static unichar_t transform[] = { 'T','r','a','n','s','f','o','r', 'm', '.', '.', '.',  '\0' };
static unichar_t stroke[] = { 'E','x','p','a','n','d',' ','S', 't', 'r', 'o','k', 'e', '.', '.', '.',  '\0' };
static unichar_t rmoverlap[] = { 'R','e','m','o','v','e',' ','O', 'v', 'e', 'r','l', 'a', 'p',  '\0' };
static unichar_t simplify[] = { 'S','i','m','p','l','i','f','y',  '\0' };
static unichar_t round2int[] = { 'R','o','u','n','d',' ','t','o',' ','i','n','t',  '\0' };
static unichar_t buildaccent[] = { 'B','u','i','l','d',' ','A','c','c','e','n','t','e','d',' ','C','h','a','r',  '\0' };
static unichar_t clockwise[] = { 'C','l','o','c','k','w','i','s', 'e',  '\0' };
static unichar_t cclockwise[] = { 'C','o','u','n','t','e','r',' ','C','l','o','c','k','w','i','s', 'e',  '\0' };
static unichar_t correct[] = { 'C','o','r','r','e','c','t',' ','D','i','r','e','c','t','i','o','n',  '\0' };
static unichar_t corner[] = { 'C','o','r','n','e','r',  '\0' };
static unichar_t curve[] = { 'C','u','r','v','e',  '\0' };
static unichar_t tangent[] = { 'T','a','n','g','e','n', 't',  '\0' };
static unichar_t autohint[] = { 'A', 'u', 't', 'o', 'H', 'i', 'n', 't',  '\0' };
static unichar_t clearhstem[] = { 'C', 'l', 'e', 'a', 'r', ' ', 'H', 'S', 't', 'e', 'm',  '\0' };
static unichar_t clearvstem[] = { 'C', 'l', 'e', 'a', 'r', ' ', 'V', 'S', 't', 'e', 'm',  '\0' };
static unichar_t addhhint[] = { 'A', 'd', 'd', ' ', 'H', 'H', 'i', 'n', 't',  '\0' };
static unichar_t addvhint[] = { 'A', 'd', 'd', ' ', 'V', 'H', 'i', 'n', 't',  '\0' };
static unichar_t createhhint[] = { 'C', 'r', 'e', 'a', 't', 'e', ' ', 'H', 'H', 'i', 'n', 't',  '\0' };
static unichar_t createvhint[] = { 'C', 'r', 'e', 'a', 't', 'e', ' ', 'V', 'H', 'i', 'n', 't',  '\0' };
static unichar_t reviewhints[] = { 'R', 'e', 'v', 'i', 'e', 'w', ' ', 'H', 'i', 'n', 't', 's',  '\0' };
static unichar_t export[] = { 'E','x','p','o','r', 't', '.', '.', '.',  '\0' };
static unichar_t palettes[] = { 'P','a','l','e','t', 't', 'e', 's',  '\0' };
static unichar_t tools[] = { 'T','o','o','l','s',  '\0' };
static unichar_t layers[] = { 'L','a','y','e', 'r','s',  '\0' };
static unichar_t metric[] = { 'M','e','t','r', 'i','c','s',  '\0' };
static unichar_t center[] = { 'C','e','n','t','e','r',' ','i','n',' ','W','i','d','t','h',  '\0' };
static unichar_t thirds[] = { 'T','h','i','r','d','s',' ','i','n',' ','W','i','d','t','h',  '\0' };
static unichar_t setwidth[] = { 'S','e','t',' ','W','i','d','t','h','.','.','.', '\0' };
static unichar_t setlbearing[] = { 'S','e','t',' ','L','B','e','a','r','i','n','g','.','.','.', '\0' };
static unichar_t setrbearing[] = { 'S','e','t',' ','R','B','e','a','r','i','n','g','.','.','.', '\0' };
static unichar_t removekern[] = { 'R','e','m','o','v','e',' ','K','e','r','n',' ','P','a','i','r','s', '\0' };

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
    FontMenuFontInfo(cv->fv->sf,cv->fv);
}

static void CVMenuPrivateInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SFPrivateInfo(cv->fv->sf);
}

static void CVMenuScale(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( mi->mid == MID_Fit ) {
	CVFit(cv);
    } else {
	double midx = (cv->width/2-cv->xoff)/cv->scale;
	double midy = (cv->height/2-cv->yoff)/cv->scale;
	CVMagnify(cv,midx,midy,mi->mid==MID_ZoomOut?-1:1);
    }
}

static void CVMenuShowHide(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showpoints = cv->showpoints = !cv->showpoints;
    GMenuBarSetItemName(cv->mb,MID_HidePoints,CVShows.showpoints?hidepoints:_showpoints);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuShowHideRulers(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    GRect pos;

    CVShows.showrulers = cv->showrulers = !cv->showrulers;
    GMenuBarSetItemName(cv->mb,MID_HideRulers,CVShows.showrulers?hiderulers:_showrulers);
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
    int pos = -1;

    if ( mi->mid == MID_Next ) {
	pos = cv->sc->enc+1;
    } else {
	pos = cv->sc->enc-1;
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
    CVInfoDraw(cv,cv->gw);
    SCUpdateAll(cv->sc);
}

int GotoChar(SplineFont *sf) {
    int pos = -1;
    static unichar_t quest[] = { 'E', 'n', 't', 'e', 'r', ' ', 't', 'h', 'e', ' ', 'n', 'a', 'm', 'e', ' ', 'o', 'f', ' ', 'a', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', ' ', 'i', 'n', ' ', 't', 'h', 'e', ' ', 'f', 'o', 'n', 't',  '\0' };
    unichar_t *ret, *end;
    int i;

    ret = GWidgetAskString(_goto,quest,NULL);
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
	    for ( temp=65535; temp>=0 ; --temp )
		if ( UnicodeCharacterNames[temp>>8][temp&0xff]!=NULL )
		    if ( u_strmatch(ret,UnicodeCharacterNames[temp>>8][temp&0xff])== 0 )
	    break;
	    if ( temp<0 )
		for ( temp=psunicodenames_cnt-1; temp>=0 ; --temp )
		    if ( psunicodenames[temp]!=NULL )
			if ( uc_strmatch(ret,psunicodenames[temp])== 0 )
		break;
	}
	if ( temp!=-1 ) {
	    for ( i=0; i<sf->charcnt; ++i )
		if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc==temp ) {
		    pos = i;
	    break;
		}
	    if ( pos==-1 && temp<sf->charcnt &&
		    ( sf->encoding_name==em_unicode || *ret=='0'))
		pos = temp;
	}
    }
    if ( pos==-1 ) {
	pos = u_strtol(ret,&end,10);
	if ( *end==',' && sf->encoding_name>=em_jis208 && sf->encoding_name<em_base ) {
	    int j = u_strtol(end+1,&end,10);
	    /* kuten */
	    if ( *end!='\0' )
		pos = -1;
	    else if ( sf->encoding_name==em_unicode ) {
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
	} else
	    pos = -1;
    }
    if ( pos<0 || pos>=sf->charcnt )
	pos = -1;
    if ( pos==-1 ) {
	unichar_t ubuf[100];
	uc_strcpy( ubuf, "Could not find the character: ");
	u_strncat(ubuf,ret,70);
	GWidgetPostNotice(_goto,ubuf);
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

    CVPaletteSetVisible(cv, mi->mid==MID_Tools, CVPaletteIsVisible(cv, mi->mid==MID_Tools));
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
    *cv->heads[cv->drawmode] = SplinePointListRemoveSelected(*cv->heads[cv->drawmode]);
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
		free(imgs);
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
    SplineCharMerge(cv->heads[cv->drawmode]);
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
	  break;
	  case MID_Cut: /*case MID_Copy:*/ case MID_Clear:
	    /* If nothing is selected, copy copies everything */
	    mi->ti.disabled = !anypoints && !anyrefs && !anyimages;
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
    
    if ( pointtype==pt_corner )
	/* Leave control points as they are */;
    else if ( pointtype==pt_tangent ) {
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
    int type = -2;
    SplinePointList *spl;
    Spline *spline, *first;

    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL && type!=-1; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) {
	    if ( type==-2 ) type = spl->first->pointtype;
	    else if ( type!=spl->first->pointtype ) type = -1;
	}
	for ( spline=spl->first->next; spline!=NULL && spline!=first && type!=-1; spline = spline->to->next ) {
	    if ( spline->to->selected ) {
		if ( type==-2 ) type = spline->to->pointtype;
		else if ( type!=spline->to->pointtype ) type = -1;
	    }
	    SplineRefigure(spline);
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

void CVTransFunc(CharView *cv,double transform[6]) {
    int anysel = cv->p.transany;
    RefChar *refs;
    ImageList *img;
    double t[6];

    SplinePointListTransform(*cv->heads[cv->drawmode],transform,!anysel);
    if ( cv->drawmode==dm_back ) {
	for ( img = cv->sc->backimages; img!=NULL; img=img->next )
	    if ( img->selected || !anysel ) {
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
		SCOutOfDateBackground(cv->sc);
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
    }
}

static void transfunc(void *d,double transform[6],int otype,BVTFunc *bvts) {
    CharView *cv = (CharView *) d;

    cv->p.transany = CVAnySel(cv,NULL,NULL,NULL);
    CVPreserveState(cv);
    CVTransFunc(cv,transform);
    CVCharChangedUpdate(cv);
}

static void CVMenuTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    TransformDlgCreate(cv,transfunc,getorigin);
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

    SCPreserveState(sc);
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
    SCCharChangedUpdate(sc,fv);
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
    *cv->heads[cv->drawmode] = SplineSetRemoveOverlap(*cv->heads[cv->drawmode]);
    CVCharChangedUpdate(cv);
}

static void CVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVPreserveState(cv);
    SplineCharSimplify(*cv->heads[cv->drawmode]);
    CVCharChangedUpdate(cv);
}

static void CVMenuAutotrace(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVAutoTrace(cv);
}

static void CVMenuBuildAccent(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int onlyaccents = e==NULL || !(e->u.mouse.state&ksm_shift);
    if ( !SFIsCompositBuildable(cv->fv->sf,cv->sc->unicodeenc) ||
	    (onlyaccents && !hascomposing(cv->fv->sf,cv->sc->unicodeenc)))
return;
    SCBuildComposit(cv->fv->sf,cv->sc,!cv->fv->onlycopydisplayed,cv->fv);
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

static void CVMenuBitmaps(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    BitmapDlg(cv->fv,cv->sc,mi->mid==MID_AvailBitmaps );
}

/* We can only paste if there's something in the copy buffer */
/* we can only copy if there's something selected to copy */
/* figure out what things are possible from the edit menu before the user */
/*  pulls it down */
static void ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anypoints = 0, splinepoints, dir = -2;
    SplinePointList *spl;
    Spline *spline, *first;
    int onlyaccents;

    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
	first = NULL;
	splinepoints = 0;
	if ( spl->first->selected ) splinepoints = true;
	for ( spline=spl->first->next; spline!=NULL && spline!=first && !splinepoints; spline = spline->to->next ) {
	    if ( spline->to->selected ) splinepoints = true;
	    if ( first == NULL ) first = spline;
	}
	if ( splinepoints ) {
	    anypoints = true;
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
	  case MID_Clockwise:
	    mi->ti.disabled = !anypoints;
	    mi->ti.checked = dir==1;
	  break;
	  case MID_Counter:
	    mi->ti.disabled = !anypoints;
	    mi->ti.checked = dir==0;
	  break;
	  case MID_Stroke: case MID_RmOverlap:
	    mi->ti.disabled = ( *cv->heads[cv->drawmode]==NULL );
	  break;
	  case MID_RegenBitmaps:
	    mi->ti.disabled = cv->fv->sf->bitmaps==NULL;
	  break;
	  /* Simplify is always available (it may not do anything though) */
	  case MID_BuildAccent:
	    mi->ti.disabled = !SFIsCompositBuildable(cv->fv->sf,cv->sc->unicodeenc);
	    onlyaccents = e==NULL || !(e->u.mouse.state&ksm_shift);
	    if ( onlyaccents ) {
		if ( !hascomposing(cv->fv->sf,cv->sc->unicodeenc))
		    mi->ti.disabled = true;
	    }
	    free(mi->ti.text);
	    mi->ti.text = uc_copy(onlyaccents?"Build Accented Char": "Build Composite Chars");
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

    if ( mi->mid==MID_ClearHStem ) {
	StemInfoFree(cv->sc->hstem);
	cv->sc->hstem = NULL;
	cv->sc->hconflicts = false;
    } else {
	StemInfoFree(cv->sc->vstem);
	cv->sc->vstem = NULL;
	cv->sc->hconflicts = false;
    }
    cv->sc->manualhints = true;
    SCOutOfDateBackground(cv->sc);
    SCUpdateAll(cv->sc);
}

static void CVMenuAddHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp1, *sp2;
    StemInfo *h;

    if ( !CVTwoForePointsSelected(cv,&sp1,&sp2))
return;

    if ( mi->mid==MID_AddHHint ) {
	if ( sp1->me.y==sp2->me.y )
return;
	h = calloc(1,sizeof(StemInfo));
	if ( sp2->me.y>sp1->me.y ) {
	    h->start = sp1->me.y;
	    h->width = sp2->me.y-sp1->me.y;
	} else {
	    h->start = sp2->me.y;
	    h->width = sp1->me.y-sp2->me.y;
	}
	SCGuessHHintInstancesAndAdd(cv->sc,h);
	cv->sc->hconflicts = StemListAnyConflicts(cv->sc->hstem);
    } else {
	if ( sp1->me.x==sp2->me.x )
return;
	h = calloc(1,sizeof(StemInfo));
	if ( sp2->me.x>sp1->me.x ) {
	    h->start = sp1->me.x;
	    h->width = sp2->me.x-sp1->me.x;
	} else {
	    h->start = sp2->me.x;
	    h->width = sp1->me.x-sp2->me.x;
	}
	SCGuessVHintInstancesAndAdd(cv->sc,h);
	cv->sc->vconflicts = StemListAnyConflicts(cv->sc->vstem);
    }
    cv->sc->manualhints = true;
    SCOutOfDateBackground(cv->sc);
    SCUpdateAll(cv->sc);
}

static void CVMenuCreateHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->hstem==NULL && cv->sc->vstem==NULL )
return;
    CVCreateHint(cv,mi->mid==MID_CreateHHint);
}

static void CVMenuReviewHints(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->hstem==NULL && cv->sc->vstem==NULL )
return;
    CVReviewHints(cv);
}

static void htlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp1, *sp2;
    int removeOverlap;

    sp1 = sp2 = NULL;
    CVTwoForePointsSelected(cv,&sp1,&sp2);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AutoHint:
	    removeOverlap = e==NULL || !(e->u.mouse.state&ksm_shift);
	    free(mi->ti.text);
	    mi->ti.text = uc_copy(removeOverlap?"AutoHint": "Full AutoHint");
	  break;
	  case MID_AddHHint:
	    mi->ti.disabled = sp1==NULL || sp2->me.y==sp1->me.y;
	  break;
	  case MID_AddVHint:
	    mi->ti.disabled = sp1==NULL || sp2->me.x==sp1->me.x;
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

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_NextPt: case MID_PrevPt:
	    mi->ti.disabled = !exactlyone;
	  break;
	}
    }
}

static void CVMenuCenter(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    DBounds bb;
    double transform[6];
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
	CVTransFunc(cv,transform);
	CVCharChangedUpdate(cv);
    }
    cv->drawmode = drawmode;
}

static void CVMenuSetWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVSetWidth(cv,mi->mid==MID_SetWidth?wt_width:
		  mi->mid==MID_SetLBearing?wt_lbearing:
		  wt_rbearing);
}

static void CVMenuRemoveKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->sc->kerns!=NULL ) {
	KernPairsFree(cv->sc->kerns);
	cv->sc->kerns = NULL;
	cv->sc->parent->changed = true;
    }
}

static GMenuItem dummyitem[] = { { new }, NULL };
static GMenuItem fllist[] = {
    { { new, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'N' }, 'N', ksm_control, NULL, NULL, MenuNew },
    { { open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, CVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { openoutline, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 0, 'u' }, 'H', ksm_control, NULL, NULL, /* No function, never avail */NULL },
    { { openbitmap, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'B' }, 'J', ksm_control, NULL, NULL, CVMenuOpenBitmap, MID_OpenBitmap },
    { { openmetrics, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'M' }, 'K', ksm_control, NULL, NULL, CVMenuOpenMetrics },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'S' }, 'S', ksm_control, NULL, NULL, CVMenuSave },
    { { saveas, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, CVMenuSaveAs },
    { { generate, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'G' }, 'G', ksm_control|ksm_shift, NULL, NULL, CVMenuGenerate },
    { { export, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 't' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuExport },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { import, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'I' }, 'I', ksm_control|ksm_shift, NULL, NULL, CVMenuImport },
    { { revert, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, CVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { print, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, 'P', ksm_control, NULL, NULL, CVMenuPrint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'U' }, 'Z', ksm_control, NULL, NULL, CVUndo, MID_Undo },
    { { redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, 'Y', ksm_control, NULL, NULL, CVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 't' }, 'X', ksm_control, NULL, NULL, CVCut, MID_Cut },
    { { _copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, 'C', ksm_control, NULL, NULL, CVCopy, MID_Copy },
    { { copyref, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'o' }, 'G', ksm_control, NULL, NULL, CVCopyRef, MID_CopyRef },
    { { copywidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'W' }, '\0', ksm_control, NULL, NULL, CVCopyWidth, MID_CopyWidth },
    { { paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, 'V', ksm_control, NULL, NULL, CVPaste, MID_Paste },
    { { clear, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'l' }, GK_Delete, 0, NULL, NULL, CVClear, MID_Clear },
    { { merge, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'M' }, 'M', ksm_control, NULL, NULL, CVMerge, MID_Merge },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { selectall, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'A' }, 'A', ksm_control, NULL, NULL, CVSelectAll, MID_SelAll },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { unlinkref, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'U' }, '\0', 0, NULL, NULL, CVUnlinkRef, MID_UnlinkRef },
    { NULL }
};

static GMenuItem ptlist[] = {
    { { curve, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'C' }, '\0', ksm_control, NULL, NULL, CVMenuPointType, MID_Curve },
    { { corner, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'o' }, '\0', ksm_control, NULL, NULL, CVMenuPointType, MID_Corner },
    { { tangent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'T' }, '\0', 0, NULL, NULL, CVMenuPointType, MID_Tangent },
    { NULL }
};

static GMenuItem ellist[] = {
    { { fontinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'F' }, 'F', ksm_control|ksm_shift, NULL, NULL, CVMenuFontInfo },
    { { privateinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, 'P', ksm_control|ksm_shift, NULL, NULL, CVMenuPrivateInfo },
    { { getinfo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'I' }, 'I', ksm_control, NULL, NULL, CVMenuGetInfo, MID_GetInfo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { bitmapsavail, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'A' }, 'B', ksm_control|ksm_shift, NULL, NULL, CVMenuBitmaps, MID_AvailBitmaps },
    { { regenbitmaps, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'B' }, 'B', ksm_control, NULL, NULL, CVMenuBitmaps, MID_RegenBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { transform, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'T' }, '\\', ksm_control, NULL, NULL, CVMenuTransform },
    { { stroke, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'E' }, 'E', ksm_control|ksm_shift, NULL, NULL, CVMenuStroke, MID_Stroke },
    { { rmoverlap, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'E' }, 'O', ksm_control|ksm_shift, NULL, NULL, CVMenuOverlap, MID_RmOverlap },
    { { simplify, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'S' }, 'M', ksm_control|ksm_shift, NULL, NULL, CVMenuSimplify, MID_Simplify },
    { { round2int, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'I' }, '_', ksm_control|ksm_shift, NULL, NULL, CVMenuRound2Int, MID_Round },
    { { autotrace, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'r' }, 'T', ksm_control|ksm_shift, NULL, NULL, CVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { clockwise, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'o' }, '\0', 0, NULL, NULL, CVMenuDir, MID_Clockwise },
    { { cclockwise, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'n' }, '\0', 0, NULL, NULL, CVMenuDir, MID_Counter },
    { { correct, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, '\0', 0, NULL, NULL, CVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { buildaccent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'B' }, 'A', ksm_control|ksm_shift, NULL, NULL, CVMenuBuildAccent, MID_BuildAccent },
    { NULL }
};

static GMenuItem htlist[] = {
    { { autohint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'H' }, 'H', ksm_control|ksm_shift, NULL, NULL, CVMenuAutoHint, MID_AutoHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { clearhstem, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearHStem },
    { { clearvstem, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'V' }, '\0', ksm_control, NULL, NULL, CVMenuClearHints, MID_ClearVStem },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { addhhint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'A' }, '\0', ksm_control, NULL, NULL, CVMenuAddHint, MID_AddHHint },
    { { addvhint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'd' }, '\0', ksm_control, NULL, NULL, CVMenuAddHint, MID_AddVHint },
    { { createhhint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'r' }, '\0', ksm_control, NULL, NULL, CVMenuCreateHint, MID_CreateHHint },
    { { createvhint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'e' }, '\0', ksm_control, NULL, NULL, CVMenuCreateHint, MID_CreateVHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { reviewhints, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, '\0', ksm_control, NULL, NULL, CVMenuReviewHints, MID_ReviewHints },
    { NULL }
};

static GMenuItem mtlist[] = {
    { { center, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'C' }, '\0', ksm_control, NULL, NULL, CVMenuCenter, MID_Center },
    { { thirds, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'T' }, '\0', ksm_control, NULL, NULL, CVMenuCenter, MID_Thirds },
    { { setwidth, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'W' }, '\0', ksm_control, NULL, NULL, CVMenuSetWidth, MID_SetWidth },
    { { setlbearing, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'L' }, '\0', 0, NULL, NULL, CVMenuSetWidth, MID_SetLBearing },
    { { setrbearing, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, '\0', 0, NULL, NULL, CVMenuSetWidth, MID_SetRBearing },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { removekern, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, '\0', ksm_control|ksm_shift, NULL, NULL, CVMenuRemoveKern, MID_RemoveKerns },
    { NULL }
};

static GMenuItem pllist[] = {
    { { tools, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'T' }, '\0', ksm_control, NULL, NULL, CVMenuPaletteShow, MID_Tools },
    { { layers, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 0, 'L' }, '\0', ksm_control, NULL, NULL, CVMenuPaletteShow, MID_Layers },
    { NULL }
};

static GMenuItem vwlist[] = {
    { { fit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'F' }, 'F', ksm_control, NULL, NULL, CVMenuScale, MID_Fit },
    { { zoomout, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'o' }, '\0', ksm_control, NULL, NULL, CVMenuScale, MID_ZoomOut },
    { { zoomin, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'i' }, '\0', ksm_control, NULL, NULL, CVMenuScale, MID_ZoomIn },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { next, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'N' }, ']', ksm_control, NULL, NULL, CVMenuChangeChar, MID_Next },
    { { prev, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, '[', ksm_control, NULL, NULL, CVMenuChangeChar, MID_Prev },
    { { _goto, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'G' }, '>', ksm_shift|ksm_control, NULL, NULL, CVMenuGotoChar },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { hidepoints, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'o' }, 'H', ksm_control, NULL, NULL, CVMenuShowHide, MID_HidePoints },
    { { nextpoint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'e' }, '}', ksm_shift|ksm_control, NULL, NULL, CVMenuNextPrevPt, MID_NextPt },
    { { prevpoint, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'v' }, '{', ksm_shift|ksm_control, NULL, NULL, CVMenuNextPrevPt, MID_PrevPt },
    { { fill, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'l' }, '\0', 0, NULL, NULL, CVMenuFill, MID_Fill },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, }},
    { { palettes, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, '\0', 0, pllist, pllistcheck },
    { { hiderulers, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'R' }, '\0', ksm_control, NULL, NULL, CVMenuShowHideRulers, MID_HideRulers },
    { NULL }
};

static GMenuItem mblist[] = {
    { { file, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'E' }, 0, 0, edlist, edlistcheck },
    { { point, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'P' }, 0, 0, ptlist, ptlistcheck },
    { { element, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'l' }, 0, 0, ellist, ellistcheck },
    { { hints, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'H' }, 0, 0, htlist, htlistcheck },
    { { view, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'V' }, 0, 0, vwlist, vwlistcheck },
    { { metric, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'M' }, 0, 0, mtlist, mtlistcheck },
    { { window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 'W' }, 0, 0, NULL, WindowMenuBuild },
    { NULL }
};

CharView *CharViewCreate(SplineChar *sc, FontView *fv) {
    CharView *cv = calloc(1,sizeof(CharView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    char buffer[200];
    GRect gsize;
    int sbsize;
    FontRequest rq;
    int as, ds, ld;
    static unichar_t fixed[] = { 'f','i','x','e','d', '\0' };
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
    cv->showpoints = CVShows.showpoints;
    cv->showrulers = CVShows.showrulers;
    cv->showfilled = CVShows.showfilled;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_ititle;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_mypointer;
    sprintf( buffer, "%.90s from %.90s ", sc->name, sc->parent->fontname );
    wattrs.icon_title = uc_copy(buffer);
    uc_strcpy(ubuf,buffer);
    if ( sc->unicodeenc!=-1 && UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]!=NULL )
	u_strcat(ubuf, UnicodeCharacterNames[sc->unicodeenc>>8][sc->unicodeenc&0xff]);
    wattrs.window_title = ubuf;
    wattrs.icon = CharIcon(cv, fv);
    if ( wattrs.icon )
	wattrs.mask |= wam_icon;
    pos.x = pos.y = 0; pos.width=pos.height = 540;

    cv->gw = gw = GDrawCreateTopWindow(NULL,&pos,cv_e_h,cv,&wattrs);
    free( wattrs.icon_title );

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
    cv->gi.u.image = calloc(1,sizeof(struct _GImage));
    cv->gi.u.image->image_type = it_mono;
    cv->gi.u.image->clut = calloc(1,sizeof(GClut));
    cv->gi.u.image->clut->trans_index = cv->gi.u.image->trans = 0;
    cv->gi.u.image->clut->clut_len = 2;
    cv->gi.u.image->clut->clut[0] = 0xffffff;
    cv->gi.u.image->clut->clut[1] = 0x808080;
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

    GMenuBarSetItemName(cv->mb,MID_HidePoints,CVShows.showpoints?hidepoints:_showpoints);
    GMenuBarSetItemName(cv->mb,MID_HideRulers,CVShows.showrulers?hiderulers:_showrulers);
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
    free(cv);
}
