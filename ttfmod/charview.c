/* Copyright (C) 2001 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * dercved from this software without specific prior written permission.

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
#include "ttfmodui.h"
#include "ttfinstrs.h"
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>

extern int _GScrollBar_Width;
extern const char *instrs[];
extern unichar_t *instrhelppopup[256];

struct charshows charshows = {
    true,		/* Instruction pane */
    TT_CONFIG_OPTION_BYTECODE_DEBUG,	/* gloss pane */
    true,		/* show original splines */
    true,		/* show grid lines */
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    true,		/* show gridfit splines if that'st legal */
    true,		/* show raster if that's legal */
# if TT_CONFIG_OPTION_BYTECODE_DEBUG
    true,		/* show twilight points if that's legal */
# endif
#endif
    12			/* ppem for figuring gridlines and doing gridfitting */
};

#define EDGE_SPACING	2
#define INSTR_WIDTH	(2*EDGE_SPACING+21*cv->numlen)
#define GLOSS_WIDTH	(2*EDGE_SPACING+28*cv->numlen)
#define CHAR_WIDTH	400
#define MIN_CHAR_WIDTH	50

#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_SelAll	2106
#define MID_Fit		2201
#define MID_ZoomOut	2202
#define MID_ZoomIn	2203
#define MID_Next	2204
#define MID_Prev	2205
#define MID_HideSplines	2210
#define MID_HideInstrs	2211
#define MID_HideGloss	2212
#define MID_HideGrid	2213
#define MID_HideGridFit	2214
#define MID_HideRaster	2215
#define MID_HideTwilight	2216

int CVClose(CharView *cv) {
    if ( cv->destroyed )
return( true );

    cv->destroyed = true;
    GDrawDestroyWindow(cv->gw);
return( true );
}

static void CVUpdateInfo(CharView *cv, GEvent *event) {
    GRect pos;

    cv->mouse.x = event->u.mouse.x;
    cv->mouse.y = event->u.mouse.y;
    cv->info.x = (cv->mouse.x-cv->xoff)/cv->scale;
    cv->info.y = (cv->vheight-cv->yoff - cv->mouse.y)/cv->scale;
    pos.x = 0; pos.y = cv->mbh; pos.width = 32767; pos.height = cv->infoh;
    GDrawRequestExpose(cv->gw,&pos,false);
}

static void CVNewScale(CharView *cv) {
    GEvent e;

    GScrollBarSetBounds(cv->vsb,-32768*cv->scale,32768*cv->scale,cv->vheight);
    GScrollBarSetBounds(cv->hsb,-32768*cv->scale,32768*cv->scale,cv->vwidth);
    GScrollBarSetPos(cv->vsb,cv->yoff);
    GScrollBarSetPos(cv->hsb,cv->xoff);

    GDrawRequestExpose(cv->v,NULL,false);
    GDrawGetPointerPosition(cv->v,&e);
    CVUpdateInfo(cv,&e);
}

static void CVFit(CharView *cv) {
    DBounds b;
    real left, right, top, bottom, hsc, wsc;

    ConicCharFindBounds(cv->cc,&b);
    bottom = b.miny;
    top = b.maxy;
    left = b.minx;
    right = b.maxx;

    if ( bottom>0 ) bottom = 0;
    if ( left>0 ) left = 0;
    if ( right<cv->cc->width ) right = cv->cc->width;
    if ( top<bottom ) GDrawIError("Bottom bigger than top!");
    if ( right<left ) GDrawIError("Left bigger than right!");
    top -= bottom;
    right -= left;
    if ( top==0 ) top = 1000;
    if ( right==0 ) right = 1000;
    wsc = cv->vwidth / right;
    hsc = cv->vheight / top;
    if ( wsc<hsc ) hsc = wsc;

    cv->scale = hsc/1.2;
    if ( cv->scale > 1.0 ) {
	cv->scale = floor(cv->scale);
    } else {
	cv->scale = 1/ceil(1/cv->scale);
    }

    cv->xoff = (cv->vwidth-right*cv->scale)/3;
    cv->yoff = (cv->vheight-top*cv->scale)/3;

    CVNewScale(cv);
}

static void CVMagnify(CharView *cv, real midx, real midy, int bigger) {
    static float scales[] = { 1, 2, 3, 4, 6, 8, 11, 16, 23, 32, 45, 64, 90, 128, 181, 256, 512, 1024, 0 };
    int i, j;

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
    cv->xoff = -(rint(midx*cv->scale) - cv->vwidth/2);
    cv->yoff = -(rint(midy*cv->scale) - cv->vheight/2);
    CVNewScale(cv);
}

static void CVChangeGlyph(CharView *cv, int glyph) {
    ConicChar *cc = cv->cc;
    ConicFont *cf = cc->parent;
    CharView *prev, *test;
    ConicChar *new = LoadGlyph(cf,glyph);
    char buf[100];
    unichar_t title[100];

    if ( new==NULL )
return;

    if ( (prev = cc->views)==cv )
	cc->views = cv->next;
    else {
	for ( test=prev->next; test!=cv && test!=NULL; prev = test, test=test->next );
	prev->next = cv->next;
    }
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    ConicPointListsFree(cv->gridfit); cv->gridfit = NULL;
    FreeType_FreeRaster(cv->raster); cv->raster = NULL;
#endif

    cv->next = new->views;
    new->views = cv;
    cv->cc = new;

    sprintf(buf,"Glyph: %d ", glyph);
    uc_strcpy(title, buf);
    if ( cf->tfont->enc!=NULL && glyph<cf->tfont->enc->cnt && psunicodenames[cf->tfont->enc->uenc[glyph]]!=NULL )
	uc_strncat(title,psunicodenames[cf->tfont->enc->uenc[glyph]],sizeof(buf)-strlen(buf)-1);
    title[sizeof(title)/sizeof(title[0])-1] = '\0';
    GDrawSetWindowTitles(cv->gw,title,NULL);

    cv->instrinfo.instrdata = &new->instrdata;
    cv->instrinfo.isel_pos = -1;
    instr_typify(&cv->instrinfo);
    GScrollBarSetBounds(cv->instrinfo.vsb,0,cv->instrinfo.lheight,cv->instrinfo.vheight/cv->fh);
    cv->instrinfo.lpos = 0;
    GScrollBarSetPos(cv->instrinfo.vsb,cv->instrinfo.lpos);

#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    FreeType_GridFitChar(cv);
#endif
#if TT_CONFIG_OPTION_BYTECODE_DEBUG
    CVGenerateGloss(cv);
    cv->instrinfo.gsel_pos = -1;
    cv->gvpos = 0;
    GScrollBarSetBounds(cv->gvsb,0,cv->instrinfo.act_cnt,cv->instrinfo.vheight/cv->fh);
    GScrollBarSetPos(cv->gvsb,cv->gvpos);
#endif

    GDrawRequestExpose(cv->v,NULL,false);
    GDrawRequestExpose(cv->instrinfo.v,NULL,false);
#if TT_CONFIG_OPTION_BYTECODE_DEBUG
    GDrawRequestExpose(cv->glossv,NULL,false);
#endif
}

static int CVClearSel(CharView *cv) {
    ConicPointList *spl;
    Conic *spline, *first;
    RefChar *rf;
    int needsupdate = false;

    for ( spl = cv->cc->conics; spl!=NULL; spl = spl->next ) {
	if ( spl->first->selected ) { needsupdate = true; spl->first->selected = false; }
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( spline->to->selected )
		{ needsupdate = true; spline->to->selected = false; }
	    if ( first==NULL ) first = spline;
	}
    }
    for ( rf=cv->cc->refs; rf!=NULL; rf = rf->next ) {
	if ( rf->selected ) { needsupdate = true; rf->selected = false; }
	for ( spl = rf->conics; spl!=NULL; spl = spl->next ) {
	    if ( spl->first->selected ) { needsupdate = true; spl->first->selected = false; }
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( spline->to->selected )
		    { needsupdate = true; spline->to->selected = false; }
		if ( first==NULL ) first = spline;
	    }
	}
    }
return( needsupdate );
}

static BasePoint *CVGetTTFPoint(CharView *cv,int pnum,BasePoint **moved) {
    ConicPointList *spl, *mspl;
    ConicPoint *sp, *msp;
    RefChar *rf;

#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    mspl = cv->gridfit; msp = NULL;
#else
    mspl = NULL; msp = NULL;
#endif
    if ( moved!=NULL ) *moved = NULL;
    for ( spl = cv->cc->conics; spl!=NULL; spl = spl->next ) {
	if ( mspl!=NULL ) msp = mspl->first;
	for ( sp = spl->first; ; ) {
	    if ( sp->me.pnum==pnum ) {
		if ( moved!=NULL && msp!=NULL ) *moved = &msp->me;
return( &sp->me );
	    }
	    if ( sp->nextcp!=NULL && sp->nextcp->pnum==pnum ) {
		if ( moved!=NULL && msp!=NULL ) *moved = msp->nextcp;
return( sp->nextcp );
	    }
	    if ( msp==NULL ) /* Do Nothing */;
	    else if ( msp->next==NULL ) msp=NULL;
	    else msp=msp->next->to;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( mspl!=NULL ) mspl = mspl->next;
    }
    for ( rf=cv->cc->refs; rf!=NULL; rf = rf->next ) {
	if ( mspl!=NULL ) msp = mspl->first;
	for ( spl = rf->conics; spl!=NULL; spl = spl->next ) {
	    for ( sp = spl->first; ; ) {
		if ( sp->me.pnum==pnum ) {
		    if ( moved!=NULL && msp!=NULL ) *moved = &msp->me;
return( &sp->me );
		}
		if ( sp->nextcp!=NULL && sp->nextcp->pnum==pnum ) {
		    if ( moved!=NULL && msp!=NULL ) *moved = msp->nextcp;
return( sp->nextcp );
		}
		if ( msp==NULL );
		else if ( msp->next==NULL ) msp=NULL;
		else msp=msp->next->to;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	    if ( mspl!=NULL ) mspl = mspl->next;
	}
    }
return( NULL );
}

static ConicPoint *CVGetPoint(CharView *cv,int pnum) {
    ConicPointList *spl;
    ConicPoint *sp;
    RefChar *rf;

    for ( spl = cv->cc->conics; spl!=NULL; spl = spl->next ) {
	for ( sp = spl->first; ; ) {
	    if ( sp->me.pnum==pnum )
return( sp );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
    for ( rf=cv->cc->refs; rf!=NULL; rf = rf->next ) {
	for ( spl = rf->conics; spl!=NULL; spl = spl->next ) {
	    for ( sp = spl->first; ; ) {
		if ( sp->me.pnum==pnum )
return( sp );
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }
return( NULL );
}

static void char_resize(CharView *cv,GEvent *event) {
    GRect pos;
    int wid=0;
    int x = 0, lh;

    if ( cv->show.instrpane ) wid += INSTR_WIDTH+cv->sbw;
    if ( cv->show.glosspane ) wid += GLOSS_WIDTH+cv->sbw;
    wid += MIN_CHAR_WIDTH+cv->sbw;
    /* height must be a multiple of the line height (if we are showing lines) */
    if ( (event->u.resize.size.height-cv->mbh-cv->infoh-2*EDGE_SPACING)%cv->fh!=0 ||
	    (event->u.resize.size.height-cv->mbh-cv->infoh-cv->fh)<0 ||
	    (event->u.resize.size.width-wid)<0 ) {
	int lc = (event->u.resize.size.height-cv->mbh-cv->infoh-2*EDGE_SPACING+cv->fh/2)/cv->fh;
	if ( lc<=0 ) lc = 1;
	if ( event->u.resize.size.width>wid ) wid = event->u.resize.size.width;
	GDrawResize(cv->gw, wid,cv->mbh+cv->infoh+lc*cv->fh+2*EDGE_SPACING );
return;
    }

    pos.width = cv->sbw;
    pos.height = event->u.resize.size.height-cv->mbh-cv->infoh;
    pos.x = INSTR_WIDTH; pos.y = cv->mbh+cv->infoh;
    GGadgetResize(cv->instrinfo.vsb,pos.width,pos.height);
    GGadgetMove(cv->instrinfo.vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(cv->instrinfo.v,pos.width,pos.height);
    if ( cv->show.instrpane ) x += INSTR_WIDTH+cv->sbw;

    cv->instrinfo.vheight = pos.height; cv->instrinfo.vwidth = pos.width;
    lh = cv->instrinfo.lheight;

    GScrollBarSetBounds(cv->instrinfo.vsb,0,lh,cv->instrinfo.vheight/cv->fh);
    if ( cv->instrinfo.lpos + cv->instrinfo.vheight/cv->fh > lh )
	cv->instrinfo.lpos = lh-cv->instrinfo.vheight/cv->fh;
    if ( cv->instrinfo.lpos < 0 ) cv->instrinfo.lpos = 0;
    GScrollBarSetPos(cv->instrinfo.vsb,cv->instrinfo.lpos);


#if TT_CONFIG_OPTION_BYTECODE_DEBUG
    pos.width = cv->sbw;
    pos.x = x+GLOSS_WIDTH;
    GGadgetResize(cv->gvsb,pos.width,pos.height);
    GGadgetMove(cv->gvsb,pos.x,pos.y);
    pos.width = pos.x-x; pos.x = x;
    GDrawMove(cv->glossv,pos.x,pos.y);
    GDrawResize(cv->glossv,pos.width,pos.height);
    cv->gvwidth = GLOSS_WIDTH;
    cv->gvheight = pos.height;
    if ( cv->show.glosspane ) x += GLOSS_WIDTH+cv->sbw;

    GScrollBarSetBounds(cv->gvsb,0,cv->instrinfo.act_cnt,cv->instrinfo.vheight/cv->fh);
    if ( cv->gvpos + cv->instrinfo.vheight/cv->fh > cv->instrinfo.act_cnt )
	cv->gvpos = cv->instrinfo.act_cnt-cv->instrinfo.vheight/cv->fh;
    if ( cv->gvpos < 0 ) cv->gvpos = 0;
    GScrollBarSetPos(cv->gvsb,cv->gvpos);
#endif


    pos.width = cv->sbw;
    pos.x = event->u.resize.size.width-cv->sbw;
    pos.height -= cv->sbw;
    GGadgetResize(cv->vsb,pos.width,pos.height);
    GGadgetMove(cv->vsb,pos.x,pos.y);
    pos.width = pos.x-x; pos.x = x;
    GDrawMove(cv->v,pos.x,pos.y);
    GDrawResize(cv->v,pos.width,pos.height);
    cv->vheight = pos.height; cv->vwidth = pos.width;

    pos.y += pos.height; pos.height = cv->sbw;
    GGadgetResize(cv->hsb,pos.width,pos.height);
    GGadgetMove(cv->hsb,pos.x,pos.y);

    CVFit(cv);
}

static void CVDrawBB(CharView *cv, GWindow pixmap, DBounds *bb) {
    GRect r;

    r.x =  cv->xoff + rint(bb->minx*cv->scale);
    r.y = -cv->yoff + cv->vheight - rint(bb->maxy*cv->scale);
    r.width = rint((bb->maxx-bb->minx)*cv->scale);
    r.height = rint((bb->maxy-bb->miny)*cv->scale);
    GDrawSetDashedLine(pixmap,1,1,0);
    GDrawDrawRect(pixmap,&r,0x000000);
    GDrawSetDashedLine(pixmap,0,0,0);
}

/* Sigh. I have to do my own clipping because at large magnifications */
/*  things can easily exceed 16 bits */
static int CVConicOutside(CharView *cv, Conic *conic) {
    int x[4], y[4];

    x[0] =  cv->xoff + rint(conic->from->me.x*cv->scale);
    y[0] = -cv->yoff + cv->vheight - rint(conic->from->me.y*cv->scale);

    x[1] =  cv->xoff + rint(conic->to->me.x*cv->scale);
    y[1] = -cv->yoff + cv->vheight - rint(conic->to->me.y*cv->scale);

    if ( conic->from->nextcp==NULL ) {
	if ( (x[0]<0 && x[1]<0) || (x[0]>=cv->vwidth && x[1]>=cv->vwidth) ||
		(y[0]<0 && y[1]<0) || (y[0]>=cv->vheight && y[1]>=cv->vheight) )
return( true );
    } else {
	x[2] =  cv->xoff + rint(conic->from->nextcp->x*cv->scale);
	y[2] = -cv->yoff + cv->vheight - rint(conic->from->nextcp->y*cv->scale);
	if ( (x[0]<0 && x[1]<0 && x[2]<0) ||
		(x[0]>=cv->vwidth && x[1]>=cv->vwidth && x[2]>=cv->vwidth ) ||
		(y[0]<0 && y[1]<0 && y[2]<0 ) ||
		(y[0]>=cv->vheight && y[1]>=cv->vheight && y[2]>=cv->vheight ) )
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
	l->asend.y = l->asstart.y = -cv->yoff + cv->vheight-l->here.y;
	l->flags = 0;
	if ( l->asend.x<0 || l->asend.x>=cv->vwidth || l->asend.y<0 || l->asend.y>=cv->vheight ) {
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
	    if (( l->asstart.x<cv->vwidth && l->next->asend.x>cv->vwidth ) ||
		    ( l->asstart.x>cv->vwidth && l->next->asend.x<cv->vwidth )) {
		y = (l->next->asend.y-l->asstart.y)*(real)(cv->vwidth-l->asstart.x)/(l->next->asend.x-l->asstart.x) +
			l->asstart.y;
		if ( l->asstart.x>cv->vwidth ) {
		    l->asstart.x = cv->vwidth;
		    l->asstart.y = y;
		} else {
		    l->next->asend.x = cv->vwidth;
		    l->next->asend.y = y;
		}
	    }
	    if (( l->asstart.y<0 && l->next->asend.y>0 ) ||
		    ( l->asstart.y>0 && l->next->asend.y<0 )) {
		x = -(l->next->asend.x-l->asstart.x)*(real)l->asstart.y/(l->next->asend.y-l->asstart.y) +
			l->asstart.x;
		if (( x<0 || x>=cv->vwidth ) && bothout )
    continue;			/* Not on screen */;
		if ( l->asstart.y<0 ) {
		    l->asstart.y = 0;
		    l->asstart.x = x;
		} else {
		    l->next->asend.y = 0;
		    l->next->asend.x = x;
		}
	    }
	    if (( l->asstart.y<cv->vheight && l->next->asend.y>cv->vheight ) ||
		    ( l->asstart.y>cv->vheight && l->next->asend.y<cv->vheight )) {
		x = (l->next->asend.x-l->asstart.x)*(real)(cv->vheight-l->asstart.y)/(l->next->asend.y-l->asstart.y) +
			l->asstart.x;
		if (( x<0 || x>=cv->vwidth ) && bothout )
    continue;			/* Not on screen */;
		if ( l->asstart.y>cv->vheight ) {
		    l->asstart.y = cv->vheight;
		    l->asstart.x = x;
		} else {
		    l->next->asend.y = cv->vheight;
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
static GPointList *MakePoly(CharView *cv, ConicPointList *spl) {
    int i, len;
    LinearApprox *lap;
    LineList *line, *prev;
    Conic *conic, *first;
    GPointList *head=NULL, *last=NULL, *cur;
    int closed;

    for ( i=0; i<2; ++i ) {
	len = 0; first = NULL;
	closed = true;
	cur = NULL;
	for ( conic = spl->first->next; conic!=NULL && conic!=first; conic=conic->to->next ) {
	    if ( !CVConicOutside(cv,conic) ) {
		lap = ConicApproximate(conic,cv->scale);
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
	    if ( first==NULL ) first = conic;
	}
	if ( i==0 && cur!=NULL )
	    cur->cnt = len;
    }
return( head );
}

	/* 0 is up, 1 is right, 2 down, 3 left */
static void TickDir(BasePoint *me,BasePoint *other, char dirs[4]) {
    if ( other==NULL )		/* No spline -> no interferance, needn't tick anything */
return;
    if ( other->x<me->x ) {
	if ( other->y<me->y ) {
	    if ( me->y-other->y > me->x-other->x ) dirs[2] = true;
	    else dirs[3] = true;
	} else {
	    if ( other->y-me->y > me->x-other->x ) dirs[0] = true;
	    else dirs[3] = true;
	}
    } else {
	if ( other->y<me->y ) {
	    if ( me->y-other->y > other->x-me->x ) dirs[2] = true;
	    else dirs[1] = true;
	} else {
	    if ( other->y-me->y > other->x-me->x ) dirs[0] = true;
	    else dirs[1] = true;
	}
    }
}

static void NumberPoint(GWindow pixmap, BasePoint *before, BasePoint *me, BasePoint *after,
	CharView *cv, int x, int y, Color col) {
    char buf[4]; unichar_t ubuf[4]; int wid;
    char dirs[4];

    memset(dirs,'\0',sizeof(dirs));
    TickDir(me,after,dirs);
    TickDir(me,before,dirs);
    sprintf(buf,"%d",me->pnum);
    uc_strcpy(ubuf,buf);
    wid = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
    /* 0 is up, 1 is right, 2 down, 3 left */
    if ( !dirs[0] || !dirs[2])
	GDrawDrawText(pixmap,x-wid/2,!dirs[2]?y+cv->sfh+cv->sas:y-cv->sfh,ubuf,-1,NULL,col);
    else
	GDrawDrawText(pixmap,!dirs[1]?x+cv->sfh:x-cv->sfh-wid,y+cv->sas/2,ubuf,-1,NULL,col);
}

static void DrawPoint(CharView *cv, GWindow pixmap, ConicPoint *sp, ConicSet *spl) {
    GRect r;
    int x, y, cx, cy;
    Color col = spl==NULL ? 0x0000ff: sp==spl->first ? 0x707000 : 0xff0000;
    char dirs[4];

    if ( sp->me.pnum==-1 )
	col = col==0xff0000 ? 0xff4040 : 0x808030;
    x =  cv->xoff + rint(sp->me.x*cv->scale);
    y = -cv->yoff + cv->vheight - rint(sp->me.y*cv->scale);
    if ( x<-4000 || y<-4000 || x>cv->vwidth+4000 || y>=cv->vheight+4000 )
return;

    memset(dirs,'\0',sizeof(dirs));

    /* draw the control points if it's selected */
    /*if ( sp->selected ) {*/
	if ( sp->nextcp!=NULL ) {
	    cx =  cv->xoff + rint(sp->nextcp->x*cv->scale);
	    cy = -cv->yoff + cv->vheight - rint(sp->nextcp->y*cv->scale);
	    GDrawDrawLine(pixmap,x,y,cx,cy, 0x0000ff);
	    GDrawDrawLine(pixmap,cx-2,cy-2,cx+2,cy+2,0x0000ff);
	    GDrawDrawLine(pixmap,cx+2,cy-2,cx-2,cy+2,0x0000ff);
	    if ( sp->nextcp->pnum!=-1 )
		NumberPoint(pixmap, &sp->me, sp->nextcp,
		    sp->next!=NULL?&sp->next->to->me:NULL,
		    cv,cx,cy,col);
	}
	if ( sp->prevcp!=NULL ) {
	    cx =  cv->xoff + rint(sp->prevcp->x*cv->scale);
	    cy = -cv->yoff + cv->vheight - rint(sp->prevcp->y*cv->scale);
	    GDrawDrawLine(pixmap,x,y,cx,cy, 0x0000ff);
	    GDrawDrawLine(pixmap,cx-2,cy-2,cx+2,cy+2,0x0000ff);
	    GDrawDrawLine(pixmap,cx+2,cy-2,cx-2,cy+2,0x0000ff);
	}
    /*}*/

    if ( x<-4 || y<-4 || x>cv->vwidth+4 || y>=cv->vheight+4 )
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
	if ( sp->nextcp )
	    cp = sp->nextcp;
	else if ( sp->prevcp )
	    cp = sp->prevcp;
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

    if ( sp->me.pnum>=0 )
	NumberPoint(pixmap,
	    sp->prevcp!=NULL?sp->prevcp:sp->prev!=NULL?&sp->prev->from->me:NULL,
	    &sp->me,
	    sp->nextcp!=NULL?sp->nextcp:sp->next!=NULL?&sp->next->to->me:NULL,
	    cv,x,y,col);
}

static void DrawBasePoint(CharView *cv,GWindow pixmap,BasePoint *me) {
    static ConicPoint sp;

    sp.me = *me;
    DrawPoint(cv,pixmap,&sp,NULL);
}

static void DrawLine(CharView *cv, GWindow pixmap,
	real x1, real y1, real x2, real y2, Color fg) {
    int ix1 = cv->xoff + rint(x1*cv->scale);
    int iy1 = -cv->yoff + cv->vheight - rint(y1*cv->scale);
    int ix2 = cv->xoff + rint(x2*cv->scale);
    int iy2 = -cv->yoff + cv->vheight - rint(y2*cv->scale);
    if ( iy1==iy2 ) {
	if ( iy1<0 || iy1>cv->vheight )
return;
	if ( ix1<0 ) ix1 = 0;
	if ( ix2>cv->vwidth ) ix2 = cv->vwidth;
    } else if ( ix1==ix2 ) {
	if ( ix1<0 || ix1>cv->vwidth )
return;
	if ( iy1<0 ) iy1 = 0;
	if ( iy2<0 ) iy2 = 0;
	if ( iy1>cv->vheight ) iy1 = cv->vheight;
	if ( iy2>cv->vheight ) iy2 = cv->vheight;
    }
    GDrawDrawLine(pixmap, ix1,iy1, ix2,iy2, fg );
}

static void CVDrawConicSet(CharView *cv, GWindow pixmap, ConicPointList *set,
	Color fg, int dopoints, DRect *clip ) {
    Conic *conic, *first;
    ConicPointList *spl;

    for ( spl = set; spl!=NULL; spl = spl->next ) {
	GPointList *gpl = MakePoly(cv,spl), *cur;
	if ( dopoints ) {
	    first = NULL;
	    for ( conic = spl->first->next; conic!=NULL && conic!=first; conic=conic->to->next ) {
		DrawPoint(cv,pixmap,conic->from,spl);
		if ( first==NULL ) first = conic;
	    }
	    if ( conic==NULL )
		DrawPoint(cv,pixmap,spl->last,spl);
	}
	for ( cur=gpl; cur!=NULL; cur=cur->next )
	    GDrawDrawPoly(pixmap,cur->gp,cur->cnt,fg);
	GPLFree(gpl);
    }
}

static void char_expose(CharView *cv,GWindow pixmap,GRect *rect) {
    RefChar *rf;
    GRect old;
    DRect clip;
    real grid_spacing = cv->cc->parent->em / (real) cv->show.ppem;
    int i,j;
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    GRect pixel;
#endif

    GDrawPushClip(pixmap,rect,&old);
    GDrawSetFont(pixmap,cv->sfont);

    clip.width = rect->width/cv->scale;
    clip.height = rect->height/cv->scale;
    clip.x = (rect->x-cv->xoff)/cv->scale;
    clip.y = (cv->vheight-rect->y-rect->height-cv->yoff)/cv->scale;

#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    if ( cv->show.raster && cv->raster!=NULL && cv->raster!=(void *) -1 ) {
	pixel.width = pixel.height = grid_spacing*cv->scale+1;
	for ( i=0; i<cv->raster->rows; ++i ) {
	    for ( j=0; j<cv->raster->cols; ++j ) {
		if ( cv->raster->bitmap[i*cv->raster->bytes_per_row+(j>>3)] & (1<<(7-(j&7))) ) {
		    pixel.x = (j+cv->raster->lb)*grid_spacing*cv->scale + cv->xoff;
		    pixel.y = cv->vheight-cv->yoff - rint((cv->raster->as-i)*grid_spacing*cv->scale);
		    GDrawFillRect(pixmap,&pixel,0xa0a0a0);
		}
	    }
	}
    }
#endif
    if ( cv->show.grid && grid_spacing*cv->scale>=2 ) {
	int max,jmax;
	for ( i = floor( clip.x/grid_spacing ), max = ceil((clip.x+clip.width)/grid_spacing);
		i<=max; ++i )
	    DrawLine(cv,pixmap,i*grid_spacing,-32768,i*grid_spacing,32767,i==0?0x808080:0xb0b0ff);
	for ( i = floor( clip.y/grid_spacing ), max = ceil((clip.y+clip.height)/grid_spacing);
		i<=max; ++i )
	    DrawLine(cv,pixmap,-32768,i*grid_spacing,32767,i*grid_spacing,i==0?0x808080:0xb0b0ff);
	if ( grid_spacing*cv->scale>=7 ) {
	    for ( i = floor( clip.x/grid_spacing ), max = ceil((clip.x+clip.width)/grid_spacing);
		    i<=max; ++i )
		for ( j = floor( clip.y/grid_spacing ), jmax = ceil((clip.y+clip.height)/grid_spacing);
			j<=jmax; ++j ) {
		    int x = (i+.5)*grid_spacing*cv->scale + cv->xoff;
		    int y = cv->vheight-cv->yoff - rint((j+.5)*grid_spacing*cv->scale);
		    GDrawDrawLine(pixmap,x-2,y,x+2,y,0xb0b0ff);
		    GDrawDrawLine(pixmap,x,y-2,x,y+2,0xb0b0ff);
		}
	}
    } else {
	/* Just draw main axes */
	DrawLine(cv,pixmap,0,-32768,0,32767,0x808080);
	DrawLine(cv,pixmap,-32768,0,32767,0,0x808080);
    }

    DrawLine(cv,pixmap,cv->cc->width,-8096,cv->cc->width,8096,0x404040);
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    if ( cv->show.gridspline ) {
	DrawLine(cv,pixmap,cv->gridwidth,-8096,cv->gridwidth,8096,0x008000);

	CVDrawConicSet(cv,pixmap,cv->gridfit,0x008000,true,&clip);
    }
#endif
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER && TT_CONFIG_OPTION_BYTECODE_DEBUG
    if ( cv->show.twilight ) {
	for ( i=0; i<cv->twilight_cnt; ++i )
	    DrawBasePoint(cv,pixmap,&cv->twilight[i]);
    }
#endif

    if ( cv->show.fore ) {
	for ( rf=cv->cc->refs; rf!=NULL; rf = rf->next ) {
	    CVDrawConicSet(cv,pixmap,rf->conics,0,true,&clip);
	    if ( /*rf->selected*/ true )
		CVDrawBB(cv,pixmap,&rf->bb);
	}

	CVDrawConicSet(cv,pixmap,cv->cc->conics,0x000000,true,&clip);
    }

    GDrawPopClip(pixmap,&old);
}

static void char_infoexpose(CharView *cv,GWindow pixmap,GRect *rect) {
    char buf[100], *bpt;
    unichar_t ubuf[100];

    sprintf( buf, "(%d,%d) ", (int) cv->info.x, (int) cv->info.y );
    bpt = buf + strlen(buf);
    if ( cv->scale <.01 )
	sprintf( bpt, "%.2f%%", cv->scale*100 );
    else if ( cv->scale<1 )
	sprintf( bpt, "%.2g%%", cv->scale*100 );
    else
	sprintf( bpt, "%.0f%%", rint(cv->scale*100) );
    uc_strcpy(ubuf,buf);
    GDrawSetFont(pixmap,cv->sfont);
    GDrawDrawText(pixmap,5,cv->mbh+cv->as,ubuf,-1,NULL,0x000000);
    GDrawDrawLine(pixmap,0,cv->mbh+cv->infoh-1,32767,cv->mbh+cv->infoh-1,0x000000);
}

#if TT_CONFIG_OPTION_BYTECODE_DEBUG
static void char_glossexpose(CharView *cv,GWindow pixmap,GRect *rect) {
    int i,y;
    struct ttfactions *act;
    char buf[100];
    unichar_t ubuf[100];

    GDrawSetFont(pixmap,cv->gfont);
    y = EDGE_SPACING+cv->as;
    for ( i=0, act = cv->instrinfo.acts; i<cv->gvpos && i<cv->instrinfo.act_cnt && y<rect->y+rect->height; ++i )
	act = act->acts;
    while ( i<cv->instrinfo.act_cnt && y<rect->y+rect->height+cv->fh ) {
	if ( i==cv->instrinfo.gsel_pos ) {
	    GRect r;
	    r.y = y-cv->as; r.x = 0; r.height = cv->fh; r.width = cv->gvwidth;
	    GDrawFillRect(pixmap,&r,0xffff00);
	}
	if ( act->newcontour )
	    GDrawDrawLine(pixmap,0,y-cv->as,cv->gvwidth,y-cv->as,0x000000);
	sprintf( buf,"Pt%4d.%c ", act->pnum,
		act->freedom.x==1.0?'x': 
		act->freedom.y==1.0?'y':'d' );
	if ( act->basedon==-1 )
	    strcat(buf,"Absolute At ");
	else if ( act->interp!=-1 )
	    /* Interpolated between */
	    sprintf(buf+strlen(buf), "<%3d:%-3d>To ", act->basedon, act->interp );
	else if ( act->basedon==act->pnum )
	    strcat(buf,"Shifted  By ");
	else
	    sprintf(buf+strlen(buf), "Base%4d By ", act->basedon );
	sprintf(buf+strlen(buf), "%.2f %c%c", act->distance/64.0,
	    act->rounded ? 'r' : ' ',
	    act->min ? 'm' : ' ');
	uc_strcpy(ubuf,buf);
	GDrawDrawText(pixmap,EDGE_SPACING,y,ubuf,-1,NULL,0x000000);
	++i;
	y+= cv->fh;
	act = act->acts;
    }
}
#endif

static void char_vscroll(CharView *cv,struct sbevent *sb) {
    int newpos = cv->yoff;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*cv->vheight/10;
      break;
      case et_sb_up:
        newpos -= cv->vheight/15;
      break;
      case et_sb_down:
        newpos += cv->vheight/15;
      break;
      case et_sb_downpage:
        newpos += 9*cv->vheight/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>8000*cv->scale-cv->vheight )
        newpos = 8000*cv->scale-cv->vheight;
    if ( newpos<-8000*cv->scale ) newpos = -8000*cv->scale;
    if ( newpos!=cv->yoff ) {
	int diff = newpos-cv->yoff;
	cv->yoff = newpos;
	GScrollBarSetPos(cv->vsb,newpos);
	GDrawScroll(cv->v,NULL,0,diff);
    }
}

static void char_hscroll(CharView *cv,struct sbevent *sb) {
    int newpos = cv->xoff;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos += 9*cv->vwidth/10;
      break;
      case et_sb_up:
        newpos += cv->vwidth/15;
      break;
      case et_sb_down:
        newpos -= cv->vwidth/15;
      break;
      case et_sb_downpage:
        newpos -= 9*cv->vwidth/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = -sb->pos;
      break;
    }
    if ( newpos>8000*cv->scale-cv->vwidth )
        newpos = 8000*cv->scale-cv->vwidth;
    if ( newpos<-8000*cv->scale ) newpos = -8000*cv->scale;
    if ( newpos!=cv->xoff ) {
	int diff = newpos-cv->xoff;
	cv->xoff = newpos;
	GScrollBarSetPos(cv->hsb,-newpos);
	GDrawScroll(cv->v,NULL,diff,0);
    }
}

#if TT_CONFIG_OPTION_BYTECODE_DEBUG
static void gloss_scroll(CharView *cv,struct sbevent *sb) {
    int newpos = cv->gvpos;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= cv->vheight/cv->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += cv->vheight/cv->fh;
      break;
      case et_sb_bottom:
        newpos = cv->instrinfo.act_cnt-cv->vheight/cv->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>cv->instrinfo.act_cnt-cv->vheight/cv->fh )
        newpos = cv->instrinfo.act_cnt-cv->vheight/cv->fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=cv->gvpos ) {
	GRect r;
	int diff = newpos-cv->gvpos;
	cv->gvpos = newpos;
	GScrollBarSetPos(cv->gvsb,cv->gvpos);
	r.x=0; r.y = EDGE_SPACING; r.width=cv->gvwidth; r.height=cv->gvheight-2*EDGE_SPACING;
	GDrawScroll(cv->glossv,&r,0,diff*cv->fh);
    }
}
#endif

static void CharViewFree(CharView *cv) {
    ConicChar *cc = cv->cc;
    CharView *prev, *test;

    if ( (prev = cc->views)==cv )
	cc->views = cv->next;
    else {
	for ( test=prev->next; test!=cv && test!=NULL; prev = test, test=test->next );
	prev->next = cv->next;
    }
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    ConicPointListsFree(cv->gridfit);
    FreeType_FreeRaster(cv->raster);
# if TT_CONFIG_OPTION_BYTECODE_DEBUG
    free(cv->twilight);
# endif
#endif
#if TT_CONFIG_OPTION_BYTECODE_DEBUG
    free(cv->cvtvals);
#endif
    free(cv);
}

static void CVChar(CharView *cv,GEvent *event) {
    int i;
    struct enctab *enc;

    if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 ) {
	if ( event->w==cv->v || event->w==cv->gw )
	    TableHelp(CHR('g','l','y','f'));
	else
	    system( "netscape http://fonts.apple.com/TTRefMan/RM05/Chap5.html &" );
#if MyMemory
    } else if ( event->u.chr.keysym==GK_F2 ) {
	printf( "Memory Check On\n" );
	__malloc_debug(5);
    } else if ( event->u.chr.keysym==GK_F3 ) {
	__malloc_debug(0);
	printf( "Memory Check Off\n" );
#endif
    } else if ( event->u.chr.chars[0]!='\0' && (enc = cv->cc->parent->tfont->enc)!=NULL ) {
	for ( i = 0; i<enc->cnt; ++i ) {
	    if ( enc->uenc[i]==event->u.chr.chars[0] ) {
		CVChangeGlyph(cv,i);
	break;
	    }
	}
    }
}

static int cv_v_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	char_expose(cv,gw,&event->u.expose.rect);
      break;
      case et_char:
	CVChar(cv,event);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
	if ( event->type==et_mousemove )
	    CVUpdateInfo(cv,event);
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int cv_iv_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    struct instrinfo *ii = &cv->instrinfo;

    switch ( event->type ) {
      case et_expose:
	instr_expose(ii,gw,&event->u.expose.rect);
      break;
      case et_char:
	CVChar(cv,event);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
	if ( event->type==et_mousemove )
	    instr_mousemove(ii,event->u.mouse.y);
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int cv_gv_e_h(GWindow gw, GEvent *event) {
#if TT_CONFIG_OPTION_BYTECODE_DEBUG
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    struct ttfactions *acts;
    int i, seek, y;
    ConicPoint *sp;
    BasePoint *bp, *mbp;
    struct instrinfo *ii;
    char buf[400];
    static unichar_t msg[400];
    double scale=0;	/* keeps compiler from complaining about uninit variable */

    switch ( event->type ) {
      case et_expose:
	char_glossexpose(cv,gw,&event->u.expose.rect);
      break;
      case et_mousemove: case et_mouseup:
	GGadgetEndPopup();
	ii = &cv->instrinfo;
	seek = (event->u.mouse.y-2)/cv->fh + cv->gvpos;
	for ( i=0, acts=ii->acts; acts!=NULL && i<seek; ++i )
	    acts = acts->acts;
	if ( acts!=NULL ) {
	    sprintf( buf,"Point %d.%s ", acts->pnum,
		    acts->freedom.x==1.0?"x": 
		    acts->freedom.y==1.0?"y":"diagonal" );
	    if ( acts->basedon==-1 )
		strcat(buf,"is positioned to ");
	    else if ( acts->interp!=-1 )
		/* Interpolated between */
		sprintf(buf+strlen(buf), "interpolated between %d and %d to ",
			acts->basedon, acts->interp );
	    else if ( acts->basedon==acts->pnum )
		strcat(buf,"Shifted  By ");
	    else
		sprintf(buf+strlen(buf), "offset from base point %d by ", acts->basedon );
	    sprintf(buf+strlen(buf), "%.2f", acts->distance/64.0 );
	    if ( acts->rounded || acts->min || acts->cvt_entry!=-1 ) {
		sprintf(buf+strlen(buf), "\n%s%s",
		    acts->rounded ? "rounded " : "",
		    acts->min ? "minimum distance ": "" );
		if ( acts->cvt_entry!=-1 && cv->cvtvals!=NULL ) {
		    int orig = (short) ptgetushort(cv->cvt->data+2*acts->cvt_entry);
		    int val = cv->cvtvals[acts->cvt_entry];
		    sprintf(buf+strlen(buf), " cvt entry=%d fword=%d pixel=%.2f",
			acts->cvt_entry, val,
				val*(double) cv->show.ppem/cv->cc->parent->em );
		    if ( orig!=val )
			sprintf(buf+strlen(buf), " (orig=%d)", orig );
		}
	    }
	    bp = CVGetTTFPoint(cv,acts->pnum,&mbp);
	    if ( bp!=NULL ) {
		scale = cv->show.ppem/(double) (cv->cc->parent->em);
		sprintf(buf+strlen(buf), "\nPoint originally at: (%g,%g) (%.2f,%.2f)",
			bp->x,bp->y, bp->x*scale, bp->y*scale);
	    }
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
	    sprintf(buf+strlen(buf), "\nFrom: (%.2f,%.2f)",
		    acts->was.x/64., acts->was.y/64.);
	    sprintf(buf+strlen(buf), "\nTo: (%.2f,%.2f)",
		    acts->is.x/64., acts->is.y/64.);
	    if ( bp!=NULL && mbp!=NULL ) {
		sprintf(buf+strlen(buf), "\nFinally at: (%g,%g) (%.2f,%.2f)",
			mbp->x,mbp->y, mbp->x*scale, mbp->y*scale);
	    }
#endif
	    sprintf( buf+strlen(buf), "\n%s", instrs[*acts->instr]);
	    if ( acts->infunc>=0 )
		sprintf( buf+strlen(buf), " in function %d", acts->infunc );
	    uc_strcpy(msg,buf);
	    GGadgetPreparePopup(GDrawGetParentWindow(ii->v),msg);
	}
      break;
      case et_mousedown:
	GGadgetEndPopup();
	ii = &cv->instrinfo;
	ii->gsel_pos = (event->u.mouse.y-2)/cv->fh + cv->gvpos;
	ii->isel_pos = -1;
	for ( i=0, acts=ii->acts; acts!=NULL && i<ii->gsel_pos; ++i )
	    acts = acts->acts;
	sp = NULL;
	ii->isel_pos = -1;
	if ( acts!=NULL ) {
	    sp = CVGetPoint(cv,acts->pnum);
	    if ( acts->instr >= ii->instrdata->instrs &&
		    acts->instr < ii->instrdata->instrs+ii->instrdata->instr_cnt ) {
		seek = acts->instr - ii->instrdata->instrs;
		y = 0;
		for ( i=0; i<seek && i<ii->instrdata->instr_cnt; ++i )
		    if ( ii->instrdata->bts[i]!=bt_wordlo )
			++y;
		ii->isel_pos = y;
		if ( ii->isel_pos<ii->lpos ||
			ii->isel_pos>=ii->lpos + cv->vheight/cv->fh ) {
		    ii->lpos = ii->isel_pos-cv->vheight/(3*cv->fh);
		    if ( ii->lpos >= ii->lheight-cv->vheight/cv->fh )
			ii->lpos = ii->lheight-cv->vheight/cv->fh;
		    if ( ii->lpos<0 ) ii->lpos = 0;
		    GScrollBarSetPos(ii->vsb,ii->lpos);
		}
	    }
	} else
	    ii->gsel_pos = -1;
	GDrawRequestExpose(cv->glossv,NULL,false);
	GDrawRequestExpose(cv->instrinfo.v,NULL,false);
	if ( CVClearSel(cv) || sp!=NULL ) {
	    if ( sp ) sp->selected = true;
	    GDrawRequestExpose(cv->v,NULL,false);
	}
      break;
    }
#endif
return( true );
}

static int cv_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	char_infoexpose(cv,gw,&event->u.expose.rect);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    char_resize(cv,event);
      break;
      case et_char:
	CVChar(cv,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == cv->vsb )
		char_vscroll(cv,&event->u.control.u.sb);
	    else if ( event->u.control.g == cv->hsb )
		char_hscroll(cv,&event->u.control.u.sb);
	    else if ( event->u.control.g == cv->instrinfo.vsb )
		instr_scroll(&cv->instrinfo,&event->u.control.u.sb);
#if TT_CONFIG_OPTION_BYTECODE_DEBUG
	    else if ( event->u.control.g == cv->gvsb )
		gloss_scroll(cv,&event->u.control.u.sb);
#endif
	  break;
	}
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
      break;
      case et_close:
	CVClose(cv);
      break;
      case et_destroy:
	CharViewFree(cv);
      break;
    }
return( true );
}

static void CVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    DelayEvent((void (*)(void *)) CVClose, cv);
}

static void CVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((CharView *) GDrawGetUserData(gw))->cc->parent->fv->owner;

    _TFVMenuSaveAs(tfv);
}

static void CVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((CharView *) GDrawGetUserData(gw))->cc->parent->fv->owner;
    _TFVMenuSave(tfv);
}

static void CVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((CharView *) GDrawGetUserData(gw))->cc->parent->fv->owner;
    DelayEvent((void (*)(void *)) _TFVMenuRevert, tfv);
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static void CVMenuScale(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( mi->mid == MID_Fit ) {
	CVFit(cv);
    } else {
	real midx = (cv->vwidth/2-cv->xoff)/cv->scale;
	real midy = (cv->vheight/2-cv->yoff)/cv->scale;
	CVMagnify(cv,midx,midy,mi->mid==MID_ZoomOut?-1:1);
    }
}

static void CVMenuChangeGlyph(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( mi->mid==MID_Next )
	CVChangeGlyph(cv,cv->cc->glyph+1);
    else if ( mi->mid==MID_Prev )
	CVChangeGlyph(cv,cv->cc->glyph-1);
}

static void CVMenuGotoGlyph(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GDrawIError("NYI");	/* !!!!! */
}

static void CVMenuShowHide(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    GEvent dummy;

    if ( mi->mid==MID_HideInstrs || mi->mid==MID_HideGloss ) {
	if ( mi->mid==MID_HideInstrs ) {
	    cv->show.instrpane = charshows.instrpane = !cv->show.instrpane;
	    GDrawSetVisible(cv->instrinfo.v,cv->show.instrpane);
	    GGadgetSetVisible(cv->instrinfo.vsb,cv->show.instrpane);
	} else {
	    cv->show.glosspane = charshows.glosspane = !cv->show.glosspane;
	    GDrawSetVisible(cv->glossv,cv->show.glosspane);
	    GGadgetSetVisible(cv->gvsb,cv->show.glosspane);
	}
	memset(&dummy,'\0',sizeof(dummy));
	dummy.type = et_resize;
	dummy.w = cv->gw;
	GDrawGetSize(cv->gw,&dummy.u.resize.size);
	dummy.u.resize.sized = true;
	GDrawPostEvent(&dummy);
    } else {
	switch ( mi->mid ) {
	  case MID_HideSplines:
	    cv->show.fore = charshows.fore = !cv->show.fore;
	  break;
	  case MID_HideGrid:
	    cv->show.grid = charshows.grid = !cv->show.grid;
	  break;
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
	  case MID_HideGridFit:
	    cv->show.gridspline = charshows.gridspline = !cv->show.gridspline;
	  break;
	  case MID_HideRaster:
	    cv->show.raster = charshows.raster = !cv->show.raster;
	  break;
# if TT_CONFIG_OPTION_BYTECODE_DEBUG
	  case MID_HideTwilight:
	    cv->show.twilight = charshows.twilight = !cv->show.twilight;
	  break;
# endif
#endif
	}
	GDrawRequestExpose(cv->v,NULL,false);
    }
}

static void CVMenuGridSize(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    char buf[20];
    unichar_t ubuf[20], *ret, *end;
    int val;

    sprintf(buf,"%d", cv->show.ppem );
    uc_strcpy(ubuf,buf);
    ret = GWidgetAskStringR(_STR_GridSize, ubuf,_STR_GridSize);
    if ( ret==NULL )
return;
    val = u_strtol(ret,&end,10);
    free(ret);
    if ( *end || val<=1 ) {
	ProtestR(_STR_GridSize);
return;
    }
    if ( val == cv->show.ppem )
return;
    cv->show.ppem = val;
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    ConicPointListsFree(cv->gridfit); cv->gridfit = NULL;
    FreeType_FreeRaster(cv->raster); cv->raster = NULL;
    FreeType_GridFitChar(cv);
#endif
    GDrawRequestExpose(cv->v,NULL,false);
}

static void vwlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_HideSplines:
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(cv->show.fore?_STR_HideFore: _STR_ShowFore,NULL));
	  break;
	  case MID_HideInstrs:
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(cv->show.instrpane?_STR_HideInstrs: _STR_ShowInstrs,NULL));
	  break;
	  case MID_HideGloss:
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(cv->show.glosspane?_STR_HideGloss: _STR_ShowGloss,NULL));
	  break;
	  case MID_HideGrid:
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(cv->show.grid?_STR_HideGrid: _STR_ShowGrid,NULL));
	  break;
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
	  case MID_HideGridFit:
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(cv->show.gridspline?_STR_HideGridFit: _STR_ShowGridFit,NULL));
	  break;
	  case MID_HideRaster:
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(cv->show.raster?_STR_HideRaster: _STR_ShowRaster,NULL));
	  break;
# if TT_CONFIG_OPTION_BYTECODE_DEBUG
	  case MID_HideTwilight:
	    free(mi->ti.text);
	    mi->ti.text = u_copy(GStringGetResource(cv->show.twilight?_STR_HideTwilight: _STR_ShowTwilight,NULL));
	  break;
# endif
#endif
	}
    }
}

static GMenuItem dummyitem[] = { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) _STR_Open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) _STR_Close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, CVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, CVMenuSave },
    { { (unichar_t *) _STR_SaveAs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, CVMenuSaveAs },
    { { (unichar_t *) _STR_Revertfile, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, CVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
/*    { { (unichar_t *) _STR_Prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },*/
/*    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},*/
    { { (unichar_t *) _STR_Quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { (unichar_t *) _STR_Undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL },
    { { (unichar_t *) _STR_Redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, NULL, MID_Cut },
    { { (unichar_t *) _STR_Copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, NULL, MID_Copy },
    { { (unichar_t *) _STR_Paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, NULL, MID_Paste },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_SelectAll, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, NULL },
    { NULL }
};

static GMenuItem vwlist[] = {
    { { (unichar_t *) _STR_Fit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 'F', ksm_control, NULL, NULL, CVMenuScale, MID_Fit },
    { { (unichar_t *) _STR_Zoomout, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, CVMenuScale, MID_ZoomOut },
    { { (unichar_t *) _STR_Zoomin, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'i' }, '\0', ksm_control, NULL, NULL, CVMenuScale, MID_ZoomIn },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_NextChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, ']', ksm_control, NULL, NULL, CVMenuChangeGlyph, MID_Next },
    { { (unichar_t *) _STR_PrevChar, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, '[', ksm_control, NULL, NULL, CVMenuChangeGlyph, MID_Prev },
    { { (unichar_t *) _STR_Goto, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'G' }, '>', ksm_shift|ksm_control, NULL, NULL, CVMenuGotoGlyph },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_ShowFore, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuShowHide, MID_HideSplines },
    { { (unichar_t *) _STR_ShowInstrs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuShowHide, MID_HideInstrs },
#if TT_CONFIG_OPTION_BYTECODE_DEBUG
    { { (unichar_t *) _STR_ShowGloss, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuShowHide, MID_HideGloss },
#endif
    { { (unichar_t *) _STR_ShowGrid, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuShowHide, MID_HideGrid },
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    { { (unichar_t *) _STR_ShowGridFit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuShowHide, MID_HideGridFit },
    { { (unichar_t *) _STR_ShowRaster, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuShowHide, MID_HideRaster },
# if TT_CONFIG_OPTION_BYTECODE_DEBUG
    { { (unichar_t *) _STR_ShowTwilight, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuShowHide, MID_HideTwilight },
# endif
#endif
    { { (unichar_t *) _STR_GridSize, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, '\0' }, '\0', ksm_control, NULL, NULL, CVMenuGridSize },
    { NULL }
};

extern GMenuItem helplist[];

static GMenuItem mblist[] = {
    { { (unichar_t *) _STR_File, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) _STR_Edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'E' }, 0, 0, edlist },
    { { (unichar_t *) _STR_View, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'V' }, 0, 0, vwlist, vwlistcheck },
    { { (unichar_t *) _STR_Window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 0, 0, NULL, WindowMenuBuild, NULL },
    { { (unichar_t *) _STR_Help, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};


/* single glyphs */
void charCreateEditor(ConicFont *cf,int glyph) {
    CharView *cv = gcalloc(1,sizeof(CharView));
    unichar_t title[100];
    GRect pos, gsize;
    GWindow gw;
    GWindowAttrs wattrs;
    FontRequest rq;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    static unichar_t num[] = { '0',  '\0' };
    int as,ds,ld;
    GGadgetData gd;
    char buf[100];
    int x;

    instrhelpsetup();

    cv->cc = LoadGlyph(cf,glyph);
    if ( cv->cc==NULL ) {
	GWidgetErrorR(_STR_CouldntReadGlyph,_STR_CouldntReadGlyphd,glyph);
	free(cv);
return;
    }
    cv->next = cv->cc->views;
    cv->cc->views = cv;
    cv->show = charshows;

    sprintf(buf,"Glyph: %d ", glyph);
    uc_strcpy(title, buf);
    if ( cf->tfont->enc!=NULL && glyph<cf->tfont->enc->cnt && psunicodenames[cf->tfont->enc->uenc[glyph]]!=NULL )
	uc_strncat(title,psunicodenames[cf->tfont->enc->uenc[glyph]],sizeof(buf)-strlen(buf)-1);
    title[sizeof(title)/sizeof(title[0])-1] = '\0';

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,100);
    cv->gw = gw = GDrawCreateTopWindow(NULL,&pos,cv_e_h,cv,&wattrs);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    cv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(cv->mb,&gsize);
    cv->mbh = gsize.height;

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -12;
    rq.weight = 400;
    cv->instrinfo.gfont = cv->gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(cv->gw,cv->gfont);
    GDrawFontMetrics(cv->gfont,&as,&ds,&ld);
    cv->numlen = GDrawGetTextWidth(cv->gw,num,1,NULL);
    cv->instrinfo.as = cv->as = as+1;
    cv->instrinfo.fh = cv->fh = cv->as+ds;
    cv->infoh = cv->fh + 2;
    rq.point_size = -10;
    cv->sfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(cv->sfont,&as,&ds,&ld);
    cv->sas = as+1;
    cv->sfh = cv->sas+ds;

    gd.pos.y = cv->mbh+cv->infoh; gd.pos.height = pos.height-gd.pos.y;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = INSTR_WIDTH;
    gd.flags = gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    if ( cv->show.instrpane )
	gd.flags |= gg_visible;
    cv->instrinfo.vsb = GScrollBarCreate(gw,&gd,cv);
    GGadgetGetSize(cv->instrinfo.vsb,&gsize);
    cv->sbw = cv->instrinfo.sbw = gsize.width;

    wattrs.mask = wam_events|wam_cursor;
    pos.x = 0; pos.y = gd.pos.y;
    pos.width = gd.pos.x; pos.height = gd.pos.height;
    cv->instrinfo.v = GWidgetCreateSubWindow(gw,&pos,cv_iv_e_h,cv,&wattrs);
    GDrawSetVisible(cv->instrinfo.v,cv->show.instrpane);
    x = (cv->show.instrpane? INSTR_WIDTH+cv->sbw : 0);

    gd.pos.x = x+GLOSS_WIDTH;
    gd.flags = gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    if ( cv->show.glosspane )
	gd.flags |= gg_visible;
    cv->gvsb = GScrollBarCreate(gw,&gd,cv);

    pos.x = x; pos.width = GLOSS_WIDTH;
    cv->glossv = GWidgetCreateSubWindow(gw,&pos,cv_gv_e_h,cv,&wattrs);
    GDrawSetVisible(cv->glossv,cv->show.glosspane);
    x += (cv->show.glosspane? GLOSS_WIDTH+cv->sbw : 0);

    gd.pos.x = x+CHAR_WIDTH;
    gd.pos.height -= cv->sbw;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    cv->vsb = GScrollBarCreate(gw,&gd,cv);

    gd.pos.x = x; gd.pos.width = CHAR_WIDTH;
    gd.pos.y = gd.pos.height; gd.pos.height = cv->sbw;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;	/* always visible */
    cv->hsb = GScrollBarCreate(gw,&gd,cv);

    pos.x = x; pos.width = CHAR_WIDTH;
    pos.height -= cv->sbw;
    cv->v = GWidgetCreateSubWindow(gw,&pos,cv_v_e_h,cv,&wattrs);
    GDrawSetVisible(cv->v,true);
    x += CHAR_WIDTH+cv->sbw;
    GDrawResize(cv->gw,x,cv->mbh+cv->infoh+CHAR_WIDTH+cv->sbw);

    cv->instrinfo.instrdata = &cv->cc->instrdata;
    cv->instrinfo.isel_pos = -1;
    instr_typify(&cv->instrinfo);

#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    FreeType_GridFitChar(cv);
#endif
#if TT_CONFIG_OPTION_BYTECODE_DEBUG
    cv->instrinfo.gsel_pos = -1;
    CVGenerateGloss(cv);
    cv->cvt = TableFind(cv->cc->parent->tfont,CHR('c','v','t',' '));
    TableFillup(cv->cvt);
#endif

    GDrawSetVisible(gw,true);
}
